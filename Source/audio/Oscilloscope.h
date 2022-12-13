#pragma once
#include "AudioUtils.h"
#include "WHead.h"
#include "Phasor.h"

namespace audio
{
	struct Oscilloscope
	{
		Oscilloscope() :
			wHead(),
			buffer(),
			phasor(0.),
			beatLength(1.f),
			Fs(0.)
		{}

		void prepare(double sampleRate, int blockSize)
		{
			Fs = sampleRate;

			auto windowSize = static_cast<int>(Fs) * 4;
			wHead.prepare(blockSize, windowSize);
			buffer.resize(windowSize);

			phasor.prepare(1. / Fs);
		}

		void operator()(const float** samples, int numChannels, int numSamples,
			const PlayHeadPos& playHead) noexcept
		{	
			wHead(numSamples);
			
			const auto rateSyncV = 1.;

			const auto bpm = playHead.bpm;
			const auto bps = bpm * .0166666667;
			const auto quarterNoteLengthInSamples = Fs / bps;
			const auto barLengthInSamples = quarterNoteLengthInSamples * 4.;
			const auto beatLen = barLengthInSamples * rateSyncV;
			phasor.inc = 1. / beatLen;
			beatLength.store(static_cast<float>(beatLen));
			
			const auto ppq = playHead.ppqPosition * .25;
			const auto ppqCh = ppq / rateSyncV;

			phasor.phase.phase = ppqCh - std::floor(ppqCh);

			for (auto s = 0; s < numSamples; ++s)
			{
				auto w = wHead[s];
				
				const auto phaseInfo = phasor();
				if (phaseInfo.retrig)
				{
					wHead.shift(-w, numSamples);
					w = wHead[s];
				}

				buffer[w] = samples[0][s];
			}
			
			if (numChannels == 2)
			{
				for (auto s = 0; s < numSamples; ++s)
				{
					const auto w = wHead[s];
					buffer[w] = (buffer[w] + samples[1][s]) * .5f;
				}
			}
		}

		void operator()(const float* samples, int numSamples,
			const PlayHeadPos& playHead) noexcept
		{
			wHead(numSamples);

			const auto rateSyncV = 1.;

			const auto bpm = playHead.bpm;
			const auto bps = bpm * .0166666667;
			const auto quarterNoteLengthInSamples = Fs / bps;
			const auto barLengthInSamples = quarterNoteLengthInSamples * 4.;
			const auto beatLen = barLengthInSamples * rateSyncV;
			phasor.inc = 1. / beatLen;
			beatLength.store(static_cast<float>(beatLen));

			const auto ppq = playHead.ppqPosition * .25;
			const auto ppqCh = ppq / rateSyncV;

			phasor.phase.phase = ppqCh - std::floor(ppqCh);

			for (auto s = 0; s < numSamples; ++s)
			{
				auto w = wHead[s];

				const auto phaseInfo = phasor();
				if (phaseInfo.retrig)
				{
					wHead.shift(-w, numSamples);
					w = wHead[s];
				}

				buffer[w] = samples[s];
			}
		}

		const float* data() const noexcept
		{
			return buffer.data();
		}
		
		const size_t windowLength() const noexcept
		{
			return buffer.size();
		}

		const float getBeatLength() const noexcept
		{
			return beatLength.load();
		}
	protected:
		WHead wHead;
		std::vector<float> buffer;
		Phasor<double> phasor;
		std::atomic<float> beatLength;
		double Fs;
	};
}