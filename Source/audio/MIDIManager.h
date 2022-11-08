#pragma once
#include "MIDILearn.h"
#include <functional>

namespace audio
{
	struct MIDIManager
	{
		MIDIManager(Params&, State&);

		void savePatch();

		void loadPatch();

		/* midiBuffer, numSamples */
		void operator()(MIDIBuffer&, int) noexcept;

		MIDILearn midiLearn;

		/* numSamples */
		std::vector<std::function<void(int)>> onInit, onEnd, onSample;
		/* midiMessage, sampleIndex */
		std::vector<std::function<void(const MidiMessage&, int)>> onCC, onNoteOn, onNoteOff, onPitchbend;
		/* sampleIndex */
		std::vector<std::function<void(int)>> onNoEvt;
	protected:

		/* numSamples */
		void processEmpty(int) noexcept;

		/* midiBuffer, numSamples */
		void processBlock(MIDIBuffer&, int) noexcept;
	};

	struct MIDINote
	{
		float velocity;
		int noteNumber;
		bool noteOn;
	};

	struct MIDINoteBuffer
	{
		MIDINoteBuffer();

		/* blockSize */
		void prepare(int);
		
		/* newNote, timestamp */
		void processNoteOn(const MIDINote&, int) noexcept;

		/* timestamp */
		void processNoteOff(int) noexcept;

		/* numSamples */
		void process(int) noexcept;

		std::vector<MIDINote> buffer;
		MIDINote curNote;
		int sampleIdx;
	};

	using MIDIVoicesArray = std::array<MIDINoteBuffer, PPD_MIDINumVoices>;

	struct MIDIPitchbendBuffer
	{
		MIDIPitchbendBuffer();

		/* blockSize */
		void prepare(int);

		void processInit() noexcept;
		
		/* pitchbend, timestamp */
		void processPitchbend(float, int) noexcept;

		/* numSamples */
		void process(int) noexcept;

		std::vector<float> buffer;
		float curPitchbend;
		int sampleIdx;
	};

	struct MIDIVoices
	{
		MIDIVoices(MIDIManager&);

		/* blockSize */
		void prepare(int);

		MIDIVoicesArray voices;
		MIDIPitchbendBuffer pitchbendBuffer;
		float pitchbendRange;
		int voiceIndex;
	};
}