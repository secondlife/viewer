/** 
 * @file llsidepaneliteminfo.h
 * @brief A panel which shows an inventory item's properties.
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

protected:
	/*virtual*/ void refresh();
	/*virtual*/ void save();

	LLViewerInventoryItem* findItem() const;
	LLViewerObject*  findObject() const;
	
	void refreshFromItem(LLViewerInventoryItem* item);

private:
	LLUUID mItemID; 	// inventory UUID for the inventory item.
	LLUUID mObjectID; 	// in-world task UUID, or null if in agent inventory.
	LLItemPropertiesObserver* mPropertiesObserver; // for syncing changes to item
	
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
