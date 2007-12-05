/** 
 * @file llwindow.h
 * @brief Basic graphical window class
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

#ifndef LL_LLWINDOW_H
#define LL_LLWINDOW_H

#include <sys/stat.h>

#include "llrect.h"
#include "llcoord.h"
#include "llstring.h"


enum ECursorType {
	UI_CURSOR_ARROW,
	UI_CURSOR_WAIT,
	UI_CURSOR_HAND,
	UI_CURSOR_IBEAM,
	UI_CURSOR_CROSS,
	UI_CURSOR_SIZENWSE,
	UI_CURSOR_SIZENESW,
	UI_CURSOR_SIZEWE,
	UI_CURSOR_SIZENS,
	UI_CURSOR_NO,
	UI_CURSOR_WORKING,
	UI_CURSOR_TOOLGRAB,
	UI_CURSOR_TOOLLAND,
	UI_CURSOR_TOOLFOCUS,
	UI_CURSOR_TOOLCREATE,
	UI_CURSOR_ARROWDRAG,
	UI_CURSOR_ARROWCOPY,	// drag with copy
	UI_CURSOR_ARROWDRAGMULTI,
	UI_CURSOR_ARROWCOPYMULTI,	// drag with copy
	UI_CURSOR_NOLOCKED,
	UI_CURSOR_ARROWLOCKED,
	UI_CURSOR_GRABLOCKED,
	UI_CURSOR_TOOLTRANSLATE,
	UI_CURSOR_TOOLROTATE,
	UI_CURSOR_TOOLSCALE,
	UI_CURSOR_TOOLCAMERA,
	UI_CURSOR_TOOLPAN,
	UI_CURSOR_TOOLZOOMIN,
	UI_CURSOR_TOOLPICKOBJECT3,
	UI_CURSOR_TOOLSIT,
	UI_CURSOR_TOOLBUY,
	UI_CURSOR_TOOLPAY,
	UI_CURSOR_TOOLOPEN,
	UI_CURSOR_PIPETTE,
	UI_CURSOR_COUNT			// Number of elements in this enum (NOT a cursor)
};

class LLSplashScreen;

class LLWindow;

class LLPreeditor;

class LLWindowCallbacks
{
public:
	virtual ~LLWindowCallbacks() {}
	virtual BOOL handleTranslatedKeyDown(KEY key,  MASK mask, BOOL repeated);
	virtual BOOL handleTranslatedKeyUp(KEY key,  MASK mask);
	virtual void handleScanKey(KEY key, BOOL key_down, BOOL key_up, BOOL key_level);
	virtual BOOL handleUnicodeChar(llwchar uni_char, MASK mask);

	virtual BOOL handleMouseDown(LLWindow *window,  LLCoordGL pos, MASK mask);
	virtual BOOL handleMouseUp(LLWindow *window,  LLCoordGL pos, MASK mask);
	virtual void handleMouseLeave(LLWindow *window);
	// return TRUE to allow window to close, which will then cause handleQuit to be called
	virtual BOOL handleCloseRequest(LLWindow *window);
	// window is about to be destroyed, clean up your business
	virtual void handleQuit(LLWindow *window);
	virtual BOOL handleRightMouseDown(LLWindow *window,  LLCoordGL pos, MASK mask);
	virtual BOOL handleRightMouseUp(LLWindow *window,  LLCoordGL pos, MASK mask);
	virtual BOOL handleMiddleMouseDown(LLWindow *window,  LLCoordGL pos, MASK mask);
	virtual BOOL handleMiddleMouseUp(LLWindow *window,  LLCoordGL pos, MASK mask);
	virtual BOOL handleActivate(LLWindow *window, BOOL activated);
	virtual void handleMouseMove(LLWindow *window,  LLCoordGL pos, MASK mask);
	virtual void handleScrollWheel(LLWindow *window,  S32 clicks);
	virtual void handleResize(LLWindow *window,  S32 width,  S32 height);
	virtual void handleFocus(LLWindow *window);
	virtual void handleFocusLost(LLWindow *window);
	virtual void handleMenuSelect(LLWindow *window,  S32 menu_item);
	virtual BOOL handlePaint(LLWindow *window,  S32 x,  S32 y,  S32 width,  S32 height);
	virtual BOOL handleDoubleClick(LLWindow *window,  LLCoordGL pos, MASK mask);			// double-click of left mouse button
	virtual void handleWindowBlock(LLWindow *window);							// window is taking over CPU for a while
	virtual void handleWindowUnblock(LLWindow *window);							// window coming back after taking over CPU for a while
	virtual void handleDataCopy(LLWindow *window, S32 data_type, void *data);
};

// Refer to llwindow_test in test/common/llwindow for usage example

class LLWindow
{
public:
	struct LLWindowResolution
	{
		S32 mWidth;
		S32 mHeight;
	};
	enum ESwapMethod
	{
		SWAP_METHOD_UNDEFINED,
		SWAP_METHOD_EXCHANGE,
		SWAP_METHOD_COPY
	};
	enum EFlags
	{
		// currently unused
	};
public:
	virtual void show() = 0;
	virtual void hide() = 0;
	virtual void close() = 0;
	virtual BOOL getVisible() = 0;
	virtual BOOL getMinimized() = 0;
	virtual BOOL getMaximized() = 0;
	virtual BOOL maximize() = 0;
	BOOL getFullscreen()	{ return mFullscreen; };
	virtual BOOL getPosition(LLCoordScreen *position) = 0;
	virtual BOOL getSize(LLCoordScreen *size) = 0;
	virtual BOOL getSize(LLCoordWindow *size) = 0;
	virtual BOOL setPosition(LLCoordScreen position) = 0;
	virtual BOOL setSize(LLCoordScreen size) = 0;
	virtual BOOL switchContext(BOOL fullscreen, LLCoordScreen size, BOOL disable_vsync) = 0;
	virtual BOOL setCursorPosition(LLCoordWindow position) = 0;
	virtual BOOL getCursorPosition(LLCoordWindow *position) = 0;
	virtual void showCursor() = 0;
	virtual void hideCursor() = 0;
	virtual BOOL isCursorHidden() = 0;
	virtual void showCursorFromMouseMove() = 0;
	virtual void hideCursorUntilMouseMove() = 0;

	// These two functions create a way to make a busy cursor instead
	// of an arrow when someone's busy doing something. Draw an
	// arrow/hour if busycount > 0.
	virtual void incBusyCount();
	virtual void decBusyCount();
	virtual void resetBusyCount() { mBusyCount = 0; }
	virtual S32 getBusyCount() const { return mBusyCount; }

	// Sets cursor, may set to arrow+hourglass
	virtual void setCursor(ECursorType cursor) = 0;
	virtual ECursorType getCursor() const { return mCurrentCursor; }

	virtual void captureMouse() = 0;
	virtual void releaseMouse() = 0;
	virtual void setMouseClipping( BOOL b ) = 0;
	virtual BOOL isClipboardTextAvailable() = 0;
	virtual BOOL pasteTextFromClipboard(LLWString &dst) = 0;
	virtual BOOL copyTextToClipboard(const LLWString &src) = 0;
	virtual void flashIcon(F32 seconds) = 0;
	virtual F32 getGamma() = 0;
	virtual BOOL setGamma(const F32 gamma) = 0; // Set the gamma
	virtual BOOL restoreGamma() = 0;			// Restore original gamma table (before updating gamma)
	virtual ESwapMethod getSwapMethod() { return mSwapMethod; }
	virtual void gatherInput() = 0;
	virtual void delayInputProcessing() = 0;
	virtual void swapBuffers() = 0;
	virtual void bringToFront() = 0;
	virtual void focusClient() { };		// this may not have meaning or be required on other platforms, therefore, it's not abstract
	
	virtual S32 stat( const char* file_name, struct stat* stat_info ) = 0;
	virtual BOOL sendEmail(const char* address,const char* subject,const char* body_text, const char* attachment=NULL, const char* attachment_displayed_name=NULL ) = 0;


	// handy coordinate space conversion routines
	// NB: screen to window and vice verse won't work on width/height coordinate pairs,
	// as the conversion must take into account left AND right border widths, etc.
	virtual BOOL convertCoords( LLCoordScreen from, LLCoordWindow *to) = 0;
	virtual BOOL convertCoords( LLCoordWindow from, LLCoordScreen *to) = 0;
	virtual BOOL convertCoords( LLCoordWindow from, LLCoordGL *to) = 0;
	virtual BOOL convertCoords( LLCoordGL from, LLCoordWindow *to) = 0;
	virtual BOOL convertCoords( LLCoordScreen from, LLCoordGL *to) = 0;
	virtual BOOL convertCoords( LLCoordGL from, LLCoordScreen *to) = 0;

	// query supported resolutions
	virtual LLWindowResolution* getSupportedResolutions(S32 &num_resolutions) = 0;
	virtual F32	getNativeAspectRatio() = 0;
	virtual F32 getPixelAspectRatio() = 0;
	virtual void setNativeAspectRatio(F32 aspect) = 0;
	
	F32 getJoystickAxis(U32 axis);
	U8 getJoystickButton(U32 button);

	void setCallbacks(LLWindowCallbacks *callbacks);

	virtual void beforeDialog() {};	// prepare to put up an OS dialog (if special measures are required, such as in fullscreen mode)
	virtual void afterDialog() {};	// undo whatever was done in beforeDialog()

// opens system default color picker
	virtual BOOL dialog_color_picker (F32 *r, F32 *g, F32 *b) { return FALSE; };

// return a platform-specific window reference (HWND on Windows, WindowRef on the Mac)
	virtual void *getPlatformWindow() = 0;
	
	// control platform's Language Text Input mechanisms.
	virtual void allowLanguageTextInput(LLPreeditor *preeditor, BOOL b) {}
	virtual void setLanguageTextInput( const LLCoordGL & pos ) {};
	virtual void updateLanguageTextInputArea() {}
	virtual void interruptLanguageTextInput() {}

protected:
	LLWindow(BOOL fullscreen, U32 flags);
	virtual ~LLWindow() {}
	virtual BOOL isValid() {return TRUE;}
	virtual BOOL canDelete() {return TRUE;}
protected:
	static LLWindowCallbacks	sDefaultCallbacks;

protected:
	LLWindowCallbacks*	mCallbacks;

	BOOL		mPostQuit;		// should this window post a quit message when destroyed?
	BOOL		mFullscreen;
	S32			mFullscreenWidth;
	S32			mFullscreenHeight;
	S32			mFullscreenBits;
	S32			mFullscreenRefresh;
	LLWindowResolution* mSupportedResolutions;
	S32			mNumSupportedResolutions;
	ECursorType	mCurrentCursor;
	BOOL		mCursorHidden;
	S32			mBusyCount;	// how deep is the "cursor busy" stack?
	BOOL		mIsMouseClipping;  // Is this window currently clipping the mouse
	ESwapMethod mSwapMethod;
	BOOL		mHideCursorPermanent;
	U32			mFlags;
	F32			mJoyAxis[6]; 
	U8			mJoyButtonState[16];
	U16			mHighSurrogate;

 	// Handle a UTF-16 encoding unit received from keyboard.
 	// Converting the series of UTF-16 encoding units to UTF-32 data,
 	// this method passes the resulting UTF-32 data to mCallback's
 	// handleUnicodeChar.  The mask should be that to be passed to the
 	// callback.  This method uses mHighSurrogate as a dedicated work
 	// variable.
	void handleUnicodeUTF16(U16 utf16, MASK mask);

	friend class LLWindowManager;
};


// LLSplashScreen
// A simple, OS-specific splash screen that we can display
// while initializing the application and before creating a GL
// window


class LLSplashScreen
{
public:
	LLSplashScreen() { };
	virtual ~LLSplashScreen() { };


	// Call to display the window.
	static LLSplashScreen * create();
	static void show();
	static void hide();
	static void update(const char* string);

	static bool isVisible();
protected:
	// These are overridden by the platform implementation
	virtual void showImpl() = 0;
	virtual void updateImpl(const char* string) = 0;
	virtual void hideImpl() = 0;

	static BOOL sVisible;

};

// Platform-neutral for accessing the platform specific message box
S32 OSMessageBox(const char* text, const char* caption, U32 type);
const U32 OSMB_OK = 0;
const U32 OSMB_OKCANCEL = 1;
const U32 OSMB_YESNO = 2;

const S32 OSBTN_YES = 0;
const S32 OSBTN_NO = 1;
const S32 OSBTN_OK = 2;
const S32 OSBTN_CANCEL = 3;

//
// LLWindowManager
// Manages window creation and error checking

class LLWindowManager
{
public:
	static LLWindow* createWindow(
		char *title,
		char *name,
		LLCoordScreen upper_left = LLCoordScreen(10, 10),
		LLCoordScreen size = LLCoordScreen(320, 240),
		U32 flags = 0,
		BOOL fullscreen = FALSE,
		BOOL clearBg = FALSE,
		BOOL disable_vsync = TRUE,
		BOOL use_gl = TRUE,
		BOOL ignore_pixel_depth = FALSE);
	static LLWindow *createWindow(
		char* title, char* name, S32 x, S32 y, S32 width, S32 height,
		U32 flags = 0,
		BOOL fullscreen = FALSE,
		BOOL clearBg = FALSE,
		BOOL disable_vsync = TRUE,
		BOOL use_gl = TRUE,
		BOOL ignore_pixel_depth = FALSE);
	static BOOL destroyWindow(LLWindow* window);
	static BOOL isWindowValid(LLWindow *window);
};

//
// helper funcs
//
extern BOOL gDebugWindowProc;

// Protocols, like "http" and "https" we support in URLs
extern const S32 gURLProtocolWhitelistCount;
extern const char* gURLProtocolWhitelist[];
extern const char* gURLProtocolWhitelistHandler[];

// Loads a URL with the user's default browser
void spawn_web_browser(const char* escaped_url);

// Opens a file with ShellExecute. Security risk!
void shell_open(const char* file_path);

void simpleEscapeString ( std::string& stringIn  );

#endif // _LL_window_h_
