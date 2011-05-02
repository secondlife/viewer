/** 
 * @file v3math.cpp
 * @brief LLVector3 class implementation.
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
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

#include "v3math.h"

//#include "vmath.h"
#include "v2math.h"
#include "v4math.h"
#include "m4math.h"
#include "m3math.h"
#include "llquaternion.h"
#include "llquantize.h"
#include "v3dmath.h"

// LLVector3
// WARNING: Don't use these for global const definitions!
// For example: 
//		const LLQuaternion(0.5f * F_PI, LLVector3::zero);
// at the top of a *.cpp file might not give you what you think.
const LLVector3 LLVector3::zero(0,0,0);
const LLVector3 LLVector3::x_axis(1.f, 0, 0);
const LLVector3 LLVector3::y_axis(0, 1.f, 0);
const LLVector3 LLVector3::z_axis(0, 0, 1.f);
const LLVector3 LLVector3::x_axis_neg(-1.f, 0, 0);
const LLVector3 LLVector3::y_axis_neg(0, -1.f, 0);
const LLVector3 LLVector3::z_axis_neg(0, 0, -1.f);
const LLVector3 LLVector3::all_one(1.f,1.f,1.f);


// Clamps each values to range (min,max).
// Returns TRUE if data changed.
BOOL LLVector3::clamp(F32 min, F32 max)
{
	BOOL ret = FALSE;

	if (mV[0] < min) { mV[0] = min; ret = TRUE; }
	if (mV[1] < min) { mV[1] = min; ret = TRUE; }
	if (mV[2] < min) { mV[2] = min; ret = TRUE; }

	if (mV[0] > max) { mV[0] = max; ret = TRUE; }
	if (mV[1] > max) { mV[1] = max; ret = TRUE; }
	if (mV[2] > max) { mV[2] = max; ret = TRUE; }

	return ret;
}

// Clamps length to an upper limit.  
// Returns TRUE if the data changed
BOOL LLVector3::clampLength( F32 length_limit )
{
	BOOL changed = FALSE;

	F32 len = length();
	if (llfinite(len))
	{
		if ( len > length_limit)
		{
			normalize();
			if (length_limit < 0.f)
			{
				length_limit = 0.f;
			}
			mV[0] *= length_limit;
			mV[1] *= length_limit;
			mV[2] *= length_limit;
			changed = TRUE;
		}
	}
	else
	{	// this vector may still be salvagable
		F32 max_abs_component = 0.f;
		for (S32 i = 0; i < 3; ++i)
		{
			F32 abs_component = fabs(mV[i]);
			if (llfinite(abs_component))
			{
				if (abs_component > max_abs_component)
				{
					max_abs_component = abs_component;
				}
			}
			else
			{
				// no it can't be salvaged --> clear it
				clear();
				changed = TRUE;
				break;
			}
		}
		if (!changed)
		{
			// yes it can be salvaged -->
			// bring the components down before we normalize
			mV[0] /= max_abs_component;
			mV[1] /= max_abs_component;
			mV[2] /= max_abs_component;
			normalize();

			if (length_limit < 0.f)
			{
				length_limit = 0.f;
			}
			mV[0] *= length_limit;
			mV[1] *= length_limit;
			mV[2] *= length_limit;
		}
	}

	return changed;
}

BOOL LLVector3::clamp(const LLVector3 &min_vec, const LLVector3 &max_vec)
{
	BOOL ret = FALSE;

	if (mV[0] < min_vec[0]) { mV[0] = min_vec[0]; ret = TRUE; }
	if (mV[1] < min_vec[1]) { mV[1] = min_vec[1]; ret = TRUE; }
	if (mV[2] < min_vec[2]) { mV[2] = min_vec[2]; ret = TRUE; }

	if (mV[0] > max_vec[0]) { mV[0] = max_vec[0]; ret = TRUE; }
	if (mV[1] > max_vec[1]) { mV[1] = max_vec[1]; ret = TRUE; }
	if (mV[2] > max_vec[2]) { mV[2] = max_vec[2]; ret = TRUE; }

	return ret;
}


// Sets all values to absolute value of their original values
// Returns TRUE if data changed
BOOL LLVector3::abs()
{
	BOOL ret = FALSE;

	if (mV[0] < 0.f) { mV[0] = -mV[0]; ret = TRUE; }
	if (mV[1] < 0.f) { mV[1] = -mV[1]; ret = TRUE; }
	if (mV[2] < 0.f) { mV[2] = -mV[2]; ret = TRUE; }

	return ret;
}

// Quatizations
void	LLVector3::quantize16(F32 lowerxy, F32 upperxy, F32 lowerz, F32 upperz)
{
	F32 x = mV[VX];
	F32 y = mV[VY];
	F32 z = mV[VZ];

	x = U16_to_F32(F32_to_U16(x, lowerxy, upperxy), lowerxy, upperxy);
	y = U16_to_F32(F32_to_U16(y, lowerxy, upperxy), lowerxy, upperxy);
	z = U16_to_F32(F32_to_U16(z, lowerz,  upperz),  lowerz,  upperz);

	mV[VX] = x;
	mV[VY] = y;
	mV[VZ] = z;
}

void	LLVector3::quantize8(F32 lowerxy, F32 upperxy, F32 lowerz, F32 upperz)
{
	mV[VX] = U8_to_F32(F32_to_U8(mV[VX], lowerxy, upperxy), lowerxy, upperxy);;
	mV[VY] = U8_to_F32(F32_to_U8(mV[VY], lowerxy, upperxy), lowerxy, upperxy);
	mV[VZ] = U8_to_F32(F32_to_U8(mV[VZ], lowerz, upperz), lowerz, upperz);
}


void 	LLVector3::snap(S32 sig_digits)
{
	mV[VX] = snap_to_sig_figs(mV[VX], sig_digits);
	mV[VY] = snap_to_sig_figs(mV[VY], sig_digits);
	mV[VZ] = snap_to_sig_figs(mV[VZ], sig_digits);
}

const LLVector3&	LLVector3::rotVec(const LLMatrix3 &mat)
{
	*this = *this * mat;
	return *this;
}

const LLVector3&	LLVector3::rotVec(const LLQuaternion &q)
{
	*this = *this * q;
	return *this;
}

const LLVector3& LLVector3::transVec(const LLMatrix4& mat)
{
	setVec(
			mV[VX] * mat.mMatrix[VX][VX] + 
			mV[VY] * mat.mMatrix[VX][VY] + 
			mV[VZ] * mat.mMatrix[VX][VZ] +
			mat.mMatrix[VX][VW],
			 
			mV[VX] * mat.mMatrix[VY][VX] + 
			mV[VY] * mat.mMatrix[VY][VY] + 
			mV[VZ] * mat.mMatrix[VY][VZ] +
			mat.mMatrix[VY][VW],

			mV[VX] * mat.mMatrix[VZ][VX] + 
			mV[VY] * mat.mMatrix[VZ][VY] + 
			mV[VZ] * mat.mMatrix[VZ][VZ] +
			mat.mMatrix[VZ][VW]);

	return *this;
}


const LLVector3&	LLVector3::rotVec(F32 angle, const LLVector3 &vec)
{
	if ( !vec.isExactlyZero() && angle )
	{
		*this = *this * LLQuaternion(angle, vec);
	}
	return *this;
}

const LLVector3&	LLVector3::rotVec(F32 angle, F32 x, F32 y, F32 z)
{
	LLVector3 vec(x, y, z);
	if ( !vec.isExactlyZero() && angle )
	{
		*this = *this * LLQuaternion(angle, vec);
	}
	return *this;
}

const LLVector3&	LLVector3::scaleVec(const LLVector3& vec)
{
	mV[VX] *= vec.mV[VX];
	mV[VY] *= vec.mV[VY];
	mV[VZ] *= vec.mV[VZ];

	return *this;
}

LLVector3			LLVector3::scaledVec(const LLVector3& vec) const
{
	LLVector3 ret = LLVector3(*this);
	ret.scaleVec(vec);
	return ret;
}

const LLVector3&	LLVector3::set(const LLVector3d &vec)
{
	mV[0] = (F32)vec.mdV[0];
	mV[1] = (F32)vec.mdV[1];
	mV[2] = (F32)vec.mdV[2];
	return (*this);
}

const LLVector3&	LLVector3::set(const LLVector4 &vec)
{
	mV[0] = vec.mV[0];
	mV[1] = vec.mV[1];
	mV[2] = vec.mV[2];
	return (*this);
}

const LLVector3&	LLVector3::setVec(const LLVector3d &vec)
{
	mV[0] = (F32)vec.mdV[0];
	mV[1] = (F32)vec.mdV[1];
	mV[2] = (F32)vec.mdV[2];
	return (*this);
}

const LLVector3&	LLVector3::setVec(const LLVector4 &vec)
{
	mV[0] = vec.mV[0];
	mV[1] = vec.mV[1];
	mV[2] = vec.mV[2];
	return (*this);
}

LLVector3::LLVector3(const LLVector2 &vec)
{
	mV[VX] = (F32)vec.mV[VX];
	mV[VY] = (F32)vec.mV[VY];
	mV[VZ] = 0;
}

LLVector3::LLVector3(const LLVector3d &vec)
{
	mV[VX] = (F32)vec.mdV[VX];
	mV[VY] = (F32)vec.mdV[VY];
	mV[VZ] = (F32)vec.mdV[VZ];
}

LLVector3::LLVector3(const LLVector4 &vec)
{
	mV[VX] = (F32)vec.mV[VX];
	mV[VY] = (F32)vec.mV[VY];
	mV[VZ] = (F32)vec.mV[VZ];
}

LLVector3::LLVector3(const LLSD& sd)
{
	setValue(sd);
}

LLSD LLVector3::getValue() const
{
	LLSD ret;
	ret[0] = mV[0];
	ret[1] = mV[1];
	ret[2] = mV[2];
	return ret;
}

void LLVector3::setValue(const LLSD& sd)
{
	mV[0] = (F32) sd[0].asReal();
	mV[1] = (F32) sd[1].asReal();
	mV[2] = (F32) sd[2].asReal();
}

const LLVector3& operator*=(LLVector3 &a, const LLQuaternion &rot)
{
    const F32 rw = - rot.mQ[VX] * a.mV[VX] - rot.mQ[VY] * a.mV[VY] - rot.mQ[VZ] * a.mV[VZ];
    const F32 rx =   rot.mQ[VW] * a.mV[VX] + rot.mQ[VY] * a.mV[VZ] - rot.mQ[VZ] * a.mV[VY];
    const F32 ry =   rot.mQ[VW] * a.mV[VY] + rot.mQ[VZ] * a.mV[VX] - rot.mQ[VX] * a.mV[VZ];
    const F32 rz =   rot.mQ[VW] * a.mV[VZ] + rot.mQ[VX] * a.mV[VY] - rot.mQ[VY] * a.mV[VX];
	
    a.mV[VX] = - rw * rot.mQ[VX] +  rx * rot.mQ[VW] - ry * rot.mQ[VZ] + rz * rot.mQ[VY];
    a.mV[VY] = - rw * rot.mQ[VY] +  ry * rot.mQ[VW] - rz * rot.mQ[VX] + rx * rot.mQ[VZ];
    a.mV[VZ] = - rw * rot.mQ[VZ] +  rz * rot.mQ[VW] - rx * rot.mQ[VY] + ry * rot.mQ[VX];

	return a;
}

// static 
BOOL LLVector3::parseVector3(const std::string& buf, LLVector3* value)
{
	if( buf.empty() || value == NULL)
	{
		return FALSE;
	}

	LLVector3 v;
	S32 count = sscanf( buf.c_str(), "%f %f %f", v.mV + 0, v.mV + 1, v.mV + 2 );
	if( 3 == count )
	{
		value->setVec( v );
		return TRUE;
	}

	return FALSE;
}
