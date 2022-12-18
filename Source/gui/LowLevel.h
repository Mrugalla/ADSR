#pragma once
#include "RadioButton.h"
#include "EnvelopeGenerator.h"
#include "Oscilloscope.h"

namespace gui
{
    struct LowLevel :
        public Comp,
		public Timer
    {
        using FlexKnob = std::unique_ptr<Knob>;
		
        struct ModeCompGain :
            public Comp
        {
            ModeCompGain(Utils& u) :
                Comp(u, "", CursorType::Default),
                lowerLim(u),
				upperLim(u)
            {
				addAndMakeVisible(lowerLim);
				addAndMakeVisible(upperLim);
				
				makeParameter(lowerLim, PID::ModeGainLowerLimit, "Lower Limit");
				makeParameter(upperLim, PID::ModeGainUpperLimit, "Upper Limit");

                layout.init
                (
                    { 1, 1 },
                    { 1 }
                );
            }

			void paint(Graphics&) override { }

            void resized() override
            {
                layout.resized();

                layout.place(lowerLim, 0, 0, 1, 1, false);
				layout.place(upperLim, 1, 0, 1, 1, false);
            }

            Knob lowerLim, upperLim;
        };

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
                PID::EnvGenTempoSync, PID::EnvGenLockDcyRls
            ),
			mode(u),
			modeComp(u),
			midiChannel(u), midiCC(u),
            atk(u), sus(u), atkBeats(u),
            dcy(), rls(), dcyBeats(), rlsBeats(),
            lockDcyRls(u),
            oscope(u, "The oscilloscope visualizes the ADSR shape.", u.audioProcessor.oscope),
            legato(u),
            inverse(u),
            tempoSync(u),
            velo(u)
        {
            addAndMakeVisible(envGen);
			
            addAndMakeVisible(mode);
            makeParameter(mode, PID::EnvGenMode, "Gain\nMode", true);
            {
                addChildComponent(modeComp);
				
                const auto& modeParam = *u.getParam(PID::EnvGenMode);
                const auto valDenorm = static_cast<int>(std::round(modeParam.getValModDenorm() - modeParam.range.start));
                
                setVisible(valDenorm == 1);
            }
			
			makeParameter(midiChannel, PID::ControllerChannel, "MIDI Channel");
            addAndMakeVisible(midiChannel);

			makeParameter(midiCC, PID::ControllerCC, "CC");
			addAndMakeVisible(midiCC);

            makeParameter(atk, PID::EnvGenAttack, "Attack", true);
            addChildComponent(atk);

            makeParameter(lockDcyRls, PID::EnvGenLockDcyRls, ButtonSymbol::UnityGain);
            addAndMakeVisible(lockDcyRls);
            switchDcyRlsParams(lockDcyRls.toggleState == 1);
            lockDcyRls.onClick.push_back([&](Button&, const Mouse&)
            {
                switchDcyRlsParams(lockDcyRls.toggleState == 1);
                resized();
            });

			makeParameter(sus, PID::EnvGenSustain, "Sustain", true);
			addAndMakeVisible(sus);

			makeParameter(atkBeats, PID::EnvGenAttackBeats, "Attack", true);
            addChildComponent(atkBeats);

			addAndMakeVisible(oscope);
            oscope.bipolar = false;

			makeParameter(legato, PID::EnvGenLegato, "Legato");
            addAndMakeVisible(legato);

            makeParameter(inverse, PID::EnvGenInverse, ButtonSymbol::InvertADSR);
            addAndMakeVisible(inverse);

			makeParameter(tempoSync, PID::EnvGenTempoSync, ButtonSymbol::TempoSync);
			addAndMakeVisible(tempoSync);

            makeParameter(velo, PID::EnvGenVelocity, "Velo");
            addAndMakeVisible(velo);

            layout.init
            (
                { 1, 13, 13, 13, 13, 1 },
                { 2, 8, 3, 1, 2, 3 }
            );

            startTimerHz(12);
        }

        void paint(Graphics& g) override
        {
            const auto thicc = utils.thicc;
            const auto thicc5 = thicc * 5.f;

			Stroke stroke(thicc, Stroke::JointStyle::curved, Stroke::EndCapStyle::butt);

            g.setColour(Colours::c(ColourID::Hover));

            const auto modeBounds = mode.getBounds().toFloat();
            const auto modeCompBounds = modeComp.getBounds().toFloat();
            BoundsF modeBothBounds
            (
                modeBounds.getX(),
                modeCompBounds.getY(),
				modeCompBounds.getRight() - modeBounds.getX(),
				modeCompBounds.getHeight()
            );
			drawRectEdges(g, modeBothBounds, thicc5, stroke);

            const auto dcyBounds = dcy->getBounds().toFloat();
            const auto rlsBounds = rls->getBounds().toFloat();
            const auto lockDcyRlsBounds = lockDcyRls.getBounds().toFloat();
			
            const PointF dcyBoundsCentre
            (
                dcyBounds.getX() + dcyBounds.getWidth() * .5f,
                dcyBounds.getY() + dcyBounds.getHeight() * .5f
            );
            const PointF rlsBoundsCentre
			(
				rlsBounds.getX() + rlsBounds.getWidth() * .5f,
				rlsBounds.getY() + rlsBounds.getHeight() * .5f
			);
            const PointF lockDcyRlsBoundsCentre
			(
				lockDcyRlsBounds.getX() + lockDcyRlsBounds.getWidth() * .5f,
				lockDcyRlsBounds.getY() + lockDcyRlsBounds.getHeight() * .5f
			);
			
			const auto x0 = dcyBoundsCentre.x;
			const auto x1 = lockDcyRlsBounds.getX() - thicc;
            const auto x2 = lockDcyRlsBounds.getRight() + thicc;
            const auto x3 = rlsBoundsCentre.x;
            const auto y0 = dcyBounds.getBottom() + thicc;
			const auto y1 = lockDcyRlsBoundsCentre.y;

            g.drawLine(x0, y0, x0, y1, thicc);
			g.drawLine(x0, y1, x1, y1, thicc);
			g.drawLine(x2, y1, x3, y1, thicc);
			g.drawLine(x3, y1, x3, y0, thicc);
        }

