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

#ifndef __JUCETICE_JOSTSEQUENCECOMPONENT_HEADER__
#define __JUCETICE_JOSTSEQUENCECOMPONENT_HEADER__

#include "../../../Config.h"
#include "../../../HostFilterComponent.h"
#include "../../../model/plugins/MidiSequencePlugin.h"
#include "../PluginEditorComponent.h"


//==============================================================================
/**
    This is the main content component for the vst window
*/
class SequenceComponent : public PluginEditorComponent,
                          public Timer,
                          public ChangeListener,
                          public ButtonListener,
                          public ComboBoxListener,
                          public SliderListener
{
public:
    //==============================================================================
    SequenceComponent (MidiSequencePlugin* plugin);
    ~SequenceComponent ();

    //==============================================================================
    int getPreferredWidth ()                        { return 580; }
    int getPreferredHeight ()                       { return 390; }
    bool isResizable ()                             { return true; }
    void updateParameters ();

    //==============================================================================
    /** Get the preferred width of the midi keyboard */
    int getMidiKeyboardWidth () const               { return 52; }

    //==============================================================================
    /** Implemented from SliderListener interface */
    void sliderValueChanged (Slider* sliderThatWasMoved);

    /** Implemented from ButtonListener interface */
    void buttonClicked (Button* buttonThatWasChanged);

    /** Implemented from ComboBoxListener interface */
    void comboBoxChanged (ComboBox* comboBoxThatHasChanged);

    /** Implemented from ChangeListener interface */
    void changeListenerCallback (void *objectThatHasChanged);

    //==============================================================================
    /** @internal */
    void timerCallback ();
    /** @internal */
    void mouseWheelMove (const MouseEvent& e, float incrementX, float incrementY);
    /** @internal */
    void resized ();

protected:

    /** Handy function that returns the type casted plugin */
    MidiSequencePlugin* getPlugin () const        { return (MidiSequencePlugin*) plugin; }

    Transport* transport;

    ImageSlider* zoomSlider;
    Slider* barSlider;

    ComboBox* quantizeBox;
    ComboBox* noteLengthBox;

    PianoGrid* pianoGrid;
    Viewport* pianoViewport;

    DropShadower* dropShadower;
    PianoGridKeyboard* midiKeyboard;
};


#endif
