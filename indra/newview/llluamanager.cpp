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

#include "llcoros.h"
#include "llerror.h"
#include "lleventcoro.h"
#include "lua_function.h"
#include "lualistener.h"
#include "stringize.h"

// skip all these link dependencies for integration testing
#ifndef LL_TEST
#include "lluilistener.h"
#include "llviewercontrol.h"

// FIXME extremely hacky way to get to the UI Listener framework. There's
// a cleaner way.
extern LLUIListener sUIListener;
#endif // ! LL_TEST

#include <boost/algorithm/string/replace.hpp>
#include <filesystem>

#include "luau/luacode.h"
#include "luau/lua.h"
#include "luau/luaconf.h"
#include "luau/lualib.h"

#include <sstream>
#include <string_view>
#include <vector>

lua_function(sleep, "sleep(seconds): pause the running coroutine")
{
    F32 seconds = lua_tonumber(L, -1);
    lua_pop(L, 1);
    llcoro::suspendUntilTimeout(seconds);
    return 0;
};

// This function consumes ALL Lua stack arguments and returns concatenated
// message string
std::string lua_print_msg(lua_State* L, const std::string_view& level)
{
    // On top of existing Lua arguments, we're going to push tostring() and
    // duplicate each existing stack entry so we can stringize each one.
    luaL_checkstack(L, 2, nullptr);
    luaL_where(L, 1);
    // start with the 'where' info at the top of the stack
    std::ostringstream out;
    out << lua_tostring(L, -1);
    lua_pop(L, 1);
    const char* sep = "";           // 'where' info ends with ": "
    // now iterate over arbitrary args, calling Lua tostring() on each and
    // concatenating with separators
    for (int p = 1, top = lua_gettop(L); p <= top; ++p)
    {
        out << sep;
        sep = " ";
        // push Lua tostring() function -- note, semantically different from
        // lua_tostring()!
        lua_getglobal(L, "tostring");
        // Now the stack is arguments 1 .. N, plus tostring().
        // Push a copy of the argument at index p.
        lua_pushvalue(L, p);
        // pop tostring() and arg-p, pushing tostring(arg-p)
        // (ignore potential error code from lua_pcall() because, if there was
        // an error, we expect the stack top to be an error message -- which
        // we'll print)
        lua_pcall(L, 1, 1, 0);
        out << lua_tostring(L, -1);
        lua_pop(L, 1);
    }
    // pop everything
    lua_settop(L, 0);
    // capture message string
    std::string msg{ out.str() };
    // put message out there for any interested party (*koff* LLFloaterLUADebug *koff*)
    LLEventPumps::instance().obtain("lua output").post(stringize(level, ": ", msg));
    return msg;
}

lua_function(print_debug, "print_debug(args...): DEBUG level logging")
{
    LL_DEBUGS("Lua") << lua_print_msg(L, "DEBUG") << LL_ENDL;
    return 0;
}

// also used for print(); see LuaState constructor
lua_function(print_info, "print_info(args...): INFO level logging")
{
    LL_INFOS("Lua") << lua_print_msg(L, "INFO") << LL_ENDL;
    return 0;
}

lua_function(print_warning, "print_warning(args...): WARNING level logging")
{
    LL_WARNS("Lua") << lua_print_msg(L, "WARN") << LL_ENDL;
    return 0;
}

#ifndef LL_TEST

lua_function(run_ui_command,
             "run_ui_command(name [, parameter]): "
             "call specified UI command with specified parameter")
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

	lua_settop(L, 0);
	return 0;
}
#endif // ! LL_TEST

lua_function(post_on, "post_on(pumpname, data): post specified data to specified LLEventPump")
{
    std::string pumpname{ lua_tostdstring(L, 1) };
    LLSD data{ lua_tollsd(L, 2) };
    lua_pop(L, 2);
    LL_INFOS("Lua") << "post_on('" << pumpname << "', " << data << ")" << LL_ENDL;
    LLEventPumps::instance().obtain(pumpname).post(data);
    return 0;
}

