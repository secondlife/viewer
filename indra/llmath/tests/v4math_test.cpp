/**
 * @file v4math_test.cpp
 * @author Adroit
 * @date 2007-03
 * @brief v4math test cases.
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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
#include "../test/lltut.h"
#include "llsd.h"

#include "../m4math.h"
#include "../v4math.h"
#include "../llquaternion.h"

namespace tut
{
	struct v4math_data
	{
	};
	typedef test_group<v4math_data> v4math_test;
	typedef v4math_test::object v4math_object;
	tut::v4math_test v4math_testcase("v4math_h");

	template<> template<>
	void v4math_object::test<1>()
	{
		LLVector4 vec4;
		ensure("1:LLVector4:Fail to initialize " ,((0 == vec4.mV[VX]) && (0 == vec4.mV[VY]) && (0 == vec4.mV[VZ])&& (1.0f == vec4.mV[VW])));
		F32 x = 10.f, y = -2.3f, z = -.023f, w = -2.0f;
		LLVector4 vec4a(x,y,z);
		ensure("2:LLVector4:Fail to initialize " ,((x == vec4a.mV[VX]) && (y == vec4a.mV[VY]) && (z == vec4a.mV[VZ])&& (1.0f == vec4a.mV[VW])));
		LLVector4 vec4b(x,y,z,w);
		ensure("3:LLVector4:Fail to initialize " ,((x == vec4b.mV[VX]) && (y == vec4b.mV[VY]) && (z == vec4b.mV[VZ])&& (w == vec4b.mV[VW])));
		const F32 vec[4] = {.112f ,23.2f, -4.2f, -.0001f};
		LLVector4 vec4c(vec);
		ensure("4:LLVector4:Fail to initialize " ,((vec[0] == vec4c.mV[VX]) && (vec[1] == vec4c.mV[VY]) && (vec[2] == vec4c.mV[VZ])&& (vec[3] == vec4c.mV[VW])));
		LLVector3 vec3(-2.23f,1.01f,42.3f);
		LLVector4 vec4d(vec3);
		ensure("5:LLVector4:Fail to initialize " ,((vec3.mV[VX] == vec4d.mV[VX]) && (vec3.mV[VY] == vec4d.mV[VY]) && (vec3.mV[VZ] == vec4d.mV[VZ])&& (1.f == vec4d.mV[VW])));
		F32 w1 = -.234f;
		LLVector4 vec4e(vec3,w1);
		ensure("6:LLVector4:Fail to initialize " ,((vec3.mV[VX] == vec4e.mV[VX]) && (vec3.mV[VY] == vec4e.mV[VY]) && (vec3.mV[VZ] == vec4e.mV[VZ])&& (w1 == vec4e.mV[VW])));
	}

	template<> template<>
	void v4math_object::test<2>()
	{
		F32 x = 10.f, y = -2.3f, z = -.023f, w = -2.0f;
		LLVector4 vec4;
		vec4.setVec(x,y,z);
		ensure("1:setVec:Fail to initialize " ,((x == vec4.mV[VX]) && (y == vec4.mV[VY]) && (z == vec4.mV[VZ])&& (1.0f == vec4.mV[VW])));
		vec4.clearVec();
		ensure("2:clearVec:Fail " ,((0 == vec4.mV[VX]) && (0 == vec4.mV[VY]) && (0 == vec4.mV[VZ])&& (1.0f == vec4.mV[VW])));
		vec4.setVec(x,y,z,w);
		ensure("3:setVec:Fail to initialize " ,((x == vec4.mV[VX]) && (y == vec4.mV[VY]) && (z == vec4.mV[VZ])&& (w == vec4.mV[VW])));
		vec4.zeroVec();
		ensure("4:zeroVec:Fail " ,((0 == vec4.mV[VX]) && (0 == vec4.mV[VY]) && (0 == vec4.mV[VZ])&& (0 == vec4.mV[VW])));
		LLVector3 vec3(-2.23f,1.01f,42.3f);
		vec4.clearVec();
		vec4.setVec(vec3);
		ensure("5:setVec:Fail to initialize " ,((vec3.mV[VX] == vec4.mV[VX]) && (vec3.mV[VY] == vec4.mV[VY]) && (vec3.mV[VZ] == vec4.mV[VZ])&& (1.f == vec4.mV[VW])));
		F32 w1 = -.234f;
		vec4.zeroVec();
		vec4.setVec(vec3,w1);
		ensure("6:setVec:Fail to initialize " ,((vec3.mV[VX] == vec4.mV[VX]) && (vec3.mV[VY] == vec4.mV[VY]) && (vec3.mV[VZ] == vec4.mV[VZ])&& (w1 == vec4.mV[VW])));
		const F32 vec[4] = {.112f ,23.2f, -4.2f, -.0001f};
		LLVector4 vec4a;
		vec4a.setVec(vec);
		ensure("7:setVec:Fail to initialize " ,((vec[0] == vec4a.mV[VX]) && (vec[1] == vec4a.mV[VY]) && (vec[2] == vec4a.mV[VZ])&& (vec[3] == vec4a.mV[VW])));
	}

	template<> template<>
	void v4math_object::test<3>()
	{
		F32 x = 10.f, y = -2.3f, z = -.023f;
		LLVector4 vec4(x,y,z);
		ensure("magVec:Fail ", is_approx_equal(vec4.magVec(), (F32) sqrt(x*x + y*y + z*z)));
		ensure("magVecSquared:Fail ", is_approx_equal(vec4.magVecSquared(), (x*x + y*y + z*z)));
	}

	template<> template<>
	void v4math_object::test<4>()
	{
		F32 x = 10.f, y = -2.3f, z = -.023f;
		LLVector4 vec4(x,y,z);
		F32 mag = vec4.normVec();
		mag = 1.f/ mag;
		ensure("1:normVec: Fail " ,is_approx_equal(mag*x,vec4.mV[VX]) && is_approx_equal(mag*y, vec4.mV[VY])&& is_approx_equal(mag*z, vec4.mV[VZ]));
		x = 0.000000001f, y = 0.000000001f, z = 0.000000001f;
		vec4.clearVec();
		vec4.setVec(x,y,z);
		mag = vec4.normVec();
		ensure("2:normVec: Fail " ,is_approx_equal(mag*x,vec4.mV[VX]) && is_approx_equal(mag*y, vec4.mV[VY])&& is_approx_equal(mag*z, vec4.mV[VZ]));
	}

	template<> template<>
	void v4math_object::test<5>()
	{
		F32 x = 10.f, y = -2.3f, z = -.023f, w = -2.0f;
		LLVector4 vec4(x,y,z,w);
		vec4.abs();
		ensure("abs:Fail " ,((x == vec4.mV[VX]) && (-y == vec4.mV[VY]) && (-z == vec4.mV[VZ])&& (-w == vec4.mV[VW])));
		vec4.clearVec();
		ensure("isExactlyClear:Fail " ,(TRUE == vec4.isExactlyClear()));
		vec4.zeroVec();
		ensure("isExactlyZero:Fail " ,(TRUE == vec4.isExactlyZero()));
	}

	template<> template<>
	void v4math_object::test<6>()
	{
		F32 x = 10.f, y = -2.3f, z = -.023f, w = -2.0f;
		LLVector4 vec4(x,y,z,w),vec4a;
		vec4a = vec4.scaleVec(vec4);
		ensure("scaleVec:Fail " ,(is_approx_equal(x*x, vec4a.mV[VX]) && is_approx_equal(y*y, vec4a.mV[VY]) && is_approx_equal(z*z, vec4a.mV[VZ])&& is_approx_equal(w*w, vec4a.mV[VW])));
	}

	template<> template<>
	void v4math_object::test<7>()
	{
		F32 x = 10.f, y = -2.3f, z = -.023f, w = -2.0f;
		LLVector4 vec4(x,y,z,w);
		ensure("1:operator [] failed " ,( x ==  vec4[0]));	
		ensure("2:operator [] failed " ,( y ==  vec4[1]));
		ensure("3:operator [] failed " ,( z ==  vec4[2]));
		ensure("4:operator [] failed " ,( w ==  vec4[3]));
		x = 23.f, y = -.2361f, z = 3.25;
		vec4.setVec(x,y,z);
		F32 &ref1 = vec4[0];
		ensure("5:operator [] failed " ,( ref1 ==  vec4[0]));
		F32 &ref2 = vec4[1];
		ensure("6:operator [] failed " ,( ref2 ==  vec4[1]));
		F32 &ref3 = vec4[2];
		ensure("7:operator [] failed " ,( ref3 ==  vec4[2]));	
		F32 &ref4 = vec4[3];
		ensure("8:operator [] failed " ,( ref4 ==  vec4[3]));
	}

	template<> template<>
	void v4math_object::test<8>()
	{
		F32 x = 10.f, y = -2.3f, z = -.023f, w = -2.0f;
		const  F32 val[16] = {
            1.f,  2.f,   3.f,    0.f,
            .34f, .1f,   -.5f,   0.f,
            2.f,  1.23f, 1.234f, 0.f,
            .89f, 0.f,   0.f,    0.f
        };
		LLMatrix4 mat(val);
		LLVector4 vec4(x,y,z,w),vec4a;
		vec4.rotVec(mat);
		vec4a.setVec(x,y,z,w);
		vec4a.rotVec(mat);
		ensure_equals("1:rotVec: Fail " ,vec4a, vec4);
		F32 a = 2.32f, b = -23.2f, c = -34.1112f, d = 1.010112f;
		LLQuaternion q(a,b,c,d);
		LLVector4 vec4b(a,b,c,d),vec4c;
		vec4b.rotVec(q);
		vec4c.setVec(a, b, c, d);
		vec4c.rotVec(q);
		ensure_equals("2:rotVec: Fail " ,vec4b, vec4c);
	}
	
	template<> template<>
	void v4math_object::test<9>()
	{
		F32 x = 10.f, y = -2.3f, z = -.023f, w = -2.0f;
		LLVector4 vec4(x,y,z,w),vec4a;;
		std::ostringstream stream1, stream2;
		stream1 << vec4;
		vec4a.setVec(x,y,z,w);
		stream2 << vec4a;
		ensure("operator << failed",(stream1.str() == stream2.str()));	
	}
	
	template<> template<>
	void v4math_object::test<10>()
	{
		F32 x1 = 1.f, y1 = 2.f, z1 = -1.1f, w1 = .23f;
		F32 x2 = 1.2f, y2 = 2.5f, z2 = 1.f, w2 = 1.3f;
		LLVector4 vec4(x1,y1,z1,w1),vec4a(x2,y2,z2,w2),vec4b;
		vec4b = vec4a + vec4;
		ensure("1:operator+:Fail to initialize " ,(is_approx_equal(x1+x2,vec4b.mV[VX]) && is_approx_equal(y1+y2,vec4b.mV[VY]) && is_approx_equal(z1+z2,vec4b.mV[VZ])));
		x1 = -2.45f, y1 = 2.1f, z1 = 3.0f;
		vec4.clearVec();
		vec4a.clearVec();
		vec4.setVec(x1,y1,z1);
		vec4a +=vec4;
		ensure_equals("2:operator+=: Fail to initialize", vec4a,vec4);
		vec4a += vec4;
		ensure("3:operator+=:Fail to initialize " ,(is_approx_equal(2*x1,vec4a.mV[VX]) && is_approx_equal(2*y1,vec4a.mV[VY]) && is_approx_equal(2*z1,vec4a.mV[VZ])));
	}
	template<> template<>
	void v4math_object::test<11>()
	{
		F32 x1 = 1.f, y1 = 2.f, z1 = -1.1f, w1 = .23f;
		F32 x2 = 1.2f, y2 = 2.5f, z2 = 1.f, w2 = 1.3f;
		LLVector4 vec4(x1,y1,z1,w1),vec4a(x2,y2,z2,w2),vec4b;
		vec4b = vec4a - vec4;
		ensure("1:operator-:Fail to initialize " ,(is_approx_equal(x2-x1,vec4b.mV[VX]) && is_approx_equal(y2-y1,vec4b.mV[VY]) && is_approx_equal(z2-z1,vec4b.mV[VZ])));
		x1 = -2.45f, y1 = 2.1f, z1 = 3.0f;
		vec4.clearVec();
		vec4a.clearVec();
		vec4.setVec(x1,y1,z1);
		vec4a -=vec4;
		ensure_equals("2:operator-=: Fail to initialize" , vec4a,-vec4);
		vec4a -=vec4;
		ensure("3:operator-=:Fail to initialize " ,(is_approx_equal(-2*x1,vec4a.mV[VX]) && is_approx_equal(-2*y1,vec4a.mV[VY]) && is_approx_equal(-2*z1,vec4a.mV[VZ])));
	}

	template<> template<>
	void v4math_object::test<12>()
	{
		F32 x1 = 1.f, y1 = 2.f, z1 = -1.1f;
		F32 x2 = 1.2f, y2 = 2.5f, z2 = 1.f;
		LLVector4 vec4(x1,y1,z1),vec4a(x2,y2,z2);
		F32 res = vec4 * vec4a;
		ensure("1:operator* failed " ,is_approx_equal(res, x1*x2 + y1*y2 + z1*z2));
		vec4a.clearVec();
		F32 mulVal = 4.2f;
		vec4a = vec4 * mulVal;
		ensure("2:operator* failed " ,is_approx_equal(x1*mulVal,vec4a.mV[VX]) && is_approx_equal(y1*mulVal, vec4a.mV[VY])&& is_approx_equal(z1*mulVal, vec4a.mV[VZ]));
		vec4a.clearVec();
		vec4a = mulVal *  vec4 ;
		ensure("3:operator* failed " ,is_approx_equal(x1*mulVal, vec4a.mV[VX]) && is_approx_equal(y1*mulVal, vec4a.mV[VY])&& is_approx_equal(z1*mulVal, vec4a.mV[VZ]));
		vec4 *= mulVal;
		ensure("4:operator*= failed " ,is_approx_equal(x1*mulVal, vec4.mV[VX]) && is_approx_equal(y1*mulVal, vec4.mV[VY])&& is_approx_equal(z1*mulVal, vec4.mV[VZ]));
	}

	template<> template<>
	void v4math_object::test<13>()
	{
		F32 x1 = 1.f, y1 = 2.f, z1 = -1.1f;
		F32 x2 = 1.2f, y2 = 2.5f, z2 = 1.f;
		LLVector4 vec4(x1,y1,z1),vec4a(x2,y2,z2),vec4b;
		vec4b = vec4 % vec4a;
		ensure("1:operator% failed " ,is_approx_equal(y1*z2 - y2*z1, vec4b.mV[VX]) && is_approx_equal(z1*x2 -z2*x1, vec4b.mV[VY]) && is_approx_equal(x1*y2-x2*y1, vec4b.mV[VZ])); 
		vec4 %= vec4a;
		ensure_equals("operator%= failed " ,vec4,vec4b); 
	}

	template<> template<>
	void v4math_object::test<14>()
	{
		F32 x = 1.f, y = 2.f, z = -1.1f,div = 4.2f;
		F32 t = 1.f / div;
		LLVector4 vec4(x,y,z), vec4a;
		vec4a = vec4/div;
		ensure("1:operator/ failed " ,is_approx_equal(x*t, vec4a.mV[VX]) && is_approx_equal(y*t, vec4a.mV[VY])&& is_approx_equal(z*t, vec4a.mV[VZ]));
		x = 1.23f, y = 4.f, z = -2.32f;
		vec4.clearVec();
		vec4a.clearVec();
		vec4.setVec(x,y,z);
		vec4a = vec4/div;
		ensure("2:operator/ failed " ,is_approx_equal(x*t, vec4a.mV[VX]) && is_approx_equal(y*t, vec4a.mV[VY])&& is_approx_equal(z*t, vec4a.mV[VZ]));
		vec4 /= div;
		ensure("3:operator/ failed " ,is_approx_equal(x*t, vec4.mV[VX]) && is_approx_equal(y*t, vec4.mV[VY])&& is_approx_equal(z*t, vec4.mV[VZ]));
	}

	template<> template<>
	void v4math_object::test<15>()
	{
		F32 x = 1.f, y = 2.f, z = -1.1f;
		LLVector4 vec4(x,y,z), vec4a;
		ensure("operator!= failed " ,(vec4 != vec4a));
		vec4a = vec4;
		ensure("operator== failed " ,(vec4 ==vec4a)); 
	}

	template<> template<>
	void v4math_object::test<16>()
	{
		F32 x = 1.f, y = 2.f, z = -1.1f;
		LLVector4 vec4(x,y,z), vec4a;
		vec4a = - vec4;
		ensure("operator- failed " , (vec4 == - vec4a));	
	}

	template<> template<>
	void v4math_object::test<17>()
	{
		F32 x = 1.f, y = 2.f, z = -1.1f,epsilon = .23425f;
		LLVector4 vec4(x,y,z), vec4a(x,y,z);
		ensure("1:are_parallel: Fail " ,(TRUE == are_parallel(vec4a,vec4,epsilon)));
		x = 21.f, y = 12.f, z = -123.1f;
		vec4a.clearVec();
		vec4a.setVec(x,y,z);
		ensure("2:are_parallel: Fail " ,(FALSE == are_parallel(vec4a,vec4,epsilon)));
	}

	template<> template<>
	void v4math_object::test<18>()
	{
		F32 x = 1.f, y = 2.f, z = -1.1f;
		F32 angle1, angle2;
		LLVector4 vec4(x,y,z), vec4a(x,y,z);
		angle1 = angle_between(vec4, vec4a);
		vec4.normVec();
		vec4a.normVec();
		angle2 = acos(vec4 * vec4a);
		ensure_approximately_equals("1:angle_between: Fail " ,angle1,angle2,8);
		F32 x1 = 21.f, y1 = 2.23f, z1 = -1.1f;
		LLVector4 vec4b(x,y,z), vec4c(x1,y1,z1);
		angle1 = angle_between(vec4b, vec4c);
		vec4b.normVec();
		vec4c.normVec();
		angle2 = acos(vec4b * vec4c);
		ensure_approximately_equals("2:angle_between: Fail " ,angle1,angle2,8);
	}

	template<> template<>
	void v4math_object::test<19>()
	{
		F32 x1 =-2.3f, y1 = 2.f,z1 = 1.2f, x2 = 1.3f, y2 = 1.f, z2 = 1.f;
		F32 val1,val2;
		LLVector4 vec4(x1,y1,z1),vec4a(x2,y2,z2);
		val1 = dist_vec(vec4,vec4a);
		val2 = (F32) sqrt((x1 - x2)*(x1 - x2) + (y1 - y2)* (y1 - y2) + (z1 - z2)* (z1 -z2));
		ensure_equals("dist_vec: Fail ",val2, val1);
		val1 = dist_vec_squared(vec4,vec4a);
		val2 =((x1 - x2)*(x1 - x2) + (y1 - y2)* (y1 - y2) + (z1 - z2)* (z1 -z2));
		ensure_equals("dist_vec_squared: Fail ",val2, val1);
	}

	template<> template<>
	void v4math_object::test<20>()
	{
		F32 x1 =-2.3f, y1 = 2.f,z1 = 1.2f, w1 = -.23f, x2 = 1.3f, y2 = 1.f, z2 = 1.f,w2 = .12f;
		F32 val = 2.3f,val1,val2,val3,val4;
		LLVector4 vec4(x1,y1,z1,w1),vec4a(x2,y2,z2,w2);
		val1 = x1 + (x2 - x1)* val;
		val2 = y1 + (y2 - y1)* val;
		val3 = z1 + (z2 - z1)* val;
		val4 = w1 + (w2 - w1)* val;
		LLVector4 vec4b = lerp(vec4,vec4a,val);
		ensure("lerp failed", ((val1 ==vec4b.mV[VX])&& (val2 ==vec4b.mV[VY]) && (val3 ==vec4b.mV[VZ])&& (val4 ==vec4b.mV[VW])));	
	}

	template<> template<>
	void v4math_object::test<21>()
	{
		F32 x = 1.f, y = 2.f, z = -1.1f;
		LLVector4 vec4(x,y,z);
		LLVector3 vec3 = vec4to3(vec4);
		ensure("vec4to3 failed", ((x == vec3.mV[VX])&& (y == vec3.mV[VY]) && (z == vec3.mV[VZ])));	
		LLVector4 vec4a = vec3to4(vec3);
		ensure_equals("vec3to4 failed",vec4a,vec4);	
	}

	template<> template<>
	void v4math_object::test<22>()
	{
		F32 x = 1.f, y = 2.f, z = -1.1f;
		LLVector4 vec4(x,y,z);
		LLSD llsd = vec4.getValue();
		LLVector3 vec3(llsd);
		LLVector4 vec4a = vec3to4(vec3);
		ensure_equals("getValue failed",vec4a,vec4);	
	}		
}
