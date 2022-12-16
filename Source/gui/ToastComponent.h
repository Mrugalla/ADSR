#pragma once
#include "Label.h"

namespace gui
{
	class ToastComp :
		public Label
	{
		Notify makeNotify(ToastComp& t)
		{
			return [&toast = t](EvtType type, const void* stuff)
			{
				if (type == EvtType::Toast)
				{
					toast.updateBounds();
					
					const auto str = static_cast<const String*>(stuff);
					toast.setText(*str);
					toast.resized();

					toast.setVisible(true);
				}
				else if (type == EvtType::ClickedEmpty)
				{
					toast.setVisible(false);
				}
			};
		}
	public:
		ToastComp(Utils& u) :
			Label(u, "", makeNotify(*this))
		{
			font = getFontLobster();
			mode = Mode::TextToLabelBounds;
			textCID = ColourID::Txt;
			tooltip = "Click somewhere else to close the toast.";

			setInterceptsMouseClicks(true, false);
		}

		void paint(Graphics& g) override
		{
			g.setColour(Colours::c(ColourID::Bg));
			g.fillAll();
			Label::paint(g);
		}

		void updateBounds()
		{
			const auto parent = getParentComponent()->getLocalBounds().toFloat();
			const auto w = parent.getWidth();
			const auto x = 0.f;
			const auto h = parent.getHeight() * .5f;
			const auto y = (parent.getHeight() - h) * .5f;
			const BoundsF nBounds(x, y, w, h);
			setBounds(nBounds.toNearestInt());
		}
	};
}