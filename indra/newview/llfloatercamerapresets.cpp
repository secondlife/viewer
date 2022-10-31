/** 
* @file llfloatercamerapresets.cpp
*
* $LicenseInfo:firstyear=2019&license=viewerlgpl$
* Second Life Viewer Source Code
* Copyright (C) 2019, Linden Research, Inc.
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

#include "llfloatercamerapresets.h"
#include "llfloaterreg.h"
#include "llnotificationsutil.h"
#include "llpresetsmanager.h"
#include "llviewercontrol.h"

LLFloaterCameraPresets::LLFloaterCameraPresets(const LLSD& key)
:   LLFloater(key)
{}

LLFloaterCameraPresets::~LLFloaterCameraPresets()
{}

BOOL LLFloaterCameraPresets::postBuild()
{
    mPresetList = getChild<LLFlatListView>("preset_list");

    LLPresetsManager::getInstance()->setPresetListChangeCameraCallback(boost::bind(&LLFloaterCameraPresets::populateList, this));

    return TRUE;
}
void LLFloaterCameraPresets::onOpen(const LLSD& key)
{
    populateList();
}

void LLFloaterCameraPresets::populateList()
{
    mPresetList->clear();

    LLPresetsManager* presetsMgr = LLPresetsManager::getInstance();
    std::list<std::string> preset_names;

    presetsMgr->loadPresetNamesFromDir(PRESETS_CAMERA, preset_names, DEFAULT_BOTTOM);

    for (std::list<std::string>::const_iterator it = preset_names.begin(); it != preset_names.end(); ++it)
    {
        const std::string& name = *it;
        bool is_default = presetsMgr->isDefaultCameraPreset(name);
        LLCameraPresetFlatItem* item = new LLCameraPresetFlatItem(name, is_default);
        item->postBuild();
        mPresetList->addItem(item);
    }
}

LLCameraPresetFlatItem::LLCameraPresetFlatItem(const std::string &preset_name, bool is_default)
    : LLPanel(),
    mPresetName(preset_name),
    mIsDefaultPrest(is_default)
{
    mCommitCallbackRegistrar.add("CameraPresets.Delete", boost::bind(&LLCameraPresetFlatItem::onDeleteBtnClick, this));
    mCommitCallbackRegistrar.add("CameraPresets.Reset", boost::bind(&LLCameraPresetFlatItem::onResetBtnClick, this));
    buildFromFile("panel_camera_preset_item.xml");
}

LLCameraPresetFlatItem::~LLCameraPresetFlatItem()
{
}

BOOL LLCameraPresetFlatItem::postBuild()
{
    mDeleteBtn = getChild<LLButton>("delete_btn");
    mDeleteBtn->setVisible(false);

    mResetBtn = getChild<LLButton>("reset_btn");
    mResetBtn->setVisible(false);

    LLStyle::Params style;
    LLTextBox* name_text = getChild<LLTextBox>("preset_name");
    LLFontDescriptor new_desc(name_text->getFont()->getFontDesc());
    new_desc.setStyle(mIsDefaultPrest ? LLFontGL::ITALIC : LLFontGL::NORMAL);
    LLFontGL* new_font = LLFontGL::getFont(new_desc);
    style.font = new_font;
    name_text->setText(mPresetName, style);

    return true;
}

void LLCameraPresetFlatItem::onMouseEnter(S32 x, S32 y, MASK mask)
{
    mDeleteBtn->setVisible(!mIsDefaultPrest);
    mResetBtn->setVisible(mIsDefaultPrest);
    getChildView("hovered_icon")->setVisible(true);
    LLPanel::onMouseEnter(x, y, mask);
}

void LLCameraPresetFlatItem::onMouseLeave(S32 x, S32 y, MASK mask)
{
    mDeleteBtn->setVisible(false);
    mResetBtn->setVisible(false);
    getChildView("hovered_icon")->setVisible(false);
    LLPanel::onMouseLeave(x, y, mask);
}

void LLCameraPresetFlatItem::setValue(const LLSD& value)
{
    if (!value.isMap()) return;;
    if (!value.has("selected")) return;
    getChildView("selected_icon")->setVisible(value["selected"]);
}

void LLCameraPresetFlatItem::onDeleteBtnClick()
{
    if (!LLPresetsManager::getInstance()->deletePreset(PRESETS_CAMERA, mPresetName))
    {
        LLSD args;
        args["NAME"] = mPresetName;
        LLNotificationsUtil::add("PresetNotDeleted", args);
    }
    else if (gSavedSettings.getString("PresetCameraActive") == mPresetName)
    {
        gSavedSettings.setString("PresetCameraActive", "");
    }
}

void LLCameraPresetFlatItem::onResetBtnClick()
{
    LLPresetsManager::getInstance()->resetCameraPreset(mPresetName);
}
