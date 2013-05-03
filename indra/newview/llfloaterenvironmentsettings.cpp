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
	getChild<LLUICtrl>("cancel_btn")->setRightMouseDownCallback(boost::bind(&LLEnvManagerNew::dumpPresets, LLEnvManagerNew::getInstance()));

	setCloseCallback(boost::bind(&LLFloaterEnvironmentSettings::cancel, this));

	LLEnvManagerNew::instance().setPreferencesChangeCallback(boost::bind(&LLFloaterEnvironmentSettings::refresh, this));
	LLDayCycleManager::instance().setModifyCallback(boost::bind(&LLFloaterEnvironmentSettings::populateDayCyclePresetsList, this));
	LLWLParamManager::instance().setPresetListChangeCallback(boost::bind(&LLFloaterEnvironmentSettings::populateSkyPresetsList, this));
	LLWaterParamManager::instance().setPresetListChangeCallback(boost::bind(&LLFloaterEnvironmentSettings::populateWaterPresetsList, this));

	return TRUE;
}

// virtual
void LLFloaterEnvironmentSettings::onOpen(const LLSD& key)
{
	refresh();
}

void LLFloaterEnvironmentSettings::onSwitchRegionSettings()
{
	getChild<LLView>("user_environment_settings")->setEnabled(mRegionSettingsRadioGroup->getSelectedIndex() != 0);

	apply();
}

void LLFloaterEnvironmentSettings::onSwitchDayCycle()
{
	bool is_fixed_sky = mDayCycleSettingsRadioGroup->getSelectedIndex() == 0;

	mSkyPresetCombo->setEnabled(is_fixed_sky);
	mDayCyclePresetCombo->setEnabled(!is_fixed_sky);

	apply();
}

void LLFloaterEnvironmentSettings::onSelectWaterPreset()
{
	apply();
}

void LLFloaterEnvironmentSettings::onSelectSkyPreset()
{
	apply();
}

void LLFloaterEnvironmentSettings::onSelectDayCyclePreset()
{
	apply();
}

void LLFloaterEnvironmentSettings::onBtnOK()
{
	// Save and apply new user preferences.
	bool use_region_settings	= mRegionSettingsRadioGroup->getSelectedIndex() == 0;
	bool use_fixed_sky			= mDayCycleSettingsRadioGroup->getSelectedIndex() == 0;
	std::string water_preset	= mWaterPresetCombo->getValue().asString();
	std::string sky_preset		= mSkyPresetCombo->getValue().asString();
	std::string day_cycle		= mDayCyclePresetCombo->getValue().asString();

	LLEnvManagerNew::instance().setUserPrefs(
		water_preset,
		sky_preset,
		day_cycle,
		use_fixed_sky,
		use_region_settings);

	// *TODO: This triggers applying user preferences again, which is suboptimal.
	closeFloater();
}

void LLFloaterEnvironmentSettings::onBtnCancel()
{
	closeFloater();
}

void LLFloaterEnvironmentSettings::refresh()
{
	LLEnvManagerNew& env_mgr = LLEnvManagerNew::instance();

	bool use_region_settings	= env_mgr.getUseRegionSettings();
	bool use_fixed_sky			= env_mgr.getUseFixedSky();

	// Set up radio buttons according to user preferences.
	mRegionSettingsRadioGroup->setSelectedIndex(use_region_settings ? 0 : 1);
	mDayCycleSettingsRadioGroup->setSelectedIndex(use_fixed_sky ? 0 : 1);

	// Populate the combo boxes with appropriate lists of available presets.
	populateWaterPresetsList();
	populateSkyPresetsList();
	populateDayCyclePresetsList();

	// Enable/disable other controls based on user preferences.
	getChild<LLView>("user_environment_settings")->setEnabled(!use_region_settings);
	mSkyPresetCombo->setEnabled(use_fixed_sky);
	mDayCyclePresetCombo->setEnabled(!use_fixed_sky);

	// Select the current presets in combo boxes.
	mWaterPresetCombo->selectByValue(env_mgr.getWaterPresetName());
	mSkyPresetCombo->selectByValue(env_mgr.getSkyPresetName());
	mDayCyclePresetCombo->selectByValue(env_mgr.getDayCycleName());
}

