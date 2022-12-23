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

        enum Mode { DirectOut, Gain, MIDICC, NumModes };
		
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
			mode(u), lowerLimit(u), upperLimit(u), midiChannel(u), midiCC(u),
            atk(u), sus(u), atkBeats(u),
            dcy(), rls(), dcyBeats(), rlsBeats(),
            lockDcyRls(u),
            oscope(u, "The oscilloscope visualizes the ADSR shape.", u.audioProcessor.oscope),
            legato(u),
            inverse(u),
            tempoSync(u),
            velo(u),
            dcyRlsLinkEnabled(false),
            atkExceedsLatency(false)
        {
            addAndMakeVisible(envGen);
			
            addAndMakeVisible(mode);
            makeParameter(mode, PID::EnvGenMode, "Mode");
            
			addAndMakeVisible(lowerLimit);
			makeParameter(lowerLimit, PID::LowerLimit, "Lower Limit");

			addAndMakeVisible(upperLimit);
			makeParameter(upperLimit, PID::UpperLimit, "Upper Limit");
			
			makeParameter(midiChannel, PID::ControllerChannel, "MIDI Channel");
            addChildComponent(midiChannel);

			makeParameter(midiCC, PID::ControllerCC, "CC");
            addChildComponent(midiCC);

            makeParameter(atk, PID::EnvGenAttack, "Attack", true);
            addChildComponent(atk);

            makeParameter(lockDcyRls, PID::EnvGenLockDcyRls, ButtonSymbol::UnityGain);
            addAndMakeVisible(lockDcyRls);
            dcyRlsLinkEnabled = lockDcyRls.toggleState == 1;
            switchDcyRlsParams(dcyRlsLinkEnabled);

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
                { 2, 13, 13, 13, 13, 1 },
                { 5, 13, 5, 1, 3, 5 }
            );

            startTimerHz(12);
        }

        void paint(Graphics& g) override
        {
            const auto thicc = utils.thicc;
            g.setColour(Colours::c(ColourID::Hover));

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
			
            if (atkExceedsLatency)
            {
                g.setColour(Colours::c(ColourID::Abort));
                auto atkBounds = layout(1, 3.f, 1, .5f, true);
                g.fillRect(atkBounds);
            }
        }

        void resized() override
        {
            layout.resized();

            layout.place(mode, 1, 0, 1, 1);
			layout.place(lowerLimit, 2.f, 0, 2.f / 3.f, 1);
			layout.place(upperLimit, 2.f + 2.f / 3.f, 0, 2.f / 3.f, 1);
            layout.place(midiChannel, 3.f + 1.f / 3.f, 0, 2.f / 3.f, 1);
			layout.place(midiCC, 4.f, 0, 2.f / 3.f, 1);

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
			
            const auto& modeParam = *utils.getParam(PID::EnvGenMode);
            const auto modeVal = static_cast<int>(std::round(modeParam.getValModDenorm() - modeParam.range.start));

            bool isCCMode = modeVal == Mode::MIDICC;
			midiChannel.setVisible(isCCMode);
			midiCC.setVisible(isCCMode);

			auto _dcyRlsLinkEnabled = utils.getParam(PID::EnvGenLockDcyRls)->getValMod() > .5f;
            if (dcyRlsLinkEnabled != _dcyRlsLinkEnabled)
            {
				dcyRlsLinkEnabled = _dcyRlsLinkEnabled;
                switchDcyRlsParams(dcyRlsLinkEnabled);
                resized();
            }

            {
                bool _atkExceedsLatency = false;
                if (utils.getParam(PID::Lookahead)->getValMod() > .5f)
                {
                    const auto maxLatency = utils.audioProcessor.envGenMIDI.maxLatencySamples;
                    const auto atkTime = utils.audioProcessor.envGenMIDI.getAttackLength(0);
                    _atkExceedsLatency = atkTime > maxLatency;
                }
                if (atkExceedsLatency != _atkExceedsLatency)
                {
                    atkExceedsLatency = _atkExceedsLatency;
                    repaint();
                }
            }
        }

    protected:
        EnvGenComp envGen;
        Knob mode, lowerLimit, upperLimit, midiChannel, midiCC;
		Knob atk, sus, atkBeats;
        FlexKnob dcy, rls, dcyBeats, rlsBeats;
        Button lockDcyRls;
        Oscilloscope oscope;
        Knob legato;
        Button inverse, tempoSync;
        Knob velo;
        bool dcyRlsLinkEnabled, atkExceedsLatency;

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