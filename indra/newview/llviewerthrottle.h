/** 
 * @file llviewerthrottle.h
 * @brief LLViewerThrottle class header file
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
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

	static const char* sNames[TC_EOF];		/* Flawfinder: ignore */
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
