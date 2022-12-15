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
			kAtkShape, kDcyShape, kRlsShape,
			kAtkBeats, kDcyBeats, kRlsBeats,
			kTempoSync, kDcyRlsLocked,
			NumParameters,
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
				const auto thicc2 = thicc * 2.f;
				const auto bnds = getLocalBounds().toFloat();
				const auto w = bnds.getWidth();
				
				const auto pathlen = static_cast<int>(w / thicc2) + 1;
				pathMod.preallocateSpace(pathlen);
				pathNorm.preallocateSpace(pathlen);
			}
			
			Path pathNorm, pathMod;
			State state;
		};
		
		EnvGenComp(Utils& u, PID _atk, PID _dcy, PID _sus, PID _rls,
			PID _atkShape, PID _dcyShape, PID _rlsShape,
			PID _atkBeats, PID _dcyBeats, PID _rlsBeats, PID _tempoSync, PID _dcyRlsLocked) :
			Comp(u, "Click here to interact with the envelope generator.", CursorType::Default),
			Timer(),
			bounds(),
			pIDs{ _atk, _dcy, _sus, _rls, _atkShape, _dcyShape, _rlsShape, _atkBeats, _dcyBeats, _rlsBeats, _tempoSync, _dcyRlsLocked },
			pVals(),
			stateComps
			{
				StateComp(u, "Attack", kAtk, _atkShape),
				StateComp(u, "Decay", kDcy, _dcyShape),
				StateComp(u, "Sustain", kSus, _sus),
				StateComp(u, "Release", kRls, _rlsShape)
			},
			atk(), dcy(), rls(),
			tempoSyncEnabled(u.getParam(_tempoSync)->getValMod() > .5f),
			lockDcyRlsEnabled(u.getParam(_dcyRlsLocked)->getValMod() > .5f)
		{
			for (auto& v : pVals)
				v = 0.f;
			
			for (auto& s : stateComps)
				addAndMakeVisible(s);
			
			switchTimeParameters(tempoSyncEnabled, lockDcyRlsEnabled);

			layout.init
			(
				{ 3, 34, 5, 3 },
				{ 3, 13, 13, 21, 34, 3 }
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
			
			const auto timeParamsWidth = static_cast<int>(std::min(layout.getW(0), layout.getH(0)) * 2);
			atk->setBounds(0, 0, timeParamsWidth, timeParamsWidth);
			dcy->setBounds(0, 0, timeParamsWidth, timeParamsWidth);
			rls->setBounds(0, 0, timeParamsWidth, timeParamsWidth);

			update();
		}
		
		void timerCallback() override
		{
			const auto ts = utils.getParam(pIDs[kTempoSync])->getValMod() > .5f;
			const auto ldr = utils.getParam(pIDs[kDcyRlsLocked])->getValMod() > .5f;
			if (tempoSyncEnabled != ts || lockDcyRlsEnabled != ldr)
			{
				tempoSyncEnabled = ts;
				lockDcyRlsEnabled = ldr;
				switchTimeParameters(tempoSyncEnabled, lockDcyRlsEnabled);
				resized();
			}

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
				auto kIdx = tempoSyncEnabled ? kAtkBeats : kAtk;
				auto pID = pIDs[kIdx];
				auto param = utils.getParam(pID);
				auto val = param->getValue();
				if (pVals[kIdx] != val)
				{
					pVals[kIdx] = val;
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
				auto kIdx = tempoSyncEnabled ? kDcyBeats : kDcy;
				auto pID = pIDs[kIdx];
				auto param = utils.getParam(pID);
				auto val = param->getValue();
				if (pVals[kIdx] != val)
				{
					pVals[kIdx] = val;
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
				auto kIdx = tempoSyncEnabled ? kRlsBeats : kRls;
				auto pID = pIDs[kIdx];
				auto param = utils.getParam(pID);
				auto val = param->getValue();
				if (pVals[kIdx] != val)
				{
					pVals[kIdx] = val;
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
			
			const auto kIdxAtk = tempoSyncEnabled ? kAtkBeats : kAtk;
			const auto& atkParam = *utils.getParam(pIDs[kIdxAtk]);
			const auto atkDenorm = atkParam.range.convertFrom0to1(pVals[kIdxAtk]);

			const auto kIdxDcy = tempoSyncEnabled ? kDcyBeats : kDcy;
			const auto& dcyParam = *utils.getParam(pIDs[kIdxDcy]);
			const auto dcyDenorm = dcyParam.range.convertFrom0to1(pVals[kIdxDcy]);

			const auto kIdxRls = tempoSyncEnabled ? kRlsBeats : kRls;
			const auto& rlsParam = *utils.getParam(pIDs[kIdxRls]);
			const auto rlsDenorm = rlsParam.range.convertFrom0to1(pVals[kIdxRls]);

			const auto sus = 1.f - pVals[kSus];
			
			const auto noteOnTime = atkDenorm + dcyDenorm;
			const auto noteOffTime = rlsDenorm;
			const auto time = noteOnTime + noteOffTime;
			if (time == 0.f)
			{
				stateComps[kAtk].setVisible(false);
				stateComps[kDcy].setVisible(false);
				stateComps[kSus].setVisible(true);
				stateComps[kRls].setVisible(false);
				stateComps[kSus].setBounds(bounds.toNearestInt());
				stateComps[kSus].update(sus, sus, 0.f, 0.f);
				atk->setVisible(false);
				dcy->setVisible(false);
				rls->setVisible(false);
				return;
			}
			else
			{
				stateComps[kSus].setVisible(false);
				atk->setVisible(true);
				dcy->setVisible(true);
				rls->setVisible(true);
				
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
					
					atk->setCentrePosition(stateComps[kAtk].getBounds().getTopRight());
				}
				else
				{
					stateComps[kAtk].setVisible(false);
					atk->setCentrePosition(static_cast<int>(bX), static_cast<int>(bY));
				}
				if (dcySize > 0.f)
				{
					const auto& dcyShapeParam = *utils.getParam(pIDs[kDcyShape]);
					const auto dcyShapeDenorm = dcyShapeParam.range.convertFrom0to1(pVals[kDcyShape]);
					const auto dcyShapeModDenorm = dcyShapeParam.range.convertFrom0to1(pVals[kDcyShapeMod]);

					stateComps[kDcy].setVisible(true);
					stateComps[kDcy].setBounds(BoundsF(bX + atkSize, bY, dcySize, h).toNearestInt());
					stateComps[kDcy].update(0.f, sus, dcyShapeDenorm, dcyShapeModDenorm);

					dcy->setCentrePosition({ stateComps[kDcy].getRight(), static_cast<int>(bY + sus * h)});
				}
				else
				{
					stateComps[kDcy].setVisible(false);
					dcy->setCentrePosition(static_cast<int>(bX + atkSize), static_cast<int>(bY + sus * h));
				}
				if (rlsSize > 0.f)
				{
					const auto& rlsShapeParam = *utils.getParam(pIDs[kRlsShape]);
					const auto rlsShapeDenorm = rlsShapeParam.range.convertFrom0to1(pVals[kRlsShape]);
					const auto rlsShapeModDenorm = rlsShapeParam.range.convertFrom0to1(pVals[kRlsShapeMod]);

					stateComps[kRls].setVisible(true);
					stateComps[kRls].setBounds(BoundsF(bX + atkSize + dcySize, bY, rlsSize, h).toNearestInt());
					stateComps[kRls].update(sus, 1.f, rlsShapeDenorm, rlsShapeModDenorm);

					rls->setCentrePosition(stateComps[kRls].getBounds().getBottomRight());
				}
				else
					stateComps[kRls].setVisible(false);
			}
		}

		BoundsF bounds;
		std::array<PID, NumParameters> pIDs;
		std::array<float, NumParamsPlus> pVals;
		std::array<StateComp, 4> stateComps;
		std::unique_ptr<Knob> atk, dcy, rls;
		bool tempoSyncEnabled, lockDcyRlsEnabled;
		
		void switchTimeParameters(bool tempoSync, bool dcyRlsLocked)
		{
			removeChildComponent(atk.get());
			removeChildComponent(dcy.get());
			removeChildComponent(rls.get());

			atk = std::make_unique<Knob>(utils);
			dcy = std::make_unique<Knob>(utils);
			rls = std::make_unique<Knob>(utils);
			
			if (tempoSync)
			{
				makeParameter(*atk, pIDs[kAtkBeats], "", false, nullptr, Knob::LooksType::Knot);
				if (dcyRlsLocked)
				{
					
					makeParameter(*dcy, { pIDs[kDcyBeats], pIDs[kRlsBeats] }, pIDs[kSus]);
					makeParameter(*rls, { pIDs[kRlsBeats], pIDs[kDcyBeats] }, "", false, nullptr, Knob::LooksType::Knot);
				}
				else
				{
					makeParameter(*dcy, pIDs[kDcyBeats], pIDs[kSus]);
					makeParameter(*rls, pIDs[kRlsBeats], "", false, nullptr, Knob::LooksType::Knot);
				}
				
			}
			else
			{
				makeParameter(*atk, pIDs[kAtk], "", false, nullptr, Knob::LooksType::Knot);
				if (dcyRlsLocked)
				{
					makeParameter(*dcy, { pIDs[kDcy], pIDs[kRls] }, pIDs[kSus]);
					makeParameter(*rls, { pIDs[kRls], pIDs[kDcy] }, "", false, nullptr, Knob::LooksType::Knot);
				}
				else
				{
					makeParameter(*dcy, pIDs[kDcy], pIDs[kSus]);
					makeParameter(*rls, pIDs[kRls], "", false, nullptr, Knob::LooksType::Knot);
				}
			}

			atk->dragMode = Knob::DragMode::Horizontal;
			dcy->dragMode = Knob::DragMode::Both;
			rls->dragMode = Knob::DragMode::Horizontal;
			
			addAndMakeVisible(*atk);
			addAndMakeVisible(*dcy);
			addAndMakeVisible(*rls);
		}
	};
}

/*

*/