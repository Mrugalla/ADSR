#pragma once

namespace smooth
{
	// a block-based parameter smoother.
	template<typename Float>
	struct Block
	{
		/* startVal */
		Block(float = 0.f);

		/* bufferOut, bufferIn, numSamples */
		void operator()(Float*, Float*, int) noexcept;

		/* buffer, val, numSamples */
		void operator()(Float*, Float, int) noexcept;

		/* buffer, numSamples */
		void operator()(Float*, int) noexcept;

	protected:
		Float curVal;
	};
	
	template<typename Float>
	struct Lowpass
	{
		static constexpr Float Pi = static_cast<Float>(3.14159265359);
		static constexpr Float Tau = Pi * static_cast<Float>(2);

		/* decay */
		static Float getXFromFc(Float) noexcept;
		/* decay, Fs */
		static Float getXFromHz(Float, Float) noexcept;
		
		/* decay */
		void makeFromDecayInSamples(Float) noexcept;
		/* decay, Fs */
		void makeFromDecayInSecs(Float, Float) noexcept;
		/* decay */
		void makeFromDecayInFc(Float) noexcept;
		/* decay, Fs */
		void makeFromDecayInHz(Float, Float) noexcept;
		/* decay, Fs */
		void makeFromDecayInMs(Float, Float) noexcept;

		void copyCutoffFrom(const Lowpass<Float>&) noexcept;

		/* startVal */
		Lowpass(const Float = static_cast<Float>(0));

		void reset();

		/* buffer, val, numSamples */
		void operator()(Float*, Float, int) noexcept;
		/* buffer, numSamples */
		void operator()(Float*, int/*numSamples*/) noexcept;
		/* val */
		Float operator()(Float) noexcept;

		void setX(Float) noexcept;

	protected:
		Float a0, b1, y1, eps, startVal;

		Float processSample(Float) noexcept;
	};

	template<typename Float>
	struct Smooth
	{
		/*smoothLenMs, Fs*/
		void makeFromDecayInMs(Float, Float);

		Smooth(float /*startVal*/ = 0.f);

		/* bufferOut, bufferIn, numSamples */
		void operator()(Float*, Float*, int) noexcept;

		/* buffer, val, numSamples */
		void operator()(Float*, Float, int) noexcept;

		/* buffer, numSamples */
		void operator()(Float*, int) noexcept;

	protected:
		Block<Float> block;
		Lowpass<Float> lowpass;
	};
}