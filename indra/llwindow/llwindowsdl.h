/** 
 * @file llwindowsdl.h
 * @brief SDL implementation of LLWindow class
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

#ifndef LL_LLWINDOWSDL_H
#define LL_LLWINDOWSDL_H

// Simple Directmedia Layer (http://libsdl.org/) implementation of LLWindow class

#include "llwindow.h"
#include "lltimer.h"

#include "SDL/SDL.h"
#include "SDL/SDL_endian.h"

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
	/*virtual*/ void minimize();
	/*virtual*/ void restore();
	/*virtual*/ BOOL getFullscreen();
	/*virtual*/ BOOL getPosition(LLCoordScreen *position);
	/*virtual*/ BOOL getSize(LLCoordScreen *size);
	/*virtual*/ BOOL getSize(LLCoordWindow *size);
	/*virtual*/ BOOL setPosition(LLCoordScreen position);
	/*virtual*/ BOOL setSize(LLCoordScreen size);
	/*virtual*/ BOOL switchContext(BOOL fullscreen, const LLCoordScreen &size, BOOL disable_vsync, const LLCoordScreen * const posp = NULL);
	/*virtual*/ BOOL setCursorPosition(LLCoordWindow position);
	/*virtual*/ BOOL getCursorPosition(LLCoordWindow *position);
	/*virtual*/ void showCursor();
	/*virtual*/ void hideCursor();
	/*virtual*/ void showCursorFromMouseMove();
	/*virtual*/ void hideCursorUntilMouseMove();
	/*virtual*/ BOOL isCursorHidden();
	/*virtual*/ void setCursor(ECursorType cursor);
	/*virtual*/ void captureMouse();
	/*virtual*/ void releaseMouse();
	/*virtual*/ void setMouseClipping( BOOL b );

	/*virtual*/ BOOL isClipboardTextAvailable();
	/*virtual*/ BOOL pasteTextFromClipboard(LLWString &dst);
	/*virtual*/ BOOL copyTextToClipboard(const LLWString & src);

	/*virtual*/ BOOL isPrimaryTextAvailable();
	/*virtual*/ BOOL pasteTextFromPrimary(LLWString &dst);
	/*virtual*/ BOOL copyTextToPrimary(const LLWString & src);
 
	/*virtual*/ void flashIcon(F32 seconds);
	/*virtual*/ F32 getGamma();
	/*virtual*/ BOOL setGamma(const F32 gamma); // Set the gamma
	/*virtual*/ U32 getFSAASamples();
	/*virtual*/ void setFSAASamples(const U32 samples);
	/*virtual*/ BOOL restoreGamma();			// Restore original gamma table (before updating gamma)
	/*virtual*/ ESwapMethod getSwapMethod() { return mSwapMethod; }
	/*virtual*/ void processMiscNativeEvents();
	/*virtual*/ void gatherInput();
	/*virtual*/ void swapBuffers();

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

	/*virtual*/ BOOL dialogColorPicker(F32 *r, F32 *g, F32 *b);

	/*virtual*/ void *getPlatformWindow();
	/*virtual*/ void bringToFront();

	/*virtual*/ void spawnWebBrowser(const std::string& escaped_url);
	
	static std::vector<std::string> getDynamicFallbackFontList();

	// Not great that these are public, but they have to be accessible
	// by non-class code and it's better than making them global.
#if LL_X11
	Window mSDL_XWindowID;
	Display *mSDL_Display;
#endif
	void (*Lock_Display)(void);
	void (*Unlock_Display)(void);

#if LL_GTK
	// Lazily initialize and check the runtime GTK version for goodness.
	static bool ll_try_gtk_init(void);
#endif // LL_GTK

#if LL_X11
	static Window get_SDL_XWindowID(void);
	static Display* get_SDL_Display(void);
#endif // LL_X11	

	static S32 getDisplayWidth();
	static S32 getDisplayHeight();

protected:
	LLWindowSDL(LLWindowCallbacks* callbacks,
		const std::string& title, int x, int y, int width, int height, U32 flags,
		BOOL fullscreen, BOOL clearBg, BOOL disable_vsync, BOOL use_gl,
		BOOL ignore_pixel_depth, U32 fsaa_samples);
	~LLWindowSDL();

	/*virtual*/ BOOL	isValid();
	/*virtual*/ LLSD    getNativeKeyData();

	void	initCursors();
	void	quitCursors();
	void	moveWindow(const LLCoordScreen& position,const LLCoordScreen& size);

	// Changes display resolution. Returns true if successful
	BOOL	setDisplayResolution(S32 width, S32 height, S32 bits, S32 refresh);

	// Go back to last fullscreen display resolution.
	BOOL	setFullscreenResolution();

	BOOL	shouldPostQuit() { return mPostQuit; }

protected:
	//
	// Platform specific methods
	//

	// create or re-create the GL context/window.  Called from the constructor and switchContext().
	BOOL createContext(int x, int y, int width, int height, int bits, BOOL fullscreen, BOOL disable_vsync);
	void destroyContext();
	void setupFailure(const std::string& text, const std::string& caption, U32 type);
	void fixWindowSize(void);
	U32 SDLCheckGrabbyKeys(SDLKey keysym, BOOL gain);
	BOOL SDLReallyCaptureInput(BOOL capture);

	//
	// Platform specific variables
	//
	U32             mGrabbyKeyFlags;
	int			mReallyCapturedCount;
	SDL_Surface *	mWindow;
	std::string mWindowTitle;
	double		mOriginalAspectRatio;
	BOOL		mNeedsResize;		// Constructor figured out the window is too big, it needs a resize.
	LLCoordScreen   mNeedsResizeSize;
	F32			mOverrideAspectRatio;
	F32		mGamma;
	U32		mFSAASamples;

	int		mSDLFlags;

	SDL_Cursor*	mSDLCursors[UI_CURSOR_COUNT];
	int             mHaveInputFocus; /* 0=no, 1=yes, else unknown */
	int             mIsMinimized; /* 0=no, 1=yes, else unknown */

	friend class LLWindowManager;

private:
#if LL_X11
	void x11_set_urgent(BOOL urgent);
	BOOL mFlashing;
	LLTimer mFlashTimer;
#endif //LL_X11
	
	U32 mKeyScanCode;
        U32 mKeyVirtualKey;
	SDLMod mKeyModifiers;
};


class LLSplashScreenSDL : public LLSplashScreen
{
public:
	LLSplashScreenSDL();
	virtual ~LLSplashScreenSDL();

	/*virtual*/ void showImpl();
	/*virtual*/ void updateImpl(const std::string& mesg);
	/*virtual*/ void hideImpl();
};

S32 OSMessageBoxSDL(const std::string& text, const std::string& caption, U32 type);

#endif //LL_LLWINDOWSDL_H
