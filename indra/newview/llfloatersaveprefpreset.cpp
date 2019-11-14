/** 
 * @file llfloatersaveprefpreset.cpp
 * @brief Floater to save a graphics / camera preset
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
#include "lllineeditor.h"
#include "llnotificationsutil.h"
#include "llpresetsmanager.h"
#include "llradiogroup.h"
#include "lltrans.h"

LLFloaterSavePrefPreset::LLFloaterSavePrefPreset(const LLSD &key)
	: LLModalDialog(key)
{
}

// virtual
BOOL LLFloaterSavePrefPreset::postBuild()
{
	LLFloaterPreference* preferences = LLFloaterReg::getTypedInstance<LLFloaterPreference>("preferences");
	if (preferences)
	{
		preferences->addDependentFloater(this);
	}
	
	mPresetCombo = getChild<LLComboBox>("preset_combo");

	mNameEditor = getChild<LLLineEditor>("preset_txt_editor");
	mNameEditor->setKeystrokeCallback(boost::bind(&LLFloaterSavePrefPreset::onPresetNameEdited, this), NULL);

	mSaveButton = getChild<LLButton>("save");
	mSaveButton->setCommitCallback(boost::bind(&LLFloaterSavePrefPreset::onBtnSave, this));
	
	mSaveRadioGroup = getChild<LLRadioGroup>("radio_save_preset");
	mSaveRadioGroup->setCommitCallback(boost::bind(&LLFloaterSavePrefPreset::onSwitchSaveReplace, this));
	
	getChild<LLButton>("cancel")->setCommitCallback(boost::bind(&LLFloaterSavePrefPreset::onBtnCancel, this));

	LLPresetsManager::instance().setPresetListChangeCallback(boost::bind(&LLFloaterSavePrefPreset::onPresetsListChange, this));

	return TRUE;
}

void LLFloaterSavePrefPreset::onPresetNameEdited()
{
	if (mSaveRadioGroup->getSelectedIndex() == 0)
	{
		// Disable saving a preset having empty name.
		std::string name = mNameEditor->getValue();
		mSaveButton->setEnabled(!name.empty());
	}
}

void LLFloaterSavePrefPreset::onOpen(const LLSD& key)
{
	LLModalDialog::onOpen(key);
	S32 index = 0;
	if (key.has("subdirectory"))
	{
		mSubdirectory = key["subdirectory"].asString();
		if (key.has("index"))
		{
			index = key["index"].asInteger();
		}
	}
	else
	{
		mSubdirectory = key.asString();
	}

	std::string floater_title = getString(std::string("title_") + mSubdirectory);

	setTitle(floater_title);

	EDefaultOptions option = DEFAULT_HIDE;
	LLPresetsManager::getInstance()->setPresetNamesInComboBox(mSubdirectory, mPresetCombo, option);

	mSaveRadioGroup->setSelectedIndex(index);
	onPresetNameEdited();
	onSwitchSaveReplace();
}

void LLFloaterSavePrefPreset::onBtnSave()
{
	bool is_saving_new = mSaveRadioGroup->getSelectedIndex() == 0;
	std::string name = is_saving_new ? mNameEditor->getText() : mPresetCombo->getSimple();

	if ((name == LLTrans::getString(PRESETS_DEFAULT)) || (name == PRESETS_DEFAULT))
	{
		LLNotificationsUtil::add("DefaultPresetNotSaved");
	}
	else 
	{
		if (is_saving_new)
		{
			std::list<std::string> preset_names;
			std::string presets_dir = LLPresetsManager::getInstance()->getPresetsDir(mSubdirectory);
			LLPresetsManager::getInstance()->loadPresetNamesFromDir(presets_dir, preset_names, DEFAULT_HIDE);
			if (std::find(preset_names.begin(), preset_names.end(), name) != preset_names.end())
			{
				LLSD args;
				args["NAME"] = name;
				LLNotificationsUtil::add("PresetAlreadyExists", args);
				return;
			}
		}
		if (!LLPresetsManager::getInstance()->savePreset(mSubdirectory, name))
		{
			LLSD args;
			args["NAME"] = name;
			LLNotificationsUtil::add("PresetNotSaved", args);
		}
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

void LLFloaterSavePrefPreset::onSwitchSaveReplace()
{
	bool is_saving_new = mSaveRadioGroup->getSelectedIndex() == 0;
	std::string label = is_saving_new ? getString("btn_label_save") : getString("btn_label_replace");
	mSaveButton->setLabel(label);
	mNameEditor->setEnabled(is_saving_new);
	mPresetCombo->setEnabled(!is_saving_new);
	if (is_saving_new)
	{
		onPresetNameEdited();
	}
	else
	{
		mSaveButton->setEnabled(true);
	}
}
