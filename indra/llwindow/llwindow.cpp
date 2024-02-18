/** 
 * @file llwindow.cpp
 * @brief Basic graphical window class
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
#include "llwindowheadless.h"

#if LL_MESA_HEADLESS
#include "llwindowmesaheadless.h"
#elif LL_SDL
#include "llwindowsdl.h"
#elif LL_WINDOWS
#include "llwindowwin32.h"
#elif LL_DARWIN
#include "llwindowmacosx.h"
#endif

#include "llerror.h"
#include "llkeyboard.h"
#include "llwindowcallbacks.h"


//
// Globals
//
LLSplashScreen *gSplashScreenp = NULL;
bool gDebugClicks = false;
bool gDebugWindowProc = false;

const S32 gURLProtocolWhitelistCount = 5;
const std::string gURLProtocolWhitelist[] = { "secondlife:", "http:", "https:", "data:", "mailto:" };

// CP: added a handler list - this is what's used to open the protocol and is based on registry entry
//	   only meaningful difference currently is that file: protocols are opened using http:
//	   since no protocol handler exists in registry for file:
//     Important - these lists should match - protocol to handler
// Maestro: This list isn't referenced anywhere that I could find
//const std::string gURLProtocolWhitelistHandler[] = { "http", "http", "https" };	


S32 OSMessageBox(const std::string& text, const std::string& caption, U32 type)
{
	// Properly hide the splash screen when displaying the message box
	bool was_visible = false;
	if (LLSplashScreen::isVisible())
	{
		was_visible = true;
		LLSplashScreen::hide();
	}

	S32 result = 0;
#if LL_MESA_HEADLESS // !!! *FIX: (?)
	LL_WARNS() << "OSMessageBox: " << text << LL_ENDL;
	return OSBTN_OK;
#elif LL_WINDOWS
	result = OSMessageBoxWin32(text, caption, type);
#elif LL_DARWIN
	result = OSMessageBoxMacOSX(text, caption, type);
#elif LL_SDL
	result = OSMessageBoxSDL(text, caption, type);
#else
#error("OSMessageBox not implemented for this platform!")
#endif

	if (was_visible)
	{
		LLSplashScreen::show();
	}

	return result;
}


//
// LLWindow
//

LLWindow::LLWindow(LLWindowCallbacks* callbacks, bool fullscreen, U32 flags)
	: mCallbacks(callbacks),
	  mPostQuit(true),
	  mFullscreen(fullscreen),
	  mFullscreenWidth(0),
	  mFullscreenHeight(0),
	  mFullscreenBits(0),
	  mFullscreenRefresh(0),
	  mSupportedResolutions(NULL),
	  mNumSupportedResolutions(0),
	  mCurrentCursor(UI_CURSOR_ARROW),
	  mNextCursor(UI_CURSOR_ARROW),
	  mCursorHidden(false),
	  mBusyCount(0),
	  mIsMouseClipping(false),
	  mMinWindowWidth(0),
	  mMinWindowHeight(0),
	  mSwapMethod(SWAP_METHOD_UNDEFINED),
	  mHideCursorPermanent(false),
	  mFlags(flags),
	  mHighSurrogate(0),
	  mRefreshRate(0)
{
}

LLWindow::~LLWindow()
{
}

//virtual
bool LLWindow::isValid()
{
	return true;
}

//virtual
bool LLWindow::canDelete()
{
	return true;
}

//virtual
void LLWindow::setTitle(const std::string title)
{
    // the action happens in the platform specific impl
}

// virtual
void LLWindow::incBusyCount()
{
	++mBusyCount;
}

// virtual
void LLWindow::decBusyCount()
{
	if (mBusyCount > 0)
	{
		--mBusyCount;
	}
}

//virtual
void LLWindow::resetBusyCount()
{
	mBusyCount = 0;
}

//virtual
S32 LLWindow::getBusyCount() const
{
	return mBusyCount;
}

//virtual
ECursorType LLWindow::getCursor() const
{
	return mCurrentCursor;
}

//virtual
bool LLWindow::dialogColorPicker(F32 *r, F32 *g, F32 *b)
{
	return false;
}

void *LLWindow::getMediaWindow()
{
	// Default to returning the platform window.
	return getPlatformWindow();
}

bool LLWindow::setSize(LLCoordScreen size)
{
	if (!getMaximized())
	{
		size.mX = llmax(size.mX, mMinWindowWidth);
		size.mY = llmax(size.mY, mMinWindowHeight);
	}
	return setSizeImpl(size);
}

bool LLWindow::setSize(LLCoordWindow size)
{
	//HACK: we are inconsistently using minimum window dimensions
	// in this case, we are constraining the inner "client" rect and other times
	// we constrain the outer "window" rect
	// There doesn't seem to be a good way to do this consistently without a bunch of platform
	// specific code
	if (!getMaximized())
	{
		size.mX = llmax(size.mX, mMinWindowWidth);
		size.mY = llmax(size.mY, mMinWindowHeight);
	}
	return setSizeImpl(size);
}


// virtual
void LLWindow::setMinSize(U32 min_width, U32 min_height, bool enforce_immediately)
{
	mMinWindowWidth = min_width;
	mMinWindowHeight = min_height;

	if (enforce_immediately)
	{
		LLCoordScreen cur_size;
		if (!getMaximized() && getSize(&cur_size))
		{
			if (cur_size.mX < mMinWindowWidth || cur_size.mY < mMinWindowHeight)
			{
				setSizeImpl(LLCoordScreen(llmin(cur_size.mX, mMinWindowWidth), llmin(cur_size.mY, mMinWindowHeight)));
			}
		}
	}
}

//virtual
void LLWindow::processMiscNativeEvents()
{
	// do nothing unless subclassed
}

//virtual
bool LLWindow::isPrimaryTextAvailable()
{
	return false; // no
}
//virtual
bool LLWindow::pasteTextFromPrimary(LLWString &dst)
{
	return false; // fail
}
// virtual
bool LLWindow::copyTextToPrimary(const LLWString &src)
{
	return false; // fail
}

// static
std::vector<std::string> LLWindow::getDynamicFallbackFontList()
{
#if LL_WINDOWS
	return LLWindowWin32::getDynamicFallbackFontList();
#elif LL_DARWIN
	return LLWindowMacOSX::getDynamicFallbackFontList();
#elif LL_SDL
	return LLWindowSDL::getDynamicFallbackFontList();
#else
	return std::vector<std::string>();
#endif
}

// static
std::vector<std::string> LLWindow::getDisplaysResolutionList()
{
#if LL_WINDOWS
	return LLWindowWin32::getDisplaysResolutionList();
#elif LL_DARWIN
	return LLWindowMacOSX::getDisplaysResolutionList();
#else
	return std::vector<std::string>();
#endif
}

#define UTF16_IS_HIGH_SURROGATE(U) ((U16)((U) - 0xD800) < 0x0400)
#define UTF16_IS_LOW_SURROGATE(U)  ((U16)((U) - 0xDC00) < 0x0400)
#define UTF16_SURROGATE_PAIR_TO_UTF32(H,L) (((H) << 10) + (L) - (0xD800 << 10) - 0xDC00 + 0x00010000)

void LLWindow::handleUnicodeUTF16(U16 utf16, MASK mask)
{
	// Note that we could discard unpaired surrogates, but I'm
	// following the Unicode Consortium's recommendation here;
	// that is, to preserve those unpaired surrogates in UTF-32
	// values.  _To_preserve_ means to pass to the callback in our
	// context.

	if (mHighSurrogate == 0)
	{
		if (UTF16_IS_HIGH_SURROGATE(utf16))
		{
			mHighSurrogate = utf16;
		}
		else
		{
			mCallbacks->handleUnicodeChar(utf16, mask);
		}
	}
	else
	{
		if (UTF16_IS_LOW_SURROGATE(utf16))
		{
			/* A legal surrogate pair.  */			
			mCallbacks->handleUnicodeChar(UTF16_SURROGATE_PAIR_TO_UTF32(mHighSurrogate, utf16), mask);
			mHighSurrogate = 0;
		}
		else if (UTF16_IS_HIGH_SURROGATE(utf16))
		{
			/* Two consecutive high surrogates.  */
			mCallbacks->handleUnicodeChar(mHighSurrogate, mask);
			mHighSurrogate = utf16;
		}
		else
		{
			/* A non-low-surrogate preceeded by a high surrogate. */
			mCallbacks->handleUnicodeChar(mHighSurrogate, mask);
			mHighSurrogate = 0;
			mCallbacks->handleUnicodeChar(utf16, mask);
		}
	}
}

