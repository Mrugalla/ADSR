#pragma once

#include "Filter.h"
#include "PRM.h"
#include "Phasor.h"
#include "WaveTable.h"
#include "XenManager.h"
#include "WHead.h"

namespace audio
{
	struct Manta
	{
		// enabled, pitch-snap, cutoff, resonance, slope, feedback, oct, semi, heat, rm-oct, rm-semi, rm-depth, gain
		static constexpr int NumParametersPerLane = 13;
		static constexpr int WaveTableSize = 1 << 13; // around min 5hz
		static constexpr int NumLanes = 3;
		static constexpr int MaxSlopeStage = 4; //4*12db/oct
		using WT = WaveTable<WaveTableSize>;
	private:
		class Filter
		{
			using Fltr = FilterBandpassSlope<MaxSlopeStage>;
		public:
			Filter();

			/* laneBuf, samples, numChannels, numSamples, fcBuf, resoBuf, stage */
			void operator()(float* const*, float* const*, int, int,
				float*, float*, int) noexcept;

		protected:
			std::array<Fltr, 2> filta;
		};
		
		struct DelayFeedback
		{
			DelayFeedback();

			/* delaySize */
			void prepare(int);

			/* samples, numChannels, numSamples, wHead, rHead, feedback */
			void operator()(float* const*, int, int, const int*, const float*, const float*) noexcept;

			AudioBuffer ringBuffer;
			int size;
		};
		
		struct RingMod
		{
			using WTFunc = WT::Func;

			RingMod();

			void createWavetable(const WTFunc&) noexcept;

			/* Fs, blockSize */
			void prepare(float, int);

			/* samples, numChannels, numSamples, rmDepth, freqHz */
			void operator()(float* const*, int, int, float*, float*) noexcept;

			WT waveTable;
		protected:
			Phasor<float> phasor;
			std::vector<float> oscBuffer;
		};

		struct Lane
		{
			Lane();

			/* sampleRate, blockSize, delaySize */
			void prepare(float, int, int);

			/* samples, numChannels, numSamples, enabled, pitch, resonance, slope, drive, feedback,
			oct, semi, rmOct, rmSemi, rmDepth, gain, wHead, xen */
			void operator()(float* const*, int, int, bool, float, float, int, float, float,
				float, float, float, float, float, float, const int*, const XenManager&) noexcept;

			/* state, laneIndex */
			void savePatch(sta::State&, int);

			/* state, laneIndex */
			void loadPatch(sta::State&, int);

			/* samples, numChannels, numSamples */
			void addTo(float* const*, int, int) noexcept;

			RingMod ringMod;
		protected:
			AudioBuffer laneBuffer;
			std::vector<float> readHead;
			Filter filter;
			PRM frequency, resonance, drive, feedback, delayRate, rmDepth, rmFreqHz, gain;
			DelayFeedback delayFB;
			float Fs, delaySizeF;

			/* numSamples, wHead, delayBuf */
			const float* getRHead(int, const int*, const float*) noexcept;

			/* x, d */
			float distort(float, float) const noexcept;

			/* samples, numChannels, numSamples, driveBuf */
			void distort(float* const*, int, int, const float*) noexcept;

			/* samples, numChannels, numSamples, gainBuf */
			void applyGain(float* const*, int, int, const float*) noexcept;
		};

	public:
		Manta(const XenManager&);

		/* sampleRate, blockSize */
		void prepare(float, int);

		/* samples, numChannels, numSamples,
		* l1Enabled [0, 1], l1Snap, l1Pitch [12, N]note, l1Resonance [1, N]q, l1Slope [1, 4]db/oct, l1Drive [0, 1]%, l1Feedback [0, 1]%, l1Oct[-3,3], l1Semi[-12,12], l1RMOct[-3,3], l1RMSemi[-12,12], l1RMDepth[0,1], l1Gain [-60, 60]db
		* l2Enabled [0, 1], l2Snap, l2Pitch [12, N]note, l2Resonance [1, N]q, l2Slope [1, 4]db/oct, l2Drive [0, 1]%, l2Feedback [0, 1]%, l2Oct[-3,3], l2Semi[-12,12], l2RMOct[-3,3], l2RMSemi[-12,12], l2RMDepth[0,1], l2Gain [-60, 60]db
		* l3Enabled [0, 1], l3Snap, l3Pitch [12, N]note, l3Resonance [1, N]q, l3Slope [1, 4]db/oct, l3Drive [0, 1]%, l3Feedback [0, 1]%, l3Oct[-3,3], l3Semi[-12,12], l3RMOct[-3,3], l3RMSemi[-12,12], l3RMDepth[0,1], l3Gain [-60, 60]db
		*/
		void operator()(float* const*, int, int,
			bool, bool, float, float, int, float, float, float, float, float, float, float, float,
			bool, bool, float, float, int, float, float, float, float, float, float, float, float,
			bool, bool, float, float, int, float, float, float, float, float, float, float, float) noexcept;
		
		void savePatch(sta::State&);

		void loadPatch(sta::State&);

		/* laneIdx */
		WT& getWaveTable(int) noexcept;

		/* laneIdx */
		const WT& getWaveTable(int) const noexcept;
		
	protected:
		const XenManager& xen;
		std::array<Lane, NumLanes> lanes;
		WHead writeHead;
	public:
		int delaySize;
	};
}

/*

*/