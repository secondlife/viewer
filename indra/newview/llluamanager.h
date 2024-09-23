/**
 * @file llluamanager.h
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
 */

#ifndef LL_LLLUAMANAGER_H
#define LL_LLLUAMANAGER_H

#include "fsyspath.h"
#include "llcoros.h"
#include "llsd.h"
#include <functional>
#include <string>
#include <utility>                  // std::pair

#include "luau/lua.h"
#include "luau/lualib.h"

class LuaState;

class LLLUAmanager
{
    friend class ScriptObserver;

public:
    // Pass a callback with this signature to obtain the result, if any, of
    // running a script or source string.
    // count <  0 means error, and result.asString() is the error message.
    // count == 0 with result.isUndefined() means the script returned no results.
    // count == 1 means the script returned one result.
    // count >  1 with result.isArray() means the script returned multiple
    //            results, represented as the entries of the result array.
    typedef std::function<void(int count, const LLSD& result)> script_result_fn;
    // same semantics as script_result_fn parameters
    typedef std::pair<int, LLSD> script_result;

    // Run the script specified by the command line passed as @a filename.
    // This can be followed by some number of command-line arguments, which
    // a Lua script can view using either '...' or predefined global 'arg'.
    // The script pathname or its arguments can be quoted using 'single
    // quotes' or "double quotes", or special characters can be \escaped.
    // runScriptFile() recognizes the case in which the whole 'filename'
    // string is a path containing spaces; if so no arguments are permitted.
    // In either form, if the script pathname isn't absolute, it is sought on
    // LuaCommandPath.
    // If autorun is true, statistics will count this as an autorun script.
    static void runScriptFile(const std::string &filename, bool autorun = false,
                              script_result_fn result_cb = {});
    // Start running a Lua script file, returning an LLCoros::Future whose
    // get() method will pause the calling coroutine until it can deliver the
    // (count, result) pair described above. Between startScriptFile() and
    // Future::get(), the caller and the Lua script coroutine will run
    // concurrently.
    // @a filename is as described for runScriptFile().
    static LLCoros::Future<script_result> startScriptFile(const std::string& filename);
    // Run a Lua script file, and pause the calling coroutine until it completes.
    // The return value is the (count, result) pair described above.
    // @a filename is as described for runScriptFile().
    static script_result waitScriptFile(const std::string& filename);

    static void runScriptLine(const std::string &chunk, script_result_fn result_cb = {});
    // Start running a Lua chunk, returning an LLCoros::Future whose
    // get() method will pause the calling coroutine until it can deliver the
    // (count, result) pair described above. Between startScriptLine() and
    // Future::get(), the caller and the Lua script coroutine will run
    // concurrently.
    static LLCoros::Future<script_result> startScriptLine(const std::string& chunk);
    // Run a Lua chunk, and pause the calling coroutine until it completes.
    // The return value is the (count, result) pair described above.
    static script_result waitScriptLine(const std::string& chunk);

    static const std::map<std::string, std::string> getScriptNames() { return sScriptNames; }

    static S32 sAutorunScriptCount;
    static S32 sScriptCount;

 private:
   static std::map<std::string, std::string> sScriptNames;
};

class LLRequireResolver
{
 public:
    static void resolveRequire(lua_State *L, std::string path);

 private:
    fsyspath mPathToResolve;
    fsyspath mSourceDir;

    LLRequireResolver(lua_State *L, const std::string& path);

    void findModule();
    lua_State *L;

    bool findModuleImpl(const std::string& absolutePath);
    void runModule(const std::string& desc, const std::string& code);
};

// RAII class to guarantee that a script entry is erased even when coro is terminated
class ScriptObserver
{
  public:
    ScriptObserver(const std::string &coro_name, const std::string &filename) : mCoroName(coro_name)
    {
        LLLUAmanager::sScriptNames[mCoroName] = filename;
    }
    ~ScriptObserver() { LLLUAmanager::sScriptNames.erase(mCoroName); }

  private:
    std::string mCoroName;
};
#endif
