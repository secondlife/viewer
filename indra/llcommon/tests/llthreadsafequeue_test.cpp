/**
 * @file llthreadsafequeue_test.cpp
 * @brief Tests for LLThreadSafeQueue.
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * Copyright (c) 2026, Linden Research, Inc.
 * $/LicenseInfo$
 */

#include "linden_common.h"

#include "llthreadsafequeue.h"

#include "../test/lltut.h"

#include <memory>

namespace tut
{
    struct llthreadsafequeue_data
    {
    };

    typedef test_group<llthreadsafequeue_data> llthreadsafequeue_group;
    typedef llthreadsafequeue_group::object object;
    llthreadsafequeue_group llthreadsafequeuegrp("llthreadsafequeue");

    template<> template<>
    void object::test<1>()
    {
        set_test_name("failed tryPush preserves move-only value");

        LLThreadSafeQueue<std::unique_ptr<int>> queue(1);
        auto first = std::make_unique<int>(1);
        ensure("failed to fill queue", queue.tryPush(std::move(first)));
        ensure("successful push retained ownership", !first);

        auto pending = std::make_unique<int>(2);
        ensure("push unexpectedly succeeded", !queue.tryPush(std::move(pending)));
        ensure("failed push consumed value", pending && *pending == 2);

        std::unique_ptr<int> popped;
        ensure("failed to pop first value", queue.tryPop(popped));
        ensure("wrong first value", popped && *popped == 1);

        ensure("retry failed", queue.tryPush(std::move(pending)));
        ensure("successful retry retained ownership", !pending);
        ensure("failed to pop retried value", queue.tryPop(popped));
        ensure("wrong retried value", popped && *popped == 2);
    }
}
