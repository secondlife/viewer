/** 
 * @file llwindowsdl.cpp
 * @brief SDL implementation of LLWindow class
 * @author This module has many fathers, and it shows.
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

#if LL_SDL

#include "linden_common.h"

#include "llwindowsdl.h"

#include "llwindowcallbacks.h"
#include "llkeyboardsdl.h"

#include "llerror.h"
#include "llgl.h"
#include "llstring.h"
#include "lldir.h"
#include "llfindlocale.h"

#if LL_GTK
extern "C" {
# include "gtk/gtk.h"
}
#include <locale.h>
#endif // LL_GTK

extern "C" {
# include "fontconfig/fontconfig.h"
}

#if LL_LINUX || LL_SOLARIS
// not necessarily available on random SDL platforms, so #if LL_LINUX
// for execv(), waitpid(), fork()
# include <unistd.h>
# include <sys/types.h>
# include <sys/wait.h>
#endif // LL_LINUX || LL_SOLARIS

extern BOOL gDebugWindowProc;

const S32 MAX_NUM_RESOLUTIONS = 200;

// static variable for ATI mouse cursor crash work-around:
static bool ATIbug = false; 

//
// LLWindowSDL
//

#if LL_X11
# include <X11/Xutil.h>
#endif //LL_X11

// TOFU HACK -- (*exactly* the same hack as LLWindowMacOSX for a similar
// set of reasons): Stash a pointer to the LLWindowSDL object here and
// maintain in the constructor and destructor.  This assumes that there will
// be only one object of this class at any time.  Currently this is true.
static LLWindowSDL *gWindowImplementation = NULL;


void maybe_lock_display(void)
{
	if (gWindowImplementation && gWindowImplementation->Lock_Display) {
		gWindowImplementation->Lock_Display();
	}
}


void maybe_unlock_display(void)
{
	if (gWindowImplementation && gWindowImplementation->Unlock_Display) {
		gWindowImplementation->Unlock_Display();
	}
}


#if LL_GTK
// Lazily initialize and check the runtime GTK version for goodness.
// static
bool LLWindowSDL::ll_try_gtk_init(void)
{
	static BOOL done_gtk_diag = FALSE;
	static BOOL gtk_is_good = FALSE;
	static BOOL done_setlocale = FALSE;
	static BOOL tried_gtk_init = FALSE;

	if (!done_setlocale)
	{
		llinfos << "Starting GTK Initialization." << llendl;
		maybe_lock_display();
		gtk_disable_setlocale();
		maybe_unlock_display();
		done_setlocale = TRUE;
	}
	
	if (!tried_gtk_init)
	{
		tried_gtk_init = TRUE;
		if (!g_thread_supported ()) g_thread_init (NULL);
		maybe_lock_display();
		gtk_is_good = gtk_init_check(NULL, NULL);
		maybe_unlock_display();
		if (!gtk_is_good)
			llwarns << "GTK Initialization failed." << llendl;
	}

	if (gtk_is_good && !done_gtk_diag)
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
		maybe_lock_display();
		const gchar* gtk_warning = gtk_check_version(
			GTK_MAJOR_VERSION,
			GTK_MINOR_VERSION,
			GTK_MICRO_VERSION);
		maybe_unlock_display();
		if (gtk_warning)
		{
			llwarns << "- GTK COMPATIBILITY WARNING: " <<
				gtk_warning << llendl;
			gtk_is_good = FALSE;
		} else {
			llinfos << "- GTK version is good." << llendl;
		}

		done_gtk_diag = TRUE;
	}

	return gtk_is_good;
}
#endif // LL_GTK


#if LL_X11
// static
Window LLWindowSDL::get_SDL_XWindowID(void)
{
	if (gWindowImplementation) {
		return gWindowImplementation->mSDL_XWindowID;
	}
	return None;
}

//static
Display* LLWindowSDL::get_SDL_Display(void)
{
	if (gWindowImplementation) {
		return gWindowImplementation->mSDL_Display;
	}
	return NULL;
}
#endif // LL_X11


LLWindowSDL::LLWindowSDL(LLWindowCallbacks* callbacks,
			 const std::string& title, S32 x, S32 y, S32 width,
			 S32 height, U32 flags,
			 BOOL fullscreen, BOOL clearBg,
			 BOOL disable_vsync,
			 BOOL ignore_pixel_depth, U32 fsaa_samples)
	: LLWindow(callbacks, fullscreen, flags),
	  Lock_Display(NULL),
	  Unlock_Display(NULL), mGamma(1.0f)
{
	// Initialize the keyboard
	gKeyboard = new LLKeyboardSDL();
	gKeyboard->setCallbacks(callbacks);
	// Note that we can't set up key-repeat until after SDL has init'd video

	mWindow = NULL;
	mNeedsResize = FALSE;
	mOverrideAspectRatio = 0.f;
	mGrabbyKeyFlags = 0;
	mReallyCapturedCount = 0;
	mHaveInputFocus = -1;
	mIsMinimized = -1;
	mFSAASamples = fsaa_samples;

#if LL_X11
	mSDL_XWindowID = None;
	mSDL_Display = NULL;
#endif // LL_X11

#if LL_GTK
	// We MUST be the first to initialize GTK so that GTK doesn't get badly
	// initialized with a non-C locale and cause lots of serious random
	// weirdness.
	ll_try_gtk_init();
#endif // LL_GTK

	// Assume 4:3 aspect ratio until we know better
	mOriginalAspectRatio = 1024.0 / 768.0;

	if (title.empty())
		mWindowTitle = "SDL Window";  // *FIX: (???)
	else
		mWindowTitle = title;

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

	mKeyScanCode = 0;
	mKeyVirtualKey = 0;
	mKeyModifiers = KMOD_NONE;
}

static SDL_Surface *Load_BMP_Resource(const char *basename)
{
	const int PATH_BUFFER_SIZE=1000;
	char path_buffer[PATH_BUFFER_SIZE];	/* Flawfinder: ignore */
	
	// Figure out where our BMP is living on the disk
	snprintf(path_buffer, PATH_BUFFER_SIZE-1, "%s%sres-sdl%s%s",	
		 gDirUtilp->getAppRODataDir().c_str(),
		 gDirUtilp->getDirDelimiter().c_str(),
		 gDirUtilp->getDirDelimiter().c_str(),
		 basename);
	path_buffer[PATH_BUFFER_SIZE-1] = '\0';
	
	return SDL_LoadBMP(path_buffer);
}

