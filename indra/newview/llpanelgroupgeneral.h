/** 
 * @file llpanelgroupgeneral.h
 * @brief General information about a group.
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
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
class LLNameBox;
class LLSpinCtrl;

class LLPanelGroupGeneral : public LLPanelGroupTab
{
public:
	LLPanelGroupGeneral(const std::string& name, const LLUUID& group_id);
	virtual ~LLPanelGroupGeneral();

	// LLPanelGroupTab
	static void* createTab(void* data);
	virtual void activate();
	virtual bool needsApply(LLString& mesg);
	virtual bool apply(LLString& mesg);
	virtual void cancel();
	static void createGroupCallback(S32 option, void* user_data);
	
	virtual void update(LLGroupChange gc);
	
	virtual BOOL postBuild();
	
	virtual void draw();

private:
	static void onCommitAny(LLUICtrl* ctrl, void* data);
	static void onCommitTitle(LLUICtrl* ctrl, void* data);
	static void onCommitEnrollment(LLUICtrl* ctrl, void* data);
	static void onClickJoin(void* userdata);
	static void onClickInfo(void* userdata);
	static void onReceiveNotices(LLUICtrl* ctrl, void* data);
	static void openProfile(void* data);

    static void joinDlgCB(S32 which, void *userdata);

	void updateMembers();

	BOOL			mPendingMemberUpdate;
	BOOL			mChanged;
	BOOL			mFirstUse;
	std::string		mIncompleteMemberDataStr;
	std::string		mConfirmGroupCreateStr;
	LLUUID			mDefaultIconID;

	// Group information
	LLLineEditor		*mGroupNameEditor;
	LLTextBox			*mGroupName;
	LLNameBox			*mFounderName;
	LLTextureCtrl		*mInsignia;
	LLTextEditor		*mEditCharter;
	LLLineEditor		*mEditName;
	LLButton			*mBtnJoinGroup;
	LLButton			*mBtnInfo;

	LLNameListCtrl	*mListVisibleMembers;

	// Options
	LLCheckBoxCtrl	*mCtrlShowInGroupList;
	LLCheckBoxCtrl	*mCtrlPublishOnWeb;
	LLCheckBoxCtrl	*mCtrlMature;
	LLCheckBoxCtrl	*mCtrlOpenEnrollment;
	LLCheckBoxCtrl	*mCtrlEnrollmentFee;
	LLSpinCtrl      *mSpinEnrollmentFee;
	LLCheckBoxCtrl	*mCtrlReceiveNotices;
	LLTextBox       *mActiveTitleLabel;
	LLComboBox		*mComboActiveTitle;

	LLGroupMgrGroupData::member_iter mMemberProgress;
};

#endif
