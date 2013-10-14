/**
 * @file llenvmanager.h
 * @brief Declaration of classes managing WindLight and water settings.
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
	static const std::string getScopeString(LLEnvKey::EScope scope);

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