void LLFloaterEnvironmentSettings::apply()
{
	// Update environment with the user choice.
	bool use_region_settings	= mRegionSettingsRadioGroup->getSelectedIndex() == 0;
	bool use_fixed_sky			= mDayCycleSettingsRadioGroup->getSelectedIndex() == 0;
	std::string water_preset	= mWaterPresetCombo->getValue().asString();
	std::string sky_preset		= mSkyPresetCombo->getValue().asString();
	std::string day_cycle		= mDayCyclePresetCombo->getValue().asString();

	LLEnvManagerNew& env_mgr = LLEnvManagerNew::instance();
	if (use_region_settings)
	{
		env_mgr.useRegionSettings();
	}
	else
	{
		if (use_fixed_sky)
		{
			env_mgr.useSkyPreset(sky_preset);
		}
		else
		{
			env_mgr.useDayCycle(day_cycle, LLEnvKey::SCOPE_LOCAL);
		}

		env_mgr.useWaterPreset(water_preset);
	}
}

void LLFloaterEnvironmentSettings::cancel()
{
	// Revert environment to user preferences.
	LLEnvManagerNew::instance().usePrefs();
}

void LLFloaterEnvironmentSettings::populateWaterPresetsList()
{
	mWaterPresetCombo->removeall();

	std::list<std::string> user_presets, system_presets;
	LLWaterParamManager::instance().getPresetNames(user_presets, system_presets);

	// Add user presets first.
	for (std::list<std::string>::const_iterator it = user_presets.begin(); it != user_presets.end(); ++it)
	{
		mWaterPresetCombo->add(*it);
	}

	if (user_presets.size() > 0)
	{
		mWaterPresetCombo->addSeparator();
	}

	// Add system presets.
	for (std::list<std::string>::const_iterator it = system_presets.begin(); it != system_presets.end(); ++it)
	{
		mWaterPresetCombo->add(*it);
	}
}

void LLFloaterEnvironmentSettings::populateSkyPresetsList()
{
	mSkyPresetCombo->removeall();

	LLWLParamManager::preset_name_list_t region_presets; // unused as we don't list region presets here
	LLWLParamManager::preset_name_list_t user_presets, sys_presets;
	LLWLParamManager::instance().getPresetNames(region_presets, user_presets, sys_presets);

	// Add user presets.
	for (LLWLParamManager::preset_name_list_t::const_iterator it = user_presets.begin(); it != user_presets.end(); ++it)
	{
		mSkyPresetCombo->add(*it);
	}

	if (!user_presets.empty())
	{
		mSkyPresetCombo->addSeparator();
	}

	// Add system presets.
	for (LLWLParamManager::preset_name_list_t::const_iterator it = sys_presets.begin(); it != sys_presets.end(); ++it)
	{
		mSkyPresetCombo->add(*it);
	}
}

void LLFloaterEnvironmentSettings::populateDayCyclePresetsList()
{
	mDayCyclePresetCombo->removeall();

	LLDayCycleManager::preset_name_list_t user_days, sys_days;
	LLDayCycleManager::instance().getPresetNames(user_days, sys_days);

	// Add user days.
	for (LLDayCycleManager::preset_name_list_t::const_iterator it = user_days.begin(); it != user_days.end(); ++it)
	{
		mDayCyclePresetCombo->add(*it);
	}

	if (user_days.size() > 0)
	{
		mDayCyclePresetCombo->addSeparator();
	}

	// Add system days.
	for (LLDayCycleManager::preset_name_list_t::const_iterator it = sys_days.begin(); it != sys_days.end(); ++it)
	{
		mDayCyclePresetCombo->add(*it);
	}
}
