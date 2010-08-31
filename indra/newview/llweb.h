/** 
 * @file llweb.h
 * @brief Functions dealing with web browsers
 * @author James Cook
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2009, Linden Research, Inc.
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
	static void loadURL(const std::string& url);
	/// Load the given url in the user's preferred web browser	
	static void loadURL(const char* url) { loadURL( ll_safe_string(url) ); }
	/// Load the given url in the Second Life internal web browser
	static void loadURLInternal(const std::string &url);
	/// Load the given url in the operating system's web browser, async if we want to return immediately
	/// before browser has spawned
	static void loadURLExternal(const std::string& url);
	static void loadURLExternal(const std::string& url, bool async);

	/// Returns escaped url (eg, " " to "%20") - used by all loadURL methods
	static std::string escapeURL(const std::string& url);
	/// Expands various strings like [LANG], [VERSION], etc. in a URL
	static std::string expandURLSubstitutions(const std::string &url,
											  const LLSD &default_subs);
};

#endif
