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

LLWLAnimator::LLWLAnimator() : mStartTime(0), mDayRate(1), mDayTime(0),
	mIsRunning(FALSE), mUseLindenTime(false)
{
	mDayTime = 0;
}

void LLWLAnimator::update(LLWLParamSet& curParams)
{
	F64 curTime;
	curTime = getDayTime();

	// don't do anything if empty
	if(mTimeTrack.size() == 0) {
		return;
	}

	// start it off
	mFirstIt = mTimeTrack.begin();
	mSecondIt = mTimeTrack.begin();
	mSecondIt++;

	// grab the two tween iterators
	while(mSecondIt != mTimeTrack.end() && curTime > mSecondIt->first) {
		mFirstIt++;
		mSecondIt++;
	}

	// scroll it around when you get to the end
	if(mSecondIt == mTimeTrack.end() || mFirstIt->first > curTime) {
		mSecondIt = mTimeTrack.begin();
		mFirstIt = mTimeTrack.end();
		mFirstIt--;
	}

	F32 weight = 0;

	if(mFirstIt->first < mSecondIt->first) {
	
		// get the delta time and the proper weight
		weight = F32 (curTime - mFirstIt->first) / 
			(mSecondIt->first - mFirstIt->first);
	
	// handle the ends
	} else if(mFirstIt->first > mSecondIt->first) {
		
		// right edge of time line
		if(curTime >= mFirstIt->first) {
			weight = F32 (curTime - mFirstIt->first) /
			((1 + mSecondIt->first) - mFirstIt->first);
		
		// left edge of time line
		} else {
			weight = F32 ((1 + curTime) - mFirstIt->first) /
			((1 + mSecondIt->first) - mFirstIt->first);
		}

	
	// handle same as whatever the last one is
	} else {
		weight = 1;
	}

	// do the interpolation and set the parameters
	curParams.mix(LLWLParamManager::instance()->mParamList[mFirstIt->second], 
		LLWLParamManager::instance()->mParamList[mSecondIt->second], weight);
}

F64 LLWLAnimator::getDayTime()
{
	if(!mIsRunning) {
		return mDayTime;
	}

	if(mUseLindenTime) {

		F32 phase = gSky.getSunPhase() / F_PI;

		// we're not solving the non-linear equation that determines sun phase
		// we're just linearly interpolating between the major points
		if (phase <= 5.0 / 4.0) {
			mDayTime = (1.0 / 3.0) * phase + (1.0 / 3.0);
		} else {
			mDayTime = phase - (1.0 / 2.0);
		}

		if(mDayTime > 1) {
			mDayTime--;
		}

		return mDayTime;
	}

	// get the time;
	mDayTime = (LLTimer::getElapsedSeconds() - mStartTime) / mDayRate;

	// clamp it
	if(mDayTime < 0) {
		mDayTime = 0;
	} 
	while(mDayTime > 1) {
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
	if(mDayTime < 0) {
		mDayTime = 0;
	} else if(mDayTime > 1) {
		mDayTime = 1;
	}
}


void LLWLAnimator::setTrack(std::map<F32, std::string>& curTrack,
							F32 dayRate, F64 dayTime, bool run)
{
	mTimeTrack = curTrack;
	mDayRate = dayRate;
	setDayTime(dayTime);

	mIsRunning = run;
}
