/** 
 * @file llwindow.cpp
 * @brief Basic graphical window class
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
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
#include "linked_lists.h"
#include "llwindowcallbacks.h"


//
// Globals
//
LLSplashScreen *gSplashScreenp = NULL;
BOOL gDebugClicks = FALSE;
BOOL gDebugWindowProc = FALSE;

const S32 gURLProtocolWhitelistCount = 3;
const std::string gURLProtocolWhitelist[] = { "file:", "http:", "https:" };

// CP: added a handler list - this is what's used to open the protocol and is based on registry entry
//	   only meaningful difference currently is that file: protocols are opened using http:
//	   since no protocol handler exists in registry for file:
//     Important - these lists should match - protocol to handler
const std::string gURLProtocolWhitelistHandler[] = { "http", "http", "https" };	


S32 OSMessageBox(const std::string& text, const std::string& caption, U32 type)
{
	// Properly hide the splash screen when displaying the message box
	BOOL was_visible = FALSE;
	if (LLSplashScreen::isVisible())
	{
		was_visible = TRUE;
		LLSplashScreen::hide();
	}

	S32 result = 0;
#if LL_MESA_HEADLESS // !!! *FIX: (???)
	llwarns << "OSMessageBox: " << text << llendl;
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

LLWindow::LLWindow(LLWindowCallbacks* callbacks, BOOL fullscreen, U32 flags)
	: mCallbacks(callbacks),
	  mPostQuit(TRUE),
	  mFullscreen(fullscreen),
	  mFullscreenWidth(0),
	  mFullscreenHeight(0),
	  mFullscreenBits(0),
	  mFullscreenRefresh(0),
	  mSupportedResolutions(NULL),
	  mNumSupportedResolutions(0),
	  mCurrentCursor(UI_CURSOR_ARROW),
	  mCursorHidden(FALSE),
	  mBusyCount(0),
	  mIsMouseClipping(FALSE),
	  mSwapMethod(SWAP_METHOD_UNDEFINED),
	  mHideCursorPermanent(FALSE),
	  mFlags(flags),
	  mHighSurrogate(0)
{ }

LLWindow::~LLWindow()
{ }

//virtual
BOOL LLWindow::isValid()
{
	return TRUE;
}

//virtual
BOOL LLWindow::canDelete()
{
	return TRUE;
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
BOOL LLWindow::dialogColorPicker(F32 *r, F32 *g, F32 *b)
{
	return FALSE;
}

void *LLWindow::getMediaWindow()
{
	// Default to returning the platform window.
	return getPlatformWindow();
}

//virtual
void LLWindow::processMiscNativeEvents()
{
	// do nothing unless subclassed
}

//virtual
BOOL LLWindow::isPrimaryTextAvailable()
{
	return FALSE; // no
}
//virtual
BOOL LLWindow::pasteTextFromPrimary(LLWString &dst)
{
	return FALSE; // fail
}
// virtual
BOOL LLWindow::copyTextToPrimary(const LLWString &src)
{
	return FALSE; // fail
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
#if LL_MESA_HEADLESS || LL_SDL  // !!! *FIX: (???)
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
	BOOL fullscreen, 
	BOOL clearBg,
	BOOL disable_vsync,
	BOOL use_gl,
	BOOL ignore_pixel_depth,
	U32 fsaa_samples)
{
	LLWindow* new_window;

	if (use_gl)
	{
#if LL_MESA_HEADLESS
		new_window = new LLWindowMesaHeadless(callbacks,
			title, name, x, y, width, height, flags, 
			fullscreen, clearBg, disable_vsync, use_gl, ignore_pixel_depth);
#elif LL_SDL
		new_window = new LLWindowSDL(callbacks,
			title, x, y, width, height, flags, 
			fullscreen, clearBg, disable_vsync, use_gl, ignore_pixel_depth, fsaa_samples);
#elif LL_WINDOWS
		new_window = new LLWindowWin32(callbacks,
			title, name, x, y, width, height, flags, 
			fullscreen, clearBg, disable_vsync, use_gl, ignore_pixel_depth, fsaa_samples);
#elif LL_DARWIN
		new_window = new LLWindowMacOSX(callbacks,
			title, name, x, y, width, height, flags, 
			fullscreen, clearBg, disable_vsync, use_gl, ignore_pixel_depth, fsaa_samples);
#endif
	}
	else
	{
		new_window = new LLWindowHeadless(callbacks,
			title, name, x, y, width, height, flags, 
			fullscreen, clearBg, disable_vsync, use_gl, ignore_pixel_depth);
	}

	if (FALSE == new_window->isValid())
	{
		delete new_window;
		llwarns << "LLWindowManager::create() : Error creating window." << llendl;
		return NULL;
	}
	sWindowList.insert(new_window);
	return new_window;
}

BOOL LLWindowManager::destroyWindow(LLWindow* window)
{
	if (sWindowList.find(window) == sWindowList.end())
	{
		llerrs << "LLWindowManager::destroyWindow() : Window pointer not valid, this window doesn't exist!" 
			<< llendl;
		return FALSE;
	}

	window->close();

	sWindowList.erase(window);

	delete window;

	return TRUE;
}

BOOL LLWindowManager::isWindowValid(LLWindow *window)
{
	return sWindowList.find(window) != sWindowList.end();
}