        void resized() override
        {
            layout.resized();

            layout.place(mode, 1, 0, 1, 1, true);
			layout.place(modeComp, 2, 0, 1, 1);

			layout.place(midiChannel, 3, 0, 1, 1);
			layout.place(midiCC, 4, 0, 1, 1);

            layout.place(envGen, 1, 1, 4, 1);

            layout.place(atk, 1, 2, 1, 1);
			layout.place(*dcy, 2, 2, 1, 1);
			layout.place(sus, 3, 2, 1, 1);
			layout.place(*rls, 4, 2, 1, 1);

            atkBeats.setBounds(atk.getBounds());
			dcyBeats->setBounds(dcy->getBounds());
            rlsBeats->setBounds(rls->getBounds());
			
			layout.place(lockDcyRls, 3, 3, 1, 1, true);

            BoundsF oscopeBounds
            (
                static_cast<float>(envGen.getX()) + envGen.bounds.getX(),
                layout.getY(4),
				envGen.bounds.getWidth(),
                layout.getH(4)
            );
            oscope.setBounds(oscopeBounds.toNearestInt());

            layout.place(legato, 1, 5, 1, 1, false);
            layout.place(inverse, 2, 5, 1, 1, true);
			layout.place(tempoSync, 3, 5, 1, 1, true);
            layout.place(velo, 4, 5, 1, 1, false);
        }

        void timerCallback() override
        {
            bool isTempoSync = utils.getParam(PID::EnvGenTempoSync)->getValMod() > .5f;
            atk.setVisible(!isTempoSync);
			dcy->setVisible(!isTempoSync);
			rls->setVisible(!isTempoSync);
			atkBeats.setVisible(isTempoSync);
			dcyBeats->setVisible(isTempoSync);
			rlsBeats->setVisible(isTempoSync);

            enum { DirectOut, Gain, NumModes };
            const auto& modeParam = *utils.getParam(PID::EnvGenMode);
            const auto valDenorm = static_cast<int>(std::round(modeParam.getValModDenorm() - modeParam.range.start));

            modeComp.setVisible(valDenorm == Gain);
        }

    protected:
        EnvGenComp envGen;
        Button mode;
        ModeCompGain modeComp;
        Knob midiChannel, midiCC;
		Knob atk, sus, atkBeats;
        FlexKnob dcy, rls, dcyBeats, rlsBeats;
        Button lockDcyRls;
        Oscilloscope oscope;
        Knob legato;
        Button inverse, tempoSync;
        Knob velo;

        void switchDcyRlsParams(bool lockEnabled)
        {
            removeChildComponent(dcy.get());
			removeChildComponent(rls.get());
			removeChildComponent(dcyBeats.get());
			removeChildComponent(rlsBeats.get());

            dcy = std::make_unique<Knob>(utils);
			rls = std::make_unique<Knob>(utils);
			dcyBeats = std::make_unique<Knob>(utils);
            rlsBeats = std::make_unique<Knob>(utils);

			if (lockEnabled)
			{
                makeParameter(*dcy, { PID::EnvGenDecay, PID::EnvGenRelease }, "Decay", true);
				makeParameter(*rls, { PID::EnvGenRelease, PID::EnvGenDecay }, "Release", true);
				makeParameter(*dcyBeats, { PID::EnvGenDecayBeats, PID::EnvGenReleaseBeats }, "Decay", true);
				makeParameter(*rlsBeats, { PID::EnvGenReleaseBeats, PID::EnvGenDecayBeats }, "Release", true);
			}
			else
			{
				makeParameter(*dcy, PID::EnvGenDecay, "Decay");
				makeParameter(*rls, PID::EnvGenRelease, "Release");
				makeParameter(*dcyBeats, PID::EnvGenDecayBeats, "Decay");
				makeParameter(*rlsBeats, PID::EnvGenReleaseBeats, "Release");
			}

            addAndMakeVisible(dcy.get());
			addAndMakeVisible(rls.get());
			addAndMakeVisible(dcyBeats.get());
			addAndMakeVisible(rlsBeats.get());
        }
    };
}