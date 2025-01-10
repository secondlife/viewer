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

#ifndef LL_LLWINDOWSDL2_H
#define LL_LLWINDOWSDL2_H

// Simple Directmedia Layer (http://libsdl.org/) implementation of LLWindow class

#if LL_LINUX
#include "llwindow.h"
#include "lltimer.h"

#include "SDL2/SDL.h"
#include "SDL2/SDL_endian.h"

// get X11-specific headers for use in low-level stuff like copy-and-paste support
#include "SDL2/SDL_syswm.h"

// AssertMacros.h does bad things.
#include "fix_macros.h"
#undef verify
#undef require


class LLWindowSDL : public LLWindow {
public:
    void show() override;
    void hide() override;
    void restore() override;

    void close() override;

    bool getVisible() const override;

    bool getMinimized() const override;

    bool getMaximized() const override;

    bool maximize() override;
    void minimize() override;

    bool getPosition(LLCoordScreen *position) const override;

    bool getSize(LLCoordScreen *size) const override;

    bool getSize(LLCoordWindow *size) const override;

    bool setPosition(LLCoordScreen position) override;

    bool setSizeImpl(LLCoordScreen size) override;

    bool setSizeImpl(LLCoordWindow size) override;

    bool switchContext(bool fullscreen, const LLCoordScreen &size, bool enable_vsync,
                                   const LLCoordScreen *const posp = NULL) override;

    bool setCursorPosition(LLCoordWindow position) override;

    bool getCursorPosition(LLCoordWindow *position) override;

    bool isWarpMouse() const override { return true; }
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

    F32 getGamma() const override;
    bool setGamma(const F32 gamma) override; // Set the gamma
    bool restoreGamma() override;            // Restore original gamma table (before updating gamma)

    U32 getFSAASamples() const override;
    void setFSAASamples(const U32 samples) override;

    void processMiscNativeEvents() override;

    void gatherInput(bool app_has_focus) override;

    void swapBuffers() override;

    void restoreGLContext() {};

    void delayInputProcessing()  override {};

    // handy coordinate space conversion routines
    bool convertCoords(LLCoordScreen from, LLCoordWindow *to) const override;

    bool convertCoords(LLCoordWindow from, LLCoordScreen *to) const override;

    bool convertCoords(LLCoordWindow from, LLCoordGL *to) const override;

    bool convertCoords(LLCoordGL from, LLCoordWindow *to) const override;

    bool convertCoords(LLCoordScreen from, LLCoordGL *to) const override;

    bool convertCoords(LLCoordGL from, LLCoordScreen *to) const override;

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

    void (*Lock_Display)(void) = nullptr;
    void (*Unlock_Display)(void) = nullptr;

    static Window get_SDL_XWindowID(void);
    static Display *get_SDL_Display(void);

    void *createSharedContext() override;
    void makeContextCurrent(void *context) override;
    void destroySharedContext(void *context) override;
    void toggleVSync(bool enable_vsync) override;

protected:
    LLWindowSDL(LLWindowCallbacks *callbacks,
                const std::string &title, const std::string& name, int x, int y, int width, int height, U32 flags,
                bool fullscreen, bool clearBg, bool enable_vsync, bool use_gl,
                bool ignore_pixel_depth, U32 fsaa_samples);

    ~LLWindowSDL();

    bool isValid() override;

    LLSD getNativeKeyData() const override;

    void initCursors();
    void quitCursors();

    void moveWindow(const LLCoordScreen &position, const LLCoordScreen &size);

    // Changes display resolution. Returns true if successful
    bool setDisplayResolution(S32 width, S32 height, S32 bits, S32 refresh);

    // Go back to last fullscreen display resolution.
    bool setFullscreenResolution();

    bool shouldPostQuit() { return mPostQuit; }

protected:
    //
    // Platform specific methods
    //

    // create or re-create the GL context/window.  Called from the constructor and switchContext().
    bool createContext(int x, int y, int width, int height, int bits, bool fullscreen, bool enable_vsync);
    void destroyContext();

    void setupFailure(const std::string &text, const std::string &caption, U32 type);

    U32 SDLCheckGrabbyKeys(U32 keysym, bool gain);

    bool SDLReallyCaptureInput(bool capture);

    //
    // Platform specific variables
    //
    U32 mGrabbyKeyFlags = 0;

    SDL_Window *mWindow = nullptr;
    SDL_Surface *mSurface;
    SDL_GLContext mContext;
    SDL_Cursor *mSDLCursors[UI_CURSOR_COUNT];

    std::string mWindowTitle;
    double mOriginalAspectRatio = 1.0f;
    bool mNeedsResize = false;        // Constructor figured out the window is too big, it needs a resize.
    LLCoordScreen mNeedsResizeSize;
    F32 mOverrideAspectRatio = 0.0f;
    F32 mGamma = 0.0f;
    U32 mFSAASamples = 0;

    int mHaveInputFocus = -1; /* 0=no, 1=yes, else unknown */

    friend class LLWindowManager;

private:
    bool mFlashing = false;
    LLTimer mFlashTimer;
    U32 mKeyVirtualKey = 0;
    U32 mKeyModifiers = KMOD_NONE;

    enum EServerProtocol{ X11, Wayland, Unknown };
    EServerProtocol mServerProtocol = Unknown;

    struct {
        Window mXWindowID = None;
        Display *mDisplay = nullptr;
    } mX11Data;

    // Wayland
    struct {
        wl_surface *mSurface = nullptr;
        uint64_t mLastFrameEvent = 0;
    } mWaylandData;

    bool mWaylandLoaded = false;

    bool isWaylandWindowNotDrawing() const;

    void setupWaylandFrameCallback();
    static void waylandFrameDoneCB(void *data, struct wl_callback *cb, uint32_t time);
    //

private:
    void tryFindFullscreenSize(int &aWidth, int &aHeight);
};

class LLSplashScreenSDL : public LLSplashScreen
{
public:
    LLSplashScreenSDL();
    virtual ~LLSplashScreenSDL();

    void showImpl();
    void updateImpl(const std::string& mesg);
    void hideImpl();
};

S32 OSMessageBoxSDL(const std::string& text, const std::string& caption, U32 type);

#endif //LL_LINUX
#endif //LL_LLWINDOWSDL_H
