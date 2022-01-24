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

#include "VstPluginExternalEditor.h"
#include "VstPluginWindow.h"

#ifdef LINUX

  BEGIN_JUCE_NAMESPACE
    extern Display* display;
    extern XContext improbableNumber;
  END_JUCE_NAMESPACE

  bool g_xerror_value;
  int temporaryErrorHandler (Display *dp, XErrorEvent *e)
  {
      g_xerror_value = true;
      return 0;
  }

  int obtainPropertyFromXWindow (Window handle, Atom atom)
  {
      int result = 0, userSize;
      unsigned long bytes, userCount;
      unsigned char *data;
      Atom userType;
      g_xerror_value = false;
      XErrorHandler oldErrorHandler = XSetErrorHandler (temporaryErrorHandler);
      XGetWindowProperty (display, handle, atom, 0, 1, false, AnyPropertyType,
                          &userType,  &userSize, &userCount, &bytes, &data);
      if (g_xerror_value == false && userCount == 1)
          result = *(int*) data;
      XSetErrorHandler (oldErrorHandler);
      return result;
  }

  static void translateJuceToXButtonModifiers (const MouseEvent& e,
                                               XEvent& ev)
  {
      int currentModifiers = e.mods.getRawFlags ();
      if (currentModifiers & ModifierKeys::leftButtonModifier)
      {
          ev.xbutton.button = Button1;
          ev.xbutton.state |= Button1Mask;
      }
      else if (currentModifiers & ModifierKeys::rightButtonModifier)
      {
          ev.xbutton.button = Button3;
          ev.xbutton.state |= Button3Mask;
      }
      else if (currentModifiers & ModifierKeys::middleButtonModifier)
      {
          ev.xbutton.button = Button2;
          ev.xbutton.state |= Button2Mask;
      }
  }

  static void translateJuceToXMotionModifiers (const MouseEvent& e,
                                               XEvent& ev)
  {
      int currentModifiers = e.mods.getRawFlags ();
      if (currentModifiers & ModifierKeys::leftButtonModifier)
      {
          ev.xmotion.state |= Button1Mask;
      }
      else if (currentModifiers & ModifierKeys::rightButtonModifier)
      {
          ev.xmotion.state |= Button3Mask;
      }
      else if (currentModifiers & ModifierKeys::middleButtonModifier)
      {
          ev.xmotion.state |= Button2Mask;
      }
  }

  static void translateJuceToXCrossingModifiers (const MouseEvent& e,
                                                 XEvent& ev)
  {
      int currentModifiers = e.mods.getRawFlags ();
      if (currentModifiers & ModifierKeys::leftButtonModifier)
      {
          ev.xcrossing.state |= Button1Mask;
      }
      else if (currentModifiers & ModifierKeys::rightButtonModifier)
      {
          ev.xcrossing.state |= Button3Mask;
      }
      else if (currentModifiers & ModifierKeys::middleButtonModifier)
      {
          ev.xcrossing.state |= Button2Mask;
      }
  }

  static void translateJuceToXMouseWheelModifiers (const MouseEvent& e,
                                                   const float increment,
                                                   XEvent& ev)
  {
      if (increment < 0)
      {
          ev.xbutton.button = Button5;
          ev.xbutton.state |= Button5Mask;
      }
      else if (increment > 0)
      {
          ev.xbutton.button = Button4;
          ev.xbutton.state |= Button4Mask;
      }
  }
#endif



//==============================================================================
VstPluginExternalEditor::VstPluginExternalEditor (BasePlugin* plugin_,
                                                  VstPluginWindow* window_)
  : PluginEditorComponent (plugin_),
#ifdef LINUX
    eventProc (0),
    handle (0),
