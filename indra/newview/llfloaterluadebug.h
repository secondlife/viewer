/**
 * @file llfloaterluadebug.h
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

#ifndef LL_LLFLOATERLUADEBUG_H
#define LL_LLFLOATERLUADEBUG_H

#include "llevents.h"
#include "llfloater.h"
#include "lua_function.h"

extern "C"
{
#include "luau/luacode.h"
#include "luau/lua.h"
#include "luau/luaconf.h"
#include "luau/lualib.h"
}

class LLLineEditor;
class LLTextEditor;

class LLFloaterLUADebug :
    public LLFloater
{
 public:
    LLFloaterLUADebug(const LLSD& key);
    virtual ~LLFloaterLUADebug();

    /*virtual*/ bool postBuild();

    void onExecuteClicked();
    void onBtnBrowse();
    void onBtnRun();

    void runSelectedScript(const std::vector<std::string> &filenames);

private:
    void completion(int count, const LLSD& result);

    LLTempBoundListener mOutConnection;

    LLTextEditor* mResultOutput;
    LLLineEditor* mLineInput;
    LLLineEditor* mScriptPath;
    U32 mAck{ 0 };
    bool mExecuting{ false };
};

#endif  // LL_LLFLOATERLUADEBUG_H

