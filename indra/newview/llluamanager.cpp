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
#include "llerror.h"
#include "lleventcoro.h"
#include "llevents.h"
#include "llfloaterreg.h"
#include "llfloaterimnearbychat.h"
#include "llfloatersidepanelcontainer.h"
#include "llinstancetracker.h"
#include "llleaplistener.h"
#include "llnotificationsutil.h"
#include "lluuid.h"
#include "llvoavatarself.h"
#include "llviewermenu.h"
#include "llviewermenufile.h"
#include "llviewerwindow.h"
#include "lluilistener.h"
#include "llanimationstates.h"
#include "llinventoryfunctions.h"
#include "stringize.h"
#include "lltoolplacer.h"

#include <boost/algorithm/string/replace.hpp>

extern "C"
{
#include "lua/lua.h"
#include "lua/lauxlib.h"
#include "lua/lualib.h"
}

#include <algorithm>
#include <cstdlib>                  // std::rand()
#include <cstring>                  // std::memcpy()
#include <map>
#include <memory>                   // std::unique_ptr
#include <string_view>
#include <vector>

#if LL_WINDOWS
#pragma comment(lib, "liblua54.a")
#endif

std::string lua_tostdstring(lua_State* L, int index);
void lua_pushstdstring(lua_State* L, const std::string& str);
LLSD lua_tollsd(lua_State* L, int index);
void lua_pushllsd(lua_State* L, const LLSD& data);

// FIXME extremely hacky way to get to the UI Listener framework. There's almost certainly a cleaner way.
extern LLUIListener sUIListener;

/**
 * LuaListener is based on LLLeap. It serves an analogous function.
 *
 * Each LuaListener instance has an int key, generated randomly to
 * inconvenience malicious Lua scripts wanting to mess with others. The idea
 * is that a given lua_State stores in its Registry:
 * - "event.listener": the int key of the corresponding LuaListener, if any
 * - "event.function": the Lua function to be called with incoming events
 * The original thought was that LuaListener would itself store the Lua
 * function -- but surprisingly, there is no C/C++ type in the API that stores
 * a Lua function.
 *
 * (We considered storing in "event.listener" the LuaListener pointer itself
 * as a light userdata, but the problem would be if Lua code overwrote that.
 * We want to prevent any Lua script from crashing the viewer, intentionally
 * or otherwise. Safer to use a key lookup.)
 *
 * Like LLLeap, each LuaListener instance also has an associated
 * LLLeapListener to respond to LLEventPump management commands.
 */
class LuaListener: public LLInstanceTracker<LuaListener, int>
{
    using super = LLInstanceTracker<LuaListener, int>;
public:
    LuaListener(lua_State* L):
        super(getUniqueKey()),
        mState(L),
        mReplyPump(LLUUID::generateNewID().asString()),
        mListener(new LLLeapListener(std::bind(&LuaListener::connect, this,
                                               std::placeholders::_1, std::placeholders::_2)))
    {
        mReplyConnection = connect(mReplyPump, "LuaListener");
    }

    std::string getReplyName() const { return mReplyPump.getName(); }
    std::string getCommandName() const { return mListener->getPumpName(); }

private:
    static int getUniqueKey()
    {
        // Find a random key that does NOT already correspond to a LuaListener
        // instance. Passing a duplicate key to LLInstanceTracker would do Bad
        // Things.
        int key;
        do
        {
            key = std::rand();
        } while (LuaListener::getInstance(key));
        // This is theoretically racy, if we were instantiating new
        // LuaListeners on multiple threads. Don't.
        return key;
    }

    LLBoundListener connect(LLEventPump& pump, const std::string& listener)
    {
        return pump.listen(listener,
                           std::bind(&LuaListener::call_lua, mState, pump.getName(),
                                     std::placeholders::_1));
    }

    static bool call_lua(lua_State* L, const std::string& pump, const LLSD& data)
    {
        if (! lua_checkstack(L, 3))
        {
            LL_WARNS("Lua") << "Cannot extend Lua stack to call listen_events() callback"
                            << LL_ENDL;
            return false;
        }
        // push the registered Lua callback function stored in our registry as
        // "event.function"
        lua_getfield(L, LUA_REGISTRYINDEX, "event.function");
        llassert(lua_isfunction(L, -1));
        // pass pump name
        lua_pushstdstring(L, pump);
        // then the data blob
        lua_pushllsd(L, data);
        // call the registered Lua listener function; allow it to return bool;
        // no message handler
        auto status = lua_pcall(L, 2, 1, 0);
        bool result{ false };
        if (status != LUA_OK)
        {
            LL_WARNS("Lua") << "Error in listen_events() callback: "
                            << lua_tostdstring(L, -1) << LL_ENDL;
        }
        else
        {
            result = lua_toboolean(L, -1);
        }
        // discard either the error message or the bool return value
        lua_pop(L, 1);
        return result;
    }

