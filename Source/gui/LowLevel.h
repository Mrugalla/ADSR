#pragma once
#include "EnvelopeGenerator.h"
#include "Oscilloscope.h"

namespace gui
{
    struct LowLevel :
        public Comp
    {
        LowLevel(Utils& u) :
            Comp(u, "", CursorType::Default),
			envGen
            (u,
                PID::EnvGenAttack, PID::EnvGenDecay,
                PID::EnvGenSustain, PID::EnvGenRelease,
                PID::EnvGenAtkShape, PID::EnvGenDcyShape,
                PID::EnvGenRlsShape
            ),
            atk(u),
			dcy(u),
			sus(u),
			rls(u),
			oscope(u, "The oscilloscope visualizes the ADSR shape.", u.audioProcessor.oscope)
        {
            addAndMakeVisible(envGen);

            makeParameter(atk, PID::EnvGenAttack, "Attack", true);
            addAndMakeVisible(atk);

			makeParameter(dcy, PID::EnvGenDecay, "Decay", true);
			addAndMakeVisible(dcy);
			
			makeParameter(sus, PID::EnvGenSustain, "Sustain", true);
			addAndMakeVisible(sus);

			makeParameter(rls, PID::EnvGenRelease, "Release", true);
			addAndMakeVisible(rls);

			addAndMakeVisible(oscope);

            layout.init
            (
                { 1, 2, 2, 2, 2, 1 },
                { 5, 2, 2 }
            );
        }

        void paint(Graphics&) override
        {
        }

        void resized() override
        {
            layout.resized();

            layout.place(envGen, 1, 0, 4, 1);

            layout.place(atk, 1, 1, 1, 1);
			layout.place(dcy, 2, 1, 1, 1);
			layout.place(sus, 3, 1, 1, 1);
			layout.place(rls, 4, 1, 1, 1);

			layout.place(oscope, 1, 2, 4, 1);
        }

    protected:
        EnvGenComp envGen;
        Knob atk, dcy, sus, rls;
        Oscilloscope oscope;
    };
}