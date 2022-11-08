#pragma once
#include "juce_core/juce_core.h"

namespace makeRange
{
	using Range = juce::NormalisableRange<float>;

	/* start, end, bias[-1, 1] */
	Range biased(float, float, float) noexcept;

	/* start, end, steps = 1 */
	Range stepped(float, float, float = 1.f) noexcept;
	
	Range toggle() noexcept;

	/* start, end */
	Range lin(float, float) noexcept;

	/* start, end, centre */
	Range withCentre(float, float, float) noexcept;

}