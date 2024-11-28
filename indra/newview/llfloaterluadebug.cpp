/**
 * @file llfloaterluadebug.cpp
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

#include "llviewerprecompiledheaders.h"

#include "llfloaterluadebug.h"

#include "lllineeditor.h"
#include "lltexteditor.h"
#include "llviewermenufile.h" // LLFilePickerReplyThread

#include "llagent.h"
#include "llappearancemgr.h"
#include "llfloaterreg.h"
#include "llfloaterimnearbychat.h"

#include "llluamanager.h"
#include "llsdutil.h"
#include "lua_function.h"
#include "stringize.h"
#include "tempset.h"


LLFloaterLUADebug::LLFloaterLUADebug(const LLSD &key)
    : LLFloater(key)
{
}


bool LLFloaterLUADebug::postBuild()
{
    mResultOutput = getChild<LLTextEditor>("result_text");
    mLineInput = getChild<LLLineEditor>("lua_cmd");
    mScriptPath = getChild<LLLineEditor>("script_path");
    mOutConnection = LLEventPumps::instance().obtain("lua output")
        .listen("LLFloaterLUADebug",
                [mResultOutput=mResultOutput](const LLSD& data)
                {
                    if(data.has("msg"))
                    {
                        LLCachedControl<bool> show_source_info(gSavedSettings, "LuaDebugShowSource", false);
                        std::string source_info = show_source_info ? data["source_info"].asString() : "";
                        mResultOutput->pasteTextWithLinebreaks(stringize(data["level"].asString(), source_info, data["msg"].asString()), true);
                    }
                    else
                    {
                        //LL.leaphelp(), LL.help()
                        mResultOutput->pasteTextWithLinebreaks(data.asString(), true);
                    }
                    mResultOutput->addLineBreakChar(true);
                    return false;
                });

    getChild<LLButton>("execute_btn")->setClickedCallback(boost::bind(&LLFloaterLUADebug::onExecuteClicked, this));
    getChild<LLButton>("browse_btn")->setClickedCallback(boost::bind(&LLFloaterLUADebug::onBtnBrowse, this));
    getChild<LLButton>("run_btn")->setClickedCallback(boost::bind(&LLFloaterLUADebug::onBtnRun, this));
    mLineInput->setCommitCallback(boost::bind(&LLFloaterLUADebug::onExecuteClicked, this));
    mLineInput->setSelectAllonCommit(false);

    return true;
}

LLFloaterLUADebug::~LLFloaterLUADebug()
{}

void LLFloaterLUADebug::onExecuteClicked()
{
    // Empirically, running Lua code that indirectly invokes the
    // "LLNotifications" listener can result (via mysterious labyrinthine
    // viewer UI byways) in a recursive call to this handler. We've seen Bad
    // Things happen to the viewer with a second call to runScriptLine() with
    // the same cmd on the same LuaState.
    if (mExecuting)
    {
        LL_DEBUGS("Lua") << "recursive call to onExecuteClicked()" << LL_ENDL;
        return;
    }
    TempSet executing(mExecuting, true);
    mResultOutput->setValue("");

    std::string cmd = mLineInput->getText();
    LLLUAmanager::runScriptLine(cmd, [this](int count, const LLSD& result)
        {
            completion(count, result);
        });
}

void LLFloaterLUADebug::onBtnBrowse()
{
    LLFilePickerReplyThread::startPicker(boost::bind(&LLFloaterLUADebug::runSelectedScript, this, _1), LLFilePicker::FFLOAD_LUA, false);
}

void LLFloaterLUADebug::onBtnRun()
{
    std::vector<std::string> filenames;
    std::string filepath = mScriptPath->getText();
    if (!filepath.empty())
    {
        filenames.push_back(filepath);
        runSelectedScript(filenames);
    }
}

void LLFloaterLUADebug::runSelectedScript(const std::vector<std::string> &filenames)
{
    if (mExecuting)
    {
        LL_DEBUGS("Lua") << "recursive call to runSelectedScript()" << LL_ENDL;
        return;
    }
    TempSet executing(mExecuting, true);
    mResultOutput->setValue("");

    std::string filepath = filenames[0];
    if (!filepath.empty())
    {
        mScriptPath->setText(filepath);
        LLLUAmanager::runScriptFile(filepath, false, [this](int count, const LLSD &result)
        {
            completion(count, result);
        });
    }
}

void LLFloaterLUADebug::completion(int count, const LLSD& result)
{
    if (count < 0)
    {
        // error: show error message
        LLStyle::Params params;
        params.readonly_color = LLUIColorTable::instance().getColor("LtRed");
        mResultOutput->appendText(result.asString(), false, params);
        mResultOutput->endOfDoc();
        return;
    }
    if (count == 0)
    {
        // no results
        mResultOutput->pasteTextWithLinebreaks(stringize("ok ", ++mAck));
        return;
    }
    if (count == 1)
    {
        // single result
        mResultOutput->pasteTextWithLinebreaks(stringize(result));
        return;
    }
    // multiple results
    const char* sep = "";
    for (const auto& item : llsd::inArray(result))
    {
        mResultOutput->insertText(sep);
        mResultOutput->pasteTextWithLinebreaks(stringize(item));
        sep = ", ";
    }
}
