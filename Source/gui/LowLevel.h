#pragma once
#include "Knob.h"
#include "EnvelopeGenerator.h"

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
			rls(u)
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

            layout.init
            (
                { 1, 2, 2, 2, 2, 1 },
                { 5, 2 }
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
        }

    protected:
        EnvGenComp envGen;
        Knob atk, dcy, sus, rls;
    };
}