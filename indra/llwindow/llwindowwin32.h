/**
 * @file llwindowwin32.h
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

#ifndef LL_LLWINDOWWIN32_H
#define LL_LLWINDOWWIN32_H

// Limit Windows API to small and manageable set.
#include "llwin32headers.h"

#include "llwindow.h"
#include "llwindowcallbacks.h"
#include "lldragdropwin32.h"
#include "llthread.h"
#include "llthreadsafequeue.h"
#include "llmutex.h"
#include "workqueue.h"

// Hack for async host by name
#define LL_WM_HOST_RESOLVED      (WM_APP + 1)
typedef void (*LLW32MsgCallback)(const MSG &msg);

class LLWindowWin32 : public LLWindow
{
public:
    void show() override;
    void hide() override;
    void close() override;
    bool getVisible() const override;
    bool getMinimized() const override;
    bool getMaximized() const override;
    bool maximize() override;
    void minimize() override;
    void restore() override;
    bool getPosition(LLCoordScreen *position) const override;
    bool getSize(LLCoordScreen *size) const override;
    bool getSize(LLCoordWindow *size) const override;
    bool setPosition(LLCoordScreen position) override;
    bool setSizeImpl(LLCoordScreen size) override;
    bool setSizeImpl(LLCoordWindow size) override;
    bool switchContext(bool fullscreen, const LLCoordScreen &size, bool enable_vsync, const LLCoordScreen * const posp = NULL) override;
    void setTitle(const std::string title) override;
    void* createSharedContext() override;
    void makeContextCurrent(void* context) override;
    void destroySharedContext(void* context) override;
    void toggleVSync(bool enable_vsync) override;
    bool setCursorPosition(LLCoordWindow position) override;
    bool getCursorPosition(LLCoordWindow *position) override;
    bool getCursorDelta(LLCoordCommon* delta) const override;
    bool isWarpMouse() const override { return !mAbsoluteCursorPosition; };
    void showCursor() override;
    void hideCursor() override;
    void showCursorFromMouseMove() override;
    void hideCursorUntilMouseMove() override;
    bool isCursorHidden() override;
    void updateCursor() override;
    ECursorType getCursor() const override;
    void captureMouse() override;
    void releaseMouse() override;
    void setMouseClipping( bool b ) override;
    bool isClipboardTextAvailable() override;
    bool pasteTextFromClipboard(LLWString &dst) override;
    bool copyTextToClipboard(const LLWString &src) override;
    void flashIcon(F32 seconds) override;
    F32 getGamma() const override;
    bool setGamma(const F32 gamma) override; // Set the gamma
    void setFSAASamples(const U32 fsaa_samples) override;
    U32 getFSAASamples() const override;
    bool restoreGamma() override;            // Restore original gamma table (before updating gamma)
    void gatherInput(bool app_has_focus) override;
    void delayInputProcessing() override;
    void swapBuffers() override;
    void restoreGLContext() {};

    // handy coordinate space conversion routines
    bool convertCoords(LLCoordScreen from, LLCoordWindow *to) const override;
    bool convertCoords(LLCoordWindow from, LLCoordScreen *to) const override;
    bool convertCoords(LLCoordWindow from, LLCoordGL *to) const override;
    bool convertCoords(LLCoordGL from, LLCoordWindow *to) const override;
    bool convertCoords(LLCoordScreen from, LLCoordGL *to) const override;
    bool convertCoords(LLCoordGL from, LLCoordScreen *to) const override;

    LLWindowResolution* getSupportedResolutions(S32 &num_resolutions) override;
    F32 getNativeAspectRatio() override;
    F32 getPixelAspectRatio() override;
    void setNativeAspectRatio(F32 ratio) override { mOverrideAspectRatio = ratio; }

    bool dialogColorPicker(F32 *r, F32 *g, F32 *b ) override;

    void *getPlatformWindow() override;
    void bringToFront() override;
    void focusClient() override;

    void allowLanguageTextInput(LLPreeditor *preeditor, bool b) override;
    void setLanguageTextInput( const LLCoordGL & pos ) override;
    void updateLanguageTextInputArea() override;
    void interruptLanguageTextInput() override;
    void spawnWebBrowser(const std::string& escaped_url, bool async) override;

    void openFolder(const std::string &path) override;

    F32 getSystemUISize() override;

    LLWindowCallbacks::DragNDropResult completeDragNDropRequest( const LLCoordGL gl_coord, const MASK mask, LLWindowCallbacks::DragNDropAction action, const std::string url );

    static std::vector<std::string> getDisplaysResolutionList();
    static std::vector<std::string> getDynamicFallbackFontList();
    static void setDPIAwareness();

    void* getDirectInput8() override;
    bool getInputDevices(U32 device_type_filter,
                                     std::function<bool(std::string&, LLSD&, void*)> osx_callback,
                                     void* win_callback,
                                     void* userdata) override;

    U32 getRawWParam() const { return mRawWParam; }

protected:
    LLWindowWin32(LLWindowCallbacks* callbacks,
        const std::string& title, const std::string& name, int x, int y, int width, int height, U32 flags,
        bool fullscreen, bool clearBg, bool enable_vsync, bool use_gl,
        bool ignore_pixel_depth, U32 fsaa_samples, U32 max_cores, F32 max_gl_version);
    ~LLWindowWin32();

    void    initCursors();
    HCURSOR loadColorCursor(LPCTSTR name);
    bool    isValid();
    void    moveWindow(const LLCoordScreen& position,const LLCoordScreen& size);
    LLSD    getNativeKeyData() const override;

    // Changes display resolution. Returns true if successful
    bool    setDisplayResolution(S32 width, S32 height, S32 refresh);

    // Go back to last fullscreen display resolution.
    bool    setFullscreenResolution();

    // Restore the display resolution to its value before we ran the app.
    bool    resetDisplayResolution();

    bool    shouldPostQuit() { return mPostQuit; }

    void    fillCandidateForm(const LLCoordGL& caret, const LLRect& bounds, CANDIDATEFORM *form);
    void    fillCharPosition(const LLCoordGL& caret, const LLRect& bounds, const LLRect& control, IMECHARPOSITION *char_position);
    void    fillCompositionLogfont(LOGFONT *logfont);
    U32     fillReconvertString(const LLWString &text, S32 focus, S32 focus_length, RECONVERTSTRING *reconvert_string);
    void    handleStartCompositionMessage();
    void    handleCompositionMessage(U32 indexes);
    bool    handleImeRequests(WPARAM request, LPARAM param, LRESULT *result);

protected:
    //
    // Platform specific methods
    //

    bool    getClientRectInScreenSpace(RECT* rectp) const;

    static LRESULT CALLBACK mainWindowProc(HWND h_wnd, UINT u_msg, WPARAM w_param, LPARAM l_param);


    //
    // Platform specific variables
    //
    WCHAR       *mWindowTitle;
    WCHAR       *mWindowClassName;

    HWND        mWindowHandle = 0;  // window handle
    HGLRC       mhRC = 0;           // OpenGL rendering context
    HDC         mhDC = 0;           // Windows Device context handle
    HINSTANCE   mhInstance;     // handle to application instance
    RECT        mOldMouseClip;  // Screen rect to which the mouse cursor was globally constrained before we changed it in clipMouse()
    WPARAM      mLastSizeWParam;
    F32         mOverrideAspectRatio;
    F32         mNativeAspectRatio;

    HCURSOR     mCursor[ UI_CURSOR_COUNT ];  // Array of all mouse cursors
    LLCoordWindow mCursorPosition;  // mouse cursor position, should only be mutated on main thread
    bool        mAbsoluteCursorPosition; // true if last position was received in absolute coordinates.
    LLMutex mRawMouseMutex;
    RAWINPUTDEVICE mRawMouse;
    LLCoordWindow mLastCursorPosition; // mouse cursor position from previous frame
    LLCoordCommon mRawMouseDelta; // raw mouse delta according to window thread
    LLCoordCommon mMouseFrameDelta; // how much the mouse moved between the last two calls to gatherInput

    MASK        mMouseMask;

    static bool sIsClassRegistered; // has the window class been registered?

    F32         mCurrentGamma;
    U32         mFSAASamples;
    U32         mMaxCores; // for debugging only -- maximum number of CPU cores to use, or 0 for no limit
    F32         mMaxGLVersion; // maximum OpenGL version to attempt to use (clamps to 3.2 - 4.6)
    WORD        mPrevGammaRamp[3][256];
    WORD        mCurrentGammaRamp[3][256];
    bool        mCustomGammaSet;

    LPWSTR      mIconResource;
    bool        mInputProcessingPaused;

    // The following variables are for Language Text Input control.
    // They are all static, since one context is shared by all LLWindowWin32
    // instances.
    static bool     sLanguageTextInputAllowed;
    static bool     sWinIMEOpened;
    static HKL      sWinInputLocale;
    static DWORD    sWinIMEConversionMode;
    static DWORD    sWinIMESentenceMode;
    static LLCoordWindow sWinIMEWindowPosition;
    LLCoordGL       mLanguageTextInputPointGL;
    LLRect          mLanguageTextInputAreaGL;

    LLPreeditor     *mPreeditor;

    LLDragDropWin32* mDragDrop;

    U32             mKeyCharCode;
    U32             mKeyScanCode;
    U32             mKeyVirtualKey;
    U32             mRawMsg;
    U32             mRawWParam;
    U32             mRawLParam;

    bool            mMouseVanish;

    // Cached values of GetWindowRect and GetClientRect to be used by app thread
    void updateWindowRect();
    RECT mRect;
    RECT mClientRect;

    struct LLWindowWin32Thread;
    LLWindowWin32Thread* mWindowThread = nullptr;
    LLThreadSafeQueue<std::function<void()>> mFunctionQueue;
    LLThreadSafeQueue<std::function<void()>> mMouseQueue;
    void post(const std::function<void()>& func);
    void postMouseButtonEvent(const std::function<void()>& func);
    void recreateWindow(RECT window_rect, DWORD dw_ex_style, DWORD dw_style);
    void kickWindowThread(HWND windowHandle=0);

    friend class LLWindowManager;
};

class LLSplashScreenWin32 : public LLSplashScreen
{
public:
    LLSplashScreenWin32();
    virtual ~LLSplashScreenWin32();

    void showImpl() override;
    void updateImpl(const std::string& mesg) override;
    void hideImpl() override;

#if LL_WINDOWS
    static LRESULT CALLBACK windowProc(HWND h_wnd, UINT u_msg,
        WPARAM w_param, LPARAM l_param);
#endif

private:
#if LL_WINDOWS
    HWND mWindow;
#endif
};

extern LLW32MsgCallback gAsyncMsgCallback;
extern LPWSTR gIconResource;

S32 OSMessageBoxWin32(const std::string& text, const std::string& caption, U32 type);

#endif //LL_LLWINDOWWIN32_H
