/**
 * @file llwlparammanager.h
 * @brief Implementation for the LLWLParamManager class.
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

#ifndef LL_WLPARAMMANAGER_H
#define LL_WLPARAMMANAGER_H

#include <vector>
#include <map>
#include "llwlparamset.h"
#include "llwlanimator.h"
#include "llwldaycycle.h"
#include "llviewercamera.h"

class LLGLSLShader;
 
// color control
struct WLColorControl {
	
	F32 r, g, b, i;				/// the values
	char const * name;			/// name to use to dereference params
	std::string mSliderName;	/// name of the slider in menu
	bool hasSliderName;			/// only set slider name for true color types
	bool isSunOrAmbientColor;			/// flag for if it's the sun or ambient color controller
	bool isBlueHorizonOrDensity;		/// flag for if it's the Blue Horizon or Density color controller

	inline WLColorControl(F32 red, F32 green, F32 blue, F32 intensity, char const * n,
		char const * sliderName = "")
		: r(red), g(green), b(blue), i(intensity), name(n), mSliderName(sliderName)
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
		params.set(name, r, g, b, i);
	}
};

// float slider control
struct WLFloatControl {
	F32 x;
	char const * name;
	F32 mult;

	inline WLFloatControl(F32 val, char const * n, F32 m=1.0f)
		: x(val), name(n), mult(m)
	{
	}

	inline WLFloatControl & operator = (LLVector4 const & val) {
		x = val.mV[0];

		return *this;
	}

	inline operator F32 (void) const {
		return x;
	}

	inline void update(LLWLParamSet & params) const {
		params.set(name, x);
	}
};

/// WindLight parameter manager class - what controls all the wind light shaders
class LLWLParamManager
{
public:

	LLWLParamManager();
	~LLWLParamManager();

	/// load a preset file
	void loadPresets(const LLString & fileName);

	/// save the preset file
	void savePresets(const LLString & fileName);

	/// load an individual preset into the sky
	void loadPreset(const LLString & name);

	/// save the parameter presets to file
	void savePreset(const LLString & name);

	/// Set shader uniforms dirty, so they'll update automatically.
	void propagateParameters(void);
	
	/// Update shader uniforms that have changed.
	void updateShaderUniforms(LLGLSLShader * shader);

	/// setup the animator to run
	void resetAnimator(F32 curTime, bool run);

	/// update information camera dependent parameters
	void update(LLViewerCamera * cam);

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

	/// Perform global initialization for this class.
	static void initClass(void);

	// Cleanup of global data that's only inited once per class.
	static void cleanupClass();
	
	/// add a param to the list
	bool addParamSet(const std::string& name, LLWLParamSet& param);

	/// add a param to the list
	BOOL addParamSet(const std::string& name, LLSD const & param);

	/// get a param from the list
	bool getParamSet(const std::string& name, LLWLParamSet& param);

	/// set the param in the list with a new param
	bool setParamSet(const std::string& name, LLWLParamSet& param);
	
	/// set the param in the list with a new param
	bool setParamSet(const std::string& name, LLSD const & param);	

	/// gets rid of a parameter and any references to it
	/// returns true if successful
	bool removeParamSet(const std::string& name, bool delete_from_disk);

	// singleton pattern implementation
	static LLWLParamManager * instance();


public:

	// helper variables
	F32 mSunAngle;
	F32 mEastAngle;
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

	// length of the day in seconds
	F32 mLengthOfDay;

	LLWLParamSet mCurParams;

	/// Sun Delta Terrain tweak variables.
	F32 mSunDeltaYaw;
	WLFloatControl mWLGamma;

	F32 mSceneLightStrength;
	
	/// Atmospherics
	WLColorControl mBlueHorizon;
	WLColorControl mHazeDensity;
	WLColorControl mBlueDensity;
	WLFloatControl mDensityMult;
	WLColorControl mHazeHorizon;
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
	
	// list of all the parameters, listed by name
	std::map<std::string, LLWLParamSet> mParamList;
	
	
private:
	// our parameter manager singleton instance
	static LLWLParamManager * sInstance;

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
