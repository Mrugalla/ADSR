#include "Range.h"
#include "Conversion.h"

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
			[numSteps, range = max - min](float start, float, float x)
			{
				for (auto i = 0; i < numSteps; ++i)
					x *= x;
				return start + x * range;
			},
			[numSteps, rangeInv = 1.f / (max - min)](float start, float, float x)
			{
				x = (x - start) * rangeInv;
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

	Range beats(int min, int max, bool withZero) noexcept
	{
		const auto minF = static_cast<float>(min);
		const auto maxF = static_cast<float>(max);
		auto range = maxF - minF + 1.f;

		auto numValues = 3 * (max - min) + 1;
		if (withZero)
			++numValues;
		const auto numValuesF = static_cast<float>(numValues);

		const auto minVal = withZero ? 0.f : std::pow(2.f, minF);
		const auto maxVal = std::pow(2.f, maxF);
		
		enum Mode { Whole, Triplet, Dotted, NumModes };

		return
		{
			minVal, maxVal,
			[minF, maxF, numValuesF, withZero](float, float, float normalized)
			{
				if(withZero && normalized == 0.f)
					return 0.f;

				const auto idxF = normalized * numValuesF; // [0, numValuesF]
				const auto idxI = static_cast<int>(idxF); // [0, numValues]
				
				const auto mode = idxI % 3; // [ 0 = Whole, 1 = Triplet, 2 = Dotted ]
				const auto mult =
					mode == Mode::Triplet ? 1.666666666667f :
					mode == Mode::Dotted ? 1.75f :
					1.f;

				const auto baseVal = std::floor(idxF * .333333333f) + minF; // [min, max]
				const auto val = std::pow(2.f, baseVal) * mult; // [minVal, maxVal]
				
				return val;
			},
			[minF, rangeInv = 1.f / range, withZero](float start, float end, float denormalized)
			{
				if (withZero && denormalized == 0.f)
					return 0.f;
				
				const auto denormFloor = audio::nextLowestPowTwoX(denormalized);
				const auto denormFrac = denormalized - denormFloor;
				const auto modeVal = denormFrac / denormFloor;
				const auto mode = modeVal < .66f ? Mode::Whole :
					modeVal < .75f ? Mode::Triplet :
					Mode::Dotted;
				
				const auto base = std::log2(denormFloor); // [minF, maxF]
				const auto val = base + modeVal;
				auto norm = (val - minF) * rangeInv; // [0, 1]

				return norm;
			},
			[](float start, float end, float denormalized)
			{
				return juce::jlimit(start, end, denormalized);
			}
		};
	}
}