#include "AutoGain.h"
#include "AudioUtils.h"
#include <random>

namespace audio
{
	using MersenneTwister = std::mt19937;
	using RandDistribution = std::uniform_real_distribution<float>;
	
	// PinkNoise
	
	PinkNoise::PinkNoise(float targetDb) :
		noise()
	{
		synthesizeWhiteNoise();
		pinkenNoise();

		const auto targetRms = Decibels::decibelsToGain(targetDb);
		const auto gain = targetRms / rms();

		for (auto& n : noise)
			n *= gain;
	}

	float PinkNoise::rms() noexcept
	{
		return getRMS(noise.data(), Size);
	}

	float* PinkNoise::data() noexcept
	{
		return noise.data();
	}

	const float* PinkNoise::data() const noexcept
	{
		return noise.data();
	}

	void PinkNoise::synthesizeWhiteNoise() noexcept
	{
		{ // PROCEDURAL WHITE NOISE
			MersenneTwister mt(420);
			RandDistribution dist(-1.f, 1.f);

			for (auto n = 0; n < Size; ++n)
				noise[n] = dist(mt);
		}
	}

	void PinkNoise::pinkenNoise() noexcept
	{
		{ // WHITE >> PINK NOISE
			std::array<float, 7> b;
			for (auto& a : b)
				a = 0.f;
			for (auto n = 0; n < Size; ++n)
			{
				const auto white = noise[n];
				b[0] = 0.99886f * b[0] + white * 0.0555179f;
				b[1] = 0.99332f * b[1] + white * 0.0750759f;
				b[2] = 0.96900f * b[2] + white * 0.1538520f;
				b[3] = 0.86650f * b[3] + white * 0.3104856f;
				b[4] = 0.55000f * b[4] + white * 0.5329522f;
				b[5] = -0.7616f * b[5] - white * 0.0168980f;
				auto pink = white * 0.5362f;
				for (auto a : b)
					pink += a;
				b[6] = white * 0.115926f;
				noise[n] = pink;
			}
		}
	}

	// AutoGain

	AutoGain::AutoGain(PinkNoise& _noise, const Range& _range, int _numGainSteps) :
		noise(_noise),
		range(_range),
		gain(),
		numGainStepsF(static_cast<float>(_numGainSteps)),
		numGainSteps(_numGainSteps),
		evaluating(true)
	{
		gain.resize(numGainSteps + 2, 1.f);
	}

	void AutoGain::evaluate(const OnPrepare& onPrepare,
		const OnProcess& onProcess, const OnClear& onClear)
	{
		onPrepare(44100.f, noise.Size);
		const auto noiseRMS = noise.rms();
		AudioBuffer buf(1, noise.Size);
		for (auto i = 0; i < gain.size(); ++i)
		{
			auto samples = buf.getArrayOfWritePointers();
			SIMD::copy(samples[0], noise.data(), noise.Size);

			auto x = static_cast<float>(i) / numGainStepsF;
			x = x > 1.f ? 1.f : x;
			const auto pVal = range.convertFrom0to1(x);

			onProcess(samples, 1, noise.Size, pVal);

			const auto samplesRead = samples[0];
			const auto nRMS = getRMS(samplesRead, noise.Size);
			gain[i] = (nRMS == 0.f || std::isnan(nRMS) || std::isinf(nRMS)) ? 0.f : noiseRMS / nRMS;
		}
		onClear();
		evaluating = false;
	}

	float AutoGain::fromDenorm(float smpl, float valPDenorm) const noexcept
	{
		if (evaluating)
			return smpl;

		return processSample(smpl, range.convertTo0to1(valPDenorm));
	}

	float AutoGain::operator()(float smpl, float valP) const noexcept
	{
		if (evaluating)
			return smpl;

		return processSample(smpl, valP);
	}

	float AutoGain::processSample(float smpl, float valP) const noexcept
	{
		const auto mcr = valP;
		const auto x = mcr * numGainStepsF;
		const auto xFloor = std::floor(x);
		const auto iF = static_cast<int>(xFloor);
		const auto iC = iF + 1;
		const auto xFrac = x - xFloor;
		const auto gF = gain[iF];
		const auto gC = gain[iC];
		const auto gRange = gC - gF;
		const auto g = gF + xFrac * gRange;
		smpl *= g;

		return smpl;
	}
}