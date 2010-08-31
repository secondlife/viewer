/** 
 * @file llplugincookiestore_test.cpp
 * @brief Unit tests for LLPluginCookieStore.
 *
 * @cond
 * $LicenseInfo:firstyear=2010&license=viewergpl$
 *
 * Copyright (c) 2010, Linden Research, Inc.
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
 * @endcond
 */

#include "linden_common.h"
#include "../test/lltut.h"

#include "../llplugincookiestore.h"


namespace tut
{
	// Main Setup
	struct LLPluginCookieStoreFixture
	{
		LLPluginCookieStoreFixture()
		{
			// We need dates definitively in the past and the future to properly test cookie expiration.
			LLDate now = LLDate::now(); 
			LLDate past(now.secondsSinceEpoch() - (60.0 * 60.0 * 24.0));	// 1 day in the past
			LLDate future(now.secondsSinceEpoch() + (60.0 * 60.0 * 24.0));	// 1 day in the future
			
			mPastString = past.asRFC1123();
			mFutureString = future.asRFC1123();
		}
		
		std::string mPastString;
		std::string mFutureString;
		LLPluginCookieStore mCookieStore;
		
		// List of cookies used for validation
		std::list<std::string> mCookies;
		
		// This sets up mCookies from a string returned by one of the functions in LLPluginCookieStore
		void setCookies(const std::string &cookies)
		{
			mCookies.clear();
			std::string::size_type start = 0;

			while(start != std::string::npos)
			{
				std::string::size_type end = cookies.find_first_of("\r\n", start);
				if(end > start)
				{
					std::string line(cookies, start, end - start);
					if(line.find_first_not_of("\r\n\t ") != std::string::npos)
					{
						// The line has some non-whitespace characters.  Save it to the list.
						mCookies.push_back(std::string(cookies, start, end - start));
					}
				}
				start = cookies.find_first_not_of("\r\n ", end);
			}
		}
		
		// This ensures that a cookie matching the one passed is in the list.
		void ensureCookie(const std::string &cookie)
		{
			std::list<std::string>::iterator iter;
			for(iter = mCookies.begin(); iter != mCookies.end(); iter++)
			{
				if(*iter == cookie)
				{
					// Found the cookie
					// TODO: this should do a smarter equality comparison on the two cookies, instead of just a string compare.
					return;
				}
			}
			
			// Didn't find this cookie
			std::string message = "cookie not found: ";
			message += cookie;
			ensure(message, false);
		}
		
		// This ensures that the number of cookies in the list matches what's expected.
		void ensureSize(const std::string &message, size_t size)
		{
			if(mCookies.size() != size)
			{
				std::stringstream full_message;
				
				full_message << message << " (expected " << size << ", actual " << mCookies.size() << ")";
				ensure(full_message.str(), false);
			}
		}
	};
	
	typedef test_group<LLPluginCookieStoreFixture> factory;
	typedef factory::object object;
	factory tf("LLPluginCookieStore test");

	// Tests
	template<> template<>
	void object::test<1>()
	{
		// Test 1: cookie uniqueness and update lists.
		// Valid, distinct cookies:
		
		std::string cookie01 = "cookieA=value; domain=example.com; path=/";
		std::string cookie02 = "cookieB=value; Domain=example.com; Path=/; Max-Age=10; Secure; Version=1; Comment=foo!; HTTPOnly"; // cookie with every supported field, in different cases.
		std::string cookie03 = "cookieA=value; domain=foo.example.com; path=/"; // different domain
		std::string cookie04 = "cookieA=value; domain=example.com; path=/bar/"; // different path
		std::string cookie05 = "cookieC; domain=example.com; path=/"; // empty value
		std::string cookie06 = "cookieD=value; domain=example.com; path=/; expires="; // different name, persistent cookie
		cookie06 += mFutureString;
		
		mCookieStore.setCookies(cookie01);
		mCookieStore.setCookies(cookie02);
		mCookieStore.setCookies(cookie03);
		mCookieStore.setCookies(cookie04);
		mCookieStore.setCookies(cookie05);
		mCookieStore.setCookies(cookie06);
		
		// Invalid cookies (these will get parse errors and not be added to the store)

		std::string badcookie01 = "cookieD=value; domain=example.com; path=/; foo=bar"; // invalid field name
		std::string badcookie02 = "cookieE=value; path=/"; // no domain

		mCookieStore.setCookies(badcookie01);
		mCookieStore.setCookies(badcookie02);
		
		// All cookies added so far should have been marked as "changed"
		setCookies(mCookieStore.getChangedCookies());
		ensureSize("count of changed cookies", 6);
		ensureCookie(cookie01);
		ensureCookie(cookie02);
		ensureCookie(cookie03);
		ensureCookie(cookie04);
		ensureCookie(cookie05);
		ensureCookie(cookie06);
		
		// Save off the current state of the cookie store (we'll restore it later)
		std::string savedCookies = mCookieStore.getAllCookies();
		
		// Test replacing cookies
		std::string cookie01a = "cookieA=newvalue; domain=example.com; path=/";	// updated value
		std::string cookie02a = "cookieB=newvalue; domain=example.com; path=/; expires="; // remove cookie (by setting an expire date in the past)
		cookie02a += mPastString;
		
		mCookieStore.setCookies(cookie01a);
		mCookieStore.setCookies(cookie02a);

		// test for getting changed cookies
		setCookies(mCookieStore.getChangedCookies());
		ensureSize("count of updated cookies", 2);
		ensureCookie(cookie01a);
		ensureCookie(cookie02a);
		
		// and for the state of the store after getting changed cookies
		setCookies(mCookieStore.getAllCookies());
		ensureSize("count of valid cookies", 5);
		ensureCookie(cookie01a);
		ensureCookie(cookie03);
		ensureCookie(cookie04);
		ensureCookie(cookie05);
		ensureCookie(cookie06);

		// Check that only the persistent cookie is returned here
		setCookies(mCookieStore.getPersistentCookies());
		ensureSize("count of persistent cookies", 1);
		ensureCookie(cookie06);

		// Restore the cookie store to a previous state and verify
		mCookieStore.setAllCookies(savedCookies);
		
		// Since setAllCookies defaults to not marking cookies as changed, this list should be empty.
		setCookies(mCookieStore.getChangedCookies());
		ensureSize("count of changed cookies after restore", 0);

		// Verify that the restore worked as it should have.
		setCookies(mCookieStore.getAllCookies());
		ensureSize("count of restored cookies", 6);
		ensureCookie(cookie01);
		ensureCookie(cookie02);
		ensureCookie(cookie03);
		ensureCookie(cookie04);
		ensureCookie(cookie05);
		ensureCookie(cookie06);
	}

}
