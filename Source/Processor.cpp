#include "Processor.h"
#include "Editor.h"

namespace audio
{
    juce::AudioProcessorEditor* Processor::createEditor()
    {
        return new gui::Editor(*this);
    }

    juce::AudioProcessor::BusesProperties ProcessorBackEnd::makeBusesProperties()
    {
        BusesProperties bp;
        bp.addBus(true, "Input", ChannelSet::stereo(), true);
        bp.addBus(false, "Output", ChannelSet::stereo(), true);
#if PPDHasSidechain
        if (!juce::JUCEApplicationBase::isStandaloneApp())
        {
            bp.addBus(true, "Sidechain", ChannelSet::stereo(), true);
        }
#endif
        return bp;
    }

    ProcessorBackEnd::ProcessorBackEnd() :
        juce::AudioProcessor(makeBusesProperties()),
        playHeadPos(),
        props(),
        sus(*this),
        state(),
#if PPDHasTuningEditor
        xenManager(),
#endif
        params(*this, state
#if PPDHasTuningEditor
            , xenManager
#endif
        ),
        macroProcessor(params),
        midiManager(params, state),
#if PPDHasHQ
        oversampler(),
#endif
        meters()
#if PPDHasStereoConfig
        , midSideEnabled(false)
#endif
#if PPDHasLookahead
		, lookaheadEnabled(false)
#endif
		, midiVoices(midiManager)
#if PPDHasTuningEditor
        , tuningEditorSynth(xenManager)
#endif
    {
        {
            juce::PropertiesFile::Options options;
            options.applicationName = JucePlugin_Name;
            options.filenameSuffix = ".settings";
            options.folderName = "Mrugalla" + juce::File::getSeparatorString() + JucePlugin_Name;
            options.osxLibrarySubFolder = "Application Support";
            options.commonToAllUsers = false;
            options.ignoreCaseOfKeyNames = false;
            options.doNotSave = false;
            options.millisecondsBeforeSaving = 20;
            options.storageFormat = juce::PropertiesFile::storeAsXML;

            props.setStorageParameters(options);
        }

        {
            playHeadPos.bpm = 120.;
            playHeadPos.ppqPosition = 0.;
            playHeadPos.isPlaying = false;
            playHeadPos.timeInSamples = 0;
        }

        startTimerHz(6);
    }

    ProcessorBackEnd::~ProcessorBackEnd()
    {
        auto user = props.getUserSettings();
        user->setValue("firstTimeUwU", false);
        user->save();
    }

    const String ProcessorBackEnd::getName() const
    {
        return JucePlugin_Name;
    }

    double ProcessorBackEnd::getTailLengthSeconds() const
    {
        return 0.;
    }

    int ProcessorBackEnd::getNumPrograms()
    {
        return 1;
    }

    int ProcessorBackEnd::getCurrentProgram()
    {
        return 0;
    }

    void ProcessorBackEnd::setCurrentProgram(int)
    {}

    const String ProcessorBackEnd::getProgramName(int)
    {
        return {};
    }

    void ProcessorBackEnd::changeProgramName(int, const juce::String&)
    {}

    bool ProcessorBackEnd::canAddBus(bool isInput) const
    {
        if (wrapperType == wrapperType_Standalone)
            return false;

        return PPDHasSidechain ? isInput : false;
    }

    bool ProcessorBackEnd::isBusesLayoutSupported(const BusesLayout& layouts) const
    {
        const auto mono = ChannelSet::mono();
        const auto stereo = ChannelSet::stereo();
        
        const auto mainIn = layouts.getMainInputChannelSet();
        const auto mainOut = layouts.getMainOutputChannelSet();

        if (mainIn != mainOut)
            return false;

        if (mainOut != stereo && mainOut != mono)
            return false;

#if PPDHasSidechain
        if (wrapperType != wrapperType_Standalone)
        {
            const auto scIn = layouts.getChannelSet(true, 1);
            if (!scIn.isDisabled())
                if (scIn != stereo && scIn != mono)
                    return false;
        }
#endif
        return true;
    }

