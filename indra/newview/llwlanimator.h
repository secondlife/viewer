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
#include "llwaterparamset.h"
#include <string>
#include <map>

struct LLWLParamKey;

class LLWLAnimator {
public:
	typedef enum e_time
	{
		TIME_LINDEN,
		TIME_LOCAL,
		TIME_CUSTOM
	} ETime;

	F64 mStartTime;
	F32 mDayRate;
	F64 mDayTime;
	
	// track to play
	std::map<F32, LLWLParamKey> mTimeTrack;
	std::map<F32, LLWLParamKey>::iterator mFirstIt, mSecondIt;

	// simple constructor
	LLWLAnimator();

	~LLWLAnimator()
	{
		delete mInterpBeginWL;
		delete mInterpBeginWater;
		delete mInterpEndWater;
	}

	// update the parameters
	void update(LLWLParamSet& curParams);

	// get time in seconds
	//F64 getTime(void);

	// returns a float 0 - 1 saying what time of day is it?
	F64 getDayTime(void);

	// sets a float 0 - 1 saying what time of day it is
	void setDayTime(F64 dayTime);

	// set an animation track
	void setTrack(std::map<F32, LLWLParamKey>& track,
		F32 dayRate, F64 dayTime = 0, bool run = true);

	void deactivate()
	{
		mIsRunning = false;
	}

	void activate(ETime time)
	{
		mIsRunning = true;
		mTimeType = time;
	}

	void startInterpolation(const LLSD& targetWater);

	bool getIsRunning()
	{
		return mIsRunning;
	}

	bool getUseCustomTime()
	{
		return mTimeType == TIME_CUSTOM;
	}

	bool getUseLocalTime()
	{
		return mTimeType == TIME_LOCAL;
	}

	bool getUseLindenTime()
	{
		return mTimeType == TIME_LINDEN;
	}

	void setTimeType(ETime time)
	{
		mTimeType = time;
	}

	ETime getTimeType()
	{
		return mTimeType;
	}

	/// convert the present time to a digital clock time
	static std::string timeToString(F32 curTime);

	/// get local time between 0 and 1
	static F64 getLocalTime();

private:
	ETime mTimeType;
	bool mIsRunning, mIsInterpolating;
	LLWLParamSet *mInterpBeginWL;
	LLWaterParamSet *mInterpBeginWater, *mInterpEndWater;
	clock_t mInterpStartTime, mInterpEndTime;

	static F64 INTERP_TOTAL_SECONDS;
};

#endif // LL_WL_ANIMATOR_H
