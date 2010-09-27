/** 
 * @file llinterp.h
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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

#ifndef LL_LLINTERP_H
#define LL_LLINTERP_H

#if defined(LL_WINDOWS)
// macro definitions for common math constants (e.g. M_PI) are declared under the _USE_MATH_DEFINES
// on Windows system.
// So, let's define _USE_MATH_DEFINES before including math.h
	#define _USE_MATH_DEFINES
#endif

#include "math.h"

// Class from which different types of interpolators can be derived

class LLInterpVal
{
public:
	virtual ~LLInterpVal() {}
	virtual void interp(LLInterpVal &target, const F32 frac); // Linear interpolation for each type
};

template <typename Type>
class LLInterp
{
public:
        LLInterp();
	virtual ~LLInterp() {}

	virtual void start();
	void update(const F32 time);
	const Type &getCurVal() const;

	void setStartVal(const Type &start_val);
	const Type &getStartVal() const;

	void setEndVal(const Type &target_val);
	const Type &getEndVal() const;

	void setStartTime(const F32 time);
	F32 getStartTime() const;

	void setEndTime(const F32 time);
	F32 getEndTime() const;

	BOOL isActive() const;
	BOOL isDone() const;
	
protected:
	F32 mStartTime;
	F32 mEndTime;
	F32 mDuration;
	BOOL mActive;
	BOOL mDone;

	Type mStartVal;
	Type mEndVal;

	F32 mCurTime;
	Type mCurVal;
};

template <typename Type>
class LLInterpLinear : public LLInterp<Type>
{
public:
	/*virtual*/ void start();
	void update(const F32 time);
	F32 getCurFrac() const;
protected:
	F32 mCurFrac;
};

template <typename Type>
class LLInterpExp : public LLInterpLinear<Type>
{
public:
	void update(const F32 time);
protected:
};

template <typename Type>
class LLInterpAttractor : public LLInterp<Type>
{
public:
	LLInterpAttractor();
	/*virtual*/ void start();
	void setStartVel(const Type &vel);
	void setForce(const F32 force);
	void update(const F32 time);
protected:
	F32 mForce;
	Type mStartVel;
	Type mVelocity;
};

template <typename Type>
class LLInterpFunc : public LLInterp<Type>
{
public:
	LLInterpFunc();
	void update(const F32 time);

	void setFunc(Type (*)(const F32, void *data), void *data);
protected:
	Type (*mFunc)(const F32 time, void *data);
	void *mData;
};


///////////////////////////////////
//
// Implementation
//
//

/////////////////////////////////
//
// LLInterp base class implementation
//

template <typename Type>
LLInterp<Type>::LLInterp()
: mStartVal(Type()), mEndVal(Type()), mCurVal(Type())
{
	mStartTime = 0.f;
	mEndTime = 1.f;
	mDuration = 1.f;
	mCurTime = 0.f;
	mDone = FALSE;
	mActive = FALSE;
}

template <class Type>
void LLInterp<Type>::setStartVal(const Type &start_val)
{
	mStartVal = start_val;
}

template <class Type>
void LLInterp<Type>::start()
{
	mCurVal = mStartVal;
	mCurTime = mStartTime;
	mDone = FALSE;
	mActive = FALSE;
}

template <class Type>
const Type &LLInterp<Type>::getStartVal() const
{
	return mStartVal;
}

template <class Type>
void LLInterp<Type>::setEndVal(const Type &end_val)
{
	mEndVal = end_val;
}

template <class Type>
const Type &LLInterp<Type>::getEndVal() const
{
	return mEndVal;
}

template <class Type>
const Type &LLInterp<Type>::getCurVal() const
{
	return mCurVal;
}


template <class Type>
void LLInterp<Type>::setStartTime(const F32 start_time)
{
	mStartTime = start_time;
	mDuration = mEndTime - mStartTime;
}

template <class Type>
F32 LLInterp<Type>::getStartTime() const
{
	return mStartTime;
}


template <class Type>
void LLInterp<Type>::setEndTime(const F32 end_time)
{
	mEndTime = end_time;
	mDuration = mEndTime - mStartTime;
}


template <class Type>
F32 LLInterp<Type>::getEndTime() const
{
	return mEndTime;
}


template <class Type>
BOOL LLInterp<Type>::isDone() const
{
	return mDone;
}

template <class Type>
BOOL LLInterp<Type>::isActive() const
{
	return mActive;
}

