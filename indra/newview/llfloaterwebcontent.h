/**
 * @file llfloaterwebcontent.h
 * @brief floater for displaying web content - e.g. profiles and search (eventually)
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

#ifndef LL_LLFLOATERWEBCONTENT_H
#define LL_LLFLOATERWEBCONTENT_H

#include "llfloater.h"
#include "llmediactrl.h"
#include "llsdparam.h"

class LLMediaCtrl;
class LLComboBox;
class LLTextBox;
class LLProgressBar;
class LLIconCtrl;

class LLFloaterWebContent :
	public LLFloater,
	public LLViewerMediaObserver,
	public LLInstanceTracker<LLFloaterWebContent, std::string>
{
public:
	typedef LLInstanceTracker<LLFloaterWebContent, std::string> instance_tracker_t;
    LOG_CLASS(LLFloaterWebContent);

	struct _Params : public LLInitParam::Block<_Params>
	{
		Optional<std::string>	url,
								target,
								window_class,
								id;
		Optional<bool>			show_chrome,
								allow_address_entry,
								trusted_content,
								show_page_title;
		Optional<LLRect>		preferred_media_size;

		_Params();
	};

	typedef LLSDParamAdapter<_Params> Params;

	LLFloaterWebContent(const Params& params);

	void initializeURLHistory();

	static LLFloater* create(Params);

	static void closeRequest(const std::string &uuid);
	static void geometryChanged(const std::string &uuid, S32 x, S32 y, S32 width, S32 height);
	void geometryChanged(S32 x, S32 y, S32 width, S32 height);

	/* virtual */ BOOL postBuild();
	/* virtual */ void onOpen(const LLSD& key);
	/* virtual */ bool matchesKey(const LLSD& key);
	/* virtual */ void onClose(bool app_quitting);
	/* virtual */ void draw();

protected:
	// inherited from LLViewerMediaObserver
	/*virtual*/ void handleMediaEvent(LLPluginClassMedia* self, EMediaEvent event);

	void onClickBack();
	void onClickForward();
	void onClickReload();
	void onClickStop();
	void onEnterAddress();
	void onPopExternal();

	static void preCreate(Params& p);
	void open_media(const Params& );
	void set_current_url(const std::string& url);

	LLMediaCtrl*	mWebBrowser;
	LLComboBox*		mAddressCombo;
	LLIconCtrl*		mSecureLockIcon;
	LLTextBox*		mStatusBarText;
	LLProgressBar*	mStatusBarProgress;

	LLView*			mBtnBack;
	LLView*			mBtnForward;
	LLView*			mBtnReload;
	LLView*			mBtnStop;

	std::string		mCurrentURL;
	std::string		mUUID;
	bool			mShowPageTitle;
};

#endif  // LL_LLFLOATERWEBCONTENT_H
