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
    void show() override {};
    void hide() override {};
    void close() override {};
    bool getVisible() override {return false;};
    bool getMinimized() override {return false;};
    bool getMaximized() override {return false;};
    bool maximize() override {return false;};
    void minimize() override {};
    void restore() override {};
    bool getFullscreen() override {return false;};
    bool getPosition(LLCoordScreen *position) override {return false;};
    bool getSize(LLCoordScreen *size) override {return false;};
    bool getSize(LLCoordWindow *size) override {return false;};
    bool setPosition(LLCoordScreen position) override {return false;};
    bool setSizeImpl(LLCoordScreen size) override {return false;};
    bool switchContext(bool fullscreen, const LLCoordScreen &size, bool disable_vsync, const LLCoordScreen * const posp = NULL) override {return false;};
    bool setCursorPosition(LLCoordWindow position) override {return false;};
    bool getCursorPosition(LLCoordWindow *position) override {return false;};
    void showCursor() override {};
    void hideCursor() override {};
    void showCursorFromMouseMove() override {};
    void hideCursorUntilMouseMove() override {};
    bool isCursorHidden() override {return false;};
    void updateCursor() override {};
    //ECursorType getCursor() override { return mCurrentCursor; };
    void captureMouse() override {};
    void releaseMouse() override {};
    void setMouseClipping( bool b ) override {};
    bool isClipboardTextAvailable() override {return false; };
    bool pasteTextFromClipboard(LLWString &dst) override {return false; };
    bool copyTextToClipboard(const LLWString &src) override {return false; };
    void flashIcon(F32 seconds) override {};
    F32 getGamma() override {return 1.0f; };
    bool setGamma(const F32 gamma) override {return false; }; // Set the gamma
    bool restoreGamma() override {return false; };   // Restore original gamma table (before updating gamma)
    void setFSAASamples(const U32 fsaa_samples) override { /* FSAA not supported yet on Mesa headless.*/ }
    U32  getFSAASamples() override { return 0; }
    //ESwapMethod getSwapMethod() override { return mSwapMethod; }
    void gatherInput(bool app_has_focus) override {};
    void delayInputProcessing() override {};
    void swapBuffers() override;
    void restoreGLContext() override {};

    // handy coordinate space conversion routines
    bool convertCoords(LLCoordScreen from, LLCoordWindow *to) override { return false; };
    bool convertCoords(LLCoordWindow from, LLCoordScreen *to) override { return false; };
    bool convertCoords(LLCoordWindow from, LLCoordGL *to) override { return false; };
    bool convertCoords(LLCoordGL from, LLCoordWindow *to) override { return false; };
    bool convertCoords(LLCoordScreen from, LLCoordGL *to) override { return false; };
    bool convertCoords(LLCoordGL from, LLCoordScreen *to) override { return false; };

    LLWindowResolution* getSupportedResolutions(S32 &num_resolutions) override { return NULL; };
    F32 getNativeAspectRatio() override { return 1.0f; };
    F32 getPixelAspectRatio() override { return 1.0f; };
    void setNativeAspectRatio(F32 ratio) override {}

    void *getPlatformWindow() override { return 0; };
    void bringToFront() override {};

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

    void showImpl() override {};
    void updateImpl(const std::string& mesg) override {};
    void hideImpl() override {};

};

#endif

#endif //LL_LLWINDOWMESAHEADLESS_H
