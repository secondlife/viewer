/**
 * @file llgtest.h
 * @brief Common helpers for GoogleTest based unit tests.
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

#ifndef LL_LLGTEST_H
#define LL_LLGTEST_H

// This header intentionally depends only on GoogleTest and the standard
// library (no llcommon), so it can be used to test libraries that must not
// pull in viewer code, such as llwebrtc.

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <optional>
#include <utility>

namespace llgtest
{

// Default time to wait for an asynchronous event before failing a test.
// Generous on purpose: build machines can be slow and heavily loaded.
inline constexpr std::chrono::milliseconds DEFAULT_TIMEOUT{ 10000 };

// Thread-safe one-shot value for handing a result from a callback running on
// another thread back to the test thread.
//
//     llgtest::AsyncValue<std::string> offer;
//     // ...on some other thread:  offer.set(sdp);
//     std::optional<std::string> sdp = offer.wait();
//     ASSERT_TRUE(sdp.has_value());
//
// Only the first set() is kept; later calls are ignored so a callback that
// fires more than once can't overwrite the value the test is inspecting.
template <typename T>
class AsyncValue
{
public:
    void set(T value)
    {
        {
            std::lock_guard<std::mutex> lock(mMutex);
            if (mValue)
            {
                return;
            }
            mValue.emplace(std::move(value));
        }
        mCondition.notify_all();
    }

    // Waits up to 'timeout' for set() to be called.  Returns the value, or
    // std::nullopt on timeout.
    std::optional<T> wait(std::chrono::milliseconds timeout = DEFAULT_TIMEOUT)
    {
        std::unique_lock<std::mutex> lock(mMutex);
        mCondition.wait_for(lock, timeout, [this] { return mValue.has_value(); });
        return mValue;
    }

    bool isSet() const
    {
        std::lock_guard<std::mutex> lock(mMutex);
        return mValue.has_value();
    }

private:
    mutable std::mutex      mMutex;
    std::condition_variable mCondition;
    std::optional<T>        mValue;
};

// Thread-safe counter that the test thread can block on until it reaches a
// target, e.g. to wait for N callbacks.
class AsyncCounter
{
public:
    void increment()
    {
        {
            std::lock_guard<std::mutex> lock(mMutex);
            ++mCount;
        }
        mCondition.notify_all();
    }

    int count() const
    {
        std::lock_guard<std::mutex> lock(mMutex);
        return mCount;
    }

    // Waits up to 'timeout' for the count to reach at least 'target'.
    // Returns true if it did.
    bool waitFor(int target, std::chrono::milliseconds timeout = DEFAULT_TIMEOUT)
    {
        std::unique_lock<std::mutex> lock(mMutex);
        return mCondition.wait_for(lock, timeout, [this, target] { return mCount >= target; });
    }

private:
    mutable std::mutex      mMutex;
    std::condition_variable mCondition;
    int                     mCount{ 0 };
};

} // namespace llgtest

#endif // LL_LLGTEST_H
