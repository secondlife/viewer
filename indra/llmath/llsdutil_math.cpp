/** 
 * @file llsdutil_math.cpp
 * @author Phoenix
 * @date 2006-05-24
 * @brief Implementation of classes, functions, etc, for using structured data.
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2009, Linden Research, Inc.
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

#include "linden_common.h"

#include "llsdutil.h"

#include "v3math.h"
#include "v4math.h"
#include "v3dmath.h"
#include "v2math.h"
#include "llquaternion.h"
#include "v4color.h"

#if LL_WINDOWS
#	define WIN32_LEAN_AND_MEAN
#	include <winsock2.h>	// for htonl
#elif LL_LINUX || LL_SOLARIS
#	include <netinet/in.h>
#elif LL_DARWIN
#	include <arpa/inet.h>
#endif

#include "llsdserialize.h"

// vector3
LLSD ll_sd_from_vector3(const LLVector3& vec)
{
	LLSD rv;
	rv.append((F64)vec.mV[VX]);
	rv.append((F64)vec.mV[VY]);
	rv.append((F64)vec.mV[VZ]);
	return rv;
}

LLVector3 ll_vector3_from_sd(const LLSD& sd, S32 start_index)
{
	LLVector3 rv;
	rv.mV[VX] = (F32)sd[start_index].asReal();
	rv.mV[VY] = (F32)sd[++start_index].asReal();
	rv.mV[VZ] = (F32)sd[++start_index].asReal();
	return rv;
}

// vector4
LLSD ll_sd_from_vector4(const LLVector4& vec)
{
	LLSD rv;
	rv.append((F64)vec.mV[VX]);
	rv.append((F64)vec.mV[VY]);
	rv.append((F64)vec.mV[VZ]);
	rv.append((F64)vec.mV[VW]);
	return rv;
}

LLVector4 ll_vector4_from_sd(const LLSD& sd, S32 start_index)
{
	LLVector4 rv;
	rv.mV[VX] = (F32)sd[start_index].asReal();
	rv.mV[VY] = (F32)sd[++start_index].asReal();
	rv.mV[VZ] = (F32)sd[++start_index].asReal();
	rv.mV[VW] = (F32)sd[++start_index].asReal();
	return rv;
}

// vector3d
LLSD ll_sd_from_vector3d(const LLVector3d& vec)
{
	LLSD rv;
	rv.append(vec.mdV[VX]);
	rv.append(vec.mdV[VY]);
	rv.append(vec.mdV[VZ]);
	return rv;
}

LLVector3d ll_vector3d_from_sd(const LLSD& sd, S32 start_index)
{
	LLVector3d rv;
	rv.mdV[VX] = sd[start_index].asReal();
	rv.mdV[VY] = sd[++start_index].asReal();
	rv.mdV[VZ] = sd[++start_index].asReal();
	return rv;
}

//vector2
LLSD ll_sd_from_vector2(const LLVector2& vec)
{
	LLSD rv;
	rv.append((F64)vec.mV[VX]);
	rv.append((F64)vec.mV[VY]);
	return rv;
}

LLVector2 ll_vector2_from_sd(const LLSD& sd)
{
	LLVector2 rv;
	rv.mV[VX] = (F32)sd[0].asReal();
	rv.mV[VY] = (F32)sd[1].asReal();
	return rv;
}

// Quaternion
LLSD ll_sd_from_quaternion(const LLQuaternion& quat)
{
	LLSD rv;
	rv.append((F64)quat.mQ[VX]);
	rv.append((F64)quat.mQ[VY]);
	rv.append((F64)quat.mQ[VZ]);
	rv.append((F64)quat.mQ[VW]);
	return rv;
}

LLQuaternion ll_quaternion_from_sd(const LLSD& sd)
{
	LLQuaternion quat;
	quat.mQ[VX] = (F32)sd[0].asReal();
	quat.mQ[VY] = (F32)sd[1].asReal();
	quat.mQ[VZ] = (F32)sd[2].asReal();
	quat.mQ[VW] = (F32)sd[3].asReal();
	return quat;
}

// color4
LLSD ll_sd_from_color4(const LLColor4& c)
{
	LLSD rv;
	rv.append(c.mV[0]);
	rv.append(c.mV[1]);
	rv.append(c.mV[2]);
	rv.append(c.mV[3]);
	return rv;
}

LLColor4 ll_color4_from_sd(const LLSD& sd)
{
	LLColor4 c;
	c.mV[0] = (F32)sd[0].asReal();
	c.mV[1] = (F32)sd[1].asReal();
	c.mV[2] = (F32)sd[2].asReal();
	c.mV[3] = (F32)sd[3].asReal();
	return c;
}
