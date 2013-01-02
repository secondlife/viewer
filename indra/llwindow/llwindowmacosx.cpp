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

#include "llkeyboardmacosx.h"
#include "llwindowcallbacks.h"
#include "llpreeditor.h"

#include "llerror.h"
#include "llgl.h"
#include "llstring.h"
#include "lldir.h"
#include "indra_constants.h"

#include <OpenGL/OpenGL.h>

extern BOOL gDebugWindowProc;

// culled from winuser.h
//const S32	WHEEL_DELTA = 120;     /* Value for rolling one detent */
// On the Mac, the scroll wheel reports a delta of 1 for each detent.
// There's also acceleration for faster scrolling, based on a slider in the system preferences.
const S32	WHEEL_DELTA = 1;     /* Value for rolling one detent */
const S32	BITS_PER_PIXEL = 32;
const S32	MAX_NUM_RESOLUTIONS = 32;


//
// LLWindowMacOSX
//

BOOL LLWindowMacOSX::sUseMultGL = FALSE;

// Cross-platform bits:

BOOL check_for_card(const char* RENDERER, const char* bad_card)
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
			return FALSE;
		}
		else
		{
			return TRUE;
		}
	}

	return FALSE;
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
							   BOOL fullscreen, BOOL clearBg,
							   BOOL disable_vsync, BOOL use_gl,
							   BOOL ignore_pixel_depth,
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
	mSimulatedRightClick = FALSE;
	mLastModifiers = 0;
	mHandsOffEvents = FALSE;
	mCursorDecoupled = FALSE;
	mCursorLastEventDeltaX = 0;
	mCursorLastEventDeltaY = 0;
	mCursorIgnoreNextDelta = FALSE;
	mNeedsResize = FALSE;
	mOverrideAspectRatio = 0.f;
	mMaximized = FALSE;
	mMinimized = FALSE;
	mLanguageTextInputAllowed = FALSE;
	mPreeditor = NULL;
	mFSAASamples = fsaa_samples;
	mForceRebuild = FALSE;

	// For reasons that aren't clear to me, LLTimers seem to be created in the "started" state.
	// Since the started state of this one is used to track whether the NMRec has been installed, it wants to start out in the "stopped" state.
	mBounceTimer.stop();

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
	LL_INFOS("Window") << "Creating context..." << LL_ENDL;
	if(createContext(x, y, width, height, 32, fullscreen, disable_vsync))
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
	}

	mCallbacks = callbacks;
	stop_glerror();
	
	
}

// These functions are used as wrappers for our internal event handling callbacks.
// It's a good idea to wrap these to avoid reworking more code than we need to within LLWindow.

void callKeyUp(unsigned short key, unsigned int mask)
{
	gKeyboard->handleKeyUp(key, mask);
}

void callKeyDown(unsigned short key, unsigned int mask)
{
	gKeyboard->handleKeyDown(key, mask);
}

void callUnicodeCallback(wchar_t character, unsigned int mask)
{
	gWindowImplementation->getCallbacks()->handleUnicodeChar(character, mask);
}

void callFocus()
{
	gWindowImplementation->getCallbacks()->handleFocus(gWindowImplementation);
}

void callFocusLost()
{
	gWindowImplementation->getCallbacks()->handleFocusLost(gWindowImplementation);
}

void callRightMouseDown(float *pos, MASK mask)
{
	LLCoordGL		outCoords;
	outCoords.mX = llround(pos[0]);
	outCoords.mY = llround(pos[1]);
	gWindowImplementation->getCallbacks()->handleRightMouseDown(gWindowImplementation, outCoords, gKeyboard->currentMask(TRUE));
}

void callRightMouseUp(float *pos, MASK mask)
{
	LLCoordGL		outCoords;
	outCoords.mX = llround(pos[0]);
	outCoords.mY = llround(pos[1]);
	gWindowImplementation->getCallbacks()->handleRightMouseUp(gWindowImplementation, outCoords, gKeyboard->currentMask(TRUE));
}

void callLeftMouseDown(float *pos, MASK mask)
{
	LLCoordGL		outCoords;
	outCoords.mX = llround(pos[0]);
	outCoords.mY = llround(pos[1]);
	gWindowImplementation->getCallbacks()->handleMouseDown(gWindowImplementation, outCoords, gKeyboard->currentMask(TRUE));
	LL_INFOS("Window") << outCoords.mX << ", " << outCoords.mY << LL_ENDL;
}

void callLeftMouseUp(float *pos, MASK mask)
{
	LLCoordGL		outCoords;
	outCoords.mX = llround(pos[0]);
	outCoords.mY = llround(pos[1]);
	gWindowImplementation->getCallbacks()->handleMouseUp(gWindowImplementation, outCoords, gKeyboard->currentMask(TRUE));
	
}

