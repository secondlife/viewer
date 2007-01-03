/** 
 * @file llwindowsdl.cpp
 * @brief Platform-dependent implementation of llwindow
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#if LL_SDL

#include "linden_common.h"

#include "llwindowsdl.h"
#include "llkeyboardsdl.h"
#include "llerror.h"
#include "llgl.h"
#include "llstring.h"
#include "lldir.h"

#include "llglheaders.h"

#include "indra_constants.h"

#if LL_GTK
# include "gtk/gtk.h"
#endif // LL_GTK

#if LL_LINUX
// not necessarily available on random SDL platforms, so #if LL_LINUX
// for execv(), waitpid(), fork()
# include <unistd.h>
# include <sys/types.h>
# include <sys/wait.h>
#endif // LL_LINUX

extern BOOL gDebugWindowProc;

// culled from winuser.h
//const S32	WHEEL_DELTA = 120;     /* Value for rolling one detent */
// On the Mac, the scroll wheel reports a delta of 1 for each detent.
// There's also acceleration for faster scrolling, based on a slider in the system preferences.
const S32	WHEEL_DELTA = 1;     /* Value for rolling one detent */
const S32	BITS_PER_PIXEL = 32;
const S32	MAX_NUM_RESOLUTIONS = 32;

//
// LLWindowSDL
//

#if LL_X11
# include <X11/Xutil.h>
// A global!  Well, SDL isn't really designed for communicating
// with multiple physical X11 displays.  Heck, it's not really
// designed for multiple X11 windows.
// So, we need this for the SDL/X11 event filter callback (which
// doesnt have a userdata parameter) and more.
static Display *SDL_Display = NULL;
static Window SDL_XWindowID = None;
#endif //LL_X11

// TOFU HACK -- (*exactly* the same hack as LLWindowMacOSX for the same reasons)
// For SDL, to put up an OS dialog in full screen mode, we must first switch OUT of full screen mode.
// The proper way to do this is to bracket the dialog with calls to beforeDialog() and afterDialog(), but these
// require a pointer to the LLWindowMacSDL object.  Stash it here and maintain in the constructor and destructor.
// This assumes that there will be only one object of this class at any time.  Hopefully this is true.
static LLWindowSDL *gWindowImplementation = NULL;

static BOOL was_fullscreen = FALSE;

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


#if LL_GTK
// Check the runtime GTK version for goodness.
static BOOL maybe_do_gtk_diagnostics(void)
{
	static BOOL done_gtk_diag = FALSE;
	static BOOL is_good = TRUE;
	gtk_disable_setlocale();
	if ((!done_gtk_diag) && gtk_init_check(NULL, NULL))
	{
		llinfos << "GTK Initialized." << llendl;
		llinfos << "- Compiled against GTK version "
			<< GTK_MAJOR_VERSION << "."
			<< GTK_MINOR_VERSION << "."
			<< GTK_MICRO_VERSION << llendl;
		llinfos << "- Running against GTK version "
			<< gtk_major_version << "."
			<< gtk_minor_version << "."
			<< gtk_micro_version << llendl;
		gchar *gtk_warning;
		gtk_warning = gtk_check_version(GTK_MAJOR_VERSION,
						GTK_MINOR_VERSION,
						GTK_MICRO_VERSION);
		if (gtk_warning)
		{
			llwarns << "- GTK COMPATIBILITY WARNING: " <<
				gtk_warning << llendl;
			is_good = FALSE;
		}

		done_gtk_diag = TRUE;
	}
	return is_good;
}
#endif // LL_GTK


BOOL check_for_card(const char* RENDERER, const char* bad_card)
{
	if (!strncasecmp(RENDERER, bad_card, strlen(bad_card)))
	{
		char buffer[1024];
		sprintf(buffer,
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




LLWindowSDL::LLWindowSDL(char *title, S32 x, S32 y, S32 width,
							   S32 height, U32 flags,
							   BOOL fullscreen, BOOL clearBg,
							   BOOL disable_vsync, BOOL use_gl,
							   BOOL ignore_pixel_depth)
	: LLWindow(fullscreen, flags), mGamma(1.0f)
{
	// Initialize the keyboard
	gKeyboard = new LLKeyboardSDL();
	// Note that we can't set up key-repeat until after SDL has init'd video

	// Ignore use_gl for now, only used for drones on PC
	mWindow = NULL;
	mCursorDecoupled = FALSE;
	mCursorLastEventDeltaX = 0;
	mCursorLastEventDeltaY = 0;
	mCursorIgnoreNextDelta = FALSE;
	mNeedsResize = FALSE;
	mOverrideAspectRatio = 0.f;
	mGrabbyKeyFlags = 0;
	mReallyCapturedCount = 0;
	mHaveInputFocus = -1;
	mIsMinimized = -1;

	// Get the original aspect ratio of the main device.
	mOriginalAspectRatio = 1024.0 / 768.0;  // !!! FIXME //(double)CGDisplayPixelsWide(mDisplay) / (double)CGDisplayPixelsHigh(mDisplay);

	if (!title)
		title = "SDL Window";  // *FIX: (???)

	// Stash the window title
	mWindowTitle = new char[strlen(title) + 1];
	strcpy(mWindowTitle, title);

	// Create the GL context and set it up for windowed or fullscreen, as appropriate.
	if(createContext(x, y, width, height, 32, fullscreen, disable_vsync))
	{
		gGLManager.initGL();

		//start with arrow cursor
		initCursors();
		setCursor( UI_CURSOR_ARROW );
	}

	stop_glerror();

	// Stash an object pointer for OSMessageBox()
	gWindowImplementation = this;

#if LL_X11
	mFlashing = FALSE;
#endif // LL_X11
}

static SDL_Surface *Load_BMP_Resource(const char *basename)
{
	const int PATH_BUFFER_SIZE=1000;
	char path_buffer[PATH_BUFFER_SIZE];
	
	// Figure out where our BMP is living on the disk
	snprintf(path_buffer, PATH_BUFFER_SIZE-1, "%s%sres-sdl%s%s",
		 gDirUtilp->getAppRODataDir().c_str(),
		 gDirUtilp->getDirDelimiter().c_str(),
		 gDirUtilp->getDirDelimiter().c_str(),
		 basename);
	path_buffer[PATH_BUFFER_SIZE-1] = '\0';
	
	return SDL_LoadBMP(path_buffer);
}

BOOL LLWindowSDL::createContext(int x, int y, int width, int height, int bits, BOOL fullscreen, BOOL disable_vsync)
{
	//bool			glneedsinit = false;
//    const char *gllibname = null;

	llinfos << "createContext, fullscreen=" << fullscreen <<
	    " size=" << width << "x" << height << llendl;

	// captures don't survive contexts
	mGrabbyKeyFlags = 0;
	mReallyCapturedCount = 0;
	
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		llinfos << "sdl_init() failed! " << SDL_GetError() << llendl;
		setupFailure("window creation error", "error", OSMB_OK);
	    return false;
	}

	SDL_version c_sdl_version;
	SDL_VERSION(&c_sdl_version);
	llinfos << "Compiled against SDL "
		<< int(c_sdl_version.major) << "."
		<< int(c_sdl_version.minor) << "."
		<< int(c_sdl_version.patch) << llendl;
	const SDL_version *r_sdl_version;
	r_sdl_version = SDL_Linked_Version();
	llinfos << " Running against SDL "
		<< int(r_sdl_version->major) << "."
		<< int(r_sdl_version->minor) << "."
		<< int(r_sdl_version->patch) << llendl;

	const SDL_VideoInfo *videoInfo = SDL_GetVideoInfo( );
	if (!videoInfo)
	{
		llinfos << "SDL_GetVideoInfo() failed! " << SDL_GetError() << llendl;
		setupFailure("Window creation error", "Error", OSMB_OK);
		return FALSE;
	}

	SDL_EnableUNICODE(1);
	SDL_WM_SetCaption(mWindowTitle, mWindowTitle);

	// Set the application icon.
	SDL_Surface *bmpsurface;
	bmpsurface = Load_BMP_Resource("ll_icon.BMP");
	if (bmpsurface)
	{
		// This attempts to give a black-keyed mask to the icon.
		SDL_SetColorKey(bmpsurface,
				SDL_SRCCOLORKEY,
				SDL_MapRGB(bmpsurface->format, 0,0,0) );
		SDL_WM_SetIcon(bmpsurface, NULL);
		// The SDL examples cheerfully avoid freeing the icon
		// surface, but I'm betting that's leaky.
		SDL_FreeSurface(bmpsurface);
		bmpsurface = NULL;
	}

	// note: these SetAttributes make Tom's 9600-on-AMD64 fail to
	// get a visual, but it's broken anyway when it does, and without
	// these SetAttributes we might easily get an avoidable substandard
	// visual to work with on most other machines.
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE,  8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE,8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, (bits <= 16) ? 16 : 24);
        SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, (bits <= 16) ? 1 : 8);

        // *FIX: try to toggle vsync here?

	mFullscreen = fullscreen;
	was_fullscreen = fullscreen;

	int sdlflags = SDL_OPENGL | SDL_RESIZABLE | SDL_ANYFORMAT;

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    	mSDLFlags = sdlflags;

	if (mFullscreen)
	{
		llinfos << "createContext: setting up fullscreen " << width << "x" << height << llendl;

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

		mWindow = SDL_SetVideoMode(width, height, bits, sdlflags | SDL_FULLSCREEN);

		if (mWindow)
		{
			mFullscreen = TRUE;
			was_fullscreen = TRUE;
			mFullscreenWidth   = mWindow->w;
			mFullscreenHeight  = mWindow->h;
			mFullscreenBits    = mWindow->format->BitsPerPixel;
			mFullscreenRefresh = -1;

			llinfos << "Running at " << mFullscreenWidth
				<< "x"   << mFullscreenHeight
				<< "x"   << mFullscreenBits
				<< " @ " << mFullscreenRefresh
				<< llendl;
		}
		else
		{
			llwarns << "createContext: fullscreen creation failure. SDL: " << SDL_GetError() << llendl;
			// No fullscreen support
			mFullscreen = FALSE;
			was_fullscreen = FALSE;
			mFullscreenWidth   = -1;
			mFullscreenHeight  = -1;
			mFullscreenBits    = -1;
			mFullscreenRefresh = -1;

			char error[256];
			sprintf(error, "Unable to run fullscreen at %d x %d.\nRunning in window.", width, height);
			OSMessageBox(error, "Error", OSMB_OK);
		}
	}

	if(!mFullscreen && (mWindow == NULL))
	{
		if (width == 0)
		    width = 1024;
		if (height == 0)
		    width = 768;

		llinfos << "createContext: creating window " << width << "x" << height << "x" << bits << llendl;
		mWindow = SDL_SetVideoMode(width, height, bits, sdlflags);

		if (!mWindow)
		{
			llwarns << "createContext: window creation failure. SDL: " << SDL_GetError() << llendl;
			setupFailure("Window creation error", "Error", OSMB_OK);
			return FALSE;
		}
	} else if (!mFullscreen && (mWindow != NULL))
	{
		llinfos << "createContext: SKIPPING - !fullscreen, but +mWindow " << width << "x" << height << "x" << bits << llendl;
	}

    /*if (!load_all_glsyms(gllibname))
    {
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
        return FALSE;
    }*/

	gGLManager.mVRAM = videoInfo->video_mem / 1024;
	if (gGLManager.mVRAM != 0)
	{
		llinfos << "Detected " << gGLManager.mVRAM << "MB VRAM." << llendl;
	}
	// If VRAM is not detected, that is handled later

#if 0  // *FIX: all video cards suck under Linux.  :)
	// Since we just created the context, it needs to be set up.
	glNeedsInit = TRUE;
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
#endif

    GLint depthBits, stencilBits, redBits, greenBits, blueBits, alphaBits;

    glGetIntegerv(GL_RED_BITS, &redBits);
    glGetIntegerv(GL_GREEN_BITS, &greenBits);
    glGetIntegerv(GL_BLUE_BITS, &blueBits);
    glGetIntegerv(GL_ALPHA_BITS, &alphaBits);
    glGetIntegerv(GL_DEPTH_BITS, &depthBits);
    glGetIntegerv(GL_STENCIL_BITS, &stencilBits);

	llinfos << "GL buffer:" << llendl
        llinfos << "  Red Bits " << S32(redBits) << llendl
        llinfos << "  Green Bits " << S32(greenBits) << llendl
        llinfos << "  Blue Bits " << S32(blueBits) << llendl
	llinfos	<< "  Alpha Bits " << S32(alphaBits) << llendl
	llinfos	<< "  Depth Bits " << S32(depthBits) << llendl
	llinfos	<< "  Stencil Bits " << S32(stencilBits) << llendl;

	GLint colorBits = redBits + greenBits + blueBits + alphaBits;
	// fixme: actually, it's REALLY important for picking that we get at
	// least 8 bits each of red,green,blue.  Alpha we can be a bit more
	// relaxed about if we have to.
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

#if 0  // *FIX: we're going to brave it for now...
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
#endif

#if LL_X11
	init_x11clipboard();
#endif // LL_X11
	
	// We need to do this here, once video is init'd
	if (-1 == SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY,
				      SDL_DEFAULT_REPEAT_INTERVAL))
	    llwarns << "Couldn't enable key-repeat: " << SDL_GetError() <<llendl;

	// Don't need to get the current gamma, since there's a call that restores it to the system defaults.
	return TRUE;
}


