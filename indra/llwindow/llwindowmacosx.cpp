/** 
 * @file llwindowmacosx.cpp
 * @brief Platform-dependent implementation of llwindow
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#if LL_DARWIN

#include "linden_common.h"

#include <Carbon/Carbon.h>

#include "llwindowmacosx.h"
#include "llkeyboardmacosx.h"
#include "llerror.h"
#include "llgl.h"
#include "llstring.h"
#include "lldir.h"

#include "llglheaders.h"

#include "indra_constants.h"

#include "llwindowmacosx-objc.h"

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

// Cross-platform bits:

void show_window_creation_error(const char* title)
{
	llwarns << title << llendl;
	shell_open( "help/window_creation_error.html");
	/*
	OSMessageBox(
	"Second Life is unable to run because it can't set up your display.\n"
	"We need to be able to make a 32-bit color window at 1024x768, with\n"
	"an 8 bit alpha channel.\n"
	"\n"
	"First, be sure your monitor is set to True Color (32-bit) in\n"
	"Start -> Control Panels -> Display -> Settings.\n"
	"\n"
	"Otherwise, this may be due to video card driver issues.\n"
	"Please make sure you have the latest video card drivers installed.\n"
	"ATI drivers are available at http://www.ati.com/\n"
	"nVidia drivers are available at http://www.nvidia.com/\n"
	"\n"
	"If you continue to receive this message, contact customer service.",
	title,
	OSMB_OK);
	*/
}

