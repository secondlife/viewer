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
//------------------------------------------------------------------------
LLTimer LLStat::sTimer;
LLFrameTimer LLStat::sFrameTimer;

void LLStat::reset()
{
	mNumValues = 0;
	mLastValue = 0.f;
	delete[] mBins;
	mBins      = new ValueEntry[mNumBins];
	mCurBin = mNumBins-1;
	mNextBin = 0;
}

LLStat::LLStat(std::string name, S32 num_bins, BOOL use_frame_timer)
:	mUseFrameTimer(use_frame_timer),
	mNumBins(num_bins),
	mName(name),
	mBins(NULL)
{
	llassert(mNumBins > 0);
	mLastTime  = 0.f;

	reset();

	if (!mName.empty())
	{
		stat_map_t::iterator iter = getStatList().find(mName);
		if (iter != getStatList().end())
			llwarns << "LLStat with duplicate name: " << mName << llendl;
		getStatList().insert(std::make_pair(mName, this));
	}
}

LLStat::stat_map_t& LLStat::getStatList()
{
	static LLStat::stat_map_t stat_list;
	return stat_list;
}


LLStat::~LLStat()
{
	delete[] mBins;

	if (!mName.empty())
	{
		// handle multiple entries with the same name
		stat_map_t::iterator iter = getStatList().find(mName);
		while (iter != getStatList().end() && iter->second != this)
			++iter;
		getStatList().erase(iter);
	}
}

void LLStat::start()
{
	if (mUseFrameTimer)
	{
		mBins[mNextBin].mBeginTime = sFrameTimer.getElapsedSeconds();
	}
	else
	{
		mBins[mNextBin].mBeginTime = sTimer.getElapsedTimeF64();
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
	if (mCurBin >= mNumBins)
	{
		mCurBin = 0;
	}
	mNextBin++;
	if (mNextBin >= mNumBins)
	{
		mNextBin = 0;
	}

	mBins[mCurBin].mValue = value;
	if (mUseFrameTimer)
	{
		mBins[mCurBin].mTime = sFrameTimer.getElapsedSeconds();
	}
	else
	{
		mBins[mCurBin].mTime = sTimer.getElapsedTimeF64();
	}
	mBins[mCurBin].mDT = (F32)(mBins[mCurBin].mTime - mBins[mCurBin].mBeginTime);

	//this value is used to prime the min/max calls
	mLastTime = mBins[mCurBin].mTime;
	mLastValue = value;

	// Set the begin time for the next stat segment.
	mBins[mNextBin].mBeginTime = mBins[mCurBin].mTime;
	mBins[mNextBin].mTime = mBins[mCurBin].mTime;
	mBins[mNextBin].mDT = 0.f;
}


F32 LLStat::getMax() const
{
	S32 i;
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
			if (i == mNextBin)
			{
				continue;
			}
			if (mBins[i].mValue > current_max)
			{
				current_max = mBins[i].mValue;
			}
		}
	}
	return current_max;
}

F32 LLStat::getMean() const
{
	S32 i;
	F32 current_mean = 0.f;
	S32 samples = 0;
	for (i = 0; (i < mNumBins) && (i < mNumValues); i++)
	{
		// Skip the bin we're currently filling.
		if (i == mNextBin)
		{
			continue;
		}
		current_mean += mBins[i].mValue;
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
	S32 i;
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
			if (i == mNextBin)
			{
				continue;
			}
			if (mBins[i].mValue < current_min)
			{
				current_min = mBins[i].mValue;
			}
		}
	}
	return current_min;
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
	return mBins[bin].mValue;
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
	return mBins[bin].mValue / mBins[bin].mDT;
}

F32 LLStat::getCurrent() const
{
	return mBins[mCurBin].mValue;
}

F32 LLStat::getCurrentPerSec() const
{
	return mBins[mCurBin].mValue / mBins[mCurBin].mDT;
}

F32 LLStat::getMeanPerSec() const
{
	S32 i;
	F32 value = 0.f;
	F32 dt    = 0.f;

	for (i = 0; (i < mNumBins) && (i < mNumValues); i++)
	{
		// Skip the bin we're currently filling.
		if (i == mNextBin)
		{
			continue;
		}
		value += mBins[i].mValue;
		dt    += mBins[i].mDT;
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
	S32 count = 0;
	for (S32 i=0; (i < mNumBins) && (i < mNumValues); i++)
	{
		if (i == mNextBin)
		{
			continue;
		}
		dur += mBins[i].mDT;
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
	F32 value;

	if (mNextBin != 0)
	{
		value = mBins[0].mValue/mBins[0].mDT;
	}
	else if (mNumValues > 0)
	{
		value = mBins[1].mValue/mBins[1].mDT;
	}
	else
	{
		value = 0.f;
	}

	for (S32 i = 0; (i < mNumBins) && (i < mNumValues); i++)
	{
		// Skip the bin we're currently filling.
		if (i == mNextBin)
		{
			continue;
		}
		value = llmax(value, mBins[i].mValue/mBins[i].mDT);
	}
	return value;
}

F32 LLStat::getMinPerSec() const
{
	S32 i;
	F32 value;
	
	if (mNextBin != 0)
	{
		value = mBins[0].mValue/mBins[0].mDT;
	}
	else if (mNumValues > 0)
	{
		value = mBins[1].mValue/mBins[0].mDT;
	}
	else
	{
		value = 0.f;
	}

	for (i = 0; (i < mNumBins) && (i < mNumValues); i++)
	{
		// Skip the bin we're currently filling.
		if (i == mNextBin)
		{
			continue;
		}
		value = llmin(value, mBins[i].mValue/mBins[i].mDT);
	}
	return value;
}

U32 LLStat::getNumValues() const
{
	return mNumValues;
}

S32 LLStat::getNumBins() const
{
	return mNumBins;
}

S32 LLStat::getNextBin() const
{
	return mNextBin;
}

F64 LLStat::getLastTime() const
{
	return mLastTime;
}