void callDoubleClick(float *pos, MASK mask)
{
	LLCoordGL	outCoords;
	outCoords.mX = llround(pos[0]);
	outCoords.mY = llround(pos[1]);
	gWindowImplementation->getCallbacks()->handleDoubleClick(gWindowImplementation, outCoords, gKeyboard->currentMask(TRUE));
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
	LLCoordGL		outCoords;
	outCoords.mX = llround(pos[0]);
	outCoords.mY = llround(pos[1]);
	float deltas[2];
	gWindowImplementation->getMouseDeltas(deltas);
	outCoords.mX += deltas[0];
	outCoords.mY += deltas[1];
	gWindowImplementation->getCallbacks()->handleMouseMove(gWindowImplementation, outCoords, gKeyboard->currentMask(TRUE));
	
}

void callScrollMoved(float delta)
{
	gWindowImplementation->getCallbacks()->handleScrollWheel(gWindowImplementation, delta);
}

void callMouseExit()
{
	gWindowImplementation->getCallbacks()->handleMouseLeave(gWindowImplementation);
}

void callWindowFocus()
{
	gWindowImplementation->getCallbacks()->handleFocus(gWindowImplementation);
}

void callWindowUnfocus()
{
	gWindowImplementation->getCallbacks()->handleFocusLost(gWindowImplementation);
}

void callDeltaUpdate(float *delta, MASK mask)
{
	gWindowImplementation->updateMouseDeltas(delta);
}

void callMiddleMouseDown(float *pos, MASK mask)
{
	LLCoordGL		outCoords;
	outCoords.mX = llround(pos[0]);
	outCoords.mY = llround(pos[1]);
	float deltas[2];
	gWindowImplementation->getMouseDeltas(deltas);
	outCoords.mX += deltas[0];
	outCoords.mY += deltas[1];
	gWindowImplementation->getCallbacks()->handleMiddleMouseDown(gWindowImplementation, outCoords, mask);
}

void callMiddleMouseUp(float *pos, MASK mask)
{
	LLCoordGL outCoords;
	outCoords.mX = llround(pos[0]);
	outCoords.mY = llround(pos[1]);
	float deltas[2];
	gWindowImplementation->getMouseDeltas(deltas);
	outCoords.mX += deltas[0];
	outCoords.mY += deltas[1];
	gWindowImplementation->getCallbacks()->handleMiddleMouseUp(gWindowImplementation, outCoords, mask);
}

void LLWindowMacOSX::updateMouseDeltas(float* deltas)
{
	if (mCursorDecoupled)
	{
		mCursorLastEventDeltaX = llround(deltas[0]);
		mCursorLastEventDeltaY = llround(deltas[1]);
		
		if (mCursorIgnoreNextDelta)
		{
			mCursorLastEventDeltaX = 0;
			mCursorLastEventDeltaY = 0;
			mCursorIgnoreNextDelta = FALSE;
		}
		LL_INFOS("Delta Update") << "Last event delta: " << mCursorLastEventDeltaX << ", " << mCursorLastEventDeltaY << LL_ENDL;
	} else {
		mCursorLastEventDeltaX = 0;
		mCursorLastEventDeltaY = 0;
	}
}

void LLWindowMacOSX::getMouseDeltas(float* delta)
{
	delta[0] = mCursorLastEventDeltaX;
	delta[1] = mCursorLastEventDeltaY;
}

BOOL LLWindowMacOSX::createContext(int x, int y, int width, int height, int bits, BOOL fullscreen, BOOL disable_vsync)
{
	BOOL			glNeedsInit = FALSE;

	mFullscreen = fullscreen;
	
	if (mWindow == NULL)
	{
		LL_INFOS("Window") << "Creating window..." << LL_ENDL;
		mWindow = getMainAppWindow();
	}

	if(mContext == NULL)
	{
		LL_INFOS("Window") << "Creating GL view..." << LL_ENDL;
		// Our OpenGL view is already defined within SecondLife.xib.
		// Get the view instead.
		mGLView = createOpenGLView(mWindow);
		mContext = getCGLContextObj(mGLView);
		// Since we just created the context, it needs to be set up.
		glNeedsInit = TRUE;
	}

	// Hook up the context to a drawable

	if(mContext != NULL)
	{
		LL_INFOS("Window") << "Setting CGL Context..." << LL_ENDL;
		LL_DEBUGS("Window") << "createContext: setting current context" << LL_ENDL;
		U32 err = CGLSetCurrentContext(mContext);
		if (err != kCGLNoError)
		{
			setupFailure("Can't activate GL rendering context", "Error", OSMB_OK);
			return FALSE;
		}
	}

	// Disable vertical sync for swap
	GLint frames_per_swap = 0;
	if (disable_vsync)
	{
		LL_DEBUGS("GLInit") << "Disabling vertical sync" << LL_ENDL;
		frames_per_swap = 0;
	}
	else
	{
		LL_DEBUGS("GLinit") << "Keeping vertical sync" << LL_ENDL;
		frames_per_swap = 1;
	}
	
	CGLSetParameter(mContext, kCGLCPSwapInterval, &frames_per_swap);

	//enable multi-threaded OpenGL
	if (sUseMultGL)
	{
		CGLError cgl_err;
		CGLContextObj ctx = CGLGetCurrentContext();

		cgl_err =  CGLEnable( ctx, kCGLCEMPEngine);

		if (cgl_err != kCGLNoError )
		{
			LL_DEBUGS("GLInit") << "Multi-threaded OpenGL not available." << LL_ENDL;
		}
		else
		{
			LL_DEBUGS("GLInit") << "Multi-threaded OpenGL enabled." << LL_ENDL;
		}
	}
	LL_INFOS("Window") << "Completed context creation." << LL_ENDL;
	// Don't need to get the current gamma, since there's a call that restores it to the system defaults.
	return TRUE;
}


