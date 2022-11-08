#include "Rectifier.h"
#include <cmath>

namespace audio
{
	void rectify(float* samples, int numSamples) noexcept
	{
		for (auto s = 0; s < numSamples; ++s)
			samples[s] = std::sqrt(samples[s] * samples[s]);
	}

	void rectify(float** samples, int numChannels, int numSamples) noexcept
	{
		for (auto ch = 0; ch < numChannels; ++ch)
			rectify(samples[ch], numSamples);
	}
}