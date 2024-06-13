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

#ifndef LL_LLWINDOWSDL2_H
#define LL_LLWINDOWSDL2_H

// Simple Directmedia Layer (http://libsdl.org/) implementation of LLWindow class

#include "llwindow.h"
#include "lltimer.h"

#include "SDL2/SDL.h"
#include "SDL2/SDL_endian.h"

#if LL_X11
// get X11-specific headers for use in low-level stuff like copy-and-paste support
#include "SDL2/SDL_syswm.h"
#endif

// AssertMacros.h does bad things.
#include "fix_macros.h"
#undef verify
#undef require


class LLWindowSDL : public LLWindow {
public:
    void show() override;

    void hide() override;

    void close() override;

    BOOL getVisible() override;

    BOOL getMinimized() override;

    BOOL getMaximized() override;

    BOOL maximize() override;

    void minimize() override;

    void restore() override;

    BOOL getFullscreen();

    BOOL getPosition(LLCoordScreen *position) override;

    BOOL getSize(LLCoordScreen *size) override;

    BOOL getSize(LLCoordWindow *size) override;

    BOOL setPosition(LLCoordScreen position) override;

    BOOL setSizeImpl(LLCoordScreen size) override;

    BOOL setSizeImpl(LLCoordWindow size) override;

    BOOL switchContext(BOOL fullscreen, const LLCoordScreen &size, BOOL disable_vsync,
                                   const LLCoordScreen *const posp = NULL) override;

    BOOL setCursorPosition(LLCoordWindow position) override;

    BOOL getCursorPosition(LLCoordWindow *position) override;

    void showCursor() override;

    void hideCursor() override;

    void showCursorFromMouseMove() override;

    void hideCursorUntilMouseMove() override;

    BOOL isCursorHidden() override;

    void updateCursor() override;

    void captureMouse() override;

    void releaseMouse() override;

    void setMouseClipping(BOOL b) override;

       void setMinSize(U32 min_width, U32 min_height, bool enforce_immediately = true) override;

    BOOL isClipboardTextAvailable() override;

    BOOL pasteTextFromClipboard(LLWString &dst) override;

    BOOL copyTextToClipboard(const LLWString &src) override;

    BOOL isPrimaryTextAvailable() override;

    BOOL pasteTextFromPrimary(LLWString &dst) override;

    BOOL copyTextToPrimary(const LLWString &src) override;

    void flashIcon(F32 seconds) override;

    F32 getGamma() override;

    BOOL setGamma(const F32 gamma) override; // Set the gamma
    U32 getFSAASamples() override;

    void setFSAASamples(const U32 samples) override;

    BOOL restoreGamma() override;            // Restore original gamma table (before updating gamma)
    ESwapMethod getSwapMethod()  override { return mSwapMethod; }

    void processMiscNativeEvents() override;

    void gatherInput() override;

    void swapBuffers() override;

    void restoreGLContext() {};

    void delayInputProcessing()  override {};

    // handy coordinate space conversion routines
    BOOL convertCoords(LLCoordScreen from, LLCoordWindow *to) override;

    BOOL convertCoords(LLCoordWindow from, LLCoordScreen *to) override;

    BOOL convertCoords(LLCoordWindow from, LLCoordGL *to) override;

    BOOL convertCoords(LLCoordGL from, LLCoordWindow *to) override;

    BOOL convertCoords(LLCoordScreen from, LLCoordGL *to) override;

    BOOL convertCoords(LLCoordGL from, LLCoordScreen *to) override;

    LLWindowResolution *getSupportedResolutions(S32 &num_resolutions) override;

    F32 getNativeAspectRatio() override;

    F32 getPixelAspectRatio() override;

    void setNativeAspectRatio(F32 ratio)  override { mOverrideAspectRatio = ratio; }

    void beforeDialog() override;

    void afterDialog() override;

    BOOL dialogColorPicker(F32 *r, F32 *g, F32 *b) override;

    void *getPlatformWindow() override;

    void bringToFront() override;
    
    void setLanguageTextInput(const LLCoordGL& pos) override;

    void spawnWebBrowser(const std::string &escaped_url, bool async) override;

