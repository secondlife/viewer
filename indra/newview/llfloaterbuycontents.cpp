/** 
 * @file llfloaterbuycontents.cpp
 * @author James Cook
 * @brief LLFloaterBuyContents class implementation
 *
 * $LicenseInfo:firstyear=2004&license=viewergpl$
 * 
 * Copyright (c) 2004-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

/**
 * Shows the contents of an object and their permissions when you
 * click "Buy..." on an object with "Sell Contents" checked.
 */

#include "llviewerprecompiledheaders.h"

#include "llfloaterbuycontents.h"

#include "llcachename.h"

#include "llagent.h"			// for agent id
#include "llalertdialog.h"
#include "llcheckboxctrl.h"
#include "llinventorymodel.h"	// for gInventory
#include "llinventoryview.h"	// for get_item_icon
#include "llselectmgr.h"
#include "llscrolllistctrl.h"
#include "llviewerobject.h"
#include "llviewerregion.h"
#include "llvieweruictrlfactory.h"
#include "llviewerwindow.h"

LLFloaterBuyContents* LLFloaterBuyContents::sInstance = NULL;

LLFloaterBuyContents::LLFloaterBuyContents()
:	LLFloater("floater_buy_contents", "FloaterBuyContentsRect", "")
{
	gUICtrlFactory->buildFloater(this, "floater_buy_contents.xml");

	childSetAction("cancel_btn", onClickCancel, this);
	childSetAction("buy_btn", onClickBuy, this);

	childDisable("item_list");
	childDisable("buy_btn");
	childDisable("wear_check");

	setDefaultBtn("cancel_btn"); // to avoid accidental buy (SL-43130)
}

LLFloaterBuyContents::~LLFloaterBuyContents()
{
	sInstance = NULL;
}


// static
void LLFloaterBuyContents::show(const LLSaleInfo& sale_info)
{
	LLObjectSelectionHandle selection = gSelectMgr->getSelection();

	if (selection->getRootObjectCount() != 1)
	{
		gViewerWindow->alertXml("BuyContentsOneOnly");
		return;
	}

	// Create a new instance only if needed
	if (sInstance)
	{
		LLScrollListCtrl* list = LLUICtrlFactory::getScrollListByName(sInstance, "item_list");
		if (list) list->deleteAllItems();
	}
	else
	{
		sInstance = new LLFloaterBuyContents();
	}

	sInstance->open(); /*Flawfinder: ignore*/
	sInstance->setFocus(TRUE);
	sInstance->mObjectSelection = gSelectMgr->getEditSelection();

	// Always center the dialog.  User can change the size,
	// but purchases are important and should be center screen.
	// This also avoids problems where the user resizes the application window
	// mid-session and the saved rect is off-center.
	sInstance->center();

	LLUUID owner_id;
	LLString owner_name;
	BOOL owners_identical = gSelectMgr->selectGetOwner(owner_id, owner_name);
	if (!owners_identical)
	{
		gViewerWindow->alertXml("BuyContentsOneOwner");
		return;
	}

	sInstance->mSaleInfo = sale_info;

	// Update the display
	LLSelectNode* node = selection->getFirstRootNode();
	if (!node) return;
	if(node->mPermissions->isGroupOwned())
	{
		char group_name[MAX_STRING];	/*Flawfinder: ignore*/
		gCacheName->getGroupName(owner_id, group_name);
		owner_name.assign(group_name);
	}

	sInstance->childSetTextArg("contains_text", "[NAME]", node->mName);
	sInstance->childSetTextArg("buy_text", "[AMOUNT]", llformat("%d", sale_info.getSalePrice()));
	sInstance->childSetTextArg("buy_text", "[NAME]", owner_name);

	// Must do this after the floater is created, because
	// sometimes the inventory is already there and 
	// the callback is called immediately.
	LLViewerObject* obj = selection->getFirstRootObject();
	sInstance->registerVOInventoryListener(obj,NULL);
	sInstance->requestVOInventory();
}


