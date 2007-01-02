/** 
 * @file llwindow.cpp
 * @brief Basic graphical window class
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
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
#elif LL_LINUX
#include "llwindowlinux.h" // currently just a dummy wrapper
#endif

#include "llerror.h"
#include "llkeyboard.h"

//static instance for default callbacks
LLWindowCallbacks	LLWindow::sDefaultCallbacks;

//
// LLWindowCallbacks
//

LLSplashScreen *gSplashScreenp = NULL;
BOOL gDebugClicks = FALSE;
BOOL gDebugWindowProc = FALSE;

const S32 gURLProtocolWhitelistCount = 3;
const char* gURLProtocolWhitelist[] = { "file", "http", "https" };

// CP: added a handler list - this is what's used to open the protocol and is based on registry entry
//	   only meaningful difference currently is that file: protocols are opened using http:
//	   since no protocol handler exists in registry for file:
//     Important - these lists should match - protocol to handler
const char* gURLProtocolWhitelistHandler[] = { "http", "http", "https" };	

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

BOOL LLWindowCallbacks::handleActivate(LLWindow *window, BOOL activated)
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


S32 OSMessageBox(const char* text, const char* caption, U32 type)
{
	// Properly hide the splash screen when displaying the message box
	BOOL was_visible = FALSE;
	if (LLSplashScreen::isVisible())
	{
		was_visible = TRUE;
		LLSplashScreen::hide();
	}

	S32 result = 0;
#if LL_MESA_HEADLESS // !!! FIXME
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
	  mFlags(flags)
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
#if LL_MESA_HEADLESS || LL_SDL  // !!! FIXME
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
void LLSplashScreen::update(const char* str)
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

LLLinkedList<LLWindow> LLWindowManager::sWindowList;

LLWindow* LLWindowManager::createWindow(
	char *title,
	char *name,
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
	char *title, char *name, S32 x, S32 y, S32 width, S32 height, U32 flags,
	BOOL fullscreen, 
	BOOL clearBg,
	BOOL disable_vsync,
	BOOL use_gl,
	BOOL ignore_pixel_depth)
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
			fullscreen, clearBg, disable_vsync, use_gl, ignore_pixel_depth);
#elif LL_WINDOWS
		new_window = new LLWindowWin32(
			title, name, x, y, width, height, flags, 
			fullscreen, clearBg, disable_vsync, use_gl, ignore_pixel_depth);
#elif LL_DARWIN
		new_window = new LLWindowMacOSX(
			title, name, x, y, width, height, flags, 
			fullscreen, clearBg, disable_vsync, use_gl, ignore_pixel_depth);
#elif LL_LINUX
		new_window = new LLWindowLinux(
			title, name, x, y, width, height, flags, 
			fullscreen, clearBg, disable_vsync, use_gl, ignore_pixel_depth);
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
	sWindowList.addDataAtEnd(new_window);
	return new_window;
}

BOOL LLWindowManager::destroyWindow(LLWindow* window)
{
	if (!sWindowList.checkData(window))
	{
		llerrs << "LLWindowManager::destroyWindow() : Window pointer not valid, this window doesn't exist!" 
			<< llendl;
		return FALSE;
	}

	window->close();

	sWindowList.removeData(window);

	delete window;

	return TRUE;
}

BOOL LLWindowManager::isWindowValid(LLWindow *window)
{
	return sWindowList.checkData(window);
}
