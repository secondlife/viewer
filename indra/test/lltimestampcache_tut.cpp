/**
 * @file lltimestampcache_tut.cpp
 * @author James Tess
 * @date 2008-12-03
 *
 * $LicenseInfo:firstyear=2008&license=viewergpl$
 * 
 * Copyright (c) 2008, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

#include <tut/tut.hpp>

#include "linden_common.h"
#include "../mapserver/lltimestampcache.h"
#include "lltut.h"


namespace tut
{
	struct LLTimestampCacheTestData
	{
	};

	typedef test_group<LLTimestampCacheTestData> LLTimestampCacheTestGroup;
	typedef LLTimestampCacheTestGroup::object LLTimestampCacheTestObject;

	LLTimestampCacheTestGroup timestampCacheTestGroup("LLTimestampCache");

	// Most common usage
	template<> template<>
		void LLTimestampCacheTestObject::test<1>()
		{
			LLTimestampCache<std::string, std::string> cache;
			// put in some data
			cache.insert("key1", "val1", 1);
			cache.insert("key2", "val2", 2);
			cache.insert("key3", "val3", 3);
			ensure_equals("size is 3", cache.size(), 3);
			// check some items
			ensure("has key1", cache.has("key1"));
			ensure("no invalid key", !cache.has("invalid key"));
			// get some items
			ensure_equals("get key1", cache.get("key1", 4), "val1");
			ensure_equals("get invalid key", 
					cache.get("invalid key", 4), std::string() );
			// timestamps
			ensure_equals("key1 timestamp updated", cache.getTimestamp("key1"), 4);
			ensure_equals("invalid key timestamp", 
					cache.getTimestamp("invalid key"), 0);
		}

	// New empty cache shouldn't have any entries
	template<> template<>
		void LLTimestampCacheTestObject::test<2>()
		{
			LLTimestampCache<std::string, std::string> cache;
			ensure_equals("starts empty",   cache.size(), 0);
			ensure_equals("has nothing",    cache.has("foo"), false);
			ensure_equals("gets nothing",   cache.get("foo", 0), std::string() );
			U32 max_time = 0xFFFFFFFF;
			ensure_equals("erases nothing", cache.eraseBefore(max_time), 0);
		}

	// Non empty cache
	template<> template<>
		void LLTimestampCacheTestObject::test<3>()
		{
			LLTimestampCache<std::string, std::string> cache;
			cache.insert("foo", "bar", 123);
			ensure_equals("size one", cache.size(), 1);
			ensure_equals("has it", cache.has("foo"), true);
			ensure_equals("timestamp correct", cache.getTimestamp("foo"), 123);
			std::string value = cache.get("foo", 456);
			ensure_equals("get value", value, "bar");
			ensure_equals("timestamp updated", cache.getTimestamp("foo"), 456);
			ensure_equals("erase nothing", cache.eraseBefore(0), 0);
			ensure_equals("erase one", cache.eraseBefore(789), 1);
			ensure_equals("empty after erase", cache.size(), 0);
		}

	// Recache of item should update timestamp
	template<> template<>
		void LLTimestampCacheTestObject::test<4>()
		{
			LLTimestampCache<std::string, std::string> cache;
			cache.insert("foo", "bar", 123);
			cache.insert("foo", "bar", 456);
			ensure_equals("duplicate suppressed", cache.size(), 1);
			ensure_equals("timestamp replaced", cache.getTimestamp("foo"), 456);
		}

	// Erasing some items
	template<> template<>
		void LLTimestampCacheTestObject::test<5>()
		{
			LLTimestampCache<std::string, std::string> cache;
			cache.insert("key1", "val1", 1);
			cache.insert("key2", "val2", 2);
			cache.insert("key3", "val3", 3);
			cache.insert("key4", "val4", 4);
			size_t erased = cache.eraseBefore(3);
			ensure_equals("erase range", erased, 2);
			ensure_equals("cache post erase", cache.size(), 2);
			ensure_equals("has key3", cache.has("key3"), true);
			ensure_equals("not has key2", cache.has("key2"), false);
		}
}