void LLFloaterBuyContents::inventoryChanged(LLViewerObject* obj,
											InventoryObjectList* inv,
								 S32 serial_num,
								 void* data)
{
	if (!obj)
	{
		llwarns << "No object in LLFloaterBuyContents::inventoryChanged" << llendl;
		return;
	}

	if (!inv)
	{
		llwarns << "No inventory in LLFloaterBuyContents::inventoryChanged"
			<< llendl;
		removeVOInventoryListener();
		return;
	}

	LLCtrlListInterface *item_list = childGetListInterface("item_list");
	if (!item_list)
	{
		removeVOInventoryListener();
		return;
	}

	// default to turning off the buy button.
	childDisable("buy_btn");

	LLUUID owner_id;
	BOOL is_group_owned;
	LLAssetType::EType asset_type;
	LLInventoryType::EType inv_type;
	S32 wearable_count = 0;
	
	InventoryObjectList::const_iterator it = inv->begin();
	InventoryObjectList::const_iterator end = inv->end();

	for ( ; it != end; ++it )
	{
		asset_type = (*it)->getType();

		// Skip folders, so we know we have inventory items only
		if (asset_type == LLAssetType::AT_CATEGORY)
			continue;

		// Skip root folders, so we know we have inventory items only
		if (asset_type == LLAssetType::AT_ROOT_CATEGORY) 
			continue;

		LLInventoryItem* inv_item = (LLInventoryItem*)((LLInventoryObject*)(*it));
		inv_type = inv_item->getInventoryType();

		// Count clothing items for later
		if (LLInventoryType::IT_WEARABLE == inv_type)
		{
			wearable_count++;
		}

		// Skip items the object's owner can't copy (and hence can't sell)
		if (!inv_item->getPermissions().getOwnership(owner_id, is_group_owned))
			continue;

		if (!inv_item->getPermissions().allowCopyBy(owner_id, owner_id))
			continue;

		// Skip items we can't transfer
		if (!inv_item->getPermissions().allowTransferTo(gAgent.getID())) 
			continue;

		// There will be at least one item shown in the display, so go
		// ahead and enable the buy button.
		childEnable("buy_btn");

		// Create the line in the list
		LLSD row;

		BOOL item_is_multi = FALSE;
		if ( inv_item->getFlags() & LLInventoryItem::II_FLAGS_LANDMARK_VISITED )
		{
			item_is_multi = TRUE;
		}

		LLUUID icon_id = get_item_icon_uuid(inv_item->getType(), 
							 inv_item->getInventoryType(),
							 inv_item->getFlags(),
							 item_is_multi);
		row["columns"][0]["column"] = "icon";
		row["columns"][0]["type"] = "icon";
		row["columns"][0]["value"] = icon_id;
		
		// Append the permissions that you will acquire (not the current
		// permissions).
		U32 next_owner_mask = inv_item->getPermissions().getMaskNextOwner();
		LLString text = (*it)->getName();

		// *TODO: Move into shared library function.
		if (!(next_owner_mask & PERM_COPY))
		{
			text.append(" (no copy)");	// XUI:translate
		}
		if (!(next_owner_mask & PERM_MODIFY))
		{
			text.append(" (no modify)");	// XUI:translate
		}
		if (!(next_owner_mask & PERM_TRANSFER))
		{
			text.append(" (no transfer)");	// XUI:translate
		}

		row["columns"][1]["column"] = "text";
		row["columns"][1]["value"] = text;
		row["columns"][1]["font"] = "SANSSERIF";

		item_list->addElement(row);
	}

	if (wearable_count > 0)
	{
		childEnable("wear_check");
		childSetValue("wear_check", LLSD(false) );
	}
	
	removeVOInventoryListener();
}


// static
void LLFloaterBuyContents::onClickBuy(void*)
{
	// Make sure this wasn't selected through other mechanisms 
	// (ie, being the default button and pressing enter.
	if(!sInstance->childIsEnabled("buy_btn"))
	{
		// We shouldn't be enabled.  Just close.
		sInstance->close();
		return;
	}

	// We may want to wear this item
	if (sInstance->childGetValue("wear_check"))
	{
		LLInventoryView::sWearNewClothing = TRUE;
	}

	// Put the items where we put new folders.
	LLUUID category_id;
	category_id = gInventory.findCategoryUUIDForType(LLAssetType::AT_CATEGORY);

	// *NOTE: doesn't work for multiple object buy, which UI does not
	// currently support sale info is used for verification only, if
	// it doesn't match region info then sale is canceled.
	gSelectMgr->sendBuy(gAgent.getID(), category_id, sInstance->mSaleInfo);

	sInstance->close();
}


// static
void LLFloaterBuyContents::onClickCancel(void*)
{
	sInstance->close();
}
