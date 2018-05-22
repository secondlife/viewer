/** 
 * @file llpanelenvironment.cpp
 * @brief LLPanelExperiences class implementation
 *
 * $LicenseInfo:firstyear=2013&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2013, Linden Research, Inc.
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


#include "llpanelprofile.h"
#include "lluictrlfactory.h"
#include "llexperiencecache.h"
#include "llagent.h"

#include "llviewerregion.h"
#include "llpanelenvironment.h"
#include "llslurl.h"
#include "lllayoutstack.h"

#include "llfloater.h"
#include "llfloaterreg.h"
#include "llfloatereditextdaycycle.h"

//static LLPanelInjector<LLPanelEnvironmentInfo> register_environment_panel("environment_panel");

LLPanelEnvironmentInfo::LLPanelEnvironmentInfo(): 
    mEnableEditing(false),
    mRegionSettingsRadioGroup(NULL),
    mDayLengthSlider(NULL),
    mDayOffsetSlider(NULL),
    mAllowOverRide(NULL)
{
}

// virtual
BOOL LLPanelEnvironmentInfo::postBuild()
{
    mRegionSettingsRadioGroup = getChild<LLRadioGroup>("environment_select_radio_group");
    mRegionSettingsRadioGroup->setCommitCallback(boost::bind(&LLPanelEnvironmentInfo::onSwitchDefaultSelection, this));

    mDayLengthSlider = getChild<LLSliderCtrl>("day_length_sld");
    mDayOffsetSlider = getChild<LLSliderCtrl>("day_offset_sld");
    mAllowOverRide = getChild<LLCheckBoxCtrl>("allow_override_chk");

    childSetCommitCallback("edit_btn", boost::bind(&LLPanelEnvironmentInfo::onBtnEdit, this), NULL);
    childSetCommitCallback("apply_btn", boost::bind(&LLPanelEnvironmentInfo::onBtnApply, this), NULL);
    childSetCommitCallback("cancel_btn", boost::bind(&LLPanelEnvironmentInfo::onBtnCancel, this), NULL);

    return TRUE;
}

// virtual
void LLPanelEnvironmentInfo::onOpen(const LLSD& key)
{
    LL_DEBUGS("Windlight") << "Panel opened, refreshing" << LL_ENDL;
    refresh();
}

// virtual
void LLPanelEnvironmentInfo::onVisibilityChange(BOOL new_visibility)
{
    // If hiding (user switched to another tab or closed the floater),
    // display user's preferred environment.
    // switching back and forth between agent's environment and the one being edited. 
    // 
}

void LLPanelEnvironmentInfo::refresh()
{
#if 0
    if (gDisconnected)
    {
        return;
    }

    populateWaterPresetsList();
    populateSkyPresetsList();
    populateDayCyclesList();

    // Init radio groups.
    const LLEnvironmentSettings& settings = LLEnvManagerNew::instance().getRegionSettings();
    const LLSD& dc = settings.getWLDayCycle();
    LLSD::Real first_frame_time = dc.size() > 0 ? dc[0][0].asReal() : 0.0f;
    const bool use_fixed_sky = dc.size() == 1 && first_frame_time < 0;
    mRegionSettingsRadioGroup->setSelectedIndex(settings.getSkyMap().size() == 0 ? 0 : 1);
    mDayCycleSettingsRadioGroup->setSelectedIndex(use_fixed_sky ? 0 : 1);

    setControlsEnabled(mEnableEditing);

    setDirty(false);
#endif
}

void LLPanelEnvironmentInfo::setControlsEnabled(bool enabled)
{
    mRegionSettingsRadioGroup->setEnabled(enabled);

    mDayLengthSlider->setEnabled(false);
    mDayOffsetSlider->setEnabled(false);
    mAllowOverRide->setEnabled(enabled);

    getChildView("edit_btn")->setEnabled(false);

    getChildView("apply_btn")->setEnabled(enabled);
    getChildView("cancel_btn")->setEnabled(enabled);

    if (enabled)
    {
        // Enable/disable some controls based on currently selected radio buttons.
        bool use_defaults = mRegionSettingsRadioGroup->getSelectedIndex() == 0;
        getChild<LLView>("edit_btn")->setEnabled(!use_defaults);

        mDayLengthSlider->setEnabled(!use_defaults);
        mDayOffsetSlider->setEnabled(!use_defaults);

    }
}

void LLPanelEnvironmentInfo::setApplyProgress(bool started)
{
//     LLLoadingIndicator* indicator = getChild<LLLoadingIndicator>("progress_indicator");
// 
//     indicator->setVisible(started);
// 
//     if (started)
//     {
//         indicator->start();
//     }
//     else
//     {
//         indicator->stop();
//     }
}

void LLPanelEnvironmentInfo::setDirty(bool dirty)
{
    getChildView("apply_btn")->setEnabled(dirty);
    getChildView("cancel_btn")->setEnabled(dirty);
}

// void LLPanelEnvironmentInfo::sendRegionSunUpdate()
// {
// #if 0
//     LLRegionInfoModel& region_info = LLRegionInfoModel::instance();
// 
//     // If the region is being switched to fixed sky,
//     // change the region's sun hour according to the (fixed) sun position.
//     // This is needed for llGetSunDirection() LSL function to work properly (STORM-1330).
//     const LLSD& sky_map = mNewRegionSettings.getSkyMap();
//     bool region_use_fixed_sky = sky_map.size() == 1;
//     if (region_use_fixed_sky)
//     {
//         LLWLParamSet param_set;
//         llassert(sky_map.isMap());
//         param_set.setAll(sky_map.beginMap()->second);
//         F32 sun_angle = param_set.getSunAngle();
// 
//         LL_DEBUGS("Windlight Sync") << "Old sun hour: " << region_info.mSunHour << LL_ENDL;
//         // convert value range from 0..2pi to 6..30
//         region_info.mSunHour = fmodf((sun_angle / F_TWO_PI) * 24.f, 24.f) + 6.f;
//     }
// 
//     region_info.setUseFixedSun(region_use_fixed_sky);
//     region_info.mUseEstateSun = !region_use_fixed_sky;
//     LL_DEBUGS("Windlight Sync") << "Sun hour: " << region_info.mSunHour << LL_ENDL;
// 
//     region_info.sendRegionTerrain(LLFloaterRegionInfo::getLastInvoice());
// #endif
// }

// void LLPanelEnvironmentInfo::fixEstateSun()
// {
//     // We don't support fixed sun estates anymore and need to fix
//     // such estates for region day cycle to take effect.
//     // *NOTE: Assuming that current estate settings have arrived already.
//     LLEstateInfoModel& estate_info = LLEstateInfoModel::instance();
//     if (estate_info.getUseFixedSun())
//     {
//         LL_INFOS() << "Switching estate to global sun" << LL_ENDL;
//         estate_info.setUseFixedSun(false);
//         estate_info.sendEstateInfo();
//     }
// }

// void LLPanelEnvironmentInfo::populateWaterPresetsList()
// {
// #if 0
//     mWaterPresetCombo->removeall();
// 
//     // If the region already has water params, add them to the list.
//     const LLEnvironmentSettings& region_settings = LLEnvManagerNew::instance().getRegionSettings();
//     if (region_settings.getWaterParams().size() != 0)
//     {
//         const std::string& region_name = gAgent.getRegion()->getName();
//         mWaterPresetCombo->add(region_name, LLWLParamKey(region_name, LLEnvKey::SCOPE_REGION).toLLSD());
//         mWaterPresetCombo->addSeparator();
//     }
// 
//     std::list<std::string> user_presets, system_presets;
//     LLWaterParamManager::instance().getPresetNames(user_presets, system_presets);
// 
//     // Add local user presets first.
//     for (std::list<std::string>::const_iterator it = user_presets.begin(); it != user_presets.end(); ++it)
//     {
//         mWaterPresetCombo->add(*it, LLWLParamKey(*it, LLEnvKey::SCOPE_LOCAL).toLLSD());
//     }
// 
//     if (user_presets.size() > 0)
//     {
//         mWaterPresetCombo->addSeparator();
//     }
// 
//     // Add local system presets.
//     for (std::list<std::string>::const_iterator it = system_presets.begin(); it != system_presets.end(); ++it)
//     {
//         mWaterPresetCombo->add(*it, LLWLParamKey(*it, LLEnvKey::SCOPE_LOCAL).toLLSD());
//     }
// 
//     // There's no way to select current preset because its name is not stored on server.
// #endif
// }
// 
// void LLPanelEnvironmentInfo::populateSkyPresetsList()
// {
// #if 0
//     mSkyPresetCombo->removeall();
// 
//     LLWLParamManager::preset_name_list_t region_presets;
//     LLWLParamManager::preset_name_list_t user_presets, sys_presets;
//     LLWLParamManager::instance().getPresetNames(region_presets, user_presets, sys_presets);
// 
//     // Add region presets.
//     std::string region_name = gAgent.getRegion() ? gAgent.getRegion()->getName() : LLTrans::getString("Unknown");
//     for (LLWLParamManager::preset_name_list_t::const_iterator it = region_presets.begin(); it != region_presets.end(); ++it)
//     {
//         std::string preset_name = *it;
//         std::string item_title = preset_name + " (" + region_name + ")";
//         mSkyPresetCombo->add(item_title, LLWLParamKey(preset_name, LLEnvKey::SCOPE_REGION).toStringVal());
//     }
// 
//     if (!region_presets.empty())
//     {
//         mSkyPresetCombo->addSeparator();
//     }
// 
//     // Add user presets.
//     for (LLWLParamManager::preset_name_list_t::const_iterator it = user_presets.begin(); it != user_presets.end(); ++it)
//     {
//         mSkyPresetCombo->add(*it, LLWLParamKey(*it, LLEnvKey::SCOPE_LOCAL).toStringVal());
//     }
// 
//     if (!user_presets.empty())
//     {
//         mSkyPresetCombo->addSeparator();
//     }
// 
//     // Add system presets.
//     for (LLWLParamManager::preset_name_list_t::const_iterator it = sys_presets.begin(); it != sys_presets.end(); ++it)
//     {
//         mSkyPresetCombo->add(*it, LLWLParamKey(*it, LLEnvKey::SCOPE_LOCAL).toStringVal());
//     }
// 
//     // Select current preset.
//     LLSD sky_map = LLEnvManagerNew::instance().getRegionSettings().getSkyMap();
//     if (sky_map.size() == 1) // if the region is set to fixed sky
//     {
//         std::string preset_name = sky_map.beginMap()->first;
//         mSkyPresetCombo->selectByValue(LLWLParamKey(preset_name, LLEnvKey::SCOPE_REGION).toStringVal());
//     }
// #endif
// }
// 
// void LLPanelEnvironmentInfo::populateDayCyclesList()
// {
// #if 0
//     mDayCyclePresetCombo->removeall();
// 
//     // If the region already has env. settings, add its day cycle to the list.
//     const LLSD& cur_region_dc = LLEnvManagerNew::instance().getRegionSettings().getWLDayCycle();
//     if (cur_region_dc.size() != 0)
//     {
//         LLViewerRegion* region = gAgent.getRegion();
//         llassert(region != NULL);
// 
//         LLWLParamKey key(region->getName(), LLEnvKey::SCOPE_REGION);
//         mDayCyclePresetCombo->add(region->getName(), key.toStringVal());
//         mDayCyclePresetCombo->addSeparator();
//     }
// 
//     // Add local user day cycles.
//     LLDayCycleManager::preset_name_list_t user_days, sys_days;
//     LLDayCycleManager::instance().getPresetNames(user_days, sys_days);
//     for (LLDayCycleManager::preset_name_list_t::const_iterator it = user_days.begin(); it != user_days.end(); ++it)
//     {
//         mDayCyclePresetCombo->add(*it, LLWLParamKey(*it, LLEnvKey::SCOPE_LOCAL).toStringVal());
//     }
// 
//     if (user_days.size() > 0)
//     {
//         mDayCyclePresetCombo->addSeparator();
//     }
// 
//     // Add local system day cycles.
//     for (LLDayCycleManager::preset_name_list_t::const_iterator it = sys_days.begin(); it != sys_days.end(); ++it)
//     {
//         mDayCyclePresetCombo->add(*it, LLWLParamKey(*it, LLEnvKey::SCOPE_LOCAL).toStringVal());
//     }
// 
//     // Current day cycle is already selected.
// #endif
// }

void LLPanelEnvironmentInfo::onSwitchDefaultSelection()
{
    bool use_defaults = mRegionSettingsRadioGroup->getSelectedIndex() == 0;
    
    getChild<LLView>("edit_btn")->setEnabled(!use_defaults);

    mDayLengthSlider->setEnabled(!use_defaults);
    mDayOffsetSlider->setEnabled(!use_defaults);

    setDirty(true);
}


void LLPanelEnvironmentInfo::onBtnApply()
{
    doApply();
}

void LLPanelEnvironmentInfo::onBtnCancel()
{
    // Reload last saved region settings.
    refresh();
}

void LLPanelEnvironmentInfo::onBtnEdit()
{
    LLFloaterEditExtDayCycle *dayeditor = (LLFloaterEditExtDayCycle *)LLFloaterReg::getInstance("env_edit_extdaycycle");

    if (dayeditor)
    {
        dayeditor->setEditCommitSignal(boost::bind(&LLPanelEnvironmentInfo::onEditiCommited, this, _1));
        dayeditor->openFloater(mEditingDayCycle, F32Hours(mDayLengthSlider->getValue().asReal()), F32Hours(mDayOffsetSlider->getValue().asReal()));
    }
}

void LLPanelEnvironmentInfo::onEditiCommited(LLSettingsDay::ptr_t newday)
{
    doEditCommited(newday);
}

void LLPanelEnvironmentInfo::doEditCommited(LLSettingsDay::ptr_t &newday)
{
    mEditingDayCycle = newday;
    /*TODO pure virtual*/
}

