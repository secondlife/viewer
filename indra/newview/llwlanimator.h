/**
 * @file llwlanimator.h
 * @brief Interface for the LLWLAnimator class.
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
