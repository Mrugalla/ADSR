#pragma once

namespace audio
{
	/* samples, numSamples, gain, gainInv */
	void crush(float*, int, float, const float);

	/* samples, numChannels, numSamples, gain */
	void crush(float**, int, int, float);
}