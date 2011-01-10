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

	LLFloaterEnvSettings::instance()->closeFloater();

	resetInternalsToDefault(LLEnvKey::SCOPE_REGION);

	maybeClearEditingScope(LLEnvKey::SCOPE_REGION, true, false);
}

/*******
 * Editing settings / UI mode
 *******/

void LLEnvManager::startEditingScope(LLEnvKey::EScope scope)
{
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

	// show scope being edited
	loadSettingsIntoManagers(scope, false);
	
	switch (scope)
	{
	case LLEnvKey::SCOPE_LOCAL:
		// not implemented here (yet)
		return;
	case LLEnvKey::SCOPE_REGION:
		LLPanelRegionTerrainInfo::instance()->setCommitControls(true);
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
		// *TODO: select terrain panel here
		mIsEditing = false;
		LLFloaterReg::showTypedInstance<LLFloaterRegionInfo>("regioninfo");
		return;
	}

	mIsEditing = false;

	updateUIFromEditability();
	LLPanelRegionTerrainInfo::instance()->cancelChanges();

	loadSettingsIntoManagers(mCurNormalScope, true);
}

void LLEnvManager::updateUIFromEditability()
{
	// *TODO When the checkbox from LLFloaterEnvSettings is moved elsewhere, opening the local environment settings window should auto-display local settings
	// Currently, disable all editing to ensure region settings are hidden from those that can't edit them (to preserve possibility of future tradable assets)
	// Remove "!gSavedSettings.getBOOL(...)" when the desired behavior is implemented
	LLFloaterEnvSettings::instance()->setControlsEnabled(canEdit(LLEnvKey::SCOPE_LOCAL) && !gSavedSettings.getBOOL("UseEnvironmentFromRegion"));
	LLPanelRegionTerrainInfo::instance()->setEnvControls(canEdit(LLEnvKey::SCOPE_REGION));
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
	switch (scope)
	{
	case LLEnvKey::SCOPE_LOCAL:
		break;
	case LLEnvKey::SCOPE_REGION:
		if (!LLEnvironmentRequestResponder::initiateRequest())
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

	if (valid)
	{
		F32 sun_hour = LLPanelRegionTerrainInfo::instance()->getSunHour();	// this slider is kept up to date
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

	mInterpNextChangeMessage = true; // reset flag

	return valid;
}


/*******
 * Outgoing Messaging
 *******/

void LLEnvManager::commitSettings(LLEnvKey::EScope scope)
{
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
		success = LLEnvironmentApplyResponder::initiateRequest(makePacket(LLEnvKey::SCOPE_REGION, metadata));
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
			LLSD referenced_skies = LLWLParamManager::getInstance()->createSkyMap(final_references);
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
		mCurNormalScope = new_scope;
		notifyOptInChange();
	}
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
