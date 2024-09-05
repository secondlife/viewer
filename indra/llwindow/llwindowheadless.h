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
    /*virtual*/ void show() override {}
    /*virtual*/ void hide() override {}
    /*virtual*/ void close() override {}
    /*virtual*/ bool getVisible() override {return false;}
    /*virtual*/ bool getMinimized() override {return false;}
    /*virtual*/ bool getMaximized() override {return false;}
    /*virtual*/ bool maximize() override {return false;}
    /*virtual*/ void minimize() override {}
    /*virtual*/ void restore() override {}
    // TODO: LLWindow::getFullscreen() is (intentionally?) NOT virtual.
    // Apparently the coder of LLWindowHeadless didn't realize that. Is it a
    // mistake to shadow the base-class method with an LLWindowHeadless
    // override when called on the subclass, yet call the base-class method
    // when indirecting through a polymorphic pointer or reference?
    bool getFullscreen() {return false;}
    /*virtual*/ bool getPosition(LLCoordScreen *position) override {return false;}
    /*virtual*/ bool getSize(LLCoordScreen *size) override {return false;}
    /*virtual*/ bool getSize(LLCoordWindow *size) override {return false;}
    /*virtual*/ bool setPosition(LLCoordScreen position) override {return false;}
    /*virtual*/ bool setSizeImpl(LLCoordScreen size) override {return false;}
    /*virtual*/ bool setSizeImpl(LLCoordWindow size) override {return false;}
    /*virtual*/ bool switchContext(bool fullscreen, const LLCoordScreen &size, bool enable_vsync, const LLCoordScreen * const posp = NULL) override {return false;}
    void* createSharedContext() override  { return nullptr; }
    void makeContextCurrent(void*) override  {}
    void destroySharedContext(void*) override  {}
    /*virtual*/ void toggleVSync(bool enable_vsync) override { }
    /*virtual*/ bool setCursorPosition(LLCoordWindow position) override {return false;}
    /*virtual*/ bool getCursorPosition(LLCoordWindow *position) override {return false;}
#if LL_WINDOWS
    /*virtual*/ bool getCursorDelta(LLCoordCommon* delta) override { return false; }
#endif
    /*virtual*/ void showCursor() override {}
    /*virtual*/ void hideCursor() override {}
    /*virtual*/ void showCursorFromMouseMove() override {}
    /*virtual*/ void hideCursorUntilMouseMove() override {}
    /*virtual*/ bool isCursorHidden() override {return false;}
    /*virtual*/ void updateCursor() override {}
    //virtual ECursorType getCursor() override { return mCurrentCursor; }
    /*virtual*/ void captureMouse() override {}
    /*virtual*/ void releaseMouse() override {}
    /*virtual*/ void setMouseClipping( bool b ) override {}
    /*virtual*/ bool isClipboardTextAvailable() override {return false; }
    /*virtual*/ bool pasteTextFromClipboard(LLWString &dst) override {return false; }
    /*virtual*/ bool copyTextToClipboard(const LLWString &src) override {return false; }
    /*virtual*/ void flashIcon(F32 seconds) override {}
    /*virtual*/ F32 getGamma() override {return 1.0f; }
    /*virtual*/ bool setGamma(const F32 gamma) override {return false; } // Set the gamma
    /*virtual*/ void setFSAASamples(const U32 fsaa_samples) override { }
    /*virtual*/ U32 getFSAASamples() override { return 0; }
    /*virtual*/ bool restoreGamma() override {return false; }   // Restore original gamma table (before updating gamma)
    //virtual ESwapMethod getSwapMethod() override { return mSwapMethod; }
    /*virtual*/ void gatherInput() override {}
    /*virtual*/ void delayInputProcessing() override {}
    /*virtual*/ void swapBuffers() override;


    // handy coordinate space conversion routines
    /*virtual*/ bool convertCoords(LLCoordScreen from, LLCoordWindow *to) override { return false; }
    /*virtual*/ bool convertCoords(LLCoordWindow from, LLCoordScreen *to) override { return false; }
    /*virtual*/ bool convertCoords(LLCoordWindow from, LLCoordGL *to) override { return false; }
    /*virtual*/ bool convertCoords(LLCoordGL from, LLCoordWindow *to) override { return false; }
    /*virtual*/ bool convertCoords(LLCoordScreen from, LLCoordGL *to) override { return false; }
    /*virtual*/ bool convertCoords(LLCoordGL from, LLCoordScreen *to) override { return false; }

    /*virtual*/ LLWindowResolution* getSupportedResolutions(S32 &num_resolutions) override { return NULL; }
    /*virtual*/ F32 getNativeAspectRatio() override { return 1.0f; }
    /*virtual*/ F32 getPixelAspectRatio() override { return 1.0f; }
    /*virtual*/ void setNativeAspectRatio(F32 ratio) override {}

    /*virtual*/ void *getPlatformWindow() override { return 0; }
    /*virtual*/ void bringToFront() override {}

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

