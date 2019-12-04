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
        static Static sData;
        return &sData;
    }
};

} // llthread namespace

#endif /* ! defined(LL_LOCKSTATIC_H) */
