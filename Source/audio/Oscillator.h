#pragma once
#include "../arch/Smooth.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include "Phasor.h"
#include <array>

namespace audio
{
	using AudioBuffer = juce::AudioBuffer<float>;
	
	template<typename Float>
	struct OscSine
	{
		OscSine();
		
		/* fsInv */
		void prepare(Float);

		void setFreqHz(Float);
		
		void reset(Float = static_cast<Float>(0));

		/* buffer, numSamples */
		Float* operator()(Float*, int) noexcept;

		Float operator()() noexcept;

		Phasor<Float> phasor;
	protected:
		Float synthesizeSample();
	};
	
	template<typename Float>
	struct RingModSimple
	{
		using Osc = OscSine<Float>;
		using Smooth = smooth::Smooth<Float>;
		
		RingModSimple();

		/* Fs, blockSize */
		void prepare(Float, int);
		
		/* samples, numChannels, numSamples, freq */
		void operator()(Float**, int, int, Float**) noexcept;

	protected:
		std::array<Osc, 2> osc;
		juce::AudioBuffer<Float> freqBuffer;
		std::array<Smooth, 2> freqSmooth;
	};
}