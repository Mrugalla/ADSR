#pragma once
#include "../arch/State.h"
#include <array>
#include <atomic>

namespace audio
{
	struct XenManager
	{
		XenManager();

		/* tmprVal, noteVal */
		void setTemperament(float, int) noexcept;
		
		/* xen, masterTune, baseNote */
		void operator()(float, float, float) noexcept;

		template<typename Float>
		Float noteToFreqHz(Float) const noexcept;

		/* note, lowestFreq, highestFreq */
		template<typename Float>
		Float noteToFreqHzWithWrap(Float, Float = static_cast<Float>(0), Float = static_cast<Float>(22000)) const noexcept;

		template<typename Float>
		Float freqHzToNote(Float) noexcept;
		
		float getXen() const noexcept;

	protected:
		float xen, masterTune, baseNote;
		std::array<std::atomic<float>, PPD_MaxXen + 1> temperaments;
	};
	
}

#include "Oscillator.h"

namespace audio
{
	struct TuningEditorSynth
	{
		using SIMD = juce::FloatVectorOperations;

		TuningEditorSynth(const XenManager& _xen);

		void loadPatch(sta::State&);

		void savePatch(sta::State&);

		/* Fs, blockSize */
		void prepare(float, int);

		/* samples, numChannels, numSamples */
		void operator()(float**, int, int) noexcept;

		std::atomic<float> pitch, gain;
		std::atomic<bool> noteOn;
	protected:
		const XenManager& xen;
		OscSine<float> osc;
		std::vector<float> buffer;
	};
}