/**
 * @file llwindowsdl.cpp
 * @brief SDL implementation of LLWindow class
 * @author This module has many fathers, and it shows.
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

#include "llwindowsdl.h"

#include "llwindowcallbacks.h"
#include "llkeyboardsdl.h"

#include "llerror.h"
#include "llgl.h"
#include "llstring.h"
#include "lldir.h"
#include "llfindlocale.h"
#include "llpreeditor.h"
#include "llsdl.h"

#if LL_LINUX
#ifdef LL_GLIB
#include <glib.h>
#endif

extern "C" {
# include "fontconfig/fontconfig.h"
}

// not necessarily available on random SDL platforms, so #if LL_LINUX
// for execv(), waitpid(), fork()
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>

#if LL_X11
LLWindowSDL::X11_DATA LLWindowSDL::sX11Data = {};
#endif
#if LL_WAYLAND
LLWindowSDL::WAYLAND_DATA LLWindowSDL::sWaylandData = {};
#endif
#endif // LL_LINUX

#if LL_DARWIN
#include <OpenGL/OpenGL.h>
#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>
#include <CoreGraphics/CGDisplayConfiguration.h>

bool LLWindowSDL::sUseMultGL = false;
#endif

bool gHiDPISupport = true;

const S32 MAX_NUM_RESOLUTIONS = 200;
const S32 DEFAULT_REFRESH_RATE = 60;

//
// LLWindowSDL
//

// TOFU HACK -- (*exactly* the same hack as LLWindowMacOSX for a similar
// set of reasons): Stash a pointer to the LLWindowSDL object here and
// maintain in the constructor and destructor.  This assumes that there will
// be only one object of this class at any time.  Currently this is true.
static LLWindowSDL *gWindowImplementation = nullptr;

LLWindowSDL::LLWindowSDL(LLWindowCallbacks* callbacks,
                         const std::string& title, const std::string& name, S32 x, S32 y, S32 width,
                         S32 height, U32 flags,
                         bool fullscreen, bool clearBg,
                         bool enable_vsync, bool use_gl,
                         bool ignore_pixel_depth, U32 fsaa_samples)
        : LLWindow(callbacks, fullscreen, flags),
        mGamma(1.0f), mFlashing(false)
{
    SDL_GL_LoadLibrary(nullptr);

    // Initialize the keyboard
    gKeyboard = new LLKeyboardSDL();
    gKeyboard->setCallbacks(callbacks);

    // Assume 4:3 aspect ratio until we know better
    mNativeAspectRatio = 1024.f / 768.f;

    if (title.empty())
        mWindowTitle = "Second Life";
    else
        mWindowTitle = title;

    // Create the GL context and set it up for windowed or fullscreen, as appropriate.
    if(createContext(x, y, width, height, 32, fullscreen, enable_vsync))
    {
        gGLManager.initWGL();
        gGLManager.initGL();

        //start with arrow cursor
        initCursors();
        setCursor( UI_CURSOR_ARROW );
    }

    stop_glerror();

    // Stash an object pointer for OSMessageBox()
    gWindowImplementation = this;
}

static SDL_Surface *Load_BMP_Resource(const char *basename)
{
    const int PATH_BUFFER_SIZE=1000;
    char path_buffer[PATH_BUFFER_SIZE]; /* Flawfinder: ignore */

    // Figure out where our BMP is living on the disk
    snprintf(path_buffer, PATH_BUFFER_SIZE-1, "%s%sres-sdl%s%s",
             gDirUtilp->getAppRODataDir().c_str(),
             gDirUtilp->getDirDelimiter().c_str(),
             gDirUtilp->getDirDelimiter().c_str(),
             basename);
    path_buffer[PATH_BUFFER_SIZE-1] = '\0';

    return SDL_LoadBMP(path_buffer);
}

void LLWindowSDL::setTitle(const std::string title)
{
    SDL_SetWindowTitle( mWindow, title.c_str() );
}

void LLWindowSDL::tryFindFullscreenSize( int &width, int &height )
{
    LL_INFOS() << "createContext: setting up fullscreen " << width << "x" << height << LL_ENDL;

    // If the requested width or height is 0, find the best default for the monitor.
    if(width == 0 || height == 0)
    {
        // Scan through the list of modes, looking for one which has:
        //      height between 700 and 800
        //      aspect ratio closest to the user's original mode
        S32 resolutionCount = 0;
        LLWindowResolution *resolutionList = getSupportedResolutions(resolutionCount);

        if(resolutionList != nullptr)
        {
            F32 closestAspect = 0;
            U32 closestHeight = 0;
            U32 closestWidth = 0;

            LL_INFOS() << "createContext: searching for a display mode, original aspect is " << mNativeAspectRatio << LL_ENDL;

            for(S32 i=0; i < resolutionCount; i++)
            {
                F32 aspect = (F32)resolutionList[i].mWidth / (F32)resolutionList[i].mHeight;

                LL_INFOS() << "createContext: width " << resolutionList[i].mWidth << " height " << resolutionList[i].mHeight << " aspect " << aspect << LL_ENDL;

                if( (resolutionList[i].mHeight >= 700) && (resolutionList[i].mHeight <= 800) &&
                    (fabs(aspect - mNativeAspectRatio) < fabs(closestAspect - mNativeAspectRatio)))
                {
                    LL_INFOS() << " (new closest mode) " << LL_ENDL;

                    // This is the closest mode we've seen yet.
                    closestWidth = resolutionList[i].mWidth;
                    closestHeight = resolutionList[i].mHeight;
                    closestAspect = aspect;
                }
            }

            width = closestWidth;
            height = closestHeight;
        }
    }

    if(width == 0 || height == 0)
    {
        // Mode search failed for some reason.  Use the old-school default.
        width = 1024;
        height = 768;
    }
}

bool LLWindowSDL::createContext(int x, int y, int width, int height, int bits, bool fullscreen, bool enable_vsync)
{
    LL_INFOS() << "createContext, fullscreen=" << fullscreen << " size=" << width << "x" << height << LL_ENDL;

    // captures don't survive contexts
    mGrabbyKeyFlags = 0;

    if (width == 0)
        width = 1024;
    if (height == 0)
        width = 768;
    if (x == 0)
        x = SDL_WINDOWPOS_UNDEFINED;
    if (y == 0)
        y = SDL_WINDOWPOS_UNDEFINED;

    mFullscreen = fullscreen;

    // Setup default backing colors
    GLint redBits{8}, greenBits{8}, blueBits{8}, alphaBits{8};
    GLint depthBits{24}, stencilBits{8};

    SDL_GL_SetAttribute(SDL_GL_RED_SIZE,   redBits);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, greenBits);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,  blueBits);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, alphaBits);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, depthBits);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, stencilBits);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    SDL_GLContextFlag context_flags{};
    if(LLRender::sGLCoreProfile)
    {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#if LL_DARWIN
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
        context_flags |= SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG;
#endif
    }

    if (gDebugGL)
    {
        context_flags |= SDL_GL_CONTEXT_DEBUG_FLAG;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, context_flags);
    SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);

    if(mFullscreen)
    {
        tryFindFullscreenSize(width, height);
    }

    SDL_PropertiesID props = SDL_CreateProperties();
    SDL_SetStringProperty(props, SDL_PROP_WINDOW_CREATE_TITLE_STRING, mWindowTitle.c_str());
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_X_NUMBER, x);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_Y_NUMBER, y);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, width);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, height);
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_RESIZABLE_BOOLEAN, true);
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_OPENGL_BOOLEAN, true);
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_FULLSCREEN_BOOLEAN, mFullscreen);
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_HIDDEN_BOOLEAN, true);
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_HIGH_PIXEL_DENSITY_BOOLEAN, gHiDPISupport);

    mWindow = SDL_CreateWindowWithProperties(props);
    if (mWindow == nullptr)
    {
        LL_WARNS() << "Window creation failure. SDL: " << SDL_GetError() << LL_ENDL;
        setupFailure("Window creation error", "Error", OSMB_OK);
        SDL_DestroyProperties(props);
        return false;
    }
    SDL_DestroyProperties(props); // Free properties once window is created

    // Create the context
    mContext = SDL_GL_CreateContext(mWindow);
    if(!mContext)
    {
        LL_WARNS() << "Cannot create GL context " << SDL_GetError() << LL_ENDL;
        setupFailure("GL Context creation error", "Error", OSMB_OK);
        return false;
    }

    if (!SDL_GL_MakeCurrent(mWindow, mContext))
    {
        LL_WARNS() << "Failed to make context current. SDL: " << SDL_GetError() << LL_ENDL;
        setupFailure("GL Context failed to set current failure", "Error", OSMB_OK);
        return false;
    }

    const SDL_DisplayMode* displayMode = SDL_GetCurrentDisplayMode(SDL_GetDisplayForWindow(mWindow));
    if(displayMode)
    {
        mRefreshRate = ll_round(displayMode->refresh_rate);
        mNativeAspectRatio = ((F32)displayMode->w) / ((F32)displayMode->h);
        if(mFullscreen)
        {
            mFullscreenWidth = displayMode->w;
            mFullscreenHeight = displayMode->h;
            mFullscreenRefresh = ll_round(displayMode->refresh_rate);

            LL_INFOS() << "Running at " << mFullscreenWidth
            << "x"   << mFullscreenHeight
            << " @ " << mFullscreenRefresh
            << LL_ENDL;
        }
    }
    else
    {
        LL_WARNS() << "Failed to get current display mode and refresh rate" << LL_ENDL;
        mRefreshRate = 0;
        mNativeAspectRatio = 1024.f / 768.f;
        if(mFullscreen) // Fallback to window size
        {
            SDL_GetWindowSize(mWindow, &mFullscreenWidth, &mFullscreenHeight);
            mFullscreenRefresh = -1;
        }
    }

    if(mRefreshRate == 0) // We failed to get a valid refresh rate above
    {
        mRefreshRate = DEFAULT_REFRESH_RATE;
    }

    /* Grab the window manager specific information */
