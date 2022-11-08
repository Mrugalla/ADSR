#include "XenManager.h"
#include "../arch/Interpolation.h"
#include "../arch/Conversion.h"

namespace audio
{
	// XenManager

	XenManager::XenManager() :
		xen(12.f),
		masterTune(440.f),
		baseNote(69.f),
		temperaments()
	{
		for (auto& t : temperaments)
			t = 0.f;
	}

	void XenManager::setTemperament(float tmprVal, int noteVal) noexcept
	{
		temperaments[noteVal] = tmprVal;
		const auto idx2 = noteVal + PPD_MaxXen;
		if (idx2 >= temperaments.size())
			temperaments[idx2] = tmprVal;
	}

	void XenManager::operator()(float _xen, float _masterTune, float _baseNote) noexcept
	{
		xen = _xen;
		masterTune = _masterTune;
		baseNote = _baseNote;
	}

	template<typename Float>
	Float XenManager::noteToFreqHz(Float note) const noexcept
	{
		const auto noteCap = juce::jlimit(static_cast<Float>(0), static_cast<Float>(PPD_MaxXen), note);
		const auto tmprmt = static_cast<Float>(temperaments[static_cast<int>(std::round(noteCap))].load());

		return noteInFreqHz(note + tmprmt, static_cast<Float>(baseNote), static_cast<Float>(xen), static_cast<Float>(masterTune));
	}

	template<typename Float>
	Float XenManager::noteToFreqHzWithWrap(Float note, Float lowestFreq, Float highestFreq) const noexcept
	{
		auto freq = noteToFreqHz(note);
		while (freq < lowestFreq)
			freq *= static_cast<Float>(2);
		while (freq >= highestFreq)
			freq *= static_cast<Float>(.5);
		return freq;
	}

	template<typename Float>
	Float XenManager::freqHzToNote(Float hz) noexcept
	{
		return freqHzInNote(hz, static_cast<Float>(baseNote), static_cast<Float>(xen), static_cast<Float>(masterTune));
	}

	float XenManager::getXen() const noexcept
	{
		return xen;
	}

	template float XenManager::noteToFreqHz<float>(float note) const noexcept;
	template double XenManager::noteToFreqHz<double>(double note) const noexcept;

	template float XenManager::noteToFreqHzWithWrap<float>(float note, float lowestFreq, float highestFreq) const noexcept;
	template double XenManager::noteToFreqHzWithWrap<double>(double note, double lowestFreq, double highestFreq) const noexcept;

	template float XenManager::freqHzToNote<float>(float hz) noexcept;
	template double XenManager::freqHzToNote<double>(double hz) noexcept;
	
	// TuningEditorSynth

	TuningEditorSynth::TuningEditorSynth(const XenManager& _xen) :
		pitch(69.f),
		gain(.25f),
		noteOn(false),

		xen(_xen),
		osc(),
		buffer()
	{

	}

	void TuningEditorSynth::loadPatch(sta::State& state)
	{
		const auto idStr = "tuningEditor";
		auto g = state.get(idStr, "gain");
		if (g != nullptr)
			gain.store(static_cast<float>(*g));
	}

	void TuningEditorSynth::savePatch(sta::State& state)
	{
		const auto idStr = "tuningEditor";
		state.set(idStr, "gain", gain.load());
	}

	void TuningEditorSynth::prepare(float Fs, int blockSize)
	{
		const auto fsInv = 1.f / Fs;
		osc.prepare(fsInv);

		buffer.resize(blockSize, 0.f);
	}

	void TuningEditorSynth::operator()(float** samples, int numChannels, int numSamples) noexcept
	{
		if (noteOn.load())
		{
			auto buf = buffer.data();

			auto g = gain.load();

			const auto freqHz = xen.noteToFreqHzWithWrap(pitch.load());
			osc.setFreqHz(freqHz);

			for (auto s = 0; s < numSamples; ++s)
				buf[s] = std::tanh(4.f * osc()) * g;

			for (auto ch = 0; ch < numChannels; ++ch)
				SIMD::add(samples[ch], buf, numSamples);
		}
	}
}