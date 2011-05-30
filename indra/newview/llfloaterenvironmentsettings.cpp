/** 
 * @file llfloaterenvironmentsettings.cpp
 * @brief LLFloaterEnvironmentSettings class definition
 *
 * $LicenseInfo:firstyear=2011&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2011, Linden Research, Inc.
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

#include "llfloaterenvironmentsettings.h"

#include "llcombobox.h"
#include "llradiogroup.h"

#include "lldaycyclemanager.h"
#include "llenvmanager.h"
#include "llwaterparammanager.h"
#include "llwlparamset.h"
#include "llwlparammanager.h"

LLFloaterEnvironmentSettings::LLFloaterEnvironmentSettings(const LLSD &key)
: 	 LLFloater(key)
	,mRegionSettingsRadioGroup(NULL)
	,mDayCycleSettingsRadioGroup(NULL)
	,mWaterPresetCombo(NULL)
	,mSkyPresetCombo(NULL)
	,mDayCyclePresetCombo(NULL)
{	
}

LLFloaterEnvironmentSettings::~LLFloaterEnvironmentSettings()
{
}

// virtual
BOOL LLFloaterEnvironmentSettings::postBuild()
{	
	mRegionSettingsRadioGroup = getChild<LLRadioGroup>("region_settings_radio_group");
	mRegionSettingsRadioGroup->setCommitCallback(boost::bind(&LLFloaterEnvironmentSettings::onSwitchRegionSettings, this));

	mDayCycleSettingsRadioGroup = getChild<LLRadioGroup>("sky_dayc_settings_radio_group");
	mDayCycleSettingsRadioGroup->setCommitCallback(boost::bind(&LLFloaterEnvironmentSettings::onSwitchDayCycle, this));

	mWaterPresetCombo = getChild<LLComboBox>("water_settings_preset_combo");
	mWaterPresetCombo->setCommitCallback(boost::bind(&LLFloaterEnvironmentSettings::onSelectWaterPreset, this));

	mSkyPresetCombo = getChild<LLComboBox>("sky_settings_preset_combo");
	mSkyPresetCombo->setCommitCallback(boost::bind(&LLFloaterEnvironmentSettings::onSelectSkyPreset, this));

	mDayCyclePresetCombo = getChild<LLComboBox>("dayc_settings_preset_combo");
	mDayCyclePresetCombo->setCommitCallback(boost::bind(&LLFloaterEnvironmentSettings::onSelectDayCyclePreset, this));

	childSetCommitCallback("ok_btn", boost::bind(&LLFloaterEnvironmentSettings::onBtnOK, this), NULL);
	getChild<LLUICtrl>("ok_btn")->setRightMouseDownCallback(boost::bind(&LLEnvManagerNew::dumpUserPrefs, LLEnvManagerNew::getInstance()));
	childSetCommitCallback("cancel_btn", boost::bind(&LLFloaterEnvironmentSettings::onBtnCancel, this), NULL);

	setCloseCallback(boost::bind(&LLFloaterEnvironmentSettings::cancel, this));

	return TRUE;
}

// virtual
void LLFloaterEnvironmentSettings::onOpen(const LLSD& key)
{
	LLEnvManagerNew *env_mgr = LLEnvManagerNew::getInstance();

	// Save UseRegionSettings and UseFixedSky settings to restore them
	// in case of "Cancel" button has been pressed.
	mUseRegionSettings = env_mgr->getUseRegionSettings();
	mUseFixedSky = env_mgr->getUseFixedSky();

	mRegionSettingsRadioGroup->setSelectedIndex(mUseRegionSettings ? 0 : 1);
	mDayCycleSettingsRadioGroup->setSelectedIndex(mUseFixedSky ? 0 : 1);

	// Update other controls state based on the selected radio buttons.
	onSwitchRegionSettings();
	onSwitchDayCycle();

	// Populate the combo boxes with appropriate lists of available presets.
	populateWaterPresetsList();
	populateSkyPresetsList();
	populateDayCyclePresetsList();

	// Save water, sky and day cycle presets to restore them
	// in case of "Cancel" button has been pressed.
	mWaterPreset = env_mgr->getWaterPresetName();
	mSkyPreset = env_mgr->getSkyPresetName();
	mDayCyclePreset = env_mgr->getDayCycleName();

	// Select the current presets in combo boxes.
	mWaterPresetCombo->selectByValue(mWaterPreset);
	mSkyPresetCombo->selectByValue(mSkyPreset);
	mDayCyclePresetCombo->selectByValue(mDayCyclePreset);

	mDirty = false;
}

void LLFloaterEnvironmentSettings::onSwitchRegionSettings()
{
	getChild<LLView>("user_environment_settings")->setEnabled(mRegionSettingsRadioGroup->getSelectedIndex() != 0);

	LLEnvManagerNew::getInstance()->setUseRegionSettings(mRegionSettingsRadioGroup->getSelectedIndex() == 0);

	mDirty = true;
}

void LLFloaterEnvironmentSettings::onSwitchDayCycle()
{
	bool is_fixed_sky = mDayCycleSettingsRadioGroup->getSelectedIndex() == 0;

	mSkyPresetCombo->setEnabled(is_fixed_sky);
	mDayCyclePresetCombo->setEnabled(!is_fixed_sky);

	if (is_fixed_sky)
	{
		LLEnvManagerNew::getInstance()->setUseSkyPreset(mSkyPresetCombo->getValue().asString());
	}
	else
	{
		LLEnvManagerNew::getInstance()->setUseDayCycle(mDayCyclePresetCombo->getValue().asString());
	}

	mDirty = true;
}

void LLFloaterEnvironmentSettings::onSelectWaterPreset()
{
	LLEnvManagerNew::getInstance()->setUseWaterPreset(mWaterPresetCombo->getValue().asString());
	mDirty = true;
}

void LLFloaterEnvironmentSettings::onSelectSkyPreset()
{
	LLEnvManagerNew::getInstance()->setUseSkyPreset(mSkyPresetCombo->getValue().asString());
	mDirty = true;
}

void LLFloaterEnvironmentSettings::onSelectDayCyclePreset()
{
	LLEnvManagerNew::getInstance()->setUseDayCycle(mDayCyclePresetCombo->getValue().asString());
	mDirty = true;
}

void LLFloaterEnvironmentSettings::onBtnOK()
{
	mDirty = false;
	closeFloater();
}

void LLFloaterEnvironmentSettings::onBtnCancel()
{
	cancel();
	closeFloater();
}

void LLFloaterEnvironmentSettings::cancel()
{
	if (!mDirty) return;

	// Restore the saved user prefs
	LLEnvManagerNew::getInstance()->setUserPrefs(mWaterPreset, mSkyPreset, mDayCyclePreset, mUseFixedSky, mUseRegionSettings);
}

void LLFloaterEnvironmentSettings::populateWaterPresetsList()
{
	mWaterPresetCombo->removeall();

	const std::map<std::string, LLWaterParamSet> &water_params_map = LLWaterParamManager::getInstance()->mParamList;
	for (std::map<std::string, LLWaterParamSet>::const_iterator it = water_params_map.begin(); it != water_params_map.end(); it++)
	{
		mWaterPresetCombo->add(it->first);
	}
}

void LLFloaterEnvironmentSettings::populateSkyPresetsList()
{
	mSkyPresetCombo->removeall();

	const std::map<LLWLParamKey, LLWLParamSet> &sky_params_map = LLWLParamManager::getInstance()->mParamList;
	for (std::map<LLWLParamKey, LLWLParamSet>::const_iterator it = sky_params_map.begin(); it != sky_params_map.end(); it++)
	{
		if (it->first.scope == LLEnvKey::SCOPE_REGION) continue; // list only local presets
		mSkyPresetCombo->add(it->first.name);
	}
}

void LLFloaterEnvironmentSettings::populateDayCyclePresetsList()
{
	mDayCyclePresetCombo->removeall();

	const LLDayCycleManager::dc_map_t& map = LLDayCycleManager::instance().getPresets();
	for (LLDayCycleManager::dc_map_t::const_iterator it = map.begin(); it != map.end(); ++it)
	{
		mDayCyclePresetCombo->add(it->first);
	}
}
