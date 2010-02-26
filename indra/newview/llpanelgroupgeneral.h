/** 
 * @file llpanelgroupgeneral.h
 * @brief General information about a group.
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

private:
	void	reset();

	void	resetDirty();

	static void onFocusEdit(LLFocusableElement* ctrl, void* data);
	static void onCommitAny(LLUICtrl* ctrl, void* data);
	static void onCommitUserOnly(LLUICtrl* ctrl, void* data);
	static void onCommitTitle(LLUICtrl* ctrl, void* data);
	static void onCommitEnrollment(LLUICtrl* ctrl, void* data);
	static void onClickInfo(void* userdata);
	static void onReceiveNotices(LLUICtrl* ctrl, void* data);
	static void openProfile(void* data);

	S32		sortMembersList(S32,const LLScrollListItem*,const LLScrollListItem*);

    static bool joinDlgCB(const LLSD& notification, const LLSD& response);

	void updateMembers();
	void updateChanged();
	bool confirmMatureApply(const LLSD& notification, const LLSD& response);

	BOOL			mPendingMemberUpdate;
	BOOL			mChanged;
	BOOL			mFirstUse;
	std::string		mIncompleteMemberDataStr;
	LLUUID			mDefaultIconID;

	// Group information (include any updates in updateChanged)
	LLLineEditor		*mGroupNameEditor;
	LLNameBox			*mFounderName;
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
