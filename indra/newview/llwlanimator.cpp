/**
 * @file llwlanimator.cpp
 * @brief Implementation for the LLWLAnimator class.
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007-2007, Linden Research, Inc.
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
