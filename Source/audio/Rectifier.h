#pragma once

namespace audio
{
	/* smpls, numSamples */
	void rectify(float*, int) noexcept;
	
	/* samples, numChannels, numSamples */
	void rectify(float**, int, int) noexcept;
}