#if LL_LINUX
    if (SDL_strcmp(SDL_GetCurrentVideoDriver(), "x11") == 0)
    {
        LL_INFOS() << "Running under X11" << LL_ENDL;
        mServerProtocol = X11;

        gGLManager.mIsX11 = true;

#if LL_X11
        sX11Data.xdisplay = (Display *)SDL_GetPointerProperty(SDL_GetWindowProperties(mWindow), SDL_PROP_WINDOW_X11_DISPLAY_POINTER, nullptr);
        sX11Data.xwindow = (Window)SDL_GetNumberProperty(SDL_GetWindowProperties(mWindow), SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);
        sX11Data.xscreen = (int)SDL_GetNumberProperty(SDL_GetWindowProperties(mWindow), SDL_PROP_WINDOW_X11_SCREEN_NUMBER, -1);
        if (sX11Data.xdisplay && sX11Data.xwindow)
        {

        }
#endif

        gGLManager.initGLX();
    }
    else if (SDL_strcmp(SDL_GetCurrentVideoDriver(), "wayland") == 0)
    {
        LL_INFOS() << "Running under Wayland" << LL_ENDL;
        mServerProtocol = Wayland;

        gGLManager.mIsWayland = true;

#if LL_WAYLAND
        sWaylandData.display = (struct wl_display *)SDL_GetPointerProperty(SDL_GetWindowProperties(mWindow), SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, nullptr);
        sWaylandData.surface = (struct wl_surface *)SDL_GetPointerProperty(SDL_GetWindowProperties(mWindow), SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, nullptr);
        if (sWaylandData.display && sWaylandData.surface)
        {
        }
#endif

        gGLManager.initEGL();

        // If set (XWayland) remove DISPLAY, this will prompt dullahan to also use Wayland
        if(getenv("DISPLAY"))
        {
            unsetenv("DISPLAY");
        }
    }
#endif

    SDL_GL_GetAttribute(SDL_GL_RED_SIZE, &redBits);
    SDL_GL_GetAttribute(SDL_GL_GREEN_SIZE, &greenBits);
    SDL_GL_GetAttribute(SDL_GL_BLUE_SIZE, &blueBits);
    SDL_GL_GetAttribute(SDL_GL_ALPHA_SIZE, &alphaBits);
    SDL_GL_GetAttribute(SDL_GL_DEPTH_SIZE, &depthBits);
    SDL_GL_GetAttribute(SDL_GL_STENCIL_SIZE, &stencilBits);

    LL_INFOS() << "GL buffer:" << LL_ENDL;
    LL_INFOS() << "  Red Bits " << S32(redBits) << LL_ENDL;
    LL_INFOS() << "  Green Bits " << S32(greenBits) << LL_ENDL;
    LL_INFOS() << "  Blue Bits " << S32(blueBits) << LL_ENDL;
    LL_INFOS() << "  Alpha Bits " << S32(alphaBits) << LL_ENDL;
    LL_INFOS() << "  Depth Bits " << S32(depthBits) << LL_ENDL;
    LL_INFOS() << "  Stencil Bits " << S32(stencilBits) << LL_ENDL;

    GLint colorBits = redBits + greenBits + blueBits + alphaBits;
    if (colorBits < 32)
    {
        setupFailure(
                "Second Life requires True Color (32-bit) to run in a window.\n"
                "Please go to Control Panels -> Display -> Settings and\n"
                "set the screen to 32-bit color.\n"
                "Alternately, if you choose to run fullscreen, Second Life\n"
                "will automatically adjust the screen each time it runs.",
                "Error",
                OSMB_OK);
        return false;
    }

    LL_PROFILER_GPU_CONTEXT;

    // Enable vertical sync
    toggleVSync(enable_vsync);

#if LL_DARWIN
    setUseMultGL(sUseMultGL);

    // Get vram via CGL on macos
    gGLManager.mVRAM = getVramSize();
#endif

    // Set the application icon.
    SDL_Surface* bmpsurface = Load_BMP_Resource("ll_icon.BMP");
    if (bmpsurface)
    {
        SDL_SetWindowIcon(mWindow, bmpsurface);
        SDL_DestroySurface(bmpsurface);
        bmpsurface = nullptr;
    }

    SDL_StartTextInput(mWindow);
    return true;
}

void* LLWindowSDL::createSharedContext()
{
    SDL_PropertiesID props = SDL_CreateProperties();
    SDL_SetStringProperty(props, SDL_PROP_WINDOW_CREATE_TITLE_STRING, "Second Life OSR Utility");
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, 1);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, 1);
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_RESIZABLE_BOOLEAN, false);
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_OPENGL_BOOLEAN, true);
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_HIDDEN_BOOLEAN, true);
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_FOCUSABLE_BOOLEAN, false);
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_UTILITY_BOOLEAN, true);
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_HIGH_PIXEL_DENSITY_BOOLEAN, gHiDPISupport);

    auto osr_window = SDL_CreateWindowWithProperties(props);
    if (osr_window == nullptr)
    {
        SDL_DestroyProperties(props);
        return nullptr;
    }
    SDL_DestroyProperties(props); // Free properties once window is created
    SDL_GLContext pContext = SDL_GL_CreateContext(osr_window);

    // Hack to ensure main window context is bound
    SDL_GL_MakeCurrent(mWindow, mContext);

    if (!pContext)
    {
        LL_WARNS() << "Creating shared OpenGL context failed!" << LL_ENDL;
        SDL_DestroyWindow(osr_window);
        return nullptr;
    }

    {
        LLMutexLock osr_lock(&mOSRMutex);
        mOSRContexts.emplace(pContext, osr_window);
    }

    LL_DEBUGS() << "Creating shared OpenGL context successful!" << LL_ENDL;
    return (void*)pContext;
}

void LLWindowSDL::makeContextCurrent(void* contextPtr)
{
    LLMutexLock osr_lock(&mOSRMutex);
    auto it = mOSRContexts.find((SDL_GLContext)contextPtr);
    if(it != mOSRContexts.end())
    {
        SDL_GL_MakeCurrent((SDL_Window*)it->second, (SDL_GLContext)it->first);
    }
    LL_PROFILER_GPU_CONTEXT;
}

void LLWindowSDL::destroySharedContext(void* contextPtr)
{
    LLMutexLock osr_lock(&mOSRMutex);
    auto it = mOSRContexts.find((SDL_GLContext)contextPtr);
    if(it != mOSRContexts.end())
    {
        SDL_GL_DestroyContext((SDL_GLContext)it->first);
        mDeadOSRWindows.push_back((SDL_Window*)it->second);
    }
}

void LLWindowSDL::toggleVSync(bool enable_vsync)
{
    if (!enable_vsync)
    {
        LL_INFOS("Window") << "Disabling vertical sync" << LL_ENDL;
        SDL_GL_SetSwapInterval(0);
    }
    else
    {
        LL_INFOS("Window") << "Enabling vertical sync" << LL_ENDL;
        SDL_GL_SetSwapInterval(1);
    }
}