#endif
    window (window_),
    editorWidth (0),
    editorHeight (0),
    offsetX (0),
    offsetY (0)
{
    DBG ("VstPluginExternalEditor::VstPluginExternalEditor");
    
    setWantsKeyboardFocus (true);
    setVisible (true);

#ifdef LINUX
    plugin->openEditor (window->getWindowHandle (), display);

    handle = getChildWindow ((Window) window->getWindowHandle ());
    if (handle)
        eventProc = (EventProcPtr) obtainPropertyFromXWindow (handle,
                                                              XInternAtom (display, "_XEventProc", false));

#else
    plugin->openEditor (window->getWindowHandle (), (void*) 0);
#endif

    if (editorWidth <= 0 || editorHeight <= 0)
        plugin->getEditorSize (editorWidth, editorHeight);

    jassert (editorWidth > 0 && editorHeight > 0);
    setSize (editorWidth, editorHeight);

#ifdef LINUX
    if (handle)
    {
        offsetX = window->getBorderThickness().getLeft ();
        offsetY = window->getTitleBarHeight()
                  + window->getMenuBarHeight()
                  + window->getBorderThickness().getTop ();

        XResizeWindow (display, handle, editorWidth, editorHeight);
        XMoveWindow (display, handle, offsetX, offsetY);
    }
#endif

    repaint ();

    startTimer (1000 / 10);
}

VstPluginExternalEditor::~VstPluginExternalEditor ()
{
    DBG ("VstPluginExternalEditor::~VstPluginExternalEditor");

    if (isTimerRunning())
        stopTimer();

#ifdef LINUX
#endif

    if (plugin)
        plugin->closeEditor ();
}

//==============================================================================
void VstPluginExternalEditor::resized ()
{
#ifdef LINUX
    if (handle && isWindowVisible (handle))
    {
        offsetX = window->getBorderThickness().getLeft ();
        offsetY = window->getTitleBarHeight()
                  + window->getMenuBarHeight()
                  + window->getBorderThickness().getTop ();

        XMoveWindow (display, handle, offsetX, offsetY);
    }
#endif

    repaint ();
}

//==============================================================================
void VstPluginExternalEditor::mouseEnter (const MouseEvent& e)
{
#ifdef LINUX
    if (! handle) return;

    XEvent ev;
    zerostruct (ev);
    ev.xcrossing.display = display;
    ev.xcrossing.type = EnterNotify;
    ev.xcrossing.window = handle;
    ev.xcrossing.root = RootWindow (display, DefaultScreen (display));
    ev.xcrossing.time = CurrentTime;
    ev.xcrossing.x = e.x;
    ev.xcrossing.y = e.y;
    ev.xcrossing.x_root = e.getScreenX ();
    ev.xcrossing.y_root = e.getScreenY ();
    ev.xcrossing.mode = NotifyNormal; // NotifyGrab, NotifyUngrab
    ev.xcrossing.detail = NotifyAncestor; // NotifyVirtual, NotifyInferior, NotifyNonlinear,NotifyNonlinearVirtual

    translateJuceToXCrossingModifiers (e, ev);

    sendEventToChild (& ev);
#endif
}

//==============================================================================
void VstPluginExternalEditor::mouseExit (const MouseEvent& e)
{
#ifdef LINUX
    if (! handle) return;

    XEvent ev;
    zerostruct (ev);
    ev.xcrossing.display = display;
    ev.xcrossing.type = LeaveNotify;
    ev.xcrossing.window = handle;
    ev.xcrossing.root = RootWindow (display, DefaultScreen (display));
    ev.xcrossing.time = CurrentTime;
    ev.xcrossing.x = e.x;
    ev.xcrossing.y = e.y;
    ev.xcrossing.x_root = e.getScreenX ();
    ev.xcrossing.y_root = e.getScreenY ();
    ev.xcrossing.mode = NotifyNormal; // NotifyGrab, NotifyUngrab
    ev.xcrossing.detail = NotifyAncestor; // NotifyVirtual, NotifyInferior, NotifyNonlinear,NotifyNonlinearVirtual
    ev.xcrossing.focus = hasKeyboardFocus (true); // TODO - yes ?

    translateJuceToXCrossingModifiers (e, ev);

    sendEventToChild (& ev);
#endif
}

//==============================================================================
void VstPluginExternalEditor::mouseMove (const MouseEvent& e)
{
#ifdef LINUX
    if (! handle) return;

    XEvent ev;
    zerostruct (ev);
    ev.xmotion.display = display;
    ev.xmotion.type = MotionNotify;
    ev.xmotion.window = handle;
    ev.xmotion.root = RootWindow (display, DefaultScreen (display));
    ev.xmotion.time = CurrentTime;
    ev.xmotion.is_hint = NotifyNormal;
    ev.xmotion.x = e.x;
    ev.xmotion.y = e.y;
    ev.xmotion.x_root = e.getScreenX ();
    ev.xmotion.y_root = e.getScreenY ();

    sendEventToChild (& ev);
#endif
}

