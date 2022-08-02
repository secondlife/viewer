/**
 * @file   datalocker.h
 * @author Nat Goodspeed
 * @date   2022-06-27
 * @brief  DataLocker bundles a mutex with specified data, providing access
 *         only when locked.
 * 
 * $LicenseInfo:firstyear=2022&license=viewerlgpl$
 * Copyright (c) 2022, Linden Research, Inc.
 * $/LicenseInfo$
 */

#if ! defined(LL_DATALOCKER_H)
#define LL_DATALOCKER_H

#include "llcoros.h"
#include LLCOROS_MUTEX_HEADER

namespace LL
{

    /**
     * DataLocker<DATA> constructs a private instance of DATA in-place. To
     * access it, you must construct an instance of DataLocker::Lock, which of
     * course locks the private DATA instance with a bundled mutex.
     *
     * Lock provides both operator*() returning a reference to the whole DATA
     * object and operator->() for access to DATA's public members.
     *
     * A class may either contain DataLocker<DATA> or derive from it. But
     * since DataLocker directly contains the mutex in question, if you intend
     * to manage static data, consider LockStatic instead. Cross-module access
     * to a static DataLocker instance could reach it before the mutex has
     * been initialized. LockStatic is specifically defended against that.
     *
     * DataLocker manages an arbitrary data object, e.g. a struct, a
     * std::map or a scalar. But if the managed object is a scalar, you might
     * consider std::atomic<DATA> instead.
     */
    template <typename DATA, typename MUTEX=LLCoros::Mutex>
    class DataLocker
    {
    public:
        using value_type = DATA;
        using mutex_t = MUTEX;

        // Construct DataLocker with whatever constructor arguments are
        // required by the contained DATA object.
        template <typename... ARGS>
        DataLocker(ARGS&&... args):
            mData(std::forward<ARGS>(args)...)
        {}

        class Lock
        {
        public:
            Lock(DataLocker& locker):
                mLock(locker.mMutex),
                mData(&locker.mData)
            {}

            operator*() const
            {
                // only while it's still locked!
                llassert(mData);
                return *mData;
            }

            value_type* operator->() const
            {
                // only while it's still locked!
                llassert(mData);
                return mData;
            }

            // sometimes we must explicitly unlock...
            void unlock()
            {
                // but once we do, access is no longer permitted
                mData = nullptr;
                mLock.unlock();
            }

        private:
            std::unique_lock<mutex_t> mLock;
            value_type* mData;
        };

    private:
        MUTEX mMutex;
        DATA mData;
    };

} // namespace LL

#endif /* ! defined(LL_DATALOCKER_H) */
