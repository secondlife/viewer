/**
 * @file llwindowwin32.cpp
 * @brief Platform-dependent implementation of llwindow
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

#include "linden_common.h"

#if LL_WINDOWS && !LL_MESA_HEADLESS

#include "llwindowwin32.h"

// LLWindow library includes
#include "llgamecontrol.h"
#include "llkeyboardwin32.h"
#include "lldragdropwin32.h"
#include "llpreeditor.h"
#include "llwindowcallbacks.h"

// Linden library includes
#include "llerror.h"
#include "llexception.h"
#include "llfasttimer.h"
#include "llgl.h"
#include "llstring.h"
#include "lldir.h"
#include "llsdutil.h"
#include "llglslshader.h"
#include "llthreadsafequeue.h"
#include "stringize.h"
#include "llframetimer.h"

// System includes
#include <commdlg.h>
#include <WinUser.h>
#include <mapi.h>
#include <process.h>    // for _spawn
#include <shellapi.h>
#include <fstream>
#include <Imm.h>
#include <iomanip>
#include <future>
#include <sstream>
#include <utility>                  // std::pair

#include <d3d9.h>
#include <dxgi1_4.h>
#include <timeapi.h>

// Require DirectInput version 8
#define DIRECTINPUT_VERSION 0x0800

#include <dinput.h>
#include <Dbt.h.>
#include <InitGuid.h> // needed for llurlentry test to build on some systems
#pragma comment(lib, "dxguid.lib") // needed for llurlentry test to build on some systems
#pragma comment(lib, "dinput8")

const S32   MAX_MESSAGE_PER_UPDATE = 20;
const S32   BITS_PER_PIXEL = 32;
const S32   MAX_NUM_RESOLUTIONS = 32;
const F32   ICON_FLASH_TIME = 0.5f;

#ifndef WM_DPICHANGED
#define WM_DPICHANGED 0x02E0
#endif

#ifndef USER_DEFAULT_SCREEN_DPI
#define USER_DEFAULT_SCREEN_DPI 96 // Win7
#endif

// Claim a couple unused GetMessage() message IDs
const UINT WM_DUMMY_(WM_USER + 0x0017);
const UINT WM_POST_FUNCTION_(WM_USER + 0x0018);

extern bool gDebugWindowProc;

static std::thread::id sWindowThreadId;
static std::thread::id sMainThreadId;

#if 1 // flip to zero to enable assertions for functions being called from wrong thread
#define ASSERT_MAIN_THREAD()
#define ASSERT_WINDOW_THREAD()
#else
#define ASSERT_MAIN_THREAD() llassert(LLThread::currentID() == sMainThreadId)
#define ASSERT_WINDOW_THREAD() llassert(LLThread::currentID() == sWindowThreadId)
#endif


LPWSTR gIconResource = IDI_APPLICATION;
LPDIRECTINPUT8 gDirectInput8;

LLW32MsgCallback gAsyncMsgCallback = NULL;

#ifndef DPI_ENUMS_DECLARED

typedef enum PROCESS_DPI_AWARENESS {
    PROCESS_DPI_UNAWARE = 0,
    PROCESS_SYSTEM_DPI_AWARE = 1,
    PROCESS_PER_MONITOR_DPI_AWARE = 2
} PROCESS_DPI_AWARENESS;

typedef enum MONITOR_DPI_TYPE {
    MDT_EFFECTIVE_DPI = 0,
    MDT_ANGULAR_DPI = 1,
    MDT_RAW_DPI = 2,
    MDT_DEFAULT = MDT_EFFECTIVE_DPI
} MONITOR_DPI_TYPE;

#endif

typedef HRESULT(STDAPICALLTYPE *SetProcessDpiAwarenessType)(_In_ PROCESS_DPI_AWARENESS value);

typedef HRESULT(STDAPICALLTYPE *GetProcessDpiAwarenessType)(
    _In_ HANDLE hprocess,
    _Out_ PROCESS_DPI_AWARENESS *value);

typedef HRESULT(STDAPICALLTYPE *GetDpiForMonitorType)(
    _In_ HMONITOR hmonitor,
    _In_ MONITOR_DPI_TYPE dpiType,
    _Out_ UINT *dpiX,
    _Out_ UINT *dpiY);

//
// LLWindowWin32
//

void show_window_creation_error(const std::string& title)
{
    LL_WARNS("Window") << title << LL_ENDL;
}

HGLRC SafeCreateContext(HDC &hdc)
{
    __try
    {
        return wglCreateContext(hdc);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return NULL;
    }
}

GLuint SafeChoosePixelFormat(HDC &hdc, const PIXELFORMATDESCRIPTOR *ppfd)
{
    return LL::seh::catcher([hdc, ppfd]{ return ChoosePixelFormat(hdc, ppfd); });
}

//static
bool LLWindowWin32::sIsClassRegistered = false;

bool    LLWindowWin32::sLanguageTextInputAllowed = true;
bool    LLWindowWin32::sWinIMEOpened = false;
HKL     LLWindowWin32::sWinInputLocale = 0;
DWORD   LLWindowWin32::sWinIMEConversionMode = IME_CMODE_NATIVE;
DWORD   LLWindowWin32::sWinIMESentenceMode = IME_SMODE_AUTOMATIC;
LLCoordWindow LLWindowWin32::sWinIMEWindowPosition(-1,-1);

static HWND sWindowHandleForMessageBox = NULL;

// The following class LLWinImm delegates Windows IMM APIs.
// It was originally introduced to support US Windows XP, on which we needed
// to dynamically load IMM32.DLL and use GetProcAddress to resolve its entry
// points. Now that that's moot, we retain this wrapper only for hooks for
// metrics.

class LLWinImm
{
public:
    static bool     isAvailable() { return true; }

public:
    // Wrappers for IMM API.
    static bool     isIME(HKL hkl);
    static HIMC     getContext(HWND hwnd);
    static bool     releaseContext(HWND hwnd, HIMC himc);
    static bool     getOpenStatus(HIMC himc);
    static bool     setOpenStatus(HIMC himc, bool status);
    static bool     getConversionStatus(HIMC himc, LPDWORD conversion, LPDWORD sentence);
    static bool     setConversionStatus(HIMC himc, DWORD conversion, DWORD sentence);
    static bool     getCompositionWindow(HIMC himc, LPCOMPOSITIONFORM form);
    static bool     setCompositionWindow(HIMC himc, LPCOMPOSITIONFORM form);
    static LONG     getCompositionString(HIMC himc, DWORD index, LPVOID data, DWORD length);
    static bool     setCompositionString(HIMC himc, DWORD index, LPVOID pComp, DWORD compLength, LPVOID pRead, DWORD readLength);
    static bool     setCompositionFont(HIMC himc, LPLOGFONTW logfont);
    static bool     setCandidateWindow(HIMC himc, LPCANDIDATEFORM candidate_form);
    static bool     notifyIME(HIMC himc, DWORD action, DWORD index, DWORD value);
};

// static
bool    LLWinImm::isIME(HKL hkl)
{
    return ImmIsIME(hkl);
}

// static
HIMC        LLWinImm::getContext(HWND hwnd)
{
    return ImmGetContext(hwnd);
}

//static
bool        LLWinImm::releaseContext(HWND hwnd, HIMC himc)
{
    return ImmReleaseContext(hwnd, himc);
}

// static
bool        LLWinImm::getOpenStatus(HIMC himc)
{
    return ImmGetOpenStatus(himc);
}

// static
bool        LLWinImm::setOpenStatus(HIMC himc, bool status)
{
    return ImmSetOpenStatus(himc, status);
}

// static
bool        LLWinImm::getConversionStatus(HIMC himc, LPDWORD conversion, LPDWORD sentence)
{
    return ImmGetConversionStatus(himc, conversion, sentence);
}

// static
bool        LLWinImm::setConversionStatus(HIMC himc, DWORD conversion, DWORD sentence)
{
    return ImmSetConversionStatus(himc, conversion, sentence);
}

// static
bool        LLWinImm::getCompositionWindow(HIMC himc, LPCOMPOSITIONFORM form)
{
    return ImmGetCompositionWindow(himc, form);
}

// static
bool        LLWinImm::setCompositionWindow(HIMC himc, LPCOMPOSITIONFORM form)
{
    return ImmSetCompositionWindow(himc, form);
}


// static
LONG        LLWinImm::getCompositionString(HIMC himc, DWORD index, LPVOID data, DWORD length)
{
    return ImmGetCompositionString(himc, index, data, length);
}


// static
bool        LLWinImm::setCompositionString(HIMC himc, DWORD index, LPVOID pComp, DWORD compLength, LPVOID pRead, DWORD readLength)
{
    return ImmSetCompositionString(himc, index, pComp, compLength, pRead, readLength);
}

// static
bool        LLWinImm::setCompositionFont(HIMC himc, LPLOGFONTW pFont)
{
    return ImmSetCompositionFont(himc, pFont);
}

// static
bool        LLWinImm::setCandidateWindow(HIMC himc, LPCANDIDATEFORM form)
{
    return ImmSetCandidateWindow(himc, form);
}

// static
bool        LLWinImm::notifyIME(HIMC himc, DWORD action, DWORD index, DWORD value)
{
    return ImmNotifyIME(himc, action, index, value);
}



class LLMonitorInfo
{
public:

    std::vector<std::string> getResolutionsList() { return mResList; }

    LLMonitorInfo()
    {
        EnumDisplayMonitors(0, 0, MonitorEnum, (LPARAM)this);
    }

private:

    static BOOL CALLBACK MonitorEnum(HMONITOR hMon, HDC hdc, LPRECT lprcMonitor, LPARAM pData)
    {
        int monitor_width = lprcMonitor->right - lprcMonitor->left;
        int monitor_height = lprcMonitor->bottom - lprcMonitor->top;

        std::ostringstream sstream;
        sstream << monitor_width << "x" << monitor_height;;
        std::string res = sstream.str();

        LLMonitorInfo* pThis = reinterpret_cast<LLMonitorInfo*>(pData);
        pThis->mResList.push_back(res);

        return TRUE;
    }

    std::vector<std::string> mResList;
};

static LLMonitorInfo sMonitorInfo;


// Thread that owns the Window Handle
// This whole struct is private to LLWindowWin32, which needs to mess with its
// members, which is why it's a struct rather than a class. In effect, we make
// the containing class a friend.
struct LLWindowWin32::LLWindowWin32Thread : public LL::ThreadPool
{
    static const int MAX_QUEUE_SIZE = 2048;

    LLThreadSafeQueue<MSG> mMessageQueue;

    LLWindowWin32Thread();

    void run() override;
    void close() override;

    // closes queue, wakes thread, waits until thread closes
    void wakeAndDestroy();

    void glReady()
    {
        mGLReady = true;
    }

    // Use DXGI to check memory (because WMI doesn't report more than 4Gb)
    void checkDXMem();

    /// called by main thread to post work to this window thread
    template <typename CALLABLE>
    void post(CALLABLE&& func)
    {
        // Ignore bool return. Shutdown timing is tricky: the main thread can
        // end up trying to post a cursor position after having closed the
        // WorkQueue.
        getQueue().post(std::forward<CALLABLE>(func));
    }

    /**
     * Like post(), Post() is a way of conveying a single work item to this
     * thread. Its virtue is that it will definitely be executed "soon" rather
     * than potentially waiting for the next frame: it uses PostMessage() to
     * break us out of the window thread's blocked GetMessage() call. It's
     * more expensive, though, not only from the Windows API latency of
     * PostMessage() and GetMessage(), but also because it involves heap
     * allocation and release.
     *
     * Require HWND from caller, even though we store an HWND locally.
     * Otherwise, if our mWindowHandle was accessed from both threads, we'd
     * have to protect it with a mutex.
     */
    template <typename CALLABLE>
    void Post(HWND windowHandle, CALLABLE&& func)
    {
        // Move func to the heap. If we knew FuncType could fit into LPARAM,
        // we could simply instantiate FuncType and pass it by value. But
        // since we don't, we must put that on the heap as well as the
        // internal heap allocation it likely requires to store func.
        auto ptr = new FuncType(std::move(func));
        WPARAM wparam{ 0xF1C };
        LL_DEBUGS("Window") << "PostMessage(" << std::hex << windowHandle
                            << ", " << WM_POST_FUNCTION_
                            << ", " << wparam << std::dec << LL_ENDL;
        PostMessage(windowHandle, WM_POST_FUNCTION_, wparam, LPARAM(ptr));
    }

    using FuncType = std::function<void()>;
    // call GetMessage() and pull enqueue messages for later processing
    HWND mWindowHandleThrd = NULL;
    HDC mhDCThrd = 0;

    // *HACK: Attempt to prevent startup crashes by deferring memory accounting
    // until after some graphics setup. See SL-20177. -Cosmic,2023-09-18
    bool mGLReady = false;
    bool mGotGLBuffer = false;
    bool mShuttingDown = false;
};


LLWindowWin32::LLWindowWin32(LLWindowCallbacks* callbacks,
                             const std::string& title, const std::string& name, S32 x, S32 y, S32 width,
                             S32 height, U32 flags,
                             bool fullscreen, bool clearBg,
                             bool enable_vsync, bool use_gl,
                             bool ignore_pixel_depth,
                             U32 fsaa_samples,
                             U32 max_cores,
                             F32 max_gl_version)
    :
    LLWindow(callbacks, fullscreen, flags),
    mAbsoluteCursorPosition(false),
    mMaxGLVersion(max_gl_version),
    mMaxCores(max_cores)
{
    sMainThreadId = LLThread::currentID();
    mWindowThread = new LLWindowWin32Thread();

    //MAINT-516 -- force a load of opengl32.dll just in case windows went sideways
    LoadLibrary(L"opengl32.dll");


    if (mMaxCores != 0)
    {
        HANDLE hProcess = GetCurrentProcess();
        mMaxCores = llmin(mMaxCores, (U32) 64);
        DWORD_PTR mask = 0;

        for (U32 i = 0; i < mMaxCores; ++i)
        {
            mask |= ((DWORD_PTR) 1) << i;
        }

        SetProcessAffinityMask(hProcess, mask);
    }

#if 0 // this is probably a bad idea, but keep it in your back pocket if you see what looks like
        // process deprioritization during profiles
    // force high thread priority
    HANDLE hProcess = GetCurrentProcess();

    if (hProcess)
    {
        int priority = GetPriorityClass(hProcess);
        if (priority < REALTIME_PRIORITY_CLASS)
        {
            if (SetPriorityClass(hProcess, REALTIME_PRIORITY_CLASS))
            {
                LL_INFOS() << "Set process priority to REALTIME_PRIORITY_CLASS" << LL_ENDL;
            }
            else
            {
                LL_INFOS() << "Failed to set process priority: " << std::hex << GetLastError() << LL_ENDL;
            }
        }
    }
#endif

#if 0  // this is also probably a bad idea, but keep it in your back pocket for getting main thread off of background thread cores (see also LLThread::threadRun)
    HANDLE hThread = GetCurrentThread();

    SYSTEM_INFO sysInfo;

    GetSystemInfo(&sysInfo);
    U32 core_count = sysInfo.dwNumberOfProcessors;

    if (max_cores != 0)
    {
        core_count = llmin(core_count, max_cores);
    }

    if (hThread)
    {
        int priority = GetThreadPriority(hThread);

        if (priority < THREAD_PRIORITY_TIME_CRITICAL)
        {
            if (SetThreadPriority(hThread, THREAD_PRIORITY_TIME_CRITICAL))
            {
                LL_INFOS() << "Set thread priority to THREAD_PRIORITY_TIME_CRITICAL" << LL_ENDL;
            }
            else
            {
                LL_INFOS() << "Failed to set thread priority: " << std::hex << GetLastError() << LL_ENDL;
            }

            // tell main thread to prefer core 0
            SetThreadIdealProcessor(hThread, 0);
        }
    }
#endif


    mFSAASamples = fsaa_samples;
    mIconResource = gIconResource;
    mOverrideAspectRatio = 0.f;
    mNativeAspectRatio = 0.f;
    mInputProcessingPaused = false;
    mPreeditor = NULL;
    mKeyCharCode = 0;
    mKeyScanCode = 0;
    mKeyVirtualKey = 0;
    mhDC = NULL;
    mhRC = NULL;
    memset(mCurrentGammaRamp, 0, sizeof(mCurrentGammaRamp));
    memset(mPrevGammaRamp, 0, sizeof(mPrevGammaRamp));
    mCustomGammaSet = false;
    mWindowHandle = NULL;

    mRect = {0, 0, 0, 0};
    mClientRect = {0, 0, 0, 0};

    if (!SystemParametersInfo(SPI_GETMOUSEVANISH, 0, &mMouseVanish, 0))
    {
        mMouseVanish = true;
    }

    // Initialize the keyboard
    gKeyboard = new LLKeyboardWin32();
    gKeyboard->setCallbacks(callbacks);

    // Initialize the Drag and Drop functionality
    mDragDrop = new LLDragDropWin32;

    // Initialize (boot strap) the Language text input management,
    // based on the system's (user's) default settings.
    allowLanguageTextInput(mPreeditor, false);

    WNDCLASS        wc;
    RECT            window_rect;

    // Set the window title
    if (title.empty())
    {
        mWindowTitle = new WCHAR[50];
        wsprintf(mWindowTitle, L"OpenGL Window");
    }
    else
    {
        mWindowTitle = new WCHAR[256]; // Assume title length < 255 chars.
        mbstowcs(mWindowTitle, title.c_str(), 255);
        mWindowTitle[255] = 0;
    }

    // Set the window class name
    if (name.empty())
    {
        mWindowClassName = new WCHAR[50];
        wsprintf(mWindowClassName, L"OpenGL Window");
    }
    else
    {
        mWindowClassName = new WCHAR[256]; // Assume title length < 255 chars.
        mbstowcs(mWindowClassName, name.c_str(), 255);
        mWindowClassName[255] = 0;
    }


    // We're not clipping yet
    SetRect( &mOldMouseClip, 0, 0, 0, 0 );

    // Make an instance of our window then define the window class
    mhInstance = GetModuleHandle(NULL);

    // Init Direct Input - needed for joystick / Spacemouse

    LPDIRECTINPUT8 di8_interface;
    HRESULT status = DirectInput8Create(
        mhInstance, // HINSTANCE hinst,
        DIRECTINPUT_VERSION, // DWORD dwVersion,
        IID_IDirectInput8, // REFIID riidltf,
        (LPVOID*)&di8_interface, // LPVOID * ppvOut,
        NULL                     // LPUNKNOWN punkOuter
        );
    if (status == DI_OK)
    {
        gDirectInput8 = di8_interface;
    }

    mSwapMethod = SWAP_METHOD_UNDEFINED;

    // No WPARAM yet.
    mLastSizeWParam = 0;

    // Windows GDI rects don't include rightmost pixel
    window_rect.left = (long) 0;
    window_rect.right = (long) width;
    window_rect.top = (long) 0;
    window_rect.bottom = (long) height;

    // Grab screen size to sanitize the window
    S32 window_border_y = GetSystemMetrics(SM_CYBORDER);
    S32 virtual_screen_x = GetSystemMetrics(SM_XVIRTUALSCREEN);
    S32 virtual_screen_y = GetSystemMetrics(SM_YVIRTUALSCREEN);
    S32 virtual_screen_width = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    S32 virtual_screen_height = GetSystemMetrics(SM_CYVIRTUALSCREEN);

    if (x < virtual_screen_x) x = virtual_screen_x;
    if (y < virtual_screen_y - window_border_y) y = virtual_screen_y - window_border_y;

    if (x + width > virtual_screen_x + virtual_screen_width) x = virtual_screen_x + virtual_screen_width - width;
    if (y + height > virtual_screen_y + virtual_screen_height) y = virtual_screen_y + virtual_screen_height - height;

    if (!sIsClassRegistered)
    {
        // Force redraw when resized and create a private device context

        // Makes double click messages.
        wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC | CS_DBLCLKS;

        // Set message handler function
        wc.lpfnWndProc = (WNDPROC) mainWindowProc;

        // unused
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;

        wc.hInstance = mhInstance;
        wc.hIcon = LoadIcon(mhInstance, mIconResource);

        // We will set the cursor ourselves
        wc.hCursor = NULL;

        // background color is not used
        if (clearBg)
        {
            wc.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);
        }
        else
        {
            wc.hbrBackground = (HBRUSH) NULL;
        }

        // we don't use windows menus
        wc.lpszMenuName = NULL;

        wc.lpszClassName = mWindowClassName;

        if (!RegisterClass(&wc))
        {
            OSMessageBox(mCallbacks->translateString("MBRegClassFailed"),
                mCallbacks->translateString("MBError"), OSMB_OK);
            return;
        }
        sIsClassRegistered = true;
    }

    //-----------------------------------------------------------------------
    // Get the current refresh rate
    //-----------------------------------------------------------------------

    DEVMODE dev_mode;
    ::ZeroMemory(&dev_mode, sizeof(DEVMODE));
    dev_mode.dmSize = sizeof(DEVMODE);
    DWORD current_refresh;
    if (EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &dev_mode))
    {
        current_refresh = dev_mode.dmDisplayFrequency;
        mNativeAspectRatio = ((F32)dev_mode.dmPelsWidth) / ((F32)dev_mode.dmPelsHeight);
    }
    else
    {
        current_refresh = 60;
    }
    mRefreshRate = current_refresh;
    //-----------------------------------------------------------------------
    // Drop resolution and go fullscreen
    // use a display mode with our desired size and depth, with a refresh
    // rate as close at possible to the users' default
    //-----------------------------------------------------------------------
    if (mFullscreen)
    {
        bool success = false;
        DWORD closest_refresh = 0;

        for (S32 mode_num = 0;; mode_num++)
        {
            if (!EnumDisplaySettings(NULL, mode_num, &dev_mode))
            {
                break;
            }

            if (dev_mode.dmPelsWidth == width &&
                dev_mode.dmPelsHeight == height)
            {
                success = true;
                if ((dev_mode.dmDisplayFrequency - current_refresh)
                    < (closest_refresh - current_refresh))
                {
                    closest_refresh = dev_mode.dmDisplayFrequency;
                }
            }
        }

        if (closest_refresh == 0)
        {
            LL_WARNS("Window") << "Couldn't find display mode " << width << " by " << height << " at " << BITS_PER_PIXEL << " bits per pixel" << LL_ENDL;
            //success = false;

            if (!EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &dev_mode))
            {
                success = false;
            }
            else
            {
                if (dev_mode.dmBitsPerPel == BITS_PER_PIXEL)
                {
                    LL_WARNS("Window") << "Current BBP is OK falling back to that" << LL_ENDL;
                    window_rect.right=width=dev_mode.dmPelsWidth;
                    window_rect.bottom=height=dev_mode.dmPelsHeight;
                    success = true;
                }
                else
                {
                    LL_WARNS("Window") << "Current BBP is BAD" << LL_ENDL;
                    success = false;
                }
            }
        }

        // If we found a good resolution, use it.
        if (success)
        {
            success = setDisplayResolution(width, height, closest_refresh);
        }

        // Keep a copy of the actual current device mode in case we minimize
        // and change the screen resolution.   JC
        EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &dev_mode);

        // If it failed, we don't want to run fullscreen
        if (success)
        {
            mFullscreen = true;
            mFullscreenWidth   = dev_mode.dmPelsWidth;
            mFullscreenHeight  = dev_mode.dmPelsHeight;
            mFullscreenRefresh = dev_mode.dmDisplayFrequency;

            LL_INFOS("Window") << "Running at " << dev_mode.dmPelsWidth
                << "x"   << dev_mode.dmPelsHeight
                << "x"   << dev_mode.dmBitsPerPel
                << " @ " << dev_mode.dmDisplayFrequency
                << LL_ENDL;
        }
        else
        {
            mFullscreen = false;
            mFullscreenWidth   = -1;
            mFullscreenHeight  = -1;
            mFullscreenRefresh = -1;

            std::map<std::string,std::string> args;
            args["[WIDTH]"] = llformat("%d", width);
            args["[HEIGHT]"] = llformat ("%d", height);
            OSMessageBox(mCallbacks->translateString("MBFullScreenErr", args),
                mCallbacks->translateString("MBError"), OSMB_OK);
        }
    }

    // TODO: add this after resolving _WIN32_WINNT issue
    //  if (!fullscreen)
    //  {
    //      TRACKMOUSEEVENT track_mouse_event;
    //      track_mouse_event.cbSize = sizeof( TRACKMOUSEEVENT );
    //      track_mouse_event.dwFlags = TME_LEAVE;
    //      track_mouse_event.hwndTrack = mWindowHandle;
    //      track_mouse_event.dwHoverTime = HOVER_DEFAULT;
    //      TrackMouseEvent( &track_mouse_event );
    //  }

    // SL-12971 dual GPU display
    DISPLAY_DEVICEA display_device;
    int             display_index = -1;
    DWORD           display_flags = 0; // EDD_GET_DEVICE_INTERFACE_NAME ?
    const size_t    display_bytes = sizeof(display_device);

    do
    {
        if (display_index >= 0)
        {
            // CHAR DeviceName  [ 32] Adapter name
            // CHAR DeviceString[128]
            CHAR text[256];

            size_t name_len = strlen(display_device.DeviceName  );
            size_t desc_len = strlen(display_device.DeviceString);

            const CHAR *name = name_len ? display_device.DeviceName   : "???";
            const CHAR *desc = desc_len ? display_device.DeviceString : "???";

            sprintf(text, "Display Device %d: %s, %s", display_index, name, desc);
            LL_INFOS("Window") << text << LL_ENDL;
        }

        ::ZeroMemory(&display_device,display_bytes);
        display_device.cb = display_bytes;

        display_index++;
    }  while( EnumDisplayDevicesA(NULL, display_index, &display_device, display_flags ));

    LL_INFOS("Window") << "Total Display Devices: " << display_index << LL_ENDL;

    //-----------------------------------------------------------------------
    // Create GL drawing context
    //-----------------------------------------------------------------------
    LLCoordScreen windowPos(x,y);
    LLCoordScreen windowSize(window_rect.right - window_rect.left,
                             window_rect.bottom - window_rect.top);
    if (!switchContext(mFullscreen, windowSize, enable_vsync, &windowPos))
    {
        return;
    }

    //start with arrow cursor
    initCursors();
    setCursor( UI_CURSOR_ARROW );

    mRawMouse.usUsagePage = 0x01;          // HID_USAGE_PAGE_GENERIC
    mRawMouse.usUsage = 0x02;              // HID_USAGE_GENERIC_MOUSE
    mRawMouse.dwFlags = 0;    // adds mouse and also ignores legacy mouse messages
    mRawMouse.hwndTarget = 0;

    RegisterRawInputDevices(&mRawMouse, 1, sizeof(mRawMouse));

    // Initialize (boot strap) the Language text input management,
    // based on the system's (or user's) default settings.
    allowLanguageTextInput(NULL, false);
}


