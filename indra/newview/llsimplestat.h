/** 
 * @file llsimplestat.h
 * @brief Runtime statistics accumulation.
 *
 * $LicenseInfo:firstyear=2010&license=viewergpl$
 * 
 * Copyright (c) 2010, Linden Research, Inc.
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