// We only support OS X 10.7's fullscreen app mode which is literally a full screen window that fills a virtual desktop.
// This makes this method obsolete.
BOOL LLWindowMacOSX::switchContext(BOOL fullscreen, const LLCoordScreen &size, BOOL disable_vsync, const LLCoordScreen * const posp)
{
	return FALSE;
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
		closeWindow(mWindow);
		mWindow = NULL;
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
	setMouseClipping(FALSE);
}

//virtual
void LLWindowMacOSX::minimize()
{
	setMouseClipping(FALSE);
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
	//	if (!mWindow)
	//	{
	//		return;
	//	}

	// Make sure cursor is visible and we haven't mangled the clipping state.
	setMouseClipping(FALSE);
	showCursor();

	destroyContext();
}

BOOL LLWindowMacOSX::isValid()
{
	if(mFullscreen)
	{
		return(TRUE);
	}

	return (mWindow != NULL);
}

BOOL LLWindowMacOSX::getVisible()
{
	BOOL result = FALSE;

	if(mFullscreen)
	{
		result = TRUE;
	}if (mWindow)
	{
			result = TRUE;
	}

	return(result);
}

BOOL LLWindowMacOSX::getMinimized()
{
	return mMinimized;
}

BOOL LLWindowMacOSX::getMaximized()
{
	return mMaximized;
}

BOOL LLWindowMacOSX::maximize()
{
	if (mWindow && !mMaximized)
	{
	}

	return mMaximized;
}

BOOL LLWindowMacOSX::getFullscreen()
{
	return mFullscreen;
}

void LLWindowMacOSX::gatherInput()
{
	// stop bouncing icon after fixed period of time
	if (mBounceTimer.getStarted() && mBounceTimer.getElapsedTimeF32() > mBounceTime)
	{
		stopDockTileBounce();
	}
	
	updateCursor();
}

BOOL LLWindowMacOSX::getPosition(LLCoordScreen *position)
{
	float rect[4];
	S32 err = -1;

	if(mFullscreen)
	{
		position->mX = 0;
		position->mY = 0;
		err = noErr;
	}
	else if(mWindow)
	{
		getContentViewBounds(mWindow, rect);

		position->mX = rect[0];
		position->mY = rect[1];
	}
	else
	{
		llerrs << "LLWindowMacOSX::getPosition(): no window and not fullscreen!" << llendl;
	}

	return (err == noErr);
}

BOOL LLWindowMacOSX::getSize(LLCoordScreen *size)
{
	float rect[4];
	S32 err = -1;

	if(mFullscreen)
	{
		size->mX = mFullscreenWidth;
		size->mY = mFullscreenHeight;
		err = noErr;
	}
	else if(mWindow)
	{
		getContentViewBounds(mWindow, rect);

		size->mX = rect[2];
		size->mY = rect[3];
	}
	else
	{
		llerrs << "LLWindowMacOSX::getPosition(): no window and not fullscreen!" << llendl;
	}

	return (err == noErr);
}

BOOL LLWindowMacOSX::getSize(LLCoordWindow *size)
{
	float rect[4];
	S32 err = -1;
	
	if(mFullscreen)
	{
		size->mX = mFullscreenWidth;
		size->mY = mFullscreenHeight;
		err = noErr;
	}
	else if(mWindow)
	{
		getContentViewBounds(mWindow, rect);
		
		size->mX = rect[2];
		size->mY = rect[3];
	}
	else
	{
		llerrs << "LLWindowMacOSX::getPosition(): no window and not fullscreen!" << llendl;
	}
	
	return (err == noErr);
}

BOOL LLWindowMacOSX::setPosition(const LLCoordScreen position)
{
	if(mWindow)
	{
		float pos[2] = {position.mX, position.mY};
		setWindowPos(mWindow, pos);
	}

	return TRUE;
}

BOOL LLWindowMacOSX::setSizeImpl(const LLCoordScreen size)
{
	if(mWindow)
	{
		setWindowSize(mWindow, size.mX, size.mY);
	}

	return TRUE;
}

BOOL LLWindowMacOSX::setSizeImpl(const LLCoordWindow size)
{
	float client_rect[4];
	if (mWindow)
	{
		S32 err = noErr;
		getContentViewBounds(mWindow, client_rect);
		if (err == noErr)
		{
			client_rect[2] += size.mX;
			client_rect[3] += size.mY;
			setWindowSize(mWindow, client_rect[2], client_rect[3]);
		}
		if (err == noErr)
		{
			return TRUE;
		}
		else
		{
			llinfos << "Error setting size" << err << llendl;
			return FALSE;
		}
	}
	return FALSE;
}

