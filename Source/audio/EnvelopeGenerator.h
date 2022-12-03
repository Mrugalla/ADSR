#pragma once
#include "MIDIManager.h"
#include "../arch/Conversion.h"
#include "PRM.h"

namespace audio
{
	struct EnvGen
	{
		static constexpr float MinVelocity = 1.f / 127.f;
		
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
			noteOn(false), legato(false),
			envRaw(1.f), env(0.f), noteOffVal(0.f), noteOnVal(0.f),
			velocitySens(0.f), veloGain(1.f),
			atkP(0.f), dcyP(0.f), susP(0.f), rlsP(0.f),
			atkShapeP(0.f), dcyShapeP(0.f), rlsShapeP(0.f),
			noteOns()
		{
			for (auto& n : noteOns)
				n = false;
		}

		void setNoteOn(int noteIdx, float velo) noexcept
		{
			if (velo < MinVelocity)
				return setNoteOff(noteIdx);
			if (noteOns[noteIdx])
				return;
			noteOns[noteIdx] = noteOn = true;
			veloGain = 1.f + velocitySens * (velo - 1.f);
			if (!legato)
				if(env < velo)
					triggerAttack();
				else
					triggerDecay();
		}
		
		void setNoteOff(int noteIdx) noexcept
		{
  			noteOns[noteIdx] = false;
			const auto notesLeft = noteOnLeft();
			if (legato)
			{
				if (!notesLeft)
					noteOn = false;
			}
			else
				if (notesLeft)
					triggerAttack();
				else
					triggerRelease();
			
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

		void processBypassed(float* samples, int numSamples) noexcept
		{
			for (auto& n : noteOns)
				n = false;
			setNoteOff(24);

			for (auto s = 0; s < numSamples; ++s)
				samples[s] = process(s);
		}

		State state;
		bool noteOn, legato;
		float envRaw, env, noteOffVal, noteOnVal;
		float velocitySens, veloGain;
		PRM atkP, dcyP, susP, rlsP;
		PRM atkShapeP, dcyShapeP, rlsShapeP;
	protected:
		std::array<bool, 128> noteOns;

		bool noteOnLeft() const noexcept
		{
			for (const auto n : noteOns)
				if (n)
					return true;
			return false;
		}

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
				triggerAttack(s);
		}

		// TRIGGER STATES

		void triggerRelease() noexcept
		{
			noteOffVal = env;
			envRaw = 0.f;
			state = State::Release;
			noteOn = false;
		}

		void triggerRelease(int s) noexcept
		{
			triggerRelease();
			synthesizeRelease(s);
		}

		void triggerAttack() noexcept
		{
			noteOnVal = env;
			envRaw = 0.f;
			state = State::Attack;
			noteOn = true;
		}

		void triggerAttack(int s) noexcept
		{
			triggerAttack();
			synthesizeAttack(s);
		}

		void triggerDecay()
		{
			noteOnVal = env;
			state = State::Decay;
			envRaw = 0.f;
			noteOn = true;
		}
		
		// SHAPE

		void shapeAttack(int s) noexcept
		{
			env = noteOnVal + (veloGain - noteOnVal) * getSkewed(envRaw, atkShapeP[s]);
		}

		void shapeDecay(int s) noexcept
		{
			const auto sus = susP[s] * veloGain;
			
			env = veloGain - (veloGain - sus) * getSkewed(envRaw, dcyShapeP[s]);
		}

		void shapeSustain() noexcept
		{
			env = envRaw * veloGain;
		}

		void shapeRelease(int s) noexcept
		{
 			env = noteOffVal - getSkewed(envRaw, rlsShapeP[s]) * noteOffVal;
		}

	public:
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
		attackShape [-1,1], decayShape [-1,1], releaseShape [-1,1], legato [0, 1], inverse [0, 1],
		velocitySensitivity [0, 1], tempoSync[0, 1],
		atkBeats [0, N]beats, dcyBeats[0, N], rlsBeats[0, N] */
		void operator()(MIDIBuffer& midi, int numSamples,
			float _atk, float _dcy, float _sus, float _rls,
			float _atkShape, float _dcyShape, float _rlsShape,
			bool _legato, bool _inverse, float _velo, bool _tempoSync,
			float _atkBeats, float _dcyBeats, float _rlsBeats, const PlayHeadPos& playHead) noexcept
		{
			if (_tempoSync && playHead.bpm != 0.)
			{
				const auto beatsPerMinute = static_cast<float>(playHead.bpm);
				const auto beatsPerSec = beatsPerMinute * .0166666667f;
				const auto samplesPerBeat = Fs / beatsPerSec;
				
				if (_atkBeats == 0.f)
					envGen.atkP(1.1f, numSamples);
				else
				{
					_atk = _atkBeats * samplesPerBeat;
					envGen.atkP(1.f / _atk, numSamples);
				}

				if (_dcyBeats == 0.f)
					envGen.dcyP(1.1f, numSamples);
				else
				{
					_dcy = _dcyBeats * samplesPerBeat;
					envGen.dcyP(1.f / _dcy, numSamples);
				}

				if (_rlsBeats == 0.f)
					envGen.rlsP(1.1f, numSamples);
				else
				{
					_rls = _rlsBeats * samplesPerBeat;
					envGen.rlsP(1.f / _rls, numSamples);
				}
			}
			else
			{
				if (_atk == 0.f)
					envGen.atkP(1.1f, numSamples);
				else
					envGen.atkP(msInInc(_atk, Fs), numSamples);
				if (_dcy == 0.f)
					envGen.dcyP(1.1f, numSamples);
				else
					envGen.dcyP(msInInc(_dcy, Fs), numSamples);
				if (_rls == 0.f)
					envGen.rlsP(1.1f, numSamples);
				else
					envGen.rlsP(msInInc(_rls, Fs), numSamples);
			}

			envGen.susP(_sus, numSamples);

			envGen.atkShapeP(std::tanh(Pi * _atkShape) * .5f + .5f, numSamples);
			envGen.dcyShapeP(-std::tanh(Pi * _dcyShape) * .5f + .5f, numSamples);
			envGen.rlsShapeP(-std::tanh(Pi * _rlsShape) * .5f + .5f, numSamples);
			
			if (midi.isEmpty())
			{
				for (auto s = 0; s < numSamples; ++s)
					buffer[s] = envGen(s);
			}
			else
			{
				envGen.legato = _legato;
				envGen.velocitySens = _velo;

				auto evt = midi.begin();
				auto ref = *evt;
				auto ts = ref.samplePosition;

				for (auto s = 0; s < numSamples; ++s)
				{
					while (s == ts)
					{
						const auto msg = ref.getMessage();
						if (msg.isNoteOnOrOff())
						{
							auto noteOn = msg.isNoteOn();
							auto noteNum = msg.getNoteNumber();
							
							if (noteOn)
								envGen.setNoteOn(noteNum, msg.getFloatVelocity());
							else
								envGen.setNoteOff(noteNum);
						}

						++evt;
						if (evt == midi.end())
							break;
						ref = *evt;
						ts = ref.samplePosition;
					}

					buffer[s] = envGen(s);
				}
			}
			
			if(_inverse)
				for (auto s = 0; s < numSamples; ++s)
					buffer[s] = 1.f - buffer[s];
		}

		void processBypassed(int numSamples) noexcept
		{
			envGen.processBypassed(buffer.data(), numSamples);
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

legato
	consecutive noteOns don't update velocity normally
		but maybe they do in new mode?

legato modulatable?

modes
	direct out
	amplitude mod
	filter mod

*/