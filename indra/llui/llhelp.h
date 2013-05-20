/** 
 * @file llhelp.h
 * @brief Abstract interface to the Help system
 * @author Tofu Linden
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

#ifndef LL_LLHELP_H
#define LL_LLHELP_H

class LLHelp
{
 public:
	virtual void showTopic(const std::string &topic) = 0;
	virtual std::string getURL(const std::string &topic) = 0;
	// return default (fallback) topic name suitable for showTopic()
	virtual std::string defaultTopic() = 0;
	// return topic to use before the user logs in
	virtual std::string preLoginTopic() = 0;
	// return topic to use for the top-level help, invoked by F1
	virtual std::string f1HelpTopic() = 0;
};

#endif // headerguard
