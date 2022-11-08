#pragma once
#include <array>
#include <functional>
#include "AudioUtils.h"
#include "../arch/State.h"

namespace audio
{
	template<size_t Size>
	struct WaveTable
	{
		static constexpr float SizeF = static_cast<float>(Size);
		static constexpr float SizeInv = 1.f / SizeF;
		static constexpr int NumExtraSamples = 4;
		static constexpr int FullSize = Size + NumExtraSamples;

		using Table = std::array<float, FullSize>;
		using Func = std::function<float(float)>;

		WaveTable();

		void create(const Func&) noexcept;

		/* state, key*/
		void savePatch(sta::State&, const String&);

		/* state, key*/
		void loadPatch(sta::State&, const String&);

		/* idx */
		float operator()(int) const noexcept;

		/* phase */
		float operator()(float) const noexcept;

		float* data() noexcept;

		const float* data() const noexcept;
		
	protected:
		Table table;
	};

	template<size_t Size>
	inline void createWaveTableSine(WaveTable<Size>& table)
	{
		table.create([](float x)
		{
			return std::sin(x * Pi);
		});
	}

	template<size_t Size>
	inline void createWaveTableSaw(WaveTable<Size>& table)
	{
		table.create([](float x)
		{
			return x;
		});
	}

	template<size_t Size>
	inline void createWaveTableTriangle(WaveTable<Size>& table)
	{
		table.create([](float x)
		{
			return 2.f * std::asin(std::sin(x * Pi)) * PiInv;
		});
	}

	template<size_t Size>
	inline void createWaveTableSquare(WaveTable<Size>& table)
	{
		table.create([](float x)
		{
			return 1.f - 2.f * std::fmod(std::floor(x), 2.f);
		});
	}

	template<size_t Size>
	inline void createWaveTableNoise(WaveTable<Size>& table)
	{
		juce::Random rand;
		table.create([r = rand](float x)
		{
			return r.nextFloat() * 2.f - 1.f;
		});
	}

}