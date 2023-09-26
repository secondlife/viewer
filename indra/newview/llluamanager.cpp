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
#include "llevents.h"
#include "llfloaterreg.h"
#include "llfloaterimnearbychat.h"
#include "llfloatersidepanelcontainer.h"
#include "llnotificationsutil.h"
#include "llvoavatarself.h"
#include "llviewermenu.h"
#include "llviewermenufile.h"
#include "llviewerwindow.h"
#include "lluilistener.h"
#include "llanimationstates.h"
#include "llinventoryfunctions.h"

#include <boost/algorithm/string/replace.hpp>

extern "C"
{
#include "lua/lua.h"
#include "lua/lauxlib.h"
#include "lua/lualib.h"
}

#include <algorithm>
#include <cstring>                  // std::memcpy()
#include <map>
#include <string_view>
#include <vector>

#if LL_WINDOWS
#pragma comment(lib, "liblua54.a")
#endif

LLSD lua_tollsd(lua_State* L, int index);
void lua_pushllsd(lua_State* L, const LLSD& data);

// FIXME extremely hacky way to get to the UI Listener framework. There's almost certainly a cleaner way.
extern LLUIListener sUIListener;

/**
 * LuaPopper is an RAII struct whose role is to pop some number of entries
 * from the Lua stack if the calling function exits early.
 */
struct LuaPopper
{
    LuaPopper(lua_State* L, int count):
        mState(L),
        mCount(count)
    {}

    LuaPopper(const LuaPopper&) = delete;
    LuaPopper& operator=(const LuaPopper&) = delete;

    ~LuaPopper()
    {
        if (mCount)
        {
            lua_pop(mState, mCount);
        }
    }

    void disarm() { set(0); }
    void set(int count) { mCount = count; }

    lua_State* mState;
    int mCount;
};

/**
 * LuaFunction is a base class containing a static registry of its static
 * subclass call() methods. call() is NOT virtual: instead, each subclass
 * constructor passes a pointer to its distinct call() method to the base-
 * class constructor, along with a name by which to register that method.
 *
 * The init() method walks the registry and registers each such name with the
 * passed lua_State.
 */
class LuaFunction
{
public:
    LuaFunction(const std::string_view& name, lua_CFunction function)
    {
        getRegistry().emplace(name, function);
    }

    static void init(lua_State* L)
    {
        for (const auto& pair: getRegistry())
        {
            lua_register(L, pair.first.c_str(), pair.second);
        }
    }

private:
    using Registry = std::map<std::string, lua_CFunction>;
    static Registry& getRegistry()
    {
        // use a function-local static to ensure it's initialized
        static Registry registry;
        return registry;
    }
};

/**
 * lua_function(name) is a macro to facilitate defining C++ functions
 * available to Lua. It defines a subclass of LuaFunction and declares a
 * static instance of that subclass, thereby forcing the compiler to call its
 * constructor at module initialization time. The constructor passes the
 * stringized instance name to its LuaFunction base-class constructor, along
 * with a pointer to the static subclass call() method. It then emits the
 * call() method definition header, to be followed by a method body enclosed
 * in curly braces as usual.
 */
#define lua_function(name)                      \
static struct name##_ : public LuaFunction      \
{                                               \
    name##_(): LuaFunction(#name, &call) {}     \
    static int call(lua_State* L);              \
} name;                                         \
int name##_::call(lua_State* L)

lua_function(print_warning)
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

lua_function(avatar_sit)
{
    gAgent.sitDown();
    return 1;
}

lua_function(avatar_stand)
{
    gAgent.standUp();
    return 1;
}

lua_function(nearby_chat_send)
{
    std::string msg(lua_tostring(L, 1));
    LLFloaterIMNearbyChat *nearby_chat = LLFloaterReg::findTypedInstance<LLFloaterIMNearbyChat>("nearby_chat");
    nearby_chat->sendChatFromViewer(msg, CHAT_TYPE_NORMAL, gSavedSettings.getBOOL("PlayChatAnim"));

    return 1;
}

lua_function(wear_by_name)
{
    std::string folder_name(lua_tostring(L, 1));
    LLAppearanceMgr::instance().wearOutfitByName(folder_name);

    return 1;
}