// changing fullscreen resolution, or switching between windowed and fullscreen mode.
BOOL LLWindowSDL::switchContext(BOOL fullscreen, LLCoordScreen size, BOOL disable_vsync)
{
	const BOOL needsRebuild = TRUE;  // Just nuke the context and start over.
	BOOL result = true;

	llinfos << "switchContext, fullscreen=" << fullscreen << llendl;
	stop_glerror();
	if(needsRebuild)
	{
		destroyContext();
		result = createContext(0, 0, size.mX, size.mY, 0, fullscreen, disable_vsync);
		if (result)
		{
			gGLManager.initGL();

			//start with arrow cursor
			initCursors();
			setCursor( UI_CURSOR_ARROW );
		}
	}

	stop_glerror();

	return result;
}

void LLWindowSDL::destroyContext()
{
	llinfos << "destroyContext begins" << llendl;
#if LL_X11
	quit_x11clipboard();
#endif // LL_X11

	// Clean up remaining GL state before blowing away window
	llinfos << "shutdownGL begins" << llendl;
	gGLManager.shutdownGL();
	llinfos << "SDL_QuitSS/VID begins" << llendl;
	SDL_QuitSubSystem(SDL_INIT_VIDEO);  // *FIX: this might be risky...
	//unload_all_glsyms();

	mWindow = NULL;
}

LLWindowSDL::~LLWindowSDL()
{
	quitCursors();
	destroyContext();

	if(mSupportedResolutions != NULL)
	{
		delete []mSupportedResolutions;
	}

	delete[] mWindowTitle;

	gWindowImplementation = NULL;
}


void LLWindowSDL::show()
{
    // *FIX: What to do with SDL?
}

void LLWindowSDL::hide()
{
    // *FIX: What to do with SDL?
}

void LLWindowSDL::minimize()
{
    // *FIX: What to do with SDL?
}

void LLWindowSDL::restore()
{
    // *FIX: What to do with SDL?
}


// close() destroys all OS-specific code associated with a window.
// Usually called from LLWindowManager::destroyWindow()
void LLWindowSDL::close()
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

BOOL LLWindowSDL::isValid()
{
	return (mWindow != NULL);
}

BOOL LLWindowSDL::getVisible()
{
	BOOL result = FALSE;

    // *FIX: This isn't really right...
	// Then what is?
	if (mWindow)
	{
		result = TRUE;
	}

	return(result);
}

BOOL LLWindowSDL::getMinimized()
{
	BOOL result = FALSE;

	if (mWindow && (1 == mIsMinimized))
	{
		result = TRUE;
	}
	return(result);
}

BOOL LLWindowSDL::getMaximized()
{
	BOOL result = FALSE;

	if (mWindow)
	{
		// TODO
	}

	return(result);
}

BOOL LLWindowSDL::maximize()
{
	// TODO
	return FALSE;
}

BOOL LLWindowSDL::getFullscreen()
{
	return mFullscreen;
}

BOOL LLWindowSDL::getPosition(LLCoordScreen *position)
{
    // *FIX: can anything be done with this?
	position->mX = 0;
	position->mY = 0;
    return TRUE;
}

BOOL LLWindowSDL::getSize(LLCoordScreen *size)
{
    if (mWindow)
    {
        size->mX = mWindow->w;
        size->mY = mWindow->h;
		return (TRUE);
    }

	llerrs << "LLWindowSDL::getPosition(): no window and not fullscreen!" << llendl;
    return (FALSE);
}

BOOL LLWindowSDL::getSize(LLCoordWindow *size)
{
    if (mWindow)
    {
        size->mX = mWindow->w;
        size->mY = mWindow->h;
		return (TRUE);
    }

	llerrs << "LLWindowSDL::getPosition(): no window and not fullscreen!" << llendl;
    return (FALSE);
}

BOOL LLWindowSDL::setPosition(const LLCoordScreen position)
{
	if(mWindow)
	{
        // *FIX: (???)
		//MacMoveWindow(mWindow, position.mX, position.mY, false);
	}

	return TRUE;
}

BOOL LLWindowSDL::setSize(const LLCoordScreen size)
{
	if(mWindow)
	{
        // *FIX: (???)
		//SizeWindow(mWindow, size.mX, size.mY, true);
	}

	return TRUE;
}

void LLWindowSDL::swapBuffers()
{
	if (mWindow)
		SDL_GL_SwapBuffers();
}

F32 LLWindowSDL::getGamma()
{
	return 1/mGamma;
}

BOOL LLWindowSDL::restoreGamma()
{
	//CGDisplayRestoreColorSyncSettings();
    SDL_SetGamma(1.0f, 1.0f, 1.0f);
	return true;
}

BOOL LLWindowSDL::setGamma(const F32 gamma)
{
	mGamma = gamma;
	if (mGamma == 0) mGamma = 0.1f;
	mGamma = 1/mGamma;
	SDL_SetGamma(mGamma, mGamma, mGamma);
	return true;
}

BOOL LLWindowSDL::isCursorHidden()
{
	return mCursorHidden;
}



// Constrains the mouse to the window.
void LLWindowSDL::setMouseClipping( BOOL b )
{
	//llinfos << "LLWindowSDL::setMouseClipping " << b << llendl;
	// Just stash the requested state.  We'll simulate this when the cursor is hidden by decoupling.
	mIsMouseClipping = b;
    //SDL_WM_GrabInput(b ? SDL_GRAB_ON : SDL_GRAB_OFF);
	adjustCursorDecouple();
}

BOOL LLWindowSDL::setCursorPosition(const LLCoordWindow position)
{
	BOOL result = TRUE;
	LLCoordScreen screen_pos;

	if (!convertCoords(position, &screen_pos))
	{
		return FALSE;
	}

	//llinfos << "setCursorPosition(" << screen_pos.mX << ", " << screen_pos.mY << ")" << llendl;

    SDL_WarpMouse(screen_pos.mX, screen_pos.mY);

	// Under certain circumstances, this will trigger us to decouple the cursor.
	adjustCursorDecouple(true);

	return result;
}