// changing fullscreen resolution, or switching between windowed and fullscreen mode.
bool LLWindowSDL::switchContext(bool fullscreen, const LLCoordScreen &size, bool enable_vsync, const LLCoordScreen * const posp)
{
    const bool needsRebuild = true;  // Just nuke the context and start over.
    bool result = true;

    LL_INFOS() << "switchContext, fullscreen=" << fullscreen << LL_ENDL;
    stop_glerror();
    if(needsRebuild)
    {
        destroyContext();
        result = createContext(0, 0, size.mX, size.mY, 32, fullscreen, enable_vsync);
        if (result)
        {
            gGLManager.initWGL();
            gGLManager.initGL();

            //start with arrow cursor
            initCursors();
            setCursor( UI_CURSOR_ARROW );
        }
    }

    stop_glerror();

    return result;
}

void LLWindowSDL::destroyContext()
{
    LL_INFOS() << "destroyContext begins" << LL_ENDL;

    {
        LLMutexLock osr_lock(&mOSRMutex);
        for(SDL_Window* pWindow : mDeadOSRWindows)
        {
            SDL_DestroyWindow(pWindow);
        }
        mDeadOSRWindows.clear();
    }

    // Stop unicode input
    SDL_StopTextInput(mWindow);

    // Clean up remaining GL state before blowing away window
    LL_INFOS() << "shutdownGL begins" << LL_ENDL;
    gGLManager.shutdownGL();

    LL_INFOS() << "Destroying SDL cursors" << LL_ENDL;
    quitCursors();

    if (mContext)
    {
        LL_INFOS() << "Destroying SDL GL Context" << LL_ENDL;
        SDL_GL_DestroyContext(mContext);
        mContext = nullptr;
    }
    else
    {
        LL_INFOS() << "SDL GL Context already destroyed" << LL_ENDL;
    }

    if (mWindow)
    {
        LL_INFOS() << "Destroying SDL Window" << LL_ENDL;
        SDL_DestroyWindow(mWindow);
        mWindow = nullptr;
    }
    else
    {
        LL_INFOS() << "SDL Window already destroyed" << LL_ENDL;
    }
    LL_INFOS() << "destroyContext end" << LL_ENDL;
}

LLWindowSDL::~LLWindowSDL()
{
    destroyContext();

    delete[] mSupportedResolutions;

    gWindowImplementation = nullptr;
}


void LLWindowSDL::show()
{
    if (mWindow)
    {
        SDL_ShowWindow(mWindow);
        SDL_RaiseWindow(mWindow);
    }
}

void LLWindowSDL::hide()
{
    if (mWindow)
    {
        SDL_HideWindow(mWindow);
    }
}

void LLWindowSDL::minimize()
{
    if (mWindow)
    {
        SDL_MinimizeWindow(mWindow);
    }
}

void LLWindowSDL::restore()
{
    if (mWindow)
    {
        SDL_RestoreWindow(mWindow);
    }
}

// close() destroys all OS-specific code associated with a window.
// Usually called from LLWindowManager::destroyWindow()
void LLWindowSDL::close()
{
    // Make sure cursor is visible and we haven't mangled the clipping state.
    setMouseClipping(false);
    showCursor();

    destroyContext();
}

bool LLWindowSDL::isValid()
{
    return mWindow != nullptr;
}

bool LLWindowSDL::getVisible()
{
    bool result = true;
    if (mWindow)
    {
        SDL_WindowFlags flags = SDL_GetWindowFlags(mWindow);
        if (flags & SDL_WINDOW_HIDDEN)
        {
            result = false;
        }
    }
    return result;
}

bool LLWindowSDL::getMinimized()
{
    bool result = false;
    if (mWindow)
    {
        SDL_WindowFlags flags = SDL_GetWindowFlags(mWindow);
        if (flags & SDL_WINDOW_MINIMIZED)
        {
            result = true;
        }
    }
    return result;
}

bool LLWindowSDL::getMaximized()
{
    bool result = false;
    if (mWindow)
    {
        SDL_WindowFlags flags = SDL_GetWindowFlags(mWindow);
        if (flags & SDL_WINDOW_MAXIMIZED)
        {
            result = true;
        }
    }

    return result;
}

bool LLWindowSDL::maximize()
{
    if (mWindow)
    {
        SDL_MaximizeWindow(mWindow);
        return true;
    }
    return false;
}

bool LLWindowSDL::getPosition(LLCoordScreen *position)
{
    if (mWindow)
    {
        SDL_GetWindowPosition(mWindow, &position->mX, &position->mY);
        return true;
    }
    return false;
}

bool LLWindowSDL::getSize(LLCoordScreen *size)
{
    if (mWindow)
    {
        SDL_GetWindowSize(mWindow, &size->mX, &size->mY);
        return true;
    }

    return false;
}

bool LLWindowSDL::getSize(LLCoordWindow *size)
{
    if (mWindow)
    {
        SDL_GetWindowSizeInPixels(mWindow, &size->mX, &size->mY);
        return true;
    }

    return false;
}

bool LLWindowSDL::setPosition(const LLCoordScreen position)
{
    if (mWindow)
    {
        SDL_SetWindowPosition(mWindow, position.mX, position.mY);
        return true;
    }

    return false;
}

template< typename T > bool setSizeImpl( const T& newSize, SDL_Window *pWin )
{
    if( !pWin )
        return false;

    SDL_WindowFlags winFlags = SDL_GetWindowFlags(pWin);

    if( winFlags & SDL_WINDOW_MAXIMIZED)
        SDL_RestoreWindow(pWin);

    SDL_SetWindowSize(pWin, newSize.mX, newSize.mY);
    return true;
}

bool LLWindowSDL::setSizeImpl(const LLCoordScreen size)
{
    return ::setSizeImpl( size, mWindow );
}

bool LLWindowSDL::setSizeImpl(const LLCoordWindow size)
{
    return ::setSizeImpl( size, mWindow );
}


void LLWindowSDL::swapBuffers()
{
    if (mWindow)
    {
        SDL_GL_SwapWindow(mWindow);
    }
    LL_PROFILER_GPU_COLLECT;
}

U32 LLWindowSDL::getFSAASamples()
{
    return mFSAASamples;
}

void LLWindowSDL::setFSAASamples(const U32 samples)
{
    mFSAASamples = samples;
}

F32 LLWindowSDL::getGamma()
{
    return 1.f / mGamma;
}

bool LLWindowSDL::restoreGamma()
{
    return true;
}

bool LLWindowSDL::setGamma(const F32 gamma)
{
    if (mWindow)
    {
        mGamma = gamma;
        if (mGamma == 0)
            mGamma = 0.1f;
        mGamma = 1.f / mGamma;
    }
    return true;
}

bool LLWindowSDL::isCursorHidden()
{
    return mCursorHidden;
}

// Constrains the mouse to the window.
void LLWindowSDL::setMouseClipping(bool b)
{
    if (mWindow)
    {
        SDL_SetWindowMouseGrab(mWindow, b);
    }
}

// virtual
void LLWindowSDL::setMinSize(U32 min_width, U32 min_height, bool enforce_immediately)
{
    LLWindow::setMinSize(min_width, min_height, enforce_immediately);

    if (mWindow && min_width > 0 && min_height > 0)
    {
        SDL_SetWindowMinimumSize(mWindow, mMinWindowWidth, mMinWindowHeight);
    }
}

bool LLWindowSDL::setCursorPosition(const LLCoordWindow position)
{
    SDL_WarpMouseInWindow(mWindow, (F32)position.mX, (F32)position.mY);
    return true;
}

bool LLWindowSDL::getCursorPosition(LLCoordWindow *position)
{
    //Point cursor_point;
    LLCoordScreen screen_pos;

    float x, y;
    SDL_GetMouseState(&x, &y);

    screen_pos.mX = (S32)x;
    screen_pos.mY = (S32)y;

    return convertCoords(screen_pos, position);
}

F32 LLWindowSDL::getNativeAspectRatio()
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

F32 LLWindowSDL::getPixelAspectRatio()
{
    F32 pixel_aspect = 1.f;
    if (getFullscreen())
    {
        LLCoordScreen screen_size;
        if (getSize(&screen_size))
        {
            pixel_aspect = getNativeAspectRatio() * (F32)screen_size.mY / (F32)screen_size.mX;
        }
    }

    return pixel_aspect;
}


// This is to support 'temporarily windowed' mode so that
// dialogs are still usable in fullscreen.
void LLWindowSDL::beforeDialog()
{
    LL_INFOS() << "LLWindowSDL::beforeDialog()" << LL_ENDL;

    if (SDLReallyCaptureInput(false)) // must ungrab input so popup works!
    {
        if (mFullscreen && mWindow )
            SDL_SetWindowFullscreen( mWindow, 0 );
    }
}

void LLWindowSDL::afterDialog()
{
    LL_INFOS() << "LLWindowSDL::afterDialog()" << LL_ENDL;

    if (mFullscreen && mWindow )
        SDL_SetWindowFullscreen( mWindow, 0 );
}

