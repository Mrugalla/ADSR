#pragma once
#include <juce_core/juce_core.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_events/juce_events.h>

#include <cmath>

namespace audio
{
	struct NullNoiseSynth :
		public juce::HighResolutionTimer
	{
		using File = juce::File;
		using UniqueStream = std::unique_ptr<juce::FileInputStream>;
		using SpecLoc = File::SpecialLocationType;

		NullNoiseSynth();

		~NullNoiseSynth();

		/* samples, numChannels, numSamples */
		void operator()(float**, int, int) noexcept;

		/* samples, numSamples */
		void operator()(float*, int) noexcept;

	protected:
		UniqueStream inputStream;
		std::vector<int> validPos;
		std::vector<float> noise;
		int writeHead, readHead;

		void hiResTimerCallback() override;
	};
}

/*

this synth makes crappy noise from data that is used in a wrong way.
it's a fun side project. contributions are welcome

todo: save and load buffer indexes after first opened plugin
*/