LLWindowWin32::~LLWindowWin32()
{
    if (sWindowHandleForMessageBox == mWindowHandle)
    {
        sWindowHandleForMessageBox = NULL;
    }

    delete mDragDrop;

    delete [] mWindowTitle;
    mWindowTitle = NULL;

    delete [] mSupportedResolutions;
    mSupportedResolutions = NULL;

    delete [] mWindowClassName;
    mWindowClassName = NULL;

    delete mWindowThread;
}

void LLWindowWin32::show()
{
    LL_DEBUGS("Window") << "Setting window to show" << LL_ENDL;
    ShowWindow(mWindowHandle, SW_SHOW);
    SetForegroundWindow(mWindowHandle);
    SetFocus(mWindowHandle);
}

void LLWindowWin32::hide()
{
    setMouseClipping(false);
    ShowWindow(mWindowHandle, SW_HIDE);
}

//virtual
void LLWindowWin32::minimize()
{
    setMouseClipping(false);
    showCursor();
    ShowWindow(mWindowHandle, SW_MINIMIZE);
}

//virtual
void LLWindowWin32::restore()
{
    ShowWindow(mWindowHandle, SW_RESTORE);
    SetForegroundWindow(mWindowHandle);
    SetFocus(mWindowHandle);
}

// See SL-12170
// According to callstack "c0000005 Access violation" happened inside __try block,
// deep in DestroyWindow and crashed viewer, which shouldn't be possible.
// I tried manually causing this exception and it was caught without issues, so
// I'm turning off optimizations for this part to be sure code executes as intended
// (it is a straw, but I have no idea why else __try can get overruled)
#pragma optimize("", off)
bool destroy_window_handler(HWND hWnd)
{
    bool res;
    __try
    {
        res = DestroyWindow(hWnd);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        res = false;
    }
    return res;
}
#pragma optimize("", on)

// close() destroys all OS-specific code associated with a window.
// Usually called from LLWindowManager::destroyWindow()
void LLWindowWin32::close()
{
    LL_DEBUGS("Window") << "Closing LLWindowWin32" << LL_ENDL;
    // Is window is already closed?
    if (!mWindowHandle)
    {
        return;
    }

    mDragDrop->reset();


    // Go back to screen mode written in the registry.
    if (mFullscreen)
    {
        resetDisplayResolution();
    }

    // Make sure cursor is visible and we haven't mangled the clipping state.
    showCursor();
    setMouseClipping(false);
    if (gKeyboard)
    {
        gKeyboard->resetKeys();
    }

    // Clean up remaining GL state
    if (gGLManager.mInited)
    {
        LL_INFOS("Window") << "Cleaning up GL" << LL_ENDL;
        gGLManager.shutdownGL();
    }

    LL_DEBUGS("Window") << "Releasing Context" << LL_ENDL;
    if (mhRC)
    {
        if (!wglMakeCurrent(NULL, NULL))
        {
            LL_WARNS("Window") << "Release of DC and RC failed" << LL_ENDL;
        }

        if (!wglDeleteContext(mhRC))
        {
            LL_WARNS("Window") << "Release of rendering context failed" << LL_ENDL;
        }

        mhRC = NULL;
    }

    // Restore gamma to the system values.
    restoreGamma();

    LL_DEBUGS("Window") << "Destroying Window" << LL_ENDL;

    if (sWindowHandleForMessageBox == mWindowHandle)
    {
        sWindowHandleForMessageBox = NULL;
    }

    mhDC = NULL;
    mWindowHandle = NULL;

    mWindowThread->wakeAndDestroy();
}

bool LLWindowWin32::isValid()
{
    return (mWindowHandle != NULL);
}

bool LLWindowWin32::getVisible() const
{
    return (mWindowHandle && IsWindowVisible(mWindowHandle));
}

bool LLWindowWin32::getMinimized() const
{
    return (mWindowHandle && IsIconic(mWindowHandle));
}

bool LLWindowWin32::getMaximized() const
{
    return (mWindowHandle && IsZoomed(mWindowHandle));
}

bool LLWindowWin32::maximize()
{
    bool success = false;
    if (!mWindowHandle) return success;

    mWindowThread->post([=]
        {
            WINDOWPLACEMENT placement;
            placement.length = sizeof(WINDOWPLACEMENT);

            if (GetWindowPlacement(mWindowHandle, &placement))
            {
                placement.showCmd = SW_MAXIMIZE;
                SetWindowPlacement(mWindowHandle, &placement);
            }
        });

    return true;
}

bool LLWindowWin32::getPosition(LLCoordScreen *position) const
{
    position->mX = mRect.left;
    position->mY = mRect.top;
    return true;
}

bool LLWindowWin32::getSize(LLCoordScreen *size) const
{
    size->mX = mRect.right - mRect.left;
    size->mY = mRect.bottom - mRect.top;
    return true;
}

bool LLWindowWin32::getSize(LLCoordWindow *size) const
{
    size->mX = mClientRect.right - mClientRect.left;
    size->mY = mClientRect.bottom - mClientRect.top;
    return true;
}

bool LLWindowWin32::setPosition(const LLCoordScreen position)
{
    LLCoordScreen size;

    if (!mWindowHandle)
    {
        return false;
    }
    getSize(&size);
    moveWindow(position, size);
    return true;
}

bool LLWindowWin32::setSizeImpl(const LLCoordScreen size)
{
    LLCoordScreen position;

    getPosition(&position);
    if (!mWindowHandle)
    {
        return false;
    }

    mWindowThread->post([=]()
        {
            WINDOWPLACEMENT placement;
            placement.length = sizeof(WINDOWPLACEMENT);

            if (GetWindowPlacement(mWindowHandle, &placement))
            {
                placement.showCmd = SW_RESTORE;
                SetWindowPlacement(mWindowHandle, &placement);
            }
        });

    moveWindow(position, size);
    return true;
}

bool LLWindowWin32::setSizeImpl(const LLCoordWindow size)
{
    RECT window_rect = {0, 0, size.mX, size.mY };
    DWORD dw_ex_style = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
    DWORD dw_style = WS_OVERLAPPEDWINDOW;

    AdjustWindowRectEx(&window_rect, dw_style, FALSE, dw_ex_style);

    return setSizeImpl(LLCoordScreen(window_rect.right - window_rect.left, window_rect.bottom - window_rect.top));
}

