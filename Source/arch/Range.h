#pragma once
#include "juce_core/juce_core.h"

namespace makeRange
{
	using Range = juce::NormalisableRange<float>;
	using String = juce::String;

	/* start, end, bias[-1, 1] */
	Range biased(float, float, float) noexcept;

	/* start, end, steps = 1 */
	Range stepped(float, float, float = 1.f) noexcept;
	
	Range toggle() noexcept;

	/* start, end */
	Range lin(float, float) noexcept;

	/* start, end, centre */
	Range withCentre(float, float, float) noexcept;

	/* min, max */
	Range foleysLogRange(float, float) noexcept;
	
	/* min, max, numSteps ]1, N] */
	Range quad(float, float, int) noexcept;
	
	/* start, end, withZero
	-4 = 1/16 beats
	-3 = 1/8 beats
	-2 = 1/4 beats
	-1 = 1/2 beats
	0 = 1 beats
	1 = 2 beats
	*/
	Range beats(int, int, bool = false) noexcept;

}