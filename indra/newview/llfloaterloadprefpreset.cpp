/** 
 * @file llfloateloadprefpreset.cpp
 * @brief Floater to load a graphics / camera preset
 *
 * $LicenseInfo:firstyear=2015&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2015, Linden Research, Inc.
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

#include "llfloaterloadprefpreset.h"

#include "llbutton.h"
#include "llcombobox.h"
#include "llfloaterpreference.h"
#include "llfloaterreg.h"
#include "llpresetsmanager.h"
#include "llviewercontrol.h"

LLFloaterLoadPrefPreset::LLFloaterLoadPrefPreset(const LLSD &key)
:   LLFloater(key)
{
}

// virtual
BOOL LLFloaterLoadPrefPreset::postBuild()
{
    LLFloaterPreference* preferences = LLFloaterReg::getTypedInstance<LLFloaterPreference>("preferences");
    if (preferences)
    {
        preferences->addDependentFloater(this);
    }
    getChild<LLButton>("ok")->setCommitCallback(boost::bind(&LLFloaterLoadPrefPreset::onBtnOk, this));
    getChild<LLButton>("cancel")->setCommitCallback(boost::bind(&LLFloaterLoadPrefPreset::onBtnCancel, this));
    LLPresetsManager::instance().setPresetListChangeCallback(boost::bind(&LLFloaterLoadPrefPreset::onPresetsListChange, this));

    return TRUE;
}

void LLFloaterLoadPrefPreset::onOpen(const LLSD& key)
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

    EDefaultOptions option = DEFAULT_TOP;
    LLPresetsManager::getInstance()->setPresetNamesInComboBox(mSubdirectory, combo, option);
    std::string preset_graphic_active = gSavedSettings.getString("PresetGraphicActive");
    if (!preset_graphic_active.empty())
    {
        combo->setSimple(preset_graphic_active);
    }
}

void LLFloaterLoadPrefPreset::onPresetsListChange()
{
    LLComboBox* combo = getChild<LLComboBox>("preset_combo");

    EDefaultOptions option = DEFAULT_TOP;
    LLPresetsManager::getInstance()->setPresetNamesInComboBox(mSubdirectory, combo, option);
    std::string preset_graphic_active = gSavedSettings.getString("PresetGraphicActive");
    if (!preset_graphic_active.empty())
    {
        combo->setSimple(preset_graphic_active);
    }
}

void LLFloaterLoadPrefPreset::onBtnCancel()
{
    closeFloater();
}

void LLFloaterLoadPrefPreset::onBtnOk()
{
    LLComboBox* combo = getChild<LLComboBox>("preset_combo");
    std::string name = combo->getSimple();

    LLPresetsManager::getInstance()->loadPreset(mSubdirectory, name);

    closeFloater();
}
