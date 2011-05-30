/**
 * @file llenvmanager.cpp
 * @brief Implementation of classes managing WindLight and water settings.
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * 
 * Copyright (c) 2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"

#include "llenvmanager.h"

#include "llagent.h"
#include "llviewerregion.h"

#include "lldaycyclemanager.h"
#include "llfloaterreg.h"
#include "llfloaterwindlight.h"
#include "llfloaterwater.h"
#include "llfloaterenvsettings.h"
#include "llwlparammanager.h"
#include "llwaterparammanager.h"
#include "llfloaterregioninfo.h"
//#include "llwindlightscrubbers.h" // *HACK commented out since this code isn't released (yet)
#include "llwlhandlers.h"
#include "llnotifications.h"

extern LLControlGroup gSavedSettings;

/*virtual*/ void LLEnvManager::initSingleton()
{
	LL_DEBUGS("Windlight") << "Initializing LLEnvManager" << LL_ENDL;

	mOrigSettingStore[LLEnvKey::SCOPE_LOCAL] = lindenDefaults();
	mCurNormalScope = (gSavedSettings.getBOOL("UseEnvironmentFromRegion") ? LLEnvKey::SCOPE_REGION : LLEnvKey::SCOPE_LOCAL);
	mInterpNextChangeMessage = true;
	mPendingOutgoingMessage = false;
	mIsEditing = false;
}

/*******
 * Region Changes
 *******/

void LLEnvManager::notifyLogin()
{
	changedRegion(false);
}
void LLEnvManager::notifyCrossing()
{
	changedRegion(true);
}
void LLEnvManager::notifyTP()
{
	changedRegion(false);
}
void LLEnvManager::changedRegion(bool interp)
{
	mInterpNextChangeMessage = interp;
	mPendingOutgoingMessage = false;

	LLFloaterReg::hideInstance("old_env_settings");
	LLFloaterReg::hideInstance("env_settings");

	resetInternalsToDefault(LLEnvKey::SCOPE_REGION);

	maybeClearEditingScope(LLEnvKey::SCOPE_REGION, true, false);
}

/*******
 * Editing settings / UI mode
 *******/

void LLEnvManager::startEditingScope(LLEnvKey::EScope scope)
{
	LL_DEBUGS("Windlight") << "Starting editing scope " << scope << LL_ENDL;

	if (mIsEditing)
	{
		LL_WARNS("Windlight") << "Tried to start editing windlight settings while already editing some settings (possibly others)!  Ignoring..." << LL_ENDL;
		return;
	}
	if (!canEdit(scope))
	{
		LL_WARNS("Windlight") << "Tried to start editing windlight settings while not allowed to!  Ignoring..." << LL_ENDL;
		return;
	}

	mIsEditing = true;
	mCurEditingScope = scope;

	// Back up local settings so that we can switch back to them later.
	if (scope != LLEnvKey::SCOPE_LOCAL)
	{
		backUpLocalSettingsIfNeeded();
	}

	// show scope being edited
	loadSettingsIntoManagers(scope, false);
	
	switch (scope)
	{
	case LLEnvKey::SCOPE_LOCAL:
		// not implemented here (yet)
		return;
	case LLEnvKey::SCOPE_REGION:
		/* LLPanelRegionTerrainInfo::instance()->setCommitControls(true); the windlight settings are no longer on the region terrain panel */
		break;
	default:
		return;
	}
}

void LLEnvManager::maybeClearEditingScope(LLEnvKey::EScope scope, bool user_initiated, bool was_commit)
{
	if (mIsEditing && mCurEditingScope == scope)
	{
		maybeClearEditingScope(user_initiated, was_commit); // handles UI, updating managers, etc.
	}
}

void LLEnvManager::maybeClearEditingScope(bool user_initiated, bool was_commit)
{
	bool clear_now = true;
	if (mIsEditing && !was_commit)
	{
		if(user_initiated)
		{
			LLSD args;
			args["SCOPE"] = getScopeString(mCurEditingScope);
			LLNotifications::instance().add("EnvEditUnsavedChangesCancel", args, LLSD(),
								boost::bind(&LLEnvManager::clearEditingScope, this, _1, _2));
			clear_now = false;
		}
		else
		{
			LLNotifications::instance().add("EnvEditExternalCancel", LLSD(), LLSD());
		}
	}

	if(clear_now)
	{
		clearEditingScope(LLSD(), LLSD());
	}
}