// changing fullscreen resolution
bool LLWindowWin32::switchContext(bool fullscreen, const LLCoordScreen& size, bool enable_vsync, const LLCoordScreen* const posp)
{
    //called from main thread
    GLuint  pixel_format;
    DEVMODE dev_mode;
    ::ZeroMemory(&dev_mode, sizeof(DEVMODE));
    dev_mode.dmSize = sizeof(DEVMODE);
    DWORD   current_refresh;
    DWORD   dw_ex_style;
    DWORD   dw_style;
    RECT    window_rect = { 0, 0, 0, 0 };
    S32 width = size.mX;
    S32 height = size.mY;
    bool auto_show = false;

    if (mhRC)
    {
        auto_show = true;
        resetDisplayResolution();
    }

    if (EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &dev_mode))
    {
        current_refresh = dev_mode.dmDisplayFrequency;
    }
    else
    {
        current_refresh = 60;
    }
    mRefreshRate = current_refresh;

    gGLManager.shutdownGL();
    //destroy gl context
    if (mhRC)
    {
        if (!wglMakeCurrent(NULL, NULL))
        {
            LL_WARNS("Window") << "Release of DC and RC failed" << LL_ENDL;
        }

        if (!wglDeleteContext(mhRC))
        {
            LL_WARNS("Window") << "Release of rendering context failed" << LL_ENDL;
        }

        mhRC = NULL;
    }

    if (fullscreen)
    {
        mFullscreen = true;
        bool success = false;
        DWORD closest_refresh = 0;

        for (S32 mode_num = 0;; mode_num++)
        {
            if (!EnumDisplaySettings(NULL, mode_num, &dev_mode))
            {
                break;
            }

            if (dev_mode.dmPelsWidth == width &&
                dev_mode.dmPelsHeight == height &&
                dev_mode.dmBitsPerPel == BITS_PER_PIXEL)
            {
                success = true;
                if ((dev_mode.dmDisplayFrequency - current_refresh)
                    < (closest_refresh - current_refresh))
                {
                    closest_refresh = dev_mode.dmDisplayFrequency;
                }
            }
        }

        if (closest_refresh == 0)
        {
            LL_WARNS("Window") << "Couldn't find display mode " << width << " by " << height << " at " << BITS_PER_PIXEL << " bits per pixel" << LL_ENDL;
            return false;
        }

        // If we found a good resolution, use it.
        if (success)
        {
            success = setDisplayResolution(width, height, closest_refresh);
        }

        // Keep a copy of the actual current device mode in case we minimize
        // and change the screen resolution.   JC
        EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &dev_mode);

        if (success)
        {
            mFullscreen = true;
            mFullscreenWidth = dev_mode.dmPelsWidth;
            mFullscreenHeight = dev_mode.dmPelsHeight;
            mFullscreenRefresh = dev_mode.dmDisplayFrequency;

            LL_INFOS("Window") << "Running at " << dev_mode.dmPelsWidth
                << "x" << dev_mode.dmPelsHeight
                << "x" << dev_mode.dmBitsPerPel
                << " @ " << dev_mode.dmDisplayFrequency
                << LL_ENDL;

            window_rect.left = (long)0;
            window_rect.right = (long)width;            // Windows GDI rects don't include rightmost pixel
            window_rect.top = (long)0;
            window_rect.bottom = (long)height;
            dw_ex_style = WS_EX_APPWINDOW;
            dw_style = WS_POPUP;

            // Move window borders out not to cover window contents.
            // This converts client rect to window rect, i.e. expands it by the window border size.
            AdjustWindowRectEx(&window_rect, dw_style, FALSE, dw_ex_style);
        }
        // If it failed, we don't want to run fullscreen
        else
        {
            mFullscreen = false;
            mFullscreenWidth = -1;
            mFullscreenHeight = -1;
            mFullscreenRefresh = -1;

            LL_INFOS("Window") << "Unable to run fullscreen at " << width << "x" << height << LL_ENDL;
            return false;
        }
    }
    else
    {
        mFullscreen = false;
        window_rect.left = (long)(posp ? posp->mX : 0);
        window_rect.right = (long)width + window_rect.left;         // Windows GDI rects don't include rightmost pixel
        window_rect.top = (long)(posp ? posp->mY : 0);
        window_rect.bottom = (long)height + window_rect.top;
        // Window with an edge
        dw_ex_style = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
        dw_style = WS_OVERLAPPEDWINDOW;
    }


    // don't post quit messages when destroying old windows
    mPostQuit = false;


    // create window
    LL_DEBUGS("Window") << "Creating window with X: " << window_rect.left
        << " Y: " << window_rect.top
        << " Width: " << (window_rect.right - window_rect.left)
        << " Height: " << (window_rect.bottom - window_rect.top)
        << " Fullscreen: " << mFullscreen
        << LL_ENDL;

    recreateWindow(window_rect, dw_ex_style, dw_style);

    if (mWindowHandle)
    {
        LL_INFOS("Window") << "window is created." << LL_ENDL ;
    }
    else
    {
        LL_WARNS("Window") << "Window creation failed, code: " << GetLastError() << LL_ENDL;
    }

    //-----------------------------------------------------------------------
    // Create GL drawing context
    //-----------------------------------------------------------------------
    static PIXELFORMATDESCRIPTOR pfd =
    {
        sizeof(PIXELFORMATDESCRIPTOR),
            1,
            PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
            PFD_TYPE_RGBA,
            BITS_PER_PIXEL,
            0, 0, 0, 0, 0, 0,   // RGB bits and shift, unused
            8,                  // alpha bits
            0,                  // alpha shift
            0,                  // accum bits
            0, 0, 0, 0,         // accum RGBA
            24,                 // depth bits
            8,                  // stencil bits, avi added for stencil test
            0,
            PFD_MAIN_PLANE,
            0,
            0, 0, 0
    };

    if (!mhDC)
    {
        close();
        OSMessageBox(mCallbacks->translateString("MBDevContextErr"),
            mCallbacks->translateString("MBError"), OSMB_OK);
        return false;
    }

    LL_INFOS("Window") << "Device context retrieved." << LL_ENDL ;

    try
    {
        // Looks like ChoosePixelFormat can crash in case of faulty driver
        if (!(pixel_format = SafeChoosePixelFormat(mhDC, &pfd)))
    {
            LL_WARNS("Window") << "ChoosePixelFormat failed, code: " << GetLastError() << LL_ENDL;
            OSMessageBox(mCallbacks->translateString("MBPixelFmtErr"),
                mCallbacks->translateString("MBError"), OSMB_OK);
        close();
            return false;
        }
    }
    catch (...)
    {
        LOG_UNHANDLED_EXCEPTION("ChoosePixelFormat");
        LLError::LLUserWarningMsg::show(mCallbacks->translateString("MBPixelFmtErr"), 8/*LAST_EXEC_GRAPHICS_INIT*/);
        close();
        return false;
    }

    LL_INFOS("Window") << "Pixel format chosen." << LL_ENDL ;

    // Verify what pixel format we actually received.
    if (!DescribePixelFormat(mhDC, pixel_format, sizeof(PIXELFORMATDESCRIPTOR),
        &pfd))
    {
        LLError::LLUserWarningMsg::show(mCallbacks->translateString("MBPixelFmtDescErr"), 8/*LAST_EXEC_GRAPHICS_INIT*/);
        close();
        return false;
    }

    // (EXP-1765) dump pixel data to see if there is a pattern that leads to unreproducible crash
    LL_INFOS("Window") << "--- begin pixel format dump ---" << LL_ENDL ;
    LL_INFOS("Window") << "pixel_format is " << pixel_format << LL_ENDL ;
    LL_INFOS("Window") << "pfd.nSize:            " << pfd.nSize << LL_ENDL ;
    LL_INFOS("Window") << "pfd.nVersion:         " << pfd.nVersion << LL_ENDL ;
    LL_INFOS("Window") << "pfd.dwFlags:          0x" << std::hex << pfd.dwFlags << std::dec << LL_ENDL ;
    LL_INFOS("Window") << "pfd.iPixelType:       " << (int)pfd.iPixelType << LL_ENDL ;
    LL_INFOS("Window") << "pfd.cColorBits:       " << (int)pfd.cColorBits << LL_ENDL ;
    LL_INFOS("Window") << "pfd.cRedBits:         " << (int)pfd.cRedBits << LL_ENDL ;
    LL_INFOS("Window") << "pfd.cRedShift:        " << (int)pfd.cRedShift << LL_ENDL ;
    LL_INFOS("Window") << "pfd.cGreenBits:       " << (int)pfd.cGreenBits << LL_ENDL ;
    LL_INFOS("Window") << "pfd.cGreenShift:      " << (int)pfd.cGreenShift << LL_ENDL ;
    LL_INFOS("Window") << "pfd.cBlueBits:        " << (int)pfd.cBlueBits << LL_ENDL ;
    LL_INFOS("Window") << "pfd.cBlueShift:       " << (int)pfd.cBlueShift << LL_ENDL ;
    LL_INFOS("Window") << "pfd.cAlphaBits:       " << (int)pfd.cAlphaBits << LL_ENDL ;
    LL_INFOS("Window") << "pfd.cAlphaShift:      " << (int)pfd.cAlphaShift << LL_ENDL ;
    LL_INFOS("Window") << "pfd.cAccumBits:       " << (int)pfd.cAccumBits << LL_ENDL ;
    LL_INFOS("Window") << "pfd.cAccumRedBits:    " << (int)pfd.cAccumRedBits << LL_ENDL ;
    LL_INFOS("Window") << "pfd.cAccumGreenBits:  " << (int)pfd.cAccumGreenBits << LL_ENDL ;
    LL_INFOS("Window") << "pfd.cAccumBlueBits:   " << (int)pfd.cAccumBlueBits << LL_ENDL ;
    LL_INFOS("Window") << "pfd.cAccumAlphaBits:  " << (int)pfd.cAccumAlphaBits << LL_ENDL ;
    LL_INFOS("Window") << "pfd.cDepthBits:       " << (int)pfd.cDepthBits << LL_ENDL ;
    LL_INFOS("Window") << "pfd.cStencilBits:     " << (int)pfd.cStencilBits << LL_ENDL ;
    LL_INFOS("Window") << "pfd.cAuxBuffers:      " << (int)pfd.cAuxBuffers << LL_ENDL ;
    LL_INFOS("Window") << "pfd.iLayerType:       " << (int)pfd.iLayerType << LL_ENDL ;
    LL_INFOS("Window") << "pfd.bReserved:        " << (int)pfd.bReserved << LL_ENDL ;
    LL_INFOS("Window") << "pfd.dwLayerMask:      " << pfd.dwLayerMask << LL_ENDL ;
    LL_INFOS("Window") << "pfd.dwVisibleMask:    " << pfd.dwVisibleMask << LL_ENDL ;
    LL_INFOS("Window") << "pfd.dwDamageMask:     " << pfd.dwDamageMask << LL_ENDL ;
    LL_INFOS("Window") << "--- end pixel format dump ---" << LL_ENDL ;

    if (!SetPixelFormat(mhDC, pixel_format, &pfd))
    {
        LLError::LLUserWarningMsg::show(mCallbacks->translateString("MBPixelFmtSetErr"), 8/*LAST_EXEC_GRAPHICS_INIT*/);
        close();
        return false;
    }


    if (!(mhRC = SafeCreateContext(mhDC)))
    {
        LLError::LLUserWarningMsg::show(mCallbacks->translateString("MBGLContextErr"), 8/*LAST_EXEC_GRAPHICS_INIT*/);
        close();
        return false;
    }

    if (!wglMakeCurrent(mhDC, mhRC))
    {
        LLError::LLUserWarningMsg::show(mCallbacks->translateString("MBGLContextActErr"), 8/*LAST_EXEC_GRAPHICS_INIT*/);
        close();
        return false;
    }

    LL_INFOS("Window") << "Drawing context is created." << LL_ENDL ;

    gGLManager.initWGL();

    if (wglChoosePixelFormatARB && wglGetPixelFormatAttribivARB)
    {
        // OK, at this point, use the ARB wglChoosePixelFormatsARB function to see if we
        // can get exactly what we want.
        GLint attrib_list[256];
        S32 cur_attrib = 0;

        attrib_list[cur_attrib++] = WGL_DEPTH_BITS_ARB;
        attrib_list[cur_attrib++] = 24;

        //attrib_list[cur_attrib++] = WGL_STENCIL_BITS_ARB; //stencil buffer is deprecated (performance penalty)
        //attrib_list[cur_attrib++] = 8;

        attrib_list[cur_attrib++] = WGL_DRAW_TO_WINDOW_ARB;
        attrib_list[cur_attrib++] = GL_TRUE;

        attrib_list[cur_attrib++] = WGL_ACCELERATION_ARB;
        attrib_list[cur_attrib++] = WGL_FULL_ACCELERATION_ARB;

        attrib_list[cur_attrib++] = WGL_SUPPORT_OPENGL_ARB;
        attrib_list[cur_attrib++] = GL_TRUE;

        attrib_list[cur_attrib++] = WGL_DOUBLE_BUFFER_ARB;
        attrib_list[cur_attrib++] = GL_TRUE;

        attrib_list[cur_attrib++] = WGL_COLOR_BITS_ARB;
        attrib_list[cur_attrib++] = 24;

        attrib_list[cur_attrib++] = WGL_ALPHA_BITS_ARB;
        attrib_list[cur_attrib++] = 0;

        U32 end_attrib = 0;
        if (mFSAASamples > 0)
        {
            end_attrib = cur_attrib;
            attrib_list[cur_attrib++] = WGL_SAMPLE_BUFFERS_ARB;
            attrib_list[cur_attrib++] = GL_TRUE;

            attrib_list[cur_attrib++] = WGL_SAMPLES_ARB;
            attrib_list[cur_attrib++] = mFSAASamples;
        }

        // End the list
        attrib_list[cur_attrib++] = 0;

        GLint pixel_formats[256];
        U32 num_formats = 0;

        // First we try and get a 32 bit depth pixel format
        BOOL result = wglChoosePixelFormatARB(mhDC, attrib_list, NULL, 256, pixel_formats, &num_formats);

        while(!result && mFSAASamples > 0)
        {
            LL_WARNS() << "FSAASamples: " << mFSAASamples << " not supported." << LL_ENDL ;

            mFSAASamples /= 2 ; //try to decrease sample pixel number until to disable anti-aliasing
            if(mFSAASamples < 2)
            {
                mFSAASamples = 0 ;
            }

            if (mFSAASamples > 0)
            {
                attrib_list[end_attrib + 3] = mFSAASamples;
            }
            else
            {
                cur_attrib = end_attrib ;
                end_attrib = 0 ;
                attrib_list[cur_attrib++] = 0 ; //end
            }
            result = wglChoosePixelFormatARB(mhDC, attrib_list, NULL, 256, pixel_formats, &num_formats);

            if(result)
            {
                LL_WARNS() << "Only support FSAASamples: " << mFSAASamples << LL_ENDL ;
            }
        }

        if (!result)
        {
            LL_WARNS() << "mFSAASamples: " << mFSAASamples << LL_ENDL ;

            close();
            show_window_creation_error("Error after wglChoosePixelFormatARB 32-bit");
            return false;
        }

        if (!num_formats)
        {
            if (end_attrib > 0)
            {
                LL_INFOS("Window") << "No valid pixel format for " << mFSAASamples << "x anti-aliasing." << LL_ENDL;
                attrib_list[end_attrib] = 0;

                BOOL result = wglChoosePixelFormatARB(mhDC, attrib_list, NULL, 256, pixel_formats, &num_formats);
                if (!result)
                {
                    close();
                    show_window_creation_error("Error after wglChoosePixelFormatARB 32-bit no AA");
                    return false;
                }
            }

            if (!num_formats)
            {
                LL_INFOS("Window") << "No 32 bit z-buffer, trying 24 bits instead" << LL_ENDL;
                // Try 24-bit format
                attrib_list[1] = 24;
                BOOL result = wglChoosePixelFormatARB(mhDC, attrib_list, NULL, 256, pixel_formats, &num_formats);
                if (!result)
                {
                    close();
                    show_window_creation_error("Error after wglChoosePixelFormatARB 24-bit");
                    return false;
                }

                if (!num_formats)
                {
                    LL_WARNS("Window") << "Couldn't get 24 bit z-buffer,trying 16 bits instead!" << LL_ENDL;
                    attrib_list[1] = 16;
                    BOOL result = wglChoosePixelFormatARB(mhDC, attrib_list, NULL, 256, pixel_formats, &num_formats);
                    if (!result || !num_formats)
                    {
                        close();
                        show_window_creation_error("Error after wglChoosePixelFormatARB 16-bit");
                        return false;
                    }
                }
            }

            LL_INFOS("Window") << "Choosing pixel formats: " << num_formats << " pixel formats returned" << LL_ENDL;
        }

        LL_INFOS("Window") << "pixel formats done." << LL_ENDL ;

        S32 swap_method = 0;
        S32   cur_format  = 0;
const   S32   max_format  = (S32)num_formats - 1;
        GLint swap_query = WGL_SWAP_METHOD_ARB;

        // SL-14705 Fix name tags showing in front of objects with AMD GPUs.
        // On AMD hardware we need to iterate from the first pixel format to the end.
        // Spec:
        //     https://www.khronos.org/registry/OpenGL/extensions/ARB/WGL_ARB_pixel_format.txt
        while (wglGetPixelFormatAttribivARB(mhDC, pixel_formats[cur_format], 0, 1, &swap_query, &swap_method))
        {
            if (swap_method == WGL_SWAP_UNDEFINED_ARB)
            {
                break;
            }
            else if (cur_format >= max_format)
            {
                cur_format = 0;
                break;
            }

            ++cur_format;
        }

        pixel_format = pixel_formats[cur_format];

        if (mhDC != 0)                                          // Does The Window Have A Device Context?
        {
            wglMakeCurrent(mhDC, 0);                            // Set The Current Active Rendering Context To Zero
            if (mhRC != 0)                                      // Does The Window Have A Rendering Context?
            {
                wglDeleteContext (mhRC);                            // Release The Rendering Context
                mhRC = 0;                                       // Zero The Rendering Context
            }
        }

        // will release and recreate mhDC, mWindowHandle
        recreateWindow(window_rect, dw_ex_style, dw_style);

        RECT rect;
        RECT client_rect;
        //initialize immediately on main thread
        if (GetWindowRect(mWindowHandle, &rect) &&
            GetClientRect(mWindowHandle, &client_rect))
        {
            mRect = rect;
            mClientRect = client_rect;
        };

        if (mWindowHandle)
        {
            LL_INFOS("Window") << "recreate window done." << LL_ENDL ;
        }
        else
        {
            // Note: if value is NULL GetDC retrieves the DC for the entire screen.
            LL_WARNS("Window") << "Window recreation failed, code: " << GetLastError() << LL_ENDL;
        }

        if (!mhDC)
        {
            LLError::LLUserWarningMsg::show(mCallbacks->translateString("MBDevContextErr"), 8/*LAST_EXEC_GRAPHICS_INIT*/);
            close();
            return false;
        }

        if (!SetPixelFormat(mhDC, pixel_format, &pfd))
        {
            LLError::LLUserWarningMsg::show(mCallbacks->translateString("MBPixelFmtSetErr"), 8/*LAST_EXEC_GRAPHICS_INIT*/);
            close();
            return false;
        }

        if (wglGetPixelFormatAttribivARB(mhDC, pixel_format, 0, 1, &swap_query, &swap_method))
        {
            switch (swap_method)
            {
            case WGL_SWAP_EXCHANGE_ARB:
                mSwapMethod = SWAP_METHOD_EXCHANGE;
                LL_DEBUGS("Window") << "Swap Method: Exchange" << LL_ENDL;
                break;
            case WGL_SWAP_COPY_ARB:
                mSwapMethod = SWAP_METHOD_COPY;
                LL_DEBUGS("Window") << "Swap Method: Copy" << LL_ENDL;
                break;
            case WGL_SWAP_UNDEFINED_ARB:
                mSwapMethod = SWAP_METHOD_UNDEFINED;
                LL_DEBUGS("Window") << "Swap Method: Undefined" << LL_ENDL;
                break;
            default:
                mSwapMethod = SWAP_METHOD_UNDEFINED;
                LL_DEBUGS("Window") << "Swap Method: Unknown" << LL_ENDL;
                break;
            }
        }
    }
    else
    {
        LL_WARNS("Window") << "No wgl_ARB_pixel_format extension!" << LL_ENDL;
        // cannot proceed without wgl_ARB_pixel_format extension, shutdown same as any other gGLManager.initGL() failure
        LLError::LLUserWarningMsg::show(mCallbacks->translateString("MBVideoDrvErr"), 8/*LAST_EXEC_GRAPHICS_INIT*/);
        close();
        return false;
    }

    // Verify what pixel format we actually received.
    if (!DescribePixelFormat(mhDC, pixel_format, sizeof(PIXELFORMATDESCRIPTOR),
        &pfd))
    {
        LLError::LLUserWarningMsg::show(mCallbacks->translateString("MBPixelFmtDescErr"), 8/*LAST_EXEC_GRAPHICS_INIT*/);
        close();
        return false;
    }

    LL_INFOS("Window") << "GL buffer: Color Bits " << S32(pfd.cColorBits)
        << " Alpha Bits " << S32(pfd.cAlphaBits)
        << " Depth Bits " << S32(pfd.cDepthBits)
        << LL_ENDL;

    mhRC = 0;
    if (wglCreateContextAttribsARB)
    { //attempt to create a specific versioned context
        mhRC = (HGLRC) createSharedContext();
        if (!mhRC)
        {
            return false;
        }
    }

    if (!wglMakeCurrent(mhDC, mhRC))
    {
        LLError::LLUserWarningMsg::show(mCallbacks->translateString("MBGLContextActErr"), 8/*LAST_EXEC_GRAPHICS_INIT*/);
        close();
        return false;
    }

    if (!gGLManager.initGL())
    {
        LLError::LLUserWarningMsg::show(mCallbacks->translateString("MBVideoDrvErr"), 8/*LAST_EXEC_GRAPHICS_INIT*/);
        close();
        return false;
    }

    // Setup Tracy gpu context
    {
        LL_PROFILER_GPU_CONTEXT;
    }

    // Disable vertical sync for swap
    toggleVSync(enable_vsync);

    SetWindowLongPtr(mWindowHandle, GWLP_USERDATA, (LONG_PTR)this);

    // register this window as handling drag/drop events from the OS
    DragAcceptFiles( mWindowHandle, TRUE );

    mDragDrop->init( mWindowHandle );

    //register joystick timer callback
    SetTimer( mWindowHandle, 0, 1000 / 30, NULL ); // 30 fps timer

    // ok to post quit messages now
    mPostQuit = true;

    // *HACK: Attempt to prevent startup crashes by deferring memory accounting
    // until after some graphics setup. See SL-20177. -Cosmic,2023-09-18
    mWindowThread->post([=]()
    {
        mWindowThread->glReady();
    });

    if (auto_show)
    {
        show();
        glClearColor(0.0f, 0.0f, 0.0f, 0.f);
        glClear(GL_COLOR_BUFFER_BIT);
        swapBuffers();
    }

    return true;
}

void LLWindowWin32::recreateWindow(RECT window_rect, DWORD dw_ex_style, DWORD dw_style)
{
    auto oldWindowHandle = mWindowHandle;
    auto oldDCHandle = mhDC;

    if (sWindowHandleForMessageBox == mWindowHandle)
    {
        sWindowHandleForMessageBox = NULL;
    }

    // zero out mWindowHandle and mhDC before destroying window so window
    // thread falls back to peekmessage
    mWindowHandle = NULL;
    mhDC = NULL;

    std::promise<std::pair<HWND, HDC>> promise;
    // What follows must be done on the window thread.
    auto window_work =
        [this,
         self=mWindowThread,
         oldWindowHandle,
         oldDCHandle,
         // bind CreateWindowEx() parameters by value instead of
         // back-referencing LLWindowWin32 members
         windowClassName=mWindowClassName,
         windowTitle=mWindowTitle,
         hInstance=mhInstance,
         window_rect,
         dw_ex_style,
         dw_style,
         &promise]
        ()
        {
            LL_DEBUGS("Window") << "recreateWindow(): window_work entry" << LL_ENDL;
            self->mWindowHandleThrd = 0;
            self->mhDCThrd = 0;

            if (oldWindowHandle)
            {
                if (oldDCHandle && !ReleaseDC(oldWindowHandle, oldDCHandle))
                {
                    LL_WARNS("Window") << "Failed to ReleaseDC" << LL_ENDL;
                }

                // important to call DestroyWindow() from the window thread
                if (!destroy_window_handler(oldWindowHandle))
                {

                    LL_WARNS("Window") << "Failed to properly close window before recreating it!"
                        << LL_ENDL;
                }
            }

            auto handle = CreateWindowEx(dw_ex_style,
                windowClassName,
                windowTitle,
                WS_CLIPSIBLINGS | WS_CLIPCHILDREN | dw_style,
                window_rect.left,                               // x pos
                window_rect.top,                                // y pos
                window_rect.right - window_rect.left,           // width
                window_rect.bottom - window_rect.top,           // height
                NULL,
                NULL,
                hInstance,
                NULL);

            if (! handle)
            {
                // Failed to create window: clear the variables. This
                // assignment is valid because we're running on mWindowThread.
                self->mWindowHandleThrd = NULL;
                self->mhDCThrd = 0;
            }
            else
            {
                // Update mWindowThread's own mWindowHandle and mhDC.
                self->mWindowHandleThrd = handle;
                self->mhDCThrd = GetDC(handle);
            }

            updateWindowRect();

            // It's important to wake up the future either way.
            promise.set_value(std::make_pair(self->mWindowHandleThrd, self->mhDCThrd));
            LL_DEBUGS("Window") << "recreateWindow(): window_work done" << LL_ENDL;
        };
    // But how we pass window_work to the window thread depends on whether we
    // already have a window handle.
    if (!oldWindowHandle)
    {
        // Pass window_work using the WorkQueue: without an existing window
        // handle, the window thread can't call GetMessage().
        LL_DEBUGS("Window") << "posting window_work to WorkQueue" << LL_ENDL;
        mWindowThread->post(window_work);
    }
    else
    {
        // Pass window_work using PostMessage(). We can still
        // PostMessage(oldHandle) because oldHandle won't be destroyed until
        // the window thread has retrieved and executed window_work.
        LL_DEBUGS("Window") << "posting window_work to message queue" << LL_ENDL;
        mWindowThread->Post(oldWindowHandle, window_work);
    }

    auto future = promise.get_future();
    // This blocks until mWindowThread processes CreateWindowEx() and calls
    // promise.set_value().
    auto pair = future.get();
    mWindowHandle = pair.first;
    mhDC = pair.second;

    sWindowHandleForMessageBox = mWindowHandle;
}

void* LLWindowWin32::createSharedContext()
{
    mMaxGLVersion = llclamp(mMaxGLVersion, 3.f, 4.6f);

    S32 version_major = llfloor(mMaxGLVersion);
    S32 version_minor = (S32)llround((mMaxGLVersion-version_major)*10);

    S32 attribs[] =
    {
        WGL_CONTEXT_MAJOR_VERSION_ARB, version_major,
        WGL_CONTEXT_MINOR_VERSION_ARB, version_minor,
        WGL_CONTEXT_PROFILE_MASK_ARB,  LLRender::sGLCoreProfile ? WGL_CONTEXT_CORE_PROFILE_BIT_ARB : WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB,
        WGL_CONTEXT_FLAGS_ARB, gDebugGL ? WGL_CONTEXT_DEBUG_BIT_ARB : 0,
        0
    };

    HGLRC rc = 0;

    bool done = false;
    while (!done)
    {
        rc = wglCreateContextAttribsARB(mhDC, mhRC, attribs);

        if (!rc)
        {
            if (attribs[3] > 0)
            { //decrement minor version
                attribs[3]--;
            }
            else if (attribs[1] > 3)
            { //decrement major version and start minor version over at 3
                attribs[1]--;
                attribs[3] = 3;
            }
            else
            { //we reached 3.0 and still failed, bail out
                done = true;
            }
        }
        else
        {
            LL_INFOS() << "Created OpenGL " << llformat("%d.%d", attribs[1], attribs[3]) <<
                (LLRender::sGLCoreProfile ? " core" : " compatibility") << " context." << LL_ENDL;
            done = true;
        }
    }

    if (!rc && !(rc = wglCreateContext(mhDC)))
    {
        close();
        LLError::LLUserWarningMsg::show(mCallbacks->translateString("MBGLContextErr"), 8/*LAST_EXEC_GRAPHICS_INIT*/);
    }

    return rc;
}

void LLWindowWin32::makeContextCurrent(void* contextPtr)
{
    wglMakeCurrent(mhDC, (HGLRC) contextPtr);
    LL_PROFILER_GPU_CONTEXT;
}

void LLWindowWin32::destroySharedContext(void* contextPtr)
{
    wglDeleteContext((HGLRC)contextPtr);
}

