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

		static constexpr int NumParameters = 7;
		enum State { kAtk, kDcy, kSus, kRls, kAtkShape, kDcyShape, kRlsShape };

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
				skewNorm = std::tanh(skewNorm) * .5f + .5f;
				skewMod = std::tanh(skewMod) * .5f + .5f;
				
				const auto thicc = utils.thicc;
				const auto bnds = getLocalBounds().toFloat();
				const auto w = bnds.getWidth();
				const auto h = bnds.getHeight();
				const auto wInv = 1.f / w;
				
				pathNorm.clear();
				pathNorm.startNewSubPath({ 0.f, startY * h });
				for (auto i = thicc; i < w; i += thicc)
				{
					const auto x = EnvGen::getSkewed(i * wInv, skewNorm);
					const auto y = startY + x * (endY - startY);
					pathNorm.lineTo({ i, y * h });
				}
				pathNorm.lineTo({ w, endY * h });

				pathMod.clear();
				pathMod.startNewSubPath({ 0.f, startY * h });
				for (auto i = thicc; i < w; i += thicc)
				{
					const auto x = EnvGen::getSkewed(i * wInv, skewMod);
					const auto y = startY + x * (endY - startY);
					pathMod.lineTo({ i, y * h });
				}
				pathMod.lineTo({ w, endY * h });
			}

			void resized() override
			{
				Knob::resized();
				layout.place(*comps[ModDial], 0, 1, 1, 1, true);
				layout.place(*comps[LockButton], 0, 2, 1, 1, true);
			}
			
			Path pathNorm, pathMod;
			State state;
		};
		
		EnvGenComp(Utils& u, PID _atk, PID _dcy, PID _sus, PID _rls, PID _atkShape, PID _dcyShape, PID _rlsShape) :
			Comp(u, "Click here to interact with the envelope generator.", CursorType::Interact),
			pIDs{ _atk, _dcy, _sus, _rls, _atkShape, _dcyShape, _rlsShape },
			pVals(),
			stateComps
			{
				StateComp(u, "Attack", kAtk, _atkShape),
				StateComp(u, "Decay", kDcy, _dcyShape),
				StateComp(u, "Sustain", kSus, _sus),
				StateComp(u, "Release", kRls, _rlsShape)
			}
		{
			for (auto& v : pVals)
				v = 0.f;
			setInterceptsMouseClicks(true, true);

			for (auto& s : stateComps)
				addAndMakeVisible(s);

			startTimerHz(24);
		}

		void paint(Graphics& g) override
		{
			const auto thicc = utils.thicc;
			g.setColour(Colours::c(ColourID::Darken));
			g.fillRoundedRectangle(bounds, thicc);
		}
		
		void resized() override
		{
			const auto thicc = utils.thicc;
			bounds = getLocalBounds().toFloat().reduced(thicc * 2.f);
			update();
		}

		BoundsF bounds;
		std::array<PID, NumParameters> pIDs;
		std::array<float, NumParameters + 4> pVals;
		std::array<StateComp, 4> stateComps;

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
			
			for (auto i = 0; i < NumParameters; ++i)
			{
				const auto pID = pIDs[i];
				const auto& param = *utils.getParam(pID);
				const auto val = param.getValue();
				if (pVals[i] != val)
				{
					pVals[i] = val;
					e = true;
				}
			}

			for (auto i = NumParameters; i < NumParameters + 3; ++i)
			{
				const auto pID = pIDs[i - 3];
				const auto& param = *utils.getParam(pID);
				const auto val = param.getValMod();
				if (pVals[i] != val)
				{
					pVals[i] = val;
					e = true;
				}
			}

			{
				const auto pID = pIDs[kSus];
				const auto& param = *utils.getParam(pID);
				const auto val = param.getValMod();
				if (pVals.back() != val)
				{
					pVals[pVals.size() - 1] = val;
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
				const auto susMod = 1.f - pVals.back();

				stateComps[kAtk].setVisible(false);
				stateComps[kDcy].setVisible(false);
				stateComps[kSus].setVisible(true);
				stateComps[kRls].setVisible(false);
				stateComps[kSus].setBounds(bounds.toNearestInt());
				stateComps[kSus].update(sus, susMod, 0.f, 0.f);
				return;
			}
			else
			{
				stateComps[kSus].setVisible(false);
				
				const auto& atkShapeParam = *utils.getParam(pIDs[kAtkShape]);
				const auto atkShapeDenorm = atkShapeParam.range.convertFrom0to1(pVals[kAtkShape]);
				const auto atkShapeModDenorm = atkShapeParam.range.convertFrom0to1(pVals[kAtkShape + 3]);

				const auto& dcyShapeParam = *utils.getParam(pIDs[kDcyShape]);
				const auto dcyShapeDenorm = dcyShapeParam.range.convertFrom0to1(pVals[kDcyShape]);
				const auto dcyShapeModDenorm = dcyShapeParam.range.convertFrom0to1(pVals[kDcyShape + 3]);

				const auto& rlsShapeParam = *utils.getParam(pIDs[kRlsShape]);
				const auto rlsShapeDenorm = rlsShapeParam.range.convertFrom0to1(pVals[kRlsShape]);
				const auto rlsShapeModDenorm = rlsShapeParam.range.convertFrom0to1(pVals[kRlsShape + 3]);
				
				const auto timeInv = 1.f / time;

				const auto atkNorm = atkDenorm * timeInv;
				const auto dcyNorm = dcyDenorm * timeInv;
				const auto rlsNorm = rlsDenorm * timeInv;

				const auto width = getWidth();
				const auto widthF = static_cast<float>(width);

				const auto atkSize = atkNorm * widthF;
				const auto dcySize = dcyNorm * widthF;
				const auto rlsSize = rlsNorm * widthF;

				if (atkSize > 0.f)
				{
					stateComps[kAtk].setVisible(true);
					stateComps[kAtk].setBounds
					(BoundsF(
						bounds.getX(),
						bounds.getY(),
						atkSize,
						h
					).toNearestInt());
					stateComps[kAtk].update(1.f, 0.f, atkShapeDenorm, atkShapeModDenorm);
				}
				else
					stateComps[kAtk].setVisible(false);
				
				if (dcySize > 0.f)
				{
					stateComps[kDcy].setVisible(true);
					stateComps[kDcy].setBounds
					(BoundsF(
						static_cast<float>(stateComps[kAtk].getRight()),
						bounds.getY(),
						dcySize,
						h
					).toNearestInt());
					stateComps[kDcy].update(0.f, sus, dcyShapeDenorm, dcyShapeModDenorm);
				}
				else
					stateComps[kDcy].setVisible(false);
				
				if (rlsSize > 0.f)
				{
					stateComps[kRls].setVisible(true);
					stateComps[kRls].setBounds
					(BoundsF(
						static_cast<float>(stateComps[kDcy].getRight()),
						bounds.getY(),
						rlsSize,
						h
					).toNearestInt());
					stateComps[kRls].update(sus, 1.f, rlsShapeDenorm, rlsShapeModDenorm);
				}
				else
					stateComps[kRls].setVisible(false);
			}
		}
	};
}