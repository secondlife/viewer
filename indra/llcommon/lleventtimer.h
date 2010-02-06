/** 
 * @file lleventtimer.h
 * @brief Cross-platform objects for doing timing 
 *
 * $LicenseInfo:firstyear=2000&license=viewergpl$
 * 
 * Copyright (c) 2000-2009, Linden Research, Inc.
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

#ifndef LL_EVENTTIMER_H					
#define LL_EVENTTIMER_H

#include "stdtypes.h"
#include "lldate.h"
#include "llinstancetracker.h"
#include "lltimer.h"

// class for scheduling a function to be called at a given frequency (approximate, inprecise)
class LL_COMMON_API LLEventTimer : protected LLInstanceTracker<LLEventTimer>
{
public:
	LLEventTimer(F32 period);	// period is the amount of time between each call to tick() in seconds
	LLEventTimer(const LLDate& time);
	virtual ~LLEventTimer();
	
	//function to be called at the supplied frequency
	// Normally return FALSE; TRUE will delete the timer after the function returns.
	virtual BOOL tick() = 0;

	static void updateClass();

protected:
	LLTimer mEventTimer;
	F32 mPeriod;
	static bool sInTickLoop;
};

#endif //LL_EVENTTIMER_H