void LLWindowMacOSX::swapBuffers()
{
	CGLFlushDrawable(mContext);
}

F32 LLWindowMacOSX::getGamma()
{
	F32 result = 1.8;	// Default to something sane

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
		&blueGamma) == noErr)
	{
		// So many choices...
		// Let's just return the green channel gamma for now.
		result = greenGamma;
	}

	return result;
}

U32 LLWindowMacOSX::getFSAASamples()
{
	return mFSAASamples;
}

void LLWindowMacOSX::setFSAASamples(const U32 samples)
{
	mFSAASamples = samples;
	mForceRebuild = TRUE;
}

BOOL LLWindowMacOSX::restoreGamma()
{
	CGDisplayRestoreColorSyncSettings();
	return true;
}

BOOL LLWindowMacOSX::setGamma(const F32 gamma)
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
		&blueGamma) != noErr)
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
		gamma) != noErr)
	{
		return false;
	}


	return true;
}

BOOL LLWindowMacOSX::isCursorHidden()
{
	return mCursorHidden;
}



// Constrains the mouse to the window.
void LLWindowMacOSX::setMouseClipping( BOOL b )
{
	// Just stash the requested state.  We'll simulate this when the cursor is hidden by decoupling.
	mIsMouseClipping = b;

	if(b)
	{
		//		llinfos << "setMouseClipping(TRUE)" << llendl;
	}
	else
	{
		//		llinfos << "setMouseClipping(FALSE)" << llendl;
	}

	adjustCursorDecouple();
}

