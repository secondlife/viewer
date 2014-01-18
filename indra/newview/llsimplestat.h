/** 
 * @file llsimplestat.h
 * @brief Runtime statistics accumulation.
 *
 * $LicenseInfo:firstyear=2010&license=viewerlgpl$
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

#ifndef LL_SIMPLESTAT_H
#define LL_SIMPLESTAT_H

// History
//
// The original source for this code is the server repositories'
// llcommon/llstat.h file.  This particular code was added after the
// viewer/server code schism but before the effort to convert common
// code to libraries was complete.  Rather than add to merge issues,
// the needed code was cut'n'pasted into this new header as it isn't
// too awful a burden.  Post-modularization, we can look at removing
// this redundancy.


/**
 * @class LLSimpleStatCounter
 * @brief Just counts events.
 *
 * Really not needed but have a pattern in mind in the future.
 * Interface limits what can be done at that's just fine.
 *
 * *TODO:  Update/transfer unit tests
 * Unit tests:  indra/test/llcommon_llstat_tut.cpp
 */
class LLSimpleStatCounter
{
public:
	inline LLSimpleStatCounter()		{ reset(); }
	// Default destructor and assignment operator are valid

	inline void reset()					{ mCount = 0; }

	inline void merge(const LLSimpleStatCounter & src)
										{ mCount += src.mCount; }
	
	inline U32 operator++()				{ return ++mCount; }

	inline U32 getCount() const			{ return mCount; }
		
protected:
	U32			mCount;
};


/**
 * @class LLSimpleStatMMM
 * @brief Templated collector of min, max and mean data for stats.
 *
 * Fed a stream of data samples, keeps a running account of the
 * min, max and mean seen since construction or the last reset()
 * call.  A freshly-constructed or reset instance returns counts
 * and values of zero.
 *
 * Overflows and underflows (integer, inf or -inf) and NaN's
 * are the caller's problem.  As is loss of precision when
 * the running sum's exponent (when parameterized by a floating
 * point of some type) differs from a given data sample's.
 *
 * Unit tests:  indra/test/llcommon_llstat_tut.cpp
 */
template <typename VALUE_T = F32>
class LLSimpleStatMMM
{
public:
	typedef VALUE_T Value;
	
public:
	LLSimpleStatMMM()				{ reset(); }
	// Default destructor and assignment operator are valid

	/**
	 * Resets the object returning all counts and derived
	 * values back to zero.
	 */
	void reset()
		{
			mCount = 0;
			mMin = Value(0);
			mMax = Value(0);
			mTotal = Value(0);
		}

	void record(Value v)
		{
			if (mCount)
			{
				mMin = llmin(mMin, v);
				mMax = llmax(mMax, v);
			}
			else
			{
				mMin = v;
				mMax = v;
			}
			mTotal += v;
			++mCount;
		}

	void merge(const LLSimpleStatMMM<VALUE_T> & src)
		{
			if (! mCount)
			{
				*this = src;
			}
			else if (src.mCount)
			{
				mMin = llmin(mMin, src.mMin);
				mMax = llmax(mMax, src.mMax);
				mCount += src.mCount;
				mTotal += src.mTotal;
			}
		}
	
	inline U32 getCount() const		{ return mCount; }
	inline Value getMin() const		{ return mMin; }
	inline Value getMax() const		{ return mMax; }
	inline Value getMean() const	{ return mCount ? mTotal / mCount : mTotal; }
		
protected:
	U32			mCount;
	Value		mMin;
	Value		mMax;
	Value		mTotal;
};

#endif // LL_SIMPLESTAT_H
