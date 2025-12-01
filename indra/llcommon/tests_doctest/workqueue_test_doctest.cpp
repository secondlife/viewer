/**
 * @file workqueue_test_doctest.cpp
 * @date   2025-02-18
 * @brief doctest: unit tests for workqueue
 *
 * $LicenseInfo:firstyear=2025&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2025, Linden Research, Inc.
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

// ---------------------------------------------------------------------------
// Auto-generated from workqueue_test.cpp at 2025-10-16T18:47:17Z
// ---------------------------------------------------------------------------

#include "doctest.h"
#include "ll_doctest_helpers.h"
#include "tut_compat_doctest.h"

#include "linden_common.h"
#include "workqueue.h"

#include <chrono>
#include <deque>

#include "../test/catch_and_store_what_in.h"
#include "llcond.h"
#include "llcoros.h"
#include "lleventcoro.h"
#include "llstring.h"
#include "stringize.h"

using namespace LL;
using namespace std::literals::chrono_literals;
using namespace std::literals::string_literals;

TUT_SUITE("llcommon")
{
    TUT_CASE("workqueue_test::object_test_1")
    {
        WorkSchedule queue("queue");

        TUT_SET_TEST_NAME("name");

        TUT_ENSURE_EQ(queue.getKey(), "queue");
        TUT_ENSURE("not findable",
            WorkSchedule::getInstance("queue") == queue.getWeak().lock());

        WorkSchedule q2;
        TUT_ENSURE("has no name",
            LLStringUtil::startsWith(q2.getKey(), "WorkQueue"));
    }

    TUT_CASE("workqueue_test::object_test_2")
    {
        WorkSchedule queue("queue");

        TUT_SET_TEST_NAME("post");

        bool wasRun{ false };

        queue.post([&wasRun]() { wasRun = true; });
        queue.close();

        TUT_ENSURE("ran too soon", !wasRun);

        queue.runUntilClose();

        TUT_ENSURE("didn't run", wasRun);
    }

    TUT_CASE("workqueue_test::object_test_3")
    {
        WorkSchedule queue("queue");

        TUT_SET_TEST_NAME("postEvery");

        using Shared = std::deque<WorkSchedule::TimePoint>;
        LLCond<Shared> data;

        auto start = WorkSchedule::TimePoint::clock::now();
        auto interval = 2s;

        queue.postEvery(
            interval,
            [&data, count = 0]() mutable
            {
                data.update_one(
                    [](Shared& shared)
                    {
                        shared.push_back(WorkSchedule::TimePoint::clock::now());
                    });

                return (++count < 3);
            });

        for (auto finish = start + 10 * interval;
             WorkSchedule::TimePoint::clock::now() < finish &&
             data.get([](const Shared& shared) { return shared.size(); }) < 3; )
        {
            queue.runPending();
            std::this_thread::sleep_for(interval / 10);
        }

        Shared result = data.get();

        TUT_ENSURE_EQ(result.size(), 3U);

        start -= interval;

        for (size_t i = 0; i < result.size(); ++i)
        {
            auto diff = result[i] - start;
            start += interval;

            auto interval_ms = interval / 1ms;
            auto diff_ms = diff / 1ms;

            std::ostringstream msg_early;
            msg_early << "call " << i << " too soon (interval " << interval_ms
                      << "ms; diff " << diff_ms << "ms)";
            const std::string early = msg_early.str();

            TUT_ENSURE(early.c_str(), diff >= interval);

            std::ostringstream msg_late;
            msg_late << "call " << i << " too late (interval " << interval_ms
                     << "ms; diff " << diff_ms << "ms)";
            const std::string late = msg_late.str();

            TUT_ENSURE(late.c_str(), diff < interval * 1.5);
        }
    }

    TUT_CASE("workqueue_test::object_test_4")
    {
        WorkSchedule queue("queue");
        WorkSchedule main("main");

        TUT_SET_TEST_NAME("postTo");

        auto qptr = WorkSchedule::getInstance("queue");

        int result = 0;

        main.postTo(
            qptr,
            []() { return 17; },
            [&result](int value) { result = value; });

        qptr->runOne();
        main.runOne();

        TUT_ENSURE_EQ(result, 17);

        std::string alpha;

        main.postTo(
            qptr,
            []() { return "abc"s; },
            [&alpha](const std::string& s) { alpha = s; });

        qptr->runPending();
        main.runPending();

        TUT_ENSURE_EQ(alpha, "abc");
    }

    TUT_CASE("workqueue_test::object_test_5")
    {
        WorkSchedule queue("queue");
        WorkSchedule main("main");

        TUT_SET_TEST_NAME("postTo with void return");

        auto qptr = WorkSchedule::getInstance("queue");

        std::string observe;

        main.postTo(
            qptr,
            [&observe]() { observe = "queue"; },
            [&observe]() { observe.append(";main"); });

        qptr->runOne();
        main.runOne();

        TUT_ENSURE_EQ(observe, "queue;main");
    }

    TUT_CASE("workqueue_test::object_test_6")
    {
        WorkSchedule queue("queue");

        TUT_SET_TEST_NAME("waitForResult");

        std::string stored;

        auto what = catch_what<WorkSchedule::Error>(
            [&queue, &stored]()
            {
                stored = queue.waitForResult(
                    []() { return "should throw"; });
            });

        TUT_ENSURE("lambda should not have run", stored.empty());
        TUT_ENSURE_NOT("waitForResult() should have thrown", what.empty());

        std::string msg =
            STRINGIZE("should mention waitForResult: " << what);
        TUT_ENSURE(msg.c_str(),
            what.find("waitForResult") != std::string::npos);

        LLCoros::instance().launch(
            "waitForResult string",
            [&queue, &stored]()
            {
                stored = queue.waitForResult(
                    []() { return "string result"; });
            });

        llcoro::suspend();

        queue.runOne();
        llcoro::suspend();

        TUT_ENSURE_EQ(stored, "string result");

        stored.clear();
        bool done = false;

        LLCoros::instance().launch(
            "waitForResult void",
            [&queue, &stored, &done]()
            {
                queue.waitForResult(
                    [&stored]() { stored = "ran"; });
                done = true;
            });

        llcoro::suspend();

        queue.runOne();
        llcoro::suspend();

        TUT_ENSURE_EQ(stored, "ran");
        TUT_ENSURE("void waitForResult() didn't return", done);
    }
}
