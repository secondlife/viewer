/** 
 * @file llviewerthrottle.h
 * @brief LLViewerThrottle class header file
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2007, Linden Research, Inc.
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

#ifndef LL_LLVIEWERTHROTTLE_H
#define LL_LLVIEWERTHROTTLE_H

#include <vector>

#include "llstring.h"
#include "llframetimer.h"
#include "llthrottle.h"

class LLViewerThrottleGroup
{
	LLViewerThrottleGroup();
	LLViewerThrottleGroup(const F32 settings[TC_EOF]);

	LLViewerThrottleGroup operator*(const F32 frac) const;
	LLViewerThrottleGroup operator+(const LLViewerThrottleGroup &b) const;
	LLViewerThrottleGroup operator-(const LLViewerThrottleGroup &b) const;

	F32 getTotal()			{ return mThrottleTotal; }
	void sendToSim() const;

	void dump();

	friend class LLViewerThrottle;
protected:
	F32 mThrottles[TC_EOF];
	F32 mThrottleTotal;
};

class LLViewerThrottle
{
public:
	LLViewerThrottle();

	void setMaxBandwidth(F32 kbits_per_second, BOOL from_event = FALSE);

	void load();
	void save() const;
	void sendToSim() const;

	F32 getMaxBandwidth()const			{ return mMaxBandwidth; }
	F32 getCurrentBandwidth() const		{ return mCurrentBandwidth; }

	void updateDynamicThrottle();
	void resetDynamicThrottle();

	LLViewerThrottleGroup getThrottleGroup(const F32 bandwidth_kbps);

	static const std::string sNames[TC_EOF];
protected:
	F32 mMaxBandwidth;
	F32 mCurrentBandwidth;

	LLViewerThrottleGroup mCurrent;

	std::vector<LLViewerThrottleGroup> mPresets;
	
	LLFrameTimer mUpdateTimer;
	F32 mThrottleFrac;
};

extern LLViewerThrottle gViewerThrottle;

#endif // LL_LLVIEWERTHROTTLE_H