void LLWindowSDL::flashIcon(F32 seconds)
{
    LL_INFOS() << "LLWindowSDL::flashIcon(" << seconds << ")" << LL_ENDL;

    F32 remaining_time = mFlashTimer.getRemainingTimeF32();
    if (remaining_time < seconds)
        remaining_time = seconds;
    mFlashTimer.reset();
    mFlashTimer.setTimerExpirySec(remaining_time);

    SDL_FlashWindow(mWindow, SDL_FLASH_UNTIL_FOCUSED);
    mFlashing = true;
}

void LLWindowSDL::maybeStopFlashIcon()
{
    if (mFlashing && mFlashTimer.hasExpired())
    {
        mFlashing = false;
        SDL_FlashWindow( mWindow, SDL_FLASH_CANCEL );
    }
}

bool LLWindowSDL::isClipboardTextAvailable()
{
    return SDL_HasClipboardText();
}

bool LLWindowSDL::pasteTextFromClipboard(LLWString &dst)
{
    if (isClipboardTextAvailable())
    {
        char* data = SDL_GetClipboardText();
        if (data)
        {
            dst = LLWString(utf8str_to_wstring(data));
            SDL_free(data);
            return true;
        }
    }
    return false;
}

bool LLWindowSDL::copyTextToClipboard(const LLWString& text)
{
    const std::string utf8 = wstring_to_utf8str(text);
    return SDL_SetClipboardText(utf8.c_str());
}

bool LLWindowSDL::isPrimaryTextAvailable()
{
    return SDL_HasPrimarySelectionText();
}

bool LLWindowSDL::pasteTextFromPrimary(LLWString &dst)
{
    if (isPrimaryTextAvailable())
    {
        char* data = SDL_GetPrimarySelectionText();
        if (data)
        {
            dst = LLWString(utf8str_to_wstring(data));
            SDL_free(data);
            return true;
        }
    }
    return false;
}

bool LLWindowSDL::copyTextToPrimary(const LLWString& text)
{
    const std::string utf8 = wstring_to_utf8str(text);
    return SDL_SetPrimarySelectionText(utf8.c_str());
}

LLWindow::LLWindowResolution* LLWindowSDL::getSupportedResolutions(S32 &num_resolutions)
{
    if (!mSupportedResolutions)
    {
        mSupportedResolutions = new LLWindowResolution[MAX_NUM_RESOLUTIONS];
        mNumSupportedResolutions = 0;

        SDL_DisplayID display = SDL_GetPrimaryDisplay();
        int num_modes = 0;
        SDL_DisplayMode **modes = SDL_GetFullscreenDisplayModes(display, &num_modes);
        num_modes = llclamp(num_modes, 0, MAX_NUM_RESOLUTIONS);
        if (modes) {
            for (int i = 0; i < num_modes; ++i) {
                SDL_DisplayMode *mode = modes[i];
                int w = mode->w;
                int h = mode->h;
                if ((w >= 800) && (h >= 600))
                {
                    // make sure we don't add the same resolution multiple times!
                    if ( (mNumSupportedResolutions == 0) ||
                        ((mSupportedResolutions[mNumSupportedResolutions-1].mWidth != w) &&
                         (mSupportedResolutions[mNumSupportedResolutions-1].mHeight != h)) )
                    {
                        mSupportedResolutions[mNumSupportedResolutions].mWidth = w;
                        mSupportedResolutions[mNumSupportedResolutions].mHeight = h;
                        mNumSupportedResolutions++;
                    }
                }
            }
            SDL_free(modes);
        }
    }

    num_resolutions = mNumSupportedResolutions;
    return mSupportedResolutions;
}

//static
std::vector<std::string> LLWindowSDL::getDisplaysResolutionList()
{
    std::vector<std::string> ret;
    if (gWindowImplementation)
    {
        S32 resolutionCount = 0;
        LLWindowResolution* resolutionList = gWindowImplementation->getSupportedResolutions(resolutionCount);
        if (resolutionList != nullptr)
        {
            for (S32 i = 0; i < resolutionCount; i++)
            {
                const LLWindowResolution& resolution = resolutionList[i];
                ret.push_back(std::to_string(resolution.mWidth) + "x" + std::to_string(resolution.mHeight));
            }
        }
    }
    return ret;
}


bool LLWindowSDL::convertCoords(LLCoordGL from, LLCoordWindow *to)
{
    if (!to)
        return false;

    if (!mWindow)
        return false;

    S32 height;
    SDL_GetWindowSizeInPixels(mWindow, nullptr, &height);

    float pixel_scale = SDL_GetWindowPixelDensity(mWindow);

    to->mX = from.mX / pixel_scale;
    to->mY = (height - from.mY - 1) / pixel_scale;

    return true;
}

bool LLWindowSDL::convertCoords(LLCoordWindow from, LLCoordGL* to)
{
    if (!to)
        return false;

    if (!mWindow)
        return false;
    S32 height;
    SDL_GetWindowSize(mWindow, nullptr, &height);

    float pixel_scale = SDL_GetWindowPixelDensity(mWindow);

    to->mX = from.mX * pixel_scale;
    to->mY = (height - from.mY - 1) * pixel_scale;

    return true;
}

bool LLWindowSDL::convertCoords(LLCoordScreen from, LLCoordWindow* to)
{
    if (!to)
        return false;

    // In the fullscreen case, window and screen coordinates are the same.
    to->mX = from.mX;
    to->mY = from.mY;
    return true;
}

bool LLWindowSDL::convertCoords(LLCoordWindow from, LLCoordScreen *to)
{
    if (!to)
        return false;

    // In the fullscreen case, window and screen coordinates are the same.
    to->mX = from.mX;
    to->mY = from.mY;
    return true;
}

bool LLWindowSDL::convertCoords(LLCoordScreen from, LLCoordGL *to)
{
    LLCoordWindow window_coord;

    return convertCoords(from, &window_coord) && convertCoords(window_coord, to);
}

bool LLWindowSDL::convertCoords(LLCoordGL from, LLCoordScreen *to)
{
    LLCoordWindow window_coord;

    return convertCoords(from, &window_coord) && convertCoords(window_coord, to);
}

void LLWindowSDL::setupFailure(const std::string& text, const std::string& caption, U32 type)
{
    close();

    OSMessageBox(text, caption, type);
}

bool LLWindowSDL::SDLReallyCaptureInput(bool capture)
{
    // note: this used to be safe to call nestedly, but in the
    // end that's not really a wise usage pattern, so don't.

    if (capture)
        mReallyCapturedCount = 1;
    else
        mReallyCapturedCount = 0;

    bool wantGrab;
    if (mReallyCapturedCount <= 0) // uncapture
    {
        wantGrab = false;
    } else // capture
    {
        wantGrab = true;
    }

    if (mReallyCapturedCount < 0) // yuck, imbalance.
    {
        mReallyCapturedCount = 0;
        LL_WARNS() << "ReallyCapture count was < 0" << LL_ENDL;
    }

    bool newGrab = wantGrab;
    if (!mFullscreen) /* only bother if we're windowed anyway */
    {
        int result;
        if (wantGrab)
        {
            // LL_INFOS() << "X11 POINTER GRABBY" << LL_ENDL;
            result = SDL_SetWindowMouseGrab(mWindow, true);
            if (0 == result)
                newGrab = true;
            else
                newGrab = false;
        }
        else
        {
            newGrab = false;
            result = SDL_SetWindowMouseGrab(mWindow, false);
        }
    }
        // pretend we got what we wanted, when really we don't care.

    // return boolean success for whether we ended up in the desired state
    return capture == newGrab;
}

U32 LLWindowSDL::SDLCheckGrabbyKeys(U32 keysym, bool gain)
{
    /* part of the fix for SL-13243: Some popular window managers like
       to totally eat alt-drag for the purposes of moving windows.  We
       spoil their day by acquiring the exclusive X11 mouse lock for as
       long as ALT is held down, so the window manager can't easily
       see what's happening.  Tested successfully with Metacity.
       And... do the same with CTRL, for other darn WMs.  We don't
       care about other metakeys as SL doesn't use them with dragging
       (for now). */

    /* We maintain a bitmap of critical keys which are up and down
       instead of simply key-counting, because SDL sometimes reports
       misbalanced keyup/keydown event pairs to us for whatever reason. */

    U32 mask = 0;
    switch (keysym)
    {
        case SDLK_LALT:
            mask = 1U << 0; break;
        case SDLK_RALT:
            mask = 1U << 1; break;
        case SDLK_LCTRL:
            mask = 1U << 2; break;
        case SDLK_RCTRL:
            mask = 1U << 3; break;
        default:
            break;
    }

    if (gain)
        mGrabbyKeyFlags |= mask;
    else
        mGrabbyKeyFlags &= ~mask;

    //LL_INFOS() << "mGrabbyKeyFlags=" << mGrabbyKeyFlags << LL_ENDL;

    /* 0 means we don't need to mousegrab, otherwise grab. */
    return mGrabbyKeyFlags;
}