void LLWindowWin32::toggleVSync(bool enable_vsync)
{
    if (wglSwapIntervalEXT == nullptr)
    {
        LL_INFOS("Window") << "VSync: wglSwapIntervalEXT not initialized" << LL_ENDL;
    }
    else if (!enable_vsync)
    {
        LL_INFOS("Window") << "Disabling vertical sync" << LL_ENDL;
        wglSwapIntervalEXT(0);
    }
    else
    {
        LL_INFOS("Window") << "Enabling vertical sync" << LL_ENDL;
        wglSwapIntervalEXT(1);
    }
}

void LLWindowWin32::moveWindow( const LLCoordScreen& position, const LLCoordScreen& size )
{
    if( mIsMouseClipping )
    {
        RECT client_rect_in_screen_space;
        if( getClientRectInScreenSpace( &client_rect_in_screen_space ) )
        {
            ClipCursor( &client_rect_in_screen_space );
        }
    }

    // if the window was already maximized, MoveWindow seems to still set the maximized flag even if
    // the window is smaller than maximized.
    // So we're going to do a restore first (which is a ShowWindow call) (SL-44655).

    // THIS CAUSES DEV-15484 and DEV-15949
    //ShowWindow(mWindowHandle, SW_RESTORE);
    // NOW we can call MoveWindow
    mWindowThread->post([=]()
        {
            MoveWindow(mWindowHandle, position.mX, position.mY, size.mX, size.mY, TRUE);
        });
}

void LLWindowWin32::setTitle(const std::string title)
{
    // TODO: Do we need to use the wide string version of this call
    // to support non-ascii usernames (and region names?)
    mWindowThread->post([=]()
        {
            SetWindowTextA(mWindowHandle, title.c_str());
        });
}

bool LLWindowWin32::setCursorPosition(const LLCoordWindow position)
{
    ASSERT_MAIN_THREAD();

    if (!mWindowHandle)
    {
        return false;
    }

    LLCoordScreen screen_pos(position.convert());

    // instantly set the cursor position from the app's point of view
    mCursorPosition = position;
    mLastCursorPosition = position;

    // Inform the application of the new mouse position (needed for per-frame
    // hover/picking to function).
    mCallbacks->handleMouseMove(this, position.convert(), (MASK)0);

    // actually set the cursor position on the window thread
    mWindowThread->post([=]()
        {
            // actually set the OS cursor position
            SetCursorPos(screen_pos.mX, screen_pos.mY);
        });

    return true;
}

bool LLWindowWin32::getCursorPosition(LLCoordWindow *position)
{
    ASSERT_MAIN_THREAD();
    if (!position)
    {
        return false;
    }

    *position = mCursorPosition;
    return true;
}

bool LLWindowWin32::getCursorDelta(LLCoordCommon* delta) const
{
    if (delta == nullptr)
    {
        return false;
    }

    *delta = mMouseFrameDelta;

    return true;
}

void LLWindowWin32::hideCursor()
{
    ASSERT_MAIN_THREAD();

    mWindowThread->post([=]()
        {
            while (ShowCursor(FALSE) >= 0)
            {
                // nothing, wait for cursor to push down
            }
        });

    mCursorHidden = true;
    mHideCursorPermanent = true;
}

void LLWindowWin32::showCursor()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_WIN32;

    ASSERT_MAIN_THREAD();

    mWindowThread->post([=]()
        {
            // makes sure the cursor shows up
            while (ShowCursor(TRUE) < 0)
            {
                // do nothing, wait for cursor to pop out
            }
        });

    mCursorHidden = false;
    mHideCursorPermanent = false;
}

void LLWindowWin32::showCursorFromMouseMove()
{
    if (!mHideCursorPermanent)
    {
        showCursor();
    }
}

void LLWindowWin32::hideCursorUntilMouseMove()
{
    if (!mHideCursorPermanent && mMouseVanish)
    {
        hideCursor();
        mHideCursorPermanent = false;
    }
}

bool LLWindowWin32::isCursorHidden()
{
    return mCursorHidden;
}


HCURSOR LLWindowWin32::loadColorCursor(LPCTSTR name)
{
    return (HCURSOR)LoadImage(mhInstance,
                              name,
                              IMAGE_CURSOR,
                              0,    // default width
                              0,    // default height
                              LR_DEFAULTCOLOR);
}


void LLWindowWin32::initCursors()
{
    mCursor[ UI_CURSOR_ARROW ]      = LoadCursor(NULL, IDC_ARROW);
    mCursor[ UI_CURSOR_WAIT ]       = LoadCursor(NULL, IDC_WAIT);
    mCursor[ UI_CURSOR_HAND ]       = LoadCursor(NULL, IDC_HAND);
    mCursor[ UI_CURSOR_IBEAM ]      = LoadCursor(NULL, IDC_IBEAM);
    mCursor[ UI_CURSOR_CROSS ]      = LoadCursor(NULL, IDC_CROSS);
    mCursor[ UI_CURSOR_SIZENWSE ]   = LoadCursor(NULL, IDC_SIZENWSE);
    mCursor[ UI_CURSOR_SIZENESW ]   = LoadCursor(NULL, IDC_SIZENESW);
    mCursor[ UI_CURSOR_SIZEWE ]     = LoadCursor(NULL, IDC_SIZEWE);
    mCursor[ UI_CURSOR_SIZENS ]     = LoadCursor(NULL, IDC_SIZENS);
    mCursor[ UI_CURSOR_SIZEALL ]    = LoadCursor(NULL, IDC_SIZEALL);
    mCursor[ UI_CURSOR_NO ]         = LoadCursor(NULL, IDC_NO);
    mCursor[ UI_CURSOR_WORKING ]    = LoadCursor(NULL, IDC_APPSTARTING);

    HMODULE module = GetModuleHandle(NULL);
    mCursor[ UI_CURSOR_TOOLGRAB ]   = LoadCursor(module, TEXT("TOOLGRAB"));
    mCursor[ UI_CURSOR_TOOLLAND ]   = LoadCursor(module, TEXT("TOOLLAND"));
    mCursor[ UI_CURSOR_TOOLFOCUS ]  = LoadCursor(module, TEXT("TOOLFOCUS"));
    mCursor[ UI_CURSOR_TOOLCREATE ] = LoadCursor(module, TEXT("TOOLCREATE"));
    mCursor[ UI_CURSOR_ARROWDRAG ]  = LoadCursor(module, TEXT("ARROWDRAG"));
    mCursor[ UI_CURSOR_ARROWCOPY ]  = LoadCursor(module, TEXT("ARROWCOPY"));
    mCursor[ UI_CURSOR_ARROWDRAGMULTI ] = LoadCursor(module, TEXT("ARROWDRAGMULTI"));
    mCursor[ UI_CURSOR_ARROWCOPYMULTI ] = LoadCursor(module, TEXT("ARROWCOPYMULTI"));
    mCursor[ UI_CURSOR_NOLOCKED ]   = LoadCursor(module, TEXT("NOLOCKED"));
    mCursor[ UI_CURSOR_ARROWLOCKED ]= LoadCursor(module, TEXT("ARROWLOCKED"));
    mCursor[ UI_CURSOR_GRABLOCKED ] = LoadCursor(module, TEXT("GRABLOCKED"));
    mCursor[ UI_CURSOR_TOOLTRANSLATE ]  = LoadCursor(module, TEXT("TOOLTRANSLATE"));
    mCursor[ UI_CURSOR_TOOLROTATE ] = LoadCursor(module, TEXT("TOOLROTATE"));
    mCursor[ UI_CURSOR_TOOLSCALE ]  = LoadCursor(module, TEXT("TOOLSCALE"));
    mCursor[ UI_CURSOR_TOOLCAMERA ] = LoadCursor(module, TEXT("TOOLCAMERA"));
    mCursor[ UI_CURSOR_TOOLPAN ]    = LoadCursor(module, TEXT("TOOLPAN"));
    mCursor[ UI_CURSOR_TOOLZOOMIN ] = LoadCursor(module, TEXT("TOOLZOOMIN"));
    mCursor[ UI_CURSOR_TOOLZOOMOUT ] = LoadCursor(module, TEXT("TOOLZOOMOUT"));
    mCursor[ UI_CURSOR_TOOLPICKOBJECT3 ] = LoadCursor(module, TEXT("TOOLPICKOBJECT3"));
    mCursor[ UI_CURSOR_PIPETTE ] = LoadCursor(module, TEXT("TOOLPIPETTE"));
    mCursor[ UI_CURSOR_TOOLSIT ]    = LoadCursor(module, TEXT("TOOLSIT"));
    mCursor[ UI_CURSOR_TOOLBUY ]    = LoadCursor(module, TEXT("TOOLBUY"));
    mCursor[ UI_CURSOR_TOOLOPEN ]   = LoadCursor(module, TEXT("TOOLOPEN"));
    mCursor[ UI_CURSOR_TOOLPATHFINDING ]    = LoadCursor(module, TEXT("TOOLPATHFINDING"));
    mCursor[ UI_CURSOR_TOOLPATHFINDING_PATH_START_ADD ] = LoadCursor(module, TEXT("TOOLPATHFINDINGPATHSTARTADD"));
    mCursor[ UI_CURSOR_TOOLPATHFINDING_PATH_START ] = LoadCursor(module, TEXT("TOOLPATHFINDINGPATHSTART"));
    mCursor[ UI_CURSOR_TOOLPATHFINDING_PATH_END ]   = LoadCursor(module, TEXT("TOOLPATHFINDINGPATHEND"));
    mCursor[ UI_CURSOR_TOOLPATHFINDING_PATH_END_ADD ]   = LoadCursor(module, TEXT("TOOLPATHFINDINGPATHENDADD"));
    mCursor[ UI_CURSOR_TOOLNO ] = LoadCursor(module, TEXT("TOOLNO"));

    // Color cursors
    mCursor[ UI_CURSOR_TOOLPLAY ]       = loadColorCursor(TEXT("TOOLPLAY"));
    mCursor[ UI_CURSOR_TOOLPAUSE ]      = loadColorCursor(TEXT("TOOLPAUSE"));
    mCursor[ UI_CURSOR_TOOLMEDIAOPEN ]  = loadColorCursor(TEXT("TOOLMEDIAOPEN"));

    // Note: custom cursors that are not found make LoadCursor() return NULL.
    for( S32 i = 0; i < UI_CURSOR_COUNT; i++ )
    {
        if( !mCursor[i] )
        {
            mCursor[i] = LoadCursor(NULL, IDC_ARROW);
        }
    }
}



void LLWindowWin32::updateCursor()
{
    ASSERT_MAIN_THREAD();
    LL_PROFILE_ZONE_SCOPED_CATEGORY_WIN32;
    if (mNextCursor == UI_CURSOR_ARROW
        && mBusyCount > 0)
    {
        mNextCursor = UI_CURSOR_WORKING;
    }

    if( mCurrentCursor != mNextCursor )
    {
        mCurrentCursor = mNextCursor;
        auto nextCursor = mCursor[mNextCursor];
        mWindowThread->post([=]()
            {
                SetCursor(nextCursor);
            });
    }
}

ECursorType LLWindowWin32::getCursor() const
{
    return mCurrentCursor;
}

void LLWindowWin32::captureMouse()
{
    SetCapture(mWindowHandle);
}

void LLWindowWin32::releaseMouse()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_WIN32;
    ReleaseCapture();
}


void LLWindowWin32::delayInputProcessing()
{
    mInputProcessingPaused = true;
}


void LLWindowWin32::gatherInput(bool app_has_focus)
{
    ASSERT_MAIN_THREAD();
    LL_PROFILE_ZONE_SCOPED_CATEGORY_WIN32;
    MSG msg;

    {
        LLMutexLock lock(&mRawMouseMutex);
        mMouseFrameDelta = mRawMouseDelta;

        mRawMouseDelta.mX = 0;
        mRawMouseDelta.mY = 0;
    }


    if (mWindowThread->getQueue().size())
    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_WIN32("gi - PostMessage");
        kickWindowThread();
    }

    while (mWindowThread->mMessageQueue.tryPopBack(msg))
    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_WIN32("gi - message queue");
        if (mInputProcessingPaused)
        {
            continue;
        }

        // For async host by name support.  Really hacky.
        if (gAsyncMsgCallback && (LL_WM_HOST_RESOLVED == msg.message))
        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_WIN32("gi - callback");
            gAsyncMsgCallback(msg);
        }
    }

    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_WIN32("gi - PeekMessage");
        S32 msg_count = 0;
        while ((msg_count < MAX_MESSAGE_PER_UPDATE) && PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            msg_count++;
        }
    }

    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_WIN32("gi - function queue");
        //process any pending functions
        std::function<void()> curFunc;
        while (mFunctionQueue.tryPopBack(curFunc))
        {
            curFunc();
        }
    }

    // send one and only one mouse move event per frame BEFORE handling mouse button presses
    if (mLastCursorPosition != mCursorPosition)
    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_WIN32("gi - mouse move");
        mCallbacks->handleMouseMove(this, mCursorPosition.convert(), mMouseMask);
    }

    mLastCursorPosition = mCursorPosition;

    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_WIN32("gi - mouse queue");
        // handle mouse button presses AFTER updating mouse cursor position
        std::function<void()> curFunc;
        while (mMouseQueue.tryPopBack(curFunc))
        {
            curFunc();
        }
    }

    mInputProcessingPaused = false;

    updateCursor();

    LLGameControl::processEvents(app_has_focus);
}

static LLTrace::BlockTimerStatHandle FTM_KEYHANDLER("Handle Keyboard");
static LLTrace::BlockTimerStatHandle FTM_MOUSEHANDLER("Handle Mouse");

#define WINDOW_IMP_POST(x) window_imp->post([=]() { x; })

