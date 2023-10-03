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

#if LL_WINDOWS
#pragma comment(lib, "liblua54.a")
#endif


LLFloaterLUADebug::LLFloaterLUADebug(const LLSD &key)
    : LLFloater(key)
{
}


BOOL LLFloaterLUADebug::postBuild()
{
    mResultOutput = getChild<LLTextEditor>("result_text");
    mLineInput = getChild<LLLineEditor>("lua_cmd");
    mScriptPath = getChild<LLLineEditor>("script_path");
    mOutConnection = LLEventPumps::instance().obtain("lua output")
        .listen("LLFloaterLUADebug",
                [mResultOutput=mResultOutput](const LLSD& data)
                {
                    mResultOutput->insertText(data.asString());
                    mResultOutput->addLineBreakChar(true);
                    return false;
                });

    getChild<LLButton>("execute_btn")->setClickedCallback(boost::bind(&LLFloaterLUADebug::onExecuteClicked, this));
    getChild<LLButton>("browse_btn")->setClickedCallback(boost::bind(&LLFloaterLUADebug::onBtnBrowse, this));
    getChild<LLButton>("run_btn")->setClickedCallback(boost::bind(&LLFloaterLUADebug::onBtnRun, this));

#if !LL_WINDOWS
    getChild<LLButton>("execute_btn")->setEnabled(false);
    getChild<LLButton>("browse_btn")->setEnabled(false);
#endif

    return TRUE;
}

LLFloaterLUADebug::~LLFloaterLUADebug() 
{}

void LLFloaterLUADebug::onExecuteClicked()
{
    mResultOutput->setValue("");

    std::string cmd = mLineInput->getText();
    LLLUAmanager::runScriptLine(cmd, [this](std::string msg)
        { 
            mResultOutput->insertText(msg);
        });
}

void LLFloaterLUADebug::onBtnBrowse()
{
    (new LLFilePickerReplyThread(boost::bind(&LLFloaterLUADebug::runSelectedScript, this, _1), LLFilePicker::FFLOAD_LUA, false))->getFile();
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
    mResultOutput->setValue("");

    std::string filepath = filenames[0];
    if (!filepath.empty())
    {
        mScriptPath->setText(filepath);
        LLLUAmanager::runScriptFile(filepath, [this](std::string msg) 
            { 
                mResultOutput->insertText(msg); 
            });
    }
}