BOOL check_for_card(const char* RENDERER, const char* bad_card)
{
	if (!strnicmp(RENDERER, bad_card, strlen(bad_card)))
	{
		char buffer[1024];/* Flawfinder: ignore */
		snprintf(buffer, sizeof(buffer), /* Flawfinder: ignore */
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
		S32 button = OSMessageBox(buffer, "Unsupported video card", OSMB_YESNO);
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
	//	{ kEventClassWindow, kEventWindowCollapsing },
	//	{ kEventClassWindow, kEventWindowCollapsed },
	//	{ kEventClassWindow, kEventWindowShown },
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
	//	{ kEventClassWindow, kEventWindowZoomed },
	//	{ kEventClassWindow, kEventWindowDrawContent },

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
	{ kEventClassTextInput, kEventTextInputUnicodeForKeyEvent }

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
	{ kEventClassTextInput, kEventTextInputUnicodeForKeyEvent }
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



LLWindowMacOSX::LLWindowMacOSX(char *title, char *name, S32 x, S32 y, S32 width,
							   S32 height, U32 flags,
							   BOOL fullscreen, BOOL clearBg,
							   BOOL disable_vsync, BOOL use_gl,
							   BOOL ignore_pixel_depth)
	: LLWindow(fullscreen, flags)
{
	// Voodoo for calling cocoa from carbon (see llwindowmacosx-objc.mm).
	setupCocoa();
	
	// Initialize the keyboard
	gKeyboard = new LLKeyboardMacOSX();

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
	mMinimized = FALSE;
	
	// For reasons that aren't clear to me, LLTimers seem to be created in the "started" state.
	// Since the started state of this one is used to track whether the NMRec has been installed, it wants to start out in the "stopped" state.
	mBounceTimer.stop();	

	// Get the original aspect ratio of the main device.
	mOriginalAspectRatio = (double)CGDisplayPixelsWide(mDisplay) / (double)CGDisplayPixelsHigh(mDisplay);

	// Stash the window title
	strcpy((char*)mWindowTitle + 1, title); /* Flawfinder: ignore */
	mWindowTitle[0] = strlen(title);	/* Flawfinder: ignore */

	mEventHandlerUPP = NewEventHandlerUPP(staticEventHandler);
	mGlobalHandlerRef = NULL;
	mWindowHandlerRef = NULL;

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
		llinfos << "createContext: setting up fullscreen " << width << "x" << height << llendl;

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

				llinfos << "createContext: searching for a display mode, original aspect is " << mOriginalAspectRatio << llendl;

				for(i=0; i < resolutionCount; i++)
				{
					F32 aspect = (F32)resolutionList[i].mWidth / (F32)resolutionList[i].mHeight;

					llinfos << "createContext: width " << resolutionList[i].mWidth << " height " << resolutionList[i].mHeight << " aspect " << aspect << llendl;

					if( (resolutionList[i].mHeight >= 700) && (resolutionList[i].mHeight <= 800) &&
						(fabs(aspect - mOriginalAspectRatio) < fabs(closestAspect - mOriginalAspectRatio)))
					{
						llinfos << " (new closest mode) " << llendl;

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
				llinfos << "createContext: switching display resolution" << llendl;
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

			llinfos << "Running at " << mFullscreenWidth
				<< "x"   << mFullscreenHeight
				<< "x"   << mFullscreenBits
				<< " @ " << mFullscreenRefresh
				<< llendl;
		}
		else
		{
			// No fullscreen support
			mFullscreen = FALSE;
			mFullscreenWidth   = -1;
			mFullscreenHeight  = -1;
			mFullscreenBits    = -1;
			mFullscreenRefresh = -1;

			char error[256];	/* Flawfinder: ignore */
			snprintf(error, sizeof(error), "Unable to run fullscreen at %d x %d.\nRunning in window.", width, height);	/* Flawfinder: ignore */
			OSMessageBox(error, "Error", OSMB_OK);
		}
	}

	if(!mFullscreen && (mWindow == NULL))
	{
		Rect			window_rect;
		//int				displayWidth = CGDisplayPixelsWide(mDisplay);
		//int				displayHeight = CGDisplayPixelsHigh(mDisplay);
		//const int		menuBarPlusTitleBar = 44;   // Ugly magic number.

		llinfos << "createContext: creating window" << llendl;

		window_rect.left = (long) x;
		window_rect.right = (long) x + width;
		window_rect.top = (long) y;
		window_rect.bottom = (long) y + height;

		//-----------------------------------------------------------------------
		// Create the window
		//-----------------------------------------------------------------------
		mWindow = NewCWindow(
			NULL,
			&window_rect,
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
		InstallWindowEventHandler (mWindow, mEventHandlerUPP, GetEventTypeCount (WindowHandlerEventList), WindowHandlerEventList, (void*)this, &mWindowHandlerRef); // add event handler

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
						//			AGL_NO_RECOVERY,	// MBW -- XXX -- Not sure if we want this attribute
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

				llinfos << "createContext: creating fullscreen pixelformat" << llendl;

				GDHandle gdhDisplay = NULL;
				err = DMGetGDeviceByDisplayID ((DisplayIDType)mDisplay, &gdhDisplay, false);

				mPixelFormat = aglChoosePixelFormat(&gdhDisplay, 1, fullscreenAttrib);
				rendererInfo = aglQueryRendererInfo(&gdhDisplay, 1);
			}
			else
			{
				GLint windowedAttrib[] =
				{
					AGL_RGBA,
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

				llinfos << "createContext: creating windowed pixelformat" << llendl;

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
			llinfos << "createContext: creating GL context" << llendl;
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

		llinfos << "createContext: attaching fullscreen drawable" << llendl;

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
		llinfos << "createContext: attaching windowed drawable" << llendl;

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
		llinfos << "createContext: setting current context" << llendl;

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
		const S32 CARD_COUNT = sizeof(CARD_LIST)/sizeof(char*);

		// Future candidates:
		// ProSavage/Twister
		// SuperSavage

		S32 i;
		for (i = 0; i < CARD_COUNT; i++)
		{
			if (check_for_card(RENDERER, CARD_LIST[i]))
			{
				close();
				shell_open( "help/unsupported_card.html" );
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

	llinfos << "GL buffer: Color Bits " << S32(colorBits)
		<< " Alpha Bits " << S32(alphaBits)
		<< " Depth Bits " << S32(depthBits)
		<< " Stencil Bits" << S32(stencilBits)
		<< llendl;

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
		llinfos << "Disabling vertical sync" << llendl;
		frames_per_swap = 0;
	}
	else
	{
		llinfos << "Keeping vertical sync" << llendl;
		frames_per_swap = 1;
	}
	aglSetInteger(mContext, AGL_SWAP_INTERVAL, &frames_per_swap);  

	// Don't need to get the current gamma, since there's a call that restores it to the system defaults.
	return TRUE;
}


// changing fullscreen resolution, or switching between windowed and fullscreen mode.
BOOL LLWindowMacOSX::switchContext(BOOL fullscreen, LLCoordScreen size, BOOL disable_vsync)
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

			llinfos << "Switched resolution to " << mFullscreenWidth
				<< "x"   << mFullscreenHeight
				<< "x"   << mFullscreenBits
				<< " @ " << mFullscreenRefresh
				<< llendl;

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
	if(needsRebuild)
	{
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
		llinfos << "destroyContext: unhooking drawable " << llendl;

		aglSetCurrentContext (NULL);
		aglSetDrawable(mContext, NULL);
	}

	// Make sure the display resolution gets restored
	if(mOldDisplayMode != NULL)
	{
		llinfos << "destroyContext: restoring display resolution " << llendl;

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
		llinfos << "destroyContext: destroying pixel format " << llendl;
		aglDestroyPixelFormat(mPixelFormat);
		mPixelFormat = NULL;
	}

	// Remove any Carbon Event handlers we installed
	if(mGlobalHandlerRef != NULL)
	{
		llinfos << "destroyContext: removing global event handler" << llendl;
		RemoveEventHandler(mGlobalHandlerRef);
		mGlobalHandlerRef = NULL;
	}

	if(mWindowHandlerRef != NULL)
	{
		llinfos << "destroyContext: removing window event handler" << llendl;
		RemoveEventHandler(mWindowHandlerRef);
		mWindowHandlerRef = NULL;
	}

	// Close the window
	if(mWindow != NULL)
	{
		llinfos << "destroyContext: disposing window" << llendl;
		DisposeWindow(mWindow);
		mWindow = NULL;
	}

	// Clean up the GL context
	if(mContext != NULL)
	{
		llinfos << "destroyContext: destroying GL context" << llendl;
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

void LLWindowMacOSX::minimize()
{
	setMouseClipping(FALSE);
	showCursor();
	CollapseWindow(mWindow, true);
}

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
	BOOL result = FALSE;
	
	// Since the set of states where we want to act "minimized" is non-trivial, it's easier to
	// track things locally than to try and retrieve the state from the window manager.
	result = mMinimized;

	return(result);
}

BOOL LLWindowMacOSX::getMaximized()
{
	BOOL result = FALSE;

	if (mWindow)
	{
		// TODO
	}

	return(result);
}

BOOL LLWindowMacOSX::maximize()
{
	// TODO
	return FALSE;
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

BOOL LLWindowMacOSX::setSize(const LLCoordScreen size)
{
	if(mWindow)
	{
		SizeWindow(mWindow, size.mX, size.mY, true);
	}

	return TRUE;
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
		//		llinfos << "setMouseClipping(TRUE)" << llendl
	}
	else
	{
		//		llinfos << "setMouseClipping(FALSE)" << llendl
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

	//	llinfos << "setCursorPosition(" << screen_pos.mX << ", " << screen_pos.mY << ")" << llendl

	newPosition.x = screen_pos.mX;
	newPosition.y = screen_pos.mY;

	CGSetLocalEventsSuppressionInterval(0.0);
	if(CGWarpMouseCursorPosition(newPosition) == noErr)
	{
		result = TRUE;
	}

	// Under certain circumstances, this will trigger us to decouple the cursor.
	adjustCursorDecouple(true);

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
		// Mozilla sometimes changes our port origin.  Fuckers.
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


S32 LLWindowMacOSX::stat(const char* file_name, struct stat* stat_info)
{
	return ::stat( file_name, stat_info );
}

void LLWindowMacOSX::flashIcon(F32 seconds)
{
	// Don't do this if we're already started, since this would try to install the NMRec twice.
	if(!mBounceTimer.getStarted())
	{
		OSErr err;

		mBounceTime = seconds;
		memset(&mBounceRec, sizeof(mBounceRec), 0);
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


BOOL LLWindowMacOSX::sendEmail(const char* address, const char* subject, const char* body_text,
									   const char* attachment, const char* attachment_displayed_name )
{
	// MBW -- XXX -- Um... yeah.  I'll get to this later.

	return false;
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




void LLWindowMacOSX::setupFailure(const char* text, const char* caption, U32 type)
{
	destroyContext();

	OSMessageBox(text, caption, type);
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
			case kEventTextInputUnicodeForKeyEvent:
				{
					UInt32 modifiers = 0;

					// First, process the raw event.
					{
						EventRef rawEvent;

						// Get the original event and extract the modifier keys, so we can ignore command-key events.
						if (GetEventParameter(event, kEventParamTextInputSendKeyboardEvent, typeEventRef, NULL, sizeof(rawEvent), NULL, &rawEvent) == noErr)
						{
							// Grab the modifiers for later use in this function...
							GetEventParameter (rawEvent, kEventParamKeyModifiers, typeUInt32, NULL, sizeof(UInt32), NULL, &modifiers);

							// and call this function recursively to handle the raw key event.
							eventHandler (myHandler, rawEvent);
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
							MASK mask = 0;
							if(modifiers & shiftKey) { mask |= MASK_SHIFT; }
							if(modifiers & (cmdKey | controlKey)) { mask |= MASK_CONTROL; }
							if(modifiers & optionKey) { mask |= MASK_ALT; }

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

					result = err;
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
		case kEventWindowBoundsChanging:
			{
				Rect currentBounds;
				Rect previousBounds;

				GetEventParameter(event, kEventParamCurrentBounds, typeQDRectangle, NULL, sizeof(Rect), NULL, &currentBounds);
				GetEventParameter(event, kEventParamPreviousBounds, typeQDRectangle, NULL, sizeof(Rect), NULL, &previousBounds);

				// This is where we would constrain move/resize to a particular screen
				if(0)
				{
					SetEventParameter(event, kEventParamCurrentBounds, typeQDRectangle, sizeof(Rect), &currentBounds);
				}
			}
			break;

		case kEventWindowBoundsChanged:
			{
				Rect newBounds;

				GetEventParameter(event, kEventParamCurrentBounds, typeQDRectangle, NULL, sizeof(Rect), NULL, &newBounds);
				aglUpdateContext(mContext);
				mCallbacks->handleResize(this, newBounds.right - newBounds.left, newBounds.bottom - newBounds.top);


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
	}
	return result;
}

const char* cursorIDToName(int id)
{
	switch (id)
	{
		case UI_CURSOR_ARROW:			return "UI_CURSOR_ARROW";
		case UI_CURSOR_WAIT:			return "UI_CURSOR_WAIT";
		case UI_CURSOR_HAND:			return "UI_CURSOR_HAND";
		case UI_CURSOR_IBEAM:			return "UI_CURSOR_IBEAM";
		case UI_CURSOR_CROSS:			return "UI_CURSOR_CROSS";
		case UI_CURSOR_SIZENWSE:		return "UI_CURSOR_SIZENWSE";
		case UI_CURSOR_SIZENESW:		return "UI_CURSOR_SIZENESW";
		case UI_CURSOR_SIZEWE:			return "UI_CURSOR_SIZEWE";
		case UI_CURSOR_SIZENS:			return "UI_CURSOR_SIZENS";
		case UI_CURSOR_NO:				return "UI_CURSOR_NO";
		case UI_CURSOR_WORKING:			return "UI_CURSOR_WORKING";
		case UI_CURSOR_TOOLGRAB:		return "UI_CURSOR_TOOLGRAB";
		case UI_CURSOR_TOOLLAND:		return "UI_CURSOR_TOOLLAND";
		case UI_CURSOR_TOOLFOCUS:		return "UI_CURSOR_TOOLFOCUS";
		case UI_CURSOR_TOOLCREATE:		return "UI_CURSOR_TOOLCREATE";
		case UI_CURSOR_ARROWDRAG:		return "UI_CURSOR_ARROWDRAG";
		case UI_CURSOR_ARROWCOPY:		return "UI_CURSOR_ARROWCOPY";
		case UI_CURSOR_ARROWDRAGMULTI:	return "UI_CURSOR_ARROWDRAGMULTI";
		case UI_CURSOR_ARROWCOPYMULTI:	return "UI_CURSOR_ARROWCOPYMULTI";
		case UI_CURSOR_NOLOCKED:		return "UI_CURSOR_NOLOCKED";
		case UI_CURSOR_ARROWLOCKED:		return "UI_CURSOR_ARROWLOCKED";
		case UI_CURSOR_GRABLOCKED:		return "UI_CURSOR_GRABLOCKED";
		case UI_CURSOR_TOOLTRANSLATE:	return "UI_CURSOR_TOOLTRANSLATE";
		case UI_CURSOR_TOOLROTATE:		return "UI_CURSOR_TOOLROTATE";
		case UI_CURSOR_TOOLSCALE:		return "UI_CURSOR_TOOLSCALE";
		case UI_CURSOR_TOOLCAMERA:		return "UI_CURSOR_TOOLCAMERA";
		case UI_CURSOR_TOOLPAN:			return "UI_CURSOR_TOOLPAN";
		case UI_CURSOR_TOOLZOOMIN:		return "UI_CURSOR_TOOLZOOMIN";
		case UI_CURSOR_TOOLPICKOBJECT3:	return "UI_CURSOR_TOOLPICKOBJECT3";
		case UI_CURSOR_TOOLSIT:			return "UI_CURSOR_TOOLSIT";
		case UI_CURSOR_TOOLBUY:			return "UI_CURSOR_TOOLBUY";
		case UI_CURSOR_TOOLPAY:			return "UI_CURSOR_TOOLPAY";
		case UI_CURSOR_TOOLOPEN:		return "UI_CURSOR_TOOLOPEN";
		case UI_CURSOR_PIPETTE:			return "UI_CURSOR_PIPETTE";		
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

void LLWindowMacOSX::setCursor(ECursorType cursor)
{
	OSStatus result = noErr;

	if (cursor == UI_CURSOR_ARROW
		&& mBusyCount > 0)
	{
		cursor = UI_CURSOR_WORKING;
	}
	
	if(mCurrentCursor == cursor)
		return;

	// RN: replace multi-drag cursors with single versions
	if (cursor == UI_CURSOR_ARROWDRAGMULTI)
	{
		cursor = UI_CURSOR_ARROWDRAG;
	}
	else if (cursor == UI_CURSOR_ARROWCOPYMULTI)
	{
		cursor = UI_CURSOR_ARROWCOPY;
	}

	switch(cursor)
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
	case UI_CURSOR_TOOLSIT:
	case UI_CURSOR_TOOLBUY:
	case UI_CURSOR_TOOLPAY:
	case UI_CURSOR_TOOLOPEN:
		result = setImageCursor(gCursors[cursor]);
		break;

	}

	if(result != noErr)
	{
		InitCursor();
	}

	mCurrentCursor = cursor;
}

ECursorType LLWindowMacOSX::getCursor()
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
	initPixmapCursor(UI_CURSOR_TOOLSIT, 1, 1);
	initPixmapCursor(UI_CURSOR_TOOLBUY, 1, 1);
	initPixmapCursor(UI_CURSOR_TOOLPAY, 1, 1);
	initPixmapCursor(UI_CURSOR_TOOLOPEN, 1, 1);

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

void LLSplashScreenMacOSX::updateImpl(const char* mesg)
{
	if(mWindow != NULL)
	{
		CFStringRef string = NULL;

		if(mesg != NULL)
		{
			string = CFStringCreateWithCString(NULL, mesg, kCFStringEncodingUTF8);
		}
		else
		{
			string = CFStringCreateWithCString(NULL, "", kCFStringEncodingUTF8);
		}

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



S32 OSMessageBoxMacOSX(const char* text, const char* caption, U32 type)
{
	S32 result = OSBTN_CANCEL;
	SInt16 retval_mac = 1;
	AlertStdCFStringAlertParamRec params;
	CFStringRef errorString = NULL;
	CFStringRef explanationString = NULL;
	DialogRef alert = NULL;
	AlertType alertType = kAlertCautionAlert;
	OSStatus err;

	if(text != NULL)
	{
		explanationString = CFStringCreateWithCString(NULL, text, kCFStringEncodingUTF8);
	}
	else
	{
		explanationString = CFStringCreateWithCString(NULL, "", kCFStringEncodingUTF8);
	}

	if(caption != NULL)
	{
		errorString = CFStringCreateWithCString(NULL, caption, kCFStringEncodingUTF8);
	}
	else
	{
		errorString = CFStringCreateWithCString(NULL, "", kCFStringEncodingUTF8);
	}

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
void spawn_web_browser(const char* escaped_url)
{
	bool found = false;
	S32 i;
	for (i = 0; i < gURLProtocolWhitelistCount; i++)
	{
		S32 len = strlen(gURLProtocolWhitelist[i]);	/* Flawfinder: ignore */
		if (!strncmp(escaped_url, gURLProtocolWhitelist[i], len)
			&& escaped_url[len] == ':')
		{
			found = true;
			break;
		}
	}

	if (!found)
	{
		llwarns << "spawn_web_browser() called for url with protocol not on whitelist: " << escaped_url << llendl;
		return;
	}

	OSStatus result = noErr;
	CFURLRef urlRef = NULL;

	llinfos << "Opening URL " << escaped_url << llendl;

	CFStringRef	stringRef = CFStringCreateWithCString(NULL, escaped_url, kCFStringEncodingUTF8);
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

void shell_open( const char* file_path )
{
	OSStatus			result = noErr;

	llinfos << "Opening " << file_path << llendl;
	CFURLRef	urlRef = NULL;

	CFStringRef	stringRef = CFStringCreateWithCString(NULL, file_path, kCFStringEncodingUTF8);
	if (stringRef)
	{
		// This will succeed if the string is a full URL, including the http://
		// Note that URLs specified this way need to be properly percent-escaped.
		urlRef = CFURLCreateWithString(NULL, stringRef, NULL);

		if(urlRef == NULL)
		{
			// This will succeed if the string is a full or partial posix path.
			// This will work even if the path contains characters that would need to be percent-escaped
			// in the URL (such as spaces).
			urlRef = CFURLCreateWithFileSystemPath(NULL, stringRef, kCFURLPOSIXPathStyle, false);
		}

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

BOOL LLWindowMacOSX::dialog_color_picker ( F32 *r, F32 *g, F32 *b)
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

static WindowRef dummywindowref = NULL;

void *LLWindowMacOSX::getPlatformWindow()
{
	if(mWindow != NULL)
		return (void*)mWindow;

	// If we're in fullscreen mode, there's no window pointer available.
	// Since Mozilla needs one to function, create a dummy window here.
	// Note that we will never destroy it, but since only one will be created per run of the application, that's okay.
	
	if(dummywindowref == NULL)
	{
		Rect window_rect = {100, 100, 200, 200};

		dummywindowref = NewCWindow(
			NULL,
			&window_rect,
			"\p",
			false,				// Create the window invisible.  
			zoomDocProc,		// Window with a grow box and a zoom box
			kLastWindowOfClass,		// create it behind other windows
			false,					// no close box
			0);
	}
	
	return (void*)dummywindowref;
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

#endif // LL_DARWIN
