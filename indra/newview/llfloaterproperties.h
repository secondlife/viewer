/** 
 * @file llfloaterproperties.h
 * @brief A floater which shows an inventory item's properties.
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

#ifndef LL_LLFLOATERPROPERTIES_H
#define LL_LLFLOATERPROPERTIES_H

#include <map>
#include "llmultifloater.h"
#include "lliconctrl.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLFloaterProperties
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLButton;
class LLCheckBoxCtrl;
class LLInventoryItem;
class LLLineEditor;
class LLRadioGroup;
class LLTextBox;

class LLPropertiesObserver;

class LLFloaterProperties : public LLFloater
{
public:
	LLFloaterProperties(const LLUUID& item_id);
	/*virtual*/ ~LLFloaterProperties();
	
	/*virtual*/ BOOL postBuild();
	/*virtual*/ void onOpen(const LLSD& key);
	void setObjectID(const LLUUID& object_id) { mObjectID = object_id; }

	void dirty() { mDirty = TRUE; }
	void refresh();
	
	static void dirtyAll();
	
protected:
	// ui callbacks
	void onClickCreator();
	void onClickOwner();
	void onCommitName();
	void onCommitDescription();
	void onCommitPermissions();
	void onCommitSaleInfo();
	void onCommitSaleType();
	void updateSaleInfo();

	LLInventoryItem* findItem() const;

	void refreshFromItem(LLInventoryItem* item);
	virtual void draw();

protected:
	// The item id of the inventory item in question.
	LLUUID mItemID;

	// mObjectID will have a value if it is associated with a task in
	// the world, and will be == LLUUID::null if it's in the agent
	// inventory.
	LLUUID mObjectID;

	BOOL mDirty;

	LLPropertiesObserver* mPropertiesObserver;
};

class LLMultiProperties : public LLMultiFloater
{
public:
	LLMultiProperties();
};

#endif // LL_LLFLOATERPROPERTIES_H
