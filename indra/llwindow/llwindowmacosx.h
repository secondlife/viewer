/**
 * @file llwindowmacosx.h
 * @brief Mac implementation of LLWindow class
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

#ifndef LL_LLWINDOWMACOSX_H
#define LL_LLWINDOWMACOSX_H

#include "llwindow.h"
#include "llwindowcallbacks.h"
#include "llwindowmacosx-objc.h"

#include "lltimer.h"

#include <ApplicationServices/ApplicationServices.h>
#include <OpenGL/OpenGL.h>

// AssertMacros.h does bad things.
#include "fix_macros.h"
#undef verify
#undef require

class LLWindowMacOSX : public LLWindow
{
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
    BOOL switchContext(BOOL fullscreen, const LLCoordScreen &size, BOOL enable_vsync, const LLCoordScreen * const posp = NULL) override;
    BOOL setCursorPosition(LLCoordWindow position) override;
    BOOL getCursorPosition(LLCoordWindow *position) override;
    void showCursor() override;
    void hideCursor() override;
    void showCursorFromMouseMove() override;
    void hideCursorUntilMouseMove() override;
    BOOL isCursorHidden() override;
    void updateCursor() override;
    ECursorType getCursor() const override;
    void captureMouse() override;
    void releaseMouse() override;
    void setMouseClipping( BOOL b ) override;
    BOOL isClipboardTextAvailable() override;
    BOOL pasteTextFromClipboard(LLWString &dst) override;
    BOOL copyTextToClipboard(const LLWString & src) override;
    void flashIcon(F32 seconds) override;
    F32 getGamma() override;
    BOOL setGamma(const F32 gamma) override; // Set the gamma
    U32 getFSAASamples() override;
    void setFSAASamples(const U32 fsaa_samples) override;
    BOOL restoreGamma() override;           // Restore original gamma table (before updating gamma)
    ESwapMethod getSwapMethod() override { return mSwapMethod; }
    void gatherInput() override;
    void delayInputProcessing() override {};
    void swapBuffers() override;

    // handy coordinate space conversion routines
    BOOL convertCoords(LLCoordScreen from, LLCoordWindow *to) override;
    BOOL convertCoords(LLCoordWindow from, LLCoordScreen *to) override;
    BOOL convertCoords(LLCoordWindow from, LLCoordGL *to) override;
    BOOL convertCoords(LLCoordGL from, LLCoordWindow *to) override;
    BOOL convertCoords(LLCoordScreen from, LLCoordGL *to) override;
    BOOL convertCoords(LLCoordGL from, LLCoordScreen *to) override;

    LLWindowResolution* getSupportedResolutions(S32 &num_resolutions) override;
    F32 getNativeAspectRatio() override;
    F32 getPixelAspectRatio() override;
    void setNativeAspectRatio(F32 ratio) override { mOverrideAspectRatio = ratio; }

    // query VRAM usage
    /*virtual*/ U32 getAvailableVRAMMegabytes() override;

    void beforeDialog() override;
    void afterDialog() override;

    BOOL dialogColorPicker(F32 *r, F32 *g, F32 *b) override;

    void *getPlatformWindow() override;
    void bringToFront() override {};

    void allowLanguageTextInput(LLPreeditor *preeditor, BOOL b) override;
    void interruptLanguageTextInput() override;
    void spawnWebBrowser(const std::string& escaped_url, bool async) override;
    F32 getSystemUISize() override;

    bool getInputDevices(U32 device_type_filter,
                         std::function<bool(std::string&, LLSD&, void*)> osx_callback,
                         void* win_callback,
                         void* userdata) override;

    static std::vector<std::string> getDisplaysResolutionList();

    static std::vector<std::string> getDynamicFallbackFontList();

    // Provide native key event data
    LLSD getNativeKeyData() override;

    void* getWindow() { return mWindow; }
    LLWindowCallbacks* getCallbacks() { return mCallbacks; }
    LLPreeditor* getPreeditor() { return mPreeditor; }

    void updateMouseDeltas(float* deltas);
    void getMouseDeltas(float* delta);

    void handleDragNDrop(std::string url, LLWindowCallbacks::DragNDropAction action);

    bool allowsLanguageInput() { return mLanguageTextInputAllowed; }

    //create a new GL context that shares a namespace with this Window's main GL context and make it current on the current thread
    // returns a pointer to be handed back to destroySharedConext/makeContextCurrent
    void* createSharedContext() override;
    //make the given context current on the current thread
    void makeContextCurrent(void* context) override;
    //destroy the given context that was retrieved by createSharedContext()
    //Must be called on the same thread that called createSharedContext()
    void destroySharedContext(void* context) override;

    void toggleVSync(bool enable_vsync) override;

