/**
 * @file v3math_test.cpp
 * @author Adroit
 * @date 2007-02
 * @brief v3math test cases.
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

#include "../v3dmath.h"
#include "../m3math.h"
#include "../v4math.h"
#include "../v3math.h"
#include "../llquaternion.h"
#include "../llquantize.h"


namespace tut
{
	struct v3math_data
	{
	};
	typedef test_group<v3math_data> v3math_test;
	typedef v3math_test::object v3math_object;
	tut::v3math_test v3math_testcase("v3math_h");

	template<> template<>
	void v3math_object::test<1>()
	{
		LLVector3 vec3;
		ensure("1:LLVector3:Fail to initialize ", ((0.f == vec3.mV[VX]) && (0.f == vec3.mV[VY]) && (0.f == vec3.mV[VZ])));
		F32 x = 2.32f, y = 1.212f, z = -.12f;
		LLVector3 vec3a(x,y,z);
		ensure("2:LLVector3:Fail to initialize ", ((2.32f == vec3a.mV[VX]) && (1.212f == vec3a.mV[VY]) && (-.12f == vec3a.mV[VZ])));
		const F32 vec[3] = {1.2f ,3.2f, -4.2f};
		LLVector3 vec3b(vec);
		ensure("3:LLVector3:Fail to initialize ", ((1.2f == vec3b.mV[VX]) && (3.2f == vec3b.mV[VY]) && (-4.2f == vec3b.mV[VZ])));
	}

	template<> template<>
	void v3math_object::test<2>()
	{
		F32 x = 2.32f, y = 1.212f, z = -.12f;
		LLVector3 vec3(x,y,z);
		LLVector3d vector3d(vec3);
		LLVector3 vec3a(vector3d);
		ensure("1:LLVector3:Fail to initialize ", vec3 == vec3a);
		LLVector4 vector4(vec3);
		LLVector3 vec3b(vector4);
		ensure("2:LLVector3:Fail to initialize ", vec3 == vec3b);
	}

	template<> template<>
	void v3math_object::test<3>()
	{
		S32 a = 231;
		LLSD llsd(a);
		LLVector3 vec3(llsd);
		LLSD sd = vec3.getValue();
		LLVector3 vec3a(sd);
		ensure("1:LLVector3:Fail to initialize ", (vec3 == vec3a));
	}

	template<> template<>
	void v3math_object::test<4>()
	{
		S32 a = 231;
		LLSD llsd(a);
		LLVector3 vec3(llsd),vec3a;
		vec3a = vec3;
		ensure("1:Operator= Fail to initialize " ,(vec3 == vec3a));
	}
	
	template<> template<>
	void v3math_object::test<5>()
	{
		F32 x = 2.32f, y = 1.212f, z = -.12f;
		LLVector3 vec3(x,y,z);
		ensure("1:isFinite= Fail to initialize ", (TRUE == vec3.isFinite()));//need more test cases:
		vec3.clearVec();
		ensure("2:clearVec:Fail to set values ", ((0.f == vec3.mV[VX]) && (0.f == vec3.mV[VY]) && (0.f == vec3.mV[VZ])));
		vec3.setVec(x,y,z);
		ensure("3:setVec:Fail to set values ", ((2.32f == vec3.mV[VX]) && (1.212f == vec3.mV[VY]) && (-.12f == vec3.mV[VZ])));
		vec3.zeroVec();
		ensure("4:zeroVec:Fail to set values ", ((0.f == vec3.mV[VX]) && (0.f == vec3.mV[VY]) && (0.f == vec3.mV[VZ])));
	}
	
	template<> template<>
	void v3math_object::test<6>()
	{
		F32 x = 2.32f, y = 1.212f, z = -.12f;
		LLVector3 vec3(x,y,z),vec3a;
		vec3.abs();
		ensure("1:abs:Fail ", ((x == vec3.mV[VX]) && (y == vec3.mV[VY]) && (-z == vec3.mV[VZ])));
		vec3a.setVec(vec3);
		ensure("2:setVec:Fail to initialize ", (vec3a == vec3));	
		const F32 vec[3] = {1.2f ,3.2f, -4.2f};
		vec3.clearVec();
		vec3.setVec(vec);
		ensure("3:setVec:Fail to initialize ", ((1.2f == vec3.mV[VX]) && (3.2f == vec3.mV[VY]) && (-4.2f == vec3.mV[VZ])));
		vec3a.clearVec();
		LLVector3d vector3d(vec3);
		vec3a.setVec(vector3d);
		ensure("4:setVec:Fail to initialize ", (vec3 == vec3a));
		LLVector4 vector4(vec3);
		vec3a.clearVec();
		vec3a.setVec(vector4);
		ensure("5:setVec:Fail to initialize ", (vec3 == vec3a));
	}

	template<> template<>
	void v3math_object::test<7>()
	{
		F32 x = 2.32f, y = 3.212f, z = -.12f;
		F32 min = 0.0001f, max = 3.0f;
		LLVector3 vec3(x,y,z);		
		ensure("1:clamp:Fail  ", TRUE == vec3.clamp(min, max) && x == vec3.mV[VX] && max == vec3.mV[VY] && min == vec3.mV[VZ]);
		x = 1.f, y = 2.2f, z = 2.8f;
		vec3.setVec(x,y,z);
		ensure("2:clamp:Fail  ", FALSE == vec3.clamp(min, max));
	}

	template<> template<>
	void v3math_object::test<8>()
	{
		F32 x = 2.32f, y = 1.212f, z = -.12f;
		LLVector3 vec3(x,y,z);		
		ensure("1:magVecSquared:Fail ", is_approx_equal(vec3.magVecSquared(), (x*x + y*y + z*z)));
		ensure("2:magVec:Fail ", is_approx_equal(vec3.magVec(), (F32) sqrt(x*x + y*y + z*z)));
	}

	template<> template<>
	void v3math_object::test<9>()
	{
		F32 x =-2.0f, y = -3.0f, z = 1.23f ;
		LLVector3 vec3(x,y,z);
		ensure("1:abs():Fail ", (TRUE == vec3.abs()));	
		ensure("2:isNull():Fail", (FALSE == vec3.isNull()));	//Returns TRUE if vector has a _very_small_ length
		x =.00000001f, y = .000001001f, z = .000001001f;
		vec3.setVec(x,y,z);
		ensure("3:isNull(): Fail ", (TRUE == vec3.isNull()));	
	}

	template<> template<>
	void v3math_object::test<10>()
	{
		F32 x =-2.0f, y = -3.0f, z = 1.f ;
		LLVector3 vec3(x,y,z),vec3a;
		ensure("1:isExactlyZero():Fail ", (TRUE == vec3a.isExactlyZero()));
		vec3a = vec3a.scaleVec(vec3);
		ensure("2:scaleVec: Fail ", vec3a.mV[VX] == 0.f && vec3a.mV[VY] == 0.f && vec3a.mV[VZ] == 0.f);
		vec3a.setVec(x,y,z);
		vec3a = vec3a.scaleVec(vec3);
		ensure("3:scaleVec: Fail ", ((4 == vec3a.mV[VX]) && (9 == vec3a.mV[VY]) &&(1 == vec3a.mV[VZ])));
		ensure("4:isExactlyZero():Fail ", (FALSE == vec3.isExactlyZero()));
	}

	template<> template<>
	void v3math_object::test<11>()
	{
		F32 x =20.0f, y = 30.0f, z = 15.f ;
		F32 angle = 100.f;
		LLVector3 vec3(x,y,z),vec3a(1.f,2.f,3.f);
		vec3a = vec3a.rotVec(angle, vec3);
		LLVector3 vec3b(1.f,2.f,3.f);
		vec3b = vec3b.rotVec(angle, vec3);
		ensure_equals("rotVec():Fail" ,vec3b,vec3a);
	}

	template<> template<>
	void v3math_object::test<12>()
	{
		F32 x =-2.0f, y = -3.0f, z = 1.f ;
		LLVector3 vec3(x,y,z);
		ensure("1:operator [] failed",( x ==  vec3[0]));	
		ensure("2:operator [] failed",( y ==  vec3[1]));
		ensure("3:operator [] failed",( z ==  vec3[2]));

		vec3.clearVec();
		x = 23.f, y = -.2361f, z = 3.25;
		vec3.setVec(x,y,z);
		F32 &ref1 = vec3[0];
		ensure("4:operator [] failed",( ref1 ==  vec3[0]));
		F32 &ref2 = vec3[1];
		ensure("5:operator [] failed",( ref2 ==  vec3[1]));
		F32 &ref3 = vec3[2];
		ensure("6:operator [] failed",( ref3 ==  vec3[2]));
	}

	template<> template<>
	void v3math_object::test<13>()
	{
		F32 x1 =1.f, y1 = 2.f,z1 = 1.2f, x2 = -2.3f, y2 = 1.11f, z2 = 1234.234f;
		F32 val1, val2, val3;
		LLVector3 vec3(x1,y1,z1), vec3a(x2,y2,z2), vec3b;
		vec3b = vec3 + vec3a ;
		val1 = x1+x2;
		val2 = y1+y2;
		val3 = z1+z2;
		ensure("1:operator+ failed",(val1 == vec3b.mV[VX]) && (val2 == vec3b.mV[VY]) && (val3 == vec3b.mV[VZ])); 

		vec3.clearVec();
		vec3a.clearVec();
		vec3b.clearVec();
		x1 = -.235f, y1 = -24.32f,z1 = 2.13f,  x2 = -2.3f, y2 = 1.f, z2 = 34.21f;
		vec3.setVec(x1,y1,z1);
		vec3a.setVec(x2,y2,z2);
		vec3b = vec3 + vec3a;
		val1 = x1+x2;
		val2 = y1+y2;
		val3 = z1+z2;
		ensure("2:operator+ failed",(val1 == vec3b.mV[VX]) && (val2 == vec3b.mV[VY]) && (val3 == vec3b.mV[VZ])); 
	}

	template<> template<>
	void v3math_object::test<14>()
	{
		F32 x1 =1.f, y1 = 2.f,z1 = 1.2f, x2 = -2.3f, y2 = 1.11f, z2 = 1234.234f;
		F32 val1, val2, val3;
		LLVector3 vec3(x1,y1,z1), vec3a(x2,y2,z2), vec3b;
		vec3b = vec3 - vec3a ;
		val1 = x1-x2;
		val2 = y1-y2;
		val3 = z1-z2;
		ensure("1:operator- failed",(val1 == vec3b.mV[VX]) && (val2 == vec3b.mV[VY]) && (val3 == vec3b.mV[VZ])); 

		vec3.clearVec();
		vec3a.clearVec();
		vec3b.clearVec();
		x1 = -.235f, y1 = -24.32f,z1 = 2.13f,  x2 = -2.3f, y2 = 1.f, z2 = 34.21f;
		vec3.setVec(x1,y1,z1);
		vec3a.setVec(x2,y2,z2);
		vec3b = vec3 - vec3a;
		val1 = x1-x2;
		val2 = y1-y2;
		val3 = z1-z2;
		ensure("2:operator- failed",(val1 == vec3b.mV[VX]) && (val2 == vec3b.mV[VY]) && (val3 == vec3b.mV[VZ])); 
	}

	template<> template<>
	void v3math_object::test<15>()
	{
		F32 x1 =1.f, y1 = 2.f,z1 = 1.2f, x2 = -2.3f, y2 = 1.11f, z2 = 1234.234f;
		F32 val1, val2, val3;
		LLVector3 vec3(x1,y1,z1), vec3a(x2,y2,z2);
		val1 = vec3 * vec3a;
		val2 = x1*x2 + y1*y2 + z1*z2;
		ensure_equals("1:operator* failed",val1,val2);

		vec3a.clearVec();
		F32 mulVal = 4.332f;
		vec3a = vec3 * mulVal;
		val1 = x1*mulVal;
		val2 = y1*mulVal;
		val3 = z1*mulVal;
		ensure("2:operator* failed",(val1 == vec3a.mV[VX]) && (val2 == vec3a.mV[VY])&& (val3 == vec3a.mV[VZ]));
		vec3a.clearVec();
		vec3a = mulVal * vec3;
		ensure("3:operator* failed ", (val1 == vec3a.mV[VX]) && (val2 == vec3a.mV[VY])&& (val3 == vec3a.mV[VZ]));
	}

	template<> template<>
	void v3math_object::test<16>()
	{
		F32 x1 =1.f, y1 = 2.f,z1 = 1.2f, x2 = -2.3f, y2 = 1.11f, z2 = 1234.234f;
		F32 val1, val2, val3;
		LLVector3 vec3(x1,y1,z1), vec3a(x2,y2,z2), vec3b;
		vec3b = vec3 % vec3a ;
		val1 = y1*z2 - y2*z1;
		val2 = z1*x2 -z2*x1;
		val3 = x1*y2-x2*y1;
		ensure("1:operator% failed",(val1 == vec3b.mV[VX]) && (val2 == vec3b.mV[VY]) && (val3 == vec3b.mV[VZ])); 

		vec3.clearVec();
		vec3a.clearVec();
		vec3b.clearVec();
		x1 =112.f, y1 = 22.3f,z1 = 1.2f, x2 = -2.3f, y2 = 341.11f, z2 = 1234.234f;
		vec3.setVec(x1,y1,z1);
		vec3a.setVec(x2,y2,z2);
		vec3b = vec3 % vec3a ;
		val1 = y1*z2 - y2*z1;
		val2 = z1*x2 -z2*x1;
		val3 = x1*y2-x2*y1;
		ensure("2:operator% failed ", (val1 == vec3b.mV[VX]) && (val2 == vec3b.mV[VY]) && (val3 == vec3b.mV[VZ])); 
	}

	template<> template<>
	void v3math_object::test<17>()
	{
		F32 x1 =1.f, y1 = 2.f,z1 = 1.2f, div = 3.2f;
		F32 t = 1.f / div, val1, val2, val3;
		LLVector3 vec3(x1,y1,z1), vec3a;
		vec3a = vec3 / div;
		val1 = x1 * t;
		val2 = y1 * t;
		val3 = z1 *t;
		ensure("1:operator/ failed",(val1 == vec3a.mV[VX]) && (val2 == vec3a.mV[VY]) && (val3 == vec3a.mV[VZ]));		

		vec3a.clearVec();
		x1 = -.235f, y1 = -24.32f, z1 = .342f, div = -2.2f;
		t = 1.f / div;
		vec3.setVec(x1,y1,z1);
		vec3a = vec3 / div;
		val1 = x1 * t;
		val2 = y1 * t;
		val3 = z1 *t;
		ensure("2:operator/ failed",(val1 == vec3a.mV[VX]) && (val2 == vec3a.mV[VY]) && (val3 == vec3a.mV[VZ]));		
	}

	template<> template<>
	void v3math_object::test<18>()
	{
		F32 x1 =1.f, y1 = 2.f,z1 = 1.2f;
		LLVector3 vec3(x1,y1,z1), vec3a(x1,y1,z1);
		ensure("1:operator== failed",(vec3 == vec3a));		

		vec3a.clearVec();
		x1 = -.235f, y1 = -24.32f, z1 = .342f;
		vec3.clearVec();
		vec3a.clearVec();
		vec3.setVec(x1,y1,z1);
		vec3a.setVec(x1,y1,z1);
		ensure("2:operator== failed ", (vec3 == vec3a));		
	}

	template<> template<>
	void v3math_object::test<19>()
	{
		F32 x1 =1.f, y1 = 2.f,z1 = 1.2f, x2 =112.f, y2 = 2.234f,z2 = 11.2f;;
		LLVector3 vec3(x1,y1,z1), vec3a(x2,y2,z2);
		ensure("1:operator!= failed",(vec3a != vec3));

		vec3.clearVec();
		vec3.clearVec();
		vec3a.setVec(vec3);
		ensure("2:operator!= failed", ( FALSE == (vec3a != vec3)));
	}

	template<> template<>
	void v3math_object::test<20>()
	{
		F32 x1 =1.f, y1 = 2.f,z1 = 1.2f, x2 =112.f, y2 = 2.2f,z2 = 11.2f;;
		LLVector3 vec3(x1,y1,z1), vec3a(x2,y2,z2);
		vec3a += vec3;
		F32 val1, val2, val3;
		val1 = x1+x2;
		val2 = y1+y2;
		val3 = z1+z2;
		ensure("1:operator+= failed",(val1 == vec3a.mV[VX]) && (val2 == vec3a.mV[VY])&& (val3 == vec3a.mV[VZ]));
	}
	
	template<> template<>
	void v3math_object::test<21>()
	{
		F32 x1 =1.f, y1 = 2.f,z1 = 1.2f, x2 =112.f, y2 = 2.2f,z2 = 11.2f;;
		LLVector3 vec3(x1,y1,z1), vec3a(x2,y2,z2);
		vec3a -= vec3;
		F32 val1, val2, val3;
		val1 = x2-x1;
		val2 = y2-y1;
		val3 = z2-z1;
		ensure("1:operator-= failed",(val1 == vec3a.mV[VX]) && (val2 == vec3a.mV[VY])&& (val3 == vec3a.mV[VZ]));
	}

	template<> template<>
	void v3math_object::test<22>()
	{
		F32 x1 =1.f, y1 = 2.f,z1 = 1.2f, x2 = -2.3f, y2 = 1.11f, z2 = 1234.234f;
		F32 val1,val2,val3;
		LLVector3 vec3(x1,y1,z1), vec3a(x2,y2,z2);
		vec3a *= vec3;
		val1 = x1*x2;
		val2 = y1*y2;
		val3 = z1*z2;
		ensure("1:operator*= failed",(val1 == vec3a.mV[VX]) && (val2 == vec3a.mV[VY])&& (val3 == vec3a.mV[VZ]));

		F32 mulVal = 4.332f;
		vec3 *=mulVal;
		val1 = x1*mulVal;
		val2 = y1*mulVal;
		val3 = z1*mulVal;
		ensure("2:operator*= failed ", is_approx_equal(val1, vec3.mV[VX]) && is_approx_equal(val2, vec3.mV[VY]) && is_approx_equal(val3, vec3.mV[VZ]));
	}

	template<> template<>
	void v3math_object::test<23>()
	{
		F32 x1 =1.f, y1 = 2.f,z1 = 1.2f, x2 = -2.3f, y2 = 1.11f, z2 = 1234.234f;
		LLVector3 vec3(x1,y1,z1), vec3a(x2,y2,z2),vec3b;
		vec3b = vec3a % vec3;
		vec3a %= vec3;
		ensure_equals("1:operator%= failed",vec3a,vec3b); 
	}

	template<> template<>
	void v3math_object::test<24>()
	{
		F32 x1 =1.f, y1 = 2.f,z1 = 1.2f, div = 3.2f;
		F32 t = 1.f / div, val1, val2, val3;
		LLVector3 vec3a(x1,y1,z1);
		vec3a /= div;
		val1 = x1 * t;
		val2 = y1 * t;
		val3 = z1 *t;
		ensure("1:operator/= failed",(val1 == vec3a.mV[VX]) && (val2 == vec3a.mV[VY]) && (val3 == vec3a.mV[VZ]));		
	}

	template<> template<>
	void v3math_object::test<25>()
	{
		F32 x1 =1.f, y1 = 2.f,z1 = 1.2f;
		LLVector3 vec3(x1,y1,z1), vec3a;
		vec3a = -vec3;
		ensure("1:operator- failed",(-vec3a == vec3));	
	}

	template<> template<>
	void v3math_object::test<26>()
	{
		F32 x1 =1.f, y1 = 2.f,z1 = 1.2f;
		std::ostringstream stream1, stream2;
		LLVector3 vec3(x1,y1,z1), vec3a;
		stream1 << vec3;
		vec3a.setVec(x1,y1,z1);
		stream2 << vec3a;
		ensure("1:operator << failed",(stream1.str() == stream2.str()));	
	}

	template<> template<>
	void v3math_object::test<27>()
	{
		F32 x1 =-2.3f, y1 = 2.f,z1 = 1.2f, x2 = 1.3f, y2 = 1.11f, z2 = 1234.234f;
		LLVector3 vec3(x1,y1,z1), vec3a(x2,y2,z2);
		ensure("1:operator< failed", (TRUE == (vec3 < vec3a)));
		x1 =-2.3f, y1 = 2.f,z1 = 1.2f, x2 = 1.3f, y2 = 2.f, z2 = 1234.234f;
		vec3.setVec(x1,y1,z1);
		vec3a.setVec(x2,y2,z2);
		ensure("2:operator< failed ", (TRUE == (vec3 < vec3a)));
		x1 =2.3f, y1 = 2.f,z1 = 1.2f, x2 = 1.3f,
		vec3.setVec(x1,y1,z1);
		vec3a.setVec(x2,y2,z2);
		ensure("3:operator< failed ", (FALSE == (vec3 < vec3a)));
	}

	template<> template<>
	void v3math_object::test<28>()
	{
		F32 x1 =1.23f, y1 = 2.f,z1 = 4.f;
		std::string buf("1.23 2. 4");
		LLVector3 vec3, vec3a(x1,y1,z1);
		LLVector3::parseVector3(buf, &vec3);
		ensure_equals("1:parseVector3 failed", vec3, vec3a);	
	}

	template<> template<>
	void v3math_object::test<29>()
	{
		F32 x1 =1.f, y1 = 2.f,z1 = 4.f;
		LLVector3 vec3(x1,y1,z1),vec3a,vec3b;
		vec3a.setVec(1,1,1);
		vec3a.scaleVec(vec3);
		ensure_equals("1:scaleVec failed", vec3, vec3a);	
		vec3a.clearVec();
		vec3a.setVec(x1,y1,z1);
		vec3a.scaleVec(vec3);
		ensure("2:scaleVec failed", ((1.f ==vec3a.mV[VX])&& (4.f ==vec3a.mV[VY]) && (16.f ==vec3a.mV[VZ])));		
	}
	
	template<> template<>
	void v3math_object::test<30>()
	{
		F32 x1 =-2.3f, y1 = 2.f,z1 = 1.2f, x2 = 1.3f, y2 = 1.11f, z2 = 1234.234f;
		F32 val = 2.3f,val1,val2,val3;
		val1 = x1 + (x2 - x1)* val;
		val2 = y1 + (y2 - y1)* val;
		val3 = z1 + (z2 - z1)* val;
		LLVector3 vec3(x1,y1,z1),vec3a(x2,y2,z2);
		LLVector3 vec3b = lerp(vec3,vec3a,val);
		ensure("1:lerp failed", ((val1 ==vec3b.mV[VX])&& (val2 ==vec3b.mV[VY]) && (val3 ==vec3b.mV[VZ])));		
	}

	template<> template<>
	void v3math_object::test<31>()
	{
		F32 x1 =-2.3f, y1 = 2.f,z1 = 1.2f, x2 = 1.3f, y2 = 1.f, z2 = 1.f;
		F32 val1,val2;
		LLVector3 vec3(x1,y1,z1),vec3a(x2,y2,z2);
		val1 = dist_vec(vec3,vec3a);
		val2 = (F32) sqrt((x1 - x2)*(x1 - x2) + (y1 - y2)* (y1 - y2) + (z1 - z2)* (z1 -z2));
		ensure_equals("1:dist_vec: Fail ",val2, val1);
		val1 = dist_vec_squared(vec3,vec3a);
		val2 =((x1 - x2)*(x1 - x2) + (y1 - y2)* (y1 - y2) + (z1 - z2)* (z1 -z2));
		ensure_equals("2:dist_vec_squared: Fail ",val2, val1);
		val1 = dist_vec_squared2D(vec3, vec3a);
		val2 =(x1 - x2)*(x1 - x2) + (y1 - y2)* (y1 - y2);
		ensure_equals("3:dist_vec_squared2D: Fail ",val2, val1);
	}

	template<> template<>
	void v3math_object::test<32>()
	{
		F32 x =12.3524f, y = -342.f,z = 4.126341f;
		LLVector3 vec3(x,y,z);
		F32 mag = vec3.normVec();
		mag = 1.f/ mag;
		F32 val1 = x* mag, val2 = y* mag, val3 = z* mag;
		ensure("1:normVec: Fail ", is_approx_equal(val1, vec3.mV[VX]) && is_approx_equal(val2, vec3.mV[VY]) && is_approx_equal(val3, vec3.mV[VZ]));
		x = 0.000000001f, y = 0.f, z = 0.f;
		vec3.clearVec();
		vec3.setVec(x,y,z);
		mag = vec3.normVec();
		val1 = x* mag, val2 = y* mag, val3 = z* mag;
		ensure("2:normVec: Fail ", (mag == 0.) && (0. == vec3.mV[VX]) && (0. == vec3.mV[VY])&& (0. == vec3.mV[VZ]));
	}

	template<> template<>
	void v3math_object::test<33>()
	{
		F32 x = -202.23412f, y = 123.2312f, z = -89.f;
		LLVector3 vec(x,y,z);
		vec.snap(2);
		ensure("1:snap: Fail ", is_approx_equal(-202.23f, vec.mV[VX]) && is_approx_equal(123.23f, vec.mV[VY]) && is_approx_equal(-89.f, vec.mV[VZ]));
	}
		
	template<> template<>
	void v3math_object::test<34>()
	{
		F32 x = 10.f, y = 20.f, z = -15.f;
		F32 x1, y1, z1;
		F32 lowerxy = 0.f, upperxy = 1.0f, lowerz = -1.0f, upperz = 1.f;
		LLVector3 vec3(x,y,z);
		vec3.quantize16(lowerxy,upperxy,lowerz,upperz);
		x1 = U16_to_F32(F32_to_U16(x, lowerxy, upperxy), lowerxy, upperxy);
		y1 = U16_to_F32(F32_to_U16(y, lowerxy, upperxy), lowerxy, upperxy);
		z1 = U16_to_F32(F32_to_U16(z, lowerz,  upperz),  lowerz,  upperz);
		ensure("1:quantize16: Fail ", is_approx_equal(x1, vec3.mV[VX]) && is_approx_equal(y1, vec3.mV[VY]) && is_approx_equal(z1, vec3.mV[VZ]));
		LLVector3 vec3a(x,y,z);
		vec3a.quantize8(lowerxy,upperxy,lowerz,upperz);
		x1 = U8_to_F32(F32_to_U8(x, lowerxy, upperxy), lowerxy, upperxy);
		y1 = U8_to_F32(F32_to_U8(y, lowerxy, upperxy), lowerxy, upperxy);
		z1 = U8_to_F32(F32_to_U8(z, lowerz, upperz), lowerz, upperz);
		ensure("2:quantize8: Fail ", is_approx_equal(x1, vec3a.mV[VX]) && is_approx_equal(y1, vec3a.mV[VY]) && is_approx_equal(z1, vec3a.mV[VZ]));
	}
}