//
// LLSplashScreen
//

// static
bool LLSplashScreen::isVisible()
{
	return gSplashScreenp ? true: false;
}

// static
LLSplashScreen *LLSplashScreen::create()
{
#if LL_MESA_HEADLESS || LL_SDL  // !!! *FIX: (?)
	return 0;
#elif LL_WINDOWS
	return new LLSplashScreenWin32;
#elif LL_DARWIN
	return new LLSplashScreenMacOSX;
#else
#error("LLSplashScreen not implemented on this platform!")
#endif
}


//static
void LLSplashScreen::show()
{
	if (!gSplashScreenp)
	{
#if LL_WINDOWS && !LL_MESA_HEADLESS
		gSplashScreenp = new LLSplashScreenWin32;
#elif LL_DARWIN
		gSplashScreenp = new LLSplashScreenMacOSX;
#endif
		if (gSplashScreenp)
		{
			gSplashScreenp->showImpl();
		}
	}
}

//static
void LLSplashScreen::update(const std::string& str)
{
	LLSplashScreen::show();
	if (gSplashScreenp)
	{
		gSplashScreenp->updateImpl(str);
	}
}

//static
void LLSplashScreen::hide()
{
	if (gSplashScreenp)
	{
		gSplashScreenp->hideImpl();
	}
	delete gSplashScreenp;
	gSplashScreenp = NULL;
}

