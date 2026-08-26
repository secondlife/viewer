/**
 * @file llcachemigration_test.cpp
 * @brief Tests for cache-directory migration guards.
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

#include "linden_common.h"
#include "../test/lltut.h"

#include "../llcachemigration.h"

namespace tut
{
struct cache_migration_test {};
typedef test_group<cache_migration_test> cache_migration_test_t;
typedef cache_migration_test_t::object cache_migration_test_object_t;
cache_migration_test_t cache_migration_tests("LLCacheMigration");

template<> template<>
void cache_migration_test_object_t::test<1>()
{
    ensure("identical cache is not migrated",
           !LLCacheMigration::required("/profile/cache", "/profile/cache"));
    ensure("normalized identical cache is not migrated",
           !LLCacheMigration::required("/profile/./cache", "/profile/cache"));
    ensure("UTF-8 normalized identical cache is not migrated",
           !LLCacheMigration::required("/profile/\xC3\xA9/./cache", "/profile/\xC3\xA9/cache"));
}

template<> template<>
void cache_migration_test_object_t::test<2>()
{
    ensure("different cache is migrated",
           LLCacheMigration::required("/profile/cache", "/system/cache"));
}
}
