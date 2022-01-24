/*
  ==============================================================================

   This file is part of the JUCE library - "Jules' Utility Class Extensions"
   Copyright 2004-7 by Raw Material Software ltd.

  ------------------------------------------------------------------------------

   JUCE can be redistributed and/or modified under the terms of the
   GNU General Public License, as published by the Free Software Foundation;
   either version 2 of the License, or (at your option) any later version.

   JUCE is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with JUCE; if not, visit www.gnu.org/licenses or write to the
   Free Software Foundation, Inc., 59 Temple Place, Suite 330,
   Boston, MA 02111-1307 USA

  ------------------------------------------------------------------------------

   If you'd like to release a closed-source product which uses JUCE, commercial
   licenses are also available: visit www.rawmaterialsoftware.com/juce for
   more information.

  ==============================================================================
*/

#include <Cocoa/Cocoa.h>
#include <AudioUnit/AUCocoaUIView.h>
#include <AudioUnit/AudioUnit.h>
#include <AudioToolbox/AudioUnitUtilities.h>
#include "AUMIDIEffectBase.h"
#include "MusicDeviceBase.h"
#include "../../juce_IncludeCharacteristics.h"
//#include "../../../../../juce_amalgamated.h"
#include "../../../../../juce.h"

//==============================================================================
#define juceFilterObjectPropertyID 0x1a45ffe9
static VoidArray activePlugins;

static const short channelConfigs[][2] = { JucePlugin_PreferredChannelConfigurations };
static const int numChannelConfigs = numElementsInArray (channelConfigs);

BEGIN_JUCE_NAMESPACE
extern void juce_setCurrentExecutableFileNameFromBundleId (const String& bundleId) throw();
END_JUCE_NAMESPACE

#if JucePlugin_IsSynth
 #define JuceAUBaseClass MusicDeviceBase
#else
 #define JuceAUBaseClass AUMIDIEffectBase
#endif

//==============================================================================
/** Somewhere in the codebase of your plugin, you need to implement this function
    and make it create an instance of the filter subclass that you're building.
*/
extern AudioProcessor* JUCE_CALLTYPE createPluginFilter();
class JuceAUView;

//==============================================================================
#define CONCAT_MACRO(Part1, Part2) Part1 ## Part2
#define JuceUICreationClass  CONCAT_MACRO (JucePlugin_AUExportPrefix, _UICreationClass)

@interface JuceUICreationClass   : NSObject <AUCocoaUIBase>
{
}

- (JuceUICreationClass*) init;
- (void) dealloc;
- (unsigned) interfaceVersion;
- (NSString *) description;
- (NSView*) uiViewForAudioUnit: (AudioUnit) inAudioUnit 
                      withSize: (NSSize) inPreferredSize;
@end


