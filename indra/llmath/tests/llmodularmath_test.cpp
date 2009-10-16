/**
 * @file modularmath_test.cpp
 * @author babbage
 * @date 2008-09
 * @brief llmodularmath tests
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

#include "../llmodularmath.h"

#include "../test/lltut.h"

namespace tut
{
	struct modularmath_data
	{
	};
	typedef test_group<modularmath_data> modularmath_test;
	typedef modularmath_test::object modularmath_object;
	tut::modularmath_test modularmath_testcase("modularmath");

	template<> template<>
	void modularmath_object::test<1>()
	{
		// lhs < rhs
		const U32 lhs = 0x000001;
		const U32 rhs = 0xFFFFFF;
		const U32 width = 24;
		U32 result = LLModularMath::subtract<width>(lhs, rhs); 
		ensure_equals("diff(0x000001, 0xFFFFFF, 24)", result, 2);
	}

	template<> template<>
	void modularmath_object::test<2>()
	{
		// lhs > rhs
		const U32 lhs = 0x000002;
		const U32 rhs = 0x000001;
		const U32 width = 24;
		U32 result = LLModularMath::subtract<width>(lhs, rhs); 
		ensure_equals("diff(0x000002, 0x000001, 24)", result, 1);
	}

	template<> template<>
	void modularmath_object::test<3>()
	{
		// lhs == rhs
		const U32 lhs = 0xABCDEF;
		const U32 rhs = 0xABCDEF;
		const U32 width = 24;
		U32 result = LLModularMath::subtract<width>(lhs, rhs); 
		ensure_equals("diff(0xABCDEF, 0xABCDEF, 24)", result, 0);
	}	
}
