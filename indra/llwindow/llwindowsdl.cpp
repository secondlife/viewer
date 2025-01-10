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
#include "llgamecontrol.h"

#ifdef LL_GLIB
#include <glib.h>
#endif

extern "C" {
# include "fontconfig/fontconfig.h"
}

#if LL_LINUX
// not necessarily available on random SDL platforms, so #if LL_LINUX
// for execv(), waitpid(), fork()
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#endif // LL_LINUX

extern bool gDebugWindowProc;

const S32 MAX_NUM_RESOLUTIONS = 200;

//
// LLWindowSDL
//

#include <X11/Xutil.h>

// TOFU HACK -- (*exactly* the same hack as LLWindowMacOSX for a similar
// set of reasons): Stash a pointer to the LLWindowSDL object here and
// maintain in the constructor and destructor.  This assumes that there will
// be only one object of this class at any time.  Currently this is true.
static LLWindowSDL *gWindowImplementation = nullptr;

void maybe_lock_display(void)
{
    if (gWindowImplementation && gWindowImplementation->Lock_Display)
        gWindowImplementation->Lock_Display();
}

void maybe_unlock_display(void)
{
    if (gWindowImplementation && gWindowImplementation->Unlock_Display)
        gWindowImplementation->Unlock_Display();
}


Window LLWindowSDL::get_SDL_XWindowID(void)
{
    if (gWindowImplementation)
        return gWindowImplementation->mX11Data.mXWindowID;
    return None;
}

Display* LLWindowSDL::get_SDL_Display(void)
{
    if (gWindowImplementation)
        return gWindowImplementation->mX11Data.mDisplay;
    return nullptr;
}

/*
 * In wayland a window does not have a state of "minimized" or gets messages that it got minimized [1]
 * There's two ways to approach this challenge:
 *   1) Ignore it, this would mean even if minimized/not visible the viewer will always fun at full fps
 *   2) Try to detect something like "minimized", the best way I found so far is to as for frame_done events. Those will
 *   only happen if parts of the viewer are visible. As such it is not the same a "minimized" but rather "this window
 *   is not fully hidden behind another window or minimized". That's (I guess) still better than nothing and running
 *   full tilt even if hidden.
 *
 * [1] As of 2024-09, maybe in the future we get nice things
*/
#ifdef LL_WAYLAND
#include <wayland-client-protocol.h>
#include <dlfcn.h>

uint32_t (*ll_wl_proxy_get_version)(struct wl_proxy *proxy);
void (*ll_wl_proxy_destroy)(struct wl_proxy *proxy);
int (*ll_wl_proxy_add_listener)(struct wl_proxy *proxy, void (**implementation)(void), void *data);

wl_interface  *ll_wl_callback_interface;

int ll_wl_callback_add_listener(struct wl_callback *wl_callback,
                         const struct wl_callback_listener *listener, void *data)
{
    return ll_wl_proxy_add_listener((struct wl_proxy *) wl_callback,
                                 (void (**)(void)) listener, data);
}

struct wl_proxy* (*ll_wl_proxy_marshal_flags)(struct wl_proxy *proxy, uint32_t opcode,
                       const struct wl_interface *interface,
                       uint32_t version,
                       uint32_t flags, ...);


bool loadWaylandClient()
{
    auto *pSO = dlopen( "libwayland-client.so.0", RTLD_NOW);
    if( !pSO )
        return false;


    ll_wl_callback_interface = (wl_interface*)dlsym(pSO, "wl_callback_interface");
    *(void**)&ll_wl_proxy_marshal_flags = dlsym(pSO, "wl_proxy_marshal_flags");
    *(void**)&ll_wl_proxy_get_version = dlsym(pSO, "wl_proxy_get_version");
    *(void**)&ll_wl_proxy_destroy = dlsym(pSO, "wl_proxy_destroy");
    *(void**)&ll_wl_proxy_add_listener = dlsym(pSO, "wl_proxy_add_listener");

    return ll_wl_callback_interface != nullptr && ll_wl_proxy_marshal_flags != nullptr &&
            ll_wl_proxy_get_version != nullptr && ll_wl_proxy_destroy != nullptr &&
            ll_wl_proxy_add_listener != nullptr;
}

struct wl_callback* ll_wl_surface_frame(struct wl_surface *wl_surface)
{
    auto version = ll_wl_proxy_get_version((struct wl_proxy *) wl_surface);

    auto callback = ll_wl_proxy_marshal_flags((struct wl_proxy *) wl_surface,
                                      WL_SURFACE_FRAME, ll_wl_callback_interface, version,
                                      0, nullptr);

    return (struct wl_callback *) callback;
}

void ll_wl_callback_destroy(struct wl_callback *wl_callback)
{
    ll_wl_proxy_destroy((struct wl_proxy *) wl_callback);
}