#if LL_X11
// This is an XFree86/XOrg-specific hack for detecting the amount of Video RAM
// on this machine.  It works by searching /var/log/var/log/Xorg.?.log or
// /var/log/XFree86.?.log for a ': (VideoRAM ?|Memory): (%d+) kB' regex, where
// '?' is the X11 display number derived from $DISPLAY
static int x11_detect_VRAM_kb_fp(FILE *fp, const char *prefix_str)
{
	const int line_buf_size = 1000;
	char line_buf[line_buf_size];
	while (fgets(line_buf, line_buf_size, fp))
	{
		//lldebugs << "XLOG: " << line_buf << llendl;

		// Why the ad-hoc parser instead of using a regex?  Our
		// favourite regex implementation - libboost_regex - is
		// quite a heavy and troublesome dependency for the client, so
		// it seems a shame to introduce it for such a simple task.
		// *FIXME: libboost_regex is a dependency now anyway, so we may
		// as well use it instead of this hand-rolled nonsense.
		const char *part1_template = prefix_str;
		const char part2_template[] = " kB";
		char *part1 = strstr(line_buf, part1_template);
		if (part1) // found start of matching line
		{
			part1 = &part1[strlen(part1_template)]; // -> after
			char *part2 = strstr(part1, part2_template);
			if (part2) // found end of matching line
			{
				// now everything between part1 and part2 is
				// supposed to be numeric, describing the
				// number of kB of Video RAM supported
				int rtn = 0;
				for (; part1 < part2; ++part1)
				{
					if (*part1 < '0' || *part1 > '9')
					{
						// unexpected char, abort parse
						rtn = 0;
						break;
					}
					rtn *= 10;
					rtn += (*part1) - '0';
				}
				if (rtn > 0)
				{
					// got the kB number.  return it now.
					return rtn;
				}
			}
		}
	}
	return 0; // 'could not detect'
}

static int x11_detect_VRAM_kb()
{
#if LL_SOLARIS && defined(__sparc)
      //  NOTE: there's no Xorg server on SPARC so just return 0
      //        and allow SDL to attempt to get the amount of VRAM
      return(0);
#else

	std::string x_log_location("/var/log/");
	std::string fname;
	int rtn = 0; // 'could not detect'
	int display_num = 0;
	FILE *fp;
	char *display_env = getenv("DISPLAY"); // e.g. :0 or :0.0 or :1.0 etc
	// parse DISPLAY number so we can go grab the right log file
	if (display_env[0] == ':' &&
	    display_env[1] >= '0' && display_env[1] <= '9')
	{
		display_num = display_env[1] - '0';
	}

	// *TODO: we could be smarter and see which of Xorg/XFree86 has the
	// freshest time-stamp.

	// Try Xorg log first
	fname = x_log_location;
	fname += "Xorg.";
	fname += ('0' + display_num);
	fname += ".log";
	fp = fopen(fname.c_str(), "r");
	if (fp)
	{
		llinfos << "Looking in " << fname
			<< " for VRAM info..." << llendl;
		rtn = x11_detect_VRAM_kb_fp(fp, ": VideoRAM: ");
		fclose(fp);
		if (0 == rtn)
		{
			fp = fopen(fname.c_str(), "r");
			if (fp)
			{
				rtn = x11_detect_VRAM_kb_fp(fp, ": Video RAM: ");
				fclose(fp);
				if (0 == rtn)
				{
					fp = fopen(fname.c_str(), "r");
					if (fp)
					{
						rtn = x11_detect_VRAM_kb_fp(fp, ": Memory: ");
						fclose(fp);
					}
				}
			}
		}
	}
	else
	{
		llinfos << "Could not open " << fname
			<< " - skipped." << llendl;	
		// Try old XFree86 log otherwise
		fname = x_log_location;
		fname += "XFree86.";
		fname += ('0' + display_num);
		fname += ".log";
		fp = fopen(fname.c_str(), "r");
		if (fp)
		{
			llinfos << "Looking in " << fname
				<< " for VRAM info..." << llendl;
			rtn = x11_detect_VRAM_kb_fp(fp, ": VideoRAM: ");
			fclose(fp);
			if (0 == rtn)
			{
				fp = fopen(fname.c_str(), "r");
				if (fp)
				{
					rtn = x11_detect_VRAM_kb_fp(fp, ": Memory: ");
					fclose(fp);
				}
			}
		}
		else
		{
			llinfos << "Could not open " << fname
				<< " - skipped." << llendl;
		}
	}
	return rtn;
#endif // LL_SOLARIS
}
#endif // LL_X11

BOOL LLWindowSDL::createContext(int x, int y, int width, int height, int bits, BOOL fullscreen, BOOL disable_vsync)
{
	//bool			glneedsinit = false;

	llinfos << "createContext, fullscreen=" << fullscreen <<
	    " size=" << width << "x" << height << llendl;

	// captures don't survive contexts
	mGrabbyKeyFlags = 0;
	mReallyCapturedCount = 0;
	
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		llinfos << "sdl_init() failed! " << SDL_GetError() << llendl;
		setupFailure("sdl_init() failure,  window creation error", "error", OSMB_OK);
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

	const SDL_VideoInfo *video_info = SDL_GetVideoInfo( );
	if (!video_info)
	{
		llinfos << "SDL_GetVideoInfo() failed! " << SDL_GetError() << llendl;
		setupFailure("SDL_GetVideoInfo() failed, Window creation error", "Error", OSMB_OK);
		return FALSE;
	}

	if (video_info->current_h > 0)
	{
		mOriginalAspectRatio = (float)video_info->current_w / (float)video_info->current_h;
		llinfos << "Original aspect ratio was " << video_info->current_w << ":" << video_info->current_h << "=" << mOriginalAspectRatio << llendl;
	}

	SDL_EnableUNICODE(1);
	SDL_WM_SetCaption(mWindowTitle.c_str(), mWindowTitle.c_str());

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
#if !LL_SOLARIS
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, (bits <= 16) ? 16 : 24);
	// We need stencil support for a few (minor) things.
	if (!getenv("LL_GL_NO_STENCIL"))
		SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
#else
	// NOTE- use smaller Z-buffer to enable more graphics cards
        //     - This should not affect better GPUs and has been proven
        //	 to provide 24-bit z-buffers when available.
	//
        // As the API states: 
	//
        // GLX_DEPTH_SIZE    Must be followed by a nonnegative
        //                   minimum size specification.  If this
        //                   value is zero, visuals with no depth
        //                   buffer are preferred.  Otherwise, the
        //                   largest available depth buffer of at
        //                   least the minimum size is preferred.

        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
