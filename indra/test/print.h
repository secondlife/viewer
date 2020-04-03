/**
 * @file   print.h
 * @author Nat Goodspeed
 * @date   2020-01-02
 * @brief  print() function for debugging
 * 
 * $LicenseInfo:firstyear=2020&license=viewerlgpl$
 * Copyright (c) 2020, Linden Research, Inc.
 * $/LicenseInfo$
 */

#if ! defined(LL_PRINT_H)
#define LL_PRINT_H

#include <iostream>

// print(..., NONL);
// leaves the output dangling, suppressing the normally appended std::endl
struct NONL_t {};
#define NONL (NONL_t())

// normal recursion end
inline
void print()
{
    std::cerr << std::endl;
}

// print(NONL) is a no-op
inline
void print(NONL_t)
{
}

template <typename T, typename... ARGS>
void print(T&& first, ARGS&&... rest)
{
    std::cerr << first;
    print(std::forward<ARGS>(rest)...);
}

#endif /* ! defined(LL_PRINT_H) */