// virtual
void LLWindowSDL::processMiscNativeEvents()
{
    {
        LLMutexLock osr_lock(&mOSRMutex);
        for(SDL_Window* pWindow : mDeadOSRWindows)
        {
            SDL_DestroyWindow(pWindow);
        }
        mDeadOSRWindows.clear();
    }
}

void LLWindowSDL::gatherInput()
{
    // This is for the case where SDL is not driving the main event loop
    if(!gSDLMainHandled)
    {
        SDL_Event event;

        // Handle all outstanding SDL events
        while (SDL_PollEvent(&event))
        {
            handleEvent(event);
        }
    }

    updateCursor();

    // This is a good time to stop flashing the icon if our mFlashTimer has
    // expired.
    if (mFlashing && mFlashTimer.hasExpired())
    {
        SDL_FlashWindow(mWindow, SDL_FLASH_CANCEL);
        mFlashing = false;
    }
}

SDL_AppResult LLWindowSDL::handleEvent(const SDL_Event& event)
{
    switch(event.type)
    {
        case SDL_EVENT_MOUSE_MOTION:
        {
            LLCoordWindow winCoord(llfloor(event.motion.x), llfloor(event.motion.y));
            LLCoordGL openGlCoord;
            convertCoords(winCoord, &openGlCoord);
            mCallbacks->handleMouseMove(this, openGlCoord, gKeyboard->currentMask(true));
            break;
        }

        case SDL_EVENT_MOUSE_WHEEL:
        {
            if(event.wheel.integer_y != 0)
            {
                mCallbacks->handleScrollWheel(this, -event.wheel.integer_y);
            }
            if (event.wheel.integer_x != 0)
            {
                mCallbacks->handleScrollHWheel(this, -event.wheel.integer_x);
            }
            break;
        }

        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        {
            LLCoordWindow winCoord(llfloor(event.button.x), llfloor(event.button.y));
            LLCoordGL openGlCoord;
            convertCoords(winCoord, &openGlCoord);
            MASK mask = gKeyboard->currentMask(true);

            if (event.button.button == SDL_BUTTON_LEFT)  // left
            {
                if (event.button.clicks == 2)
                    mCallbacks->handleDoubleClick(this, openGlCoord, mask);
                else
                    mCallbacks->handleMouseDown(this, openGlCoord, mask);
            }
            else if (event.button.button == SDL_BUTTON_RIGHT)
            {
                mCallbacks->handleRightMouseDown(this, openGlCoord, mask);
            }
            else if (event.button.button == SDL_BUTTON_MIDDLE)  // middle
            {
                mCallbacks->handleMiddleMouseDown(this, openGlCoord, mask);
            }
            else
            {
                mCallbacks->handleOtherMouseDown(this, openGlCoord, mask, event.button.button);
            }

            break;
        }

        case SDL_EVENT_MOUSE_BUTTON_UP:
        {
            LLCoordWindow winCoord(llfloor(event.button.x), llfloor(event.button.y));
            LLCoordGL openGlCoord;
            convertCoords(winCoord, &openGlCoord);
            MASK mask = gKeyboard->currentMask(true);

            if (event.button.button == SDL_BUTTON_LEFT)  // left
            {
                mCallbacks->handleMouseUp(this, openGlCoord, mask);
            }
            else if (event.button.button == SDL_BUTTON_RIGHT)  // right
            {
                mCallbacks->handleRightMouseUp(this, openGlCoord, mask);
            }
            else if (event.button.button == SDL_BUTTON_MIDDLE)  // middle
            {
                mCallbacks->handleMiddleMouseUp(this, openGlCoord, mask);
            }
            else
            {
                mCallbacks->handleOtherMouseUp(this, openGlCoord, mask, event.button.button);
            }

            break;
        }

        case SDL_EVENT_KEY_DOWN:
        {
            mKeyVirtualKey = event.key.key;
            mKeyModifiers = event.key.mod;

            if (mKeyVirtualKey == SDLK_RETURN2 || mKeyVirtualKey == SDLK_KP_ENTER)
            {
                mKeyVirtualKey = SDLK_RETURN;
            }

            gKeyboard->handleKeyDown(mKeyVirtualKey, mKeyModifiers);

            if (mKeyVirtualKey == SDLK_RETURN)
            {
                // fix return key not working when capslock, scrolllock or numlock are enabled
                mKeyModifiers &= (~(SDL_KMOD_NUM | SDL_KMOD_CAPS | SDL_KMOD_MODE | SDL_KMOD_SCROLL));
                mCallbacks->handleUnicodeChar(mKeyVirtualKey, gKeyboard->currentMask(false));
            }

            // part of the fix for SL-13243
            if (SDLCheckGrabbyKeys(mKeyVirtualKey, true) != 0)
                SDLReallyCaptureInput(true);
            break;
        }

        case SDL_EVENT_KEY_UP:
        {
            mKeyVirtualKey = event.key.key;
            mKeyModifiers = event.key.mod;

            if (mKeyVirtualKey == SDLK_RETURN2 || mKeyVirtualKey == SDLK_KP_ENTER)
            {
                mKeyVirtualKey = SDLK_RETURN;
            }

            gKeyboard->handleKeyUp(mKeyVirtualKey, mKeyModifiers);
            if (SDLCheckGrabbyKeys(mKeyVirtualKey, false) == 0)
                SDLReallyCaptureInput(false); // part of the fix for SL-13243
            break;
        }

        case SDL_EVENT_TEXT_INPUT:
        {
            auto string = utf8str_to_wstring(event.text.text);
            MASK current_mask = gKeyboard->currentMask(false);
            for (auto key : string)
            {
                mKeyVirtualKey = key;
                mCallbacks->handleUnicodeChar(key, current_mask);
            }
            break;
        }

        case SDL_EVENT_WINDOW_EXPOSED:
        {
            mCallbacks->handlePaint(this, 0, 0, 0, 0);
            break;
        }

        case SDL_EVENT_WINDOW_RESIZED:
        {
            LL_INFOS() << "Handling a resize event: " << event.window.data1 << "x" << event.window.data2 << LL_ENDL;
            F32 pixel_density = SDL_GetWindowPixelDensity(mWindow);
            S32 width = llmax(event.window.data1, (S32)mMinWindowWidth) * pixel_density;
            S32 height = llmax(event.window.data2, (S32)mMinWindowHeight) * pixel_density;

            mCallbacks->handleResize(this, width, height);
            break;
        }
        case SDL_EVENT_WINDOW_MOUSE_ENTER:
            break;
        case SDL_EVENT_WINDOW_MOUSE_LEAVE:
            mCallbacks->handleMouseLeave(this);
            break;
        case SDL_EVENT_WINDOW_FOCUS_GAINED:
            //SDL_SetWindowKeyboardGrab(mWindow, true);
            mCallbacks->handleFocus(this);
            break;
        case SDL_EVENT_WINDOW_FOCUS_LOST:
            mCallbacks->handleFocusLost(this);
            //SDL_SetWindowKeyboardGrab(mWindow, false);
            break;
        case SDL_EVENT_WINDOW_RESTORED:
            mCallbacks->handleActivate(this, true);
            break;
        case SDL_EVENT_WINDOW_MAXIMIZED:
            mCallbacks->handleActivate(this, true);
            break;
        case SDL_EVENT_WINDOW_MINIMIZED:
            mCallbacks->handleActivate(this, false);
            break;
        case SDL_EVENT_WINDOW_DISPLAY_CHANGED:
        {
            // Update refresh rate when changing monitors
            const SDL_DisplayMode* displayMode = SDL_GetCurrentDisplayMode(event.window.data1);
            if(displayMode)
            {
                mRefreshRate = ll_round(displayMode->refresh_rate);
                mNativeAspectRatio = ((F32)displayMode->w) / ((F32)displayMode->h);
            }
            mCallbacks->handleDisplayChanged();
            break;
        }
        case SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED:
        {
            S32 w, h = 0;
            SDL_GetWindowSizeInPixels(mWindow, &w, &h);
            mCallbacks->handleDPIChanged(this, getSystemUISize(), w, h);
            break;
        }
        case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        {
            if(mCallbacks->handleCloseRequest(this, true))
            {
                // Get the app to initiate cleanup.
                mCallbacks->handleQuit(this);
                // The app is responsible for calling destroyWindow when done with GL
            }
            break;
        }
        default:
            break;
    }

    return SDL_APP_CONTINUE;
}

