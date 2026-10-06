/**
 * @file llmessagetemplate_test.cpp
 * @brief LLMessageTemplate counter and timing statistics tests.
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2026, Linden Research, Inc.
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
#include "lltut.h"
#include "llmessagetemplate.h"

#include <thread>
#include <vector>

namespace tut
{
    struct messagetemplate
    {
        LLMessageTemplate mTemplate{ "TestMessage", (U32)1, MFT_HIGH };
    };
    typedef test_group<messagetemplate> messagetemplate_t;
    typedef messagetemplate_t::object messagetemplate_object_t;
    tut::messagetemplate_t tut_messagetemplate("LLMessageTemplate");

    // Receive counters start at zero and accumulate.
    template<> template<>
    void messagetemplate_object_t::test<1>()
    {
        ensure_equals("initial receive count", mTemplate.getReceiveCount(), (U32)0);
        ensure_equals("initial receive bytes", mTemplate.getReceiveBytes(), (U32)0);
        ensure_equals("initial invalid count", mTemplate.getReceiveInvalid(), (U32)0);

        mTemplate.recordReceive();
        ensure_equals("receive count", mTemplate.getReceiveCount(), (U32)1);

        mTemplate.recordReceiveCount(100, false);
        mTemplate.recordReceiveCount(50, true);
        ensure_equals("receive count", mTemplate.getReceiveCount(), (U32)3);
        ensure_equals("receive bytes", mTemplate.getReceiveBytes(), (U32)150);
        ensure_equals("invalid count", mTemplate.getReceiveInvalid(), (U32)1);

        mTemplate.resetReceiveCounts();
        ensure_equals("reset receive count", mTemplate.getReceiveCount(), (U32)0);
        ensure_equals("reset receive bytes", mTemplate.getReceiveBytes(), (U32)0);
        ensure_equals("reset invalid count", mTemplate.getReceiveInvalid(), (U32)0);
    }

    // Decode timing statistics accumulate and track the maximum.
    template<> template<>
    void messagetemplate_object_t::test<2>()
    {
        ensure_equals("initial decoded", mTemplate.getTotalDecoded(), (U32)0);
        ensure_equals("initial decode time", mTemplate.getTotalDecodeTime(), 0.f);
        ensure_equals("initial max decode time", mTemplate.getMaxDecodeTimePerMsg(), 0.f);

        mTemplate.recordDecodeTime(1.f);
        mTemplate.recordDecodeTime(3.f);
        mTemplate.recordDecodeTime(2.f);

        ensure_equals("decoded", mTemplate.getTotalDecoded(), (U32)3);
        ensure_equals("total decode time", mTemplate.getTotalDecodeTime(), 6.f);
        ensure_equals("max decode time", mTemplate.getMaxDecodeTimePerMsg(), 3.f);

        mTemplate.resetDecodeStats();
        ensure_equals("reset decoded", mTemplate.getTotalDecoded(), (U32)0);
        ensure_equals("reset decode time", mTemplate.getTotalDecodeTime(), 0.f);
        ensure_equals("reset max decode time", mTemplate.getMaxDecodeTimePerMsg(), 0.f);
    }

    // Counters are updated by the UDP receiver thread while the main thread
    // reports on them: concurrent updates must not lose any counts.
    template<> template<>
    void messagetemplate_object_t::test<3>()
    {
        const int thread_count = 4;
        const int per_thread = 1000;

        std::vector<std::thread> threads;
        for (int i = 0; i < thread_count; ++i)
        {
            threads.emplace_back([this]()
            {
                for (int j = 0; j < per_thread; ++j)
                {
                    mTemplate.recordReceiveCount(10, (j % 2) == 0);
                    mTemplate.recordDecodeTime(1.f);
                }
            });
        }
        for (auto& thread : threads)
        {
            thread.join();
        }

        ensure_equals("receive count", mTemplate.getReceiveCount(), (U32)(thread_count * per_thread));
        ensure_equals("receive bytes", mTemplate.getReceiveBytes(), (U32)(thread_count * per_thread * 10));
        ensure_equals("invalid count", mTemplate.getReceiveInvalid(), (U32)(thread_count * per_thread / 2));
        ensure_equals("decoded", mTemplate.getTotalDecoded(), (U32)(thread_count * per_thread));
        ensure_equals("total decode time", mTemplate.getTotalDecodeTime(), (F32)(thread_count * per_thread));
        ensure_equals("max decode time", mTemplate.getMaxDecodeTimePerMsg(), 1.f);
    }

    // Copying a template carries the counters over, so that the explicit copy
    // operations stay in sync with the atomic members.
    template<> template<>
    void messagetemplate_object_t::test<4>()
    {
        mTemplate.recordReceiveCount(42, true);
        mTemplate.recordDecodeTime(0.5f);

        LLMessageTemplate copied(mTemplate);
        ensure_equals("copied receive count", copied.getReceiveCount(), (U32)1);
        ensure_equals("copied receive bytes", copied.getReceiveBytes(), (U32)42);
        ensure_equals("copied invalid count", copied.getReceiveInvalid(), (U32)1);
        ensure_equals("copied decoded", copied.getTotalDecoded(), (U32)1);
        ensure_equals("copied decode time", copied.getTotalDecodeTime(), 0.5f);
        ensure_equals("copied max decode time", copied.getMaxDecodeTimePerMsg(), 0.5f);

        LLMessageTemplate assigned("OtherTestMessage", (U32)2, MFT_HIGH);
        assigned = mTemplate;
        ensure_equals("assigned receive count", assigned.getReceiveCount(), (U32)1);
        ensure_equals("assigned decoded", assigned.getTotalDecoded(), (U32)1);
        ensure_equals("assigned name", std::string(assigned.mName), std::string("TestMessage"));
    }
}
