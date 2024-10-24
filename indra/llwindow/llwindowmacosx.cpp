/**
 * @file llwindowmacosx.cpp
 * @brief Platform-dependent implementation of llwindow
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#include "linden_common.h"

#include "llwindowmacosx.h"

#include "llgamecontrol.h"
#include "llkeyboardmacosx.h"
#include "llwindowcallbacks.h"
#include "llpreeditor.h"

#include "llerror.h"
#include "llgl.h"
#include "llstring.h"
#include "lldir.h"
#include "indra_constants.h"

#include <OpenGL/OpenGL.h>
#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>
#include <CoreGraphics/CGDisplayConfiguration.h>

#include <IOKit/IOCFPlugIn.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/IOMessage.h>
#include <IOKit/hid/IOHIDUsageTables.h>
#include <IOKit/hid/IOHIDLib.h>
#include <IOKit/usb/IOUSBLib.h>

extern bool gDebugWindowProc;
bool gHiDPISupport = true;

const S32   BITS_PER_PIXEL = 32;
const S32   MAX_NUM_RESOLUTIONS = 32;

const S32   DEFAULT_REFRESH_RATE = 60;

namespace
{
    NSKeyEventRef mRawKeyEvent = NULL;
}
//
// LLWindowMacOSX
//

bool LLWindowMacOSX::sUseMultGL = false;

// Cross-platform bits:

bool check_for_card(const char* RENDERER, const char* bad_card)
{
    if (!strnicmp(RENDERER, bad_card, strlen(bad_card)))
    {
        std::string buffer = llformat(
            "Your video card appears to be a %s, which Second Life does not support.\n"
            "\n"
            "Second Life requires a video card with 32 Mb of memory or more, as well as\n"
            "multitexture support.  We explicitly support nVidia GeForce 2 or better, \n"
            "and ATI Radeon 8500 or better.\n"
            "\n"
            "If you own a supported card and continue to receive this message, try \n"
            "updating to the latest video card drivers. Otherwise look in the\n"
            "secondlife.com support section or e-mail technical support\n"
            "\n"
            "You can try to run Second Life, but it will probably crash or run\n"
            "very slowly.  Try anyway?",
            bad_card);
        S32 button = OSMessageBox(buffer.c_str(), "Unsupported video card", OSMB_YESNO);
        if (OSBTN_YES == button)
        {
            return false;
        }
        else
        {
            return true;
        }
    }

    return false;
}

// Switch to determine whether we capture all displays, or just the main one.
// We may want to base this on the setting of _DEBUG...

#define CAPTURE_ALL_DISPLAYS 0
//static double getDictDouble (CFDictionaryRef refDict, CFStringRef key);
static long getDictLong (CFDictionaryRef refDict, CFStringRef key);

// MBW -- HACK ALERT
// On the Mac, to put up an OS dialog in full screen mode, we must first switch OUT of full screen mode.
// The proper way to do this is to bracket the dialog with calls to beforeDialog() and afterDialog(), but these
// require a pointer to the LLWindowMacOSX object.  Stash it here and maintain in the constructor and destructor.
// This assumes that there will be only one object of this class at any time.  Hopefully this is true.
static LLWindowMacOSX *gWindowImplementation = NULL;

LLWindowMacOSX::LLWindowMacOSX(LLWindowCallbacks* callbacks,
                               const std::string& title, const std::string& name, S32 x, S32 y, S32 width,
                               S32 height, U32 flags,
                               bool fullscreen, bool clearBg,
                               bool enable_vsync, bool use_gl,
                               bool ignore_pixel_depth,
                               U32 fsaa_samples)
    : LLWindow(NULL, fullscreen, flags)
{
    // *HACK: During window construction we get lots of OS events for window
    // reshape, activate, etc. that the viewer isn't ready to handle.
    // Route them to a dummy callback structure until the end of constructor.
    LLWindowCallbacks null_callbacks;
    mCallbacks = &null_callbacks;

    // Voodoo for calling cocoa from carbon (see llwindowmacosx-objc.mm).
    setupCocoa();

    // Initialize the keyboard
    gKeyboard = new LLKeyboardMacOSX();
    gKeyboard->setCallbacks(callbacks);

    // Ignore use_gl for now, only used for drones on PC
    mWindow = NULL;
    mContext = NULL;
    mPixelFormat = NULL;
    mDisplay = CGMainDisplayID();
    mSimulatedRightClick = false;
    mLastModifiers = 0;
    mHandsOffEvents = false;
    mCursorDecoupled = false;
    mCursorLastEventDeltaX = 0;
    mCursorLastEventDeltaY = 0;
    mCursorIgnoreNextDelta = false;
    mNeedsResize = false;
    mOverrideAspectRatio = 0.f;
    mMaximized = false;
    mMinimized = false;
    mLanguageTextInputAllowed = false;
    mPreeditor = NULL;
    mFSAASamples = fsaa_samples;
    mForceRebuild = false;

    // Get the original aspect ratio of the main device.
    mOriginalAspectRatio = (double)CGDisplayPixelsWide(mDisplay) / (double)CGDisplayPixelsHigh(mDisplay);

    // Stash the window title
    mWindowTitle = title;
    //mWindowTitle[0] = title.length();

    mDragOverrideCursor = -1;

    // Set up global event handlers (the fullscreen case needs this)
    //InstallStandardEventHandler(GetApplicationEventTarget());

    // Stash an object pointer for OSMessageBox()
    gWindowImplementation = this;
    // Create the GL context and set it up for windowed or fullscreen, as appropriate.
    if(createContext(x, y, width, height, 32, fullscreen, enable_vsync))
    {
        if(mWindow != NULL)
        {
            makeWindowOrderFront(mWindow);
        }

        if (!gGLManager.initGL())
        {
            setupFailure(
                "Second Life is unable to run because your video card drivers\n"
                "are out of date or unsupported. Please make sure you have\n"
                "the latest video card drivers installed.\n"
                "If you continue to receive this message, contact customer service.",
                "Error",
                OSMB_OK);
            return;
        }

        //start with arrow cursor
        initCursors();
        setCursor( UI_CURSOR_ARROW );

        allowLanguageTextInput(NULL, false);
    }

    mCallbacks = callbacks;
    stop_glerror();


}

// These functions are used as wrappers for our internal event handling callbacks.
// It's a good idea to wrap these to avoid reworking more code than we need to within LLWindow.

bool callKeyUp(NSKeyEventRef event, unsigned short key, unsigned int mask)
{
    mRawKeyEvent = event;
    bool retVal = gKeyboard->handleKeyUp(key, mask);
    mRawKeyEvent = NULL;
    return retVal;
}

bool callKeyDown(NSKeyEventRef event, unsigned short key, unsigned int mask, wchar_t character)
{
    //if (mask!=MASK_NONE)
    {
    if((key == gKeyboard->inverseTranslateKey('Z')) && (character == 'y'))
    {
        key = gKeyboard->inverseTranslateKey('Y');
    }
    else if ((key == gKeyboard->inverseTranslateKey('Y')) && (character == 'z'))
    {
        key = gKeyboard->inverseTranslateKey('Z');
    }
    }

    mRawKeyEvent = event;
    bool retVal = gKeyboard->handleKeyDown(key, mask);
    mRawKeyEvent = NULL;
    return retVal;
}

void callResetKeys()
{
    gKeyboard->resetKeys();
}

bool callUnicodeCallback(wchar_t character, unsigned int mask)
{
    NativeKeyEventData eventData;

    memset(&eventData, 0, sizeof(NativeKeyEventData));

    eventData.mKeyEvent = NativeKeyEventData::KEYCHAR;
    eventData.mEventType = 0;
    eventData.mEventModifiers = mask;
    eventData.mEventKeyCode = 0;
    eventData.mEventChars = character;
    eventData.mEventUnmodChars = character;
    eventData.mEventRepeat = false;

    mRawKeyEvent = &eventData;

    bool result = gWindowImplementation->getCallbacks()->handleUnicodeChar(character, mask);
    mRawKeyEvent = NULL;
    return result;
}

void callFocus()
{
    if (gWindowImplementation)
    {
        gWindowImplementation->getCallbacks()->handleFocus(gWindowImplementation);
    }
}

void callFocusLost()
{
    if (gWindowImplementation)
    {
        gWindowImplementation->getCallbacks()->handleFocusLost(gWindowImplementation);
    }
}

void callRightMouseDown(float *pos, MASK mask)
{
    if (gWindowImplementation->allowsLanguageInput())
    {
        gWindowImplementation->interruptLanguageTextInput();
    }

    LLCoordGL       outCoords;
    outCoords.mX = ll_round(pos[0]);
    outCoords.mY = ll_round(pos[1]);
    gWindowImplementation->getCallbacks()->handleRightMouseDown(gWindowImplementation, outCoords, gKeyboard->currentMask(true));
}

void callRightMouseUp(float *pos, MASK mask)
{
    if (gWindowImplementation->allowsLanguageInput())
    {
        gWindowImplementation->interruptLanguageTextInput();
    }

    LLCoordGL       outCoords;
    outCoords.mX = ll_round(pos[0]);
    outCoords.mY = ll_round(pos[1]);
    gWindowImplementation->getCallbacks()->handleRightMouseUp(gWindowImplementation, outCoords, gKeyboard->currentMask(true));
}

void callLeftMouseDown(float *pos, MASK mask)
{
    if (gWindowImplementation->allowsLanguageInput())
    {
        gWindowImplementation->interruptLanguageTextInput();
    }

    LLCoordGL       outCoords;
    outCoords.mX = ll_round(pos[0]);
    outCoords.mY = ll_round(pos[1]);
    gWindowImplementation->getCallbacks()->handleMouseDown(gWindowImplementation, outCoords, gKeyboard->currentMask(true));
}

void callLeftMouseUp(float *pos, MASK mask)
{
    if (gWindowImplementation->allowsLanguageInput())
    {
        gWindowImplementation->interruptLanguageTextInput();
    }

    LLCoordGL       outCoords;
    outCoords.mX = ll_round(pos[0]);
    outCoords.mY = ll_round(pos[1]);
    gWindowImplementation->getCallbacks()->handleMouseUp(gWindowImplementation, outCoords, gKeyboard->currentMask(true));

}

void callDoubleClick(float *pos, MASK mask)
{
    if (gWindowImplementation->allowsLanguageInput())
    {
        gWindowImplementation->interruptLanguageTextInput();
    }

    LLCoordGL   outCoords;
    outCoords.mX = ll_round(pos[0]);
    outCoords.mY = ll_round(pos[1]);
    gWindowImplementation->getCallbacks()->handleDoubleClick(gWindowImplementation, outCoords, gKeyboard->currentMask(true));
}

void callResize(unsigned int width, unsigned int height)
{
    if (gWindowImplementation != NULL)
    {
        gWindowImplementation->getCallbacks()->handleResize(gWindowImplementation, width, height);
    }
}

void callMouseMoved(float *pos, MASK mask)
{
    LLCoordGL       outCoords;
    outCoords.mX = ll_round(pos[0]);
    outCoords.mY = ll_round(pos[1]);
    float deltas[2];
    gWindowImplementation->getMouseDeltas(deltas);
    outCoords.mX += deltas[0];
    outCoords.mY += deltas[1];
    gWindowImplementation->getCallbacks()->handleMouseMove(gWindowImplementation, outCoords, gKeyboard->currentMask(true));
    //gWindowImplementation->getCallbacks()->handleScrollWheel(gWindowImplementation, 0);
}

void callMouseDragged(float *pos, MASK mask)
{
    LLCoordGL       outCoords;
    outCoords.mX = ll_round(pos[0]);
    outCoords.mY = ll_round(pos[1]);
    float deltas[2];
    gWindowImplementation->getMouseDeltas(deltas);
    outCoords.mX += deltas[0];
    outCoords.mY += deltas[1];
    gWindowImplementation->getCallbacks()->handleMouseDragged(gWindowImplementation, outCoords, gKeyboard->currentMask(true));
}

void callScrollMoved(float deltaX, float deltaY)
{
    if ( gWindowImplementation && gWindowImplementation->getCallbacks() )
    {
        gWindowImplementation->getCallbacks()->handleScrollHWheel(gWindowImplementation, deltaX);
        gWindowImplementation->getCallbacks()->handleScrollWheel(gWindowImplementation, deltaY);
    }
}

void callMouseExit()
{
    gWindowImplementation->getCallbacks()->handleMouseLeave(gWindowImplementation);
}

void callWindowFocus()
{
   if ( gWindowImplementation && gWindowImplementation->getCallbacks() )
    {
        gWindowImplementation->getCallbacks()->handleFocus (gWindowImplementation);
    }
    else
    {
        LL_WARNS("COCOA") << "Window Implementation or callbacks not yet initialized." << LL_ENDL;
    }


}

void callWindowUnfocus()
{
    if ( gWindowImplementation && gWindowImplementation->getCallbacks() )
    {
        gWindowImplementation->getCallbacks()->handleFocusLost(gWindowImplementation);
    }
}

void callWindowHide()
{
    if ( gWindowImplementation && gWindowImplementation->getCallbacks() )
    {
        gWindowImplementation->getCallbacks()->handleActivate(gWindowImplementation, false);
    }
}

void callWindowUnhide()
{
    if ( gWindowImplementation && gWindowImplementation->getCallbacks() )
    {
        gWindowImplementation->getCallbacks()->handleActivate(gWindowImplementation, true);
    }
}

void callWindowDidChangeScreen()
{
    if ( gWindowImplementation && gWindowImplementation->getCallbacks() )
    {
        gWindowImplementation->getCallbacks()->handleWindowDidChangeScreen(gWindowImplementation);
    }
}

void callDeltaUpdate(float *delta, MASK mask)
{
    gWindowImplementation->updateMouseDeltas(delta);
}

void callOtherMouseDown(float *pos, MASK mask, int button)
{
    LLCoordGL       outCoords;
    outCoords.mX = ll_round(pos[0]);
    outCoords.mY = ll_round(pos[1]);
    float deltas[2];
    gWindowImplementation->getMouseDeltas(deltas);
    outCoords.mX += deltas[0];
    outCoords.mY += deltas[1];

    if (button == 2)
    {
        gWindowImplementation->getCallbacks()->handleMiddleMouseDown(gWindowImplementation, outCoords, mask);
    }
    else
    {
        gWindowImplementation->getCallbacks()->handleOtherMouseDown(gWindowImplementation, outCoords, mask, button + 1);
    }
}

void callOtherMouseUp(float *pos, MASK mask, int button)
{
    LLCoordGL outCoords;
    outCoords.mX = ll_round(pos[0]);
    outCoords.mY = ll_round(pos[1]);
    float deltas[2];
    gWindowImplementation->getMouseDeltas(deltas);
    outCoords.mX += deltas[0];
    outCoords.mY += deltas[1];
    if (button == 2)
    {
        gWindowImplementation->getCallbacks()->handleMiddleMouseUp(gWindowImplementation, outCoords, mask);
    }
    else
    {
        gWindowImplementation->getCallbacks()->handleOtherMouseUp(gWindowImplementation, outCoords, mask, button + 1);
    }
}

void callModifier(MASK mask)
{
    gKeyboard->handleModifier(mask);
}

void callHandleDragEntered(std::string url)
{
    gWindowImplementation->handleDragNDrop(url, LLWindowCallbacks::DNDA_START_TRACKING);
}

void callHandleDragExited(std::string url)
{
    gWindowImplementation->handleDragNDrop(url, LLWindowCallbacks::DNDA_STOP_TRACKING);
}

void callHandleDragUpdated(std::string url)
{
    gWindowImplementation->handleDragNDrop(url, LLWindowCallbacks::DNDA_TRACK);
}

void callHandleDragDropped(std::string url)
{
    gWindowImplementation->handleDragNDrop(url, LLWindowCallbacks::DNDA_DROPPED);
}

void callQuitHandler()
{
    if (gWindowImplementation)
    {
        if(gWindowImplementation->getCallbacks()->handleCloseRequest(gWindowImplementation))
        {
            gWindowImplementation->getCallbacks()->handleQuit(gWindowImplementation);
        }
    }
}

void getPreeditSelectionRange(int *position, int *length)
{
    if (gWindowImplementation->getPreeditor())
    {
        gWindowImplementation->getPreeditor()->getSelectionRange(position, length);
    }
}

void getPreeditMarkedRange(int *position, int *length)
{
    if (gWindowImplementation->getPreeditor())
    {
        gWindowImplementation->getPreeditor()->getPreeditRange(position, length);
    }
}

void setPreeditMarkedRange(int position, int length)
{
    if (gWindowImplementation->getPreeditor())
    {
        gWindowImplementation->getPreeditor()->markAsPreedit(position, length);
    }
}

bool handleUnicodeCharacter(wchar_t c)
{
    bool success = false;
    if (gWindowImplementation->getPreeditor())
    {
        success = gWindowImplementation->getPreeditor()->handleUnicodeCharHere(c);
    }

    return success;
}

void resetPreedit()
{
    if (gWindowImplementation->getPreeditor())
    {
        gWindowImplementation->getPreeditor()->resetPreedit();
    }
}

// For reasons of convenience, handle IME updates here.
// This largely mirrors the old implementation, only sans the carbon parameters.
void setMarkedText(unsigned short *unitext, unsigned int *selectedRange, unsigned int *replacementRange, long text_len, attributedStringInfo segments)
{
    if (gWindowImplementation->getPreeditor())
    {
        LLPreeditor *preeditor = gWindowImplementation->getPreeditor();
        preeditor->resetPreedit();
        // This should be a viable replacement for the kEventParamTextInputSendReplaceRange parameter.
        if (replacementRange[0] < replacementRange[1])
        {
            const LLWString& text = preeditor->getPreeditString();
            const S32 location = wstring_wstring_length_from_utf16_length(text, 0, replacementRange[0]);
            const S32 length = wstring_wstring_length_from_utf16_length(text, location, replacementRange[1]);
            preeditor->markAsPreedit(location, length);
        }

        LLWString fix_str = utf16str_to_wstring(llutf16string(unitext, text_len));

        S32 caret_position = fix_str.length();

        preeditor->updatePreedit(fix_str, segments.seg_lengths, segments.seg_standouts, caret_position);
    }
}

void getPreeditLocation(float *location, unsigned int length)
{
    if (gWindowImplementation->getPreeditor())
    {
        LLPreeditor *preeditor = gWindowImplementation->getPreeditor();
        LLCoordGL coord;
        LLCoordScreen screen;
        LLRect rect;

        preeditor->getPreeditLocation(length, &coord, &rect, NULL);

        float c[4] = {float(coord.mX), float(coord.mY), 0, 0};

        convertRectToScreen(gWindowImplementation->getWindow(), c);

        location[0] = c[0];
        location[1] = c[1];
    }
}

void LLWindowMacOSX::updateMouseDeltas(float* deltas)
{
    if (mCursorDecoupled)
    {
        mCursorLastEventDeltaX = ll_round(deltas[0]);
        mCursorLastEventDeltaY = ll_round(-deltas[1]);

        if (mCursorIgnoreNextDelta)
        {
            mCursorLastEventDeltaX = 0;
            mCursorLastEventDeltaY = 0;
            mCursorIgnoreNextDelta = false;
        }
    } else {
        mCursorLastEventDeltaX = 0;
        mCursorLastEventDeltaY = 0;
    }
}

void LLWindowMacOSX::getMouseDeltas(float* delta) const
{
    delta[0] = mCursorLastEventDeltaX;
    delta[1] = mCursorLastEventDeltaY;
}

bool LLWindowMacOSX::createContext(int x, int y, int width, int height, int bits, bool fullscreen, bool enable_vsync)
{
    mFullscreen = fullscreen;

    if (mWindow == NULL)
    {
        mWindow = getMainAppWindow();
    }

    if(mContext == NULL)
    {
        // Our OpenGL view is already defined within SecondLife.xib.
        // Get the view instead.
        mGLView = createOpenGLView(mWindow, mFSAASamples, enable_vsync);
        mContext = getCGLContextObj(mGLView);
        gGLManager.mVRAM = getVramSize(mGLView);

        if(!mPixelFormat)
        {
            CGLPixelFormatAttribute attribs[] =
            {
                kCGLPFANoRecovery,
                kCGLPFADoubleBuffer,
                kCGLPFAClosestPolicy,
                kCGLPFAAccelerated,
                kCGLPFAMultisample,
                kCGLPFASampleBuffers, static_cast<CGLPixelFormatAttribute>((mFSAASamples > 0 ? 1 : 0)),
                kCGLPFASamples, static_cast<CGLPixelFormatAttribute>(mFSAASamples),
                kCGLPFAStencilSize, static_cast<CGLPixelFormatAttribute>(8),
                kCGLPFADepthSize, static_cast<CGLPixelFormatAttribute>(24),
                kCGLPFAAlphaSize, static_cast<CGLPixelFormatAttribute>(8),
                kCGLPFAColorSize, static_cast<CGLPixelFormatAttribute>(24),
                kCGLPFAOpenGLProfile, static_cast<CGLPixelFormatAttribute>(kCGLOGLPVersion_GL4_Core),
                static_cast<CGLPixelFormatAttribute>(0)
            };

            GLint numPixelFormats;
            CGLChoosePixelFormat (attribs, &mPixelFormat, &numPixelFormats);

            if(mPixelFormat == NULL) {
                CGLChoosePixelFormat (attribs, &mPixelFormat, &numPixelFormats);
            }
        }

    }

    // This sets up our view to recieve text from our non-inline text input window.
    setupInputWindow(mWindow, mGLView);

    // Hook up the context to a drawable

    if(mContext != NULL)
    {


        U32 err = CGLSetCurrentContext(mContext);
        if (err != kCGLNoError)
        {
            setupFailure("Can't activate GL rendering context", "Error", OSMB_OK);
            return false;
        }
    }

    mRefreshRate = CGDisplayModeGetRefreshRate(CGDisplayCopyDisplayMode(mDisplay));
    if(mRefreshRate == 0)
    {
        //consider adding more appropriate fallback later
        mRefreshRate = DEFAULT_REFRESH_RATE;
    }

    // Disable vertical sync for swap
    toggleVSync(enable_vsync);

    //enable multi-threaded OpenGL
    if (sUseMultGL)
    {
        CGLError cgl_err;
        CGLContextObj ctx = CGLGetCurrentContext();

        cgl_err =  CGLEnable( ctx, kCGLCEMPEngine);

        if (cgl_err != kCGLNoError )
        {
            LL_INFOS("GLInit") << "Multi-threaded OpenGL not available." << LL_ENDL;
        }
        else
        {
            LL_INFOS("GLInit") << "Multi-threaded OpenGL enabled." << LL_ENDL;
        }
    }
    makeFirstResponder(mWindow, mGLView);

    return true;
}


// We only support OS X 10.7's fullscreen app mode which is literally a full screen window that fills a virtual desktop.
// This makes this method obsolete.
bool LLWindowMacOSX::switchContext(bool fullscreen, const LLCoordScreen &size, bool enable_vsync, const LLCoordScreen * const posp)
{
    return false;
}

void LLWindowMacOSX::destroyContext()
{
    if (!mContext)
    {
        // We don't have a context
        return;
    }
    // Unhook the GL context from any drawable it may have
    if(mContext != NULL)
    {
        LL_DEBUGS("Window") << "destroyContext: unhooking drawable " << LL_ENDL;
        CGLSetCurrentContext(NULL);
    }

    // Clean up remaining GL state before blowing away window
    gGLManager.shutdownGL();

    // Clean up the pixel format
    if(mPixelFormat != NULL)
    {
        CGLDestroyPixelFormat(mPixelFormat);
        mPixelFormat = NULL;
    }

    // Clean up the GL context
    if(mContext != NULL)
    {
        CGLDestroyContext(mContext);
    }

    // Destroy our LLOpenGLView
    if(mGLView != NULL)
    {
        removeGLView(mGLView);
        mGLView = NULL;
    }

    // Close the window
    if(mWindow != NULL)
    {
        NSWindowRef dead_window = mWindow;
        mWindow = NULL;
        closeWindow(dead_window);
    }

}

LLWindowMacOSX::~LLWindowMacOSX()
{
    destroyContext();

    if(mSupportedResolutions != NULL)
    {
        delete []mSupportedResolutions;
    }

    gWindowImplementation = NULL;

}


void LLWindowMacOSX::show()
{
}

void LLWindowMacOSX::hide()
{
    setMouseClipping(false);
}

//virtual
void LLWindowMacOSX::minimize()
{
    setMouseClipping(false);
    showCursor();
}

//virtual
void LLWindowMacOSX::restore()
{
    show();
}


// close() destroys all OS-specific code associated with a window.
// Usually called from LLWindowManager::destroyWindow()
void LLWindowMacOSX::close()
{
    // Is window is already closed?
    //  if (!mWindow)
    //  {
    //      return;
    //  }

    // Make sure cursor is visible and we haven't mangled the clipping state.
    setMouseClipping(false);
    showCursor();

    destroyContext();
}

bool LLWindowMacOSX::isValid()
{
    if(mFullscreen)
    {
        return(true);
    }

    return (mWindow != NULL);
}

bool LLWindowMacOSX::getVisible() const
{
    bool result = false;

    if(mFullscreen)
    {
        result = true;
    }if (mWindow)
    {
            result = true;
    }

    return(result);
}

bool LLWindowMacOSX::getMinimized() const
{
    return mMinimized;
}

bool LLWindowMacOSX::getMaximized() const
{
    return mMaximized;
}

bool LLWindowMacOSX::maximize()
{
    if (mWindow && !mMaximized)
    {
    }

    return mMaximized;
}

void LLWindowMacOSX::gatherInput(bool app_has_focus)
{
    updateCursor();
    LLGameControl::processEvents(app_has_focus);
}

bool LLWindowMacOSX::getPosition(LLCoordScreen *position) const
{
    S32 err = -1;

    if(mFullscreen)
    {
        position->mX = 0;
        position->mY = 0;
        err = noErr;
    }
    else if(mWindow)
    {
        const CGPoint & pos = getContentViewBoundsPosition(mWindow);

        position->mX = pos.x;
        position->mY = pos.y;

        err = noErr;
    }
    else
    {
        LL_ERRS() << "LLWindowMacOSX::getPosition(): no window and not fullscreen!" << LL_ENDL;
    }

    return (err == noErr);
}

bool LLWindowMacOSX::getSize(LLCoordScreen *size) const
{
    S32 err = -1;

    if(mFullscreen)
    {
        size->mX = mFullscreenWidth;
        size->mY = mFullscreenHeight;
        err = noErr;
    }
    else if(mWindow)
    {
        const CGSize & sz = gHiDPISupport ? getDeviceContentViewSize(mWindow, mGLView) : getContentViewBoundsSize(mWindow);

        size->mX = sz.width;
        size->mY = sz.height;
        err = noErr;
    }
    else
    {
        LL_ERRS() << "LLWindowMacOSX::getSize(): no window and not fullscreen!" << LL_ENDL;
    }

    return (err == noErr);
}

bool LLWindowMacOSX::getSize(LLCoordWindow *size) const
{
    S32 err = -1;

    if(mFullscreen)
    {
        size->mX = mFullscreenWidth;
        size->mY = mFullscreenHeight;
        err = noErr;
    }
    else if(mWindow)
    {
        const CGSize & sz = gHiDPISupport ? getDeviceContentViewSize(mWindow, mGLView) : getContentViewBoundsSize(mWindow);

        size->mX = sz.width;
        size->mY = sz.height;
        err = noErr;


    }
    else
    {
        LL_ERRS() << "LLWindowMacOSX::getSize(): no window and not fullscreen!" << LL_ENDL;
    }

    return (err == noErr);
}

bool LLWindowMacOSX::setPosition(const LLCoordScreen position)
{
    if(mWindow)
    {
        float pos[2] = {float(position.mX), float(position.mY)};
        setWindowPos(mWindow, pos);
    }

    return true;
}

bool LLWindowMacOSX::setSizeImpl(const LLCoordScreen size)
{
    if(mWindow)
    {
        LLCoordWindow to;
        convertCoords(size, &to);
        setWindowSize(mWindow, to.mX, to.mY);
        return true;
    }

    return false;
}

bool LLWindowMacOSX::setSizeImpl(const LLCoordWindow size)
{
    if (mWindow)
    {
        const int titlePadding = 22;
        setWindowSize(mWindow, size.mX, size.mY + titlePadding);
        return true;
    }

    return false;
}

void LLWindowMacOSX::swapBuffers()
{
    CGLFlushDrawable(mContext);
}

void LLWindowMacOSX::restoreGLContext()
{
    CGLSetCurrentContext(mContext);
}

F32 LLWindowMacOSX::getGamma() const
{
    F32 result = 2.2;   // Default to something sane

    CGGammaValue redMin;
    CGGammaValue redMax;
    CGGammaValue redGamma;
    CGGammaValue greenMin;
    CGGammaValue greenMax;
    CGGammaValue greenGamma;
    CGGammaValue blueMin;
    CGGammaValue blueMax;
    CGGammaValue blueGamma;

    if(CGGetDisplayTransferByFormula(
        mDisplay,
        &redMin,
        &redMax,
        &redGamma,
        &greenMin,
        &greenMax,
        &greenGamma,
        &blueMin,
        &blueMax,
        &blueGamma) == static_cast<CGError>(noErr))
    {
        // So many choices...
        // Let's just return the green channel gamma for now.
        result = greenGamma;
    }

    return result;
}

U32 LLWindowMacOSX::getFSAASamples() const
{
    return mFSAASamples;
}

void LLWindowMacOSX::setFSAASamples(const U32 samples)
{
    mFSAASamples = samples;
    mForceRebuild = true;
}

bool LLWindowMacOSX::restoreGamma()
{
    CGDisplayRestoreColorSyncSettings();
    return true;
}

bool LLWindowMacOSX::setGamma(const F32 gamma)
{
    CGGammaValue redMin;
    CGGammaValue redMax;
    CGGammaValue redGamma;
    CGGammaValue greenMin;
    CGGammaValue greenMax;
    CGGammaValue greenGamma;
    CGGammaValue blueMin;
    CGGammaValue blueMax;
    CGGammaValue blueGamma;

    // MBW -- XXX -- Should we allow this in windowed mode?

    if(CGGetDisplayTransferByFormula(
        mDisplay,
        &redMin,
        &redMax,
        &redGamma,
        &greenMin,
        &greenMax,
        &greenGamma,
        &blueMin,
        &blueMax,
        &blueGamma) != static_cast<CGError>(noErr))
    {
        return false;
    }

    if(CGSetDisplayTransferByFormula(
        mDisplay,
        redMin,
        redMax,
        gamma,
        greenMin,
        greenMax,
        gamma,
        blueMin,
        blueMax,
        gamma) != static_cast<CGError>(noErr))
    {
        return false;
    }


    return true;
}

bool LLWindowMacOSX::isCursorHidden()
{
    return mCursorHidden;
}



// Constrains the mouse to the window.
void LLWindowMacOSX::setMouseClipping( bool b )
{
    // Just stash the requested state.  We'll simulate this when the cursor is hidden by decoupling.
    mIsMouseClipping = b;

    if(b)
    {
        //      LL_INFOS() << "setMouseClipping(true)" << LL_ENDL;
    }
    else
    {
        //      LL_INFOS() << "setMouseClipping(false)" << LL_ENDL;
    }

    adjustCursorDecouple();
}

bool LLWindowMacOSX::setCursorPosition(const LLCoordWindow position)
{
    bool result = false;
    LLCoordScreen screen_pos;

    if (!convertCoords(position, &screen_pos))
    {
        return false;
    }

    CGPoint newPosition;

    //  LL_INFOS() << "setCursorPosition(" << screen_pos.mX << ", " << screen_pos.mY << ")" << LL_ENDL;

    newPosition.x = screen_pos.mX;
    newPosition.y = screen_pos.mY;

    CGSetLocalEventsSuppressionInterval(0.0);
    if(CGWarpMouseCursorPosition(newPosition) == static_cast<CGError>(noErr))
    {
        result = true;
    }

    // Under certain circumstances, this will trigger us to decouple the cursor.
    adjustCursorDecouple(true);

    // trigger mouse move callback
    LLCoordGL gl_pos;
    convertCoords(position, &gl_pos);
    float scale = getSystemUISize();
    gl_pos.mX *= scale;
    gl_pos.mY *= scale;
    mCallbacks->handleMouseMove(this, gl_pos, (MASK)0);

    return result;
}

bool LLWindowMacOSX::getCursorPosition(LLCoordWindow *position)
{
    float cursor_point[2];
    LLCoordScreen screen_pos;

    if(mWindow == NULL)
        return false;

    getCursorPos(mWindow, cursor_point);

    if(mCursorDecoupled)
    {
        //      CGMouseDelta x, y;

        // If the cursor's decoupled, we need to read the latest movement delta as well.
        //      CGGetLastMouseDelta( &x, &y );
        //      cursor_point.h += x;
        //      cursor_point.v += y;

        // CGGetLastMouseDelta may behave strangely when the cursor's first captured.
        // Stash in the event handler instead.
        cursor_point[0] += mCursorLastEventDeltaX;
        cursor_point[1] += mCursorLastEventDeltaY;
    }

    float scale = getSystemUISize();
    position->mX = cursor_point[0] * scale;
    position->mY = cursor_point[1] * scale;

    return true;
}

void LLWindowMacOSX::adjustCursorDecouple(bool warpingMouse)
{
    if(mIsMouseClipping && mCursorHidden)
    {
        if(warpingMouse)
        {
            // The cursor should be decoupled.  Make sure it is.
            if(!mCursorDecoupled)
            {
                //          LL_INFOS() << "adjustCursorDecouple: decoupling cursor" << LL_ENDL;
                CGAssociateMouseAndMouseCursorPosition(false);
                mCursorDecoupled = true;
                mCursorIgnoreNextDelta = true;
            }
        }
    }
    else
    {
        // The cursor should not be decoupled.  Make sure it isn't.
        if(mCursorDecoupled)
        {
            //          LL_INFOS() << "adjustCursorDecouple: recoupling cursor" << LL_ENDL;
            CGAssociateMouseAndMouseCursorPosition(true);
            mCursorDecoupled = false;
        }
    }
}

F32 LLWindowMacOSX::getNativeAspectRatio()
{
    if (mFullscreen)
    {
        return (F32)mFullscreenWidth / (F32)mFullscreenHeight;
    }
    else
    {
        // The constructor for this class grabs the aspect ratio of the monitor before doing any resolution
        // switching, and stashes it in mOriginalAspectRatio.  Here, we just return it.

        if (mOverrideAspectRatio > 0.f)
        {
            return mOverrideAspectRatio;
        }

        return mOriginalAspectRatio;
    }
}

F32 LLWindowMacOSX::getPixelAspectRatio()
{
    //macOS always enforces a 1:1 pixel aspect ratio, regardless of video mode
    return 1.f;
}

//static SInt32 oldWindowLevel;

// MBW -- XXX -- There's got to be a better way than this.  Find it, please...

// Since we're no longer supporting the "typical" fullscreen mode with CGL or NSOpenGL anymore, these are unnecessary. -Geenz
void LLWindowMacOSX::beforeDialog()
{
}

void LLWindowMacOSX::afterDialog()
{
    //For fix problem with Core Flow view on OSX
    restoreGLContext();
}


void LLWindowMacOSX::flashIcon(F32 seconds)
{
    // For consistency with macOS conventions, the number of seconds given is ignored and
    // left up to the OS (which will actually bounce it for one second).
    requestUserAttention();
}

bool LLWindowMacOSX::isClipboardTextAvailable()
{
    return pasteBoardAvailable();
}

bool LLWindowMacOSX::pasteTextFromClipboard(LLWString &dst)
{
    unsigned short* pboard_data = copyFromPBoard(); // must free returned data
    llutf16string str(pboard_data);
    free(pboard_data);

    dst = utf16str_to_wstring(str);
    if (dst != L"")
    {
        return true;
    } else {
        return false;
    }
}

bool LLWindowMacOSX::copyTextToClipboard(const LLWString &s)
{
    bool result = false;
    llutf16string utf16str = wstring_to_utf16str(s);

    result = copyToPBoard(utf16str.data(), utf16str.length());

    return result;
}


// protected
bool LLWindowMacOSX::resetDisplayResolution()
{
    // This is only called from elsewhere in this class, and it's not used by the Mac implementation.
    return true;
}


LLWindow::LLWindowResolution* LLWindowMacOSX::getSupportedResolutions(S32 &num_resolutions)
{
    if (!mSupportedResolutions)
    {
        CFArrayRef modes = CGDisplayCopyAllDisplayModes(mDisplay, nullptr);

        if(modes != NULL)
        {
            CFIndex index, cnt;

            mSupportedResolutions = new LLWindowResolution[MAX_NUM_RESOLUTIONS];
            mNumSupportedResolutions = 0;

            //  Examine each mode
            cnt = CFArrayGetCount( modes );

            for ( index = 0; (index < cnt) && (mNumSupportedResolutions < MAX_NUM_RESOLUTIONS); index++ )
            {
                //  Pull the mode dictionary out of the CFArray
                CFDictionaryRef mode = (CFDictionaryRef)CFArrayGetValueAtIndex( modes, index );
                long width = getDictLong(mode, kCGDisplayWidth);
                long height = getDictLong(mode, kCGDisplayHeight);
                long bits = getDictLong(mode, kCGDisplayBitsPerPixel);

                if(bits == BITS_PER_PIXEL && width >= 800 && height >= 600)
                {
                    bool resolution_exists = false;
                    for(S32 i = 0; i < mNumSupportedResolutions; i++)
                    {
                        if (mSupportedResolutions[i].mWidth == width &&
                            mSupportedResolutions[i].mHeight == height)
                        {
                            resolution_exists = true;
                        }
                    }
                    if (!resolution_exists)
                    {
                        mSupportedResolutions[mNumSupportedResolutions].mWidth = width;
                        mSupportedResolutions[mNumSupportedResolutions].mHeight = height;
                        mNumSupportedResolutions++;
                    }
                }
            }
            CFRelease(modes);
        }
    }

    num_resolutions = mNumSupportedResolutions;
    return mSupportedResolutions;
}

bool LLWindowMacOSX::convertCoords(LLCoordGL from, LLCoordWindow *to) const
{
    to->mX = from.mX;
    to->mY = from.mY;
    return true;
}

bool LLWindowMacOSX::convertCoords(LLCoordWindow from, LLCoordGL* to) const
{
    to->mX = from.mX;
    to->mY = from.mY;
    return true;
}

bool LLWindowMacOSX::convertCoords(LLCoordScreen from, LLCoordWindow* to) const
{
    if(mWindow)
    {
        float mouse_point[2];

        mouse_point[0] = from.mX;
        mouse_point[1] = from.mY;

        convertScreenToWindow(mWindow, mouse_point);

        to->mX = mouse_point[0];
        to->mY = mouse_point[1];

        return true;
    }
    return false;
}

bool LLWindowMacOSX::convertCoords(LLCoordWindow from, LLCoordScreen *to) const
{
    if(mWindow)
    {
        float mouse_point[2];

        mouse_point[0] = from.mX;
        mouse_point[1] = from.mY;

        convertWindowToScreen(mWindow, mouse_point);

        to->mX = mouse_point[0];
        to->mY = mouse_point[1];

        return true;
    }
    return false;
}

bool LLWindowMacOSX::convertCoords(LLCoordScreen from, LLCoordGL *to) const
{
    LLCoordWindow window_coord;

    return(convertCoords(from, &window_coord) && convertCoords(window_coord, to));
}

bool LLWindowMacOSX::convertCoords(LLCoordGL from, LLCoordScreen *to) const
{
    LLCoordWindow window_coord;

    return(convertCoords(from, &window_coord) && convertCoords(window_coord, to));
}




void LLWindowMacOSX::setupFailure(const std::string& text, const std::string& caption, U32 type)
{
    destroyContext();

    OSMessageBox(text, caption, type);
}

            // Note on event recording - QUIT is a known special case and we are choosing NOT to record it for the record and playback feature
            // it is handled at a very low-level
const char* cursorIDToName(int id)
{
    switch (id)
    {
        case UI_CURSOR_ARROW:                           return "UI_CURSOR_ARROW";
        case UI_CURSOR_WAIT:                            return "UI_CURSOR_WAIT";
        case UI_CURSOR_HAND:                            return "UI_CURSOR_HAND";
        case UI_CURSOR_IBEAM:                           return "UI_CURSOR_IBEAM";
        case UI_CURSOR_CROSS:                           return "UI_CURSOR_CROSS";
        case UI_CURSOR_SIZENWSE:                        return "UI_CURSOR_SIZENWSE";
        case UI_CURSOR_SIZENESW:                        return "UI_CURSOR_SIZENESW";
        case UI_CURSOR_SIZEWE:                          return "UI_CURSOR_SIZEWE";
        case UI_CURSOR_SIZENS:                          return "UI_CURSOR_SIZENS";
        case UI_CURSOR_SIZEALL:                         return "UI_CURSOR_SIZEALL";
        case UI_CURSOR_NO:                              return "UI_CURSOR_NO";
        case UI_CURSOR_WORKING:                         return "UI_CURSOR_WORKING";
        case UI_CURSOR_TOOLGRAB:                        return "UI_CURSOR_TOOLGRAB";
        case UI_CURSOR_TOOLLAND:                        return "UI_CURSOR_TOOLLAND";
        case UI_CURSOR_TOOLFOCUS:                       return "UI_CURSOR_TOOLFOCUS";
        case UI_CURSOR_TOOLCREATE:                      return "UI_CURSOR_TOOLCREATE";
        case UI_CURSOR_ARROWDRAG:                       return "UI_CURSOR_ARROWDRAG";
        case UI_CURSOR_ARROWCOPY:                       return "UI_CURSOR_ARROWCOPY";
        case UI_CURSOR_ARROWDRAGMULTI:                  return "UI_CURSOR_ARROWDRAGMULTI";
        case UI_CURSOR_ARROWCOPYMULTI:                  return "UI_CURSOR_ARROWCOPYMULTI";
        case UI_CURSOR_NOLOCKED:                        return "UI_CURSOR_NOLOCKED";
        case UI_CURSOR_ARROWLOCKED:                     return "UI_CURSOR_ARROWLOCKED";
        case UI_CURSOR_GRABLOCKED:                      return "UI_CURSOR_GRABLOCKED";
        case UI_CURSOR_TOOLTRANSLATE:                   return "UI_CURSOR_TOOLTRANSLATE";
        case UI_CURSOR_TOOLROTATE:                      return "UI_CURSOR_TOOLROTATE";
        case UI_CURSOR_TOOLSCALE:                       return "UI_CURSOR_TOOLSCALE";
        case UI_CURSOR_TOOLCAMERA:                      return "UI_CURSOR_TOOLCAMERA";
        case UI_CURSOR_TOOLPAN:                         return "UI_CURSOR_TOOLPAN";
        case UI_CURSOR_TOOLZOOMIN:                      return "UI_CURSOR_TOOLZOOMIN";
        case UI_CURSOR_TOOLZOOMOUT:                     return "UI_CURSOR_TOOLZOOMOUT";
        case UI_CURSOR_TOOLPICKOBJECT3:                 return "UI_CURSOR_TOOLPICKOBJECT3";
        case UI_CURSOR_TOOLPLAY:                        return "UI_CURSOR_TOOLPLAY";
        case UI_CURSOR_TOOLPAUSE:                       return "UI_CURSOR_TOOLPAUSE";
        case UI_CURSOR_TOOLMEDIAOPEN:                   return "UI_CURSOR_TOOLMEDIAOPEN";
        case UI_CURSOR_PIPETTE:                         return "UI_CURSOR_PIPETTE";
        case UI_CURSOR_TOOLSIT:                         return "UI_CURSOR_TOOLSIT";
        case UI_CURSOR_TOOLBUY:                         return "UI_CURSOR_TOOLBUY";
        case UI_CURSOR_TOOLOPEN:                        return "UI_CURSOR_TOOLOPEN";
        case UI_CURSOR_TOOLPATHFINDING:                 return "UI_CURSOR_PATHFINDING";
        case UI_CURSOR_TOOLPATHFINDING_PATH_START:      return "UI_CURSOR_PATHFINDING_START";
        case UI_CURSOR_TOOLPATHFINDING_PATH_START_ADD:  return "UI_CURSOR_PATHFINDING_START_ADD";
        case UI_CURSOR_TOOLPATHFINDING_PATH_END:        return "UI_CURSOR_PATHFINDING_END";
        case UI_CURSOR_TOOLPATHFINDING_PATH_END_ADD:    return "UI_CURSOR_PATHFINDING_END_ADD";
        case UI_CURSOR_TOOLNO:                          return "UI_CURSOR_NO";
    }

    LL_ERRS() << "cursorIDToName: unknown cursor id" << id << LL_ENDL;

    return "UI_CURSOR_ARROW";
}

static CursorRef gCursors[UI_CURSOR_COUNT];


static void initPixmapCursor(int cursorid, int hotspotX, int hotspotY)
{
    // cursors are in <Application Bundle>/Contents/Resources/cursors_mac/UI_CURSOR_FOO.tif
    std::string fullpath = gDirUtilp->add(
        gDirUtilp->getAppRODataDir(),
        "cursors_mac",
        cursorIDToName(cursorid) + std::string(".tif"));

    gCursors[cursorid] = createImageCursor(fullpath.c_str(), hotspotX, hotspotY);
}

void LLWindowMacOSX::updateCursor()
{
    S32 result = 0;

    if (mDragOverrideCursor != -1)
    {
        // A drag is in progress...remember the requested cursor and we'll
        // restore it when it is done
        mCurrentCursor = mNextCursor;
        return;
    }

    if (mNextCursor == UI_CURSOR_ARROW
        && mBusyCount > 0)
    {
        mNextCursor = UI_CURSOR_WORKING;
    }

    if(mCurrentCursor == mNextCursor)
    {
        if(mCursorHidden && mHideCursorPermanent && isCGCursorVisible())
        {
            hideNSCursor();
            adjustCursorDecouple();
        }
        return;
    }

    // RN: replace multi-drag cursors with single versions
    if (mNextCursor == UI_CURSOR_ARROWDRAGMULTI)
    {
        mNextCursor = UI_CURSOR_ARROWDRAG;
    }
    else if (mNextCursor == UI_CURSOR_ARROWCOPYMULTI)
    {
        mNextCursor = UI_CURSOR_ARROWCOPY;
    }

    switch(mNextCursor)
    {
    default:
    case UI_CURSOR_ARROW:
        setArrowCursor();
        if(mCursorHidden)
        {
            // Since InitCursor resets the hide level, correct for it here.
            hideNSCursor();
        }
        break;

        // MBW -- XXX -- Some of the standard Windows cursors have no standard Mac equivalents.
        //    Find out what they look like and replicate them.

        // These are essentially correct
    case UI_CURSOR_WAIT:        /* Apple purposely doesn't allow us to set the beachball cursor manually.  Let NSApp figure out when to do this. */ break;
    case UI_CURSOR_IBEAM:       setIBeamCursor();   break;
    case UI_CURSOR_CROSS:       setCrossCursor();   break;
    case UI_CURSOR_HAND:        setPointingHandCursor();    break;
        //      case UI_CURSOR_NO:          SetThemeCursor(kThemeNotAllowedCursor); break;
    case UI_CURSOR_ARROWCOPY:   setCopyCursor();    break;

        // Double-check these
    case UI_CURSOR_NO:
    case UI_CURSOR_SIZEWE:
    case UI_CURSOR_SIZENS:
    case UI_CURSOR_SIZENWSE:
    case UI_CURSOR_SIZENESW:
    case UI_CURSOR_WORKING:
    case UI_CURSOR_TOOLGRAB:
    case UI_CURSOR_TOOLLAND:
    case UI_CURSOR_TOOLFOCUS:
    case UI_CURSOR_TOOLCREATE:
    case UI_CURSOR_ARROWDRAG:
    case UI_CURSOR_NOLOCKED:
    case UI_CURSOR_ARROWLOCKED:
    case UI_CURSOR_GRABLOCKED:
    case UI_CURSOR_PIPETTE:
    case UI_CURSOR_TOOLTRANSLATE:
    case UI_CURSOR_TOOLROTATE:
    case UI_CURSOR_TOOLSCALE:
    case UI_CURSOR_TOOLCAMERA:
    case UI_CURSOR_TOOLPAN:
    case UI_CURSOR_TOOLZOOMIN:
    case UI_CURSOR_TOOLPICKOBJECT3:
    case UI_CURSOR_TOOLPLAY:
    case UI_CURSOR_TOOLPAUSE:
    case UI_CURSOR_TOOLMEDIAOPEN:
    case UI_CURSOR_TOOLSIT:
    case UI_CURSOR_TOOLBUY:
    case UI_CURSOR_TOOLOPEN:
    case UI_CURSOR_TOOLPATHFINDING:
    case UI_CURSOR_TOOLPATHFINDING_PATH_START:
    case UI_CURSOR_TOOLPATHFINDING_PATH_START_ADD:
    case UI_CURSOR_TOOLPATHFINDING_PATH_END:
    case UI_CURSOR_TOOLPATHFINDING_PATH_END_ADD:
    case UI_CURSOR_TOOLNO:
        result = setImageCursor(gCursors[mNextCursor]);
        break;

    }

    if(result != noErr)
    {
        setArrowCursor();
    }

    mCurrentCursor = mNextCursor;
}

