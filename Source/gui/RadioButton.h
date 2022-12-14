#pragma once
#include "Button.h"

namespace gui
{
	struct RadioButton :
		public Comp
	{
		using FlexButton = std::unique_ptr<Button>;
		using Buttons = std::vector<FlexButton>;
		
		RadioButton(Utils& u) :
			Comp(u, "", CursorType::Default),
			buttons(),
			hasLock(false),
			vertical(false)
		{}

		void makeParameter(PID pID)
		{
			setTooltip(param::toTooltip(pID));
			auto& param = *utils.getParam(pID);
			const auto& range = param.range;
			const auto start = range.start;
			const auto end = range.end;
			const auto numValues = static_cast<int>(1 + end - start);

			buttons.reserve(numValues + 1);
			for (auto i = 0; i < numValues; ++i)
			{
				const auto val = range.convertTo0to1(static_cast<float>(i) + range.start);
				
				buttons.emplace_back(std::make_unique<Button>(utils));
				auto& btn = *buttons.back();
				makeToggleButton(btn, param.getText(val, 24));
				btn.tooltip = tooltip;
				
				btn.onClick[0] = [this, i, pID, val](Button&, const Mouse& mouse)
				{
					if (mouse.mouseWasDraggedSinceMouseDown())
						return;
					auto& param = *utils.getParam(pID);
					param.setValueWithGesture(val);
				};
				btn.onMouseWheel.push_back([this, pID](Button&, const Mouse&, const MouseWheel& wheel)
				{
					auto& param = *utils.getParam(pID);
					auto& range = param.range;
					const auto start = range.start;
					const auto end = range.end;
					const auto numValues = 1.f + end - start;
					const auto deltaY = wheel.deltaY > 0.f ? 1.f : -1.f;
					auto val = param.getValueDenorm();
					val += deltaY * range.interval;
					if (val < range.start)
						val += numValues;
					else if (val > range.end)
						val -= numValues;
					param.setValueWithGesture(range.convertTo0to1(val));
				});

				addAndMakeVisible(btn);
			}
			
			auto& front = *buttons.front();
			front.onTimer.push_back([this, pID](Button&)
			{
				bool needsRepaint = false;

				const auto& param = *utils.getParam(pID);
				
				auto& lockBtn = *buttons.back();
				auto locked = param.isLocked() ? 1 : 0;
				if (locked != lockBtn.toggleState)
				{
					lockBtn.toggleState = locked;
					auto alpha = 1.f - static_cast<float>(locked) * .8f;
					setAlpha(alpha);
					needsRepaint = true;
				}
				
				const auto valDenorm = param.getValModDenorm() - param.range.start;
				const auto index = static_cast<int>(std::round(valDenorm));
				if (setSelected(index))
					needsRepaint = true;

				if (needsRepaint)
				{
					for (auto& oc : onChange)
						oc();

					repaintWithChildren(this);
				}
			});
			
			hasLock = true;
			buttons.emplace_back(std::make_unique<Button>(utils));
			auto& lockBtn = *buttons.back();
			makeToggleButton(lockBtn, "L");
			lockBtn.toggleState = param.isLocked();
			lockBtn.onClick[0] = [this, pID](Button&, const Mouse&)
			{
				auto& param = *utils.getParam(pID);
				param.switchLock();
			};
			addAndMakeVisible(lockBtn);

			{
				const auto valDenorm = param.getValueDenorm() - param.range.start;
				const auto index = static_cast<int>(valDenorm);
				setSelected(index);
				front.startTimerHz(12);
			}
		}
		
		void paint(Graphics&) override {}

		void resized() override
		{
			const auto numButtons = buttons.size();
			if (numButtons == 0)
				return;
			
			const auto numButtonsF = static_cast<float>(numButtons);
			const auto numButtonsInv = 1.f / numButtonsF;

			const auto thicc = utils.thicc;
			const auto bounds = getLocalBounds().toFloat().reduced(thicc);

			if (vertical)
			{
				const auto x = bounds.getX();
				const auto w = bounds.getWidth();
				const auto h = bounds.getHeight() * numButtonsInv;
				auto y = bounds.getY();

				for (auto& btn : buttons)
				{
					btn->setBounds(BoundsF(x, y, w, h).toNearestInt());
					y += h;
				}
			}
			else
			{
				const auto y = bounds.getY();
				const auto h = bounds.getHeight();
				const auto w = bounds.getWidth() * numButtonsInv;
				auto x = bounds.getX();

				for (auto& btn : buttons)
				{
					btn->setBounds(BoundsF(x, y, w, h).toNearestInt());
					x += w;
				}
			}

			if (hasLock)
			{
				auto& lockBtn = *buttons.back();
				lockBtn.setBounds
				(
					maxQuadIn(lockBtn.getBounds().toFloat()).toNearestInt()
				);
			}
		}
		
		std::vector<std::function<void()>> onChange;
	protected:
		Buttons buttons;
		bool hasLock;
	
		int getSelected() const noexcept
		{
			auto end = static_cast<int>(buttons.size()) - (hasLock ? 1 : 0);

			for (auto i = 0; i < end; ++i)
			{
				if (buttons[i]->toggleState == 1)
					return i;
			}
			return -1;
		}

		bool setSelected(int i) noexcept
		{
			auto selected = getSelected();
			if (i == selected)
				return false;

			auto end = static_cast<int>(buttons.size()) - (hasLock ? 1 : 0);

			for (auto j = 0; j < end; ++j)
				buttons[j]->toggleState = 0;

			buttons[i]->toggleState = 1;

			return true;
		}

	public:
		bool vertical;
	};
}