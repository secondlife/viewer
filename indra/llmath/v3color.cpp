/** 
 * @file v3color.cpp
 * @brief LLColor3 class implementation.
 *
 * Copyright (c) 2000-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "linden_common.h"

#include "v3color.h"
#include "v4color.h"

LLColor3 LLColor3::white(1.0f, 1.0f, 1.0f);
LLColor3 LLColor3::black(0.0f, 0.0f, 0.0f);
LLColor3 LLColor3::grey (0.5f, 0.5f, 0.5f);

LLColor3::LLColor3(const LLColor4 &a)
{
	mV[0] = a.mV[0];
	mV[1] = a.mV[1];
	mV[2] = a.mV[2];
}

LLColor3::LLColor3(const LLSD &sd)
{
	mV[0] = (F32) sd[0].asReal();
	mV[1] = (F32) sd[1].asReal();
	mV[2] = (F32) sd[2].asReal();
}

const LLColor3& LLColor3::operator=(const LLColor4 &a) 
{
	mV[0] = a.mV[0];
	mV[1] = a.mV[1];
	mV[2] = a.mV[2];
	return (*this);
}

std::ostream& operator<<(std::ostream& s, const LLColor3 &a) 
{
	s << "{ " << a.mV[VX] << ", " << a.mV[VY] << ", " << a.mV[VZ] << " }";
	return s;
}
