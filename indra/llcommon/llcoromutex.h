/**
 * @file   llcoromutex.h
 * @author Nat Goodspeed
 * @date   2024-09-04
 * @brief  Coroutine-aware synchronization primitives
 *
 * $LicenseInfo:firstyear=2024&license=viewerlgpl$
 * Copyright (c) 2024, Linden Research, Inc.
 * $/LicenseInfo$
 */

#if ! defined(LL_LLCOROMUTEX_H)
#define LL_LLCOROMUTEX_H

#include "mutex.h"
#include <boost/fiber/future/promise.hpp>
#include <boost/fiber/future/future.hpp>

// e.g. #include LLCOROS_MUTEX_HEADER
#define LLCOROS_MUTEX_HEADER   <boost/fiber/mutex.hpp>
#define LLCOROS_RMUTEX_HEADER  <boost/fiber/recursive_mutex.hpp>
#define LLCOROS_CONDVAR_HEADER <boost/fiber/condition_variable.hpp>

namespace boost {
    namespace fibers {
        class mutex;
        class recursive_mutex;
        enum class cv_status;
        class condition_variable;
    }
}

namespace llcoro
{

/**
 * Aliases for promise and future. An older underlying future implementation
 * required us to wrap future; that's no longer needed. However -- if it's
 * important to restore kill() functionality, we might need to provide a
 * proxy, so continue using the aliases.
 */
template <typename T>
using Promise = boost::fibers::promise<T>;
template <typename T>
using Future = boost::fibers::future<T>;
template <typename T>
inline
static Future<T> getFuture(Promise<T>& promise) { return promise.get_future(); }

// use mutex, lock, condition_variable suitable for coroutines
using Mutex = boost::fibers::mutex;
using RMutex = boost::fibers::recursive_mutex;
// With C++17, LockType is deprecated: at this point we can directly
// declare 'std::unique_lock lk(some_mutex)' without explicitly stating
// the mutex type. Sadly, making LockType an alias template for
// std::unique_lock doesn't work the same way: Class Template Argument
// Deduction only works for class templates, not alias templates.
using LockType = std::unique_lock<Mutex>;
using cv_status = boost::fibers::cv_status;
using ConditionVariable = boost::fibers::condition_variable;

} // namespace llcoro

#endif /* ! defined(LL_LLCOROMUTEX_H) */
