/**
 * @file   scope_exit.h
 * @author Nat Goodspeed
 * @date   2024-08-15
 * @brief  Cheap imitation of std::experimental::scope_exit
 *
 * $LicenseInfo:firstyear=2024&license=viewerlgpl$
 * Copyright (c) 2024, Linden Research, Inc.
 * $/LicenseInfo$
 */

#if ! defined(LL_SCOPE_EXIT_H)
#define LL_SCOPE_EXIT_H

#include <functional>

namespace LL
{

class scope_exit
{
public:
    scope_exit(const std::function<void()>& func): mFunc(func) {}
    scope_exit(const scope_exit&) = delete;
    scope_exit& operator=(const scope_exit&) = delete;
    ~scope_exit() { mFunc(); }

private:
    std::function<void()> mFunc;
};

} // namespace LL

#endif /* ! defined(LL_SCOPE_EXIT_H) */