LRESULT CALLBACK LLWindowWin32::mainWindowProc(HWND h_wnd, UINT u_msg, WPARAM w_param, LPARAM l_param)
{
    ASSERT_WINDOW_THREAD();
    LL_PROFILE_ZONE_SCOPED_CATEGORY_WIN32;

    if (u_msg == WM_POST_FUNCTION_)
    {
        // from LLWindowWin32Thread::Post()
        // Cast l_param back to the pointer to the heap FuncType
        // allocated by Post(). Capture in unique_ptr so we'll delete
        // once we're done with it.
        std::unique_ptr<LLWindowWin32Thread::FuncType>
            ptr(reinterpret_cast<LLWindowWin32Thread::FuncType*>(l_param));
        (*ptr)();
        return 0;
    }

    // Ignore clicks not originated in the client area, i.e. mouse-up events not preceded with a WM_LBUTTONDOWN.
    // This helps prevent avatar walking after maximizing the window by double-clicking the title bar.
    static bool sHandleLeftMouseUp = true;

    // Ignore the double click received right after activating app.
    // This is to avoid triggering double click teleport after returning focus (see MAINT-3786).
    static bool sHandleDoubleClick = true;

    LLWindowWin32* window_imp = (LLWindowWin32*)GetWindowLongPtr(h_wnd, GWLP_USERDATA);

    if (NULL != window_imp)
    {
        // Juggle to make sure we can get negative positions for when
        // mouse is outside window.
        LLCoordWindow window_coord((S32)(S16)LOWORD(l_param), (S32)(S16)HIWORD(l_param));

        // pass along extended flag in mask
        MASK mask = (l_param >> 16 & KF_EXTENDED) ? MASK_EXTENDED : 0x0;
        bool eat_keystroke = true;

        switch (u_msg)
        {
            RECT    update_rect;
            S32     update_width;
            S32     update_height;

        case WM_TIMER:
        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_WIN32("mwp - WM_TIMER");
            WINDOW_IMP_POST(window_imp->mCallbacks->handleTimerEvent(window_imp));
            break;
        }

        case WM_DEVICECHANGE:
        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_WIN32("mwp - WM_DEVICECHANGE");
            if (w_param == DBT_DEVNODES_CHANGED || w_param == DBT_DEVICEARRIVAL)
            {
                WINDOW_IMP_POST(window_imp->mCallbacks->handleDeviceChange(window_imp));

                return 1;
            }
            break;
        }

        case WM_PAINT:
        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_WIN32("mwp - WM_PAINT");
            GetUpdateRect(window_imp->mWindowHandle, &update_rect, FALSE);
            update_width = update_rect.right - update_rect.left + 1;
            update_height = update_rect.bottom - update_rect.top + 1;

            WINDOW_IMP_POST(window_imp->mCallbacks->handlePaint(window_imp, update_rect.left, update_rect.top,
                update_width, update_height));
            break;
        }
        case WM_PARENTNOTIFY:
        {
            break;
        }

        case WM_SETCURSOR:
        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_WIN32("mwp - WM_SETCURSOR");
            // This message is sent whenever the cursor is moved in a window.
            // You need to set the appropriate cursor appearance.

            // Only take control of cursor over client region of window
            // This allows Windows(tm) to handle resize cursors, etc.
            if (LOWORD(l_param) == HTCLIENT)
            {
                SetCursor(window_imp->mCursor[window_imp->mCurrentCursor]);
                return 0;
            }
            break;
        }
        case WM_ENTERMENULOOP:
        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_WIN32("mwp - WM_ENTERMENULOOP");
            WINDOW_IMP_POST(window_imp->mCallbacks->handleWindowBlock(window_imp));
            break;
        }

        case WM_EXITMENULOOP:
        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_WIN32("mwp - WM_EXITMENULOOP");
            WINDOW_IMP_POST(window_imp->mCallbacks->handleWindowUnblock(window_imp));
            break;
        }

        case WM_ACTIVATEAPP:
        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_WIN32("mwp - WM_ACTIVATEAPP");
            window_imp->post([=]()
                {
                    // This message should be sent whenever the app gains or loses focus.
                    BOOL activating = (BOOL)w_param;

                    if (window_imp->mFullscreen)
                    {
                        // When we run fullscreen, restoring or minimizing the app needs
                        // to switch the screen resolution
                        if (activating)
                        {
                            window_imp->setFullscreenResolution();
                            window_imp->restore();
                        }
                        else
                        {
                            window_imp->minimize();
                            window_imp->resetDisplayResolution();
                        }
                    }

                    if (!activating)
                    {
                        sHandleDoubleClick = false;
                    }

                    window_imp->mCallbacks->handleActivateApp(window_imp, activating);
                });
            break;
        }
        case WM_ACTIVATE:
        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_WIN32("mwp - WM_ACTIVATE");
            window_imp->post([=]()
                {
                    // Can be one of WA_ACTIVE, WA_CLICKACTIVE, or WA_INACTIVE
                    BOOL activating = (LOWORD(w_param) != WA_INACTIVE);

                    if (!activating && LLWinImm::isAvailable() && window_imp->mPreeditor)
                    {
                        window_imp->interruptLanguageTextInput();
                    }
                });

            break;
        }

        case WM_QUERYOPEN:
            // TODO: use this to return a nice icon
            break;

        case WM_SYSCOMMAND:
        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_WIN32("mwp - WM_SYSCOMMAND");
            switch (w_param)
            {
            case SC_KEYMENU:
                // Disallow the ALT key from triggering the default system menu.
                return 0;

            case SC_SCREENSAVE:
            case SC_MONITORPOWER:
                // eat screen save messages and prevent them!
                return 0;
            }
            break;
        }
        case WM_CLOSE:
        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_WIN32("mwp - WM_CLOSE");
            window_imp->post([=]()
                {
                    // Will the app allow the window to close?
                    if (window_imp->mCallbacks->handleCloseRequest(window_imp))
                    {
                        // Get the app to initiate cleanup.
                        window_imp->mCallbacks->handleQuit(window_imp);
                        // The app is responsible for calling destroyWindow when done with GL
                    }
                });
            return 0;
        }
        case WM_DESTROY:
        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_WIN32("mwp - WM_DESTROY");
            if (window_imp->shouldPostQuit())
            {
                PostQuitMessage(0);  // Posts WM_QUIT with an exit code of 0
            }
            return 0;
        }
        case WM_COMMAND:
        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_WIN32("mwp - WM_COMMAND");
            if (!HIWORD(w_param)) // this message is from a menu
            {
                WINDOW_IMP_POST(window_imp->mCallbacks->handleMenuSelect(window_imp, LOWORD(w_param)));
            }
            break;
        }
        case WM_SYSKEYDOWN:
        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_WIN32("mwp - WM_SYSKEYDOWN");
            // allow system keys, such as ALT-F4 to be processed by Windows
            eat_keystroke = false;
            // intentional fall-through here
            [[fallthrough]];
        }
        case WM_KEYDOWN:
        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_WIN32("mwp - WM_KEYDOWN");
            window_imp->post([=]()
                {
                    window_imp->mKeyCharCode = 0; // don't know until wm_char comes in next
                    window_imp->mKeyScanCode = (l_param >> 16) & 0xff;
                    window_imp->mKeyVirtualKey = (U32)w_param;
                    window_imp->mRawMsg = u_msg;
                    window_imp->mRawWParam = (U32)w_param;
                    window_imp->mRawLParam = (U32)l_param;

                    gKeyboard->handleKeyDown((U16)w_param, mask);
                });
            if (eat_keystroke) return 0;    // skip DefWindowProc() handling if we're consuming the keypress
            break;
        }
        case WM_SYSKEYUP:
            eat_keystroke = false;
            // intentional fall-through here
            [[fallthrough]];
        case WM_KEYUP:
        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_WIN32("mwp - WM_KEYUP");
            window_imp->post([=]()
            {
                window_imp->mKeyScanCode = (l_param >> 16) & 0xff;
                window_imp->mKeyVirtualKey = (U32)w_param;
                window_imp->mRawMsg = u_msg;
                window_imp->mRawWParam = (U32)w_param;
                window_imp->mRawLParam = (U32)l_param;

                {
                    LL_PROFILE_ZONE_NAMED_CATEGORY_WIN32("mwp - WM_KEYUP");
                    gKeyboard->handleKeyUp((U16)w_param, mask);
                }
            });
            if (eat_keystroke) return 0;    // skip DefWindowProc() handling if we're consuming the keypress
            break;
        }
        case WM_IME_SETCONTEXT:
        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_WIN32("mwp - WM_IME_SETCONTEXT");
            if (LLWinImm::isAvailable() && window_imp->mPreeditor)
            {
                l_param &= ~ISC_SHOWUICOMPOSITIONWINDOW;
                // Invoke DefWinProc with the modified LPARAM.
            }
            break;
        }
        case WM_IME_STARTCOMPOSITION:
        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_WIN32("mwp - WM_IME_STARTCOMPOSITION");
            if (LLWinImm::isAvailable() && window_imp->mPreeditor)
            {
                WINDOW_IMP_POST(window_imp->handleStartCompositionMessage());
                return 0;
            }
            break;
        }
        case WM_IME_ENDCOMPOSITION:
        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_WIN32("mwp - WM_IME_ENDCOMPOSITION");
            if (LLWinImm::isAvailable() && window_imp->mPreeditor)
            {
                return 0;
            }
            break;
        }
        case WM_IME_COMPOSITION:
        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_WIN32("mwp - WM_IME_COMPOSITION");
            if (LLWinImm::isAvailable() && window_imp->mPreeditor)
            {
                WINDOW_IMP_POST(window_imp->handleCompositionMessage((U32)l_param));
                return 0;
            }
            break;
        }
        case WM_IME_REQUEST:
        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_WIN32("mwp - WM_IME_REQUEST");
            if (LLWinImm::isAvailable() && window_imp->mPreeditor)
            {
                LRESULT result;
                window_imp->handleImeRequests(w_param, l_param, &result);
                return result;
            }
            break;
        }
        case WM_CHAR:
        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_WIN32("mwp - WM_CHAR");
            window_imp->post([=]()
                {
                    window_imp->mKeyCharCode = (U32)w_param;
                    window_imp->mRawMsg = u_msg;
                    window_imp->mRawWParam = (U32)w_param;
                    window_imp->mRawLParam = (U32)l_param;

                    // Should really use WM_UNICHAR eventually, but it requires a specific Windows version and I need
                    // to figure out how that works. - Doug
                    //
                    // ... Well, I don't think so.
                    // How it works is explained in Win32 API document, but WM_UNICHAR didn't work
                    // as specified at least on Windows XP SP1 Japanese version.  I have never used
                    // it since then, and I'm not sure whether it has been fixed now, but I don't think
                    // it is worth trying.  The good old WM_CHAR works just fine even for supplementary
                    // characters.  We just need to take care of surrogate pairs sent as two WM_CHAR's
                    // by ourselves.  It is not that tough.  -- Alissa Sabre @ SL

                    // Even if LLWindowCallbacks::handleUnicodeChar(llwchar, bool) returned false,
                    // we *did* processed the event, so I believe we should not pass it to DefWindowProc...
                    window_imp->handleUnicodeUTF16((U16)w_param, gKeyboard->currentMask(false));
                });
            return 0;
        }
        case WM_NCLBUTTONDOWN:
        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_WIN32("mwp - WM_NCLBUTTONDOWN");
            {
                // A click in a non-client area, e.g. title bar or window border.
                window_imp->post([=]()
                    {
                        sHandleLeftMouseUp = false;
                        sHandleDoubleClick = true;
                    });
            }
            break;
        }
        case WM_LBUTTONDOWN:
        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_WIN32("mwp - WM_LBUTTONDOWN");
            {
                LL_RECORD_BLOCK_TIME(FTM_MOUSEHANDLER);
                window_imp->postMouseButtonEvent([=]()
                    {
                        sHandleLeftMouseUp = true;

                        if (LLWinImm::isAvailable() && window_imp->mPreeditor)
                        {
                            window_imp->interruptLanguageTextInput();
                        }

                        MASK mask = gKeyboard->currentMask(true);
                        auto gl_coord = window_imp->mCursorPosition.convert();
                        window_imp->mCallbacks->handleMouseMove(window_imp, gl_coord, mask);
                        window_imp->mCallbacks->handleMouseDown(window_imp, gl_coord, mask);
                    });

                return 0;
            }
            break;
        }

        case WM_LBUTTONDBLCLK:
        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_WIN32("mwp - WM_LBUTTONDBLCLK");
            window_imp->postMouseButtonEvent([=]()
                {
                    //RN: ignore right button double clicks for now
                    //case WM_RBUTTONDBLCLK:
                    if (!sHandleDoubleClick)
                    {
                        sHandleDoubleClick = true;
                        return;
                    }
                    MASK mask = gKeyboard->currentMask(true);

                    // generate move event to update mouse coordinates
                    window_imp->mCursorPosition = window_coord;
                    window_imp->mCallbacks->handleDoubleClick(window_imp, window_imp->mCursorPosition.convert(), mask);
                });

            return 0;
        }
        case WM_LBUTTONUP:
        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_WIN32("mwp - WM_LBUTTONUP");
            {
                window_imp->postMouseButtonEvent([=]()
                    {
                        LL_RECORD_BLOCK_TIME(FTM_MOUSEHANDLER);
                        if (!sHandleLeftMouseUp)
                        {
                            sHandleLeftMouseUp = true;
                            return;
                        }
                        sHandleDoubleClick = true;


                        MASK mask = gKeyboard->currentMask(true);
                        // generate move event to update mouse coordinates
                        window_imp->mCursorPosition = window_coord;
                        window_imp->mCallbacks->handleMouseUp(window_imp, window_imp->mCursorPosition.convert(), mask);
                    });
            }
            return 0;
        }
        case WM_RBUTTONDBLCLK:
        case WM_RBUTTONDOWN:
        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_WIN32("mwp - WM_RBUTTONDOWN");
            {
                LL_RECORD_BLOCK_TIME(FTM_MOUSEHANDLER);
                window_imp->post([=]()
                    {
                        if (LLWinImm::isAvailable() && window_imp->mPreeditor)
                        {
                            WINDOW_IMP_POST(window_imp->interruptLanguageTextInput());
                        }

                        MASK mask = gKeyboard->currentMask(true);
                        // generate move event to update mouse coordinates
                        auto gl_coord = window_imp->mCursorPosition.convert();
                        window_imp->mCallbacks->handleMouseMove(window_imp, gl_coord, mask);
                        window_imp->mCallbacks->handleRightMouseDown(window_imp, gl_coord, mask);
                    });
            }
            return 0;
        }
        break;

        case WM_RBUTTONUP:
        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_WIN32("mwp - WM_RBUTTONUP");
            {
                LL_RECORD_BLOCK_TIME(FTM_MOUSEHANDLER);
                window_imp->postMouseButtonEvent([=]()
                    {
                        MASK mask = gKeyboard->currentMask(true);
                        window_imp->mCallbacks->handleRightMouseUp(window_imp, window_imp->mCursorPosition.convert(), mask);
                    });
            }
        }
        break;

        case WM_MBUTTONDOWN:
            //      case WM_MBUTTONDBLCLK:
        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_WIN32("mwp - WM_MBUTTONDOWN");
            {
                LL_RECORD_BLOCK_TIME(FTM_MOUSEHANDLER);
                window_imp->postMouseButtonEvent([=]()
                    {
                        if (LLWinImm::isAvailable() && window_imp->mPreeditor)
                        {
                            window_imp->interruptLanguageTextInput();
                        }

                        MASK mask = gKeyboard->currentMask(true);
                        window_imp->mCallbacks->handleMiddleMouseDown(window_imp, window_imp->mCursorPosition.convert(), mask);
                    });
            }
        }
        break;

        case WM_MBUTTONUP:
        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_WIN32("mwp - WM_MBUTTONUP");
            {
                LL_RECORD_BLOCK_TIME(FTM_MOUSEHANDLER);
                window_imp->postMouseButtonEvent([=]()
                    {
                        MASK mask = gKeyboard->currentMask(true);
                        window_imp->mCallbacks->handleMiddleMouseUp(window_imp, window_imp->mCursorPosition.convert(), mask);
                    });
            }
        }
        break;
        case WM_XBUTTONDOWN:
        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_WIN32("mwp - WM_XBUTTONDOWN");
            window_imp->postMouseButtonEvent([=]()
                {
                    LL_RECORD_BLOCK_TIME(FTM_MOUSEHANDLER);
                    S32 button = GET_XBUTTON_WPARAM(w_param);
                    if (LLWinImm::isAvailable() && window_imp->mPreeditor)
                    {
                        window_imp->interruptLanguageTextInput();
                    }

                    MASK mask = gKeyboard->currentMask(true);
                    // Windows uses numbers 1 and 2 for buttons, remap to 4, 5
                    window_imp->mCallbacks->handleOtherMouseDown(window_imp, window_imp->mCursorPosition.convert(), mask, button + 3);
                });

        }
        break;

        case WM_XBUTTONUP:
        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_WIN32("mwp - WM_XBUTTONUP");
            window_imp->postMouseButtonEvent([=]()
                {

                    LL_RECORD_BLOCK_TIME(FTM_MOUSEHANDLER);

                    S32 button = GET_XBUTTON_WPARAM(w_param);
                    MASK mask = gKeyboard->currentMask(true);
                    // Windows uses numbers 1 and 2 for buttons, remap to 4, 5
                    window_imp->mCallbacks->handleOtherMouseUp(window_imp, window_imp->mCursorPosition.convert(), mask, button + 3);
                });
        }
        break;

        case WM_MOUSEWHEEL:
        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_WIN32("mwp - WM_MOUSEWHEEL");
            static short z_delta = 0;

            RECT    client_rect;

            // eat scroll events that occur outside our window, since we use mouse position to direct scroll
            // instead of keyboard focus
            // NOTE: mouse_coord is in *window* coordinates for scroll events
            POINT mouse_coord = { (S32)(S16)LOWORD(l_param), (S32)(S16)HIWORD(l_param) };

            if (ScreenToClient(window_imp->mWindowHandle, &mouse_coord)
                && GetClientRect(window_imp->mWindowHandle, &client_rect))
            {
                // we have a valid mouse point and client rect
                if (mouse_coord.x < client_rect.left || client_rect.right < mouse_coord.x
                    || mouse_coord.y < client_rect.top || client_rect.bottom < mouse_coord.y)
                {
                    // mouse is outside of client rect, so don't do anything
                    return 0;
                }
            }

            S16 incoming_z_delta = HIWORD(w_param);
            z_delta += incoming_z_delta;
            // cout << "z_delta " << z_delta << endl;

            // current mouse wheels report changes in increments of zDelta (+120, -120)
            // Future, higher resolution mouse wheels may report smaller deltas.
            // So we sum the deltas and only act when we've exceeded WHEEL_DELTA
            //
            // If the user rapidly spins the wheel, we can get messages with
            // large deltas, like 480 or so.  Thus we need to scroll more quickly.
            if (z_delta <= -WHEEL_DELTA || WHEEL_DELTA <= z_delta)
            {
                short clicks = -z_delta / WHEEL_DELTA;
                WINDOW_IMP_POST(window_imp->mCallbacks->handleScrollWheel(window_imp, clicks));
                z_delta = 0;
            }
            return 0;
        }
        /*
        // TODO: add this after resolving _WIN32_WINNT issue
        case WM_MOUSELEAVE:
        {
        window_imp->mCallbacks->handleMouseLeave(window_imp);

        //              TRACKMOUSEEVENT track_mouse_event;
        //              track_mouse_event.cbSize = sizeof( TRACKMOUSEEVENT );
        //              track_mouse_event.dwFlags = TME_LEAVE;
        //              track_mouse_event.hwndTrack = h_wnd;
        //              track_mouse_event.dwHoverTime = HOVER_DEFAULT;
        //              TrackMouseEvent( &track_mouse_event );
        return 0;
        }
        */
        case WM_MOUSEHWHEEL:
        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_WIN32("mwp - WM_MOUSEHWHEEL");
            static short h_delta = 0;

            RECT    client_rect;

            // eat scroll events that occur outside our window, since we use mouse position to direct scroll
            // instead of keyboard focus
            // NOTE: mouse_coord is in *window* coordinates for scroll events
            POINT mouse_coord = { (S32)(S16)LOWORD(l_param), (S32)(S16)HIWORD(l_param) };

            if (ScreenToClient(window_imp->mWindowHandle, &mouse_coord)
                && GetClientRect(window_imp->mWindowHandle, &client_rect))
            {
                // we have a valid mouse point and client rect
                if (mouse_coord.x < client_rect.left || client_rect.right < mouse_coord.x
                    || mouse_coord.y < client_rect.top || client_rect.bottom < mouse_coord.y)
                {
                    // mouse is outside of client rect, so don't do anything
                    return 0;
                }
            }

            S16 incoming_h_delta = HIWORD(w_param);
            h_delta += incoming_h_delta;

            // If the user rapidly spins the wheel, we can get messages with
            // large deltas, like 480 or so.  Thus we need to scroll more quickly.
            if (h_delta <= -WHEEL_DELTA || WHEEL_DELTA <= h_delta)
            {
                WINDOW_IMP_POST(window_imp->mCallbacks->handleScrollHWheel(window_imp, h_delta / WHEEL_DELTA));
                h_delta = 0;
            }
            return 0;
        }
        // Handle mouse movement within the window
        case WM_MOUSEMOVE:
        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_WIN32("mwp - WM_MOUSEMOVE");
            // DO NOT use mouse event queue for move events to ensure cursor position is updated
            // when button events are handled
            WINDOW_IMP_POST(
                {
                    LL_PROFILE_ZONE_NAMED_CATEGORY_WIN32("mwp - WM_MOUSEMOVE lambda");

                    MASK mask = gKeyboard->currentMask(true);
                    window_imp->mMouseMask = mask;
                    window_imp->mCursorPosition = window_coord;
                });
            return 0;
        }

        case WM_GETMINMAXINFO:
        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_WIN32("mwp - WM_GETMINMAXINFO");
            LPMINMAXINFO min_max = (LPMINMAXINFO)l_param;
            min_max->ptMinTrackSize.x = window_imp->mMinWindowWidth;
            min_max->ptMinTrackSize.y = window_imp->mMinWindowHeight;
            return 0;
        }

        case WM_MOVE:
        {
            window_imp->updateWindowRect();
            return 0;
        }
        case WM_SIZE:
        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_WIN32("mwp - WM_SIZE");
            window_imp->updateWindowRect();

            // There's an odd behavior with WM_SIZE that I would call a bug. If
            // the window is maximized, and you call MoveWindow() with a size smaller
            // than a maximized window, it ends up sending WM_SIZE with w_param set
            // to SIZE_MAXIMIZED -- which isn't true. So the logic below doesn't work.
            // (SL-44655). Fixed it by calling ShowWindow(SW_RESTORE) first (see
            // LLWindowWin32::moveWindow in this file).

            // If we are now restored, but we weren't before, this
            // means that the window was un-minimized.
            if (w_param == SIZE_RESTORED && window_imp->mLastSizeWParam != SIZE_RESTORED)
            {
                WINDOW_IMP_POST(window_imp->mCallbacks->handleActivate(window_imp, true));
            }

            // handle case of window being maximized from fully minimized state
            if (w_param == SIZE_MAXIMIZED && window_imp->mLastSizeWParam != SIZE_MAXIMIZED)
            {
                WINDOW_IMP_POST(window_imp->mCallbacks->handleActivate(window_imp, true));
            }

            // Also handle the minimization case
            if (w_param == SIZE_MINIMIZED && window_imp->mLastSizeWParam != SIZE_MINIMIZED)
            {
                WINDOW_IMP_POST(window_imp->mCallbacks->handleActivate(window_imp, false));
            }

            // Actually resize all of our views
            if (w_param != SIZE_MINIMIZED)
            {
                // Ignore updates for minimizing and minimized "windows"
                WINDOW_IMP_POST(window_imp->mCallbacks->handleResize(window_imp,
                    LOWORD(l_param),
                    HIWORD(l_param)));
            }

            window_imp->mLastSizeWParam = w_param;

            return 0;
        }

        case WM_DPICHANGED:
        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_WIN32("mwp - WM_DPICHANGED");
            LPRECT lprc_new_scale;
            F32 new_scale = F32(LOWORD(w_param)) / F32(USER_DEFAULT_SCREEN_DPI);
            lprc_new_scale = (LPRECT)l_param;
            S32 new_width = lprc_new_scale->right - lprc_new_scale->left;
            S32 new_height = lprc_new_scale->bottom - lprc_new_scale->top;
            WINDOW_IMP_POST(window_imp->mCallbacks->handleDPIChanged(window_imp, new_scale, new_width, new_height));

            SetWindowPos(h_wnd,
                HWND_TOP,
                lprc_new_scale->left,
                lprc_new_scale->top,
                new_width,
                new_height,
                SWP_NOZORDER | SWP_NOACTIVATE);

            return 0;
        }

        case WM_SETFOCUS:
        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_WIN32("mwp - WM_SETFOCUS");
            WINDOW_IMP_POST(window_imp->mCallbacks->handleFocus(window_imp));
            return 0;
        }

        case WM_KILLFOCUS:
        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_WIN32("mwp - WM_KILLFOCUS");
            WINDOW_IMP_POST(window_imp->mCallbacks->handleFocusLost(window_imp));
            return 0;
        }

        case WM_COPYDATA:
        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_WIN32("mwp - WM_COPYDATA");
            {
                // received a URL
                PCOPYDATASTRUCT myCDS = (PCOPYDATASTRUCT)l_param;
                void* data = new U8[myCDS->cbData];
                memcpy(data, myCDS->lpData, myCDS->cbData);
                auto myType = myCDS->dwData;

                window_imp->post([=]()
                    {
                       window_imp->mCallbacks->handleDataCopy(window_imp, (S32)myType, data);
                       delete[] data;
                    });
            };
            return 0;
        }
        case WM_SETTINGCHANGE:
        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_WIN32("mwp - WM_SETTINGCHANGE");
            if (w_param == SPI_SETMOUSEVANISH)
            {
                if (!SystemParametersInfo(SPI_GETMOUSEVANISH, 0, &window_imp->mMouseVanish, 0))
                {
                    WINDOW_IMP_POST(window_imp->mMouseVanish = true);
                }
            }
        }
        break;

        case WM_INPUT:
        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_WIN32("MWP - WM_INPUT");

            UINT dwSize = 0;
            GetRawInputData((HRAWINPUT)l_param, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));
            llassert(dwSize < 1024);

            U8 lpb[1024];

            if (GetRawInputData((HRAWINPUT)l_param, RID_INPUT, (void*)lpb, &dwSize, sizeof(RAWINPUTHEADER)) == dwSize)
            {
                RAWINPUT* raw = (RAWINPUT*)lpb;

                if (raw->header.dwType == RIM_TYPEMOUSE)
                {
                    LLMutexLock lock(&window_imp->mRawMouseMutex);

                    bool absolute_coordinates = (raw->data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE);

                    if (absolute_coordinates)
                    {
                        static S32 prev_absolute_x = 0;
                        static S32 prev_absolute_y = 0;
                        S32 absolute_x;
                        S32 absolute_y;

                        if ((raw->data.mouse.usFlags & 0x10) == 0x10) // touch screen? touch? Not defined in header
                        {
                            // touch screen spams (0,0) coordinates in a number of situations
                            // (0,0) might need to be filtered
                            absolute_x = raw->data.mouse.lLastX;
                            absolute_y = raw->data.mouse.lLastY;
                        }
                        else
                        {
                            bool v_desktop = (raw->data.mouse.usFlags & MOUSE_VIRTUAL_DESKTOP) == MOUSE_VIRTUAL_DESKTOP;

                            S32 width = GetSystemMetrics(v_desktop ? SM_CXVIRTUALSCREEN : SM_CXSCREEN);
                            S32 height = GetSystemMetrics(v_desktop ? SM_CYVIRTUALSCREEN : SM_CYSCREEN);

                            absolute_x = (S32)((raw->data.mouse.lLastX / 65535.0f) * width);
                            absolute_y = (S32)((raw->data.mouse.lLastY / 65535.0f) * height);
                        }

                        window_imp->mRawMouseDelta.mX += absolute_x - prev_absolute_x;
                        window_imp->mRawMouseDelta.mY -= absolute_y - prev_absolute_y;

                        prev_absolute_x = absolute_x;
                        prev_absolute_y = absolute_y;
                        window_imp->mAbsoluteCursorPosition = true;
                    }
                    else
                    {
                        S32 speed;
                        const S32 DEFAULT_SPEED(10);
                        SystemParametersInfo(SPI_GETMOUSESPEED, 0, &speed, 0);
                        if (speed == DEFAULT_SPEED)
                        {
                            window_imp->mRawMouseDelta.mX += raw->data.mouse.lLastX;
                            window_imp->mRawMouseDelta.mY -= raw->data.mouse.lLastY;
                        }
                        else
                        {
                            window_imp->mRawMouseDelta.mX += (S32)round((F32)raw->data.mouse.lLastX * (F32)speed / DEFAULT_SPEED);
                            window_imp->mRawMouseDelta.mY -= (S32)round((F32)raw->data.mouse.lLastY * (F32)speed / DEFAULT_SPEED);
                        }
                        window_imp->mAbsoluteCursorPosition = false;
                    }
                }
            }
        }
        break;

        //list of messages we get often that we don't care to log about
        case WM_NCHITTEST:
        case WM_NCMOUSEMOVE:
        case WM_NCMOUSELEAVE:
        case WM_MOVING:
        case WM_WINDOWPOSCHANGING:
        case WM_WINDOWPOSCHANGED:
        break;

        default:
        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_WIN32("mwp - default");
            LL_DEBUGS("Window") << "Unhandled windows message code: 0x" << std::hex << U32(u_msg) << LL_ENDL;
        }
        break;
        }
    }
    else
    {
        // (NULL == window_imp)
        LL_DEBUGS("Window") << "No window implementation to handle message with, message code: " << U32(u_msg) << LL_ENDL;
    }

    // pass unhandled messages down to Windows
    LRESULT ret;
    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_WIN32("mwp - DefWindowProc");
        ret = DefWindowProc(h_wnd, u_msg, w_param, l_param);
    }
    return ret;
}

