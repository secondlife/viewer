/**
 * @file   throttle.h
 * @author Nat Goodspeed
 * @date   2024-08-12
 * @brief  Throttle class
 *
 * $LicenseInfo:firstyear=2024&license=viewerlgpl$
 * Copyright (c) 2024, Linden Research, Inc.
 * $/LicenseInfo$
 */

#if ! defined(LL_THROTTLE_H)
#define LL_THROTTLE_H

#include "apply.h"                  // LL::bind_front()
#include "llerror.h"
#include <functional>
#include <iomanip>                  // std::quoted()

class ThrottleBase
{
public:
    ThrottleBase(F64 interval):
        mInterval(interval)
    {}

protected:
    bool too_fast();                // not const: we update mNext
    F64 mInterval, mNext{ 0. };
};

/**
 * An instance of Throttle mediates calls to some other specified function,
 * ensuring that it's called no more often than the specified time interval.
 * Throttle is an abstract base class that delegates the behavior when the
 * specified interval is exceeded.
 */
template <typename SIGNATURE>
class Throttle: public ThrottleBase
{
public:
    Throttle(const std::string& desc,
             const std::function<SIGNATURE>& func,
             F64 interval):
        ThrottleBase(interval),
        mDesc(desc),
        mFunc(func)
    {}
    // Constructing Throttle with a member function pointer but without an
    // instance pointer requires you to pass the instance pointer/reference as
    // the first argument to operator()().
    template <typename R, class C>
    Throttle(const std::string& desc, R C::* method, F64 interval):
        Throttle(desc, std::mem_fn(method), interval)
    {}
    template <typename R, class C>
    Throttle(const std::string& desc, R C::* method, C* instance, F64 interval):
        Throttle(desc, LL::bind_front(method, instance), interval)
    {}
    template <typename R, class C>
    Throttle(const std::string& desc, R C::* method, const C* instance, F64 interval):
        Throttle(desc, LL::bind_front(method, instance), interval)
    {}
    virtual ~Throttle() {}

    template <typename... ARGS>
    auto operator()(ARGS... args)
    {
        if (too_fast())
        {
            suppress();
            using rtype = decltype(mFunc(std::forward<ARGS>(args)...));
            if constexpr (! std::is_same_v<rtype, void>)
            {
                return rtype{};
            }
        }
        else
        {
            return mFunc(std::forward<ARGS>(args)...);
        }
    }

protected:
    // override with desired behavior when calls come too often
    virtual void suppress() = 0;
    const std::string mDesc;

private:
    std::function<SIGNATURE> mFunc;
};

/**
 * An instance of LogThrottle mediates calls to some other specified function,
 * ensuring that it's called no more often than the specified time interval.
 * When that interval is exceeded, it logs a message at the specified log
 * level. It uses LL_MUMBLES_ONCE() logic to prevent spamming, since a too-
 * frequent call may well be spammy.
 */
template <LLError::ELevel LOGLEVEL, typename SIGNATURE>
class LogThrottle: public Throttle<SIGNATURE>
{
    using super = Throttle<SIGNATURE>;
public:
    LogThrottle(const std::string& desc,
                const std::function<SIGNATURE>& func,
                F64 interval):
        super(desc, func, interval)
    {}
    template <typename R, class C>
    LogThrottle(const std::string& desc, R C::* method, F64 interval):
        super(desc, method, interval)
    {}
    template <typename R, class C>
    LogThrottle(const std::string& desc, R C::* method, C* instance, F64 interval):
        super(desc, method, instance, interval)
    {}
    template <typename R, class C>
    LogThrottle(const std::string& desc, R C::* method, const C* instance, F64 interval):
        super(desc, method, instance, interval)
    {}

private:
    void suppress() override
    {
        // Using lllog(), the macro underlying LL_WARNS() et al., allows
        // specifying compile-time LOGLEVEL. It does NOT support a variable
        // LOGLEVEL, which is why LOGLEVEL is a non-type template parameter.
        // See llvlog() for variable support, which is a bit more expensive.
        // true = only print the log message once
        lllog(LOGLEVEL, true, "LogThrottle") << std::quoted(super::mDesc)
                                             << " called more than once per "
                                             << super::mInterval
                                             << LL_ENDL;
    }
};

#endif /* ! defined(LL_THROTTLE_H) */
