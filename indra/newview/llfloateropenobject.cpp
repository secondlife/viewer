/** 
 * @file llfloateropenobject.cpp
 * @brief LLFloaterOpenObject class implementation
 *
 * $LicenseInfo:firstyear=2004&license=viewergpl$
 * 
 * Copyright (c) 2004-2009, Linden Research, Inc.
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
//	LLUICtrlFactory::getInstance()->buildFloater(this,"floater_openobject.xml");
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
	childSetTextArg("object_name", "[DESC]", std::string("Object") ); // *Note: probably do not want to translate this
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
	
	// Enable the copy || copy & wear buttons only if we have something we can copy or copy & wear (respectively).
	bool copy_enabled = false;
	bool wear_enabled = false;

	LLSelectNode* node = mObjectSelection->getFirstRootNode();
	if (node) 
	{
		name = node->mName;
		copy_enabled = true;
		
		LLViewerObject* object = node->getObject();
		if (object)
		{
			// this folder is coming from an object, as there is only one folder in an object, the root,
			// we need to collect the entire contents and handle them as a group
			LLInventoryObject::object_list_t inventory_objects;
			object->getInventoryContents(inventory_objects);
			
			if (!inventory_objects.empty())
			{
				for (LLInventoryObject::object_list_t::iterator it = inventory_objects.begin(); 
					 it != inventory_objects.end(); 
					 ++it)
				{
					LLInventoryItem* item = static_cast<LLInventoryItem*> ((LLInventoryObject*)(*it));
					LLInventoryType::EType type = item->getInventoryType();
					if (type == LLInventoryType::IT_OBJECT 
						|| type == LLInventoryType::IT_ATTACHMENT 
						|| type == LLInventoryType::IT_WEARABLE
						|| type == LLInventoryType::IT_GESTURE)
					{
						wear_enabled = true;
						break;
					}
				}
			}
		}
	}

	childSetTextArg("object_name", "[DESC]", name);
	childSetEnabled("copy_to_inventory_button", copy_enabled);
	childSetEnabled("copy_and_wear_button", wear_enabled);

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