ECursorType LLWindowMacOSX::getCursor() const
{
    return mCurrentCursor;
}

void LLWindowMacOSX::initCursors()
{
    initPixmapCursor(UI_CURSOR_NO, 8, 8);
    initPixmapCursor(UI_CURSOR_WORKING, 1, 1);
    initPixmapCursor(UI_CURSOR_TOOLGRAB, 2, 14);
    initPixmapCursor(UI_CURSOR_TOOLLAND, 13, 8);
    initPixmapCursor(UI_CURSOR_TOOLFOCUS, 7, 6);
    initPixmapCursor(UI_CURSOR_TOOLCREATE, 7, 7);
    initPixmapCursor(UI_CURSOR_ARROWDRAG, 1, 1);
    initPixmapCursor(UI_CURSOR_ARROWCOPY, 1, 1);
    initPixmapCursor(UI_CURSOR_NOLOCKED, 8, 8);
    initPixmapCursor(UI_CURSOR_ARROWLOCKED, 1, 1);
    initPixmapCursor(UI_CURSOR_GRABLOCKED, 2, 14);
    initPixmapCursor(UI_CURSOR_PIPETTE, 3, 29);
    initPixmapCursor(UI_CURSOR_TOOLTRANSLATE, 1, 1);
    initPixmapCursor(UI_CURSOR_TOOLROTATE, 1, 1);
    initPixmapCursor(UI_CURSOR_TOOLSCALE, 1, 1);
    initPixmapCursor(UI_CURSOR_TOOLCAMERA, 7, 6);
    initPixmapCursor(UI_CURSOR_TOOLPAN, 7, 6);
    initPixmapCursor(UI_CURSOR_TOOLZOOMIN, 7, 6);
    initPixmapCursor(UI_CURSOR_TOOLZOOMOUT, 7, 6);
    initPixmapCursor(UI_CURSOR_TOOLPICKOBJECT3, 1, 1);
    initPixmapCursor(UI_CURSOR_TOOLPLAY, 1, 1);
    initPixmapCursor(UI_CURSOR_TOOLPAUSE, 1, 1);
    initPixmapCursor(UI_CURSOR_TOOLMEDIAOPEN, 1, 1);
    initPixmapCursor(UI_CURSOR_TOOLSIT, 20, 15);
    initPixmapCursor(UI_CURSOR_TOOLBUY, 20, 15);
    initPixmapCursor(UI_CURSOR_TOOLOPEN, 20, 15);
    initPixmapCursor(UI_CURSOR_TOOLPATHFINDING, 16, 16);
    initPixmapCursor(UI_CURSOR_TOOLPATHFINDING_PATH_START, 16, 16);
    initPixmapCursor(UI_CURSOR_TOOLPATHFINDING_PATH_START_ADD, 16, 16);
    initPixmapCursor(UI_CURSOR_TOOLPATHFINDING_PATH_END, 16, 16);
    initPixmapCursor(UI_CURSOR_TOOLPATHFINDING_PATH_END_ADD, 16, 16);
    initPixmapCursor(UI_CURSOR_TOOLNO, 8, 8);

    initPixmapCursor(UI_CURSOR_SIZENWSE, 10, 10);
    initPixmapCursor(UI_CURSOR_SIZENESW, 10, 10);
    initPixmapCursor(UI_CURSOR_SIZEWE, 10, 10);
    initPixmapCursor(UI_CURSOR_SIZENS, 10, 10);
    initPixmapCursor(UI_CURSOR_SIZEALL, 10, 10);

}

