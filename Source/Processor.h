#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_events/juce_events.h>

#include "audio/XenManager.h"
#include "audio/MIDIManager.h"
#include "audio/MIDILearn.h"
#include "audio/ProcessSuspend.h"
#include "audio/DryWetMix.h"
#if PPDHasStereoConfig
#include "audio/MidSide.h"
#endif
#include "audio/Oversampling.h"
#include "audio/Meter.h"

#include "audio/EnvelopeGenerator.h"
#include "audio/Oscilloscope.h"

#include "audio/AudioUtils.h"

namespace audio
{
    using MacroProcessor = param::MacroProcessor;
    using Timer = juce::Timer;

    struct ProcessorBackEnd :
        public juce::AudioProcessor,
        public Timer
    {
        using ChannelSet = juce::AudioChannelSet;
        using AppProps = juce::ApplicationProperties;

        ProcessorBackEnd();

        const String getName() const override;
        double getTailLengthSeconds() const override;
        int getNumPrograms() override;
        int getCurrentProgram() override;
        void setCurrentProgram(int) override;
        const String getProgramName(int) override;
        void changeProgramName(int, const String&) override;
        bool isBusesLayoutSupported(const BusesLayout&) const override;
        AppProps* getProps() noexcept;
        bool canAddBus(bool) const override;

        void savePatch();
        void loadPatch();

        bool hasEditor() const override;
        bool acceptsMidi() const override;
        bool producesMidi() const override;
        bool isMidiEffect() const override;

        juce::AudioProcessor::BusesProperties makeBusesProperties();

        PlayHeadPos playHeadPos;
        AppProps props;
        ProcessSuspender sus;

        XenManager xenManager;
        State state;
        Params params;
        MacroProcessor macroProcessor;
        MIDIManager midiManager;
        DryWetMix dryWetMix;
#if PPDHasHQ
        Oversampler oversampler;
#endif
        Meters meters;
        MIDIVoices midiVoices;
#if PPDHasTuningEditor
        TuningEditorSynth tuningEditorSynth;
#endif

        void forcePrepareToPlay();

        void timerCallback() override;

        void processBlockBypassed(AudioBuffer&, juce::MidiBuffer&) override;

#if PPDHasStereoConfig
        bool midSideEnabled;
#endif
#if PPDHasLookahead
		bool lookaheadEnabled;
#endif

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProcessorBackEnd)
    };

    struct Processor :
        public ProcessorBackEnd
    {
        Processor();

        void prepareToPlay(double, int) override;

        void processBlock(AudioBuffer&, juce::MidiBuffer&) override;

        void processBlockBypassed(AudioBuffer&, juce::MidiBuffer&) override;
        
        /* samples, numChannels, numSamples, midi, samplesSC, numChannelsSC */
        void processBlockPreUpscaled(float**, int numChannels, int numSamples, juce::MidiBuffer& midi) noexcept;

        /* samples, numChannels, numSamples, samplesSC, numChannelsSC */
        void processBlockUpsampled(float**, int, int
#if PPDHasSidechain
            , float**, int
#endif
        ) noexcept;

        void releaseResources() override;
		
        /////////////////////////////////////////////
        /////////////////////////////////////////////
        void getStateInformation(juce::MemoryBlock&) override;
		/* data, sizeInBytes */
        void setStateInformation(const void*, int) override;
		
        void savePatch();

        void loadPatch();

        juce::AudioProcessorEditor* createEditor() override;

        EnvGenMIDI envGenMIDI;
		Oscilloscope oscope;
    };
}