/**
 * @file llenvmanager.cpp
 * @brief Implementation of classes managing WindLight and water settings.
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

#include "llenvmanager.h"

#include "llagent.h"
#include "lldaycyclemanager.h"
#include "llviewercontrol.h" // for gSavedSettings
#include "llviewerregion.h"
#include "llwaterparammanager.h"
#include "llwlhandlers.h"
#include "llwlparammanager.h"
#include "lltrans.h"

std::string LLWLParamKey::toString() const
{
	switch (scope)
	{
	case SCOPE_LOCAL:
		return name + std::string(" (") + LLTrans::getString("Local") + std::string(")");
		break;
	case SCOPE_REGION:
		return name + std::string(" (") + LLTrans::getString("Region") + std::string(")");
		break;
	default:
		return name + " (?)";
	}
}

std::string LLEnvPrefs::getWaterPresetName() const
{
	if (mWaterPresetName.empty())
	{
		LL_WARNS() << "Water preset name is empty" << LL_ENDL;
	}

	return mWaterPresetName;
}

std::string LLEnvPrefs::getSkyPresetName() const
{
	if (mSkyPresetName.empty())
	{
		LL_WARNS() << "Sky preset name is empty" << LL_ENDL;
	}

	return mSkyPresetName;
}

std::string LLEnvPrefs::getDayCycleName() const
{
	if (mDayCycleName.empty())
	{
		LL_WARNS() << "Day cycle name is empty" << LL_ENDL;
	}

	return mDayCycleName;
}

void LLEnvPrefs::setUseRegionSettings(bool val)
{
	mUseRegionSettings = val;
}

void LLEnvPrefs::setUseWaterPreset(const std::string& name)
{
	mUseRegionSettings = false;
	mWaterPresetName = name;
}

void LLEnvPrefs::setUseSkyPreset(const std::string& name)
{
	mUseRegionSettings = false;
	mUseDayCycle = false;
	mSkyPresetName = name;
}

void LLEnvPrefs::setUseDayCycle(const std::string& name)
{
	mUseRegionSettings = false;
	mUseDayCycle = true;
	mDayCycleName = name;
}

//=============================================================================
LLEnvManagerNew::LLEnvManagerNew():
	mInterpNextChangeMessage(true),
	mCurRegionUUID(LLUUID::null),
	mLastReceivedID(LLUUID::null)
{

	// Set default environment settings.
	mUserPrefs.mUseRegionSettings = true;
	mUserPrefs.mUseDayCycle = true;
	mUserPrefs.mWaterPresetName = "Default";
	mUserPrefs.mSkyPresetName = "Default";
	mUserPrefs.mDayCycleName = "Default";

	LL_DEBUGS("Windlight")<<LL_ENDL;
	gAgent.addRegionChangedCallback(boost::bind(&LLEnvManagerNew::onRegionChange, this));
}

bool LLEnvManagerNew::getUseRegionSettings() const
{
	return mUserPrefs.getUseRegionSettings();
}

bool LLEnvManagerNew::getUseDayCycle() const
{
	return mUserPrefs.getUseDayCycle();
}

bool LLEnvManagerNew::getUseFixedSky() const
{
	return mUserPrefs.getUseFixedSky();
}

std::string LLEnvManagerNew::getWaterPresetName() const
{
	return mUserPrefs.getWaterPresetName();
}

std::string LLEnvManagerNew::getSkyPresetName() const
{
	return mUserPrefs.getSkyPresetName();
}

std::string LLEnvManagerNew::getDayCycleName() const
{
	return mUserPrefs.getDayCycleName();
}

const LLEnvironmentSettings& LLEnvManagerNew::getRegionSettings() const
{
	return !mNewRegionPrefs.isEmpty() ? mNewRegionPrefs : mCachedRegionPrefs;
}

void LLEnvManagerNew::setRegionSettings(const LLEnvironmentSettings& new_settings)
{
	// Set region settings override that will be used locally
	// until user either uploads the changes or goes to another region.
	mNewRegionPrefs = new_settings;
}

bool LLEnvManagerNew::usePrefs()
{
	LL_DEBUGS("Windlight") << "Displaying preferred environment" << LL_ENDL;
	updateManagersFromPrefs(false);
	return true;
}

bool LLEnvManagerNew::useDefaults()
{
	bool rslt;

	rslt  = useDefaultWater();
	rslt &= useDefaultSky();

	return rslt;
}

bool LLEnvManagerNew::useRegionSettings()
{
	bool rslt;

	rslt  = useRegionSky();
	rslt &= useRegionWater();

	return rslt;
}

bool LLEnvManagerNew::useWaterPreset(const std::string& name)
{
	LL_DEBUGS("Windlight") << "Displaying water preset " << name << LL_ENDL;
	LLWaterParamManager& water_mgr = LLWaterParamManager::instance();
	bool rslt = water_mgr.getParamSet(name, water_mgr.mCurParams);
	llassert(rslt == true);
	return rslt;
}

bool LLEnvManagerNew::useWaterParams(const LLSD& params)
{
	LL_DEBUGS("Windlight") << "Displaying water params" << LL_ENDL;
	LLWaterParamManager::instance().mCurParams.setAll(params);
	return true;
}

bool LLEnvManagerNew::useSkyPreset(const std::string& name)
{
	LLWLParamManager& sky_mgr = LLWLParamManager::instance();
	LLWLParamSet param_set;

	if (!sky_mgr.getParamSet(LLWLParamKey(name, LLEnvKey::SCOPE_LOCAL), param_set))
	{
		LL_WARNS() << "No sky preset named " << name << LL_ENDL;
		return false;
	}

	LL_DEBUGS("Windlight") << "Displaying sky preset " << name << LL_ENDL;
	sky_mgr.applySkyParams(param_set.getAll());
	return true;
}

bool LLEnvManagerNew::useSkyParams(const LLSD& params)
{
	LL_DEBUGS("Windlight") << "Displaying sky params" << LL_ENDL;
	LLWLParamManager::instance().applySkyParams(params);
	return true;
}

bool LLEnvManagerNew::useDayCycle(const std::string& name, LLEnvKey::EScope scope)
{
	LLSD params;

	if (scope == LLEnvKey::SCOPE_REGION)
	{
		LL_DEBUGS("Windlight") << "Displaying region day cycle " << name << LL_ENDL;
		params = getRegionSettings().getWLDayCycle();
	}
	else
	{
		LL_DEBUGS("Windlight") << "Displaying local day cycle " << name << LL_ENDL;

		if (!LLDayCycleManager::instance().getPreset(name, params))
		{
			LL_WARNS() << "No day cycle named " << name << LL_ENDL;
			return false;
		}
	}

	bool rslt = LLWLParamManager::instance().applyDayCycleParams(params, scope);
	llassert(rslt == true);
	return rslt;
}

bool LLEnvManagerNew::useDayCycleParams(const LLSD& params, LLEnvKey::EScope scope, F32 time /* = 0.5*/)
{
	LL_DEBUGS("Windlight") << "Displaying day cycle params" << LL_ENDL;
	return LLWLParamManager::instance().applyDayCycleParams(params, scope);
}

