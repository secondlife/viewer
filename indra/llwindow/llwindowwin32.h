/** 
 * @file llwindowwin32.h
 * @brief Windows implementation of LLWindow class
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLWINDOWWIN32_H
#define LL_LLWINDOWWIN32_H

// Limit Windows API to small and manageable set.
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <windows.h>

#include "llwindow.h"

// Hack for async host by name
#define LL_WM_HOST_RESOLVED      (WM_APP + 1)
typedef void (*LLW32MsgCallback)(const MSG &msg);

class LLWindowWin32 : public LLWindow
{
public:
	/*virtual*/ void show();
	/*virtual*/ void hide();
	/*virtual*/ void close();
	/*virtual*/ BOOL getVisible();
	/*virtual*/ BOOL getMinimized();
	/*virtual*/ BOOL getMaximized();
	/*virtual*/ BOOL maximize();
	/*virtual*/ BOOL getFullscreen();
	/*virtual*/ BOOL getPosition(LLCoordScreen *position);
	/*virtual*/ BOOL getSize(LLCoordScreen *size);
	/*virtual*/ BOOL getSize(LLCoordWindow *size);
	/*virtual*/ BOOL setPosition(LLCoordScreen position);
	/*virtual*/ BOOL setSize(LLCoordScreen size);
	/*virtual*/ BOOL switchContext(BOOL fullscreen, LLCoordScreen size, BOOL disable_vsync);
	/*virtual*/ BOOL setCursorPosition(LLCoordWindow position);
	/*virtual*/ BOOL getCursorPosition(LLCoordWindow *position);
	/*virtual*/ void showCursor();
	/*virtual*/ void hideCursor();
	/*virtual*/ void showCursorFromMouseMove();
	/*virtual*/ void hideCursorUntilMouseMove();
	/*virtual*/ BOOL isCursorHidden();
	/*virtual*/ void setCursor(ECursorType cursor);
	/*virtual*/ ECursorType getCursor() const;
	/*virtual*/ void captureMouse();
	/*virtual*/ void releaseMouse();
	/*virtual*/ void setMouseClipping( BOOL b );
	/*virtual*/ BOOL isClipboardTextAvailable();
	/*virtual*/ BOOL pasteTextFromClipboard(LLWString &dst);
	/*virtual*/ BOOL copyTextToClipboard(const LLWString &src);
	/*virtual*/ void flashIcon(F32 seconds);
	/*virtual*/ F32 getGamma();
	/*virtual*/ BOOL setGamma(const F32 gamma); // Set the gamma
	/*virtual*/ BOOL restoreGamma();			// Restore original gamma table (before updating gamma)
	/*virtual*/ ESwapMethod getSwapMethod() { return mSwapMethod; }
	/*virtual*/ void gatherInput();
	/*virtual*/ void delayInputProcessing();
	/*virtual*/ void swapBuffers();

	/*virtual*/ LLString getTempFileName();
	/*virtual*/ void deleteFile( const char* file_name );
	/*virtual*/ S32 stat( const char* file_name, struct stat* stat_info );
	/*virtual*/ BOOL sendEmail(const char* address,const char* subject,const char* body_text,const char* attachment=NULL, const char* attachment_displayed_name=NULL);


	// handy coordinate space conversion routines
	/*virtual*/ BOOL convertCoords(LLCoordScreen from, LLCoordWindow *to);
	/*virtual*/ BOOL convertCoords(LLCoordWindow from, LLCoordScreen *to);
	/*virtual*/ BOOL convertCoords(LLCoordWindow from, LLCoordGL *to);
	/*virtual*/ BOOL convertCoords(LLCoordGL from, LLCoordWindow *to);
	/*virtual*/ BOOL convertCoords(LLCoordScreen from, LLCoordGL *to);
	/*virtual*/ BOOL convertCoords(LLCoordGL from, LLCoordScreen *to);

	/*virtual*/ LLWindowResolution* getSupportedResolutions(S32 &num_resolutions);
	/*virtual*/ F32	getNativeAspectRatio();
	/*virtual*/ F32 getPixelAspectRatio();
	/*virtual*/ void setNativeAspectRatio(F32 ratio) { mOverrideAspectRatio = ratio; }

	/*virtual*/	BOOL dialog_color_picker (F32 *r, F32 *g, F32 *b );

	/*virtual*/ void *getPlatformWindow();
	/*virtual*/ void bringToFront();
	/*virtual*/ void focusClient();

protected:
	LLWindowWin32(
		char *title, char *name, int x, int y, int width, int height, U32 flags, 
		BOOL fullscreen, BOOL clearBg, BOOL disable_vsync, BOOL use_gl,
		BOOL ignore_pixel_depth);
	~LLWindowWin32();

	void	initCursors();
	HCURSOR loadColorCursor(LPCTSTR name);
	BOOL	isValid();
	void	moveWindow(const LLCoordScreen& position,const LLCoordScreen& size);


	// Changes display resolution. Returns true if successful
	BOOL	setDisplayResolution(S32 width, S32 height, S32 bits, S32 refresh);

	// Go back to last fullscreen display resolution.
	BOOL	setFullscreenResolution();

	// Restore the display resolution to its value before we ran the app.
	BOOL	resetDisplayResolution();

	void	minimize();
	void	restore();

	BOOL	shouldPostQuit() { return mPostQuit; }


protected:
	//
	// Platform specific methods
	//

	BOOL	getClientRectInScreenSpace(RECT* rectp);
	void 	updateJoystick( );

	static LRESULT CALLBACK mainWindowProc(HWND h_wnd, UINT u_msg, WPARAM w_param, LPARAM l_param);
	static BOOL CALLBACK enumChildWindows(HWND h_wnd, LPARAM l_param);


	//
	// Platform specific variables
	//
	WCHAR		*mWindowTitle;
	WCHAR		*mWindowClassName;

	HWND		mWindowHandle;	// window handle
	HGLRC		mhRC;			// OpenGL rendering context
	HDC			mhDC;			// Windows Device context handle
	HINSTANCE	mhInstance;		// handle to application instance
	WNDPROC		mWndProc;		// user-installable window proc
	RECT		mOldMouseClip;  // Screen rect to which the mouse cursor was globally constrained before we changed it in clipMouse()
	WPARAM		mLastSizeWParam;
	F32			mOverrideAspectRatio;
	F32			mNativeAspectRatio;

	HCURSOR		mCursor[ UI_CURSOR_COUNT ];  // Array of all mouse cursors

	static BOOL sIsClassRegistered; // has the window class been registered?

	F32			mCurrentGamma;
	WORD		mPrevGammaRamp[256*3];
	WORD		mCurrentGammaRamp[256*3];

	LPWSTR		mIconResource;
	BOOL		mMousePositionModified;
	BOOL		mInputProcessingPaused;

	friend class LLWindowManager;
};

class LLSplashScreenWin32 : public LLSplashScreen
{
public:
	LLSplashScreenWin32();
	virtual ~LLSplashScreenWin32();

	/*virtual*/ void showImpl();
	/*virtual*/ void updateImpl(const char* mesg);
	/*virtual*/ void hideImpl();

#if LL_WINDOWS
	static LRESULT CALLBACK windowProc(HWND h_wnd, UINT u_msg, 
		WPARAM w_param, LPARAM l_param);
#endif

private:
#if LL_WINDOWS
	HWND mWindow;
#endif
};

extern LLW32MsgCallback gAsyncMsgCallback;
extern LPWSTR gIconResource;

static void	handleMessage( const MSG& msg );

S32 OSMessageBoxWin32(const char* text, const char* caption, U32 type);

#endif //LL_LLWINDOWWIN32_H
