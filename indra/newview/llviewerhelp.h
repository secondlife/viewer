/** 
 * @file llviewerhelp.h
 * @brief Utility functions for the Help system
 * @author Tofu Linden
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

	// return topic derived from viewer UI focus, else default topic
	std::string getTopicFromFocus();

	/// return default (fallback) topic name suitable for showTopic()
	/*virtual*/ std::string defaultTopic();

	// return topic to use before the user logs in
	/*virtual*/ std::string preLoginTopic();

	// return topic to use for the top-level help, invoked by F1
	/*virtual*/ std::string f1HelpTopic();

 private:
	static void showHelp(); // make sure help UI is visible & raised
	static void setRawURL(std::string url); // send URL to help UI
};

#endif // header guard
