/**
 * @file llcachemigration.h
 * @brief Cache-directory migration guards.
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2026, Linden Research, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * $/LicenseInfo$
 */

#ifndef LL_LLCACHEMIGRATION_H
#define LL_LLCACHEMIGRATION_H

#include "fsyspath.h"

namespace LLCacheMigration
{
inline bool required(const std::string& old_cache,
                     const std::string& new_cache)
{
    return fsyspath(old_cache).lexically_normal() != fsyspath(new_cache).lexically_normal();
}
}

#endif // LL_LLCACHEMIGRATION_H
