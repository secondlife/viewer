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
 * A floater wrapper for llpanelinventory
 */

#include "llviewerprecompiledheaders.h"

#include "llfloateropenobject.h"

#include "llcachename.h"
#include "llbutton.h"
#include "lltextbox.h"

#include "llalertdialog.h"
#include "llinventorybridge.h"
#include "llfloaterinventory.h"
#include "llinventorymodel.h"
#include "llpanelinventory.h"
#include "llfloaterreg.h"
#include "llselectmgr.h"
#include "lluiconstants.h"
#include "llviewerobject.h"
#include "lluictrlfactory.h"
#include "llviewerwindow.h"


LLFloaterOpenObject::LLFloaterOpenObject(const LLSD& key)
:	LLFloater(key),
	mPanelInventory(NULL),
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
	mPanelInventory = getChild<LLPanelInventory>("object_contents");
	return TRUE;
}

void LLFloaterOpenObject::onOpen(const LLSD& key)
{
	LLObjectSelectionHandle object_selection = LLSelectMgr::getInstance()->getSelection();
	if (object_selection->getRootObjectCount() != 1)
	{
		LLNotifications::instance().add("UnableToViewContentsMoreThanOne");
		closeFloater();
		return;
	}
	if(!(object_selection->getPrimaryObject())) 
	{
		closeFloater();
		return;
	}
	mObjectSelection = LLSelectMgr::getInstance()->getEditSelection();
}
void LLFloaterOpenObject::refresh()
{
	mPanelInventory->refresh();

	std::string name;
	BOOL enabled;

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

	childSetTextArg("object_name", "[DESC]", name);
	childSetEnabled("copy_to_inventory_button", enabled);
	childSetEnabled("copy_and_wear_button", enabled);

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
		LLNotifications::instance().add("OnlyCopyContentsOfSingleItem");
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

		LLNotifications::instance().add("OpenObjectCannotCopy");
	}
}

// static
void LLFloaterOpenObject::callbackMoveInventory(S32 result, void* data)
{
	LLCatAndWear* cat = (LLCatAndWear*)data;

	if (result == 0)
	{
		LLFloaterInventory::showAgentInventory();
		LLFloaterInventory* view = LLFloaterInventory::getActiveInventory();
		if (view)
		{
			view->getPanel()->setSelection(cat->mCatID, TAKE_FOCUS_NO);
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

