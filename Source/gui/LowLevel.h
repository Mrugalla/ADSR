#pragma once
#include "EnvelopeGenerator.h"
#include "Oscilloscope.h"

namespace gui
{
    struct LowLevel :
        public Comp,
		public Timer
    {
        LowLevel(Utils& u) :
            Comp(u, "", CursorType::Default),
			Timer(),
			envGen
            (u,
                PID::EnvGenAttack, PID::EnvGenDecay,
                PID::EnvGenSustain, PID::EnvGenRelease,
                PID::EnvGenAtkShape, PID::EnvGenDcyShape,
                PID::EnvGenRlsShape,
				PID::EnvGenAttackBeats, PID::EnvGenDecayBeats,
				PID::EnvGenReleaseBeats,
                PID::EnvGenTempoSync
            ),
            atk(u),
			dcy(u),
			sus(u),
			rls(u),
			atkBeats(u),
			dcyBeats(u),
			rlsBeats(u),
            oscope(u, "The oscilloscope visualizes the ADSR shape.", u.audioProcessor.oscope),
            legato(u),
            inverse(u),
            tempoSync(u),
            velo(u)
        {
            addAndMakeVisible(envGen);

            makeParameter(atk, PID::EnvGenAttack, "Attack", true);
            addChildComponent(atk);

			makeParameter(dcy, PID::EnvGenDecay, "Decay", true);
            addChildComponent(dcy);
			
			makeParameter(sus, PID::EnvGenSustain, "Sustain", true);
			addAndMakeVisible(sus);

			makeParameter(rls, PID::EnvGenRelease, "Release", true);
            addChildComponent(rls);

			makeParameter(atkBeats, PID::EnvGenAttackBeats, "Attack", true);
            addChildComponent(atkBeats);

			makeParameter(dcyBeats, PID::EnvGenDecayBeats, "Decay", true);
            addChildComponent(dcyBeats);
			
			makeParameter(rlsBeats, PID::EnvGenReleaseBeats, "Release", true);
            addChildComponent(rlsBeats);

			addAndMakeVisible(oscope);
            oscope.bipolar = false;

            makeParameter(legato, PID::EnvGenLegato, ButtonSymbol::Legato);
            addAndMakeVisible(legato);

            makeParameter(inverse, PID::EnvGenInverse, "Inverse", true);
            addAndMakeVisible(inverse);

			makeParameter(tempoSync, PID::EnvGenTempoSync, ButtonSymbol::TempoSync);
			addAndMakeVisible(tempoSync);

            makeParameter(velo, PID::EnvGenVelocity, "Velo");
            addAndMakeVisible(velo);

            layout.init
            (
                { 1, 2, 2, 2, 2, 1 },
                { 8, 3, 2, 3 }
            );

            startTimerHz(12);
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

			layout.place(atkBeats, 1, 1, 1, 1);
			layout.place(dcyBeats, 2, 1, 1, 1);
			layout.place(rlsBeats, 4, 1, 1, 1);

            const auto thicc = utils.thicc;
            const auto thicc5 = thicc * 5.f;
            oscope.setBounds(layout(1, 2, 4, 1).reduced(thicc5).toNearestInt());

            layout.place(legato, 1, 3, 1, 1, true);
            layout.place(inverse, 2, 3, 1, 1, true);
			layout.place(tempoSync, 3, 3, 1, 1, true);
            layout.place(velo, 4, 3, 1, 1, false);
        }

        void timerCallback() override
        {
            bool isTempoSync = utils.getParam(PID::EnvGenTempoSync)->getValMod() > .5f;
            atk.setVisible(!isTempoSync);
			dcy.setVisible(!isTempoSync);
			rls.setVisible(!isTempoSync);
			atkBeats.setVisible(isTempoSync);
			dcyBeats.setVisible(isTempoSync);
			rlsBeats.setVisible(isTempoSync);
        }

    protected:
        EnvGenComp envGen;
		Knob atk, dcy, sus, rls, atkBeats, dcyBeats, rlsBeats;
        Oscilloscope oscope;
        Button legato, inverse, tempoSync;
        Knob velo;
    };
}