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

#include "fsyspath.h"
#include "llcoros.h"
#include "llerror.h"
#include "lleventcoro.h"
#include "llsdutil.h"
#include "llviewercontrol.h"
#include "lua_function.h"
#include "lualistener.h"
#include "stringize.h"

#include <boost/algorithm/string/replace.hpp>

#include "luau/luacode.h"
#include "luau/lua.h"
#include "luau/luaconf.h"
#include "luau/lualib.h"

#include <sstream>
#include <string_view>
#include <vector>

S32 LLLUAmanager::sAutorunScriptCount = 0;
S32 LLLUAmanager::sScriptCount = 0;
std::map<std::string, std::string> LLLUAmanager::sScriptNames;

lua_function(sleep, "sleep(seconds): pause the running coroutine")
{
    lua_checkdelta(L, -1);
    lua_Number seconds = lua_tonumber(L, -1);
    lua_pop(L, 1);
    llcoro::suspendUntilTimeout(narrow(seconds));
    LuaState::getParent(L).set_interrupts_counter(0);
    return 0;
};

// This function consumes ALL Lua stack arguments and returns concatenated
// message string
std::string lua_print_msg(lua_State* L, std::string_view level)
{
    // On top of existing Lua arguments, we're going to push tostring() and
    // duplicate each existing stack entry so we can stringize each one.
    lluau_checkstack(L, 2);
    luaL_where(L, 1);
    // start with the 'where' info at the top of the stack
    std::string source_info{ lua_tostring(L, -1) };
    lua_pop(L, 1);

    std::ostringstream out;
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
    LLEventPumps::instance().obtain("lua output").post(llsd::map("msg", msg, "level", stringize(level, ": "), "source_info", source_info));

    llcoro::suspend();
    return source_info + msg;
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

lua_function(post_on, "post_on(pumpname, data): post specified data to specified LLEventPump")
{
    lua_checkdelta(L, -2);
    std::string pumpname{ lua_tostdstring(L, 1) };
    LLSD data{ lua_tollsd(L, 2) };
    lua_pop(L, 2);
    LL_DEBUGS("Lua") << "post_on('" << pumpname << "', " << data << ")" << LL_ENDL;
    LLEventPumps::instance().obtain(pumpname).post(data);
    return 0;
}

lua_function(get_event_pumps,
             "get_event_pumps():\n"
             "Returns replypump, commandpump: names of LLEventPumps specific to this chunk.\n"
             "Events posted to replypump are queued for get_event_next().\n"
             "post_on(commandpump, ...) to engage LLEventAPI operations (see helpleap()).")
{
    lua_checkdelta(L, 2);
    lluau_checkstack(L, 2);
    auto& listener{ LuaState::obtainListener(L) };
    // return the reply pump name and the command pump name on caller's lua_State
    lua_pushstdstring(L, listener.getReplyName());
    lua_pushstdstring(L, listener.getCommandName());
    return 2;
}

lua_function(get_event_next,
             "get_event_next():\n"
             "Returns the next (pumpname, data) pair from the replypump whose name\n"
             "is returned by get_event_pumps(). Blocks the calling chunk until an\n"
             "event becomes available.")
{
    lua_checkdelta(L, 2);
    lluau_checkstack(L, 2);
    auto& listener{ LuaState::obtainListener(L) };
    const auto& [pump, data]{ listener.getNext() };
    lua_pushstdstring(L, pump);
    lua_pushllsd(L, data);
    LuaState::getParent(L).set_interrupts_counter(0);
    return 2;
}

LLCoros::Future<LLLUAmanager::script_result>
LLLUAmanager::startScriptFile(const std::string& filename)
{
    // Despite returning from startScriptFile(), we need this Promise to
    // remain alive until the callback has fired.
    auto promise{ std::make_shared<LLCoros::Promise<script_result>>() };
    runScriptFile(filename, false,
                  [promise](int count, LLSD result)
                  { promise->set_value({ count, result }); });
    return LLCoros::getFuture(*promise);
}

LLLUAmanager::script_result LLLUAmanager::waitScriptFile(const std::string& filename)
{
    return startScriptFile(filename).get();
}

void LLLUAmanager::runScriptFile(const std::string &filename, bool autorun,
                                 script_result_fn result_cb)
{
    // A script_result_fn will be called when LuaState::expr() completes.
    LLCoros::instance().launch(filename, [filename, autorun, result_cb]()
    {
        ScriptObserver observer(LLCoros::getName(), filename);
        LLSD paths(gSavedSettings.getLLSD("LuaCommandPath"));
        LL_DEBUGS("Lua") << "LuaCommandPath = " << paths << LL_ENDL;
        // allow LuaCommandPath to be specified relative to install dir
        ScriptCommand command(filename, paths, gDirUtilp->getAppRODataDir());
        auto error = command.error();
        if (! error.empty())
        {
            if (result_cb)
            {
                result_cb(-1, error);
            }
            return;
        }

        llifstream in_file;
        in_file.open(command.script);
        // At this point, since ScriptCommand did not report an error, we
        // should be able to assume that 'script' exists. If we can't open it,
        // something else is wrong?!
        llassert(in_file.is_open());
        if (autorun)
        {
            sAutorunScriptCount++;
        }
        sScriptCount++;

        LuaState L;
        std::string text{std::istreambuf_iterator<char>(in_file), {}};
        auto [count, result] = L.expr(command.script, text, command.args);
        if (result_cb)
        {
            result_cb(count, result);
        }
    });
}

LLCoros::Future<LLLUAmanager::script_result>
LLLUAmanager::startScriptLine(const std::string& chunk)
{
    // Despite returning from startScriptLine(), we need this Promise to
    // remain alive until the callback has fired.
    auto promise{ std::make_shared<LLCoros::Promise<script_result>>() };
    runScriptLine(chunk,
                  [promise](int count, LLSD result)
                  { promise->set_value({ count, result }); });
    return LLCoros::getFuture(*promise);
}

LLLUAmanager::script_result LLLUAmanager::waitScriptLine(const std::string& chunk)
{
    return startScriptLine(chunk).get();
}

void LLLUAmanager::runScriptLine(const std::string& chunk, script_result_fn result_cb)
{
    // find a suitable abbreviation for the chunk string
    std::string shortchunk{ chunk };
    const size_t shortlen = 40;
    std::string::size_type eol = shortchunk.find_first_of("\r\n");
    if (eol != std::string::npos)
        shortchunk = shortchunk.substr(0, eol);
    if (shortchunk.length() > shortlen)
        shortchunk = stringize(shortchunk.substr(0, shortlen), "...");

    std::string desc{ "lua: " + shortchunk };
    LLCoros::instance().launch(desc, [desc, chunk, result_cb]()
    {
        LuaState L;
        auto [count, result] = L.expr(desc, chunk);
        if (result_cb)
        {
            result_cb(count, result);
        }
    });
}

std::string read_file(const std::string &name)
{
    llifstream in_file;
    in_file.open(name.c_str());

    if (in_file.is_open())
    {
        return std::string{std::istreambuf_iterator<char>(in_file), {}};
    }

    return {};
}

lua_function(require, "require(module_name) : load module_name.lua from known places")
{
    lua_checkdelta(L);
    std::string name = lua_tostdstring(L, 1);
    lua_pop(L, 1);

    // resolveRequire() does not return in case of error.
    LLRequireResolver::resolveRequire(L, name);

    // resolveRequire() returned the newly-loaded module on the stack top.
    // Return it.
    return 1;
}

// push loaded module or throw Lua error
void LLRequireResolver::resolveRequire(lua_State *L, std::string path)
{
    LLRequireResolver resolver(L, std::move(path));
    // findModule() pushes the loaded module or throws a Lua error.
    resolver.findModule();
}

LLRequireResolver::LLRequireResolver(lua_State *L, const std::string& path) :
    mPathToResolve(fsyspath(path).lexically_normal()),
    L(L)
{
    mSourceDir = lluau::source_path(L).parent_path();

    if (mPathToResolve.is_absolute())
        luaL_argerrorL(L, 1, "cannot require a full path");
}

// push the loaded module or throw a Lua error
void LLRequireResolver::findModule()
{
    // If mPathToResolve is absolute, this replaces mSourceDir.
    fsyspath absolutePath(mSourceDir / mPathToResolve);

    // Push _MODULES table on stack for checking and saving to the cache
    luaL_findtable(L, LUA_REGISTRYINDEX, "_MODULES", 1);
    // Remove that stack entry no matter how we exit
    LuaRemover rm_MODULES(L, -1);

    // Check if the module is already in _MODULES table, read from file
    // otherwise.
    // findModuleImpl() pushes module if found, nothing if not, may throw Lua
    // error.
    if (findModuleImpl(absolutePath))
        return;

    // not already cached - prep error message just in case
    auto fail{
        [L=L, path=mPathToResolve.string()]()
        { luaL_error(L, "could not find require('%s')", path.data()); }};

    if (mPathToResolve.is_absolute())
    {
        // no point searching known directories for an absolute path
        fail();
    }

    LLSD lib_paths(gSavedSettings.getLLSD("LuaRequirePath"));
    LL_DEBUGS("Lua") << "LuaRequirePath = " << lib_paths << LL_ENDL;
    for (const auto& path : llsd::inArray(lib_paths))
    {
        // if path is already absolute, operator/() preserves it
        auto abspath(fsyspath(gDirUtilp->getAppRODataDir()) / path.asString());
        fsyspath absolutePathOpt = (abspath / mPathToResolve);

        if (absolutePathOpt.empty())
            luaL_error(L, "error requiring module '%s'", mPathToResolve.string().data());

        if (findModuleImpl(absolutePathOpt))
            return;
    }

    // not found
    fail();
}

// expects _MODULES table on stack top (and leaves it there)
// - if found, pushes loaded module and returns true
// - not found, pushes nothing and returns false
// - may throw Lua error
bool LLRequireResolver::findModuleImpl(const std::string& absolutePath)
{
    std::string possibleSuffixedPaths[] = {absolutePath + ".luau", absolutePath + ".lua"};

    for (const auto& suffixedPath : possibleSuffixedPaths)
    {
         // Check _MODULES cache for module
        lua_getfield(L, -1, suffixedPath.data());
        if (!lua_isnil(L, -1))
        {
            return true;
        }
        lua_pop(L, 1);

        // Try to read the matching file
        std::string source = read_file(suffixedPath);
        if (!source.empty())
        {
            // Try to run the loaded source. This will leave either a string
            // error message or the module contents on the stack top.
            runModule(suffixedPath, source);

            // If the stack top is an error message string, raise it.
            if (lua_isstring(L, -1))
                lua_error(L);

            // duplicate the new module: _MODULES newmodule newmodule
            lua_pushvalue(L, -1);
            // store _MODULES[found path] = newmodule
            lua_setfield(L, -3, suffixedPath.data());

            return true;
        }
    }

    return false;
}

// push string error message or new module
void LLRequireResolver::runModule(const std::string& desc, const std::string& code)
{
    // Here we just loaded a new module 'code', need to run it and get its result.
    lua_State *ML = lua_mainthread(L);

    {
        // If loadstring() returns (! LUA_OK) then there's an error message on
        // the stack. If it returns LUA_OK then the newly-loaded module code
        // is on the stack.
        LL_DEBUGS("Lua") << "Loading module " << desc << LL_ENDL;
        if (lluau::loadstring(ML, desc, code) != LUA_OK)
        {
            // error message on stack top
            LL_DEBUGS("Lua") << "Error loading module " << desc << ": "
                             << lua_tostring(ML, -1) << LL_ENDL;
            lua_pushliteral(ML, "loadstring: ");
            // stack contains error, "loadstring: "
            // swap: insert stack top at position -2
            lua_insert(ML, -2);
            // stack contains "loadstring: ", error
            lua_concat(ML, 2);
            // stack contains "loadstring: " + error
        }
        else                        // module code on stack top
        {
            // push debug module
            lua_getglobal(ML, "debug");
            // push debug.traceback
            lua_getfield(ML, -1, "traceback");
            // stack contains module code, debug, debug.traceback
            // ditch debug
            lua_replace(ML, -2);
            // stack contains module code, debug.traceback
            // swap: insert stack top at position -2
            lua_insert(ML, -2);
            // stack contains debug.traceback, module code
            LL_DEBUGS("Lua") << "Loaded module " << desc << ", running" << LL_ENDL;
            // no arguments, one return value
            // pass debug.traceback as the error function
            int status = lua_pcall(ML, 0, 1, -2);
            // lua_pcall() has popped the module code and replaced it with its
            // return value. Regardless of status or the type of the stack
            // top, get rid of debug.traceback on the stack.
            lua_remove(ML, -2);

            if (status == LUA_OK)
            {
                auto top{ lua_gettop(ML) };
                std::string type{ (top == 0)? "nothing"
                                  : lua_typename(ML, lua_type(ML, -1)) };
                LL_DEBUGS("Lua") << "Module " << desc << " returned " << type << LL_ENDL;
                if ((top == 0) || ! (lua_istable(ML, -1) || lua_isfunction(ML, -1)))
                {
                    lua_pushfstring(ML, "module %s must return a table or function, not %s",
                                    desc.data(), type.data());
                }
            }
            else if (status == LUA_YIELD)
            {
                LL_DEBUGS("Lua") << "Module " << desc << " yielded" << LL_ENDL;
                lua_pushfstring(ML, "module %s can not yield", desc.data());
            }
            else
            {
                llassert(lua_isstring(ML, -1));
                LL_DEBUGS("Lua") << "Module " << desc << " error: "
                                 << lua_tostring(ML, -1) << LL_ENDL;
            }
        }
    }
    // There's now a return value (string error message or module) on top of ML.
    // Move return value to L's stack.
    if (ML != L)
    {
        lua_xmove(ML, L, 1);
    }
}
