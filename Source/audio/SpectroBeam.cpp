#include "SpectroBeam.h"

namespace audio
{
	template<size_t Order>
	SpectroBeam<Order>::SpectroBeam() :
		smpls(),
		fft(Order),
		fifo(),
		window(),
		buffer(),
		ready(false),
		idx(0)
	{
		SIMD::clear(buffer.data(), Size2);
		SIMD::clear(fifo.data(), Size2);

		// gaussian window
		for (auto i = 0; i < Size; ++i)
		{
			const auto norm = static_cast<float>(i) * SizeInv;
			const auto x = norm * 2.f - 1.f;
			const auto w = std::exp(-x * x * 16.f);
			window[i] = w;
		}
	}

	template<size_t Order>
	void SpectroBeam<Order>::prepare(int blockSize)
	{
		smpls.resize(blockSize);
	}

	template<size_t Order>
	void SpectroBeam<Order>::operator()(float** samples, int numChannels, int numSamples) noexcept
	{
		const auto chInv = 1.f / numChannels;

		for (auto s = 0; s < numSamples; ++s)
		{
			auto mid = samples[0][s];
			for (auto ch = 1; ch < numChannels; ++ch)
				mid += samples[ch][s];
			mid *= chInv;
			smpls[s] = mid;
		}

		process(numSamples);
	}

	template<size_t Order>
	void SpectroBeam<Order>::process(int numSamples) noexcept
	{
		auto fif = fifo.data();
		for (auto s = 0; s < numSamples; ++s)
		{
			fif[idx] = smpls[s];
			++idx;
			if (idx == Size)
			{
				const auto wndw = window.data();
				auto buf = buffer.data();

				SIMD::multiply(fif, wndw, Size);
				fft.performRealOnlyForwardTransform(fif, true);
				SIMD::copy(buf, fif, Size);
				ready.store(true);
				idx = 0;
			}
		}
	}

	template struct SpectroBeam<1>;
	template struct SpectroBeam<2>;
	template struct SpectroBeam<3>;
	template struct SpectroBeam<4>;
	template struct SpectroBeam<5>;
	template struct SpectroBeam<6>;
	template struct SpectroBeam<7>;
	template struct SpectroBeam<8>;
	template struct SpectroBeam<9>;
	template struct SpectroBeam<10>;
	template struct SpectroBeam<11>;
	template struct SpectroBeam<12>;
	template struct SpectroBeam<13>;
	template struct SpectroBeam<14>;
	template struct SpectroBeam<15>;
}