BOOL LLWindowMacOSX::setCursorPosition(const LLCoordWindow position)
{
	BOOL result = FALSE;
	LLCoordScreen screen_pos;

	if (!convertCoords(position, &screen_pos))
	{
		return FALSE;
	}

	CGPoint newPosition;

	//	llinfos << "setCursorPosition(" << screen_pos.mX << ", " << screen_pos.mY << ")" << llendl;

	newPosition.x = screen_pos.mX;
	newPosition.y = screen_pos.mY;

	CGSetLocalEventsSuppressionInterval(0.0);
	if(CGWarpMouseCursorPosition(newPosition) == noErr)
	{
		result = TRUE;
	}

	// Under certain circumstances, this will trigger us to decouple the cursor.
	adjustCursorDecouple(true);

	// trigger mouse move callback
	LLCoordGL gl_pos;
	convertCoords(position, &gl_pos);
	mCallbacks->handleMouseMove(this, gl_pos, (MASK)0);

	return result;
}
/*
static void fixOrigin(void)
{
	GrafPtr port;
	Rect portrect;

	::GetPort(&port);
	::GetPortBounds(port, &portrect);
	if((portrect.left != 0) || (portrect.top != 0))
	{
		// Mozilla sometimes changes our port origin.
		::SetOrigin(0,0);
	}
}
*/
BOOL LLWindowMacOSX::getCursorPosition(LLCoordWindow *position)
{
	float cursor_point[2];
	LLCoordScreen screen_pos;

	if(mWindow == NULL)
		return FALSE;
	
	getCursorPos(mWindow, cursor_point);

	if(mCursorDecoupled)
	{
		//		CGMouseDelta x, y;

		// If the cursor's decoupled, we need to read the latest movement delta as well.
		//		CGGetLastMouseDelta( &x, &y );
		//		cursor_point.h += x;
		//		cursor_point.v += y;

		// CGGetLastMouseDelta may behave strangely when the cursor's first captured.
		// Stash in the event handler instead.
		cursor_point[0] += mCursorLastEventDeltaX;
		cursor_point[1] += mCursorLastEventDeltaY;
	}

	position->mX = cursor_point[0];
	position->mY = cursor_point[1];

	return TRUE;
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
				//			llinfos << "adjustCursorDecouple: decoupling cursor" << llendl;
				CGAssociateMouseAndMouseCursorPosition(false);
				mCursorDecoupled = true;
				mCursorIgnoreNextDelta = TRUE;
			}
		}
	}
	else
	{
		// The cursor should not be decoupled.  Make sure it isn't.
		if(mCursorDecoupled)
		{
			//			llinfos << "adjustCursorDecouple: recoupling cursor" << llendl;
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
	//OS X always enforces a 1:1 pixel aspect ratio, regardless of video mode
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
}


void LLWindowMacOSX::flashIcon(F32 seconds)
{
	// Don't do this if we're already started, since this would try to install the NMRec twice.
	if(!mBounceTimer.getStarted())
	{
		S32 err = 0;
		// TODO: Implement icon bouncing
		mBounceTime = seconds;
		if(err == 0)
		{
			mBounceTimer.start();
		}
		else
		{
			// This is very not-fatal (only problem is the icon will not bounce), but we'd like to find out about it somehow...
			llinfos << "NMInstall failed with error code " << err << llendl;
		}
	}
}

BOOL LLWindowMacOSX::isClipboardTextAvailable()
{
	BOOL result = false;
	// TODO: Clipboard support.
	return result;
}

BOOL LLWindowMacOSX::pasteTextFromClipboard(LLWString &dst)
{
	BOOL result = false;
	
	// TODO: Clipboard support.

	return result;
}

BOOL LLWindowMacOSX::copyTextToClipboard(const LLWString &s)
{
	BOOL result = false;
	
	// TODO: Clipboard support.

	return result;
}


// protected
BOOL LLWindowMacOSX::resetDisplayResolution()
{
	// This is only called from elsewhere in this class, and it's not used by the Mac implementation.
	return true;
}


LLWindow::LLWindowResolution* LLWindowMacOSX::getSupportedResolutions(S32 &num_resolutions)
{
	if (!mSupportedResolutions)
	{
		CFArrayRef modes = CGDisplayAvailableModes(mDisplay);

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
					BOOL resolution_exists = FALSE;
					for(S32 i = 0; i < mNumSupportedResolutions; i++)
					{
						if (mSupportedResolutions[i].mWidth == width &&
							mSupportedResolutions[i].mHeight == height)
						{
							resolution_exists = TRUE;
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
		}
	}

	num_resolutions = mNumSupportedResolutions;
	return mSupportedResolutions;
}

BOOL LLWindowMacOSX::convertCoords(LLCoordGL from, LLCoordWindow *to)
{
	S32		client_height;
	float	client_rect[4];
	getContentViewBounds(mWindow, client_rect);
	if (!mWindow ||
		NULL == to)
	{
		return FALSE;
	}

	to->mX = from.mX;
	client_height = client_rect[3];
	to->mY = from.mY - 1;

	return TRUE;
}

BOOL LLWindowMacOSX::convertCoords(LLCoordWindow from, LLCoordGL* to)
{
	S32		client_height;
	float	client_rect[4];
	
	getContentViewBounds(mWindow, client_rect);
	
	if (!mWindow ||
		NULL == to)
	{
		return FALSE;
	}

	to->mX = from.mX;
	client_height = client_rect[3];
	to->mY = from.mY - 1;

	return TRUE;
}

BOOL LLWindowMacOSX::convertCoords(LLCoordScreen from, LLCoordWindow* to)
{
	if(mFullscreen)
	{
		// In the fullscreen case, window and screen coordinates are the same.
		to->mX = from.mX;
		to->mY = from.mY;
		return TRUE;
	}
	else if(mWindow)
	{
		float mouse_point[2];

		mouse_point[0] = from.mX;
		mouse_point[1] = from.mY;
		
		convertScreenToWindow(mWindow, mouse_point);

		to->mX = mouse_point[0];
		to->mY = mouse_point[1];

		return TRUE;
	}

	return FALSE;
}

BOOL LLWindowMacOSX::convertCoords(LLCoordWindow from, LLCoordScreen *to)
{
	if(mFullscreen)
	{
		// In the fullscreen case, window and screen coordinates are the same.
		to->mX = from.mX;
		to->mY = from.mY;
		return TRUE;
	}
	else if(mWindow)
	{
		float mouse_point[2];

		mouse_point[0] = from.mX;
		mouse_point[1] = from.mY;
		convertWindowToScreen(mWindow, mouse_point);

		to->mX = mouse_point[0];
		to->mY = mouse_point[1];

		return TRUE;
	}

	return FALSE;
}

BOOL LLWindowMacOSX::convertCoords(LLCoordScreen from, LLCoordGL *to)
{
	LLCoordWindow window_coord;

	return(convertCoords(from, &window_coord) && convertCoords(window_coord, to));
}

BOOL LLWindowMacOSX::convertCoords(LLCoordGL from, LLCoordScreen *to)
{
	LLCoordWindow window_coord;

	return(convertCoords(from, &window_coord) && convertCoords(window_coord, to));
}




void LLWindowMacOSX::setupFailure(const std::string& text, const std::string& caption, U32 type)
{
	destroyContext();

	OSMessageBox(text, caption, type);
}

const char* cursorIDToName(int id)
{
	switch (id)
	{
		case UI_CURSOR_ARROW:							return "UI_CURSOR_ARROW";
		case UI_CURSOR_WAIT:							return "UI_CURSOR_WAIT";
		case UI_CURSOR_HAND:							return "UI_CURSOR_HAND";
		case UI_CURSOR_IBEAM:							return "UI_CURSOR_IBEAM";
		case UI_CURSOR_CROSS:							return "UI_CURSOR_CROSS";
		case UI_CURSOR_SIZENWSE:						return "UI_CURSOR_SIZENWSE";
		case UI_CURSOR_SIZENESW:						return "UI_CURSOR_SIZENESW";
		case UI_CURSOR_SIZEWE:							return "UI_CURSOR_SIZEWE";
		case UI_CURSOR_SIZENS:							return "UI_CURSOR_SIZENS";
		case UI_CURSOR_NO:								return "UI_CURSOR_NO";
		case UI_CURSOR_WORKING:							return "UI_CURSOR_WORKING";
		case UI_CURSOR_TOOLGRAB:						return "UI_CURSOR_TOOLGRAB";
		case UI_CURSOR_TOOLLAND:						return "UI_CURSOR_TOOLLAND";
		case UI_CURSOR_TOOLFOCUS:						return "UI_CURSOR_TOOLFOCUS";
		case UI_CURSOR_TOOLCREATE:						return "UI_CURSOR_TOOLCREATE";
		case UI_CURSOR_ARROWDRAG:						return "UI_CURSOR_ARROWDRAG";
		case UI_CURSOR_ARROWCOPY:						return "UI_CURSOR_ARROWCOPY";
		case UI_CURSOR_ARROWDRAGMULTI:					return "UI_CURSOR_ARROWDRAGMULTI";
		case UI_CURSOR_ARROWCOPYMULTI:					return "UI_CURSOR_ARROWCOPYMULTI";
		case UI_CURSOR_NOLOCKED:						return "UI_CURSOR_NOLOCKED";
		case UI_CURSOR_ARROWLOCKED:						return "UI_CURSOR_ARROWLOCKED";
		case UI_CURSOR_GRABLOCKED:						return "UI_CURSOR_GRABLOCKED";
		case UI_CURSOR_TOOLTRANSLATE:					return "UI_CURSOR_TOOLTRANSLATE";
		case UI_CURSOR_TOOLROTATE:						return "UI_CURSOR_TOOLROTATE";
		case UI_CURSOR_TOOLSCALE:						return "UI_CURSOR_TOOLSCALE";
		case UI_CURSOR_TOOLCAMERA:						return "UI_CURSOR_TOOLCAMERA";
		case UI_CURSOR_TOOLPAN:							return "UI_CURSOR_TOOLPAN";
		case UI_CURSOR_TOOLZOOMIN:						return "UI_CURSOR_TOOLZOOMIN";
		case UI_CURSOR_TOOLPICKOBJECT3:					return "UI_CURSOR_TOOLPICKOBJECT3";
		case UI_CURSOR_TOOLPLAY:						return "UI_CURSOR_TOOLPLAY";
		case UI_CURSOR_TOOLPAUSE:						return "UI_CURSOR_TOOLPAUSE";
		case UI_CURSOR_TOOLMEDIAOPEN:					return "UI_CURSOR_TOOLMEDIAOPEN";
		case UI_CURSOR_PIPETTE:							return "UI_CURSOR_PIPETTE";
		case UI_CURSOR_TOOLSIT:							return "UI_CURSOR_TOOLSIT";
		case UI_CURSOR_TOOLBUY:							return "UI_CURSOR_TOOLBUY";
		case UI_CURSOR_TOOLOPEN:						return "UI_CURSOR_TOOLOPEN";
		case UI_CURSOR_TOOLPATHFINDING:					return "UI_CURSOR_PATHFINDING";
		case UI_CURSOR_TOOLPATHFINDING_PATH_START:		return "UI_CURSOR_PATHFINDING_START";
		case UI_CURSOR_TOOLPATHFINDING_PATH_START_ADD:	return "UI_CURSOR_PATHFINDING_START_ADD";
		case UI_CURSOR_TOOLPATHFINDING_PATH_END:		return "UI_CURSOR_PATHFINDING_END";
		case UI_CURSOR_TOOLPATHFINDING_PATH_END_ADD:	return "UI_CURSOR_PATHFINDING_END_ADD";
		case UI_CURSOR_TOOLNO:							return "UI_CURSOR_NO";
	}

	llerrs << "cursorIDToName: unknown cursor id" << id << llendl;

	return "UI_CURSOR_ARROW";
}

static CursorRef gCursors[UI_CURSOR_COUNT];


static void initPixmapCursor(int cursorid, int hotspotX, int hotspotY)
{
	// cursors are in <Application Bundle>/Contents/Resources/cursors_mac/UI_CURSOR_FOO.tif
	std::string fullpath = gDirUtilp->getAppRODataDir();
	fullpath += gDirUtilp->getDirDelimiter();
	fullpath += "cursors_mac";
	fullpath += gDirUtilp->getDirDelimiter();
	fullpath += cursorIDToName(cursorid);
	fullpath += ".tif";

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
		return;

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
		InitCursor();
		if(mCursorHidden)
		{
			// Since InitCursor resets the hide level, correct for it here.
			hideNSCursor();
		}
		break;

		// MBW -- XXX -- Some of the standard Windows cursors have no standard Mac equivalents.
		//    Find out what they look like and replicate them.

		// These are essentially correct
	case UI_CURSOR_WAIT:		/* Apple purposely doesn't allow us to set the beachball cursor manually. */	break;
	case UI_CURSOR_IBEAM:		setIBeamCursor();	break;
	case UI_CURSOR_CROSS:		setCrossCursor();	break;
	case UI_CURSOR_HAND:		setPointingHandCursor();	break;
		//		case UI_CURSOR_NO:			SetThemeCursor(kThemeNotAllowedCursor);	break;
	case UI_CURSOR_ARROWCOPY:   setCopyCursor();	break;

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
		InitCursor();
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
	initPixmapCursor(UI_CURSOR_TOOLTRANSLATE, 1, 1);
	initPixmapCursor(UI_CURSOR_TOOLROTATE, 1, 1);
	initPixmapCursor(UI_CURSOR_TOOLSCALE, 1, 1);
	initPixmapCursor(UI_CURSOR_TOOLCAMERA, 7, 6);
	initPixmapCursor(UI_CURSOR_TOOLPAN, 7, 6);
	initPixmapCursor(UI_CURSOR_TOOLZOOMIN, 7, 6);
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
		//		llinfos << "hideCursor: hiding" << llendl;
		mCursorHidden = TRUE;
		mHideCursorPermanent = TRUE;
		hideNSCursor();
	}
	else
	{
		//		llinfos << "hideCursor: already hidden" << llendl;
	}

	adjustCursorDecouple();
}

void LLWindowMacOSX::showCursor()
{
	if(mCursorHidden)
	{
		//		llinfos << "showCursor: showing" << llendl;
		mCursorHidden = FALSE;
		mHideCursorPermanent = FALSE;
		showNSCursor();
	}
	else
	{
		//		llinfos << "showCursor: already visible" << llendl;
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
		mHideCursorPermanent = FALSE;
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
#if 0
	IBNibRef nib = NULL;
	S32 err;

	err = CreateNibReference(CFSTR("SecondLife"), &nib);

	if(err == noErr)
	{
		CreateWindowFromNib(nib, CFSTR("Splash Screen"), &mWindow);

		DisposeNibReference(nib);
	}

	if(mWindow != NULL)
	{
		ShowWindow(mWindow);
	}
#endif
}

void LLSplashScreenMacOSX::updateImpl(const std::string& mesg)
{
	if(mWindow != NULL)
	{
		CFStringRef string = NULL;

		string = CFStringCreateWithCString(NULL, mesg.c_str(), kCFStringEncodingUTF8);
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
	// TODO: Implement a native NSAlert function that replicates all of this.
	return 0;
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
		llwarns << "spawn_web_browser called for url with protocol not on whitelist: " << escaped_url << llendl;
		return;
	}

	S32 result = 0;
	CFURLRef urlRef = NULL;

	llinfos << "Opening URL " << escaped_url << llendl;

	CFStringRef	stringRef = CFStringCreateWithCString(NULL, escaped_url.c_str(), kCFStringEncodingUTF8);
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
			llinfos << "Error " << result << " on open." << llendl;
		}

		CFRelease(urlRef);
	}
	else
	{
		llinfos << "Error: couldn't create URL." << llendl;
	}
}

