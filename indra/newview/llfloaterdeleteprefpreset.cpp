/** 
 * @file llfloaterdeleteprefpreset.cpp
 * @brief Floater to delete a graphics / camera preset
 *
 * $LicenseInfo:firstyear=2014&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2014, Linden Research, Inc.
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

#include "llfloaterdeleteprefpreset.h"

#include "llbutton.h"
#include "llcombobox.h"
#include "llfloaterpreference.h"
#include "llnotificationsutil.h"
#include "llpresetsmanager.h"
#include "llviewercontrol.h"
#include "llfloaterreg.h"

LLFloaterDeletePrefPreset::LLFloaterDeletePrefPreset(const LLSD &key)
:   LLFloater(key)
{
}

// virtual
BOOL LLFloaterDeletePrefPreset::postBuild()
{
    LLFloaterPreference* preferences = LLFloaterReg::getTypedInstance<LLFloaterPreference>("preferences");
    if (preferences)
    {
        preferences->addDependentFloater(this);
    }
    getChild<LLButton>("delete")->setCommitCallback(boost::bind(&LLFloaterDeletePrefPreset::onBtnDelete, this));
    getChild<LLButton>("cancel")->setCommitCallback(boost::bind(&LLFloaterDeletePrefPreset::onBtnCancel, this));
    LLPresetsManager::instance().setPresetListChangeCallback(boost::bind(&LLFloaterDeletePrefPreset::onPresetsListChange, this));

    return TRUE;
}

void LLFloaterDeletePrefPreset::onOpen(const LLSD& key)
{
    mSubdirectory = key.asString();
    std::string title_type = std::string("title_") + mSubdirectory;
    if (hasString(title_type))
    {
        std::string floater_title = getString(title_type);
        setTitle(floater_title);
    }
    else
    {
        LL_WARNS() << title_type << " not found" << LL_ENDL;
        setTitle(title_type);
    }

    LLComboBox* combo = getChild<LLComboBox>("preset_combo");
    EDefaultOptions option = DEFAULT_HIDE;
    bool action;
    action = LLPresetsManager::getInstance()->setPresetNamesInComboBox(mSubdirectory, combo, option);

    LLButton* delete_btn = getChild<LLButton>("delete");
    delete_btn->setEnabled(action);
}

void LLFloaterDeletePrefPreset::onBtnDelete()
{
    LLComboBox* combo = getChild<LLComboBox>("preset_combo");
    std::string name = combo->getSimple();

    if (!LLPresetsManager::getInstance()->deletePreset(mSubdirectory, name))
    {
        LLSD args;
        args["NAME"] = name;
        LLNotificationsUtil::add("PresetNotDeleted", args);
    }
    else if (mSubdirectory == PRESETS_CAMERA)
    {
        if (gSavedSettings.getString("PresetCameraActive") == name)
        {
            gSavedSettings.setString("PresetCameraActive", "");
        }
    }

    closeFloater();
}

void LLFloaterDeletePrefPreset::onPresetsListChange()
{
    LLComboBox* combo = getChild<LLComboBox>("preset_combo");

    EDefaultOptions option = DEFAULT_HIDE;

    LLPresetsManager::getInstance()->setPresetNamesInComboBox(mSubdirectory, combo, option);
}

void LLFloaterDeletePrefPreset::onBtnCancel()
{
    closeFloater();
}
