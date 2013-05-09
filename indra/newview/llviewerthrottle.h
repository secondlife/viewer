/** 
 * @file llviewerthrottle.h
 * @brief LLViewerThrottle class header file
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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