lua_function(open_floater)
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

lua_function(close_floater)
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

lua_function(close_all_floaters)
{
    close_all_windows();
    return 1;
}

lua_function(click_child)
{
	std::string parent_name(lua_tostring(L, 1));
	std::string child_name(lua_tostring(L, 2));

	LLFloater *floater = LLFloaterReg::findInstance(parent_name);
	LLUICtrl *child = floater->getChild<LLUICtrl>(child_name, true);
	child->onCommit();

	return 1;
}

lua_function(snapshot_to_file)
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

lua_function(open_wearing_tab)
{ 
    LLFloaterSidePanelContainer::showPanel("appearance", LLSD().with("type", "now_wearing")); 
    return 1;
}

lua_function(set_debug_setting_bool) 
{
    std::string setting_name(lua_tostring(L, 1));
    bool value(lua_toboolean(L, 2));

    gSavedSettings.setBOOL(setting_name, value);
    return 1;
}

lua_function(get_avatar_name)
{
    std::string name = gAgentAvatarp->getFullname();
    lua_pushstring(L, name.c_str());
    return 1;
}

lua_function(is_avatar_flying)
{
    lua_pushboolean(L, gAgent.getFlying());
    return 1;
}

lua_function(play_animation)
{
	std::string anim_name = lua_tostring(L,1);

	EAnimRequest req = ANIM_REQUEST_START;
	if (lua_gettop(L) > 1)
	{
		req = (EAnimRequest) (int) lua_tonumber(L, 2);
	}

	LLInventoryModel::cat_array_t cat_array;
	LLInventoryModel::item_array_t item_array;
	LLNameItemCollector has_name(anim_name);
	gInventory.collectDescendentsIf(gInventory.getRootFolderID(),
									cat_array,
									item_array,
									LLInventoryModel::EXCLUDE_TRASH,
									has_name);
	for (auto& item: item_array)
	{
		if (item->getType() == LLAssetType::AT_ANIMATION)
		{
			LLUUID anim_id = item->getAssetUUID();
			LL_INFOS() << "Playing animation " << anim_id << LL_ENDL;
			gAgent.sendAnimationRequest(anim_id, req);
			return 1;
		}
	}
	LL_WARNS() << "No animation found for name " << anim_name << LL_ENDL;

	return 1;
}

lua_function(env_setting_event)
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

