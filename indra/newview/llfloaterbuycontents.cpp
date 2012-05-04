/** 
 * @file llfloaterbuycontents.cpp
 * @author James Cook
 * @brief LLFloaterBuyContents class implementation
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

/**
 * Shows the contents of an object and their permissions when you
 * click "Buy..." on an object with "Sell Contents" checked.
 */

#include "llviewerprecompiledheaders.h"

#include "llfloaterbuycontents.h"

#include "llcachename.h"

#include "llagent.h"			// for agent id
#include "llcheckboxctrl.h"
#include "llinventorydefines.h"
#include "llinventoryfunctions.h"
#include "llinventorymodel.h"	// for gInventory
#include "llfirstuse.h"
#include "llfloaterreg.h"
#include "llfloaterinventory.h"	// for LLInventoryIcon::getIcon
#include "llnotificationsutil.h"
#include "llselectmgr.h"
#include "llscrolllistctrl.h"
#include "llviewerobject.h"
#include "llviewerregion.h"
#include "lluictrlfactory.h"
#include "llviewerwindow.h"

LLFloaterBuyContents::LLFloaterBuyContents(const LLSD& key)
:	LLFloater(key)
{
}

BOOL LLFloaterBuyContents::postBuild()
{

	getChild<LLUICtrl>("cancel_btn")->setCommitCallback( boost::bind(&LLFloaterBuyContents::onClickCancel, this));
	getChild<LLUICtrl>("buy_btn")->setCommitCallback( boost::bind(&LLFloaterBuyContents::onClickBuy, this));

	getChildView("item_list")->setEnabled(FALSE);
	getChildView("buy_btn")->setEnabled(FALSE);
	getChildView("wear_check")->setEnabled(FALSE);

	setDefaultBtn("cancel_btn"); // to avoid accidental buy (SL-43130)

	// Always center the dialog.  User can change the size,
	// but purchases are important and should be center screen.
	// This also avoids problems where the user resizes the application window
	// mid-session and the saved rect is off-center.
	center();
	
	return TRUE;
}

LLFloaterBuyContents::~LLFloaterBuyContents()
{
}


// static
void LLFloaterBuyContents::show(const LLSaleInfo& sale_info)
{
	LLObjectSelectionHandle selection = LLSelectMgr::getInstance()->getSelection();

	if (selection->getRootObjectCount() != 1)
	{
		LLNotificationsUtil::add("BuyContentsOneOnly");
		return;
	}
	
	LLFloaterBuyContents* floater = LLFloaterReg::showTypedInstance<LLFloaterBuyContents>("buy_object_contents");
	if (!floater)
		return;
	
	LLScrollListCtrl* list = floater->getChild<LLScrollListCtrl>("item_list");
	if (list)
		list->deleteAllItems();

	floater->mObjectSelection = LLSelectMgr::getInstance()->getEditSelection();

	LLUUID owner_id;
	std::string owner_name;
	BOOL owners_identical = LLSelectMgr::getInstance()->selectGetOwner(owner_id, owner_name);
	if (!owners_identical)
	{
		LLNotificationsUtil::add("BuyContentsOneOwner");
		return;
	}

	floater->mSaleInfo = sale_info;

	// Update the display
	LLSelectNode* node = selection->getFirstRootNode();
	if (!node) return;
	if(node->mPermissions->isGroupOwned())
	{
		gCacheName->getGroupName(owner_id, owner_name);
	}

	floater->getChild<LLUICtrl>("contains_text")->setTextArg("[NAME]", node->mName);
	floater->getChild<LLUICtrl>("buy_text")->setTextArg("[AMOUNT]", llformat("%d", sale_info.getSalePrice()));
	floater->getChild<LLUICtrl>("buy_text")->setTextArg("[NAME]", owner_name);

	// Must do this after the floater is created, because
	// sometimes the inventory is already there and 
	// the callback is called immediately.
	LLViewerObject* obj = selection->getFirstRootObject();
	floater->registerVOInventoryListener(obj,NULL);
	floater->requestVOInventory();
}


