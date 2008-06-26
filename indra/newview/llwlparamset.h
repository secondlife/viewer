/**
 * @file llwlparamset.h
 * @brief Interface for the LLWLParamSet class.
 *
 * $LicenseInfo:firstyear=2005&license=viewergpl$
 * 
 * Copyright (c) 2005-2007, Linden Research, Inc.
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

#ifndef LL_WLPARAM_SET_H
#define LL_WLPARAM_SET_H

#include <string>
#include <map>

#include "v4math.h"
#include "v4color.h"
#include "llglslshader.h"

class LLFloaterWindLight;
class LLWLParamSet;

/// A class representing a set of parameter values for the WindLight shaders.
class LLWLParamSet {

	friend class LLWLParamManager;

public:
	std::string mName;	
	
private:

	LLSD mParamValues;
	
	float mCloudScrollXOffset, mCloudScrollYOffset;

public:

	LLWLParamSet();

	/// Update this set of shader uniforms from the parameter values.
	void update(LLGLSLShader * shader) const;

	/// set the total llsd
	void setAll(const LLSD& val);
	
	/// get the total llsd
	const LLSD& getAll();		
	

	/// Set a float parameter.
	/// \param paramName	The name of the parameter to set.
	/// \param x			The float value to set.
	void set(const std::string& paramName, float x);

	/// Set a float2 parameter.
	/// \param paramName	The name of the parameter to set.
	/// \param x			The x component's value to set.
	/// \param y			The y component's value to set.
	void set(const std::string& paramName, float x, float y);

	/// Set a float3 parameter.
	/// \param paramName	The name of the parameter to set.
	/// \param x			The x component's value to set.
	/// \param y			The y component's value to set.
	/// \param z			The z component's value to set.
	void set(const std::string& paramName, float x, float y, float z);

	/// Set a float4 parameter.
	/// \param paramName	The name of the parameter to set.
	/// \param x			The x component's value to set.
	/// \param y			The y component's value to set.
	/// \param z			The z component's value to set.
	/// \param w			The w component's value to set.
	void set(const std::string& paramName, float x, float y, float z, float w);

	/// Set a float4 parameter.
	/// \param paramName	The name of the parameter to set.
	/// \param val			An array of the 4 float values to set the parameter to.
	void set(const std::string& paramName, const float * val);

	/// Set a float4 parameter.
	/// \param paramName	The name of the parameter to set.
	/// \param val			A struct of the 4 float values to set the parameter to.
	void set(const std::string& paramName, const LLVector4 & val);

	/// Set a float4 parameter.
	/// \param paramName	The name of the parameter to set.
	/// \param val			A struct of the 4 float values to set the parameter to.
	void set(const std::string& paramName, const LLColor4 & val);

	/// Get a float4 parameter.
	/// \param paramName	The name of the parameter to set.
	/// \param error		A flag to set if it's not the proper return type
	LLVector4 getVector(const std::string& paramName, bool& error);

	/// Get an integer parameter
	/// \param paramName	The name of the parameter to set.
	/// \param error		A flag to set if it's not the proper return type	
	F32 getFloat(const std::string& paramName, bool& error);
	
	
	// specific getters and setters
	
	
	/// set the star's brightness
	/// \param val brightness value
	void setStarBrightness(F32 val);
	
	/// get the star brightness value;
	F32 getStarBrightness();	
	
	void setSunAngle(F32 val);
	F32 getSunAngle();	
	
	void setEastAngle(F32 val);
	F32 getEastAngle();	
	
							
	
	/// set the cloud scroll x enable value
	/// \param val scroll x value	
	void setEnableCloudScrollX(bool val);

	/// get the scroll x enable value;	
	bool getEnableCloudScrollX();
	
	/// set the star's brightness
	/// \param val scroll y bool value		
	void setEnableCloudScrollY(bool val);	

	/// get the scroll enable y value;
	bool getEnableCloudScrollY();
	
	/// set the cloud scroll x enable value
	/// \param val scroll x value	
	void setCloudScrollX(F32 val);

	/// get the scroll x enable value;	
	F32 getCloudScrollX();
	
	/// set the star's brightness
	/// \param val scroll y bool value		
	void setCloudScrollY(F32 val);	

	/// get the scroll enable y value;
	F32 getCloudScrollY();	

	/// interpolate two parameter sets
	/// \param src			The parameter set to start with
	/// \param dest			The parameter set to end with
	/// \param weight		The amount to interpolate
	void mix(LLWLParamSet& src, LLWLParamSet& dest, 
		F32 weight);

	void updateCloudScrolling(void);
};

inline void LLWLParamSet::setAll(const LLSD& val)
{
	if(val.isMap()) {
		mParamValues = val;
	}
}

inline const LLSD& LLWLParamSet::getAll()
{
	return mParamValues;
}

inline void LLWLParamSet::setStarBrightness(float val) {
	mParamValues["star_brightness"] = val;
}

inline F32 LLWLParamSet::getStarBrightness() {
	return (F32) mParamValues["star_brightness"].asReal();
}

inline F32 LLWLParamSet::getSunAngle() {
	return (F32) mParamValues["sun_angle"].asReal();
}

inline F32 LLWLParamSet::getEastAngle() {
	return (F32) mParamValues["east_angle"].asReal();
}


inline void LLWLParamSet::setEnableCloudScrollX(bool val) {
	mParamValues["enable_cloud_scroll"][0] = val;
}

inline bool LLWLParamSet::getEnableCloudScrollX() {
	return mParamValues["enable_cloud_scroll"][0].asBoolean();
}

inline void LLWLParamSet::setEnableCloudScrollY(bool val) {
	mParamValues["enable_cloud_scroll"][1] = val;
}

inline bool LLWLParamSet::getEnableCloudScrollY() {
	return mParamValues["enable_cloud_scroll"][1].asBoolean();
}


inline void LLWLParamSet::setCloudScrollX(F32 val) {
	mParamValues["cloud_scroll_rate"][0] = val;
}

inline F32 LLWLParamSet::getCloudScrollX() {
	return (F32) mParamValues["cloud_scroll_rate"][0].asReal();
}

inline void LLWLParamSet::setCloudScrollY(F32 val) {
	mParamValues["cloud_scroll_rate"][1] = val;
}

inline F32 LLWLParamSet::getCloudScrollY() {
	return (F32) mParamValues["cloud_scroll_rate"][1].asReal();
}


#endif // LL_WLPARAM_SET_H
