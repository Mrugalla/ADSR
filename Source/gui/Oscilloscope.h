#pragma once
#include "../audio/Oscilloscope.h"
#include "Button.h"

namespace gui
{
	struct Oscilloscope :
		public Comp,
		public Timer
	{
		using Oscope = audio::Oscilloscope;
		static constexpr int FPS = 24;

		Oscilloscope(Utils& u, String&& _tooltip, const Oscope& _oscope) :
			Comp(u, _tooltip, CursorType::Default),
			Timer(),
			oscope(_oscope),
			curve(),
			bipolar(true)
		{
			startTimerHz(FPS);
		}

		void resized() override
		{
			const auto thicc = utils.thicc;
			bounds = getLocalBounds().toFloat().reduced(thicc);

			curve = Path();
			curve.preallocateSpace(static_cast<int>(bounds.getWidth()) + 1);
		}

		void paint(Graphics& g) override
		{
			const auto thicc = utils.thicc;
			Stroke stroke(thicc, Stroke::JointStyle::beveled, Stroke::EndCapStyle::rounded);
			
			g.setColour(Colours::c(ColourID::Darken));
			g.fillRoundedRectangle(bounds, thicc);
			
			const auto data = oscope.data();
			const auto size = oscope.windowLength();
			const auto sizeF = static_cast<float>(size);
			const auto beatLength = oscope.getBeatLength();
			const auto w = bounds.getWidth();
			const auto h = bounds.getHeight();
			const auto xScale = w / std::min(beatLength, sizeF);
			const auto bipolarVal = bipolar ? 1.f : 0.f;
			const auto yScale = h - bipolarVal * (h * .5f - h);
			const auto xScaleInv = 1.f / xScale;
			const auto xOff = bounds.getX();
			const auto yOff = bounds.getY() + yScale;
			
			curve.clear();
			auto y = yOff - data[0] * yScale;
			curve.startNewSubPath(xOff, y);
			for (auto i = 1.f; i <= w; ++i)
			{
				const auto x = xOff + i;
				const auto idx = static_cast<int>(i * xScaleInv);
				y = yOff - data[idx] * yScale;
				curve.lineTo(x, y);
			}

			g.setColour(Colours::c(ColourID::Txt));
			g.strokePath(curve, stroke);
		}

		void timerCallback() override
		{
			repaint();
		}

	protected:
		const Oscope& oscope;
		BoundsF bounds;
		Path curve;
	public:
		bool bipolar;
	};
}

/*

todo:

performance: only repaint part if the interface that was changed

*/