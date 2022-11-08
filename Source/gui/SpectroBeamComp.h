#pragma once
#include "Comp.h"
#include "../audio/SpectroBeam.h"
#include "../audio/XenManager.h"
#include "../arch/Interpolation.h"
#include "../arch/Conversion.h"
#include <array>

namespace gui
{
	template<size_t Order>
	struct SpectroBeamComp :
		public Comp,
		public Timer
	{
		using SpecBeam = audio::SpectroBeam<Order>;
		static constexpr int Size = SpecBeam::Size;
		static constexpr float SizeF = static_cast<float>(Size);
		static constexpr float SizeInv = 1.f / SizeF;
		static constexpr float SizeFHalf = SizeF * .5f;

		SpectroBeamComp(Utils&, SpecBeam&);

		void paint(Graphics&) override;

		void timerCallback() override;
		
		ColourID mainColCID;
	protected:
		const audio::XenManager& xen;
		SpecBeam& beam;
		Image img;
	};
}