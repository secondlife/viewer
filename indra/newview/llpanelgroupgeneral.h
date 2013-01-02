/** 
 * @file llpanelgroupgeneral.h
 * @brief General information about a group.
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

#ifndef LL_LLPANELGROUPGENERAL_H
#define LL_LLPANELGROUPGENERAL_H

#include "llpanelgroup.h"

class LLLineEditor;
class LLTextBox;
class LLTextureCtrl;
class LLTextEditor;
class LLButton;
class LLNameListCtrl;
class LLCheckBoxCtrl;
class LLComboBox;
class LLSpinCtrl;
class LLAvatarName;

class LLPanelGroupGeneral : public LLPanelGroupTab
{
public:
	LLPanelGroupGeneral();
	virtual ~LLPanelGroupGeneral();

	// LLPanelGroupTab
	virtual void activate();
	virtual bool needsApply(std::string& mesg);
	virtual bool apply(std::string& mesg);
	virtual void cancel();
	bool createGroupCallback(const LLSD& notification, const LLSD& response);
	
	virtual void update(LLGroupChange gc);
	
	virtual BOOL postBuild();
	
	virtual void draw();

	virtual void setGroupID(const LLUUID& id);

	virtual void setupCtrls	(LLPanel* parent);

	void onNameCache(const LLUUID& update_id, LLGroupMemberData* member, const LLAvatarName& av_name);
private:
	void	reset();

	void	resetDirty();

	static void onFocusEdit(LLFocusableElement* ctrl, void* data);
	static void onCommitAny(LLUICtrl* ctrl, void* data);
	static void onCommitUserOnly(LLUICtrl* ctrl, void* data);
	static void onCommitEnrollment(LLUICtrl* ctrl, void* data);
	static void onClickInfo(void* userdata);
	static void onReceiveNotices(LLUICtrl* ctrl, void* data);
	static void openProfile(void* data);

	S32	 sortMembersList(S32,const LLScrollListItem*,const LLScrollListItem*);
	void addMember(LLGroupMemberData* member);

    static bool joinDlgCB(const LLSD& notification, const LLSD& response);

	void updateMembers();
	void updateChanged();
	bool confirmMatureApply(const LLSD& notification, const LLSD& response);

	BOOL			mPendingMemberUpdate;
	BOOL			mChanged;
	BOOL			mFirstUse;
	std::string		mIncompleteMemberDataStr;

	// Group information (include any updates in updateChanged)
	LLLineEditor		*mGroupNameEditor;
	LLTextBox			*mFounderName;
	LLTextureCtrl		*mInsignia;
	LLTextEditor		*mEditCharter;

	LLNameListCtrl	*mListVisibleMembers;

	// Options (include any updates in updateChanged)
	LLCheckBoxCtrl	*mCtrlShowInGroupList;
	LLCheckBoxCtrl	*mCtrlOpenEnrollment;
	LLCheckBoxCtrl	*mCtrlEnrollmentFee;
	LLSpinCtrl      *mSpinEnrollmentFee;
	LLCheckBoxCtrl	*mCtrlReceiveNotices;
	LLCheckBoxCtrl  *mCtrlListGroup;
	LLTextBox       *mActiveTitleLabel;
	LLComboBox		*mComboActiveTitle;
	LLComboBox		*mComboMature;

	LLGroupMgrGroupData::member_list_t::iterator mMemberProgress;
};

#endif
