#include "WaveTable.h"
#include "../arch/Interpolation.h"

namespace audio
{
	template<size_t Size>
	WaveTable<Size>::WaveTable() :
		table()
	{
		create([](float x) { return std::cos(x * Pi); });
	}

	template<size_t Size>
	void WaveTable<Size>::create(const Func& func) noexcept
	{
		auto x = -1.f + SizeInv * .5f;
		const auto inc = 2.f * SizeInv;
		for (auto s = 0; s < Size; ++s, x += inc)
			table[s] = func(x);
		for (auto i = 0; i < NumExtraSamples; ++i)
			table[Size + i] = table[i];
	}

	template<size_t Size>
	void WaveTable<Size>::savePatch(sta::State& state, const String& key)
	{
		juce::MemoryBlock mb;
		const auto dataSize = FullSize * sizeof(float);
		mb.append(table.data(), dataSize);
		const auto base64 = mb.toBase64Encoding();
		state.set(key, "wt", base64, false);
	}

	template<size_t Size>
	void WaveTable<Size>::loadPatch(sta::State& state, const String& key)
	{
		auto var = state.get(key, "wt");
		if (var != nullptr)
		{
			const auto base64 = var->toString();
			juce::MemoryBlock mb;
			mb.fromBase64Encoding(base64);
#if JUCE_DEBUG
			const auto mbSize = mb.getSize();
#endif
			const auto dataSize = FullSize * sizeof(float);
			jassert(mbSize == dataSize);
			mb.copyTo(table.data(), 0, dataSize);
		}
	}

	template<size_t Size>
	float WaveTable<Size>::operator()(int idx) const noexcept
	{
		return table[idx];
	}

	template<size_t Size>
	float WaveTable<Size>::operator()(float phase) const noexcept
	{
		const auto idx = phase * SizeF;
		return interpolate::lerp(table.data(), idx);
	}

	template<size_t Size>
	float* WaveTable<Size>::data() noexcept
	{
		return table.data();
	}

	template<size_t Size>
	const float* WaveTable<Size>::data() const noexcept
	{
		return table.data();
	}

	template struct WaveTable<1 << 8>;
	template struct WaveTable<1 << 9>;
	template struct WaveTable<1 << 10>;
	template struct WaveTable<1 << 11>;
	template struct WaveTable<1 << 12>;
	template struct WaveTable<1 << 13>;
}