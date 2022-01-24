/*
 ==============================================================================

 This file is part of the JUCETICE project - Copyright 2007 by Lucio Asnaghi.

 JUCETICE is based around the JUCE library - "Jules' Utility Class Extensions"
 Copyright 2007 by Julian Storer.

 ------------------------------------------------------------------------------

 JUCE and JUCETICE can be redistributed and/or modified under the terms of
 the GNU Lesser General Public License, as published by the Free Software
 Foundation; either version 2 of the License, or (at your option) any later
 version.

 JUCE and JUCETICE are distributed in the hope that they will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU Lesser General Public License
 along with JUCE and JUCETICE; if not, visit www.gnu.org/licenses or write to
 Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 Boston, MA 02111-1307 USA

 ==============================================================================
*/

#ifndef __JUCETICE_JOSTVSTPLUGIN_HEADER__
#define __JUCETICE_JOSTVSTPLUGIN_HEADER__

#include "../BasePlugin.h"

#if JOST_USE_VST

#include "audioeffectx.h"
#include "AEffEditor.hpp"

#ifndef VstInt32
  #define VstInt32 long
#endif
#ifndef VstIntPtr
  #define VstIntPtr long
#endif


//==============================================================================
/**
    Our specific extended flags that saves us to call canDo in audio callbacks
*/
enum
{
    effFlagsExCanSendVstEvents          = 1 << 0,
    effFlagsExCanSendVstMidiEvents      = 1 << 1,
    effFlagsExCanSendVstTimeInfo        = 1 << 2,
    effFlagsExCanReceiveVstEvents       = 1 << 3,
    effFlagsExCanReceiveVstMidiEvents   = 1 << 4,
    effFlagsExCanReceiveVstTimeInfo     = 1 << 5,
    effFlagsExCanProcessOffline         = 1 << 6,
    effFlagsExCanUseAsInsert            = 1 << 7,
    effFlagsExCanUseAsSend              = 1 << 8,
    effFlagsExCanMixDryWet              = 1 << 9
};

//==============================================================================
/**
    This is a helper class, that will help our poor vst plugin understand
    its wacky vst event structure.
*/
class VstPluginMidiManager
{
public:

    //==============================================================================
    VstPluginMidiManager ();
    ~VstPluginMidiManager ();

    //==============================================================================
    void convertMidiMessages (MidiBuffer& midiMessages, const int blockSamples);

    //==============================================================================
    VstEvents* getMidiEvents ()         { return (VstEvents*) ptrEventBuffer; }

protected:

    // vst internal midi event buffer
    VstMidiEvent midiEventsBuffer [2048];
    char* ptrEventBuffer;
};


//==============================================================================
/**
    The magnificient (whuuuat?!) audio plugin interface. In all its messy !
*/
class VstPlugin : public BasePlugin,
                  public Timer
{
public:
    //==============================================================================
    VstPlugin ();
    ~VstPlugin ();

    //==============================================================================
    int getType () const                   { return JOST_PLUGINTYPE_VST; }

    //==============================================================================
    bool loadPluginFromFile (const File& filePath);
    File getFile () const;

    //==============================================================================
    const String getName () const;
    int getID () const;
    int getNumInputs () const;
    int getNumOutputs () const;
    int getNumMidiInputs () const;
    int getNumMidiOutputs () const;
    bool producesMidi () const;
    bool acceptsMidi () const;
    void* getLowLevelHandle ();

    //==============================================================================
    void processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages);
    void prepareToPlay (double sampleRate, int samplesPerBlock);
    void releaseResources();
    void timerCallback ();

    //==============================================================================
    void setParameterReal (int paramNumber, float value);
    float getParameterReal (int paramNumber);
    const String getParameterTextReal (int partNumber, float value);

    //==============================================================================
    int getNumPrograms ();
    void setCurrentProgram (int programNumber);
    int getCurrentProgram ();
    const String getProgramName (int programNumber);
    const String getCurrentProgramName ();
    void getStateInformation (MemoryBlock& destData);
    void setStateInformation (const void* data, int sizeInBytes);

    //==============================================================================
    bool hasEditor () const;
    bool wantsEditor () const;

    void getEditorSize (int& width, int& height);
    void openEditor (void* handle, void* display);
    void idleEditor ();
    void closeEditor ();

    //==============================================================================
    /** @internal */
    void sendMidiEventToOutput (VstEvents* events);
    /** @internal */
    int dispatch (const int opcode, const int index, const int value, void* const ptr, float opt) const;
    /** @internal */
    VstIntPtr handleCallback (VstInt32 opcode, VstInt32 index, VstInt32 value, void *ptr, float opt);

private:

    //==============================================================================
    /** @internal */
    bool canDo (const String& canDoString) const;

    //==============================================================================
    /** Some old plugins still may want to use host provided file selectors */
    int openFileSelector (VstFileSelect *ptr);
    int closeFileSelector (VstFileSelect *ptr);

    void* module;
    AEffect* effect;
    uint32 flagsEx;
    File pluginFile;

    VstPluginMidiManager midiManager;
    MidiBuffer sendToOutMidi;
    
    double sampleRate;
    int blockSize;
};

#endif // JOST_USE_VST

#endif // __JUCETICE_JOSTVSTPLUGIN_HEADER__
