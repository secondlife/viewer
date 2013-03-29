/*
 *  LLCalc.cpp
 * Copyright 2008 Aimee Walton.
 * $LicenseInfo:firstyear=2008&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2008, Linden Research, Inc.
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
 *
 */

#include "linden_common.h"

#include "llcalc.h"

#include "llcalcparser.h"
#include "llmath.h"


// Variable names for use in the build floater
const char* LLCalc::X_POS = "PX";
const char* LLCalc::Y_POS = "PY";
const char* LLCalc::Z_POS = "PZ";
const char* LLCalc::X_SCALE = "SX";
const char* LLCalc::Y_SCALE = "SY";
const char* LLCalc::Z_SCALE = "SZ";
const char* LLCalc::X_ROT = "RX";
const char* LLCalc::Y_ROT = "RY";
const char* LLCalc::Z_ROT = "RZ";
const char* LLCalc::HOLLOW = "HLW";
const char* LLCalc::CUT_BEGIN = "CB";
const char* LLCalc::CUT_END = "CE";
const char* LLCalc::PATH_BEGIN = "PB";
const char* LLCalc::PATH_END = "PE";
const char* LLCalc::TWIST_BEGIN = "TB";
const char* LLCalc::TWIST_END = "TE";
const char* LLCalc::X_SHEAR = "SHX";
const char* LLCalc::Y_SHEAR = "SHY";
const char* LLCalc::X_TAPER = "TPX";
const char* LLCalc::Y_TAPER = "TPY";
const char* LLCalc::RADIUS_OFFSET = "ROF";
const char* LLCalc::REVOLUTIONS = "REV";
const char* LLCalc::SKEW = "SKW";
const char* LLCalc::X_HOLE = "HLX";
const char* LLCalc::Y_HOLE = "HLY";
const char* LLCalc::TEX_U_SCALE = "TSU";
const char* LLCalc::TEX_V_SCALE = "TSV";
const char* LLCalc::TEX_U_OFFSET = "TOU";
const char* LLCalc::TEX_V_OFFSET = "TOV";
const char* LLCalc::TEX_ROTATION = "TROT";
const char* LLCalc::TEX_TRANSPARENCY = "TRNS";
const char* LLCalc::TEX_GLOW = "GLOW";


LLCalc* LLCalc::sInstance = NULL;

LLCalc::LLCalc() : mLastErrorPos(0)
{
	// Init table of constants
	mConstants["PI"] = F_PI;
	mConstants["TWO_PI"] = F_TWO_PI;
	mConstants["PI_BY_TWO"] = F_PI_BY_TWO;
	mConstants["SQRT_TWO_PI"] = F_SQRT_TWO_PI;
	mConstants["SQRT2"] = F_SQRT2;
	mConstants["SQRT3"] = F_SQRT3;
	mConstants["DEG_TO_RAD"] = DEG_TO_RAD;
	mConstants["RAD_TO_DEG"] = RAD_TO_DEG;
	mConstants["GRAVITY"] = GRAVITY;
}

LLCalc::~LLCalc()
{
}

//static
void LLCalc::cleanUp()
{
	delete sInstance;
	sInstance = NULL;
}

//static
LLCalc* LLCalc::getInstance()
{
    if (!sInstance)	sInstance = new LLCalc();
	return sInstance;
}

void LLCalc::setVar(const std::string& name, const F32& value)
{
	mVariables[name] = value;
}

void LLCalc::clearVar(const std::string& name)
{
	mVariables.erase(name);
}

void LLCalc::clearAllVariables()
{
	mVariables.clear();
}

/*
void LLCalc::updateVariables(LLSD& vars)
{
	LLSD::map_iterator cIt = vars.beginMap();
	for(; cIt != vars.endMap(); cIt++)
	{
		setVar(cIt->first, (F32)(LLSD::Real)cIt->second);
	}
}
*/

bool LLCalc::evalString(const std::string& expression, F32& result)
{
	std::string expr_upper = expression;
	LLStringUtil::toUpper(expr_upper);
	
	LLCalcParser calc(result, &mConstants, &mVariables);

	mLastErrorPos = 0;
	std::string::iterator start = expr_upper.begin();
 	parse_info<std::string::iterator> info;
	
	try
	{
		info = parse(start, expr_upper.end(), calc, space_p);
		lldebugs << "Math expression: " << expression << " = " << result << llendl;
	}
	catch(parser_error<std::string, std::string::iterator> &e)
	{
		mLastErrorPos = e.where - expr_upper.begin();
		
		llinfos << "Calc parser exception: " << e.descriptor << " at " << mLastErrorPos << " in expression: " << expression << llendl;
		return false;
	}
	
	if (!info.full)
	{
		mLastErrorPos = info.stop - expr_upper.begin();
		llinfos << "Unhandled syntax error at " << mLastErrorPos << " in expression: " << expression << llendl;
		return false;
	}
	
	return true;
}