void LLWindowMacOSX::captureMouse()
{
    // By registering a global CarbonEvent handler for mouse move events, we ensure that
    // mouse events are always processed.  Thus, capture and release are unnecessary.
}

void LLWindowMacOSX::releaseMouse()
{
    // By registering a global CarbonEvent handler for mouse move events, we ensure that
    // mouse events are always processed.  Thus, capture and release are unnecessary.
}

void LLWindowMacOSX::hideCursor()
{
    if(!mCursorHidden)
    {
        //      LL_INFOS() << "hideCursor: hiding" << LL_ENDL;
        mCursorHidden = true;
        mHideCursorPermanent = true;
        hideNSCursor();
    }
    else
    {
        //      LL_INFOS() << "hideCursor: already hidden" << LL_ENDL;
    }

    adjustCursorDecouple();
}

void LLWindowMacOSX::showCursor()
{
    if(mCursorHidden || !isCGCursorVisible())
    {
        //      LL_INFOS() << "showCursor: showing" << LL_ENDL;
        mCursorHidden = false;
        mHideCursorPermanent = false;
        showNSCursor();
    }
    else
    {
        //      LL_INFOS() << "showCursor: already visible" << LL_ENDL;
    }

    adjustCursorDecouple();
}

void LLWindowMacOSX::showCursorFromMouseMove()
{
    if (!mHideCursorPermanent)
    {
        showCursor();
    }
}

