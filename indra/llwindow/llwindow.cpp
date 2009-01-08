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

//static instance for default callbacks
LLWindowCallbacks	LLWindow::sDefaultCallbacks;

//
// LLWindowCallbacks
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

BOOL LLWindowCallbacks::handleTranslatedKeyDown(const KEY key, const MASK mask, BOOL repeated)
{
	return FALSE;
}


BOOL LLWindowCallbacks::handleTranslatedKeyUp(const KEY key, const MASK mask)
{
	return FALSE;
}

void LLWindowCallbacks::handleScanKey(KEY key, BOOL key_down, BOOL key_up, BOOL key_level)
{
}

BOOL LLWindowCallbacks::handleUnicodeChar(llwchar uni_char, MASK mask)
{
	return FALSE;
}


BOOL LLWindowCallbacks::handleMouseDown(LLWindow *window, const LLCoordGL pos, MASK mask)
{
	return FALSE;
}

BOOL LLWindowCallbacks::handleMouseUp(LLWindow *window, const LLCoordGL pos, MASK mask)
{
	return FALSE;
}

void LLWindowCallbacks::handleMouseLeave(LLWindow *window)
{
	return;
}

BOOL LLWindowCallbacks::handleCloseRequest(LLWindow *window)
{
	//allow the window to close
	return TRUE;
}

void LLWindowCallbacks::handleQuit(LLWindow *window)
{
	if(LLWindowManager::destroyWindow(window) == FALSE)	
	{
		llerrs << "LLWindowCallbacks::handleQuit() : Couldn't destroy window" << llendl;
	}
}

BOOL LLWindowCallbacks::handleRightMouseDown(LLWindow *window, const LLCoordGL pos, MASK mask)
{
	return FALSE;
}

BOOL LLWindowCallbacks::handleRightMouseUp(LLWindow *window, const LLCoordGL pos, MASK mask)
{
	return FALSE;
}

BOOL LLWindowCallbacks::handleMiddleMouseDown(LLWindow *window, const LLCoordGL pos, MASK mask)
{
	return FALSE;
}

BOOL LLWindowCallbacks::handleMiddleMouseUp(LLWindow *window, const LLCoordGL pos, MASK mask)
{
	return FALSE;
}

BOOL LLWindowCallbacks::handleActivate(LLWindow *window, BOOL activated)
{
	return FALSE;
}

BOOL LLWindowCallbacks::handleActivateApp(LLWindow *window, BOOL activating)
{
	return FALSE;
}

void LLWindowCallbacks::handleMouseMove(LLWindow *window, const LLCoordGL pos, MASK mask)
{
}

void LLWindowCallbacks::handleScrollWheel(LLWindow *window, S32 clicks)
{
}

void LLWindowCallbacks::handleResize(LLWindow *window, const S32 width, const S32 height)
{
}

void LLWindowCallbacks::handleFocus(LLWindow *window)
{
}

void LLWindowCallbacks::handleFocusLost(LLWindow *window)
{
}

void LLWindowCallbacks::handleMenuSelect(LLWindow *window, const S32 menu_item)
{
}

BOOL LLWindowCallbacks::handlePaint(LLWindow *window, const S32 x, const S32 y, 
									const S32 width, const S32 height)
{
	return FALSE;
}

BOOL LLWindowCallbacks::handleDoubleClick(LLWindow *window, const LLCoordGL pos, MASK mask)
{
	return FALSE;
}

void LLWindowCallbacks::handleWindowBlock(LLWindow *window)
{
}

void LLWindowCallbacks::handleWindowUnblock(LLWindow *window)
{
}

void LLWindowCallbacks::handleDataCopy(LLWindow *window, S32 data_type, void *data)
{
}

BOOL LLWindowCallbacks::handleTimerEvent(LLWindow *window)
{
	return FALSE;
}

BOOL LLWindowCallbacks::handleDeviceChange(LLWindow *window)
{
	return FALSE;
}

void LLWindowCallbacks::handlePingWatchdog(LLWindow *window, const char * msg)
{

}

void LLWindowCallbacks::handlePauseWatchdog(LLWindow *window)
{

}

void LLWindowCallbacks::handleResumeWatchdog(LLWindow *window)
{

}


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

LLWindow::LLWindow(BOOL fullscreen, U32 flags)
	: mCallbacks(&sDefaultCallbacks),
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
{
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

void LLWindow::setCallbacks(LLWindowCallbacks *callbacks)
{
	mCallbacks = callbacks;
	if (gKeyboard)
	{
		gKeyboard->setCallbacks(callbacks);
	}
}

void *LLWindow::getMediaWindow()
{
	// Default to returning the platform window.
	return getPlatformWindow();
}

// static
std::string LLWindow::getFontListSans()
{
#if LL_WINDOWS
	return LLWindowWin32::getFontListSans();
#elif LL_DARWIN
	return LLWindowMacOSX::getFontListSans();
#elif LL_SDL
	return LLWindowSDL::getFontListSans();
#else
	return "";
#endif
}

//virtual
void LLWindow::processMiscNativeEvents()
{
	// do nothing unless subclassed
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
	const std::string& title,
	const std::string& name,
	LLCoordScreen upper_left,
	LLCoordScreen size,
	U32 flags,
	BOOL fullscreen, 
	BOOL clearBg,
	BOOL disable_vsync,
	BOOL use_gl,
	BOOL ignore_pixel_depth)
{
	return createWindow(
		title, name, upper_left.mX, upper_left.mY, size.mX, size.mY, flags, 
		fullscreen, clearBg, disable_vsync, use_gl, ignore_pixel_depth);
}

LLWindow* LLWindowManager::createWindow(
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
		new_window = new LLWindowMesaHeadless(
			title, name, x, y, width, height, flags, 
			fullscreen, clearBg, disable_vsync, use_gl, ignore_pixel_depth);
#elif LL_SDL
		new_window = new LLWindowSDL(
			title, x, y, width, height, flags, 
			fullscreen, clearBg, disable_vsync, use_gl, ignore_pixel_depth, fsaa_samples);
#elif LL_WINDOWS
		new_window = new LLWindowWin32(
			title, name, x, y, width, height, flags, 
			fullscreen, clearBg, disable_vsync, use_gl, ignore_pixel_depth, fsaa_samples);
#elif LL_DARWIN
		new_window = new LLWindowMacOSX(
			title, name, x, y, width, height, flags, 
			fullscreen, clearBg, disable_vsync, use_gl, ignore_pixel_depth, fsaa_samples);
#endif
	}
	else
	{
		new_window = new LLWindowHeadless(
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