//////////////////////////////
//
// LLInterpLinear derived class implementation.
//
template <typename Type>
void LLInterpLinear<Type>::start()
{
	LLInterp<Type>::start();
	mCurFrac = 0.f;
}

template <typename Type>
void LLInterpLinear<Type>::update(const F32 time)
{
	F32 target_frac = (time - this->mStartTime) / this->mDuration;
	F32 dfrac = target_frac - this->mCurFrac;
	if (target_frac >= 0.f)
	{
		this->mActive = TRUE;
	}
	
	if (target_frac > 1.f)
	{
		this->mCurVal = this->mEndVal;
		this->mCurFrac = 1.f;
		this->mCurTime = time;
		this->mDone = TRUE;
		return;
	}

	target_frac = llmin(1.f, target_frac);
	target_frac = llmax(0.f, target_frac);

	if (dfrac >= 0.f)
	{
		F32 total_frac = 1.f - this->mCurFrac;
		F32 inc_frac = dfrac / total_frac;
		this->mCurVal = inc_frac * this->mEndVal + (1.f - inc_frac) * this->mCurVal;
		this->mCurTime = time;
	}
	else
	{
		F32 total_frac = this->mCurFrac - 1.f;
		F32 inc_frac = dfrac / total_frac;
		this->mCurVal = inc_frac * this->mStartVal + (1.f - inc_frac) * this->mCurVal;
		this->mCurTime = time;
	}
	mCurFrac = target_frac;
}

template <class Type>
F32 LLInterpLinear<Type>::getCurFrac() const
{
	return mCurFrac;
}


//////////////////////////////
//
// LLInterpAttractor derived class implementation.
//


template <class Type>
LLInterpAttractor<Type>::LLInterpAttractor() : LLInterp<Type>()
{
	mForce = 0.1f;
	mVelocity *= 0.f;
	mStartVel *= 0.f;
}

template <class Type>
void LLInterpAttractor<Type>::start()
{
	LLInterp<Type>::start();
	mVelocity = mStartVel;
}


template <class Type>
void LLInterpAttractor<Type>::setStartVel(const Type &vel)
{
	mStartVel = vel;
}

template <class Type>
void LLInterpAttractor<Type>::setForce(const F32 force)
{
	mForce = force;
}

template <class Type>
void LLInterpAttractor<Type>::update(const F32 time)
{
	if (time > this->mStartTime)
	{
		this->mActive = TRUE;
	}
	else
	{
		return;
	}
	if (time > this->mEndTime)
	{
		this->mDone = TRUE;
		return;
	}

	F32 dt = time - this->mCurTime;
	Type dist_val = this->mEndVal - this->mCurVal;
	Type dv = 0.5*dt*dt*this->mForce*dist_val;
	this->mVelocity += dv;
	this->mCurVal += this->mVelocity * dt;
	this->mCurTime = time;
}


//////////////////////////////
//
// LLInterpFucn derived class implementation.
//


template <class Type>
LLInterpFunc<Type>::LLInterpFunc() : LLInterp<Type>()
{
	mFunc = NULL;
	mData = NULL;
}

template <class Type>
void LLInterpFunc<Type>::setFunc(Type (*func)(const F32, void *data), void *data)
{
	mFunc = func;
	mData = data;
}

template <class Type>
void LLInterpFunc<Type>::update(const F32 time)
{
	if (time > this->mStartTime)
	{
		this->mActive = TRUE;
	}
	else
	{
		return;
	}
	if (time > this->mEndTime)
	{
		this->mDone = TRUE;
		return;
	}

	this->mCurVal = (*mFunc)(time - this->mStartTime, mData);
	this->mCurTime = time;
}

//////////////////////////////
//
// LLInterpExp derived class implementation.
//

template <class Type>
void LLInterpExp<Type>::update(const F32 time)
{
	F32 target_frac = (time - this->mStartTime) / this->mDuration;
	if (target_frac >= 0.f)
	{
		this->mActive = TRUE;
	}
	
	if (target_frac > 1.f)
	{
		this->mCurVal = this->mEndVal;
		this->mCurFrac = 1.f;
		this->mCurTime = time;
		this->mDone = TRUE;
		return;
	}

	this->mCurFrac = 1.f - (F32)(exp(-2.f*target_frac));
	this->mCurVal = this->mStartVal + this->mCurFrac * (this->mEndVal - this->mStartVal);
	this->mCurTime = time;
}

#endif // LL_LLINTERP_H

