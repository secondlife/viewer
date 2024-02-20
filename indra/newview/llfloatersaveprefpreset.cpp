/** 
 * @file llfloatersaveprefpreset.cpp
 * @brief Floater to save a graphics preset
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

#include "llfloatersaveprefpreset.h"

#include "llbutton.h"
#include "llcombobox.h"
#include "llfloaterpreference.h"
#include "llfloaterreg.h"
#include "llnotificationsutil.h"
#include "llpresetsmanager.h"
#include "lltrans.h"

LLFloaterSavePrefPreset::LLFloaterSavePrefPreset(const LLSD &key)
	: LLFloater(key)
{
}

// virtual
bool LLFloaterSavePrefPreset::postBuild()
{
	LLFloaterPreference* preferences = LLFloaterReg::getTypedInstance<LLFloaterPreference>("preferences");
	if (preferences)
	{
		preferences->addDependentFloater(this);
	}
	
	getChild<LLComboBox>("preset_combo")->setTextEntryCallback(boost::bind(&LLFloaterSavePrefPreset::onPresetNameEdited, this));
	getChild<LLComboBox>("preset_combo")->setCommitCallback(boost::bind(&LLFloaterSavePrefPreset::onPresetNameEdited, this));
	getChild<LLButton>("save")->setCommitCallback(boost::bind(&LLFloaterSavePrefPreset::onBtnSave, this));

	getChild<LLButton>("cancel")->setCommitCallback(boost::bind(&LLFloaterSavePrefPreset::onBtnCancel, this));

	LLPresetsManager::instance().setPresetListChangeCallback(boost::bind(&LLFloaterSavePrefPreset::onPresetsListChange, this));
	
	mSaveButton = getChild<LLButton>("save");
	mPresetCombo = getChild<LLComboBox>("preset_combo");

	return true;
}

void LLFloaterSavePrefPreset::onPresetNameEdited()
{
	// Disable saving a preset having empty name.
	std::string name = mPresetCombo->getSimple();

	mSaveButton->setEnabled(!name.empty());
}

void LLFloaterSavePrefPreset::onOpen(const LLSD& key)
{
	mSubdirectory = key.asString();

	EDefaultOptions option = DEFAULT_HIDE;
	LLPresetsManager::getInstance()->setPresetNamesInComboBox(mSubdirectory, mPresetCombo, option);

	onPresetNameEdited();
}

void LLFloaterSavePrefPreset::onBtnSave()
{
	std::string name = mPresetCombo->getSimple();

	std::string upper_name(name);
	LLStringUtil::toUpper(upper_name);

	if ((name == LLTrans::getString(PRESETS_DEFAULT)) || (upper_name == PRESETS_DEFAULT_UPPER))
	{
		LLNotificationsUtil::add("DefaultPresetNotSaved");
	}
	else if (!LLPresetsManager::getInstance()->savePreset(mSubdirectory, name))
	{
		LLSD args;
		args["NAME"] = name;
		LLNotificationsUtil::add("PresetNotSaved", args);
	}

	closeFloater();
}

void LLFloaterSavePrefPreset::onPresetsListChange()
{
	EDefaultOptions option = DEFAULT_HIDE;
	LLPresetsManager::getInstance()->setPresetNamesInComboBox(mSubdirectory, mPresetCombo, option);
}

void LLFloaterSavePrefPreset::onBtnCancel()
{
	closeFloater();
}
