#pragma once

namespace audio
{
	template<typename Float>
	struct PhaseInfo
	{
		PhaseInfo(Float, Float);

		Float phase;
		bool retrig;
	};

	template<typename Float>
	struct Phasor
	{
		using Phase = PhaseInfo<Float>;

		void setFrequencyHz(Float) noexcept;

		/* phase, inc */
		Phasor(Float = static_cast<Float>(0), Float = static_cast<Float>(0));

		/* fsInv */
		void prepare(Float) noexcept;
		
		void reset(Float = static_cast<Float>(0)) noexcept;
		
		Phase operator()() noexcept;

		Phase phase;
		Float inc, fsInv;
	};
}