void LLWindowMacOSX::hideCursorUntilMouseMove()
{
    if (!mHideCursorPermanent)
    {
        hideCursor();
        mHideCursorPermanent = false;
    }
}



//
// LLSplashScreenMacOSX
//
LLSplashScreenMacOSX::LLSplashScreenMacOSX()
{
    mWindow = NULL;
}

LLSplashScreenMacOSX::~LLSplashScreenMacOSX()
{
}

void LLSplashScreenMacOSX::showImpl()
{
    // This code _could_ be used to display a spash screen...
}

void LLSplashScreenMacOSX::updateImpl(const std::string& mesg)
{
    if(mWindow != NULL)
    {
        CFStringCreateWithCString(NULL, mesg.c_str(), kCFStringEncodingUTF8);
    }
}


void LLSplashScreenMacOSX::hideImpl()
{
    if(mWindow != NULL)
    {
        mWindow = NULL;
    }
}

S32 OSMessageBoxMacOSX(const std::string& text, const std::string& caption, U32 type)
{
    return showAlert(text, caption, type);
}

// Open a URL with the user's default web browser.
// Must begin with protocol identifier.
void LLWindowMacOSX::spawnWebBrowser(const std::string& escaped_url, bool async)
{
    // I'm fairly certain that this is all legitimate under Apple's currently supported APIs.

    bool found = false;
    S32 i;
    for (i = 0; i < gURLProtocolWhitelistCount; i++)
    {
        if (escaped_url.find(gURLProtocolWhitelist[i]) != std::string::npos)
        {
            found = true;
            break;
        }
    }

    if (!found)
    {
        LL_WARNS() << "spawn_web_browser called for url with protocol not on whitelist: " << escaped_url << LL_ENDL;
        return;
    }

    S32 result = 0;
    CFURLRef urlRef = NULL;

    LL_INFOS() << "Opening URL " << escaped_url << LL_ENDL;

    CFStringRef stringRef = CFStringCreateWithCString(NULL, escaped_url.c_str(), kCFStringEncodingUTF8);
    if (stringRef)
    {
        // This will succeed if the string is a full URL, including the http://
        // Note that URLs specified this way need to be properly percent-escaped.
        urlRef = CFURLCreateWithString(NULL, stringRef, NULL);

        // Don't use CRURLCreateWithFileSystemPath -- only want valid URLs

        CFRelease(stringRef);
    }

    if (urlRef)
    {
        result = LSOpenCFURLRef(urlRef, NULL);

        if (result != noErr)
        {
            LL_INFOS() << "Error " << result << " on open." << LL_ENDL;
        }

        CFRelease(urlRef);
    }
    else
    {
        LL_INFOS() << "Error: couldn't create URL." << LL_ENDL;
    }
}

