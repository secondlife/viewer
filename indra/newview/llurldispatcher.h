/**
 * @file llurldispatcher.h
 * @brief Central registry for all SL URL handlers
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007, Linden Research, Inc.
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
#ifndef LLURLDISPATCHER_H
#define LLURLDISPATCHER_H

class LLURLDispatcher
{
public:
	static bool isSLURL(const std::string& url);
		// Is this any sort of secondlife:// or sl:// URL?

	static bool isSLURLCommand(const std::string& url);
		// Is this a special secondlife://app/ URL?

	static bool dispatch(const std::string& url);
		// At startup time and on clicks in internal web browsers,
		// teleport, open map, or run requested command.
		// Handles:
		//   secondlife://RegionName/123/45/67/
		//   secondlife://app/agent/3d6181b0-6a4b-97ef-18d8-722652995cf1/show
		//   sl://app/foo/bar
		// Returns true if someone handled the URL.
	static bool dispatchRightClick(const std::string& url);

		//   builds: http://slurl.com/secondlife/RegionName/x/y/z/
	static std::string buildSLURL(const std::string& regionname, S32 x, S32 y, S32 z);
};

#endif