bool LLWindowWin32::convertCoords(LLCoordGL from, LLCoordWindow *to) const
{
    S32     client_height;
    RECT    client_rect;
    LLCoordWindow window_position;

    if (!mWindowHandle ||
        !GetClientRect(mWindowHandle, &client_rect) ||
        NULL == to)
    {
        return false;
    }

    to->mX = from.mX;
    client_height = client_rect.bottom - client_rect.top;
    to->mY = client_height - from.mY - 1;

    return true;
}

bool LLWindowWin32::convertCoords(LLCoordWindow from, LLCoordGL* to) const
{
    S32     client_height;
    RECT    client_rect;

    if (!mWindowHandle ||
        !GetClientRect(mWindowHandle, &client_rect) ||
        NULL == to)
    {
        return false;
    }

    to->mX = from.mX;
    client_height = client_rect.bottom - client_rect.top;
    to->mY = client_height - from.mY - 1;

    return true;
}

bool LLWindowWin32::convertCoords(LLCoordScreen from, LLCoordWindow* to) const
{
    POINT mouse_point;

    mouse_point.x = from.mX;
    mouse_point.y = from.mY;
    bool result = ScreenToClient(mWindowHandle, &mouse_point);

    if (result)
    {
        to->mX = mouse_point.x;
        to->mY = mouse_point.y;
    }

    return result;
}

bool LLWindowWin32::convertCoords(LLCoordWindow from, LLCoordScreen *to) const
{
    POINT mouse_point;

    mouse_point.x = from.mX;
    mouse_point.y = from.mY;
    bool result = ClientToScreen(mWindowHandle, &mouse_point);

    if (result)
    {
        to->mX = mouse_point.x;
        to->mY = mouse_point.y;
    }

    return result;
}

bool LLWindowWin32::convertCoords(LLCoordScreen from, LLCoordGL *to) const
{
    LLCoordWindow window_coord;

    if (!mWindowHandle || (NULL == to))
    {
        return false;
    }

    convertCoords(from, &window_coord);
    convertCoords(window_coord, to);
    return true;
}

bool LLWindowWin32::convertCoords(LLCoordGL from, LLCoordScreen *to) const
{
    LLCoordWindow window_coord;

    if (!mWindowHandle || (NULL == to))
    {
        return false;
    }

    convertCoords(from, &window_coord);
    convertCoords(window_coord, to);
    return true;
}


bool LLWindowWin32::isClipboardTextAvailable()
{
    return IsClipboardFormatAvailable(CF_UNICODETEXT);
}


bool LLWindowWin32::pasteTextFromClipboard(LLWString &dst)
{
    bool success = false;

    if (IsClipboardFormatAvailable(CF_UNICODETEXT))
    {
        if (OpenClipboard(mWindowHandle))
        {
            HGLOBAL h_data = GetClipboardData(CF_UNICODETEXT);
            if (h_data)
            {
                WCHAR *utf16str = (WCHAR*) GlobalLock(h_data);
                if (utf16str)
                {
                    dst = utf16str_to_wstring(utf16str);
                    LLWStringUtil::removeWindowsCR(dst);
                    GlobalUnlock(h_data);
                    success = true;
                }
            }
            CloseClipboard();
        }
    }

    return success;
}


bool LLWindowWin32::copyTextToClipboard(const LLWString& wstr)
{
    bool success = false;

    if (OpenClipboard(mWindowHandle))
    {
        EmptyClipboard();

        // Provide a copy of the data in Unicode format.
        LLWString sanitized_string(wstr);
        LLWStringUtil::addCRLF(sanitized_string);
        llutf16string out_utf16 = wstring_to_utf16str(sanitized_string);
        const size_t size_utf16 = (out_utf16.length() + 1) * sizeof(WCHAR);

        // Memory is allocated and then ownership of it is transfered to the system.
        HGLOBAL hglobal_copy_utf16 = GlobalAlloc(GMEM_MOVEABLE, size_utf16);
        if (hglobal_copy_utf16)
        {
            WCHAR* copy_utf16 = (WCHAR*) GlobalLock(hglobal_copy_utf16);
            if (copy_utf16)
            {
                memcpy(copy_utf16, out_utf16.c_str(), size_utf16);  /* Flawfinder: ignore */
                GlobalUnlock(hglobal_copy_utf16);

                if (SetClipboardData(CF_UNICODETEXT, hglobal_copy_utf16))
                {
                    success = true;
                }
            }
        }

        CloseClipboard();
    }

    return success;
}

// Constrains the mouse to the window.
void LLWindowWin32::setMouseClipping( bool b )
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_WIN32;
    ASSERT_MAIN_THREAD();
    if( b != mIsMouseClipping )
    {
        bool success = false;

        if( b )
        {
            GetClipCursor( &mOldMouseClip );

            RECT client_rect_in_screen_space;
            if( getClientRectInScreenSpace( &client_rect_in_screen_space ) )
            {
                success = ClipCursor( &client_rect_in_screen_space );
            }
        }
        else
        {
            // Must restore the old mouse clip, which may be set by another window.
            success = ClipCursor( &mOldMouseClip );
            SetRect( &mOldMouseClip, 0, 0, 0, 0 );
        }

        if( success )
        {
            mIsMouseClipping = b;
        }
    }
}

bool LLWindowWin32::getClientRectInScreenSpace( RECT* rectp ) const
{
    bool success = false;

    RECT client_rect;
    if (mWindowHandle && GetClientRect(mWindowHandle, &client_rect))
    {
        POINT top_left;
        top_left.x = client_rect.left;
        top_left.y = client_rect.top;
        ClientToScreen(mWindowHandle, &top_left);

        POINT bottom_right;
        bottom_right.x = client_rect.right;
        bottom_right.y = client_rect.bottom;
        ClientToScreen(mWindowHandle, &bottom_right);

        SetRect(rectp,
            top_left.x,
            top_left.y,
            bottom_right.x,
            bottom_right.y);

        success = true;
    }

    return success;
}

void LLWindowWin32::flashIcon(F32 seconds)
{
    mWindowThread->post([=]()
        {
            FLASHWINFO flash_info;

            flash_info.cbSize = sizeof(FLASHWINFO);
            flash_info.hwnd = mWindowHandle;
            flash_info.dwFlags = FLASHW_TRAY;
            flash_info.uCount = UINT(seconds / ICON_FLASH_TIME);
            flash_info.dwTimeout = DWORD(1000.f * ICON_FLASH_TIME); // milliseconds
            FlashWindowEx(&flash_info);
        });
}

F32 LLWindowWin32::getGamma() const
{
    return mCurrentGamma;
}

bool LLWindowWin32::restoreGamma()
{
    ASSERT_MAIN_THREAD();
    if (mCustomGammaSet)
    {
        LL_DEBUGS("Window") << "Restoring gamma" << LL_ENDL;
        mCustomGammaSet = false;
        return SetDeviceGammaRamp(mhDC, mPrevGammaRamp);
    }
    return true;
}

bool LLWindowWin32::setGamma(const F32 gamma)
{
    ASSERT_MAIN_THREAD();
    mCurrentGamma = gamma;

    //Get the previous gamma ramp to restore later.
    if (!mCustomGammaSet)
    {
        if (!gGLManager.mIsIntel) // skip for Intel GPUs (see SL-11341)
        {
            LL_DEBUGS("Window") << "Getting the previous gamma ramp to restore later" << LL_ENDL;
            if (!GetDeviceGammaRamp(mhDC, mPrevGammaRamp))
            {
                LL_WARNS("Window") << "Failed to get the previous gamma ramp" << LL_ENDL;
                return false;
            }
        }
        mCustomGammaSet = true;
    }

    LL_DEBUGS("Window") << "Setting gamma to " << gamma << LL_ENDL;

    for ( int i = 0; i < 256; ++i )
    {
        int mult = 256 - ( int ) ( ( gamma - 1.0f ) * 128.0f );

        int value = mult * i;

        if ( value > 0xffff )
            value = 0xffff;

        mCurrentGammaRamp[0][i] =
            mCurrentGammaRamp[1][i] =
            mCurrentGammaRamp[2][i] = (WORD) value;
    };

    return SetDeviceGammaRamp ( mhDC, mCurrentGammaRamp );
}

void LLWindowWin32::setFSAASamples(const U32 fsaa_samples)
{
    ASSERT_MAIN_THREAD();
    mFSAASamples = fsaa_samples;
}

U32 LLWindowWin32::getFSAASamples() const
{
    return mFSAASamples;
}

LLWindow::LLWindowResolution* LLWindowWin32::getSupportedResolutions(S32 &num_resolutions)
{
    ASSERT_MAIN_THREAD();
    if (!mSupportedResolutions)
    {
        mSupportedResolutions = new LLWindowResolution[MAX_NUM_RESOLUTIONS];
        DEVMODE dev_mode;
        ::ZeroMemory(&dev_mode, sizeof(DEVMODE));
        dev_mode.dmSize = sizeof(DEVMODE);

        mNumSupportedResolutions = 0;
        for (S32 mode_num = 0; mNumSupportedResolutions < MAX_NUM_RESOLUTIONS; mode_num++)
        {
            if (!EnumDisplaySettings(NULL, mode_num, &dev_mode))
            {
                break;
            }

            if (dev_mode.dmBitsPerPel == BITS_PER_PIXEL &&
                dev_mode.dmPelsWidth >= 800 &&
                dev_mode.dmPelsHeight >= 600)
            {
                bool resolution_exists = false;
                for(S32 i = 0; i < mNumSupportedResolutions; i++)
                {
                    if (mSupportedResolutions[i].mWidth == dev_mode.dmPelsWidth &&
                        mSupportedResolutions[i].mHeight == dev_mode.dmPelsHeight)
                    {
                        resolution_exists = true;
                    }
                }
                if (!resolution_exists)
                {
                    mSupportedResolutions[mNumSupportedResolutions].mWidth = dev_mode.dmPelsWidth;
                    mSupportedResolutions[mNumSupportedResolutions].mHeight = dev_mode.dmPelsHeight;
                    mNumSupportedResolutions++;
                }
            }
        }
    }

    num_resolutions = mNumSupportedResolutions;
    return mSupportedResolutions;
}


F32 LLWindowWin32::getNativeAspectRatio()
{
    if (mOverrideAspectRatio > 0.f)
    {
        return mOverrideAspectRatio;
    }
    else if (mNativeAspectRatio > 0.f)
    {
        // we grabbed this value at startup, based on the user's desktop settings
        return mNativeAspectRatio;
    }
    // RN: this hack presumes that the largest supported resolution is monitor-limited
    // and that pixels in that mode are square, therefore defining the native aspect ratio
    // of the monitor...this seems to work to a close approximation for most CRTs/LCDs
    S32 num_resolutions;
    LLWindowResolution* resolutions = getSupportedResolutions(num_resolutions);

    return ((F32)resolutions[num_resolutions - 1].mWidth / (F32)resolutions[num_resolutions - 1].mHeight);
}

F32 LLWindowWin32::getPixelAspectRatio()
{
    F32 pixel_aspect = 1.f;
    if (getFullscreen())
    {
        LLCoordScreen screen_size;
        getSize(&screen_size);
        pixel_aspect = getNativeAspectRatio() * (F32)screen_size.mY / (F32)screen_size.mX;
    }

    return pixel_aspect;
}

// Change display resolution.  Returns true if successful.
// protected
bool LLWindowWin32::setDisplayResolution(S32 width, S32 height, S32 refresh)
{
    DEVMODE dev_mode;
    ::ZeroMemory(&dev_mode, sizeof(DEVMODE));
    dev_mode.dmSize = sizeof(DEVMODE);
    bool success = false;

    // Don't change anything if we don't have to
    if (EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &dev_mode))
    {
        if (dev_mode.dmPelsWidth        == width &&
            dev_mode.dmPelsHeight       == height &&
            dev_mode.dmDisplayFrequency == refresh )
        {
            // ...display mode identical, do nothing
            return true;
        }
    }

    memset(&dev_mode, 0, sizeof(dev_mode));
    dev_mode.dmSize = sizeof(dev_mode);
    dev_mode.dmPelsWidth        = width;
    dev_mode.dmPelsHeight       = height;
    dev_mode.dmDisplayFrequency = refresh;
    dev_mode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY;

    // CDS_FULLSCREEN indicates that this is a temporary change to the device mode.
    LONG cds_result = ChangeDisplaySettings(&dev_mode, CDS_FULLSCREEN);

    success = (DISP_CHANGE_SUCCESSFUL == cds_result);

    if (!success)
    {
        LL_WARNS("Window") << "setDisplayResolution failed, "
            << width << "x" << height << " @ " << refresh << LL_ENDL;
    }

    return success;
}

// protected
bool LLWindowWin32::setFullscreenResolution()
{
    if (mFullscreen)
    {
        return setDisplayResolution( mFullscreenWidth, mFullscreenHeight, mFullscreenRefresh);
    }
    else
    {
        return false;
    }
}

// protected
bool LLWindowWin32::resetDisplayResolution()
{
    LL_DEBUGS("Window") << "resetDisplayResolution START" << LL_ENDL;

    LONG cds_result = ChangeDisplaySettings(NULL, 0);

    bool success = (DISP_CHANGE_SUCCESSFUL == cds_result);

    if (!success)
    {
        LL_WARNS("Window") << "resetDisplayResolution failed" << LL_ENDL;
    }

    LL_DEBUGS("Window") << "resetDisplayResolution END" << LL_ENDL;

    return success;
}

void LLWindowWin32::swapBuffers()
{
    {
        LL_PROFILE_ZONE_SCOPED_CATEGORY_WIN32;
        SwapBuffers(mhDC);
    }

    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_WIN32("GPU Collect");
        LL_PROFILER_GPU_COLLECT;
    }
}


//
// LLSplashScreenImp
//
LLSplashScreenWin32::LLSplashScreenWin32()
:   mWindow(NULL)
{
}

LLSplashScreenWin32::~LLSplashScreenWin32()
{
}

void LLSplashScreenWin32::showImpl()
{
    // This appears to work.  ???
    HINSTANCE hinst = GetModuleHandle(NULL);

    mWindow = CreateDialog(hinst,
        TEXT("SPLASHSCREEN"),
        NULL,   // no parent
        (DLGPROC) LLSplashScreenWin32::windowProc);
    ShowWindow(mWindow, SW_SHOW);

    // Should set taskbar text without creating a header for the window (caption)
    SetWindowTextA(mWindow, "Second Life");
}


void LLSplashScreenWin32::updateImpl(const std::string& mesg)
{
    if (!mWindow) return;

    int output_str_len = MultiByteToWideChar(CP_UTF8, 0, mesg.c_str(), static_cast<int>(mesg.length()), NULL, 0);
    if( output_str_len>1024 )
        return;

    WCHAR w_mesg[1025];//big enought to keep null terminatos

    MultiByteToWideChar (CP_UTF8, 0, mesg.c_str(), static_cast<int>(mesg.length()), w_mesg, output_str_len);

    //looks like MultiByteToWideChar didn't add null terminator to converted string, see EXT-4858
    w_mesg[output_str_len] = 0;

    SendDlgItemMessage(mWindow,
        666,        // HACK: text id
        WM_SETTEXT,
        FALSE,
        (LPARAM)w_mesg);
}


void LLSplashScreenWin32::hideImpl()
{
    if (mWindow)
    {
        if (!destroy_window_handler(mWindow))
        {
            LL_WARNS("Window") << "Failed to properly close splash screen window!" << LL_ENDL;
        }
        mWindow = NULL;
    }
}


// static
LRESULT CALLBACK LLSplashScreenWin32::windowProc(HWND h_wnd, UINT u_msg,
                                            WPARAM w_param, LPARAM l_param)
{
    // Just give it to windows
    return DefWindowProc(h_wnd, u_msg, w_param, l_param);
}

//
// Helper Funcs
//

S32 OSMessageBoxWin32(const std::string& text, const std::string& caption, U32 type)
{
    UINT uType;

    switch(type)
    {
    case OSMB_OK:
        uType = MB_OK;
        break;
    case OSMB_OKCANCEL:
        uType = MB_OKCANCEL;
        break;
    case OSMB_YESNO:
        uType = MB_YESNO;
        break;
    default:
        uType = MB_OK;
        break;
    }

    // AG: Of course, the using of the static global variable sWindowHandleForMessageBox
    // instead of using the field mWindowHandle of the class LLWindowWin32 looks strange.
    // But in fact, the function OSMessageBoxWin32() doesn't have access to gViewerWindow
    // because the former is implemented in the library llwindow which is abstract enough.
    //
    // "This is why I'm doing it this way, instead of what you would think would be more obvious..."
    // (C) Nat Goodspeed
    if (!IsWindow(sWindowHandleForMessageBox))
    {
        sWindowHandleForMessageBox = NULL;
    }
    int retval_win = MessageBoxW(sWindowHandleForMessageBox, // HWND
                                 ll_convert_string_to_wide(text).c_str(),
                                 ll_convert_string_to_wide(caption).c_str(),
                                 uType);
    S32 retval;

    switch(retval_win)
    {
    case IDYES:
        retval = OSBTN_YES;
        break;
    case IDNO:
        retval = OSBTN_NO;
        break;
    case IDOK:
        retval = OSBTN_OK;
        break;
    case IDCANCEL:
        retval = OSBTN_CANCEL;
        break;
    default:
        retval = OSBTN_CANCEL;
        break;
    }

    return retval;
}

void shell_open(const std::string &file, bool async)
{
    std::wstring url_utf16 = ll_convert(file);

    // let the OS decide what to use to open the URL
    SHELLEXECUTEINFO sei = {sizeof(sei)};
    // NOTE: this assumes that SL will stick around long enough to complete the DDE message exchange
    // necessary for ShellExecuteEx to complete
    if (async)
    {
        sei.fMask = SEE_MASK_ASYNCOK;
    }
    sei.nShow  = SW_SHOWNORMAL;
    sei.lpVerb = L"open";
    sei.lpFile = url_utf16.c_str();
    ShellExecuteEx(&sei);
}

void LLWindowWin32::spawnWebBrowser(const std::string& escaped_url, bool async)
{
    bool found = false;
    S32 i;
    for (i = 0; i < gURLProtocolWhitelistCount; i++)
    {
        if (escaped_url.find(gURLProtocolWhitelist[i]) == 0)
        {
            found = true;
            break;
        }
    }

    if (!found)
    {
        LL_WARNS("Window") << "spawn_web_browser() called for url with protocol not on whitelist: " << escaped_url << LL_ENDL;
        return;
    }

    LL_INFOS("Window") << "Opening URL " << escaped_url << LL_ENDL;

    // replaced ShellExecute code with ShellExecuteEx since ShellExecute doesn't work
    // reliablly on Vista.

    shell_open(escaped_url, async);
}