void LLEnvManager::clearEditingScope(const LLSD& notification, const LLSD& response)
{
	if(notification.isDefined() && response.isDefined() && LLNotification::getSelectedOption(notification, response) != 0)
	{
#if 0
		// *TODO: select terrain panel here
		mIsEditing = false;
		LLFloaterReg::showTypedInstance<LLFloaterRegionInfo>("regioninfo");
#endif
		return;
	}

	mIsEditing = false;

	updateUIFromEditability();
	/* LLPanelRegionTerrainInfo::instance()->cancelChanges(); the terrain panel no longer has windlight data - if this is needed, it should move. */

	loadSettingsIntoManagers(mCurNormalScope, true);
}

void LLEnvManager::updateUIFromEditability()
{
	// *TODO When the checkbox from LLFloaterEnvSettings is moved elsewhere, opening the local environment settings window should auto-display local settings
	// Currently, disable all editing to ensure region settings are hidden from those that can't edit them (to preserve possibility of future tradable assets)
	// Remove "!gSavedSettings.getBOOL(...)" when the desired behavior is implemented
//	LLFloaterEnvSettings::instance()->setControlsEnabled(canEdit(LLEnvKey::SCOPE_LOCAL) && !gSavedSettings.getBOOL("UseEnvironmentFromRegion"));
// 	LLPanelRegionTerrainInfo::instance()->setEnvControls(canEdit(LLEnvKey::SCOPE_REGION));
	// enable estate UI iff canEdit(LLEnvKey::SCOPE_ESTATE), etc.
}

bool LLEnvManager::regionCapable()
{
	return !gAgent.getRegion()->getCapability("EnvironmentSettings").empty();
}

const std::string LLEnvManager::getScopeString(LLEnvKey::EScope scope)
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

bool LLEnvManager::canEdit(LLEnvKey::EScope scope)
{
	// can't edit while a message is being sent or if already editing
	if (mPendingOutgoingMessage || mIsEditing)
	{
		return false;
	}

	// check permissions and caps
	switch (scope)
	{
	case LLEnvKey::SCOPE_LOCAL:
		return true; // always permitted to edit local
	case LLEnvKey::SCOPE_REGION:
		bool owner_or_god_or_manager;
		{
			LLViewerRegion* region = gAgent.getRegion();
			if (NULL == region || region->getCapability("EnvironmentSettings").empty())
			{
				// not a windlight-aware region
				return false;
			}
			owner_or_god_or_manager = gAgent.isGodlike()
				|| (region && (region->getOwner() == gAgent.getID()))
				|| (region && region->isEstateManager());
		}
		return owner_or_god_or_manager;
	default:
		return false;
	}
}

/*******
 * Incoming Messaging
 *******/

void LLEnvManager::refreshFromStorage(LLEnvKey::EScope scope)
{
	// Back up local env. settings so that we can switch to them later.
	if (scope != LLEnvKey::SCOPE_LOCAL)
	{
		backUpLocalSettingsIfNeeded();
	}

	switch (scope)
	{
	case LLEnvKey::SCOPE_LOCAL:
		break;
	case LLEnvKey::SCOPE_REGION:
		if (!LLEnvironmentRequest::initiate())
		{
			// don't have a cap for this, presume invalid response
			processIncomingMessage(LLSD(), scope);
		}
		break;
	default:
		processIncomingMessage(LLSD(), scope);
		break;
	}
}