BOOL LLWindowSDL::getCursorPosition(LLCoordWindow *position)
{
	//Point cursor_point;
	LLCoordScreen screen_pos;

	//GetMouse(&cursor_point);
    int x, y;
    SDL_GetMouseState(&x, &y);

	screen_pos.mX = x;
	screen_pos.mY = y;

	return convertCoords(screen_pos, position);
}

void LLWindowSDL::adjustCursorDecouple(bool warpingMouse)
{
	if(mIsMouseClipping && mCursorHidden)
	{
		if(warpingMouse)
		{
			// The cursor should be decoupled.  Make sure it is.
			if(!mCursorDecoupled)
			{
				//			llinfos << "adjustCursorDecouple: decoupling cursor" << llendl;
				//CGAssociateMouseAndMouseCursorPosition(false);
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
			//CGAssociateMouseAndMouseCursorPosition(true);
			mCursorDecoupled = false;
		}
	}
}

F32 LLWindowSDL::getNativeAspectRatio()
{
#if 0
	// RN: this hack presumes that the largest supported resolution is monitor-limited
	// and that pixels in that mode are square, therefore defining the native aspect ratio
	// of the monitor...this seems to work to a close approximation for most CRTs/LCDs
	S32 num_resolutions;
	LLWindowResolution* resolutions = getSupportedResolutions(num_resolutions);


	return ((F32)resolutions[num_resolutions - 1].mWidth / (F32)resolutions[num_resolutions - 1].mHeight);
	//rn: AC
#endif

	// MBW -- there are a couple of bad assumptions here.  One is that the display list won't include
	//		ridiculous resolutions nobody would ever use.  The other is that the list is in order.

	// New assumptions:
	// - pixels are square (the only reasonable choice, really)
	// - The user runs their display at a native resolution, so the resolution of the display
	//    when the app is launched has an aspect ratio that matches the monitor.

	//RN: actually, the assumption that there are no ridiculous resolutions (above the display's native capabilities) has 
	// been born out in my experience.  
	// Pixels are often not square (just ask the people who run their LCDs at 1024x768 or 800x600 when running fullscreen, like me)
	// The ordering of display list is a blind assumption though, so we should check for max values
	// Things might be different on the Mac though, so I'll defer to MBW

	// The constructor for this class grabs the aspect ratio of the monitor before doing any resolution
	// switching, and stashes it in mOriginalAspectRatio.  Here, we just return it.

	if (mOverrideAspectRatio > 0.f)
	{
		return mOverrideAspectRatio;
	}

	return mOriginalAspectRatio;
}

F32 LLWindowSDL::getPixelAspectRatio()
{
	F32 pixel_aspect = 1.f;
	if (getFullscreen())
	{
		LLCoordScreen screen_size;
		getSize(&screen_size);
		pixel_aspect = getNativeAspectRatio() * (F32)screen_size.mY / (F32)screen_size.mX;
	}

	return pixel_aspect;
}


// some of this stuff is to support 'temporarily windowed' mode so that
// dialogs are still usable in fullscreen.  HOWEVER! - it's not enabled/working
// yet.
static LLCoordScreen old_size;
static BOOL old_fullscreen;
void LLWindowSDL::beforeDialog()
{
	llinfos << "LLWindowSDL::beforeDialog()" << llendl;

	if (SDLReallyCaptureInput(FALSE) // must ungrab input so popup works!
	    && getSize(&old_size))
	{
		old_fullscreen = was_fullscreen;
		
		if (old_fullscreen)
		{
			// NOT YET WORKING
			//switchContext(FALSE, old_size, TRUE);
		}
	}

#if LL_X11
	if (SDL_Display)
	{
		// Everything that we/SDL asked for should happen before we
		// potentially hand control over to GTK.
		XSync(SDL_Display, False);
	}
#endif // LL_X11

#if LL_GTK
	// this is a good time to grab some GTK version information for
	// diagnostics
	maybe_do_gtk_diagnostics();
#endif // LL_GTK
}

void LLWindowSDL::afterDialog()
{
	llinfos << "LLWindowSDL::afterDialog()" << llendl;
	if (old_fullscreen && !was_fullscreen)
	{
		// *FIX: NOT YET WORKING (see below)
		//switchContext(TRUE, old_size, TRUE);
	}
	// *FIX: we need to restore the GL context using
	// LLViewerWindow::restoreGL() - but how??
}


S32 LLWindowSDL::stat(const char* file_name, struct stat* stat_info)
{
	return ::stat( file_name, stat_info );
}

#if LL_X11
// set/reset the XWMHints flag for 'urgency' that usually makes the icon flash
void LLWindowSDL::x11_set_urgent(BOOL urgent)
{
	if (SDL_Display && !mFullscreen)
	{
		XWMHints *wm_hints;
		
		llinfos << "X11 hint for urgency, " << urgent << llendl;

		wm_hints = XGetWMHints(SDL_Display, mSDL_XWindowID);
		if (!wm_hints)
			wm_hints = XAllocWMHints();

		if (urgent)
			wm_hints->flags |= XUrgencyHint;
		else
			wm_hints->flags &= ~XUrgencyHint;

		XSetWMHints(SDL_Display, mSDL_XWindowID, wm_hints);
		XFree(wm_hints);
		XSync(SDL_Display, False);
	}
}
#endif // LL_X11

void LLWindowSDL::flashIcon(F32 seconds)
{
#if !LL_X11
	llinfos << "Stub LLWindowSDL::flashIcon(" << seconds << ")" << llendl;
#else	
	llinfos << "X11 LLWindowSDL::flashIcon(" << seconds << ")" << llendl;
	
	F32 remaining_time = mFlashTimer.getRemainingTimeF32();
	if (remaining_time < seconds)
		remaining_time = seconds;
	mFlashTimer.reset();
	mFlashTimer.setTimerExpirySec(remaining_time);

	x11_set_urgent(TRUE);
	mFlashing = TRUE;
#endif // LL_X11
}

#if LL_X11
/* Lots of low-level X11 stuff to handle X11 copy-and-paste */

/* Our X11 clipboard support is a bit bizarre in various
   organically-grown ways.  Ideally it should be fixed to do
   real string-type negotiation (this would make pasting to
   xterm faster and pasting to UTF-8 emacs work properly), but
   right now it has the rare and desirable trait of being
   generally stable and working. */

/* PRIMARY and CLIPBOARD are the two main kinds of
   X11 clipboard.  A third are the CUT_BUFFERs which an
   obsolete holdover from X10 days and use a quite orthogonal
   mechanism.  CLIPBOARD is the type whose design most
   closely matches SL's own win32-alike explicit copy-and-paste
   paradigm.

   Pragmatically we support all three to varying degrees.  When
   we paste into SL, it is strictly from CLIPBOARD.  When we copy,
   we support (to as full an extent as the clipboard content type
   allows) CLIPBOARD, PRIMARY, and CUT_BUFFER0.
 */
#define SL_READWRITE_XCLIPBOARD_TYPE XInternAtom(SDL_Display, "CLIPBOARD", False)
#define SL_WRITE_XCLIPBOARD_TYPE XA_PRIMARY

/* This is where our own private cutbuffer goes - we don't use
   a regular cutbuffer (XA_CUT_BUFFER0 etc) for intermediate
   storage because their use isn't really defined for holding UTF8. */
#define SL_CUTBUFFER_TYPE XInternAtom(SDL_Display, "SECONDLIFE_CUTBUFFER", False)

/* These defines, and convert_data/convert_x11clipboard,
   mostly exist to support non-text or unusually-encoded
   clipboard data, which we don't really have a need for at
   the moment. */
#define SDLCLIPTYPE(A, B, C, D) (int)((A<<24)|(B<<16)|(C<<8)|(D<<0))
#define FORMAT_PREFIX	"SECONDLIFE_x11clipboard_0x"

typedef Atom x11clipboard_type;

static
x11clipboard_type convert_format(int type)
{
	switch (type)
	{
	case SDLCLIPTYPE('T', 'E', 'X', 'T'):
		// old-style X11 clipboard, strictly only ISO 8859-1 encoding
		return XA_STRING;
	case SDLCLIPTYPE('U', 'T', 'F', '8'):
		// newer de-facto UTF8 clipboard atom
		return XInternAtom(SDL_Display, "UTF8_STRING", False);
	default:
	{
		/* completely arbitrary clipboard types... we don't actually use
		these right now, and support is skeletal. */
		char format[sizeof(FORMAT_PREFIX)+8+1];

		sprintf(format, "%s%08lx", FORMAT_PREFIX, (unsigned long)type);
		return XInternAtom(SDL_Display, format, False);
	}
    }
}

/* convert platform string to x11 clipboard format.  for our
   purposes this is pretty trivial right now. */
static int
convert_data(int type, char *dst, const char *src, int srclen)
{
	int dstlen;

	dstlen = 0;
	switch (type)
	{
	case SDLCLIPTYPE('T', 'E', 'X', 'T'):
	case SDLCLIPTYPE('U', 'T', 'F', '8'):
		if ( srclen == 0 )
			srclen = strlen(src);
		
		dstlen = srclen + 1;
		
		if ( dst ) // assume caller made it big enough by asking us
		{
			memcpy(dst, src, srclen);
			dst[srclen] = '\0';
		}
		break;
		
	default:
		llwarns << "convert_data: Unknown medium type" << llendl;
		break;
	}
	return(dstlen);
}

/* Convert x11clipboard data to platform string.  This too is
   pretty trivial for our needs right now, and just about identical
   to above. */
static int
convert_x11clipboard(int type, char *dst, const char *src, int srclen)
{
	int dstlen;

	dstlen = 0;
	switch (type)
	{
	case SDLCLIPTYPE('U', 'T', 'F', '8'):
	case SDLCLIPTYPE('T', 'E', 'X', 'T'):
		if ( srclen == 0 )
			srclen = strlen(src);
		
		dstlen = srclen + 1;
		
		if ( dst ) // assume caller made it big enough by asking us
		{
			memcpy(dst, src, srclen);
			dst[srclen] = '\0';
		}
		break;
		
	default:
		llwarns << "convert_x11clipboard: Unknown medium type" << llendl;
		break;
	}
	return dstlen;
}

int
LLWindowSDL::is_empty_x11clipboard(void)
{
	int retval;

	Lock_Display();
	retval = ( XGetSelectionOwner(SDL_Display, SL_READWRITE_XCLIPBOARD_TYPE) == None );
	Unlock_Display();

	return(retval);
}

void
LLWindowSDL::put_x11clipboard(int type, int srclen, const char *src)
{
	x11clipboard_type format;
	int dstlen;
	char *dst;

	format = convert_format(type);
	dstlen = convert_data(type, NULL, src, srclen);

	dst = (char *)malloc(dstlen);
	if ( dst != NULL )
	{
		Window root = DefaultRootWindow(SDL_Display);
		Lock_Display();
		convert_data(type, dst, src, srclen);
		// Cutbuffers are only allowed to have STRING atom types,
		// but Emacs puts UTF8 inside them anyway.  We cautiously
		// don't.
		if (type == SDLCLIPTYPE('T','E','X','T'))
		{
			// dstlen-1 so we don't include the trailing \0
			llinfos << "X11: Populating cutbuffer." <<llendl;
			XChangeProperty(SDL_Display, root,
					XA_CUT_BUFFER0, XA_STRING, 8, PropModeReplace,
					(unsigned char*)dst, dstlen-1);
		} else {
			// Should we clear the cutbuffer if we can't put the selection in
			// it because it's a UTF8 selection?  Eh, no great reason I think.
			//XDeleteProperty(SDL_Display, root, XA_CUT_BUFFER0);
		}
		// Private cutbuffer of an appropriate type.
		XChangeProperty(SDL_Display, root,
				SL_CUTBUFFER_TYPE, format, 8, PropModeReplace,
				(unsigned char*)dst, dstlen-1);
		free(dst);
		
		/* Claim ownership of both PRIMARY and CLIPBOARD */
		XSetSelectionOwner(SDL_Display, SL_READWRITE_XCLIPBOARD_TYPE,
				   mSDL_XWindowID, CurrentTime);
		XSetSelectionOwner(SDL_Display, SL_WRITE_XCLIPBOARD_TYPE,
				   mSDL_XWindowID, CurrentTime);
		
		Unlock_Display();
	}
}

void
LLWindowSDL::get_x11clipboard(int type, int *dstlen, char **dst)
{
	x11clipboard_type format;
	
	*dstlen = 0;
	format = convert_format(type);

	Window owner;
	Atom selection;
	Atom seln_type;
	int seln_format;
	unsigned long nbytes;
	unsigned long overflow;
	char *src;
	
	Lock_Display();
	owner = XGetSelectionOwner(SDL_Display, SL_READWRITE_XCLIPBOARD_TYPE);
	Unlock_Display();
	if (owner == None)
	{
		// Fall right back to ancient X10 cut-buffers
		owner = DefaultRootWindow(SDL_Display);
		selection = XA_CUT_BUFFER0;
	} else if (owner == mSDL_XWindowID)
	{
		// Use our own uncooked opaque string property
		owner = DefaultRootWindow(SDL_Display);
		selection = SL_CUTBUFFER_TYPE;
	}
	else
	{
		// Use full-on X11-style clipboard negotiation with the owning app
		int selection_response = 0;
		SDL_Event event;
		
		owner = mSDL_XWindowID;
		Lock_Display();
		selection = XInternAtom(SDL_Display, "SDL_SELECTION", False);
		XConvertSelection(SDL_Display, SL_READWRITE_XCLIPBOARD_TYPE, format,
				  selection, owner, CurrentTime);
		Unlock_Display();
		llinfos << "X11: Waiting for clipboard to arrive." <<llendl;
		while ( ! selection_response )
		{
			// Only look for SYSWMEVENTs, or we may lose keypresses
			// etc.
			SDL_PumpEvents();
			if (1 == SDL_PeepEvents(&event, 1, SDL_GETEVENT,
						SDL_SYSWMEVENTMASK) )
			{
				if ( event.type == SDL_SYSWMEVENT )
				{
					XEvent xevent =
						event.syswm.msg->event.xevent;
				
					if ( (xevent.type == SelectionNotify)&&
					     (xevent.xselection.requestor == owner) )
						selection_response = 1;
				}
			} else {
				llinfos << "X11: Waiting for SYSWM event..." <<  llendl;
			}
		}
		llinfos << "X11: Clipboard arrived." <<llendl;
	}

	Lock_Display();
	if ( XGetWindowProperty(SDL_Display, owner, selection, 0, INT_MAX/4,
				False, format, &seln_type, &seln_format,
				&nbytes, &overflow, (unsigned char **)&src) == Success )
	{
		if ( seln_type == format )
		{
			*dstlen = convert_x11clipboard(type, NULL, src, nbytes);
			*dst = (char *)realloc(*dst, *dstlen);
			if ( *dst == NULL )
				*dstlen = 0;
			else
				convert_x11clipboard(type, *dst, src, nbytes);
		}
		XFree(src);
	}
	
	Unlock_Display();
}

int clipboard_filter_callback(const SDL_Event *event)
{
	/* Post all non-window manager specific events */
	if ( event->type != SDL_SYSWMEVENT )
	{
		return(1);
	}

	/* Handle window-manager specific clipboard events */
	switch (event->syswm.msg->event.xevent.type) {
	/* Copy the selection from SL_CUTBUFFER_TYPE to the requested property */
	case SelectionRequest: {
		XSelectionRequestEvent *req;
		XEvent sevent;
		int seln_format;
		unsigned long nbytes;
		unsigned long overflow;
		unsigned char *seln_data;

		req = &event->syswm.msg->event.xevent.xselectionrequest;
		sevent.xselection.type = SelectionNotify;
		sevent.xselection.display = req->display;
		sevent.xselection.selection = req->selection;
		sevent.xselection.target = None;
		sevent.xselection.property = None;
		sevent.xselection.requestor = req->requestor;
		sevent.xselection.time = req->time;
		if ( XGetWindowProperty(SDL_Display, DefaultRootWindow(SDL_Display),
					SL_CUTBUFFER_TYPE, 0, INT_MAX/4, False, req->target,
					&sevent.xselection.target, &seln_format,
					&nbytes, &overflow, &seln_data) == Success )
		{
			if ( sevent.xselection.target == req->target)
			{
				if ( sevent.xselection.target == XA_STRING ||
				     sevent.xselection.target ==
				     convert_format(SDLCLIPTYPE('U','T','F','8')) )
				{
					if ( seln_data[nbytes-1] == '\0' )
						--nbytes;
				}
				XChangeProperty(SDL_Display, req->requestor, req->property,
						req->target, seln_format, PropModeReplace,
						seln_data, nbytes);
				sevent.xselection.property = req->property;
#define XA_TARGETS XInternAtom(SDL_Display, "TARGETS", False)
			} else if (XA_TARGETS == req->target) {
				/* only advertise what we currently support */
				const int num_supported = 3;
				Atom supported[num_supported] = {
					XA_STRING, // will be over-written below
					XInternAtom(SDL_Display, "TEXT",False),
					XA_TARGETS
				};
				supported[0] = sevent.xselection.target;
				XChangeProperty(SDL_Display, req->requestor,
						req->property, XA_ATOM, 32, PropModeReplace,
						(unsigned char*)supported,
						num_supported);
				sevent.xselection.property = req->property;
				llinfos << "Clipboard: An app asked us what selections format we offer." << llendl;
			} else {
				llinfos << "Clipboard: An app requested an unsupported selection format " << req->target << ", we have " << sevent.xselection.target << llendl;
			    sevent.xselection.target = None;
			}
			XFree(seln_data);
		}
		int sendret =
			XSendEvent(SDL_Display,req->requestor,False,0,&sevent);
		if ((sendret==BadValue) || (sendret==BadWindow))
			llwarns << "Clipboard SendEvent failed" << llendl;
		XSync(SDL_Display, False);
	}
		break;
	}
	
	/* Post the event for X11 clipboard reading above */
	return(1);
}

int
LLWindowSDL::init_x11clipboard(void)
{
	SDL_SysWMinfo info;
	int retval;

	/* Grab the window manager specific information */
	retval = -1;
	SDL_SetError("SDL is not running on known window manager");

	SDL_VERSION(&info.version);
	if ( SDL_GetWMInfo(&info) )
	{
		/* Save the information for later use */
		if ( info.subsystem == SDL_SYSWM_X11 )
		{
			SDL_Display = info.info.x11.display;
			SDL_XWindowID = info.info.x11.wmwindow;
			mSDL_XWindowID = info.info.x11.wmwindow;
			Lock_Display = info.info.x11.lock_func;
			Unlock_Display = info.info.x11.unlock_func;
			
			/* Enable the special window hook events */
			SDL_EventState(SDL_SYSWMEVENT, SDL_ENABLE);
			SDL_SetEventFilter(clipboard_filter_callback);
			
			retval = 0;
		}
		else
		{
			SDL_SetError("SDL is not running on X11");
		}
	}
	return(retval);
}

void
LLWindowSDL::quit_x11clipboard(void)
{
	SDL_Display = NULL;
	SDL_XWindowID = None;
	mSDL_XWindowID = None;
	Lock_Display = NULL;
	Unlock_Display = NULL;

	SDL_SetEventFilter(NULL); // Stop custom event filtering
}

/************************************************/

BOOL LLWindowSDL::isClipboardTextAvailable()
{
	return !is_empty_x11clipboard();
}

BOOL LLWindowSDL::pasteTextFromClipboard(LLWString &dst)
{
	int cliplen; // seems 1 or 2 bytes longer than expected
	char *cliptext = NULL;
	get_x11clipboard(SDLCLIPTYPE('U','T','F','8'), &cliplen, &cliptext);
	if (cliptext)
	{
		llinfos << "X11: Got UTF8 clipboard text." << llendl;
		// at some future time we can use cliplen instead of relying on \0,
		// if we ever grok non-ascii, non-utf8 encodings on the clipboard.
		std::string clip_str(cliptext);
		// we can't necessarily trust the incoming text to be valid UTF-8,
		// but utf8str_to_wstring() seems to do an appropriate level of
		// validation for avoiding over-reads.
		dst = utf8str_to_wstring(clip_str);
		/*llinfos << "X11 pasteTextFromClipboard: cliplen=" << cliplen <<
			" strlen(cliptext)=" << strlen(cliptext) <<
			" clip_str.length()=" << clip_str.length() <<
			" dst.length()=" << dst.length() <<
			llendl;*/
		free(cliptext);
		return TRUE; // success
	}
	get_x11clipboard(SDLCLIPTYPE('T','E','X','T'), &cliplen, &cliptext);
	if (cliptext)
	{
		llinfos << "X11: Got ISO 8859-1 clipboard text." << llendl;
		std::string clip_str(cliptext);
		std::string utf8_str = rawstr_to_utf8(clip_str);
		dst = utf8str_to_wstring(utf8_str);
		free(cliptext);
	}
	return FALSE; // failure
}

BOOL LLWindowSDL::copyTextToClipboard(const LLWString &s)
{
	std::string utf8text = wstring_to_utf8str(s);
	const char* cstr = utf8text.c_str();
	int cstrlen = strlen(cstr);
	int i;
	for (i=0; i<cstrlen; ++i)
	{
		if (0x80 & (unsigned char)cstr[i])
		{
			// Found an 8-bit character; use new-style UTF8 clipboard
			llinfos << "X11: UTF8 copyTextToClipboard" << llendl;
			put_x11clipboard(SDLCLIPTYPE('U','T','F','8'), cstrlen, cstr);
			return TRUE;
		}
	}
	// Didn't find any 8-bit characters; use old-style ISO 8859-1 clipboard
	llinfos << "X11: ISO 8859-1 copyTextToClipboard" << llendl;
	put_x11clipboard(SDLCLIPTYPE('T','E','X','T'), cstrlen, cstr);
	return TRUE;
}
#else

BOOL LLWindowSDL::isClipboardTextAvailable()
{
	return FALSE; // unsupported
}

BOOL LLWindowSDL::pasteTextFromClipboard(LLWString &dst)
{
	return FALSE; // unsupported
}

BOOL LLWindowSDL::copyTextToClipboard(const LLWString &s)
{
	return FALSE;  // unsupported
}
#endif // LL_X11

BOOL LLWindowSDL::sendEmail(const char* address, const char* subject, const char* body_text,
									   const char* attachment, const char* attachment_displayed_name )
{
	// MBW -- XXX -- Um... yeah.  I'll get to this later.

	return FALSE;
}


LLWindow::LLWindowResolution* LLWindowSDL::getSupportedResolutions(S32 &num_resolutions)
{
	if (!mSupportedResolutions)
	{
		mSupportedResolutions = new LLWindowResolution[MAX_NUM_RESOLUTIONS];
		mNumSupportedResolutions = 0;

        SDL_Rect **modes = SDL_ListModes(NULL, SDL_OPENGL | SDL_FULLSCREEN);
        if ( (modes != NULL) && (modes != ((SDL_Rect **) -1)) )
        {
            int count = 0;
            while (*modes)  // they're sorted biggest to smallest, so find end...
            {
                modes++;
                count++;
            }

            while (count--)
            {
                modes--;
                SDL_Rect *r = *modes;
                int w = r->w;
                int h = r->h;
                if ((w >= 800) && (h >= 600))
                {
                    // make sure we don't add the same resolution multiple times!
                    if ( (mNumSupportedResolutions == 0) ||
                         ((mSupportedResolutions[mNumSupportedResolutions-1].mWidth != w) &&
                          (mSupportedResolutions[mNumSupportedResolutions-1].mHeight != h)) )
                    {
                        mSupportedResolutions[mNumSupportedResolutions].mWidth = w;
                        mSupportedResolutions[mNumSupportedResolutions].mHeight = h;
                        mNumSupportedResolutions++;
                    }
                }
            }
        }
	}

	num_resolutions = mNumSupportedResolutions;
	return mSupportedResolutions;
}

BOOL LLWindowSDL::convertCoords(LLCoordGL from, LLCoordWindow *to)
{
    if (!to)
        return FALSE;

	to->mX = from.mX;
	to->mY = mWindow->h - from.mY - 1;

	return TRUE;
}

BOOL LLWindowSDL::convertCoords(LLCoordWindow from, LLCoordGL* to)
{
    if (!to)
        return FALSE;

	to->mX = from.mX;
	to->mY = mWindow->h - from.mY - 1;

	return TRUE;
}

BOOL LLWindowSDL::convertCoords(LLCoordScreen from, LLCoordWindow* to)
{
    if (!to)
		return FALSE;

	// In the fullscreen case, window and screen coordinates are the same.
	to->mX = from.mX;
	to->mY = from.mY;
    return (TRUE);
}

BOOL LLWindowSDL::convertCoords(LLCoordWindow from, LLCoordScreen *to)
{
    if (!to)
		return FALSE;

	// In the fullscreen case, window and screen coordinates are the same.
	to->mX = from.mX;
	to->mY = from.mY;
    return (TRUE);
}

BOOL LLWindowSDL::convertCoords(LLCoordScreen from, LLCoordGL *to)
{
	LLCoordWindow window_coord;

	return(convertCoords(from, &window_coord) && convertCoords(window_coord, to));
}

BOOL LLWindowSDL::convertCoords(LLCoordGL from, LLCoordScreen *to)
{
	LLCoordWindow window_coord;

	return(convertCoords(from, &window_coord) && convertCoords(window_coord, to));
}




void LLWindowSDL::setupFailure(const char* text, const char* caption, U32 type)
{
	destroyContext();

	OSMessageBox(text, caption, type);
}

BOOL LLWindowSDL::SDLReallyCaptureInput(BOOL capture)
{
	// note: this used to be safe to call nestedly, but in the
	// end that's not really a wise usage pattern, so don't.

	if (capture)
		mReallyCapturedCount = 1;
	else
		mReallyCapturedCount = 0;
	
	SDL_GrabMode wantmode, newmode;
	if (mReallyCapturedCount <= 0) // uncapture
	{
		wantmode = SDL_GRAB_OFF;
	} else // capture
	{
		wantmode = SDL_GRAB_ON;
	}
	
	if (mReallyCapturedCount < 0) // yuck, imbalance.
	{
		mReallyCapturedCount = 0;
		llwarns << "ReallyCapture count was < 0" << llendl;
	}

	if (!mFullscreen) /* only bother if we're windowed anyway */
	{
#if LL_X11
		if (SDL_Display)
		{
			/* we dirtily mix raw X11 with SDL so that our pointer
			   isn't (as often) constrained to the limits of the
			   window while grabbed, which feels nicer and
			   hopefully eliminates some reported 'sticky pointer'
			   problems.  We use raw X11 instead of
			   SDL_WM_GrabInput() because the latter constrains
			   the pointer to the window and also steals all
			   *keyboard* input from the window manager, which was
			   frustrating users. */
			int result;
			if (wantmode == SDL_GRAB_ON)
			{
				//llinfos << "X11 POINTER GRABBY" << llendl;
				//newmode = SDL_WM_GrabInput(wantmode);
				result = XGrabPointer(SDL_Display, mSDL_XWindowID,
						      True, 0, GrabModeAsync,
						      GrabModeAsync,
						      None, None, CurrentTime);
				if (GrabSuccess == result)
					newmode = SDL_GRAB_ON;
				else
					newmode = SDL_GRAB_OFF;
			} else if (wantmode == SDL_GRAB_OFF)
			{
				//llinfos << "X11 POINTER UNGRABBY" << llendl;
				newmode = SDL_GRAB_OFF;
				//newmode = SDL_WM_GrabInput(SDL_GRAB_OFF);

				XUngrabPointer(SDL_Display, CurrentTime);
				// Make sure the ungrab happens RIGHT NOW.
				XSync(SDL_Display, False);
			} else
			{
				newmode = SDL_GRAB_QUERY; // neutral
			}
		} else // not actually running on X11, for some reason
			newmode = wantmode;
#endif // LL_X11
	} else {
		// pretend we got what we wanted, when really we don't care.
		newmode = wantmode;
	}
	
	// return boolean success for whether we ended up in the desired state
	return (capture && SDL_GRAB_ON==newmode) ||
		(!capture && SDL_GRAB_OFF==newmode);
}

U32 LLWindowSDL::SDLCheckGrabbyKeys(SDLKey keysym, BOOL gain)
{
	/* part of the fix for SL-13243: Some popular window managers like
	   to totally eat alt-drag for the purposes of moving windows.  We
	   spoil their day by acquiring the exclusive X11 mouse lock for as
	   long as LALT is held down, so the window manager can't easily
	   see what's happening.  Tested successfully with Metacity.
	   And... do the same with CTRL, for other darn WMs.  We don't
	   care about other metakeys as SL doesn't use them with dragging
	   (for now). */

	/* We maintain a bitmap of critical keys which are up and down
	   instead of simply key-counting, because SDL sometimes reports
	   misbalanced keyup/keydown event pairs to us for whatever reason. */

	U32 mask = 0;
	switch (keysym)
	{
	case SDLK_LALT:
		mask = 1U << 0; break;
	case SDLK_LCTRL:
		mask = 1U << 1; break;
	case SDLK_RCTRL:
		mask = 1U << 2; break;
	default:
		break;
	}

	if (gain)
		mGrabbyKeyFlags |= mask;
	else
		mGrabbyKeyFlags &= ~mask;

	//llinfos << "mGrabbyKeyFlags=" << mGrabbyKeyFlags << llendl;

	/* 0 means we don't need to mousegrab, otherwise grab. */
	return mGrabbyKeyFlags;
}

void LLWindowSDL::gatherInput()
{
    const Uint32 CLICK_THRESHOLD = 300;  // milliseconds
    static int leftClick = 0;
    static int rightClick = 0;
    static Uint32 lastLeftDown = 0;
    static Uint32 lastRightDown = 0;
    SDL_Event event;

    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
            case SDL_MOUSEMOTION:
            {
                LLCoordWindow winCoord(event.button.x, event.button.y);
                LLCoordGL openGlCoord;
                convertCoords(winCoord, &openGlCoord);
				MASK mask = gKeyboard->currentMask(TRUE);
				mCallbacks->handleMouseMove(this, openGlCoord, mask);
                break;
            }

            case SDL_KEYDOWN:
                gKeyboard->handleKeyDown(event.key.keysym.sym, event.key.keysym.mod);
		// part of the fix for SL-13243
		if (SDLCheckGrabbyKeys(event.key.keysym.sym, TRUE) != 0)
			SDLReallyCaptureInput(TRUE);

                if (event.key.keysym.unicode)
		        mCallbacks->handleUnicodeChar(event.key.keysym.unicode, gKeyboard->currentMask(FALSE));
                break;

            case SDL_KEYUP:
		if (SDLCheckGrabbyKeys(event.key.keysym.sym, FALSE) == 0)
			SDLReallyCaptureInput(FALSE); // part of the fix for SL-13243

		// This is a testing hack to pop up a dialog when 4 is pressed
		//if (event.key.keysym.sym == SDLK_4)
		//OSMessageBox("a whole bunch of text goes right here, whee!  test test test.", "this is the title!", OSMB_YESNO);

		gKeyboard->handleKeyUp(event.key.keysym.sym, event.key.keysym.mod);
                break;

            case SDL_MOUSEBUTTONDOWN:
            {
                bool isDoubleClick = false;
                LLCoordWindow winCoord(event.button.x, event.button.y);
                LLCoordGL openGlCoord;
                convertCoords(winCoord, &openGlCoord);
		MASK mask = gKeyboard->currentMask(TRUE);

                if (event.button.button == SDL_BUTTON_LEFT)   // SDL doesn't manage double clicking...
                {
                    Uint32 now = SDL_GetTicks();
                    if ((now - lastLeftDown) > CLICK_THRESHOLD)
                        leftClick = 1;
                    else
                    {
                        if (++leftClick >= 2)
                        {
                            leftClick = 0;
			    isDoubleClick = true;
                        }
                    }
                    lastLeftDown = now;
                }
                else if (event.button.button == SDL_BUTTON_RIGHT)
                {
                    Uint32 now = SDL_GetTicks();
                    if ((now - lastRightDown) > CLICK_THRESHOLD)
                        rightClick = 1;
                    else
                    {
                        if (++rightClick >= 2)
                        {
                            rightClick = 0;
    					    isDoubleClick = true;
                        }
                    }
                    lastRightDown = now;
                }

                if (event.button.button == SDL_BUTTON_LEFT)  // left
                {
                    if (isDoubleClick)
				        mCallbacks->handleDoubleClick(this, openGlCoord, mask);
                    else
    				    mCallbacks->handleMouseDown(this, openGlCoord, mask);
                }

                else if (event.button.button == SDL_BUTTON_RIGHT)  // right ... yes, it's 3, not 2, in SDL...
                {
                    // right double click isn't handled right now in Second Life ... if (isDoubleClick)
				    mCallbacks->handleRightMouseDown(this, openGlCoord, mask);
                }

                else if (event.button.button == SDL_BUTTON_MIDDLE)  // middle
				    ;  // Middle mouse isn't handled right now in Second Life ... mCallbacks->handleMiddleMouseDown(this, openGlCoord, mask);
                else if (event.button.button == 4)  // mousewheel up...thanks to X11 for making SDL consider these "buttons".
					mCallbacks->handleScrollWheel(this, -1);
                else if (event.button.button == 5)  // mousewheel down...thanks to X11 for making SDL consider these "buttons".
					mCallbacks->handleScrollWheel(this, 1);

                break;
            }

            case SDL_MOUSEBUTTONUP:
            {
                LLCoordWindow winCoord(event.button.x, event.button.y);
                LLCoordGL openGlCoord;
                convertCoords(winCoord, &openGlCoord);
		MASK mask = gKeyboard->currentMask(TRUE);

                if (event.button.button == SDL_BUTTON_LEFT)  // left
				    mCallbacks->handleMouseUp(this, openGlCoord, mask);
                else if (event.button.button == SDL_BUTTON_RIGHT)  // right ... yes, it's 3, not 2, in SDL...
				    mCallbacks->handleRightMouseUp(this, openGlCoord, mask);
                else if (event.button.button == SDL_BUTTON_MIDDLE)  // middle
				    ;  // UNUSED IN SECOND LIFE RIGHT NOW mCallbacks->handleMiddleMouseUp(this, openGlCoord, mask);

                // don't handle mousewheel here...

                break;
            }

            case SDL_VIDEOEXPOSE:  // VIDEOEXPOSE doesn't specify the damage, but hey, it's OpenGL...repaint the whole thing!
			    mCallbacks->handlePaint(this, 0, 0, mWindow->w, mWindow->h);
                break;

            case SDL_VIDEORESIZE:  // *FIX: handle this?
		llinfos << "Handling a resize event: " << event.resize.w <<
			"x" << event.resize.h << llendl;

		// *FIX: I'm not sure this is necessary!
		mWindow = SDL_SetVideoMode(event.resize.w, event.resize.h, 32, mSDLFlags);
		if (!mWindow)
		{
			// *FIX: More informative dialog?
			llinfos << "Could not recreate context after resize! Quitting..." << llendl;
			if(mCallbacks->handleCloseRequest(this))
    			{
    				// Get the app to initiate cleanup.
    				mCallbacks->handleQuit(this);
    				// The app is responsible for calling destroyWindow when done with GL
    			}
                break;
		}
		
		mCallbacks->handleResize(this, event.resize.w, event.resize.h );
                break;

            case SDL_ACTIVEEVENT:
                if (event.active.state & SDL_APPINPUTFOCUS)
                {
			// Note that for SDL (particularly on X11), keyboard
			// and mouse focus are independent things.  Here we are
			// tracking keyboard focus state changes.

			// We have to do our own state massaging because SDL
			// can send us two unfocus events in a row for example,
			// which confuses the focus code [SL-24071].
			if (event.active.gain != mHaveInputFocus)
			{
				if (event.active.gain)
					mCallbacks->handleFocus(this);
				else
					mCallbacks->handleFocusLost(this);
			
				mHaveInputFocus = !!event.active.gain;
			}
                }
                if (event.active.state & SDL_APPACTIVE)
                {
					// Change in iconification/minimization state.
					if ((!event.active.gain) != mIsMinimized)
					{
						mCallbacks->handleActivate(this, !!event.active.gain);
						llinfos << "SDL deiconification state switched to " << BOOL(event.active.gain) << llendl;
			
						mIsMinimized = (!event.active.gain);
					}
					else
					{
						llinfos << "Ignored bogus redundant SDL deiconification state switch to " << BOOL(event.active.gain) << llendl;
					}
                }
                break;

            case SDL_QUIT:
			    if(mCallbacks->handleCloseRequest(this))
    			{
    				// Get the app to initiate cleanup.
    				mCallbacks->handleQuit(this);
    				// The app is responsible for calling destroyWindow when done with GL
    			}
                break;
	default:
		//llinfos << "Unhandled SDL event type " << event.type << llendl;
		break;
        }
    }

