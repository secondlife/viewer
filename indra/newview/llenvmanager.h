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

	const LLSD& getWLDayCycle() const
	{
		return mWLDayCycle;
	}

	const LLSD& getWaterParams() const
	{
		return mWaterParams;
	}

	const LLSD& getSkyMap() const
	{
		return mSkyMap;
	}

	F64 getDayTime() const
	{
		return mDayTime;
	}

	bool isEmpty() const
	{
		return mWLDayCycle.size() == 0;
	}

	void clear()
	{
		*this = LLEnvironmentSettings();
	}

	LLSD makePacket(const LLSD& metadata) const
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
	LOG_CLASS(LLEnvManager);
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

	// for debugging purposes
	void dumpScopes();

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
	// If not done already, save current local environment settings, so that we can switch to them later.
	void backUpLocalSettingsIfNeeded();

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

/**
 * User environment preferences.
 */
class LLEnvPrefs
{
public:
	LLEnvPrefs() : mUseRegionSettings(true), mUseDayCycle(true) {}

	bool getUseRegionSettings() const { return mUseRegionSettings; }
	bool getUseDayCycle() const { return mUseDayCycle; }
	bool getUseFixedSky() const { return !getUseDayCycle(); }

	std::string getWaterPresetName() const;
	std::string getSkyPresetName() const;
	std::string getDayCycleName() const;

	void setUseRegionSettings(bool val);
	void setUseWaterPreset(const std::string& name);
	void setUseSkyPreset(const std::string& name);
	void setUseDayCycle(const std::string& name);

	bool			mUseRegionSettings;
	bool			mUseDayCycle;
	std::string		mWaterPresetName;
	std::string		mSkyPresetName;
	std::string		mDayCycleName;
};

/**
 * Setting:
 * 1. Use region settings.
 * 2. Use my setting: <water preset> + <fixed_sky>|<day_cycle>
 */
class LLEnvManagerNew : public LLSingleton<LLEnvManagerNew>
{
	LOG_CLASS(LLEnvManagerNew);
public:
	typedef boost::signals2::signal<void()> prefs_change_signal_t;
	typedef boost::signals2::signal<void()> region_settings_change_signal_t;
	typedef boost::signals2::signal<void()> region_change_signal_t;
	typedef boost::signals2::signal<void(bool)> region_settings_applied_signal_t;

	LLEnvManagerNew();

	// getters to access user env. preferences
	bool getUseRegionSettings() const;
	bool getUseDayCycle() const;
	bool getUseFixedSky() const;
	std::string getWaterPresetName() const;
	std::string getSkyPresetName() const;
	std::string getDayCycleName() const;

	/// @return cached env. settings of the current region.
	const LLEnvironmentSettings& getRegionSettings() const;

	/**
	 * Set new region settings without uploading them to the region.
	 *
	 * The override will be reset when the changes are applied to the region (=uploaded)
	 * or user teleports to another region.
	 */
	void setRegionSettings(const LLEnvironmentSettings& new_settings);

	// Change environment w/o changing user preferences.
	bool usePrefs();
	bool useDefaults();
	bool useRegionSettings();
	bool useWaterPreset(const std::string& name);
	bool useWaterParams(const LLSD& params);
	bool useSkyPreset(const std::string& name);
	bool useSkyParams(const LLSD& params);
	bool useDayCycle(const std::string& name, LLEnvKey::EScope scope);
	bool useDayCycleParams(const LLSD& params, LLEnvKey::EScope scope, F32 time = 0.5);

	// setters for user env. preferences
	void setUseRegionSettings(bool val);
	void setUseWaterPreset(const std::string& name);
	void setUseSkyPreset(const std::string& name);
	void setUseDayCycle(const std::string& name);
	void setUserPrefs(
		const std::string& water_preset,
		const std::string& sky_preset,
		const std::string& day_cycle_preset,
		bool use_fixed_sky,
		bool use_region_settings);

	// debugging methods
	void dumpUserPrefs();
	void dumpPresets();

	// Misc.
	void requestRegionSettings();
	bool sendRegionSettings(const LLEnvironmentSettings& new_settings);
	boost::signals2::connection setPreferencesChangeCallback(const prefs_change_signal_t::slot_type& cb);
	boost::signals2::connection setRegionSettingsChangeCallback(const region_settings_change_signal_t::slot_type& cb);
	boost::signals2::connection setRegionChangeCallback(const region_change_signal_t::slot_type& cb);
	boost::signals2::connection setRegionSettingsAppliedCallback(const region_settings_applied_signal_t::slot_type& cb);

	static bool canEditRegionSettings(); /// @return true if we have access to editing region environment

	// Public callbacks.
	void onRegionCrossing();
	void onTeleport();
	void onRegionSettingsResponse(const LLSD& content);
	void onRegionSettingsApplyResponse(bool ok);

private:
	friend class LLSingleton<LLEnvManagerNew>;
	/*virtual*/ void initSingleton();

	void loadUserPrefs();
	void saveUserPrefs();

	void updateSkyFromPrefs();
	void updateWaterFromPrefs(bool interpolate);
	void updateManagersFromPrefs(bool interpolate);

	bool useRegionSky();
	bool useRegionWater();

	bool useDefaultSky();
	bool useDefaultWater();

	void onRegionChange(bool interpolate);

	/// Emitted when user environment preferences change.
	prefs_change_signal_t mUsePrefsChangeSignal;

	/// Emitted when region environment settings update comes.
	region_settings_change_signal_t	mRegionSettingsChangeSignal;

	/// Emitted when agent region changes. Move to LLAgent?
	region_change_signal_t	mRegionChangeSignal;

	/// Emitted when agent region changes. Move to LLAgent?
	region_settings_applied_signal_t mRegionSettingsAppliedSignal;

	LLEnvPrefs				mUserPrefs;					/// User environment preferences.
	LLEnvironmentSettings	mCachedRegionPrefs;			/// Cached region environment settings.
	LLEnvironmentSettings	mNewRegionPrefs;			/// Not-yet-uploaded modified region env. settings.
	bool					mInterpNextChangeMessage;	/// Interpolate env. settings on next region change.
	LLUUID					mCurRegionUUID;				/// To avoid duplicated region env. settings requests.
	LLUUID					mLastReceivedID;			/// Id of last received region env. settings.
};

#endif // LL_LLENVMANAGER_H

