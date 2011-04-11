/**
 * @file llurldispatcher.h
 * @brief Central registry for all SL URL handlers
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
#ifndef LLURLDISPATCHER_H
#define LLURLDISPATCHER_H
class LLMediaCtrl;


class LLURLDispatcher
{
public:
	
	static bool dispatch(const std::string& slurl,
						 const std::string& nav_type,
						 LLMediaCtrl* web,
						 bool trusted_browser);	
		// At startup time and on clicks in internal web browsers,
		// teleport, open map, or run requested command.
		// @param url
		//   secondlife://RegionName/123/45/67/
		//   secondlife:///app/agent/3d6181b0-6a4b-97ef-18d8-722652995cf1/show
		//   sl://app/foo/bar
		// @param nav_type
		//   type of navigation type (see LLQtWebKit::LLWebPage::acceptNavigationRequest)
		// @param web
		//	 Pointer to LLMediaCtrl sending URL, can be NULL
		// @param trusted_browser
		//   True if coming inside the app AND from a brower instance
		//   that navigates to trusted (Linden Lab) pages.
		// Returns true if someone handled the URL.

	static bool dispatchRightClick(const std::string& slurl);

	static bool dispatchFromTextEditor(const std::string& slurl);
};

#endif
