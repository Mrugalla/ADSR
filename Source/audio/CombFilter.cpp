#include "CombFilter.h"
#include "../arch/Interpolation.h"

namespace audio
{
	// CombFilter::DelayFeedback
	
	CombFilter::DelayFeedback::DelayFeedback() :
		ringBuffer(),
		lowpass{ 0.f, 0.f },
		size(0)
	{}

	void CombFilter::DelayFeedback::prepare(float Fs, int _size)
	{
		size = _size;

		ringBuffer.setSize(2, size, false, true, false);

		for (auto& lp : lowpass)
			lp.makeFromDecayInHz(1000.f, Fs);
	}

	void CombFilter::DelayFeedback::operator()(float* const* samples, int numChannels, int numSamples,
		const int* wHead, const float* fbBuf, const float* dampBuf,
		const float* const* readHead) noexcept
	{
		auto ringBuf = ringBuffer.getArrayOfWritePointers();

		for (auto ch = 0; ch < numChannels; ++ch)
		{
			auto smpls = samples[ch];
			auto ring = ringBuf[ch];
			const auto rHead = readHead[ch];
			auto& lp = lowpass[ch];

			for (auto s = 0; s < numSamples; ++s)
			{
				lp.setX(dampBuf[s]);

				const auto w = wHead[s];
				const auto r = rHead[s];
				const auto fb = fbBuf[s];

				const auto sOut = lp(interpolate::cubicHermiteSpline(ring, r, size)) * fb + smpls[s];
				const auto sIn = sOut;

				ring[w] = sIn;
				smpls[s] = sOut;
			}
		}
	}

	// CombFilter

	CombFilter::CombFilter(MIDIVoices& _midiVoices, const XenManager& _xenManager) :
		midiVoices(_midiVoices),
		xenManager(_xenManager),

		writeHead(),
		readHeadBuffer(),
		delay(),

		feedbackP(0.f),
		dampP(1.f),
		retuneP(0.f),

		Fs(0.f), sizeF(0.f), curDelay(0.f), curNote(48.f),
		size(0)
	{}

	void CombFilter::prepare(float sampleRate, int blockSize)
	{
		Fs = sampleRate;

		sizeF = std::ceil(freqHzInSamples(LowestFrequencyHz, Fs));
		size = static_cast<int>(sizeF);

		writeHead.prepare(blockSize, size);
		readHeadBuffer.setSize(2, blockSize, false, false, false);
		delay.prepare(Fs, size);

		const auto freqHz = xenManager.noteToFreqHzWithWrap(curNote, LowestFrequencyHz);
		curDelay = freqHzInSamples(freqHz, Fs);

		feedbackP.prepare(sampleRate, blockSize, 10.f);
		dampP.prepare(sampleRate, blockSize, 10.f);
		retuneP.prepare(sampleRate, blockSize, 10.f);
	}

	void CombFilter::operator()(float* const* samples, int numChannels, int numSamples,
		float _feedback, float _damp, float _retune) noexcept
	{
		writeHead(numSamples);
		const auto wHead = writeHead.data();

		const auto retuneBuf = retuneP(_retune, numSamples);

		{ // calculate readhead indexes from note buffer
			auto rHeadBuf = readHeadBuffer.getArrayOfWritePointers();

			auto& noteBuffer = midiVoices.voices[0].buffer;

			for (auto s = 0; s < numSamples; ++s)
			{
				auto nNote = static_cast<float>(noteBuffer[0].noteNumber);
				const auto pb = midiVoices.pitchbendBuffer.buffer[s];

				nNote = juce::jlimit(1.f, 127.f, nNote + retuneBuf[s] + pb);

				if (curNote != nNote)
				{
					curNote = nNote;
					const auto freqHz = xenManager.noteToFreqHzWithWrap(curNote, LowestFrequencyHz);
					curDelay = freqHzInSamples(freqHz, Fs);
				}

				const auto w = static_cast<float>(wHead[s]);
				auto r = w - curDelay;
				if (r < 0.f)
					r += sizeF;

				rHeadBuf[0][s] = r;
			}

			for (auto ch = 1; ch < numChannels; ++ch) // only until i have a better idea
				SIMD::copy(rHeadBuf[ch], rHeadBuf[0], numSamples);
		}

		const auto fbBuf = feedbackP(_feedback, numSamples);
		const auto rHeadBufConst = readHeadBuffer.getArrayOfReadPointers();

		const auto xFromHz = smooth::Lowpass<float>::getXFromHz(_damp, Fs);
		const auto dampBuf = dampP(xFromHz, numSamples);

		delay(samples, numChannels, numSamples,
			wHead, fbBuf, dampBuf, rHeadBufConst);
	}
}