    ProcessorBackEnd::AppProps* ProcessorBackEnd::getProps() noexcept
    {
        return &props;
    }

    void ProcessorBackEnd::savePatch()
    {
        params.savePatch(props);
        midiManager.savePatch();
#if PPDHasTuningEditor
        tuningEditorSynth.savePatch(state);
#endif
    }

    void ProcessorBackEnd::loadPatch()
    {
        params.loadPatch(props);
        midiManager.loadPatch();
#if PPDHasTuningEditor
        tuningEditorSynth.loadPatch(state);
#endif
    }

    bool ProcessorBackEnd::hasEditor() const { return PPDHasEditor; }
    bool ProcessorBackEnd::acceptsMidi() const { return true; }
    bool ProcessorBackEnd::producesMidi() const { return true; }
    bool ProcessorBackEnd::isMidiEffect() const { return false; }

    void ProcessorBackEnd::forcePrepareToPlay()
    {
        sus.suspend();
    }

    void ProcessorBackEnd::timerCallback()
    {
        bool shallForcePrepare = false;
#if PPDHasHQ
        const auto ovsrEnabled = params[PID::HQ]->getValMod() > .5f;
        if (oversampler.isEnabled() != ovsrEnabled)
            shallForcePrepare = true;
#endif
#if PPDHasLookahead
		const auto _lookaheadEnabled = params[PID::Lookahead]->getValMod() > .5f;
		if (lookaheadEnabled != _lookaheadEnabled)
			shallForcePrepare = true;
#endif

        if (shallForcePrepare)
			forcePrepareToPlay();
    }

    void ProcessorBackEnd::processBlockBypassed(AudioBuffer& buffer, juce::MidiBuffer&)
    {
        macroProcessor();

        auto mainBus = getBus(true, 0);
        auto mainBuffer = mainBus->getBusBuffer(buffer);

        if (sus.suspendIfNeeded(mainBuffer))
            return;
        const auto numSamples = mainBuffer.getNumSamples();
        if (numSamples == 0)
            return;

        auto samples = mainBuffer.getArrayOfWritePointers();
        const auto constSamples = mainBuffer.getArrayOfReadPointers();
        const auto numChannels = mainBuffer.getNumChannels();

        dryWetMix.processBypass(samples, numChannels, numSamples);
#if PPDHasGainIn
        meters.processIn(constSamples, numChannels, numSamples);
#endif
        meters.processOut(constSamples, numChannels, numSamples);
    }

    // PROCESSOR

    Processor::Processor() :
        ProcessorBackEnd(),
        envGenMIDI(),
        oscope(),
        lowerLimit(0.f), upperLimit(1.f),
        wetLatencyCompensation(),
        midiDelay()
    {}

    void Processor::prepareToPlay(double sampleRate, int maxBlockSize)
    {
        auto latency = 0.f;
        //auto sampleRateUp = sampleRate;
        auto blockSizeUp = maxBlockSize;
#if PPDHasHQ
        oversampler.setEnabled(params[PID::HQ]->getValMod() > .5f);
        oversampler.prepare(sampleRate, maxBlockSize);
        sampleRateUp = oversampler.getFsUp();
        blockSizeUp = oversampler.getBlockSizeUp();
        latency = oversampler.getLatency();
#endif
		
#if PPDHasLookahead
        lookaheadEnabled = params[PID::Lookahead]->getValMod() > .5f;
#endif
        //const auto sampleRateUpF = static_cast<float>(sampleRateUp);
        const auto sampleRateF = static_cast<float>(sampleRate);

        midiVoices.prepare(blockSizeUp);
#if PPDHasTuningEditor
		tuningEditorSynth.prepare(sampleRateF, maxBlockSize);
#endif

		envGenMIDI.prepare(sampleRateF, maxBlockSize);
#if PPDHasLookahead
		auto envGenLatency = lookaheadEnabled ? msInSamples(envGenMIDI.MaxLatencyMs, sampleRateF) : 0.f;
        latency += envGenLatency;
        wetLatencyCompensation.prepare(maxBlockSize, static_cast<int>(std::round(envGenLatency)));
#endif
        oscope.prepare(sampleRateF, maxBlockSize);
        lowerLimit.prepare(sampleRateF, maxBlockSize, 10.f);
		upperLimit.prepare(sampleRateF, maxBlockSize, 10.f);

		const auto latencyInt = static_cast<int>(latency);
        dryWetMix.prepare(sampleRateF, maxBlockSize, latencyInt);
        meters.prepare(sampleRateF, maxBlockSize);
        setLatencySamples(latencyInt);
        sus.prepareToPlay();
    }

