/**
 * @file _thread.h
 * @brief thread type abstraction
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2012, Linden Research, Inc.
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

#ifndef LLCOREINT_THREAD_H_
#define LLCOREINT_THREAD_H_

#include "linden_common.h"

#include <functional>
#include <thread>

#include "_refcounted.h"

namespace LLCoreInt
{

class HttpThread
{
public:
    HttpThread() = delete;                              // Not defined
    HttpThread(const HttpThread&) = delete;             // Not defined
    void operator=(const HttpThread &) = delete;        // Not defined

private:
    void run()
    { // THREAD CONTEXT
        // run the thread function
        mThreadFunc(this);
    } // THREAD CONTEXT

public:
    /// Constructs a thread object for concurrent execution but does
    /// not start running.  Caller receives on refcount on the thread
    /// instance.  If the thread is started, another will be taken
    /// out for the exit handler.
    explicit HttpThread(std::function<void (HttpThread *)> threadFunc)
          : mThreadFunc(threadFunc)
    {
        // this creates a std thread that will call HttpThread::run on this instance
        // and pass it the threadfunc callable...
        std::function<void()> f = std::bind(&HttpThread::run, this);

        mThread = std::make_unique<std::thread>(f);
        mNativeHandle = mThread->native_handle();
    }

    ~HttpThread() = default;

    inline void join()
    {
        mThread->join();
    }

    inline bool joinable() const
    {
        return mThread->joinable();
    }

    inline void detach()
    {
        mThread->detach();
    }

    // A very hostile method to force a thread to quit
    inline void cancel()
    {
#if LL_WINDOWS
        TerminateThread(mNativeHandle, 0);
#else
        pthread_cancel(mNativeHandle);
#endif
    }

private:
    std::function<void(HttpThread *)> mThreadFunc;
    std::unique_ptr<std::thread> mThread;
    std::thread::native_handle_type mNativeHandle;
}; // end class HttpThread

} // end namespace LLCoreInt

#endif // LLCOREINT_THREAD_H_


