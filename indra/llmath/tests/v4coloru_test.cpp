/**
 * @file v4coloru_tut.cpp
 * @author Adroit
 * @date 2007-03
 * @brief v4coloru test cases.
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007-2009, Linden Research, Inc.
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
#include "../test/lltut.h"

#include "llsd.h"

#include "../v4coloru.h"


namespace tut
{
	struct v4coloru_data
	{
	};
	typedef test_group<v4coloru_data> v4coloru_test;
	typedef v4coloru_test::object v4coloru_object;
	tut::v4coloru_test v4coloru_testcase("v4coloru");

	template<> template<>
	void v4coloru_object::test<1>()
	{
		LLColor4U llcolor4u;
		ensure("1:LLColor4u:Fail to initialize ", ((0 == llcolor4u.mV[VX]) && (0 == llcolor4u.mV[VY]) && (0 == llcolor4u.mV[VZ])&& (255 == llcolor4u.mV[VW])));

		U8 r = 0x12, g = 0xFF, b = 0xAF, a = 0x23;
		LLColor4U llcolor4u1(r,g,b);
		ensure("2:LLColor4u:Fail to initialize ", ((r == llcolor4u1.mV[VX]) && (g == llcolor4u1.mV[VY]) && (b == llcolor4u1.mV[VZ])&& (255 == llcolor4u1.mV[VW])));
		
		LLColor4U llcolor4u2(r,g,b,a);
		ensure("3:LLColor4u:Fail to initialize ", ((r == llcolor4u2.mV[VX]) && (g == llcolor4u2.mV[VY]) && (b == llcolor4u2.mV[VZ])&& (a == llcolor4u2.mV[VW])));

		const U8 vec[4] = {0x12,0xFF,0xAF,0x23};
		LLColor4U llcolor4u3(vec);
		ensure("4:LLColor4u:Fail to initialize ", ((vec[0] == llcolor4u3.mV[VX]) && (vec[1] == llcolor4u3.mV[VY]) && (vec[2] == llcolor4u3.mV[VZ])&& (vec[3] == llcolor4u3.mV[VW])));		

		LLSD sd = llcolor4u3.getValue();
		LLColor4U llcolor4u4(sd);
		ensure_equals("5:LLColor4u (LLSD) Failed ", llcolor4u4, llcolor4u3);
	}
	
	template<> template<>
	void v4coloru_object::test<2>()
	{
		LLColor4U llcolor4ua(1, 2, 3, 4);
		LLSD sd = llcolor4ua.getValue();
		LLColor4U llcolor4u;
		llcolor4u.setValue(sd);
		ensure_equals("setValue(LLSD)/getValue Failed ", llcolor4u, llcolor4ua);
	}

	template<> template<>
	void v4coloru_object::test<3>()
	{
		U8 r = 0x12, g = 0xFF, b = 0xAF, a = 0x23;
		LLColor4U llcolor4u(r,g,b,a);
		llcolor4u.setToBlack();
		ensure("setToBlack:Fail to set black ", ((0 == llcolor4u.mV[VX]) && (0 == llcolor4u.mV[VY]) && (0 == llcolor4u.mV[VZ])&& (255 == llcolor4u.mV[VW])));

		llcolor4u.setToWhite();
		ensure("setToWhite:Fail to white ", ((255 == llcolor4u.mV[VX]) && (255 == llcolor4u.mV[VY]) && (255 == llcolor4u.mV[VZ])&& (255 == llcolor4u.mV[VW])));
	}
	
	template<> template<>
	void v4coloru_object::test<4>()
	{
		U8 r = 0x12, g = 0xFF, b = 0xAF, a = 0x23;
		LLColor4U llcolor4ua(r,g,b,a);
		LLSD sd = llcolor4ua.getValue();
		LLColor4U llcolor4u = (LLColor4U)sd;
		ensure_equals("Operator=(LLSD) Failed ",  llcolor4u, llcolor4ua);
	}

	template<> template<>
	void v4coloru_object::test<5>()
	{
		U8 r = 0x12, g = 0xFF, b = 0xAF, a = 0x23;
		LLColor4U llcolor4u;
		llcolor4u.setVec(r,g,b,a);
		ensure("1:setVec:Fail to set the values ", ((r == llcolor4u.mV[VX]) && (g == llcolor4u.mV[VY]) && (b == llcolor4u.mV[VZ])&& (a == llcolor4u.mV[VW])));

		llcolor4u.setToBlack();
		llcolor4u.setVec(r,g,b);
		ensure("2:setVec:Fail to set the values ", ((r == llcolor4u.mV[VX]) && (g == llcolor4u.mV[VY]) && (b == llcolor4u.mV[VZ])&& (255 == llcolor4u.mV[VW])));

		LLColor4U llcolor4u1;
		llcolor4u1.setVec(llcolor4u);
		ensure_equals("3:setVec:Fail to set the values ", llcolor4u1,llcolor4u);
		
		const U8 vec[4] = {0x12,0xFF,0xAF,0x23};
		LLColor4U llcolor4u2;
		llcolor4u2.setVec(vec);
		ensure("4:setVec:Fail to set the values ", ((vec[0] == llcolor4u2.mV[VX]) && (vec[1] == llcolor4u2.mV[VY]) && (vec[2] == llcolor4u2.mV[VZ])&& (vec[3] == llcolor4u2.mV[VW])));		
	}
	
	template<> template<>
	void v4coloru_object::test<6>()
	{
		U8 alpha = 0x12;
		LLColor4U llcolor4u;
		llcolor4u.setAlpha(alpha);
		ensure("setAlpha:Fail to set alpha value ", (alpha == llcolor4u.mV[VW]));
	}
	
	template<> template<>
	void v4coloru_object::test<7>()
	{
		U8 r = 0x12, g = 0xFF, b = 0xAF;
		LLColor4U llcolor4u(r,g,b);
		ensure("magVecSquared:Fail ", is_approx_equal(llcolor4u.magVecSquared(), (F32)(r*r + g*g + b*b)));
		ensure("magVec:Fail ", is_approx_equal(llcolor4u.magVec(), fsqrtf(r*r + g*g + b*b)));
	}

	template<> template<>
	void v4coloru_object::test<8>()
	{
		U8 r = 0x12, g = 0xFF, b = 0xAF;
		std::ostringstream stream1, stream2;
		LLColor4U llcolor4u1(r,g,b),llcolor4u2;
		stream1 << llcolor4u1;
		llcolor4u2.setVec(r,g,b);
		stream2 << llcolor4u2;
		ensure("operator << failed ", (stream1.str() == stream2.str()));	
	}

	template<> template<>
	void v4coloru_object::test<9>()
	{
		U8 r1 = 0x12, g1 = 0xFF, b1 = 0xAF;
		U8 r2 = 0x1C, g2 = 0x9A, b2 = 0x1B;
		LLColor4U llcolor4u1(r1,g1,b1), llcolor4u2(r2,g2,b2),llcolor4u3;
		llcolor4u3 = llcolor4u1 + llcolor4u2;
		ensure_equals(
			"1a.operator+:Fail to Add the values ",
			llcolor4u3.mV[VX],
			(U8)(r1+r2));
		ensure_equals(
			"1b.operator+:Fail to Add the values ",
			llcolor4u3.mV[VY],
			(U8)(g1+g2));
		ensure_equals(
			"1c.operator+:Fail to Add the values ",
			llcolor4u3.mV[VZ],
			(U8)(b1+b2));

		llcolor4u2 += llcolor4u1;
		ensure_equals(
			"2a.operator+=:Fail to Add the values ",
			llcolor4u2.mV[VX],
			(U8)(r1+r2));
		ensure_equals(
			"2b.operator+=:Fail to Add the values ",
			llcolor4u2.mV[VY],
			(U8)(g1+g2));
		ensure_equals(
			"2c.operator+=:Fail to Add the values ",
			llcolor4u2.mV[VZ],
			(U8)(b1+b2));
	}

	template<> template<>
	void v4coloru_object::test<10>()
	{
		U8 r1 = 0x12, g1 = 0xFF, b1 = 0xAF;
		U8 r2 = 0x1C, g2 = 0x9A, b2 = 0x1B;
		LLColor4U llcolor4u1(r1,g1,b1), llcolor4u2(r2,g2,b2),llcolor4u3;
		llcolor4u3 = llcolor4u1 - llcolor4u2;
		ensure_equals(
			"1a. operator-:Fail to Add the values ",
			llcolor4u3.mV[VX],
			(U8)(r1-r2));
		ensure_equals(
			"1b. operator-:Fail to Add the values ",
			llcolor4u3.mV[VY],
			(U8)(g1-g2));
		ensure_equals(
			"1c. operator-:Fail to Add the values ",
			llcolor4u3.mV[VZ],
			(U8)(b1-b2));

		llcolor4u1 -= llcolor4u2;
		ensure_equals(
			"2a. operator-=:Fail to Add the values ",
			llcolor4u1.mV[VX],
			(U8)(r1-r2));
		ensure_equals(
			"2b. operator-=:Fail to Add the values ",
			llcolor4u1.mV[VY],
			(U8)(g1-g2));
		ensure_equals(
			"2c. operator-=:Fail to Add the values ",
			llcolor4u1.mV[VZ],
			(U8)(b1-b2));
	}

	template<> template<>
	void v4coloru_object::test<11>()
	{
		U8 r1 = 0x12, g1 = 0xFF, b1 = 0xAF;
		U8 r2 = 0x1C, g2 = 0x9A, b2 = 0x1B;
		LLColor4U llcolor4u1(r1,g1,b1), llcolor4u2(r2,g2,b2),llcolor4u3;
		llcolor4u3 = llcolor4u1 * llcolor4u2;
		ensure_equals(
			"1a. operator*:Fail to multiply the values",
			llcolor4u3.mV[VX],
			(U8)(r1*r2));
		ensure_equals(
			"1b. operator*:Fail to multiply the values",
			llcolor4u3.mV[VY],
			(U8)(g1*g2));
		ensure_equals(
			"1c. operator*:Fail to multiply the values",
			llcolor4u3.mV[VZ],
			(U8)(b1*b2));
		
		U8 mulVal = 123;
		llcolor4u1 *= mulVal;
		ensure_equals(
			"2a. operator*=:Fail to multiply the values",
			llcolor4u1.mV[VX],
			(U8)(r1*mulVal));
		ensure_equals(
			"2b. operator*=:Fail to multiply the values",
			llcolor4u1.mV[VY],
			(U8)(g1*mulVal));
		ensure_equals(
			"2c. operator*=:Fail to multiply the values",
			llcolor4u1.mV[VZ],
			(U8)(b1*mulVal));
	}

	template<> template<>
	void v4coloru_object::test<12>()
	{
		U8 r = 0x12, g = 0xFF, b = 0xAF;
		LLColor4U llcolor4u(r,g,b),llcolor4u1;
		llcolor4u1 = llcolor4u;
		ensure("operator== failed to ensure the equality ", (llcolor4u1 == llcolor4u));	
		llcolor4u1.setToBlack();
		ensure("operator!= failed to ensure the equality ", (llcolor4u1 != llcolor4u));	
	}

	template<> template<>
	void v4coloru_object::test<13>()
	{
		U8 r = 0x12, g = 0xFF, b = 0xAF, a = 12;
		LLColor4U llcolor4u(r,g,b,a);
		U8 modVal = 45;
		llcolor4u %= modVal;
		ensure_equals("operator%=:Fail ", llcolor4u.mV[VW], (U8)(a * modVal));
	}

	template<> template<>
	void v4coloru_object::test<14>()
	{
		U8 r = 0x12, g = 0xFF, b = 0xAF, a = 12;
		LLColor4U llcolor4u1(r,g,b,a);
		std::string color("12, 23, 132, 50");
		LLColor4U::parseColor4U(color, &llcolor4u1);
		ensure("parseColor4U() failed to parse the color value ", ((12 == llcolor4u1.mV[VX]) && (23 == llcolor4u1.mV[VY]) && (132 == llcolor4u1.mV[VZ])&& (50 == llcolor4u1.mV[VW])));

		color = "12, 23, 132";
		ensure("2:parseColor4U() failed to parse the color value ",  (FALSE == LLColor4U::parseColor4U(color, &llcolor4u1)));

		color = "12";
		ensure("2:parseColor4U() failed to parse the color value ",  (FALSE == LLColor4U::parseColor4U(color, &llcolor4u1)));
	}

	template<> template<>
	void v4coloru_object::test<15>()
	{
		U8 r = 12, g = 123, b = 3, a = 2;
		LLColor4U llcolor4u(r,g,b,a),llcolor4u1;
		const F32 fVal = 3.f;
		llcolor4u1 = llcolor4u.multAll(fVal);
		ensure("multAll:Fail to multiply ", (((U8)llround(r * fVal) == llcolor4u1.mV[VX]) && (U8)llround(g * fVal) == llcolor4u1.mV[VY]
											&& ((U8)llround(b * fVal) == llcolor4u1.mV[VZ])&& ((U8)llround(a * fVal) == llcolor4u1.mV[VW])));		
	}

	template<> template<>
	void v4coloru_object::test<16>()
	{
		U8 r1 = 12, g1 = 123, b1 = 3, a1 = 2;
		U8 r2 = 23, g2 = 230, b2 = 124, a2 = 255;
		LLColor4U llcolor4u(r1,g1,b1,a1),llcolor4u1(r2,g2,b2,a2);
		llcolor4u1 = llcolor4u1.addClampMax(llcolor4u);
		ensure("1:addClampMax():Fail to add the value ",  ((r1+r2 == llcolor4u1.mV[VX]) && (255 == llcolor4u1.mV[VY]) && (b1+b2 == llcolor4u1.mV[VZ])&& (255 == llcolor4u1.mV[VW])));

		r1 = 132, g1 = 3, b1 = 3, a1 = 2;
		r2 = 123, g2 = 230, b2 = 154, a2 = 25;
		LLColor4U llcolor4u2(r1,g1,b1,a1),llcolor4u3(r2,g2,b2,a2);
		llcolor4u3 = llcolor4u3.addClampMax(llcolor4u2);
		ensure("2:addClampMax():Fail to add the value ",  ((255 == llcolor4u3.mV[VX]) && (g1+g2 == llcolor4u3.mV[VY]) && (b1+b2 == llcolor4u3.mV[VZ])&& (a1+a2 == llcolor4u3.mV[VW])));
	}

	template<> template<>
	void v4coloru_object::test<17>()
	{
		F32 r = 23.f, g = 12.32f, b = -12.3f;
		LLColor3 color3(r,g,b);
		LLColor4U llcolor4u;
		llcolor4u.setVecScaleClamp(color3);
		const S32 MAX_COLOR = 255;
		F32 color_scale_factor = MAX_COLOR/r;
		S32 r2 = llround(r * color_scale_factor);
		S32 g2 = llround(g * color_scale_factor);
		ensure("setVecScaleClamp():Fail to add the value ",  ((r2 == llcolor4u.mV[VX]) && (g2 == llcolor4u.mV[VY]) && (0 == llcolor4u.mV[VZ])&& (255 == llcolor4u.mV[VW])));
	}
}