bool LLEnvManager::processIncomingMessage(const LLSD& unvalidated_content, const LLEnvKey::EScope scope)
{
	if (scope != LLEnvKey::SCOPE_REGION)
	{
		return false;
	}

	// Start out with defaults
	resetInternalsToDefault(scope);
	updateUIFromEditability();

	// Validate
	//std::set<std::string> empty_set;
	//LLWLPacketScrubber scrubber(scope, empty_set);
	//LLSD windlight_llsd = scrubber.scrub(unvalidated_content);

	//bool valid = windlight_llsd.isDefined(); // successful scrub

	// *HACK - Don't have the validator, so just use content without validating.  Should validate here for third-party grids.
	LLSD windlight_llsd(unvalidated_content);
	bool valid = true;
	// end HACK

	mLastReceivedID = unvalidated_content[0]["messageID"].asUUID();		// if the message was valid, grab the UUID from it and save it for next outbound update message
	LL_DEBUGS("Windlight Sync") << "mLastReceivedID: " << mLastReceivedID << LL_ENDL;
	LL_DEBUGS("Windlight Sync") << "windlight_llsd: " << windlight_llsd << LL_ENDL;

	if (valid)
	{
		// TODO - the sun controls are moving; this should be updated
		F32 sun_hour = 0;
		LLPanelRegionTerrainInfo* terrain_panel = LLPanelRegionTerrainInfo::instance();

		if (terrain_panel)
		{
			sun_hour = terrain_panel->getSunHour();	// this slider is kept up to date
		}
		else
		{
			llwarns << "Cannot instantiate the terrain panel (exiting?)" << llendl;
		}

		LLWLParamManager::getInstance()->addAllSkies(scope, windlight_llsd[2]);
		LLEnvironmentSettings newSettings(windlight_llsd[1], windlight_llsd[2], windlight_llsd[3], sun_hour);
		mOrigSettingStore[scope] = newSettings;
	}
	else
	{
		LL_WARNS("Windlight Sync") << "Failed to parse windlight settings!" << LL_ENDL;
		// presume defaults (already reset above)
	}

	maybeClearEditingScope(scope, false, false);

	// refresh display with new settings, if applicable
	if (mCurNormalScope == scope && !mIsEditing) // if mIsEditing still, must be editing some other scope, so don't load
	{
		loadSettingsIntoManagers(scope, mInterpNextChangeMessage);
	}
	else
	{
		LL_DEBUGS("Windlight Sync") << "Not loading settings (mCurNormalScope=" << mCurNormalScope << ", scope=" << scope << ", mIsEditing=" << mIsEditing << ")" << LL_ENDL;
	}

	mInterpNextChangeMessage = true; // reset flag

	return valid;
}


/*******
 * Outgoing Messaging
 *******/

void LLEnvManager::commitSettings(LLEnvKey::EScope scope)
{
	LL_DEBUGS("Windlight Sync") << "commitSettings(scope = " << scope << ")" << LL_ENDL;

	bool success = true;
	switch (scope)
	{
	case (LLEnvKey::SCOPE_LOCAL):
		// not implemented - LLWLParamManager and LLWaterParamManager currently manage local storage themselves
		break;
	case (LLEnvKey::SCOPE_REGION):
		mPendingOutgoingMessage = true;
		LLSD metadata(LLSD::emptyMap());
		metadata["regionID"] = gAgent.getRegion()->getRegionID();
		metadata["messageID"] = mLastReceivedID;			// add last received update ID to outbound message so simulator can handle concurrent updates

		saveSettingsFromManagers(scope);	// save current settings into settings store before grabbing from settings store and sending
		success = LLEnvironmentApply::initiateRequest(makePacket(LLEnvKey::SCOPE_REGION, metadata));
		if(success)
		{
			// while waiting for the return message, render old settings
			// (as of Aug 09, we should get an updated RegionInfo packet, which triggers a re-request of Windlight data, which causes us to show new settings)
			loadSettingsIntoManagers(LLEnvKey::SCOPE_REGION, true);
		}
		break;
	}

	if(success)
	{
		// with mPendingOutgoingMessage = true, nothing is editable
		updateUIFromEditability();
		maybeClearEditingScope(true, true);
	}
	else
	{
		mPendingOutgoingMessage = false;
	}
}

LLSD LLEnvManager::makePacket(LLEnvKey::EScope scope, const LLSD& metadata)
{
	return mOrigSettingStore[scope].makePacket(metadata);
}

void LLEnvManager::commitSettingsFinished(LLEnvKey::EScope scope)
{
	mPendingOutgoingMessage = false;

	updateUIFromEditability();
}

void LLEnvManager::applyLocalSettingsToRegion()
{
	// Immediately apply current environment settings to region.
	LLEnvManager::instance().commitSettings(LLEnvKey::SCOPE_REGION);
}

/*******
 * Loading defaults
 *******/

