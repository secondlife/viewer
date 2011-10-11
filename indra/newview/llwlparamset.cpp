/**
 * @file llwlparamset.cpp
 * @brief Implementation for the LLWLParamSet class.
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

#include "llwlparamset.h"
#include "llwlanimator.h"

#include "llwlparammanager.h"
#include "llglslshader.h"
#include "lluictrlfactory.h"
#include "llsliderctrl.h"

#include <llgl.h>

#include <sstream>

LLWLParamSet::LLWLParamSet(void) :
	mName("Unnamed Preset"),
	mCloudScrollXOffset(0.f), mCloudScrollYOffset(0.f)	
{
/* REMOVE or init the LLSD
	const std::map<std::string, LLVector4>::value_type hardcodedPreset[] = {
		std::make_pair("lightnorm",				LLVector4(0.f, 0.707f, -0.707f, 0.f)),
		std::make_pair("sunlight_color",		LLVector4(0.6f, 0.6f, 2.83f, 2.27f)),
		std::make_pair("ambient",				LLVector4(0.27f, 0.33f, 0.44f, 1.19f)),
		std::make_pair("blue_horizon",			LLVector4(0.3f, 0.4f, 0.9f, 1.f)),
		std::make_pair("blue_density",			LLVector4(0.3f, 0.4f, 0.8f, 1.f)),
		std::make_pair("haze_horizon",			LLVector4(0.6f, 0.6f, 0.6f, 1.f)),
		std::make_pair("haze_density",			LLVector4(0.3f, 0.3f, 0.3f, 1.f)),
		std::make_pair("cloud_shadow",			LLVector4(0.f, 0.f, 0.f, 0.f)),
		std::make_pair("density_multiplier",	LLVector4(0.001f, 0.001f, 0.001f, 0.001f)),
		std::make_pair("distance_multiplier",	LLVector4(1.f, 1.f, 1.f, 1.f)),
		std::make_pair("max_y",					LLVector4(600.f, 600.f, 600.f, 0.f)),
		std::make_pair("glow",					LLVector4(15.f, 0.001f, -0.03125f, 0.f)),
		std::make_pair("cloud_color",			LLVector4(0.0f, 0.0f, 0.0f, 0.0f)),
		std::make_pair("cloud_pos_density1",	LLVector4(0.f, 0.f, 0.f, 1.f)),
		std::make_pair("cloud_pos_density2",	LLVector4(0.f, 0.f, 0.f, 1.f)),
		std::make_pair("cloud_scale",			LLVector4(0.42f, 0.f, 0.f, 1.f)),
		std::make_pair("gamma",					LLVector4(2.0f, 2.0f, 2.0f, 0.0f)),
	};
	std::map<std::string, LLVector4>::value_type const * endHardcodedPreset = 
		hardcodedPreset + LL_ARRAY_SIZE(hardcodedPreset);

	mParamValues.insert(hardcodedPreset, endHardcodedPreset);
*/
}

static LLFastTimer::DeclareTimer FTM_WL_PARAM_UPDATE("WL Param Update");

void LLWLParamSet::update(LLGLSLShader * shader) const 
{	
	LLFastTimer t(FTM_WL_PARAM_UPDATE);

	for(LLSD::map_const_iterator i = mParamValues.beginMap();
		i != mParamValues.endMap();
		++i)
	{
		

		const std::string& param = i->first;
		
		if(	param == "star_brightness" || param == "preset_num" || param == "sun_angle" ||
			param == "east_angle" || param == "enable_cloud_scroll" ||
			param == "cloud_scroll_rate" || param == "lightnorm" ) 
		{
			continue;
		}
		
		if(param == "cloud_pos_density1") 
		{
			LLVector4 val;
			val.mV[0] = F32(i->second[0].asReal()) + mCloudScrollXOffset;
			val.mV[1] = F32(i->second[1].asReal()) + mCloudScrollYOffset;
			val.mV[2] = (F32) i->second[2].asReal();
			val.mV[3] = (F32) i->second[3].asReal();
			stop_glerror();
			shader->uniform4fv(param, 1, val.mV);
			stop_glerror();
		} 
		else // param is the uniform name
		{
			LLVector4 val;
			
			// handle all the different cases
			if(i->second.isArray() && i->second.size() == 4) 
			{
				val.mV[0] = (F32) i->second[0].asReal();
				val.mV[1] = (F32) i->second[1].asReal();
				val.mV[2] = (F32) i->second[2].asReal();
				val.mV[3] = (F32) i->second[3].asReal();															
			} 
			else if(i->second.isReal()) 
			{
				val.mV[0] = (F32) i->second.asReal();
			} 
			else if(i->second.isInteger()) 
			{
				val.mV[0] = (F32) i->second.asReal();
			} 
			else if(i->second.isBoolean())
			{
				val.mV[0] = i->second.asBoolean();
			}
			stop_glerror();
			shader->uniform4fv(param, 1, val.mV);
			stop_glerror();
		}
	}
}

void LLWLParamSet::set(const std::string& paramName, float x) 
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

void LLWLParamSet::set(const std::string& paramName, float x, float y) {
	mParamValues[paramName][0] = x;
	mParamValues[paramName][1] = y;
}

void LLWLParamSet::set(const std::string& paramName, float x, float y, float z) 
{
	mParamValues[paramName][0] = x;
	mParamValues[paramName][1] = y;
	mParamValues[paramName][2] = z;
}

