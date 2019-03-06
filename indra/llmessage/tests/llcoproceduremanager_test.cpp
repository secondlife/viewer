/** 
 * @file llcoproceduremanager_test.cpp
 * @author Brad
 * @date 2019-02
 * @brief LLCoprocedureManager unit test
 *
 * $LicenseInfo:firstyear=2019&license=viewerlgpl$
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
#include "llsdserialize.h"

#include "../llcoproceduremanager.h"

#include "../test/lltut.h"


#if LL_WINDOWS
// disable unreachable code warnings
#pragma warning(disable: 4702)
#endif

LLCoreHttpUtil::HttpCoroutineAdapter::HttpCoroutineAdapter(std::string const&, unsigned int, unsigned int)
{
}

void LLCoreHttpUtil::HttpCoroutineAdapter::cancelSuspendedOperation()
{
}

LLCoreHttpUtil::HttpCoroutineAdapter::~HttpCoroutineAdapter()
{
}

LLCore::HttpRequest::HttpRequest()
{
}

LLCore::HttpRequest::~HttpRequest()
{
}

namespace tut
{
    struct coproceduremanager_test
    {
        coproceduremanager_test()
        {
        }
    };
    typedef test_group<coproceduremanager_test> coproceduremanager_t;
    typedef coproceduremanager_t::object coproceduremanager_object_t;
    tut::coproceduremanager_t tut_coproceduremanager("LLCoprocedureManager");


    template<> template<>
    void coproceduremanager_object_t::test<1>()
    {
        int foo = 0;
        LLUUID queueId = LLCoprocedureManager::instance().enqueueCoprocedure("PoolName", "ProcName",
            [&foo] (LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t & ptr, const LLUUID & id) {
                foo = 1;
            });
        ensure_equals("coprocedure failed to update foo", foo, 1);
    }
}  // namespace tut
