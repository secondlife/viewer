/**
 * @file llfloaterluascriptsinfo.cpp
 *
 * $LicenseInfo:firstyear=2024&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2024, Linden Research, Inc.
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

#include "llfloaterluascripts.h"
#include "llevents.h"
#include <filesystem>
#include "llluamanager.h"
#include "llscrolllistctrl.h"
#include "llviewerwindow.h"
#include "llwindow.h"
#include "llviewermenu.h"

const F32 REFRESH_INTERVAL = 1.0f;

LLFloaterLUAScripts::LLFloaterLUAScripts(const LLSD &key)
 :  LLFloater(key),
    mUpdateTimer(new LLTimer()),
    mContextMenuHandle()
{
    mCommitCallbackRegistrar.add("Script.OpenFolder", {[this](LLUICtrl*, const LLSD &userdata)
    {
        if (mScriptList->hasSelectedItem())
        {
            std::string target_folder_path = std::filesystem::path((mScriptList->getFirstSelected()->getColumn(1)->getValue().asString())).parent_path().string();
            gViewerWindow->getWindow()->openFolder(target_folder_path);
        }
    }, cb_info::UNTRUSTED_BLOCK });
    mCommitCallbackRegistrar.add("Script.Terminate", {[this](LLUICtrl*, const LLSD &userdata)
    {
        std::vector<LLSD> coros = mScriptList->getAllSelectedValues();
        for (auto coro_name : coros)
        {
            LLCoros::instance().killreq(coro_name);
        }
    }, cb_info::UNTRUSTED_BLOCK });
}


bool LLFloaterLUAScripts::postBuild()
{
    mScriptList = getChild<LLScrollListCtrl>("scripts_list");
    mScriptList->setRightMouseDownCallback([this](LLUICtrl *ctrl, S32 x, S32 y, MASK mask) { onScrollListRightClicked(ctrl, x, y);});

    LLContextMenu *menu = LLUICtrlFactory::getInstance()->createFromFile<LLContextMenu>(
        "menu_lua_scripts.xml", gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());
    if (menu)
    {
        mContextMenuHandle = menu->getHandle();
    }

    return true;
}

LLFloaterLUAScripts::~LLFloaterLUAScripts()
{
    auto menu = mContextMenuHandle.get();
    if (menu)
    {
        menu->die();
        mContextMenuHandle.markDead();
    }
}

void LLFloaterLUAScripts::draw()
{
    if (mUpdateTimer->checkExpirationAndReset(REFRESH_INTERVAL))
    {
        populateScriptList();
    }
    LLFloater::draw();
}

void LLFloaterLUAScripts::populateScriptList()
{
    S32  prev_pos = mScriptList->getScrollPos();
    std::vector<LLSD> prev_selected = mScriptList->getAllSelectedValues();
    mScriptList->clearRows();
    mScriptList->updateColumns(true);
    std::map<std::string, std::string> scripts = LLLUAmanager::getScriptNames();
    for (auto &it : scripts)
    {
        LLSD row;
        row["value"] = it.first;
        row["columns"][0]["value"] = std::filesystem::path((it.second)).stem().string();
        row["columns"][0]["column"] = "script_name";
        row["columns"][1]["value"] = it.second;
        row["columns"][1]["column"] = "script_path";
        mScriptList->addElement(row);
    }
    mScriptList->setScrollPos(prev_pos);
    for (auto value : prev_selected)
    {
        mScriptList->setSelectedByValue(value, true);
    }
}

void LLFloaterLUAScripts::onScrollListRightClicked(LLUICtrl *ctrl, S32 x, S32 y)
{
    LLScrollListItem *item = mScriptList->hitItem(x, y);
    if (item)
    {
        if (!item->getSelected())
            mScriptList->selectItemAt(x, y, MASK_NONE);

        auto menu = mContextMenuHandle.get();
        if (menu)
        {
            menu->setItemEnabled(std::string("open_folder"), (mScriptList->getNumSelected() == 1));
            menu->show(x, y);
            LLMenuGL::showPopup(this, menu, x, y);
        }
    }
}