void LLEnvManager::resetInternalsToDefault(LLEnvKey::EScope scope)
{
	if (LLEnvKey::SCOPE_LOCAL != scope)
	{
		LLWLParamManager::getInstance()->clearParamSetsOfScope(scope);
	}

	mOrigSettingStore[scope] = lindenDefaults();
	LLWLParamManager::getInstance()->mAnimator.setTimeType(LLWLAnimator::TIME_LINDEN);
}

const LLEnvironmentSettings& LLEnvManager::lindenDefaults()
{
	static bool loaded = false;
	static LLEnvironmentSettings defSettings;

	if (!loaded)
	{
		LLWaterParamSet defaultWater;
		LLWaterParamManager::instance().getParamSet("default", defaultWater);

		// *TODO save default skies (remove hack in LLWLDayCycle::loadDayCycle when this is done)

		defSettings.saveParams(
			LLWLDayCycle::loadCycleDataFromFile("default.xml"), // frames will refer to local presets, which is okay
			LLSD(LLSD::emptyMap()), // should never lose the default sky presets (read-only)
			defaultWater.getAll(),
			0.0);

		loaded = true;
	}

	return defSettings;
}

/*******
 * Manipulation of Param Managers
 *******/

void LLEnvManager::loadSettingsIntoManagers(LLEnvKey::EScope scope, bool interpolate)
{
	LL_DEBUGS("Windlight Sync") << "Loading settings (scope = " << scope << ")" << LL_ENDL;

	LLEnvironmentSettings settings = mOrigSettingStore[scope];

	if(interpolate)
	{
		LLWLParamManager::getInstance()->mAnimator.startInterpolation(settings.getWaterParams());
	}

	LLWLParamManager::getInstance()->addAllSkies(scope, settings.getSkyMap());
	LLWLParamManager::getInstance()->mDay.loadDayCycle(settings.getWLDayCycle(), scope);
	LLWLParamManager::getInstance()->resetAnimator(settings.getDayTime(), true);

	LLWaterParamManager::getInstance()->mCurParams.setAll(settings.getWaterParams());
}

void LLEnvManager::saveSettingsFromManagers(LLEnvKey::EScope scope)
{
	LL_DEBUGS("Windlight Sync") << "Saving settings (scope = " << scope << ")" << LL_ENDL;
	switch (scope)
	{
	case LLEnvKey::SCOPE_LOCAL:
		mOrigSettingStore[scope].saveParams(
							LLWLParamManager::getInstance()->mDay.asLLSD(),
							LLSD(LLSD::emptyMap()), // never overwrite
							LLWaterParamManager::getInstance()->mCurParams.getAll(),
							LLWLParamManager::getInstance()->mAnimator.mDayTime);
		break;
	case LLEnvKey::SCOPE_REGION:
		{
			// ensure only referenced region-scope skies are saved, resolve naming collisions, etc.
			std::map<LLWLParamKey, LLWLParamSet> final_references = LLWLParamManager::getInstance()->finalizeFromDayCycle(scope);
			LLSD referenced_skies = LLWLParamManager::createSkyMap(final_references);

			LL_DEBUGS("Windlight Sync") << "Dumping referenced skies (" << final_references.size() << ") to LLSD: " << referenced_skies << LL_ENDL;

			mOrigSettingStore[scope].saveParams(
								LLWLParamManager::getInstance()->mDay.asLLSD(),
								referenced_skies,
								LLWaterParamManager::getInstance()->mCurParams.getAll(),
								LLWLParamManager::getInstance()->mAnimator.mDayTime);
		}
		break;
	default:
		return;
	}
}

void LLEnvManager::backUpLocalSettingsIfNeeded()
{
	// *HACK: Back up local env. settings so that we can switch to them later.
	// Otherwise local day cycle is likely to be reset.
	static bool sSavedLocalSettings = false;

	if (!sSavedLocalSettings)
	{
		LL_DEBUGS("Windlight") << "Backing up local environment settings" << LL_ENDL;
		saveSettingsFromManagers(LLEnvKey::SCOPE_LOCAL);
		sSavedLocalSettings = true;
	}
}

/*******
 * Setting desired display level
 *******/