    void Processor::processBlock(AudioBuffer& buffer, MIDIBuffer& midi)
    {
        const ScopedNoDenormals noDenormals;

        macroProcessor();

        auto mainBus = getBus(true, 0);
        auto mainBuffer = mainBus->getBusBuffer(buffer);
        
        if (sus.suspendIfNeeded(mainBuffer))
            return;

        const auto numSamples = mainBuffer.getNumSamples();
        if (numSamples == 0)
            return;

#if PPDHasTuningEditor
        xenManager
        (
            std::round(params[PID::Xen]->getValModDenorm()),
            params[PID::MasterTune]->getValModDenorm(),
            std::round(params[PID::BaseNote]->getValModDenorm())
        );


        midiVoices.pitchbendRange = std::round(params[PID::PitchbendRange]->getValModDenorm());
#endif	
        midiManager(midi, numSamples);
		
        const auto _playHead = getPlayHead();
        const auto _playHeadPos = _playHead->getPosition();
        const bool playHeadValid = _playHeadPos.hasValue();
        if (playHeadValid)
        {
			playHeadPos.bpm = *_playHeadPos->getBpm();
			playHeadPos.ppqPosition = *_playHeadPos->getPpqPosition();
			playHeadPos.isPlaying = _playHeadPos->getIsPlaying();
			playHeadPos.timeInSamples = *_playHeadPos->getTimeInSamples();
        }

        if (params[PID::Power]->getValMod() < .5f)
            return processBlockBypassed(buffer, midi);

        const auto samples = mainBuffer.getArrayOfWritePointers();
#if PPDHasGainIn || PPDHasGainOut
        const auto constSamples = mainBuffer.getArrayOfReadPointers();
#endif
        const auto numChannels = mainBuffer.getNumChannels();

#if PPD_MixOrGainDry
        bool muteDry = params[PID::MuteDry]->getValMod() > .5f;
#endif
        dryWetMix.saveDry
        (
            samples,
            numChannels,
            numSamples,
#if PPDHasGainIn
            params[PID::GainIn]->getValueDenorm(),
#if PPDHasUnityGain
            params[PID::UnityGain]->getValMod(),
#endif
#endif
#if PPD_MixOrGainDry == 0
            params[PID::Mix]->getValMod()
#else
			, params[PID::Mix]->getValModDenorm()
#endif
#if PPDHasGainOut
            , params[PID::Gain]->getValModDenorm()
#if PPDHasPolarity
            , (params[PID::Polarity]->getValMod() > .5f ? -1.f : 1.f)
#endif
#endif
        );

#if PPDHasGainIn
        meters.processIn(constSamples, numChannels, numSamples);
#endif

#if PPDHasStereoConfig
        midSideEnabled = numChannels == 2 && params[PID::StereoConfig]->getValMod() > .5f;
        if (midSideEnabled)
        {
            encodeMS(samples, numSamples, 0);
#if PPDHasSidechain
            encodeMS(samples, numSamples, 1);
#endif
        }

#endif
        processBlockPreUpscaled(samples, numChannels, numSamples, midi);

#if PPDHasHQ
        auto resampledBuf = &oversampler.upsample(buffer);
#else
        auto resampledBuf = &buffer;
#endif
        auto resampledMainBuf = mainBus->getBusBuffer(*resampledBuf);

#if PPDHasSidechain
        if (wrapperType != wrapperType_Standalone)
        {
            auto scBus = getBus(true, 1);
            if (scBus != nullptr)
                if (scBus->isEnabled())
                {
                    auto scBuffer = scBus->getBusBuffer(*resampledBuf);

                    processBlockUpsampled
                    (
                        resampledMainBuf.getArrayOfWritePointers(),
                        resampledMainBuf.getNumChannels(),
                        resampledMainBuf.getNumSamples(),
                        scBuffer.getArrayOfWritePointers(),
                        scBuffer.getNumChannels()
                    );
                }
		}
        else
        {

        }
#else
        processBlockUpsampled
        (
            resampledMainBuf.getArrayOfWritePointers(),
            resampledMainBuf.getNumChannels(),
            resampledMainBuf.getNumSamples()
        );
#endif

#if PPDHasHQ
        oversampler.downsample(mainBuffer);
#endif

#if PPDHasStereoConfig
        if (midSideEnabled)
        {
            decodeMS(samples, numSamples, 0);
#if PPDHasSidechain
            encodeMS(samples, numSamples, 1);
#endif
        }
#endif
#if PPDHasGainOut
        dryWetMix.processOutGain(samples, numChannels, numSamples);
#endif
#if PPDHasTuningEditor
        tuningEditorSynth(samples, numChannels, numSamples);
#endif
#if PPDHasClipper
        {
            const auto isClipping = params[PID::Clipper]->getValMod() > .5f ? 1.f : 0.f;
            if (isClipping)
            {
                for (auto ch = 0; ch < numChannels; ++ch)
                    for (auto s = 0; s < numSamples; ++s)
                        samples[ch][s] = softclip(samples[ch][s], .6f);
            }
        }
#endif
#if PPDHasGainOut
        meters.processOut(constSamples, numChannels, numSamples);
#endif
#if PPD_MixOrGainDry
        if (!muteDry)
#endif
        dryWetMix.processMix
        (
            samples,
            numChannels,
            numSamples
#if PPDHasDelta
            , params[PID::Delta]->getValMod() > .5f
#endif
        );

#if JUCE_DEBUG
        for (auto ch = 0; ch < numChannels; ++ch)
        {
            auto smpls = samples[ch];

            for (auto s = 0; s < numSamples; ++s)
            {
                if (smpls[s] > 2.f)
                    smpls[s] = 2.f;
                else if (smpls[s] < -2.f)
                    smpls[s] = -2.f;
            }
        }
#endif
    }