void LLEnvManagerNew::setUseRegionSettings(bool val)
{
	mUserPrefs.setUseRegionSettings(val);
	saveUserPrefs();
	updateManagersFromPrefs(false);
}

void LLEnvManagerNew::setUseWaterPreset(const std::string& name)
{
	// *TODO: make sure the preset exists.
	if (name.empty())
	{
		LL_WARNS() << "Empty water preset name passed" << LL_ENDL;
		return;
	}

	mUserPrefs.setUseWaterPreset(name);
	saveUserPrefs();
	updateManagersFromPrefs(false);
}

void LLEnvManagerNew::setUseSkyPreset(const std::string& name)
{
	// *TODO: make sure the preset exists.
	if (name.empty())
	{
		LL_WARNS() << "Empty sky preset name passed" << LL_ENDL;
		return;
	}

	mUserPrefs.setUseSkyPreset(name);
	saveUserPrefs();
	updateManagersFromPrefs(false);
}

void LLEnvManagerNew::setUseDayCycle(const std::string& name)
{
	if (!LLDayCycleManager::instance().presetExists(name))
	{
		LL_WARNS() << "Invalid day cycle name passed" << LL_ENDL;
		return;
	}

	mUserPrefs.setUseDayCycle(name);
	saveUserPrefs();
	updateManagersFromPrefs(false);
}

void LLEnvManagerNew::loadUserPrefs()
{
	// operate on members directly to avoid side effects
	mUserPrefs.mWaterPresetName	= gSavedSettings.getString("WaterPresetName");
	mUserPrefs.mSkyPresetName	= gSavedSettings.getString("SkyPresetName");
	mUserPrefs.mDayCycleName	= gSavedSettings.getString("DayCycleName");

	bool use_region_settings = gSavedSettings.getBOOL("EnvironmentPersistAcrossLogin") ? gSavedSettings.getBOOL("UseEnvironmentFromRegion") : true;
	mUserPrefs.mUseRegionSettings	= use_region_settings;
	mUserPrefs.mUseDayCycle			= gSavedSettings.getBOOL("UseDayCycle");

	if (mUserPrefs.mUseRegionSettings)
	{
		requestRegionSettings();
	}
}

