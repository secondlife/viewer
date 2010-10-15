/** 
 * @file llweb.h
 * @brief Functions dealing with web browsers
 * @author James Cook
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
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

#ifndef LL_LLWEB_H
#define LL_LLWEB_H

#include <string>

///
/// The LLWeb class provides various static methods to display the
/// contents of a Url in a web browser. Variations are provided to 
/// let you specifically use the Second Life internal browser, the
/// operating system's default browser, or to respect the user's
/// setting for which of these two they prefer to use with SL.
///
class LLWeb
{
public:
	static void initClass();
	
	/// Load the given url in the user's preferred web browser
	static void loadURL(const std::string& url, const std::string& target, const std::string& uuid = LLStringUtil::null);
	static void loadURL(const std::string& url) { loadURL(url, LLStringUtil::null); }
	/// Load the given url in the user's preferred web browser	
	static void loadURL(const char* url, const std::string& target) { loadURL( ll_safe_string(url), target); }
	static void loadURL(const char* url) { loadURL( ll_safe_string(url), LLStringUtil::null ); }
	/// Load the given url in the Second Life internal web browser
	static void loadURLInternal(const std::string &url, const std::string& target, const std::string& uuid = LLStringUtil::null);
	static void loadURLInternal(const std::string &url) { loadURLInternal(url, LLStringUtil::null); }
	/// Load the given url in the operating system's web browser, async if we want to return immediately
	/// before browser has spawned
	static void loadURLExternal(const std::string& url) { loadURLExternal(url,  LLStringUtil::null); };
	static void loadURLExternal(const std::string& url, const std::string& uuid);
	static void loadURLExternal(const std::string& url, bool async, const std::string& uuid = LLStringUtil::null);

	/// Returns escaped url (eg, " " to "%20") - used by all loadURL methods
	static std::string escapeURL(const std::string& url);
	/// Expands various strings like [LANG], [VERSION], etc. in a URL
	static std::string expandURLSubstitutions(const std::string &url,
											  const LLSD &default_subs);
};

#endif
