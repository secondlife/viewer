/** 
 * @file llfloaterbulkpermissions.h
 * @brief Allow multiple task inventory properties to be set in one go.
 * @author Michelle2 Zenovka
 *
 * $LicenseInfo:firstyear=2008&license=viewerlgpl$
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

#ifndef LL_LLBULKPERMISSION_H
#define LL_LLBULKPERMISSION_H

#include "lldarray.h"
#include "llinventory.h"
#include "llviewerobject.h"
#include "llvoinventorylistener.h"
#include "llmap.h"
#include "lluuid.h"

#include "llfloater.h"
#include "llscrolllistctrl.h"

class LLFloaterBulkPermission : public LLFloater, public LLVOInventoryListener
{
	friend class LLFloaterReg;
public:

	BOOL postBuild();

private:
	
	LLFloaterBulkPermission(const LLSD& seed);	
	virtual ~LLFloaterBulkPermission() {}

	BOOL start(); // returns TRUE if the queue has started, otherwise FALSE.
	BOOL nextObject();
	BOOL popNext();

	// This is the callback method for the viewer object currently
	// being worked on.
	/*virtual*/ void inventoryChanged(LLViewerObject* obj,
								 LLInventoryObject::object_list_t* inv,
								 S32 serial_num,
								 void* queue);
	
	// This is called by inventoryChanged
	void handleInventory(LLViewerObject* viewer_obj,
								LLInventoryObject::object_list_t* inv);


	void updateInventory(LLViewerObject* object,
								LLViewerInventoryItem* item,
								U8 key,
								bool is_new);

	void onCloseBtn();
	void onOkBtn();
	void onApplyBtn();
	void onCommitCopy();
	void onCheckAll() { doCheckUncheckAll(TRUE); }
	void onUncheckAll() { doCheckUncheckAll(FALSE); }
	
	// returns true if this is done
	BOOL isDone() const { return (mCurrentObjectID.isNull() || (mObjectIDs.count() == 0)); }

	//Read the settings and Apply the permissions
	void doApply();
	void doCheckUncheckAll(BOOL check);

private:
	// UI
	LLScrollListCtrl* mMessages;
	LLButton* mCloseBtn;

	// Object Queue
	LLDynamicArray<LLUUID> mObjectIDs;
	LLUUID mCurrentObjectID;
	BOOL mDone;

	LLUUID mID;

	const char* mStartString;
};

#endif