LLSD LLWindowMacOSX::getNativeKeyData()
{
	LLSD result = LLSD::emptyMap();
#if 0
	if(mRawKeyEvent)
	{
		char char_code = 0;
		UInt32 key_code = 0;
		UInt32 modifiers = 0;
		UInt32 keyboard_type = 0;

		GetEventParameter (mRawKeyEvent, kEventParamKeyMacCharCodes, typeChar, NULL, sizeof(char), NULL, &char_code);
		GetEventParameter (mRawKeyEvent, kEventParamKeyCode, typeUInt32, NULL, sizeof(UInt32), NULL, &key_code);
		GetEventParameter (mRawKeyEvent, kEventParamKeyModifiers, typeUInt32, NULL, sizeof(UInt32), NULL, &modifiers);
		GetEventParameter (mRawKeyEvent, kEventParamKeyboardType, typeUInt32, NULL, sizeof(UInt32), NULL, &keyboard_type);

		result["char_code"] = (S32)char_code;
		result["key_code"] = (S32)key_code;
		result["modifiers"] = (S32)modifiers;
		result["keyboard_type"] = (S32)keyboard_type;

#if 0
		// This causes trouble for control characters -- apparently character codes less than 32 (escape, control-A, etc)
		// cause llsd serialization to create XML that the llsd deserializer won't parse!
		std::string unicode;
		S32 err = noErr;
		EventParamType actualType = typeUTF8Text;
		UInt32 actualSize = 0;
		char *buffer = NULL;

		err = GetEventParameter (mRawKeyEvent, kEventParamKeyUnicodes, typeUTF8Text, &actualType, 0, &actualSize, NULL);
		if(err == noErr)
		{
			// allocate a buffer and get the actual data.
			buffer = new char[actualSize];
			err = GetEventParameter (mRawKeyEvent, kEventParamKeyUnicodes, typeUTF8Text, &actualType, actualSize, &actualSize, buffer);
			if(err == noErr)
			{
				unicode.assign(buffer, actualSize);
			}
			delete[] buffer;
		}

		result["unicode"] = unicode;
#endif

	}
#endif

	lldebugs << "native key data is: " << result << llendl;

	return result;
}