// static
SDL_AppResult LLWindowSDL::handleEvents(const SDL_Event& event)
{
    if(!gWindowImplementation) return SDL_APP_CONTINUE;

    return gWindowImplementation->handleEvent(event);
}

static SDL_Cursor *makeSDLCursorFromBMP(const char *filename, int hotx, int hoty)
{
    SDL_Cursor *sdlcursor = nullptr;
    SDL_Surface *bmpsurface;

    // Load cursor pixel data from BMP file
    bmpsurface = Load_BMP_Resource(filename);
    if (bmpsurface && bmpsurface->w%8==0)
    {
        SDL_Surface *cursurface;
        LL_DEBUGS() << "Loaded cursor file " << filename << " "
                    << bmpsurface->w << "x" << bmpsurface->h << LL_ENDL;
        SDL_PixelFormat pix_format = SDL_GetPixelFormatForMasks(32,
            SDL_Swap32LE(0xFFU),
            SDL_Swap32LE(0xFF00U),
            SDL_Swap32LE(0xFF0000U),
            SDL_Swap32LE(0xFF000000U));
        if (pix_format == SDL_PIXELFORMAT_UNKNOWN)
        {
            return nullptr;
        }

        const SDL_PixelFormatDetails* pix_format_details = SDL_GetPixelFormatDetails(pix_format);
        if(!pix_format_details)
        {
            return nullptr;
        }

        cursurface = SDL_CreateSurface(bmpsurface->w,
                                           bmpsurface->h,
                                           pix_format);
        SDL_FillSurfaceRect(cursurface, nullptr, SDL_Swap32LE(0x00000000U));

        // Blit the cursor pixel data onto a 32-bit RGBA surface so we
        // only have to cope with processing one type of pixel format.
        if (SDL_BlitSurface(bmpsurface, nullptr,
                                 cursurface, nullptr))
        {
            // n.b. we already checked that width is a multiple of 8.
            const int bitmap_bytes = (cursurface->w * cursurface->h) / 8;
            unsigned char *cursor_data = new unsigned char[bitmap_bytes];
            unsigned char *cursor_mask = new unsigned char[bitmap_bytes];
            memset(cursor_data, 0, bitmap_bytes);
            memset(cursor_mask, 0, bitmap_bytes);
            int i,j;
            // Walk the RGBA cursor pixel data, extracting both data and
            // mask to build SDL-friendly cursor bitmaps from.  The mask
            // is inferred by color-keying against 200,200,200
            for (i=0; i<cursurface->h; ++i) {
                for (j=0; j<cursurface->w; ++j) {
                    U8 *pixelp =
                            ((U8*)cursurface->pixels)
                            + cursurface->pitch * i
                            + j*pix_format_details->bytes_per_pixel;
                    U8 srcred = pixelp[0];
                    U8 srcgreen = pixelp[1];
                    U8 srcblue = pixelp[2];
                    bool mask_bit = (srcred != 200)
                                    || (srcgreen != 200)
                                    || (srcblue != 200);
                    bool data_bit = mask_bit && (srcgreen <= 80);//not 0x80
                    unsigned char bit_offset = (cursurface->w/8) * i
                                               + j/8;
                    cursor_data[bit_offset] |= (data_bit) << (7 - (j&7));
                    cursor_mask[bit_offset] |= (mask_bit) << (7 - (j&7));
                }
            }
            sdlcursor = SDL_CreateCursor((Uint8*)cursor_data,
                                         (Uint8*)cursor_mask,
                                         cursurface->w, cursurface->h,
                                         hotx, hoty);
            delete[] cursor_data;
            delete[] cursor_mask;
        } else {
            LL_WARNS() << "CURSOR BLIT FAILURE, cursurface: " << cursurface << LL_ENDL;
        }
        SDL_DestroySurface(cursurface);
        SDL_DestroySurface(bmpsurface);
    } else {
        LL_WARNS() << "CURSOR LOAD FAILURE " << filename << LL_ENDL;
    }

    return sdlcursor;
}

void LLWindowSDL::updateCursor()
{
    if (mCurrentCursor != mNextCursor)
    {
        if (mNextCursor < UI_CURSOR_COUNT)
        {
            SDL_Cursor *sdlcursor = mSDLCursors[mNextCursor];
            // Try to default to the arrow for any cursors that
            // did not load correctly.
            if (!sdlcursor && mSDLCursors[UI_CURSOR_ARROW])
                sdlcursor = mSDLCursors[UI_CURSOR_ARROW];
            if (sdlcursor)
                SDL_SetCursor(sdlcursor);

            mCurrentCursor = mNextCursor;
        }
        else
        {
            LL_WARNS() << "Tried to set invalid cursor number " << mNextCursor << LL_ENDL;
        }
    }
}

void LLWindowSDL::initCursors()
{
    // Blank the cursor pointer array for those we may miss.
    for (int i=0; i<UI_CURSOR_COUNT; ++i)
    {
        mSDLCursors[i] = nullptr;
    }

    // Pre-make an SDL cursor for each of the known cursor types.
    // We hardcode the hotspots - to avoid that we'd have to write
    // a .cur file loader.
    // NOTE: SDL doesn't load RLE-compressed BMP files.
    mSDLCursors[UI_CURSOR_ARROW] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_DEFAULT);
    mSDLCursors[UI_CURSOR_WAIT] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_WAIT);
    mSDLCursors[UI_CURSOR_HAND] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_POINTER);
    mSDLCursors[UI_CURSOR_IBEAM] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_TEXT);
    mSDLCursors[UI_CURSOR_CROSS] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_CROSSHAIR);
    mSDLCursors[UI_CURSOR_SIZENWSE] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NWSE_RESIZE);
    mSDLCursors[UI_CURSOR_SIZENESW] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NESW_RESIZE);
    mSDLCursors[UI_CURSOR_SIZEWE] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_EW_RESIZE);
    mSDLCursors[UI_CURSOR_SIZENS] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NS_RESIZE);
    mSDLCursors[UI_CURSOR_SIZEALL] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_MOVE);
    mSDLCursors[UI_CURSOR_NO] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NOT_ALLOWED);
    mSDLCursors[UI_CURSOR_WORKING] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_PROGRESS);
    mSDLCursors[UI_CURSOR_TOOLGRAB] = makeSDLCursorFromBMP("lltoolgrab.BMP",2,13);
    mSDLCursors[UI_CURSOR_TOOLLAND] = makeSDLCursorFromBMP("lltoolland.BMP",1,6);
    mSDLCursors[UI_CURSOR_TOOLFOCUS] = makeSDLCursorFromBMP("lltoolfocus.BMP",8,5);
    mSDLCursors[UI_CURSOR_TOOLCREATE] = makeSDLCursorFromBMP("lltoolcreate.BMP",7,7);
    mSDLCursors[UI_CURSOR_ARROWDRAG] = makeSDLCursorFromBMP("arrowdrag.BMP",0,0);
    mSDLCursors[UI_CURSOR_ARROWCOPY] = makeSDLCursorFromBMP("arrowcop.BMP",0,0);
    mSDLCursors[UI_CURSOR_ARROWDRAGMULTI] = makeSDLCursorFromBMP("llarrowdragmulti.BMP",0,0);
    mSDLCursors[UI_CURSOR_ARROWCOPYMULTI] = makeSDLCursorFromBMP("arrowcopmulti.BMP",0,0);
    mSDLCursors[UI_CURSOR_NOLOCKED] = makeSDLCursorFromBMP("llnolocked.BMP",8,8);
    mSDLCursors[UI_CURSOR_ARROWLOCKED] = makeSDLCursorFromBMP("llarrowlocked.BMP",0,0);
    mSDLCursors[UI_CURSOR_GRABLOCKED] = makeSDLCursorFromBMP("llgrablocked.BMP",2,13);
    mSDLCursors[UI_CURSOR_TOOLTRANSLATE] = makeSDLCursorFromBMP("lltooltranslate.BMP",0,0);
    mSDLCursors[UI_CURSOR_TOOLROTATE] = makeSDLCursorFromBMP("lltoolrotate.BMP",0,0);
    mSDLCursors[UI_CURSOR_TOOLSCALE] = makeSDLCursorFromBMP("lltoolscale.BMP",0,0);
    mSDLCursors[UI_CURSOR_TOOLCAMERA] = makeSDLCursorFromBMP("lltoolcamera.BMP",7,5);
    mSDLCursors[UI_CURSOR_TOOLPAN] = makeSDLCursorFromBMP("lltoolpan.BMP",7,5);
    mSDLCursors[UI_CURSOR_TOOLZOOMIN] = makeSDLCursorFromBMP("lltoolzoomin.BMP",7,5);
    mSDLCursors[UI_CURSOR_TOOLZOOMOUT] = makeSDLCursorFromBMP("lltoolzoomout.BMP", 7, 5);
    mSDLCursors[UI_CURSOR_TOOLPICKOBJECT3] = makeSDLCursorFromBMP("toolpickobject3.BMP",0,0);
    mSDLCursors[UI_CURSOR_TOOLPLAY] = makeSDLCursorFromBMP("toolplay.BMP",0,0);
    mSDLCursors[UI_CURSOR_TOOLPAUSE] = makeSDLCursorFromBMP("toolpause.BMP",0,0);
    mSDLCursors[UI_CURSOR_TOOLMEDIAOPEN] = makeSDLCursorFromBMP("toolmediaopen.BMP",0,0);
    mSDLCursors[UI_CURSOR_PIPETTE] = makeSDLCursorFromBMP("lltoolpipette.BMP",2,28);
    mSDLCursors[UI_CURSOR_TOOLSIT] = makeSDLCursorFromBMP("toolsit.BMP",20,15);
    mSDLCursors[UI_CURSOR_TOOLBUY] = makeSDLCursorFromBMP("toolbuy.BMP",20,15);
    mSDLCursors[UI_CURSOR_TOOLOPEN] = makeSDLCursorFromBMP("toolopen.BMP",20,15);
    mSDLCursors[UI_CURSOR_TOOLPATHFINDING] = makeSDLCursorFromBMP("lltoolpathfinding.BMP", 16, 16);
    mSDLCursors[UI_CURSOR_TOOLPATHFINDING_PATH_START] = makeSDLCursorFromBMP("lltoolpathfindingpathstart.BMP", 16, 16);
    mSDLCursors[UI_CURSOR_TOOLPATHFINDING_PATH_START_ADD] = makeSDLCursorFromBMP("lltoolpathfindingpathstartadd.BMP", 16, 16);
    mSDLCursors[UI_CURSOR_TOOLPATHFINDING_PATH_END] = makeSDLCursorFromBMP("lltoolpathfindingpathend.BMP", 16, 16);
    mSDLCursors[UI_CURSOR_TOOLPATHFINDING_PATH_END_ADD] = makeSDLCursorFromBMP("lltoolpathfindingpathendadd.BMP", 16, 16);
    mSDLCursors[UI_CURSOR_TOOLNO] = makeSDLCursorFromBMP("llno.BMP",8,8);
}