// String should match ndof, so string mapping code was copied as is
static char mapChar( char c )
{
    unsigned char uc = ( unsigned char ) c;

    switch( uc )
    {
        case '/': return '-'; // use dash instead of slash

        case 0x7F: return ' ';
        case 0x80: return 'A';
        case 0x81: return 'A';
        case 0x82: return 'C';
        case 0x83: return 'E';
        case 0x84: return 'N';
        case 0x85: return 'O';
        case 0x86: return 'U';
        case 0x87: return 'a';
        case 0x88: return 'a';
        case 0x89: return 'a';
        case 0x8A: return 'a';
        case 0x8B: return 'a';
        case 0x8C: return 'a';
        case 0x8D: return 'c';
        case 0x8E: return 'e';
        case 0x8F: return 'e';
        case 0x90: return ' ';
        case 0x91: return ' '; // ? '
        case 0x92: return ' '; // ? '
        case 0x93: return ' '; // ? "
        case 0x94: return ' '; // ? "
        case 0x95: return ' ';
        case 0x96: return ' ';
        case 0x97: return ' ';
        case 0x98: return ' ';
        case 0x99: return ' ';
        case 0x9A: return ' ';
        case 0x9B: return 0x27;
        case 0x9C: return 0x22;
        case 0x9D: return ' ';
        case 0x9E: return ' ';
        case 0x9F: return ' ';
        case 0xA0: return ' ';
        case 0xA1: return ' ';
        case 0xA2: return ' ';
        case 0xA3: return ' ';
        case 0xA4: return ' ';
        case 0xA5: return ' ';
        case 0xA6: return ' ';
        case 0xA7: return ' ';
        case 0xA8: return ' ';
        case 0xA9: return ' ';
        case 0xAA: return ' ';
        case 0xAB: return ' ';
        case 0xAC: return ' ';
        case 0xAD: return ' ';
        case 0xAE: return ' ';
        case 0xAF: return ' ';
        case 0xB0: return ' ';
        case 0xB1: return ' ';
        case 0xB2: return ' ';
        case 0xB3: return ' ';
        case 0xB4: return ' ';
        case 0xB5: return ' ';
        case 0xB6: return ' ';
        case 0xB7: return ' ';
        case 0xB8: return ' ';
        case 0xB9: return ' ';
        case 0xBA: return ' ';
        case 0xBB: return ' ';
        case 0xBC: return ' ';
        case 0xBD: return ' ';
        case 0xBE: return ' ';
        case 0xBF: return ' ';
        case 0xC0: return ' ';
        case 0xC1: return ' ';
        case 0xC2: return ' ';
        case 0xC3: return ' ';
        case 0xC4: return ' ';
        case 0xC5: return ' ';
        case 0xC6: return ' ';
        case 0xC7: return ' ';
        case 0xC8: return ' ';
        case 0xC9: return ' ';
        case 0xCA: return ' ';
        case 0xCB: return 'A';
        case 0xCC: return 'A';
        case 0xCD: return 'O';
        case 0xCE: return ' ';
        case 0xCF: return ' ';
        case 0xD0: return '-';
        case 0xD1: return '-';
        case 0xD2: return 0x22;
        case 0xD3: return 0x22;
        case 0xD4: return 0x27;
        case 0xD5: return 0x27;
        case 0xD6: return '-'; // use dash instead of slash
        case 0xD7: return ' ';
        case 0xD8: return 'y';
        case 0xD9: return 'Y';
        case 0xDA: return '-'; // use dash instead of slash
        case 0xDB: return ' ';
        case 0xDC: return '<';
        case 0xDD: return '>';
        case 0xDE: return ' ';
        case 0xDF: return ' ';
        case 0xE0: return ' ';
        case 0xE1: return ' ';
        case 0xE2: return ',';
        case 0xE3: return ',';
        case 0xE4: return ' ';
        case 0xE5: return 'A';
        case 0xE6: return 'E';
        case 0xE7: return 'A';
        case 0xE8: return 'E';
        case 0xE9: return 'E';
        case 0xEA: return 'I';
        case 0xEB: return 'I';
        case 0xEC: return 'I';
        case 0xED: return 'I';
        case 0xEE: return 'O';
        case 0xEF: return 'O';
        case 0xF0: return ' ';
        case 0xF1: return 'O';
        case 0xF2: return 'U';
        case 0xF3: return 'U';
        case 0xF4: return 'U';
        case 0xF5: return '|';
        case 0xF6: return ' ';
        case 0xF7: return ' ';
        case 0xF8: return ' ';
        case 0xF9: return ' ';
        case 0xFA: return '.';
        case 0xFB: return ' ';
        case 0xFC: return ' ';
        case 0xFD: return 0x22;
        case 0xFE: return ' ';
        case 0xFF: return ' ';
    }
    return c;
}

