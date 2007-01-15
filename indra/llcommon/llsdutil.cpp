/** 
 * @file llsdutil.cpp
 * @author Phoenix
 * @date 2006-05-24
 * @brief Implementation of classes, functions, etc, for using structured data.
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "linden_common.h"

#include "llsdutil.h"

#if LL_WINDOWS
#	define WIN32_LEAN_AND_MEAN
#	include <winsock2.h>	// for htonl
#elif LL_LINUX
#	include <netinet/in.h>
#elif LL_DARWIN
#	include <arpa/inet.h>
#endif



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

// U32
LLSD ll_sd_from_U32(const U32 val)
{
	std::vector<U8> v;
	U32 net_order = htonl(val);

	v.resize(4);
	memcpy(&(v[0]), &net_order, 4);		/* Flawfinder: ignore */

	return LLSD(v);
}

U32 ll_U32_from_sd(const LLSD& sd)
{
	U32 ret;
	std::vector<U8> v = sd.asBinary();
	if (v.size() < 4)
	{
		return 0;
	}
	memcpy(&ret, &(v[0]), 4);		/* Flawfinder: ignore */
	ret = ntohl(ret);
	return ret;
}

//U64
LLSD ll_sd_from_U64(const U64 val)
{
	std::vector<U8> v;
	U32 high, low;

	high = (U32)(val >> 32);
	low = (U32)val;
	high = htonl(high);
	low = htonl(low);

	v.resize(8);
	memcpy(&(v[0]), &high, 4);		/* Flawfinder: ignore */
	memcpy(&(v[4]), &low, 4);		/* Flawfinder: ignore */

	return LLSD(v);
}

U64 ll_U64_from_sd(const LLSD& sd)
{
	U32 high, low;
	std::vector<U8> v = sd.asBinary();

	if (v.size() < 8)
	{
		return 0;
	}

	memcpy(&high, &(v[0]), 4);		/* Flawfinder: ignore */
	memcpy(&low, &(v[4]), 4);		/* Flawfinder: ignore */
	high = ntohl(high);
	low = ntohl(low);

	return ((U64)high) << 32 | low;
}

// IP Address (stored in net order in a U32, so don't need swizzling)
LLSD ll_sd_from_ipaddr(const U32 val)
{
	std::vector<U8> v;

	v.resize(4);
	memcpy(&(v[0]), &val, 4);		/* Flawfinder: ignore */

	return LLSD(v);
}

U32 ll_ipaddr_from_sd(const LLSD& sd)
{
	U32 ret;
	std::vector<U8> v = sd.asBinary();
	if (v.size() < 4)
	{
		return 0;
	}
	memcpy(&ret, &(v[0]), 4);		/* Flawfinder: ignore */
	return ret;
}