void LLWindowSDL::quitCursors()
{
    if (mWindow)
    {
        for (int i=0; i<UI_CURSOR_COUNT; ++i)
        {
            if (mSDLCursors[i])
            {
                SDL_DestroyCursor(mSDLCursors[i]);
                mSDLCursors[i] = nullptr;
            }
        }
    }
    else
    {
        // SDL doesn't refcount cursors, so if the window has
        // already been destroyed then the cursors have gone with it.
        LL_INFOS() << "Skipping quitCursors: mWindow already gone." << LL_ENDL;
        for (int i=0; i<UI_CURSOR_COUNT; ++i)
            mSDLCursors[i] = nullptr;
    }
}

void LLWindowSDL::captureMouse()
{
    // SDL already enforces the semantics that captureMouse is
    // used for, i.e. that we continue to get mouse events as long
    // as a button is down regardless of whether we left the
    // window, and in a less obnoxious way than SDL_WM_GrabInput
    // which would confine the cursor to the window too.

    LL_DEBUGS() << "LLWindowSDL::captureMouse" << LL_ENDL;
}

void LLWindowSDL::releaseMouse()
{
    // see LWindowSDL::captureMouse()

    LL_DEBUGS() << "LLWindowSDL::releaseMouse" << LL_ENDL;
}

void LLWindowSDL::hideCursor()
{
    if(!mCursorHidden)
    {
        // LL_INFOS() << "hideCursor: hiding" << LL_ENDL;
        mCursorHidden = true;
        mHideCursorPermanent = true;
        SDL_HideCursor();
    }
    else
    {
        // LL_INFOS() << "hideCursor: already hidden" << LL_ENDL;
    }
}

void LLWindowSDL::showCursor()
{
    if(mCursorHidden)
    {
        // LL_INFOS() << "showCursor: showing" << LL_ENDL;
        mCursorHidden = false;
        mHideCursorPermanent = false;
        SDL_ShowCursor();
    }
    else
    {
        // LL_INFOS() << "showCursor: already visible" << LL_ENDL;
    }
}

void LLWindowSDL::showCursorFromMouseMove()
{
    if (!mHideCursorPermanent)
    {
        showCursor();
    }
}

void LLWindowSDL::hideCursorUntilMouseMove()
{
    if (!mHideCursorPermanent)
    {
        hideCursor();
        mHideCursorPermanent = false;
    }
}

//
// LLSplashScreenSDL - I don't think we'll bother to implement this; it's
// fairly obsolete at this point.
//
LLSplashScreenSDL::LLSplashScreenSDL()
{
}

LLSplashScreenSDL::~LLSplashScreenSDL()
{
}

void LLSplashScreenSDL::showImpl()
{
}

void LLSplashScreenSDL::updateImpl(const std::string& mesg)
{
}

void LLSplashScreenSDL::hideImpl()
{
}

S32 OSMessageBoxSDL(const std::string& text, const std::string& caption, U32 type)
{
    SDL_MessageBoxData oData = { SDL_MESSAGEBOX_INFORMATION, nullptr, caption.c_str(), text.c_str(), 0, nullptr, nullptr };
    SDL_MessageBoxButtonData btnOk[] = {{SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, OSBTN_OK, "OK" }};
    SDL_MessageBoxButtonData btnOkCancel [] =  {{SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, OSBTN_OK, "OK" }, {SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, OSBTN_CANCEL, "Cancel"} };
    SDL_MessageBoxButtonData btnYesNo[] = { {SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, OSBTN_YES, "Yes" }, {SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, OSBTN_NO, "No"} };

    switch (type)
    {
        default:
        case OSMB_OK:
            oData.flags = SDL_MESSAGEBOX_WARNING;
            oData.buttons = btnOk;
            oData.numbuttons = 1;
            break;
        case OSMB_OKCANCEL:
            oData.flags = SDL_MESSAGEBOX_INFORMATION;
            oData.buttons = btnOkCancel;
            oData.numbuttons = 2;
            break;
        case OSMB_YESNO:
            oData.flags = SDL_MESSAGEBOX_INFORMATION;
            oData.buttons = btnYesNo;
            oData.numbuttons = 2;
            break;
    }

    if(gWindowImplementation != nullptr)
        gWindowImplementation->beforeDialog();

    int btn{0};
    if(SDL_ShowMessageBox( &oData, &btn ))
    {
        if(gWindowImplementation != nullptr)
            gWindowImplementation->afterDialog();
        return btn;
    }

    if(gWindowImplementation != nullptr)
        gWindowImplementation->afterDialog();

    return OSBTN_CANCEL;
}

bool LLWindowSDL::dialogColorPicker( F32 *r, F32 *g, F32 *b)
{
    return false;
}

/*
        Make the raw keyboard data available - used to poke through to LLQtWebKit so
        that Qt/Webkit has access to the virtual keycodes etc. that it needs
*/
LLSD LLWindowSDL::getNativeKeyData()
{
    LLSD result = LLSD::emptyMap();

    U32 modifiers = 0; // pretend-native modifiers... oh what a tangled web we weave!

    // we go through so many levels of device abstraction that I can't really guess
    // what a plugin under GDK under Qt under SL under SDL under X11 considers
    // a 'native' modifier mask.  this has been sort of reverse-engineered... they *appear*
    // to match GDK consts, but that may be co-incidence.
    modifiers |= (mKeyModifiers & SDL_KMOD_LSHIFT) ? 0x0001 : 0;
    modifiers |= (mKeyModifiers & SDL_KMOD_RSHIFT) ? 0x0001 : 0;// munge these into the same shift
    modifiers |= (mKeyModifiers & SDL_KMOD_CAPS)   ? 0x0002 : 0;
    modifiers |= (mKeyModifiers & SDL_KMOD_LCTRL)  ? 0x0004 : 0;
    modifiers |= (mKeyModifiers & SDL_KMOD_RCTRL)  ? 0x0004 : 0;// munge these into the same ctrl
    modifiers |= (mKeyModifiers & SDL_KMOD_LALT)   ? 0x0008 : 0;// untested
    modifiers |= (mKeyModifiers & SDL_KMOD_RALT)   ? 0x0008 : 0;// untested
    // *todo: test ALTs - I don't have a case for testing these.  Do you?
    // *todo: NUM? - I don't care enough right now (and it's not a GDK modifier).

    result["virtual_key"] = (S32)mKeyVirtualKey;
    result["virtual_key_win"] = (S32)LLKeyboardSDL::mapSDL2toWin( mKeyVirtualKey );
    result["modifiers"] = (S32)modifiers;
    return result;
}