    lua_State* mState;
    LLEventStream mReplyPump;
    LLTempBoundListener mReplyConnection;
    std::unique_ptr<LLLeapListener> mListener;
};

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

    static lua_CFunction get(const std::string& key)
    {
        // use find() instead of subscripting to avoid creating an entry for
        // unknown key
        const auto& registry{ getRegistry() };
        auto found{ registry.find(key) };
        return (found == registry.end())? nullptr : found->second;
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
// {
//     ... supply method body here, referencing 'L' ...
// }

lua_function(print_debug)
{
    luaL_where(L, 1);
    LL_DEBUGS("Lua") << lua_tostring(L, 2) << ": " << lua_tostring(L, 1) << LL_ENDL;
    lua_pop(L, 2);
    return 0;
}

// also used for print(); see LuaState constructor
lua_function(print_info)
{
    luaL_where(L, 1);
    LL_INFOS("Lua") << lua_tostring(L, 2) << ": " << lua_tostring(L, 1) << LL_ENDL;
    lua_pop(L, 2);
    return 0;
}

lua_function(print_warning)
{
    luaL_where(L, 1);
    LL_WARNS("Lua") << lua_tostring(L, 2) << ": " << lua_tostring(L, 1) << LL_ENDL;
    lua_pop(L, 2);
    return 0;
}

lua_function(avatar_sit)
{
    gAgent.sitDown();
    return 0;
}

lua_function(avatar_stand)
{
    gAgent.standUp();
    return 0;
}

lua_function(nearby_chat_send)
{
    std::string msg(lua_tostring(L, 1));
    LLFloaterIMNearbyChat *nearby_chat = LLFloaterReg::findTypedInstance<LLFloaterIMNearbyChat>("nearby_chat");
    nearby_chat->sendChatFromViewer(msg, CHAT_TYPE_NORMAL, gSavedSettings.getBOOL("PlayChatAnim"));

    lua_pop(L, 1);
    return 0;
}

lua_function(wear_by_name)
{
    std::string folder_name(lua_tostring(L, 1));
    LLAppearanceMgr::instance().wearOutfitByName(folder_name);

    lua_pop(L, 1);
    return 0;
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

    lua_pop(L, 1);
    return 0;
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

    lua_pop(L, 1);
    return 0;
}

lua_function(close_all_floaters)
{
    close_all_windows();
    return 0;
}

lua_function(click_child)
{
	std::string parent_name(lua_tostring(L, 1));
	std::string child_name(lua_tostring(L, 2));

	LLFloater *floater = LLFloaterReg::findInstance(parent_name);
	LLUICtrl *child = floater->getChild<LLUICtrl>(child_name, true);
	child->onCommit();

	lua_pop(L, 2);
	return 0;
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

    lua_pop(L, 1);
    return 0;
}

lua_function(open_wearing_tab)
{ 
    LLFloaterSidePanelContainer::showPanel("appearance", LLSD().with("type", "now_wearing")); 
    return 0;
}

lua_function(set_debug_setting_bool) 
{
    std::string setting_name(lua_tostring(L, 1));
    bool value(lua_toboolean(L, 2));

    gSavedSettings.setBOOL(setting_name, value);
    lua_pop(L, 2);
    return 0;
}

lua_function(get_avatar_name)
{
    std::string name = gAgentAvatarp->getFullname();
    luaL_checkstack(L, 1, nullptr);
    lua_pushstring(L, name.c_str());
    return 1;
}

lua_function(is_avatar_flying)
{
    luaL_checkstack(L, 1, nullptr);
    lua_pushboolean(L, gAgent.getFlying());
    return 1;
}

lua_function(play_animation)
{
	// on exit, pop all passed arguments, so always return 0
	LuaPopper popper(L, lua_gettop(L));

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
			return 0;
		}
	}
	LL_WARNS() << "No animation found for name " << anim_name << LL_ENDL;

	return 0;
}

lua_function(env_setting_event)
{ 
    handle_env_setting_event(lua_tostring(L, 1));
    lua_pop(L, 1);
    return 0;
}

