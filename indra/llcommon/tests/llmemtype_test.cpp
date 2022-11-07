/**
 * @file   llmemtype_test.cpp
 * @author Palmer Truelson
 * @date   2008-03-
 * @brief  Test for llmemtype.cpp.
 * 
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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
