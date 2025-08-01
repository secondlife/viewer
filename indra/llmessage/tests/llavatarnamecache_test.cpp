/**
 * @file llavatarnamecache_test.cpp
 * @author James Cook
 * @brief LLAvatarNameCache test cases.
 *
 * $LicenseInfo:firstyear=2010&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#include "linden_common.h"

#include "../llavatarnamecache.h"

#include "../test/lldoctest.h"

TEST_SUITE("LLAvatarNameCache") {

TEST_CASE("test_1")
{

        bool valid = false;
        S32 max_age = 0;

        valid = max_age_from_cache_control("max-age=3600", &max_age);
        CHECK_MESSAGE(valid, "typical input valid");
        CHECK_MESSAGE(max_age == 3600, "typical input parsed");

        valid = max_age_from_cache_control(
            " max-age=600 , no-cache,private=\"stuff\" ", &max_age);
        CHECK_MESSAGE(valid, "complex input valid");
        CHECK_MESSAGE(max_age == 600, "complex input parsed");

        valid = max_age_from_cache_control(
            "no-cache, max-age = 123 ", &max_age);
        CHECK_MESSAGE(valid, "complex input 2 valid");
        CHECK_MESSAGE(max_age == 123, "complex input 2 parsed");
    
}

TEST_CASE("test_2")
{

        bool valid = false;
        S32 max_age = -1;

        valid = max_age_from_cache_control("", &max_age);
        CHECK_MESSAGE(!valid, "empty input returns invalid");
        CHECK_MESSAGE(max_age == -1, "empty input doesn't change val");

        valid = max_age_from_cache_control("no-cache", &max_age);
        CHECK_MESSAGE(!valid, "no max-age field returns invalid");

        valid = max_age_from_cache_control("max", &max_age);
        CHECK_MESSAGE(!valid, "just 'max' returns invalid");

        valid = max_age_from_cache_control("max-age", &max_age);
        CHECK_MESSAGE(!valid, "partial max-age is invalid");

        valid = max_age_from_cache_control("max-age=", &max_age);
        CHECK_MESSAGE(!valid, "longer partial max-age is invalid");

        valid = max_age_from_cache_control("max-age=FOO", &max_age);
        CHECK_MESSAGE(!valid, "invalid integer max-age is invalid");

        valid = max_age_from_cache_control("max-age 234", &max_age);
        CHECK_MESSAGE(!valid, "space separated max-age is invalid");

        valid = max_age_from_cache_control("max-age=0", &max_age);
        CHECK_MESSAGE(valid, "zero max-age is valid");

        // *TODO: Handle "0000" as zero
        //valid = max_age_from_cache_control("max-age=0000", &max_age);
        //CHECK_MESSAGE(valid, "multi-zero max-age is valid");

        valid = max_age_from_cache_control("max-age=-123", &max_age);
        CHECK_MESSAGE(!valid, "less than zero max-age is invalid");
    
}

} // TEST_SUITE