// void LLPanelEnvironmentInfo::onRegionSettingschange()
// {
//     LL_DEBUGS("Windlight") << "Region settings changed, refreshing" << LL_ENDL;
//     refresh();
// 
//     // Stop applying progress indicator (it may be running if it's us who initiated settings update).
//     setApplyProgress(false);
// }
// 
// void LLPanelEnvironmentInfo::onRegionSettingsApplied(bool ok)
// {
//     // If applying new settings has failed, stop the indicator right away.
//     // Otherwise it will be stopped when we receive the updated settings from server.
//     if (ok)
//     {
//         // Set the region sun phase/flags according to the chosen new preferences.
//         //
//         // If we do this earlier we may get jerky transition from fixed sky to a day cycle (STORM-1481).
//         // That is caused by the simulator re-sending the region info, which in turn makes us
//         // re-request and display old region environment settings while the new ones haven't been applied yet.
//         sendRegionSunUpdate();
// 
//         // Switch estate to not using fixed sun for the region day cycle to work properly (STORM-1506).
//         fixEstateSun();
//     }
//     else
//     {
//         setApplyProgress(false);
// 
//         // We need to re-request environment setting here,
//         // otherwise our subsequent attempts to change region settings will fail with the following error:
//         // "Unable to update environment settings because the last update your viewer saw was not the same
//         // as the last update sent from the simulator.  Try sending your update again, and if this
//         // does not work, try leaving and returning to the region."
//         //		LLEnvManagerNew::instance().requestRegionSettings();
//     }
// }


