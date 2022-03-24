/**
 * @file   lockstatic.h
 * @author Nat Goodspeed
 * @date   2019-12-03
 * @brief  LockStatic class provides mutex-guarded access to the specified
 *         static data.
 * 
 * $LicenseInfo:firstyear=2019&license=viewerlgpl$
 * Copyright (c) 2019, Linden Research, Inc.
 * $/LicenseInfo$
 */

#if ! defined(LL_LOCKSTATIC_H)
#define LL_LOCKSTATIC_H

#include "mutex.h"                  // std::unique_lock

namespace llthread
{

// Instantiate this template to obtain a pointer to the canonical static
// instance of Static while holding a lock on that instance. Use of
// Static::mMutex presumes that Static declares some suitable mMutex.
template <typename Static>
class LockStatic
{
    typedef std::unique_lock<decltype(Static::mMutex)> lock_t;
public:
    LockStatic():
        mData(getStatic()),
        mLock(mData->mMutex)
    {}
    Static* get() const { return mData; }
    operator Static*() const { return get(); }
    Static* operator->() const { return get(); }
    // sometimes we must explicitly unlock...
    void unlock()
    {
        // but once we do, access is no longer permitted
        mData = nullptr;
        mLock.unlock();
    }
protected:
    Static* mData;
    lock_t mLock;
private:
    Static* getStatic()
    {
        // Static::mMutex must be function-local static rather than class-
        // static. Some of our consumers must function properly (therefore
        // lock properly) even when the containing module's static variables
        // have not yet been runtime-initialized. A mutex requires
        // construction. A static class member might not yet have been
        // constructed.
        //
        // We could store a dumb mutex_t*, notice when it's NULL and allocate a
        // heap mutex -- but that's vulnerable to race conditions. And we can't
        // defend the dumb pointer with another mutex.
        //
        // We could store a std::atomic<mutex_t*> -- but a default-constructed
        // std::atomic<T> does not contain a valid T, even a default-constructed
        // T! Which means std::atomic, too, requires runtime initialization.
        //
        // But a function-local static is guaranteed to be initialized exactly
        // once: the first time control reaches that declaration.
        static Static sData;
        return &sData;
    }
};

} // llthread namespace

#endif /* ! defined(LL_LOCKSTATIC_H) */
