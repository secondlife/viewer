/**
 * @file llwaterparammanager.h
 * @brief Implementation for the LLWaterParamManager class.
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

#ifndef LL_WATER_PARAMMANAGER_H
#define LL_WATER_PARAMMANAGER_H

#include <vector>
#include <map>
#include "llwaterparamset.h"
#include "llviewercamera.h"
#include "v4color.h"

const F32 WATER_FOG_LIGHT_CLAMP = 0.3f;

// color control
struct WaterColorControl {
	
	F32 mR, mG, mB, mA, mI;			/// the values
	std::string mName;				/// name to use to dereference params
	std::string mSliderName;		/// name of the slider in menu
	bool mHasSliderName;			/// only set slider name for true color types

	inline WaterColorControl(F32 red, F32 green, F32 blue, F32 alpha,
							 F32 intensity, const std::string& n, const std::string& sliderName = LLStringUtil::null)
		: mR(red), mG(green), mB(blue), mA(alpha), mI(intensity), mName(n), mSliderName(sliderName)
	{
		// if there's a slider name, say we have one
		mHasSliderName = false;
		if (mSliderName != "") {
			mHasSliderName = true;
		}
	}

	inline WaterColorControl & operator = (LLColor4 const & val) 
	{
		mR = val.mV[0];
		mG = val.mV[1];
		mB = val.mV[2];
		mA = val.mV[3];		
		return *this;
	}

	inline operator LLColor4 (void) const
	{
		return LLColor4(mR, mG, mB, mA);
	}

	inline WaterColorControl & operator = (LLVector4 const & val) 
	{
		mR = val.mV[0];
		mG = val.mV[1];
		mB = val.mV[2];
		mA = val.mV[3];		
		return *this;
	}

	inline operator LLVector4 (void) const 
	{
		return LLVector4(mR, mG, mB, mA);
	}

	inline operator LLVector3 (void) const 
	{
		return LLVector3(mR, mG, mB);
	}

	inline void update(LLWaterParamSet & params) const 
	{
		params.set(mName, mR, mG, mB, mA);
	}
};

struct WaterVector3Control 
{
	F32 mX;
	F32 mY;
	F32 mZ;

	std::string mName;

	// basic constructor
	inline WaterVector3Control(F32 valX, F32 valY, F32 valZ, const std::string& n)
		: mX(valX), mY(valY), mZ(valZ), mName(n)
	{
	}

	inline WaterVector3Control & operator = (LLVector3 const & val) 
	{
		mX = val.mV[0];
		mY = val.mV[1];
		mZ = val.mV[2];

		return *this;
	}

	inline void update(LLWaterParamSet & params) const 
	{
		params.set(mName, mX, mY, mZ);
	}

};

struct WaterVector2Control 
{
	F32 mX;
	F32 mY;

	std::string mName;

	// basic constructor
	inline WaterVector2Control(F32 valX, F32 valY, const std::string& n)
		: mX(valX), mY(valY), mName(n)
	{
	}

	inline WaterVector2Control & operator = (LLVector2 const & val) 
	{
		mX = val.mV[0];
		mY = val.mV[1];

		return *this;
	}

	inline void update(LLWaterParamSet & params) const 
	{
		params.set(mName, mX, mY);
	}
};

// float slider control
struct WaterFloatControl 
{
	F32 mX;
	std::string mName;
	F32 mMult;

	inline WaterFloatControl(F32 val, const std::string& n, F32 m=1.0f)
		: mX(val), mName(n), mMult(m)
	{
	}

	inline WaterFloatControl & operator = (LLVector4 const & val) 
	{
		mX = val.mV[0];

		return *this;
	}

	inline operator F32 (void) const 
	{
		return mX;
	}

	inline void update(LLWaterParamSet & params) const 
	{
		params.set(mName, mX);
	}
};

// float slider control
struct WaterExpFloatControl 
{
	F32 mExp;
	std::string mName;
	F32 mBase;

	inline WaterExpFloatControl(F32 val, const std::string& n, F32 b)
		: mExp(val), mName(n), mBase(b)
	{
	}

	inline WaterExpFloatControl & operator = (F32 val) 
	{
		mExp = log(val) / log(mBase);

		return *this;
	}

	inline operator F32 (void) const 
	{
		return pow(mBase, mExp);
	}

	inline void update(LLWaterParamSet & params) const 
	{
		params.set(mName, pow(mBase, mExp));
	}
};


/// WindLight parameter manager class - what controls all the wind light shaders
class LLWaterParamManager
{
public:

	LLWaterParamManager();
	~LLWaterParamManager();

	/// load a preset file
	void loadAllPresets(const std::string & fileName);

	/// load an individual preset into the sky

	void loadPreset(const std::string & name,bool propagate=true);

	/// save the parameter presets to file
	void savePreset(const std::string & name);

	/// send the parameters to the shaders
	void propagateParameters(void);

	/// update information for the shader
	void update(LLViewerCamera * cam);

	/// Update shader uniforms that have changed.
	void updateShaderUniforms(LLGLSLShader * shader);

	/// Perform global initialization for this class.
	static void initClass(void);

	// Cleanup of global data that's only inited once per class.
	static void cleanupClass();

	/// add a param to the list
	bool addParamSet(const std::string& name, LLWaterParamSet& param);

	/// add a param to the list
	BOOL addParamSet(const std::string& name, LLSD const & param);

	/// get a param from the list
	bool getParamSet(const std::string& name, LLWaterParamSet& param);

	/// set the param in the list with a new param
	bool setParamSet(const std::string& name, LLWaterParamSet& param);
	
	/// set the param in the list with a new param
	bool setParamSet(const std::string& name, LLSD const & param);	

	/// gets rid of a parameter and any references to it
	/// returns true if successful
	bool removeParamSet(const std::string& name, bool delete_from_disk);

	/// set the normap map we want for water
	bool setNormalMapID(const LLUUID& img);

	void setDensitySliderValue(F32 val);

	/// getters for all the different things water param manager maintains
	LLUUID getNormalMapID(void);
	LLVector2 getWave1Dir(void);
	LLVector2 getWave2Dir(void);
	F32 getScaleAbove(void);
	F32 getScaleBelow(void);
	LLVector3 getNormalScale(void);
	F32 getFresnelScale(void);
	F32 getFresnelOffset(void);
	F32 getBlurMultiplier(void);
	F32 getFogDensity(void);
	LLColor4 getFogColor(void);

	// singleton pattern implementation
	static LLWaterParamManager * instance();

public:

	LLWaterParamSet mCurParams;

	/// Atmospherics
	WaterColorControl mFogColor;
	WaterExpFloatControl mFogDensity;
	WaterFloatControl mUnderWaterFogMod;

	/// wavelet scales and directions
	WaterVector3Control mNormalScale;
	WaterVector2Control mWave1Dir;
	WaterVector2Control mWave2Dir;

	// controls how water is reflected and refracted
	WaterFloatControl mFresnelScale;
	WaterFloatControl mFresnelOffset;
	WaterFloatControl mScaleAbove;
	WaterFloatControl mScaleBelow;
	WaterFloatControl mBlurMultiplier;
	
	// list of all the parameters, listed by name
	std::map<std::string, LLWaterParamSet> mParamList;

	F32 mDensitySliderValue;

private:
	LLVector4 mWaterPlane;
	F32 mWaterFogKS;

	// our parameter manager singleton instance
	static LLWaterParamManager * sInstance;
};

inline void LLWaterParamManager::setDensitySliderValue(F32 val)
{
	val /= 10.0f;
	val = 1.0f - val;
	val *= val * val;
//	val *= val;
	mDensitySliderValue = val;
}

inline LLUUID LLWaterParamManager::getNormalMapID()
{	
	return mCurParams.mParamValues["normalMap"].asUUID();
}

inline bool LLWaterParamManager::setNormalMapID(const LLUUID& id)
{
	mCurParams.mParamValues["normalMap"] = id;
	return true;
}

inline LLVector2 LLWaterParamManager::getWave1Dir(void)
{
	bool err;
	return mCurParams.getVector2("wave1Dir", err);
}

inline LLVector2 LLWaterParamManager::getWave2Dir(void)
{
	bool err;
	return mCurParams.getVector2("wave2Dir", err);
}

inline F32 LLWaterParamManager::getScaleAbove(void)
{
	bool err;
	return mCurParams.getFloat("scaleAbove", err);
}

inline F32 LLWaterParamManager::getScaleBelow(void)
{
	bool err;
	return mCurParams.getFloat("scaleBelow", err);
}

inline LLVector3 LLWaterParamManager::getNormalScale(void)
{
	bool err;
	return mCurParams.getVector3("normScale", err);
}

inline F32 LLWaterParamManager::getFresnelScale(void)
{
	bool err;
	return mCurParams.getFloat("fresnelScale", err);
}

inline F32 LLWaterParamManager::getFresnelOffset(void)
{
	bool err;
	return mCurParams.getFloat("fresnelOffset", err);
}

inline F32 LLWaterParamManager::getBlurMultiplier(void)
{
	bool err;
	return mCurParams.getFloat("blurMultiplier", err);
}

inline LLColor4 LLWaterParamManager::getFogColor(void)
{
	bool err;
	return LLColor4(mCurParams.getVector4("waterFogColor", err));
}

#endif
