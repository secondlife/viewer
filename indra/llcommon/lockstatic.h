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
#include "llexception.h"
#include <typeinfo>

namespace llthread
{

class LockStaticBase
{
public:
    // trying to lock Static after cleanup() has been called
    struct Dead: public LLException
    {
        Dead(const std::string& what): LLException(what) {}
    };

protected:
    static void throwDead(const char* mangled);
};

// Instantiate this template to obtain a pointer to the canonical static
// instance of Static while holding a lock on that instance. Use of
// Static::mMutex presumes that Static declares some suitable mMutex.
template <typename Static>
class LockStatic: public LockStaticBase
{
    typedef std::unique_lock<decltype(Static::mMutex)> lock_t;
public:
    LockStatic():
        mData(getStatic()),
        mLock(getLock(mData))
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
    // explicit destruction
    // We used to store a static instance of Static in getStatic(). The
    // trouble with that is that at some point during final termination
    // cleanup, the compiler calls ~Static(), destroying the mutex. If some
    // later static object's destructor tries to lock our Static, we blow up
    // trying to lock a destroyed mutex object. This can happen, for instance,
    // if some class's destructor tries to reference an LLSingleton.
    // Since a plain dumb pointer has no destructor, the compiler leaves it
    // alone, so the referenced heap Static instance can survive until we
    // explicitly call this method.
    void cleanup()
    {
        // certainly don't claim to lock after this point!
        mData = nullptr;
        Static*& ptrref{ getStatic() };
        Static* ptr{ ptrref };
        ptrref = nullptr;
        delete ptr;
    }
protected:
    Static* mData;
    lock_t mLock;
private:
    static lock_t getLock(Static* data)
    {
        // data can be false if cleanup() has already been called. If so, no
        // code in the caller is valid that depends on this instance. We dare
        // to throw an exception because trying to lock Static after it's been
        // deleted is not part of normal processing. There are callers who
        // want to handle this exception, but it should indeed be treated as
        // exceptional.
        if (! data)
        {
            throwDead(typeid(LockStatic<Static>).name());
        }
        // Usual case: data isn't nullptr, carry on.
        return lock_t(data->mMutex);
    }

    Static*& getStatic()
    {
        // Our Static instance must be function-local static rather than
        // class-static. Some of our consumers must function properly
        // (therefore lock properly) even when the containing module's static
        // variables have not yet been runtime-initialized. A mutex requires
        // construction. A static class member might not yet have been
        // constructed.
        //
        // We could store a dumb mutex_t* class member, notice when it's NULL
        // and allocate a heap mutex -- but that's vulnerable to race
        // conditions. And we can't defend the dumb pointer with another
        // mutex.
        //
        // We could store a std::atomic<mutex_t*> -- but a default-constructed
        // std::atomic<T> does not contain a valid T, even a default-constructed
        // T! Which means std::atomic, too, requires runtime initialization.
        //
        // But a function-local static is guaranteed to be initialized exactly
        // once: the first time control reaches that declaration. Importantly,
        // since a plain dumb pointer has no destructor, the compiler lets our
        // heap Static instance survive until someone calls cleanup() (above).
        static Static* sData{ new Static };
        return sData;
    }
};

} // llthread namespace

#endif /* ! defined(LL_LOCKSTATIC_H) */
