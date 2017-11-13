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

#include "llenvironment.h"


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
	childSetCommitCallback("cancel_btn", boost::bind(&LLFloaterEnvironmentSettings::onBtnCancel, this), NULL);

	setCloseCallback(boost::bind(&LLFloaterEnvironmentSettings::cancel, this));

    LLEnvironment::instance().setSkyListChange(boost::bind(&LLFloaterEnvironmentSettings::populateSkyPresetsList, this));
    LLEnvironment::instance().setWaterListChange(boost::bind(&LLFloaterEnvironmentSettings::populateWaterPresetsList, this));
    LLEnvironment::instance().setDayCycleListChange(boost::bind(&LLFloaterEnvironmentSettings::populateDayCyclePresetsList, this));

	return TRUE;
}

// virtual
void LLFloaterEnvironmentSettings::onOpen(const LLSD& key)
{
	refresh();
}

//virtual
void LLFloaterEnvironmentSettings::onClose(bool app_quitting)
{
    if (!app_quitting)
        LLEnvironment::instance().applyChosenEnvironment();
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
    bool use_region_settings	= mRegionSettingsRadioGroup->getSelectedIndex() == 0;

    if (use_region_settings)
    {
        LLEnvironment::instance().clearUserSettings();
    }
    else
    {
        LLEnvironment::instance().clearUserSettings();

        bool use_fixed_sky = mDayCycleSettingsRadioGroup->getSelectedIndex() == 0;

        if (!use_fixed_sky)
        {
            std::string day_cycle = mDayCyclePresetCombo->getValue().asString();
            LLSettingsDay::ptr_t day = LLEnvironment::instance().findDayCycleByName(day_cycle);
            if (day)
            {
                LLEnvironment::instance().setDayFor(LLEnvironment::ENV_LOCAL, day);
            }
        }
        else
        {
            std::string water_preset = mWaterPresetCombo->getValue().asString();
            std::string sky_preset = mSkyPresetCombo->getValue().asString();
            
            LLSettingsSky::ptr_t sky = LLEnvironment::instance().findSkyByName(sky_preset);
            LLSettingsWater::ptr_t water = LLEnvironment::instance().findWaterByName(water_preset);

            LLEnvironment::instance().setSkyFor(LLEnvironment::ENV_LOCAL, sky);
            LLEnvironment::instance().setWaterFor(LLEnvironment::ENV_LOCAL, water);
        }
    }

#if 0
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
#endif
    closeFloater();
}

void LLFloaterEnvironmentSettings::onBtnCancel()
{
	closeFloater();
}

void LLFloaterEnvironmentSettings::refresh()
{
    LLSettingsDay::ptr_t day = LLEnvironment::instance().getChosenDay();
    LLSettingsSky::ptr_t sky = LLEnvironment::instance().getChosenSky();
    LLSettingsWater::ptr_t water = LLEnvironment::instance().getChosenWater();

    
    bool use_region_settings = true;
	bool use_fixed_sky		   = !day;

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
    if (water)
        mWaterPresetCombo->selectByValue(water->getName());
    else
        mWaterPresetCombo->selectByValue(LLSD());
    if (sky)
        mSkyPresetCombo->selectByValue(sky->getName());
    else
        mSkyPresetCombo->selectByValue(LLSD());
    if (day)
        mDayCyclePresetCombo->selectByValue(day->getName());
    else
        mDayCyclePresetCombo->selectByValue(LLSD());
}

void LLFloaterEnvironmentSettings::apply()
{
	// Update environment with the user choice.
	bool use_region_settings	= mRegionSettingsRadioGroup->getSelectedIndex() == 0;
	bool use_fixed_sky			= mDayCycleSettingsRadioGroup->getSelectedIndex() == 0;
    std::string water_preset   = mWaterPresetCombo->getValue().asString();
    std::string sky_preset     = mSkyPresetCombo->getValue().asString();
	std::string day_cycle		= mDayCyclePresetCombo->getValue().asString();

	if (use_region_settings)
	{
		//env_mgr.useRegionSettings();
	}
	else
	{
		if (use_fixed_sky)
		{
            LLSettingsSky::ptr_t psky = LLEnvironment::instance().findSkyByName(sky_preset);
            if (psky)
                LLEnvironment::instance().selectSky(psky, LLEnvironment::TRANSITION_FAST);
		}
		else
		{
            LLSettingsDay::ptr_t pday = LLEnvironment::instance().findDayCycleByName(day_cycle);
            if (pday)
                LLEnvironment::instance().selectDayCycle(pday, LLEnvironment::TRANSITION_FAST);
		}

        LLSettingsWater::ptr_t pwater = LLEnvironment::instance().findWaterByName(water_preset);
        if (pwater)
            LLEnvironment::instance().selectWater(pwater, LLEnvironment::TRANSITION_FAST);
	}
}

void LLFloaterEnvironmentSettings::cancel()
{
	// Revert environment to user preferences.
//	LLEnvManagerNew::instance().usePrefs();
}

void LLFloaterEnvironmentSettings::populateWaterPresetsList()
{
    mWaterPresetCombo->removeall();

    LLEnvironment::list_name_id_t list = LLEnvironment::instance().getWaterList();

    for (LLEnvironment::list_name_id_t::iterator it = list.begin(); it != list.end(); ++it)
    {
        mWaterPresetCombo->add((*it).first, LLSD::String((*it).first)); // 
    }
}

void LLFloaterEnvironmentSettings::populateSkyPresetsList()
{
	mSkyPresetCombo->removeall();

    LLEnvironment::list_name_id_t list = LLEnvironment::instance().getSkyList();

    for (LLEnvironment::list_name_id_t::iterator it = list.begin(); it != list.end(); ++it)
    {
        mSkyPresetCombo->add((*it).first, LLSD::String((*it).first));
    }
}

void LLFloaterEnvironmentSettings::populateDayCyclePresetsList()
{
	mDayCyclePresetCombo->removeall();

    LLEnvironment::list_name_id_t list = LLEnvironment::instance().getDayCycleList();

    for (LLEnvironment::list_name_id_t::iterator it = list.begin(); it != list.end(); ++it)
    {
        mDayCyclePresetCombo->add((*it).first, LLSD::String((*it).first));
    }
}
