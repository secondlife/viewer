/** 
 * @file llstat.cpp
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

#include "linden_common.h"

#include "llstat.h"
#include "lllivefile.h"
#include "llerrorcontrol.h"
#include "llframetimer.h"
#include "timing.h"
#include "llsd.h"
#include "llsdserialize.h"
#include "llstl.h"
#include "u64.h"


// statics
LLStat::stat_map_t LLStat::sStatList;
//------------------------------------------------------------------------
LLTimer LLStat::sTimer;
LLFrameTimer LLStat::sFrameTimer;

void LLStat::init()
{
	llassert(mNumBins > 0);
	mNumValues = 0;
	mLastValue = 0.f;
	mLastTime = 0.f;
	mCurBin = (mNumBins-1);
	mNextBin = 0;
	mBins      = new F32[mNumBins];
	mBeginTime = new F64[mNumBins];
	mTime      = new F64[mNumBins];
	mDT        = new F32[mNumBins];
	for (U32 i = 0; i < mNumBins; i++)
	{
		mBins[i]      = 0.f;
		mBeginTime[i] = 0.0;
		mTime[i]      = 0.0;
		mDT[i]        = 0.f;
	}

	if (!mName.empty())
	{
		stat_map_t::iterator iter = sStatList.find(mName);
		if (iter != sStatList.end())
			llwarns << "LLStat with duplicate name: " << mName << llendl;
		sStatList.insert(std::make_pair(mName, this));
	}
}

LLStat::LLStat(const U32 num_bins, const BOOL use_frame_timer)
	: mUseFrameTimer(use_frame_timer),
	  mNumBins(num_bins)
{
	init();
}

LLStat::LLStat(std::string name, U32 num_bins, BOOL use_frame_timer)
:	mUseFrameTimer(use_frame_timer),
	mNumBins(num_bins),
	mName(name)
{
	init();
}

LLStat::~LLStat()
{
	delete[] mBins;
	delete[] mBeginTime;
	delete[] mTime;
	delete[] mDT;

	if (!mName.empty())
	{
		// handle multiple entries with the same name
		stat_map_t::iterator iter = sStatList.find(mName);
		while (iter != sStatList.end() && iter->second != this)
			++iter;
		sStatList.erase(iter);
	}
}

void LLStat::reset()
{
	U32 i;

	mNumValues = 0;
	mLastValue = 0.f;
	mCurBin = (mNumBins-1);
	delete[] mBins;
	delete[] mBeginTime;
	delete[] mTime;
	delete[] mDT;
	mBins      = new F32[mNumBins];
	mBeginTime = new F64[mNumBins];
	mTime      = new F64[mNumBins];
	mDT        = new F32[mNumBins];
	for (i = 0; i < mNumBins; i++)
	{
		mBins[i]      = 0.f;
		mBeginTime[i] = 0.0;
		mTime[i]      = 0.0;
		mDT[i]        = 0.f;
	}
}

void LLStat::setBeginTime(const F64 time)
{
	mBeginTime[mNextBin] = time;
}

void LLStat::addValueTime(const F64 time, const F32 value)
{
	if (mNumValues < mNumBins)
	{
		mNumValues++;
	}

	// Increment the bin counters.
	mCurBin++;
	if ((U32)mCurBin == mNumBins)
	{
		mCurBin = 0;
	}
	mNextBin++;
	if ((U32)mNextBin == mNumBins)
	{
		mNextBin = 0;
	}

	mBins[mCurBin] = value;
	mTime[mCurBin] = time;
	mDT[mCurBin] = (F32)(mTime[mCurBin] - mBeginTime[mCurBin]);
	//this value is used to prime the min/max calls
	mLastTime = mTime[mCurBin];
	mLastValue = value;

	// Set the begin time for the next stat segment.
	mBeginTime[mNextBin] = mTime[mCurBin];
	mTime[mNextBin] = mTime[mCurBin];
	mDT[mNextBin] = 0.f;
}

void LLStat::start()
{
	if (mUseFrameTimer)
	{
		mBeginTime[mNextBin] = sFrameTimer.getElapsedSeconds();
	}
	else
	{
		mBeginTime[mNextBin] = sTimer.getElapsedTimeF64();
	}
}

void LLStat::addValue(const F32 value)
{
	if (mNumValues < mNumBins)
	{
		mNumValues++;
	}

	// Increment the bin counters.
	mCurBin++;
	if ((U32)mCurBin == mNumBins)
	{
		mCurBin = 0;
	}
	mNextBin++;
	if ((U32)mNextBin == mNumBins)
	{
		mNextBin = 0;
	}

	mBins[mCurBin] = value;
	if (mUseFrameTimer)
	{
		mTime[mCurBin] = sFrameTimer.getElapsedSeconds();
	}
	else
	{
		mTime[mCurBin] = sTimer.getElapsedTimeF64();
	}
	mDT[mCurBin] = (F32)(mTime[mCurBin] - mBeginTime[mCurBin]);

	//this value is used to prime the min/max calls
	mLastTime = mTime[mCurBin];
	mLastValue = value;

	// Set the begin time for the next stat segment.
	mBeginTime[mNextBin] = mTime[mCurBin];
	mTime[mNextBin] = mTime[mCurBin];
	mDT[mNextBin] = 0.f;
}


F32 LLStat::getMax() const
{
	U32 i;
	F32 current_max = mLastValue;
	if (mNumBins == 0)
	{
		current_max = 0.f;
	}
	else
	{
		for (i = 0; (i < mNumBins) && (i < mNumValues); i++)
		{
			// Skip the bin we're currently filling.
			if (i == (U32)mNextBin)
			{
				continue;
			}
			if (mBins[i] > current_max)
			{
				current_max = mBins[i];
			}
		}
	}
	return current_max;
}

F32 LLStat::getMean() const
{
	U32 i;
	F32 current_mean = 0.f;
	U32 samples = 0;
	for (i = 0; (i < mNumBins) && (i < mNumValues); i++)
	{
		// Skip the bin we're currently filling.
		if (i == (U32)mNextBin)
		{
			continue;
		}
		current_mean += mBins[i];
		samples++;
	}

	// There will be a wrap error at 2^32. :)
	if (samples != 0)
	{
		current_mean /= samples;
	}
	else
	{
		current_mean = 0.f;
	}
	return current_mean;
}

F32 LLStat::getMin() const
{
	U32 i;
	F32 current_min = mLastValue;

	if (mNumBins == 0)
	{
		current_min = 0.f;
	}
	else
	{
		for (i = 0; (i < mNumBins) && (i < mNumValues); i++)
		{
			// Skip the bin we're currently filling.
			if (i == (U32)mNextBin)
			{
				continue;
			}
			if (mBins[i] < current_min)
			{
				current_min = mBins[i];
			}
		}
	}
	return current_min;
}

F32 LLStat::getSum() const
{
	U32 i;
	F32 sum = 0.f;
	for (i = 0; (i < mNumBins) && (i < mNumValues); i++)
	{
		// Skip the bin we're currently filling.
		if (i == (U32)mNextBin)
		{
			continue;
		}
		sum += mBins[i];
	}

	return sum;
}

F32 LLStat::getSumDuration() const
{
	U32 i;
	F32 sum = 0.f;
	for (i = 0; (i < mNumBins) && (i < mNumValues); i++)
	{
		// Skip the bin we're currently filling.
		if (i == (U32)mNextBin)
		{
			continue;
		}
		sum += mDT[i];
	}

	return sum;
}

F32 LLStat::getPrev(S32 age) const
{
	S32 bin;
	bin = mCurBin - age;

	while (bin < 0)
	{
		bin += mNumBins;
	}

	if (bin == mNextBin)
	{
		// Bogus for bin we're currently working on.
		return 0.f;
	}
	return mBins[bin];
}

F32 LLStat::getPrevPerSec(S32 age) const
{
	S32 bin;
	bin = mCurBin - age;

	while (bin < 0)
	{
		bin += mNumBins;
	}

	if (bin == mNextBin)
	{
		// Bogus for bin we're currently working on.
		return 0.f;
	}
	return mBins[bin] / mDT[bin];
}

F64 LLStat::getPrevBeginTime(S32 age) const
{
	S32 bin;
	bin = mCurBin - age;

	while (bin < 0)
	{
		bin += mNumBins;
	}

	if (bin == mNextBin)
	{
		// Bogus for bin we're currently working on.
		return 0.f;
	}

	return mBeginTime[bin];
}

F64 LLStat::getPrevTime(S32 age) const
{
	S32 bin;
	bin = mCurBin - age;

	while (bin < 0)
	{
		bin += mNumBins;
	}

	if (bin == mNextBin)
	{
		// Bogus for bin we're currently working on.
		return 0.f;
	}

	return mTime[bin];
}

F32 LLStat::getBin(S32 bin) const
{
	return mBins[bin];
}

F32 LLStat::getBinPerSec(S32 bin) const
{
	return mBins[bin] / mDT[bin];
}

F64 LLStat::getBinBeginTime(S32 bin) const
{
	return mBeginTime[bin];
}

F64 LLStat::getBinTime(S32 bin) const
{
	return mTime[bin];
}

F32 LLStat::getCurrent() const
{
	return mBins[mCurBin];
}

F32 LLStat::getCurrentPerSec() const
{
	return mBins[mCurBin] / mDT[mCurBin];
}

F64 LLStat::getCurrentBeginTime() const
{
	return mBeginTime[mCurBin];
}

F64 LLStat::getCurrentTime() const
{
	return mTime[mCurBin];
}

F32 LLStat::getCurrentDuration() const
{
	return mDT[mCurBin];
}

F32 LLStat::getMeanPerSec() const
{
	U32 i;
	F32 value = 0.f;
	F32 dt    = 0.f;

	for (i = 0; (i < mNumBins) && (i < mNumValues); i++)
	{
		// Skip the bin we're currently filling.
		if (i == (U32)mNextBin)
		{
			continue;
		}
		value += mBins[i];
		dt    += mDT[i];
	}

	if (dt > 0.f)
	{
		return value/dt;
	}
	else
	{
		return 0.f;
	}
}

F32 LLStat::getMeanDuration() const
{
	F32 dur = 0.0f;
	U32 count = 0;
	for (U32 i=0; (i < mNumBins) && (i < mNumValues); i++)
	{
		if (i == (U32)mNextBin)
		{
			continue;
		}
		dur += mDT[i];
		count++;
	}

	if (count > 0)
	{
		dur /= F32(count);
		return dur;
	}
	else
	{
		return 0.f;
	}
}

F32 LLStat::getMaxPerSec() const
{
	U32 i;
	F32 value;

	if (mNextBin != 0)
	{
		value = mBins[0]/mDT[0];
	}
	else if (mNumValues > 0)
	{
		value = mBins[1]/mDT[1];
	}
	else
	{
		value = 0.f;
	}

	for (i = 0; (i < mNumBins) && (i < mNumValues); i++)
	{
		// Skip the bin we're currently filling.
		if (i == (U32)mNextBin)
		{
			continue;
		}
		value = llmax(value, mBins[i]/mDT[i]);
	}
	return value;
}

F32 LLStat::getMinPerSec() const
{
	U32 i;
	F32 value;
	
	if (mNextBin != 0)
	{
		value = mBins[0]/mDT[0];
	}
	else if (mNumValues > 0)
	{
		value = mBins[1]/mDT[1];
	}
	else
	{
		value = 0.f;
	}

	for (i = 0; (i < mNumBins) && (i < mNumValues); i++)
	{
		// Skip the bin we're currently filling.
		if (i == (U32)mNextBin)
		{
			continue;
		}
		value = llmin(value, mBins[i]/mDT[i]);
	}
	return value;
}

F32 LLStat::getMinDuration() const
{
	F32 dur = 0.0f;
	for (U32 i=0; (i < mNumBins) && (i < mNumValues); i++)
	{
		dur = llmin(dur, mDT[i]);
	}
	return dur;
}

U32 LLStat::getNumValues() const
{
	return mNumValues;
}

S32 LLStat::getNumBins() const
{
	return mNumBins;
}

S32 LLStat::getCurBin() const
{
	return mCurBin;
}

S32 LLStat::getNextBin() const
{
	return mNextBin;
}

F64 LLStat::getLastTime() const
{
	return mLastTime;
}