lua_function(get_event_pumps,
             "get_event_pumps():\n"
             "Returns replypump, commandpump: names of LLEventPumps specific to this chunk.\n"
             "Events posted to replypump are queued for get_event_next().\n"
             "post_on(commandpump, ...) to engage LLEventAPI operations (see helpleap()).")
{
    luaL_checkstack(L, 2, nullptr);
    auto listener{ LuaState::obtainListener(L) };
    // return the reply pump name and the command pump name on caller's lua_State
    lua_pushstdstring(L, listener->getReplyName());
    lua_pushstdstring(L, listener->getCommandName());
    return 2;
}

lua_function(get_event_next,
             "get_event_next():\n"
             "Returns the next (pumpname, data) pair from the replypump whose name\n"
             "is returned by get_event_pumps(). Blocks the calling chunk until an\n"
             "event becomes available.")
{
    luaL_checkstack(L, 2, nullptr);
    auto listener{ LuaState::obtainListener(L) };
    const auto& [pump, data]{ listener->getNext() };
    lua_pushstdstring(L, pump);
    lua_pushllsd(L, data);
    return 2;
}

lua_function(await_event,
             "await_event(pumpname [, timeout [, value to return if timeout (default nil)]]):\n"
             "pause the running Lua chunk until the next event on the named LLEventPump")
{
    auto pumpname{ lua_tostdstring(L, 1) };
    LLSD result;
    if (lua_gettop(L) > 1)
    {
        auto timeout{ lua_tonumber(L, 2) };
        // with no 3rd argument, should be LLSD()
        auto dftval{ lua_tollsd(L, 3) };
        lua_settop(L, 0);
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

void LLLUAmanager::runScriptFile(const std::string& filename, script_finished_fn cb)
{
    // A script_finished_fn is used to initialize the LuaState.
    // It will be called when the LuaState is destroyed.
    LuaState L(cb);
    runScriptFile(L, filename);
}

void LLLUAmanager::runScriptFile(const std::string& filename, script_result_fn cb)
{
    LuaState L;
    // A script_result_fn will be called when LuaState::expr() completes.
    runScriptFile(L, filename, cb);
}

LLCoros::Future<std::pair<int, LLSD>>
LLLUAmanager::startScriptFile(LuaState& L, const std::string& filename)
{
    // Despite returning from startScriptFile(), we need this Promise to
    // remain alive until the callback has fired.
    auto promise{ std::make_shared<LLCoros::Promise<std::pair<int, LLSD>>>() };
    runScriptFile(L, filename,
                  [promise](int count, LLSD result)
                  { promise->set_value({ count, result }); });
    return LLCoros::getFuture(*promise);
}

std::pair<int, LLSD> LLLUAmanager::waitScriptFile(LuaState& L, const std::string& filename)
{
    return startScriptFile(L, filename).get();
}

void LLLUAmanager::runScriptFile(LuaState& L, const std::string& filename, script_result_fn cb)
{
    LLCoros::instance().launch(filename, [&L, filename, cb]()
    {
        llifstream in_file;
        in_file.open(filename.c_str());

        if (in_file.is_open()) 
        {
            std::string text{std::istreambuf_iterator<char>(in_file), {}};
            auto [count, result] = L.expr(filename, text);
            if (cb)
            {
                cb(count, result);
            }
        }
        else
        {
            auto msg{ stringize("unable to open script file '", filename, "'") };
            LL_WARNS("Lua") << msg << LL_ENDL;
            if (cb)
            {
                cb(-1, msg);
            }
        }
    });
}

void LLLUAmanager::runScriptLine(const std::string& chunk, script_finished_fn cb)
{
    // A script_finished_fn is used to initialize the LuaState.
    // It will be called when the LuaState is destroyed.
    LuaState L(cb);
    runScriptLine(L, chunk);
}

void LLLUAmanager::runScriptLine(const std::string& chunk, script_result_fn cb)
{
    LuaState L;
    // A script_result_fn will be called when LuaState::expr() completes.
    runScriptLine(L, chunk, cb);
}

LLCoros::Future<std::pair<int, LLSD>>
LLLUAmanager::startScriptLine(LuaState& L, const std::string& chunk)
{
    // Despite returning from startScriptLine(), we need this Promise to
    // remain alive until the callback has fired.
    auto promise{ std::make_shared<LLCoros::Promise<std::pair<int, LLSD>>>() };
    runScriptLine(L, chunk,
                  [promise](int count, LLSD result)
                  { promise->set_value({ count, result }); });
    return LLCoros::getFuture(*promise);
}

std::pair<int, LLSD> LLLUAmanager::waitScriptLine(LuaState& L, const std::string& chunk)
{
    return startScriptLine(L, chunk).get();
}

void LLLUAmanager::runScriptLine(LuaState& L, const std::string& chunk, script_result_fn cb)
{
    // find a suitable abbreviation for the chunk string
    std::string shortchunk{ chunk };
    const size_t shortlen = 40;
    std::string::size_type eol = shortchunk.find_first_of("\r\n");
    if (eol != std::string::npos)
        shortchunk = shortchunk.substr(0, eol);
    if (shortchunk.length() > shortlen)
        shortchunk = stringize(shortchunk.substr(0, shortlen), "...");

    std::string desc{ stringize("lua: ", shortchunk) };
    LLCoros::instance().launch(desc, [&L, desc, chunk, cb]()
    {
        auto [count, result] = L.expr(desc, chunk);
        if (cb)
        {
            cb(count, result);
        }
    });
}

void LLLUAmanager::runScriptOnLogin()
{
#ifndef LL_TEST
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
#endif // ! LL_TEST
}

std::string read_file(const std::string &name)
{
    llifstream in_file;
    in_file.open(name.c_str());

    if (in_file.is_open())
    {
        std::string text {std::istreambuf_iterator<char>(in_file), {}};
        return text;
    }

    return std::string();
}

LLRequireResolver::LLRequireResolver(lua_State *L, const std::string& path) : mPathToResolve(path), L(L)
{
    //Luau lua_Debug and lua_getinfo() are different compared to default Lua:
    //see https://github.com/luau-lang/luau/blob/80928acb92d1e4b6db16bada6d21b1fb6fa66265/VM/include/lua.h
    lua_Debug ar;
    lua_getinfo(L, 1, "s", &ar);
    mSourceChunkname = ar.source;

    std::filesystem::path fs_path(mPathToResolve);
    mPathToResolve = fs_path.lexically_normal().string();

    if (fs_path.is_absolute())
        luaL_argerrorL(L, 1, "cannot require a full path");
}

[[nodiscard]] LLRequireResolver::ResolvedRequire LLRequireResolver::resolveRequire(lua_State *L, std::string path)
{
    LLRequireResolver resolver(L, std::move(path));
    ModuleStatus status = resolver.findModule();
    if (status != ModuleStatus::FileRead)
        return ResolvedRequire {status};
    else
        return ResolvedRequire {status, resolver.mAbsolutePath, resolver.mSourceCode};
}

LLRequireResolver::ModuleStatus LLRequireResolver::findModule()
{
    resolveAndStoreDefaultPaths();

    // Put _MODULES table on stack for checking and saving to the cache
    luaL_findtable(L, LUA_REGISTRYINDEX, "_MODULES", 1);

    // Check if the module is already in _MODULES table, read from file otherwise
    LLRequireResolver::ModuleStatus moduleStatus = findModuleImpl();

    if (moduleStatus != LLRequireResolver::ModuleStatus::NotFound)
        return moduleStatus;

    if (std::filesystem::path(mPathToResolve).is_absolute())
        return moduleStatus;

    std::vector<std::string> lib_paths {gDirUtilp->getExpandedFilename(LL_PATH_SCRIPTS, "lua")};

    for (const auto& path : lib_paths)
    {
        std::string absolutePathOpt = (std::filesystem::path(path) / mPathToResolve).u8string();

        if (absolutePathOpt.empty())
            luaL_errorL(L, "error requiring module");

        mAbsolutePath = absolutePathOpt;

        moduleStatus = findModuleImpl();

        if (moduleStatus != LLRequireResolver::ModuleStatus::NotFound)
            return moduleStatus;
    }

    return LLRequireResolver::ModuleStatus::NotFound;
}

LLRequireResolver::ModuleStatus LLRequireResolver::findModuleImpl()
{
    std::string possibleSuffixedPaths[] = {mAbsolutePath + ".luau", mAbsolutePath + ".lua"};

     for (auto suffixedPath : possibleSuffixedPaths)
    {
         // Check _MODULES cache for module
        lua_getfield(L, -1, suffixedPath.c_str());
        if (!lua_isnil(L, -1))
        {
            return ModuleStatus::Cached;
        }
        lua_pop(L, 1);

        // Try to read the matching file
        std::string source = read_file(suffixedPath);
        if (!source.empty())
        {
            mAbsolutePath = suffixedPath;
            mSourceCode = source;
            return ModuleStatus::FileRead;
        }
    }

    return ModuleStatus::NotFound;
}

void LLRequireResolver::resolveAndStoreDefaultPaths()
{
    if (!std::filesystem::path(mPathToResolve).is_absolute())
    {
        mAbsolutePath = (std::filesystem::path((mSourceChunkname)).parent_path() / mPathToResolve).u8string();;
    }
    else
    {
        mAbsolutePath = mPathToResolve;
    }
}

static int finishrequire(lua_State *L)
{
    if (lua_isstring(L, -1))
        lua_error(L);

    return 1;
}

lua_function(require, "require(module_name) : module_name can be fullpath or just the name, in both cases without .lua")
{
    std::string name = lua_tostdstring(L, 1);

    LLRequireResolver::ResolvedRequire resolvedRequire = LLRequireResolver::resolveRequire(L, name);

    if (resolvedRequire.status == LLRequireResolver::ModuleStatus::Cached) 
    {
        // remove _MODULES from stack
        lua_remove(L, -2);
        return finishrequire(L);
    }
    else if (resolvedRequire.status == LLRequireResolver::ModuleStatus::NotFound)
        luaL_errorL(L, "error requiring module");

    // module needs to run in a new thread, isolated from the rest
    // note: we create ML on main thread so that it doesn't inherit environment of L
    lua_State *GL = lua_mainthread(L);
    lua_State *ML = lua_newthread(GL);
    lua_xmove(GL, L, 1);

    // new thread needs to have the globals sandboxed
    luaL_sandboxthread(ML);

    {
        if (lluau::loadstring(ML, resolvedRequire.absolutePath.c_str(), resolvedRequire.sourceCode.c_str()) == LUA_OK)
        {
            int status = lua_resume(ML, L, 0);

            if (status == LUA_OK)
            {
                if (lua_gettop(ML) == LUA_OK)
                    lua_pushstring(ML, "module must return a value");
                else if (!lua_istable(ML, -1) && !lua_isfunction(ML, -1))
                    lua_pushstring(ML, "module must return a table or function");
            }
            else if (status == LUA_YIELD)
            {
                lua_pushstring(ML, "module can not yield");
            }
            else if (!lua_isstring(ML, -1))
            {
                lua_pushstring(ML, "unknown error while running module");
            }
        }
    }
    // there's now a return value on top of ML; L stack: _MODULES ML
    lua_xmove(ML, L, 1);
    lua_pushvalue(L, -1);
    lua_setfield(L, -4, resolvedRequire.absolutePath.c_str());

    //remove _MODULES from stack
    lua_remove(L, -3);
    // remove ML from stack
    lua_remove(L, -2);
    
    // L stack: [module name] [result]
    return finishrequire(L);
}
