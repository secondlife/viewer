/** 
 * @file llslurl.h
 * @brief SLURL manipulation
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * 
 * Copyright (c) 2009, Linden Research, Inc.
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

#ifndef LL_SLURL_H
#define LL_SLURL_H

#include <string>

// IAN BUG: where should this live?
// IAN BUG: are static utility functions right?  See LLUUID.
// question of whether to have a LLSLURL object or a 
// some of this was moved from LLURLDispatcher

/**
 * SLURL manipulation
 */
class LLSLURL
{
public:
	static const std::string PREFIX_SL_HELP;
	static const std::string PREFIX_SL;
	static const std::string PREFIX_SECONDLIFE;
	static const std::string PREFIX_SLURL;
	static const std::string PREFIX_SLURL_OLD;
	static const std::string PREFIX_SLURL_WWW;

	static const std::string APP_TOKEN;

	/**
	 * Is this any sort of secondlife:// or sl:// URL?
	 */
	static bool isSLURL(const std::string& url);

	/**
	 * Is this a special secondlife://app/ URL?
	 */
	static bool isSLURLCommand(const std::string& url);

	/**
	 * Not sure what it is.
	 */
	static bool isSLURLHelp(const std::string& url);

	/**
	 * builds: http://slurl.com/secondlife/Region%20Name/x/y/z/ escaping result url.
	 */
	static std::string buildSLURL(const std::string& regionname, S32 x, S32 y, S32 z);

	/// Build a SLURL like secondlife:///app/agent/<uuid>/inspect
	static std::string buildCommand(const char* noun, const LLUUID& id, const char* verb);

	/**
	 * builds: http://slurl.com/secondlife/Region Name/x/y/z/ without escaping result url.
	 */
	static std::string buildUnescapedSLURL(const std::string& regionname, S32 x, S32 y, S32 z);

	/**
	 * builds SLURL from global position. Returns escaped or unescaped url.
	 * Returns escaped url by default.
	 */
	static std::string buildSLURLfromPosGlobal(const std::string& regionname,
											   const LLVector3d& global_pos,
											   bool escaped = true);
	/**
	 * Strip protocol part from the URL.
	 */
	static std::string stripProtocol(const std::string& url);

	/**
	 * Convert global position to X, Y Z
	 */
	static void globalPosToXYZ(const LLVector3d& pos, S32& x, S32& y, S32& z);

private:
	static bool matchPrefix(const std::string& url, const std::string& prefix);

};

#endif