BOOL LLWindowMacOSX::dialogColorPicker( F32 *r, F32 *g, F32 *b)
{
	// Is this even used anywhere?  Do we really need an OS color picker?
	BOOL	retval = FALSE;
	//S32		error = 0;
	return (retval);
}


void *LLWindowMacOSX::getPlatformWindow()
{
	// NOTE: this will be NULL in fullscreen mode.  Plan accordingly.
	return (void*)mWindow;
}

void LLWindowMacOSX::stopDockTileBounce()
{
	mBounceTimer.stop();
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

void LLWindowMacOSX::allowLanguageTextInput(LLPreeditor *preeditor, BOOL b)
{
	// TODO: IME support
}

void LLWindowMacOSX::interruptLanguageTextInput()
{
	// TODO: IME support
}

//static
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

#if LL_OS_DRAGDROP_ENABLED
/*
S16 LLWindowMacOSX::dragTrackingHandler(DragTrackingMessage message, WindowRef theWindow,
						  void * handlerRefCon, DragRef drag)
{
	S16 result = 0;
	LLWindowMacOSX *self = (LLWindowMacOSX*)handlerRefCon;

	lldebugs << "drag tracking handler, message = " << message << llendl;

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

OSErr LLWindowMacOSX::handleDragNDrop(DragRef drag, LLWindowCallbacks::DragNDropAction action)
{
	OSErr result = dragNotAcceptedErr;	// overall function result
	OSErr err = noErr;	// for local error handling

	// Get the mouse position and modifiers of this drag.
	SInt16 modifiers, mouseDownModifiers, mouseUpModifiers;
	::GetDragModifiers(drag, &modifiers, &mouseDownModifiers, &mouseUpModifiers);
	MASK mask = LLWindowMacOSX::modifiersToMask(modifiers);

	Point mouse_point;
	// This will return the mouse point in global screen coords
	::GetDragMouse(drag, &mouse_point, NULL);
	LLCoordScreen screen_coords(mouse_point.h, mouse_point.v);
	LLCoordGL gl_pos;
	convertCoords(screen_coords, &gl_pos);

	// Look at the pasteboard and try to extract an URL from it
	PasteboardRef   pasteboard;
	if(GetDragPasteboard(drag, &pasteboard) == noErr)
	{
		ItemCount num_items = 0;
		// Treat an error here as an item count of 0
		(void)PasteboardGetItemCount(pasteboard, &num_items);

		// Only deal with single-item drags.
		if(num_items == 1)
		{
			PasteboardItemID item_id = NULL;
			CFArrayRef flavors = NULL;
			CFDataRef data = NULL;

			err = PasteboardGetItemIdentifier(pasteboard, 1, &item_id); // Yes, this really is 1-based.

			// Try to extract an URL from the pasteboard
			if(err == noErr)
			{
				err = PasteboardCopyItemFlavors( pasteboard, item_id, &flavors);
			}

			if(err == noErr)
			{
				if(CFArrayContainsValue(flavors, CFRangeMake(0, CFArrayGetCount(flavors)), kUTTypeURL))
				{
					// This is an URL.
					err = PasteboardCopyItemFlavorData(pasteboard, item_id, kUTTypeURL, &data);
				}
				else if(CFArrayContainsValue(flavors, CFRangeMake(0, CFArrayGetCount(flavors)), kUTTypeUTF8PlainText))
				{
					// This is a string that might be an URL.
					err = PasteboardCopyItemFlavorData(pasteboard, item_id, kUTTypeUTF8PlainText, &data);
				}

			}

			if(flavors != NULL)
			{
				CFRelease(flavors);
			}

			if(data != NULL)
			{
				std::string url;
				url.assign((char*)CFDataGetBytePtr(data), CFDataGetLength(data));
				CFRelease(data);

				if(!url.empty())
				{
					LLWindowCallbacks::DragNDropResult res =
						mCallbacks->handleDragNDrop(this, gl_pos, mask, action, url);

					switch (res) {
						case LLWindowCallbacks::DND_NONE:		// No drop allowed
							if (action == LLWindowCallbacks::DNDA_TRACK)
							{
								mDragOverrideCursor = kThemeNotAllowedCursor;
							}
							else {
								mDragOverrideCursor = -1;
							}
							break;
						case LLWindowCallbacks::DND_MOVE:		// Drop accepted would result in a "move" operation
							mDragOverrideCursor = kThemePointingHandCursor;
							result = noErr;
							break;
						case LLWindowCallbacks::DND_COPY:		// Drop accepted would result in a "copy" operation
							mDragOverrideCursor = kThemeCopyArrowCursor;
							result = noErr;
							break;
						case LLWindowCallbacks::DND_LINK:		// Drop accepted would result in a "link" operation:
							mDragOverrideCursor = kThemeAliasArrowCursor;
							result = noErr;
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
						SetThemeCursor(mDragOverrideCursor);
					}

				}
			}
		}
	}

	return result;
}
*/
#endif // LL_OS_DRAGDROP_ENABLED
