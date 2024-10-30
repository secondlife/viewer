/**
 * @file glworkqueue.h
 * @brief High performance work queue for usage in real-time rendering work
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
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

#pragma once

#include <mutex>
#include <condition_variable>
#include <queue>
#include "llsingleton.h"

namespace LL
{
    //============================================================================

    // High performance WorkQueue for usage in real-time rendering work
    class GLWorkQueue
    {
    public:
        using Work = std::function<void()>;

        GLWorkQueue() = default;

        void post(const Work& value);

        size_t size();

        bool done();

        Work pop();

        void runOne();

        void runUntilClose();

        void close();

        bool isClosed();

    private:
        std::mutex mMutex;
        std::condition_variable mCondition;
        std::queue<Work> mQueue;
        bool mClosed = false;
    };


    class GLThreadPool : public LLSimpleton<GLThreadPool>
    {
    public:
        GLThreadPool(U32 thread_count = 1);

        ~GLThreadPool();

        void run();

        void post(const GLWorkQueue::Work& value);
    
    private:
        std::vector<std::thread> mThreads;
        GLWorkQueue mQueue;
    };


    // helper for waiting on a job to complete
    class GLThreadSync
    {
    public:

        // reset the sync to the "unfinished" state
        void reset();

        // wait for sync to be set
        void wait();

        // call at start of job
        void start();

        // call when job has finished
        void finish();

        // helper class to automatically call start and finish
        class Guard
        {
        public:
            Guard(GLThreadSync& sync) : mSync(sync) { mSync.start(); }
            ~Guard() { mSync.finish(); }

        private:
            GLThreadSync& mSync;
        };

    private:
        std::condition_variable mCondition;
        bool mDone = false;
        std::mutex mMutex;

    };
};
