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

#include "threadpool_fwd.h"
#include "workqueue.h"
#include <memory>                   // std::unique_ptr
#include <string>
#include <thread>
#include <utility>                  // std::pair
#include <vector>

namespace LL
{

    class ThreadPoolBase: public LLInstanceTracker<ThreadPoolBase, std::string>
    {
    private:
        using super = LLInstanceTracker<ThreadPoolBase, std::string>;

    public:
        /**
         * Pass ThreadPoolBase a string name. This can be used to look up the
         * relevant WorkQueue.
         *
         * The number of threads you pass sets the compile-time default. But
         * if the user has overridden the LLSD map in the "ThreadPoolSizes"
         * setting with a key matching this ThreadPool name, that setting
         * overrides this parameter.
         */
        ThreadPoolBase(const std::string& name, size_t threads,
                       WorkQueueBase* queue, bool auto_shutdown = true);
        virtual ~ThreadPoolBase();

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

        /**
         * Override run() if you need special processing. The default run()
         * implementation simply calls WorkQueue::runUntilClose().
         */
        virtual void run();

        /**
         * getConfiguredWidth() returns the setting, if any, for the specified
         * ThreadPool name. Returns dft if the "ThreadPoolSizes" map does not
         * contain the specified name.
         */
        static
        size_t getConfiguredWidth(const std::string& name, size_t dft=0);

        /**
         * This getWidth() returns the width of the instantiated ThreadPool
         * with the specified name, if any. If no instance exists, returns its
         * getConfiguredWidth() if any. If there's no instance and no relevant
         * override, return dft. Presumably dft should match the threads
         * parameter passed to the ThreadPool constructor call that will
         * eventually instantiate the ThreadPool with that name.
         */
        static
        size_t getWidth(const std::string& name, size_t dft);

    protected:
        std::unique_ptr<WorkQueueBase> mQueue;
        std::vector<std::pair<std::string, std::thread>> mThreads;
        bool mAutomaticShutdown;

    private:
        void run(const std::string& name);

        std::string mName;
        size_t mThreadCount;
    };

    /**
     * Specialize with WorkQueue or, for timestamped tasks, WorkSchedule
     */
    template <class QUEUE>
    struct ThreadPoolUsing: public ThreadPoolBase
    {
        using queue_t = QUEUE;

        /**
         * Pass ThreadPoolUsing a string name. This can be used to look up the
         * relevant WorkQueue.
         *
         * The number of threads you pass sets the compile-time default. But
         * if the user has overridden the LLSD map in the "ThreadPoolSizes"
         * setting with a key matching this ThreadPool name, that setting
         * overrides this parameter.
         *
         * Pass an explicit capacity to limit the size of the queue.
         * Constraining the queue can cause a submitter to block. Do not
         * constrain any ThreadPool accepting work from the main thread.
         */
        ThreadPoolUsing(const std::string& name,
                        size_t threads=1,
                        size_t capacity=1024*1024,
                        bool auto_shutdown = true):
            ThreadPoolBase(name, threads, new queue_t(name, capacity, false), auto_shutdown)
        {}
        ~ThreadPoolUsing() override {}

        /**
         * obtain a non-const reference to the specific WorkQueue subclass to
         * post work to it
         */
        queue_t& getQueue() { return static_cast<queue_t&>(*mQueue); }
    };

    /// ThreadPool is shorthand for using the simpler WorkQueue
    using ThreadPool = ThreadPoolUsing<WorkQueue>;

} // namespace LL

#endif /* ! defined(LL_THREADPOOL_H) */
