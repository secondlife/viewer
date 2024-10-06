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

#include "SDL2/SDL.h"

#include "llerror.h"
#include "llwindow.h"

void sdl_logger(void *userdata, int category, SDL_LogPriority priority, const char *message)
{
    LL_DEBUGS("SDL2") << "log='" << message << "'" << LL_ENDL;
}

void init_sdl()
{
    SDL_version c_sdl_version;
    SDL_VERSION(&c_sdl_version);
    LL_INFOS() << "Compiled against SDL "
               << int(c_sdl_version.major) << "."
               << int(c_sdl_version.minor) << "."
               << int(c_sdl_version.patch) << LL_ENDL;
    SDL_version r_sdl_version;
    SDL_GetVersion(&r_sdl_version);
    LL_INFOS() << "Running with SDL "
               << int(r_sdl_version.major) << "."
               << int(r_sdl_version.minor) << "."
               << int(r_sdl_version.patch) << LL_ENDL;
#ifdef LL_LINUX
    // For linux we SDL_INIT_VIDEO and _AUDIO
    std::initializer_list<std::tuple< char const*, char const * > > hintList =
            {
                    {SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR,"0"},
                    {SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH,"1"},
                    {SDL_HINT_IME_INTERNAL_EDITING,"1"}
            };

    for (auto hint: hintList)
    {
        SDL_SetHint(std::get<0>(hint), std::get<1>(hint));
    }

    std::initializer_list<std::tuple<uint32_t, char const*, bool>> initList=
            { {SDL_INIT_VIDEO,"SDL_INIT_VIDEO", true},
              {SDL_INIT_AUDIO,"SDL_INIT_AUDIO", false},
            };
#else
    // For non-linux platforms we still SDL_INIT_VIDEO because it is a pre-requisite
    // for SDL_INIT_GAMECONTROLLER.
    std::initializer_list<std::tuple<uint32_t, char const*, bool>> initList=
            { {SDL_INIT_VIDEO,"SDL_INIT_VIDEO", false},
            };
#endif // LL_LINUX
    // We SDL_INIT_GAMECONTROLLER later in the startup process to make it
    // more likely we'll catch initial SDL_CONTROLLERDEVICEADDED events.

    for (auto subSystem : initList)
    {
        if (SDL_InitSubSystem(std::get<0>(subSystem)) < 0)
        {
            LL_WARNS() << "SDL_InitSubSystem for " << std::get<1>(subSystem) << " failed " << SDL_GetError() << LL_ENDL;

            if (std::get<2>(subSystem))
            {
                OSMessageBox("SDL_Init() failure", "error", OSMB_OK);
                return;
            }
        }
    }

    SDL_LogSetOutputFunction(&sdl_logger, nullptr);
}

void quit_sdl()
{
    SDL_Quit();
}
