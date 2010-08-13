/**
 * @file llwlanimator.h
 * @brief Interface for the LLWLAnimator class.
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

#ifndef LL_WL_ANIMATOR_H
#define LL_WL_ANIMATOR_H

#include "llwlparamset.h"
#include <string>
#include <map>

class LLWLAnimator {
public:
	F64 mStartTime;
	F32 mDayRate;
	F64 mDayTime;
	
	// track to play
	std::map<F32, std::string> mTimeTrack;
	std::map<F32, std::string>::iterator mFirstIt, mSecondIt;

	// params to use
	//std::map<std::string, LLWLParamSet> mParamList;

	bool mIsRunning;
	bool mUseLindenTime;

	// simple constructor
	LLWLAnimator();

	// update the parameters
	void update(LLWLParamSet& curParams);

	// get time in seconds
	//F64 getTime(void);

	// returns a float 0 - 1 saying what time of day is it?
	F64 getDayTime(void);

	// sets a float 0 - 1 saying what time of day it is
	void setDayTime(F64 dayTime);

	// set an animation track
	void setTrack(std::map<F32, std::string>& track,
		F32 dayRate, F64 dayTime = 0, bool run = true);

};

#endif // LL_WL_ANIMATOR_H
