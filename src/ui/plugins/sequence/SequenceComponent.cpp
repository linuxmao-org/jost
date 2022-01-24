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

#include "SequenceComponent.h"

//==============================================================================
SequenceComponent::SequenceComponent (MidiSequencePlugin* plugin_)
  : PluginEditorComponent (plugin_),
    zoomSlider (0),
    pianoGrid (0),
    pianoViewport (0),
    dropShadower (0),
    midiKeyboard (0)
{
    MidiSequencePlugin* seq = getPlugin ();

    transport = seq->getParentHost ()->getTransport();

    seq->addChangeListener (this);
    transport->addChangeListener (this);

    // grid
    pianoGrid = new PianoGrid ();
    pianoGrid->setTimeDivision (transport->getTimeDenominator());
    pianoGrid->setSnapQuantize (seq->getIntValue (PROP_SEQNOTESNAP, 4));
    pianoGrid->setNumBars (seq->getIntValue (PROP_SEQBAR, transport->getNumBars()));
    pianoGrid->setBarWidth (seq->getIntValue (PROP_SEQCOLSIZE, 130));
    pianoGrid->setRowsOffset (seq->getIntValue (PROP_SEQROWOFFSET, 0));
    pianoGrid->setListener (seq);

    addAndMakeVisible (pianoViewport = new Viewport (String::empty));
    pianoViewport->setScrollBarsShown (false, true);
    pianoViewport->setScrollBarThickness (12);
    pianoViewport->setViewedComponent (pianoGrid);

    updateParameters ();

    // midi keyboard
    addAndMakeVisible (midiKeyboard = new PianoGridKeyboard (plugin->getKeyboardState ()));
    midiKeyboard->setMidiChannel (1);
    midiKeyboard->setMidiChannelsToDisplay (1 << 0);
    midiKeyboard->setKeyPressBaseOctave (3);
    midiKeyboard->setLowestVisibleKey (pianoGrid->getRowsOffset());
    midiKeyboard->setKeyWidth (12); // fixed !
    midiKeyboard->setAvailableRange (0, 119);
    midiKeyboard->setScrollButtonsVisible (true);
    midiKeyboard->addChangeListener (this);

    dropShadower = new DropShadower (0.4f, 2, 0, 2.5f);
    dropShadower->setOwner (midiKeyboard);

    // note length combobox
    addAndMakeVisible (noteLengthBox = new ComboBox (String::empty));
    noteLengthBox->setEditableText (false);
    noteLengthBox->setJustificationType (Justification::centredLeft);
    noteLengthBox->addItem (T("64th"), 64);
    noteLengthBox->addItem (T("32th"), 32);
    noteLengthBox->addItem (T("16th"), 16);
    noteLengthBox->addItem (T("8th"), 8);
    noteLengthBox->addItem (T("beat"), 4);
    noteLengthBox->addItem (T("1/2 bar"), 2);
    noteLengthBox->addItem (T("1 bar"), 1);
    noteLengthBox->setTooltip (T("Note length"));
    noteLengthBox->addListener (this);

    noteLengthBox->setSelectedId (seq->getIntValue (PROP_SEQNOTELENGTH, 4));

    // bar count slider
    addAndMakeVisible (barSlider = new Slider (String::empty));
    barSlider->setRange (1, 16, 1);
    barSlider->setSliderStyle (Slider::IncDecButtons);
    barSlider->setTextBoxStyle (Slider::TextBoxLeft, false, 80, 20);
    barSlider->setValue (transport->getNumBars (), false);
    barSlider->setTooltip (T("Bar count"));
    barSlider->addListener (this);

    // quantize box
    addAndMakeVisible (quantizeBox = new ComboBox (String::empty));
    quantizeBox->setEditableText (false);
    quantizeBox->setJustificationType (Justification::centredLeft);
    quantizeBox->addItem (T("off"), 1);
    quantizeBox->addItem (T("1 bar"), 2);
    quantizeBox->addItem (T("1/2 bar"), 3);
    quantizeBox->addItem (T("beat"), 5);
    quantizeBox->addItem (T("8th"), 9);
    quantizeBox->addItem (T("16th"), 17);
    quantizeBox->addItem (T("32th"), 33);
    quantizeBox->addItem (T("64th"), 65);
    quantizeBox->setTooltip (T("Snap"));
    quantizeBox->addListener (this);

    quantizeBox->setSelectedId (seq->getIntValue (PROP_SEQNOTESNAP, 4) + 1);

    // zoom slider
    addAndMakeVisible (zoomSlider = new ImageSlider (String::empty));
    zoomSlider->setOrientation (ImageSlider::LinearHorizontal);
    zoomSlider->setRange (10, 1024, 1);
    zoomSlider->setSkewFactor (0.5f);
    zoomSlider->setValue (pianoGrid->getBarWidth (), false);
    zoomSlider->setSliderStyle (Slider::LinearHorizontal);
    zoomSlider->setTextBoxStyle (Slider::NoTextBox, false, 80, 20);
    zoomSlider->setTooltip (T("Zoom factor"));
    zoomSlider->addListener (this);

    if (transport->isPlaying ())
        startTimer (1000 / 20); // 20 frames per seconds
}