void LLWLParamSet::set(const std::string& paramName, float x, float y, float z, float w) 
{
	mParamValues[paramName][0] = x;
	mParamValues[paramName][1] = y;
	mParamValues[paramName][2] = z;
	mParamValues[paramName][3] = w;
}

void LLWLParamSet::set(const std::string& paramName, const float * val) 
{
	mParamValues[paramName][0] = val[0];
	mParamValues[paramName][1] = val[1];
	mParamValues[paramName][2] = val[2];
	mParamValues[paramName][3] = val[3];
}

void LLWLParamSet::set(const std::string& paramName, const LLVector4 & val) 
{
	mParamValues[paramName][0] = val.mV[0];
	mParamValues[paramName][1] = val.mV[1];
	mParamValues[paramName][2] = val.mV[2];
	mParamValues[paramName][3] = val.mV[3];
}

void LLWLParamSet::set(const std::string& paramName, const LLColor4 & val) 
{
	mParamValues[paramName][0] = val.mV[0];
	mParamValues[paramName][1] = val.mV[1];
	mParamValues[paramName][2] = val.mV[2];
	mParamValues[paramName][3] = val.mV[3];
}

LLVector4 LLWLParamSet::getVector(const std::string& paramName, bool& error) 
{
	
	// test to see if right type
	LLSD cur_val = mParamValues.get(paramName);
	if (!cur_val.isArray()) 
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

F32 LLWLParamSet::getFloat(const std::string& paramName, bool& error) 
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



void LLWLParamSet::setSunAngle(float val) 
{
	// keep range 0 - 2pi
	if(val > F_TWO_PI || val < 0)
	{
		F32 num = val / F_TWO_PI;
		num -= floor(num);
		val = F_TWO_PI * num;
	}

	mParamValues["sun_angle"] = val;
}


void LLWLParamSet::setEastAngle(float val) 
{
	// keep range 0 - 2pi
	if(val > F_TWO_PI || val < 0)
	{
		F32 num = val / F_TWO_PI;
		num -= floor(num);
		val = F_TWO_PI * num;
	}

	mParamValues["east_angle"] = val;
}


void LLWLParamSet::mix(LLWLParamSet& src, LLWLParamSet& dest, F32 weight)
{
	// set up the iterators

	// keep cloud positions and coverage the same
	/// TODO masking will do this later
	F32 cloudPos1X = (F32) mParamValues["cloud_pos_density1"][0].asReal();
	F32 cloudPos1Y = (F32) mParamValues["cloud_pos_density1"][1].asReal();
	F32 cloudPos2X = (F32) mParamValues["cloud_pos_density2"][0].asReal();
	F32 cloudPos2Y = (F32) mParamValues["cloud_pos_density2"][1].asReal();
	F32 cloudCover = (F32) mParamValues["cloud_shadow"][0].asReal();

	LLSD srcVal;
	LLSD destVal;

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

	// now mix the extra parameters
	setStarBrightness((1 - weight) * (F32) src.getStarBrightness()
		+ weight * (F32) dest.getStarBrightness());

	llassert(src.getSunAngle() >= - F_PI && 
					src.getSunAngle() <= 3 * F_PI);
	llassert(dest.getSunAngle() >= - F_PI && 
					dest.getSunAngle() <= 3 * F_PI);
	llassert(src.getEastAngle() >= 0 && 
					src.getEastAngle() <= 4 * F_PI);
	llassert(dest.getEastAngle() >= 0 && 
					dest.getEastAngle() <= 4 * F_PI);

	// sun angle and east angle require some handling to make sure
	// they go in circles.  Yes quaternions would work better.
	F32 srcSunAngle = src.getSunAngle();
	F32 destSunAngle = dest.getSunAngle();
	F32 srcEastAngle = src.getEastAngle();
	F32 destEastAngle = dest.getEastAngle();
	
	if(fabsf(srcSunAngle - destSunAngle) > F_PI) 
	{
		if(srcSunAngle > destSunAngle) 
		{
			destSunAngle += 2 * F_PI;
		} 
		else 
		{
			srcSunAngle += 2 * F_PI;
		}
	}

	if(fabsf(srcEastAngle - destEastAngle) > F_PI) 
	{
		if(srcEastAngle > destEastAngle) 
		{
			destEastAngle += 2 * F_PI;
		} 
		else 
		{
			srcEastAngle += 2 * F_PI;
		}
	}

	setSunAngle((1 - weight) * srcSunAngle + weight * destSunAngle);
	setEastAngle((1 - weight) * srcEastAngle + weight * destEastAngle);
	
	// now setup the sun properly

	// reset those cloud positions
	mParamValues["cloud_pos_density1"][0] = cloudPos1X;
	mParamValues["cloud_pos_density1"][1] = cloudPos1Y;
	mParamValues["cloud_pos_density2"][0] = cloudPos2X;
	mParamValues["cloud_pos_density2"][1] = cloudPos2Y;
	mParamValues["cloud_shadow"][0] = cloudCover;
}

void LLWLParamSet::updateCloudScrolling(void) 
{
	static LLTimer s_cloud_timer;

	F64 delta_t = s_cloud_timer.getElapsedTimeAndResetF64();

	if(getEnableCloudScrollX())
	{
		mCloudScrollXOffset += F32(delta_t * (getCloudScrollX() - 10.f) / 100.f);
	}
	if(getEnableCloudScrollY())
	{
		mCloudScrollYOffset += F32(delta_t * (getCloudScrollY() - 10.f) / 100.f);
	}
}
