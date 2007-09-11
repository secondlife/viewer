/** 
 * @file llpanelpermissions.h
 * @brief LLPanelPermissions class header file
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLPANELPERMISSIONS_H
#define LL_LLPANELPERMISSIONS_H

#include "llpanel.h"
#include "lluuid.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class llpanelpermissions
//
// Panel for permissions of an object.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLCheckBoxCtrl;
class LLTextBox;
class LLButton;
class LLLineEditor;
class LLRadioGroup;
class LLComboBox;
class LLNameBox;

class LLPanelPermissions : public LLPanel
{
public:
	LLPanelPermissions(const std::string& title);
	virtual ~LLPanelPermissions();

	virtual	BOOL	postBuild();

	// MANIPULATORS
	void refresh();							// refresh all labels as needed
//	void setPermCheckboxes(U32 mask_on, U32 mask_off, 
//						   LLCheckBoxCtrl* move, LLCheckboxCtrl* edit,
//						   LLCheckBoxCtrl* copy);
protected:
	// statics
	static void onClickClaim(void*);
	static void onClickRelease(void*);
	static void onClickCreator(void*);
	static void onClickOwner(void*);
	static void onClickGroup(void*);
	static void cbGroupID(LLUUID group_id, void* userdata);
	static void onClickDeedToGroup(void*);

	static void onCommitPerm(LLUICtrl *ctrl, void *data, U8 field, U32 perm);

//	static void onCommitGroupMove(LLUICtrl *ctrl, void *data);
//	static void onCommitGroupCopy(LLUICtrl *ctrl, void *data);
//	static void onCommitGroupModify(LLUICtrl *ctrl, void *data);
	static void onCommitGroupShare(LLUICtrl *ctrl, void *data);

	static void onCommitEveryoneMove(LLUICtrl *ctrl, void *data);
	static void onCommitEveryoneCopy(LLUICtrl *ctrl, void *data);
	//static void onCommitEveryoneModify(LLUICtrl *ctrl, void *data);

	static void onCommitNextOwnerModify(LLUICtrl* ctrl, void* data);
	static void onCommitNextOwnerCopy(LLUICtrl* ctrl, void* data);
	static void onCommitNextOwnerTransfer(LLUICtrl* ctrl, void* data);
	
	static void onCommitName(LLUICtrl* ctrl, void* data);
	static void onCommitDesc(LLUICtrl* ctrl, void* data);

	static void onCommitSaleInfo(LLUICtrl* ctrl, void* data);
	static void onCommitSaleType(LLUICtrl* ctrl, void* data);	
	void setAllSaleInfo();

	static void	onCommitClickAction(LLUICtrl* ctrl, void*);
	static void onCommitIncludeInSearch(LLUICtrl* ctrl, void*);

protected:
	LLNameBox*		mLabelGroupName;		// group name

	//LLTextBox*		mBuyerLabel;
	//LLCheckBoxCtrl*	mCheckBuyerModify;
	//LLCheckBoxCtrl*	mCheckBuyerCopy;

	LLUUID			mCreatorID;
	LLUUID			mOwnerID;
	LLUUID			mLastOwnerID;
};


#endif // LL_LLPANELPERMISSIONS_H
