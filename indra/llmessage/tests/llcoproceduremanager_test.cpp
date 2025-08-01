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

#include "../test/lldoctest.h"
#include "../test/sync.h"


#if LL_WINDOWS
// disable unreachable code warnings
#pragma warning(disable: 4702)
#endif

LLCoreHttpUtil::HttpCoroutineAdapter::HttpCoroutineAdapter(std::string const&, unsigned int)
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

TEST_SUITE("LLCoprocedureManager") {

struct coproceduremanager_test
{

        coproceduremanager_test()
        {
        
};

TEST_CASE_FIXTURE(coproceduremanager_test, "test_1")
{

        Sync sync;
        int foo = 0;
        LLCoprocedureManager::instance().initializePool("PoolName");
        LLCoprocedureManager::instance().enqueueCoprocedure("PoolName", "ProcName",
            [&foo, &sync] (LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t & ptr, const LLUUID & id) {
                sync.bump();
                foo = 1;
            
}

TEST_CASE_FIXTURE(coproceduremanager_test, "test_2")
{

        const size_t capacity = 2;
        boost::fibers::buffered_channel<std::function<void(void)>> chan(capacity);

        boost::fibers::fiber worker([&chan]() {
            chan.value_pop()();
        
}

TEST_CASE_FIXTURE(coproceduremanager_test, "test_3")
{

        boost::fibers::unbuffered_channel<std::function<void(void)>> chan;

        boost::fibers::fiber worker([&chan]() {
            chan.value_pop()();
        
}

TEST_CASE_FIXTURE(coproceduremanager_test, "test_4")
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

} // TEST_SUITE