#if LL_X11
    // This is a good time to stop flashing the icon if our mFlashTimer has
    // expired.
    if (mFlashing && mFlashTimer.hasExpired())
    {
	    x11_set_urgent(FALSE);
	    mFlashing = FALSE;
    }
#endif // LL_X11
}

static SDL_Cursor *makeSDLCursorFromBMP(const char *filename, int hotx, int hoty)
{
	SDL_Cursor *sdlcursor = NULL;
	SDL_Surface *bmpsurface;

	// Load cursor pixel data from BMP file
	bmpsurface = Load_BMP_Resource(filename);
	if (bmpsurface && bmpsurface->w%8==0)
	{
		SDL_Surface *cursurface;
		llinfos << "Loaded cursor file " << filename << " "
			<< bmpsurface->w << "x" << bmpsurface->h << llendl;
		cursurface = SDL_CreateRGBSurface (SDL_SWSURFACE,
						   bmpsurface->w,
						   bmpsurface->h,
						   32,
						   0xFFU,
						   0xFF00U,
						   0xFF0000U,
						   0xFF000000U);
		SDL_FillRect(cursurface, NULL, 0x00000000U);

		// Blit the cursor pixel data onto a 32-bit RGBA surface so we
		// only have to cope with processing one type of pixel format.
		if (0 == SDL_BlitSurface(bmpsurface, NULL,
					 cursurface, NULL))
		{
			// n.b. we already checked that width is a multiple of 8.
			const int bitmap_bytes = (cursurface->w * cursurface->h) / 8;
			unsigned char *cursor_data = new unsigned char[bitmap_bytes];
			unsigned char *cursor_mask = new unsigned char[bitmap_bytes];
			memset(cursor_data, 0, bitmap_bytes);
			memset(cursor_mask, 0, bitmap_bytes);
			int i,j;
			// Walk the RGBA cursor pixel data, extracting both data and
			// mask to build SDL-friendly cursor bitmaps from.  The mask
			// is inferred by color-keying against 200,200,200
			for (i=0; i<cursurface->h; ++i) {
				for (j=0; j<cursurface->w; ++j) {
					unsigned char *pixelp =
						((unsigned char *)cursurface->pixels)
						+ cursurface->pitch * i
						+ j*cursurface->format->BytesPerPixel;
					unsigned char srcred = pixelp[0];
					unsigned char srcgreen = pixelp[1];
					unsigned char srcblue = pixelp[2];
					BOOL mask_bit = (srcred != 200)
						|| (srcgreen != 200)
						|| (srcblue != 200);
					BOOL data_bit = mask_bit && (srcgreen <= 80);//not 0x80
					unsigned char bit_offset = (cursurface->w/8) * i
						+ j/8;
					cursor_data[bit_offset]	|= (data_bit) << (7 - (j&7));
					cursor_mask[bit_offset]	|= (mask_bit) << (7 - (j&7));
				}
			}
			sdlcursor = SDL_CreateCursor((Uint8*)cursor_data,
						     (Uint8*)cursor_mask,
						     cursurface->w, cursurface->h,
						     hotx, hoty);
			delete[] cursor_data;
			delete[] cursor_mask;
		} else {
			llwarns << "CURSOR BLIT FAILURE, cursurface: " << cursurface << llendl;
		}
		SDL_FreeSurface(cursurface);
		SDL_FreeSurface(bmpsurface);
	} else {
		llwarns << "CURSOR LOAD FAILURE " << filename << llendl;
	}

	return sdlcursor;
}

