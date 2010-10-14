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
class LLNotification;

class LLFloaterMediaBrowser : 
	public LLFloater, 
	public LLViewerMediaObserver
{
public:
    LOG_CLASS(LLFloaterMediaBrowser);
	LLFloaterMediaBrowser(const LLSD& key);

	static void create(const std::string &url, const std::string& target, const std::string& uuid = LLStringUtil::null);

	static void closeRequest(const std::string &uuid);
	static void geometryChanged(const std::string &uuid, S32 x, S32 y, S32 width, S32 height);
	void geometryChanged(S32 x, S32 y, S32 width, S32 height);
	
	/*virtual*/ BOOL postBuild();
	/*virtual*/ void onClose(bool app_quitting);
	/*virtual*/ void draw();

	// inherited from LLViewerMediaObserver
	/*virtual*/ void handleMediaEvent(LLPluginClassMedia* self, EMediaEvent event);

	void openMedia(const std::string& media_url, const std::string& target);
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
	boost::shared_ptr<LLNotification> mCurNotification;
	std::string mUUID;
};

#endif  // LL_LLFLOATERMEDIABROWSER_H

