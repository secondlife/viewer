/** 
 * @file v3dmath.cpp
 * @brief LLVector3d class implementation.
 *
 * Copyright (c) 2000-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "linden_common.h"

//#include <sstream>	// gcc 2.95.2 doesn't support sstream 

#include "v3dmath.h"

//#include "vmath.h"
#include "v4math.h"
#include "m4math.h"
#include "m3math.h"
#include "llquaternion.h"
#include "llquantize.h"

// LLVector3d
// WARNING: Don't use these for global const definitions!
// For example: 
//		const LLQuaternion(0.5f * F_PI, LLVector3d::zero);
// at the top of a *.cpp file might not give you what you think.
const LLVector3d LLVector3d::zero(0,0,0);
const LLVector3d LLVector3d::x_axis(1, 0, 0);
const LLVector3d LLVector3d::y_axis(0, 1, 0);
const LLVector3d LLVector3d::z_axis(0, 0, 1);
const LLVector3d LLVector3d::x_axis_neg(-1, 0, 0);
const LLVector3d LLVector3d::y_axis_neg(0, -1, 0);
const LLVector3d LLVector3d::z_axis_neg(0, 0, -1);


// Clamps each values to range (min,max).
// Returns TRUE if data changed.
BOOL LLVector3d::clamp(F64 min, F64 max)
{
	BOOL ret = FALSE;

	if (mdV[0] < min) { mdV[0] = min; ret = TRUE; }
	if (mdV[1] < min) { mdV[1] = min; ret = TRUE; }
	if (mdV[2] < min) { mdV[2] = min; ret = TRUE; }

	if (mdV[0] > max) { mdV[0] = max; ret = TRUE; }
	if (mdV[1] > max) { mdV[1] = max; ret = TRUE; }
	if (mdV[2] > max) { mdV[2] = max; ret = TRUE; }

	return ret;
}

// Sets all values to absolute value of their original values
// Returns TRUE if data changed
BOOL LLVector3d::abs()
{
	BOOL ret = FALSE;

	if (mdV[0] < 0.0) { mdV[0] = -mdV[0]; ret = TRUE; }
	if (mdV[1] < 0.0) { mdV[1] = -mdV[1]; ret = TRUE; }
	if (mdV[2] < 0.0) { mdV[2] = -mdV[2]; ret = TRUE; }

	return ret;
}

std::ostream& operator<<(std::ostream& s, const LLVector3d &a) 
{
	s << "{ " << a.mdV[VX] << ", " << a.mdV[VY] << ", " << a.mdV[VZ] << " }";
	return s;
}

const LLVector3d& LLVector3d::operator=(const LLVector4 &a) 
{
	mdV[0] = a.mV[0];
	mdV[1] = a.mV[1];
	mdV[2] = a.mV[2];
	return *this;
}

const LLVector3d&	LLVector3d::rotVec(const LLMatrix3 &mat)
{
	*this = *this * mat;
	return *this;
}

const LLVector3d&	LLVector3d::rotVec(const LLQuaternion &q)
{
	*this = *this * q;
	return *this;
}

const LLVector3d&	LLVector3d::rotVec(F64 angle, const LLVector3d &vec)
{
	if ( !vec.isExactlyZero() && angle )
	{
		*this = *this * LLMatrix3((F32)angle, vec);
	}
	return *this;
}

const LLVector3d&	LLVector3d::rotVec(F64 angle, F64 x, F64 y, F64 z)
{
	LLVector3d vec(x, y, z);
	if ( !vec.isExactlyZero() && angle )
	{
		*this = *this * LLMatrix3((F32)angle, vec);
	}
	return *this;
}


BOOL LLVector3d::parseVector3d(const char* buf, LLVector3d* value)
{
	if( buf == NULL || buf[0] == '\0' || value == NULL)
	{
		return FALSE;
	}

	LLVector3d v;
	S32 count = sscanf( buf, "%lf %lf %lf", v.mdV + 0, v.mdV + 1, v.mdV + 2 );
	if( 3 == count )
	{
		value->setVec( v );
		return TRUE;
	}

	return FALSE;
}

