/** 
 * @file llfloaterinventory.h
 * @brief LLFloaterInventory, LLInventoryFolder, and LLInventoryItem
 * class definition
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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

	LLInventoryPanel* getPanel();
private:
	LLPanelMainInventory* mPanelMainInventory;
};

#endif // LL_LLFLOATERINVENTORY_H