void handle_notification_dialog(const LLSD &notification, const LLSD &response, lua_State *L, std::string response_cb)
{
    if (!response_cb.empty())
    {
        S32 option = LLNotificationsUtil::getSelectedOption(notification, response);

        luaL_checkstack(L, 1, nullptr);
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

    lua_pop(L, lua_gettop(L));
    return 0;
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

    lua_pop(L, lua_gettop(L));
    return 0;
}

lua_function(add_menu_separator)
{
    std::string menu(lua_tostring(L, 1));
    gMenuBarView->findChildMenuByName(menu, true)->addSeparator();

    lua_pop(L, 1);
    return 0;
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

    lua_pop(L, lua_gettop(L));
    return 0; 
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

    lua_pop(L, lua_gettop(L));
    return 0;
}

// rez_prim({x, y}, prim_type)
// avatar is the reference point
lua_function(rez_prim)
{
    lua_rawgeti(L, 1, 1);
    F32 x = lua_tonumber(L, -1);
    lua_pop(L, 1);
    lua_rawgeti(L, 1, 2);
    F32 y = lua_tonumber(L, -1);
    lua_pop(L, 1);

    S32 type(lua_tonumber(L, 2));  // primitive shapes 1-8

    LLVector3 obj_pos = gAgent.getPositionAgent() + LLVector3(x, y, -0.5);
    bool res = LLToolPlacer::rezNewObject(type, NULL, 0, TRUE, gAgent.getPositionAgent(), obj_pos, gAgent.getRegion(), 0);

    LL_INFOS() << "Rezing a prim: type " << LLPrimitive::pCodeToString(type) << ", coordinates: " << obj_pos << " Success: " << res << LL_ENDL;

    lua_pop(L, lua_gettop(L));
    return 0;
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

	lua_pop(L, top);
	return 0;
}

lua_function(post_on)
{
    std::string pumpname{ lua_tostring(L, 1) };
    LLSD data{ lua_tollsd(L, 2) };
    lua_pop(L, 2);
    LLEventPumps::instance().obtain(pumpname).post(data);
    return 0;
}

lua_function(listen_events)
{
    if (! lua_isfunction(L, 1))
    {
        return luaL_typeerror(L, 1, "function");
    }
    luaL_checkstack(L, 2, nullptr);

    // Get the lua_State* for the main thread of this state, in case we were
    // called from a coroutine thread. We're going to make callbacks into Lua
    // code, and we want to do it on the main thread rather than a (possibly
    // suspended) coroutine thread.
    // Registry table is at pseudo-index LUA_REGISTRYINDEX
    // Main thread is at registry key LUA_RIDX_MAINTHREAD
    auto regtype{ lua_geti(L, LUA_REGISTRYINDEX, LUA_RIDX_MAINTHREAD) };
    // Not finding the main thread at the documented place isn't a user error,
    // it's a Problem
    llassert_always(regtype == LUA_TTHREAD);
    lua_State* mainthread{ lua_tothread(L, -1) };
    // pop the main thread
    lua_pop(L, 1);

    luaL_checkstack(mainthread, 1, nullptr);
    LuaListener::ptr_t listener;
    // Does the main thread already have a LuaListener stored in the registry?
    // That is, has this Lua chunk already called listen_events()?
    auto keytype{ lua_getfield(mainthread, LUA_REGISTRYINDEX, "event.listener") };
    llassert(keytype == LUA_TNIL || keytype == LUA_TNUMBER);
    if (keytype == LUA_TNUMBER)
    {
        // We do already have a LuaListener. Retrieve it.
        int isint;
        listener = LuaListener::getInstance(lua_tointegerx(mainthread, -1, &isint));
        // pop the int "event.listener" key
        lua_pop(mainthread, 1);
        // Nobody should have destroyed this LuaListener instance!
        llassert(isint && listener);
    }
    else
    {
        // pop the nil "event.listener" key
        lua_pop(mainthread, 1);
        // instantiate a new LuaListener, binding the mainthread state
        listener.reset(new LuaListener(mainthread));
        // set its key in the field where we'll look for it later
        lua_pushinteger(mainthread, listener->getKey());
        lua_setfield(mainthread, LUA_REGISTRYINDEX, "event.listener");
    }

    // Now that we've found or created our LuaListener, store the passed Lua
    // function as the callback. Beware: our caller passed the function on L's
    // stack, but we want to store it on the mainthread registry.
    if (L != mainthread)
    {
        // push 1 value (the Lua function) from L's stack to mainthread's
        lua_xmove(L, mainthread, 1);
    }
    lua_setfield(mainthread, LUA_REGISTRYINDEX, "event.function");

    // return the reply pump name and the command pump name on caller's lua_State
    lua_pushstdstring(L, listener->getReplyName());
    lua_pushstdstring(L, listener->getCommandName());
    return 2;
}

lua_function(await_event)
{
    // await_event(pumpname [, timeout [, value to return if timeout (default nil)]])
    auto pumpname{ lua_tostdstring(L, 1) };
    LLSD result;
    if (lua_gettop(L) > 1)
    {
        auto timeout{ lua_tonumber(L, 2) };
        // with no 3rd argument, should be LLSD()
        auto dftval{ lua_tollsd(L, 3) };
        lua_pop(L, lua_gettop(L));
        result = llcoro::suspendUntilEventOnWithTimeout(pumpname, timeout, dftval);
    }
    else
    {
        // no timeout
        lua_pop(L, 1);
        result = llcoro::suspendUntilEventOn(pumpname);
    }
    lua_pushllsd(L, result);
    return 1;
}

/**
 * RAII class to manage the lifespan of a lua_State
 */
class LuaState
{
public:
    LuaState(const std::string_view& desc, LLLUAmanager::script_finished_fn cb):
        mDesc(desc),
        mCallback(cb),
        mState(luaL_newstate())
    {
        luaL_openlibs(mState);
        LuaFunction::init(mState);
        // Try to make print() write to our log.
        lua_register(mState, "print", LuaFunction::get("print_info"));
    }

    LuaState(const LuaState&) = delete;
    LuaState& operator=(const LuaState&) = delete;

    ~LuaState()
    {
        // Did somebody call listen_events() on this LuaState?
        // That is, is there a LuaListener key in its registry?
        auto keytype{ lua_getfield(mState, LUA_REGISTRYINDEX, "event.listener") };
        if (keytype == LUA_TNUMBER)
        {
            // We do have a LuaListener. Retrieve it.
            int isint;
            auto listener{ LuaListener::getInstance(lua_tointegerx(mState, -1, &isint)) };
            // pop the int "event.listener" key
            lua_pop(mState, 1);
            // if we got a LuaListener instance, destroy it
            // (if (! isint), lua_tointegerx() returned 0, but key 0 might
            // validly designate someone ELSE's LuaListener)
            if (isint && listener)
            {
                auto lptr{ listener.get() };
                listener.reset();
                delete lptr;
            }
        }

        lua_close(mState);

        if (mCallback)
        {
            // mError potentially set by previous checkLua() call(s)
            mCallback(mError);
        }    
    }

    bool checkLua(int r)
    {
        if (r != LUA_OK)
        {
            mError = lua_tostring(mState, -1);
            lua_pop(mState, 1);

            LL_WARNS() << mDesc << ": " << mError << LL_ENDL;
            return false;
        }
        return true;
    }

    operator lua_State*() const { return mState; }

private:
    std::string mDesc;
    LLLUAmanager::script_finished_fn mCallback;
    lua_State* mState;
    std::string mError;
};

void LLLUAmanager::runScriptFile(const std::string& filename, script_finished_fn cb)
{
    std::string desc{ stringize("runScriptFile('", filename, "')") };
    LLCoros::instance().launch(desc, [desc, filename, cb]()
    {
        LuaState L(desc, cb);

        auto LUA_sleep_func = [](lua_State *L)
        {
            F32 seconds = lua_tonumber(L, -1);
            lua_pop(L, 1);
            llcoro::suspendUntilTimeout(seconds);
            return 0;
        };

        lua_register(L, "sleep", LUA_sleep_func);

        if (L.checkLua(luaL_dofile(L, filename.c_str())))
        {
            lua_getglobal(L, "call_once_func");
            if (lua_isfunction(L, -1))
            {
                // call call_once_func(), setting internal error message if
                // error
                L.checkLua(lua_pcall(L, 0, 0, 0));
            }
        }
    });
}

void LLLUAmanager::runScriptLine(const std::string& cmd, script_finished_fn cb)
{
    // find a suitable abbreviation for the cmd string
    std::string_view shortcmd{ cmd };
    const size_t shortlen = 40;
    std::string::size_type eol = shortcmd.find_first_of("\r\n");
    if (eol != std::string::npos)
        shortcmd = shortcmd.substr(0, eol);
    if (shortcmd.length() > shortlen)
        shortcmd = stringize(shortcmd.substr(0, shortlen), "...");

    std::string desc{ stringize("runScriptLine('", shortcmd, "')") };
    LLCoros::instance().launch(desc, [desc, cmd, cb]()
    {
        LuaState L(desc, cb);
        L.checkLua(luaL_dostring(L, cmd.c_str()));
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

void lua_pushstdstring(lua_State* L, const std::string& str)
{
    luaL_checkstack(L, 1, nullptr);
    lua_pushlstring(L, str.c_str(), str.length());
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

        // This is the most important of the luaL_checkstack() calls because a
        // deeply nested Lua structure will enter this case at each level, and
        // we'll need another 2 stack slots to traverse each nested table.
        luaL_checkstack(L, 2, nullptr);
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
    // might need 2 slots for array or map
    luaL_checkstack(L, 2, nullptr);
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
        lua_Integer key{ 0 };
        for (const auto& item: llsd::inArray(data))
        {
            // push new array value: table at -2, value at -1
            lua_pushllsd(L, item);
            // pop value, assign table[key] = value
            lua_seti(L, -2, ++key);
        }
        break;
    }

    case LLSD::TypeString:
    case LLSD::TypeUUID:
    case LLSD::TypeDate:
    case LLSD::TypeURI:
    default:
    {
        lua_pushstdstring(L, data.asString());
        break;
    }
    }
}
