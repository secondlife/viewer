/**
 * @file llwlparammanager.h
 * @brief Implementation for the LLWLParamManager class.
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
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

#ifndef LL_WLPARAMMANAGER_H
#define LL_WLPARAMMANAGER_H

#include <list>
#include <map>
#include "llenvmanager.h"
#include "llwlparamset.h"
#include "llwlanimator.h"
#include "llwldaycycle.h"
#include "llviewercamera.h"
#include "lltrans.h"

class LLGLSLShader;
 
// color control
struct WLColorControl {
	
	F32 r, g, b, i;				/// the values
	std::string mName;			/// name to use to dereference params
	std::string mSliderName;	/// name of the slider in menu
	bool hasSliderName;			/// only set slider name for true color types
	bool isSunOrAmbientColor;			/// flag for if it's the sun or ambient color controller
	bool isBlueHorizonOrDensity;		/// flag for if it's the Blue Horizon or Density color controller

	inline WLColorControl(F32 red, F32 green, F32 blue, F32 intensity,
						  const std::string& n, const std::string& sliderName = LLStringUtil::null)
		: r(red), g(green), b(blue), i(intensity), mName(n), mSliderName(sliderName)
	{
		// if there's a slider name, say we have one
		hasSliderName = false;
		if (mSliderName != "") {
			hasSliderName = true;
		}

		// if it's the sun controller
		isSunOrAmbientColor = false;
		if (mSliderName == "WLSunlight" || mSliderName == "WLAmbient") {
			isSunOrAmbientColor = true;
		}

		isBlueHorizonOrDensity = false;
		if (mSliderName == "WLBlueHorizon" || mSliderName == "WLBlueDensity") {
			isBlueHorizonOrDensity = true;
		}
	}

	inline WLColorControl & operator = (LLVector4 const & val) {
		r = val.mV[0];
		g = val.mV[1];
		b = val.mV[2];
		i = val.mV[3];
		return *this;
	}

	inline operator LLVector4 (void) const {
		return LLVector4(r, g, b, i);
	}

	inline operator LLVector3 (void) const {
		return LLVector3(r, g, b);
	}

	inline void update(LLWLParamSet & params) const {
		params.set(mName, r, g, b, i);
	}
};

// float slider control
struct WLFloatControl {
	F32 x;
	std::string mName;
	F32 mult;

	inline WLFloatControl(F32 val, const std::string& n, F32 m=1.0f)
		: x(val), mName(n), mult(m)
	{
	}

	inline WLFloatControl & operator = (F32 val) {
		x = val;
		return *this;
	}

	inline operator F32 (void) const {
		return x;
	}

	inline void update(LLWLParamSet & params) const {
		params.set(mName, x);
	}
};

struct LLWLParamKey : LLEnvKey
{
public:
	// scope and source of a param set (WL sky preset)
	std::string name;
	EScope scope;

	// for conversion from LLSD
	static const int NAME_IDX = 0;
	static const int SCOPE_IDX = 1;

	inline LLWLParamKey(const std::string& n, EScope s)
		: name(n), scope(s)
	{
	}

	inline LLWLParamKey(LLSD llsd)
		: name(llsd[NAME_IDX].asString()), scope(EScope(llsd[SCOPE_IDX].asInteger()))
	{
	}

	inline LLWLParamKey() // NOT really valid, just so std::maps can return a default of some sort
		: name(""), scope(SCOPE_LOCAL)
	{
	}

	inline LLWLParamKey(std::string& stringVal)
	{
		size_t len = stringVal.length();
		if (len > 0)
		{
			name = stringVal.substr(0, len - 1);
			scope = (EScope) atoi(stringVal.substr(len - 1, len).c_str());
		}
	}

	inline std::string toStringVal() const
	{
		std::stringstream str;
		str << name << scope;
		return str.str();
	}

	inline LLSD toLLSD() const
	{
		LLSD llsd = LLSD::emptyArray();
		llsd.append(LLSD(name));
		llsd.append(LLSD(scope));
		return llsd;
	}

	inline void fromLLSD(const LLSD& llsd)
	{
		name = llsd[NAME_IDX].asString();
		scope = EScope(llsd[SCOPE_IDX].asInteger());
	}

	inline bool operator <(const LLWLParamKey other) const
	{
		if (name < other.name)
		{	
			return true;
		}
		else if (name > other.name)
		{
			return false;
		}
		else
		{
			return scope < other.scope;
		}
	}

	inline bool operator ==(const LLWLParamKey other) const
	{
		return (name == other.name) && (scope == other.scope);
	}

	inline std::string toString() const
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
};

/// WindLight parameter manager class - what controls all the wind light shaders
class LLWLParamManager : public LLSingleton<LLWLParamManager>
{
	LOG_CLASS(LLWLParamManager);

public:
	typedef std::list<std::string> preset_name_list_t;
	typedef std::list<LLWLParamKey> preset_key_list_t;
	typedef boost::signals2::signal<void()> preset_list_signal_t;

	/// save the parameter presets to file
	void savePreset(const LLWLParamKey key);

	/// Set shader uniforms dirty, so they'll update automatically.
	void propagateParameters(void);
	
	/// Update shader uniforms that have changed.
	void updateShaderUniforms(LLGLSLShader * shader);

	/// setup the animator to run
	void resetAnimator(F32 curTime, bool run);

	/// update information camera dependent parameters
	void update(LLViewerCamera * cam);

	/// apply specified day cycle, setting time to noon by default
	bool applyDayCycleParams(const LLSD& params, LLEnvKey::EScope scope, F32 time = 0.5);

	/// apply specified fixed sky params
	bool applySkyParams(const LLSD& params);

	// get where the light is pointing
	inline LLVector4 getLightDir(void) const;

	// get where the light is pointing
	inline LLVector4 getClampedLightDir(void) const;

	// get where the light is pointing
	inline LLVector4 getRotatedLightDir(void) const;
	
	/// get the dome's offset
	inline F32 getDomeOffset(void) const;

	/// get the radius of the dome
	inline F32 getDomeRadius(void) const;
	
	/// add a param set (preset) to the list
	bool addParamSet(const LLWLParamKey& key, LLWLParamSet& param);

	/// add a param set (preset) to the list
	BOOL addParamSet(const LLWLParamKey& key, LLSD const & param);

	/// get a param set (preset) from the list
	bool getParamSet(const LLWLParamKey& key, LLWLParamSet& param);

	/// check whether the preset is in the list
	bool hasParamSet(const LLWLParamKey& key);

	/// set the param in the list with a new param
	bool setParamSet(const LLWLParamKey& key, LLWLParamSet& param);
	
	/// set the param in the list with a new param
	bool setParamSet(const LLWLParamKey& key, LLSD const & param);

	/// gets rid of a parameter and any references to it
	/// ignores "delete_from_disk" if the scope is not local
	void removeParamSet(const LLWLParamKey& key, bool delete_from_disk);

	/// clear parameter mapping of a given scope
	void clearParamSetsOfScope(LLEnvKey::EScope scope);

	/// @return true if the preset comes out of the box
	bool isSystemPreset(const std::string& preset_name) const;

	/// @return user and system preset names as a single list
	void getPresetNames(preset_name_list_t& region, preset_name_list_t& user, preset_name_list_t& sys) const;

	/// @return user preset names
	void getUserPresetNames(preset_name_list_t& user) const;

	/// @return keys of all known presets
	void getPresetKeys(preset_key_list_t& keys) const;

	/// Emitted when a preset gets added or deleted.
	boost::signals2::connection setPresetListChangeCallback(const preset_list_signal_t::slot_type& cb);

	/// add all skies in LLSD using the given scope
	void addAllSkies(LLEnvKey::EScope scope, const LLSD& preset_map);

	/// refresh region-scope presets
	void refreshRegionPresets();

	// returns all skies referenced by the current day cycle (in mDay), with their final names
	// side effect: applies changes to all internal structures!  (trashes all unreferenced skies in scope, keys in day cycle rescoped to scope, etc.)
	std::map<LLWLParamKey, LLWLParamSet> finalizeFromDayCycle(LLWLParamKey::EScope scope);

	// returns all skies in map (intended to be called with output from a finalize)
	static LLSD createSkyMap(std::map<LLWLParamKey, LLWLParamSet> map);

	/// escape string in a way different from LLURI::escape()
	static std::string escapeString(const std::string& str);

	// helper variables
	LLWLAnimator mAnimator;

	/// actual direction of the sun
	LLVector4 mLightDir;

	/// light norm adjusted so haze works correctly
	LLVector4 mRotatedLightDir;

	/// clamped light norm for shaders that
	/// are adversely affected when the sun goes below the
	/// horizon
	LLVector4 mClampedLightDir;

	// list of params and how they're cycled for days
	LLWLDayCycle mDay;

	LLWLParamSet mCurParams;

	/// Sun Delta Terrain tweak variables.
	F32 mSunDeltaYaw;
	WLFloatControl mWLGamma;

	F32 mSceneLightStrength;
	
	/// Atmospherics
	WLColorControl mBlueHorizon;
	WLFloatControl mHazeDensity;
	WLColorControl mBlueDensity;
	WLFloatControl mDensityMult;
	WLFloatControl mHazeHorizon;
	WLFloatControl mMaxAlt;

	/// Lighting
	WLColorControl mLightnorm;
	WLColorControl mSunlight;
	WLColorControl mAmbient;
	WLColorControl mGlow;

	/// Clouds
	WLColorControl mCloudColor;
	WLColorControl mCloudMain;
	WLFloatControl mCloudCoverage;
	WLColorControl mCloudDetail;
	WLFloatControl mDistanceMult;
	WLFloatControl mCloudScale;

	/// sky dome
	F32 mDomeOffset;
	F32 mDomeRadius;
	

private:

	friend class LLWLAnimator;

	void loadAllPresets();
	void loadPresetsFromDir(const std::string& dir);
	bool loadPreset(const std::string& path);

	static std::string getSysDir();
	static std::string getUserDir();

	friend class LLSingleton<LLWLParamManager>;
	/*virtual*/ void initSingleton();
	LLWLParamManager();
	~LLWLParamManager();

	// list of all the parameters, listed by name
	std::map<LLWLParamKey, LLWLParamSet> mParamList;

	preset_list_signal_t mPresetListChangeSignal;
};

inline F32 LLWLParamManager::getDomeOffset(void) const
{
	return mDomeOffset;
}

inline F32 LLWLParamManager::getDomeRadius(void) const
{
	return mDomeRadius;
}

inline LLVector4 LLWLParamManager::getLightDir(void) const
{
	return mLightDir;
}

inline LLVector4 LLWLParamManager::getClampedLightDir(void) const
{
	return mClampedLightDir;
}

inline LLVector4 LLWLParamManager::getRotatedLightDir(void) const
{
	return mRotatedLightDir;
}

#endif