void LLWindowSDL::waylandFrameDoneCB(void *data, struct wl_callback *cb, uint32_t time)
{
    static uint32_t frame_count {0};
    static F64SecondsImplicit lastInfo{0};

    ++frame_count;

    ll_wl_callback_destroy(cb);
    auto pThis = reinterpret_cast<LLWindowSDL*>(data);
    pThis->mWaylandData.mLastFrameEvent = LLTimer::getTotalTime();

    auto now = LLTimer::getElapsedSeconds();
    if( lastInfo > 0)
    {
        auto diff = now.value() - lastInfo.value();
        constexpr double FPS_INFO_INTERVAL = 60.f * 5.f;
        if( diff >= FPS_INFO_INTERVAL)
        {
            double fFPS = frame_count;
            fFPS /= diff;
            LL_INFOS() << "Wayland: FPS " << fFPS << " fps, " << frame_count << " #frames time " << (diff) << LL_ENDL;
            frame_count = 0;
            lastInfo = now;
        }
    }
    else
        lastInfo = now;

    pThis->setupWaylandFrameCallback(); // ask for a new frame
}

bool LLWindowSDL::isWaylandWindowNotDrawing() const
{
    if(!mWaylandLoaded || Wayland != mServerProtocol || mWaylandData.mLastFrameEvent == 0)
        return false;

    auto currentTime = LLTimer::getTotalTime();
    if( (currentTime - mWaylandData.mLastFrameEvent) > 250000 )
        return true;

    return false;
}

void LLWindowSDL::setupWaylandFrameCallback()
{
    if(!mWaylandLoaded) return;

    static  wl_callback_listener frame_listener { nullptr };
    frame_listener.done = &LLWindowSDL::waylandFrameDoneCB;

    auto cb = ll_wl_surface_frame(mWaylandData.mSurface);
    ll_wl_callback_add_listener(cb, &frame_listener, this);
}
#else
bool LLWindowSDL::isWaylandWindowNotDrawing()
{
    return false;
}
void LLWindowSDL::setupWaylandFrameCallback()
{
    LL_WARNS() << "Viewer is running under Wayland, but was not compiled with full wayland support!" << LL_ENDL;
}
#endif