void LLWindowSDL::setCursor(ECursorType cursor)
{
	if (mCurrentCursor != cursor)
	{
		if (cursor < UI_CURSOR_COUNT)
		{
			SDL_Cursor *sdlcursor = mSDLCursors[cursor];
			// Try to default to the arrow for any cursors that
			// did not load correctly.
			if (!sdlcursor && mSDLCursors[UI_CURSOR_ARROW])
				sdlcursor = mSDLCursors[UI_CURSOR_ARROW];
			if (sdlcursor)
				SDL_SetCursor(sdlcursor);
		} else {
			llwarns << "Tried to set invalid cursor number " << cursor << llendl;
		}
		mCurrentCursor = cursor;
	}
}

ECursorType LLWindowSDL::getCursor()
{
	return mCurrentCursor;
}

void LLWindowSDL::initCursors()
{
	int i;
	// Blank the cursor pointer array for those we may miss.
	for (i=0; i<UI_CURSOR_COUNT; ++i)
	{
		mSDLCursors[i] = NULL;
	}
	// Pre-make an SDL cursor for each of the known cursor types.
	// We hardcode the hotspots - to avoid that we'd have to write
	// a .cur file loader.
	// NOTE: SDL doesn't load RLE-compressed BMP files.
	mSDLCursors[UI_CURSOR_ARROW] = makeSDLCursorFromBMP("llarrow.BMP",0,0);
	mSDLCursors[UI_CURSOR_WAIT] = makeSDLCursorFromBMP("wait.BMP",12,15);
	mSDLCursors[UI_CURSOR_HAND] = makeSDLCursorFromBMP("hand.BMP",7,10);
	mSDLCursors[UI_CURSOR_IBEAM] = makeSDLCursorFromBMP("ibeam.BMP",15,16);
	mSDLCursors[UI_CURSOR_CROSS] = makeSDLCursorFromBMP("cross.BMP",16,14);
	mSDLCursors[UI_CURSOR_SIZENWSE] = makeSDLCursorFromBMP("sizenwse.BMP",14,17);
	mSDLCursors[UI_CURSOR_SIZENESW] = makeSDLCursorFromBMP("sizenesw.BMP",17,17);
	mSDLCursors[UI_CURSOR_SIZEWE] = makeSDLCursorFromBMP("sizewe.BMP",16,14);
	mSDLCursors[UI_CURSOR_SIZENS] = makeSDLCursorFromBMP("sizens.BMP",17,16);
	mSDLCursors[UI_CURSOR_NO] = makeSDLCursorFromBMP("llno.BMP",8,8);
	mSDLCursors[UI_CURSOR_WORKING] = makeSDLCursorFromBMP("working.BMP",12,15);
	mSDLCursors[UI_CURSOR_TOOLGRAB] = makeSDLCursorFromBMP("lltoolgrab.BMP",2,13);
	mSDLCursors[UI_CURSOR_TOOLLAND] = makeSDLCursorFromBMP("lltoolland.BMP",1,6);
	mSDLCursors[UI_CURSOR_TOOLFOCUS] = makeSDLCursorFromBMP("lltoolfocus.BMP",8,5);
	mSDLCursors[UI_CURSOR_TOOLCREATE] = makeSDLCursorFromBMP("lltoolcreate.BMP",7,7);
	mSDLCursors[UI_CURSOR_ARROWDRAG] = makeSDLCursorFromBMP("arrowdrag.BMP",0,0);
	mSDLCursors[UI_CURSOR_ARROWCOPY] = makeSDLCursorFromBMP("arrowcop.BMP",0,0);
	mSDLCursors[UI_CURSOR_ARROWDRAGMULTI] = makeSDLCursorFromBMP("llarrowdragmulti.BMP",0,0);
	mSDLCursors[UI_CURSOR_ARROWCOPYMULTI] = makeSDLCursorFromBMP("arrowcopmulti.BMP",0,0);
	mSDLCursors[UI_CURSOR_NOLOCKED] = makeSDLCursorFromBMP("llnolocked.BMP",8,8);
	mSDLCursors[UI_CURSOR_ARROWLOCKED] = makeSDLCursorFromBMP("llarrowlocked.BMP",0,0);
	mSDLCursors[UI_CURSOR_GRABLOCKED] = makeSDLCursorFromBMP("llgrablocked.BMP",2,13);
	mSDLCursors[UI_CURSOR_TOOLTRANSLATE] = makeSDLCursorFromBMP("lltooltranslate.BMP",0,0);
	mSDLCursors[UI_CURSOR_TOOLROTATE] = makeSDLCursorFromBMP("lltoolrotate.BMP",0,0);
	mSDLCursors[UI_CURSOR_TOOLSCALE] = makeSDLCursorFromBMP("lltoolscale.BMP",0,0);
	mSDLCursors[UI_CURSOR_TOOLCAMERA] = makeSDLCursorFromBMP("lltoolcamera.BMP",7,5);
	mSDLCursors[UI_CURSOR_TOOLPAN] = makeSDLCursorFromBMP("lltoolpan.BMP",7,5);
	mSDLCursors[UI_CURSOR_TOOLZOOMIN] = makeSDLCursorFromBMP("lltoolzoomin.BMP",7,5);
	mSDLCursors[UI_CURSOR_TOOLPICKOBJECT3] = makeSDLCursorFromBMP("toolpickobject3.BMP",0,0);
	mSDLCursors[UI_CURSOR_TOOLSIT] = makeSDLCursorFromBMP("toolsit.BMP",0,0);
	mSDLCursors[UI_CURSOR_TOOLBUY] = makeSDLCursorFromBMP("toolbuy.BMP",0,0);
	mSDLCursors[UI_CURSOR_TOOLPAY] = makeSDLCursorFromBMP("toolpay.BMP",0,0);
	mSDLCursors[UI_CURSOR_TOOLOPEN] = makeSDLCursorFromBMP("toolopen.BMP",0,0);
	mSDLCursors[UI_CURSOR_PIPETTE] = makeSDLCursorFromBMP("lltoolpipette.BMP",2,28);
}

