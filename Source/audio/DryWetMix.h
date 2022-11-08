#pragma once
#include "LatencyCompensation.h"
#include <array>

namespace audio
{
	class DryWetMix
	{
		enum
		{
#if PPDHasGainIn
			GainIn,
#endif
			Mix,
#if PPDHasGainOut
			GainOut,
#endif
			NumBufs
		};

	public:
		DryWetMix();

		/* sampleRate, blockSize, latency */
		void prepare(float, int, int);

		/* samples, numChannels, numSamples, gainInP, unityGainP, mixP, gainOutP, polarityP */
		void saveDry
		(
			float**, int, int,
#if PPDHasGainIn
			float,
#if PPDHasUnityGain
			float,
#endif
#endif
			float
#if PPDHasGainOut
			, float
#if PPDHasPolarity
			, float
#endif
#endif
		) noexcept;

		/* samples, numChannels, numSamples */
		void processBypass(float**, int, int) noexcept;

#if PPDHasGainOut
		/* samples, numChannels, numSamples */
		void processOutGain(float**, int, int) const noexcept;
#endif

		/* samples, numChannels, numSamples, delta */
		void processMix(float**, int, int
#if PPDHasDelta
			, bool
#endif
			) const noexcept;

	protected:
		LatencyCompensation latencyCompensation;

		AudioBuffer buffers;
		
#if PPDHasGainIn
		Smooth gainInSmooth;
#endif
		Smooth mixSmooth;
#if PPDHasGainOut
		Smooth gainOutSmooth;
#endif
		
		AudioBuffer dryBuf;
	};
}