void LLEnvManagerNew::saveUserPrefs()
{
	gSavedSettings.setString("WaterPresetName",			getWaterPresetName());
	gSavedSettings.setString("SkyPresetName",			getSkyPresetName());
	gSavedSettings.setString("DayCycleName",			getDayCycleName());

	gSavedSettings.setBOOL("UseEnvironmentFromRegion",	getUseRegionSettings());
	gSavedSettings.setBOOL("UseDayCycle",				getUseDayCycle());

	mUsePrefsChangeSignal();
}

void LLEnvManagerNew::setUserPrefs(
	const std::string& water_preset,
	const std::string& sky_preset,
	const std::string& day_cycle_preset,
	bool use_fixed_sky,
	bool use_region_settings)
{
	// operate on members directly to avoid side effects
	mUserPrefs.mWaterPresetName	= water_preset;
	mUserPrefs.mSkyPresetName	= sky_preset;
	mUserPrefs.mDayCycleName	= day_cycle_preset;

	mUserPrefs.mUseRegionSettings	= use_region_settings;
	mUserPrefs.mUseDayCycle			= !use_fixed_sky;

	saveUserPrefs();
	updateManagersFromPrefs(false);
}

void LLEnvManagerNew::dumpUserPrefs()
{
	LL_DEBUGS("Windlight") << "WaterPresetName: "	<< gSavedSettings.getString("WaterPresetName") << LL_ENDL;
	LL_DEBUGS("Windlight") << "SkyPresetName: "		<< gSavedSettings.getString("SkyPresetName") << LL_ENDL;
	LL_DEBUGS("Windlight") << "DayCycleName: "		<< gSavedSettings.getString("DayCycleName") << LL_ENDL;

	LL_DEBUGS("Windlight") << "UseEnvironmentFromRegion: "	<< gSavedSettings.getBOOL("UseEnvironmentFromRegion") << LL_ENDL;
	LL_DEBUGS("Windlight") << "UseDayCycle: "				<< gSavedSettings.getBOOL("UseDayCycle") << LL_ENDL;
}

void LLEnvManagerNew::dumpPresets()
{
	const LLEnvironmentSettings& region_settings = getRegionSettings();
	std::string region_name = gAgent.getRegion() ? gAgent.getRegion()->getName() : "Unknown region";

	// Dump water presets.
	LL_DEBUGS("Windlight") << "Waters:" << LL_ENDL;
	if (region_settings.getWaterParams().size() != 0)
	{
		LL_DEBUGS("Windlight") << " - " << region_name << LL_ENDL;
	}
	LLWaterParamManager::preset_name_list_t water_presets;
	LLWaterParamManager::instance().getPresetNames(water_presets);
	for (LLWaterParamManager::preset_name_list_t::const_iterator it = water_presets.begin(); it != water_presets.end(); ++it)
	{
		LL_DEBUGS("Windlight") << " - " << *it << LL_ENDL;
	}

	// Dump sky presets.
	LL_DEBUGS("Windlight") << "Skies:" << LL_ENDL;
	LLWLParamManager::preset_key_list_t sky_preset_keys;
	LLWLParamManager::instance().getPresetKeys(sky_preset_keys);
	for (LLWLParamManager::preset_key_list_t::const_iterator it = sky_preset_keys.begin(); it != sky_preset_keys.end(); ++it)
	{
		std::string preset_name = it->name;
		std::string item_title;

		if (it->scope == LLEnvKey::SCOPE_LOCAL) // local preset
		{
			item_title = preset_name;
		}
		else // region preset
		{
			item_title = preset_name + " (" + region_name + ")";
		}
		LL_DEBUGS("Windlight") << " - " << item_title << LL_ENDL;
	}

	// Dump day cycles.
	LL_DEBUGS("Windlight") << "Days:" << LL_ENDL;
	const LLSD& cur_region_dc = region_settings.getWLDayCycle();
	if (cur_region_dc.size() != 0)
	{
		LL_DEBUGS("Windlight") << " - " << region_name << LL_ENDL;
	}
	LLDayCycleManager::preset_name_list_t days;
	LLDayCycleManager::instance().getPresetNames(days);
	for (LLDayCycleManager::preset_name_list_t::const_iterator it = days.begin(); it != days.end(); ++it)
	{
		LL_DEBUGS("Windlight") << " - " << *it << LL_ENDL;
	}
}