void LLWindowSDL::quitCursors()
{
	int i;
	if (mWindow)
	{
		for (i=0; i<UI_CURSOR_COUNT; ++i)
		{
			if (mSDLCursors[i])
			{
				SDL_FreeCursor(mSDLCursors[i]);
				mSDLCursors[i] = NULL;
			}
		}
	} else {
		// SDL doesn't refcount cursors, so if the window has
		// already been destroyed then the cursors have gone with it.
		llinfos << "Skipping quitCursors: mWindow already gone." << llendl;
		for (i=0; i<UI_CURSOR_COUNT; ++i)
			mSDLCursors[i] = NULL;
	}
}

void LLWindowSDL::captureMouse()
{
	// SDL already enforces the semantics that captureMouse is
	// used for, i.e. that we continue to get mouse events as long
	// as a button is down regardless of whether we left the
	// window, and in a less obnoxious way than SDL_WM_GrabInput
	// which would confine the cursor to the window too.

	//llinfos << "LLWindowSDL::captureMouse" << llendl;
}

void LLWindowSDL::releaseMouse()
{
	// see LWindowSDL::captureMouse()
	
	//llinfos << "LLWindowSDL::releaseMouse" << llendl;
}

void LLWindowSDL::hideCursor()
{
	if(!mCursorHidden)
	{
		//		llinfos << "hideCursor: hiding" << llendl;
		mCursorHidden = TRUE;
		mHideCursorPermanent = TRUE;
		SDL_ShowCursor(0);
	}
	else
	{
		//		llinfos << "hideCursor: already hidden" << llendl;
	}

	adjustCursorDecouple();
}

