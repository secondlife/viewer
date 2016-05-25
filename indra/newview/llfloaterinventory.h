/** 
 * @file llfloaterinventory.h
 * @brief LLFloaterInventory, LLInventoryFolder, and LLInventoryItem
 * class definition
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#ifndef LL_LLFLOATERINVENTORY_H
#define LL_LLFLOATERINVENTORY_H

#include "llfloater.h"
#include "llfoldertype.h"

class LLInventoryPanel;
class LLPanelMainInventory;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLFloaterInventory
//
// This deals with the buttons and views used to navigate as
// well as controlling the behavior of the overall object.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLFloaterInventory : public LLFloater
{
public:
	LLFloaterInventory(const LLSD& key);
	~LLFloaterInventory();

	BOOL postBuild();

	// This method makes sure that an inventory view exists, is
	// visible, and has focus. The view chosen is returned.
	static LLFloaterInventory* showAgentInventory();

	// Final cleanup, destroy all open inventory views.
	static void cleanup();

	// Inherited functionality
	/*virtual*/ void onOpen(const LLSD& key);
	/*virtual*/ void onClose(bool app_quitting);

	LLInventoryPanel* getPanel();
	LLPanelMainInventory* getMainInventoryPanel() { return mPanelMainInventory;}
private:
	LLPanelMainInventory* mPanelMainInventory;
};

#endif // LL_LLFLOATERINVENTORY_H



