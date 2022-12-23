#pragma once
#include "AudioUtils.h"

namespace audio
{
    class MIDIDelay
	{
        static constexpr int NumEvents = 64;

        struct Evt
        {
            Evt() :
				msg(MIDIMessage::noteOn(1, 0, 0.f)),
                time(0),
				used(false)
            {}

            MIDIMessage msg;
            int time;
            bool used;
        };

    public:
		MIDIDelay() :
            evts(),
            outputBuffer()
		{
            outputBuffer.ensureSize(NumEvents);
        }

        void operator()(MIDIBuffer& midi, int numSamples, int delayTimeSamples) noexcept
		{
            if (delayTimeSamples < 1)
                return;

            for (auto itRef : midi)
            {
                auto msg = itRef.getMessage();
                midiToEvts(msg, itRef.samplePosition + delayTimeSamples);
            }

            outputBuffer.clear();

            for (auto s = 0; s < numSamples; ++s)
                evtsToOutput(s);
            
            midi.swapWith(outputBuffer);
		}

    protected:
		std::array<Evt, NumEvents> evts;
        MIDIBuffer outputBuffer;

        void midiToEvts(MIDIMessage& msg, int time) noexcept
        {
            for (auto i = 0; i < NumEvents; ++i)
            {
                auto& evt = evts[i];

                if (!evt.used)
                {
                    evt.time = time;
                    evt.msg = msg;
                    evt.used = true;
                    return;
                }
            }
        }

        void evtsToOutput(int s) noexcept
        {
            for (auto i = 0; i < NumEvents; ++i)
            {
                auto& evt = evts[i];

                if (evt.used)
                {
                    --evt.time;
                    if (evt.time < 1)
                    {
                        if (outputBuffer.addEvent(evt.msg, s))
                        {
                            evt.used = false;
                        }
                    }
                }
            }
                
        }
	};
}

/*

to do:

if lookahead enabled, gui needs to show if attack parameter longer than possible latency

*/