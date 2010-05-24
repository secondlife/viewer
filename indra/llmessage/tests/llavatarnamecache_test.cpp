/**
 * @file llhost_test.cpp
 * @author Adroit
 * @date 2007-02
 * @brief llhost test cases.
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
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
	tut::avatarnamecache_test avatarnamecache_testcase("llavatarnamecache");

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
