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

#include <cstring>                  // std::memcpy()

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

void initLUA(lua_State *L)
{
    lua_register(L, "print_warning", lua_printWarning);

    lua_register(L, "avatar_sit", lua_avatar_sit);
    lua_register(L, "avatar_stand", lua_avatar_stand);

    lua_register(L, "nearby_chat_send", lua_nearby_chat_send);
    lua_register(L, "wear_by_name", lua_wear_by_name);
    lua_register(L, "open_floater", lua_open_floater);
    lua_register(L, "close_all_floaters", lua_close_all_floaters);
    lua_register(L, "open_wearing_tab", lua_open_wearing_tab);
    lua_register(L, "snapshot_to_file", lua_snapshot_to_file);

    lua_register(L, "get_avatar_name", lua_get_avatar_name);
    lua_register(L, "is_avatar_flying", lua_is_avatar_flying);

    lua_register(L, "env_setting_event", lua_env_setting_event);
    lua_register(L, "set_debug_setting_bool", lua_set_debug_setting_bool);

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

std::string lua_tostdstring(lua_State* L, int index)
{
    size_t len;
    const char* strval{ lua_tolstring(L, index, &len) };
    return { strval, len };
}

// By analogy with existing lua_tomumble() functions, return an LLSD object
// corresponding to the Lua object at stack index 'index' in state L.
// This function assumes that a Lua caller is fully aware that they're trying
// to call a viewer function. In other words, the caller must specifically
// construct Lua data convertible to LLSD.
LLSD lua_tollsd(lua_State* L, int index)
{
    switch (lua_type(L, index))
    {
    case LUA_TNONE:
    case LUA_TNIL:
        return {};

    case LUA_TBOOLEAN:
        return bool(lua_toboolean(L, index));

    case LUA_TNUMBER:
    {
        // check if integer truncation leaves the number intact
        lua_Integer intval{ lua_tointeger(L, index) };
        lua_Number  numval{ lua_tonumber(L, index) };
        if (lua_Number(intval) == numval)
        {
            return intval;
        }
        else
        {
            return numval;
        }
    }

    case LUA_TSTRING:
        return lua_tostdstring(L, index);

    case LUA_TUSERDATA:
    {
        LLSD::Binary binary(lua_objlen(L, index));
        std::memcpy(binary.data(), lua_touserdata(L, index), binary.size());
        return binary;
    }

    case LUA_TTABLE:
    {
        // A Lua table correctly constructed to convert to LLSD will have
        // either consecutive integer keys starting at 1, which we represent
        // as an LLSD array (with Lua index 1 at C++ index 0), or will have
        // all string keys.
        // Possible looseness could include:
        // - All integer keys >= 1, but with "holes," could produce an LLSD
        //   array with isUndefined() entries at unspecified keys. We could
        //   specify a maximum size of allowable gap, and throw an error if a
        //   gap exceeds that size.
        // - A mix of integer and string keys could produce an LLSD map in
        //   which the integer keys are converted to string. (Key conversion
        //   must be performed in C++, not Lua, to avoid confusing
        //   lua_next().)
        // - However, since in Lua t[0] and t["0"] are distinct table entries,
        //   do not consider converting numeric string keys to int to return
        //   an LLSD array.
        // But until we get more experience with actual Lua scripts in
        // practice, let's say that any deviation is a Lua coding error.
        // An important property of the strict definition above is that most
        // conforming data blobs can make a round trip across the language
        // boundary and still compare equal. A non-conforming data blob would
        // lose that property.
        // Known exceptions to round trip identity:
        // - Empty LLSD map and empty LLSD array convert to empty Lua table.
        //   But empty Lua table converts to isUndefined() LLSD object.
        // - LLSD::Real with integer value returns as LLSD::Integer.
        // - LLSD::UUID, LLSD::Date and LLSD::URI all convert to Lua string,
        //   and so return as LLSD::String.
        lua_pushnil(L);             // first key
        if (! lua_next(L, index))
        {
            // it's a table, but the table is empty -- no idea if it should be
            // modeled as empty array or empty map -- return isUndefined(),
            // which can be consumed as either
            return {};
        }
        // key is at index -2, value at index -1
        // from here until lua_next() returns 0, have to lua_pop(2) if we
        // return early
        // Remember the type of the first key
        auto firstkeytype{ lua_type(L, -2) };
        switch (firstkeytype)
        {
        case LUA_TNUMBER:
        {
            LLSD result{ LLSD::emptyArray() };
            // right away expand the result array to the size we'll need
            result[lua_objlen(L, index) - 1] = LLSD();
            // track the consecutive indexes we require
            LLSD::Integer expected_index{ 0 };
            do
            {
                auto arraykeytype{ lua_type(L, -2) };
                switch (arraykeytype)
                {
                case LUA_TNUMBER:
                {
                    lua_Number actual_index{ lua_tonumber(L, -2) };
                    if (actual_index != lua_Number(++expected_index))
                    {
                        // key isn't consecutive starting at 1 (may not even be an
                        // integer) - this doesn't fit our LLSD array constraints
                        lua_pop(L, 2);
                        return luaL_error(L, "Expected array key %d, got %f instead",
                                          int(expected_index), actual_index);
                    }

                    result.append(lua_tollsd(L, -1));
                    break;
                }

                case LUA_TSTRING:
                    // break out strings specially to report the value
                    lua_pop(L, 2);
                    return luaL_error(L, "Cannot convert string array key '%s' to LLSD",
                                      lua_tostring(L, -2));

                default:
                    lua_pop(L, 2);
                    return luaL_error(L, "Cannot convert %s array key to LLSD",
                                      lua_typename(L, arraykeytype));
                }

                // remove value, keep key for next iteration
                lua_pop(L, 1);
            } while (lua_next(L, index) != 0);
            return result;
        }

        case LUA_TSTRING:
        {
            LLSD result{ LLSD::emptyMap() };
            do
            {
                auto mapkeytype{ lua_type(L, -2) };
                if (mapkeytype != LUA_TSTRING)
                {
                    lua_pop(L, 2);
                    return luaL_error(L, "Cannot convert %s map key to LLSD",
                                      lua_typename(L, mapkeytype));
                }
                
                result[lua_tostdstring(L, -2)] = lua_tollsd(L, -1);
                // remove value, keep key for next iteration
                lua_pop(L, 1);
            } while (lua_next(L, index) != 0);
            return result;
        }

        default:
            lua_pop(L, 2);
            return luaL_error(L, "Cannot convert %s table key to LLSD",
                              lua_typename(L, firstkeytype));
        }
    }

    default:
        // Other Lua entities (e.g. function, C function, light userdata,
        // thread, userdata) are not convertible to LLSD, indicating a coding
        // error in the caller.
        return luaL_error(L, "Cannot convert type %s to LLSD",
                          lua_typename(L, lua_type(L, index)));
    }
}

// By analogy with existing lua_pushmumble() functions, push onto state L's
// stack a Lua object corresponding to the passed LLSD object.
int lua_pushllsd(lua_State* L, const LLSD& data)
{
    switch (data.type())
    {
    case LLSD::TypeUndefined:
        lua_pushnil(L);
        break;

    case LLSD::TypeBoolean:
        lua_pushboolean(L, data.asBoolean());
        break;

    case LLSD::TypeInteger:
        lua_pushinteger(L, data.asInteger());
        break;

    case LLSD::TypeReal:
        lua_pushnumber(L, data.asReal());
        break;

    case LLSD::TypeBinary:
    {
        auto binary{ data.asBinary() };
        std::memcpy(lua_newuserdata(L, binary.size()), binary.data(), binary.size());
        break;
    }

    case LLSD::TypeMap:
    {
        // push a new table with space for our non-array keys
        lua_createtable(L, 0, data.size());
        for (const auto& pair: llsd::inMap(data))
        {
            // push value -- so now table is at -2, value at -1
            lua_pushllsd(L, pair.second);
            // pop value, assign to table[key]
            lua_setfield(L, -2, pair.first.c_str());
        }
        break;
    }

    case LLSD::TypeArray:
    {
        // push a new table with space for array entries
        lua_createtable(L, data.size(), 0);
        lua_Integer index{ 0 };
        for (const auto& item: llsd::inArray(data))
        {
            // push new index value: table at -2, index at -1
            lua_pushinteger(L, ++index);
            // push new array value: table at -3, index at -2, value at -1
            lua_pushllsd(L, item);
            // pop key and value, assign table[key] = value
            lua_settable(L, -3);
        }
        break;
    }

    case LLSD::TypeString:
    case LLSD::TypeUUID:
    case LLSD::TypeDate:
    case LLSD::TypeURI:
    default:
    {
        auto strdata{ data.asString() };
        lua_pushlstring(L, strdata.c_str(), strdata.length());
        break;
    }
    }
}
