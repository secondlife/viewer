/** 
 * @file llweb.h
 * @brief Functions dealing with web browsers
 * @author James Cook
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

#ifndef LL_LLWEB_H
#define LL_LLWEB_H

#include <string>
#include "llalertdialog.h"

class LLWeb
{
public:
	static void initClass();
	
	// Loads unescaped url in either internal web browser or external
	// browser, depending on user settings.
	static void loadURL(const std::string& url);
	
	static void loadURL(const char* url) { loadURL( ll_safe_string(url) ); }

	// Loads unescaped url in external browser.
	static void loadURLExternal(const std::string& url);

	// Returns escaped (eg, " " to "%20") url
	static std::string escapeURL(const std::string& url);

	class URLLoader : public LLAlertDialog::URLLoader
	{
		virtual void load(const std::string& url);
	};

	static URLLoader sAlertURLLoader;
};

#endif
