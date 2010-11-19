/** 
 * @file llfloateropenobject.cpp
 * @brief LLFloaterOpenObject class implementation
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
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

/*
 * Shows the contents of an object.
 * A floater wrapper for LLPanelObjectInventory
 */

#include "llviewerprecompiledheaders.h"

#include "llfloateropenobject.h"

#include "llcachename.h"
#include "llbutton.h"
#include "llnotificationsutil.h"
#include "lltextbox.h"

#include "llinventorybridge.h"
#include "llfloaterinventory.h"
#include "llinventorymodel.h"
#include "llinventorypanel.h"
#include "llpanelobjectinventory.h"
#include "llfloaterreg.h"
#include "llselectmgr.h"
#include "lluiconstants.h"
#include "llviewerobject.h"
#include "lluictrlfactory.h"
#include "llviewerwindow.h"


LLFloaterOpenObject::LLFloaterOpenObject(const LLSD& key)
:	LLFloater(key),
	mPanelInventoryObject(NULL),
	mDirty(TRUE)
{
	mCommitCallbackRegistrar.add("OpenObject.MoveToInventory",	boost::bind(&LLFloaterOpenObject::onClickMoveToInventory, this));
	mCommitCallbackRegistrar.add("OpenObject.MoveAndWear",		boost::bind(&LLFloaterOpenObject::onClickMoveAndWear, this));
}

LLFloaterOpenObject::~LLFloaterOpenObject()
{
//	sInstance = NULL;
}

// virtual
BOOL LLFloaterOpenObject::postBuild()
{
	getChild<LLUICtrl>("object_name")->setTextArg("[DESC]", std::string("Object") ); // *Note: probably do not want to translate this
	mPanelInventoryObject = getChild<LLPanelObjectInventory>("object_contents");
	
	refresh();
	return TRUE;
}

void LLFloaterOpenObject::onOpen(const LLSD& key)
{
	LLObjectSelectionHandle object_selection = LLSelectMgr::getInstance()->getSelection();
	if (object_selection->getRootObjectCount() != 1)
	{
		LLNotificationsUtil::add("UnableToViewContentsMoreThanOne");
		closeFloater();
		return;
	}
	if(!(object_selection->getPrimaryObject())) 
	{
		closeFloater();
		return;
	}
	mObjectSelection = LLSelectMgr::getInstance()->getEditSelection();
	refresh();
}

void LLFloaterOpenObject::refresh()
{
	mPanelInventoryObject->refresh();

	std::string name = "";
	BOOL enabled = FALSE;

	LLSelectNode* node = mObjectSelection->getFirstRootNode();
	if (node) 
	{
		name = node->mName;
		enabled = TRUE;
	}
	else
	{
		name = "";
		enabled = FALSE;
	}
	
	getChild<LLUICtrl>("object_name")->setTextArg("[DESC]", name);
	getChildView("copy_to_inventory_button")->setEnabled(enabled);
	getChildView("copy_and_wear_button")->setEnabled(enabled);

}

void LLFloaterOpenObject::draw()
{
	if (mDirty)
	{
		refresh();
		mDirty = FALSE;
	}
	LLFloater::draw();
}

void LLFloaterOpenObject::dirty()
{
	mDirty = TRUE;
}



void LLFloaterOpenObject::moveToInventory(bool wear)
{
	if (mObjectSelection->getRootObjectCount() != 1)
	{
		LLNotificationsUtil::add("OnlyCopyContentsOfSingleItem");
		return;
	}

	LLSelectNode* node = mObjectSelection->getFirstRootNode();
	if (!node) return;
	LLViewerObject* object = node->getObject();
	if (!object) return;

	LLUUID object_id = object->getID();
	std::string name = node->mName;

	// Either create a sub-folder of clothing, or of the root folder.
	LLUUID parent_category_id;
	if (wear)
	{
		parent_category_id = gInventory.findCategoryUUIDForType(
			LLFolderType::FT_CLOTHING);
	}
	else
	{
		parent_category_id = gInventory.getRootFolderID();
	}
	LLUUID category_id = gInventory.createNewCategory(parent_category_id, 
		LLFolderType::FT_NONE, 
		name);

	LLCatAndWear* data = new LLCatAndWear;
	data->mCatID = category_id;
	data->mWear = wear;

	// Copy and/or move the items into the newly created folder.
	// Ignore any "you're going to break this item" messages.
	BOOL success = move_inv_category_world_to_agent(object_id, category_id, TRUE,
													callbackMoveInventory, 
													(void*)data);
	if (!success)
	{
		delete data;
		data = NULL;

		LLNotificationsUtil::add("OpenObjectCannotCopy");
	}
}

// static
void LLFloaterOpenObject::callbackMoveInventory(S32 result, void* data)
{
	LLCatAndWear* cat = (LLCatAndWear*)data;

	if (result == 0)
	{
		LLInventoryPanel *active_panel = LLInventoryPanel::getActiveInventoryPanel();
		if (active_panel)
		{
			active_panel->setSelection(cat->mCatID, TAKE_FOCUS_NO);
		}
	}

	delete cat;
}

void LLFloaterOpenObject::onClickMoveToInventory()
{
	moveToInventory(false);
	closeFloater();
}

void LLFloaterOpenObject::onClickMoveAndWear()
{
	moveToInventory(true);
	closeFloater();
}

