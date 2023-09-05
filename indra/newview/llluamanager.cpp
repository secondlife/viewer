/** 
 * @file llluamanager.cpp
 * @brief classes and functions for interfacing with LUA. 
 *
 * $LicenseInfo:firstyear=2023&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2023, Linden Research, Inc.
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
 *
 */

#include "llviewerprecompiledheaders.h"
#include "llluamanager.h"

#include "llagent.h"
#include "llappearancemgr.h"
#include "llfloaterreg.h"
#include "llfloaterimnearbychat.h"

extern "C"
{
#include "lua/lua.h"
#include "lua/lauxlib.h"
#include "lua/lualib.h"
}

#if LL_WINDOWS
#pragma comment(lib, "liblua54.a")
#endif

int lua_printWarning(lua_State *L)
{
    std::string msg(lua_tostring(L, 1));

    LL_WARNS() << msg << LL_ENDL;
    return 1;
}

int lua_avatar_sit(lua_State *L)
{
    gAgent.sitDown();
    return 1;
}

int lua_avatar_stand(lua_State *L)
{
    gAgent.standUp();
    return 1;
}

int lua_nearby_chat_send(lua_State *L)
{
    std::string msg(lua_tostring(L, 1));
    LLFloaterIMNearbyChat *nearby_chat = LLFloaterReg::findTypedInstance<LLFloaterIMNearbyChat>("nearby_chat");
    nearby_chat->sendChatFromViewer(msg, CHAT_TYPE_NORMAL, gSavedSettings.getBOOL("PlayChatAnim"));

    return 1;
}

int lua_wear_by_name(lua_State *L)
{
    std::string folder_name(lua_tostring(L, 1));
    LLAppearanceMgr::instance().wearOutfitByName(folder_name);

    return 1;
}

bool checkLua(lua_State *L, int r)
{
    if (r != LUA_OK)
    {
        std::string lua_error = lua_tostring(L, -1);

        LL_WARNS() << lua_error << LL_ENDL;
        return false;
    }
    return true;
}

void initLUA(lua_State *L)
{
    lua_register(L, "lua_printWarning", lua_printWarning);

    lua_register(L, "lua_avatar_sit", lua_avatar_sit);
    lua_register(L, "lua_avatar_stand", lua_avatar_stand);

    lua_register(L, "lua_nearby_chat_send", lua_nearby_chat_send);
    lua_register(L, "lua_wear_by_name", lua_wear_by_name);
}

void LLLUAmanager::runScriptFile(const std::string &filename)
{
    LLCoros::instance().launch("LUAScriptFileCoro", [filename]()
    {
        lua_State *L = luaL_newstate();
        luaL_openlibs(L);
        initLUA(L);

        auto LUA_sleep_func = [](lua_State *L)
        {
            F32 seconds = lua_tonumber(L, -1);
            llcoro::suspendUntilTimeout(seconds);
            return 0;
        };

        lua_register(L, "sleep", LUA_sleep_func);

        if (checkLua(L, luaL_dofile(L, filename.c_str())))
        {
            lua_getglobal(L, "call_once_func");
            if (lua_isfunction(L, -1))
            {
                if (checkLua(L, lua_pcall(L, 0, 0, 0))) {}
            }
        }

        lua_close(L);
    });
}

void LLLUAmanager::runScriptLine(const std::string &cmd) 
{
    LLCoros::instance().launch("LUAScriptFileCoro", [cmd]()
    {
        lua_State *L = luaL_newstate();
        luaL_openlibs(L);
        initLUA(L);
        int r = luaL_dostring(L, cmd.c_str());
        if (r != LUA_OK)
        {
            std::string lua_error = lua_tostring(L, -1);
            LL_WARNS() << lua_error << LL_ENDL;
        }
        lua_close(L);
    });
}
