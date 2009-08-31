/** 
 * @file llfloatermediabrowser.h
 * @brief HTML Help floater - uses embedded web browser control
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2009, Linden Research, Inc.
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

#ifndef LL_LLFLOATERMEDIABROWSER_H
#define LL_LLFLOATERMEDIABROWSER_H

#include "llhtmlhelp.h"
#include "llfloater.h"
#include "llmediactrl.h"

class LLViewerHtmlHelp : public LLHtmlHelp
{
public:
	LLViewerHtmlHelp();
	virtual ~LLViewerHtmlHelp();

	/*virtual*/ void show();
	/*virtual*/ void show(std::string start_url);
	void show(std::string start_url, std::string title);

	static bool onClickF1HelpLoadURL(const LLSD& notification, const LLSD& response);

};

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

extern LLViewerHtmlHelp gViewerHtmlHelp;

#endif  // LL_LLFLOATERMEDIABROWSER_H