//==============================================================================
void VstPluginExternalEditor::mouseDown (const MouseEvent& e)
{
#ifdef LINUX
    if (! handle) return;

    XEvent ev;
    zerostruct (ev);
    ev.xbutton.display = display;
    ev.xbutton.type = ButtonPress;
    ev.xbutton.window = handle;
    ev.xbutton.root = RootWindow (display, DefaultScreen (display));
    ev.xbutton.time = CurrentTime;
    ev.xbutton.x = e.x;
    ev.xbutton.y = e.y;
    ev.xbutton.x_root = e.getScreenX ();
    ev.xbutton.y_root = e.getScreenY ();

    translateJuceToXButtonModifiers (e, ev);

    sendEventToChild (& ev);
#endif
}

//==============================================================================
void VstPluginExternalEditor::mouseDrag (const MouseEvent& e)
{
#ifdef LINUX
    if (! handle) return;

    XEvent ev;
    zerostruct (ev);
    ev.xmotion.display = display;
    ev.xmotion.type = MotionNotify;
    ev.xmotion.window = handle;
    ev.xmotion.root = RootWindow (display, DefaultScreen (display));
    ev.xmotion.time = CurrentTime;
    ev.xmotion.x = e.x ;
    ev.xmotion.y = e.y;
    ev.xmotion.x_root = e.getScreenX ();
    ev.xmotion.y_root = e.getScreenY ();
    ev.xmotion.is_hint = NotifyNormal;

    translateJuceToXMotionModifiers (e, ev);

    sendEventToChild (& ev);
#endif
}

//==============================================================================
void VstPluginExternalEditor::mouseUp (const MouseEvent& e)
{
#ifdef LINUX
    if (! handle) return;

    XEvent ev;
    zerostruct (ev);
    ev.xbutton.display = display;
    ev.xbutton.type = ButtonRelease;
    ev.xbutton.window = handle;
    ev.xbutton.root = RootWindow (display, DefaultScreen (display));
    ev.xbutton.time = CurrentTime;
    ev.xbutton.x = e.x;
    ev.xbutton.y = e.y;
    ev.xbutton.x_root = e.getScreenX ();
    ev.xbutton.y_root = e.getScreenY ();

    translateJuceToXButtonModifiers (e, ev);

    sendEventToChild (& ev);
#endif
}

//==============================================================================
void VstPluginExternalEditor::mouseWheelMove (const MouseEvent& e,
                                              float incrementX,
                                              float incrementY)
{
#ifdef LINUX
    if (! handle) return;

    XEvent ev;
    zerostruct (ev);
    ev.xbutton.display = display;
    ev.xbutton.type = ButtonPress;
    ev.xbutton.window = handle;
    ev.xbutton.root = RootWindow (display, DefaultScreen (display));
    ev.xbutton.time = CurrentTime;
    ev.xbutton.x = e.x;
    ev.xbutton.y = e.y;
    ev.xbutton.x_root = e.getScreenX ();
    ev.xbutton.y_root = e.getScreenY ();

    translateJuceToXMouseWheelModifiers (e, incrementY, ev);

    sendEventToChild (& ev);

    // TODO - put a usleep here ?

    ev.xbutton.type = ButtonRelease;
    sendEventToChild (& ev);
#endif
}

bool VstPluginExternalEditor::keyPressed (const KeyPress &key)
{
#ifdef LINUX
    if (! handle) return false;

    XEvent ev;
    zerostruct (ev);

    ev.xkey.type = 2; // KeyPress
    ev.xkey.display = display;
    ev.xkey.window = handle;
    ev.xkey.root = RootWindow (display, DefaultScreen (display));
    ev.xkey.subwindow = None;
    ev.xkey.time = CurrentTime;
//    ev.xkey.x = e.x;
//    ev.xkey.y = e.y;
//    ev.xkey.x_root = e.getScreenX ();
//    ev.xkey.y_root = e.getScreenY ();
//    ev.xkey.state = e.y;
    ev.xkey.keycode = key.getKeyCode();
    ev.xkey.same_screen = True;

    sendEventToChild (& ev);
    
    return true;
#endif
}

