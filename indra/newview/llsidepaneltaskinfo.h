/** 
 * @file llsidepaneltaskinfo.h
 * @brief LLSidepanelTaskInfo class header file
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

class LLCheckBoxCtrl;
class LLComboBox;
class LLNameBox;
class LLViewerObject;
class LLTextBase;

class LLSidepanelTaskInfo : public LLSidepanelInventorySubpanel
{
public:
	LLSidepanelTaskInfo();
	virtual ~LLSidepanelTaskInfo();

	/*virtual*/	BOOL postBuild();
	/*virtual*/ void handleVisibilityChange ( BOOL new_visibility );

	void setObjectSelection(LLObjectSelectionHandle selection);

	const LLUUID& getSelectedUUID();
	LLViewerObject* getFirstSelectedObject();

	static LLSidepanelTaskInfo *getActivePanel();
protected:
	/*virtual*/ void refresh();	// refresh all labels as needed
	/*virtual*/ void save();
	/*virtual*/ void updateVerbs();

	void refreshAll(); // ignore current keyboard focus and update all fields

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
	LLPointer<LLViewerObject>	mObject;
	LLObjectSelectionHandle		mObjectSelection;
	static LLSidepanelTaskInfo* sActivePanel;
	
private:
	// Pointers cached here to speed up the "disableAll" function which gets called on idle
	LLUICtrl*	mDAPermModify;
	LLView*		mDACreator;
	LLUICtrl*	mDACreatorName;
	LLView*		mDAOwner;
	LLUICtrl*	mDAOwnerName;
	LLView*		mDAGroup;
	LLUICtrl*	mDAGroupName;
	LLView*		mDAButtonSetGroup;
	LLUICtrl*	mDAObjectName;
	LLView*		mDAName;
	LLView*		mDADescription;
	LLUICtrl*	mDAObjectDescription;
	LLView*		mDAPermissions;
	LLUICtrl*	mDACheckboxShareWithGroup;
	LLView*		mDAButtonDeed;
	LLUICtrl*	mDACheckboxAllowEveryoneMove;
	LLUICtrl*	mDACheckboxAllowEveryoneCopy;
	LLView*		mDANextOwnerCan;
	LLUICtrl*	mDACheckboxNextOwnerCanModify;
	LLUICtrl*	mDACheckboxNextOwnerCanCopy;
	LLUICtrl*	mDACheckboxNextOwnerCanTransfer;
	LLUICtrl*	mDACheckboxForSale;
	LLUICtrl*	mDASearchCheck;
	LLComboBox*	mDAComboSaleType;
	LLUICtrl*	mDACost;
	LLUICtrl*	mDAEditCost;
	LLView*		mDALabelClickAction;
	LLComboBox*	mDAComboClickAction;
	LLTextBase* mDAPathfindingAttributes;
	LLView*		mDAB;
	LLView*		mDAO;
	LLView*		mDAG;
	LLView*		mDAE;
	LLView*		mDAN;
	LLView*		mDAF;
};


#endif // LL_LLSIDEPANELTASKINFO_H
