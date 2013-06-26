/** 
 * @file llstat.h
 * @brief Runtime statistics accumulation.
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#ifndef LL_LLSTAT_H
#define LL_LLSTAT_H

#include <map>

#include "lltimer.h"
#include "llframetimer.h"

class	LLSD;

// ----------------------------------------------------------------------------
class LL_COMMON_API LLStat
{
private:
	typedef std::multimap<std::string, LLStat*> stat_map_t;

	static stat_map_t& getStatList();

public:
	LLStat(std::string name = std::string(), S32 num_bins = 32, BOOL use_frame_timer = FALSE);
	~LLStat();

	void start();	// Start the timer for the current "frame", otherwise uses the time tracked from
					// the last addValue
	void reset();
	void addValue(const F32 value = 1.f); // Adds the current value being tracked, and tracks the DT.
	void addValue(const S32 value) { addValue((F32)value); }
	void addValue(const U32 value) { addValue((F32)value); }

	S32 getNextBin() const;
	
	F32 getPrev(S32 age) const;				// Age is how many "addValues" previously - zero is current
	F32 getPrevPerSec(S32 age) const;		// Age is how many "addValues" previously - zero is current
	F32 getCurrent() const;
	F32 getCurrentPerSec() const;
	
	F32 getMin() const;
	F32 getMinPerSec() const;
	F32 getMean() const;
	F32 getMeanPerSec() const;
	F32 getMeanDuration() const;
	F32 getMax() const;
	F32 getMaxPerSec() const;

	U32 getNumValues() const;
	S32 getNumBins() const;

	F64 getLastTime() const;
private:
	BOOL mUseFrameTimer;
	U32 mNumValues;
	U32 mNumBins;
	F32 mLastValue;
	F64 mLastTime;

	struct ValueEntry
	{
		ValueEntry()
		:	mValue(0.f),
			mBeginTime(0.0),
			mTime(0.0),
			mDT(0.f)
		{}
		F32 mValue;
		F64 mBeginTime;
		F64 mTime;
		F32 mDT;
	};
	ValueEntry* mBins;
	S32 mCurBin;
	S32 mNextBin;
	
	std::string mName;

	static LLTimer sTimer;
	static LLFrameTimer sFrameTimer;
	
public:
	static LLStat* getStat(const std::string& name)
	{
		// return the first stat that matches 'name'
		stat_map_t::iterator iter = getStatList().find(name);
		if (iter != getStatList().end())
			return iter->second;
		else
			return NULL;
	}
};
	
#endif // LL_STAT_
