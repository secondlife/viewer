/**
 * @file llenvmanager.h
 * @brief Declaration of classes managing WindLight and water settings.
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

#ifndef LL_LLENVMANAGER_H
#define LL_LLENVMANAGER_H

#include "llmemory.h"
#include "llsd.h"

class LLWLParamManager;
class LLWaterParamManager;
class LLWLAnimator;

// generic key
struct LLEnvKey
{
public:
	// Note: enum ordering is important; for example, a region-level floater (1) will see local and region (all values that are <=)
	typedef enum e_scope
	{
		SCOPE_LOCAL,				// 0
		SCOPE_REGION//,				// 1
		// SCOPE_ESTATE,			// 2
		// etc.
	} EScope;
};

class LLEnvironmentSettings
{
public:
	LLEnvironmentSettings() :
		mWLDayCycle(LLSD::emptyMap()),
		mSkyMap(LLSD::emptyMap()),
		mWaterParams(LLSD::emptyMap()),
		mDayTime(0.f)
	{}
	LLEnvironmentSettings(const LLSD& dayCycle, const LLSD& skyMap, const LLSD& waterParams, F64 dayTime) :
		mWLDayCycle(dayCycle),
		mSkyMap(skyMap),
		mWaterParams(waterParams),
		mDayTime(dayTime)
	{}
	~LLEnvironmentSettings() {}

	void saveParams(const LLSD& dayCycle, const LLSD& skyMap, const LLSD& waterParams, F64 dayTime)
	{
		mWLDayCycle = dayCycle;
		mSkyMap = skyMap;
		mWaterParams = waterParams;
		mDayTime = dayTime;
	}

	LLSD& getWLDayCycle()
	{
		return mWLDayCycle;
	}

	LLSD& getWaterParams()
	{
		return mWaterParams;
	}

	LLSD& getSkyMap()
	{
		return mSkyMap;
	}

	F64 getDayTime()
	{
		return mDayTime;
	}

	LLSD makePacket(const LLSD& metadata)
	{
		LLSD full_packet = LLSD::emptyArray();

		// 0: metadata
		full_packet.append(metadata);

		// 1: day cycle
		full_packet.append(mWLDayCycle);

		// 2: map of sky setting names to sky settings (as LLSD)
		full_packet.append(mSkyMap);

		// 3: water params
		full_packet.append(mWaterParams);

		return full_packet;
	}

private:
	LLSD mWLDayCycle, mWaterParams, mSkyMap;
	F64 mDayTime;
};

// not thread-safe
class LLEnvManager : public LLSingleton<LLEnvManager>
{
public:
	// sets scopes (currently, only region-scope) to startup states
	// delay calling these until as close as possible to knowing whether the remote service is capable of holding windlight settings
	void notifyCrossing();
	// these avoid interpolation on the next incoming message (if it comes)
	void notifyLogin();
	void notifyTP();

	// request settings again from remote storage (currently implemented only for region)
	void refreshFromStorage(LLEnvKey::EScope scope);
	// stores settings and starts transitions (as necessary)
	// validates packet and returns whether it was valid
	// loads defaults if not valid
	// returns whether or not arguments were valid
	bool processIncomingMessage(const LLSD& packet, LLEnvKey::EScope scope);
	// saves settings in the given scope to persistent storage appropriate for that scope
	void commitSettings(LLEnvKey::EScope scope);
	// called back when the commit finishes
	void commitSettingsFinished(LLEnvKey::EScope scope);
	// Immediately apply current settings from managers to region.
	void applyLocalSettingsToRegion();

	/* 
	 * notify of changes in god/not-god mode, estate ownership, etc.
	 * should be called every time after entering new region (after receiving new caps)
	 */
	void notifyPermissionChange();

	bool regionCapable();
	static const std::string getScopeString(LLEnvKey::EScope scope);
	bool canEdit(LLEnvKey::EScope scope);
	// enables and populates UI
	// populates display (param managers) with scope's settings
	// silently fails if canEdit(scope) is false!
	void startEditingScope(LLEnvKey::EScope scope);
	// cancel and close UI as necessary
	// reapplies unedited settings
	// displays the settings from the scope that user has set (i.e. opt-in setting for now)
	void maybeClearEditingScope(bool user_initiated, bool was_commit);
	// clear the scope only if was editing that scope
	void maybeClearEditingScope(LLEnvKey::EScope scope, bool user_initiated, bool was_commit);
	// actually do the clearing
	void clearEditingScope(const LLSD& notification, const LLSD& response);

	// clear and reload defaults into scope
	void resetInternalsToDefault(LLEnvKey::EScope scope);

	// sets which scope is to be displayed
	// fix me if/when adding more levels of scope
	void setNormallyDisplayedScope(LLEnvKey::EScope scope);
	// gets normally displayed scope
	LLEnvKey::EScope getNormallyDisplayedScope() const;

private:
	// overriden initializer
	friend class LLSingleton<LLEnvManager>;
	virtual void initSingleton();
	// helper function (when region changes, but before caps are received)
	void changedRegion(bool interpolate);
	// apply to current display and UI
	void loadSettingsIntoManagers(LLEnvKey::EScope scope, bool interpolate);
	// save from current display and UI into memory (mOrigSettingStore)
	void saveSettingsFromManagers(LLEnvKey::EScope scope);

	// Save copy of settings from the current ones in the param managers
	LLEnvironmentSettings collateFromParamManagers();
	// bundle settings (already committed from UI) into an LLSD
	LLSD makePacket(LLEnvKey::EScope scope, const LLSD& metadata);

	void updateUIFromEditability();

	// only call when setting *changes*, not just when it might have changed
	// saves local settings into mOrigSettingStore when necessary
	void notifyOptInChange();

	// calculate Linden default settings
	static const LLEnvironmentSettings& lindenDefaults();

	std::map<LLEnvKey::EScope, LLEnvironmentSettings> mOrigSettingStore; // settings which have been committed from UI

	bool mInterpNextChangeMessage;
	bool mPendingOutgoingMessage;
	bool mIsEditing;
	LLEnvKey::EScope mCurNormalScope; // scope being displayed when not editing (i.e. most of the time)
	LLEnvKey::EScope mCurEditingScope;
	LLUUID mLastReceivedID;
};

#endif // LL_LLENVMANAGER_H

