/** 
 * @file llsidepaneltaskinfo.h
 * @brief LLSidepanelTaskInfo class header file
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
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

#ifndef LL_LLSIDEPANELTASKINFO_H
#define LL_LLSIDEPANELTASKINFO_H

#include "llsidepanelinventorysubpanel.h"
#include "lluuid.h"
#include "llselectmgr.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLSidepanelTaskInfo
//
// Panel for permissions of an object.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLNameBox;
class LLCheckBoxCtrl;
class LLViewerObject;

class LLSidepanelTaskInfo : public LLSidepanelInventorySubpanel
{
public:
	LLSidepanelTaskInfo();
	virtual ~LLSidepanelTaskInfo();

	/*virtual*/	BOOL postBuild();
	/*virtual*/ void setVisible(BOOL visible);

	void setObjectSelection(LLObjectSelectionHandle selection);

	const LLUUID& getSelectedUUID();
	LLViewerObject* getFirstSelectedObject();

	static LLSidepanelTaskInfo *getActivePanel();
protected:
	/*virtual*/ void refresh();	// refresh all labels as needed
	/*virtual*/ void save();
	/*virtual*/ void updateVerbs();

	// statics
	static void onClickClaim(void*);
	static void onClickRelease(void*);
		   void onClickGroup();
		   void cbGroupID(LLUUID group_id);
	static void onClickDeedToGroup(void*);

	static void onCommitPerm(LLUICtrl *ctrl, void *data, U8 field, U32 perm);

	static void onCommitGroupShare(LLUICtrl *ctrl, void *data);

	static void onCommitEveryoneMove(LLUICtrl *ctrl, void *data);
	static void onCommitEveryoneCopy(LLUICtrl *ctrl, void *data);

	static void onCommitNextOwnerModify(LLUICtrl* ctrl, void* data);
	static void onCommitNextOwnerCopy(LLUICtrl* ctrl, void* data);
	static void onCommitNextOwnerTransfer(LLUICtrl* ctrl, void* data);
	
	static void onCommitName(LLUICtrl* ctrl, void* data);
	static void onCommitDesc(LLUICtrl* ctrl, void* data);

	static void onCommitSaleInfo(LLUICtrl* ctrl, void* data);
	static void onCommitSaleType(LLUICtrl* ctrl, void* data);	
	void setAllSaleInfo();

	static void	onCommitClickAction(LLUICtrl* ctrl, void* data);
	static void onCommitIncludeInSearch(LLUICtrl* ctrl, void*);

	static void	doClickAction(U8 click_action);
	void disableAll();

private:
	LLNameBox*		mLabelGroupName;		// group name

	LLUUID			mCreatorID;
	LLUUID			mOwnerID;
	LLUUID			mLastOwnerID;

protected:
	void 						onOpenButtonClicked();
	void 						onPayButtonClicked();
	void 						onBuyButtonClicked();
	void 						onDetailsButtonClicked();
private:
	LLButton*					mOpenBtn;
	LLButton*					mPayBtn;
	LLButton*					mBuyBtn;
	LLButton*					mDetailsBtn;

protected:
	LLViewerObject*				getObject();
private:
	LLViewerObject*				mObject;
	LLObjectSelectionHandle mObjectSelection;
	static LLSidepanelTaskInfo* sActivePanel;
};


#endif // LL_LLSIDEPANELTASKINFO_H
