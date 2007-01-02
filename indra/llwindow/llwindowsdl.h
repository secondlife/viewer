/** 
 * @file llwindowsdl.h
 * @brief SDL implementation of LLWindow class
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLWINDOWSDL_H
#define LL_LLWINDOWSDL_H

// Simple Directmedia Layer (http://libsdl.org/) implementation of LLWindow class

#include "llwindow.h"

#include "SDL/SDL.h"

#if LL_X11
// get X11-specific headers for use in low-level stuff like copy-and-paste support
#include "SDL/SDL_syswm.h"
#endif

// AssertMacros.h does bad things.
#undef verify
#undef check
#undef require


class LLWindowSDL : public LLWindow
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
	/*virtual*/ ECursorType getCursor();
	/*virtual*/ void captureMouse();
	/*virtual*/ void releaseMouse();
	/*virtual*/ void setMouseClipping( BOOL b );
	/*virtual*/ BOOL isClipboardTextAvailable();
	/*virtual*/ BOOL pasteTextFromClipboard(LLWString &dst);
	/*virtual*/ BOOL copyTextToClipboard(const LLWString & src);
	/*virtual*/ void flashIcon(F32 seconds);
	/*virtual*/ F32 getGamma();
	/*virtual*/ BOOL setGamma(const F32 gamma); // Set the gamma
	/*virtual*/ BOOL restoreGamma();			// Restore original gamma table (before updating gamma)
	/*virtual*/ ESwapMethod getSwapMethod() { return mSwapMethod; }
	/*virtual*/ void gatherInput();
	/*virtual*/ void swapBuffers();

	/*virtual*/ LLString getTempFileName();
	/*virtual*/ void deleteFile( const char* file_name );
	/*virtual*/ S32 stat( const char* file_name, struct stat* stat_info );
	/*virtual*/ BOOL sendEmail(const char* address,const char* subject,const char* body_text,const char* attachment=NULL, const char* attachment_displayed_name=NULL);

    /*virtual*/ void delayInputProcessing() { };

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

	/*virtual*/ void beforeDialog();
	/*virtual*/ void afterDialog();

	/*virtual*/ BOOL dialog_color_picker(F32 *r, F32 *g, F32 *b);

	/*virtual*/ void *getPlatformWindow();
	/*virtual*/ void bringToFront();
	
protected:
	LLWindowSDL(
		char *title, int x, int y, int width, int height, U32 flags,
		BOOL fullscreen, BOOL clearBg, BOOL disable_vsync, BOOL use_gl,
		BOOL ignore_pixel_depth);
	~LLWindowSDL();

	void	initCursors();
	void	quitCursors();
	BOOL	isValid();
	void	moveWindow(const LLCoordScreen& position,const LLCoordScreen& size);


	// Changes display resolution. Returns true if successful
	BOOL	setDisplayResolution(S32 width, S32 height, S32 bits, S32 refresh);

	// Go back to last fullscreen display resolution.
	BOOL	setFullscreenResolution();

	void	minimize();
	void	restore();

	BOOL	shouldPostQuit() { return mPostQuit; }


protected:
	//
	// Platform specific methods
	//

	// create or re-create the GL context/window.  Called from the constructor and switchContext().
	BOOL createContext(int x, int y, int width, int height, int bits, BOOL fullscreen, BOOL disable_vsync);
	void destroyContext();
	void setupFailure(const char* text, const char* caption, U32 type);
	void adjustCursorDecouple(bool warpingMouse = false);
	void fixWindowSize(void);
	U32 SDLCheckGrabbyKeys(SDLKey keysym, BOOL gain);
	BOOL SDLReallyCaptureInput(BOOL capture);

	//
	// Platform specific variables
	//
	U32             mGrabbyKeyFlags;
	int			mReallyCapturedCount;
	SDL_Surface *	mWindow;
	char * 		mWindowTitle;
	double		mOriginalAspectRatio;
	BOOL		mCursorDecoupled;
	S32			mCursorLastEventDeltaX;
	S32			mCursorLastEventDeltaY;
	BOOL		mCursorIgnoreNextDelta;
	BOOL		mNeedsResize;		// Constructor figured out the window is too big, it needs a resize.
	LLCoordScreen   mNeedsResizeSize;
	F32			mOverrideAspectRatio;
	F32		mGamma;

	int		mSDLFlags;

	SDL_Cursor*	mSDLCursors[UI_CURSOR_COUNT];
	int             mHaveInputFocus; /* 0=no, 1=yes, else unknown */
	int             mIsMinimized; /* 0=no, 1=yes, else unknown */

	friend class LLWindowManager;

#if LL_X11
private:
	// These are set up by the X11 clipboard initialization code
	Window mSDL_XWindowID;
	void (*Lock_Display)(void);
	void (*Unlock_Display)(void);
	// more X11 clipboard stuff
	int init_x11clipboard(void);
	void quit_x11clipboard(void);
	int is_empty_x11clipboard(void);
	void put_x11clipboard(int type, int srclen, const char *src);
	void get_x11clipboard(int type, int *dstlen, char **dst);
	void x11_set_urgent(BOOL urgent);
	BOOL mFlashing;
	LLTimer mFlashTimer;
#endif //LL_X11

	
};


class LLSplashScreenSDL : public LLSplashScreen
{
public:
	LLSplashScreenSDL();
	virtual ~LLSplashScreenSDL();

	/*virtual*/ void showImpl();
	/*virtual*/ void updateImpl(const char* mesg);
	/*virtual*/ void hideImpl();
};

S32 OSMessageBoxSDL(const char* text, const char* caption, U32 type);

void load_url_external(const char* url);
void shell_open( const char* file_path );

#endif //LL_LLWINDOWSDL_H
