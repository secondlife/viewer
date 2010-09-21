/**
 * @file   llmemtype_test.cpp
 * @author Palmer Truelson
 * @date   2008-03-
 * @brief  Test for llmemtype.cpp.
 * 
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 *
 * Copyright (c) 2009-2009, Linden Research, Inc.
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

#include "../llmemtype.h"
#include "../test/lltut.h"
#include "../llallocator.h"


#include <stack>

std::stack<S32> memTypeStack;

void LLAllocator::pushMemType(S32 i)
{
	memTypeStack.push(i);
}

S32 LLAllocator::popMemType(void)
{
	S32 ret = memTypeStack.top();
	memTypeStack.pop();
	return ret;
}

namespace tut
{
    struct llmemtype_data
    {
    };

    typedef test_group<llmemtype_data> factory;
    typedef factory::object object;
}
namespace
{
        tut::factory llmemtype_test_factory("LLMemType");
}

namespace tut
{
    template<> template<>
    void object::test<1>()
    {
        ensure("Simplest test ever", true);
    }

	// test with no scripts
	template<> template<>
	void object::test<2>()
	{
		{
			LLMemType m1(LLMemType::MTYPE_INIT);
		}
		ensure("Test that you can construct and destruct the mem type");
	}

	// test creation and stack testing
	template<> template<>
	void object::test<3>()
	{
		{
			ensure("Test that creation and destruction properly inc/dec the stack");			
			ensure_equals(memTypeStack.size(), 0);
			{
				LLMemType m1(LLMemType::MTYPE_INIT);
				ensure_equals(memTypeStack.size(), 1);
				LLMemType m2(LLMemType::MTYPE_STARTUP);
				ensure_equals(memTypeStack.size(), 2);
			}
			ensure_equals(memTypeStack.size(), 0);
		}
	}

	// test with no scripts
	template<> template<>
	void object::test<4>()
	{
		// catch the begining and end
		std::string test_name = LLMemType::getNameFromID(LLMemType::MTYPE_INIT.mID);
		ensure_equals("Init name", test_name, "Init");

		std::string test_name2 = LLMemType::getNameFromID(LLMemType::MTYPE_VOLUME.mID);
		ensure_equals("Volume name", test_name2, "Volume");

		std::string test_name3 = LLMemType::getNameFromID(LLMemType::MTYPE_OTHER.mID);
		ensure_equals("Other name", test_name3, "Other");

        std::string test_name4 = LLMemType::getNameFromID(-1);
        ensure_equals("Invalid name", test_name4, "INVALID");
	}

};