// String should match ndof for manufacturer based search to work
static void sanitizeString( char* inCStr )
{
    char* charIt = inCStr;
    while ( *charIt )
    {
        *charIt = mapChar( *charIt );
        charIt++;
    }
}

struct HidDevice
{
    long mAxis;
    long mLocalID;
    char mProduct[256];
    char mManufacturer[256];
    long mUsage;
    long mUsagePage;
};

static void populate_device_info( io_object_t io_obj_p, CFDictionaryRef device_dic, HidDevice* devicep )
{
    CFMutableDictionaryRef io_properties = nil;
    io_registry_entry_t entry1;
    io_registry_entry_t entry2;
    kern_return_t rc;

    // Mac OS X currently is not mirroring all USB properties to HID page so need to look at USB device page also
    // get dictionary for usb properties: step up two levels and get CF dictionary for USB properties
    // try to get parent1
    rc = IORegistryEntryGetParentEntry( io_obj_p, kIOServicePlane, &entry1 );
    if ( KERN_SUCCESS == rc )
    {
        rc = IORegistryEntryGetParentEntry( entry1, kIOServicePlane, &entry2 );

        IOObjectRelease( entry1 );

        if ( KERN_SUCCESS == rc )
        {
            rc = IORegistryEntryCreateCFProperties( entry2, &io_properties, kCFAllocatorDefault, kNilOptions );
            // either way, release parent2
            IOObjectRelease( entry2 );
        }
    }
    if ( KERN_SUCCESS == rc )
    {
        // IORegistryEntryCreateCFProperties() succeeded
        if ( io_properties != nil )
        {
            CFTypeRef dict_element = 0;
            // get device info
            // try hid dictionary first, if fail then go to usb dictionary


            dict_element = CFDictionaryGetValue( device_dic, CFSTR(kIOHIDProductKey) );
            if ( !dict_element )
            {
                dict_element = CFDictionaryGetValue( io_properties, CFSTR( "USB Product Name" ) );
            }
            if ( dict_element )
            {
                bool res = CFStringGetCString((CFStringRef)dict_element, devicep->mProduct, 256, kCFStringEncodingUTF8);
                sanitizeString(devicep->mProduct);
                if ( !res )
                {
                    LL_WARNS("Joystick") << "Failed to populate mProduct" << LL_ENDL;
                }
            }

            dict_element = CFDictionaryGetValue( device_dic, CFSTR( kIOHIDManufacturerKey ) );
            if ( !dict_element )
            {
                dict_element = CFDictionaryGetValue( io_properties, CFSTR( "USB Vendor Name" ) );
            }
            if ( dict_element )
            {
                bool res = CFStringGetCString( (CFStringRef)dict_element, devicep->mManufacturer, 256, kCFStringEncodingUTF8 );
                sanitizeString(devicep->mManufacturer);
                if ( !res )
                {
                    LL_WARNS("Joystick") << "Failed to populate mManufacturer" << LL_ENDL;
                }
            }

            dict_element = CFDictionaryGetValue( device_dic, CFSTR( kIOHIDLocationIDKey ) );
            if ( !dict_element )
            {
                dict_element = CFDictionaryGetValue( io_properties, CFSTR( "locationID" ) );
            }
            if ( dict_element )
            {
                bool res = CFNumberGetValue( (CFNumberRef)dict_element, kCFNumberLongType, &devicep->mLocalID );
                if ( !res )
                {
                    LL_WARNS("Joystick") << "Failed to populate mLocalID" << LL_ENDL;
                }
            }

            dict_element = CFDictionaryGetValue( device_dic, CFSTR( kIOHIDPrimaryUsagePageKey ) );
            if ( dict_element )
            {
                bool res = CFNumberGetValue( (CFNumberRef)dict_element, kCFNumberLongType, &devicep->mUsagePage );
                if ( !res )
                {
                    LL_WARNS("Joystick") << "Failed to populate mUsagePage" << LL_ENDL;
                }
                dict_element = CFDictionaryGetValue( device_dic, CFSTR( kIOHIDPrimaryUsageKey ) );
                if ( dict_element )
                {
                    if ( !CFNumberGetValue( (CFNumberRef)dict_element, kCFNumberLongType, &devicep->mUsage ) )
                    {
                        LL_WARNS("Joystick") << "Failed to populate mUsage" << LL_ENDL;
                    }
                }
            }

            //Add axis, because ndof lib checks sutability by axises as well as other elements
            devicep->mAxis = 0;
            CFTypeRef hid_elements = CFDictionaryGetValue( device_dic, CFSTR( kIOHIDElementKey ) );
            if ( hid_elements && CFGetTypeID( hid_elements ) == CFArrayGetTypeID( ) )
            {
                long count = CFArrayGetCount( (CFArrayRef) hid_elements );
                for (int i = 0; i < count; ++i)
                {
                    CFTypeRef element = CFArrayGetValueAtIndex((CFArrayRef) hid_elements, i);
                    if (element && CFGetTypeID( element ) == CFDictionaryGetTypeID( ))
                    {
                        long type = 0, usage_page = 0, usage = 0;

                        CFTypeRef ref_value = CFDictionaryGetValue( (CFDictionaryRef) element, CFSTR( kIOHIDElementTypeKey ) );
                        if ( ref_value )
                        {
                            CFNumberGetValue( (CFNumberRef)ref_value, kCFNumberLongType, &type );
                        }

                        ref_value = CFDictionaryGetValue( (CFDictionaryRef) element, CFSTR( kIOHIDElementUsagePageKey ) );
                        if ( ref_value )
                        {
                             CFNumberGetValue( (CFNumberRef)ref_value, kCFNumberLongType, &usage_page );
                        }

                        ref_value = CFDictionaryGetValue( (CFDictionaryRef) element, CFSTR( kIOHIDElementUsageKey ) );
                        if ( ref_value )
                        {
                            CFNumberGetValue( (CFNumberRef)ref_value, kCFNumberLongType, &usage );
                        }
                        if ( type != 0
                            && type != kIOHIDElementTypeCollection
                            && usage_page == kHIDPage_GenericDesktop)
                        {
                            switch( usage )
                            {
                                case kHIDUsage_GD_X:
                                case kHIDUsage_GD_Y:
                                case kHIDUsage_GD_Z:
                                case kHIDUsage_GD_Rx:
                                case kHIDUsage_GD_Ry:
                                case kHIDUsage_GD_Rz:
                                    devicep->mAxis++;
                                    break;
                                default:
                                    break;
                            }
                        }
                    }
                }
            }

            CFRelease(io_properties);
        }
        else
        {
            LL_WARNS("Joystick") << "Failed to populate fields" << LL_ENDL;
        }
    }
}