//==============================================================================
class JuceAU   : public JuceAUBaseClass,
                 public AudioProcessorListener,
                 public AudioPlayHead,
                 public ComponentListener
{
public:
    //==============================================================================
    JuceAU (AudioUnit component)
#if JucePlugin_IsSynth
        : MusicDeviceBase (component, 0, 1),
#else
        : AUMIDIEffectBase (component),
#endif
          juceFilter (0),
          bufferSpace (2, 16),
          channels (0),
          prepared (false)
    {
        if (activePlugins.size() == 0)
        {
            initialiseJuce_GUI();

#ifdef JucePlugin_CFBundleIdentifier
            juce_setCurrentExecutableFileNameFromBundleId (JucePlugin_CFBundleIdentifier);
#endif
        }

        juceFilter = createPluginFilter();
        juceFilter->setPlayHead (this);
        juceFilter->addListener (this);

        jassert (juceFilter != 0);
        Globals()->UseIndexedParameters (juceFilter->getNumParameters());

        activePlugins.add (this);

        zerostruct (auEvent);
        auEvent.mArgument.mParameter.mAudioUnit = GetComponentInstance();
        auEvent.mArgument.mParameter.mScope = kAudioUnitScope_Global;
        auEvent.mArgument.mParameter.mElement = 0;
    }

    ~JuceAU()
    {
        delete juceFilter;
        juceFilter = 0;

        juce_free (channels);
        channels = 0;

        jassert (activePlugins.contains (this));
        activePlugins.removeValue (this);

        if (activePlugins.size() == 0)
            shutdownJuce_GUI();
    }

    //==============================================================================
    ComponentResult GetPropertyInfo (AudioUnitPropertyID inID,
                                     AudioUnitScope inScope,
                                     AudioUnitElement inElement,
                                     UInt32& outDataSize,
                                     Boolean& outWritable)
    {
        if (inScope == kAudioUnitScope_Global)
        {
            if (inID == juceFilterObjectPropertyID)
            {
                outWritable = false;
                outDataSize = sizeof (void*) * 2;
                return noErr;
            }
            else if (inID == kAudioUnitProperty_OfflineRender)
            {
                outWritable = true;
                outDataSize = sizeof (UInt32);
                return noErr;
            }
            else if (inID == kMusicDeviceProperty_InstrumentCount)
            {
                outDataSize = sizeof (UInt32);
                outWritable = false;
                return noErr;
            }
            else if (inID == kAudioUnitProperty_CocoaUI)
            {
                outDataSize = sizeof (AudioUnitCocoaViewInfo);
                outWritable = true;
                return noErr;
            }
        }

        return JuceAUBaseClass::GetPropertyInfo (inID, inScope, inElement, outDataSize, outWritable);
    }

    ComponentResult GetProperty (AudioUnitPropertyID inID,
                                 AudioUnitScope inScope,
                                 AudioUnitElement inElement,
                                 void* outData)
    {
        if (inScope == kAudioUnitScope_Global)
        {
            if (inID == juceFilterObjectPropertyID)
            {
                ((void**) outData)[0] = (void*) juceFilter;
                ((void**) outData)[1] = (void*) this;
                return noErr;
            }
            else if (inID == kAudioUnitProperty_OfflineRender)
            {
                *(UInt32*) outData = (juceFilter != 0 && juceFilter->isNonRealtime()) ? 1 : 0;
                return noErr;
            }
            else if (inID == kMusicDeviceProperty_InstrumentCount)
            {
                *(UInt32*) outData = 1;
                return noErr;
            }
            else if (inID == kAudioUnitProperty_CocoaUI)
            {
                AudioUnitCocoaViewInfo* info = (AudioUnitCocoaViewInfo*) outData;
                NSBundle* b = [NSBundle bundleForClass: [JuceUICreationClass class]];

                info->mCocoaAUViewClass[0] = (CFStringRef) [[[JuceUICreationClass class] className] retain];
                info->mCocoaAUViewBundleLocation = (CFURLRef) [[NSURL fileURLWithPath: [b bundlePath]] retain];

                return noErr;
            }
        }

        return JuceAUBaseClass::GetProperty (inID, inScope, inElement, outData);
    }

    ComponentResult SetProperty (AudioUnitPropertyID inID,
                                 AudioUnitScope inScope,
                                 AudioUnitElement inElement,
                                 const void* inData,
                                 UInt32 inDataSize)
    {
        if (inScope == kAudioUnitScope_Global && inID == kAudioUnitProperty_OfflineRender)
        {
            if (juceFilter != 0)
                juceFilter->setNonRealtime ((*(UInt32*) inData) != 0);

            return noErr;
        }

        return JuceAUBaseClass::SetProperty (inID, inScope, inElement, inData, inDataSize);
    }

    ComponentResult SaveState (CFPropertyListRef* outData)
    {
        ComponentResult err = JuceAUBaseClass::SaveState (outData);

        if (err != noErr)
            return err;

        jassert (CFGetTypeID (*outData) == CFDictionaryGetTypeID());

        CFMutableDictionaryRef dict = (CFMutableDictionaryRef) *outData;

        if (juceFilter != 0)
        {
            MemoryBlock state;
            juceFilter->getCurrentProgramStateInformation (state);

            if (state.getSize() > 0)
            {
                CFDataRef ourState = CFDataCreate (kCFAllocatorDefault, (const UInt8*) state.getData(), state.getSize());
                CFDictionarySetValue (dict, CFSTR("jucePluginState"), ourState);
                CFRelease (ourState);
            }
        }

        return noErr;
    }

    ComponentResult RestoreState (CFPropertyListRef inData)
    {
        ComponentResult err = JuceAUBaseClass::RestoreState (inData);

        if (err != noErr)
            return err;

        if (juceFilter != 0)
        {
            CFDictionaryRef dict = (CFDictionaryRef) inData;
            CFDataRef data = 0;

            if (CFDictionaryGetValueIfPresent (dict, CFSTR("jucePluginState"),
                                               (const void**) &data))
            {
                if (data != 0)
                {
                    const int numBytes = (int) CFDataGetLength (data);
                    const JUCE_NAMESPACE::uint8* const rawBytes = CFDataGetBytePtr (data);

                    if (numBytes > 0)
                        juceFilter->setCurrentProgramStateInformation (rawBytes, numBytes);
                }
            }
        }

        return noErr;
    }

    UInt32 SupportedNumChannels (const AUChannelInfo** outInfo)
    {
        // You need to actually add some configurations to the JucePlugin_PreferredChannelConfigurations
        // value in your JucePluginCharacteristics.h file..
        jassert (numChannelConfigs > 0);

        if (outInfo != 0)
        {
            *outInfo = channelInfo;

            for (int i = 0; i < numChannelConfigs; ++i)
            {
#if JucePlugin_IsSynth
                channelInfo[i].inChannels = 0;
#else
                channelInfo[i].inChannels = channelConfigs[i][0];
#endif
                channelInfo[i].outChannels = channelConfigs[i][1];
            }
        }

        return numChannelConfigs;
    }

    //==============================================================================
    ComponentResult GetParameterInfo (AudioUnitScope inScope,
                                      AudioUnitParameterID inParameterID,
                                      AudioUnitParameterInfo& outParameterInfo)
    {
        const int index = (int) inParameterID;

        if (inScope == kAudioUnitScope_Global
             && juceFilter != 0
             && index < juceFilter->getNumParameters())
        {
            outParameterInfo.flags = kAudioUnitParameterFlag_IsWritable
                                      | kAudioUnitParameterFlag_IsReadable
                                      | kAudioUnitParameterFlag_HasCFNameString;

            const String name (juceFilter->getParameterName (index));

            // set whether the param is automatable (unnamed parameters aren't allowed to be automated)
            if (name.isEmpty() || ! juceFilter->isParameterAutomatable (index))
                outParameterInfo.flags |= kAudioUnitParameterFlag_NonRealTime;

            if (juceFilter->isMetaParameter (index))
                outParameterInfo.flags |= kAudioUnitParameterFlag_IsGlobalMeta;

            AUBase::FillInParameterName (outParameterInfo,
                                         PlatformUtilities::juceStringToCFString (name),
                                         false);

            outParameterInfo.minValue = 0.0f;
            outParameterInfo.maxValue = 1.0f;
            outParameterInfo.defaultValue = 0.0f;
            outParameterInfo.unit = kAudioUnitParameterUnit_Generic;

            return noErr;
        }
        else
        {
            return kAudioUnitErr_InvalidParameter;
        }
    }

    ComponentResult GetParameter (AudioUnitParameterID inID,
                                  AudioUnitScope inScope,
                                  AudioUnitElement inElement,
                                  Float32& outValue)
    {
        if (inScope == kAudioUnitScope_Global && juceFilter != 0)
        {
            outValue = juceFilter->getParameter ((int) inID);
            return noErr;
        }

        return AUBase::GetParameter (inID, inScope, inElement, outValue);
    }

    ComponentResult SetParameter (AudioUnitParameterID inID,
                                  AudioUnitScope inScope,
                                  AudioUnitElement inElement,
                                  Float32 inValue,
                                  UInt32 inBufferOffsetInFrames)
    {
        if (inScope == kAudioUnitScope_Global && juceFilter != 0)
        {
            juceFilter->setParameter ((int) inID, inValue);
            return noErr;
        }

        return AUBase::SetParameter (inID, inScope, inElement, inValue, inBufferOffsetInFrames);
    }

    //==============================================================================
    ComponentResult Version()                   { return JucePlugin_VersionCode; }

    bool SupportsTail()                         { return true; }
    Float64 GetTailTime()                       { return 0; }

    Float64 GetSampleRate()
    {
        return GetOutput(0)->GetStreamFormat().mSampleRate;
    }

    Float64 GetLatency()
    {
        jassert (GetSampleRate() > 0);

        if (GetSampleRate() <= 0)
            return 0.0;

        return juceFilter->getLatencySamples() / GetSampleRate();
    }

    //==============================================================================
    bool getCurrentPosition (AudioPlayHead::CurrentPositionInfo& info)
    {
        info.timeSigNumerator = 0;
        info.timeSigDenominator = 0;
        info.timeInSeconds = 0;
        info.editOriginTime = 0;
        info.ppqPositionOfLastBarStart = 0;
        info.isPlaying = false;
        info.isRecording = false;

        switch (lastSMPTETime.mType)
        {
            case kSMPTETimeType24:
                info.frameRate = AudioPlayHead::fps24;
                break;

            case kSMPTETimeType25:
                info.frameRate = AudioPlayHead::fps25;
                break;

            case kSMPTETimeType30Drop:
                info.frameRate = AudioPlayHead::fps30drop;
                break;

            case kSMPTETimeType30:
                info.frameRate = AudioPlayHead::fps30;
                break;

            case kSMPTETimeType2997:
                info.frameRate = AudioPlayHead::fps2997;
                break;

            case kSMPTETimeType2997Drop:
                info.frameRate = AudioPlayHead::fps2997drop;
                break;

            //case kSMPTETimeType60:
            //case kSMPTETimeType5994:
            default:
                info.frameRate = AudioPlayHead::fpsUnknown;
                break;
        }

        if (CallHostBeatAndTempo (&info.ppqPosition, &info.bpm) != noErr)
        {
            info.ppqPosition = 0;
            info.bpm = 0;
        }

        UInt32 outDeltaSampleOffsetToNextBeat;
        double outCurrentMeasureDownBeat;
        float num;
        UInt32 den;

        if (CallHostMusicalTimeLocation (&outDeltaSampleOffsetToNextBeat, &num, &den,
                                         &outCurrentMeasureDownBeat) == noErr)
        {
            info.timeSigNumerator = (int) num;
            info.timeSigDenominator = den;
            info.ppqPositionOfLastBarStart = outCurrentMeasureDownBeat;
        }

        double outCurrentSampleInTimeLine, outCycleStartBeat, outCycleEndBeat;
        Boolean playing, playchanged, looping;

        if (CallHostTransportState (&playing,
                                    &playchanged,
                                    &outCurrentSampleInTimeLine,
                                    &looping,
                                    &outCycleStartBeat,
                                    &outCycleEndBeat) == noErr)
        {
            info.isPlaying = playing;
            info.timeInSeconds = outCurrentSampleInTimeLine / GetSampleRate();
        }

        return true;
    }

    void sendAUEvent (const AudioUnitEventType type, const int index) throw()
    {
        if (AUEventListenerNotify != 0)
        {
            auEvent.mEventType = type;
            auEvent.mArgument.mParameter.mParameterID = (AudioUnitParameterID) index;
            AUEventListenerNotify (0, 0, &auEvent);
        }
    }

    void audioProcessorParameterChanged (AudioProcessor*, int index, float newValue)
    {
        sendAUEvent (kAudioUnitEvent_ParameterValueChange, index);
    }

    void audioProcessorParameterChangeGestureBegin (AudioProcessor*, int index)
    {
        sendAUEvent (kAudioUnitEvent_BeginParameterChangeGesture, index);
    }

    void audioProcessorParameterChangeGestureEnd (AudioProcessor*, int index)
    {
        sendAUEvent (kAudioUnitEvent_EndParameterChangeGesture, index);
    }

    void audioProcessorChanged (AudioProcessor*)
    {
        // xxx is there an AU equivalent?
    }

    bool StreamFormatWritable (AudioUnitScope inScope, AudioUnitElement element)
    {
        return ! IsInitialized();
    }

    // (these two slightly different versions are because the definition changed between 10.4 and 10.5)
    ComponentResult StartNote (MusicDeviceInstrumentID, MusicDeviceGroupID, NoteInstanceID&, UInt32, const MusicDeviceNoteParams&) { return noErr; }
    ComponentResult StartNote (MusicDeviceInstrumentID, MusicDeviceGroupID, NoteInstanceID*, UInt32, const MusicDeviceNoteParams&) { return noErr; }
    ComponentResult StopNote (MusicDeviceGroupID, NoteInstanceID, UInt32)   { return noErr; }

    //==============================================================================
    ComponentResult Initialize()
    {
#if ! JucePlugin_IsSynth
        const int numIns = GetInput(0) != 0 ? GetInput(0)->GetStreamFormat().mChannelsPerFrame : 0;
#endif
        const int numOuts = GetOutput(0) != 0 ? GetOutput(0)->GetStreamFormat().mChannelsPerFrame : 0;

        bool isValidChannelConfig = false;

        for (int i = 0; i < numChannelConfigs; ++i)
#if JucePlugin_IsSynth
            if (numOuts == channelConfigs[i][1])
#else
            if (numIns == channelConfigs[i][0] && numOuts == channelConfigs[i][1])
#endif
                isValidChannelConfig = true;

        if (! isValidChannelConfig)
            return kAudioUnitErr_FormatNotSupported;

        JuceAUBaseClass::Initialize();
        prepareToPlay();
        return noErr;
    }

    void Cleanup()
    {
        JuceAUBaseClass::Cleanup();

        if (juceFilter != 0)
            juceFilter->releaseResources();

        bufferSpace.setSize (2, 16);
        midiEvents.clear();
        prepared = false;
    }

    ComponentResult Reset (AudioUnitScope inScope, AudioUnitElement inElement)
    {
        if (! prepared)
            prepareToPlay();

        if (juceFilter != 0)
            juceFilter->reset();

        return JuceAUBaseClass::Reset (inScope, inElement);
    }

    void prepareToPlay()
    {
        if (juceFilter != 0)
        {
#if ! JucePlugin_IsSynth
            juceFilter->setPlayConfigDetails (GetInput(0)->GetStreamFormat().mChannelsPerFrame,
#else
            juceFilter->setPlayConfigDetails (0,
#endif
                                              GetOutput(0)->GetStreamFormat().mChannelsPerFrame,
                                              GetSampleRate(),
                                              GetMaxFramesPerSlice());

            bufferSpace.setSize (juceFilter->getNumInputChannels() + juceFilter->getNumOutputChannels(),
                                 GetMaxFramesPerSlice() + 32);

            juceFilter->prepareToPlay (GetSampleRate(),
                                       GetMaxFramesPerSlice());

            midiEvents.clear();

            juce_free (channels);
            channels = (float**) juce_calloc (sizeof (float*) * jmax (juceFilter->getNumInputChannels(),
                                                                      juceFilter->getNumOutputChannels()) + 4);

            prepared = true;
        }
    }

    ComponentResult Render (AudioUnitRenderActionFlags &ioActionFlags,
                            const AudioTimeStamp& inTimeStamp,
                            UInt32 nFrames)
    {
        lastSMPTETime = inTimeStamp.mSMPTETime;

#if ! JucePlugin_IsSynth
        return JuceAUBaseClass::Render (ioActionFlags, inTimeStamp, nFrames);
#else
        // synths can't have any inputs..
        AudioBufferList inBuffer;
        inBuffer.mNumberBuffers = 0;

        return ProcessBufferLists (ioActionFlags, inBuffer, GetOutput(0)->GetBufferList(), nFrames);
#endif
    }


    OSStatus ProcessBufferLists (AudioUnitRenderActionFlags& ioActionFlags,
                                 const AudioBufferList& inBuffer,
                                 AudioBufferList& outBuffer,
                                 UInt32 numSamples)
    {
        if (juceFilter != 0)
        {
            jassert (prepared);

            int numOutChans = 0;
            int nextSpareBufferChan = 0;
            bool needToReinterleave = false;
            const int numIn = juceFilter->getNumInputChannels();
            const int numOut = juceFilter->getNumOutputChannels();

            unsigned int i;
            for (i = 0; i < outBuffer.mNumberBuffers; ++i)
            {
                AudioBuffer& buf = outBuffer.mBuffers[i];

                if (buf.mNumberChannels == 1)
                {
                    channels [numOutChans++] = (float*) buf.mData;
                }
                else
                {
                    needToReinterleave = true;

                    for (unsigned int subChan = 0; subChan < buf.mNumberChannels && numOutChans < numOut; ++subChan)
                        channels [numOutChans++] = bufferSpace.getSampleData (nextSpareBufferChan++);
                }

                if (numOutChans >= numOut)
                    break;
            }

            int numInChans = 0;

            for (i = 0; i < inBuffer.mNumberBuffers; ++i)
            {
                const AudioBuffer& buf = inBuffer.mBuffers[i];

                if (buf.mNumberChannels == 1)
                {
                    if (numInChans < numOutChans)
                        memcpy (channels [numInChans], (const float*) buf.mData, sizeof (float) * numSamples);
                    else
                        channels [numInChans] = (float*) buf.mData;

                    ++numInChans;
                }
                else
                {
                    // need to de-interleave..
                    for (unsigned int subChan = 0; subChan < buf.mNumberChannels && numInChans < numIn; ++subChan)
                    {
                        float* dest;

                        if (numInChans < numOutChans)
                        {
                            dest = channels [numInChans++];
                        }
                        else
                        {
                            dest = bufferSpace.getSampleData (nextSpareBufferChan++);
                            channels [numInChans++] = dest;
                        }

                        const float* src = ((const float*) buf.mData) + subChan;

                        for (int j = numSamples; --j >= 0;)
                        {
                            *dest++ = *src;
                            src += buf.mNumberChannels;
                        }
                    }
                }

                if (numInChans >= numIn)
                    break;
            }

            {
                AudioSampleBuffer buffer (channels, jmax (numIn, numOut), numSamples);

                const ScopedLock sl (juceFilter->getCallbackLock());

                if (juceFilter->isSuspended())
                {
                    for (int i = 0; i < numOut; ++i)
                        zeromem (channels [i], sizeof (float) * numSamples);
                }
                else
                {
                    juceFilter->processBlock (buffer, midiEvents);
                }
            }

            if (! midiEvents.isEmpty())
            {
#if JucePlugin_ProducesMidiOutput
                const JUCE_NAMESPACE::uint8* midiEventData;
                int midiEventSize, midiEventPosition;
                MidiBuffer::Iterator i (midiEvents);

                while (i.getNextEvent (midiEventData, midiEventSize, midiEventPosition))
                {
                    jassert (((unsigned int) midiEventPosition) < (unsigned int) numSamples);



                    //xxx
                }
#else
                // if your plugin creates midi messages, you'll need to set
                // the JucePlugin_ProducesMidiOutput macro to 1 in your
                // JucePluginCharacteristics.h file
                //jassert (midiEvents.getNumEvents() <= numMidiEventsComingIn);
#endif
                midiEvents.clear();
            }

            if (needToReinterleave)
            {
                nextSpareBufferChan = 0;

                for (i = 0; i < outBuffer.mNumberBuffers; ++i)
                {
                    AudioBuffer& buf = outBuffer.mBuffers[i];

                    if (buf.mNumberChannels > 1)
                    {
                        for (unsigned int subChan = 0; subChan < buf.mNumberChannels; ++subChan)
                        {
                            const float* src = bufferSpace.getSampleData (nextSpareBufferChan++);
                            float* dest = ((float*) buf.mData) + subChan;

                            for (int j = numSamples; --j >= 0;)
                            {
                                *dest = *src++;
                                dest += buf.mNumberChannels;
                            }
                        }
                    }
                }
            }

#if ! JucePlugin_SilenceInProducesSilenceOut
            ioActionFlags &= ~kAudioUnitRenderAction_OutputIsSilence;
#endif
        }

        return noErr;
    }

protected:
    OSStatus HandleMidiEvent (UInt8 nStatus,
                              UInt8 inChannel,
                              UInt8 inData1,
                              UInt8 inData2,
                              long inStartFrame)
    {
#if JucePlugin_WantsMidiInput
        JUCE_NAMESPACE::uint8 data [4];
        data[0] = nStatus | inChannel;
        data[1] = inData1;
        data[2] = inData2;

        midiEvents.addEvent (data, 3, inStartFrame);
#endif

        return noErr;
    }

    //==============================================================================
    ComponentResult GetPresets (CFArrayRef* outData) const
    {
        if (outData != 0)
        {
            const int numPrograms = juceFilter->getNumPrograms();
            presetsArray.ensureSize (sizeof (AUPreset) * numPrograms, true);
            AUPreset* const presets = (AUPreset*) presetsArray.getData();

            CFMutableArrayRef presetsArray = CFArrayCreateMutable (0, numPrograms, 0);

            for (int i = 0; i < numPrograms; ++i)
            {
                presets[i].presetNumber = i;
                presets[i].presetName = PlatformUtilities::juceStringToCFString (juceFilter->getProgramName (i));

                CFArrayAppendValue (presetsArray, presets + i);
            }

            *outData = (CFArrayRef) presetsArray;
        }

        return noErr;
    }

    OSStatus NewFactoryPresetSet (const AUPreset& inNewFactoryPreset)
    {
        const int numPrograms = juceFilter->getNumPrograms();
        const SInt32 chosenPresetNumber = (int) inNewFactoryPreset.presetNumber;

        if (chosenPresetNumber >= numPrograms)
            return kAudioUnitErr_InvalidProperty;

        AUPreset chosenPreset;
        chosenPreset.presetNumber = chosenPresetNumber;
        chosenPreset.presetName = PlatformUtilities::juceStringToCFString (juceFilter->getProgramName (chosenPresetNumber));

        juceFilter->setCurrentProgram (chosenPresetNumber);
        SetAFactoryPresetAsCurrent (chosenPreset);

        return noErr;
    }

    void componentMovedOrResized (Component& component, bool wasMoved, bool wasResized)
    {
        if (wasResized)
        {
            NSView* view = (NSView*) component.getWindowHandle();
            [[view superview] setFrameSize: NSMakeSize (component.getWidth(), component.getHeight())];
            [view setFrame: NSMakeRect (0, 0, component.getWidth(), component.getHeight())];
            [view setNeedsDisplay: YES];
        }
    }

    //==============================================================================
private:
    AudioProcessor* juceFilter;
    AudioSampleBuffer bufferSpace;
    float** channels;
    MidiBuffer midiEvents;
    bool prepared;
    SMPTETime lastSMPTETime;
    AUChannelInfo channelInfo [numChannelConfigs];
    AudioUnitEvent auEvent;
    mutable MemoryBlock presetsArray;
};

//==============================================================================
@interface JuceUIViewClass : NSView
{
    AudioProcessor* filter;
    JuceAU* au;
    AudioProcessorEditor* editorComp;
}

- (JuceUIViewClass*) initWithFilter: (AudioProcessor*) filter
                             withAU: (JuceAU*) au
                      withComponent: (AudioProcessorEditor*) editorComp;
- (void) dealloc;
- (void) viewDidMoveToWindow;
- (BOOL) mouseDownCanMoveWindow;

@end

@implementation JuceUIViewClass

- (JuceUIViewClass*) initWithFilter: (AudioProcessor*) filter_
                             withAU: (JuceAU*) au_
                      withComponent: (AudioProcessorEditor*) editorComp_
{
    filter = filter_;
    au = au_;
    editorComp = editorComp_;

    [super initWithFrame: NSMakeRect (0, 0, editorComp_->getWidth(), editorComp_->getHeight())];
    [self setHidden: NO];
    [self setPostsFrameChangedNotifications: YES];

    editorComp->addToDesktop (0, (void*) self);
    editorComp->setVisible (true);
    editorComp->addComponentListener (au);

    return self;
}

- (void) dealloc
{
    // there's some kind of component currently modal, but the host
    // is trying to delete our plugin..
    jassert (Component::getCurrentlyModalComponent() == 0);

    if (editorComp != 0)
        filter->editorBeingDeleted (editorComp);

    deleteAndZero (editorComp);

    [super dealloc];
}

- (void) viewDidMoveToWindow
{
    if ([self window] != 0)
    {
        [[self window] setAcceptsMouseMovedEvents: YES];

        if (editorComp != 0)
            [[self window] makeFirstResponder: (NSView*) editorComp->getWindowHandle()];
    }
}

- (BOOL) mouseDownCanMoveWindow
{
    return NO;
}

@end

@implementation JuceUICreationClass

- (JuceUICreationClass*) init
{
    return [super init];
}

- (void) dealloc
{
    [super dealloc];
}

- (unsigned) interfaceVersion
{
    return 0;
}

- (NSString*) description
{
	return [NSString stringWithString: @JucePlugin_Name];
}

- (NSView*) uiViewForAudioUnit: (AudioUnit) inAudioUnit 
                      withSize: (NSSize) inPreferredSize
{
    void* pointers[2];
    UInt32 propertySize = sizeof (pointers);

    if (AudioUnitGetProperty (inAudioUnit,
                              juceFilterObjectPropertyID,
                              kAudioUnitScope_Global,
                              0,
                              pointers,
                              &propertySize) != noErr)
        return 0;

    AudioProcessor* filter = (AudioProcessor*) pointers[0];
    JuceAU* au = (JuceAU*) pointers[1];

    if (filter == 0)
        return 0;

    AudioProcessorEditor* editorComp = filter->createEditorIfNeeded();
    
    if (editorComp == 0)
        return 0;
DBG (String (inPreferredSize.width) + " " + String (inPreferredSize.height));
DBG (String (editorComp->getWidth()) + " " + String (editorComp->getHeight()));

    return [[[JuceUIViewClass alloc] initWithFilter: filter
                                             withAU: au
                                      withComponent: editorComp] autorelease];
}
@end

//==============================================================================
#define JUCE_COMPONENT_ENTRYX(Class, Name, Suffix) \
extern "C" __attribute__((visibility("default"))) ComponentResult Name ## Suffix (ComponentParameters* params, Class* obj); \
extern "C" __attribute__((visibility("default"))) ComponentResult Name ## Suffix (ComponentParameters* params, Class* obj) \
{ \
    return ComponentEntryPoint<Class>::Dispatch(params, obj); \
}

#define JUCE_COMPONENT_ENTRY(Class, Name, Suffix) JUCE_COMPONENT_ENTRYX(Class, Name, Suffix)

JUCE_COMPONENT_ENTRY (JuceAU, JucePlugin_AUExportPrefix, Entry)
//JUCE_COMPONENT_ENTRY (JuceAUView, JucePlugin_AUExportPrefix, ViewEntry)
