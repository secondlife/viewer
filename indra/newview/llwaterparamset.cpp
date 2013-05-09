/**
 * @file llwaterparamset.cpp
 * @brief Implementation for the LLWaterParamSet class.
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
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

#include "llviewerprecompiledheaders.h"

#include "llwaterparamset.h"
#include "llsd.h"

#include "llwaterparammanager.h"
#include "lluictrlfactory.h"
#include "llsliderctrl.h"
#include "llviewertexturelist.h"
#include "llviewercontrol.h"
#include "lluuid.h"

#include <llgl.h>

#include <sstream>

LLWaterParamSet::LLWaterParamSet(void) :
	mName("Unnamed Preset")
{
	LLSD vec4;
	LLSD vec3;
	LLSD real(0.0f);

	vec4 = LLSD::emptyArray();
	vec4.append(22.f/255.f);
	vec4.append(43.f/255.f);
	vec4.append(54.f/255.f);
	vec4.append(0.f/255.f);

	vec3 = LLSD::emptyArray();
	vec3.append(2);
	vec3.append(2);
	vec3.append(2);

	LLSD wave1, wave2;
	wave1 = LLSD::emptyArray();
	wave2 = LLSD::emptyArray();
	wave1.append(0.5f);
	wave1.append(-.17f);
	wave2.append(0.58f);
	wave2.append(-.67f);

	mParamValues.insert("waterFogColor", vec4);
	mParamValues.insert("waterFogDensity", 16.0f);
	mParamValues.insert("underWaterFogMod", 0.25f);
	mParamValues.insert("normScale", vec3);
	mParamValues.insert("fresnelScale", 0.5f);
	mParamValues.insert("fresnelOffset", 0.4f);
	mParamValues.insert("scaleAbove", 0.025f);
	mParamValues.insert("scaleBelow", 0.2f);
	mParamValues.insert("blurMultiplier", 0.01f);
	mParamValues.insert("wave1Dir", wave1);
	mParamValues.insert("wave2Dir", wave2);
	mParamValues.insert("normalMap", DEFAULT_WATER_NORMAL);

}

void LLWaterParamSet::set(const std::string& paramName, float x) 
{	
	// handle case where no array
	if(mParamValues[paramName].isReal()) 
	{
		mParamValues[paramName] = x;
	} 
	
	// handle array
	else if(mParamValues[paramName].isArray() &&
			mParamValues[paramName][0].isReal())
	{
		mParamValues[paramName][0] = x;
	}
}

void LLWaterParamSet::set(const std::string& paramName, float x, float y) {
	mParamValues[paramName][0] = x;
	mParamValues[paramName][1] = y;
}

void LLWaterParamSet::set(const std::string& paramName, float x, float y, float z)
{
	mParamValues[paramName][0] = x;
	mParamValues[paramName][1] = y;
	mParamValues[paramName][2] = z;
}

void LLWaterParamSet::set(const std::string& paramName, float x, float y, float z, float w) 
{
	mParamValues[paramName][0] = x;
	mParamValues[paramName][1] = y;
	mParamValues[paramName][2] = z;
	mParamValues[paramName][3] = w;
}

void LLWaterParamSet::set(const std::string& paramName, const float * val) 
{
	mParamValues[paramName][0] = val[0];
	mParamValues[paramName][1] = val[1];
	mParamValues[paramName][2] = val[2];
	mParamValues[paramName][3] = val[3];
}

void LLWaterParamSet::set(const std::string& paramName, const LLVector4 & val) 
{
	mParamValues[paramName][0] = val.mV[0];
	mParamValues[paramName][1] = val.mV[1];
	mParamValues[paramName][2] = val.mV[2];
	mParamValues[paramName][3] = val.mV[3];
}

void LLWaterParamSet::set(const std::string& paramName, const LLColor4 & val) 
{
	mParamValues[paramName][0] = val.mV[0];
	mParamValues[paramName][1] = val.mV[1];
	mParamValues[paramName][2] = val.mV[2];
	mParamValues[paramName][3] = val.mV[3];
}

LLVector4 LLWaterParamSet::getVector4(const std::string& paramName, bool& error) 
{
	
	// test to see if right type
	LLSD cur_val = mParamValues.get(paramName);
	if (!cur_val.isArray() || cur_val.size() != 4) 
	{
		error = true;
		return LLVector4(0,0,0,0);
	}
	
	LLVector4 val;
	val.mV[0] = (F32) cur_val[0].asReal();
	val.mV[1] = (F32) cur_val[1].asReal();
	val.mV[2] = (F32) cur_val[2].asReal();
	val.mV[3] = (F32) cur_val[3].asReal();
	
	error = false;
	return val;
}

LLVector3 LLWaterParamSet::getVector3(const std::string& paramName, bool& error) 
{
	
	// test to see if right type
	LLSD cur_val = mParamValues.get(paramName);
	if (!cur_val.isArray()|| cur_val.size() != 3) 
	{
		error = true;
		return LLVector3(0,0,0);
	}
	
	LLVector3 val;
	val.mV[0] = (F32) cur_val[0].asReal();
	val.mV[1] = (F32) cur_val[1].asReal();
	val.mV[2] = (F32) cur_val[2].asReal();
	
	error = false;
	return val;
}

LLVector2 LLWaterParamSet::getVector2(const std::string& paramName, bool& error) 
{
	// test to see if right type
	LLSD cur_val = mParamValues.get(paramName);
	if (!cur_val.isArray() || cur_val.size() != 2) 
	{
		error = true;
		return LLVector2(0,0);
	}
	
	LLVector2 val;
	val.mV[0] = (F32) cur_val[0].asReal();
	val.mV[1] = (F32) cur_val[1].asReal();
	
	error = false;
	return val;
}

F32 LLWaterParamSet::getFloat(const std::string& paramName, bool& error) 
{
	
	// test to see if right type
	LLSD cur_val = mParamValues.get(paramName);
	if (cur_val.isArray() && cur_val.size() != 0)
	{
		error = false;
		return (F32) cur_val[0].asReal();	
	}
	
	if(cur_val.isReal())
	{
		error = false;
		return (F32) cur_val.asReal();
	}
	
	error = true;
	return 0;
}

// Added for interpolation effect in DEV-33645
// Based on LLWLParamSet::mix, but written by Jacob without an intimate knowledge of how WindLight works.
// The function definition existed in the header but was never implemented.  If you think there is something
// wrong with this, you're probably right.  Ask Jacob, Q, or a member of the original WindLight team.
void LLWaterParamSet::mix(LLWaterParamSet& src, LLWaterParamSet& dest, F32 weight)
{
	// Setup
	LLSD srcVal, destVal;													// LLSD holders for get/set calls, reusable

	// Iterate through values
	for(LLSD::map_iterator iter = mParamValues.beginMap(); iter != mParamValues.endMap(); ++iter)
	{
		// If param exists in both src and dest, set the holder variables, otherwise skip
		if(src.mParamValues.has(iter->first) && dest.mParamValues.has(iter->first))
		{
			srcVal = src.mParamValues[iter->first];
			destVal = dest.mParamValues[iter->first];
		}
		else
		{
			continue;
		}
		
		if(iter->second.isReal())									// If it's a real, interpolate directly
		{
			iter->second = srcVal.asReal() + ((destVal.asReal() - srcVal.asReal()) * weight);
		}		
		else if(iter->second.isArray() && iter->second[0].isReal()	// If it's an array of reals, loop through the reals and interpolate on those
				&& iter->second.size() == srcVal.size() && iter->second.size() == destVal.size())
		{
			// Actually do interpolation: old value + (difference in values * factor)
			for(int i=0; i < iter->second.size(); ++i) 
			{
				// iter->second[i] = (1.f-weight)*(F32)srcVal[i].asReal() + weight*(F32)destVal[i].asReal();	// old way of doing it -- equivalent but one more operation
				iter->second[i] = srcVal[i].asReal() + ((destVal[i].asReal() - srcVal[i].asReal()) * weight);
			}
		}
		else														// Else, skip
		{
			continue;
		}
	}
}
