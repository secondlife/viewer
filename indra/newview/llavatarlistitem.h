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

#include "llcallingcard.h" // for LLFriendObserver

class LLAvatarIconCtrl;

class LLAvatarListItem : public LLPanel, public LLFriendObserver
{
public:
	class ContextMenu
	{
	public:
		virtual void show(LLView* spawning_view, const LLUUID& id, S32 x, S32 y) = 0;
	};

	LLAvatarListItem();
	virtual ~LLAvatarListItem();

	virtual BOOL postBuild();
	virtual void onMouseLeave(S32 x, S32 y, MASK mask);
	virtual void onMouseEnter(S32 x, S32 y, MASK mask);
	virtual BOOL handleRightMouseDown(S32 x, S32 y, MASK mask);
	virtual void setValue(const LLSD& value);
	virtual void changed(U32 mask); // from LLFriendObserver

	void setStatus(const std::string& status);
	void setOnline(bool online);
	void setName(const std::string& name);
	void setAvatarId(const LLUUID& id);
	
	const LLUUID& getAvatarId() const;
	const std::string getAvatarName() const;

	void onInfoBtnClick();
	void onProfileBtnClick();

	void showSpeakingIndicator(bool show) { mSpeakingIndicator->setVisible(show); }
	void showInfoBtn(bool show_info_btn) {mInfoBtn->setVisible(show_info_btn); }
	void showStatus(bool show_status);

	void setContextMenu(ContextMenu* menu) { mContextMenu = menu; }

private:

	typedef enum e_online_status {
		E_OFFLINE,
		E_ONLINE,
		E_UNKNOWN,
	} EOnlineStatus;

	void onNameCache(const std::string& first_name, const std::string& last_name);

	LLAvatarIconCtrl*mAvatarIcon;
	LLTextBox* mAvatarName;
	LLTextBox* mStatus;
	
	LLOutputMonitorCtrl* mSpeakingIndicator;
	LLButton* mInfoBtn;
	LLButton* mProfileBtn;
	ContextMenu* mContextMenu;

	LLUUID mAvatarId;
	EOnlineStatus mOnlineStatus;
};

#endif //LL_LLAVATARLISTITEM_H
