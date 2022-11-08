#include "Bitcrusher.h"
#include <cmath>

namespace audio
{
	void crush(float* samples, int numSamples, float gain,
		const float gainInv)
	{
		for (auto s = 0; s < numSamples; ++s)
			samples[s] = std::round(samples[s] * gain) * gainInv;
	}

	void crush(float** samples, int numChannels, int numSamples, float gain)
	{
		const auto gainInv = 1.f / gain;
		for (auto ch = 0; ch < numChannels; ++ch)
			crush(samples[ch], numSamples, gain, gainInv);
	}
}