lua_function(show_notification)
{
    std::string notification(lua_tostring(L, 1));

    if (lua_type(L, 2) == LUA_TTABLE)
    {
        LLSD args = lua_tollsd(L, 2);

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

lua_function(add_menu_item)
{
    std::string menu(lua_tostring(L, 1));
    if (lua_type(L, 2) == LUA_TTABLE)
    {
        LLSD args = lua_tollsd(L, 2);

        LLMenuItemCallGL::Params item_params;
        item_params.name  = args["name"];
        item_params.label = args["label"];

        LLUICtrl::CommitCallbackParam item_func;
        item_func.function_name = args["function"];
        if (args.has("parameter"))
        {
            item_func.parameter = args["parameter"];
        }
        item_params.on_click = item_func;

        LLMenuItemCallGL *menu_item = LLUICtrlFactory::create<LLMenuItemCallGL>(item_params);
        gMenuBarView->findChildMenuByName(menu, true)->append(menu_item);
    }

    return 1;
}

lua_function(add_menu_separator)
{
    std::string menu(lua_tostring(L, 1));
    gMenuBarView->findChildMenuByName(menu, true)->addSeparator();

    return 1;
}

lua_function(add_menu)
{ 
    if (lua_type(L, 1) == LUA_TTABLE)
    {
        LLSD args = lua_tollsd(L, 1);

        LLMenuGL::Params item_params;
        item_params.name  = args["name"];
        item_params.label = args["label"];
        item_params.can_tear_off = args["tear_off"];

        LLMenuGL *menu = LLUICtrlFactory::create<LLMenuGL>(item_params);
        gMenuBarView->appendMenu(menu);
    }

    return 1; 
}

lua_function(add_branch)
{
    std::string menu(lua_tostring(L, 1));
    if (lua_type(L, 2) == LUA_TTABLE)
    {
        LLSD args = lua_tollsd(L, 2);

        LLMenuGL::Params item_params;
        item_params.name  = args["name"];
        item_params.label = args["label"];
        item_params.can_tear_off = args["tear_off"];

        LLMenuGL *branch = LLUICtrlFactory::create<LLMenuGL>(item_params);
        gMenuBarView->findChildMenuByName(menu, true)->appendMenu(branch);
    }

    return 1;
}

lua_function(run_ui_command)
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

lua_function(post_on_pump)
{
    std::string pumpname{ lua_tostring(L, -2) };
    LLSD data{ lua_tollsd(L, -1) };
    lua_pop(L, 2);
    LLEventPumps::instance().obtain(pumpname).post(data);
    return 1;
}

lua_function(listen_events)
{
    if (! lua_isfunction(L, -1))
    {
        return luaL_typerror(L, 1, "function");
    }
    // return the distinct LLEventPump name so Lua code can post that with a
    // request as the reply pump
    return 1;
}

void initLUA(lua_State *L)
{
    LuaFunction::init(L);
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
//
// For proper error handling, we REQUIRE that the Lua runtime be compiled as
// C++ so errors are raised as C++ exceptions rather than as longjmp() calls:
// http://www.lua.org/manual/5.4/manual.html#4.4
// "Internally, Lua uses the C longjmp facility to handle errors. (Lua will
// use exceptions if you compile it as C++; search for LUAI_THROW in the
// source code for details.)"
// Some blocks within this function construct temporary C++ objects in the
// expectation that these objects will be properly destroyed even if code
// reached by that block raises a Lua error.
LLSD lua_tollsd(lua_State* L, int index)
{
    switch (lua_type(L, index))
    {
    case LUA_TNONE:
        // Should LUA_TNONE be an error instead of returning isUndefined()?
    case LUA_TNIL:
        return {};

    case LUA_TBOOLEAN:
        return bool(lua_toboolean(L, index));

    case LUA_TNUMBER:
    {
        // check if integer truncation leaves the number intact
        int isint;
        lua_Integer intval{ lua_tointegerx(L, index, &isint) };
        if (isint)
        {
            return LLSD::Integer(intval);
        }
        else
        {
            return lua_tonumber(L, index);
        }
    }

    case LUA_TSTRING:
        return lua_tostdstring(L, index);

    case LUA_TUSERDATA:
    {
        LLSD::Binary binary(lua_rawlen(L, index));
        std::memcpy(binary.data(), lua_touserdata(L, index), binary.size());
        return binary;
    }

    case LUA_TTABLE:
    {
        // A Lua table correctly constructed to convert to LLSD will have
        // either consecutive integer keys starting at 1, which we represent
        // as an LLSD array (with Lua key 1 at C++ index 0), or will have
        // all string keys.
        //
        // In the belief that Lua table traversal skips "holes," that is, it
        // doesn't report any key/value pair whose value is nil, we allow a
        // table with integer keys >= 1 but with "holes." This produces an
        // LLSD array with isUndefined() entries at unspecified keys. There
        // would be no other way for a Lua caller to construct an
        // isUndefined() LLSD array entry. However, to guard against crazy int
        // keys, we forbid gaps larger than a certain size: crazy int keys
        // could result in a crazy large contiguous LLSD array.
        //
        // Possible looseness could include:
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
        // key is at stack index -2, value at index -1
        // from here until lua_next() returns 0, have to lua_pop(2) if we
        // return early
        LuaPopper popper(L, 2);
        // Remember the type of the first key
        auto firstkeytype{ lua_type(L, -2) };
        switch (firstkeytype)
        {
        case LUA_TNUMBER:
        {
            // First Lua key is a number: try to convert table to LLSD array.
            // This is tricky because we don't know in advance the size of the
            // array. The Lua reference manual says that lua_rawlen() is the
            // same as the length operator '#'; but the length operator states
            // that it might stop at any "hole" in the subject table.
            // Moreover, the Lua next() function (and presumably lua_next())
            // traverses a table in unspecified order, even for numeric keys
            // (emphasized in the doc).
            // Make a preliminary pass over the whole table to validate and to
            // collect keys.
            std::vector<LLSD::Integer> keys;
            // Try to determine the length of the table. If the length
            // operator is truthful, avoid allocations while we grow the keys
            // vector. Even if it's not, we can still grow the vector, albeit
            // a little less efficiently.
            keys.reserve(luaL_len(L, index));
            do
            {
                auto arraykeytype{ lua_type(L, -2) };
                switch (arraykeytype)
                {
                case LUA_TNUMBER:
                {
                    int isint;
                    lua_Integer intkey{ lua_tointegerx(L, -2, &isint) };
                    if (! isint)
                    {
                        // key isn't an integer - this doesn't fit our LLSD
                        // array constraints
                        return luaL_error(L, "Expected integer array key, got %f instead",
                                          lua_tonumber(L, -2));
                    }
                    if (intkey < 1)
                    {
                        return luaL_error(L, "array key %d out of bounds", int(intkey));
                    }

                    keys.push_back(LLSD::Integer(intkey));
                    break;
                }

                case LUA_TSTRING:
                    // break out strings specially to report the value
                    return luaL_error(L, "Cannot convert string array key '%s' to LLSD",
                                      lua_tostring(L, -2));

                default:
                    return luaL_error(L, "Cannot convert %s array key to LLSD",
                                      lua_typename(L, arraykeytype));
                }

                // remove value, keep key for next iteration
                lua_pop(L, 1);
            } while (lua_next(L, index) != 0);
            popper.disarm();
            // Table keys are all integers: are they reasonable integers?
            // Arbitrary max: may bite us, but more likely to protect us
            size_t array_max{ 10000 };
            if (keys.size() > array_max)
            {
                return luaL_error(L, "Conversion from Lua to LLSD array limited to %d entries",
                                  int(array_max));
            }
            // We know the smallest key is >= 1. Check the largest. We also
            // know the vector is NOT empty, else we wouldn't have gotten here.
            std::sort(keys.begin(), keys.end());
            LLSD::Integer highkey = *keys.rbegin();
            if ((highkey - LLSD::Integer(keys.size())) > 100)
            {
                // Looks like we've gone beyond intentional array gaps into
                // crazy key territory.
                return luaL_error(L, "Gaps in Lua table too large for conversion to LLSD array");
            }
            // right away expand the result array to the size we'll need
            LLSD result{ LLSD::emptyArray() };
            result[highkey - 1] = LLSD();
            // Traverse the table again, and this time populate result array.
            lua_pushnil(L);         // first key
            while (lua_next(L, index))
            {
                // key at stack index -2, value at index -1
                // We've already validated lua_tointegerx() for each key.
                // Don't forget to subtract 1 from Lua key for LLSD subscript!
                result[LLSD::Integer(lua_tointeger(L, -2)) - 1] = lua_tollsd(L, -1);
                // remove value, keep key for next iteration
                lua_pop(L, 1);
            }
            return result;
        }

        case LUA_TSTRING:
        {
            // First Lua key is a string: try to convert table to LLSD map
            LLSD result{ LLSD::emptyMap() };
            do
            {
                auto mapkeytype{ lua_type(L, -2) };
                if (mapkeytype != LUA_TSTRING)
                {
                    return luaL_error(L, "Cannot convert %s map key to LLSD",
                                      lua_typename(L, mapkeytype));
                }
                
                result[lua_tostdstring(L, -2)] = lua_tollsd(L, -1);
                // remove value, keep key for next iteration
                lua_pop(L, 1);
            } while (lua_next(L, index) != 0);
            popper.disarm();
            return result;
        }

        default:
            // First Lua key isn't number or string: sorry
            return luaL_error(L, "Cannot convert %s table key to LLSD",
                              lua_typename(L, firstkeytype));
        }
    }

    default:
        // Other Lua entities (e.g. function, C function, light userdata,
        // thread, userdata) are not convertible to LLSD, indicating a coding
        // error in the caller.
        return luaL_error(L, "Cannot convert type %s to LLSD", luaL_typename(L, index));
    }
}

// By analogy with existing lua_pushmumble() functions, push onto state L's
// stack a Lua object corresponding to the passed LLSD object.
void lua_pushllsd(lua_State* L, const LLSD& data)
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
        std::memcpy(lua_newuserdata(L, binary.size()),
                    binary.data(), binary.size());
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
