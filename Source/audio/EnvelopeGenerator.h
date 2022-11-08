#pragma once
#include "MIDIManager.h"
#include "../arch/Conversion.h"
#include "PRM.h"

namespace audio
{
	struct EnvGen
	{
		enum class State
		{
			Attack,
			Decay,
			Sustain,
			Release,
			NumStates
		};

		EnvGen() :
			state(State::Release),
			noteOn(false),
			envRaw(1.f), env(0.f), noteOffVal(0.f), noteOnVal(0.f),
			atkP(0.f), dcyP(0.f), susP(0.f), rlsP(0.f),
			atkShapeP(0.f), dcyShapeP(0.f), rlsShapeP(0.f)
		{
			
		}

		void prepare(float Fs, int blockSize)
		{
			atkP.prepare(Fs, blockSize, 15.f);
			dcyP.prepare(Fs, blockSize, 15.f);
			susP.prepare(Fs, blockSize, 15.f);
			rlsP.prepare(Fs, blockSize, 15.f);
			atkShapeP.prepare(Fs, blockSize, 15.f);
			dcyShapeP.prepare(Fs, blockSize, 15.f);
			rlsShapeP.prepare(Fs, blockSize, 15.f);
		}

		float operator()(int s) noexcept
		{
			return process(s);
		}

		float process(int s) noexcept
		{
			switch (state)
			{
			case State::Attack:
				synthesizeAttack(s);
				break;
			case State::Decay:
				synthesizeDecay(s);
				break;
			case State::Sustain:
				synthesizeSustain(s);
				break;
			case State::Release:
				synthesizeRelease(s);
				break;
			}

			switch (state)
			{
			case State::Attack:
				shapeAttack(s);
				break;
			case State::Decay:
				shapeDecay(s);
				break;
			case State::Sustain:
				shapeSustain();
				break;
			case State::Release:
				shapeRelease(s);
				break;
			}
			
			return env;
		}

		State state;
		bool noteOn;
		float envRaw, env, noteOffVal, noteOnVal;
		PRM atkP, dcyP, susP, rlsP;
		PRM atkShapeP, dcyShapeP, rlsShapeP;

		// SYNTHESIZE
		
		void synthesizeAttack(int s) noexcept
		{
			if (noteOn)
			{
				const auto atk = atkP[s];
				
				envRaw += atk;
				if (envRaw >= 1.f)
				{
					envRaw = 0.f;
					state = State::Decay;
					synthesizeDecay(s);
				}
			}
			else
				triggerRelease(s);
		}

		void synthesizeDecay(int s) noexcept
		{
			if (noteOn)
			{
				const auto dcy = dcyP[s];

				envRaw += dcy;
				if (envRaw >= 1.f)
				{
					state = State::Sustain;
					synthesizeSustain(s);
				}
			}
			else
				triggerRelease(s);
		}

		void synthesizeSustain(int s) noexcept
		{
			if (!noteOn)
				return triggerRelease(s);

			envRaw = susP[s];
		}

		void synthesizeRelease(int s) noexcept
		{
			if (!noteOn)
			{
				const auto rls = rlsP[s];
				
				envRaw += rls;
				if (envRaw > 1.f)
					envRaw = 1.f;
			}
			else
			{
				noteOnVal = env;
				envRaw = 0.f;
				state = State::Attack;
			}
		}

		//
		
		void triggerRelease(int s) noexcept
		{
			noteOffVal = env;
			envRaw = 0.f;
			state = State::Release;
			synthesizeRelease(s);
		}
		
		// SHAPE

		void shapeAttack(int s) noexcept
		{
			env = noteOnVal + getSkewed(envRaw, atkShapeP[s]) * (1.f - noteOnVal);
		}

		void shapeDecay(int s) noexcept
		{
			const auto sus = susP[s];
			
			env = 1.f - getSkewed(envRaw, dcyShapeP[s]) * (1.f - sus);
		}

		void shapeSustain() noexcept
		{
			env = envRaw;
		}

		void shapeRelease(int s) noexcept
		{
 			env = noteOffVal - getSkewed(envRaw, rlsShapeP[s]) * noteOffVal;
		}

		static float getSkewed(float x, float bias) noexcept
		{
			const auto b2 = 2.f * bias;
			const auto bM = 1.f - bias;
			const auto xy = bM - x + b2 * x;
			if (xy == 0.f)
				return 0.f;
			return bias * x / xy;
		}
	};

	struct EnvGenMIDI
	{
		EnvGenMIDI() :
			buffer(),
			envGen(),
			Fs(1.f)
		{
		}

		void prepare(float _Fs, int blockSize)
		{
			Fs = _Fs;
			envGen.prepare(Fs, blockSize);
			buffer.resize(blockSize);
		}
		
		/* midiBuffer, numSamples, atk [0, N]ms, dcy [0, N]ms, sus [0, 1], rls [0, N]ms,
		attackShape [-1,1], decayShape [-1,1], releaseShape [-1,1] */
		void operator()(MIDIBuffer& midi, int numSamples,
			float _atk, float _dcy, float _sus, float _rls,
			float _atkShape, float _dcyShape, float _rlsShape)
		{
			if (_atk == 0.f)
				envGen.atkP(1.1f, numSamples);
			else
				envGen.atkP(msInInc(_atk, Fs), numSamples);
			if (_dcy == 0.f)
				envGen.dcyP(1.1f, numSamples);
			else
				envGen.dcyP(msInInc(_dcy, Fs), numSamples);
			envGen.susP(_sus, numSamples);
			if (_rls == 0.f)
				envGen.rlsP(1.1f, numSamples);
			else
				envGen.rlsP(msInInc(_rls, Fs), numSamples);
			envGen.atkShapeP(std::tanh(Pi * _atkShape) * .5f + .5f, numSamples);
			envGen.dcyShapeP(-std::tanh(Pi * _dcyShape) * .5f + .5f, numSamples);
			envGen.rlsShapeP(-std::tanh(Pi * _rlsShape) * .5f + .5f, numSamples);
			
			if (midi.isEmpty())
			{
				for (auto s = 0; s < numSamples; ++s)
					buffer[s] = envGen(s);
				return;
			}

			auto evt = midi.begin();
			auto ref = *evt;
			auto ts = ref.samplePosition;

			for (auto s = 0; s < numSamples; ++s)
			{
				while (s == ts)
				{
					const auto msg = ref.getMessage();
					if (msg.isNoteOnOrOff())
						envGen.noteOn = msg.isNoteOn();
					++evt;
					if (evt == midi.end())
						break;
					ref = *evt;
					ts = ref.samplePosition;
				}

				buffer[s] = envGen(s);
			}
		}

		float operator[](int s) const noexcept
		{
			return buffer[s];
		}

		const float* data() const noexcept
		{
			return buffer.data();
		}

		static float getSkewed(float x, float bias) noexcept
		{
			return EnvGen::getSkewed(x, bias);
		}

		std::vector<float> buffer;
	protected:
		EnvGen envGen;
		float Fs;
	};
}

/*

todo:

make GUI interface

*/