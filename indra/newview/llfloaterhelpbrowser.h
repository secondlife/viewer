/** 
 * @file llfloaterhelpbrowser.h
 * @brief HTML Help floater - uses embedded web browser control
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

#ifndef LL_LLFLOATERHELPBROWSER_H
#define LL_LLFLOATERHELPBROWSER_H

#include "llfloater.h"
#include "llmediactrl.h"


class LLMediaCtrl;

class LLFloaterHelpBrowser : 
	public LLFloater, 
	public LLViewerMediaObserver
{
 public:
	LLFloaterHelpBrowser(const LLSD& key);

	/*virtual*/ BOOL postBuild();
	/*virtual*/ void onClose(bool app_quitting);
	/*virtual*/ void onOpen(const LLSD& key);

	// inherited from LLViewerMediaObserver
	/*virtual*/ void handleMediaEvent(LLPluginClassMedia* self, EMediaEvent event);

	void openMedia(const std::string& media_url);
	
 private:
	void buildURLHistory();
	void setCurrentURL(const std::string& url);

	static void onClickClose(void* user_data);
	static void onClickOpenWebBrowser(void* user_data);

 private:
	LLMediaCtrl* mBrowser;
	std::string mCurrentURL;
};

#endif  // LL_LLFLOATERHELPBROWSER_H

