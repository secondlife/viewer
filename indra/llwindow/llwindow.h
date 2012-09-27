/** 
 * @file llwindow.h
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

#ifndef LL_LLWINDOW_H
#define LL_LLWINDOW_H

#include "llrect.h"
#include "llcoord.h"
#include "llstring.h"
#include "llcursortypes.h"
#include "llsd.h"

class LLSplashScreen;
class LLPreeditor;
class LLWindowCallbacks;

// Refer to llwindow_test in test/common/llwindow for usage example

class LLWindow : public LLInstanceTracker<LLWindow>
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
	virtual void minimize() = 0;
	virtual void restore() = 0;
	BOOL getFullscreen()	{ return mFullscreen; };
	virtual BOOL getPosition(LLCoordScreen *position) = 0;
	virtual BOOL getSize(LLCoordScreen *size) = 0;
	virtual BOOL getSize(LLCoordWindow *size) = 0;
	virtual BOOL setPosition(LLCoordScreen position) = 0;
	BOOL setSize(LLCoordScreen size);
	BOOL setSize(LLCoordWindow size);
	virtual void setMinSize(U32 min_width, U32 min_height, bool enforce_immediately = true);
	virtual BOOL switchContext(BOOL fullscreen, const LLCoordScreen &size, BOOL disable_vsync, const LLCoordScreen * const posp = NULL) = 0;
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
	virtual void resetBusyCount();
	virtual S32 getBusyCount() const;

	// Sets cursor, may set to arrow+hourglass
	virtual void setCursor(ECursorType cursor) { mNextCursor = cursor; };
	virtual ECursorType getCursor() const;
	virtual void updateCursor() = 0;

	virtual void captureMouse() = 0;
	virtual void releaseMouse() = 0;
	virtual void setMouseClipping( BOOL b ) = 0;

	virtual BOOL isClipboardTextAvailable() = 0;
	virtual BOOL pasteTextFromClipboard(LLWString &dst) = 0;
	virtual BOOL copyTextToClipboard(const LLWString &src) = 0;

	virtual BOOL isPrimaryTextAvailable();
	virtual BOOL pasteTextFromPrimary(LLWString &dst);
	virtual BOOL copyTextToPrimary(const LLWString &src);
 
	virtual void flashIcon(F32 seconds) = 0;
	virtual F32 getGamma() = 0;
	virtual BOOL setGamma(const F32 gamma) = 0; // Set the gamma
	virtual void setFSAASamples(const U32 fsaa_samples) = 0; //set number of FSAA samples
	virtual U32	 getFSAASamples() = 0;
	virtual BOOL restoreGamma() = 0;			// Restore original gamma table (before updating gamma)
	virtual ESwapMethod getSwapMethod() { return mSwapMethod; }
	virtual void processMiscNativeEvents();
	virtual void gatherInput() = 0;
	virtual void delayInputProcessing() = 0;
	virtual void swapBuffers() = 0;
	virtual void bringToFront() = 0;
	virtual void focusClient() { };		// this may not have meaning or be required on other platforms, therefore, it's not abstract
	
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
	
	virtual void beforeDialog() {};	// prepare to put up an OS dialog (if special measures are required, such as in fullscreen mode)
	virtual void afterDialog() {};	// undo whatever was done in beforeDialog()

	// opens system default color picker, modally
	// Returns TRUE if valid color selected
	virtual BOOL dialogColorPicker(F32 *r, F32 *g, F32 *b);

// return a platform-specific window reference (HWND on Windows, WindowRef on the Mac, Gtk window on Linux)
	virtual void *getPlatformWindow() = 0;

// return the platform-specific window reference we use to initialize llmozlib (HWND on Windows, WindowRef on the Mac, Gtk window on Linux)
	virtual void *getMediaWindow();
	
	// control platform's Language Text Input mechanisms.
	virtual void allowLanguageTextInput(LLPreeditor *preeditor, BOOL b) {}
	virtual void setLanguageTextInput( const LLCoordGL & pos ) {};
	virtual void updateLanguageTextInputArea() {}
	virtual void interruptLanguageTextInput() {}
	virtual void spawnWebBrowser(const std::string& escaped_url, bool async) {};

	static std::vector<std::string> getDynamicFallbackFontList();
	
	// Provide native key event data
	virtual LLSD getNativeKeyData() { return LLSD::emptyMap(); }

protected:
	LLWindow(LLWindowCallbacks* callbacks, BOOL fullscreen, U32 flags);
	virtual ~LLWindow();
	// Defaults to true
	virtual BOOL isValid();
	// Defaults to true
	virtual BOOL canDelete();

	virtual BOOL setSizeImpl(LLCoordScreen size) = 0;
	virtual BOOL setSizeImpl(LLCoordWindow size) = 0;

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
	ECursorType	mNextCursor;
	BOOL		mCursorHidden;
	S32			mBusyCount;	// how deep is the "cursor busy" stack?
	BOOL		mIsMouseClipping;  // Is this window currently clipping the mouse
	ESwapMethod mSwapMethod;
	BOOL		mHideCursorPermanent;
	U32			mFlags;
	U16			mHighSurrogate;
	S32			mMinWindowWidth;
	S32			mMinWindowHeight;

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
	static void update(const std::string& string);

	static bool isVisible();
protected:
	// These are overridden by the platform implementation
	virtual void showImpl() = 0;
	virtual void updateImpl(const std::string& string) = 0;
	virtual void hideImpl() = 0;

	static BOOL sVisible;

};

// Platform-neutral for accessing the platform specific message box
S32 OSMessageBox(const std::string& text, const std::string& caption, U32 type);
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
	static LLWindow *createWindow(
		LLWindowCallbacks* callbacks,
		const std::string& title, const std::string& name, S32 x, S32 y, S32 width, S32 height,
		U32 flags = 0,
		BOOL fullscreen = FALSE,
		BOOL clearBg = FALSE,
		BOOL disable_vsync = TRUE,
		BOOL ignore_pixel_depth = FALSE,
		U32 fsaa_samples = 0);
	static BOOL destroyWindow(LLWindow* window);
	static BOOL isWindowValid(LLWindow *window);
};

//
// helper funcs
//
extern BOOL gDebugWindowProc;

// Protocols, like "http" and "https" we support in URLs
extern const S32 gURLProtocolWhitelistCount;
extern const std::string gURLProtocolWhitelist[];
extern const std::string gURLProtocolWhitelistHandler[];

void simpleEscapeString ( std::string& stringIn  );

#endif // _LL_window_h_
