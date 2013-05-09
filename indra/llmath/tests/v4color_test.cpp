/**
 * @file v4color_test.cpp
 * @author Adroit
 * @date 2007-03
 * @brief v4color test cases.
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

#include "../v4coloru.h"
#include "llsd.h"
#include "../v3color.h"
#include "../v4color.h"


namespace tut
{
	struct v4color_data
	{
	};
	typedef test_group<v4color_data> v4color_test;
	typedef v4color_test::object v4color_object;
	tut::v4color_test v4color_testcase("v4color_h");

	template<> template<>
	void v4color_object::test<1>()
	{
		LLColor4 llcolor4;
		ensure("1:LLColor4:Fail to initialize ", ((0 == llcolor4.mV[VX]) && (0 == llcolor4.mV[VY]) && (0 == llcolor4.mV[VZ])&& (1.0f == llcolor4.mV[VW])));

		F32 r = 0x20, g = 0xFFFF, b = 0xFF, a = 0xAF;
		LLColor4 llcolor4a(r,g,b);
		ensure("2:LLColor4:Fail to initialize ", ((r == llcolor4a.mV[VX]) && (g == llcolor4a.mV[VY]) && (b == llcolor4a.mV[VZ])&& (1.0f == llcolor4a.mV[VW])));
		
		LLColor4 llcolor4b(r,g,b,a);
		ensure("3:LLColor4:Fail to initialize ", ((r == llcolor4b.mV[VX]) && (g == llcolor4b.mV[VY]) && (b == llcolor4b.mV[VZ])&& (a == llcolor4b.mV[VW])));
		
		const F32 vec[4] = {.112f ,23.2f, -4.2f, -.0001f};
		LLColor4 llcolor4c(vec);
		ensure("4:LLColor4:Fail to initialize ", ((vec[0] == llcolor4c.mV[VX]) && (vec[1] == llcolor4c.mV[VY]) && (vec[2] == llcolor4c.mV[VZ])&& (vec[3] == llcolor4c.mV[VW])));		
		
		LLColor3 llcolor3(-2.23f,1.01f,42.3f);
		F32 val = -.1f;
		LLColor4 llcolor4d(llcolor3,val);
		ensure("5:LLColor4:Fail to initialize ", ((llcolor3.mV[VX] == llcolor4d.mV[VX]) && (llcolor3.mV[VY] == llcolor4d.mV[VY]) && (llcolor3.mV[VZ] == llcolor4d.mV[VZ])&& (val == llcolor4d.mV[VW])));

		LLSD sd = llcolor4d.getValue();
		LLColor4 llcolor4e(sd);
		ensure_equals("6:LLColor4:(LLSD) failed ", llcolor4d, llcolor4e);
		
		U8 r1 = 0xF2, g1 = 0xFA, b1 = 0xBF;
		LLColor4U color4u(r1,g1,b1);
		LLColor4 llcolor4g(color4u);
		const F32 SCALE = 1.f/255.f;
		F32 r2 = r1*SCALE, g2 = g1* SCALE, b2 = b1* SCALE;
		ensure("7:LLColor4:Fail to initialize ", ((r2 == llcolor4g.mV[VX]) && (g2 == llcolor4g.mV[VY]) && (b2 == llcolor4g.mV[VZ])));
	}

	template<> template<>
	void v4color_object::test<2>()
	{
		LLColor4 llcolor(1.0, 2.0, 3.0, 4.0);
		LLSD llsd = llcolor.getValue();
		LLColor4 llcolor4(llsd), llcolor4a;
		llcolor4a.setValue(llsd);
		ensure("setValue: failed", (llcolor4 == llcolor4a));
		LLSD sd = llcolor4a.getValue();
		LLColor4 llcolor4b(sd);
		ensure("getValue: Failed ", (llcolor4b == llcolor4a));
	}

	template<> template<>
	void v4color_object::test<3>()
	{
		F32 r = 0x20, g = 0xFFFF, b = 0xFF,a = 0xAF;
		LLColor4 llcolor4(r,g,b,a);
		llcolor4.setToBlack();
		ensure("setToBlack:Fail to set the black ", ((0 == llcolor4.mV[VX]) && (0 == llcolor4.mV[VY]) && (0 == llcolor4.mV[VZ])&& (1.0f == llcolor4.mV[VW])));

		llcolor4.setToWhite();
		ensure("setToWhite:Fail to set the white ", ((1.f == llcolor4.mV[VX]) && (1.f == llcolor4.mV[VY]) && (1.f == llcolor4.mV[VZ])&& (1.0f == llcolor4.mV[VW])));
	}

	template<> template<>
	void v4color_object::test<4>()
	{
		F32 r = 0x20, g = 0xFFFF, b = 0xFF, a = 0xAF;
		LLColor4 llcolor4;
		llcolor4.setVec(r,g,b);
		ensure("1:setVec:Fail to set the values ", ((r == llcolor4.mV[VX]) && (g == llcolor4.mV[VY]) && (b == llcolor4.mV[VZ])&& (1.f == llcolor4.mV[VW])));

		llcolor4.setVec(r,g,b,a);
		ensure("2:setVec:Fail to set the values ", ((r == llcolor4.mV[VX]) && (g == llcolor4.mV[VY]) && (b == llcolor4.mV[VZ])&& (a == llcolor4.mV[VW])));

		LLColor4 llcolor4a; 
		llcolor4a.setVec(llcolor4);
		ensure_equals("3:setVec:Fail to set the values ", llcolor4a,llcolor4);

		LLColor3 llcolor3(-2.23f,1.01f,42.3f);
		llcolor4a.setVec(llcolor3);
		ensure("4:setVec:Fail to set the values ", ((llcolor3.mV[VX] == llcolor4a.mV[VX]) && (llcolor3.mV[VY] == llcolor4a.mV[VY]) && (llcolor3.mV[VZ] == llcolor4a.mV[VZ])));

		F32 val = -.33f;
		llcolor4a.setVec(llcolor3,val);
		ensure("4:setVec:Fail to set the values ", ((llcolor3.mV[VX] == llcolor4a.mV[VX]) && (llcolor3.mV[VY] == llcolor4a.mV[VY]) && (llcolor3.mV[VZ] == llcolor4a.mV[VZ]) && (val == llcolor4a.mV[VW])));

		const F32 vec[4] = {.112f ,23.2f, -4.2f, -.0001f};
		LLColor4 llcolor4c;
		llcolor4c.setVec(vec);
		ensure("5:setVec:Fail to initialize ", ((vec[0] == llcolor4c.mV[VX]) && (vec[1] == llcolor4c.mV[VY]) && (vec[2] == llcolor4c.mV[VZ])&& (vec[3] == llcolor4c.mV[VW])));		

		U8 r1 = 0xF2, g1 = 0xFA, b1= 0xBF;
		LLColor4U color4u(r1,g1,b1);
		llcolor4.setVec(color4u);
		const F32 SCALE = 1.f/255.f;
		F32 r2 = r1*SCALE, g2 = g1* SCALE, b2 = b1* SCALE;
		ensure("6:setVec:Fail to initialize ", ((r2 == llcolor4.mV[VX]) && (g2 == llcolor4.mV[VY]) && (b2 == llcolor4.mV[VZ])));
	}

	template<> template<>
	void v4color_object::test<5>()
	{
		F32 alpha = 0xAF;
		LLColor4 llcolor4;
		llcolor4.setAlpha(alpha);
		ensure("setAlpha:Fail to initialize ", (alpha == llcolor4.mV[VW]));
	}

	template<> template<>
	void v4color_object::test<6>()
	{
		F32 r = 0x20, g = 0xFFFF, b = 0xFF;
		LLColor4 llcolor4(r,g,b);
		ensure("magVecSquared:Fail ", is_approx_equal(llcolor4.magVecSquared(), (r*r + g*g + b*b)));
		ensure("magVec:Fail ", is_approx_equal(llcolor4.magVec(), (F32) sqrt(r*r + g*g + b*b)));
	}

	template<> template<>
	void v4color_object::test<7>()
	{
		F32 r = 0x20, g = 0xFFFF, b = 0xFF;
		LLColor4 llcolor4(r,g,b);
		F32 vecMag = llcolor4.normVec();
		F32 mag = (F32) sqrt(r*r + g*g + b*b);
		F32 oomag = 1.f / mag;
		F32 val1 = r * oomag, val2 = g * oomag,	val3 = b * oomag;
		ensure("1:normVec failed ", (is_approx_equal(val1, llcolor4.mV[0]) && is_approx_equal(val2, llcolor4.mV[1]) && is_approx_equal(val3, llcolor4.mV[2]) && is_approx_equal(vecMag, mag)));
	}

	template<> template<>
	void v4color_object::test<8>()
	{
		LLColor4 llcolor4;
		ensure("1:isOpaque failed ",(1 == llcolor4.isOpaque()));
		F32 r = 0x20, g = 0xFFFF, b = 0xFF,a = 1.f;
		llcolor4.setVec(r,g,b,a);
		ensure("2:isOpaque failed ",(1 == llcolor4.isOpaque()));
		a = 2.f;
		llcolor4.setVec(r,g,b,a);
		ensure("3:isOpaque failed ",(0 == llcolor4.isOpaque()));
	}

	template<> template<>
	void v4color_object::test<9>()
	{
		F32 r = 0x20, g = 0xFFFF, b = 0xFF;
		LLColor4 llcolor4(r,g,b);
		ensure("1:operator [] failed",( r ==  llcolor4[0]));	
		ensure("2:operator [] failed",( g ==  llcolor4[1]));
		ensure("3:operator [] failed",( b ==  llcolor4[2]));

		r = 0xA20, g = 0xFBFF, b = 0xFFF;
		llcolor4.setVec(r,g,b);
		F32 &ref1 = llcolor4[0];
		ensure("4:operator [] failed",( ref1 ==  llcolor4[0]));
		F32 &ref2 = llcolor4[1];
		ensure("5:operator [] failed",( ref2 ==  llcolor4[1]));
		F32 &ref3 = llcolor4[2];
		ensure("6:operator [] failed",( ref3 ==  llcolor4[2]));
	}

	template<> template<>
	void v4color_object::test<10>()
	{
		F32 r = 0x20, g = 0xFFFF, b = 0xFF;
		LLColor3 llcolor3(r,g,b);
		LLColor4 llcolor4a,llcolor4b;
		llcolor4a = llcolor3;
		ensure("Operator=:Fail to initialize ", ((llcolor3.mV[0] == llcolor4a.mV[VX]) && (llcolor3.mV[1] == llcolor4a.mV[VY]) && (llcolor3.mV[2] == llcolor4a.mV[VZ])));
		LLSD sd = llcolor4a.getValue();
		llcolor4b = LLColor4(sd);
		ensure_equals("Operator= LLSD:Fail ", llcolor4a, llcolor4b);
	}

	template<> template<>
	void v4color_object::test<11>()
	{
		F32 r = 0x20, g = 0xFFFF, b = 0xFF;
		std::ostringstream stream1, stream2;
		LLColor4 llcolor4a(r,g,b),llcolor4b;
		stream1 << llcolor4a;
		llcolor4b.setVec(r,g,b);
		stream2 << llcolor4b;
		ensure("operator << failed ", (stream1.str() == stream2.str()));	
	}

	template<> template<>
	void v4color_object::test<12>()
	{
		F32 r1 = 0x20, g1 = 0xFFFF, b1 = 0xFF;
		F32 r2 = 0xABF, g2 = 0xFB, b2 = 0xFFF;
		LLColor4 llcolor4a(r1,g1,b1),llcolor4b(r2,g2,b2),llcolor4c;
		llcolor4c = llcolor4b + llcolor4a;
		ensure("operator+:Fail to Add the values ",  (is_approx_equal(r1+r2,llcolor4c.mV[VX]) && is_approx_equal(g1+g2,llcolor4c.mV[VY]) && is_approx_equal(b1+b2,llcolor4c.mV[VZ])));

		llcolor4b += llcolor4a;
		ensure("operator+=:Fail to Add the values ",  (is_approx_equal(r1+r2,llcolor4b.mV[VX]) && is_approx_equal(g1+g2,llcolor4b.mV[VY]) && is_approx_equal(b1+b2,llcolor4b.mV[VZ])));
	}

	template<> template<>
	void v4color_object::test<13>()
	{
		F32 r1 = 0x20, g1 = 0xFFFF, b1 = 0xFF;
		F32 r2 = 0xABF, g2 = 0xFB, b2 = 0xFFF;
		LLColor4 llcolor4a(r1,g1,b1),llcolor4b(r2,g2,b2),llcolor4c;
		llcolor4c = llcolor4a - llcolor4b;
		ensure("operator-:Fail to subtract the values ",  (is_approx_equal(r1-r2,llcolor4c.mV[VX]) && is_approx_equal(g1-g2,llcolor4c.mV[VY]) && is_approx_equal(b1-b2,llcolor4c.mV[VZ])));

		llcolor4a -= llcolor4b;
		ensure("operator-=:Fail to subtract the values ",  (is_approx_equal(r1-r2,llcolor4a.mV[VX]) && is_approx_equal(g1-g2,llcolor4a.mV[VY]) && is_approx_equal(b1-b2,llcolor4a.mV[VZ])));
	}

	template<> template<>
	void v4color_object::test<14>()
	{
		F32 r1 = 0x20, g1 = 0xFFFF, b1 = 0xFF;
		F32 r2 = 0xABF, g2 = 0xFB, b2 = 0xFFF;
		LLColor4 llcolor4a(r1,g1,b1),llcolor4b(r2,g2,b2),llcolor4c;
		llcolor4c = llcolor4a * llcolor4b;
		ensure("1:operator*:Fail to multiply the values",  (is_approx_equal(r1*r2,llcolor4c.mV[VX]) && is_approx_equal(g1*g2,llcolor4c.mV[VY]) && is_approx_equal(b1*b2,llcolor4c.mV[VZ])));
		
		F32 mulVal = 3.33f;
		llcolor4c = llcolor4a * mulVal;
		ensure("2:operator*:Fail ",  (is_approx_equal(r1*mulVal,llcolor4c.mV[VX]) && is_approx_equal(g1*mulVal,llcolor4c.mV[VY]) && is_approx_equal(b1*mulVal,llcolor4c.mV[VZ])));
		llcolor4c = mulVal * llcolor4a;
		ensure("3:operator*:Fail to multiply the values",  (is_approx_equal(r1*mulVal,llcolor4c.mV[VX]) && is_approx_equal(g1*mulVal,llcolor4c.mV[VY]) && is_approx_equal(b1*mulVal,llcolor4c.mV[VZ])));

		llcolor4a *= mulVal;
		ensure("4:operator*=:Fail to multiply the values ",  (is_approx_equal(r1*mulVal,llcolor4a.mV[VX]) && is_approx_equal(g1*mulVal,llcolor4a.mV[VY]) && is_approx_equal(b1*mulVal,llcolor4a.mV[VZ])));

		LLColor4 llcolor4d(r1,g1,b1),llcolor4e(r2,g2,b2);
		llcolor4e *= llcolor4d;
		ensure("5:operator*=:Fail to multiply the values ",  (is_approx_equal(r1*r2,llcolor4e.mV[VX]) && is_approx_equal(g1*g2,llcolor4e.mV[VY]) && is_approx_equal(b1*b2,llcolor4e.mV[VZ])));
	}

	template<> template<>
	void v4color_object::test<15>()
	{
		F32 r = 0x20, g = 0xFFFF, b = 0xFF,a = 0x30;
		F32 div = 12.345f;
		LLColor4 llcolor4a(r,g,b,a),llcolor4b;
		llcolor4b = llcolor4a % div;//chnage only alpha value nor r,g,b;
		ensure("1operator%:Fail ",  (is_approx_equal(r,llcolor4b.mV[VX]) && is_approx_equal(g,llcolor4b.mV[VY]) && is_approx_equal(b,llcolor4b.mV[VZ])&& is_approx_equal(div*a,llcolor4b.mV[VW])));
		
		llcolor4b = div % llcolor4a;
		ensure("2operator%:Fail ",  (is_approx_equal(r,llcolor4b.mV[VX]) && is_approx_equal(g,llcolor4b.mV[VY]) && is_approx_equal(b,llcolor4b.mV[VZ])&& is_approx_equal(div*a,llcolor4b.mV[VW])));

		llcolor4a %= div;
		ensure("operator%=:Fail ",  (is_approx_equal(a*div,llcolor4a.mV[VW])));
	}

	template<> template<>
	void v4color_object::test<16>()
	{
		F32 r = 0x20, g = 0xFFFF, b = 0xFF,a = 0x30;
		LLColor4 llcolor4a(r,g,b,a),llcolor4b;
		llcolor4b = llcolor4a;
		ensure("1:operator== failed to ensure the equality ", (llcolor4b == llcolor4a));	
		F32 r1 = 0x2, g1 = 0xFF, b1 = 0xFA;
		LLColor3 llcolor3(r1,g1,b1);
		llcolor4b = llcolor3;
		ensure("2:operator== failed to ensure the equality ", (llcolor4b == llcolor3));	
		ensure("2:operator!= failed to ensure the equality ", (llcolor4a != llcolor3));
	}

	template<> template<>
	void v4color_object::test<17>()
	{
		F32 r = 0x20, g = 0xFFFF, b = 0xFF;
		LLColor4 llcolor4a(r,g,b),llcolor4b;
		LLColor3 llcolor3 = vec4to3(llcolor4a);
		ensure("vec4to3:Fail to convert vec4 to vec3 ",  (is_approx_equal(llcolor3.mV[VX],llcolor4a.mV[VX]) && is_approx_equal(llcolor3.mV[VY],llcolor4a.mV[VY]) && is_approx_equal(llcolor3.mV[VZ],llcolor4a.mV[VZ])));
		llcolor4b = vec3to4(llcolor3);
		ensure_equals("vec3to4:Fail to convert vec3 to vec4 ",  llcolor4b, llcolor4a);
	}

	template<> template<>
	void v4color_object::test<18>()
	{
		F32 r1 = 0x20, g1 = 0xFFFF, b1 = 0xFF, val = 0x20;
		F32 r2 = 0xABF, g2 = 0xFB, b2 = 0xFFF;
		LLColor4 llcolor4a(r1,g1,b1),llcolor4b(r2,g2,b2),llcolor4c;
		llcolor4c = lerp(llcolor4a,llcolor4b,val);
		ensure("lerp:Fail ",  (is_approx_equal(r1 + (r2 - r1)* val,llcolor4c.mV[VX]) && is_approx_equal(g1 + (g2 - g1)* val,llcolor4c.mV[VY]) && is_approx_equal(b1 + (b2 - b1)* val,llcolor4c.mV[VZ])));
	}

	template<> template<>
	void v4color_object::test<19>()
	{
		F32 r = 12.0f, g = -2.3f, b = 1.32f, a = 5.0f;
		LLColor4 llcolor4a(r,g,b,a),llcolor4b;
		std::string color("red");
		LLColor4::parseColor(color, &llcolor4b);
		ensure_equals("1:parseColor() failed to parse the color value ", llcolor4b, LLColor4::red);

		color = "12.0, -2.3, 1.32, 5.0";
		LLColor4::parseColor(color, &llcolor4b);
		llcolor4a = llcolor4a * (1.f / 255.f);
		ensure_equals("2:parseColor() failed to parse the color value ",  llcolor4a,llcolor4b);

		color = "yellow5";
		llcolor4a.setVec(r,g,b);
		LLColor4::parseColor(color, &llcolor4a);
		ensure_equals("3:parseColor() failed to parse the color value ", llcolor4a, LLColor4::yellow5);
	}

	template<> template<>
	void v4color_object::test<20>()
	{
		F32 r = 12.0f, g = -2.3f, b = 1.32f, a = 5.0f;
		LLColor4 llcolor4a(r,g,b,a),llcolor4b;
		std::string color("12.0, -2.3, 1.32, 5.0");
		LLColor4::parseColor4(color, &llcolor4b);
		ensure_equals("parseColor4() failed to parse the color value ",  llcolor4a, llcolor4b);
	}
}
