/** 
 * @file llavatarlistitem.h
 * @avatar list item header file
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

#ifndef LL_LLAVATARLISTITEM_H
#define LL_LLAVATARLISTITEM_H

#include "llpanel.h"
#include "lloutputmonitorctrl.h"
#include "llbutton.h"
#include "lltextbox.h"
#include "llstyle.h"

#include "llcallingcard.h" // for LLFriendObserver

class LLAvatarIconCtrl;

class LLAvatarListItem : public LLPanel, public LLFriendObserver
{
public:
	class ContextMenu
	{
	public:
		virtual void show(LLView* spawning_view, const std::vector<LLUUID>& selected_uuids, S32 x, S32 y) = 0;
	};

	/**
	 * Creates an instance of LLAvatarListItem.
	 *
	 * It is not registered with LLDefaultChildRegistry. It is built via LLUICtrlFactory::buildPanel
	 * or via registered LLCallbackMap depend on passed parameter.
	 * 
	 * @param not_from_ui_factory if true instance will be build with LLUICtrlFactory::buildPanel 
	 * otherwise it should be registered via LLCallbackMap before creating.
	 */
	LLAvatarListItem(bool not_from_ui_factory = true);
	virtual ~LLAvatarListItem();

	virtual BOOL postBuild();
	virtual void onMouseLeave(S32 x, S32 y, MASK mask);
	virtual void onMouseEnter(S32 x, S32 y, MASK mask);
	virtual void setValue(const LLSD& value);
	virtual void changed(U32 mask); // from LLFriendObserver

	void setOnline(bool online);
	void setName(const std::string& name);
	void setHighlight(const std::string& highlight);
	void setAvatarId(const LLUUID& id, bool ignore_status_changes = false);
	void setLastInteractionTime(U32 secs_since);
	//Show/hide profile/info btn, translating speaker indicator and avatar name coordinates accordingly
	void setShowProfileBtn(bool show);
	void setShowInfoBtn(bool show);
	void setSpeakingIndicatorVisible(bool visible);
	void setAvatarIconVisible(bool visible);
	
	const LLUUID& getAvatarId() const;
	const std::string getAvatarName() const;

	void onInfoBtnClick();
	void onProfileBtnClick();

	void showSpeakingIndicator(bool show) { mSpeakingIndicator->setVisible(show); }
	void showInfoBtn(bool show_info_btn) {mInfoBtn->setVisible(show_info_btn); }
	void showLastInteractionTime(bool show);

	void setContextMenu(ContextMenu* menu) { mContextMenu = menu; }

	/**
	 * This method was added to fix EXT-2364 (Items in group/ad-hoc IM participant list (avatar names) should be reshaped when adding/removing the "(Moderator)" label)
	 * But this is a *HACK. The real reason of it was in incorrect logic while hiding profile/info/speaker buttons
	 * *TODO: new reshape method should be provided in lieu of this one to be called when visibility if those buttons is changed
	 */
	void reshapeAvatarName();

protected:
	/**
	 * Contains indicator to show voice activity. 
	 */
	LLOutputMonitorCtrl* mSpeakingIndicator;

private:

	typedef enum e_online_status {
		E_OFFLINE,
		E_ONLINE,
		E_UNKNOWN,
	} EOnlineStatus;

	void setNameInternal(const std::string& name, const std::string& highlight);
	void onNameCache(const std::string& first_name, const std::string& last_name);

	std::string formatSeconds(U32 secs);

	LLAvatarIconCtrl* mAvatarIcon;
	LLTextBox* mAvatarName;
	LLTextBox* mLastInteractionTime;
	LLStyle::Params mAvatarNameStyle;
	
	LLButton* mInfoBtn;
	LLButton* mProfileBtn;
	ContextMenu* mContextMenu;

	LLUUID mAvatarId;
	std::string mHighlihtSubstring; // substring to highlight
	EOnlineStatus mOnlineStatus;
	//Flag indicating that info/profile button shouldn't be shown at all.
	//Speaker indicator and avatar name coords are translated accordingly
	bool mShowInfoBtn;
	bool mShowProfileBtn;

	static bool	sStaticInitialized; // this variable is introduced to improve code readability
	static S32	sIconWidth; // icon width + padding
	static S32  sInfoBtnWidth; //info btn width + padding
	static S32  sProfileBtnWidth; //profile btn width + padding
	static S32  sSpeakingIndicatorWidth; //speaking indicator width + padding
};

#endif //LL_LLAVATARLISTITEM_H