#endif
        SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, (bits <= 16) ? 1 : 8);

        // *FIX: try to toggle vsync here?

	mFullscreen = fullscreen;

	int sdlflags = SDL_OPENGL | SDL_RESIZABLE | SDL_ANYFORMAT;

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	if (mFSAASamples > 0)
	{
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, mFSAASamples);
	}
	
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
		if (!mWindow && bits > 16)
		{
			SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
			mWindow = SDL_SetVideoMode(width, height, bits, sdlflags | SDL_FULLSCREEN);
		}

		if (mWindow)
		{
			mFullscreen = TRUE;
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
			mFullscreenWidth   = -1;
			mFullscreenHeight  = -1;
			mFullscreenBits    = -1;
			mFullscreenRefresh = -1;

			std::string error = llformat("Unable to run fullscreen at %d x %d.\nRunning in window.", width, height);	
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
		if (!mWindow && bits > 16)
		{
			SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
			mWindow = SDL_SetVideoMode(width, height, bits, sdlflags);
		}

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
	
	// Detect video memory size.
# if LL_X11
	gGLManager.mVRAM = x11_detect_VRAM_kb() / 1024;
	if (gGLManager.mVRAM != 0)
	{
		llinfos << "X11 log-parser detected " << gGLManager.mVRAM << "MB VRAM." << llendl;
	} else
# endif // LL_X11
	{
		// fallback to letting SDL detect VRAM.
		// note: I've not seen SDL's detection ever actually find
		// VRAM != 0, but if SDL *does* detect it then that's a bonus.
		gGLManager.mVRAM = video_info->video_mem / 1024;
		if (gGLManager.mVRAM != 0)
		{
			llinfos << "SDL detected " << gGLManager.mVRAM << "MB VRAM." << llendl;
		}
	}
	// If VRAM is not detected, that is handled later

	// *TODO: Now would be an appropriate time to check for some
	// explicitly unsupported cards.
	//const char* RENDERER = (const char*) glGetString(GL_RENDERER);

	GLint depthBits, stencilBits, redBits, greenBits, blueBits, alphaBits;

	glGetIntegerv(GL_RED_BITS, &redBits);
	glGetIntegerv(GL_GREEN_BITS, &greenBits);
	glGetIntegerv(GL_BLUE_BITS, &blueBits);
	glGetIntegerv(GL_ALPHA_BITS, &alphaBits);
	glGetIntegerv(GL_DEPTH_BITS, &depthBits);
	glGetIntegerv(GL_STENCIL_BITS, &stencilBits);
	
	llinfos << "GL buffer:" << llendl;
        llinfos << "  Red Bits " << S32(redBits) << llendl;
        llinfos << "  Green Bits " << S32(greenBits) << llendl;
        llinfos << "  Blue Bits " << S32(blueBits) << llendl;
	llinfos	<< "  Alpha Bits " << S32(alphaBits) << llendl;
	llinfos	<< "  Depth Bits " << S32(depthBits) << llendl;
	llinfos	<< "  Stencil Bits " << S32(stencilBits) << llendl;

	GLint colorBits = redBits + greenBits + blueBits + alphaBits;
	// fixme: actually, it's REALLY important for picking that we get at
	// least 8 bits each of red,green,blue.  Alpha we can be a bit more
	// relaxed about if we have to.
#if LL_SOLARIS && defined(__sparc)
//  again the __sparc required because Xsun support, 32bit are very pricey on SPARC
	if(colorBits < 24)		//HACK:  on SPARC allow 24-bit color
#else
	if (colorBits < 32)
#endif
	{
		close();
		setupFailure(
#if LL_SOLARIS && defined(__sparc)
			"Second Life requires at least 24-bit color on SPARC to run in a window.\n"
			"Please use fbconfig to set your default color depth to 24 bits.\n"
			"You may also need to adjust the X11 setting in SMF.  To do so use\n"
			"  'svccfg -s svc:/application/x11/x11-server setprop options/default_depth=24'\n"
#else
			"Second Life requires True Color (32-bit) to run in a window.\n"
			"Please go to Control Panels -> Display -> Settings and\n"
			"set the screen to 32-bit color.\n"
#endif
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
	/* Grab the window manager specific information */
	SDL_SysWMinfo info;
	SDL_VERSION(&info.version);
	if ( SDL_GetWMInfo(&info) )
	{
		/* Save the information for later use */
		if ( info.subsystem == SDL_SYSWM_X11 )
		{
			mSDL_Display = info.info.x11.display;
			mSDL_XWindowID = info.info.x11.wmwindow;
			Lock_Display = info.info.x11.lock_func;
			Unlock_Display = info.info.x11.unlock_func;
		}
		else
		{
			llwarns << "We're not running under X11?  Wild."
				<< llendl;
		}
	}
	else
	{
		llwarns << "We're not running under any known WM.  Wild."
			<< llendl;
	}
#endif // LL_X11


	//make sure multisampling is disabled by default
	glDisable(GL_MULTISAMPLE_ARB);
	
	// We need to do this here, once video is init'd
	if (-1 == SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY,
				      SDL_DEFAULT_REPEAT_INTERVAL))
	    llwarns << "Couldn't enable key-repeat: " << SDL_GetError() <<llendl;

	// Don't need to get the current gamma, since there's a call that restores it to the system defaults.
	return TRUE;
}


// changing fullscreen resolution, or switching between windowed and fullscreen mode.
BOOL LLWindowSDL::switchContext(BOOL fullscreen, const LLCoordScreen &size, BOOL disable_vsync, const LLCoordScreen * const posp)
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
	mSDL_Display = NULL;
	mSDL_XWindowID = None;
	Lock_Display = NULL;
	Unlock_Display = NULL;
#endif // LL_X11

	// Clean up remaining GL state before blowing away window
	llinfos << "shutdownGL begins" << llendl;
	gGLManager.shutdownGL();
	llinfos << "SDL_QuitSS/VID begins" << llendl;
	SDL_QuitSubSystem(SDL_INIT_VIDEO);  // *FIX: this might be risky...

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

//virtual
void LLWindowSDL::minimize()
{
    // *FIX: What to do with SDL?
}

//virtual
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

BOOL LLWindowSDL::setSizeImpl(const LLCoordScreen size)
{
	if(mWindow)
	{
		// Push a resize event onto SDL's queue - we'll handle it
		// when it comes out again.
		SDL_Event event;
		event.type = SDL_VIDEORESIZE;
		event.resize.w = size.mX;
		event.resize.h = size.mY;
		SDL_PushEvent(&event); // copied into queue

		return TRUE;
	}
		
	return FALSE;
}

BOOL LLWindowSDL::setSizeImpl(const LLCoordWindow size)
{
	if(mWindow)
	{
		// Push a resize event onto SDL's queue - we'll handle it
		// when it comes out again.
		SDL_Event event;
		event.type = SDL_VIDEORESIZE;
		event.resize.w = size.mX;
		event.resize.h = size.mY;
		SDL_PushEvent(&event); // copied into queue

		return TRUE;
	}

	return FALSE;
}


void LLWindowSDL::swapBuffers()
{
	if (mWindow)
	{	
		SDL_GL_SwapBuffers();
	}
}

U32 LLWindowSDL::getFSAASamples()
{
	return mFSAASamples;
}

void LLWindowSDL::setFSAASamples(const U32 samples)
{
	mFSAASamples = samples;
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
    //SDL_WM_GrabInput(b ? SDL_GRAB_ON : SDL_GRAB_OFF);
}

// virtual
void LLWindowSDL::setMinSize(U32 min_width, U32 min_height, bool enforce_immediately)
{
	LLWindow::setMinSize(min_width, min_height, enforce_immediately);

#if LL_X11
	// Set the minimum size limits for X11 window
	// so the window manager doesn't allow resizing below those limits.
	XSizeHints* hints = XAllocSizeHints();
	hints->flags |= PMinSize;
	hints->min_width = mMinWindowWidth;
	hints->min_height = mMinWindowHeight;

	XSetWMNormalHints(mSDL_Display, mSDL_XWindowID, hints);

	XFree(hints);
#endif
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

	// do the actual forced cursor move.
	SDL_WarpMouse(screen_pos.mX, screen_pos.mY);
	
	//llinfos << llformat("llcw %d,%d -> scr %d,%d", position.mX, position.mY, screen_pos.mX, screen_pos.mY) << llendl;

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
		if (getSize(&screen_size))
		{
			pixel_aspect = getNativeAspectRatio() * (F32)screen_size.mY / (F32)screen_size.mX;
		}
	}

	return pixel_aspect;
}