LLWindowSDL::LLWindowSDL(LLWindowCallbacks* callbacks,
                         const std::string& title, const std::string& name, S32 x, S32 y, S32 width,
                         S32 height, U32 flags,
                         bool fullscreen, bool clearBg,
                         bool enable_vsync, bool use_gl,
                         bool ignore_pixel_depth, U32 fsaa_samples)
        : LLWindow(callbacks, fullscreen, flags),
        Lock_Display(nullptr),
        Unlock_Display(nullptr), mGamma(1.0f)
{
    // Initialize the keyboard
    gKeyboard = new LLKeyboardSDL();
    gKeyboard->setCallbacks(callbacks);

    // Assume 4:3 aspect ratio until we know better
    mOriginalAspectRatio = 1024.0 / 768.0;

    if (title.empty())
        mWindowTitle = "Second Life";
    else
        mWindowTitle = title;

    // Create the GL context and set it up for windowed or fullscreen, as appropriate.
    if(createContext(x, y, width, height, 32, fullscreen, enable_vsync))
    {
        gGLManager.initGL();

        //start with arrow cursor
        initCursors();
        setCursor( UI_CURSOR_ARROW );
    }

    stop_glerror();

    // Stash an object pointer for OSMessageBox()
    gWindowImplementation = this;

    mFlashing = false;

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

            LL_INFOS() << "createContext: searching for a display mode, original aspect is " << mOriginalAspectRatio << LL_ENDL;

            for(S32 i=0; i < resolutionCount; i++)
            {
                F32 aspect = (F32)resolutionList[i].mWidth / (F32)resolutionList[i].mHeight;

                LL_INFOS() << "createContext: width " << resolutionList[i].mWidth << " height " << resolutionList[i].mHeight << " aspect " << aspect << LL_ENDL;

                if( (resolutionList[i].mHeight >= 700) && (resolutionList[i].mHeight <= 800) &&
                    (fabs(aspect - mOriginalAspectRatio) < fabs(closestAspect - mOriginalAspectRatio)))
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

    U32 context_flags = 0;
    if (gDebugGL)
    {
        context_flags |= SDL_GL_CONTEXT_DEBUG_FLAG;
    }
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, context_flags);
    SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);

    int sdlflags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;

    if( mFullscreen )
    {
        sdlflags |= SDL_WINDOW_FULLSCREEN;
        tryFindFullscreenSize( width, height );
    }

    mWindow = SDL_CreateWindow( mWindowTitle.c_str(), x, y, width, height, sdlflags );
    if (mWindow == nullptr)
    {
        LL_WARNS() << "Window creation failure. SDL: " << SDL_GetError() << LL_ENDL;
        setupFailure("Window creation error", "Error", OSMB_OK);
    }

    // Create the context
    mContext = SDL_GL_CreateContext(mWindow);
    if(!mContext)
    {
        LL_WARNS() << "Cannot create GL context " << SDL_GetError() << LL_ENDL;
        close();
        setupFailure("GL Context creation error", "Error", OSMB_OK);
    }

    if (SDL_GL_MakeCurrent(mWindow, mContext) != 0)
    {
        LL_WARNS() << "Failed to make context current. SDL: " << SDL_GetError() << LL_ENDL;
        close();
        setupFailure("GL Context failed to set current failure", "Error", OSMB_OK);
    }

    mSurface = SDL_GetWindowSurface(mWindow);
    if(mFullscreen)
    {
        if (mSurface)
        {
            mFullscreen = true;
            mFullscreenWidth = mSurface->w;
            mFullscreenHeight = mSurface->h;
            mFullscreenRefresh = -1;

            LL_INFOS() << "Running at " << mFullscreenWidth
                       << "x"   << mFullscreenHeight
                       << " @ " << mFullscreenRefresh
                       << LL_ENDL;
        }
        else
        {
            LL_WARNS() << "createContext: fullscreen creation failure. SDL: " << SDL_GetError() << LL_ENDL;

            mFullscreen = false;
            mFullscreenWidth   = -1;
            mFullscreenHeight  = -1;
            mFullscreenRefresh = -1;

            std::string error = llformat("Unable to run fullscreen at %d x %d.\nRunning in window.", width, height);
            setupFailure( error, "Error", OSMB_OK );
        }
    }

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
        close();
        setupFailure(
                "Second Life requires True Color (32-bit) to run in a window.\n"
                "Please go to Control Panels -> Display -> Settings and\n"
                "set the screen to 32-bit color.\n"
                "Alternately, if you choose to run fullscreen, Second Life\n"
                "will automatically adjust the screen each time it runs.",
                "Error",
                OSMB_OK);
    }

    LL_PROFILER_GPU_CONTEXT;

    // Enable vertical sync
    toggleVSync(enable_vsync);

    // Set the application icon.
    SDL_Surface* bmpsurface = Load_BMP_Resource("ll_icon.BMP");
    if (bmpsurface)
    {
        SDL_SetWindowIcon(mWindow, bmpsurface);
        SDL_FreeSurface(bmpsurface);
        bmpsurface = nullptr;
    }

    /* Grab the window manager specific information */
    SDL_SysWMinfo info;
    SDL_VERSION(&info.version);
    if ( SDL_GetWindowWMInfo(mWindow, &info) )
    {
        /* Save the information for later use */
        if (info.subsystem == SDL_SYSWM_X11)
        {
            mX11Data.mDisplay = info.info.x11.display;
            mX11Data.mXWindowID = info.info.x11.window;
            mServerProtocol = X11;
            LL_INFOS() << "Running under X11" << LL_ENDL;
        }
        else if (info.subsystem == SDL_SYSWM_WAYLAND)
        {
#ifdef LL_WAYLAND
            mWaylandLoaded = loadWaylandClient();
            if(!mWaylandLoaded)
            {
                LL_WARNS() << "Failed to load wayland-client.so or grab required functions" << LL_ENDL;
            }
#endif

            mWaylandData.mSurface = info.info.wl.surface;
            mServerProtocol = Wayland;

            setupWaylandFrameCallback();

            // If set (XWayland) remove DISPLAY, this will prompt dullahan to also use Wayland
            if( getenv("DISPLAY") )
                unsetenv("DISPLAY");

            LL_INFOS() << "Running under Wayland" << LL_ENDL;
            LL_WARNS() << "Be aware that with at least SDL2 the window will not receive minimizing events, thus minimized state can only be estimated." << LL_ENDL;
        }
        else
        {
            LL_WARNS() << "Unsupported windowing system" << LL_ENDL;
        }
    }
    else
    {
        LL_WARNS() << "We're not running under any known WM. Wild." << LL_ENDL;
    }

    SDL_StartTextInput();
    //make sure multisampling is disabled by default
    glDisable(GL_MULTISAMPLE_ARB);

    // Don't need to get the current gamma, since there's a call that restores it to the system defaults.
    return true;
}

void* LLWindowSDL::createSharedContext()
{
    SDL_GLContext pContext = SDL_GL_CreateContext(mWindow);
    if (pContext)
    {
        LL_DEBUGS() << "Creating shared OpenGL context successful!" << LL_ENDL;
        return (void*)pContext;
    }

    LL_WARNS() << "Creating shared OpenGL context failed!" << LL_ENDL;
    return nullptr;
}

void LLWindowSDL::makeContextCurrent(void* contextPtr)
{
    SDL_GL_MakeCurrent(mWindow, contextPtr);
    LL_PROFILER_GPU_CONTEXT;
}

