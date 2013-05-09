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
#include "llwindowmacosx-objc.h"
#include "llpreeditor.h"

#include "llerror.h"
#include "llgl.h"
#include "llstring.h"
#include "lldir.h"
#include "indra_constants.h"

#include <Carbon/Carbon.h>
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
WindowRef LLWindowMacOSX::sMediaWindow = NULL;

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
static double getDictDouble (CFDictionaryRef refDict, CFStringRef key);
static long getDictLong (CFDictionaryRef refDict, CFStringRef key);




// CarbonEvents we're interested in.
static EventTypeSpec WindowHandlerEventList[] =
{
	// Window-related events
	{ kEventClassWindow, kEventWindowActivated },
	{ kEventClassWindow, kEventWindowDeactivated },
	{ kEventClassWindow, kEventWindowShown },
	{ kEventClassWindow, kEventWindowHidden },
	{ kEventClassWindow, kEventWindowCollapsed },
	{ kEventClassWindow, kEventWindowExpanded },
	{ kEventClassWindow, kEventWindowGetClickActivation },
	{ kEventClassWindow, kEventWindowClose },
	{ kEventClassWindow, kEventWindowBoundsChanging },
	{ kEventClassWindow, kEventWindowBoundsChanged },
	{ kEventClassWindow, kEventWindowGetIdealSize },

	// Mouse events
	{ kEventClassMouse, kEventMouseDown },
	{ kEventClassMouse, kEventMouseUp },
	{ kEventClassMouse, kEventMouseDragged },
	{ kEventClassMouse, kEventMouseWheelMoved },
	{ kEventClassMouse, kEventMouseMoved },

	// Keyboard events
	// No longer handle raw key down events directly.
	// When text input events come in, extract the raw key events from them and process at that point.
	// This allows input methods to eat keystrokes the way they're supposed to.
//	{ kEventClassKeyboard, kEventRawKeyDown },
//	{ kEventClassKeyboard, kEventRawKeyRepeat },
	{ kEventClassKeyboard, kEventRawKeyUp },
	{ kEventClassKeyboard, kEventRawKeyModifiersChanged },

	// Text input events
	{ kEventClassTextInput, kEventTextInputUnicodeForKeyEvent },
	{ kEventClassTextInput, kEventTextInputUpdateActiveInputArea },
	{ kEventClassTextInput, kEventTextInputOffsetToPos },
	{ kEventClassTextInput, kEventTextInputPosToOffset },
	{ kEventClassTextInput, kEventTextInputShowHideBottomWindow },
	{ kEventClassTextInput, kEventTextInputGetSelectedText },
	{ kEventClassTextInput, kEventTextInputFilterText },

	// TSM Document Access events (advanced input method support)
	{ kEventClassTSMDocumentAccess, kEventTSMDocumentAccessGetLength },
	{ kEventClassTSMDocumentAccess, kEventTSMDocumentAccessGetSelectedRange },
	{ kEventClassTSMDocumentAccess, kEventTSMDocumentAccessGetCharacters },
	{ kEventClassTSMDocumentAccess, kEventTSMDocumentAccessGetFont },
	{ kEventClassTSMDocumentAccess, kEventTSMDocumentAccessGetGlyphInfo },
	{ kEventClassTSMDocumentAccess, kEventTSMDocumentAccessLockDocument },
	{ kEventClassTSMDocumentAccess, kEventTSMDocumentAccessUnlockDocument }
};

static EventTypeSpec GlobalHandlerEventList[] =
{
	// Mouse events
	{ kEventClassMouse, kEventMouseDown },
	{ kEventClassMouse, kEventMouseUp },
	{ kEventClassMouse, kEventMouseDragged },
	{ kEventClassMouse, kEventMouseWheelMoved },
	{ kEventClassMouse, kEventMouseMoved },

	// Keyboard events
	// No longer handle raw key down events directly.
	// When text input events come in, extract the raw key events from them and process at that point.
	// This allows input methods to eat keystrokes the way they're supposed to.
//	{ kEventClassKeyboard, kEventRawKeyDown },
//	{ kEventClassKeyboard, kEventRawKeyRepeat },
	{ kEventClassKeyboard, kEventRawKeyUp },
	{ kEventClassKeyboard, kEventRawKeyModifiersChanged },

	// Text input events
	{ kEventClassTextInput, kEventTextInputUpdateActiveInputArea },
	{ kEventClassTextInput, kEventTextInputUnicodeForKeyEvent },
	{ kEventClassTextInput, kEventTextInputOffsetToPos },
	{ kEventClassTextInput, kEventTextInputPosToOffset },
	{ kEventClassTextInput, kEventTextInputShowHideBottomWindow },
	{ kEventClassTextInput, kEventTextInputGetSelectedText },
	{ kEventClassTextInput, kEventTextInputFilterText },

	// TSM Document Access events (advanced input method support)
	{ kEventClassTSMDocumentAccess, kEventTSMDocumentAccessGetLength },
	{ kEventClassTSMDocumentAccess, kEventTSMDocumentAccessGetSelectedRange },
	{ kEventClassTSMDocumentAccess, kEventTSMDocumentAccessGetCharacters },
	{ kEventClassTSMDocumentAccess, kEventTSMDocumentAccessGetFont },
	{ kEventClassTSMDocumentAccess, kEventTSMDocumentAccessGetGlyphInfo },
	{ kEventClassTSMDocumentAccess, kEventTSMDocumentAccessLockDocument },
	{ kEventClassTSMDocumentAccess, kEventTSMDocumentAccessUnlockDocument }
};