void LLEnvManagerNew::requestRegionSettings()
{
	LL_DEBUGS("Windlight") << LL_ENDL;
	LLEnvironmentRequest::initiate();
}

bool LLEnvManagerNew::sendRegionSettings(const LLEnvironmentSettings& new_settings)
{
	LLSD metadata;

	metadata["regionID"] = gAgent.getRegion()->getRegionID();
	// add last received update ID to outbound message so simulator can handle concurrent updates
	metadata["messageID"] = mLastReceivedID;

	return LLEnvironmentApply::initiateRequest(new_settings.makePacket(metadata));
}

boost::signals2::connection LLEnvManagerNew::setPreferencesChangeCallback(const prefs_change_signal_t::slot_type& cb)
{
	return mUsePrefsChangeSignal.connect(cb);
}

boost::signals2::connection LLEnvManagerNew::setRegionSettingsChangeCallback(const region_settings_change_signal_t::slot_type& cb)
{
	return mRegionSettingsChangeSignal.connect(cb);
}

boost::signals2::connection LLEnvManagerNew::setRegionSettingsAppliedCallback(const region_settings_applied_signal_t::slot_type& cb)
{
	return mRegionSettingsAppliedSignal.connect(cb);
}

// static
bool LLEnvManagerNew::canEditRegionSettings()
{
	LLViewerRegion* region = gAgent.getRegion();
	BOOL owner_or_god = gAgent.isGodlike() || (region && region->getOwner() == gAgent.getID());
	BOOL owner_or_god_or_manager = owner_or_god || (region && region->isEstateManager());

	LL_DEBUGS("Windlight") << "Can edit region settings: " << (bool) owner_or_god_or_manager << LL_ENDL;
	return owner_or_god_or_manager;
}

// static
const std::string LLEnvManagerNew::getScopeString(LLEnvKey::EScope scope)
{
	switch(scope)
	{
		case LLEnvKey::SCOPE_LOCAL:
			return LLTrans::getString("LocalSettings");
		case LLEnvKey::SCOPE_REGION:
			return LLTrans::getString("RegionSettings");
		default:
			return " (?)";
	}
}

void LLEnvManagerNew::onRegionSettingsResponse(const LLSD& content)
{
	// If the message was valid, grab the UUID from it and save it for next outbound update message.
	mLastReceivedID = content[0]["messageID"].asUUID();

	// Refresh cached region settings.
	LL_DEBUGS("Windlight") << "Received region environment settings: " << content << LL_ENDL;
	F32 sun_hour = 0; // *TODO
	LLEnvironmentSettings new_settings(content[1], content[2], content[3], sun_hour);
	mCachedRegionPrefs = new_settings;

	// Load region sky presets.
	LLWLParamManager::instance().refreshRegionPresets();

	// If using server settings, update managers.
	if (getUseRegionSettings())
	{
		updateManagersFromPrefs(mInterpNextChangeMessage);
	}

	// Let interested parties know about the region settings update.
	mRegionSettingsChangeSignal();

	// reset
	mInterpNextChangeMessage = false;
}

void LLEnvManagerNew::onRegionSettingsApplyResponse(bool ok)
{
	LL_DEBUGS("Windlight") << "Applying region settings " << (ok ? "succeeded" : "failed") << LL_ENDL;

	// Clear locally modified region settings because they have just been uploaded.
	mNewRegionPrefs.clear();

	mRegionSettingsAppliedSignal(ok);
}

//-- private methods ----------------------------------------------------------

// virtual
void LLEnvManagerNew::initSingleton()
{
	LL_DEBUGS("Windlight") << "Initializing LLEnvManagerNew" << LL_ENDL;

	loadUserPrefs();
}

void LLEnvManagerNew::updateSkyFromPrefs()
{
	bool success = true;

	// Sync sky with user prefs.
	if (getUseRegionSettings()) // apply region-wide settings
	{
		success = useRegionSky();
	}
	else // apply user-specified settings
	{
		if (getUseDayCycle())
		{
			success = useDayCycle(getDayCycleName(), LLEnvKey::SCOPE_LOCAL);
		}
		else
		{
			success = useSkyPreset(getSkyPresetName());
		}
	}

	// If something went wrong, fall back to defaults.
	if (!success)
	{
		// *TODO: fix user prefs
		useDefaultSky();
	}
}

