/**
 * @file llwlparamset.h
 * @brief Interface for the LLWaterParamSet class.
 *
 * $LicenseInfo:firstyear=2005&license=viewergpl$
 * 
 * Copyright (c) 2005-2009, Linden Research, Inc.
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

#ifndef LL_WATER_PARAM_SET_H
#define LL_WATER_PARAM_SET_H

#include <string>
#include <map>

#include "v4math.h"
#include "v4color.h"
#include "llviewershadermgr.h"

class LLFloaterWater;
class LLWaterParamSet;

/// A class representing a set of parameter values for the Water shaders.
class LLWaterParamSet 
{
	friend class LLWaterParamManager;

public:
	std::string mName;	
	
private:

	LLSD mParamValues;

public:

	LLWaterParamSet();

	/// Bind this set of parameter values to the uniforms of a particular shader.
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
	LLVector4 getVector4(const std::string& paramName, bool& error);

	/// Get a float3 parameter.
	/// \param paramName	The name of the parameter to set.
	/// \param error		A flag to set if it's not the proper return type
	LLVector3 getVector3(const std::string& paramName, bool& error);
	
	/// Get a float2 parameter.
	/// \param paramName	The name of the parameter to set.
	/// \param error		A flag to set if it's not the proper return type
	LLVector2 getVector2(const std::string& paramName, bool& error);
	
	/// Get an integer parameter
	/// \param paramName	The name of the parameter to set.
	/// \param error		A flag to set if it's not the proper return type	
	F32 getFloat(const std::string& paramName, bool& error);
		
	/// interpolate two parameter sets
	/// \param src			The parameter set to start with
	/// \param dest			The parameter set to end with
	/// \param weight		The amount to interpolate
	void mix(LLWaterParamSet& src, LLWaterParamSet& dest, 
		F32 weight);

};

inline void LLWaterParamSet::setAll(const LLSD& val)
{
	if(val.isMap()) {
		LLSD::map_const_iterator mIt = val.beginMap();
		for(; mIt != val.endMap(); mIt++)
		{
			mParamValues[mIt->first] = mIt->second;
		}
	}
}

inline const LLSD& LLWaterParamSet::getAll()
{
	return mParamValues;
}

#endif // LL_WaterPARAM_SET_H