HidDevice populate_device( io_object_t io_obj )
{
    void* interfacep = nullptr;
    HidDevice device;
    memset( &device, 0, sizeof( HidDevice ) );
    CFMutableDictionaryRef device_dic = 0;
    kern_return_t result = IORegistryEntryCreateCFProperties( io_obj, &device_dic, kCFAllocatorDefault, kNilOptions );

    if ( KERN_SUCCESS == result
        && device_dic )
    {
        IOReturn io_result = kIOReturnSuccess;
        HRESULT query_result = S_OK;
        SInt32 the_score = 0;
        IOCFPlugInInterface **the_interface = NULL;


        io_result = IOCreatePlugInInterfaceForService( io_obj, kIOHIDDeviceUserClientTypeID,
                                                        kIOCFPlugInInterfaceID, &the_interface, &the_score );
        if ( io_result == kIOReturnSuccess )
        {
            query_result = ( *the_interface )->QueryInterface( the_interface, CFUUIDGetUUIDBytes( kIOHIDDeviceInterfaceID ), ( LPVOID * ) & ( interfacep ) );
            if ( query_result != S_OK )
            {
                LL_WARNS("Joystick") << "QueryInterface failed" << LL_ENDL;
            }
            IODestroyPlugInInterface( the_interface );
        }
        else
        {
            LL_WARNS("Joystick") << "IOCreatePlugInInterfaceForService failed" << LL_ENDL;
        }

        if ( interfacep )
        {
            result = ( *( IOHIDDeviceInterface** )interfacep )->open( interfacep, 0 );

            if ( result != kIOReturnSuccess)
            {
                LL_WARNS("Joystick") << "open failed" << LL_ENDL;
            }
        }
        // extract needed fields
        populate_device_info( io_obj, device_dic, &device );

        // Release interface
        if ( interfacep )
        {
            ( *( IOHIDDeviceInterface** ) interfacep )->close( interfacep );

            ( *( IOHIDDeviceInterface** ) interfacep )->Release( interfacep );

            interfacep = NULL;
        }

        CFRelease( device_dic );
    }
    else
    {
        LL_WARNS("Joystick") << "populate_device failed" << LL_ENDL;
    }

    return device;
}

static void get_devices(std::list<HidDevice> &list_of_devices,
                          io_iterator_t inIODeviceIterator)
{
    IOReturn result = kIOReturnSuccess;    // assume success( optimist! )
    io_object_t io_obj = 0;

    while ( 0 != (io_obj = IOIteratorNext( inIODeviceIterator ) ) )
    {
        HidDevice device = populate_device( io_obj );

        //  Should match ndof
        if (device.mAxis >= 3
            || (device.mUsagePage == kHIDPage_GenericDesktop
                && (device.mUsage == kHIDUsage_GD_MultiAxisController
                    || device.mUsage == kHIDUsage_GD_GamePad
                    || device.mUsage == kHIDUsage_GD_Joystick))
            || (device.mUsagePage == kHIDPage_Game
                && device.mUsage == kHIDUsage_Game_3DGameController)
            || strstr(device.mManufacturer, "3Dconnexion"))
        {
            list_of_devices.push_back(device);
        }
        else
        {
            LL_DEBUGS("Joystick");
            list_of_devices.push_back(device);
            LL_CONT << "Device axes: " << (S32)device.mAxis
                    << " Device HIDUsepage: " << (S32)device.mUsagePage
                    << " Device HIDUsage: " << (S32)device.mUsage;
            LL_ENDL;
        }


        // release the device object, it is no longer needed
        result = IOObjectRelease( io_obj );
        if ( KERN_SUCCESS != result )
        {
            LL_WARNS("Joystick") << "IOObjectRelease failed" << LL_ENDL;
        }
    }
}

bool LLWindowMacOSX::getInputDevices(U32 device_type_filter,
                                     std::function<bool(std::string&, LLSD&, void*)> osx_callback,
                                     void* win_callback,
                                     void* userdata)
{
    bool return_value = false;
    CFMutableDictionaryRef device_dict_ref;
    IOReturn result = kIOReturnSuccess;    // assume success( optimist! )

    // Set up matching dictionary to search the I/O Registry for HID devices we are interested in. Dictionary reference is NULL if error.

    // A dictionary to match devices to?
    device_dict_ref = IOServiceMatching( kIOHIDDeviceKey );

    // BUG FIX! one reference is consumed by IOServiceGetMatchingServices
    CFRetain( device_dict_ref );
    io_iterator_t io_iter = 0;

    // create an IO object iterator
    result = IOServiceGetMatchingServices( kIOMasterPortDefault, device_dict_ref, &io_iter );
    if ( kIOReturnSuccess != result )
    {
        LL_WARNS("Joystick") << "IOServiceGetMatchingServices failed" << LL_ENDL;
    }

    if ( io_iter )
    {
        // add all existing devices
        std::list<HidDevice> device_list;

        get_devices(device_list, io_iter);

        std::list<HidDevice>::iterator iter;

        for (iter = device_list.begin(); iter != device_list.end(); ++iter)
        {
            std::string label(iter->mProduct);
            LLSD data;
            data["manufacturer"] = std::string(iter->mManufacturer);
            data["product"] = label;

            if (osx_callback(label, data, userdata))
            {
                break; //found device
            }
        }
        return_value = true;
    }

    CFRelease( device_dict_ref );
    return return_value;
}

