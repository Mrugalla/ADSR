#include "Oscillator.h"
#include "../arch/Conversion.h"

namespace audio
{
	// OscSine

	template<typename Float>
	OscSine<Float>::OscSine() :
		phasor()
	{}

	template<typename Float>
	void OscSine<Float>::prepare(Float fsInv)
	{
		phasor.prepare(fsInv);
	}

	template<typename Float>
	void OscSine<Float>::setFreqHz(Float hz)
	{
		phasor.setFrequencyHz(hz);
	}

	template<typename Float>
	void OscSine<Float>::reset(Float phase)
	{
		phasor.reset(phase);
	}

	template<typename Float>
	Float* OscSine<Float>::operator()(Float* buffer, int numSamples) noexcept
	{
		for (auto s = 0; s < numSamples; ++s)
			buffer[s] = synthesizeSample();
		return buffer;
	}

	template<typename Float>
	Float OscSine<Float>::operator()() noexcept
	{
		return synthesizeSample();
	}

	template<typename Float>
	Float OscSine<Float>::synthesizeSample()
	{
		const auto phase = phasor().phase;
		return std::cos(phase * static_cast<Float>(Tau));
	}

	template struct OscSine<float>;
	template struct OscSine<double>;
	
	// RingModSimple

	template<typename Float>
	RingModSimple<Float>::RingModSimple() :
		osc(),
		freqBuffer(),
		freqSmooth{ 20.f, 20.f }
	{}

	template<typename Float>
	void RingModSimple<Float>::prepare(Float Fs, int blockSize)
	{
		const auto fsInv = static_cast<Float>(1) / Fs;

		for (auto& osci : osc)
			osci.prepare(fsInv);
		freqBuffer.setSize(2, blockSize, false, false, false);
		for (auto& f : freqSmooth)
			f.makeFromDecayInMs(static_cast<Float>(20), Fs);
	}

	template<typename Float>
	void RingModSimple<Float>::operator()(Float** samples, int numChannels, int numSamples,
		Float** _freq) noexcept
	{
		auto freqBufs = freqBuffer.getArrayOfWritePointers();

		for (auto ch = 0; ch < numChannels; ++ch)
		{
			auto freq = _freq[ch];

			auto smpls = samples[ch];
			auto freqBuf = freqBufs[ch];
			auto& osci = osc[ch];

			freqSmooth[ch](freqBuf, freq, numSamples);

			for (auto s = 0; s < numSamples; ++s)
			{
				const auto smpl = smpls[s];
				osci.setFreqHz(freqBuf[s]);
				const auto mod = osci();

				smpls[s] = smpl * mod;
			}
		}
	}

	template struct RingModSimple<float>;
	template struct RingModSimple<double>;
}