// This is to support 'temporarily windowed' mode so that
// dialogs are still usable in fullscreen.
void LLWindowSDL::beforeDialog()
{
	bool running_x11 = false;
#if LL_X11
	running_x11 = (mSDL_XWindowID != None);
#endif //LL_X11

	llinfos << "LLWindowSDL::beforeDialog()" << llendl;

	if (SDLReallyCaptureInput(FALSE)) // must ungrab input so popup works!
	{
		if (mFullscreen)
		{
			// need to temporarily go non-fullscreen; bless SDL
			// for providing a SDL_WM_ToggleFullScreen() - though
			// it only works in X11
			if (running_x11 && mWindow)
			{
				SDL_WM_ToggleFullScreen(mWindow);
			}
		}
	}

#if LL_X11
	if (mSDL_Display)
	{
		// Everything that we/SDL asked for should happen before we
		// potentially hand control over to GTK.
		maybe_lock_display();
		XSync(mSDL_Display, False);
		maybe_unlock_display();
	}
#endif // LL_X11

#if LL_GTK
	// this is a good time to grab some GTK version information for
	// diagnostics, if not already done.
	ll_try_gtk_init();
#endif // LL_GTK

	maybe_lock_display();
}

void LLWindowSDL::afterDialog()
{
	bool running_x11 = false;
#if LL_X11
	running_x11 = (mSDL_XWindowID != None);
#endif //LL_X11

	llinfos << "LLWindowSDL::afterDialog()" << llendl;

	maybe_unlock_display();

	if (mFullscreen)
	{
		// need to restore fullscreen mode after dialog - only works
		// in X11
		if (running_x11 && mWindow)
		{
			SDL_WM_ToggleFullScreen(mWindow);
		}
	}
}


