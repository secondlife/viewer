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
    void show() override {}
    void hide() override {}
    void close() override {}
    bool getVisible() const override {return false;}
    bool getMinimized() const override {return false;}
    bool getMaximized() const override {return false;}
    bool maximize() override {return false;}
    void minimize() override {}
    void restore() override {}
    bool getFullscreen() const override {return false;};
    bool getPosition(LLCoordScreen *position) const override {return false;}
    bool getSize(LLCoordScreen *size) const override {return false;}
    bool getSize(LLCoordWindow *size) const override {return false;}
    bool setPosition(LLCoordScreen position) override {return false;}
    bool setSizeImpl(LLCoordScreen size) override {return false;}
    bool setSizeImpl(LLCoordWindow size) override {return false;}
    bool switchContext(bool fullscreen, const LLCoordScreen &size, bool enable_vsync, const LLCoordScreen * const posp = NULL) override {return false;}
    void* createSharedContext() override  { return nullptr; }
    void makeContextCurrent(void*) override  {}
    void destroySharedContext(void*) override  {}
    void toggleVSync(bool enable_vsync) override { }
    bool setCursorPosition(LLCoordWindow position) override {return false;}
    bool getCursorPosition(LLCoordWindow *position) override {return false;}
#if LL_WINDOWS
    bool getCursorDelta(LLCoordCommon* delta) const override { return false; }
#endif
    bool isWarpMouse() const override { return true; }
    void showCursor() override {}
    void hideCursor() override {}
    void showCursorFromMouseMove() override {}
    void hideCursorUntilMouseMove() override {}
    bool isCursorHidden() override {return false;}
    void updateCursor() override {}
    void captureMouse() override {}
    void releaseMouse() override {}
    void setMouseClipping( bool b ) override {}
    bool isClipboardTextAvailable() override {return false; }
    bool pasteTextFromClipboard(LLWString &dst) override {return false; }
    bool copyTextToClipboard(const LLWString &src) override {return false; }
    void flashIcon(F32 seconds) override {}
    F32 getGamma() const override {return 1.0f; }
    bool setGamma(const F32 gamma) override {return false; } // Set the gamma
    void setFSAASamples(const U32 fsaa_samples) override { }
    U32 getFSAASamples() const override { return 0; }
    bool restoreGamma() override {return false; }   // Restore original gamma table (before updating gamma)
    void gatherInput(bool app_has_focus) override {}
    void delayInputProcessing() override {}
    void swapBuffers() override;


    // handy coordinate space conversion routines
    bool convertCoords(LLCoordScreen from, LLCoordWindow *to) const override { return false; }
    bool convertCoords(LLCoordWindow from, LLCoordScreen *to) const override { return false; }
    bool convertCoords(LLCoordWindow from, LLCoordGL *to) const override { return false; }
    bool convertCoords(LLCoordGL from, LLCoordWindow *to) const override { return false; }
    bool convertCoords(LLCoordScreen from, LLCoordGL *to) const override { return false; }
    bool convertCoords(LLCoordGL from, LLCoordScreen *to) const override { return false; }

    LLWindowResolution* getSupportedResolutions(S32 &num_resolutions) override { return NULL; }
    F32 getNativeAspectRatio() override { return 1.0f; }
    F32 getPixelAspectRatio() override { return 1.0f; }
    void setNativeAspectRatio(F32 ratio) override {}

    void *getPlatformWindow() override { return 0; }
    void bringToFront() override {}

    LLWindowHeadless(LLWindowCallbacks* callbacks,
        const std::string& title, const std::string& name,
        S32 x, S32 y,
        S32 width, S32 height,
        U32 flags,  bool fullscreen, bool clear_background,
        bool enable_vsync, bool use_gl, bool ignore_pixel_depth);
    virtual ~LLWindowHeadless();

private:
};

class LLSplashScreenHeadless : public LLSplashScreen
{
public:
    LLSplashScreenHeadless() {}
    virtual ~LLSplashScreenHeadless() {}

    /*virtual*/ void showImpl() override {}
    /*virtual*/ void updateImpl(const std::string& mesg) override {}
    /*virtual*/ void hideImpl() override {}

};

#endif //LL_LLWINDOWHEADLESS_H