void LLWindowWin32::openFolder(const std::string &path)
{
    shell_open(path, false);
}

/*
    Make the raw keyboard data available - used to poke through to LLQtWebKit so
    that Qt/Webkit has access to the virtual keycodes etc. that it needs
*/
LLSD LLWindowWin32::getNativeKeyData() const
{
    LLSD result = LLSD::emptyMap();

    result["scan_code"] = (S32)mKeyScanCode;
    result["virtual_key"] = (S32)mKeyVirtualKey;
    result["msg"] = ll_sd_from_U32(mRawMsg);
    result["w_param"] = ll_sd_from_U32(mRawWParam);
    result["l_param"] = ll_sd_from_U32(mRawLParam);

    return result;
}

bool LLWindowWin32::dialogColorPicker( F32 *r, F32 *g, F32 *b )
{
    bool retval = false;

    static CHOOSECOLOR cc;
    static COLORREF crCustColors[16];
    cc.lStructSize = sizeof(CHOOSECOLOR);
    cc.hwndOwner = mWindowHandle;
    cc.hInstance = NULL;
    cc.rgbResult = RGB ((*r * 255.f),(*g *255.f),(*b * 255.f));
    //cc.rgbResult = RGB (0x80,0x80,0x80);
    cc.lpCustColors = crCustColors;
    cc.Flags = CC_RGBINIT | CC_FULLOPEN;
    cc.lCustData = 0;
    cc.lpfnHook = NULL;
    cc.lpTemplateName = NULL;

    // This call is modal, so pause agent
    //send_agent_pause();   // this is in newview and we don't want to set up a dependency
    {
        retval = ChooseColor(&cc);
    }
    //send_agent_resume();  // this is in newview and we don't want to set up a dependency

    *b = ((F32)((cc.rgbResult >> 16) & 0xff)) / 255.f;

    *g = ((F32)((cc.rgbResult >> 8) & 0xff)) / 255.f;

    *r = ((F32)(cc.rgbResult & 0xff)) / 255.f;

    return (retval);
}

void *LLWindowWin32::getPlatformWindow()
{
    return (void*)mWindowHandle;
}

void LLWindowWin32::bringToFront()
{
    mWindowThread->post([=]()
        {
            BringWindowToTop(mWindowHandle);
        });
}

// set (OS) window focus back to the client
void LLWindowWin32::focusClient()
{
    mWindowThread->post([=]()
        {
            SetFocus(mWindowHandle);
        });
}

void LLWindowWin32::allowLanguageTextInput(LLPreeditor *preeditor, bool b)
{
    if (b == sLanguageTextInputAllowed || !LLWinImm::isAvailable())
    {
        return;
    }

    if (preeditor != mPreeditor && !b)
    {
        // This condition may occur with a call to
        // setEnabled(bool) from LLTextEditor or LLLineEditor
        // when the control is not focused.
        // We need to silently ignore the case so that
        // the language input status of the focused control
        // is not disturbed.
        return;
    }

    // Take care of old and new preeditors.
    if (preeditor != mPreeditor || !b)
    {
        if (sLanguageTextInputAllowed)
        {
            interruptLanguageTextInput();
        }
        mPreeditor = (b ? preeditor : NULL);
    }

    sLanguageTextInputAllowed = b;

    if (sLanguageTextInputAllowed)
    {
        mWindowThread->post([=]()
        {
            // Allowing: Restore the previous IME status, so that the user has a feeling that the previous
            // text input continues naturally.  Be careful, however, the IME status is meaningful only during the user keeps
            // using same Input Locale (aka Keyboard Layout).
            if (sWinIMEOpened && GetKeyboardLayout(0) == sWinInputLocale)
            {
                HIMC himc = LLWinImm::getContext(mWindowHandle);
                LLWinImm::setOpenStatus(himc, true);
                LLWinImm::setConversionStatus(himc, sWinIMEConversionMode, sWinIMESentenceMode);
                LLWinImm::releaseContext(mWindowHandle, himc);
            }
        });
    }
    else
    {
        mWindowThread->post([=]()
        {
            // Disallowing: Turn off the IME so that succeeding key events bypass IME and come to us directly.
            // However, do it after saving the current IME  status.  We need to restore the status when
            //   allowing language text input again.
            sWinInputLocale = GetKeyboardLayout(0);
            sWinIMEOpened = LLWinImm::isIME(sWinInputLocale);
            if (sWinIMEOpened)
            {
                HIMC himc = LLWinImm::getContext(mWindowHandle);
                sWinIMEOpened = LLWinImm::getOpenStatus(himc);
                if (sWinIMEOpened)
                {
                    LLWinImm::getConversionStatus(himc, &sWinIMEConversionMode, &sWinIMESentenceMode);

                    // We need both ImmSetConversionStatus and ImmSetOpenStatus here to surely disable IME's
                    // keyboard hooking, because Some IME reacts only on the former and some other on the latter...
                    LLWinImm::setConversionStatus(himc, IME_CMODE_NOCONVERSION, sWinIMESentenceMode);
                    LLWinImm::setOpenStatus(himc, false);
                }
                LLWinImm::releaseContext(mWindowHandle, himc);
            }
        });
    }
}

void LLWindowWin32::fillCandidateForm(const LLCoordGL& caret, const LLRect& bounds,
        CANDIDATEFORM *form)
{
    LLCoordWindow caret_coord, top_left, bottom_right;
    convertCoords(caret, &caret_coord);
    convertCoords(LLCoordGL(bounds.mLeft, bounds.mTop), &top_left);
    convertCoords(LLCoordGL(bounds.mRight, bounds.mBottom), &bottom_right);

    memset(form, 0, sizeof(CANDIDATEFORM));
    form->dwStyle = CFS_EXCLUDE;
    form->ptCurrentPos.x = caret_coord.mX;
    form->ptCurrentPos.y = caret_coord.mY;
    form->rcArea.left   = top_left.mX;
    form->rcArea.top    = top_left.mY;
    form->rcArea.right  = bottom_right.mX;
    form->rcArea.bottom = bottom_right.mY;
}


// Put the IME window at the right place (near current text input).   Point coordinates should be the top of the current text line.
void LLWindowWin32::setLanguageTextInput( const LLCoordGL & position )
{
    if (sLanguageTextInputAllowed && LLWinImm::isAvailable())
    {
        HIMC himc = LLWinImm::getContext(mWindowHandle);

        LLCoordWindow win_pos;
        convertCoords( position, &win_pos );

        if ( win_pos.mX >= 0 && win_pos.mY >= 0 &&
            (win_pos.mX != sWinIMEWindowPosition.mX) || (win_pos.mY != sWinIMEWindowPosition.mY) )
        {
            COMPOSITIONFORM ime_form;
            memset( &ime_form, 0, sizeof(ime_form) );
            ime_form.dwStyle = CFS_POINT;
            ime_form.ptCurrentPos.x = win_pos.mX;
            ime_form.ptCurrentPos.y = win_pos.mY;

            LLWinImm::setCompositionWindow( himc, &ime_form );

            sWinIMEWindowPosition = win_pos;
        }

        LLWinImm::releaseContext(mWindowHandle, himc);
    }
}


void LLWindowWin32::fillCharPosition(const LLCoordGL& caret, const LLRect& bounds, const LLRect& control,
        IMECHARPOSITION *char_position)
{
    LLCoordScreen caret_coord, top_left, bottom_right;
    convertCoords(caret, &caret_coord);
    convertCoords(LLCoordGL(bounds.mLeft, bounds.mTop), &top_left);
    convertCoords(LLCoordGL(bounds.mRight, bounds.mBottom), &bottom_right);

    char_position->pt.x = caret_coord.mX;
    char_position->pt.y = top_left.mY;  // Windows wants the coordinate of upper left corner of a character...
    char_position->cLineHeight = bottom_right.mY - top_left.mY;
    char_position->rcDocument.left   = top_left.mX;
    char_position->rcDocument.top    = top_left.mY;
    char_position->rcDocument.right  = bottom_right.mX;
    char_position->rcDocument.bottom = bottom_right.mY;
}

void LLWindowWin32::fillCompositionLogfont(LOGFONT *logfont)
{
    // Our font is a list of FreeType recognized font files that may
    // not have a corresponding ones in Windows' fonts.  Hence, we
    // can't simply tell Windows which font we are using.  We will
    // notify a _standard_ font for a current input locale instead.
    // We use a hard-coded knowledge about the Windows' standard
    // configuration to do so...

    memset(logfont, 0, sizeof(LOGFONT));

    const WORD lang_id = LOWORD(GetKeyboardLayout(0));
    switch (PRIMARYLANGID(lang_id))
    {
    case LANG_CHINESE:
        // We need to identify one of two Chinese fonts.
        switch (SUBLANGID(lang_id))
        {
        case SUBLANG_CHINESE_SIMPLIFIED:
        case SUBLANG_CHINESE_SINGAPORE:
            logfont->lfCharSet = GB2312_CHARSET;
            lstrcpy(logfont->lfFaceName, TEXT("SimHei"));
            break;
        case SUBLANG_CHINESE_TRADITIONAL:
        case SUBLANG_CHINESE_HONGKONG:
        case SUBLANG_CHINESE_MACAU:
        default:
            logfont->lfCharSet = CHINESEBIG5_CHARSET;
            lstrcpy(logfont->lfFaceName, TEXT("MingLiU"));
            break;
        }
        break;
    case LANG_JAPANESE:
        logfont->lfCharSet = SHIFTJIS_CHARSET;
        lstrcpy(logfont->lfFaceName, TEXT("MS Gothic"));
        break;
    case LANG_KOREAN:
        logfont->lfCharSet = HANGUL_CHARSET;
        lstrcpy(logfont->lfFaceName, TEXT("Gulim"));
        break;
    default:
        logfont->lfCharSet = ANSI_CHARSET;
        lstrcpy(logfont->lfFaceName, TEXT("Tahoma"));
        break;
    }

    logfont->lfHeight = mPreeditor->getPreeditFontSize();
    logfont->lfWeight = FW_NORMAL;
}

U32 LLWindowWin32::fillReconvertString(const LLWString &text,
    S32 focus, S32 focus_length, RECONVERTSTRING *reconvert_string)
{
    const llutf16string text_utf16 = wstring_to_utf16str(text);
    const DWORD required_size = sizeof(RECONVERTSTRING) + (static_cast<DWORD>(text_utf16.length()) + 1) * sizeof(WCHAR);
    if (reconvert_string && reconvert_string->dwSize >= required_size)
    {
        const DWORD focus_utf16_at = wstring_utf16_length(text, 0, focus);
        const DWORD focus_utf16_length = wstring_utf16_length(text, focus, focus_length);

        reconvert_string->dwVersion = 0;
        reconvert_string->dwStrLen = static_cast<DWORD>(text_utf16.length());
        reconvert_string->dwStrOffset = sizeof(RECONVERTSTRING);
        reconvert_string->dwCompStrLen = focus_utf16_length;
        reconvert_string->dwCompStrOffset = focus_utf16_at * sizeof(WCHAR);
        reconvert_string->dwTargetStrLen = 0;
        reconvert_string->dwTargetStrOffset = focus_utf16_at * sizeof(WCHAR);

        const LPWSTR text = (LPWSTR)((BYTE *)reconvert_string + sizeof(RECONVERTSTRING));
        memcpy(text, text_utf16.c_str(), (text_utf16.length() + 1) * sizeof(WCHAR));
    }
    return required_size;
}

void LLWindowWin32::updateLanguageTextInputArea()
{
    if (!mPreeditor || !LLWinImm::isAvailable())
    {
        return;
    }

    LLCoordGL caret_coord;
    LLRect preedit_bounds;
    if (mPreeditor->getPreeditLocation(-1, &caret_coord, &preedit_bounds, NULL))
    {
        mLanguageTextInputPointGL = caret_coord;
        mLanguageTextInputAreaGL = preedit_bounds;

        CANDIDATEFORM candidate_form;
        fillCandidateForm(caret_coord, preedit_bounds, &candidate_form);

        HIMC himc = LLWinImm::getContext(mWindowHandle);
        // Win32 document says there may be up to 4 candidate windows.
        // This magic number 4 appears only in the document, and
        // there are no constant/macro for the value...
        for (int i = 3; i >= 0; --i)
        {
            candidate_form.dwIndex = i;
            LLWinImm::setCandidateWindow(himc, &candidate_form);
        }
        LLWinImm::releaseContext(mWindowHandle, himc);
    }
}

void LLWindowWin32::interruptLanguageTextInput()
{
    ASSERT_MAIN_THREAD();
    if (mPreeditor && LLWinImm::isAvailable())
    {
        HIMC himc = LLWinImm::getContext(mWindowHandle);
        LLWinImm::notifyIME(himc, NI_COMPOSITIONSTR, CPS_COMPLETE, 0);
        LLWinImm::releaseContext(mWindowHandle, himc);
    }
}

void LLWindowWin32::handleStartCompositionMessage()
{
    // Let IME know the font to use in feedback UI.
    LOGFONT logfont;
    fillCompositionLogfont(&logfont);
    HIMC himc = LLWinImm::getContext(mWindowHandle);
    LLWinImm::setCompositionFont(himc, &logfont);
    LLWinImm::releaseContext(mWindowHandle, himc);
}

// Handle WM_IME_COMPOSITION message.

void LLWindowWin32::handleCompositionMessage(const U32 indexes)
{
    if (!mPreeditor)
    {
        return;
    }
    bool needs_update = false;
    LLWString result_string;
    LLWString preedit_string;
    S32 preedit_string_utf16_length = 0;
    LLPreeditor::segment_lengths_t preedit_segment_lengths;
    LLPreeditor::standouts_t preedit_standouts;

    // Step I: Receive details of preedits from IME.

    HIMC himc = LLWinImm::getContext(mWindowHandle);

    if (indexes & GCS_RESULTSTR)
    {
        LONG size = LLWinImm::getCompositionString(himc, GCS_RESULTSTR, NULL, 0);
        if (size >= 0)
        {
            const LPWSTR data = new WCHAR[size / sizeof(WCHAR) + 1];
            size = LLWinImm::getCompositionString(himc, GCS_RESULTSTR, data, size);
            if (size > 0)
            {
                result_string = utf16str_to_wstring(llutf16string(data, size / sizeof(WCHAR)));
            }
            delete[] data;
            needs_update = true;
        }
    }

    if (indexes & GCS_COMPSTR)
    {
        LONG size = LLWinImm::getCompositionString(himc, GCS_COMPSTR, NULL, 0);
        if (size >= 0)
        {
            const LPWSTR data = new WCHAR[size / sizeof(WCHAR) + 1];
            size = LLWinImm::getCompositionString(himc, GCS_COMPSTR, data, size);
            if (size > 0)
            {
                preedit_string_utf16_length = size / sizeof(WCHAR);
                preedit_string = utf16str_to_wstring(llutf16string(data, size / sizeof(WCHAR)));
            }
            delete[] data;
            needs_update = true;
        }
    }

    if ((indexes & GCS_COMPCLAUSE) && preedit_string.length() > 0)
    {
        LONG size = LLWinImm::getCompositionString(himc, GCS_COMPCLAUSE, NULL, 0);
        if (size > 0)
        {
            const LPDWORD data = new DWORD[size / sizeof(DWORD)];
            size = LLWinImm::getCompositionString(himc, GCS_COMPCLAUSE, data, size);
            if (size >= sizeof(DWORD) * 2
                && data[0] == 0 && data[size / sizeof(DWORD) - 1] == preedit_string_utf16_length)
            {
                preedit_segment_lengths.resize(size / sizeof(DWORD) - 1);
                S32 offset = 0;
                for (U32 i = 0; i < preedit_segment_lengths.size(); i++)
                {
                    const S32 length = wstring_wstring_length_from_utf16_length(preedit_string, offset, data[i + 1] - data[i]);
                    preedit_segment_lengths[i] = length;
                    offset += length;
                }
            }
            delete[] data;
        }
    }

    if ((indexes & GCS_COMPATTR) && preedit_segment_lengths.size() > 1)
    {
        LONG size = LLWinImm::getCompositionString(himc, GCS_COMPATTR, NULL, 0);
        if (size > 0)
        {
            const LPBYTE data = new BYTE[size / sizeof(BYTE)];
            size = LLWinImm::getCompositionString(himc, GCS_COMPATTR, data, size);
            if (size == preedit_string_utf16_length)
            {
                preedit_standouts.assign(preedit_segment_lengths.size(), false);
                S32 offset = 0;
                for (U32 i = 0; i < preedit_segment_lengths.size(); i++)
                {
                    if (ATTR_TARGET_CONVERTED == data[offset] || ATTR_TARGET_NOTCONVERTED == data[offset])
                    {
                        preedit_standouts[i] = true;
                    }
                    offset += wstring_utf16_length(preedit_string, offset, preedit_segment_lengths[i]);
                }
            }
            delete[] data;
        }
    }

    S32 caret_position = static_cast<S32>(preedit_string.length());
    if (indexes & GCS_CURSORPOS)
    {
        const S32 caret_position_utf16 = LLWinImm::getCompositionString(himc, GCS_CURSORPOS, NULL, 0);
        if (caret_position_utf16 >= 0 && caret_position <= preedit_string_utf16_length)
        {
            caret_position = wstring_wstring_length_from_utf16_length(preedit_string, 0, caret_position_utf16);
        }
    }

    if (indexes == 0)
    {
        // I'm not sure this condition really happens, but
        // Windows SDK document says it is an indication
        // of "reset everything."
        needs_update = true;
    }

    LLWinImm::releaseContext(mWindowHandle, himc);

    // Step II: Update the active preeditor.

    if (needs_update)
    {
        if (preedit_string.length() != 0 || result_string.length() != 0)
        {
            mPreeditor->resetPreedit();
        }

        if (result_string.length() > 0)
        {
            for (LLWString::const_iterator i = result_string.begin(); i != result_string.end(); i++)
            {
                mPreeditor->handleUnicodeCharHere(*i);
            }
        }

        if (preedit_string.length() == 0)
        {
            preedit_segment_lengths.clear();
            preedit_standouts.clear();
        }
        else
        {
            if (preedit_segment_lengths.size() == 0)
            {
                preedit_segment_lengths.assign(1, static_cast<S32>(preedit_string.length()));
            }
            if (preedit_standouts.size() == 0)
            {
                preedit_standouts.assign(preedit_segment_lengths.size(), false);
            }
        }
        mPreeditor->updatePreedit(preedit_string, preedit_segment_lengths, preedit_standouts, caret_position);

        // Some IME doesn't query char position after WM_IME_COMPOSITION,
        // so we need to update them actively.
        updateLanguageTextInputArea();
    }
}

// Given a text and a focus range, find_context finds and returns a
// surrounding context of the focused subtext.  A variable pointed
// to by offset receives the offset in llwchars of the beginning of
// the returned context string in the given wtext.

static LLWString find_context(const LLWString & wtext, S32 focus, S32 focus_length, S32 *offset)
{
    static const S32 CONTEXT_EXCESS = 30;   // This value is by experiences.

    const S32 e = llmin((S32) wtext.length(), focus + focus_length + CONTEXT_EXCESS);
    S32 end = focus + focus_length;
    while (end < e && '\n' != wtext[end])
    {
        end++;
    }

    const S32 s = llmax(0, focus - CONTEXT_EXCESS);
    S32 start = focus;
    while (start > s && '\n' != wtext[start - 1])
    {
        --start;
    }

    *offset = start;
    return wtext.substr(start, end - start);
}

// final stage of handling drop requests - both from WM_DROPFILES message
// for files and via IDropTarget interface requests.
LLWindowCallbacks::DragNDropResult LLWindowWin32::completeDragNDropRequest( const LLCoordGL gl_coord, const MASK mask, LLWindowCallbacks::DragNDropAction action, const std::string url )
{
    ASSERT_MAIN_THREAD();
    return mCallbacks->handleDragNDrop( this, gl_coord, mask, action, url );
}

// Handle WM_IME_REQUEST message.
// If it handled the message, returns true.  Otherwise, false.
// When it handled the message, the value to be returned from
// the Window Procedure is set to *result.

