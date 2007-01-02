/** 
 * @file llwindowheadless.h
 * @brief Headless definition of LLWindow class
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLWINDOWHEADLESS_H
#define LL_LLWINDOWHEADLESS_H

#include "llwindow.h"

class LLWindowHeadless : public LLWindow
{
public:
	/*virtual*/ void show() {};
	/*virtual*/ void hide() {};
	/*virtual*/ void close() {};
	/*virtual*/ BOOL getVisible() {return FALSE;};
	/*virtual*/ BOOL getMinimized() {return FALSE;};
	/*virtual*/ BOOL getMaximized() {return FALSE;};
	/*virtual*/ BOOL maximize() {return FALSE;};
	/*virtual*/ BOOL getFullscreen() {return FALSE;};
	/*virtual*/ BOOL getPosition(LLCoordScreen *position) {return FALSE;};
	/*virtual*/ BOOL getSize(LLCoordScreen *size) {return FALSE;};
	/*virtual*/ BOOL getSize(LLCoordWindow *size) {return FALSE;};
	/*virtual*/ BOOL setPosition(LLCoordScreen position) {return FALSE;};
	/*virtual*/ BOOL setSize(LLCoordScreen size) {return FALSE;};
	/*virtual*/ BOOL switchContext(BOOL fullscreen, LLCoordScreen size, BOOL disable_vsync) {return FALSE;};
	/*virtual*/ BOOL setCursorPosition(LLCoordWindow position) {return FALSE;};
	/*virtual*/ BOOL getCursorPosition(LLCoordWindow *position) {return FALSE;};
	/*virtual*/ void showCursor() {};
	/*virtual*/ void hideCursor() {};
	/*virtual*/ void showCursorFromMouseMove() {};
	/*virtual*/ void hideCursorUntilMouseMove() {};
	/*virtual*/ BOOL isCursorHidden() {return FALSE;};
	/*virtual*/ void setCursor(ECursorType cursor) {};
	//virtual ECursorType getCursor() { return mCurrentCursor; };
	/*virtual*/ void captureMouse() {};
	/*virtual*/ void releaseMouse() {};
	/*virtual*/ void setMouseClipping( BOOL b ) {};
	/*virtual*/ BOOL isClipboardTextAvailable() {return FALSE; };
	/*virtual*/ BOOL pasteTextFromClipboard(LLWString &dst) {return FALSE; };
	/*virtual*/ BOOL copyTextToClipboard(const LLWString &src) {return FALSE; };
	/*virtual*/ void flashIcon(F32 seconds) {};
	/*virtual*/ F32 getGamma() {return 1.0f; };
	/*virtual*/ BOOL setGamma(const F32 gamma) {return FALSE; }; // Set the gamma
	/*virtual*/ BOOL restoreGamma() {return FALSE; };	// Restore original gamma table (before updating gamma)
	//virtual ESwapMethod getSwapMethod() { return mSwapMethod; }
	/*virtual*/ void gatherInput() {};
	/*virtual*/ void delayInputProcessing() {};
	/*virtual*/ void swapBuffers();

	/*virtual*/ LLString getTempFileName() {return LLString(""); };
	/*virtual*/ void deleteFile( const char* file_name ) {};
	/*virtual*/ S32 stat( const char* file_name, struct stat* stat_info ) {return 0; };
	/*virtual*/ BOOL sendEmail(const char* address,const char* subject,const char* body_text,const char* attachment=NULL, const char* attachment_displayed_name=NULL) { return FALSE; };


	// handy coordinate space conversion routines
	/*virtual*/ BOOL convertCoords(LLCoordScreen from, LLCoordWindow *to) { return FALSE; };
	/*virtual*/ BOOL convertCoords(LLCoordWindow from, LLCoordScreen *to) { return FALSE; };
	/*virtual*/ BOOL convertCoords(LLCoordWindow from, LLCoordGL *to) { return FALSE; };
	/*virtual*/ BOOL convertCoords(LLCoordGL from, LLCoordWindow *to) { return FALSE; };
	/*virtual*/ BOOL convertCoords(LLCoordScreen from, LLCoordGL *to) { return FALSE; };
	/*virtual*/ BOOL convertCoords(LLCoordGL from, LLCoordScreen *to) { return FALSE; };

	/*virtual*/ LLWindowResolution* getSupportedResolutions(S32 &num_resolutions) { return NULL; };
	/*virtual*/ F32	getNativeAspectRatio() { return 1.0f; };
	/*virtual*/ F32 getPixelAspectRatio() { return 1.0f; };
	/*virtual*/ void setNativeAspectRatio(F32 ratio) {}

	/*virtual*/ void *getPlatformWindow() { return 0; };
	/*virtual*/ void bringToFront() {};
	
	LLWindowHeadless(char *title, char *name, S32 x, S32 y, S32 width, S32 height,
				  U32 flags,  BOOL fullscreen, BOOL clearBg,
				  BOOL disable_vsync, BOOL use_gl, BOOL ignore_pixel_depth);
	virtual ~LLWindowHeadless();

private:
};

class LLSplashScreenHeadless : public LLSplashScreen
{
public:
	LLSplashScreenHeadless() {};
	virtual ~LLSplashScreenHeadless() {};

	/*virtual*/ void showImpl() {};
	/*virtual*/ void updateImpl(const char* mesg) {};
	/*virtual*/ void hideImpl() {};

};

#endif //LL_LLWINDOWHEADLESS_H

