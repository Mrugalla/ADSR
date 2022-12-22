#pragma once
#include "AudioUtils.h"

namespace audio
{
    struct MIDIDelay
	{
        static constexpr int NumEvents = 1024;

		MIDIDelay() :
            list(),
			used(),
            outputBuffer()
		{
            for (auto& u : used)
                u = false;
            outputBuffer.ensureSize(NumEvents);
        }

        void operator()(MIDIBuffer& midi, int numSamples, int delayTimeSamples) noexcept
		{
            if (delayTimeSamples < 1)
                return;

            for (auto itRef : midi)
            {
                auto msg = itRef.getMessage();
                addEvent(msg, itRef.samplePosition + delayTimeSamples);
            }

            outputBuffer.clear();

            for (auto s = 0; s < numSamples; ++s)
                processEvents(s);
            
            midi.swapWith(outputBuffer);
		}

    protected:
		std::array<MIDIMessage, NumEvents> list;
        std::array<bool, NumEvents> used;
        MIDIBuffer outputBuffer;

        void addEvent(MIDIMessage& msg, int timeStamp) noexcept
        {
            for (auto i = 0; i < NumEvents; ++i)
                if (!used[i])
                {
                    msg.setTimeStamp(static_cast<double>(timeStamp));
                    list[i] = msg;
                    used[i] = true;
                    return;
                }
        }

        void processEvents(int s) noexcept
        {
            for (auto i = 0; i < NumEvents; ++i)
                if (used[i])
                {
                    auto timeStamp = list[i].getTimeStamp() - 1.;
                    if (timeStamp < 1.)
                    {
                        outputBuffer.addEvent(list[i], s);
                        used[i] = false;
                    }
                    list[i].setTimeStamp(timeStamp);
                }
        }
	};
}