bool LLWindowWin32::handleImeRequests(WPARAM request, LPARAM param, LRESULT *result)
{
    if ( mPreeditor )
    {
        switch (request)
        {
            case IMR_CANDIDATEWINDOW:       // http://msdn2.microsoft.com/en-us/library/ms776080.aspx
            {
                LLCoordGL caret_coord;
                LLRect preedit_bounds;
                mPreeditor->getPreeditLocation(-1, &caret_coord, &preedit_bounds, NULL);

                CANDIDATEFORM *const form = (CANDIDATEFORM *)param;
                DWORD const dwIndex = form->dwIndex;
                fillCandidateForm(caret_coord, preedit_bounds, form);
                form->dwIndex = dwIndex;

                *result = 1;
                return true;
            }
            case IMR_QUERYCHARPOSITION:
            {
                IMECHARPOSITION *const char_position = (IMECHARPOSITION *)param;

                // char_position->dwCharPos counts in number of
                // WCHARs, i.e., UTF-16 encoding units, so we can't simply pass the
                // number to getPreeditLocation.

                const LLWString & wtext = mPreeditor->getPreeditString();
                S32 preedit, preedit_length;
                mPreeditor->getPreeditRange(&preedit, &preedit_length);
                LLCoordGL caret_coord;
                LLRect preedit_bounds, text_control;
                const S32 position = wstring_wstring_length_from_utf16_length(wtext, preedit, char_position->dwCharPos);

                if (!mPreeditor->getPreeditLocation(position, &caret_coord, &preedit_bounds, &text_control))
                {
                    LL_WARNS("Window") << "*** IMR_QUERYCHARPOSITON called but getPreeditLocation failed." << LL_ENDL;
                    return false;
                }

                fillCharPosition(caret_coord, preedit_bounds, text_control, char_position);

                *result = 1;
                return true;
            }
            case IMR_COMPOSITIONFONT:
            {
                fillCompositionLogfont((LOGFONT *)param);

                *result = 1;
                return true;
            }
            case IMR_RECONVERTSTRING:
            {
                mPreeditor->resetPreedit();
                const LLWString & wtext = mPreeditor->getPreeditString();
                S32 select, select_length;
                mPreeditor->getSelectionRange(&select, &select_length);

                S32 context_offset;
                const LLWString context = find_context(wtext, select, select_length, &context_offset);

                RECONVERTSTRING * const reconvert_string = (RECONVERTSTRING *)param;
                const U32 size = fillReconvertString(context, select - context_offset, select_length, reconvert_string);
                if (reconvert_string)
                {
                    if (select_length == 0)
                    {
                        // Let the IME to decide the reconversion range, and
                        // adjust the reconvert_string structure accordingly.
                        HIMC himc = LLWinImm::getContext(mWindowHandle);
                        const bool adjusted = LLWinImm::setCompositionString(himc,
                                    SCS_QUERYRECONVERTSTRING, reconvert_string, size, NULL, 0);
                        LLWinImm::releaseContext(mWindowHandle, himc);
                        if (adjusted)
                        {
                            const llutf16string & text_utf16 = wstring_to_utf16str(context);
                            const S32 new_preedit_start = reconvert_string->dwCompStrOffset / sizeof(WCHAR);
                            const S32 new_preedit_end = new_preedit_start + reconvert_string->dwCompStrLen;
                            select = utf16str_wstring_length(text_utf16, new_preedit_start);
                            select_length = utf16str_wstring_length(text_utf16, new_preedit_end) - select;
                            select += context_offset;
                        }
                    }
                    mPreeditor->markAsPreedit(select, select_length);
                }

                *result = size;
                return true;
            }
            case IMR_CONFIRMRECONVERTSTRING:
            {
                *result = 0;
                return true;
            }
            case IMR_DOCUMENTFEED:
            {
                const LLWString & wtext = mPreeditor->getPreeditString();
                S32 preedit, preedit_length;
                mPreeditor->getPreeditRange(&preedit, &preedit_length);

                S32 context_offset;
                LLWString context = find_context(wtext, preedit, preedit_length, &context_offset);
                preedit -= context_offset;
                preedit_length = llmin(preedit_length, (S32)context.length() - preedit);
                if (preedit_length > 0 && preedit >= 0)
                {
                    // IMR_DOCUMENTFEED may be called when we have an active preedit.
                    // We should pass the context string *excluding* the preedit string.
                    // Otherwise, some IME are confused.
                    context.erase(preedit, preedit_length);
                }

                RECONVERTSTRING *reconvert_string = (RECONVERTSTRING *)param;
                *result = fillReconvertString(context, preedit, 0, reconvert_string);
                return true;
            }
            default:
                return false;
        }
    }

    return false;
}

//static
void LLWindowWin32::setDPIAwareness()
{
    HMODULE hShcore = LoadLibrary(L"shcore.dll");
    if (hShcore != NULL)
    {
        SetProcessDpiAwarenessType pSPDA;
        pSPDA = (SetProcessDpiAwarenessType)GetProcAddress(hShcore, "SetProcessDpiAwareness");
        if (pSPDA)
        {

            HRESULT hr = pSPDA(PROCESS_PER_MONITOR_DPI_AWARE);
            if (hr != S_OK)
            {
                LL_WARNS() << "SetProcessDpiAwareness() function returned an error. Will use legacy DPI awareness API of Win XP/7" << LL_ENDL;
            }
        }
        FreeLibrary(hShcore);
    }
    else
    {
        LL_WARNS() << "Could not load shcore.dll library (included by <ShellScalingAPI.h> from Win 8.1 SDK. Will use legacy DPI awareness API of Win XP/7" << LL_ENDL;
    }
}

void* LLWindowWin32::getDirectInput8()
{
    return &gDirectInput8;
}

bool LLWindowWin32::getInputDevices(U32 device_type_filter,
                                    std::function<bool(std::string&, LLSD&, void*)> osx_callback,
                                    void * di8_devices_callback,
                                    void* userdata)
{
    if (gDirectInput8 != NULL)
    {
        // Enumerate devices
        HRESULT status = gDirectInput8->EnumDevices(
            (DWORD) device_type_filter,        // DWORD dwDevType,
            (LPDIENUMDEVICESCALLBACK)di8_devices_callback,  // LPDIENUMDEVICESCALLBACK lpCallback, // BOOL DIEnumDevicesCallback( LPCDIDEVICEINSTANCE lpddi, LPVOID pvRef ) // BOOL CALLBACK DinputDevice::DevicesCallback
            (LPVOID*)userdata, // LPVOID pvRef
            DIEDFL_ATTACHEDONLY       // DWORD dwFlags
            );

        return status == DI_OK;
    }
    return false;
}

F32 LLWindowWin32::getSystemUISize()
{
    F32 scale_value = 1.f;
    HWND hWnd = (HWND)getPlatformWindow();
    HDC hdc = GetDC(hWnd);
    HMONITOR hMonitor;
    HANDLE hProcess = GetCurrentProcess();
    PROCESS_DPI_AWARENESS dpi_awareness;

    HMODULE hShcore = LoadLibrary(L"shcore.dll");

    if (hShcore != NULL)
    {
        GetProcessDpiAwarenessType pGPDA;
        pGPDA = (GetProcessDpiAwarenessType)GetProcAddress(hShcore, "GetProcessDpiAwareness");
        GetDpiForMonitorType pGDFM;
        pGDFM = (GetDpiForMonitorType)GetProcAddress(hShcore, "GetDpiForMonitor");
        if (pGPDA != NULL && pGDFM != NULL)
        {
            pGPDA(hProcess, &dpi_awareness);
            if (dpi_awareness == PROCESS_PER_MONITOR_DPI_AWARE)
            {
                POINT    pt;
                UINT     dpix = 0, dpiy = 0;
                HRESULT  hr = E_FAIL;
                RECT     rect;

                GetWindowRect(hWnd, &rect);
                // Get the DPI for the monitor, on which the center of window is displayed and set the scaling factor
                pt.x = (rect.left + rect.right) / 2;
                pt.y = (rect.top + rect.bottom) / 2;
                hMonitor = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
                hr = pGDFM(hMonitor, MDT_EFFECTIVE_DPI, &dpix, &dpiy);
                if (hr == S_OK)
                {
                    scale_value = F32(dpix) / F32(USER_DEFAULT_SCREEN_DPI);
                }
                else
                {
                    LL_WARNS() << "Could not determine DPI for monitor. Setting scale to default 100 %" << LL_ENDL;
                    scale_value = 1.0f;
                }
            }
            else
            {
                LL_WARNS() << "Process is not per-monitor DPI-aware. Setting scale to default 100 %" << LL_ENDL;
                scale_value = 1.0f;
            }
        }
        FreeLibrary(hShcore);
    }
    else
    {
        LL_WARNS() << "Could not load shcore.dll library (included by <ShellScalingAPI.h> from Win 8.1 SDK). Using legacy DPI awareness API of Win XP/7" << LL_ENDL;
        scale_value = F32(GetDeviceCaps(hdc, LOGPIXELSX)) / F32(USER_DEFAULT_SCREEN_DPI);
    }

    ReleaseDC(hWnd, hdc);
    return scale_value;
}

//static
std::vector<std::string> LLWindowWin32::getDisplaysResolutionList()
{
    return sMonitorInfo.getResolutionsList();
}

//static
std::vector<std::string> LLWindowWin32::getDynamicFallbackFontList()
{
    // Fonts previously in getFontListSans() have moved to fonts.xml.
    return std::vector<std::string>();
}
#endif // LL_WINDOWS

inline LLWindowWin32::LLWindowWin32Thread::LLWindowWin32Thread()
    : LL::ThreadPool("Window Thread", 1, MAX_QUEUE_SIZE, true /*should be false, temporary workaround for SL-18721*/)
{
    LL::ThreadPool::start();
}

void LLWindowWin32::LLWindowWin32Thread::close()
{
    LL::ThreadPool::close();
    if (!mShuttingDown)
    {
        LL_WARNS() << "Closing window thread without using destroy_window_handler" << LL_ENDL;
        // Workaround for SL-18721 in case window closes too early and abruptly
        LLSplashScreen::show();
        LLSplashScreen::update("..."); // will be updated later
        mShuttingDown = true;
    }
}


/**
 * LogChange is to log changes in status while trying to avoid spamming the
 * log with repeated messages, especially in a tight loop. It refuses to log
 * a continuous run of identical messages, but logs every time the message
 * changes. (It will happily spam when messages quickly bounce back and
 * forth.)
 */
class LogChange
{
public:
    LogChange(const std::string& tag):
        mTag(tag)
    {}

    template <typename... Items>
    void always(Items&&... items)
    {
        // This odd construct ensures that the stringize() call is only
        // executed if DEBUG logging is enabled for the passed tag.
        LL_DEBUGS(mTag.c_str());
        log(LL_CONT, stringize(std::forward<Items>(items)...));
        LL_ENDL;
    }

    template <typename... Items>
    void onChange(Items&&... items)
    {
        LL_DEBUGS(mTag.c_str());
        auto str = stringize(std::forward<Items>(items)...);
        if (str != mPrev)
        {
            log(LL_CONT, str);
        }
        LL_ENDL;
    }

private:
    void log(std::ostream& out, const std::string& message)
    {
        mPrev = message;
        out << message;
    }
    std::string mTag;
    std::string mPrev;
};

void LLWindowWin32::LLWindowWin32Thread::checkDXMem()
{
    if (!mGLReady || mGotGLBuffer) { return; }

    if ((gGLManager.mHasAMDAssociations || gGLManager.mHasNVXGpuMemoryInfo) && gGLManager.mVRAM != 0)
    { // OpenGL already told us the memory budget, don't ask DX
        mGotGLBuffer = true;
        return;
    }

    IDXGIFactory4* p_factory = nullptr;

    HRESULT res = CreateDXGIFactory1(__uuidof(IDXGIFactory4), (void**)&p_factory);

    if (FAILED(res))
    {
        LL_WARNS() << "CreateDXGIFactory1 failed: 0x" << std::hex << res << LL_ENDL;
    }
    else
    {
        IDXGIAdapter3* p_dxgi_adapter = nullptr;
        UINT graphics_adapter_index = 0;
        while (true)
        {
            res = p_factory->EnumAdapters(graphics_adapter_index, reinterpret_cast<IDXGIAdapter**>(&p_dxgi_adapter));
            if (FAILED(res))
            {
                if (graphics_adapter_index == 0)
                {
                    LL_WARNS() << "EnumAdapters failed: 0x" << std::hex << res << LL_ENDL;
                }
            }
            else
            {
                if (graphics_adapter_index == 0) // Should it check largest one isntead of first?
                {
                    DXGI_QUERY_VIDEO_MEMORY_INFO info;
                    p_dxgi_adapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &info);

                    // Alternatively use GetDesc from below to get adapter's memory
                    UINT64 budget_mb = info.Budget / (1024 * 1024);
                    if (gGLManager.mVRAM < (S32)budget_mb)
                    {
                        gGLManager.mVRAM = (S32)budget_mb;
                        LL_INFOS("RenderInit") << "New VRAM Budget (DX9): " << gGLManager.mVRAM << " MB" << LL_ENDL;
                    }
                    else
                    {
                        LL_INFOS("RenderInit") << "VRAM Budget (DX9): " << budget_mb
                            << " MB, current (WMI): " << gGLManager.mVRAM << " MB" << LL_ENDL;
                    }
                }

                DXGI_ADAPTER_DESC desc;
                p_dxgi_adapter->GetDesc(&desc);
                std::wstring description_w((wchar_t*)desc.Description);
                std::string description = ll_convert_wide_to_string(description_w);
                LL_INFOS("Window") << "Graphics adapter index: " << graphics_adapter_index << ", "
                    << "Description: " << description << ", "
                    << "DeviceId: " << desc.DeviceId << ", "
                    << "SubSysId: " << desc.SubSysId << ", "
                    << "AdapterLuid: " << desc.AdapterLuid.HighPart << "_" << desc.AdapterLuid.LowPart << ", "
                    << "DedicatedVideoMemory: " << desc.DedicatedVideoMemory / 1024 / 1024 << ", "
                    << "DedicatedSystemMemory: " << desc.DedicatedSystemMemory / 1024 / 1024 << ", "
                    << "SharedSystemMemory: " << desc.SharedSystemMemory / 1024 / 1024 << LL_ENDL;
            }

            if (p_dxgi_adapter)
            {
                p_dxgi_adapter->Release();
                p_dxgi_adapter = NULL;
            }
            else
            {
                break;
            }

            graphics_adapter_index++;
        }
    }

    if (p_factory)
    {
        p_factory->Release();
    }

    mGotGLBuffer = true;
}

void LLWindowWin32::LLWindowWin32Thread::run()
{
    sWindowThreadId = std::this_thread::get_id();
    LogChange logger("Window");

    //as good a place as any to up the MM timer resolution (see ms_sleep)
    //attempt to set timer resolution to 1ms
    TIMECAPS tc;
    if (timeGetDevCaps(&tc, sizeof(TIMECAPS)) == TIMERR_NOERROR)
    {
        timeBeginPeriod(llclamp((U32) 1, tc.wPeriodMin, tc.wPeriodMax));
    }

    while (! getQueue().done())
    {
        LL_PROFILE_ZONE_SCOPED_CATEGORY_WIN32;

        // Check memory budget using DirectX if OpenGL doesn't have the means to tell us
        checkDXMem();

        if (mWindowHandleThrd != 0)
        {
            MSG msg;
            BOOL status;
            if (mhDCThrd == 0)
            {
                LL_PROFILE_ZONE_NAMED_CATEGORY_WIN32("w32t - PeekMessage");
                logger.onChange("PeekMessage(", std::hex, mWindowHandleThrd, ")");
                status = PeekMessage(&msg, mWindowHandleThrd, 0, 0, PM_REMOVE);
            }
            else
            {
                LL_PROFILE_ZONE_NAMED_CATEGORY_WIN32("w32t - GetMessage");
                logger.always("GetMessage(", std::hex, mWindowHandleThrd, ")");
                status = GetMessage(&msg, NULL, 0, 0);
            }
            if (status > 0)
            {
                logger.always("got MSG (", std::hex, msg.hwnd, ", ", msg.message,
                              ", ", msg.wParam, ")");
                TranslateMessage(&msg);
                DispatchMessage(&msg);

                mMessageQueue.pushFront(msg);
            }
        }

        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_WIN32("w32t - Function Queue");
            logger.onChange("runPending()");
            //process any pending functions
            getQueue().runPending();
        }

#if 0
        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_WIN32("w32t - Sleep");
            logger.always("sleep(1)");
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
#endif
    }
}

void LLWindowWin32::LLWindowWin32Thread::wakeAndDestroy()
{
    if (mQueue->isClosed())
    {
        LL_WARNS() << "Tried to close Queue. Win32 thread Queue already closed." << LL_ENDL;
        return;
    }

    mShuttingDown = true;

    // Make sure we don't leave a blank toolbar button.
    // Also hiding window now prevents user from suspending it
    // via some action (like dragging it around)
    ShowWindow(mWindowHandleThrd, SW_HIDE);

    // Schedule destruction
    HWND old_handle = mWindowHandleThrd;
    post([this]()
         {
             if (IsWindow(mWindowHandleThrd))
             {
                 if (mhDCThrd)
                 {
                     if (!ReleaseDC(mWindowHandleThrd, mhDCThrd))
                     {
                         LL_WARNS("Window") << "Release of ghDC failed!" << LL_ENDL;
                     }
                     mhDCThrd = NULL;
                 }

                 // This causes WM_DESTROY to be sent *immediately*
                 if (!destroy_window_handler(mWindowHandleThrd))
                 {
                     LL_WARNS("Window") << "Failed to destroy Window! " << std::hex << GetLastError() << LL_ENDL;
                 }
             }
             else
             {
                 // Something killed the window while we were busy destroying gl or handle somehow got broken
                 LL_WARNS("Window") << "Failed to destroy Window, invalid handle!" << LL_ENDL;
             }
             mWindowHandleThrd = NULL;
             mhDCThrd = NULL;
             mGLReady = false;
         });

    LL_DEBUGS("Window") << "Closing window's pool queue" << LL_ENDL;
    mQueue->close();

    // Post a nonsense user message to wake up the thread in
    // case it is waiting for a getMessage()
    if (old_handle)
    {
        WPARAM wparam{ 0xB0B0 };
        LL_DEBUGS("Window") << "PostMessage(" << std::hex << old_handle
            << ", " << WM_DUMMY_
            << ", " << wparam << ")" << std::dec << LL_ENDL;
        PostMessage(old_handle, WM_DUMMY_, wparam, 0x1337);
    }

    // There are cases where window will refuse to close,
    // can't wait forever on join, check state instead
    LLTimer timeout;
    timeout.setTimerExpirySec(2.0);
    while (!getQueue().done() && !timeout.hasExpired() && mWindowHandleThrd)
    {
        ms_sleep(100);
    }

    if (getQueue().done() || mWindowHandleThrd == NULL)
    {
        // Window is closed, started closing or is cleaning up
        // now wait for our single thread to die.
        if (mWindowHandleThrd)
        {
            LL_INFOS("Window") << "Window is closing, waiting on pool's thread to join, time since post: " << timeout.getElapsedSeconds() << "s" << LL_ENDL;
        }
        else
        {
            LL_DEBUGS("Window") << "Waiting on pool's thread, time since post: " << timeout.getElapsedSeconds() << "s" << LL_ENDL;
        }
        for (auto& pair : mThreads)
        {
            pair.second.join();
        }
    }
    else
    {
        // Something suspended window thread, can't afford to wait forever
        // so kill thread instead
        // Ex: This can happen if user starts dragging window arround (if it
        // was visible) or a modal notification pops up
        LL_WARNS("Window") << "Window is frozen, couldn't perform clean exit" << LL_ENDL;

        for (auto& pair : mThreads)
        {
            // very unsafe
            TerminateThread(pair.second.native_handle(), 0);
            pair.second.detach();
        }
    }
    LL_DEBUGS("Window") << "thread pool shutdown complete" << LL_ENDL;
}

void LLWindowWin32::post(const std::function<void()>& func)
{
    mFunctionQueue.pushFront(func);
}

void LLWindowWin32::postMouseButtonEvent(const std::function<void()>& func)
{
    mMouseQueue.pushFront(func);
}

void LLWindowWin32::kickWindowThread(HWND windowHandle)
{
    if (! windowHandle)
        windowHandle = mWindowHandle;
    if (windowHandle)
    {
        // post a nonsense user message to wake up the Window Thread in
        // case any functions are pending and no windows events came
        // through this frame
        WPARAM wparam{ 0xB0B0 };
        LL_DEBUGS("Window") << "PostMessage(" << std::hex << windowHandle
                            << ", " << WM_DUMMY_
                            << ", " << wparam << ")" << std::dec << LL_ENDL;
        PostMessage(windowHandle, WM_DUMMY_, wparam, 0x1337);
    }
}

void LLWindowWin32::updateWindowRect()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_WIN32;
    //called from window thread
    RECT rect;
    RECT client_rect;

    if (GetWindowRect(mWindowHandle, &rect) &&
        GetClientRect(mWindowHandle, &client_rect))
    {
        post([=]
            {
                mRect = rect;
                mClientRect = client_rect;
            });
    }
}
