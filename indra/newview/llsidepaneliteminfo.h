/** 
 * @file llsidepaneliteminfo.h
 * @brief A panel which shows an inventory item's properties.
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

#ifndef LL_LLSIDEPANELITEMINFO_H
#define LL_LLSIDEPANELITEMINFO_H

#include "llsidepanelinventorysubpanel.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLSidepanelItemInfo
// Object properties for inventory side panel.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLButton;
class LLViewerInventoryItem;
class LLItemPropertiesObserver;
class LLObjectInventoryObserver;
class LLViewerObject;
class LLPermissions;

class LLSidepanelItemInfo : public LLSidepanelInventorySubpanel
{
public:
	LLSidepanelItemInfo();
	virtual ~LLSidepanelItemInfo();
	
	/*virtual*/ BOOL postBuild();
	/*virtual*/ void reset();

	void setObjectID(const LLUUID& object_id);
	void setItemID(const LLUUID& item_id);
	void setEditMode(BOOL edit);

	const LLUUID& getObjectID() const;

protected:
	/*virtual*/ void refresh();
	/*virtual*/ void save();

	LLViewerInventoryItem* findItem() const;
	LLViewerObject*  findObject() const;
	
	void refreshFromItem(LLViewerInventoryItem* item);

private:
	void startObjectInventoryObserver();
	void stopObjectInventoryObserver();

	LLUUID mItemID; 	// inventory UUID for the inventory item.
	LLUUID mObjectID; 	// in-world task UUID, or null if in agent inventory.
	LLItemPropertiesObserver* mPropertiesObserver; // for syncing changes to item
	LLObjectInventoryObserver* mObjectInventoryObserver; // for syncing changes to items inside an object
	
	//
	// UI Elements
	// 
protected:
	void 						onClickCreator();
	void 						onClickOwner();
	void 						onCommitName();
	void 						onCommitDescription();
	void 						onCommitPermissions();
	void 						onCommitSaleInfo();
	void 						onCommitSaleType();
	void 						updateSaleInfo();
};

#endif // LL_LLSIDEPANELITEMINFO_H
