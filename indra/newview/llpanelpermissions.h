/** 
 * @file llpanelpermissions.h
 * @brief LLPanelPermissions class header file
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
class LLViewerInventoryItem;

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

	static LLViewerInventoryItem* findItem(LLUUID &object_id);

protected:
	void disableAll();
	
private:
	LLNameBox*		mLabelGroupName;		// group name

	LLUUID			mCreatorID;
	LLUUID			mOwnerID;
	LLUUID			mLastOwnerID;
};


#endif // LL_LLPANELPERMISSIONS_H