//
// LLWindowManager
//

// TODO: replace with std::set
static std::set<LLWindow*> sWindowList;

LLWindow* LLWindowManager::createWindow(
	LLWindowCallbacks* callbacks,
	const std::string& title, const std::string& name, S32 x, S32 y, S32 width, S32 height, U32 flags,
	bool fullscreen, 
	bool clearBg,
	bool enable_vsync,
	bool use_gl,
	bool ignore_pixel_depth,
	U32 fsaa_samples,
    U32 max_cores,
    U32 max_vram,
    F32 max_gl_version)
{
	LLWindow* new_window;

	if (use_gl)
	{
#if LL_MESA_HEADLESS
		new_window = new LLWindowMesaHeadless(callbacks,
			title, name, x, y, width, height, flags, 
			fullscreen, clearBg, enable_vsync, use_gl, ignore_pixel_depth);
#elif LL_SDL
		new_window = new LLWindowSDL(callbacks,
			title, x, y, width, height, flags, 
			fullscreen, clearBg, enable_vsync, use_gl, ignore_pixel_depth, fsaa_samples);
#elif LL_WINDOWS
		new_window = new LLWindowWin32(callbacks,
			title, name, x, y, width, height, flags, 
			fullscreen, clearBg, enable_vsync, use_gl, ignore_pixel_depth, fsaa_samples, max_cores, max_vram, max_gl_version);
#elif LL_DARWIN
		new_window = new LLWindowMacOSX(callbacks,
			title, name, x, y, width, height, flags, 
			fullscreen, clearBg, enable_vsync, use_gl, ignore_pixel_depth, fsaa_samples, max_vram);
#endif
	}
	else
	{
		new_window = new LLWindowHeadless(callbacks,
			title, name, x, y, width, height, flags, 
			fullscreen, clearBg, enable_vsync, use_gl, ignore_pixel_depth);
	}

	if (false == new_window->isValid())
	{
		delete new_window;
		LL_WARNS() << "LLWindowManager::create() : Error creating window." << LL_ENDL;
		return NULL;
	}
	sWindowList.insert(new_window);
	return new_window;
}

bool LLWindowManager::destroyWindow(LLWindow* window)
{
	if (sWindowList.find(window) == sWindowList.end())
	{
		LL_ERRS() << "LLWindowManager::destroyWindow() : Window pointer not valid, this window doesn't exist!" 
			<< LL_ENDL;
		return false;
	}

	window->close();

	sWindowList.erase(window);

	delete window;

	return true;
}

bool LLWindowManager::isWindowValid(LLWindow *window)
{
	return sWindowList.find(window) != sWindowList.end();
}

//coordinate conversion utility funcs that forward to llwindow
LLCoordCommon LL_COORD_TYPE_WINDOW::convertToCommon() const
{
	const LLCoordWindow& self = LLCoordWindow::getTypedCoords(*this);

	LLCoordGL out;
	LLWindow::instance_snapshot().begin()->convertCoords(self, &out);
	return out.convert();
}

void LL_COORD_TYPE_WINDOW::convertFromCommon(const LLCoordCommon& from)
{
	LLCoordWindow& self = LLCoordWindow::getTypedCoords(*this);

	LLCoordGL from_gl(from);
	LLWindow::instance_snapshot().begin()->convertCoords(from_gl, &self);
}

LLCoordCommon LL_COORD_TYPE_SCREEN::convertToCommon() const
{
	const LLCoordScreen& self = LLCoordScreen::getTypedCoords(*this);

	LLCoordGL out;
	LLWindow::instance_snapshot().begin()->convertCoords(self, &out);
	return out.convert();
}

void LL_COORD_TYPE_SCREEN::convertFromCommon(const LLCoordCommon& from)
{
	LLCoordScreen& self = LLCoordScreen::getTypedCoords(*this);

	LLCoordGL from_gl(from);
	LLWindow::instance_snapshot().begin()->convertCoords(from_gl, &self);
}
