/**
* @file llstatsaccumulator.h
* @brief Class for accumulating statistics.
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

#ifndef LL_STATS_ACCUMULATOR_H
#define  LL_STATS_ACCUMULATOR_H
#include "llsd.h"

class LLStatsAccumulator
{
public:
    inline LLStatsAccumulator()
    {
        reset();
    }

    inline void push(F32 val)
    {
        if (mCountOfNextUpdatesToIgnore > 0)
        {
            mCountOfNextUpdatesToIgnore--;
            return;
        }

        mCount++;
        mSum += val;
        mSumOfSquares += val * val;
        if (mCount == 1 || val > mMaxValue)
        {
            mMaxValue = val;
        }
        if (mCount == 1 || val < mMinValue)
        {
            mMinValue = val;
        }
    }

    inline F32 getSum() const
    {
        return F32(mSum);
    }

    inline F32 getSumOfSquares() const
    {
        return F32(mSumOfSquares);
    }

    inline F32 getMean() const
    {
        return (mCount == 0) ? 0.f : ((F32)mSum) / mCount;
    }

    inline F32 getMinValue() const
    {
        return mMinValue;
    }

    inline F32 getMaxValue() const
    {
        return mMaxValue;
    }

    inline F32 getStdDev() const
    {
        const F64 mean = getMean();
        if (mCount < 2)
        {
            return 0.f;
        }
        else
        {
            F32 variance = F32 (mSumOfSquares / mCount - (mean * mean));
            return sqrt(llmax(0.f, variance));
        }
    }

    inline U32 getCount() const
    {
        return mCount;
    }

    inline void reset()
    {
        mCount = 0;
        mSum = mSumOfSquares = 0.f;
        mMinValue = 0.0f;
        mMaxValue = 0.0f;
        mCountOfNextUpdatesToIgnore = 0;
    }

    inline LLSD asLLSD() const
    {
        LLSD data;
        data["count"] = S32(getCount());
        data["sum"] = getSum();
        data["sum_of_squares"] = getSumOfSquares();
        data["mean"] = getMean();
        data["std_dev"] = getStdDev();
        data["min"] = getMinValue();
        data["max"] = getMaxValue();
        return data;
    }

private:
    S32 mCount;
    F64 mSum;
    F64 mSumOfSquares;
    F32 mMinValue;
    F32 mMaxValue;
    U32 mCountOfNextUpdatesToIgnore;
};

// Assumes the samples are frame times
inline F32 fps(LLStatsAccumulator& accum)
{
    F32 mean = accum.getMean();
    if (mean > 0.f)
    {
        return 1.0f/mean;
    }
    return 0.0f;
}

// Assumes the samples are frame times
// ofr = observed frame rate, which has frame times weighted by length, since long frames count more toward user experience.
inline F32 ofr(LLStatsAccumulator& accum)
{
    F32 sum = accum.getSum();
    F32 sum_of_squares = accum.getSumOfSquares();
    if (sum_of_squares > 0.0f)
    {
        return sum/sum_of_squares;
    }
    return 0.0f;
}

#endif
