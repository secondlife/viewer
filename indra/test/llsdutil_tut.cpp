/**
 * @file llsdutil_tut.cpp
 * @author Adroit
 * @date 2007-02
 * @brief LLSD conversion routines test cases.
 *
 * Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "linden_common.h"
#include "lltut.h"
#include "v3color.h"
#include "v4math.h"
#include "m4math.h"
#include "llsdutil.h"


namespace tut
{
	struct llsdutil_data
	{
	};
	typedef test_group<llsdutil_data> llsdutil_test;;
	typedef llsdutil_test::object llsdutil_object;
	tut::llsdutil_test tutil("llsdutil");

	template<> template<>
	void llsdutil_object::test<1>()
	{
		LLSD sd;
		U64 valueIn , valueOut;
		valueIn = U64L(0xFEDCBA9876543210);
		sd = ll_sd_from_U64(valueIn);
		valueOut = ll_U64_from_sd(sd);
		ensure_equals("U64 valueIn->sd->valueOut", valueIn, valueOut);
	}

	template<> template<>
	void llsdutil_object::test<2>()
	{
		LLSD sd;
		U32 valueIn, valueOut;
		valueIn = 0x87654321;
		sd = ll_sd_from_U32(valueIn);
		valueOut = ll_U32_from_sd(sd);
		ensure_equals("U32 valueIn->sd->valueOut", valueIn, valueOut);
	}

	template<> template<>
	void llsdutil_object::test<3>()
	{
		U32 valueIn, valueOut;
		valueIn = 0x87654321;
		LLSD sd;
		sd = ll_sd_from_ipaddr(valueIn);
		valueOut = ll_ipaddr_from_sd(sd);
		ensure_equals("valueIn->sd->valueOut", valueIn, valueOut);
	}

	template<> template<>
	void llsdutil_object::test<4>()
	{
		LLSD sd;
		LLVector3 vec1(-1.0, 2.0, -3.0);
		sd = ll_sd_from_vector3(vec1); 
		LLVector3 vec2 = ll_vector3_from_sd(sd);
		ensure_equals("vector3 -> sd -> vector3: 1", vec1, vec2);

		LLVector3 vec3(sd);
		ensure_equals("vector3 -> sd -> vector3: 2", vec1, vec3);

		sd.clear();
		vec1.setVec(0., 0., 0.);
		sd = ll_sd_from_vector3(vec1); 
		vec2 = ll_vector3_from_sd(sd);
		ensure_equals("vector3 -> sd -> vector3: 3", vec1, vec2);
		sd.clear();
	}

	template<> template<>
	void llsdutil_object::test<5>()
	{
		LLSD sd;
		LLVector3d vec1((F64)(U64L(0xFEDCBA9876543210) << 2), -1., 0);
		sd = ll_sd_from_vector3d(vec1); 
		LLVector3d vec2 = ll_vector3d_from_sd(sd);
		ensure_equals("vector3d -> sd -> vector3d: 1", vec1, vec2);
		
		LLVector3d vec3(sd); 
		ensure_equals("vector3d -> sd -> vector3d : 2", vec1, vec3);
	}

	template<> template<>
	void llsdutil_object::test<6>()
	{
		LLSD sd;
		LLVector2 vec((F32) -3., (F32) 4.2);
		sd = ll_sd_from_vector2(vec); 
		LLVector2 vec1 = ll_vector2_from_sd(sd);
		ensure_equals("vector2 -> sd -> vector2", vec, vec1);
		
		LLSD sd2 = ll_sd_from_vector2(vec1); 
		ensure_equals("sd -> vector2 -> sd: 2", sd, sd2);
	}

	template<> template<>
	void llsdutil_object::test<7>()
	{
		LLSD sd;
		LLQuaternion quat((F32) 1., (F32) -0.98, (F32) 2.3, (F32) 0xffff);
		sd = ll_sd_from_quaternion(quat); 
		LLQuaternion quat1 = ll_quaternion_from_sd(sd);
		ensure_equals("LLQuaternion -> sd -> LLQuaternion", quat, quat1);
		
		LLSD sd2 = ll_sd_from_quaternion(quat1); 
		ensure_equals("sd -> LLQuaternion -> sd ", sd, sd2);
	}

	template<> template<>
	void llsdutil_object::test<8>()
	{
		LLSD sd;
		LLColor4 c(1.0f, 2.2f, 4.0f, 7.f);
		sd = ll_sd_from_color4(c); 
		LLColor4 c1 =ll_color4_from_sd(sd);
		ensure_equals("LLColor4 -> sd -> LLColor4", c, c1);
		
		LLSD sd1 = ll_sd_from_color4(c1);
		ensure_equals("sd -> LLColor4 -> sd", sd, sd1);
	}
}
