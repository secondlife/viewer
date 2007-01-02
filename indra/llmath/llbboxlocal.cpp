/** 
 * @file llbboxlocal.cpp
 * @brief General purpose bounding box class (Not axis aligned).
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "linden_common.h"

#include "llbboxlocal.h"
#include "m4math.h"

void LLBBoxLocal::addPoint(const LLVector3& p)
{
	mMin.mV[VX] = llmin( p.mV[VX], mMin.mV[VX] );
	mMin.mV[VY] = llmin( p.mV[VY], mMin.mV[VY] );
	mMin.mV[VZ] = llmin( p.mV[VZ], mMin.mV[VZ] );
	mMax.mV[VX] = llmax( p.mV[VX], mMax.mV[VX] );
	mMax.mV[VY] = llmax( p.mV[VY], mMax.mV[VY] );
	mMax.mV[VZ] = llmax( p.mV[VZ], mMax.mV[VZ] );
}

void LLBBoxLocal::expand( F32 delta )
{
	mMin.mV[VX] -= delta;
	mMin.mV[VY] -= delta;
	mMin.mV[VZ] -= delta;
	mMax.mV[VX] += delta;
	mMax.mV[VY] += delta;
	mMax.mV[VZ] += delta;
}

LLBBoxLocal operator*(const LLBBoxLocal &a, const LLMatrix4 &b)
{
	return LLBBoxLocal( a.mMin * b, a.mMax * b );
}
