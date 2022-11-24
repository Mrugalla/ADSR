#pragma once
#include "Knob.h"
#include "../audio/EnvelopeGenerator.h"

namespace gui
{
	struct EnvGenComp :
		public Comp,
		public Timer
	{
		using MIDI = juce::MidiBuffer;
		using EnvGen = audio::EnvGenMIDI;

		enum State
		{
			kAtk, kDcy, kSus, kRls,
			kAtkShape, kDcyShape, kRlsShape, NumParameters,
			kAtkShapeMod, kDcyShapeMod, kRlsShapeMod,
			NumParamsPlus
		};

		struct StateComp :
			public Knob
		{
			enum { Value, MaxModDepth, ValMod, ModBias, Meter, NumValues };
			enum { ModDial, LockButton, NumComps };
			
			StateComp(Utils& u, String&& _name, State _state, PID pIDShape) :
				Knob(u),
				pathNorm(), pathMod(),
				state(_state)
			{
				makeParameter(*this, pIDShape, std::move(_name), true);
				label.setVisible(false);
			}

			void paint(Graphics& g) override
			{
				const auto thicc = utils.thicc;

				if (isMouseOverOrDragging())
				{
					g.setColour(Colours::c(ColourID::Hover));
					g.fillAll();
				}
				if (!pathMod.isEmpty())
				{
					g.setColour(Colours::c(ColourID::Mod));
					g.strokePath(pathMod, Stroke(thicc));
				}
				if (!pathNorm.isEmpty())
				{
					g.setColour(Colours::c(ColourID::Interact));
					g.strokePath(pathNorm, Stroke(thicc));
				}
			}
			
			void mouseEnter(const Mouse& mouse) override
			{
				Knob::mouseEnter(mouse);
				repaint();
			}

			void mouseExit(const Mouse& mouse) override
			{
				Knob::mouseExit(mouse);
				repaint();
			}

			void update(float startY, float endY, float skewNorm, float skewMod)
			{
				if (state != kAtk)
				{
					skewNorm *= -1.f;
					skewMod *= -1.f;
				}
				skewNorm = std::tanh(Pi * skewNorm) * .5f + .5f;
				skewMod = std::tanh(Pi * skewMod) * .5f + .5f;
				
				const auto thicc = utils.thicc;
				const auto thicc2 = thicc * 2.f;
				const auto bnds = getLocalBounds().toFloat();
				const auto w = bnds.getWidth();
				const auto h = bnds.getHeight();
				const auto wInv = 1.f / w;
				const auto rangeY = endY - startY;
				
				pathNorm.clear();
				pathNorm.startNewSubPath({ 0.f, startY * h });
				for (auto i = thicc2; i < w; i += thicc2)
				{
					const auto x = EnvGen::getSkewed(i * wInv, skewNorm);
					const auto y = startY + x * rangeY;
					pathNorm.lineTo({ i, y * h });
				}
				pathNorm.lineTo({ w, endY * h });

				pathMod.clear();
				pathMod.startNewSubPath({ 0.f, startY * h });
				for (auto i = thicc2; i < w; i += thicc2)
				{
					const auto x = EnvGen::getSkewed(i * wInv, skewMod);
					const auto y = startY + x * rangeY;
					pathMod.lineTo({ i, y * h });
				}
				pathMod.lineTo({ w, endY * h });
			}

			void resized() override
			{
				Knob::resized();
				layout.place(*comps[ModDial], 0, 1, 1, 1, true);
				layout.place(*comps[LockButton], 0, 2, 1, 1, true);

				const auto thicc = utils.thicc;
				const auto bnds = getLocalBounds().toFloat();
				const auto w = bnds.getWidth();
				
				auto pathlen = static_cast<int>(w / thicc) + 1;
				pathMod.preallocateSpace(pathlen);
				pathNorm.preallocateSpace(pathlen);
			}
			
			Path pathNorm, pathMod;
			State state;
		};
		
		EnvGenComp(Utils& u, PID _atk, PID _dcy, PID _sus, PID _rls, PID _atkShape, PID _dcyShape, PID _rlsShape) :
			Comp(u, "Click here to interact with the envelope generator.", CursorType::Default),
			bounds(),
			pIDs{ _atk, _dcy, _sus, _rls, _atkShape, _dcyShape, _rlsShape },
			pVals(),
			stateComps
			{
				StateComp(u, "Attack", kAtk, _atkShape),
				StateComp(u, "Decay", kDcy, _dcyShape),
				StateComp(u, "Sustain", kSus, _sus),
				StateComp(u, "Release", kRls, _rlsShape)
			},
			legato(u),
			inverse(u),
			velo(u),

			atk(u),
			dcy(u),
			rls(u)
		{
			for (auto& v : pVals)
				v = 0.f;
			
			for (auto& s : stateComps)
				addAndMakeVisible(s);
			
			makeParameter(legato, PID::EnvGenLegato, ButtonSymbol::Legato);
			addAndMakeVisible(legato);

			makeParameter(inverse, PID::EnvGenInverse, "Inverse", true);
			addAndMakeVisible(inverse);

			makeParameter(velo, PID::EnvGenVelocity, "Velo");
			addAndMakeVisible(velo);

			makeParameter(atk, PID::EnvGenAttack, "", false, nullptr, Knob::LooksType::Knot);
			addAndMakeVisible(atk);
			atk.dragMode = Knob::DragMode::Horizontal;

			makeParameter(dcy, PID::EnvGenDecay, PID::EnvGenSustain);
			addAndMakeVisible(dcy);
			dcy.dragMode = Knob::DragMode::Both;

			makeParameter(rls, PID::EnvGenRelease, "", false, nullptr, Knob::LooksType::Knot);
			addAndMakeVisible(rls);
			rls.dragMode = Knob::DragMode::Horizontal;

			layout.init
			(
				{ 2, 21, 3, 2 },
				{ 2, 8, 8, 13, 21, 2 }
			);

			startTimerHz(PPDFPSKnobs);
		}

		void paint(Graphics& g) override
		{
			const auto thicc = utils.thicc;
			g.setColour(Colours::c(ColourID::Darken));
			g.fillRoundedRectangle(bounds, thicc);
		}
		
		void resized() override
		{
			layout.resized();

			bounds = layout(1, 1, 2, 4, false);
			
			layout.place(legato, 2, 1, 1, 1, false);
			layout.place(inverse, 2, 2, 1, 1, false);
			layout.place(velo, 2, 3, 1, 1, false);

			const auto timeParamsWidth = static_cast<int>(std::min(layout.getW(0), layout.getH(0)) * 2);
			atk.setBounds(0, 0, timeParamsWidth, timeParamsWidth);
			dcy.setBounds(0, 0, timeParamsWidth, timeParamsWidth);
			rls.setBounds(0, 0, timeParamsWidth, timeParamsWidth);

			update();
		}
		
		void timerCallback() override
		{
			if (needsUpdate())
			{
				update();
				repaint();
			}
		}

		bool needsUpdate() noexcept
		{
			bool e = false;
			
			{ // ATTACK
				auto pID = pIDs[kAtk];
				auto param = utils.getParam(pID);
				auto val = param->getValue();
				if (pVals[kAtk] != val)
				{
					pVals[kAtk] = val;
					e = true;
				}
				const auto iShape = kAtkShape;
				pID = pIDs[iShape];
				param = utils.getParam(pID);
				val = param->getValue();
				const auto valMod = param->getValMod();
				const auto iMod = kAtkShapeMod;
				if (pVals[iShape] != val || pVals[iMod] != valMod)
				{
					pVals[iShape] = val;
					pVals[iMod] = valMod;
					e = true;
				}
			}
			{ // DECAY
				auto pID = pIDs[kDcy];
				auto param = utils.getParam(pID);
				auto val = param->getValue();
				if (pVals[kDcy] != val)
				{
					pVals[kDcy] = val;
					e = true;
				}
				const auto iShape = kDcyShape;
				pID = pIDs[iShape];
				param = utils.getParam(pID);
				val = param->getValue();
				const auto valMod = param->getValMod();
				const auto iMod = kDcyShapeMod;
				if (pVals[iShape] != val || pVals[iMod] != valMod)
				{
					pVals[iShape] = val;
					pVals[iMod] = valMod;
					e = true;
				}
			}
			{ // RELEASE
				auto pID = pIDs[kRls];
				auto param = utils.getParam(pID);
				auto val = param->getValue();
				if (pVals[kRls] != val)
				{
					pVals[kRls] = val;
					e = true;
				}
				const auto iShape = kRlsShape;
				pID = pIDs[iShape];
				param = utils.getParam(pID);
				val = param->getValue();
				const auto valMod = param->getValMod();
				const auto iMod = kRlsShapeMod;
				if (pVals[iShape] != val || pVals[iMod] != valMod)
				{
					pVals[iShape] = val;
					pVals[iMod] = valMod;
					e = true;
				}
			}
			
			{ // SUSTAIN
				auto pID = pIDs[kSus];
				auto param = utils.getParam(pID);
				auto val = param->getValue();
				if (pVals[kSus] != val)
				{
					pVals[kSus] = val;
					e = true;
				}
			}
			
			return e;
		}

		void update()
		{
			const auto h = bounds.getHeight();
			
			const auto& atkParam = *utils.getParam(pIDs[kAtk]);
			const auto atkDenorm = atkParam.range.convertFrom0to1(pVals[kAtk]);

			const auto& dcyParam = *utils.getParam(pIDs[kDcy]);
			const auto dcyDenorm = dcyParam.range.convertFrom0to1(pVals[kDcy]);

			const auto& rlsParam = *utils.getParam(pIDs[kRls]);
			const auto rlsDenorm = rlsParam.range.convertFrom0to1(pVals[kRls]);

			const auto sus = 1.f - pVals[kSus];
			
			const auto noteOnTime = atkDenorm + dcyDenorm;
			const auto noteOffTime = rlsDenorm;
			const auto time = noteOnTime + noteOffTime;
			if (time < 1.f)
			{
				stateComps[kAtk].setVisible(false);
				stateComps[kDcy].setVisible(false);
				stateComps[kSus].setVisible(true);
				stateComps[kRls].setVisible(false);
				stateComps[kSus].setBounds(bounds.toNearestInt());
				stateComps[kSus].update(sus, sus, 0.f, 0.f);
				atk.setVisible(false);
				dcy.setVisible(false);
				rls.setVisible(false);
				return;
			}
			else
			{
				stateComps[kSus].setVisible(false);
				atk.setVisible(true);
				dcy.setVisible(true);
				rls.setVisible(true);
				
				const auto timeInv = 1.f / time;

				const auto atkNorm = atkDenorm * timeInv;
				const auto dcyNorm = dcyDenorm * timeInv;
				const auto rlsNorm = rlsDenorm * timeInv;

				const auto widthF = bounds.getWidth();
				const auto bX = bounds.getX();
				const auto bY = bounds.getY();

				const auto atkSize = atkNorm * widthF;
				const auto dcySize = dcyNorm * widthF;
				const auto rlsSize = rlsNorm * widthF;

				if (atkSize > 0.f)
				{
					const auto& atkShapeParam = *utils.getParam(pIDs[kAtkShape]);
					const auto atkShapeDenorm = atkShapeParam.range.convertFrom0to1(pVals[kAtkShape]);
					const auto atkShapeModDenorm = atkShapeParam.range.convertFrom0to1(pVals[kAtkShapeMod]);

					stateComps[kAtk].setVisible(true);
					stateComps[kAtk].setBounds(BoundsF(bX, bY, atkSize, h).toNearestInt());
					stateComps[kAtk].update(1.f, 0.f, atkShapeDenorm, atkShapeModDenorm);
					
					atk.setCentrePosition(stateComps[kAtk].getBounds().getTopRight());
				}
				else
					stateComps[kAtk].setVisible(false);
				
				if (dcySize > 0.f)
				{
					const auto& dcyShapeParam = *utils.getParam(pIDs[kDcyShape]);
					const auto dcyShapeDenorm = dcyShapeParam.range.convertFrom0to1(pVals[kDcyShape]);
					const auto dcyShapeModDenorm = dcyShapeParam.range.convertFrom0to1(pVals[kDcyShapeMod]);

					stateComps[kDcy].setVisible(true);
					stateComps[kDcy].setBounds(BoundsF(bX + atkSize, bY, dcySize, h).toNearestInt());
					stateComps[kDcy].update(0.f, sus, dcyShapeDenorm, dcyShapeModDenorm);

					dcy.setCentrePosition({ stateComps[kDcy].getRight(), static_cast<int>(bY + sus * h)});
				}
				else
					stateComps[kDcy].setVisible(false);
				
				if (rlsSize > 0.f)
				{
					const auto& rlsShapeParam = *utils.getParam(pIDs[kRlsShape]);
					const auto rlsShapeDenorm = rlsShapeParam.range.convertFrom0to1(pVals[kRlsShape]);
					const auto rlsShapeModDenorm = rlsShapeParam.range.convertFrom0to1(pVals[kRlsShapeMod]);

					stateComps[kRls].setVisible(true);
					stateComps[kRls].setBounds(BoundsF(bX + atkSize + dcySize, bY, rlsSize, h).toNearestInt());
					stateComps[kRls].update(sus, 1.f, rlsShapeDenorm, rlsShapeModDenorm);

					rls.setCentrePosition(stateComps[kRls].getBounds().getBottomRight());
				}
				else
					stateComps[kRls].setVisible(false);
			}
		}

		BoundsF bounds;
		std::array<PID, NumParameters> pIDs;
		std::array<float, NumParamsPlus> pVals;
		std::array<StateComp, 4> stateComps;
		Button legato, inverse;
		Knob velo;

		Knob atk, dcy, rls;
	};
}

/*

todo:

legato modulatable?

make oscilloscope window

make symbol for inverse

*/