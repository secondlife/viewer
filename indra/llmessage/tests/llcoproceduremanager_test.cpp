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

#include "llwin32headers.h"

#include "linden_common.h"
#include "llsdserialize.h"

#include "../llcoproceduremanager.h"

#include <functional>

#include <boost/fiber/fiber.hpp>
#include <boost/fiber/buffered_channel.hpp>
#include <boost/fiber/unbuffered_channel.hpp>

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
		// TODO: fix me. timing issues.the coproc gets executed after a frame, access violation in release

		/*
        int foo = 0;
        LLUUID queueId = LLCoprocedureManager::instance().enqueueCoprocedure("PoolName", "ProcName",
            [&foo] (LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t & ptr, const LLUUID & id) {
                foo = 1;
            });

		ensure_equals("coprocedure failed to update foo", foo, 1);
        
        LLCoprocedureManager::instance().close("PoolName");
		*/
    }

    template<> template<>
    void coproceduremanager_object_t::test<2>()
    {
        const size_t capacity = 2;
        boost::fibers::buffered_channel<std::function<void(void)>> chan(capacity);

        boost::fibers::fiber worker([&chan]() {
            chan.value_pop()();
        });

        chan.push([]() {
            LL_INFOS("Test") << "test 1" << LL_ENDL;
        });

        worker.join();
    }

    template<> template<>
    void coproceduremanager_object_t::test<3>()
    {
        boost::fibers::unbuffered_channel<std::function<void(void)>> chan;

        boost::fibers::fiber worker([&chan]() {
            chan.value_pop()();
        });

        chan.push([]() {
            LL_INFOS("Test") << "test 1" << LL_ENDL;
        });

        worker.join();
    }

    template<> template<>
    void coproceduremanager_object_t::test<4>()
    {
        boost::fibers::buffered_channel<std::function<void(void)>> chan(4);

        boost::fibers::fiber worker([&chan]() {
            std::function<void(void)> f;

            // using namespace std::chrono_literals;
            // const auto timeout = 5s;
            // boost::fibers::channel_op_status status;
            while (chan.pop(f) != boost::fibers::channel_op_status::closed)
            {
                LL_INFOS("CoWorker") << "got coproc" << LL_ENDL;
                f();
            }
            LL_INFOS("CoWorker") << "got closed" << LL_ENDL;
        });

        int counter = 0;

        for (int i = 0; i < 5; ++i)
        {
            LL_INFOS("CoMain") << "pushing coproc " << i << LL_ENDL;
            chan.push([&counter]() {
                LL_INFOS("CoProc") << "in coproc" << LL_ENDL;
                ++counter;
            });
        }

        LL_INFOS("CoMain") << "closing channel" << LL_ENDL;
        chan.close();

        LL_INFOS("CoMain") << "joining worker" << LL_ENDL;
        worker.join();

        LL_INFOS("CoMain") << "checking count" << LL_ENDL;
        ensure_equals("coprocedure failed to update counter", counter, 5);
    }
}  // namespace tut
