/**
 * @file llwlanimator.cpp
 * @brief Implementation for the LLWLAnimator class.
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

#include "llviewerprecompiledheaders.h"

#include "llwlanimator.h"
#include "llsky.h"
#include "pipeline.h"
#include "llwlparammanager.h"
#include "llwaterparammanager.h"

extern LLControlGroup gSavedSettings;

F64 LLWLAnimator::INTERP_TOTAL_SECONDS = 3.f;

LLWLAnimator::LLWLAnimator() : mStartTime(0.f), mDayRate(1.f), mDayTime(0.f),
							mIsRunning(FALSE), mIsInterpolating(FALSE), mTimeType(TIME_LINDEN),
							mInterpStartTime(), mInterpEndTime()
{
	mInterpBeginWL = new LLWLParamSet();
	mInterpBeginWater = new LLWaterParamSet();
	mInterpEndWater = new LLWaterParamSet();
}

void LLWLAnimator::update(LLWLParamSet& curParams)
{
	//llassert(mUseLindenTime != mUseLocalTime);

	F64 curTime;
	curTime = getDayTime();

	// don't do anything if empty
	if(mTimeTrack.size() == 0)
	{
		return;
	}

	// start it off
	mFirstIt = mTimeTrack.begin();
	mSecondIt = mTimeTrack.begin();
	mSecondIt++;

	// grab the two tween iterators
	while(mSecondIt != mTimeTrack.end() && curTime > mSecondIt->first)
	{
		mFirstIt++;
		mSecondIt++;
	}

	// scroll it around when you get to the end
	if(mSecondIt == mTimeTrack.end() || mFirstIt->first > curTime)
	{
		mSecondIt = mTimeTrack.begin();
		mFirstIt = mTimeTrack.end();
		mFirstIt--;
	}

	F32 weight = 0;

	if(mFirstIt->first < mSecondIt->first)
	{
	
		// get the delta time and the proper weight
		weight = F32 (curTime - mFirstIt->first) / 
			(mSecondIt->first - mFirstIt->first);
	
	// handle the ends
	}
	else if(mFirstIt->first > mSecondIt->first)
	{
		
		// right edge of time line
		if(curTime >= mFirstIt->first)
		{
			weight = F32 (curTime - mFirstIt->first) /
			((1 + mSecondIt->first) - mFirstIt->first);
		// left edge of time line
		}
		else
		{
			weight = F32 ((1 + curTime) - mFirstIt->first) /
			((1 + mSecondIt->first) - mFirstIt->first);
		}

	// handle same as whatever the last one is
	}
	else
	{
		weight = 1;
	}

	if(mIsInterpolating)
	{
		// *TODO_JACOB: this is kind of laggy.  Not sure why.  The part that lags is the curParams.mix call, and none of the other mixes.  It works, though.
		clock_t current = clock();
		if(current >= mInterpEndTime)
		{
			mIsInterpolating = false;
			return;
		}
		
		// determine moving target for final interpolation value
		// *TODO: this will not work with lazy loading of sky presets.
		LLWLParamSet buf = LLWLParamSet();
		buf.setAll(LLWLParamManager::getInstance()->mParamList[mFirstIt->second].getAll());	// just give it some values, otherwise it has no params to begin with (see comment in constructor)
		buf.mix(LLWLParamManager::getInstance()->mParamList[mFirstIt->second], LLWLParamManager::getInstance()->mParamList[mSecondIt->second], weight);	// mix to determine moving target for interpolation finish (as below)

		// mix from previous value to moving target
		weight = (current - mInterpStartTime) / (INTERP_TOTAL_SECONDS * CLOCKS_PER_SEC);
		curParams.mix(*mInterpBeginWL, buf, weight);
		
		// mix water
		LLWaterParamManager::getInstance()->mCurParams.mix(*mInterpBeginWater, *mInterpEndWater, weight);
	}
	else
	{
	// do the interpolation and set the parameters
		// *TODO: this will not work with lazy loading of sky presets.
		curParams.mix(LLWLParamManager::getInstance()->mParamList[mFirstIt->second], LLWLParamManager::getInstance()->mParamList[mSecondIt->second], weight);
	}
}

F64 LLWLAnimator::getDayTime()
{
	if(!mIsRunning)
	{
		return mDayTime;
	}
	else if(mTimeType == TIME_LINDEN)
	{
		F32 phase = gSky.getSunPhase() / F_PI;

		// we're not solving the non-linear equation that determines sun phase
		// we're just linearly interpolating between the major points
		if (phase <= 5.0 / 4.0) {
			mDayTime = (1.0 / 3.0) * phase + (1.0 / 3.0);
		}
		else
		{
			mDayTime = phase - (1.0 / 2.0);
		}

		if(mDayTime > 1)
		{
			mDayTime--;
		}

		return mDayTime;
	}
	else if(mTimeType == TIME_LOCAL)
	{
		return getLocalTime();
	}

	// get the time;
	mDayTime = (LLTimer::getElapsedSeconds() - mStartTime) / mDayRate;

	// clamp it
	if(mDayTime < 0)
	{
		mDayTime = 0;
	} 
	while(mDayTime > 1)
	{
		mDayTime--;
	}

	return (F32)mDayTime;
}

void LLWLAnimator::setDayTime(F64 dayTime)
{
	//retroactively set start time;
	mStartTime = LLTimer::getElapsedSeconds() - dayTime * mDayRate;
	mDayTime = dayTime;

	// clamp it
	if(mDayTime < 0)
	{
		mDayTime = 0;
	}
	else if(mDayTime > 1)
	{
		mDayTime = 1;
	}
}


void LLWLAnimator::setTrack(std::map<F32, LLWLParamKey>& curTrack,
							F32 dayRate, F64 dayTime, bool run)
{
	mTimeTrack = curTrack;
	mDayRate = dayRate;
	setDayTime(dayTime);

	mIsRunning = run;
}

void LLWLAnimator::startInterpolation(const LLSD& targetWater)
{
	mInterpBeginWL->setAll(LLWLParamManager::getInstance()->mCurParams.getAll());
	mInterpBeginWater->setAll(LLWaterParamManager::getInstance()->mCurParams.getAll());
	
	mInterpStartTime = clock();
	mInterpEndTime = mInterpStartTime + clock_t(INTERP_TOTAL_SECONDS) * CLOCKS_PER_SEC;

	// Don't set any ending WL -- this is continuously calculated as the animator updates since it's a moving target
	mInterpEndWater->setAll(targetWater);

	mIsInterpolating = true;
}

std::string LLWLAnimator::timeToString(F32 curTime)
{
	S32 hours;
	S32 min;
	bool isPM = false;

	// get hours and minutes
	hours = (S32) (24.0 * curTime);
	curTime -= ((F32) hours / 24.0f);
	min = llround(24.0f * 60.0f * curTime);

	// handle case where it's 60
	if(min == 60) 
	{
		hours++;
		min = 0;
	}

	// set for PM
	if(hours >= 12 && hours < 24)
	{
		isPM = true;
	}

	// convert to non-military notation
	if(hours >= 24) 
	{
		hours = 12;
	} 
	else if(hours > 12) 
	{
		hours -= 12;
	} 
	else if(hours == 0) 
	{
		hours = 12;
	}

	// make the string
	std::stringstream newTime;
	newTime << hours << ":";
	
	// double 0
	if(min < 10) 
	{
		newTime << 0;
	}
	
	// finish it
	newTime << min << " ";
	if(isPM) 
	{
		newTime << "PM";
	} 
	else 
	{
		newTime << "AM";
	}

	return newTime.str();
}

F64 LLWLAnimator::getLocalTime()
{
	char buffer[9];
	time_t rawtime;
	struct tm* timeinfo;

	time(&rawtime);
	timeinfo = localtime(&rawtime);
	strftime(buffer, 9, "%H:%M:%S", timeinfo);
	std::string timeStr(buffer);

	F64 tod = ((F64)atoi(timeStr.substr(0,2).c_str())) / 24.f +
			  ((F64)atoi(timeStr.substr(3,2).c_str())) / 1440.f + 
			  ((F64)atoi(timeStr.substr(6,2).c_str())) / 86400.f;
	return tod;
}
