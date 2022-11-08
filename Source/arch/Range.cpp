#include "Range.h"
//#include "Conversion.h"

namespace makeRange
{
	Range biased(float start, float end, float bias) noexcept
	{
		// https://www.desmos.com/calculator/ps8q8gftcr
		const auto a = bias * .5f + .5f;
		const auto a2 = 2.f * a;
		const auto aM = 1.f - a;

		const auto r = end - start;
		const auto aR = r * a;
		if (bias != 0.f)
			return
		{
				start, end,
				[a2, aM, aR](float min, float, float x)
				{
					const auto denom = aM - x + a2 * x;
					if (denom == 0.f)
						return min;
					return min + aR * x / denom;
				},
				[a2, aM, aR](float min, float, float x)
				{
					const auto denom = a2 * min + aR - a2 * x - min + x;
					if (denom == 0.f)
						return 0.f;
					auto val = aM * (x - min) / denom;
					return val > 1.f ? 1.f : val;
				},
				[](float min, float max, float x)
				{
					return x < min ? min : x > max ? max : x;
				}
		};
		else return { start, end };
	}

	Range stepped(float start, float end, float steps) noexcept
	{
		return { start, end, steps };
	}

	Range toggle() noexcept
	{
		return stepped(0.f, 1.f);
	}

	Range lin(float start, float end) noexcept
	{
		const auto range = end - start;

		return
		{
				start,
				end,
				[range](float min, float, float normalized)
				{
					return min + normalized * range;
				},
				[inv = 1.f / range](float min, float, float denormalized)
				{
					return (denormalized - min) * inv;
				},
				[](float min, float max, float x)
				{
					return juce::jlimit(min, max, x);
				}
		};
	}

	Range withCentre(float start, float end, float centre) noexcept
	{
		const auto r = end - start;
		const auto v = (centre - start) / r;

		return makeRange::biased(start, end, 2.f * v - 1.f);
	}

	Range foleysLogRange(float min, float max) noexcept
	{
		return
		{
			min, max,
			[](float start, float end, float normalised)
			{
				return start + (std::pow(2.f, normalised * 10.f) - 1.f) * (end - start) / 1023.f;
			},
			[](float start, float end, float value)
			{
				return (std::log(((value - start) * 1023.f / (end - start)) + 1.f) / std::log(2.f)) * .1f;
			},
			[](float start, float end, float value)
			{
				// optimised for frequencies: >3 kHz: 2 decimals
				if (value > 3000.f)
					return juce::jlimit(start, end, 100.f * juce::roundToInt(value / 100.f));

				// optimised for frequencies: 1-3 kHz: 1 decimal
				if (value > 1000.f)
					return juce::jlimit(start, end, 10.f * juce::roundToInt(value * .1f));

				return juce::jlimit(start, end, std::round(value));
			}
		};
	}

	Range quad(float min, float max, int numSteps) noexcept
	{
		return
		{
			min, max,
			[numSteps](float start, float end, float x)
			{
				for (auto i = 0; i < numSteps; ++i)
					x *= x;
				return start + x * (end - start);
			},
			[numSteps](float start, float end, float x)
			{
				x = (x - start) / (end - start);
				for (auto i = 0; i < numSteps; ++i)
					x = std::sqrt(x);
				return x;
			},
			[](float start, float end, float x)
			{
				return juce::jlimit(start, end, x);
			}
		};
	}
}