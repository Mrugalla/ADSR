#pragma once
#include "AudioUtils.h"
#include <juce_dsp/juce_dsp.h>

namespace audio
{
	template<size_t Order>
	struct SpectroBeam
	{
		static constexpr size_t Size = 1 << Order;
		static constexpr size_t Size2 = Size * 2;
		static constexpr size_t SizeHalf = Size / 2;
		static constexpr float SizeF = static_cast<float>(Size);
		static constexpr float SizeInv = 1.f / SizeF;
		
		using FFT = juce::dsp::FFT;
		using Fifo = std::array<float, Size>;
		using Fifo2 = std::array<float, Size2>;

		SpectroBeam();

		/* blockSize */
		void prepare(int);

		/* samples, numChannels, numSamples */
		void operator()(float**, int, int) noexcept;

		/* numSamples */
		void process(int) noexcept;

	protected:
		std::vector<float> smpls;
		FFT fft;
		Fifo2 fifo;
		Fifo window;
	public:
		Fifo2 buffer;
		std::atomic<bool> ready;
	protected:
		int idx;
	};
}