SequenceComponent::~SequenceComponent ()
{
    transport->removeChangeListener (this);

    deleteAndZero (dropShadower);

    deleteAllChildren ();
}

//==============================================================================
void SequenceComponent::resized ()
{
    int keysWidth = getMidiKeyboardWidth ();
    int keysButtonHeight = 12;
    int headHeight = 32;

    // header
    noteLengthBox->setBounds (getWidth () - 340, 2, 60, 16);
    barSlider->setBounds (getWidth () - 260, 2, 60, 16);
    quantizeBox->setBounds (getWidth () - 180, 2, 60, 16);
    zoomSlider->setBounds (getWidth () - 100, 2, 100, 16);

    // content
    midiKeyboard->setBounds (0, headHeight, keysWidth, getHeight() - headHeight);
    pianoViewport->setBounds (keysWidth,
                              headHeight,
                              getWidth () - keysWidth,
                              getHeight() - headHeight);

    pianoGrid->setSize (300, getHeight() - headHeight - keysButtonHeight);
    pianoGrid->updateSize ();
}

//==============================================================================
void SequenceComponent::mouseWheelMove (const MouseEvent& e,
                                        float incrementX,
                                        float incrementY)
{
    int note = pianoGrid->getRowsOffset();

    if (incrementY < 0)
        note = (note - 1) / 12;
    else
        note = note / 12 + 1;

    note = jmin (jmax (note * 12, 0), 127 - pianoGrid->getVisibleRows ());

    midiKeyboard->setLowestVisibleKey (note);

    pianoGrid->setRowsOffset (midiKeyboard ? midiKeyboard->getLowestVisibleKey() : note);
    pianoGrid->resized ();

    // update plugin
    getPlugin ()->setValue (PROP_SEQROWOFFSET, midiKeyboard->getLowestVisibleKey());
}

//==============================================================================
void SequenceComponent::updateParameters ()
{
    MidiSequencePlugin* sequencer = getPlugin ();

    pianoGrid->removeAllNotes (false);
    for (int i = 0; i < sequencer->getNumNoteOn (); i++)
    {
        int note = -1;
        float beat = -1;
        float length = 0.0f;
        sequencer->getNoteOnIndexed (i, note, beat, length);

        if (length > 0.0f)
            pianoGrid->addNote (note, beat, length);
    }

    pianoGrid->resized ();
}

//==============================================================================
void SequenceComponent::changeListenerCallback (void *objectThatHasChanged)
{
    MidiSequencePlugin* seq = getPlugin ();

    if (objectThatHasChanged == midiKeyboard)
    {
        pianoGrid->setRowsOffset (midiKeyboard->getLowestVisibleKey());
        pianoGrid->resized ();

        // update plugin
        seq->setValue (PROP_SEQROWOFFSET, midiKeyboard->getLowestVisibleKey());
    }
    else if (objectThatHasChanged == transport)
    {
        if (transport->isPlaying ())
        {
            startTimer (1000 / 20); // 20 frames per seconds
        }
        else if (! transport->isPlaying ())
        {
            stopTimer ();
        }

        pianoGrid->setIndicatorPosition (transport->getPositionAbsolute ());
    }
    else if (objectThatHasChanged == seq)
    {
        updateParameters ();
    }
}

void SequenceComponent::buttonClicked (Button* buttonThatWasClicked)
{
}

void SequenceComponent::comboBoxChanged (ComboBox* comboBoxThatHasChanged)
{
    MidiSequencePlugin* seq = getPlugin ();

    if (comboBoxThatHasChanged == quantizeBox)
    {
        pianoGrid->setSnapQuantize (quantizeBox->getSelectedId () - 1);
    }
    else if (comboBoxThatHasChanged == noteLengthBox)
    {
        float noteLength = pianoGrid->getTimeDivision () / (float) noteLengthBox->getSelectedId ();

        pianoGrid->setNoteLengthInBeats (noteLength);

        seq->setValue (PROP_SEQNOTELENGTH, noteLengthBox->getSelectedId ());
    }
}

void SequenceComponent::sliderValueChanged (Slider* sliderThatWasMoved)
{
    MidiSequencePlugin* seq = getPlugin ();

    if (sliderThatWasMoved == zoomSlider)
    {
        const int newBarSize = roundFloatToInt (zoomSlider->getValue ());
        pianoGrid->setBarWidth (newBarSize);

        // update plugin
        seq->setValue (PROP_SEQCOLSIZE, newBarSize);
    }
    else if (sliderThatWasMoved == barSlider)
    {
        const int newBarCount = roundFloatToInt (barSlider->getValue ());
        pianoGrid->setNumBars (newBarCount);
        pianoGrid->notifyListenersOfTimeSignatureChange ();

        // update plugin
        seq->setValue (PROP_SEQBAR, newBarCount);

        pianoGrid->resized();
    }
}

//==============================================================================
void SequenceComponent::timerCallback ()
{
    pianoGrid->setIndicatorPosition (transport->getPositionAbsolute ());
}

