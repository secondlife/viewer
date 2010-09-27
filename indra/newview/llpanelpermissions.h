/** 
 * @file llpanelpermissions.h
 * @brief LLPanelPermissions class header file
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

#ifndef LL_LLPANELPERMISSIONS_H
#define LL_LLPANELPERMISSIONS_H

#include "llpanel.h"
#include "lluuid.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class llpanelpermissions
//
// Panel for permissions of an object.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLNameBox;

class LLPanelPermissions : public LLPanel
{
public:
	LLPanelPermissions();
	virtual ~LLPanelPermissions();

	/*virtual*/	BOOL	postBuild();

	void refresh();							// refresh all labels as needed

protected:
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

	static void	onCommitClickAction(LLUICtrl* ctrl, void*);
	static void onCommitIncludeInSearch(LLUICtrl* ctrl, void*);

protected:
	void disableAll();
	
private:
	LLNameBox*		mLabelGroupName;		// group name

	LLUUID			mCreatorID;
	LLUUID			mOwnerID;
	LLUUID			mLastOwnerID;
};


#endif // LL_LLPANELPERMISSIONS_H
