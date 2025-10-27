/**
 * @file llwindowsdl.h
 * @brief SDL implementation of LLWindow class
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

#ifndef LL_LLWINDOWSDL_H
#define LL_LLWINDOWSDL_H

// Simple Directmedia Layer (http://libsdl.org/) implementation of LLWindow class

#include "llwindow.h"
#include "lltimer.h"

#include "SDL3/SDL.h"
#include "SDL3/SDL_endian.h"

class LLPreeditor;

class LLWindowSDL final : public LLWindow
{
public:
    void show() override;
    void hide() override;
    void restore() override;

    void close() override;

    bool getVisible() override;

    bool getMinimized() override;

    bool getMaximized() override;

    bool maximize() override;
    void minimize() override;

    bool getPosition(LLCoordScreen *position) override;

    bool getSize(LLCoordScreen *size) override;
    bool getSize(LLCoordWindow *size) override;

    bool setPosition(LLCoordScreen position) override;

    bool setSizeImpl(LLCoordScreen size) override;
    bool setSizeImpl(LLCoordWindow size) override;

    bool switchContext(bool fullscreen, const LLCoordScreen &size, bool enable_vsync,
                                   const LLCoordScreen *const posp = NULL) override;

    bool setCursorPosition(LLCoordWindow position) override;

    bool getCursorPosition(LLCoordWindow *position) override;
    bool isWrapMouse() const override { return true; }
    void showCursor() override;
    void hideCursor() override;
    bool isCursorHidden() override;

    void showCursorFromMouseMove() override;
    void hideCursorUntilMouseMove() override;

    void updateCursor() override;

    void captureMouse() override;
    void releaseMouse() override;

    void setMouseClipping(bool b) override;

    void setMinSize(U32 min_width, U32 min_height, bool enforce_immediately = true) override;

    bool isClipboardTextAvailable() override;
    bool pasteTextFromClipboard(LLWString &dst) override;
    bool copyTextToClipboard(const LLWString &src) override;

    bool isPrimaryTextAvailable() override;
    bool pasteTextFromPrimary(LLWString &dst) override;
    bool copyTextToPrimary(const LLWString &src) override;

    void flashIcon(F32 seconds) override;
    void maybeStopFlashIcon();

    F32 getGamma() override;
    bool setGamma(const F32 gamma) override; // Set the gamma
    bool restoreGamma() override;            // Restore original gamma table (before updating gamma)

    U32 getFSAASamples() override;
    void setFSAASamples(const U32 samples) override;

    void processMiscNativeEvents() override;

    void gatherInput() override;

    SDL_AppResult handleEvent(const SDL_Event& event);
    static SDL_AppResult handleEvents(const SDL_Event& event);

    void swapBuffers() override;

    void delayInputProcessing()  override {};

    // handy coordinate space conversion routines
    bool convertCoords(LLCoordScreen from, LLCoordWindow *to) override;
    bool convertCoords(LLCoordWindow from, LLCoordScreen *to) override;
    bool convertCoords(LLCoordWindow from, LLCoordGL *to) override;
    bool convertCoords(LLCoordGL from, LLCoordWindow *to) override;
    bool convertCoords(LLCoordScreen from, LLCoordGL *to) override;
    bool convertCoords(LLCoordGL from, LLCoordScreen *to) override;

    LLWindowResolution *getSupportedResolutions(S32 &num_resolutions) override;

    F32 getNativeAspectRatio() override;
    F32 getPixelAspectRatio() override;
    void setNativeAspectRatio(F32 ratio)  override { mOverrideAspectRatio = ratio; }

    void beforeDialog() override;
    void afterDialog() override;

    bool dialogColorPicker(F32 *r, F32 *g, F32 *b) override;

    void *getPlatformWindow() override;

    void bringToFront() override;

    void setLanguageTextInput(const LLCoordGL& pos) override;

    void spawnWebBrowser(const std::string &escaped_url, bool async) override;

    void setTitle(const std::string title) override;

    static std::vector<std::string> getDynamicFallbackFontList();

    void *createSharedContext() override;
    void makeContextCurrent(void *context) override;
    void destroySharedContext(void *context) override;
    void toggleVSync(bool enable_vsync) override;

    F32 getSystemUISize() override;

    static std::vector<std::string> getDisplaysResolutionList();

#if LL_DARWIN
    static U64 getVramSize();
    static void setUseMultGL(bool use_mult_gl);

    static bool sUseMultGL;
#endif

protected:
    LLWindowSDL(LLWindowCallbacks *callbacks,
                const std::string &title, const std::string& name, int x, int y, int width, int height, U32 flags,
                bool fullscreen, bool clearBg, bool enable_vsync, bool use_gl,
                bool ignore_pixel_depth, U32 fsaa_samples);

    ~LLWindowSDL();

    bool isValid() override;

    LLSD getNativeKeyData() override;

    void initCursors();
    void quitCursors();

protected:
    //
    // Platform specific methods
    //

    // create or re-create the GL context/window.  Called from the constructor and switchContext().
    bool createContext(int x, int y, int width, int height, int bits, bool fullscreen, bool enable_vsync);
    void destroyContext();

    void setupFailure(const std::string &text, const std::string &caption, U32 type);

    bool SDLReallyCaptureInput(bool capture);
    U32 SDLCheckGrabbyKeys(U32 keysym, bool gain);

    //
    // Platform specific variables
    //
    U32 mGrabbyKeyFlags = 0;
    S32 mReallyCapturedCount = 0;
    SDL_Window *mWindow = nullptr;
    SDL_GLContext mContext;
    SDL_Cursor *mSDLCursors[UI_CURSOR_COUNT];

    std::string mWindowTitle;
    F32 mNativeAspectRatio = 0.0f;
    F32 mOverrideAspectRatio = 0.0f;
    F32 mGamma = 0.0f;
    U32 mFSAASamples = 0;

    friend class LLWindowManager;

private:
    bool mFlashing = false;
    LLTimer mFlashTimer;
    U32 mKeyVirtualKey = 0;
    U32 mKeyModifiers = SDL_KMOD_NONE;

    void tryFindFullscreenSize(int &aWidth, int &aHeight);
};

class LLSplashScreenSDL : public LLSplashScreen
{
public:
    LLSplashScreenSDL();
    virtual ~LLSplashScreenSDL();

    void showImpl() override;
    void updateImpl(const std::string& mesg) override;
    void hideImpl() override;
};

S32 OSMessageBoxSDL(const std::string& text, const std::string& caption, U32 type);

#endif //LL_LLWINDOWSDL_H
