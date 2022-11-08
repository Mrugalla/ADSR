/*
  ==============================================================================

    NullNoiseSynth.cpp
    Created: 2 Nov 2022 11:07:59pm
    Author:  Hans

  ==============================================================================
*/

#include "NullNoiseSynth.h"

namespace audio
{
	NullNoiseSynth::NullNoiseSynth() :
		HighResolutionTimer(),
		inputStream(File::getSpecialLocation(SpecLoc::currentApplicationFile).createInputStream()),
		validPos(),
		noise(),
		writeHead(0),
		readHead(0)
	{
		noise.reserve(441);

		auto& stream = *inputStream.get();

		validPos.reserve(1024);

		while (!stream.isExhausted())
		{
			const auto pos = stream.getPosition();
			const auto smpl = stream.readFloatBigEndian();
			if (!std::isnan(smpl) && !std::isinf(smpl))
			{
				if (smpl > -1.f && smpl < 1.f)
				{
					const auto absSmpl = std::abs(smpl);
					if (absSmpl > .000001f)
					{
						validPos.push_back(static_cast<int>(pos));
					}
				}
			}
		}

		for (auto n = 0; n < noise.capacity(); ++n)
		{
			stream.setPosition(validPos[writeHead]);

			auto smpl = stream.readFloatBigEndian();

			noise.emplace_back(smpl);

			++writeHead;
			if (writeHead == validPos.size())
				writeHead = 0;
		}

		startTimer(static_cast<int>(1000.f / 25.f));
	}

	NullNoiseSynth::~NullNoiseSynth()
	{
		stopTimer();
	}

	void NullNoiseSynth::operator()(float** samples, int numChannels, int numSamples) noexcept
	{
		for (auto ch = 0; ch < numChannels; ++ch)
		{
			auto smpls = samples[ch];

			operator()(smpls, numSamples);
		}
	}

	void NullNoiseSynth::operator()(float* samples, int numSamples) noexcept
	{
		auto smpls = samples;

		for (auto s = 0; s < numSamples; ++s)
		{
			smpls[s] = noise[readHead];

			++readHead;
			if (readHead == noise.size())
				readHead = 0;
		}
	}

	void NullNoiseSynth::hiResTimerCallback()
	{
		auto& stream = *inputStream.get();

		for (auto n = 0; n < noise.size(); ++n)
		{
			stream.setPosition(validPos[writeHead]);

			auto smpl = stream.readFloatBigEndian();

			noise[n] = smpl;

			++writeHead;
			if (writeHead == validPos.size())
				writeHead = 0;
		}
	}
}