#pragma once
#include <array>
#include "PRM.h"
#include "AutoGain.h"

namespace audio
{
	struct OverdriveReNeo
	{
		enum PanVec
		{
			Gain,
			DriveL,
			DriveR,
			DriveC,
			NumPanVecs
		};

		using Range = makeRange::Range;

		struct Filter
		{
			Filter();

			void reset() noexcept;

			/* hz, Fs */
			void setFreq(float, float) noexcept;

			float operator()(float) noexcept;

		protected:
			std::array<smooth::Lowpass<float>, 2> filtr;
		};

		static constexpr float DriveBoost = 420.f;

		static constexpr float GlueBoost = 8.f;
		static constexpr float GlueBoostInv = 1.f / GlueBoost;
		
		/* pinkNoise, muffleRange */
		OverdriveReNeo(PinkNoise&, const Range&);

		/* sampleRate, blockSize */
		void prepare(float, int);

		/* samples, numChannels, numSamples, drive[0,1], muffle[20,20k], pan[-1,1], scrap[0,1] */
		void operator()(float* const*, int, int, float, float, float, float) noexcept;

	private:
		std::array<Filter, 2> filtr;
		PRM muffled, pan, drive, scrap;
		AutoGain muffleGain, driveGain, scrapGain;

		std::array<std::vector<float>, NumPanVecs> panVecs;

		float Fs;

		void resetFilter() noexcept;

		void initAutoGain();

		/* smpls, numSamples, muffleBuf, driveBuf, fltr, scrapBuf */
		void processBlockMono(float*, int, const float*, const float*, Filter&, const float*) noexcept;

		/* samples, numChannels, numSamples, muffleBuf, driveBuf, pan, scrapBuf */
		void processBlockStereo(float* const*, int, int, const float*, const float*, float, const float*) noexcept;

		/* x, p, xy */
		float waveshape(float, float, float) const noexcept;

		/* x, ms, ds, scp */
		float applyAutoGain(float, float, float, float) noexcept;

		/* pan, numSamples */
		void updatePanVecs(float, int) noexcept;
	};
}

/*

todo:

autogain not perfect

add pitch correction?

steep highpass for remove dc offset?

pan knob greyed out on mono tracks

*/