void LLEnvManager::setNormallyDisplayedScope(LLEnvKey::EScope new_scope)
{
	// temp, just save the scope directly as a value in the future when there's more than two
	bool want_region = (LLEnvKey::SCOPE_REGION == new_scope);
	gSavedSettings.setBOOL("UseEnvironmentFromRegion", want_region);

	if (mCurNormalScope != new_scope)
	{
		LL_INFOS("Windlight") << "Switching to scope " << new_scope << LL_ENDL;
		mCurNormalScope = new_scope;
		notifyOptInChange();
	}
}

LLEnvKey::EScope LLEnvManager::getNormallyDisplayedScope() const
{
	return mCurNormalScope;
}

void LLEnvManager::notifyOptInChange()
{
	bool opt_in = gSavedSettings.getBOOL("UseEnvironmentFromRegion");

	// Save local settings if switching to region
	if(opt_in)
	{
		LL_INFOS("Windlight") << "Saving currently-displayed settings as current local settings..." << LL_ENDL;
		saveSettingsFromManagers(LLEnvKey::SCOPE_LOCAL);
	}

	maybeClearEditingScope(true, false);
}

void LLEnvManager::dumpScopes()
{
	LLSD scope_dump;

	scope_dump = makePacket(LLEnvKey::SCOPE_LOCAL, LLSD());
	LL_DEBUGS("Windlight") << "Local scope:" << scope_dump << LL_ENDL;

	scope_dump = makePacket(LLEnvKey::SCOPE_REGION, LLSD());
	LL_DEBUGS("Windlight") << "Region scope:" << scope_dump << LL_ENDL;
}


//=============================================================================

std::string LLEnvPrefs::getWaterPresetName() const
{
	if (mWaterPresetName.empty())
	{
		llwarns << "Water preset name is empty" << llendl;
	}

	return mWaterPresetName;
}

std::string LLEnvPrefs::getSkyPresetName() const
{
	if (mSkyPresetName.empty())
	{
		llwarns << "Sky preset name is empty" << llendl;
	}

	return mSkyPresetName;
}

std::string LLEnvPrefs::getDayCycleName() const
{
	if (mDayCycleName.empty())
	{
		llwarns << "Day cycle name is empty" << llendl;
	}

	return mDayCycleName;
}

void LLEnvPrefs::setUseDefaults(bool val)
{
	mUseDefaults = val;
}

void LLEnvPrefs::setUseWaterPreset(const std::string& name)
{
	setUseDefaults(false);
	mWaterPresetName = name;
}

void LLEnvPrefs::setUseSkyPreset(const std::string& name)
{
	setUseDefaults(false);
	mUseDayCycle = false;
	mSkyPresetName = name;
}

void LLEnvPrefs::setUseDayCycle(const std::string& name)
{
	setUseDefaults(false);
	mUseDayCycle = true;
	mDayCycleName = name;
}

//=============================================================================
LLEnvManagerNew::LLEnvManagerNew()
{
	mInterpNextChangeMessage = true;

	// Set default environment settings.
	mUserPrefs.mUseDefaults = true;
	mUserPrefs.mUseDayCycle = true;
	mUserPrefs.mWaterPresetName = "Default";
	mUserPrefs.mSkyPresetName = "Default";
	mUserPrefs.mDayCycleName = "Default";
}

bool LLEnvManagerNew::getUseRegionSettings() const
{
	return mUserPrefs.getUseDefaults();
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
	return mCachedRegionPrefs;
}

bool LLEnvManagerNew::usePrefs()
{
	LL_DEBUGS("Windlight") << "Displaying preferred environment" << LL_ENDL;
	updateManagersFromPrefs(false);
	return true;
}

bool LLEnvManagerNew::useDefaults()
{
	bool rslt = useWaterPreset("Default") && useDayCycle("Default", LLEnvKey::SCOPE_LOCAL);
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
			llwarns << "No day cycle named " << name << llendl;
			return false;
		}
	}

	bool rslt = LLWLParamManager::instance().applyDayCycleParams(params, scope);
	llassert(rslt == true);
	return rslt;
}

bool LLEnvManagerNew::useDayCycleParams(const LLSD& params, LLEnvKey::EScope scope)
{
	LL_DEBUGS("Windlight") << "Displaying day cycle params" << LL_ENDL;
	LLWLParamManager::instance().applyDayCycleParams(params, scope);
	return true;
}