//==============================================================================
void VstPluginExternalEditor::filesDropped (const StringArray& filenames,
                                            int mouseX,
                                            int mouseY)
{
    DBG ("VstPluginExternalEditor::filesDropped");
}

//==============================================================================
void VstPluginExternalEditor::visibilityChanged ()
{
    if (isShowing())
    {
#ifdef LINUX
        if (handle) XMapRaised (display, handle);
#endif
    }
    else
    {
#ifdef LINUX
        if (handle) XUnmapWindow (display, handle);
#endif
    }
}

//==============================================================================
void VstPluginExternalEditor::timerCallback ()
{
    if (plugin)
        plugin->idleEditor ();
}

void VstPluginExternalEditor::paint (Graphics& g)
{
    g.fillAll (Colours::white);

#ifdef LINUX
    if (handle)
    {
        Rectangle clip = g.getClipBounds ();

        XEvent ev;
        zerostruct (ev);
        ev.xexpose.type = Expose;
        ev.xexpose.display = display;
        ev.xexpose.window = handle;

        ev.xexpose.x = clip.getX();
        ev.xexpose.y = clip.getY();
        ev.xexpose.width = clip.getWidth();
        ev.xexpose.height = clip.getHeight();

        sendEventToChild (& ev);
#endif
    }
}

void VstPluginExternalEditor::internalRepaint (int x, int y, int w, int h)
{
/*
#ifdef LINUX
    if (handle)
    {
        XEvent ev;
        zerostruct (ev);
        ev.xexpose.type = Expose;
        ev.xexpose.display = display;
        ev.xexpose.window = handle;

        ev.xexpose.x = x;
        ev.xexpose.y = y;
        ev.xexpose.width = w;
        ev.xexpose.height = h;

        sendEventToChild (& ev);
    }
#endif
*/
    Component::internalRepaint (x, y, w, h);
}

//==============================================================================
int VstPluginExternalEditor::getPreferredWidth ()
{
    int width, height;

    if (plugin)
        plugin->getEditorSize (width, height);

    return width; // editorWidth
}

int VstPluginExternalEditor::getPreferredHeight ()
{
    int width, height;

    if (plugin)
        plugin->getEditorSize (width, height);

    return height; // editorHeight
}

bool VstPluginExternalEditor::isResizable ()
{
    return false;
}

void VstPluginExternalEditor::updateWindowPosition (const int x, const int y)
{
#ifdef LINUX
    if (! handle) return;

    XEvent ev;
    zerostruct (ev);
    ev.xgravity.type = GravityNotify;
    ev.xgravity.display = display;
    ev.xgravity.window = handle;
    ev.xgravity.event = (Window) window->getWindowHandle ();
    ev.xgravity.x = x;
    ev.xgravity.y = y;

    // DBG ("Window Position Updated to: " + String (x) + " " + String (y));

    sendEventToChild (& ev);
#endif
}

//==============================================================================
#ifdef LINUX
void VstPluginExternalEditor::sendEventToChild (XEvent* event)
{
    if (eventProc)
    {
        // if the plugin publish a event procedure, so it doesn't have
        // a message thread running, pass the event directly
        eventProc (event);
    }
    else
    {
        // if the plugin have a message thread running, then send events to
        // that window: it will be caught by the message thread !
        if (handle)
        {
            XSendEvent (display, handle, False, 0L, event);
            XFlush (display);
        }
    }
}

Window VstPluginExternalEditor::getChildWindow (Window windowToCheck)
{
    Window rootWindow, parentWindow;
    Window *childWindows;
    unsigned int nchildren;

    XQueryTree (display,
                windowToCheck,
                &rootWindow,
                &parentWindow,
                &childWindows,
                &nchildren);

    if (nchildren > 0 && childWindows)
    {
        Window returnWindow = childWindows [0];

        XFree (childWindows);

        return returnWindow;
    }
    
    if (childWindows)
        XFree (childWindows);

    return 0;
}

bool VstPluginExternalEditor::isWindowVisible (Window windowToCheck)
{
    XWindowAttributes wsa;

    XGetWindowAttributes (display, windowToCheck, &wsa);

    return wsa.map_state == IsViewable;
}

#endif
