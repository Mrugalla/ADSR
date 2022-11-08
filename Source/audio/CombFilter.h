#pragma once
#include <array>
#include "WHead.h"
#include "AudioUtils.h"
#include "PRM.h"
#include "MIDIManager.h"
#include "XenManager.h"

namespace audio
{
	class CombFilter
	{
		struct DelayFeedback
		{
			using Lowpass = smooth::Lowpass<float>;

			DelayFeedback();

			/* Fs, size */
			void prepare(float, int);

			//samples, numChannels, numSamples, wHead, feedbackBuffer[-1,1], dampBuf, readHead
			void operator()(float**, int, int, const int*, const float*, const float*, const float**) noexcept;

		protected:
			AudioBuffer ringBuffer;
			std::array<Lowpass, 2> lowpass;
			int size;
		};

		static constexpr float LowestFrequencyHz = 20.f;

	public:
		CombFilter(MIDIVoices&, const XenManager&);
		
		/* sampleRate, blockSize */
		void prepare(float, int);

		/* samples, numChannels, numSamples, feedback ]-1,1[, damp ]0, 22050[hz, retune [-n,n]semi */
		void operator()(float**, int, int, float, float, float) noexcept;

	protected:
		MIDIVoices& midiVoices;
		const XenManager& xenManager;
		
		WHead writeHead;
		AudioBuffer readHeadBuffer;
		DelayFeedback delay;

		PRM feedbackP, dampP, retuneP;
		
		float Fs, sizeF, curDelay, curNote;
		int size;
	};
}