void LLWindowSDL::showCursor()
{
	if(mCursorHidden)
	{
		//		llinfos << "showCursor: showing" << llendl;
		mCursorHidden = FALSE;
		mHideCursorPermanent = FALSE;
		SDL_ShowCursor(1);
	}
	else
	{
		//		llinfos << "showCursor: already visible" << llendl;
	}

	adjustCursorDecouple();
}

void LLWindowSDL::showCursorFromMouseMove()
{
	if (!mHideCursorPermanent)
	{
		showCursor();
	}
}

void LLWindowSDL::hideCursorUntilMouseMove()
{
	if (!mHideCursorPermanent)
	{
		hideCursor();
		mHideCursorPermanent = FALSE;
	}
}



//
// LLSplashScreenSDL
//
LLSplashScreenSDL::LLSplashScreenSDL()
{
}

LLSplashScreenSDL::~LLSplashScreenSDL()
{
}

void LLSplashScreenSDL::showImpl()
{
}

void LLSplashScreenSDL::updateImpl(const char* mesg)
{
}


void LLSplashScreenSDL::hideImpl()
{
}



#if LL_GTK
static void response_callback (GtkDialog *dialog,
			       gint       arg1,
			       gpointer   user_data)
{
	gint *response = (gint*)user_data;
	*response = arg1;
	gtk_widget_destroy(GTK_WIDGET(dialog));
	gtk_main_quit();
}