void LLFloaterBuyContents::inventoryChanged(LLViewerObject* obj,
											LLInventoryObject::object_list_t* inv,
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
	getChildView("buy_btn")->setEnabled(FALSE);

	LLUUID owner_id;
	BOOL is_group_owned;
	LLAssetType::EType asset_type;
	LLInventoryType::EType inv_type;
	S32 wearable_count = 0;
	
	LLInventoryObject::object_list_t::const_iterator it = inv->begin();
	LLInventoryObject::object_list_t::const_iterator end = inv->end();

	for ( ; it != end; ++it )
	{
		asset_type = (*it)->getType();

		// Skip folders, so we know we have inventory items only
		if (asset_type == LLAssetType::AT_CATEGORY)
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
		getChildView("buy_btn")->setEnabled(TRUE);

		// Create the line in the list
		LLSD row;

		BOOL item_is_multi = FALSE;
		if ((inv_item->getFlags() & LLInventoryItemFlags::II_FLAGS_LANDMARK_VISITED
		     || inv_item->getFlags() & LLInventoryItemFlags::II_FLAGS_OBJECT_HAS_MULTIPLE_ITEMS)
		     && !(inv_item->getFlags() & LLInventoryItemFlags::II_FLAGS_WEARABLES_MASK))
		{
			item_is_multi = TRUE;
		}

		std::string icon_name = LLInventoryIcon::getIconName(inv_item->getType(), 
								 inv_item->getInventoryType(),
								 inv_item->getFlags(),
								 item_is_multi);
		row["columns"][0]["column"] = "icon";
		row["columns"][0]["type"] = "icon";
		row["columns"][0]["value"] = icon_name;
		
		// Append the permissions that you will acquire (not the current
		// permissions).
		U32 next_owner_mask = inv_item->getPermissions().getMaskNextOwner();
		std::string text = (*it)->getName();

		if (!(next_owner_mask & PERM_COPY))
		{
			text.append(getString("no_copy_text"));
		}
		if (!(next_owner_mask & PERM_MODIFY))
		{
			text.append(getString("no_modify_text"));
		}
		if (!(next_owner_mask & PERM_TRANSFER))
		{
			text.append(getString("no_transfer_text"));
		}
		
		row["columns"][1]["column"] = "text";
		row["columns"][1]["value"] = text;
		row["columns"][1]["font"] = "SANSSERIF";

		item_list->addElement(row);
	}

	if (wearable_count > 0)
	{
		getChildView("wear_check")->setEnabled(TRUE);
		getChild<LLUICtrl>("wear_check")->setValue(LLSD(false) );
	}
	
	removeVOInventoryListener();
}


void LLFloaterBuyContents::onClickBuy()
{
	// Make sure this wasn't selected through other mechanisms 
	// (ie, being the default button and pressing enter.
	if(!getChildView("buy_btn")->getEnabled())
	{
		// We shouldn't be enabled.  Just close.
		closeFloater();
		return;
	}

	// We may want to wear this item
	if (getChild<LLUICtrl>("wear_check")->getValue())
	{
		LLInventoryState::sWearNewClothing = TRUE;
	}

	// Put the items where we put new folders.
	LLUUID category_id;
	category_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_ROOT_INVENTORY);

	// *NOTE: doesn't work for multiple object buy, which UI does not
	// currently support sale info is used for verification only, if
	// it doesn't match region info then sale is canceled.
	LLSelectMgr::getInstance()->sendBuy(gAgent.getID(), category_id, mSaleInfo);

	// NOTE: do this here instead of on receipt of object, since contents are transfered
	// via a generic BulkUpdateInventory message with no way of distinguishing it from
	// other inventory operations
	LLFirstUse::newInventory();
	closeFloater();
}

void LLFloaterBuyContents::onClickCancel()
{
	closeFloater();
}