protected:
    LLWindowMacOSX(LLWindowCallbacks* callbacks,
        const std::string& title, const std::string& name, int x, int y, int width, int height, U32 flags,
        BOOL fullscreen, BOOL clearBg, BOOL enable_vsync, BOOL use_gl,
        BOOL ignore_pixel_depth,
        U32 fsaa_samples);
        ~LLWindowMacOSX();

    void    initCursors();
    BOOL    isValid() override;
    void    moveWindow(const LLCoordScreen& position,const LLCoordScreen& size);


    // Changes display resolution. Returns true if successful
    BOOL    setDisplayResolution(S32 width, S32 height, S32 bits, S32 refresh);

    // Go back to last fullscreen display resolution.
    BOOL    setFullscreenResolution();

    // Restore the display resolution to its value before we ran the app.
    BOOL    resetDisplayResolution();

    BOOL    shouldPostQuit() { return mPostQuit; }

    //Satisfy MAINT-3135 and MAINT-3288 with a flag.
    /*virtual */ void setOldResize(bool oldresize) override {setResizeMode(oldresize, mGLView); }

private:
    void restoreGLContext();

protected:
    //
    // Platform specific methods
    //

    // create or re-create the GL context/window.  Called from the constructor and switchContext().
    BOOL createContext(int x, int y, int width, int height, int bits, BOOL fullscreen, BOOL enable_vsync);
    void destroyContext();
    void setupFailure(const std::string& text, const std::string& caption, U32 type);
    void adjustCursorDecouple(bool warpingMouse = false);
    static MASK modifiersToMask(S16 modifiers);

#if LL_OS_DRAGDROP_ENABLED

    //static OSErr dragTrackingHandler(DragTrackingMessage message, WindowRef theWindow, void * handlerRefCon, DragRef theDrag);
    //static OSErr dragReceiveHandler(WindowRef theWindow, void * handlerRefCon,    DragRef theDrag);


#endif // LL_OS_DRAGDROP_ENABLED

    //
    // Platform specific variables
    //

    // Use generic pointers here.  This lets us do some funky Obj-C interop using Obj-C objects without having to worry about any compilation problems that may arise.
    NSWindowRef         mWindow;
    GLViewRef           mGLView;
    CGLContextObj       mContext;
    CGLPixelFormatObj   mPixelFormat;
    CGDirectDisplayID   mDisplay;

    LLRect      mOldMouseClip;  // Screen rect to which the mouse cursor was globally constrained before we changed it in clipMouse()
    std::string mWindowTitle;
    double      mOriginalAspectRatio;
    BOOL        mSimulatedRightClick;
    U32         mLastModifiers;
    BOOL        mHandsOffEvents;    // When true, temporarially disable CarbonEvent processing.
    // Used to allow event processing when putting up dialogs in fullscreen mode.
    BOOL        mCursorDecoupled;
    S32         mCursorLastEventDeltaX;
    S32         mCursorLastEventDeltaY;
    BOOL        mCursorIgnoreNextDelta;
    BOOL        mNeedsResize;       // Constructor figured out the window is too big, it needs a resize.
    LLCoordScreen   mNeedsResizeSize;
    F32         mOverrideAspectRatio;
    BOOL        mMaximized;
    BOOL        mMinimized;
    U32         mFSAASamples;
    BOOL        mForceRebuild;

    S32 mDragOverrideCursor;

    // Input method management through Text Service Manager.
    BOOL        mLanguageTextInputAllowed;
    LLPreeditor*    mPreeditor;

public:
    static BOOL sUseMultGL;

    friend class LLWindowManager;

};


class LLSplashScreenMacOSX : public LLSplashScreen
{
public:
    LLSplashScreenMacOSX();
    virtual ~LLSplashScreenMacOSX();

    void showImpl();
    void updateImpl(const std::string& mesg);
    void hideImpl();

private:
    WindowRef   mWindow;
};

S32 OSMessageBoxMacOSX(const std::string& text, const std::string& caption, U32 type);

void load_url_external(const char* url);

#endif //LL_LLWINDOWMACOSX_H
