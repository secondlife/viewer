/** 
 * @file llwindowheadless.h
 * @brief Headless definition of LLWindow class
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
	/*virtual*/ BOOL switchContext(BOOL fullscreen, const LLCoordScreen &size, BOOL disable_vsync, const LLCoordScreen * const posp = NULL) {return FALSE;};
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
	/*virtual*/ void setFSAASamples(const U32 fsaa_samples) { }
	/*virtual*/ U32 getFSAASamples() { return 0; }
	/*virtual*/ BOOL restoreGamma() {return FALSE; };	// Restore original gamma table (before updating gamma)
	//virtual ESwapMethod getSwapMethod() { return mSwapMethod; }
	/*virtual*/ void gatherInput() {};
	/*virtual*/ void delayInputProcessing() {};
	/*virtual*/ void swapBuffers();

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
	
	LLWindowHeadless(const std::string& title, const std::string& name, S32 x, S32 y, S32 width, S32 height,
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
	/*virtual*/ void updateImpl(const std::string& mesg) {};
	/*virtual*/ void hideImpl() {};

};

#endif //LL_LLWINDOWHEADLESS_H

