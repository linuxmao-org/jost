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

#include "PluginLoader.h"


//==============================================================================
bool PluginLoader::canUnderstand (const File& file)
{
    DBG ("PluginLoader::canUnderstand");

    if (! file.exists ())
    {
        printf ("Plugin %s doesn't exists on disk !", (const char*) file.getFullPathName ());
        return false;
    }

    // try with VST
#if JOST_USE_VST
    {
        VstPlugin* loadedPlugin = new VstPlugin ();
        bool loadedOk = loadedPlugin->loadPluginFromFile (file);
        
        if (loadedOk)
        {
            deleteAndZero (loadedPlugin);
            return true;
        }
        
        deleteAndZero (loadedPlugin);
    }
#endif

    // try with DSSI
#if JUCE_ALSA && JOST_USE_DSSI
    {
        DssiPlugin* loadedPlugin = new DssiPlugin ();
        bool loadedOk = loadedPlugin->loadPluginFromFile (file);
        
        if (loadedOk)
        {
            deleteAndZero (loadedPlugin);
            return true;
        }
        
        deleteAndZero (loadedPlugin);
    }
#endif

    // try with LADSPA
#if JOST_USE_LADSPA
    {
        LadspaPlugin* loadedPlugin = new LadspaPlugin ();
        bool loadedOk = loadedPlugin->loadPluginFromFile (file);
        
        if (loadedOk)
        {
            deleteAndZero (loadedPlugin);
            return true;
        }
        
        deleteAndZero (loadedPlugin);
    }
#endif

    return false;
}

//==============================================================================
BasePlugin* PluginLoader::getFromFile (const File& file)
{    DBG ("PluginLoader::getFromFile");

    BasePlugin* loadedPlugin = 0;

    if (! file.exists ())
    {
        printf ("Plugin %s doesn't exists on disk !", (const char*) file.getFullPathName ());
        return loadedPlugin;
    }

    // try with VST
#if JOST_USE_VST
    {
        loadedPlugin = new VstPlugin ();
        if (loadedPlugin->loadPluginFromFile (file))
            return loadedPlugin;
        deleteAndZero (loadedPlugin);
    }
#endif

    // try with DSSI
#if JUCE_ALSA && JOST_USE_DSSI
    {
        loadedPlugin = new DssiPlugin ();
        if (loadedPlugin->loadPluginFromFile (file))
            return loadedPlugin;
        deleteAndZero (loadedPlugin);
    }
#endif

    // try with LADSPA
#if JOST_USE_LADSPA
    {
        loadedPlugin = new LadspaPlugin ();
        if (loadedPlugin->loadPluginFromFile (file))
            return loadedPlugin;
        deleteAndZero (loadedPlugin);
    }
#endif

    return 0;
}

//==============================================================================
BasePlugin* PluginLoader::getFromTypeID (const int typeID,
                                         BasePlugin* inputPlugin,
                                         BasePlugin* outputPlugin)
{
    DBG ("PluginLoader::getFromTypeID");

    BasePlugin* plugin = 0;
    
    switch (typeID)
    {
    case JOST_PLUGINTYPE_INPUT:
        plugin = inputPlugin;
        break;
    case JOST_PLUGINTYPE_OUTPUT:
        plugin = outputPlugin;
        break;
    case JOST_PLUGINTYPE_MIDIIN:
        plugin = new MidiInputPlugin (0);
        break;
    case JOST_PLUGINTYPE_MIDIOUT:
        plugin = new MidiOutputPlugin (0);
        break;
    case JOST_PLUGINTYPE_MIDISEQ:
        plugin = new MidiSequencePlugin ();
        break;
    case JOST_PLUGINTYPE_MIDIKEY:
        plugin = new MidiKeyboardPlugin ();
        break;
    case JOST_PLUGINTYPE_MIDIMONITOR:
        plugin = new MidiMonitorPlugin ();
        break;
    case JOST_PLUGINTYPE_MIDIFILTER:
        plugin = new MidiFilterPlugin ();
        break;
    case JOST_PLUGINTYPE_MIDIPADS:
        plugin = new MidiPadsPlugin ();
        break;
    case JOST_PLUGINTYPE_AUDIOSPECMETER:
        plugin = new AudioSpecMeterPlugin ();
        break;
    case JOST_PLUGINTYPE_VST:
    case JOST_PLUGINTYPE_LADSPA:
    case JOST_PLUGINTYPE_DSSI:
        break;
    default:
        jassertfalse
        // You are trying to instantiate an internal plugin which doesn't exist !
        // Probably you have not keep in sync this function with handlePopupMenu one...
        break;
    }
    
    return plugin;
}

//==============================================================================
BasePlugin* PluginLoader::handlePopupMenu ()
{
    DBG ("PluginLoader::handlePopupMenu");

    PopupMenu menu;
    menu.addItem (JOST_PLUGINTYPE_MIDIIN,          "Add midi input");
    menu.addItem (JOST_PLUGINTYPE_MIDIOUT,         "Add midi output");
    menu.addItem (JOST_PLUGINTYPE_MIDISEQ,         "Add midi sequencer");
#if 1
    menu.addItem (JOST_PLUGINTYPE_MIDIKEY,         "Add midi controller");
#endif
    menu.addItem (JOST_PLUGINTYPE_MIDIPADS,        "Add midi pads");
#if 1
    menu.addItem (JOST_PLUGINTYPE_MIDIFILTER,      "Add midi filter");
#endif
    menu.addItem (JOST_PLUGINTYPE_MIDIMONITOR,     "Add midi monitor");
    menu.addSeparator ();
    menu.addItem (JOST_PLUGINTYPE_AUDIOSPECMETER,  "Add audio meter");

    BasePlugin* plugin = 0;

    const int result = menu.show();
    if (result)
        plugin = PluginLoader::getFromTypeID (result, 0, 0);
        
    return plugin;
}

