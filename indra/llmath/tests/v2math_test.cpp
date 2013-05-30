/**
 * @file v2math_test.cpp
 * @author Adroit
 * @date 2007-02
 * @brief v2math test cases.
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

#include "../v2math.h"


namespace tut
{
	struct v2math_data
	{
	};
	typedef test_group<v2math_data> v2math_test;
	typedef v2math_test::object v2math_object;
	tut::v2math_test v2math_testcase("v2math_h");

	template<> template<>
	void v2math_object::test<1>()
	{
		LLVector2 vec2;
		ensure("LLVector2:Fail to initialize ", (0.f == vec2.mV[VX] && 0.f == vec2.mV[VY]));

		F32 x =2.0f, y = 3.2f ;
		LLVector2 vec3(x,y);
		ensure("LLVector2(F32 x, F32 y):Fail to initialize ", (x == vec3.mV[VX]) && (y == vec3.mV[VY]));

		const F32 vec[2] = {3.2f, 4.5f};
		LLVector2 vec4(vec);
		ensure("LLVector2(const F32 *vec):Fail to initialize ", (vec[0] == vec4.mV[VX]) && (vec[1] == vec4.mV[VY]));

		vec4.clearVec();
		ensure("clearVec():Fail to clean the values ", (0.f == vec4.mV[VX] && 0.f == vec4.mV[VY]));

		vec3.zeroVec();
		ensure("zeroVec():Fail to fill the zero ", (0.f == vec3.mV[VX] && 0.f == vec3.mV[VY]));
	}

	template<> template<>
	void v2math_object::test<2>()
	{
		F32 x = 123.356f, y = 2387.453f;
		LLVector2 vec2,vec3;
		vec2.setVec(x, y);
		ensure("1:setVec: Fail  ", (x == vec2.mV[VX]) && (y == vec2.mV[VY]));

		vec3.setVec(vec2);
		ensure("2:setVec: Fail   " ,(vec2 == vec3));

		vec3.zeroVec();
		const F32 vec[2] = {3.24653f, 457653.4f};
		vec3.setVec(vec);
		ensure("3:setVec: Fail  ", (vec[0] == vec3.mV[VX]) && (vec[1] == vec3.mV[VY]));
	}

	template<> template<>
	void v2math_object::test<3>()
	{
		F32 x = 2.2345f, y = 3.5678f ;
		LLVector2 vec2(x,y);
		ensure("magVecSquared:Fail ", is_approx_equal(vec2.magVecSquared(), (x*x + y*y)));
		ensure("magVec:Fail ", is_approx_equal(vec2.magVec(), (F32) sqrt(x*x + y*y)));
	}

	template<> template<>
	void v2math_object::test<4>()
	{
		F32 x =-2.0f, y = -3.0f ;
		LLVector2 vec2(x,y);
		ensure_equals("abs():Fail", vec2.abs(), TRUE);
		ensure("abs() x", is_approx_equal(vec2.mV[VX], 2.f));
		ensure("abs() y", is_approx_equal(vec2.mV[VY], 3.f));

		ensure("isNull():Fail ", FALSE == vec2.isNull());	//Returns TRUE if vector has a _very_small_ length

		x =.00000001f, y = .000001001f;
		vec2.setVec(x, y);
		ensure("isNull(): Fail ", TRUE == vec2.isNull());	
	}

	template<> template<>
	void v2math_object::test<5>()
	{
		F32 x =1.f, y = 2.f;
		LLVector2 vec2(x, y), vec3;
		vec3 = vec3.scaleVec(vec2);
		ensure("scaleVec: Fail ", vec3.mV[VX] == 0. && vec3.mV[VY] == 0.);
		ensure("isExactlyZero(): Fail", TRUE == vec3.isExactlyZero());

		vec3.setVec(2.f, 1.f);
		vec3 = vec3.scaleVec(vec2);
		ensure("scaleVec: Fail ", (2.f == vec3.mV[VX]) && (2.f == vec3.mV[VY]));
		ensure("isExactlyZero():Fail", FALSE == vec3.isExactlyZero());
	}

	template<> template<>
	void v2math_object::test<6>()
	{
		F32 x1 =1.f, y1 = 2.f, x2 = -2.3f, y2 = 1.11f;
		F32 val1, val2;
		LLVector2 vec2(x1, y1), vec3(x2, y2), vec4;
		vec4 = vec2 + vec3 ;
		val1 = x1+x2;
		val2 = y1+y2;
		ensure("1:operator+ failed",(val1 == vec4.mV[VX]) && ((val2 == vec4.mV[VY]))); 

		vec2.clearVec();
		vec3.clearVec();
		x1 = -.235f, y1 = -24.32f,  x2 = -2.3f, y2 = 1.f;
		vec2.setVec(x1, y1);
		vec3.setVec(x2, y2);
		vec4 = vec2 + vec3;
		val1 = x1+x2;
		val2 = y1+y2;
		ensure("2:operator+ failed",(val1 == vec4.mV[VX]) && ((val2 == vec4.mV[VY]))); 
	}

	template<> template<>
	void v2math_object::test<7>()
	{
		F32 x1 =1.f, y1 = 2.f,  x2 = -2.3f, y2 = 1.11f;
		F32 val1, val2;
		LLVector2 vec2(x1, y1), vec3(x2, y2), vec4;
		vec4 = vec2 - vec3 ;
		val1 = x1-x2;
		val2 = y1-y2;
		ensure("1:operator- failed",(val1 == vec4.mV[VX]) && ((val2 == vec4.mV[VY]))); 

		vec2.clearVec();
		vec3.clearVec();
		vec4.clearVec();
		x1 = -.235f, y1 = -24.32f,  x2 = -2.3f, y2 = 1.f;
		vec2.setVec(x1, y1);
		vec3.setVec(x2, y2);
		vec4 = vec2 - vec3;
		val1 = x1-x2;
		val2 = y1-y2;
		ensure("2:operator- failed",(val1 == vec4.mV[VX]) && ((val2 == vec4.mV[VY]))); 
	}

	template<> template<>
	void v2math_object::test<8>()
	{
		F32 x1 =1.f, y1 = 2.f,  x2 = -2.3f, y2 = 1.11f;
		F32 val1, val2;
		LLVector2 vec2(x1, y1), vec3(x2, y2);
		val1 = vec2 * vec3;
		val2 = x1*x2 + y1*y2;
		ensure("1:operator* failed",(val1 == val2));

		vec3.clearVec();
		F32 mulVal = 4.332f;
		vec3 = vec2 * mulVal;
		val1 = x1*mulVal;
		val2 = y1*mulVal;
		ensure("2:operator* failed",(val1 == vec3.mV[VX]) && (val2 == vec3.mV[VY]));

		vec3.clearVec();
		vec3 = mulVal * vec2;
		ensure("3:operator* failed",(val1 == vec3.mV[VX]) && (val2 == vec3.mV[VY]));		
	}

	template<> template<>
	void v2math_object::test<9>()
	{
		F32 x1 =1.f, y1 = 2.f, div = 3.2f;
		F32 val1, val2;
		LLVector2 vec2(x1, y1), vec3;
		vec3 = vec2 / div;
		val1 = x1 / div;
		val2 = y1 / div;
		ensure("1:operator/ failed", is_approx_equal(val1, vec3.mV[VX]) && is_approx_equal(val2, vec3.mV[VY]));		

		vec3.clearVec();
		x1 = -.235f, y1 = -24.32f, div = -2.2f;
		vec2.setVec(x1, y1);
		vec3 = vec2 / div;
		val1 = x1 / div;
		val2 = y1 / div;
		ensure("2:operator/ failed", is_approx_equal(val1, vec3.mV[VX]) && is_approx_equal(val2, vec3.mV[VY]));		
	}

	template<> template<>
	void v2math_object::test<10>()
	{
		F32 x1 =1.f, y1 = 2.f,  x2 = -2.3f, y2 = 1.11f;
		F32 val1, val2;
		LLVector2 vec2(x1, y1), vec3(x2, y2), vec4;
		vec4 = vec2 % vec3;
		val1 = x1*y2 - x2*y1;
		val2 = y1*x2 - y2*x1;
		ensure("1:operator% failed",(val1 == vec4.mV[VX]) && (val2 == vec4.mV[VY]));	

		vec2.clearVec();
		vec3.clearVec();
		vec4.clearVec();
		x1 = -.235f, y1 = -24.32f,  x2 = -2.3f, y2 = 1.f;
		vec2.setVec(x1, y1);
		vec3.setVec(x2, y2);
		vec4 = vec2 % vec3;
		val1 = x1*y2 - x2*y1;
		val2 = y1*x2 - y2*x1;
		ensure("2:operator% failed",(val1 == vec4.mV[VX]) && (val2 == vec4.mV[VY]));	
	}
	template<> template<>
	void v2math_object::test<11>()
	{
		F32 x1 =1.f, y1 = 2.f;
		LLVector2 vec2(x1, y1), vec3(x1, y1);
		ensure("1:operator== failed",(vec2 == vec3));
		
		vec2.clearVec();
		vec3.clearVec();
		x1 = -.235f, y1 = -24.32f;
		vec2.setVec(x1, y1);
		vec3.setVec(vec2);
		ensure("2:operator== failed",(vec2 == vec3));
	}

	template<> template<>
	void v2math_object::test<12>()
	{
		F32 x1 = 1.f, y1 = 2.f,x2 = 2.332f, y2 = -1.23f;
		LLVector2 vec2(x1, y1), vec3(x2, y2);
		ensure("1:operator!= failed",(vec2 != vec3));
		
		vec2.clearVec();
		vec3.clearVec();
		vec2.setVec(x1, y1);
		vec3.setVec(vec2);
		ensure("2:operator!= failed", (FALSE == (vec2 != vec3)));
	}
	template<> template<>
	void v2math_object::test<13>()
	{
		F32 x1 = 1.f, y1 = 2.f,x2 = 2.332f, y2 = -1.23f;
		F32 val1, val2;
		LLVector2 vec2(x1, y1), vec3(x2, y2);
		vec2 +=vec3;
		val1 = x1+x2;
		val2 = y1+y2;
		ensure("1:operator+= failed",(val1 == vec2.mV[VX]) && (val2 == vec2.mV[VY]));
		
		vec2.setVec(x1, y1);
		vec2 -=vec3;
		val1 = x1-x2;
		val2 = y1-y2;
		ensure("2:operator-= failed",(val1 == vec2.mV[VX]) && (val2 == vec2.mV[VY]));
		
		vec2.clearVec();
		vec3.clearVec();
		x1 = -21.000466f, y1 = 2.98382f,x2 = 0.332f, y2 = -01.23f;
		vec2.setVec(x1, y1);
		vec3.setVec(x2, y2);
		vec2 +=vec3;
		val1 = x1+x2;
		val2 = y1+y2;
		ensure("3:operator+= failed",(val1 == vec2.mV[VX]) && (val2 == vec2.mV[VY]));

		vec2.setVec(x1, y1);
		vec2 -=vec3;
		val1 = x1-x2;
		val2 = y1-y2;
		ensure("4:operator-= failed", is_approx_equal(val1, vec2.mV[VX]) && is_approx_equal(val2, vec2.mV[VY]));
	}

	template<> template<>
	void v2math_object::test<14>()
	{
		F32 x1 =1.f, y1 = 2.f;
		F32 val1, val2, mulVal = 4.332f;
		LLVector2 vec2(x1, y1);
		vec2 /=mulVal;
		val1 = x1 / mulVal;
		val2 = y1 / mulVal;
		ensure("1:operator/= failed", is_approx_equal(val1, vec2.mV[VX]) && is_approx_equal(val2, vec2.mV[VY]));
		
		vec2.clearVec();
		x1 = .213f, y1 = -2.34f, mulVal = -.23f;
		vec2.setVec(x1, y1);
		vec2 /=mulVal;
		val1 = x1 / mulVal;
		val2 = y1 / mulVal;
		ensure("2:operator/= failed", is_approx_equal(val1, vec2.mV[VX]) && is_approx_equal(val2, vec2.mV[VY]));
	}

	template<> template<>
	void v2math_object::test<15>()
	{
		F32 x1 =1.f, y1 = 2.f;
		F32 val1, val2, mulVal = 4.332f;
		LLVector2 vec2(x1, y1);
		vec2 *=mulVal;
		val1 = x1*mulVal;
		val2 = y1*mulVal;
		ensure("1:operator*= failed",(val1 == vec2.mV[VX]) && (val2 == vec2.mV[VY]));
		
		vec2.clearVec();
		x1 = .213f, y1 = -2.34f, mulVal = -.23f;
		vec2.setVec(x1, y1);
		vec2 *=mulVal;
		val1 = x1*mulVal;
		val2 = y1*mulVal;
		ensure("2:operator*= failed",(val1 == vec2.mV[VX]) && (val2 == vec2.mV[VY]));
	}
	
	template<> template<>
	void v2math_object::test<16>()
	{
		F32 x1 =1.f, y1 = 2.f,  x2 = -2.3f, y2 = 1.11f;
		F32 val1, val2;
		LLVector2 vec2(x1, y1), vec3(x2, y2);
		vec2 %= vec3;
		val1 = x1*y2 - x2*y1;
		val2 = y1*x2 - y2*x1;
		ensure("1:operator%= failed",(val1 == vec2.mV[VX]) && (val2 == vec2.mV[VY]));	
	}

	template<> template<>
	void v2math_object::test<17>()
	{
		F32 x1 =1.f, y1 = 2.f;
		LLVector2 vec2(x1, y1),vec3;
		vec3 = -vec2;
		ensure("1:operator- failed",(-vec3 == vec2));	
	}

	template<> template<>
	void v2math_object::test<18>()
	{
		F32 x1 =1.f, y1 = 2.f;
		std::ostringstream stream1, stream2;
		LLVector2 vec2(x1, y1),vec3;
		stream1 << vec2;
		vec3.setVec(x1, y1);
		stream2 << vec3;
		ensure("1:operator << failed",(stream1.str() == stream2.str()));	
	}

	template<> template<>
	void v2math_object::test<19>()
	{
		F32 x1 =1.0f, y1 = 2.0f, x2 = -.32f, y2 = .2234f;
		LLVector2 vec2(x1, y1),vec3(x2, y2);
		ensure("1:operator < failed",(vec3 < vec2));	

		x1 = 1.0f, y1 = 2.0f, x2 = 1.0f, y2 = 3.2234f;
		vec2.setVec(x1, y1);
		vec3.setVec(x2, y2);
		ensure("2:operator < failed", (FALSE == (vec3 < vec2)));	
	}

	template<> template<>
	void v2math_object::test<20>()
	{
		F32 x1 =1.0f, y1 = 2.0f;
		LLVector2 vec2(x1, y1);
		ensure("1:operator [] failed",( x1 ==  vec2[0]));	
		ensure("2:operator [] failed",( y1 ==  vec2[1]));

		vec2.clearVec();
		x1 = 23.0f, y1 = -.2361f;
		vec2.setVec(x1, y1);
		F32 ref1 = vec2[0];
		ensure("3:operator [] failed", ( ref1 ==  x1));
		F32 ref2 = vec2[1];
		ensure("4:operator [] failed", ( ref2 ==  y1));
	}

	template<> template<>
	void v2math_object::test<21>()
	{
		F32 x1 =1.f, y1 = 2.f, x2 = -.32f, y2 = .2234f;
		F32 val1, val2;
		LLVector2 vec2(x1, y1),vec3(x2, y2);		
		val1 = dist_vec_squared2D(vec2, vec3);
		val2 = (x1 - x2)*(x1 - x2) + (y1 - y2)* (y1 - y2);
		ensure_equals("dist_vec_squared2D values are not equal",val2, val1);

		val1 = dist_vec_squared(vec2, vec3);
		ensure_equals("dist_vec_squared values are not equal",val2, val1);

		val1 = 	dist_vec(vec2, vec3);
		val2 = (F32) sqrt((x1 - x2)*(x1 - x2) + (y1 - y2)* (y1 - y2));
		ensure_equals("dist_vec values are not equal",val2, val1);
	}

	template<> template<>
	void v2math_object::test<22>()
	{
		F32 x1 =1.f, y1 = 2.f, x2 = -.32f, y2 = .2234f,fVal = .0121f;
		F32 val1, val2;
		LLVector2 vec2(x1, y1),vec3(x2, y2);
		LLVector2 vec4 = lerp(vec2, vec3, fVal);
		val1 = x1 + (x2 - x1) * fVal;
		val2 = y1 + (y2 - y1) * fVal;
		ensure("lerp values are not equal", ((val1 == vec4.mV[VX]) && (val2 == vec4.mV[VY])));
	}

	template<> template<>
	void v2math_object::test<23>()
	{
		F32 x1 =1.f, y1 = 2.f;
		F32 val1, val2;
		LLVector2 vec2(x1, y1);

		F32 vecMag = vec2.normVec();
		F32 mag = (F32) sqrt(x1*x1 + y1*y1);

		F32 oomag = 1.f / mag;
		val1 = x1 * oomag;
		val2 = y1 * oomag;

		ensure("normVec failed", is_approx_equal(val1, vec2.mV[VX]) && is_approx_equal(val2, vec2.mV[VY]) && is_approx_equal(vecMag, mag));

		x1 =.00000001f, y1 = 0.f;

		vec2.setVec(x1, y1);
		vecMag = vec2.normVec();
		ensure("normVec failed should be 0.", 0. == vec2.mV[VX] && 0. == vec2.mV[VY] && vecMag == 0.);
	}
}
