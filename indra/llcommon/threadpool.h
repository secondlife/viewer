/**
 * @file   threadpool.h
 * @author Nat Goodspeed
 * @date   2021-10-21
 * @brief  ThreadPool configures a WorkQueue along with a pool of threads to
 *         service it.
 * 
 * $LicenseInfo:firstyear=2021&license=viewerlgpl$
 * Copyright (c) 2021, Linden Research, Inc.
 * $/LicenseInfo$
 */

#if ! defined(LL_THREADPOOL_H)
#define LL_THREADPOOL_H

#include "workqueue.h"
#include <string>
#include <thread>
#include <utility>                  // std::pair
#include <vector>

namespace LL
{

    class ThreadPool: public LLInstanceTracker<ThreadPool, std::string>
    {
    private:
        using super = LLInstanceTracker<ThreadPool, std::string>;
    public:
        /**
         * Pass ThreadPool a string name. This can be used to look up the
         * relevant WorkQueue.
         */
        ThreadPool(const std::string& name, size_t threads=1, size_t capacity=1024);
        virtual ~ThreadPool();

        /**
         * Launch the ThreadPool. Until this call, a constructed ThreadPool
         * launches no threads. That permits coders to derive from ThreadPool,
         * or store it as a member of some other class, but refrain from
         * launching it until all other construction is complete.
         */
        void start();

        /**
         * ThreadPool listens for application shutdown messages on the "LLApp"
         * LLEventPump. Call close() to shut down this ThreadPool early.
         */
        void close();

        std::string getName() const { return mName; }
        size_t getWidth() const { return mThreads.size(); }
        /// obtain a non-const reference to the WorkQueue to post work to it
        WorkQueue& getQueue() { return mQueue; }

        /**
         * Override run() if you need special processing. The default run()
         * implementation simply calls WorkQueue::runUntilClose().
         */
        virtual void run();

    private:
        void run(const std::string& name);

        WorkQueue mQueue;
        std::string mName;
        size_t mThreadCount;
        std::vector<std::pair<std::string, std::thread>> mThreads;
    };

} // namespace LL

#endif /* ! defined(LL_THREADPOOL_H) */
