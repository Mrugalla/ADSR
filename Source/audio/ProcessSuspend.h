#pragma once
#include "AudioUtils.h"

namespace audio
{
	struct ProcessSuspender
	{
		enum class Stage
		{
			Running,
			Suspending,
			Suspended,
			NumStages
		};

		ProcessSuspender(juce::AudioProcessor&);

		void suspend() noexcept;

		/* returns true if suspending is needed (= return from processBlock) */
		bool suspendIfNeeded(AudioBuffer&) noexcept;
		
		void prepareToPlay() noexcept;

	protected:
		juce::AudioProcessor& processor;
		std::atomic<Stage> stage;
	};
}