    void openFile(const std::string &file_name);

    void setTitle(const std::string title) override;

    static std::vector<std::string> getDynamicFallbackFontList();

    // Not great that these are public, but they have to be accessible
    // by non-class code and it's better than making them global.
#if LL_X11
    Window mSDL_XWindowID;
    Display *mSDL_Display;
#endif

    void (*Lock_Display)(void);

    void (*Unlock_Display)(void);

#if LL_X11

    static Window get_SDL_XWindowID(void);

    static Display *get_SDL_Display(void);

#endif // LL_X11

    void *createSharedContext() override;

    void makeContextCurrent(void *context) override;

    void destroySharedContext(void *context) override;

    void toggleVSync(bool enable_vsync) override;

    U32 getAvailableVRAMMegabytes() override;

protected:
    LLWindowSDL(LLWindowCallbacks *callbacks,
                const std::string &title, int x, int y, int width, int height, U32 flags,
                BOOL fullscreen, BOOL clearBg, BOOL disable_vsync, BOOL use_gl,
                BOOL ignore_pixel_depth, U32 fsaa_samples);

    ~LLWindowSDL();

    BOOL isValid() override;

    LLSD getNativeKeyData() override;

    void initCursors();

    void quitCursors();

    void moveWindow(const LLCoordScreen &position, const LLCoordScreen &size);

    // Changes display resolution. Returns true if successful
    BOOL setDisplayResolution(S32 width, S32 height, S32 bits, S32 refresh);

    // Go back to last fullscreen display resolution.
    BOOL setFullscreenResolution();

    BOOL shouldPostQuit() { return mPostQuit; }

protected:
    //
    // Platform specific methods
    //

    // create or re-create the GL context/window.  Called from the constructor and switchContext().
    BOOL createContext(int x, int y, int width, int height, int bits, BOOL fullscreen, BOOL disable_vsync);

    void destroyContext();

    void setupFailure(const std::string &text, const std::string &caption, U32 type);

    void fixWindowSize(void);

    U32 SDLCheckGrabbyKeys(U32 keysym, BOOL gain);

    BOOL SDLReallyCaptureInput(BOOL capture);

    //
    // Platform specific variables
    //
    U32 mGrabbyKeyFlags;
    int mReallyCapturedCount;

    SDL_Window *mWindow;
    SDL_Surface *mSurface;
    SDL_GLContext mContext;
    SDL_Cursor *mSDLCursors[UI_CURSOR_COUNT];

    std::string mWindowTitle;
    double mOriginalAspectRatio;
    BOOL mNeedsResize;        // Constructor figured out the window is too big, it needs a resize.
    LLCoordScreen mNeedsResizeSize;
    F32 mOverrideAspectRatio;
    F32 mGamma;
    U32 mFSAASamples;

    int mSDLFlags;

    int mHaveInputFocus; /* 0=no, 1=yes, else unknown */
    int mIsMinimized; /* 0=no, 1=yes, else unknown */

    friend class LLWindowManager;

private:
#if LL_X11

    void x11_set_urgent(BOOL urgent);

    BOOL mFlashing;
    LLTimer mFlashTimer;
#endif //LL_X11

    U32 mKeyVirtualKey;
    U32 mKeyModifiers;
    std::string mInputType;

public:
#if LL_X11

    static Display *getSDLDisplay();

    LLWString const &getPrimaryText() const { return mPrimaryClipboard; }

    LLWString const &getSecondaryText() const { return mSecondaryClipboard; }

    void clearPrimaryText() { mPrimaryClipboard.clear(); }

    void clearSecondaryText() { mSecondaryClipboard.clear(); }

private:
    void tryFindFullscreenSize(int &aWidth, int &aHeight);

    void initialiseX11Clipboard();

    bool getSelectionText(Atom selection, LLWString &text);

    bool getSelectionText(Atom selection, Atom type, LLWString &text);

    bool setSelectionText(Atom selection, const LLWString &text);

#endif
    LLWString mPrimaryClipboard;
    LLWString mSecondaryClipboard;
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

#endif //LL_LLWINDOWSDL_H