void LLEnvManagerNew::setUseRegionSettings(bool val)
{
	mUserPrefs.setUseDefaults(val);
	saveUserPrefs();
	updateManagersFromPrefs(false);
}

void LLEnvManagerNew::setUseWaterPreset(const std::string& name)
{
	// *TODO: make sure the preset exists.
	if (name.empty())
	{
		llwarns << "Empty water preset name passed" << llendl;
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
		llwarns << "Empty sky preset name passed" << llendl;
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
		llwarns << "Invalid day cycle name passed" << llendl;
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

	mUserPrefs.mUseDefaults		= gSavedSettings.getBOOL("UseEnvironmentFromRegion");
	mUserPrefs.mUseDayCycle		= gSavedSettings.getBOOL("UseDayCycle");
}

void LLEnvManagerNew::saveUserPrefs()
{
	gSavedSettings.setString("WaterPresetName",			getWaterPresetName());
	gSavedSettings.setString("SkyPresetName",			getSkyPresetName());
	gSavedSettings.setString("DayCycleName",			getDayCycleName());

	gSavedSettings.setBOOL("UseEnvironmentFromRegion",	getUseRegionSettings());
	gSavedSettings.setBOOL("UseDayCycle",				getUseDayCycle());
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

	mUserPrefs.mUseDefaults		= use_region_settings;
	mUserPrefs.mUseDayCycle		= !use_fixed_sky;

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

void LLEnvManagerNew::requestRegionSettings()
{
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

boost::signals2::connection LLEnvManagerNew::setRegionSettingsChangeCallback(const region_settings_change_signal_t::slot_type& cb)
{
	return mRegionSettingsChangeSignal.connect(cb);
}

boost::signals2::connection LLEnvManagerNew::setRegionChangeCallback(const region_change_signal_t::slot_type& cb)
{
	return mRegionChangeSignal.connect(cb);
}

boost::signals2::connection LLEnvManagerNew::setRegionSettingsAppliedCallback(const region_settings_applied_signal_t::slot_type& cb)
{
	return mRegionSettingsAppliedSignal.connect(cb);
}

void LLEnvManagerNew::onRegionCrossing()
{
	LL_DEBUGS("Windlight") << "Crossed region" << LL_ENDL;
	onRegionChange(true);
}

void LLEnvManagerNew::onTeleport()
{
	LL_DEBUGS("Windlight") << "Teleported" << LL_ENDL;
	onRegionChange(false);
}

void LLEnvManagerNew::onRegionSettingsResponse(const LLSD& content)
{
	// If the message was valid, grab the UUID from it and save it for next outbound update message.
	mLastReceivedID = content[0]["messageID"].asUUID();

	// Refresh cached region settings.
	LL_DEBUGS("Windlight") << "Caching region environment settings: " << content << LL_ENDL;
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
	mRegionSettingsAppliedSignal(ok);
}

//-- private methods ----------------------------------------------------------

// virtual
void LLEnvManagerNew::initSingleton()
{
	LL_DEBUGS("Windlight") << "Initializing LLEnvManagerNew" << LL_ENDL;

	loadUserPrefs();
}

void LLEnvManagerNew::updateManagersFromPrefs(bool interpolate)
{
	// Apply water settings.
	LLWaterParamManager::instance().applyUserPrefs(interpolate);

	// Apply sky settings.
	LLWLParamManager::instance().applyUserPrefs(interpolate);
}

void LLEnvManagerNew::onRegionChange(bool interpolate)
{
	// Avoid duplicating region setting requests
	// by checking whether the region is actually changing.
	LLViewerRegion* regionp = gAgent.getRegion();
	LLUUID region_uuid = regionp ? regionp->getRegionID() : LLUUID::null;
	if (region_uuid == mCurRegionUUID)
	{
		return;
	}

	// *TODO: clear environment settings of the previous region?

	// Request environment settings of the new region.
	LL_DEBUGS("Windlight") << "New viewer region: " << region_uuid << LL_ENDL;
	mCurRegionUUID = region_uuid;
	mInterpNextChangeMessage = interpolate;
	requestRegionSettings();

	// Let interested parties know agent region has been changed.
	mRegionChangeSignal();
}
