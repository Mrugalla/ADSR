#pragma once
#include "AudioUtils.h"
#include "WHead.h"
#include "Phasor.h"

namespace audio
{
	struct DelayFF
	{
		DelayFF() :
			ringBuffer()
		{
		}

		void prepare(int delaySize)
		{
			ringBuffer.setSize(2, delaySize, false, true, false);
		}
		
		void operator()(float* smpls, float* ring, int numSamples,
			const int* wHead, const int* rHead) noexcept
		{
			for (auto s = 0; s < numSamples; ++s)
			{
				const auto w = wHead[s];
				const auto r = rHead[s];

				ring[w] = smpls[s];
				smpls[s] = ring[r];
			}
		}

		void operator()(float* smpls, float* ring, int numSamples,
			const int* wHead, const int* rHead0, const int* rHead1, const PhaseInfo<float>* xFade) noexcept
		{
			for (auto s = 0; s < numSamples; ++s)
			{
				const auto w = wHead[s];
				const auto r0 = rHead0[s];
				const auto r1 = rHead1[s];
				auto xF = xFade[s].phase < 1.f ? xFade[s].phase : 1.f;

				ring[w] = smpls[s];
				smpls[s] = ring[r0] + xF * (ring[r1] - ring[r0]);
			}
		}

		void operator()(float** samples, int numChannels, int numSamples,
			const int* wHead, const int* rHead) noexcept
		{
			auto ring = ringBuffer.getArrayOfWritePointers();
			for (auto ch = 0; ch < numChannels; ++ch)
				operator()(samples[ch], ring[ch], numSamples, wHead, rHead);
		}

		void operator()(float** samples, int numChannels, int numSamples,
			const int* wHead, const int* rHead0, const int* rHead1, const PhaseInfo<float>* xFade) noexcept
		{
			auto ring = ringBuffer.getArrayOfWritePointers();
			for (auto ch = 0; ch < numChannels; ++ch)
				operator()(samples[ch], ring[ch], numSamples, wHead, rHead0, rHead1, xFade);
		}

		AudioBuffer ringBuffer;
	};

	struct DelayFFXFade
	{
		enum class State
		{
			Running,
			Moving,
			NumStates
		};
		
		DelayFFXFade() :
			delay(),
			wHead(),
			readHeads(),
			delayPositions{ 0, 0 },
			delayIdx(0),
			size(0),
			state(State::Running),
			phasor(),
			phasorBuffer()
		{}

		void prepare(int blockSize, int delaySize)
		{
			size = delaySize;
			delay.prepare(size);
			wHead.prepare(blockSize, size);
			for(auto& readHead: readHeads)
				readHead.resize(blockSize);
			phasorBuffer.resize(blockSize);
		}

		void operator()(float** samples, int numChannels, int numSamples,
			int _nDelayPos, int _grainLength) noexcept
		{
			wHead(numSamples);
			const auto w = wHead.data();

			switch (state)
			{
			case State::Running:
				processRunning(samples, numChannels, numSamples, w, _nDelayPos, _grainLength);
				break;
			case State::Moving:
				processMoving(samples, numChannels, numSamples, w);
				break;
			}
		}

		void processRunning(float** samples, int numChannels, int numSamples, 
			const int* w, int _nDelayPos, int _grainLength) noexcept
		{
			auto curDelayPos = delayPositions[delayIdx];
			const bool keepRunning = curDelayPos == _nDelayPos;

			if (keepRunning)
			{
				auto readHead = readHeads[delayIdx].data();

				for (auto s = 0; s < numSamples; ++s)
				{
					auto r = w[s] - curDelayPos;
					if (r < 0)
						r += size;
					readHead[s] = r;
				}

				delay(samples, numChannels, numSamples, w, readHead);
			}
			else
			{
				delayIdx = 1 - delayIdx;
				delayPositions[delayIdx] = _nDelayPos;
				phasor.reset();
				phasor.inc = 1.f / static_cast<float>(_grainLength);
				
				processMoving(samples, numChannels, numSamples, w);
			}
		}

		void processMoving(float** samples, int numChannels, int numSamples,
			const int* w) noexcept
		{
			for (auto s = 0; s < numSamples; ++s)
				phasorBuffer[s] = phasor();
			
			for (auto i = 0; i < 2; ++i)
			{
				auto readHead = readHeads[i].data();
				
				for (auto s = 0; s < numSamples; ++s)
				{
					auto r = w[s] - delayPositions[i];
					if (r < 0)
						r += size;
					readHead[s] = r;
				}
			}

			delay
			(
				samples,
				numChannels,
				numSamples,
				w,
				readHeads[1 - delayIdx].data(),
				readHeads[delayIdx].data(),
				phasorBuffer.data()
			);

			for(auto s = 0; s < numSamples; ++s)
				if (phasorBuffer[s].retrig)
				{
					state = State::Running;
					break;
				}
		}

		DelayFF delay;
		WHead wHead;
		std::array<std::vector<int>, 2> readHeads;
		std::array<int, 2> delayPositions;
		int delayIdx, size;
		State state;
		Phasor<float> phasor;
		std::vector<PhaseInfo<float>> phasorBuffer;
	};

	struct DelayFFXFadeLatencyComp
	{
		DelayFFXFadeLatencyComp() :
			delay()
		{}

		void prepare(int blockSize, int delaySize)
		{
			delay.prepare(blockSize, delaySize);
		}

		void operator()(float** samples, int numChannels, int numSamples,
			int curLatency, int grainLength) noexcept
		{
			if (delay.size == 0)
				return;

			auto nDelayPos = delay.size - curLatency;
			if (nDelayPos < 0)
				nDelayPos = 0;
			else if(nDelayPos >= delay.size)
				nDelayPos = delay.size - 1;

			delay(samples, numChannels, numSamples, nDelayPos, grainLength);
		}

		DelayFFXFade delay;
	};
}