S32 OSMessageBoxSDL(const char* text, const char* caption, U32 type)
{
	S32 rtn = OSBTN_CANCEL;

#if LL_GTK
	maybe_do_gtk_diagnostics();
#endif // LL_GTK

	if(gWindowImplementation != NULL)
		gWindowImplementation->beforeDialog();

	gtk_disable_setlocale();
	if (gtk_init_check(NULL, NULL)
	    // We can NOT expect to combine GTK and SDL's aggressive fullscreen
	    && ((NULL==gWindowImplementation) || (!was_fullscreen))
	    )
	{
		GtkWidget *win = NULL;

		llinfos << "Creating a dialog because we're in windowed mode and GTK is happy." << llendl;
		
		GtkDialogFlags flags = GTK_DIALOG_MODAL;
		GtkMessageType messagetype;
		GtkButtonsType buttons;
		switch (type)
		{
		default:
		case OSMB_OK:
			messagetype = GTK_MESSAGE_WARNING;
			buttons = GTK_BUTTONS_OK;
			break;
		case OSMB_OKCANCEL:
			messagetype = GTK_MESSAGE_QUESTION;
			buttons = GTK_BUTTONS_OK_CANCEL;
			break;
		case OSMB_YESNO:
			messagetype = GTK_MESSAGE_QUESTION;
			buttons = GTK_BUTTONS_YES_NO;
			break;
		}
		win = gtk_message_dialog_new(NULL,
                                             flags, messagetype, buttons,
                                             text);

# if LL_X11
		// Make GTK tell the window manager to associate this
		// dialog with our non-GTK SDL window, which should try
		// to keep it on top etc.
		if (SDL_XWindowID != None)
		{
			gtk_widget_realize(GTK_WIDGET(win)); // so we can get its gdkwin
			GdkWindow *gdkwin = gdk_window_foreign_new(SDL_XWindowID);
			gdk_window_set_transient_for(GTK_WIDGET(win)->window,
						     gdkwin);
		}
# endif //LL_X11

		gtk_window_set_position(GTK_WINDOW(win),
					GTK_WIN_POS_CENTER_ON_PARENT);

		gtk_window_set_type_hint(GTK_WINDOW(win),
					 GDK_WINDOW_TYPE_HINT_DIALOG);

		if (caption)
			gtk_window_set_title(GTK_WINDOW(win), caption);

		gint response = GTK_RESPONSE_NONE;
		g_signal_connect (win,
				  "response", 
				  G_CALLBACK (response_callback),
				  &response);

		// we should be able to us a gtk_dialog_run(), but it's
		// apparently not written to exist in a world without a higher
		// gtk_main(), so we manage its signal/destruction outselves.
		gtk_widget_show_all (win);
		gtk_main();

		//llinfos << "response: " << response << llendl;
		switch (response)
		{
		case GTK_RESPONSE_OK:     rtn = OSBTN_OK; break;
		case GTK_RESPONSE_YES:    rtn = OSBTN_YES; break;
		case GTK_RESPONSE_NO:     rtn = OSBTN_NO; break;
		case GTK_RESPONSE_APPLY:  rtn = OSBTN_OK; break;
		case GTK_RESPONSE_NONE:
		case GTK_RESPONSE_CANCEL:
		case GTK_RESPONSE_CLOSE:
		case GTK_RESPONSE_DELETE_EVENT:
		default: rtn = OSBTN_CANCEL;
		}
	}
	else
	{
		fprintf(stderr, "MSGBOX: %s: %s\n", caption, text);
		llinfos << "Skipping dialog because we're in fullscreen mode or GTK is not happy." << llendl;
		rtn = OSBTN_OK;
	}

	if(gWindowImplementation != NULL)
		gWindowImplementation->afterDialog();

	return rtn;
}

static void color_changed_callback(GtkWidget *widget,
				   gpointer user_data)
{
	GtkColorSelection *colorsel = GTK_COLOR_SELECTION(widget);
	GdkColor *colorp = (GdkColor*)user_data;
	
	gtk_color_selection_get_current_color(colorsel, colorp);
}

BOOL LLWindowSDL::dialog_color_picker ( F32 *r, F32 *g, F32 *b)
{
	BOOL rtn = FALSE;

	beforeDialog();

	gtk_disable_setlocale();
	if (gtk_init_check(NULL, NULL)
	    // We can NOT expect to combine GTK and SDL's aggressive fullscreen
	    && !was_fullscreen
	    )
	{
		GtkWidget *win = NULL;

		win = gtk_color_selection_dialog_new(NULL);

# if LL_X11
		// Get GTK to tell the window manager to associate this
		// dialog with our non-GTK SDL window, which should try
		// to keep it on top etc.
		if (SDL_XWindowID != None)
		{
			gtk_widget_realize(GTK_WIDGET(win)); // so we can get its gdkwin
			GdkWindow *gdkwin = gdk_window_foreign_new(SDL_XWindowID);
			gdk_window_set_transient_for(GTK_WIDGET(win)->window,
						     gdkwin);
		}
# endif //LL_X11

		GtkColorSelection *colorsel = GTK_COLOR_SELECTION (GTK_COLOR_SELECTION_DIALOG(win)->colorsel);

		GdkColor color, orig_color;
		orig_color.red = guint16(65535 * *r);
		orig_color.green= guint16(65535 * *g);
		orig_color.blue = guint16(65535 * *b);
		color = orig_color;

		gtk_color_selection_set_previous_color (colorsel, &color);
		gtk_color_selection_set_current_color (colorsel, &color);
		gtk_color_selection_set_has_palette (colorsel, TRUE);
		gtk_color_selection_set_has_opacity_control(colorsel, FALSE);

		gint response = GTK_RESPONSE_NONE;
		g_signal_connect (win,
				  "response", 
				  G_CALLBACK (response_callback),
				  &response);

		g_signal_connect (G_OBJECT (colorsel), "color_changed",
				  G_CALLBACK (color_changed_callback),
				  &color);

		gtk_window_set_modal(GTK_WINDOW(win), TRUE);
		gtk_widget_show_all(win);
		// hide the help button - we don't service it.
		gtk_widget_hide(GTK_COLOR_SELECTION_DIALOG(win)->help_button);
		gtk_main();

		if (response == GTK_RESPONSE_OK &&
		    (orig_color.red != color.red
		     || orig_color.green != color.green
		     || orig_color.blue != color.blue) )
		{
			*r = color.red / 65535.0f;
			*g = color.green / 65535.0f;
			*b = color.blue / 65535.0f;
			rtn = TRUE;
		}
	}

	afterDialog();

	return rtn;
}
#else
S32 OSMessageBoxSDL(const char* text, const char* caption, U32 type)
{
	fprintf(stderr, "MSGBOX: %s: %s\n", caption, text);
	return 0;
}

BOOL LLWindowSDL::dialog_color_picker ( F32 *r, F32 *g, F32 *b)
{
	return (FALSE);
}
#endif // LL_GTK

// Open a URL with the user's default web browser.
// Must begin with protocol identifier.
void spawn_web_browser(const char* escaped_url)
{
	llinfos << "spawn_web_browser: " << escaped_url << llendl;
	
#if LL_LINUX
#  if LL_X11
	if (SDL_Display) // Just in case - before forking.
		XSync(SDL_Display, False);
#  endif // LL_X11

	std::string cmd;
	cmd  = gDirUtilp->getAppRODataDir().c_str();
	cmd += gDirUtilp->getDirDelimiter().c_str();
	cmd += "launch_url.sh";
	char* const argv[] = {(char*)cmd.c_str(), (char*)escaped_url, NULL};

	pid_t pid = fork();
	if (pid == 0)
	{ // child
		// disconnect from stdin/stdout/stderr, or child will
		// keep our output pipe undesirably alive if it outlives us.
		close(0);
		close(1);
		close(2);
		// end ourself by running the command
		execv(cmd.c_str(), argv);
		// if execv returns at all, there was a problem.
		llwarns << "execv failure when trying to start " << cmd << llendl;
		_exit(1); // _exit because we don't want atexit() clean-up!
	} else {
		if (pid > 0)
		{
			// parent - wait for child to die
			int childExitStatus;
			waitpid(pid, &childExitStatus, 0);
		} else {
			llwarns << "fork failure." << llendl;
		}
	}
#endif // LL_LINUX

	llinfos << "spawn_web_browser returning." << llendl;
}

void shell_open( const char* file_path )
{
    // *FIX: (???)
    fprintf(stderr, "shell_open: %s\n", file_path);
}

void *LLWindowSDL::getPlatformWindow()
{
#if LL_X11
	// pointer to our static raw X window
	return (void*)&SDL_XWindowID;
#else
	// doubt we really want to return a high-level SDL structure here.
	return NULL;
#endif
}

void LLWindowSDL::bringToFront()
{
    // *FIX: (???)
    fprintf(stderr, "bringToFront\n");
}

#endif // LL_SDL
