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

#include "../test/lltut.h"

namespace tut
{
	struct avatarnamecache_data
	{
	};
	typedef test_group<avatarnamecache_data> avatarnamecache_test;
	typedef avatarnamecache_test::object avatarnamecache_object;
	tut::avatarnamecache_test avatarnamecache_testcase("LLAvatarNameCache");

	template<> template<>
	void avatarnamecache_object::test<1>()
	{
		bool valid = false;
		S32 max_age = 0;

		valid = max_age_from_cache_control("max-age=3600", &max_age);
		ensure("typical input valid", valid);
		ensure_equals("typical input parsed", max_age, 3600);

		valid = max_age_from_cache_control(
			" max-age=600 , no-cache,private=\"stuff\" ", &max_age);
		ensure("complex input valid", valid);
		ensure_equals("complex input parsed", max_age, 600);

		valid = max_age_from_cache_control(
			"no-cache, max-age = 123 ", &max_age);
		ensure("complex input 2 valid", valid);
		ensure_equals("complex input 2 parsed", max_age, 123);
	}

	template<> template<>
	void avatarnamecache_object::test<2>()
	{
		bool valid = false;
		S32 max_age = -1;

		valid = max_age_from_cache_control("", &max_age);
		ensure("empty input returns invalid", !valid);
		ensure_equals("empty input doesn't change val", max_age, -1);

		valid = max_age_from_cache_control("no-cache", &max_age);
		ensure("no max-age field returns invalid", !valid);

		valid = max_age_from_cache_control("max", &max_age);
		ensure("just 'max' returns invalid", !valid);

		valid = max_age_from_cache_control("max-age", &max_age);
		ensure("partial max-age is invalid", !valid);

		valid = max_age_from_cache_control("max-age=", &max_age);
		ensure("longer partial max-age is invalid", !valid);

		valid = max_age_from_cache_control("max-age=FOO", &max_age);
		ensure("invalid integer max-age is invalid", !valid);

		valid = max_age_from_cache_control("max-age 234", &max_age);
		ensure("space separated max-age is invalid", !valid);

		valid = max_age_from_cache_control("max-age=0", &max_age);
		ensure("zero max-age is valid", valid);

		// *TODO: Handle "0000" as zero
		//valid = max_age_from_cache_control("max-age=0000", &max_age);
		//ensure("multi-zero max-age is valid", valid);

		valid = max_age_from_cache_control("max-age=-123", &max_age);
		ensure("less than zero max-age is invalid", !valid);
	}
}
