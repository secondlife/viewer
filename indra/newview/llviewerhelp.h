/** 
 * @file llviewerhelp.h
 * @brief Utility functions for the Help system
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

#ifndef LL_LLVIEWERHELP_H
#define LL_LLVIEWERHELP_H

// The Help UI lives in llfloaterhelpbrowser, llviewerhelp provides a
// layer of abstraction that protects help-system-using code from the details of
// the Help UI floater and how help topics are converted into URLs.

#include "llhelp.h" // our abstract base
#include "llsingleton.h"

class LLUICtrl;

class LLViewerHelp : public LLHelp, public LLSingleton<LLViewerHelp>
{
	friend class LLSingleton<LLViewerHelp>;

 public:
	/// display the specified help topic in the help viewer
	/*virtual*/ void showTopic(const std::string &topic);

	std::string getURL(const std::string& topic);

	// return topic derived from viewer UI focus, else default topic
	std::string getTopicFromFocus();

	/// return default (fallback) topic name suitable for showTopic()
	/*virtual*/ std::string defaultTopic();

	// return topic to use before the user logs in
	/*virtual*/ std::string preLoginTopic();

	// return topic to use for the top-level help, invoked by F1
	/*virtual*/ std::string f1HelpTopic();
};

#endif // header guard