LLSD LLWindowMacOSX::getNativeKeyData() const
{
    LLSD result = LLSD::emptyMap();

    if(mRawKeyEvent)
    {
        result["event_type"] = LLSD::Integer(mRawKeyEvent->mEventType);
        result["event_modifiers"] = LLSD::Integer(mRawKeyEvent->mEventModifiers);
        result["event_keycode"] = LLSD::Integer(mRawKeyEvent->mEventKeyCode);
        result["event_chars"] = (mRawKeyEvent->mEventChars) ? LLSD(LLSD::Integer(mRawKeyEvent->mEventChars)) : LLSD();
        result["event_umodchars"] = (mRawKeyEvent->mEventUnmodChars) ? LLSD(LLSD::Integer(mRawKeyEvent->mEventUnmodChars)) : LLSD();
        result["event_isrepeat"] = LLSD::Boolean(mRawKeyEvent->mEventRepeat);
    }

    LL_DEBUGS() << "native key data is: " << result << LL_ENDL;

    return result;
}

bool LLWindowMacOSX::dialogColorPicker( F32 *r, F32 *g, F32 *b)
{
    bool    retval = false;
    OSErr   error = noErr;
    NColorPickerInfo    info;

    memset(&info, 0, sizeof(info));
    info.theColor.color.rgb.red = (UInt16)(*r * 65535.f);
    info.theColor.color.rgb.green = (UInt16)(*g * 65535.f);
    info.theColor.color.rgb.blue = (UInt16)(*b * 65535.f);
    info.placeWhere = kCenterOnMainScreen;

    error = NPickColor(&info);

    if (error == noErr)
    {
        retval = info.newColorChosen;
        if (info.newColorChosen)
        {
            *r = ((float) info.theColor.color.rgb.red) / 65535.0;
            *g = ((float) info.theColor.color.rgb.green) / 65535.0;
            *b = ((float) info.theColor.color.rgb.blue) / 65535.0;
        }
    }

    return (retval);
}

void *LLWindowMacOSX::getPlatformWindow()
{
    // NOTE: this will be NULL in fullscreen mode.  Plan accordingly.
    return (void*)mWindow;
}

// get a double value from a dictionary
/*
static double getDictDouble (CFDictionaryRef refDict, CFStringRef key)
{
    double double_value;
    CFNumberRef number_value = (CFNumberRef) CFDictionaryGetValue(refDict, key);
    if (!number_value) // if can't get a number for the dictionary
        return -1;  // fail
    if (!CFNumberGetValue(number_value, kCFNumberDoubleType, &double_value)) // or if cant convert it
        return -1; // fail
    return double_value; // otherwise return the long value
}*/

// get a long value from a dictionary
static long getDictLong (CFDictionaryRef refDict, CFStringRef key)
{
    long int_value;
    CFNumberRef number_value = (CFNumberRef) CFDictionaryGetValue(refDict, key);
    if (!number_value) // if can't get a number for the dictionary
        return -1;  // fail
    if (!CFNumberGetValue(number_value, kCFNumberLongType, &int_value)) // or if cant convert it
        return -1; // fail
    return int_value; // otherwise return the long value
}

void LLWindowMacOSX::allowLanguageTextInput(LLPreeditor *preeditor, bool b)
{
    if (preeditor != mPreeditor && !b)
    {
        // This condition may occur by a call to
        // setEnabled(bool) against LLTextEditor or LLLineEditor
        // when the control is not focused.
        // We need to silently ignore the case so that
        // the language input status of the focused control
        // is not disturbed.
        return;
    }

    // Take care of old and new preeditors.
    if (preeditor != mPreeditor || !b)
    {
        // We need to interrupt before updating mPreeditor,
        // so that the fix string from input method goes to
        // the old preeditor.
        if (mLanguageTextInputAllowed)
        {
            interruptLanguageTextInput();
        }
        mPreeditor = (b ? preeditor : NULL);
    }

    if (b == mLanguageTextInputAllowed)
    {
        return;
    }
    mLanguageTextInputAllowed = b;
    allowDirectMarkedTextInput(b, mGLView); // mLanguageTextInputAllowed and mMarkedTextAllowed should be updated at once (by Pell Smit
}

class sharedContext
{
public:
    CGLContextObj mContext;
};

void* LLWindowMacOSX::createSharedContext()
{
    sharedContext* sc = new sharedContext();
    CGLCreateContext(mPixelFormat, mContext, &(sc->mContext));

    if (sUseMultGL)
    {
        CGLEnable(mContext, kCGLCEMPEngine);
    }

    return (void *)sc;
}

void LLWindowMacOSX::makeContextCurrent(void* context)
{
    CGLSetCurrentContext(((sharedContext*)context)->mContext);

    //enable multi-threaded OpenGL
    if (sUseMultGL)
    {
        CGLError cgl_err;
        CGLContextObj ctx = CGLGetCurrentContext();

        cgl_err =  CGLEnable( ctx, kCGLCEMPEngine);

        if (cgl_err != kCGLNoError )
        {
            LL_INFOS("GLInit") << "Multi-threaded OpenGL not available." << LL_ENDL;
        }
        else
        {
            LL_INFOS("GLInit") << "Multi-threaded OpenGL enabled." << LL_ENDL;
        }
    }

}

void LLWindowMacOSX::destroySharedContext(void* context)
{
    sharedContext* sc = (sharedContext*)context;

    CGLDestroyContext(sc->mContext);

    delete sc;
}

void LLWindowMacOSX::toggleVSync(bool enable_vsync)
{
    GLint frames_per_swap = 0;
    if (!enable_vsync)
    {
        frames_per_swap = 0;
    }
    else
    {
        frames_per_swap = 1;
    }

    CGLSetParameter(mContext, kCGLCPSwapInterval, &frames_per_swap);
}

void LLWindowMacOSX::interruptLanguageTextInput()
{
    commitCurrentPreedit(mGLView);
}

// static
std::vector<std::string> LLWindowMacOSX::getDisplaysResolutionList()
{
    std::vector<std::string> resolution_list;

    CGDirectDisplayID display_ids[10];
    uint32_t found_displays = 0;
    CGError err = CGGetActiveDisplayList(10, display_ids, &found_displays);

    if (kCGErrorSuccess != err)
    {
        LL_WARNS() << "Couldn't get a list of active displays" << LL_ENDL;
        return std::vector<std::string>();
    }

    for (uint32_t i = 0; i < found_displays; i++)
    {
        S32 monitor_width = CGDisplayPixelsWide(display_ids[i]);
        S32 monitor_height = CGDisplayPixelsHigh(display_ids[i]);

        std::ostringstream sstream;
        sstream << monitor_width << "x" << monitor_height;;
        std::string res = sstream.str();

        resolution_list.push_back(res);
    }

    return resolution_list;
}

// static
std::vector<std::string> LLWindowMacOSX::getDynamicFallbackFontList()
{
    // Fonts previously in getFontListSans() have moved to fonts.xml.
    return std::vector<std::string>();
}

// static
MASK LLWindowMacOSX::modifiersToMask(S16 modifiers)
{
    MASK mask = 0;
    if(modifiers & MAC_SHIFT_KEY) { mask |= MASK_SHIFT; }
    if(modifiers & (MAC_CMD_KEY | MAC_CTRL_KEY)) { mask |= MASK_CONTROL; }
    if(modifiers & MAC_ALT_KEY) { mask |= MASK_ALT; }
    return mask;
}

F32 LLWindowMacOSX::getSystemUISize()
{
    return gHiDPISupport ? ::getDeviceUnitSize(mGLView) : LLWindow::getSystemUISize();
}

void LLWindowMacOSX::openFolder(const std::string &path)
{
    openFolderWithFinder(path.c_str());
}

#if LL_OS_DRAGDROP_ENABLED
/*
S16 LLWindowMacOSX::dragTrackingHandler(DragTrackingMessage message, WindowRef theWindow,
                          void * handlerRefCon, DragRef drag)
{
    S16 result = 0;
    LLWindowMacOSX *self = (LLWindowMacOSX*)handlerRefCon;

    LL_DEBUGS() << "drag tracking handler, message = " << message << LL_ENDL;

    switch(message)
    {
        case kDragTrackingInWindow:
            result = self->handleDragNDrop(drag, LLWindowCallbacks::DNDA_TRACK);
        break;

        case kDragTrackingEnterHandler:
            result = self->handleDragNDrop(drag, LLWindowCallbacks::DNDA_START_TRACKING);
        break;

        case kDragTrackingLeaveHandler:
            result = self->handleDragNDrop(drag, LLWindowCallbacks::DNDA_STOP_TRACKING);
        break;

        default:
        break;
    }

    return result;
}
OSErr LLWindowMacOSX::dragReceiveHandler(WindowRef theWindow, void * handlerRefCon,
                                         DragRef drag)
{
    LLWindowMacOSX *self = (LLWindowMacOSX*)handlerRefCon;
    return self->handleDragNDrop(drag, LLWindowCallbacks::DNDA_DROPPED);

}
*/
void LLWindowMacOSX::handleDragNDrop(std::string url, LLWindowCallbacks::DragNDropAction action)
{
    MASK mask = LLWindowMacOSX::modifiersToMask(getModifiers());

    float mouse_point[2];
    // This will return the mouse point in window coords
    getCursorPos(mWindow, mouse_point);
    LLCoordWindow window_coords(mouse_point[0], mouse_point[1]);
    LLCoordGL gl_pos;
    convertCoords(window_coords, &gl_pos);

    if(!url.empty())
    {
        LLWindowCallbacks::DragNDropResult res =
        mCallbacks->handleDragNDrop(this, gl_pos, mask, action, url);

        switch (res) {
            case LLWindowCallbacks::DND_NONE:       // No drop allowed
                if (action == LLWindowCallbacks::DNDA_TRACK)
                {
                    mDragOverrideCursor = 0;
                }
                else {
                    mDragOverrideCursor = -1;
                }
                break;
            case LLWindowCallbacks::DND_MOVE:       // Drop accepted would result in a "move" operation
                mDragOverrideCursor = UI_CURSOR_NO;
                break;
            case LLWindowCallbacks::DND_COPY:       // Drop accepted would result in a "copy" operation
                mDragOverrideCursor = UI_CURSOR_ARROWCOPY;
                break;
            default:
                mDragOverrideCursor = -1;
                break;
        }
        // This overrides the cursor being set by setCursor.
        // This is a bit of a hack workaround because lots of areas
        // within the viewer just blindly set the cursor.
        if (mDragOverrideCursor == -1)
        {
            // Restore the cursor
            ECursorType temp_cursor = mCurrentCursor;
            // get around the "setting the same cursor" code in setCursor()
            mCurrentCursor = UI_CURSOR_COUNT;
            setCursor(temp_cursor);
        }
        else {
            // Override the cursor
            switch (mDragOverrideCursor) {
                case 0:
                    setArrowCursor();
                    break;
                case UI_CURSOR_NO:
                    setNotAllowedCursor();
                case UI_CURSOR_ARROWCOPY:
                    setCopyCursor();
                default:
                    break;
            };
        }
    }
}

#endif // LL_OS_DRAGDROP_ENABLED
