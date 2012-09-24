/** 
 * @file llwindowheadless.h
 * @brief Headless definition of LLWindow class
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
	/*virtual*/ void minimize() {};
	/*virtual*/ void restore() {};
	/*virtual*/ BOOL getFullscreen() {return FALSE;};
	/*virtual*/ BOOL getPosition(LLCoordScreen *position) {return FALSE;};
	/*virtual*/ BOOL getSize(LLCoordScreen *size) {return FALSE;};
	/*virtual*/ BOOL getSize(LLCoordWindow *size) {return FALSE;};
	/*virtual*/ BOOL setPosition(LLCoordScreen position) {return FALSE;};
	/*virtual*/ BOOL setSizeImpl(LLCoordScreen size) {return FALSE;};
	/*virtual*/ BOOL setSizeImpl(LLCoordWindow size) {return FALSE;};
	/*virtual*/ BOOL switchContext(BOOL fullscreen, const LLCoordScreen &size, BOOL disable_vsync, const LLCoordScreen * const posp = NULL) {return FALSE;};
	/*virtual*/ BOOL setCursorPosition(LLCoordWindow position) {return FALSE;};
	/*virtual*/ BOOL getCursorPosition(LLCoordWindow *position) {return FALSE;};
	/*virtual*/ void showCursor() {};
	/*virtual*/ void hideCursor() {};
	/*virtual*/ void showCursorFromMouseMove() {};
	/*virtual*/ void hideCursorUntilMouseMove() {};
	/*virtual*/ BOOL isCursorHidden() {return FALSE;};
	/*virtual*/ void updateCursor() {};
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
	
	LLWindowHeadless(LLWindowCallbacks* callbacks,
		const std::string& title, const std::string& name,
		S32 x, S32 y, 
		S32 width, S32 height,
		U32 flags,  BOOL fullscreen, BOOL clear_background,
		BOOL disable_vsync, BOOL ignore_pixel_depth);
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

