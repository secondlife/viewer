/** 
 * @file llplugincookiestore.h
 * @brief LLPluginCookieStore provides central storage for http cookies used by plugins
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

#ifndef LL_LLPLUGINCOOKIESTORE_H
#define LL_LLPLUGINCOOKIESTORE_H

#include "lldate.h"
#include <map>
#include <string>
#include <iostream>

class LLPluginCookieStore
{
	LOG_CLASS(LLPluginCookieStore);
public:
	LLPluginCookieStore();
	~LLPluginCookieStore();

	// gets all cookies currently in storage -- use when initializing a plugin
	std::string getAllCookies();
	void writeAllCookies(std::ostream& s);
	
	// gets only persistent cookies (i.e. not session cookies) -- use when writing cookies to a file
	std::string getPersistentCookies();
	void writePersistentCookies(std::ostream& s);
	
	// gets cookies which are marked as "changed" -- use when sending periodic updates to plugins
	std::string getChangedCookies(bool clear_changed = true);
	void writeChangedCookies(std::ostream& s, bool clear_changed = true);
	
	// (re)initializes internal data structures and bulk-sets cookies -- use when reading cookies from a file
	void setAllCookies(const std::string &cookies, bool mark_changed = false);
	void readAllCookies(std::istream& s, bool mark_changed = false);
	
	// sets one or more cookies (without reinitializing anything) -- use when receiving cookies from a plugin
	void setCookies(const std::string &cookies, bool mark_changed = true);
	void readCookies(std::istream& s, bool mark_changed = true);

	// quote or unquote a string as per the definition of 'quoted-string' in rfc2616
	static std::string quoteString(const std::string &s);
	static std::string unquoteString(const std::string &s);
	
private:

	void setOneCookie(const std::string &s, std::string::size_type cookie_start, std::string::size_type cookie_end, bool mark_changed);

	class Cookie
	{
	public:
		static Cookie *createFromString(const std::string &s, std::string::size_type cookie_start = 0, std::string::size_type cookie_end = std::string::npos);
		
		// Construct a string from the cookie that uniquely represents it, to be used as a key in a std::map.
		std::string getKey() const;
		
		const std::string &getCookie() const { return mCookie; };
		bool isSessionCookie() const { return mDate.isNull(); };

		bool isDead() const { return mDead; };
		void setDead(bool dead) { mDead = dead; };
		
		bool isChanged() const { return mChanged; };
		void setChanged(bool changed) { mChanged = changed; };

		const LLDate &getDate() const { return mDate; };
		
	private:
		Cookie(const std::string &s, std::string::size_type cookie_start = 0, std::string::size_type cookie_end = std::string::npos);
		bool parse();
		std::string::size_type findFieldEnd(std::string::size_type start = 0, std::string::size_type end = std::string::npos);
		bool matchName(std::string::size_type start, std::string::size_type end, const char *name);
		
		std::string mCookie;	// The full cookie, in RFC 2109 string format
		LLDate mDate;			// The expiration date of the cookie.  For session cookies, this will be a null date (mDate.isNull() is true).
		// Start/end indices of various parts of the cookie string.  Stored as indices into the string to save space and time.
		std::string::size_type mNameStart, mNameEnd;
		std::string::size_type mValueStart, mValueEnd;
		std::string::size_type mDomainStart, mDomainEnd;
		std::string::size_type mPathStart, mPathEnd;
		bool mDead;
		bool mChanged;
	};
	
	typedef std::map<std::string, Cookie*> cookie_map_t;
	
	cookie_map_t mCookies;
	bool mHasChangedCookies;
	
	void clearCookies();
	void removeCookie(const std::string &key);
};

#endif // LL_LLPLUGINCOOKIESTORE_H