void LLWindowSDL::destroySharedContext(void* contextPtr)
{
    SDL_GL_DeleteContext(contextPtr);
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

    // Stop unicode input
    SDL_StopTextInput();

    // Clean up remaining GL state before blowing away window
    LL_INFOS() << "shutdownGL begins" << LL_ENDL;
    gGLManager.shutdownGL();

    mX11Data.mDisplay = nullptr;
    mX11Data.mXWindowID = None;
    Lock_Display = nullptr;
    Unlock_Display = nullptr;
    mServerProtocol = Unknown;

    LL_INFOS() << "Destroying SDL cursors" << LL_ENDL;
    quitCursors();

    if (mContext)
    {
        LL_INFOS() << "Destroying SDL GL Context" << LL_ENDL;
        SDL_GL_DeleteContext(mContext);
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

    LL_INFOS() << "SDL_QuitSS/VID begins" << LL_ENDL;
    SDL_QuitSubSystem(SDL_INIT_VIDEO);  // *FIX: this might be risky...
}

LLWindowSDL::~LLWindowSDL()
{
    destroyContext();

    delete []mSupportedResolutions;

    gWindowImplementation = nullptr;
}


void LLWindowSDL::show()
{
    if (mWindow)
    {
        SDL_ShowWindow(mWindow);
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

bool LLWindowSDL::getVisible() const
{
    bool result = false;
    if (mWindow)
    {
        Uint32 flags = SDL_GetWindowFlags(mWindow);
        if (flags & SDL_WINDOW_SHOWN)
        {
            result = true;
        }
    }
    return result;
}

bool LLWindowSDL::getMinimized() const
{
    if( isWaylandWindowNotDrawing() )
        return true;

    bool result = false;
    if (mWindow)
    {
        Uint32 flags = SDL_GetWindowFlags(mWindow);
        if (flags & SDL_WINDOW_MINIMIZED)
        {
            result = true;
        }
    }
    return result;
}

bool LLWindowSDL::getMaximized() const
{
    bool result = false;
    if (mWindow)
    {
        Uint32 flags = SDL_GetWindowFlags(mWindow);
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

bool LLWindowSDL::getPosition(LLCoordScreen *position) const
{
    if (mWindow)
    {
        SDL_GetWindowPosition(mWindow, &position->mX, &position->mY);
        return true;
    }
    return false;
}

bool LLWindowSDL::getSize(LLCoordScreen *size) const
{
    if (mSurface)
    {
        size->mX = mSurface->w;
        size->mY = mSurface->h;
        return true;
    }

    return false;
}

bool LLWindowSDL::getSize(LLCoordWindow *size) const
{
    if (mSurface)
    {
        size->mX = mSurface->w;
        size->mY = mSurface->h;
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

    auto nFlags = SDL_GetWindowFlags( pWin );

    if( nFlags & SDL_WINDOW_MAXIMIZED )
        SDL_RestoreWindow( pWin );

    SDL_SetWindowSize( pWin, newSize.mX, newSize.mY );
    SDL_Event event;
    event.type = SDL_WINDOWEVENT;
    event.window.event = SDL_WINDOWEVENT_RESIZED;
    event.window.windowID = SDL_GetWindowID( pWin );
    event.window.data1 = newSize.mX;
    event.window.data2 = newSize.mY;
    SDL_PushEvent( &event );

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

U32 LLWindowSDL::getFSAASamples() const
{
    return mFSAASamples;
}

void LLWindowSDL::setFSAASamples(const U32 samples)
{
    mFSAASamples = samples;
}

F32 LLWindowSDL::getGamma() const
{
    return 1.f / mGamma;
}

bool LLWindowSDL::restoreGamma()
{
    if (mWindow)
    {
        Uint16 ramp[256];
        SDL_CalculateGammaRamp(1.f, ramp);
        SDL_SetWindowGammaRamp(mWindow, ramp, ramp, ramp);
    }
    return true;
}

bool LLWindowSDL::setGamma(const F32 gamma)
{
    if (mWindow)
    {
        Uint16 ramp[256];

        mGamma = gamma;
        if (mGamma == 0)
            mGamma = 0.1f;
        mGamma = 1.f / mGamma;

        SDL_CalculateGammaRamp(mGamma, ramp);
        SDL_SetWindowGammaRamp(mWindow, ramp, ramp, ramp);
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
    if( mWindow )
        SDL_SetWindowGrab( mWindow, b?SDL_TRUE:SDL_FALSE );
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
    bool result = true;
    LLCoordScreen screen_pos;

    if (!convertCoords(position, &screen_pos))
        return false;

    // do the actual forced cursor move.
    SDL_WarpMouseInWindow(mWindow, screen_pos.mX, screen_pos.mY);

    return result;
}

bool LLWindowSDL::getCursorPosition(LLCoordWindow *position)
{
    //Point cursor_point;
    LLCoordScreen screen_pos;

    int x, y;
    SDL_GetMouseState(&x, &y);

    screen_pos.mX = x;
    screen_pos.mY = y;

    return convertCoords(screen_pos, position);
}

F32 LLWindowSDL::getNativeAspectRatio()
{
    // MBW -- there are a couple of bad assumptions here.  One is that the display list won't include
    //      ridiculous resolutions nobody would ever use.  The other is that the list is in order.

    // New assumptions:
    // - pixels are square (the only reasonable choice, really)
    // - The user runs their display at a native resolution, so the resolution of the display
    //    when the app is launched has an aspect ratio that matches the monitor.

    //RN: actually, the assumption that there are no ridiculous resolutions (above the display's native capabilities) has
    // been born out in my experience.
    // Pixels are often not square (just ask the people who run their LCDs at 1024x768 or 800x600 when running fullscreen, like me)
    // The ordering of display list is a blind assumption though, so we should check for max values
    // Things might be different on the Mac though, so I'll defer to MBW

    // The constructor for this class grabs the aspect ratio of the monitor before doing any resolution
    // switching, and stashes it in mOriginalAspectRatio.  Here, we just return it.

    if (mOverrideAspectRatio > 0.f)
        return mOverrideAspectRatio;

    return mOriginalAspectRatio;
}

F32 LLWindowSDL::getPixelAspectRatio()
{
    F32 pixel_aspect = 1.f;
    if (getFullscreen())
    {
        LLCoordScreen screen_size;
        if (getSize(&screen_size))
            pixel_aspect = getNativeAspectRatio() * (F32)screen_size.mY / (F32)screen_size.mX;
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

    if (mServerProtocol == X11 && mX11Data.mDisplay)
    {
        // Everything that we/SDL asked for should happen before we
        // potentially hand control over to GTK.
        maybe_lock_display();
        XSync(mX11Data.mDisplay, False);
        maybe_unlock_display();
    }

    maybe_lock_display();
}

void LLWindowSDL::afterDialog()
{
    LL_INFOS() << "LLWindowSDL::afterDialog()" << LL_ENDL;

    maybe_unlock_display();

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
    return SDL_HasClipboardText() == SDL_TRUE;
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
    return SDL_SetClipboardText(utf8.c_str()) == 0; // success == 0
}

bool LLWindowSDL::isPrimaryTextAvailable()
{
    return SDL_HasPrimarySelectionText() == SDL_TRUE;
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
    return SDL_SetPrimarySelectionText(utf8.c_str()) == 0; // success == 0
}

LLWindow::LLWindowResolution* LLWindowSDL::getSupportedResolutions(S32 &num_resolutions)
{
    if (!mSupportedResolutions)
    {
        mSupportedResolutions = new LLWindowResolution[MAX_NUM_RESOLUTIONS];
        mNumSupportedResolutions = 0;

        // <FS:ND> Use display no from mWindow/mSurface here?
        int max = SDL_GetNumDisplayModes(0);
        max = llclamp( max, 0, MAX_NUM_RESOLUTIONS );

        for( int i =0; i < max; ++i )
        {
            SDL_DisplayMode mode = { SDL_PIXELFORMAT_UNKNOWN, 0, 0, 0, 0 };
            if (SDL_GetDisplayMode( 0 , i, &mode) != 0)
                continue;

            int w = mode.w;
            int h = mode.h;
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
    }

    num_resolutions = mNumSupportedResolutions;
    return mSupportedResolutions;
}

bool LLWindowSDL::convertCoords(LLCoordGL from, LLCoordWindow *to) const
{
    if (!to)
        return false;

    to->mX = from.mX;
    to->mY = mSurface->h - from.mY - 1;

    return true;
}

bool LLWindowSDL::convertCoords(LLCoordWindow from, LLCoordGL* to) const
{
    if (!to)
        return false;

    to->mX = from.mX;
    to->mY = mSurface->h - from.mY - 1;

    return true;
}

bool LLWindowSDL::convertCoords(LLCoordScreen from, LLCoordWindow* to) const
{
    if (!to)
        return false;

    // In the fullscreen case, window and screen coordinates are the same.
    to->mX = from.mX;
    to->mY = from.mY;
    return true;
}

bool LLWindowSDL::convertCoords(LLCoordWindow from, LLCoordScreen *to) const
{
    if (!to)
        return false;

    // In the fullscreen case, window and screen coordinates are the same.
    to->mX = from.mX;
    to->mY = from.mY;
    return true;
}

bool LLWindowSDL::convertCoords(LLCoordScreen from, LLCoordGL *to) const
{
    LLCoordWindow window_coord;

    return convertCoords(from, &window_coord) && convertCoords(window_coord, to);
}

bool LLWindowSDL::convertCoords(LLCoordGL from, LLCoordScreen *to) const
{
    LLCoordWindow window_coord;

    return convertCoords(from, &window_coord) && convertCoords(window_coord, to);
}

void LLWindowSDL::setupFailure(const std::string& text, const std::string& caption, U32 type)
{
    destroyContext();

    OSMessageBox(text, caption, type);

    // This is so catastrophic > bail as fast as possible. Otherwise the viewer can be stuck in a perpetual state of startup pain
    std::terminate();
}

bool LLWindowSDL::SDLReallyCaptureInput(bool capture)
{
    if (!mFullscreen && mWindow ) /* only bother if we're windowed anyway */
        SDL_SetWindowGrab( mWindow, capture?SDL_TRUE:SDL_FALSE);

    return capture;
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


void check_vm_bloat()
{
#if LL_LINUX
    // watch our own VM and RSS sizes, warn if we bloated rapidly
    static const std::string STATS_FILE = "/proc/self/stat";
    FILE *fp = fopen(STATS_FILE.c_str(), "r");
    if (fp)
    {
        static long long last_vm_size = 0;
        static long long last_rss_size = 0;
        const long long significant_vm_difference = 250 * 1024*1024;
        const long long significant_rss_difference = 50 * 1024*1024;
        long long this_vm_size = 0;
        long long this_rss_size = 0;

        ssize_t res;
        size_t dummy;
        char *ptr = nullptr;
        for (int i=0; i<22; ++i) // parse past the values we don't want
        {
            res = getdelim(&ptr, &dummy, ' ', fp);
            if (-1 == res)
            {
                LL_WARNS() << "Unable to parse " << STATS_FILE << LL_ENDL;
                goto finally;
            }
            free(ptr);
            ptr = nullptr;
        }
        // 23rd space-delimited entry is vsize
        res = getdelim(&ptr, &dummy, ' ', fp);
        llassert(ptr);
        if (-1 == res)
        {
            LL_WARNS() << "Unable to parse " << STATS_FILE << LL_ENDL;
            goto finally;
        }
        this_vm_size = atoll(ptr);
        free(ptr);
        ptr = nullptr;
        // 24th space-delimited entry is RSS
        res = getdelim(&ptr, &dummy, ' ', fp);
        llassert(ptr);
        if (-1 == res)
        {
            LL_WARNS() << "Unable to parse " << STATS_FILE << LL_ENDL;
            goto finally;
        }
        this_rss_size = getpagesize() * atoll(ptr);
        free(ptr);
        ptr = nullptr;

        LL_INFOS() << "VM SIZE IS NOW " << (this_vm_size/(1024*1024)) << " MB, RSS SIZE IS NOW " << (this_rss_size/(1024*1024)) << " MB" << LL_ENDL;

        if (llabs(last_vm_size - this_vm_size) > significant_vm_difference)
        {
            if (this_vm_size > last_vm_size)
            {
                LL_WARNS() << "VM size grew by " << (this_vm_size - last_vm_size)/(1024*1024) << " MB in last frame" << LL_ENDL;
            }
            else
            {
                LL_INFOS() << "VM size shrank by " << (last_vm_size - this_vm_size)/(1024*1024) << " MB in last frame" << LL_ENDL;
            }
        }

        if (llabs(last_rss_size - this_rss_size) > significant_rss_difference)
        {
            if (this_rss_size > last_rss_size)
            {
                LL_WARNS() << "RSS size grew by " << (this_rss_size - last_rss_size)/(1024*1024) << " MB in last frame" << LL_ENDL;
            }
            else
            {
                LL_INFOS() << "RSS size shrank by " << (last_rss_size - this_rss_size)/(1024*1024) << " MB in last frame" << LL_ENDL;
            }
        }

        last_rss_size = this_rss_size;
        last_vm_size = this_vm_size;

finally:
        if (ptr)
            free(ptr);
        fclose(fp);
    }
#endif
}


// virtual
void LLWindowSDL::processMiscNativeEvents()
{
    // Pump until we've nothing left to do or passed 1/15th of a
    // second pumping for this frame.
    static LLTimer pump_timer;
    pump_timer.reset();
    pump_timer.setTimerExpirySec(1.0f / 15.0f);
    do
    {
        g_main_context_iteration(g_main_context_default(), false);
    } while( g_main_context_pending(g_main_context_default()) && !pump_timer.hasExpired());

    // hack - doesn't belong here - but this is just for debugging
    if (getenv("LL_DEBUG_BLOAT"))
        check_vm_bloat();
}

void LLWindowSDL::gatherInput(bool app_has_focus)
{
    SDL_Event event;

    // Handle all outstanding SDL events
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
            case SDL_MOUSEWHEEL:
            {
                if( event.wheel.y != 0 )
                {
                    mCallbacks->handleScrollWheel(this, -event.wheel.y);
                }
                if (event.wheel.x != 0)
                {
                    mCallbacks->handleScrollHWheel(this, -event.wheel.x);
                }
                break;
            }

            case SDL_MOUSEMOTION:
            {
                LLCoordWindow winCoord(event.motion.x, event.motion.y);
                LLCoordGL openGlCoord;
                convertCoords(winCoord, &openGlCoord);
                MASK mask = gKeyboard->currentMask(true);
                mCallbacks->handleMouseMove(this, openGlCoord, mask);
                break;
            }

            case SDL_TEXTINPUT:
            {
                auto string = utf8str_to_utf16str( event.text.text );
                mKeyModifiers = gKeyboard->currentMask( false );

                for( auto key: string )
                {
                    mKeyVirtualKey = key;

                    if( (MASK_CONTROL|MASK_ALT)&mKeyModifiers )
                        gKeyboard->handleKeyDown(mKeyVirtualKey, mKeyModifiers );
                    else
                        handleUnicodeUTF16( key, mKeyModifiers );
                }
                break;
            }

            case SDL_KEYDOWN:
                mKeyVirtualKey = event.key.keysym.sym;
                mKeyModifiers = event.key.keysym.mod;

                // treat all possible Enter/Return keys the same
                if (mKeyVirtualKey == SDLK_RETURN2 || mKeyVirtualKey == SDLK_KP_ENTER)
                    mKeyVirtualKey = SDLK_RETURN;

                gKeyboard->handleKeyDown(mKeyVirtualKey, mKeyModifiers );

                // <FS:ND> Slightly hacky :| To make the viewer honor enter (eg to accept form input) we've to not only send handleKeyDown but also send a
                // invoke handleUnicodeUTF16 in case the user hits return.
                // Note that we cannot blindly use handleUnicodeUTF16 for each SDL_KEYDOWN. Doing so will create bogus keyboard input (like % for cursor left).
                if( mKeyVirtualKey == SDLK_RETURN )
                {
                    // fix return key not working when capslock, scrolllock or numlock are enabled
                    mKeyModifiers &= (~(KMOD_NUM | KMOD_CAPS | KMOD_MODE | KMOD_SCROLL));
                    handleUnicodeUTF16( mKeyVirtualKey, mKeyModifiers );
                }

                // part of the fix for SL-13243
                if (SDLCheckGrabbyKeys(event.key.keysym.sym, true) != 0)
                    SDLReallyCaptureInput(true);

                break;

            case SDL_KEYUP:
                mKeyVirtualKey = event.key.keysym.sym;
                mKeyModifiers = event.key.keysym.mod;

                // treat all possible Enter/Return keys the same
                if (mKeyVirtualKey == SDLK_RETURN2 || mKeyVirtualKey == SDLK_KP_ENTER)
                    mKeyVirtualKey = SDLK_RETURN;

                if (SDLCheckGrabbyKeys(mKeyVirtualKey, false) == 0)
                    SDLReallyCaptureInput(false); // part of the fix for SL-13243

                gKeyboard->handleKeyUp(mKeyVirtualKey,mKeyModifiers);
                break;

            case SDL_MOUSEBUTTONDOWN:
            {
                LLCoordWindow winCoord(event.button.x, event.button.y);
                LLCoordGL openGlCoord;
                convertCoords(winCoord, &openGlCoord);
                MASK mask = gKeyboard->currentMask(true);

                if (event.button.button == SDL_BUTTON_LEFT)  // left
                {
                    if (event.button.clicks >= 2)
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

            case SDL_MOUSEBUTTONUP:
            {
                LLCoordWindow winCoord(event.button.x, event.button.y);
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

            case SDL_WINDOWEVENT:
            {
                switch(event.window.event)
                {
                    //case SDL_WINDOWEVENT_SIZE_CHANGED: <FS:ND> SDL_WINDOWEVENT_SIZE_CHANGED is followed by SDL_WINDOWEVENT_RESIZED, so handling one shall be enough
                    case SDL_WINDOWEVENT_RESIZED:
                    {
                        LL_INFOS() << "Handling a resize event: " << event.window.data1 << "x" << event.window.data2 << LL_ENDL;
                        S32 width = llmax(event.window.data1, (S32)mMinWindowWidth);
                        S32 height = llmax(event.window.data2, (S32)mMinWindowHeight);

                        mSurface = SDL_GetWindowSurface(mWindow);
                        mCallbacks->handleResize(this, width, height);
                        break;
                    }
                    case SDL_WINDOWEVENT_LEAVE:
                        mCallbacks->handleMouseLeave(this);
                        break;
                    case SDL_WINDOWEVENT_FOCUS_GAINED:
                        mCallbacks->handleFocus(this);
                        break;
                    case SDL_WINDOWEVENT_FOCUS_LOST:
                        mCallbacks->handleFocusLost(this);
                        break;
                    case SDL_WINDOWEVENT_EXPOSED:
                    case SDL_WINDOWEVENT_SHOWN:
                    case SDL_WINDOWEVENT_HIDDEN:
                    case SDL_WINDOWEVENT_MINIMIZED:
                    case SDL_WINDOWEVENT_MAXIMIZED:
                    case SDL_WINDOWEVENT_RESTORED:
                    {
                        Uint32 flags = SDL_GetWindowFlags(mWindow);
                        bool minimized = (flags & SDL_WINDOW_MINIMIZED);
                        bool hidden = (flags & SDL_WINDOW_HIDDEN);

                        mCallbacks->handleActivate(this, !minimized || !hidden);
                        LL_INFOS() << "SDL deiconification state switched to " << minimized << LL_ENDL;
                        break;
                    }
                }
                break;
            }
            case SDL_QUIT:
                if(mCallbacks->handleCloseRequest(this))
                {
                    // Get the app to initiate cleanup.
                    mCallbacks->handleQuit(this);
                    // The app is responsible for calling destroyWindow when done with GL
                }
                break;
            default:
                LLGameControl::handleEvent(event, app_has_focus);
                break;
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
        cursurface = SDL_CreateRGBSurface (SDL_SWSURFACE,
                                           bmpsurface->w,
                                           bmpsurface->h,
                                           32,
                                           SDL_SwapLE32(0xFFU),
                                           SDL_SwapLE32(0xFF00U),
                                           SDL_SwapLE32(0xFF0000U),
                                           SDL_SwapLE32(0xFF000000U));
        SDL_FillRect(cursurface, nullptr, SDL_SwapLE32(0x00000000U));

        // Blit the cursor pixel data onto a 32-bit RGBA surface so we
        // only have to cope with processing one type of pixel format.
        if (0 == SDL_BlitSurface(bmpsurface, nullptr,
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
                            + j*cursurface->format->BytesPerPixel;
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
        SDL_FreeSurface(cursurface);
        SDL_FreeSurface(bmpsurface);
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
    for ( int i=0; i<UI_CURSOR_COUNT; ++i)
        mSDLCursors[i] = nullptr;

    // Pre-make an SDL cursor for each of the known cursor types.
    // We hardcode the hotspots - to avoid that we'd have to write
    // a .cur file loader.
    // NOTE: SDL doesn't load RLE-compressed BMP files.
    mSDLCursors[UI_CURSOR_ARROW] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
    mSDLCursors[UI_CURSOR_WAIT] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_WAIT);
    mSDLCursors[UI_CURSOR_HAND] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);
    mSDLCursors[UI_CURSOR_IBEAM] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_IBEAM);
    mSDLCursors[UI_CURSOR_CROSS] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_CROSSHAIR);
    mSDLCursors[UI_CURSOR_SIZENWSE] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENWSE);
    mSDLCursors[UI_CURSOR_SIZENESW] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENESW);
    mSDLCursors[UI_CURSOR_SIZEWE] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEWE);
    mSDLCursors[UI_CURSOR_SIZENS] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENS);
    mSDLCursors[UI_CURSOR_SIZEALL] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEALL);
    mSDLCursors[UI_CURSOR_NO] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NO);
    mSDLCursors[UI_CURSOR_WORKING] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_WAITARROW);
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
                SDL_FreeCursor(mSDLCursors[i]);
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
        SDL_ShowCursor(SDL_DISABLE);
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
        SDL_ShowCursor(SDL_ENABLE);
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

    int btn{0};
    if( 0 == SDL_ShowMessageBox( &oData, &btn ) )
        return btn;
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
LLSD LLWindowSDL::getNativeKeyData() const
{
    LLSD result = LLSD::emptyMap();

    U32 modifiers = 0; // pretend-native modifiers... oh what a tangled web we weave!

    // we go through so many levels of device abstraction that I can't really guess
    // what a plugin under GDK under Qt under SL under SDL under X11 considers
    // a 'native' modifier mask.  this has been sort of reverse-engineered... they *appear*
    // to match GDK consts, but that may be co-incidence.
    modifiers |= (mKeyModifiers & KMOD_LSHIFT) ? 0x0001 : 0;
    modifiers |= (mKeyModifiers & KMOD_RSHIFT) ? 0x0001 : 0;// munge these into the same shift
    modifiers |= (mKeyModifiers & KMOD_CAPS)   ? 0x0002 : 0;
    modifiers |= (mKeyModifiers & KMOD_LCTRL)  ? 0x0004 : 0;
    modifiers |= (mKeyModifiers & KMOD_RCTRL)  ? 0x0004 : 0;// munge these into the same ctrl
    modifiers |= (mKeyModifiers & KMOD_LALT)   ? 0x0008 : 0;// untested
    modifiers |= (mKeyModifiers & KMOD_RALT)   ? 0x0008 : 0;// untested
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

    if (SDL_OpenURL(escaped_url.c_str()) != 0)
    {
        LL_WARNS() << "spawn_web_browser failed with error: " << SDL_GetError() << LL_ENDL;
    }

    LL_INFOS() << "spawn_web_browser returning." << LL_ENDL;
}

void* LLWindowSDL::getPlatformWindow()
{
    return (void*)mWindow;
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

    SDL_SetTextInputRect(&r);
}