void LLEnvManagerNew::updateWaterFromPrefs(bool interpolate)
{
	LLWaterParamManager& water_mgr = LLWaterParamManager::instance();
	LLSD target_water_params;

	// Determine new water settings based on user prefs.

	{
		// Fall back to default water.
		LLWaterParamSet default_water;
		water_mgr.getParamSet("Default", default_water);
		target_water_params = default_water.getAll();
	}

	if (getUseRegionSettings())
	{
		// *TODO: make sure whether region settings belong to the current region?
		const LLSD& region_water_params = getRegionSettings().getWaterParams();
		if (region_water_params.size() != 0) // region has no water settings
		{
			LL_DEBUGS("Windlight") << "Applying region water" << LL_ENDL;
			target_water_params = region_water_params;
		}
		else
		{
			LL_DEBUGS("Windlight") << "Applying default water" << LL_ENDL;
		}
	}
	else
	{
		std::string water = getWaterPresetName();
		LL_DEBUGS("Windlight") << "Applying water preset [" << water << "]" << LL_ENDL;
		LLWaterParamSet params;
		if (!water_mgr.getParamSet(water, params))
		{
			LL_WARNS() << "No water preset named " << water << ", falling back to defaults" << LL_ENDL;
			water_mgr.getParamSet("Default", params);

			// *TODO: Fix user preferences accordingly.
		}
		target_water_params = params.getAll();
	}

	// Sync water with user prefs.
	water_mgr.applyParams(target_water_params, interpolate);
}

void LLEnvManagerNew::updateManagersFromPrefs(bool interpolate)
{
	LL_DEBUGS("Windlight")<<LL_ENDL;
	// Apply water settings.
	updateWaterFromPrefs(interpolate);

	// Apply sky settings.
	updateSkyFromPrefs();
}

bool LLEnvManagerNew::useRegionSky()
{
	const LLEnvironmentSettings& region_settings = getRegionSettings();

	// If region is set to defaults,
	if (region_settings.getSkyMap().size() == 0)
	{
		// well... apply the default sky settings.
		useDefaultSky();
		return true;
	}

	// Otherwise apply region day cycle/skies.
	LL_DEBUGS("Windlight") << "Applying region sky" << LL_ENDL;

	// *TODO: Support fixed sky from region. Just do sky reset for now.
	if (region_settings.getSkyMap().size() == 1)
	{
		// Region is set to fixed sky. Reset.
		useSkyParams(region_settings.getSkyMap().beginMap()->second);
	}
	return useDayCycleParams(
		region_settings.getWLDayCycle(),
		LLEnvKey::SCOPE_REGION,
		region_settings.getDayTime());
}

bool LLEnvManagerNew::useRegionWater()
{
	const LLEnvironmentSettings& region_settings = getRegionSettings();
	const LLSD& region_water = region_settings.getWaterParams();

	// If region is set to defaults,
	if (region_water.size() == 0)
	{
		// well... apply the default water settings.
		return useDefaultWater();
	}

	// Otherwise apply region water.
	LL_DEBUGS("Windlight") << "Applying region sky" << LL_ENDL;
	return useWaterParams(region_water);
}

bool LLEnvManagerNew::useDefaultSky()
{
	return useDayCycle("Default", LLEnvKey::SCOPE_LOCAL);
}

bool LLEnvManagerNew::useDefaultWater()
{
	return useWaterPreset("Default");
}


void LLEnvManagerNew::onRegionChange()
{
	// Avoid duplicating region setting requests
	// by checking whether the region is actually changing.
	LLViewerRegion* regionp = gAgent.getRegion();
	LLUUID region_uuid = regionp ? regionp->getRegionID() : LLUUID::null;
	if (region_uuid != mCurRegionUUID)
	{
	// Clear locally modified region settings.
	mNewRegionPrefs.clear();

	// *TODO: clear environment settings of the previous region?

	// Request environment settings of the new region.
	mCurRegionUUID = region_uuid;
		// for region crossings, interpolate the change; for teleports, don't
		mInterpNextChangeMessage = (gAgent.getTeleportState() == LLAgent::TELEPORT_NONE);
		LL_DEBUGS("Windlight") << (mInterpNextChangeMessage ? "Crossed" : "Teleported")
							   << " to new region: " << region_uuid
							   << LL_ENDL;
	requestRegionSettings();
	}
	else
	{
		LL_DEBUGS("Windlight") << "disregarding region change; interp: "
							   << (mInterpNextChangeMessage ? "true" : "false")
							   << " regionp: " << regionp
							   << " old: " << mCurRegionUUID
							   << " new: " << region_uuid
							   << LL_ENDL;
	}
}
