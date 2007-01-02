/** 
 * @file llfloateropenobject.cpp
 * @brief LLFloaterOpenObject class implementation
 *
 * Copyright (c) 2004-$CurrentYear$, Linden Research, Inc.
 * $License$
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

#include "llagent.h"			// for agent id
#include "llalertdialog.h"
#include "llinventoryview.h"
#include "llinventorymodel.h"
#include "llpanelinventory.h"
#include "llselectmgr.h"
#include "lluiconstants.h"
#include "llviewerobject.h"
#include "llvieweruictrlfactory.h"
#include "llviewerwindow.h"


LLFloaterOpenObject* LLFloaterOpenObject::sInstance = NULL;

LLFloaterOpenObject::LLFloaterOpenObject()
:	LLFloater("object_contents"),
	mPanelInventory(NULL),
	mDirty(TRUE)
{
	LLCallbackMap::map_t factory_map;
	factory_map["object_contents"] = LLCallbackMap(createPanelInventory, this);
	gUICtrlFactory->buildFloater(this,"floater_openobject.xml",&factory_map);

	childSetAction("copy_to_inventory_button", onClickMoveToInventory, this);
	childSetAction("copy_and_wear_button", onClickMoveAndWear, this);
	childSetTextArg("object_name", "[DESC]", "Object");
}

LLFloaterOpenObject::~LLFloaterOpenObject()
{
	gSelectMgr->deselectAll();
	sInstance = NULL;
}

void LLFloaterOpenObject::refresh()
{
	mPanelInventory->refresh();

	LLSelectNode* node = gSelectMgr->getFirstRootNode();
	if (node)
	{
		std::string name = node->mName;
		childSetTextArg("object_name", "[DESC]", name);
	}
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

// static
void LLFloaterOpenObject::dirty()
{
	if (sInstance) sInstance->mDirty = TRUE;
}

// static
void LLFloaterOpenObject::show()
{
	if (gSelectMgr->getRootObjectCount() != 1)
	{
		gViewerWindow->alertXml("UnableToViewContentsMoreThanOne");
		return;
	}

	// Create a new instance only if needed
	if (!sInstance)
	{
		sInstance = new LLFloaterOpenObject();
		sInstance->center();
	}

	sInstance->open();
	sInstance->setFocus(TRUE);
}


// static
void LLFloaterOpenObject::moveToInventory(bool wear)
{
	if (gSelectMgr->getRootObjectCount() != 1)
	{
		gViewerWindow->alertXml("OnlyCopyContentsOfSingleItem");
		return;
	}

	LLSelectNode* node = gSelectMgr->getFirstRootNode();
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
			LLAssetType::AT_CLOTHING);
	}
	else
	{
		parent_category_id = gAgent.getInventoryRootID();
	}
	LLUUID category_id = gInventory.createNewCategory(parent_category_id, 
		LLAssetType::AT_NONE, 
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

		gViewerWindow->alertXml("OpenObjectCannotCopy");
	}
}

// static
void LLFloaterOpenObject::callbackMoveInventory(S32 result, void* data)
{
	LLCatAndWear* cat = (LLCatAndWear*)data;

	if (result == 0)
	{
		LLInventoryView::showAgentInventory();
		LLInventoryView* view = LLInventoryView::getActiveInventory();
		if (view)
		{
			view->getPanel()->setSelection(cat->mCatID, TAKE_FOCUS_NO);
		}
	}

	delete cat;
}


// static
void LLFloaterOpenObject::onClickMoveToInventory(void* data)
{
	LLFloaterOpenObject* self = (LLFloaterOpenObject*)data;
	moveToInventory(false);
	self->close();
}

// static
void LLFloaterOpenObject::onClickMoveAndWear(void* data)
{
	LLFloaterOpenObject* self = (LLFloaterOpenObject*)data;
	moveToInventory(true);
	self->close();
}

//static
void* LLFloaterOpenObject::createPanelInventory(void* data)
{
	LLFloaterOpenObject* floater = (LLFloaterOpenObject*)data;
	floater->mPanelInventory = new LLPanelInventory("Object Contents", LLRect());
	return floater->mPanelInventory;
}
