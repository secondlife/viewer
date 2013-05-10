/**
 * @file v3color_test.cpp
 * @author Adroit
 * @date 2007-03
 * @brief v3color test cases.
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

#include "../v3color.h"


namespace tut
{
	struct v3color_data
	{
	};
	typedef test_group<v3color_data> v3color_test;
	typedef v3color_test::object v3color_object;
	tut::v3color_test v3color_testcase("v3color_h");

	template<> template<>
	void v3color_object::test<1>()
	{
		LLColor3 llcolor3;
		ensure("1:LLColor3:Fail to default-initialize ", (0.0f == llcolor3.mV[0]) && (0.0f == llcolor3.mV[1]) && (0.0f == llcolor3.mV[2]));
		F32 r = 2.0f, g = 3.2f, b = 1.f;
		F32 v1,v2,v3;
		LLColor3 llcolor3a(r,g,b);
		ensure("2:LLColor3:Fail to initialize " ,(2.0f == llcolor3a.mV[0]) && (3.2f == llcolor3a.mV[1]) && (1.f == llcolor3a.mV[2]));
		
		const F32 vec[3] = {2.0f, 3.2f,1.f};
		LLColor3 llcolor3b(vec);
		ensure("3:LLColor3:Fail to initialize " ,(2.0f == llcolor3b.mV[0]) && (3.2f == llcolor3b.mV[1]) && (1.f == llcolor3b.mV[2]));
		const char* str = "561122";
		LLColor3 llcolor3c(str);
		v1 = (F32)86.0f/255.0f; // 0x56 = 86
		v2 = (F32)17.0f/255.0f; // 0x11 = 17
		v3 = (F32)34.0f/255.f;  // 0x22 = 34
		ensure("4:LLColor3:Fail to initialize " , is_approx_equal(v1, llcolor3c.mV[0]) && is_approx_equal(v2, llcolor3c.mV[1]) && is_approx_equal(v3, llcolor3c.mV[2]));
	}

	template<> template<>
	void v3color_object::test<2>()
	{
		LLColor3 llcolor3;
		llcolor3.setToBlack();
		ensure("setToBlack:Fail to set black ", ((llcolor3.mV[0] == 0.f) && (llcolor3.mV[1] == 0.f) && (llcolor3.mV[2] == 0.f)));
		llcolor3.setToWhite();
		ensure("setToWhite:Fail to set white  ", ((llcolor3.mV[0] == 1.f) && (llcolor3.mV[1] == 1.f) && (llcolor3.mV[2] == 1.f)));
	}

	template<> template<>
	void v3color_object::test<3>()
	{
		F32 r = 2.3436212f, g = 1231.f, b = 4.7849321232f;
		LLColor3 llcolor3, llcolor3a;
		llcolor3.setVec(r,g,b);
		ensure("1:setVec(r,g,b) Fail ",((r == llcolor3.mV[0]) && (g == llcolor3.mV[1]) && (b == llcolor3.mV[2])));
		llcolor3a.setVec(llcolor3);
		ensure_equals("2:setVec(LLColor3) Fail ", llcolor3,llcolor3a);
		F32 vec[3] = {1.2324f, 2.45634f, .234563f};
		llcolor3.setToBlack();
		llcolor3.setVec(vec);
		ensure("3:setVec(F32*) Fail ",((vec[0] == llcolor3.mV[0]) && (vec[1] == llcolor3.mV[1]) && (vec[2] == llcolor3.mV[2])));
	}

	template<> template<>
	void v3color_object::test<4>()
	{
		F32 r = 2.3436212f, g = 1231.f, b = 4.7849321232f;
		LLColor3 llcolor3(r,g,b);
		ensure("magVecSquared:Fail ", is_approx_equal(llcolor3.magVecSquared(), (r*r + g*g + b*b)));
		ensure("magVec:Fail ", is_approx_equal(llcolor3.magVec(), (F32) sqrt(r*r + g*g + b*b)));
	}

	template<> template<>
	void v3color_object::test<5>()
	{
		F32 r = 2.3436212f, g = 1231.f, b = 4.7849321232f;
		F32 val1, val2,val3;
		LLColor3 llcolor3(r,g,b);
		F32 vecMag = llcolor3.normVec();
		F32 mag = (F32) sqrt(r*r + g*g + b*b);
		F32 oomag = 1.f / mag;
		val1 = r * oomag;
		val2 = g * oomag;
		val3 = b * oomag;
		ensure("1:normVec failed ", (is_approx_equal(val1, llcolor3.mV[0]) && is_approx_equal(val2, llcolor3.mV[1]) && is_approx_equal(val3, llcolor3.mV[2]) && is_approx_equal(vecMag, mag)));
		r = .000000000f, g = 0.f, b = 0.0f;
		llcolor3.setVec(r,g,b);
		vecMag = llcolor3.normVec();
		ensure("2:normVec failed should be 0. ", (0. == llcolor3.mV[0] && 0. == llcolor3.mV[1] && 0. == llcolor3.mV[2] && vecMag == 0.));
	}

	template<> template<>
	void v3color_object::test<6>()
	{
		F32 r = 2.3436212f, g = -1231.f, b = .7849321232f;
		std::ostringstream stream1, stream2;
		LLColor3 llcolor3(r,g,b),llcolor3a;
		stream1 << llcolor3;
		llcolor3a.setVec(r,g,b);
		stream2 << llcolor3a;
		ensure("operator << failed ", (stream1.str() == stream2.str()));	
	}
		
	template<> template<>
	void v3color_object::test<7>()
	{
		F32 r = 2.3436212f, g = -1231.f, b = .7849321232f;
		LLColor3 llcolor3(r,g,b),llcolor3a;
		llcolor3a = llcolor3;
		ensure("operator == failed ", (llcolor3a == llcolor3));	
	}

	template<> template<>
	void v3color_object::test<8>()
	{
		F32 r1 =1.f, g1 = 2.f,b1 = 1.2f, r2 = -2.3f, g2 = 1.11f, b2 = 1234.234f;
		LLColor3 llcolor3(r1,g1,b1),llcolor3a(r2,g2,b2),llcolor3b;
		llcolor3b = llcolor3 + llcolor3a ;
		ensure("1:operator+ failed",is_approx_equal(r1+r2 ,llcolor3b.mV[0]) && is_approx_equal(g1+g2,llcolor3b.mV[1])&& is_approx_equal(b1+b2,llcolor3b.mV[2]));
		r1 = -.235f, g1 = -24.32f, b1 = 2.13f,  r2 = -2.3f, g2 = 1.f, b2 = 34.21f;
		llcolor3.setVec(r1,g1,b1);
		llcolor3a.setVec(r2,g2,b2);
		llcolor3b = llcolor3 + llcolor3a;
		ensure("2:operator+ failed",is_approx_equal(r1+r2 ,llcolor3b.mV[0]) && is_approx_equal(g1+g2,llcolor3b.mV[1])&& is_approx_equal(b1+b2,llcolor3b.mV[2]));
	}

	template<> template<>
	void v3color_object::test<9>()
	{
		F32 r1 =1.f, g1 = 2.f,b1 = 1.2f, r2 = -2.3f, g2 = 1.11f, b2 = 1234.234f;
		LLColor3 llcolor3(r1,g1,b1),llcolor3a(r2,g2,b2),llcolor3b;
		llcolor3b = llcolor3 - llcolor3a ;
		ensure("1:operator- failed",is_approx_equal(r1-r2 ,llcolor3b.mV[0]) && is_approx_equal(g1-g2,llcolor3b.mV[1])&& is_approx_equal(b1-b2,llcolor3b.mV[2]));
		r1 = -.235f, g1 = -24.32f, b1 = 2.13f,  r2 = -2.3f, g2 = 1.f, b2 = 34.21f;
		llcolor3.setVec(r1,g1,b1);
		llcolor3a.setVec(r2,g2,b2);
		llcolor3b = llcolor3 - llcolor3a;
		ensure("2:operator- failed",is_approx_equal(r1-r2 ,llcolor3b.mV[0]) && is_approx_equal(g1-g2,llcolor3b.mV[1])&& is_approx_equal(b1-b2,llcolor3b.mV[2]));
	}

	template<> template<>
	void v3color_object::test<10>()
	{
		F32 r1 =1.f, g1 = 2.f,b1 = 1.2f, r2 = -2.3f, g2 = 1.11f, b2 = 1234.234f;
		LLColor3 llcolor3(r1,g1,b1),llcolor3a(r2,g2,b2),llcolor3b;
		llcolor3b = llcolor3 * llcolor3a;
		ensure("1:operator* failed",is_approx_equal(r1*r2 ,llcolor3b.mV[0]) && is_approx_equal(g1*g2,llcolor3b.mV[1])&& is_approx_equal(b1*b2,llcolor3b.mV[2]));
		llcolor3a.setToBlack();
		F32 mulVal = 4.332f;
		llcolor3a = llcolor3 * mulVal;
		ensure("2:operator* failed",is_approx_equal(r1*mulVal ,llcolor3a.mV[0]) && is_approx_equal(g1*mulVal,llcolor3a.mV[1])&& is_approx_equal(b1*mulVal,llcolor3a.mV[2]));
		llcolor3a.setToBlack();
		llcolor3a = mulVal * llcolor3;
		ensure("3:operator* failed",is_approx_equal(r1*mulVal ,llcolor3a.mV[0]) && is_approx_equal(g1*mulVal,llcolor3a.mV[1])&& is_approx_equal(b1*mulVal,llcolor3a.mV[2]));
	}

	template<> template<>
	void v3color_object::test<11>()
	{
		F32 r = 2.3436212f, g = 1231.f, b = 4.7849321232f;
		LLColor3 llcolor3(r,g,b),llcolor3a;
		llcolor3a = -llcolor3;
		ensure("operator- failed ", (-llcolor3a == llcolor3));	
	}

	template<> template<>
	void v3color_object::test<12>()
	{
		F32 r = 2.3436212f, g = 1231.f, b = 4.7849321232f;
		LLColor3 llcolor3(r,g,b),llcolor3a(r,g,b);
		ensure_equals("1:operator== failed",llcolor3a,llcolor3);
		r = 13.3436212f, g = -11.f, b = .7849321232f;
		llcolor3.setVec(r,g,b);
		llcolor3a.setVec(r,g,b);
		ensure_equals("2:operator== failed",llcolor3a,llcolor3);
	}

	template<> template<>
	void v3color_object::test<13>()
	{
		F32 r1 =1.f, g1 = 2.f,b1 = 1.2f, r2 = -2.3f, g2 = 1.11f, b2 = 1234.234f;
		LLColor3 llcolor3(r1,g1,b1),llcolor3a(r2,g2,b2);
		ensure("1:operator!= failed",(llcolor3 != llcolor3a));
		llcolor3.setToBlack();
		llcolor3a.setVec(llcolor3);
		ensure("2:operator!= failed", ( FALSE == (llcolor3a != llcolor3)));
	}

	template<> template<>
	void v3color_object::test<14>()
	{
		F32 r1 =1.f, g1 = 2.f,b1 = 1.2f, r2 = -2.3f, g2 = 1.11f, b2 = 1234.234f;
		LLColor3 llcolor3(r1,g1,b1),llcolor3a(r2,g2,b2);
		llcolor3a += llcolor3;
		ensure("1:operator+= failed",is_approx_equal(r1+r2 ,llcolor3a.mV[0]) && is_approx_equal(g1+g2,llcolor3a.mV[1])&& is_approx_equal(b1+b2,llcolor3a.mV[2]));
		llcolor3.setVec(r1,g1,b1);
		llcolor3a.setVec(r2,g2,b2);
		llcolor3a += llcolor3;
		ensure("2:operator+= failed",is_approx_equal(r1+r2 ,llcolor3a.mV[0]) && is_approx_equal(g1+g2,llcolor3a.mV[1])&& is_approx_equal(b1+b2,llcolor3a.mV[2]));
	}

	template<> template<>
	void v3color_object::test<15>()
	{
		F32 r1 =1.f, g1 = 2.f,b1 = 1.2f, r2 = -2.3f, g2 = 1.11f, b2 = 1234.234f;
		LLColor3 llcolor3(r1,g1,b1),llcolor3a(r2,g2,b2);
		llcolor3a -= llcolor3;
		ensure("1:operator-= failed", is_approx_equal(r2-r1, llcolor3a.mV[0]));
		ensure("2:operator-= failed", is_approx_equal(g2-g1, llcolor3a.mV[1]));
		ensure("3:operator-= failed", is_approx_equal(b2-b1, llcolor3a.mV[2]));
		llcolor3.setVec(r1,g1,b1);
		llcolor3a.setVec(r2,g2,b2);
		llcolor3a -= llcolor3;
		ensure("4:operator-= failed", is_approx_equal(r2-r1, llcolor3a.mV[0]));
		ensure("5:operator-= failed", is_approx_equal(g2-g1, llcolor3a.mV[1]));
		ensure("6:operator-= failed", is_approx_equal(b2-b1, llcolor3a.mV[2]));
	}
	
	template<> template<>
	void v3color_object::test<16>()
	{
		F32 r1 =1.f, g1 = 2.f,b1 = 1.2f, r2 = -2.3f, g2 = 1.11f, b2 = 1234.234f;
		LLColor3 llcolor3(r1,g1,b1),llcolor3a(r2,g2,b2);
		llcolor3a *= llcolor3;
		ensure("1:operator*= failed",is_approx_equal(r1*r2 ,llcolor3a.mV[0]) && is_approx_equal(g1*g2,llcolor3a.mV[1])&& is_approx_equal(b1*b2,llcolor3a.mV[2]));
		F32 mulVal = 4.332f;
		llcolor3 *=mulVal;
		ensure("2:operator*= failed",is_approx_equal(r1*mulVal ,llcolor3.mV[0]) && is_approx_equal(g1*mulVal,llcolor3.mV[1])&& is_approx_equal(b1*mulVal,llcolor3.mV[2]));
	}

	template<> template<>
	void v3color_object::test<17>()
	{
		F32 r = 2.3436212f, g = -1231.f, b = .7849321232f;
		LLColor3 llcolor3(r,g,b);
		llcolor3.clamp();
		ensure("1:clamp:Fail to clamp " ,(1.0f == llcolor3.mV[0]) && (0.f == llcolor3.mV[1]) && (b == llcolor3.mV[2]));
		r = -2.3436212f, g = -1231.f, b = 67.7849321232f;
		llcolor3.setVec(r,g,b);
		llcolor3.clamp();
		ensure("2:clamp:Fail to clamp " ,(0.f == llcolor3.mV[0]) && (0.f == llcolor3.mV[1]) && (1.f == llcolor3.mV[2]));
	}

	template<> template<>
	void v3color_object::test<18>()
	{
		F32 r1 =1.f, g1 = 2.f,b1 = 1.2f, r2 = -2.3f, g2 = 1.11f, b2 = 1234.234f;
		F32 val = 2.3f,val1,val2,val3;
		LLColor3 llcolor3(r1,g1,b1),llcolor3a(r2,g2,b2);
		val1 = r1 + (r2 - r1)* val;
		val2 = g1 + (g2 - g1)* val;
		val3 = b1 + (b2 - b1)* val;
		LLColor3 llcolor3b = lerp(llcolor3,llcolor3a,val);
		ensure("lerp failed ", ((val1 ==llcolor3b.mV[0])&& (val2 ==llcolor3b.mV[1]) && (val3 ==llcolor3b.mV[2])));		
	}

	template<> template<>
	void v3color_object::test<19>()
	{
		F32 r1 =1.f, g1 = 2.f,b1 = 1.2f, r2 = -2.3f, g2 = 1.11f, b2 = 1234.234f;
		LLColor3 llcolor3(r1,g1,b1),llcolor3a(r2,g2,b2);
		F32 val = distVec(llcolor3,llcolor3a);
		ensure("distVec failed ", is_approx_equal((F32) sqrt((r1-r2)*(r1-r2) + (g1-g2)*(g1-g2) + (b1-b2)*(b1-b2)) ,val));
		
		F32 val1 = distVec_squared(llcolor3,llcolor3a);
		ensure("distVec_squared failed ", is_approx_equal(((r1-r2)*(r1-r2) + (g1-g2)*(g1-g2) + (b1-b2)*(b1-b2)) ,val1));
	}

	template<> template<>
	void v3color_object::test<20>()
	{
		F32 r1 = 1.02223f, g1 = 22222.212f, b1 = 122222.00002f;
		LLColor3 llcolor31(r1,g1,b1);

		LLSD sd = llcolor31.getValue();
		LLColor3 llcolor32;
		llcolor32.setValue(sd);
		ensure_equals("LLColor3::setValue/getValue failed", llcolor31, llcolor32);

		LLColor3 llcolor33(sd);
		ensure_equals("LLColor3(LLSD) failed", llcolor31, llcolor33);
	}
}
