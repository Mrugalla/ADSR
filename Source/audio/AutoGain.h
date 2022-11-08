#pragma once
#include <array>
#include <functional>
#include "../arch/Range.h"

namespace audio
{
	struct PinkNoise
	{
		static constexpr int Size = 1 << 11;
		
		/* targetDb */
		PinkNoise(float = -24.f);

		float rms() noexcept;

		float* data() noexcept;

		const float* data() const noexcept;

	protected:
		std::array<float, Size> noise;

		void synthesizeWhiteNoise() noexcept;

		void pinkenNoise() noexcept;
	};

	struct AutoGain
	{
		/* sampleRate, blockSize */
		using OnPrepare = std::function<void(float, int)>;
		/* samples, numChannels, numSamples, valP */
		using OnProcess = std::function<void(float**, int, int, float)>;
		using OnClear = std::function<void()>;
		using Range = makeRange::Range;

		/* noise, range, numGainSteps */
		AutoGain(PinkNoise&, const Range & = { 0.f, 1.f }, int = 5);

		void evaluate(const OnPrepare&,
			const OnProcess&, const OnClear & = []() {});

		/* smpl, valPDenorm */
		float fromDenorm(float, float) const noexcept;

		/* smpl, valP */
		float operator()(float, float) const noexcept;

	protected:
		PinkNoise& noise;
		Range range;
		std::vector<float> gain;
		float numGainStepsF;
		int numGainSteps;
		bool evaluating;

		/* smpl, valP */
		float processSample(float, float) const noexcept;
	};

}