    void Processor::processBlockBypassed(AudioBuffer& buffer, juce::MidiBuffer& midi)
    {
		const auto numChannels = buffer.getNumChannels();
		const auto numSamples = buffer.getNumSamples();
		auto samples = buffer.getArrayOfWritePointers();

        envGenMIDI.processBypassed(numSamples);
        oscope(envGenMIDI.data(), numSamples, playHeadPos);

        const auto mode = static_cast<int>(std::rint(params[PID::EnvGenMode]->getValModDenorm()));
        enum { DirectOut };
		if(mode == DirectOut)
            for (auto ch = 0; ch < numChannels; ++ch)
                SIMD::copy(samples[ch], envGenMIDI.data(), numSamples);
		
        ProcessorBackEnd::processBlockBypassed(buffer, midi);
    }

    void Processor::processBlockPreUpscaled(float** samples, int numChannels, int numSamples,
        MIDIBuffer& midi) noexcept
    {
        if (params[PID::Lookahead]->getValMod() > .5f)
        {
            wetLatencyCompensation(samples, numChannels, numSamples);
            const auto time = std::round(envGenMIDI.maxLatencySamples - envGenMIDI.getAttackLength(numSamples - 1));
            midiDelay
            (
                midi,
                numSamples,
                static_cast<int>(time)
            );
        }
		
        envGenMIDI
        (
            midi,
            numSamples,
            params[PID::EnvGenAttack]->getValModDenorm(),
			params[PID::EnvGenDecay]->getValModDenorm(),
			params[PID::EnvGenSustain]->getValMod(),
			params[PID::EnvGenRelease]->getValModDenorm(),
			params[PID::EnvGenAtkShape]->getValModDenorm(),
			params[PID::EnvGenDcyShape]->getValModDenorm(),
			params[PID::EnvGenRlsShape]->getValModDenorm(),
			static_cast<int>(std::round(params[PID::EnvGenLegato]->getValModDenorm())),
			params[PID::EnvGenInverse]->getValModDenorm() > .5f,
            params[PID::EnvGenVelocity]->getValMod(),
			params[PID::EnvGenTempoSync]->getValMod() > .5f,
			params[PID::EnvGenAttackBeats]->getValModDenorm(),
			params[PID::EnvGenDecayBeats]->getValModDenorm(),
			params[PID::EnvGenReleaseBeats]->getValModDenorm(),
            playHeadPos
        );

        auto envGenData = envGenMIDI.data();

        const auto lowerLimitBuf = lowerLimit(decibelToGain(params[PID::LowerLimit]->getValModDenorm(), -59.9f), numSamples);
		const auto upperLimitBuf = upperLimit(decibelToGain(params[PID::UpperLimit]->getValModDenorm(), -59.9f), numSamples);
        for (auto s = 0; s < numSamples; ++s)
			envGenData[s] = lowerLimitBuf[s] + envGenData[s] * (upperLimitBuf[s] - lowerLimitBuf[s]);

        oscope(envGenMIDI.data(), numSamples, playHeadPos);
		
        const auto mode = static_cast<int>(std::round(params[PID::EnvGenMode]->getValModDenorm()));
		enum { DirectOut, Gain, MIDICC, NumModes };
        int midiCh, midiCC;
        switch (mode)
        {
		case Gain:
			for (auto ch = 0; ch < numChannels; ++ch)
				SIMD::multiply(samples[ch], envGenData, numSamples);
			break;
		case MIDICC:
            midiCh = static_cast<int>(std::round(params[PID::ControllerChannel]->getValModDenorm()));
            midiCC = static_cast<int>(std::round(params[PID::ControllerCC]->getValModDenorm()));
            for (auto s = 0; s < numSamples; ++s)
            {
                const auto envScaled = std::round(envGenData[s] * 127.f);
                const auto val = static_cast<juce::uint8>(envScaled);
                const auto evt = MIDIMessage::controllerEvent(midiCh, midiCC, val);
                midi.addEvent(evt, s);
            }
            break;
        default: // DirectOut
            for (auto ch = 0; ch < numChannels; ++ch)
                SIMD::copy(samples[ch], envGenData, numSamples);
            break;
        }
    }

    void Processor::processBlockUpsampled(float**, int, int
#if PPDHasSidechain
        , float**, int
#endif
    ) noexcept
    {
        
    }

    void Processor::releaseResources() {}

    /////////////////////////////////////////////
    /////////////////////////////////////////////;
    void Processor::getStateInformation(juce::MemoryBlock& destData)
    {
        savePatch();
        state.savePatch(*this, destData);
    }

    void Processor::setStateInformation(const void* data, int sizeInBytes)
    {
        state.loadPatch(*this, data, sizeInBytes);
        loadPatch();
    }

    void Processor::savePatch()
    {
        ProcessorBackEnd::savePatch();
    }

    void Processor::loadPatch()
    {
        ProcessorBackEnd::loadPatch();
        forcePrepareToPlay();
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new audio::Processor();
}

// make default playheadpos