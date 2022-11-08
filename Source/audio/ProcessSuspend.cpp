#include "ProcessSuspend.h"

namespace audio
{
	ProcessSuspender::ProcessSuspender(juce::AudioProcessor& p) :
		processor(p),
		stage(Stage::Running)
	{

	}

	void ProcessSuspender::suspend() noexcept
	{
		stage.store(Stage::Suspending);
	}

	bool ProcessSuspender::suspendIfNeeded(AudioBuffer& buf) noexcept
	{
		auto samples = buf.getArrayOfWritePointers();
		const auto numChannels = buf.getNumChannels();
		const auto numSamples = buf.getNumSamples();

		const auto stg = stage.load();
		if (stg == Stage::Running)
			return false;

		if (numSamples != 0)
			for (auto ch = 0; ch < numChannels; ++ch)
				SIMD::fill(samples[ch], 0.f, numSamples);

		if (stg == Stage::Suspending)
		{
			stage.store(Stage::Suspended);
			processor.prepareToPlay
			(
				processor.getSampleRate(),
				processor.getBlockSize()
			);
		}

		return true;
	}

	void ProcessSuspender::prepareToPlay() noexcept
	{
		stage.store(Stage::Running);
	}
}