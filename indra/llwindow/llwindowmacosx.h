/** 
 * @file llwindowmacosx.h
 * @brief Mac implementation of LLWindow class
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

#ifndef LL_LLWINDOWMACOSX_H
#define LL_LLWINDOWMACOSX_H

#include "llwindow.h"

#include <Carbon/Carbon.h>
#include <AGL/agl.h>

// AssertMacros.h does bad things.
#undef verify
#undef check
#undef require


class LLWindowMacOSX : public LLWindow
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
	/*virtual*/ void delayInputProcessing() {};
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

	/*virtual*/ void beforeDialog();
	/*virtual*/ void afterDialog();

	/*virtual*/ BOOL dialog_color_picker(F32 *r, F32 *g, F32 *b);

	/*virtual*/ void *getPlatformWindow();
	/*virtual*/ void bringToFront() {};
	
	/*virtual*/ void allowLanguageTextInput(LLPreeditor *preeditor, BOOL b);
	/*virtual*/ void updateLanguageTextInputArea(const LLCoordGL& caret, const LLRect& bounds);
	/*virtual*/ void interruptLanguageTextInput();

protected:
	LLWindowMacOSX(
		char *title, char *name, int x, int y, int width, int height, U32 flags,
		BOOL fullscreen, BOOL clearBg, BOOL disable_vsync, BOOL use_gl,
		BOOL ignore_pixel_depth);
	~LLWindowMacOSX();

	void	initCursors();
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

	// create or re-create the GL context/window.  Called from the constructor and switchContext().
	BOOL createContext(int x, int y, int width, int height, int bits, BOOL fullscreen, BOOL disable_vsync);
	void destroyContext();
	void setupFailure(const char* text, const char* caption, U32 type);
	static pascal OSStatus staticEventHandler (EventHandlerCallRef myHandler, EventRef event, void* userData);
	OSStatus eventHandler (EventHandlerCallRef myHandler, EventRef event);
	void adjustCursorDecouple(bool warpingMouse = false);
	void fixWindowSize(void);
	void stopDockTileBounce();


	//
	// Platform specific variables
	//
	WindowRef		mWindow;
	AGLContext		mContext;
	AGLPixelFormat	mPixelFormat;
	CGDirectDisplayID	mDisplay;
	CFDictionaryRef		mOldDisplayMode;
	EventLoopTimerRef	mTimer;
	EventHandlerUPP 	mEventHandlerUPP;
	EventHandlerRef		mGlobalHandlerRef;
	EventHandlerRef		mWindowHandlerRef;
	Rect		mOldMouseClip;  // Screen rect to which the mouse cursor was globally constrained before we changed it in clipMouse()
	Str255 		mWindowTitle;
	double		mOriginalAspectRatio;
	BOOL		mSimulatedRightClick;
	UInt32		mLastModifiers;
	BOOL		mHandsOffEvents;	// When true, temporarially disable CarbonEvent processing.
	// Used to allow event processing when putting up dialogs in fullscreen mode.
	BOOL		mCursorDecoupled;
	S32			mCursorLastEventDeltaX;
	S32			mCursorLastEventDeltaY;
	BOOL		mCursorIgnoreNextDelta;
	BOOL		mNeedsResize;		// Constructor figured out the window is too big, it needs a resize.
	LLCoordScreen   mNeedsResizeSize;
	F32			mOverrideAspectRatio;
	BOOL		mMinimized;

	F32			mBounceTime;
	NMRec		mBounceRec;
	LLTimer		mBounceTimer;

	// Imput method management through Text Service Manager.
	TSMDocumentID	mTSMDocument;
	BOOL		mLanguageTextInputAllowed;
	ScriptCode	mTSMScriptCode;
	LangCode	mTSMLangCode;
	LLPreeditor*	mPreeditor;

	friend class LLWindowManager;
};


class LLSplashScreenMacOSX : public LLSplashScreen
{
public:
	LLSplashScreenMacOSX();
	virtual ~LLSplashScreenMacOSX();

	/*virtual*/ void showImpl();
	/*virtual*/ void updateImpl(const char* mesg);
	/*virtual*/ void hideImpl();

private:
	WindowRef   mWindow;
};

S32 OSMessageBoxMacOSX(const char* text, const char* caption, U32 type);

void load_url_external(const char* url);

#endif //LL_LLWINDOWMACOSX_H
