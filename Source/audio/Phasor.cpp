#include "Phasor.h"

namespace audio
{
	// PhasorInfo

	template<typename Float>
	PhaseInfo<Float>::PhaseInfo(Float _phase, Float _retrig) :
		phase(_phase),
		retrig(_retrig)
	{}

	template struct PhaseInfo<float>;
	template struct PhaseInfo<double>;

	// Phasor
	
	template<typename Float>
	void Phasor<Float>::setFrequencyHz(Float hz) noexcept
	{
		inc = hz * fsInv;
	}

	template<typename Float>
	Phasor<Float>::Phasor(Float _phase, Float _inc) :
		phase(_phase, false),
		inc(_inc),
		fsInv(static_cast<Float>(1))
	{

	}

	template<typename Float>
	void Phasor<Float>::prepare(Float _fsInv) noexcept
	{
		fsInv = _fsInv;
	}

	template<typename Float>
	void Phasor<Float>::reset(Float p) noexcept
	{
		phase.phase = p;
	}

	template<typename Float>
	Phasor<Float>::Phase Phasor<Float>::operator()() noexcept
	{
		phase.phase += inc;
		if (phase.phase >= static_cast<Float>(1))
		{
			--phase.phase;
			phase.retrig = true;
			return phase;
		}
		phase.retrig = false;
		return phase;
	}

	template struct Phasor<float>;
	template struct Phasor<double>;
}