static EventTypeSpec CommandHandlerEventList[] =
{
	{ kEventClassCommand, kEventCommandProcess }
};

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
	mOldDisplayMode = NULL;
	mTimer = NULL;
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
	mTSMDocument = NULL; // Just in case.
	mLanguageTextInputAllowed = FALSE;
	mTSMScriptCode = 0;
	mTSMLangCode = 0;
	mPreeditor = NULL;
	mRawKeyEvent = NULL;
	mFSAASamples = fsaa_samples;
	mForceRebuild = FALSE;

	// For reasons that aren't clear to me, LLTimers seem to be created in the "started" state.
	// Since the started state of this one is used to track whether the NMRec has been installed, it wants to start out in the "stopped" state.
	mBounceTimer.stop();

	// Get the original aspect ratio of the main device.
	mOriginalAspectRatio = (double)CGDisplayPixelsWide(mDisplay) / (double)CGDisplayPixelsHigh(mDisplay);

	// Stash the window title
	strcpy((char*)mWindowTitle + 1, title.c_str()); /* Flawfinder: ignore */
	mWindowTitle[0] = title.length();

	mEventHandlerUPP = NewEventHandlerUPP(staticEventHandler);
	mMoveEventCampartorUPP = NewEventComparatorUPP(staticMoveEventComparator);
	mGlobalHandlerRef = NULL;
	mWindowHandlerRef = NULL;

	mDragOverrideCursor = -1;

	// We're not clipping yet
	SetRect( &mOldMouseClip, 0, 0, 0, 0 );

	// Set up global event handlers (the fullscreen case needs this)
	InstallStandardEventHandler(GetApplicationEventTarget());

	// Stash an object pointer for OSMessageBox()
	gWindowImplementation = this;

	// Create the GL context and set it up for windowed or fullscreen, as appropriate.
	if(createContext(x, y, width, height, 32, fullscreen, disable_vsync))
	{
		if(mWindow != NULL)
		{
			// MBW -- XXX -- I think we can now do this here?
			// Constrain the window to the screen it's mostly on, resizing if necessary.
			ConstrainWindowToScreen(
				mWindow,
				kWindowStructureRgn,
				kWindowConstrainMayResize |
				//				kWindowConstrainStandardOptions |
				0,
				NULL,
				NULL);

			MacShowWindow(mWindow);
			BringToFront(mWindow);
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

BOOL LLWindowMacOSX::createContext(int x, int y, int width, int height, int bits, BOOL fullscreen, BOOL disable_vsync)
{
	OSStatus		err;
	BOOL			glNeedsInit = FALSE;

	if(mGlobalHandlerRef == NULL)
	{
		InstallApplicationEventHandler(mEventHandlerUPP, GetEventTypeCount (CommandHandlerEventList), CommandHandlerEventList, (void*)this, &mGlobalHandlerRef);
	}

	mFullscreen = fullscreen;

	if (mFullscreen && (mOldDisplayMode == NULL))
	{
		LL_INFOS("Window") << "createContext: setting up fullscreen " << width << "x" << height << LL_ENDL;

		// NOTE: The refresh rate will be REPORTED AS 0 for many DVI and notebook displays.  Plan accordingly.
		double refresh = getDictDouble (CGDisplayCurrentMode (mDisplay),  kCGDisplayRefreshRate);

		// If the requested width or height is 0, find the best default for the monitor.
		if((width == 0) || (height == 0))
		{
			// Scan through the list of modes, looking for one which has:
			//		height between 700 and 800
			//		aspect ratio closest to the user's original mode
			S32 resolutionCount = 0;
			LLWindowResolution *resolutionList = getSupportedResolutions(resolutionCount);

			if(resolutionList != NULL)
			{
				F32 closestAspect = 0;
				U32 closestHeight = 0;
				U32 closestWidth = 0;
				int i;

				LL_DEBUGS("Window") << "createContext: searching for a display mode, original aspect is " << mOriginalAspectRatio << LL_ENDL;

				for(i=0; i < resolutionCount; i++)
				{
					F32 aspect = (F32)resolutionList[i].mWidth / (F32)resolutionList[i].mHeight;

					LL_DEBUGS("Window") << "createContext: width " << resolutionList[i].mWidth << " height " << resolutionList[i].mHeight << " aspect " << aspect << LL_ENDL;

					if( (resolutionList[i].mHeight >= 700) && (resolutionList[i].mHeight <= 800) &&
						(fabs(aspect - mOriginalAspectRatio) < fabs(closestAspect - mOriginalAspectRatio)))
					{
						LL_DEBUGS("Window") << " (new closest mode) " << LL_ENDL;

						// This is the closest mode we've seen yet.
						closestWidth = resolutionList[i].mWidth;
						closestHeight = resolutionList[i].mHeight;
						closestAspect = aspect;
					}
				}

				width = closestWidth;
				height = closestHeight;
			}
		}

		if((width == 0) || (height == 0))
		{
			// Mode search failed for some reason.  Use the old-school default.
			width = 1024;
			height = 768;
		}

		if (true)
		{
			// Fullscreen support
			CFDictionaryRef refDisplayMode = 0;
			boolean_t exactMatch = false;

#if CAPTURE_ALL_DISPLAYS
			// Capture all displays (may want to do this for final build)
			CGCaptureAllDisplays ();
#else
			// Capture only the main display (useful for debugging)
			CGDisplayCapture (mDisplay);
#endif

			// Switch the display to the desired resolution and refresh
			refDisplayMode = CGDisplayBestModeForParametersAndRefreshRate(
				mDisplay,
				BITS_PER_PIXEL,
				width,
				height,
				refresh,
				&exactMatch);

			if (refDisplayMode)
			{
				LL_DEBUGS("Window") << "createContext: switching display resolution" << LL_ENDL;
				mOldDisplayMode = CGDisplayCurrentMode (mDisplay);
				CGDisplaySwitchToMode (mDisplay, refDisplayMode);
				//				CFRelease(refDisplayMode);

				AddEventTypesToHandler(mGlobalHandlerRef, GetEventTypeCount (GlobalHandlerEventList), GlobalHandlerEventList);
			}


			mFullscreen = TRUE;
			mFullscreenWidth   = CGDisplayPixelsWide(mDisplay);
			mFullscreenHeight  = CGDisplayPixelsHigh(mDisplay);
			mFullscreenBits    = CGDisplayBitsPerPixel(mDisplay);
			mFullscreenRefresh = llround(getDictDouble (CGDisplayCurrentMode (mDisplay),  kCGDisplayRefreshRate));

			LL_INFOS("Window") << "Running at " << mFullscreenWidth
				<< "x"   << mFullscreenHeight
				<< "x"   << mFullscreenBits
				<< " @ " << mFullscreenRefresh
				<< LL_ENDL;
		}
		else
		{
			// No fullscreen support
			mFullscreen = FALSE;
			mFullscreenWidth   = -1;
			mFullscreenHeight  = -1;
			mFullscreenBits    = -1;
			mFullscreenRefresh = -1;

			std::string error= llformat("Unable to run fullscreen at %d x %d.\nRunning in window.", width, height);
			OSMessageBox(error, "Error", OSMB_OK);
		}
	}

	if(!mFullscreen && (mWindow == NULL))
	{
		//int				displayWidth = CGDisplayPixelsWide(mDisplay);
		//int				displayHeight = CGDisplayPixelsHigh(mDisplay);
		//const int		menuBarPlusTitleBar = 44;   // Ugly magic number.

		LL_DEBUGS("Window") << "createContext: creating window" << LL_ENDL;

		mPreviousWindowRect.left = (long) x;
		mPreviousWindowRect.right = (long) x + width;
		mPreviousWindowRect.top = (long) y;
		mPreviousWindowRect.bottom = (long) y + height;

		//-----------------------------------------------------------------------
		// Create the window
		//-----------------------------------------------------------------------
		mWindow = NewCWindow(
			NULL,
			&mPreviousWindowRect,
			mWindowTitle,
			false,				// Create the window invisible.  Whoever calls createContext() should show it after any moving/resizing.
			//		noGrowDocProc,		// Window with no grow box and no zoom box
			zoomDocProc,		// Window with a grow box and a zoom box
			//		zoomNoGrow,			// Window with a zoom box but no grow box
			kFirstWindowOfClass,
			true,
			(long)this);

		if (!mWindow)
		{
			setupFailure("Window creation error", "Error", OSMB_OK);
			return FALSE;
		}

		// Turn on live resize.
		// For this to work correctly, we need to be able to call LLViewerWindow::draw from
		// the event handler for kEventWindowBoundsChanged.  It's not clear that we have access from here.
		//	err = ChangeWindowAttributes(mWindow, kWindowLiveResizeAttribute, 0);

		// Set up window event handlers (some window-related events ONLY go to window handlers.)
		InstallStandardEventHandler(GetWindowEventTarget(mWindow));
		InstallWindowEventHandler(mWindow, mEventHandlerUPP, GetEventTypeCount (WindowHandlerEventList), WindowHandlerEventList, (void*)this, &mWindowHandlerRef); // add event handler
#if LL_OS_DRAGDROP_ENABLED
		InstallTrackingHandler( dragTrackingHandler, mWindow, (void*)this );
		InstallReceiveHandler( dragReceiveHandler, mWindow, (void*)this );
#endif // LL_OS_DRAGDROP_ENABLED
	}

	{
		// Create and initialize our TSM document for language text input.
		// If an error occured, we can do nothing better than simply ignore it.
		// mTSMDocument will be kept NULL in case.
		if (mTSMDocument)
		{
			DeactivateTSMDocument(mTSMDocument);
			DeleteTSMDocument(mTSMDocument);
			mTSMDocument = NULL;
		}
		static InterfaceTypeList types = { kUnicodeDocument };
		err = NewTSMDocument(1, types, &mTSMDocument, 0);
		if (err != noErr)
		{
			LL_WARNS("Window") << "createContext: couldn't create a TSMDocument (" << err << ")" << LL_ENDL;
		}
		if (mTSMDocument)
		{
			ActivateTSMDocument(mTSMDocument);
			allowLanguageTextInput(NULL, FALSE);
		}
	}

	if(mContext == NULL)
	{
		AGLRendererInfo rendererInfo = NULL;

		//-----------------------------------------------------------------------
		// Create GL drawing context
		//-----------------------------------------------------------------------

		if(mPixelFormat == NULL)
		{
			if(mFullscreen)
			{
				GLint fullscreenAttrib[] =
				{
					AGL_RGBA,
					AGL_FULLSCREEN,
					AGL_NO_RECOVERY,
					AGL_SAMPLE_BUFFERS_ARB, mFSAASamples > 0 ? 1 : 0,
					AGL_SAMPLES_ARB, mFSAASamples,
					AGL_DOUBLEBUFFER,
					AGL_CLOSEST_POLICY,
					AGL_ACCELERATED,
					AGL_RED_SIZE, 8,
					AGL_GREEN_SIZE, 8,
					AGL_BLUE_SIZE, 8,
					AGL_ALPHA_SIZE, 8,
					AGL_DEPTH_SIZE, 24,
					AGL_STENCIL_SIZE, 8,
					AGL_NONE
				};

				LL_DEBUGS("Window") << "createContext: creating fullscreen pixelformat" << LL_ENDL;

				GDHandle gdhDisplay = NULL;
				err = DMGetGDeviceByDisplayID ((DisplayIDType)mDisplay, &gdhDisplay, false);

				mPixelFormat = aglChoosePixelFormat(&gdhDisplay, 1, fullscreenAttrib);
				rendererInfo = aglQueryRendererInfo(&gdhDisplay, 1);
			}
			else
			{
				// NOTE from Leslie:
				//
				// AGL_NO_RECOVERY, when combined with AGL_ACCELERATED prevents software rendering
				// fallback which means we won't hvae shaders that compile and link but then don't
				// work.  The drawback is that our shader compilation will be a bit more finicky though.

				GLint windowedAttrib[] =
				{
					AGL_RGBA,
					AGL_NO_RECOVERY,
					AGL_DOUBLEBUFFER,
					AGL_CLOSEST_POLICY,
					AGL_ACCELERATED,
					AGL_SAMPLE_BUFFERS_ARB, mFSAASamples > 0 ? 1 : 0,
					AGL_SAMPLES_ARB, mFSAASamples,
					AGL_RED_SIZE, 8,
					AGL_GREEN_SIZE, 8,
					AGL_BLUE_SIZE, 8,
					AGL_ALPHA_SIZE, 8,
					AGL_DEPTH_SIZE, 24,
					AGL_STENCIL_SIZE, 8,
					AGL_NONE
				};

				LL_DEBUGS("Window") << "createContext: creating windowed pixelformat" << LL_ENDL;

				mPixelFormat = aglChoosePixelFormat(NULL, 0, windowedAttrib);

				GDHandle gdhDisplay = GetMainDevice();
				rendererInfo = aglQueryRendererInfo(&gdhDisplay, 1);
			}

			// May want to get the real error text like this:
			// (char *) aglErrorString(aglGetError());

			if(aglGetError() != AGL_NO_ERROR)
			{
				setupFailure("Can't find suitable pixel format", "Error", OSMB_OK);
				return FALSE;
			}
		}

		if(mPixelFormat)
		{
			LL_DEBUGS("Window") << "createContext: creating GL context" << LL_ENDL;
			mContext = aglCreateContext(mPixelFormat, NULL);
		}

		if(mContext == NULL)
		{
			setupFailure("Can't make GL context", "Error", OSMB_OK);
			return FALSE;
		}

		gGLManager.mVRAM = 0;

		if(rendererInfo != NULL)
		{
			GLint result;

			if(aglDescribeRenderer(rendererInfo, AGL_VIDEO_MEMORY, &result))
			{
				//				llinfos << "createContext: aglDescribeRenderer(AGL_VIDEO_MEMORY) returned " << result << llendl;
				gGLManager.mVRAM = result / (1024 * 1024);
			}
			else
			{
				//				llinfos << "createContext: aglDescribeRenderer(AGL_VIDEO_MEMORY) failed." << llendl;
			}

			// This could be useful at some point, if it takes into account the memory already used by screen buffers, etc...
			if(aglDescribeRenderer(rendererInfo, AGL_TEXTURE_MEMORY, &result))
			{
				//				llinfos << "createContext: aglDescribeRenderer(AGL_TEXTURE_MEMORY) returned " << result << llendl;
			}
			else
			{
				//				llinfos << "createContext: aglDescribeRenderer(AGL_TEXTURE_MEMORY) failed." << llendl;
			}

			aglDestroyRendererInfo(rendererInfo);
		}

		// Since we just created the context, it needs to be set up.
		glNeedsInit = TRUE;
	}

	// Hook up the context to a drawable
	if (mFullscreen && (mOldDisplayMode != NULL))
	{
		// We successfully captured the display.  Use a fullscreen drawable

		LL_DEBUGS("Window") << "createContext: attaching fullscreen drawable" << LL_ENDL;

#if CAPTURE_ALL_DISPLAYS
		// Capture all displays (may want to do this for final build)
		aglDisable (mContext, AGL_FS_CAPTURE_SINGLE);
#else
		// Capture only the main display (useful for debugging)
		aglEnable (mContext, AGL_FS_CAPTURE_SINGLE);
#endif

		if (!aglSetFullScreen (mContext, 0, 0, 0, 0))
		{
			setupFailure("Can't set GL fullscreen", "Error", OSMB_OK);
			return FALSE;
		}
	}
	else if(!mFullscreen && (mWindow != NULL))
	{
		LL_DEBUGS("Window") << "createContext: attaching windowed drawable" << LL_ENDL;

		// We created a window.  Use it as the drawable.
		if(!aglSetDrawable(mContext, GetWindowPort (mWindow)))
		{
			setupFailure("Can't set GL drawable", "Error", OSMB_OK);
			return FALSE;
		}
	}
	else
	{
		setupFailure("Can't get fullscreen or windowed drawable.", "Error", OSMB_OK);
		return FALSE;
	}

	if(mContext != NULL)
	{
		LL_DEBUGS("Window") << "createContext: setting current context" << LL_ENDL;

		if (!aglSetCurrentContext(mContext))
		{
			setupFailure("Can't activate GL rendering context", "Error", OSMB_OK);
			return FALSE;
		}
	}

	if(glNeedsInit)
	{
		// Check for some explicitly unsupported cards.
		const char* RENDERER = (const char*) glGetString(GL_RENDERER);

		const char* CARD_LIST[] =
		{	"RAGE 128",
		"RIVA TNT2",
		"Intel 810",
		"3Dfx/Voodoo3",
		"Radeon 7000",
		"Radeon 7200",
		"Radeon 7500",
		"Radeon DDR",
		"Radeon VE",
		"GDI Generic" };
		const S32 CARD_COUNT = LL_ARRAY_SIZE(CARD_LIST);

		// Future candidates:
		// ProSavage/Twister
		// SuperSavage

		S32 i;
		for (i = 0; i < CARD_COUNT; i++)
		{
			if (check_for_card(RENDERER, CARD_LIST[i]))
			{
				close();
				return FALSE;
			}
		}
	}

	GLint colorBits, alphaBits, depthBits, stencilBits;

	if(	!aglDescribePixelFormat(mPixelFormat, AGL_BUFFER_SIZE, &colorBits) ||
		!aglDescribePixelFormat(mPixelFormat, AGL_ALPHA_SIZE, &alphaBits) ||
		!aglDescribePixelFormat(mPixelFormat, AGL_DEPTH_SIZE, &depthBits) ||
		!aglDescribePixelFormat(mPixelFormat, AGL_STENCIL_SIZE, &stencilBits))
	{
		close();
		setupFailure("Can't get pixel format description", "Error", OSMB_OK);
		return FALSE;
	}

	LL_INFOS("GLInit") << "GL buffer: Color Bits " << S32(colorBits)
		<< " Alpha Bits " << S32(alphaBits)
		<< " Depth Bits " << S32(depthBits)
		<< " Stencil Bits" << S32(stencilBits)
		<< LL_ENDL;

	if (colorBits < 32)
	{
		close();
		setupFailure(
			"Second Life requires True Color (32-bit) to run in a window.\n"
			"Please go to Control Panels -> Display -> Settings and\n"
			"set the screen to 32-bit color.\n"
			"Alternately, if you choose to run fullscreen, Second Life\n"
			"will automatically adjust the screen each time it runs.",
			"Error",
			OSMB_OK);
		return FALSE;
	}

	if (alphaBits < 8)
	{
		close();
		setupFailure(
			"Second Life is unable to run because it can't get an 8 bit alpha\n"
			"channel.  Usually this is due to video card driver issues.\n"
			"Please make sure you have the latest video card drivers installed.\n"
			"Also be sure your monitor is set to True Color (32-bit) in\n"
			"Control Panels -> Display -> Settings.\n"
			"If you continue to receive this message, contact customer service.",
			"Error",
			OSMB_OK);
		return FALSE;
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
	aglSetInteger(mContext, AGL_SWAP_INTERVAL, &frames_per_swap);

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

	// Don't need to get the current gamma, since there's a call that restores it to the system defaults.
	return TRUE;
}


// changing fullscreen resolution, or switching between windowed and fullscreen mode.
BOOL LLWindowMacOSX::switchContext(BOOL fullscreen, const LLCoordScreen &size, BOOL disable_vsync, const LLCoordScreen * const posp)
{
	BOOL needsRebuild = FALSE;
	BOOL result = true;

	if(fullscreen)
	{
		if(mFullscreen)
		{
			// Switching resolutions in fullscreen mode.  Don't need to rebuild for this.
			// Fullscreen support
			CFDictionaryRef refDisplayMode = 0;
			boolean_t exactMatch = false;

			// Switch the display to the desired resolution and refresh
			refDisplayMode = CGDisplayBestModeForParametersAndRefreshRate(
				mDisplay,
				BITS_PER_PIXEL,
				size.mX,
				size.mY,
				getDictDouble (CGDisplayCurrentMode (mDisplay),  kCGDisplayRefreshRate),
				&exactMatch);

			if (refDisplayMode)
			{
				CGDisplaySwitchToMode (mDisplay, refDisplayMode);
				//				CFRelease(refDisplayMode);
			}

			mFullscreenWidth   = CGDisplayPixelsWide(mDisplay);
			mFullscreenHeight  = CGDisplayPixelsHigh(mDisplay);
			mFullscreenBits    = CGDisplayBitsPerPixel(mDisplay);
			mFullscreenRefresh = llround(getDictDouble (CGDisplayCurrentMode (mDisplay),  kCGDisplayRefreshRate));

			LL_INFOS("Window") << "Switched resolution to " << mFullscreenWidth
				<< "x"   << mFullscreenHeight
				<< "x"   << mFullscreenBits
				<< " @ " << mFullscreenRefresh
				<< LL_ENDL;

			// Update the GL context to the new screen size
			if (!aglUpdateContext(mContext))
			{
				setupFailure("Can't set GL fullscreen", "Error", OSMB_OK);
				result = FALSE;
			}
		}
		else
		{
			// Switching from windowed to fullscreen
			needsRebuild = TRUE;
		}
	}
	else
	{
		if(mFullscreen)
		{
			// Switching from fullscreen to windowed
			needsRebuild = TRUE;
		}
		else
		{
			// Windowed to windowed -- not sure why we would be called like this.  Just change the window size.
			// The bounds changed event handler will do the rest.
			if(mWindow != NULL)
			{
				::SizeWindow(mWindow, size.mX, size.mY, true);
			}
		}
	}

	stop_glerror();
	if(needsRebuild || mForceRebuild)
	{
		mForceRebuild = FALSE;
		destroyContext();
		result = createContext(0, 0, size.mX, size.mY, 0, fullscreen, disable_vsync);
		if (result)
		{
			if(mWindow != NULL)
			{
				MacShowWindow(mWindow);
				BringToFront(mWindow);
			}

			llverify(gGLManager.initGL());

			//start with arrow cursor
			initCursors();
			setCursor( UI_CURSOR_ARROW );
		}
	}

	stop_glerror();

	return result;
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

		aglSetCurrentContext (NULL);
		aglSetDrawable(mContext, NULL);
	}

	// Make sure the display resolution gets restored
	if(mOldDisplayMode != NULL)
	{
		LL_DEBUGS("Window") << "destroyContext: restoring display resolution " << LL_ENDL;

		CGDisplaySwitchToMode (mDisplay, mOldDisplayMode);

#if CAPTURE_ALL_DISPLAYS
		// Uncapture all displays (may want to do this for final build)
		CGReleaseAllDisplays ();
#else
		// Uncapture only the main display (useful for debugging)
		CGDisplayRelease (mDisplay);
#endif

		//		CFRelease(mOldDisplayMode);

		mOldDisplayMode = NULL;

		// Remove the global event handlers the fullscreen case needed
		RemoveEventTypesFromHandler(mGlobalHandlerRef, GetEventTypeCount (GlobalHandlerEventList), GlobalHandlerEventList);
	}

	// Clean up remaining GL state before blowing away window
	gGLManager.shutdownGL();

	// Clean up the pixel format
	if(mPixelFormat != NULL)
	{
		LL_DEBUGS("Window") << "destroyContext: destroying pixel format " << LL_ENDL;
		aglDestroyPixelFormat(mPixelFormat);
		mPixelFormat = NULL;
	}

	// Remove any Carbon Event handlers we installed
	if(mGlobalHandlerRef != NULL)
	{
		LL_DEBUGS("Window") << "destroyContext: removing global event handler" << LL_ENDL;
		RemoveEventHandler(mGlobalHandlerRef);
		mGlobalHandlerRef = NULL;
	}

	if(mWindowHandlerRef != NULL)
	{
		LL_DEBUGS("Window") << "destroyContext: removing window event handler" << LL_ENDL;
		RemoveEventHandler(mWindowHandlerRef);
		mWindowHandlerRef = NULL;
	}

	// Cleanup any TSM document we created.
	if(mTSMDocument != NULL)
	{
		LL_DEBUGS("Window") << "destroyContext: deleting TSM document" << LL_ENDL;
		DeactivateTSMDocument(mTSMDocument);
		DeleteTSMDocument(mTSMDocument);
		mTSMDocument = NULL;
	}

	// Close the window
	if(mWindow != NULL)
	{
		LL_DEBUGS("Window") << "destroyContext: disposing window" << LL_ENDL;
		DisposeWindow(mWindow);
		mWindow = NULL;
	}

	// Clean up the GL context
	if(mContext != NULL)
	{
		LL_DEBUGS("Window") << "destroyContext: destroying GL context" << LL_ENDL;
		aglDestroyContext(mContext);
		mContext = NULL;
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
	if(IsWindowCollapsed(mWindow))
		CollapseWindow(mWindow, false);

	MacShowWindow(mWindow);
	BringToFront(mWindow);
}

void LLWindowMacOSX::hide()
{
	setMouseClipping(FALSE);
	HideWindow(mWindow);
}

//virtual
void LLWindowMacOSX::minimize()
{
	setMouseClipping(FALSE);
	showCursor();
	CollapseWindow(mWindow, true);
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
		if(MacIsWindowVisible(mWindow))
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
		ZoomWindow(mWindow, inContent, true);
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

	// Use the old-school version so we get AppleEvent handler dispatch and menuselect handling.
	// Anything that has an event handler will get processed inside WaitNextEvent, so we only need to handle
	// the odd stuff here.
	EventRecord evt;
	while(WaitNextEvent(everyEvent, &evt, 0, NULL))
	{
		//			printf("WaitNextEvent returned true, event is %d.\n", evt.what);
		switch(evt.what)
		{
		case mouseDown:
			{
				short part;
				WindowRef window;
				long selectResult;
				part = FindWindow(evt.where, &window);
				switch ( part )
				{
				case inMenuBar:
					selectResult = MenuSelect(evt.where);

					HiliteMenu(0);
					break;
				}
			}
			break;

		case kHighLevelEvent:
			AEProcessAppleEvent (&evt);
			break;

		case updateEvt:
			// We shouldn't be getting these regularly (since our window will be buffered), but we need to handle them correctly...
			BeginUpdate((WindowRef)evt.message);
			EndUpdate((WindowRef)evt.message);
			break;

		}
	}
	
	updateCursor();
}

BOOL LLWindowMacOSX::getPosition(LLCoordScreen *position)
{
	Rect window_rect;
	OSStatus err = -1;

	if(mFullscreen)
	{
		position->mX = 0;
		position->mY = 0;
		err = noErr;
	}
	else if(mWindow)
	{
		err = GetWindowBounds(mWindow, kWindowContentRgn, &window_rect);

		position->mX = window_rect.left;
		position->mY = window_rect.top;
	}
	else
	{
		llerrs << "LLWindowMacOSX::getPosition(): no window and not fullscreen!" << llendl;
	}

	return (err == noErr);
}

BOOL LLWindowMacOSX::getSize(LLCoordScreen *size)
{
	Rect window_rect;
	OSStatus err = -1;

	if(mFullscreen)
	{
		size->mX = mFullscreenWidth;
		size->mY = mFullscreenHeight;
		err = noErr;
	}
	else if(mWindow)
	{
		err = GetWindowBounds(mWindow, kWindowContentRgn, &window_rect);

		size->mX = window_rect.right - window_rect.left;
		size->mY = window_rect.bottom - window_rect.top;
	}
	else
	{
		llerrs << "LLWindowMacOSX::getPosition(): no window and not fullscreen!" << llendl;
	}

	return (err == noErr);
}

BOOL LLWindowMacOSX::getSize(LLCoordWindow *size)
{
	Rect window_rect;
	OSStatus err = -1;

	if(mFullscreen)
	{
		size->mX = mFullscreenWidth;
		size->mY = mFullscreenHeight;
		err = noErr;
	}
	else if(mWindow)
	{
		err = GetWindowBounds(mWindow, kWindowContentRgn, &window_rect);

		size->mX = window_rect.right - window_rect.left;
		size->mY = window_rect.bottom - window_rect.top;
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
		MacMoveWindow(mWindow, position.mX, position.mY, false);
	}

	return TRUE;
}

BOOL LLWindowMacOSX::setSizeImpl(const LLCoordScreen size)
{
	if(mWindow)
	{
		SizeWindow(mWindow, size.mX, size.mY, true);
	}

	return TRUE;
}

BOOL LLWindowMacOSX::setSizeImpl(const LLCoordWindow size)
{
	Rect client_rect;
	if (mWindow)
	{
		OSStatus err = GetWindowBounds(mWindow, kWindowContentRgn, &client_rect);
		if (err == noErr)
		{
			client_rect.right = client_rect.left + size.mX;
			client_rect.bottom = client_rect.top + size.mY;
			err = SetWindowBounds(mWindow, kWindowContentRgn, &client_rect);
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
	aglSwapBuffers(mContext);
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

BOOL LLWindowMacOSX::getCursorPosition(LLCoordWindow *position)
{
	Point cursor_point;
	LLCoordScreen screen_pos;
	GrafPtr save;

	if(mWindow == NULL)
		return FALSE;

	::GetPort(&save);
	::SetPort(GetWindowPort(mWindow));
	fixOrigin();

	// gets the mouse location in local coordinates
	::GetMouse(&cursor_point);

//	lldebugs << "getCursorPosition(): cursor is at " << cursor_point.h << ", " << cursor_point.v << "  port origin: " << portrect.left << ", " << portrect.top << llendl;

	::SetPort(save);

	if(mCursorDecoupled)
	{
		//		CGMouseDelta x, y;

		// If the cursor's decoupled, we need to read the latest movement delta as well.
		//		CGGetLastMouseDelta( &x, &y );
		//		cursor_point.h += x;
		//		cursor_point.v += y;

		// CGGetLastMouseDelta may behave strangely when the cursor's first captured.
		// Stash in the event handler instead.
		cursor_point.h += mCursorLastEventDeltaX;
		cursor_point.v += mCursorLastEventDeltaY;
	}

	position->mX = cursor_point.h;
	position->mY = cursor_point.v;

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
				FlushSpecificEventsFromQueue(GetCurrentEventQueue(), mMoveEventCampartorUPP, NULL);
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

void LLWindowMacOSX::beforeDialog()
{
	if(mFullscreen)
	{

#if CAPTURE_ALL_DISPLAYS
		// Uncapture all displays (may want to do this for final build)
		CGReleaseAllDisplays ();
#else
		// Uncapture only the main display (useful for debugging)
		CGDisplayRelease (mDisplay);
#endif
		// kDocumentWindowClass
		// kMovableModalWindowClass
		// kAllWindowClasses

		//		GLint order = 0;
		//		aglSetInteger(mContext, AGL_ORDER_CONTEXT_TO_FRONT, &order);
		aglSetDrawable(mContext, NULL);
		//		GetWindowGroupLevel(GetWindowGroupOfClass(kAllWindowClasses), &oldWindowLevel);
		//		SetWindowGroupLevel(GetWindowGroupOfClass(kAllWindowClasses), CGShieldingWindowLevel());

		mHandsOffEvents = TRUE;

	}
}

void LLWindowMacOSX::afterDialog()
{
	if(mFullscreen)
	{
		mHandsOffEvents = FALSE;

		//		SetWindowGroupLevel(GetWindowGroupOfClass(kAllWindowClasses), oldWindowLevel);
		aglSetFullScreen(mContext, 0, 0, 0, 0);
		//		GLint order = 1;
		//		aglSetInteger(mContext, AGL_ORDER_CONTEXT_TO_FRONT, &order);

#if CAPTURE_ALL_DISPLAYS
		// Capture all displays (may want to do this for final build)
		CGCaptureAllDisplays ();
#else
		// Capture only the main display (useful for debugging)
		CGDisplayCapture (mDisplay);
#endif
	}
}


void LLWindowMacOSX::flashIcon(F32 seconds)
{
	// Don't do this if we're already started, since this would try to install the NMRec twice.
	if(!mBounceTimer.getStarted())
	{
		OSErr err;

		mBounceTime = seconds;
		memset(&mBounceRec, 0, sizeof(mBounceRec));
		mBounceRec.qType = nmType;
		mBounceRec.nmMark = 1;
		err = NMInstall(&mBounceRec);
		if(err == noErr)
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
	OSStatus err;
	ScrapRef scrap;
	ScrapFlavorFlags flags;
	BOOL result = false;

	err = GetCurrentScrap(&scrap);

	if(err == noErr)
	{
		err = GetScrapFlavorFlags(scrap, kScrapFlavorTypeUnicode, &flags);
	}

	if(err == noErr)
		result = true;

	return result;
}

BOOL LLWindowMacOSX::pasteTextFromClipboard(LLWString &dst)
{
	OSStatus err;
	ScrapRef scrap;
	Size len;
	BOOL result = false;

	err = GetCurrentScrap(&scrap);

	if(err == noErr)
	{
		err = GetScrapFlavorSize(scrap, kScrapFlavorTypeUnicode, &len);
	}

	if((err == noErr) && (len > 0))
	{
		int u16len = len / sizeof(U16);
		U16 *temp = new U16[u16len + 1];
		if (temp)
		{
			memset(temp, 0, (u16len + 1) * sizeof(temp[0]));
			err = GetScrapFlavorData(scrap, kScrapFlavorTypeUnicode, &len, temp);
			if (err == noErr)
			{
				// convert \r\n to \n and \r to \n in the incoming text.
				U16 *s, *d;
				for(s = d = temp; s[0] != '\0'; s++, d++)
				{
					if(s[0] == '\r')
					{
						if(s[1] == '\n')
						{
							// CRLF, a.k.a. DOS newline.  Collapse to a single '\n'.
							s++;
						}

						d[0] = '\n';
					}
					else
					{
						d[0] = s[0];
					}
				}

				d[0] = '\0';

				dst = utf16str_to_wstring(temp);

				result = true;
			}
			delete[] temp;
		}
	}

	return result;
}

BOOL LLWindowMacOSX::copyTextToClipboard(const LLWString &s)
{
	OSStatus err;
	ScrapRef scrap;
	//Size len;
	//char *temp;
	BOOL result = false;

	if (!s.empty())
	{
		err = GetCurrentScrap(&scrap);
		if (err == noErr)
			err = ClearScrap(&scrap);

		if (err == noErr)
		{
			llutf16string utf16str = wstring_to_utf16str(s);
			size_t u16len = utf16str.length() * sizeof(U16);
			err = PutScrapFlavor(scrap, kScrapFlavorTypeUnicode, kScrapFlavorMaskNone, u16len, utf16str.data());
			if (err == noErr)
				result = true;
		}
	}

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
	Rect	client_rect;

	if(mFullscreen)
	{
		// In the fullscreen case, the "window" is the entire screen.
		client_rect.left = 0;
		client_rect.top = 0;
		client_rect.right = mFullscreenWidth;
		client_rect.bottom = mFullscreenHeight;
	}
	else if (!mWindow ||
		(GetWindowBounds(mWindow, kWindowContentRgn, &client_rect) != noErr) ||
		NULL == to)
	{
		return FALSE;
	}

	to->mX = from.mX;
	client_height = client_rect.bottom - client_rect.top;
	to->mY = client_height - from.mY - 1;

	return TRUE;
}

BOOL LLWindowMacOSX::convertCoords(LLCoordWindow from, LLCoordGL* to)
{
	S32		client_height;
	Rect	client_rect;

	if(mFullscreen)
	{
		// In the fullscreen case, the "window" is the entire screen.
		client_rect.left = 0;
		client_rect.top = 0;
		client_rect.right = mFullscreenWidth;
		client_rect.bottom = mFullscreenHeight;
	}
	else if (!mWindow ||
		(GetWindowBounds(mWindow, kWindowContentRgn, &client_rect) != noErr) ||
		NULL == to)
	{
		return FALSE;
	}

	to->mX = from.mX;
	client_height = client_rect.bottom - client_rect.top;
	to->mY = client_height - from.mY - 1;

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
		GrafPtr save;
		Point mouse_point;

		mouse_point.h = from.mX;
		mouse_point.v = from.mY;

		::GetPort(&save);
		::SetPort(GetWindowPort(mWindow));
		fixOrigin();

		::GlobalToLocal(&mouse_point);

		to->mX = mouse_point.h;
		to->mY = mouse_point.v;

		::SetPort(save);

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
		GrafPtr save;
		Point mouse_point;

		mouse_point.h = from.mX;
		mouse_point.v = from.mY;
		::GetPort(&save);
		::SetPort(GetWindowPort(mWindow));
		fixOrigin();

		LocalToGlobal(&mouse_point);

		to->mX = mouse_point.h;
		to->mY = mouse_point.v;

		::SetPort(save);

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

pascal Boolean LLWindowMacOSX::staticMoveEventComparator( EventRef event, void* data)
{
	UInt32 				evtClass = GetEventClass (event);
	UInt32 				evtKind = GetEventKind (event);

	if ((evtClass == kEventClassMouse) && ((evtKind == kEventMouseDragged) || (evtKind == kEventMouseMoved)))
	{
		return true;
	}

	else
	{
		return false;
	}
}


pascal OSStatus LLWindowMacOSX::staticEventHandler(EventHandlerCallRef myHandler, EventRef event, void* userData)
{
	LLWindowMacOSX *self = (LLWindowMacOSX*)userData;

	return(self->eventHandler(myHandler, event));
}

OSStatus LLWindowMacOSX::eventHandler (EventHandlerCallRef myHandler, EventRef event)
{
	OSStatus			result = eventNotHandledErr;
	UInt32 				evtClass = GetEventClass (event);
	UInt32 				evtKind = GetEventKind (event);

	// Always handle command events, even in hands-off mode.
	if((evtClass == kEventClassCommand) && (evtKind == kEventCommandProcess))
	{
		HICommand command;
		GetEventParameter (event, kEventParamDirectObject, typeHICommand, NULL, sizeof(command), NULL, &command);

		switch(command.commandID)
		{
		case kHICommandQuit:
			if(mCallbacks->handleCloseRequest(this))
			{
				// Get the app to initiate cleanup.
				mCallbacks->handleQuit(this);
				// The app is responsible for calling destroyWindow when done with GL
			}
			result = noErr;
			break;

		default:
			// MBW -- XXX -- Should we handle other events here?
			break;
		}
	}

	if(mHandsOffEvents)
	{
		return(result);
	}

	switch (evtClass)
	{
	case kEventClassTextInput:
		{
			switch (evtKind)
			{
			case kEventTextInputUpdateActiveInputArea:
				{
					EventParamType param_type;

					long fix_len;
					UInt32 text_len;
					if (mPreeditor
						&& (result = GetEventParameter(event, kEventParamTextInputSendFixLen,
										typeLongInteger, &param_type, sizeof(fix_len), NULL, &fix_len)) == noErr
						&& typeLongInteger == param_type
						&& (result = GetEventParameter(event, kEventParamTextInputSendText,
										typeUnicodeText, &param_type, 0, &text_len, NULL)) == noErr
						&& typeUnicodeText == param_type)
					{
						// Handle an optional (but essential to facilitate TSMDA) ReplaceRange param.
						CFRange range;
						if (GetEventParameter(event, kEventParamTextInputSendReplaceRange,
								typeCFRange, &param_type, sizeof(range), NULL, &range) == noErr
							&& typeCFRange == param_type)
						{
							// Although the spec. is unclear, replace range should
							// not present when there is an active preedit.  We just
							// ignore the case.  markAsPreedit will detect the case and warn it.
							const LLWString & text = mPreeditor->getPreeditString();
							const S32 location = wstring_wstring_length_from_utf16_length(text, 0, range.location);
							const S32 length = wstring_wstring_length_from_utf16_length(text, location, range.length);
							mPreeditor->markAsPreedit(location, length);
						}
						mPreeditor->resetPreedit();

						// Receive the text from input method.
						U16 *const text = new U16[text_len / sizeof(U16)];
						GetEventParameter(event, kEventParamTextInputSendText, typeUnicodeText, NULL, text_len, NULL, text);
						if (fix_len < 0)
						{
							// Do we still need this?  Seems obsolete...
							fix_len = text_len;
						}
						const LLWString fix_string
								= utf16str_to_wstring(llutf16string(text, fix_len / sizeof(U16)));
						const LLWString preedit_string
								= utf16str_to_wstring(llutf16string(text + fix_len / sizeof(U16), (text_len - fix_len) / sizeof(U16)));
						delete[] text;

						// Handle fixed (comitted) string.
						if (fix_string.length() > 0)
						{
							for (LLWString::const_iterator i = fix_string.begin(); i != fix_string.end(); i++)
							{
								mPreeditor->handleUnicodeCharHere(*i);
							}
						}

						// Receive the segment info and caret position.
						LLPreeditor::segment_lengths_t preedit_segment_lengths;
						LLPreeditor::standouts_t preedit_standouts;
						S32 caret_position = preedit_string.length();
						UInt32 text_range_array_size;
						if (GetEventParameter(event, kEventParamTextInputSendHiliteRng, typeTextRangeArray,
								&param_type, 0, &text_range_array_size, NULL) == noErr
							&& typeTextRangeArray == param_type
							&& text_range_array_size > sizeof(TextRangeArray))
						{
							// TextRangeArray is a variable-length struct.
							TextRangeArray * const text_range_array = (TextRangeArray *) new char[text_range_array_size];
							GetEventParameter(event, kEventParamTextInputSendHiliteRng, typeTextRangeArray,
									NULL, text_range_array_size, NULL, text_range_array);

							// WARNING: We assume ranges are in ascending order,
							// although the condition is undocumented.  It seems
							// OK to assume this.  I also assumed
							// the ranges are contiguous in previous versions, but I
							// have heard a rumore that older versions os ATOK may
							// return ranges with some _gap_.  I don't know whether
							// it is true, but I'm preparing my code for the case.

							const S32 ranges = text_range_array->fNumOfRanges;
							preedit_segment_lengths.reserve(ranges);
							preedit_standouts.reserve(ranges);

							S32 last_bytes = 0;
							S32 last_utf32 = 0;
							for (S32 i = 0; i < ranges; i++)
							{
								const TextRange &range = text_range_array->fRange[i];
								if (range.fStart > last_bytes)
								{
									const S32 length_utf16 = (range.fStart - last_bytes) / sizeof(U16);
									const S32 length_utf32 = wstring_wstring_length_from_utf16_length(preedit_string, last_utf32, length_utf16);
									preedit_segment_lengths.push_back(length_utf32);
									preedit_standouts.push_back(FALSE);
									last_utf32 += length_utf32;
								}
								if (range.fEnd > range.fStart)
								{
									const S32 length_utf16 = (range.fEnd - range.fStart) / sizeof(U16);
									const S32 length_utf32 = wstring_wstring_length_from_utf16_length(preedit_string, last_utf32, length_utf16);
									preedit_segment_lengths.push_back(length_utf32);
									preedit_standouts.push_back(
										kTSMHiliteSelectedRawText == range.fHiliteStyle
										|| kTSMHiliteSelectedConvertedText ==  range.fHiliteStyle
										|| kTSMHiliteSelectedText == range.fHiliteStyle);
									last_utf32 += length_utf32;
								}
								if (kTSMHiliteCaretPosition == range.fHiliteStyle)
								{
									caret_position = last_utf32;
								}
								last_bytes = range.fEnd;
							}
							if (preedit_string.length() > last_utf32)
							{
								preedit_segment_lengths.push_back(preedit_string.length() - last_utf32);
								preedit_standouts.push_back(FALSE);
							}

							delete[] (char *) text_range_array;
						}

						// Handle preedit string.
						if (preedit_string.length() == 0)
						{
							preedit_segment_lengths.clear();
							preedit_standouts.clear();
						}
						else if (preedit_segment_lengths.size() == 0)
						{
							preedit_segment_lengths.push_back(preedit_string.length());
							preedit_standouts.push_back(FALSE);
						}
						mPreeditor->updatePreedit(preedit_string, preedit_segment_lengths, preedit_standouts, caret_position);

						result = noErr;
					}
				}
				break;

			case kEventTextInputUnicodeForKeyEvent:
				{
					UInt32 modifiers = 0;


					// First, process the raw event.
					{
						EventRef rawEvent = NULL;

						// Get the original event and extract the modifier keys, so we can ignore command-key events.
						if (GetEventParameter(event, kEventParamTextInputSendKeyboardEvent, typeEventRef, NULL, sizeof(rawEvent), NULL, &rawEvent) == noErr)
						{
							// Grab the modifiers for later use in this function...
							GetEventParameter (rawEvent, kEventParamKeyModifiers, typeUInt32, NULL, sizeof(UInt32), NULL, &modifiers);

							// and call this function recursively to handle the raw key event.
							eventHandler (myHandler, rawEvent);

							// save the raw event until we're done processing the unicode input as well.
							mRawKeyEvent = rawEvent;
						}
					}

					OSStatus err = noErr;
					EventParamType actualType = typeUnicodeText;
					UInt32 actualSize = 0;
					size_t actualCount = 0;
					U16 *buffer = NULL;

					// Get the size of the unicode data
					err = GetEventParameter (event, kEventParamTextInputSendText, typeUnicodeText, &actualType, 0, &actualSize, NULL);
					if(err == noErr)
					{
						// allocate a buffer and get the actual data.
						actualCount = actualSize / sizeof(U16);
						buffer = new U16[actualCount];
						err = GetEventParameter (event, kEventParamTextInputSendText, typeUnicodeText, &actualType, actualSize, &actualSize, buffer);
					}

					if(err == noErr)
					{
						if(modifiers & (cmdKey | controlKey))
						{
							// This was a menu key equivalent.  Ignore it.
						}
						else
						{
							MASK mask = LLWindowMacOSX::modifiersToMask(modifiers);

							llassert( actualType == typeUnicodeText );

							// The result is a UTF16 buffer.  Pass the characters in turn to handleUnicodeChar.

							// Convert to UTF32 and go character-by-character.
							llutf16string utf16(buffer, actualCount);
							LLWString utf32 = utf16str_to_wstring(utf16);
							LLWString::iterator iter;

							for(iter = utf32.begin(); iter != utf32.end(); iter++)
							{
								mCallbacks->handleUnicodeChar(*iter, mask);
							}
						}
					}

					if(buffer != NULL)
					{
						delete[] buffer;
					}

					mRawKeyEvent = NULL;
					result = err;
				}
				break;

			case kEventTextInputOffsetToPos:
				{
					EventParamType param_type;
					long offset;
					if (mPreeditor
						&& GetEventParameter(event, kEventParamTextInputSendTextOffset, typeLongInteger,
								&param_type, sizeof(offset), NULL, &offset) == noErr
						&& typeLongInteger == param_type)
					{
						S32 preedit, preedit_length;
						mPreeditor->getPreeditRange(&preedit, &preedit_length);
						const LLWString & text = mPreeditor->getPreeditString();

						LLCoordGL caret_coord;
						LLRect preedit_bounds;
						if (0 <= offset
							&& mPreeditor->getPreeditLocation(wstring_wstring_length_from_utf16_length(text, preedit, offset / sizeof(U16)),
															  &caret_coord, &preedit_bounds, NULL))
						{
							LLCoordGL caret_base_coord(caret_coord.mX, preedit_bounds.mBottom);
							LLCoordScreen caret_base_coord_screen;
							convertCoords(caret_base_coord, &caret_base_coord_screen);
							Point qd_point;
							qd_point.h = caret_base_coord_screen.mX;
							qd_point.v = caret_base_coord_screen.mY;
							SetEventParameter(event, kEventParamTextInputReplyPoint, typeQDPoint, sizeof(qd_point), &qd_point);

							short line_height = (short) preedit_bounds.getHeight();
							SetEventParameter(event, kEventParamTextInputReplyLineHeight, typeShortInteger, sizeof(line_height), &line_height);

							result = noErr;
						}
						else
						{
							result = errOffsetInvalid;
						}
					}
				}
				break;

			case kEventTextInputGetSelectedText:
				{
					if (mPreeditor)
					{
						S32 selection, selection_length;
						mPreeditor->getSelectionRange(&selection, &selection_length);
						if (selection_length)
						{
							const LLWString text = mPreeditor->getPreeditString().substr(selection, selection_length);
							const llutf16string text_utf16 = wstring_to_utf16str(text);
							result = SetEventParameter(event, kEventParamTextInputReplyText, typeUnicodeText,
										text_utf16.length() * sizeof(U16), text_utf16.c_str());
						}
					}
				}
				break;
			}
		}
		break;

	case kEventClassKeyboard:
		{
			UInt32 keyCode = 0;
			char charCode = 0;
			UInt32 modifiers = 0;

			// Some of these may fail for some event types.  That's fine.
			GetEventParameter (event, kEventParamKeyCode, typeUInt32, NULL, sizeof(UInt32), NULL, &keyCode);
			GetEventParameter (event, kEventParamKeyModifiers, typeUInt32, NULL, sizeof(UInt32), NULL, &modifiers);

			// save the raw event so getNativeKeyData can use it.
			mRawKeyEvent = event;

			//			printf("key event, key code = 0x%08x, char code = 0x%02x (%c), modifiers = 0x%08x\n", keyCode, charCode, (char)charCode, modifiers);
			//			fflush(stdout);

			switch (evtKind)
			{
			case kEventRawKeyDown:
			case kEventRawKeyRepeat:
				if (gDebugWindowProc)
				{
					printf("key down, key code = 0x%08x, char code = 0x%02x (%c), modifiers = 0x%08x\n",
							(unsigned int)keyCode, charCode, (char)charCode, (unsigned int)modifiers);
					fflush(stdout);
				}
				gKeyboard->handleKeyDown(keyCode, modifiers);
				result = eventNotHandledErr;
				break;

			case kEventRawKeyUp:
				if (gDebugWindowProc)
				{
					printf("key up,   key code = 0x%08x, char code = 0x%02x (%c), modifiers = 0x%08x\n",
							(unsigned int)keyCode, charCode, (char)charCode, (unsigned int)modifiers);
					fflush(stdout);
				}
				gKeyboard->handleKeyUp(keyCode, modifiers);
				result = eventNotHandledErr;
				break;

			case kEventRawKeyModifiersChanged:
				// The keyboard input system wants key up/down events for modifier keys.
				// Mac OS doesn't supply these directly, but can supply events when the collective modifier state changes.
				// Use these events to generate up/down events for the modifiers.

				if((modifiers & shiftKey) && !(mLastModifiers & shiftKey))
				{
					if (gDebugWindowProc) printf("Shift key down event\n");
					gKeyboard->handleKeyDown(0x38, (modifiers & 0x00FFFFFF) | ((0x38 << 24) & 0xFF000000));
				}
				else if(!(modifiers & shiftKey) && (mLastModifiers & shiftKey))
				{
					if (gDebugWindowProc) printf("Shift key up event\n");
					gKeyboard->handleKeyUp(0x38, (modifiers & 0x00FFFFFF) | ((0x38 << 24) & 0xFF000000));
				}

				if((modifiers & alphaLock) && !(mLastModifiers & alphaLock))
				{
					if (gDebugWindowProc) printf("Caps lock down event\n");
					gKeyboard->handleKeyDown(0x39, (modifiers & 0x00FFFFFF) | ((0x39 << 24) & 0xFF000000));
				}
				else if(!(modifiers & alphaLock) && (mLastModifiers & alphaLock))
				{
					if (gDebugWindowProc) printf("Caps lock up event\n");
					gKeyboard->handleKeyUp(0x39, (modifiers & 0x00FFFFFF) | ((0x39 << 24) & 0xFF000000));
				}

				if((modifiers & controlKey) && !(mLastModifiers & controlKey))
				{
					if (gDebugWindowProc) printf("Control key down event\n");
					gKeyboard->handleKeyDown(0x3b, (modifiers & 0x00FFFFFF) | ((0x3b << 24) & 0xFF000000));
				}
				else if(!(modifiers & controlKey) && (mLastModifiers & controlKey))
				{
					if (gDebugWindowProc) printf("Control key up event\n");
					gKeyboard->handleKeyUp(0x3b, (modifiers & 0x00FFFFFF) | ((0x3b << 24) & 0xFF000000));
				}

				if((modifiers & optionKey) && !(mLastModifiers & optionKey))
				{
					if (gDebugWindowProc) printf("Option key down event\n");
					gKeyboard->handleKeyDown(0x3a, (modifiers & 0x00FFFFFF) | ((0x3a << 24) & 0xFF000000));
				}
				else if(!(modifiers & optionKey) && (mLastModifiers & optionKey))
				{
					if (gDebugWindowProc) printf("Option key up event\n");
					gKeyboard->handleKeyUp(0x3a, (modifiers & 0x00FFFFFF) | ((0x3a << 24) & 0xFF000000));
				}

				// When the state of the 'Fn' key (the one that changes some of the mappings on a powerbook/macbook keyboard
				// to an embedded keypad) changes, it may subsequently cause a key up event to be lost, which may lead to
				// a movement key getting "stuck" down.  This is bad.
				// This is an OS bug -- even the GetKeys() API doesn't tell you the key has been released.
				// This workaround causes all held-down keys to be reset whenever the state of the Fn key changes.  This isn't
				// exactly what we want, but it does avoid the case where you get stuck running forward.
				if((modifiers & kEventKeyModifierFnMask) != (mLastModifiers & kEventKeyModifierFnMask))
				{
					if (gDebugWindowProc) printf("Fn key state change event\n");
					gKeyboard->resetKeys();
				}

				if (gDebugWindowProc) fflush(stdout);

				mLastModifiers = modifiers;
				result = eventNotHandledErr;
				break;
			}

			mRawKeyEvent = NULL;
		}
		break;

	case kEventClassMouse:
		{
			result = CallNextEventHandler(myHandler, event);
			if (eventNotHandledErr == result)
			{ // only handle events not already handled (prevents wierd resize interaction)
				EventMouseButton	button = kEventMouseButtonPrimary;
				HIPoint				location = {0.0f, 0.0f};
				UInt32				modifiers = 0;
				UInt32				clickCount = 1;
				long				wheelDelta = 0;
				LLCoordScreen		inCoords;
				LLCoordGL			outCoords;
				MASK				mask = 0;

				GetEventParameter(event, kEventParamMouseButton, typeMouseButton, NULL, sizeof(button), NULL, &button);
				GetEventParameter(event, kEventParamMouseLocation, typeHIPoint, NULL, sizeof(location), NULL, &location);
				GetEventParameter(event, kEventParamKeyModifiers, typeUInt32, NULL, sizeof(modifiers), NULL, &modifiers);
				GetEventParameter(event, kEventParamMouseWheelDelta, typeLongInteger, NULL, sizeof(wheelDelta), NULL, &wheelDelta);
				GetEventParameter(event, kEventParamClickCount, typeUInt32, NULL, sizeof(clickCount), NULL, &clickCount);

				inCoords.mX = llround(location.x);
				inCoords.mY = llround(location.y);

				if(modifiers & shiftKey) { mask |= MASK_SHIFT; }
				if(modifiers & controlKey) { mask |= MASK_CONTROL; }
				if(modifiers & optionKey) { mask |= MASK_ALT; }

				if(mCursorDecoupled)
				{
					CGMouseDelta x, y;

					// If the cursor's decoupled, we need to read the latest movement delta as well.
					CGGetLastMouseDelta( &x, &y );
					mCursorLastEventDeltaX = x;
					mCursorLastEventDeltaY = y;

					if(mCursorIgnoreNextDelta)
					{
						mCursorLastEventDeltaX = 0;
						mCursorLastEventDeltaY = 0;
						mCursorIgnoreNextDelta = FALSE;
					}
				}
				else
				{
					mCursorLastEventDeltaX = 0;
					mCursorLastEventDeltaY = 0;
				}

				inCoords.mX += mCursorLastEventDeltaX;
				inCoords.mY += mCursorLastEventDeltaY;

				convertCoords(inCoords, &outCoords);

				//				printf("coords in: %d, %d; coords out: %d, %d\n", inCoords.mX, inCoords.mY, outCoords.mX, outCoords.mY);
				//				fflush(stdout);


				switch (evtKind)
				{
				case kEventMouseDown:
					if (mLanguageTextInputAllowed)
					{
						// We need to interrupt before handling mouse events,
						// so that the fixed string from IM are delivered to
						// the currently focused UI component.
						interruptLanguageTextInput();
					}
					switch(button)
					{
					case kEventMouseButtonPrimary:
						if(modifiers & cmdKey)
						{
							// Simulate a right click
							mSimulatedRightClick = true;
							mCallbacks->handleRightMouseDown(this, outCoords, mask);
						}
						else if(clickCount == 2)
						{
							// Windows double-click events replace the second mousedown event in a double-click.
							mCallbacks->handleDoubleClick(this, outCoords, mask);
						}
						else
						{
							mCallbacks->handleMouseDown(this, outCoords, mask);
						}
						break;
					case kEventMouseButtonSecondary:
						mCallbacks->handleRightMouseDown(this, outCoords, mask);
						break;

					case kEventMouseButtonTertiary:
						mCallbacks->handleMiddleMouseDown(this, outCoords, mask);
						break;
					}
					result = noErr;
					break;
				case kEventMouseUp:

					switch(button)
					{
					case kEventMouseButtonPrimary:
						if(mSimulatedRightClick)
						{
							// End of simulated right click
							mSimulatedRightClick = false;
							mCallbacks->handleRightMouseUp(this, outCoords, mask);
						}
						else
						{
							mCallbacks->handleMouseUp(this, outCoords, mask);
						}
						break;
					case kEventMouseButtonSecondary:
						mCallbacks->handleRightMouseUp(this, outCoords, mask);
						break;

					case kEventMouseButtonTertiary:
						mCallbacks->handleMiddleMouseUp(this, outCoords, mask);
						break;
					}
					result = noErr;
					break;

				case kEventMouseWheelMoved:
					{
						static S32  z_delta = 0;

						z_delta += wheelDelta;

						if (z_delta <= -WHEEL_DELTA || WHEEL_DELTA <= z_delta)
						{
							mCallbacks->handleScrollWheel(this, -z_delta / WHEEL_DELTA);
							z_delta = 0;
						}
					}
					result = noErr;
					break;

				case kEventMouseDragged:
				case kEventMouseMoved:
					mCallbacks->handleMouseMove(this, outCoords, mask);
					result = noErr;
					break;

				}
			}
		}
		break;

	case kEventClassWindow:
		switch(evtKind)
		{
		case kEventWindowActivated:
			if (mTSMDocument)
			{
				ActivateTSMDocument(mTSMDocument);
			}
			mCallbacks->handleFocus(this);
			break;
		case kEventWindowDeactivated:
			if (mTSMDocument)
			{
				DeactivateTSMDocument(mTSMDocument);
			}
			mCallbacks->handleFocusLost(this);
			break;

		case kEventWindowBoundsChanging:
			{
				// This is where we would constrain move/resize to a particular screen

				const S32 MIN_WIDTH  = mMinWindowWidth;
				const S32 MIN_HEIGHT = mMinWindowHeight;

				Rect currentBounds;
				Rect previousBounds;

				GetEventParameter(event, kEventParamCurrentBounds, typeQDRectangle, NULL, sizeof(Rect), NULL, &currentBounds);
				GetEventParameter(event, kEventParamPreviousBounds, typeQDRectangle, NULL, sizeof(Rect), NULL, &previousBounds);

				// Put an offset into window un-maximize operation since the kEventWindowGetIdealSize
				// event only allows the specification of size and not position.
				if (mMaximized)
				{
					short leftOffset = mPreviousWindowRect.left - currentBounds.left;
					currentBounds.left += leftOffset;
					currentBounds.right += leftOffset;

					short topOffset = mPreviousWindowRect.top - currentBounds.top;
					currentBounds.top += topOffset;
					currentBounds.bottom += topOffset;
				}
				else
				{
					// Store off the size for future un-maximize operations
					mPreviousWindowRect = previousBounds;
				}

				if ((currentBounds.right - currentBounds.left) < MIN_WIDTH)
				{
					currentBounds.right = currentBounds.left + MIN_WIDTH;
				}

				if ((currentBounds.bottom - currentBounds.top) < MIN_HEIGHT)
				{
					currentBounds.bottom = currentBounds.top + MIN_HEIGHT;
				}

				SetEventParameter(event, kEventParamCurrentBounds, typeQDRectangle, sizeof(Rect), &currentBounds);
				result = noErr;
			}
			break;

		case kEventWindowBoundsChanged:
			{
				// Get new window bounds
				Rect newBounds;
				GetEventParameter(event, kEventParamCurrentBounds, typeQDRectangle, NULL, sizeof(Rect), NULL, &newBounds);

				// Get previous window bounds
				Rect oldBounds;
				GetEventParameter(event, kEventParamPreviousBounds, typeQDRectangle, NULL, sizeof(Rect), NULL, &oldBounds);

				// Determine if the new size is larger than the old
				bool newBoundsLarger = ((newBounds.right - newBounds.left) >= (oldBounds.right - oldBounds.left));
				newBoundsLarger &= ((newBounds.bottom - newBounds.top) >= (oldBounds.bottom - oldBounds.top));

				// Check to see if this is a zoom event (+ button on window pane)
				unsigned int eventParams;
				GetEventParameter(event, kEventParamAttributes, typeUInt32, NULL, sizeof(int), NULL, &eventParams);
				bool isZoomEvent = ((eventParams & kWindowBoundsChangeZoom) != 0);

				// Maximized flag is if zoom event and increasing window size
				mMaximized = (isZoomEvent && newBoundsLarger);

				aglUpdateContext(mContext);

				mCallbacks->handleResize(this, newBounds.right - newBounds.left, newBounds.bottom - newBounds.top);
			}
			break;

		case kEventWindowGetIdealSize:
			// Only recommend a new ideal size when un-maximizing
			if (mMaximized == TRUE)
			{
				Point nonMaximizedSize;

				nonMaximizedSize.v = mPreviousWindowRect.bottom - mPreviousWindowRect.top;
				nonMaximizedSize.h = mPreviousWindowRect.right - mPreviousWindowRect.left;

				SetEventParameter(event, kEventParamDimensions, typeQDPoint, sizeof(Point), &nonMaximizedSize);
				result = noErr;
			}
			break;

		case kEventWindowClose:
			if(mCallbacks->handleCloseRequest(this))
			{
				// Get the app to initiate cleanup.
				mCallbacks->handleQuit(this);
				// The app is responsible for calling destroyWindow when done with GL
			}
			result = noErr;
			break;

		case kEventWindowHidden:
			//					llinfos << "LLWindowMacOSX: Deactivating on hide" << llendl;
			mMinimized = TRUE;
			mCallbacks->handleActivate(this, false);
			//					result = noErr;
			break;

		case kEventWindowShown:
			//					llinfos << "LLWindowMacOSX: Activating on show" << llendl;
			mMinimized = FALSE;
			mCallbacks->handleActivate(this, true);
			//					result = noErr;
			break;

		case kEventWindowCollapsed:
			//					llinfos << "LLWindowMacOSX: Deactivating on collapse" << llendl;
			mMinimized = TRUE;
			mCallbacks->handleActivate(this, false);
			//					result = noErr;
			break;

		case kEventWindowExpanded:
			//					llinfos << "LLWindowMacOSX: Activating on expand" << llendl;
			mMinimized = FALSE;
			mCallbacks->handleActivate(this, true);
			//					result = noErr;
			break;

		case kEventWindowGetClickActivation:
			//					BringToFront(mWindow);
			//					result = noErr;
			break;
		}
		break;

	case kEventClassTSMDocumentAccess:
		if (mPreeditor)
		{
			switch(evtKind)
			{

			case kEventTSMDocumentAccessGetLength:
				{
					// Return the number of UTF-16 units in the text, excluding those for preedit.

					S32 preedit, preedit_length;
					mPreeditor->getPreeditRange(&preedit, &preedit_length);
					const LLWString & text = mPreeditor->getPreeditString();
					const CFIndex length = wstring_utf16_length(text, 0, preedit)
						+ wstring_utf16_length(text, preedit + preedit_length, text.length());
					result = SetEventParameter(event, kEventParamTSMDocAccessCharacterCount, typeCFIndex, sizeof(length), &length);
				}
				break;

			case kEventTSMDocumentAccessGetSelectedRange:
				{
					// Return the selected range, excluding preedit.
					// In our preeditor, preedit and selection are exclusive, so,
					// when it has a preedit, there is no selection and the
					// insertion point is on the preedit that corrupses into the
					// beginning of the preedit when the preedit was removed.

					S32 preedit, preedit_length;
					mPreeditor->getPreeditRange(&preedit, &preedit_length);
					const LLWString & text = mPreeditor->getPreeditString();

					CFRange range;
					if (preedit_length)
					{
						range.location = wstring_utf16_length(text, 0, preedit);
						range.length = 0;
					}
					else
					{
						S32 selection, selection_length;
						mPreeditor->getSelectionRange(&selection, &selection_length);
						range.location = wstring_utf16_length(text, 0, selection);
						range.length = wstring_utf16_length(text, selection, selection_length);
					}

					result = SetEventParameter(event, kEventParamTSMDocAccessReplyCharacterRange, typeCFRange, sizeof(range), &range);
				}
				break;

			case kEventTSMDocumentAccessGetCharacters:
				{
					UniChar *target_pointer;
					CFRange range;
					EventParamType param_type;
					if ((result = GetEventParameter(event, kEventParamTSMDocAccessSendCharacterRange,
										typeCFRange, &param_type, sizeof(range), NULL, &range)) == noErr
						&& typeCFRange == param_type
						&& (result = GetEventParameter(event, kEventParamTSMDocAccessSendCharactersPtr,
										typePtr, &param_type, sizeof(target_pointer), NULL, &target_pointer)) == noErr
						&& typePtr == param_type)
					{
						S32 preedit, preedit_length;
						mPreeditor->getPreeditRange(&preedit, &preedit_length);
						const LLWString & text = mPreeditor->getPreeditString();

						// The GetCharacters event of TSMDA has a fundamental flaw;
						// An input method need to decide the starting offset and length
						// *before* it actually see the contents, so it is impossible
						// to guarantee the character-aligned access.  The event reply
						// has no way to indicate a condition something like "Request
						// was not fulfilled due to unaligned access.  Please retry."
						// Any error sent back to the input method stops use of TSMDA
						// entirely during the session...
						// We need to simulate very strictly the behaviour as if the
						// underlying *text engine* holds the contents in UTF-16.
						// I guess this is the reason why Apple repeats saying "all
						// text handling application should use UTF-16."  They are
						// trying to _fix_ the flaw by changing the appliations...
						// ... or, domination of UTF-16 in the industry may be a part
						// of the company vision, and Apple is trying to force third
						// party developers to obey their vision.  Remember that use
						// of 16 bits per _a_character_ was one of the very fundamental
						// Unicode design policy on its early days (during late 80s)
						// and the original Unicode design was by two Apple employees...

						const llutf16string text_utf16
							= wstring_to_utf16str(text, preedit)
							+ wstring_to_utf16str(text.substr(preedit + preedit_length));

						llassert_always(sizeof(U16) == sizeof(UniChar));
						llassert(0 <= range.location && 0 <= range.length && range.location + range.length <= text_utf16.length());
						memcpy(target_pointer, text_utf16.c_str() + range.location, range.length * sizeof(UniChar));

						// Note that result has already been set above.
					}
				}
				break;

			}
		}
		break;
	}
	return result;
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
	OSStatus result = noErr;

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
			::HideCursor();
		}
		break;

		// MBW -- XXX -- Some of the standard Windows cursors have no standard Mac equivalents.
		//    Find out what they look like and replicate them.

		// These are essentially correct
	case UI_CURSOR_WAIT:		SetThemeCursor(kThemeWatchCursor);	break;
	case UI_CURSOR_IBEAM:		SetThemeCursor(kThemeIBeamCursor);	break;
	case UI_CURSOR_CROSS:		SetThemeCursor(kThemeCrossCursor);	break;
	case UI_CURSOR_HAND:		SetThemeCursor(kThemePointingHandCursor);	break;
		//		case UI_CURSOR_NO:			SetThemeCursor(kThemeNotAllowedCursor);	break;
	case UI_CURSOR_ARROWCOPY:   SetThemeCursor(kThemeCopyArrowCursor);	break;

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
		::HideCursor();
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
		::ShowCursor();
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
	OSStatus err;

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

		if(string != NULL)
		{
			ControlRef progressText = NULL;
			ControlID id;
			OSStatus err;

			id.signature = 'what';
			id.id = 0;

			err = GetControlByID(mWindow, &id, &progressText);
			if(err == noErr)
			{
				err = SetControlData(progressText, kControlEntireControl, kControlStaticTextCFStringTag, sizeof(CFStringRef), (Ptr)&string);
				Draw1Control(progressText);
			}

			CFRelease(string);
		}
	}
}


void LLSplashScreenMacOSX::hideImpl()
{
	if(mWindow != NULL)
	{
		DisposeWindow(mWindow);
		mWindow = NULL;
	}
}



S32 OSMessageBoxMacOSX(const std::string& text, const std::string& caption, U32 type)
{
	S32 result = OSBTN_CANCEL;
	SInt16 retval_mac = 1;
	AlertStdCFStringAlertParamRec params;
	CFStringRef errorString = NULL;
	CFStringRef explanationString = NULL;
	DialogRef alert = NULL;
	AlertType alertType = kAlertCautionAlert;
	OSStatus err;

	explanationString = CFStringCreateWithCString(NULL, text.c_str(), kCFStringEncodingUTF8);
	errorString = CFStringCreateWithCString(NULL, caption.c_str(), kCFStringEncodingUTF8);

	params.version = kStdCFStringAlertVersionOne;
	params.movable = false;
	params.helpButton = false;
	params.defaultText = (CFStringRef)kAlertDefaultOKText;
	params.cancelText = 0;
	params.otherText = 0;
	params.defaultButton = 1;
	params.cancelButton = 0;
	params.position = kWindowDefaultPosition;
	params.flags = 0;

	switch(type)
	{
	case OSMB_OK:
	default:
		break;
	case OSMB_OKCANCEL:
		params.cancelText = (CFStringRef)kAlertDefaultCancelText;
		params.cancelButton = 2;
		break;
	case OSMB_YESNO:
		alertType = kAlertNoteAlert;
		params.defaultText = CFSTR("Yes");
		params.cancelText = CFSTR("No");
		params.cancelButton = 2;
		break;
	}

	if(gWindowImplementation != NULL)
		gWindowImplementation->beforeDialog();

	err = CreateStandardAlert(
		alertType,
		errorString,
		explanationString,
		&params,
		&alert);

	if(err == noErr)
	{
		err = RunStandardAlert(
			alert,
			NULL,
			&retval_mac);
	}

	if(gWindowImplementation != NULL)
		gWindowImplementation->afterDialog();

	switch(type)
	{
	case OSMB_OK:
	case OSMB_OKCANCEL:
	default:
		if(retval_mac == 1)
			result = OSBTN_OK;
		else
			result = OSBTN_CANCEL;
		break;
	case OSMB_YESNO:
		if(retval_mac == 1)
			result = OSBTN_YES;
		else
			result = OSBTN_NO;
		break;
	}

	if(errorString != NULL)
	{
		CFRelease(errorString);
	}

	if(explanationString != NULL)
	{
		CFRelease(explanationString);
	}

	return result;
}

// Open a URL with the user's default web browser.
// Must begin with protocol identifier.
void LLWindowMacOSX::spawnWebBrowser(const std::string& escaped_url, bool async)
{
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

	OSStatus result = noErr;
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
		OSStatus err = noErr;
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


	lldebugs << "native key data is: " << result << llendl;

	return result;
}


BOOL LLWindowMacOSX::dialogColorPicker( F32 *r, F32 *g, F32 *b)
{
	BOOL	retval = FALSE;
	OSErr	error = noErr;
	NColorPickerInfo	info;

	memset(&info, 0, sizeof(info));
	info.theColor.color.rgb.red = (UInt16)(*r * 65535.f);
	info.theColor.color.rgb.green = (UInt16)(*g * 65535.f);
	info.theColor.color.rgb.blue = (UInt16)(*b * 65535.f);
	info.placeWhere = kCenterOnMainScreen;

	if(gWindowImplementation != NULL)
		gWindowImplementation->beforeDialog();

	error = NPickColor(&info);

	if(gWindowImplementation != NULL)
		gWindowImplementation->afterDialog();

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

void *LLWindowMacOSX::getMediaWindow()
{
	/*
		Mozilla needs to be initialized with a WindowRef to function properly.
		(There's no good reason for this, since it shouldn't be interacting with our window in any way, but that's another issue.)
		If we're in windowed mode, we _could_ hand it our actual window pointer, but a subsequent switch to fullscreen will destroy that window,
		which trips up Mozilla.
		Instead of using our actual window, we create an invisible window which will persist for the lifetime of the application and pass that to Mozilla.
		This satisfies its deep-seated need to latch onto a WindowRef and solves the issue with switching between fullscreen and windowed modes.

		Note that we will never destroy this window (by design!), but since only one will ever be created per run of the application, that's okay.
	*/

	if(sMediaWindow == NULL)
	{
		Rect window_rect = {100, 100, 200, 200};

		sMediaWindow = NewCWindow(
			NULL,
			&window_rect,
			(ConstStr255Param) "\p",
			false,				// Create the window invisible.
			zoomDocProc,		// Window with a grow box and a zoom box
			kLastWindowOfClass,		// create it behind other windows
			false,					// no close box
			0);
	}

	return (void*)sMediaWindow;
}

void LLWindowMacOSX::stopDockTileBounce()
{
	NMRemove(&mBounceRec);
	mBounceTimer.stop();
}

// get a double value from a dictionary
static double getDictDouble (CFDictionaryRef refDict, CFStringRef key)
{
	double double_value;
	CFNumberRef number_value = (CFNumberRef) CFDictionaryGetValue(refDict, key);
	if (!number_value) // if can't get a number for the dictionary
		return -1;  // fail
	if (!CFNumberGetValue(number_value, kCFNumberDoubleType, &double_value)) // or if cant convert it
		return -1; // fail
	return double_value; // otherwise return the long value
}

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
	ScriptLanguageRecord script_language;

	if (preeditor != mPreeditor && !b)
	{
		// This condition may occur by a call to
		// setEnabled(BOOL) against LLTextEditor or LLLineEditor
		// when the control is not focused.
		// We need to silently ignore the case so that
		// the language input status of the focused control
		// is not disturbed.
		return;
	}

	UseInputWindow(mTSMDocument, !b);

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

	if (b)
	{
		if (mTSMScriptCode != smRoman)
		{
			script_language.fScript = mTSMScriptCode;
			script_language.fLanguage = mTSMLangCode;
			SetTextServiceLanguage(&script_language);
		}
	}
	else
	{
		GetTextServiceLanguage(&script_language);
		mTSMScriptCode = script_language.fScript;
		mTSMLangCode = script_language.fLanguage;
		if (mTSMScriptCode != smRoman)
		{
			script_language.fScript = smRoman;
			script_language.fLanguage = langEnglish;
			SetTextServiceLanguage(&script_language);
		}
	}
}

void LLWindowMacOSX::interruptLanguageTextInput()
{
	if (mTSMDocument)
	{
		FixTSMDocument(mTSMDocument);
	}
	// Don't we need to call resetPreedit here?
	// Well, if Apple's TSM document is correct, we don't.
}

//static
std::vector<std::string> LLWindowMacOSX::getDynamicFallbackFontList()
{
	// Fonts previously in getFontListSans() have moved to fonts.xml.
	return std::vector<std::string>();
}

// static
MASK LLWindowMacOSX::modifiersToMask(SInt16 modifiers)
{
	MASK mask = 0;
	if(modifiers & shiftKey) { mask |= MASK_SHIFT; }
	if(modifiers & (cmdKey | controlKey)) { mask |= MASK_CONTROL; }
	if(modifiers & optionKey) { mask |= MASK_ALT; }
	return mask;
}

#if LL_OS_DRAGDROP_ENABLED

OSErr LLWindowMacOSX::dragTrackingHandler(DragTrackingMessage message, WindowRef theWindow,
						  void * handlerRefCon, DragRef drag)
{
	OSErr result = noErr;
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

#endif // LL_OS_DRAGDROP_ENABLED
