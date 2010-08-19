/** 
 * @file llfloatermediabrowser.h
 * @brief media browser floater - uses embedded media browser control
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

#ifndef LL_LLFLOATERMEDIABROWSER_H
#define LL_LLFLOATERMEDIABROWSER_H

#include "llfloater.h"
#include "llmediactrl.h"


class LLComboBox;
class LLMediaCtrl;

class LLFloaterMediaBrowser : 
	public LLFloater, 
	public LLViewerMediaObserver
{
public:
	LLFloaterMediaBrowser(const LLSD& key);

	/*virtual*/ BOOL postBuild();
	/*virtual*/ void onClose(bool app_quitting);
	/*virtual*/ void draw();
	/*virtual*/ void onOpen(const LLSD& key);

	// inherited from LLViewerMediaObserver
	/*virtual*/ void handleMediaEvent(LLPluginClassMedia* self, EMediaEvent event);

	void openMedia(const std::string& media_url);
	void buildURLHistory();
	std::string getSupportURL();
	void setCurrentURL(const std::string& url);

	static void onEnterAddress(LLUICtrl* ctrl, void* user_data);
	static void onClickRefresh(void* user_data);
	static void onClickBack(void* user_data);
	static void onClickForward(void* user_data);
	static void onClickGo(void* user_data);
	static void onClickClose(void* user_data);
	static void onClickOpenWebBrowser(void* user_data);
	static void onClickAssign(void* user_data);
	static void onClickRewind(void* user_data);
	static void onClickPlay(void* user_data);
	static void onClickStop(void* user_data);
	static void onClickSeek(void* user_data);

private:
	LLMediaCtrl* mBrowser;
	LLComboBox* mAddressCombo;
	std::string mCurrentURL;
};

#endif  // LL_LLFLOATERMEDIABROWSER_H

