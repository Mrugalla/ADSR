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
		
        struct ModeComp :
            public Comp
        {
			ModeComp(Utils& u) :
				Comp(u, "", CursorType::Default)
            {}

            void paint(Graphics& g) override
            {
                auto thicc = utils.thicc;
				auto bounds = getLocalBounds().toFloat().reduced(thicc);
				g.setColour(Colours::c(ColourID::Hover));
                g.setFont(getFontLobster());
                g.drawFittedText("Direct Out", bounds.toNearestInt(), Just::centred, 1);
            }
        };

        struct ModeCompGain :
            public ModeComp
        {
            ModeCompGain(Utils& u) :
                ModeComp(u),
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

			void paint(Graphics&) override {}

            void resized() override
            {
                layout.resized();

                layout.place(lowerLim, 0, 0, 1, 1, false);
				layout.place(upperLim, 1, 0, 1, 1, false);
            }

            Knob lowerLim, upperLim;
        };

        struct ModeCompFilter :
            public ModeComp
        {
            ModeCompFilter(Utils& u) :
                ModeComp(u),
                type(u),
                freq(u),
                q(u),
                range(u)
            {
                addAndMakeVisible(type);
                addAndMakeVisible(freq);
                addAndMakeVisible(q);
				addAndMakeVisible(range);

                makeParameter(type, PID::ModeFilterType, "Type");
                makeParameter(freq, PID::ModeFilterCutoff, "Pitch");
				makeParameter(q, PID::ModeFilterQ, "Q");
				makeParameter(range, PID::ModeFilterRange, "Range");

                layout.init
                (
                    { 1, 1, 1, 1 },
                    { 1 }
                );
            }

            void paint(Graphics&) override {}

            void resized() override
            {
                layout.resized();

                layout.place(type, 0, 0, 1, 1, false);
                layout.place(freq, 1, 0, 1, 1, false);
                layout.place(q, 2, 0, 1, 1, false);
                layout.place(range, 3, 0, 1, 1, false);
            }

            Knob type, freq, q, range;
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
			modeComp(),
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
            {
                mode.makeParameter(PID::EnvGenMode);
                mode.vertical = false;
				
                const auto& modeParam = *u.getParam(PID::EnvGenMode);
                const auto valDenorm = static_cast<int>(std::round(modeParam.getValModDenorm() - modeParam.range.start));

                switch (valDenorm)
                {
                case 0:
                    modeComp = std::make_unique<ModeComp>(u);
                    break;
                case 1:
                    modeComp = std::make_unique<ModeCompGain>(u);
                    break;
				case 2:
					modeComp = std::make_unique<ModeCompFilter>(u);
					break;
                default:
                    modeComp = std::make_unique<ModeComp>(u);
                    break;
                }

                addAndMakeVisible(*modeComp);
                mode.onChange.push_back([&]()
                {
                    enum { DirectOut, Gain, Filter, NumModes };
                    const auto& modeParam = *u.getParam(PID::EnvGenMode);
					const auto valDenorm = static_cast<int>(std::round(modeParam.getValModDenorm() - modeParam.range.start));
                    
                    removeChildComponent(modeComp.get());

                    switch (valDenorm)
                    {
                    case 0:
                        modeComp = std::make_unique<ModeComp>(u);
                        break;
                    case 1:
                        modeComp = std::make_unique<ModeCompGain>(u);
                        break;
					case 2:
						modeComp = std::make_unique<ModeCompFilter>(u);
						break;
                    default:
                        modeComp = std::make_unique<ModeComp>(u);
                        break;
                    }

                    addAndMakeVisible(*modeComp);
                    resized();
                });
            }

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
                { 2, 3, 8, 3, 1, 3, 3 }
            );

            startTimerHz(12);
        }

        void paint(Graphics& g) override
        {
            const auto thicc = utils.thicc;
			Stroke stroke(thicc, Stroke::JointStyle::curved, Stroke::EndCapStyle::butt);

            g.setColour(Colours::c(ColourID::Hover));

            auto modeBounds = mode.getBounds().toFloat();
			auto modeCompBounds = modeComp->getBounds().toFloat();
            BoundsF modeBothBounds
            (
                modeBounds.getX(),
                modeBounds.getY(),
				modeBounds.getWidth(),
				modeBounds.getHeight() + modeCompBounds.getHeight()
            );
			
			drawRectEdges(g, modeBothBounds, thicc * 5.f, stroke);

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

            layout.place(mode, 1, 0, 4, 1);
			layout.place(*modeComp, 1, 1, 4, 1);

            layout.place(envGen, 1, 2, 4, 1);

            layout.place(atk, 1, 3, 1, 1);
			layout.place(*dcy, 2, 3, 1, 1);
			layout.place(sus, 3, 3, 1, 1);
			layout.place(*rls, 4, 3, 1, 1);

            atkBeats.setBounds(atk.getBounds());
			dcyBeats->setBounds(dcy->getBounds());
            rlsBeats->setBounds(rls->getBounds());
			
			layout.place(lockDcyRls, 3, 4, 1, 1, true);

            const auto thicc = utils.thicc;
            const auto thicc5 = thicc * 5.f;
            oscope.setBounds(layout(1, 5, 4, 1).reduced(thicc5).toNearestInt());

            layout.place(legato, 1, 6, 1, 1, false);
            layout.place(inverse, 2, 6, 1, 1, true);
			layout.place(tempoSync, 3, 6, 1, 1, true);
            layout.place(velo, 4, 6, 1, 1, false);
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
        }

    protected:
        EnvGenComp envGen;
        RadioButton mode;
        std::unique_ptr<ModeComp> modeComp;
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