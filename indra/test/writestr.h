/**
 * @file   writestr.h
 * @author Nat Goodspeed
 * @date   2024-05-21
 * @brief  writestr() function for when iostream isn't set up
 *
 * $LicenseInfo:firstyear=2024&license=viewerlgpl$
 * Copyright (c) 2024, Linden Research, Inc.
 * $/LicenseInfo$
 */

#if ! defined(LL_WRITESTR_H)
#define LL_WRITESTR_H

#include "stringize.h"

#ifndef LL_WINDOWS

#include <unistd.h>

#else  // LL_WINDOWS

#include <io.h>
inline
int write(int fd, const void* buffer, unsigned int count)
{
    return _write(fd, buffer, count);
}

#endif  // LL_WINDOWS

template <typename... ARGS>
auto writestr(int fd, ARGS&&... args)
{
    std::string str{ stringize(std::forward<ARGS>(args)..., '\n') };
    return write(fd, str.data(), str.length());
}

#endif /* ! defined(LL_WRITESTR_H) */