#if LL_X11
// set/reset the XWMHints flag for 'urgency' that usually makes the icon flash
void LLWindowSDL::x11_set_urgent(BOOL urgent)
{
	if (mSDL_Display && !mFullscreen)
	{
		XWMHints *wm_hints;
		
		llinfos << "X11 hint for urgency, " << urgent << llendl;

		maybe_lock_display();
		wm_hints = XGetWMHints(mSDL_Display, mSDL_XWindowID);
		if (!wm_hints)
			wm_hints = XAllocWMHints();

		if (urgent)
			wm_hints->flags |= XUrgencyHint;
		else
			wm_hints->flags &= ~XUrgencyHint;

		XSetWMHints(mSDL_Display, mSDL_XWindowID, wm_hints);
		XFree(wm_hints);
		XSync(mSDL_Display, False);
		maybe_unlock_display();
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


#if LL_GTK
BOOL LLWindowSDL::isClipboardTextAvailable()
{
	if (ll_try_gtk_init())
	{
		GtkClipboard * const clipboard =
			gtk_clipboard_get(GDK_NONE);
		return gtk_clipboard_wait_is_text_available(clipboard) ?
			TRUE : FALSE;
	}
	return FALSE; // failure
}

BOOL LLWindowSDL::pasteTextFromClipboard(LLWString &text)
{
	if (ll_try_gtk_init())
	{
		GtkClipboard * const clipboard =
			gtk_clipboard_get(GDK_NONE);
		gchar * const data = gtk_clipboard_wait_for_text(clipboard);
		if (data)
		{
			text = LLWString(utf8str_to_wstring(data));
			g_free(data);
			return TRUE;
		}
	}
	return FALSE; // failure
}

BOOL LLWindowSDL::copyTextToClipboard(const LLWString &text)
{
	if (ll_try_gtk_init())
	{
		const std::string utf8 = wstring_to_utf8str(text);
		GtkClipboard * const clipboard =
			gtk_clipboard_get(GDK_NONE);
		gtk_clipboard_set_text(clipboard, utf8.c_str(), utf8.length());
		return TRUE;
	}
	return FALSE; // failure
}


BOOL LLWindowSDL::isPrimaryTextAvailable()
{
	if (ll_try_gtk_init())
	{
		GtkClipboard * const clipboard =
			gtk_clipboard_get(GDK_SELECTION_PRIMARY);
		return gtk_clipboard_wait_is_text_available(clipboard) ?
			TRUE : FALSE;
	}
	return FALSE; // failure
}

BOOL LLWindowSDL::pasteTextFromPrimary(LLWString &text)
{
	if (ll_try_gtk_init())
	{
		GtkClipboard * const clipboard =
			gtk_clipboard_get(GDK_SELECTION_PRIMARY);
		gchar * const data = gtk_clipboard_wait_for_text(clipboard);
		if (data)
		{
			text = LLWString(utf8str_to_wstring(data));
			g_free(data);
			return TRUE;
		}
	}
	return FALSE; // failure
}

BOOL LLWindowSDL::copyTextToPrimary(const LLWString &text)
{
	if (ll_try_gtk_init())
	{
		const std::string utf8 = wstring_to_utf8str(text);
		GtkClipboard * const clipboard =
			gtk_clipboard_get(GDK_SELECTION_PRIMARY);
		gtk_clipboard_set_text(clipboard, utf8.c_str(), utf8.length());
		return TRUE;
	}
	return FALSE; // failure
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

BOOL LLWindowSDL::isPrimaryTextAvailable()
{
	return FALSE; // unsupported
}

BOOL LLWindowSDL::pasteTextFromPrimary(LLWString &dst)
{
	return FALSE; // unsupported
}

BOOL LLWindowSDL::copyTextToPrimary(const LLWString &s)
{
	return FALSE;  // unsupported
}

#endif // LL_GTK

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
            while (*modes && count<MAX_NUM_RESOLUTIONS)  // they're sorted biggest to smallest, so find end...
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




void LLWindowSDL::setupFailure(const std::string& text, const std::string& caption, U32 type)
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
		if (mSDL_Display)
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
				maybe_lock_display();
				result = XGrabPointer(mSDL_Display, mSDL_XWindowID,
						      True, 0, GrabModeAsync,
						      GrabModeAsync,
						      None, None, CurrentTime);
				maybe_unlock_display();
				if (GrabSuccess == result)
					newmode = SDL_GRAB_ON;
				else
					newmode = SDL_GRAB_OFF;
			} else if (wantmode == SDL_GRAB_OFF)
			{
				//llinfos << "X11 POINTER UNGRABBY" << llendl;
				newmode = SDL_GRAB_OFF;
				//newmode = SDL_WM_GrabInput(SDL_GRAB_OFF);
				
				maybe_lock_display();
				XUngrabPointer(mSDL_Display, CurrentTime);
				// Make sure the ungrab happens RIGHT NOW.
				XSync(mSDL_Display, False);
				maybe_unlock_display();
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
	   long as ALT is held down, so the window manager can't easily
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
	case SDLK_RALT:
		mask = 1U << 1; break;
	case SDLK_LCTRL:
		mask = 1U << 2; break;
	case SDLK_RCTRL:
		mask = 1U << 3; break;
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


void check_vm_bloat()
{
#if LL_LINUX
	// watch our own VM and RSS sizes, warn if we bloated rapidly
	FILE *fp = fopen("/proc/self/stat", "r");
	if (fp)
	{
		static long long last_vm_size = 0;
		static long long last_rss_size = 0;
		const long long significant_vm_difference = 250 * 1024*1024;
		const long long significant_rss_difference = 50 * 1024*1024;

		size_t dummy;
		char *ptr;
		size_t delim_result = 0;
		for (int i=0; i<22 && delim_result > -1; ++i) // parse past the values we don't want
		{
			ptr = NULL;
			delim_result = getdelim(&ptr, &dummy, ' ', fp);
			free(ptr);
		}
		// 23rd space-delimited entry is vsize
		ptr = NULL;
		delim_result = getdelim(&ptr, &dummy, ' ', fp);
		llassert(ptr);
		long long this_vm_size = atoll(ptr);
		free(ptr);
		// 24th space-delimited entry is RSS
		ptr = NULL;
		delim_result = getdelim(&ptr, &dummy, ' ', fp);
		llassert(ptr);
		long long this_rss_size = getpagesize() * atoll(ptr);
		free(ptr);

		llinfos << "VM SIZE IS NOW " << (this_vm_size/(1024*1024)) << " MB, RSS SIZE IS NOW " << (this_rss_size/(1024*1024)) << " MB" << llendl;

		if (llabs(last_vm_size - this_vm_size) >
		    significant_vm_difference)
		{
			if (this_vm_size > last_vm_size)
			{
				llwarns << "VM size grew by " << (this_vm_size - last_vm_size)/(1024*1024) << " MB in last frame" << llendl;
			}
			else
			{
				llinfos << "VM size shrank by " << (last_vm_size - this_vm_size)/(1024*1024) << " MB in last frame" << llendl;
			}
		}

		if (llabs(last_rss_size - this_rss_size) >
		    significant_rss_difference)
		{
			if (this_rss_size > last_rss_size)
			{
				llwarns << "RSS size grew by " << (this_rss_size - last_rss_size)/(1024*1024) << " MB in last frame" << llendl;
			}
			else
			{
				llinfos << "RSS size shrank by " << (last_rss_size - this_rss_size)/(1024*1024) << " MB in last frame" << llendl;
			}
		}

		last_rss_size = this_rss_size;
		last_vm_size = this_vm_size;

		fclose(fp);
	}
#endif // LL_LINUX
}


// virtual
void LLWindowSDL::processMiscNativeEvents()
{
#if LL_GTK
	// Pump GTK events to avoid starvation for:
	// * DBUS servicing
	// * Anything else which quietly hooks into the default glib/GTK loop
    if (ll_try_gtk_init())
    {
	    // Yuck, Mozilla's GTK callbacks play with the locale - push/pop
	    // the locale to protect it, as exotic/non-C locales
	    // causes our code lots of general critical weirdness
	    // and crashness. (SL-35450)
	    static std::string saved_locale;
	    saved_locale = ll_safe_string(setlocale(LC_ALL, NULL));

	    // Pump until we've nothing left to do or passed 1/15th of a
	    // second pumping for this frame.
	    static LLTimer pump_timer;
	    pump_timer.reset();
	    pump_timer.setTimerExpirySec(1.0f / 15.0f);
	    do {
		     // Always do at least one non-blocking pump
		    gtk_main_iteration_do(FALSE);
	    } while (gtk_events_pending() &&
		     !pump_timer.hasExpired());

	    setlocale(LC_ALL, saved_locale.c_str() );
    }
#endif // LL_GTK

    // hack - doesn't belong here - but this is just for debugging
    if (getenv("LL_DEBUG_BLOAT"))
    {
	    check_vm_bloat();
    }
}

void LLWindowSDL::gatherInput()
{
    const Uint32 CLICK_THRESHOLD = 300;  // milliseconds
    static int leftClick = 0;
    static int rightClick = 0;
    static Uint32 lastLeftDown = 0;
    static Uint32 lastRightDown = 0;
    SDL_Event event;

    // Handle all outstanding SDL events
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
		    mKeyScanCode = event.key.keysym.scancode;
		    mKeyVirtualKey = event.key.keysym.unicode;
		    mKeyModifiers = event.key.keysym.mod;

		    gKeyboard->handleKeyDown(event.key.keysym.sym, event.key.keysym.mod);
		    // part of the fix for SL-13243
		    if (SDLCheckGrabbyKeys(event.key.keysym.sym, TRUE) != 0)
			    SDLReallyCaptureInput(TRUE);

		    if (event.key.keysym.unicode)
		    {
			    handleUnicodeUTF16(event.key.keysym.unicode,
					       gKeyboard->currentMask(FALSE));
		    }
                break;

            case SDL_KEYUP:
		    mKeyScanCode = event.key.keysym.scancode;
		    mKeyVirtualKey = event.key.keysym.unicode;
		    mKeyModifiers = event.key.keysym.mod;

		    if (SDLCheckGrabbyKeys(event.key.keysym.sym, FALSE) == 0)
			    SDLReallyCaptureInput(FALSE); // part of the fix for SL-13243

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

                else if (event.button.button == SDL_BUTTON_RIGHT)  // right
                {
			mCallbacks->handleRightMouseDown(this, openGlCoord, mask);
                }

                else if (event.button.button == SDL_BUTTON_MIDDLE)  // middle
				{
				    mCallbacks->handleMiddleMouseDown(this, openGlCoord, mask);
				}
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
                else if (event.button.button == SDL_BUTTON_RIGHT)  // right
			mCallbacks->handleRightMouseUp(this, openGlCoord, mask);
                else if (event.button.button == SDL_BUTTON_MIDDLE)  // middle
			mCallbacks->handleMiddleMouseUp(this, openGlCoord, mask);
                // don't handle mousewheel here...

                break;
            }

            case SDL_VIDEOEXPOSE:  // VIDEOEXPOSE doesn't specify the damage, but hey, it's OpenGL...repaint the whole thing!
			    mCallbacks->handlePaint(this, 0, 0, mWindow->w, mWindow->h);
                break;

            case SDL_VIDEORESIZE:  // *FIX: handle this?
            {
		llinfos << "Handling a resize event: " << event.resize.w <<
			"x" << event.resize.h << llendl;

		S32 width = llmax(event.resize.w, (S32)mMinWindowWidth);
		S32 height = llmax(event.resize.h, (S32)mMinWindowHeight);

		// *FIX: I'm not sure this is necessary!
		mWindow = SDL_SetVideoMode(width, height, 32, mSDLFlags);
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

		mCallbacks->handleResize(this, width, height);
                break;
            }
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
				mHaveInputFocus = !!event.active.gain;

				if (mHaveInputFocus)
					mCallbacks->handleFocus(this);
				else
					mCallbacks->handleFocusLost(this);
			}
                }
                if (event.active.state & SDL_APPACTIVE)
                {
			// Change in iconification/minimization state.
			if ((!event.active.gain) != mIsMinimized)
			{
				mIsMinimized = (!event.active.gain);

				mCallbacks->handleActivate(this, !mIsMinimized);
				llinfos << "SDL deiconification state switched to " << BOOL(event.active.gain) << llendl;
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
	
	updateCursor();

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
		lldebugs << "Loaded cursor file " << filename << " "
			 << bmpsurface->w << "x" << bmpsurface->h << llendl;
		cursurface = SDL_CreateRGBSurface (SDL_SWSURFACE,
						   bmpsurface->w,
						   bmpsurface->h,
						   32,
						   SDL_SwapLE32(0xFFU),
						   SDL_SwapLE32(0xFF00U),
						   SDL_SwapLE32(0xFF0000U),
						   SDL_SwapLE32(0xFF000000U));
		SDL_FillRect(cursurface, NULL, SDL_SwapLE32(0x00000000U));

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
					U8 *pixelp =
						((U8*)cursurface->pixels)
						+ cursurface->pitch * i
						+ j*cursurface->format->BytesPerPixel;
					U8 srcred = pixelp[0];
					U8 srcgreen = pixelp[1];
					U8 srcblue = pixelp[2];
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

void LLWindowSDL::updateCursor()
{
	if (ATIbug) {
		// cursor-updating is very flaky when this bug is
		// present; do nothing.
		return;
	}

	if (mCurrentCursor != mNextCursor)
	{
		if (mNextCursor < UI_CURSOR_COUNT)
		{
			SDL_Cursor *sdlcursor = mSDLCursors[mNextCursor];
			// Try to default to the arrow for any cursors that
			// did not load correctly.
			if (!sdlcursor && mSDLCursors[UI_CURSOR_ARROW])
				sdlcursor = mSDLCursors[UI_CURSOR_ARROW];
			if (sdlcursor)
				SDL_SetCursor(sdlcursor);
		} else {
			llwarns << "Tried to set invalid cursor number " << mNextCursor << llendl;
		}
		mCurrentCursor = mNextCursor;
	}
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
	mSDLCursors[UI_CURSOR_TOOLPLAY] = makeSDLCursorFromBMP("toolplay.BMP",0,0);
	mSDLCursors[UI_CURSOR_TOOLPAUSE] = makeSDLCursorFromBMP("toolpause.BMP",0,0);
	mSDLCursors[UI_CURSOR_TOOLMEDIAOPEN] = makeSDLCursorFromBMP("toolmediaopen.BMP",0,0);
	mSDLCursors[UI_CURSOR_PIPETTE] = makeSDLCursorFromBMP("lltoolpipette.BMP",2,28);
	mSDLCursors[UI_CURSOR_TOOLSIT] = makeSDLCursorFromBMP("toolsit.BMP",20,15);
	mSDLCursors[UI_CURSOR_TOOLBUY] = makeSDLCursorFromBMP("toolbuy.BMP",20,15);
	mSDLCursors[UI_CURSOR_TOOLOPEN] = makeSDLCursorFromBMP("toolopen.BMP",20,15);
	mSDLCursors[UI_CURSOR_TOOLPATHFINDING] = makeSDLCursorFromBMP("lltoolpathfinding.BMP", 16, 16);
	mSDLCursors[UI_CURSOR_TOOLPATHFINDING_PATH_START] = makeSDLCursorFromBMP("lltoolpathfindingpathstart.BMP", 16, 16);
	mSDLCursors[UI_CURSOR_TOOLPATHFINDING_PATH_START_ADD] = makeSDLCursorFromBMP("lltoolpathfindingpathstartadd.BMP", 16, 16);
	mSDLCursors[UI_CURSOR_TOOLPATHFINDING_PATH_END] = makeSDLCursorFromBMP("lltoolpathfindingpathend.BMP", 16, 16);
	mSDLCursors[UI_CURSOR_TOOLPATHFINDING_PATH_END_ADD] = makeSDLCursorFromBMP("lltoolpathfindingpathendadd.BMP", 16, 16);
	mSDLCursors[UI_CURSOR_TOOLNO] = makeSDLCursorFromBMP("llno.BMP",8,8);

	if (getenv("LL_ATI_MOUSE_CURSOR_BUG") != NULL) {
		llinfos << "Disabling cursor updating due to LL_ATI_MOUSE_CURSOR_BUG" << llendl;
		ATIbug = true;
	}
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

	lldebugs << "LLWindowSDL::captureMouse" << llendl;
}

void LLWindowSDL::releaseMouse()
{
	// see LWindowSDL::captureMouse()
	
	lldebugs << "LLWindowSDL::releaseMouse" << llendl;
}

void LLWindowSDL::hideCursor()
{
	if(!mCursorHidden)
	{
		// llinfos << "hideCursor: hiding" << llendl;
		mCursorHidden = TRUE;
		mHideCursorPermanent = TRUE;
		SDL_ShowCursor(0);
	}
	else
	{
		// llinfos << "hideCursor: already hidden" << llendl;
	}
}

void LLWindowSDL::showCursor()
{
	if(mCursorHidden)
	{
		// llinfos << "showCursor: showing" << llendl;
		mCursorHidden = FALSE;
		mHideCursorPermanent = FALSE;
		SDL_ShowCursor(1);
	}
	else
	{
		// llinfos << "showCursor: already visible" << llendl;
	}
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
// LLSplashScreenSDL - I don't think we'll bother to implement this; it's
// fairly obsolete at this point.
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

void LLSplashScreenSDL::updateImpl(const std::string& mesg)
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

S32 OSMessageBoxSDL(const std::string& text, const std::string& caption, U32 type)
{
	S32 rtn = OSBTN_CANCEL;

	if(gWindowImplementation != NULL)
		gWindowImplementation->beforeDialog();

	if (LLWindowSDL::ll_try_gtk_init())
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
		win = gtk_message_dialog_new(NULL, flags, messagetype, buttons, "%s",
									 text.c_str());

# if LL_X11
		// Make GTK tell the window manager to associate this
		// dialog with our non-GTK SDL window, which should try
		// to keep it on top etc.
		if (gWindowImplementation &&
		    gWindowImplementation->mSDL_XWindowID != None)
		{
			gtk_widget_realize(GTK_WIDGET(win)); // so we can get its gdkwin
			GdkWindow *gdkwin = gdk_window_foreign_new(gWindowImplementation->mSDL_XWindowID);
			gdk_window_set_transient_for(GTK_WIDGET(win)->window,
						     gdkwin);
		}
# endif //LL_X11

		gtk_window_set_position(GTK_WINDOW(win),
					GTK_WIN_POS_CENTER_ON_PARENT);

		gtk_window_set_type_hint(GTK_WINDOW(win),
					 GDK_WINDOW_TYPE_HINT_DIALOG);

		if (!caption.empty())
			gtk_window_set_title(GTK_WINDOW(win), caption.c_str());

		gint response = GTK_RESPONSE_NONE;
		g_signal_connect (win,
				  "response", 
				  G_CALLBACK (response_callback),
				  &response);

		// we should be able to use a gtk_dialog_run(), but it's
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
		llinfos << "MSGBOX: " << caption << ": " << text << llendl;
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


/*
        Make the raw keyboard data available - used to poke through to LLQtWebKit so
        that Qt/Webkit has access to the virtual keycodes etc. that it needs
*/
LLSD LLWindowSDL::getNativeKeyData()
{
        LLSD result = LLSD::emptyMap();

	U32 modifiers = 0; // pretend-native modifiers... oh what a tangled web we weave!

	// we go through so many levels of device abstraction that I can't really guess
	// what a plugin under GDK under Qt under SL under SDL under X11 considers
	// a 'native' modifier mask.  this has been sort of reverse-engineered... they *appear*
	// to match GDK consts, but that may be co-incidence.
	modifiers |= (mKeyModifiers & KMOD_LSHIFT) ? 0x0001 : 0;
	modifiers |= (mKeyModifiers & KMOD_RSHIFT) ? 0x0001 : 0;// munge these into the same shift
	modifiers |= (mKeyModifiers & KMOD_CAPS)   ? 0x0002 : 0;
	modifiers |= (mKeyModifiers & KMOD_LCTRL)  ? 0x0004 : 0;
	modifiers |= (mKeyModifiers & KMOD_RCTRL)  ? 0x0004 : 0;// munge these into the same ctrl
	modifiers |= (mKeyModifiers & KMOD_LALT)   ? 0x0008 : 0;// untested
	modifiers |= (mKeyModifiers & KMOD_RALT)   ? 0x0008 : 0;// untested
	// *todo: test ALTs - I don't have a case for testing these.  Do you?
	// *todo: NUM? - I don't care enough right now (and it's not a GDK modifier).

        result["scan_code"] = (S32)mKeyScanCode;
        result["virtual_key"] = (S32)mKeyVirtualKey;
	result["modifiers"] = (S32)modifiers;

        return result;
}


BOOL LLWindowSDL::dialogColorPicker( F32 *r, F32 *g, F32 *b)
{
	BOOL rtn = FALSE;

	beforeDialog();

	if (ll_try_gtk_init())
	{
		GtkWidget *win = NULL;

		win = gtk_color_selection_dialog_new(NULL);

# if LL_X11
		// Get GTK to tell the window manager to associate this
		// dialog with our non-GTK SDL window, which should try
		// to keep it on top etc.
		if (mSDL_XWindowID != None)
		{
			gtk_widget_realize(GTK_WIDGET(win)); // so we can get its gdkwin
			GdkWindow *gdkwin = gdk_window_foreign_new(mSDL_XWindowID);
			gdk_window_set_transient_for(GTK_WIDGET(win)->window,
						     gdkwin);
		}
# endif //LL_X11

		GtkColorSelection *colorsel = GTK_COLOR_SELECTION (GTK_COLOR_SELECTION_DIALOG(win)->colorsel);

		GdkColor color, orig_color;
		orig_color.pixel = 0;
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
S32 OSMessageBoxSDL(const std::string& text, const std::string& caption, U32 type)
{
	llinfos << "MSGBOX: " << caption << ": " << text << llendl;
	return 0;
}

BOOL LLWindowSDL::dialogColorPicker( F32 *r, F32 *g, F32 *b)
{
	return (FALSE);
}
#endif // LL_GTK

#if LL_LINUX || LL_SOLARIS
// extracted from spawnWebBrowser for clarity and to eliminate
//  compiler confusion regarding close(int fd) vs. LLWindow::close()
void exec_cmd(const std::string& cmd, const std::string& arg)
{
	char* const argv[] = {(char*)cmd.c_str(), (char*)arg.c_str(), NULL};
	fflush(NULL);
	pid_t pid = fork();
	if (pid == 0)
	{ // child
		// disconnect from stdin/stdout/stderr, or child will
		// keep our output pipe undesirably alive if it outlives us.
		close(0);
		close(1);
		close(2);
		// end ourself by running the command
		execv(cmd.c_str(), argv);	/* Flawfinder: ignore */
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
}
#endif

// Open a URL with the user's default web browser.
// Must begin with protocol identifier.
void LLWindowSDL::spawnWebBrowser(const std::string& escaped_url, bool async)
{
	llinfos << "spawn_web_browser: " << escaped_url << llendl;
	
#if LL_LINUX || LL_SOLARIS
# if LL_X11
	if (mSDL_Display)
	{
		maybe_lock_display();
		// Just in case - before forking.
		XSync(mSDL_Display, False);
		maybe_unlock_display();
	}
# endif // LL_X11

	std::string cmd, arg;
	cmd  = gDirUtilp->getAppRODataDir();
	cmd += gDirUtilp->getDirDelimiter();
	cmd += "etc";
	cmd += gDirUtilp->getDirDelimiter();
	cmd += "launch_url.sh";
	arg = escaped_url;
	exec_cmd(cmd, arg);
#endif // LL_LINUX || LL_SOLARIS

	llinfos << "spawn_web_browser returning." << llendl;
}


void *LLWindowSDL::getPlatformWindow()
{
#if LL_GTK && LL_LLMOZLIB_ENABLED
	if (LLWindowSDL::ll_try_gtk_init())
	{
		maybe_lock_display();

		GtkWidget *owin = gtk_window_new(GTK_WINDOW_POPUP);
		// Why a layout widget?  A MozContainer would be ideal, but
		// it involves exposing Mozilla headers to mozlib-using apps.
		// A layout widget with a GtkWindow parent has the desired
		// properties of being plain GTK, having a window, and being
		// derived from a GtkContainer.
		GtkWidget *rtnw = gtk_layout_new(NULL, NULL);
		gtk_container_add(GTK_CONTAINER(owin), rtnw);
		gtk_widget_realize(rtnw);
		GTK_WIDGET_UNSET_FLAGS(GTK_WIDGET(rtnw), GTK_NO_WINDOW);
		
		maybe_unlock_display();
		
		return rtnw;
	}
#endif // LL_GTK && LL_LLMOZLIB_ENABLED
	// Unixoid mozilla really needs GTK.
	return NULL;
}

void LLWindowSDL::bringToFront()
{
	// This is currently used when we are 'launched' to a specific
	// map position externally.
	llinfos << "bringToFront" << llendl;
#if LL_X11
	if (mSDL_Display && !mFullscreen)
	{
		maybe_lock_display();
		XRaiseWindow(mSDL_Display, mSDL_XWindowID);
		XSync(mSDL_Display, False);
		maybe_unlock_display();
	}
#endif // LL_X11
}

//static
std::vector<std::string> LLWindowSDL::getDynamicFallbackFontList()
{
	// Use libfontconfig to find us a nice ordered list of fallback fonts
	// specific to this system.
	std::string final_fallback("/usr/share/fonts/truetype/kochi/kochi-gothic.ttf");
	const int max_font_count_cutoff = 40; // fonts are expensive in the current system, don't enumerate an arbitrary number of them
	// Our 'ideal' font properties which define the sorting results.
	// slant=0 means Roman, index=0 means the first face in a font file
	// (the one we actually use), weight=80 means medium weight,
	// spacing=0 means proportional spacing.
	std::string sort_order("slant=0:index=0:weight=80:spacing=0");
	// elide_unicode_coverage removes fonts from the list whose unicode
	// range is covered by fonts earlier in the list.  This usually
	// removes ~90% of the fonts as redundant (which is great because
	// the font list can be huge), but might unnecessarily reduce the
	// renderable range if for some reason our FreeType actually fails
	// to use some of the fonts we want it to.
	const bool elide_unicode_coverage = true;
	std::vector<std::string> rtns;
	FcFontSet *fs = NULL;
	FcPattern *sortpat = NULL;

	llinfos << "Getting system font list from FontConfig..." << llendl;

	// If the user has a system-wide language preference, then favor
	// fonts from that language group.  This doesn't affect the types
	// of languages that can be displayed, but ensures that their
	// preferred language is rendered from a single consistent font where
	// possible.
	FL_Locale *locale = NULL;
	FL_Success success = FL_FindLocale(&locale, FL_MESSAGES);
	if (success != 0)
	{
		if (success >= 2 && locale->lang) // confident!
		{
			LL_INFOS("AppInit") << "Language " << locale->lang << LL_ENDL;
			LL_INFOS("AppInit") << "Location " << locale->country << LL_ENDL;
			LL_INFOS("AppInit") << "Variant " << locale->variant << LL_ENDL;

			llinfos << "Preferring fonts of language: "
				<< locale->lang
				<< llendl;
			sort_order = "lang=" + std::string(locale->lang) + ":"
				+ sort_order;
		}
	}
	FL_FreeLocale(&locale);

	if (!FcInit())
	{
		llwarns << "FontConfig failed to initialize." << llendl;
		rtns.push_back(final_fallback);
		return rtns;
	}

	sortpat = FcNameParse((FcChar8*) sort_order.c_str());
	if (sortpat)
	{
		// Sort the list of system fonts from most-to-least-desirable.
		FcResult result;
		fs = FcFontSort(NULL, sortpat, elide_unicode_coverage,
				NULL, &result);
		FcPatternDestroy(sortpat);
	}

	int found_font_count = 0;
	if (fs)
	{
		// Get the full pathnames to the fonts, where available,
		// which is what we really want.
		found_font_count = fs->nfont;
		for (int i=0; i<fs->nfont; ++i)
		{
			FcChar8 *filename;
			if (FcResultMatch == FcPatternGetString(fs->fonts[i],
								FC_FILE, 0,
								&filename)
			    && filename)
			{
				rtns.push_back(std::string((const char*)filename));
				if (rtns.size() >= max_font_count_cutoff)
					break; // hit limit
			}
		}
		FcFontSetDestroy (fs);
	}

	lldebugs << "Using font list: " << llendl;
	for (std::vector<std::string>::iterator it = rtns.begin();
		 it != rtns.end();
		 ++it)
	{
		lldebugs << "  file: " << *it << llendl;
	}
	llinfos << "Using " << rtns.size() << "/" << found_font_count << " system fonts." << llendl;

	rtns.push_back(final_fallback);
	return rtns;
}

#endif // LL_SDL
