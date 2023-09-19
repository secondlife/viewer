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
#include "llcallbacklist.h"
#include "llfloaterreg.h"
#include "llfloaterimnearbychat.h"
#include "llfloatersidepanelcontainer.h"
#include "llnotificationsutil.h"
#include "llvoavatarself.h"
#include "llviewermenu.h"
#include "llviewermenufile.h"
#include "llviewerwindow.h"
#include "lluilistener.h"

#include <boost/algorithm/string/replace.hpp>

extern "C"
{
#include "lua/lua.h"
#include "lua/lauxlib.h"
#include "lua/lualib.h"
}

#if LL_WINDOWS
#pragma comment(lib, "liblua54.a")
#endif

// FIXME extremely hacky way to get to the UI Listener framework. There's almost certainly a cleaner way.
extern LLUIListener sUIListener;

int lua_printWarning(lua_State *L)
{
    std::string msg(lua_tostring(L, 1));

    LL_WARNS() << msg << LL_ENDL;
    return 1;
}

bool checkLua(lua_State *L, int r, std::string &error_msg)
{
    if (r != LUA_OK)
    {
        error_msg = lua_tostring(L, -1);

        LL_WARNS() << error_msg << LL_ENDL;
        return false;
    }
    return true;
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

int lua_open_floater(lua_State *L)
{
    std::string floater_name(lua_tostring(L, 1));

    LLSD key;
    if (floater_name == "profile")
    {
        key["id"] = gAgentID;
    }
    LLFloaterReg::showInstance(floater_name, key);

    return 1;
}

int lua_close_floater(lua_State *L)
{
    std::string floater_name(lua_tostring(L, 1));

    LLSD key;
    if (floater_name == "profile")
    {
        key["id"] = gAgentID;
    }
    LLFloaterReg::hideInstance(floater_name, key);

    return 1;
}

int lua_close_all_floaters(lua_State *L)
{
    close_all_windows();
    return 1;
}

int lua_snapshot_to_file(lua_State *L)
{
    std::string filename(lua_tostring(L, 1));

    //don't take snapshot from coroutine
    doOnIdleOneTime([filename]()
    {
        gViewerWindow->saveSnapshot(filename,
                                gViewerWindow->getWindowWidthRaw(),
                                gViewerWindow->getWindowHeightRaw(),
                                gSavedSettings.getBOOL("RenderUIInSnapshot"),
                                gSavedSettings.getBOOL("RenderHUDInSnapshot"),
                                FALSE,
                                LLSnapshotModel::SNAPSHOT_TYPE_COLOR,
                                LLSnapshotModel::SNAPSHOT_FORMAT_PNG);
    });

    return 1;
}

int lua_open_wearing_tab(lua_State *L)
{ 
    LLFloaterSidePanelContainer::showPanel("appearance", LLSD().with("type", "now_wearing")); 
    return 1;
}

int lua_set_debug_setting_bool(lua_State *L) 
{
    std::string setting_name(lua_tostring(L, 1));
    bool value(lua_toboolean(L, 2));

    gSavedSettings.setBOOL(setting_name, value);
    return 1;
}

    int lua_get_avatar_name(lua_State *L)
{
    std::string name = gAgentAvatarp->getFullname();
    lua_pushstring(L, name.c_str());
    return 1;
}

int lua_is_avatar_flying(lua_State *L)
{
    lua_pushboolean(L, gAgent.getFlying());
    return 1;
}

int lua_env_setting_event(lua_State *L)
{ 
    handle_env_setting_event(lua_tostring(L, 1));
    return 1;
}

void handle_notification_dialog(const LLSD &notification, const LLSD &response, lua_State *L, std::string response_cb)
{
    if (!response_cb.empty())
    {
        S32 option = LLNotificationsUtil::getSelectedOption(notification, response);

        lua_pushinteger(L, option);
        lua_setglobal(L, response_cb.c_str());
    }
}

int lua_show_notification(lua_State *L)
{
    std::string notification(lua_tostring(L, 1));

    if (lua_type(L, 2) == LUA_TTABLE)
    {
        LLSD args;

        // push first key
        lua_pushnil(L);
        while (lua_next(L, 2) != 0)
        {
            // right now -2 is key, -1 is value
            lua_rawgeti(L, -1, 1);
            lua_rawgeti(L, -2, 2);
            std::string key = lua_tostring(L, -2);
            std::string value = lua_tostring(L, -1);
            args[key] = value;
            lua_pop(L, 3);
        } 

        std::string response_cb;
        if (lua_type(L, 3) == LUA_TSTRING)
        {
            response_cb = lua_tostring(L, 3);
        }

        LLNotificationsUtil::add(notification, args, LLSD(), boost::bind(handle_notification_dialog, _1, _2, L, response_cb));
    }
    else if (lua_type(L, 2) == LUA_TSTRING)
    {
        std::string response_cb = lua_tostring(L, 2);
        LLNotificationsUtil::add(notification, LLSD(), LLSD(), boost::bind(handle_notification_dialog, _1, _2, L, response_cb));
    }
    else 
    {
        LLNotificationsUtil::add(notification);
    }

    return 1;
}

int lua_run_ui_command(lua_State *L)
{
	int top = lua_gettop(L);
	std::string func_name;
	if (top >= 1)
	{
		func_name = lua_tostring(L,1);
	}
	std::string parameter;
	if (top >= 2)
	{
		parameter = lua_tostring(L,2);
	}
	LL_WARNS("LUA") << "running ui func " << func_name << " parameter " << parameter << LL_ENDL;
	LLSD event;
	event["function"] = func_name;
	if (!parameter.empty())
	{
		event["parameter"] = parameter; 
	}
	sUIListener.call(event);

	return 1;
}

void initLUA(lua_State *L)
{
    lua_register(L, "print_warning", lua_printWarning);

    lua_register(L, "avatar_sit", lua_avatar_sit);
    lua_register(L, "avatar_stand", lua_avatar_stand);

    lua_register(L, "nearby_chat_send", lua_nearby_chat_send);
    lua_register(L, "wear_by_name", lua_wear_by_name);
    lua_register(L, "open_floater", lua_open_floater);
    lua_register(L, "close_floater", lua_close_floater);
    lua_register(L, "close_all_floaters", lua_close_all_floaters);
    lua_register(L, "open_wearing_tab", lua_open_wearing_tab);
    lua_register(L, "snapshot_to_file", lua_snapshot_to_file);

    lua_register(L, "get_avatar_name", lua_get_avatar_name);
    lua_register(L, "is_avatar_flying", lua_is_avatar_flying);

    lua_register(L, "env_setting_event", lua_env_setting_event);
    lua_register(L, "set_debug_setting_bool", lua_set_debug_setting_bool);

    lua_register(L, "show_notification", lua_show_notification);
 
    lua_register(L, "run_ui_command", lua_run_ui_command);
}

void LLLUAmanager::runScriptFile(const std::string &filename, script_finished_fn cb)
{
    LLCoros::instance().launch("LUAScriptFileCoro", [filename, cb]()
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

        std::string lua_error;
        if (checkLua(L, luaL_dofile(L, filename.c_str()), lua_error))
        {
            lua_getglobal(L, "call_once_func");
            if (lua_isfunction(L, -1))
            {
                if (checkLua(L, lua_pcall(L, 0, 0, 0), lua_error)) {}
            }
        }
        lua_close(L);

        if (cb) 
        {
            cb(lua_error);
        }
    });
}

void LLLUAmanager::runScriptLine(const std::string &cmd, script_finished_fn cb)
{
    LLCoros::instance().launch("LUAScriptFileCoro", [cmd, cb]()
    {
        lua_State *L = luaL_newstate();
        luaL_openlibs(L);
        initLUA(L);
        int r = luaL_dostring(L, cmd.c_str());

        std::string lua_error;
        if (r != LUA_OK)
        {
            lua_error = lua_tostring(L, -1);
        }
        lua_close(L);

        if (cb) 
        {
            cb(lua_error);
        }
    });
}

void LLLUAmanager::runScriptOnLogin()
{
    std::string filename = gSavedSettings.getString("AutorunLuaScriptName");
    if (filename.empty()) 
    {
        LL_INFOS() << "Script name wasn't set." << LL_ENDL;
        return;
    }

    filename = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, filename);
    if (!gDirUtilp->fileExists(filename)) 
    {
        LL_INFOS() << filename << " was not found." << LL_ENDL;
        return;
    }

    runScriptFile(filename);
}
