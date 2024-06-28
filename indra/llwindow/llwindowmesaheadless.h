/**
 * @file llwindowmesaheadless.h
 * @brief Windows implementation of LLWindow class
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

#ifndef LL_LLWINDOWMESAHEADLESS_H
#define LL_LLWINDOWMESAHEADLESS_H

#if LL_MESA_HEADLESS

#include "llwindow.h"
#include "GL/glu.h"
#include "GL/osmesa.h"

class LLWindowMesaHeadless : public LLWindow
{
public:
    /*virtual*/ void show() {};
    /*virtual*/ void hide() {};
    /*virtual*/ void close() {};
    /*virtual*/ bool getVisible() {return false;};
    /*virtual*/ bool getMinimized() {return false;};
    /*virtual*/ bool getMaximized() {return false;};
    /*virtual*/ bool maximize() {return false;};
    /*virtual*/ void minimize() {};
    /*virtual*/ void restore() {};
    /*virtual*/ bool getFullscreen() {return false;};
    /*virtual*/ bool getPosition(LLCoordScreen *position) {return false;};
    /*virtual*/ bool getSize(LLCoordScreen *size) {return false;};
    /*virtual*/ bool getSize(LLCoordWindow *size) {return false;};
    /*virtual*/ bool setPosition(LLCoordScreen position) {return false;};
    /*virtual*/ bool setSizeImpl(LLCoordScreen size) {return false;};
    /*virtual*/ bool switchContext(bool fullscreen, const LLCoordScreen &size, bool disable_vsync, const LLCoordScreen * const posp = NULL) {return false;};
    /*virtual*/ bool setCursorPosition(LLCoordWindow position) {return false;};
    /*virtual*/ bool getCursorPosition(LLCoordWindow *position) {return false;};
    /*virtual*/ void showCursor() {};
    /*virtual*/ void hideCursor() {};
    /*virtual*/ void showCursorFromMouseMove() {};
    /*virtual*/ void hideCursorUntilMouseMove() {};
    /*virtual*/ bool isCursorHidden() {return false;};
    /*virtual*/ void updateCursor() {};
    //virtual ECursorType getCursor() { return mCurrentCursor; };
    /*virtual*/ void captureMouse() {};
    /*virtual*/ void releaseMouse() {};
    /*virtual*/ void setMouseClipping( bool b ) {};
    /*virtual*/ bool isClipboardTextAvailable() {return false; };
    /*virtual*/ bool pasteTextFromClipboard(LLWString &dst) {return false; };
    /*virtual*/ bool copyTextToClipboard(const LLWString &src) {return false; };
    /*virtual*/ void flashIcon(F32 seconds) {};
    /*virtual*/ F32 getGamma() {return 1.0f; };
    /*virtual*/ bool setGamma(const F32 gamma) {return false; }; // Set the gamma
    /*virtual*/ bool restoreGamma() {return false; };   // Restore original gamma table (before updating gamma)
    /*virtual*/ void setFSAASamples(const U32 fsaa_samples) { /* FSAA not supported yet on Mesa headless.*/ }
    /*virtual*/ U32  getFSAASamples() { return 0; }
    //virtual ESwapMethod getSwapMethod() { return mSwapMethod; }
    /*virtual*/ void gatherInput() {};
    /*virtual*/ void delayInputProcessing() {};
    /*virtual*/ void swapBuffers();
    /*virtual*/ void restoreGLContext() {};

    // handy coordinate space conversion routines
    /*virtual*/ bool convertCoords(LLCoordScreen from, LLCoordWindow *to) { return false; };
    /*virtual*/ bool convertCoords(LLCoordWindow from, LLCoordScreen *to) { return false; };
    /*virtual*/ bool convertCoords(LLCoordWindow from, LLCoordGL *to) { return false; };
    /*virtual*/ bool convertCoords(LLCoordGL from, LLCoordWindow *to) { return false; };
    /*virtual*/ bool convertCoords(LLCoordScreen from, LLCoordGL *to) { return false; };
    /*virtual*/ bool convertCoords(LLCoordGL from, LLCoordScreen *to) { return false; };

    /*virtual*/ LLWindowResolution* getSupportedResolutions(S32 &num_resolutions) { return NULL; };
    /*virtual*/ F32 getNativeAspectRatio() { return 1.0f; };
    /*virtual*/ F32 getPixelAspectRatio() { return 1.0f; };
    /*virtual*/ void setNativeAspectRatio(F32 ratio) {}

    /*virtual*/ void *getPlatformWindow() { return 0; };
    /*virtual*/ void bringToFront() {};

    LLWindowMesaHeadless(LLWindowCallbacks* callbacks,
                         const std::string& title, const std::string& name, S32 x, S32 y, S32 width, S32 height,
                  U32 flags,  bool fullscreen, bool clearBg,
                  bool disable_vsync, bool use_gl, bool ignore_pixel_depth);
    ~LLWindowMesaHeadless();

private:
    OSMesaContext   mMesaContext;
    unsigned char * mMesaBuffer;
};

class LLSplashScreenMesaHeadless : public LLSplashScreen
{
public:
    LLSplashScreenMesaHeadless() {};
    virtual ~LLSplashScreenMesaHeadless() {};

    /*virtual*/ void showImpl() {};
    /*virtual*/ void updateImpl(const std::string& mesg) {};
    /*virtual*/ void hideImpl() {};

};

#endif

#endif //LL_LLWINDOWMESAHEADLESS_H
