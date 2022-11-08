#pragma once
#include <cmath>
#include <functional>

#include <juce_graphics/juce_graphics.h>

namespace interpolate
{
	enum class Type
	{
		Lerp,
		CubicHermiteSpline,
		NumTypes
	};

	static constexpr int NumTypes = static_cast<int>(Type::NumTypes);

	/* samples, idx, size */
	template<typename T>
	T lerp(const T*, T, int) noexcept;

	/* samples, idx */
	template<typename T>
	T lerp(const T*, T) noexcept;

	/* samples, idx, size */
	template<typename T>
	T cubicHermiteSpline(const T*, T, int) noexcept;

	/* samples, idx */
	template<typename T>
	T cubicHermiteSpline(const T*, T) noexcept;

	///

	namespace polynomial
	{
		template<typename Float>
		std::function<Float(Float)> getFunc(const std::vector<juce::Point<Float>>&);
	}
}