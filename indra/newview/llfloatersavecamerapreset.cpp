/** 
 * @file llfloatersavecamerapreset.cpp
 * @brief Floater to save a camera preset
 *
 * $LicenseInfo:firstyear=2020&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2020, Linden Research, Inc.
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

#include "llfloatersavecamerapreset.h"

#include "llagent.h"
#include "llagentcamera.h"
#include "llbutton.h"
#include "llcombobox.h"
#include "llfloaterpreference.h"
#include "llfloaterreg.h"
#include "lllineeditor.h"
#include "llnotificationsutil.h"
#include "llpresetsmanager.h"
#include "llradiogroup.h"
#include "lltrans.h"
#include "llvoavatarself.h"

LLFloaterSaveCameraPreset::LLFloaterSaveCameraPreset(const LLSD &key)
	: LLModalDialog(key)
{
}

// virtual
BOOL LLFloaterSaveCameraPreset::postBuild()
{
	mPresetCombo = getChild<LLComboBox>("preset_combo");

	mNameEditor = getChild<LLLineEditor>("preset_txt_editor");
	mNameEditor->setKeystrokeCallback(boost::bind(&LLFloaterSaveCameraPreset::onPresetNameEdited, this), NULL);

	mSaveButton = getChild<LLButton>("save");
	mSaveButton->setCommitCallback(boost::bind(&LLFloaterSaveCameraPreset::onBtnSave, this));
	
	mSaveRadioGroup = getChild<LLRadioGroup>("radio_save_preset");
	mSaveRadioGroup->setCommitCallback(boost::bind(&LLFloaterSaveCameraPreset::onSwitchSaveReplace, this));
	
	getChild<LLButton>("cancel")->setCommitCallback(boost::bind(&LLFloaterSaveCameraPreset::onBtnCancel, this));

	LLPresetsManager::instance().setPresetListChangeCallback(boost::bind(&LLFloaterSaveCameraPreset::onPresetsListChange, this));

	return TRUE;
}

void LLFloaterSaveCameraPreset::onPresetNameEdited()
{
	if (mSaveRadioGroup->getSelectedIndex() == 0)
	{
		// Disable saving a preset having empty name.
		std::string name = mNameEditor->getValue();
		mSaveButton->setEnabled(!name.empty());
	}
}

void LLFloaterSaveCameraPreset::onOpen(const LLSD& key)
{
	LLModalDialog::onOpen(key);
	S32 index = 0;
	if (key.has("index"))
	{
		index = key["index"].asInteger();
	}

	LLPresetsManager::getInstance()->setPresetNamesInComboBox(PRESETS_CAMERA, mPresetCombo, DEFAULT_BOTTOM);

	mSaveRadioGroup->setSelectedIndex(index);
	onPresetNameEdited();
	onSwitchSaveReplace();
}

void LLFloaterSaveCameraPreset::onBtnSave()
{
	bool is_saving_new = mSaveRadioGroup->getSelectedIndex() == 0;
	std::string name = is_saving_new ? mNameEditor->getText() : mPresetCombo->getSimple();

	if ((name == LLTrans::getString(PRESETS_DEFAULT)) || (name == PRESETS_DEFAULT))
	{
		LLNotificationsUtil::add("DefaultPresetNotSaved");
	}
	else 
	{
		if (isAgentAvatarValid() && gAgentAvatarp->getParent())
		{
			gSavedSettings.setLLSD("AvatarSitRotation", gAgent.getFrameAgent().getQuaternion().getValue());
		}
		if (gAgentCamera.isJoystickCameraUsed())
		{
			gSavedSettings.setVector3("CameraOffsetRearView", gAgentCamera.getCurrentCameraOffset());
			gSavedSettings.setVector3d("FocusOffsetRearView", gAgentCamera.getCurrentFocusOffset());
			gAgentCamera.resetCameraZoomFraction();
			gAgentCamera.setFocusOnAvatar(TRUE, TRUE, FALSE);
		}
		else
		{
			LLVector3 camera_offset = gSavedSettings.getVector3("CameraOffsetRearView") * gAgentCamera.getCurrentCameraZoomFraction();
			gSavedSettings.setVector3("CameraOffsetRearView", camera_offset);
			gAgentCamera.resetCameraZoomFraction();
		}
		if (is_saving_new)
		{
			std::list<std::string> preset_names;
			LLPresetsManager::getInstance()->loadPresetNamesFromDir(PRESETS_CAMERA, preset_names, DEFAULT_HIDE);
			if (std::find(preset_names.begin(), preset_names.end(), name) != preset_names.end())
			{
				LLSD args;
				args["NAME"] = name;
				LLNotificationsUtil::add("PresetAlreadyExists", args);
				return;
			}
		}
		if (!LLPresetsManager::getInstance()->savePreset(PRESETS_CAMERA, name))
		{
			LLSD args;
			args["NAME"] = name;
			LLNotificationsUtil::add("PresetNotSaved", args);
		}
	}

	closeFloater();
}

void LLFloaterSaveCameraPreset::onPresetsListChange()
{
	LLPresetsManager::getInstance()->setPresetNamesInComboBox(PRESETS_CAMERA, mPresetCombo, DEFAULT_BOTTOM);
}

void LLFloaterSaveCameraPreset::onBtnCancel()
{
	closeFloater();
}

void LLFloaterSaveCameraPreset::onSwitchSaveReplace()
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
