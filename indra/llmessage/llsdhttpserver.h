/** 
 * @file llsdhttpserver.h
 * @brief Standard LLSD services
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2007, Linden Research, Inc.
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

#ifndef LL_LLSDHTTPSERVER_H
#define LL_LLSDHTTPSERVER_H

/** 
 * This module implements and defines common services that should be included
 * in all server URL trees.  These services facilitate debugging and
 * introsepction.
 */

class LLHTTPStandardServices
{
public:
	static void useServices();
		/**< 
			Having a call to this function causes the following services to be
			registered:
				/web/echo			-- echo input
				/web/hello			-- return "hello"
				/web/server/api		-- return a list of url paths on the server
				/web/server/api/<..path..>
									-- return description of the path
		*/
};

#endif // LL_LLSDHTTPSERVER_H
