/**
 * @file llsdl.cpp
 * @brief SDL2 initialization
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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

#include <initializer_list>
#include <list>

#include "SDL3/SDL.h"

#ifndef LL_SDL_APP
#define SDL_MAIN_HANDLED 1
#include "SDL3/SDL_main.h"
#endif

#include "llerror.h"
#include "llwindow.h"

bool gSDLMainHandled = false;

void sdl_logger(void *userdata, int category, SDL_LogPriority priority, const char *message)
{
    switch (priority)
    {
        case SDL_LOG_PRIORITY_TRACE:
        case SDL_LOG_PRIORITY_VERBOSE:
        case SDL_LOG_PRIORITY_DEBUG:
            LL_DEBUGS("SDL") << "log='" << message << "'" << LL_ENDL;
            break;
        case SDL_LOG_PRIORITY_INFO:
            LL_INFOS("SDL") << "log='" << message << "'" << LL_ENDL;
            break;
        case SDL_LOG_PRIORITY_WARN:
        case SDL_LOG_PRIORITY_ERROR:
        case SDL_LOG_PRIORITY_CRITICAL:
            LL_WARNS("SDL") << "log='" << message << "'" << LL_ENDL;
            break;
        case SDL_LOG_PRIORITY_INVALID:
        default:
            break;
    }
}

void init_sdl(const std::string& app_name)
{
#ifndef LL_SDL_APP
    if (!gSDLMainHandled)
    {
        SDL_SetMainReady();
    }
#endif

    SDL_SetLogOutputFunction(&sdl_logger, nullptr);

    const int c_sdl_version = SDL_VERSION;
    LL_INFOS() << "Compiled against SDL "
               << SDL_VERSIONNUM_MAJOR(c_sdl_version) << "."
               << SDL_VERSIONNUM_MINOR(c_sdl_version) << "."
               << SDL_VERSIONNUM_MICRO(c_sdl_version) << LL_ENDL;
    const int r_sdl_version = SDL_GetVersion();
    LL_INFOS() << "Running with SDL "
               << SDL_VERSIONNUM_MAJOR(r_sdl_version) << "."
               << SDL_VERSIONNUM_MINOR(r_sdl_version) << "."
               << SDL_VERSIONNUM_MICRO(r_sdl_version) << LL_ENDL;

#if LL_WINDOWS && defined(LL_SDL_WINDOW)
    Uint32 style = 0;
#if defined(CS_BYTEALIGNCLIENT) && defined(CS_OWNDC)
    style = (CS_BYTEALIGNCLIENT | CS_OWNDC);
#endif
    SDL_RegisterApp(app_name.c_str(), style, nullptr);
#endif

#if LL_SDL_WINDOW
    // For linux we SDL_INIT_VIDEO and _AUDIO
    std::initializer_list<std::tuple< char const*, char const * > > hintList =
            {
                    {SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR,"0"},
                    {SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH,"1"},
                    {SDL_HINT_MOUSE_EMULATE_WARP_WITH_RELATIVE,"0"},
                    {SDL_HINT_KEYCODE_OPTIONS,"french_numbers,latin_letters"}
            };

    for (auto hint: hintList)
    {
        SDL_SetHint(std::get<0>(hint), std::get<1>(hint));
    }

    std::initializer_list<std::tuple<uint32_t, char const*, bool>> initList=
            {
                {SDL_INIT_VIDEO,"SDL_INIT_VIDEO", true},
                {SDL_INIT_JOYSTICK,"SDL_INIT_JOYSTICK", true},
                {SDL_INIT_GAMEPAD,"SDL_INIT_GAMEPAD", true},
            };
#else
    // For non-linux platforms we still SDL_INIT_VIDEO because it is a pre-requisite
    // for SDL_INIT_GAMECONTROLLER.
    std::initializer_list<std::tuple<uint32_t, char const*, bool>> initList=
            {
                {SDL_INIT_VIDEO,"SDL_INIT_VIDEO", false},
            };
#endif // LL_LINUX
    // We SDL_INIT_GAMECONTROLLER later in the startup process to make it
    // more likely we'll catch initial SDL_CONTROLLERDEVICEADDED events.

    for (auto subSystem : initList)
    {
        if (!SDL_InitSubSystem(std::get<0>(subSystem)))
        {
            LL_WARNS() << "SDL_InitSubSystem for " << std::get<1>(subSystem) << " failed " << SDL_GetError() << LL_ENDL;

            if (std::get<2>(subSystem))
            {
                OSMessageBox("SDL_Init() failure", "error", OSMB_OK);
                return;
            }
        }
    }
}

void quit_sdl()
{
#if LL_WINDOWS && defined(LL_SDL_WINDOW)
    SDL_UnregisterApp();
#endif
    SDL_Quit();
}