// Open a URL with the user's default web browser.
// Must begin with protocol identifier.
void LLWindowSDL::spawnWebBrowser(const std::string& escaped_url, bool async)
{
    bool found = false;
    S32 i;
    for (i = 0; i < gURLProtocolWhitelistCount; i++)
    {
        if (escaped_url.find(gURLProtocolWhitelist[i]) != std::string::npos)
        {
            found = true;
            break;
        }
    }

    if (!found)
    {
        LL_WARNS() << "spawn_web_browser called for url with protocol not on whitelist: " << escaped_url << LL_ENDL;
        return;
    }

    LL_INFOS() << "spawn_web_browser: " << escaped_url << LL_ENDL;

    if (!SDL_OpenURL(escaped_url.c_str()))
    {
        LL_WARNS() << "spawn_web_browser failed with error: " << SDL_GetError() << LL_ENDL;
    }

    LL_INFOS() << "spawn_web_browser returning." << LL_ENDL;
}

void* LLWindowSDL::getPlatformWindow()
{
    void* ret = nullptr;
    if (mWindow)
    {
#if LL_WINDOWS
        ret = SDL_GetPointerProperty(SDL_GetWindowProperties(mWindow), SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
#elif LL_DARWIN
        ret = SDL_GetPointerProperty(SDL_GetWindowProperties(mWindow), SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, nullptr);
#endif
    }
    return ret;
}

void LLWindowSDL::bringToFront()
{
    // This is currently used when we are 'launched' to a specific
    // map position externally.
    LL_INFOS() << "bringToFront" << LL_ENDL;
    if (mWindow && !mFullscreen)
    {
        SDL_RaiseWindow(mWindow);
    }
}

//static
std::vector<std::string> LLWindowSDL::getDynamicFallbackFontList()
{
    std::vector<std::string> rtns;
#if LL_LINUX
    // Use libfontconfig to find us a nice ordered list of fallback fonts
    // specific to this system.
    std::string final_fallback("/usr/share/fonts/truetype/kochi/kochi-gothic.ttf");
    const int max_font_count_cutoff = 40; // fonts are expensive in the current system, don't enumerate an arbitrary number of them
    // Our 'ideal' font properties which define the sorting results.
    // slant=0 means Roman, index=0 means the first face in a font file
    // (the one we actually use), weight=80 means medium weight,
    // spacing=0 means proportional spacing.
    std::string sort_order("slant=0:index=0:weight=80:spacing=0");
    // elide_unicode_coverage removes fonts from the list whose unicode
    // range is covered by fonts earlier in the list.  This usually
    // removes ~90% of the fonts as redundant (which is great because
    // the font list can be huge), but might unnecessarily reduce the
    // renderable range if for some reason our FreeType actually fails
    // to use some of the fonts we want it to.
    const bool elide_unicode_coverage = true;

    FcFontSet *fs = nullptr;
    FcPattern *sortpat = nullptr;

    LL_INFOS() << "Getting system font list from FontConfig..." << LL_ENDL;

    // If the user has a system-wide language preference, then favor
    // fonts from that language group.  This doesn't affect the types
    // of languages that can be displayed, but ensures that their
    // preferred language is rendered from a single consistent font where
    // possible.
    FL_Locale *locale = nullptr;
    FL_Success success = FL_FindLocale(&locale, FL_MESSAGES);
    if (success != 0)
    {
        if (success >= 2 && locale->lang) // confident!
        {
            LL_INFOS("AppInit") << "Language " << locale->lang << LL_ENDL;
            LL_INFOS("AppInit") << "Location " << locale->country << LL_ENDL;
            LL_INFOS("AppInit") << "Variant " << locale->variant << LL_ENDL;

            LL_INFOS() << "Preferring fonts of language: "
                       << locale->lang
                       << LL_ENDL;
            sort_order = "lang=" + std::string(locale->lang) + ":"
                         + sort_order;
        }
    }
    FL_FreeLocale(&locale);

    if (!FcInit())
    {
        LL_WARNS() << "FontConfig failed to initialize." << LL_ENDL;
        rtns.push_back(final_fallback);
        return rtns;
    }

    sortpat = FcNameParse((FcChar8*) sort_order.c_str());
    if (sortpat)
    {
        // Sort the list of system fonts from most-to-least-desirable.
        FcResult result;
        fs = FcFontSort(nullptr, sortpat, elide_unicode_coverage, nullptr, &result);
        FcPatternDestroy(sortpat);
    }

    int found_font_count = 0;
    if (fs)
    {
        // Get the full pathnames to the fonts, where available,
        // which is what we really want.
        found_font_count = fs->nfont;
        for (int i=0; i<fs->nfont; ++i)
        {
            FcChar8 *filename;
            if (FcResultMatch == FcPatternGetString(fs->fonts[i], FC_FILE, 0, &filename) && filename)
            {
                rtns.push_back(std::string((const char*)filename));
                if (rtns.size() >= max_font_count_cutoff)
                    break; // hit limit
            }
        }
        FcFontSetDestroy (fs);
    }

    LL_DEBUGS() << "Using font list: " << LL_ENDL;
    for (auto it = rtns.begin(); it != rtns.end(); ++it)
    {
        LL_DEBUGS() << "  file: " << *it << LL_ENDL;
    }

    LL_INFOS() << "Using " << rtns.size() << "/" << found_font_count << " system fonts." << LL_ENDL;

    rtns.push_back(final_fallback);
#endif
    return rtns;
}

void LLWindowSDL::setLanguageTextInput(const LLCoordGL& position)
{
    LLCoordWindow win_pos;
    convertCoords( position, &win_pos );

    SDL_Rect r;
    r.x = win_pos.mX;
    r.y = win_pos.mY;
    r.w = 500;
    r.h = 16;

    SDL_SetTextInputArea(mWindow, &r, 0);
}

F32 LLWindowSDL::getSystemUISize()
{
    if(mWindow)
    {
        F32 scale = SDL_GetWindowDisplayScale(mWindow);
        if (scale > 0.0f)
        {
            return scale;
        }
    }
    return 1.f;
}

#if LL_DARWIN
// static
U64 LLWindowSDL::getVramSize()
{
    CGLRendererInfoObj info = 0;
    GLint vram_megabytes = 0;
    int num_renderers = 0;
    CGLError the_err = CGLQueryRendererInfo (CGDisplayIDToOpenGLDisplayMask(kCGDirectMainDisplay), &info, &num_renderers);
    if(0 == the_err)
    {
        // The name, uses, and other platform definitions of gGLManager.mVRAM suggest that this is supposed to be total vram in MB,
        // rather than, say, just the texture memory. The two exceptions are:
        // 1. LLAppViewer::getViewerInfo() puts the value in a field labeled "TEXTURE_MEMORY"
        // 2. For years, this present function used kCGLRPTextureMemoryMegabytes
        // Now we use kCGLRPVideoMemoryMegabytes to bring it in line with everything else (except thatone label).
        CGLDescribeRenderer (info, 0, kCGLRPVideoMemoryMegabytes, &vram_megabytes);
        CGLDestroyRendererInfo (info);
    }
    else
    {
        vram_megabytes = 256;
    }

    return (U64)vram_megabytes; // return value is in megabytes.
}

//static
void LLWindowSDL::setUseMultGL(bool use_mult_gl)
{
    bool was_enabled = sUseMultGL;

    sUseMultGL = use_mult_gl;

    if (gGLManager.mInited)
    {
        CGLContextObj ctx = CGLGetCurrentContext();
        //enable multi-threaded OpenGL (whether or not sUseMultGL actually changed)
        if (sUseMultGL)
        {
            CGLError cgl_err =  CGLEnable( ctx, kCGLCEMPEngine);
            if (cgl_err != kCGLNoError )
            {
                LL_INFOS("GLInit") << "Multi-threaded OpenGL not available." << LL_ENDL;
                sUseMultGL = false;
            }
            else
            {
                LL_INFOS("GLInit") << "Multi-threaded OpenGL enabled." << LL_ENDL;
            }
        }
        else if (was_enabled)
        {
            CGLDisable( ctx, kCGLCEMPEngine);
            LL_INFOS("GLInit") << "Multi-threaded OpenGL disabled." << LL_ENDL;
        }
    }
}
#endif

