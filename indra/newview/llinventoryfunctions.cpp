/** 
 * @file llinventoryfunctions.cpp
 * @brief Implementation of the inventory view and associated stuff.
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

#include "llviewerprecompiledheaders.h"

#include <utility> // for std::pair<>

#include "llinventoryfunctions.h"

// library includes
#include "llagent.h"
#include "llagentwearables.h"
#include "llcallingcard.h"
#include "llfloaterreg.h"
#include "llinventorydefines.h"
#include "llsdserialize.h"
#include "llfiltereditor.h"
#include "llspinctrl.h"
#include "llui.h"
#include "message.h"

// newview includes
#include "llappearancemgr.h"
#include "llappviewer.h"
#include "llclipboard.h"
#include "lldonotdisturbnotificationstorage.h"
#include "llfloaterinventory.h"
#include "llfloatersidepanelcontainer.h"
#include "llfocusmgr.h"
#include "llfolderview.h"
#include "llgesturemgr.h"
#include "lliconctrl.h"
#include "llimview.h"
#include "llinventorybridge.h"
#include "llinventorymodel.h"
#include "llinventorypanel.h"
#include "lllineeditor.h"
#include "llmarketplacenotifications.h"
#include "llmarketplacefunctions.h"
#include "llmenugl.h"
#include "llnotificationsutil.h"
#include "llpanelmaininventory.h"
#include "llpreviewanim.h"
#include "llpreviewgesture.h"
#include "llpreviewnotecard.h"
#include "llpreviewscript.h"
#include "llpreviewsound.h"
#include "llpreviewtexture.h"
#include "llresmgr.h"
#include "llscrollbar.h"
#include "llscrollcontainer.h"
#include "llselectmgr.h"
#include "llsidepanelinventory.h"
#include "lltabcontainer.h"
#include "lltooldraganddrop.h"
#include "lltrans.h"
#include "lluictrlfactory.h"
#include "llviewermessage.h"
#include "llviewerfoldertype.h"
#include "llviewerobjectlist.h"
#include "llviewerregion.h"
#include "llviewerwindow.h"
#include "llvoavatarself.h"
#include "llwearablelist.h"

#include <boost/foreach.hpp>

BOOL LLInventoryState::sWearNewClothing = FALSE;
LLUUID LLInventoryState::sWearNewClothingTransactionID;

// Helper function : callback to update a folder after inventory action happened in the background
void update_folder_cb(const LLUUID& dest_folder)
{
    LLViewerInventoryCategory* dest_cat = gInventory.getCategory(dest_folder);
    gInventory.updateCategory(dest_cat);
    gInventory.notifyObservers();
}

// Generates a string containing the path to the item specified by
// item_id.
void append_path(const LLUUID& id, std::string& path)
{
	std::string temp;
	const LLInventoryObject* obj = gInventory.getObject(id);
	LLUUID parent_id;
	if(obj) parent_id = obj->getParentUUID();
	std::string forward_slash("/");
	while(obj)
	{
		obj = gInventory.getCategory(parent_id);
		if(obj)
		{
			temp.assign(forward_slash + obj->getName() + temp);
			parent_id = obj->getParentUUID();
		}
	}
	path.append(temp);
}

void update_marketplace_folder_hierarchy(const LLUUID cat_id)
{
    // When changing the marketplace status of a folder, the only thing that needs to happen is
    // for all observers of the folder to, possibly, change the display label of the folder
    // so that's the only thing we change on the update mask.
    gInventory.addChangedMask(LLInventoryObserver::LABEL, cat_id);
    gInventory.notifyObservers();

    // Update all descendent folders down
	LLInventoryModel::cat_array_t* cat_array;
	LLInventoryModel::item_array_t* item_array;
	gInventory.getDirectDescendentsOf(cat_id,cat_array,item_array);
    
    LLInventoryModel::cat_array_t cat_array_copy = *cat_array;
    for (LLInventoryModel::cat_array_t::iterator iter = cat_array_copy.begin(); iter != cat_array_copy.end(); iter++)
    {
        LLInventoryCategory* category = *iter;
        update_marketplace_folder_hierarchy(category->getUUID());
    }
    return;
}

void update_marketplace_category(const LLUUID& cat_id)
{
    // When changing the marketplace status of a folder, we usually have to change the status of all
    // folders in the same listing. This is because the display of each folder is affected by the
    // overall status of the whole listing.
    // Consequently, the only way to correctly update a folder anywhere in the marketplace is to 
    // update the whole listing from its listing root.
    // This is not as bad as it seems as we only update folders, not items, and the folder nesting depth 
    // is limited to 4.
    // We also take care of degenerated cases so we don't update all folders in the inventory by mistake.

    const LLUUID marketplace_listings_uuid = gInventory.findCategoryUUIDForType(LLFolderType::FT_MARKETPLACE_LISTINGS, false);
    // No marketplace -> likely called too early... or
    // Not a descendent of the marketplace listings root and not part of marketplace -> likely called in error then...
    if (marketplace_listings_uuid.isNull() || (!gInventory.isObjectDescendentOf(cat_id, marketplace_listings_uuid) && !LLMarketplaceData::instance().isListed(cat_id) && !LLMarketplaceData::instance().isVersionFolder(cat_id)))
    {
        // In those cases, just do the regular category update
        LLViewerInventoryCategory* cat = gInventory.getCategory(cat_id);
        gInventory.updateCategory(cat);
        gInventory.notifyObservers();
        return;
    }
    
    // Grab marketplace listing data for this folder
    S32 depth = depth_nesting_in_marketplace(cat_id);
    LLUUID listing_uuid = nested_parent_id(cat_id, depth);
    
    // Verify marketplace data consistency for this listing
    if (LLMarketplaceData::instance().isListed(listing_uuid))
    {
        LLUUID version_folder_uuid = LLMarketplaceData::instance().getVersionFolderID(listing_uuid);
        if (version_folder_uuid.notNull() && !gInventory.isObjectDescendentOf(version_folder_uuid, listing_uuid))
        {
            // *TODO : Confirm with Producer that this is what we want to happen in that case!
            llinfos << "Merov : Delisting as the version folder is not under the listing folder anymore!!" << llendl;
            LLMarketplaceData::instance().deleteListing(listing_uuid);
        }
        if (!gInventory.isObjectDescendentOf(listing_uuid, marketplace_listings_uuid))
        {
            // *TODO : Confirm with Producer that this is what we want to happen in that case!
            llinfos << "Merov : Delisting as the listing folder is not under the marketplace folder anymore!!" << llendl;
            LLMarketplaceData::instance().deleteListing(listing_uuid);
        }
    }
    
    // Update all descendents starting from the listing root
    update_marketplace_folder_hierarchy(listing_uuid);

    return;
}

void rename_category(LLInventoryModel* model, const LLUUID& cat_id, const std::string& new_name)
{
	LLViewerInventoryCategory* cat;

	if (!model ||
		!get_is_category_renameable(model, cat_id) ||
		(cat = model->getCategory(cat_id)) == NULL ||
		cat->getName() == new_name)
	{
		return;
	}

	LLPointer<LLViewerInventoryCategory> new_cat = new LLViewerInventoryCategory(cat);
	new_cat->rename(new_name);
	new_cat->updateServer(FALSE);
	model->updateCategory(new_cat);

	model->notifyObservers();
}

void copy_inventory_category(LLInventoryModel* model,
							 LLViewerInventoryCategory* cat,
							 const LLUUID& parent_id,
							 const LLUUID& root_copy_id)
{
	// Create the initial folder
	LLUUID new_cat_uuid = gInventory.createNewCategory(parent_id, LLFolderType::FT_NONE, cat->getName());
	model->notifyObservers();
	
	// We need to exclude the initial root of the copy to avoid recursively copying the copy, etc...
	LLUUID root_id = (root_copy_id.isNull() ? new_cat_uuid : root_copy_id);

	// Get the content of the folder
	LLInventoryModel::cat_array_t* cat_array;
	LLInventoryModel::item_array_t* item_array;
	gInventory.getDirectDescendentsOf(cat->getUUID(),cat_array,item_array);

	// Copy all the items
	LLInventoryModel::item_array_t item_array_copy = *item_array;
	for (LLInventoryModel::item_array_t::iterator iter = item_array_copy.begin(); iter != item_array_copy.end(); iter++)
	{
		LLInventoryItem* item = *iter;
        LLPointer<LLInventoryCallback> cb = new LLBoostFuncInventoryCallback(boost::bind(update_folder_cb, new_cat_uuid));
		copy_inventory_item(
							gAgent.getID(),
							item->getPermissions().getOwner(),
							item->getUUID(),
							new_cat_uuid,
							std::string(),
							cb);
	}
	
	// Copy all the folders
	LLInventoryModel::cat_array_t cat_array_copy = *cat_array;
	for (LLInventoryModel::cat_array_t::iterator iter = cat_array_copy.begin(); iter != cat_array_copy.end(); iter++)
	{
		LLViewerInventoryCategory* category = *iter;
		if (category->getUUID() != root_id)
		{
			copy_inventory_category(model, category, new_cat_uuid, root_id);
		}
	}
}

class LLInventoryCollectAllItems : public LLInventoryCollectFunctor
{
public:
	virtual bool operator()(LLInventoryCategory* cat, LLInventoryItem* item)
	{
		return true;
	}
};

BOOL get_is_parent_to_worn_item(const LLUUID& id)
{
	const LLViewerInventoryCategory* cat = gInventory.getCategory(id);
	if (!cat)
	{
		return FALSE;
	}

	LLInventoryModel::cat_array_t cats;
	LLInventoryModel::item_array_t items;
	LLInventoryCollectAllItems collect_all;
	gInventory.collectDescendentsIf(LLAppearanceMgr::instance().getCOF(), cats, items, LLInventoryModel::EXCLUDE_TRASH, collect_all);

	for (LLInventoryModel::item_array_t::const_iterator it = items.begin(); it != items.end(); ++it)
	{
		const LLViewerInventoryItem * const item = *it;

		llassert(item->getIsLinkType());

		LLUUID linked_id = item->getLinkedUUID();
		const LLViewerInventoryItem * const linked_item = gInventory.getItem(linked_id);

		if (linked_item)
		{
			LLUUID parent_id = linked_item->getParentUUID();

			while (!parent_id.isNull())
			{
				LLInventoryCategory * parent_cat = gInventory.getCategory(parent_id);

				if (cat == parent_cat)
				{
					return TRUE;
				}

				parent_id = parent_cat->getParentUUID();
			}
		}
	}

	return FALSE;
}

BOOL get_is_item_worn(const LLUUID& id)
{
	const LLViewerInventoryItem* item = gInventory.getItem(id);
	if (!item)
		return FALSE;

	// Consider the item as worn if it has links in COF.
	if (LLAppearanceMgr::instance().isLinkInCOF(id))
	{
		return TRUE;
	}

	switch(item->getType())
	{
		case LLAssetType::AT_OBJECT:
		{
			if (isAgentAvatarValid() && gAgentAvatarp->isWearingAttachment(item->getLinkedUUID()))
				return TRUE;
			break;
		}
		case LLAssetType::AT_BODYPART:
		case LLAssetType::AT_CLOTHING:
			if(gAgentWearables.isWearingItem(item->getLinkedUUID()))
				return TRUE;
			break;
		case LLAssetType::AT_GESTURE:
			if (LLGestureMgr::instance().isGestureActive(item->getLinkedUUID()))
				return TRUE;
			break;
		default:
			break;
	}
	return FALSE;
}

BOOL get_can_item_be_worn(const LLUUID& id)
{
	const LLViewerInventoryItem* item = gInventory.getItem(id);
	if (!item)
		return FALSE;

	if (LLAppearanceMgr::isLinkInCOF(item->getLinkedUUID()))
	{
		// an item having links in COF (i.e. a worn item)
		return FALSE;
	}

	if (gInventory.isObjectDescendentOf(id, LLAppearanceMgr::instance().getCOF()))
	{
		// a non-link object in COF (should not normally happen)
		return FALSE;
	}
	
	const LLUUID trash_id = gInventory.findCategoryUUIDForType(
			LLFolderType::FT_TRASH);

	// item can't be worn if base obj in trash, see EXT-7015
	if (gInventory.isObjectDescendentOf(item->getLinkedUUID(),
			trash_id))
	{
		return false;
	}

	switch(item->getType())
	{
		case LLAssetType::AT_OBJECT:
		{
			if (isAgentAvatarValid() && gAgentAvatarp->isWearingAttachment(item->getLinkedUUID()))
			{
				// Already being worn
				return FALSE;
			}
			else
			{
				// Not being worn yet.
				return TRUE;
			}
			break;
		}
		case LLAssetType::AT_BODYPART:
		case LLAssetType::AT_CLOTHING:
			if(gAgentWearables.isWearingItem(item->getLinkedUUID()))
			{
				// Already being worn
				return FALSE;
			}
			else
			{
				// Not being worn yet.
				return TRUE;
			}
			break;
		default:
			break;
	}
	return FALSE;
}

BOOL get_is_item_removable(const LLInventoryModel* model, const LLUUID& id)
{
	if (!model)
	{
		return FALSE;
	}

	// Can't delete an item that's in the library.
	if (!model->isObjectDescendentOf(id, gInventory.getRootFolderID()))
	{
		return FALSE;
	}

	// Disable delete from COF folder; have users explicitly choose "detach/take off",
	// unless the item is not worn but in the COF (i.e. is bugged).
	if (LLAppearanceMgr::instance().getIsProtectedCOFItem(id))
	{
		if (get_is_item_worn(id))
		{
			return FALSE;
		}
	}

	const LLInventoryObject *obj = model->getItem(id);
	if (obj && obj->getIsLinkType())
	{
		return TRUE;
	}
	if (get_is_item_worn(id))
	{
		return FALSE;
	}
	return TRUE;
}

BOOL get_is_category_removable(const LLInventoryModel* model, const LLUUID& id)
{
	// NOTE: This function doesn't check the folder's children.
	// See LLFolderBridge::isItemRemovable for a function that does
	// consider the children.

	if (!model)
	{
		return FALSE;
	}

	if (!model->isObjectDescendentOf(id, gInventory.getRootFolderID()))
	{
		return FALSE;
	}

	if (!isAgentAvatarValid()) return FALSE;

	const LLInventoryCategory* category = model->getCategory(id);
	if (!category)
	{
		return FALSE;
	}

	const LLFolderType::EType folder_type = category->getPreferredType();
	
	if (LLFolderType::lookupIsProtectedType(folder_type))
	{
		return FALSE;
	}

	// Can't delete the outfit that is currently being worn.
	if (folder_type == LLFolderType::FT_OUTFIT)
	{
		const LLViewerInventoryItem *base_outfit_link = LLAppearanceMgr::instance().getBaseOutfitLink();
		if (base_outfit_link && (category == base_outfit_link->getLinkedCategory()))
		{
			return FALSE;
		}
	}

	return TRUE;
}

BOOL get_is_category_renameable(const LLInventoryModel* model, const LLUUID& id)
{
	if (!model)
	{
		return FALSE;
	}

	LLViewerInventoryCategory* cat = model->getCategory(id);

	if (cat && !LLFolderType::lookupIsProtectedType(cat->getPreferredType()) &&
		cat->getOwnerID() == gAgent.getID())
	{
		return TRUE;
	}
	return FALSE;
}

void show_task_item_profile(const LLUUID& item_uuid, const LLUUID& object_id)
{
	LLFloaterSidePanelContainer::showPanel("inventory", LLSD().with("id", item_uuid).with("object", object_id));
}

void show_item_profile(const LLUUID& item_uuid)
{
	LLUUID linked_uuid = gInventory.getLinkedItemID(item_uuid);
	LLFloaterSidePanelContainer::showPanel("inventory", LLSD().with("id", linked_uuid));
}

void show_item_original(const LLUUID& item_uuid)
{
	LLFloater* floater_inventory = LLFloaterReg::getInstance("inventory");
	if (!floater_inventory)
	{
		llwarns << "Could not find My Inventory floater" << llendl;
		return;
	}

	//sidetray inventory panel
	LLSidepanelInventory *sidepanel_inventory =	LLFloaterSidePanelContainer::getPanel<LLSidepanelInventory>("inventory");

	bool do_reset_inventory_filter = !floater_inventory->isInVisibleChain();

	LLInventoryPanel* active_panel = LLInventoryPanel::getActiveInventoryPanel();
	if (!active_panel) 
	{
		//this may happen when there is no floatera and other panel is active in inventory tab

		if	(sidepanel_inventory)
		{
			sidepanel_inventory->showInventoryPanel();
		}
	}
	
	active_panel = LLInventoryPanel::getActiveInventoryPanel();
	if (!active_panel) 
	{
		return;
	}
	active_panel->setSelection(gInventory.getLinkedItemID(item_uuid), TAKE_FOCUS_NO);
	
	if(do_reset_inventory_filter)
	{
		reset_inventory_filter();
	}
}


void reset_inventory_filter()
{
	//inventory floater
	bool floater_inventory_visible = false;

	LLFloaterReg::const_instance_list_t& inst_list = LLFloaterReg::getFloaterList("inventory");
	for (LLFloaterReg::const_instance_list_t::const_iterator iter = inst_list.begin(); iter != inst_list.end(); ++iter)
	{
		LLFloaterInventory* floater_inventory = dynamic_cast<LLFloaterInventory*>(*iter);
		if (floater_inventory)
		{
			LLPanelMainInventory* main_inventory = floater_inventory->getMainInventoryPanel();

			main_inventory->onFilterEdit("");

			if(floater_inventory->getVisible())
			{
				floater_inventory_visible = true;
			}
		}
	}

	if(!floater_inventory_visible)
	{
		LLSidepanelInventory *sidepanel_inventory =	LLFloaterSidePanelContainer::getPanel<LLSidepanelInventory>("inventory");
		if (sidepanel_inventory)
		{
			LLPanelMainInventory* main_inventory = sidepanel_inventory->getMainInventoryPanel();
			if (main_inventory)
			{
				main_inventory->onFilterEdit("");
			}
		}
	}
}

void open_outbox()
{
	LLFloaterReg::showInstance("outbox");
}

// Create a new folder in destFolderId with the same name as the item name and return the uuid of the new folder
// Note: this is used locally in various situation where we need to wrap an item into a special folder
LLUUID create_folder_for_item(LLInventoryItem* item, const LLUUID& destFolderId)
{
	llassert(item);
	llassert(destFolderId.notNull());

	LLUUID created_folder_id = gInventory.createNewCategory(destFolderId, LLFolderType::FT_NONE, item->getName());
	gInventory.notifyObservers();
    
    // *TODO : Create different notifications for the various cases
	LLNotificationsUtil::add("OutboxFolderCreated");

	return created_folder_id;
}

void move_to_outbox_cb_action(const LLSD& payload)
{
	LLViewerInventoryItem * viitem = gInventory.getItem(payload["item_id"].asUUID());
	LLUUID dest_folder_id = payload["dest_folder_id"].asUUID();

	if (viitem)
	{	
		// when moving item directly into outbox create folder with that name
		if (dest_folder_id == gInventory.findCategoryUUIDForType(LLFolderType::FT_OUTBOX, false))
		{
			dest_folder_id = create_folder_for_item(viitem, dest_folder_id);
		}

		LLUUID parent = viitem->getParentUUID();

		gInventory.changeItemParent(
			viitem,
			dest_folder_id,
			false);

		LLUUID top_level_folder = payload["top_level_folder"].asUUID();

		if (top_level_folder != LLUUID::null)
		{
			LLViewerInventoryCategory* category;

			while (parent.notNull())
			{
				LLInventoryModel::cat_array_t* cat_array;
				LLInventoryModel::item_array_t* item_array;
				gInventory.getDirectDescendentsOf(parent,cat_array,item_array);

				LLUUID next_parent;

				category = gInventory.getCategory(parent);

				if (!category) break;

				next_parent = category->getParentUUID();

				if (cat_array->empty() && item_array->empty())
				{
					gInventory.removeCategory(parent);
				}

				if (parent == top_level_folder)
				{
					break;
				}

				parent = next_parent;
			}
		}

		open_outbox();
	}
}

void copy_item_to_outbox(LLInventoryItem* inv_item, LLUUID dest_folder, const LLUUID& top_level_folder, S32 operation_id)
{
	// Collapse links directly to items/folders
	LLViewerInventoryItem * viewer_inv_item = (LLViewerInventoryItem *) inv_item;
	LLViewerInventoryCategory * linked_category = viewer_inv_item->getLinkedCategory();
	if (linked_category != NULL)
	{
		copy_folder_to_outbox(linked_category, dest_folder, top_level_folder, operation_id);
	}
	else
	{
		LLViewerInventoryItem * linked_item = viewer_inv_item->getLinkedItem();
		if (linked_item != NULL)
		{
			inv_item = (LLInventoryItem *) linked_item;
		}
		
		// Check for copy permissions
		if (inv_item->getPermissions().allowOperationBy(PERM_COPY, gAgent.getID(), gAgent.getGroupID()))
		{
			// when moving item directly into outbox create folder with that name
			if (dest_folder == gInventory.findCategoryUUIDForType(LLFolderType::FT_OUTBOX, false))
			{
				dest_folder = create_folder_for_item(inv_item, dest_folder);
			}
			
			copy_inventory_item(gAgent.getID(),
								inv_item->getPermissions().getOwner(),
								inv_item->getUUID(),
								dest_folder,
								inv_item->getName(),
								LLPointer<LLInventoryCallback>(NULL));

			open_outbox();
		}
		else
		{
			LLSD payload;
			payload["item_id"] = inv_item->getUUID();
			payload["dest_folder_id"] = dest_folder;
			payload["top_level_folder"] = top_level_folder;
			payload["operation_id"] = operation_id;
			
			LLMarketplaceInventoryNotifications::addNoCopyNotification(payload, move_to_outbox_cb_action);
		}
	}
}

void move_item_within_outbox(LLInventoryItem* inv_item, LLUUID dest_folder, S32 operation_id)
{
	// when moving item directly into outbox create folder with that name
	if (dest_folder == gInventory.findCategoryUUIDForType(LLFolderType::FT_OUTBOX, false))
	{
		dest_folder = create_folder_for_item(inv_item, dest_folder);
	}
	
	LLViewerInventoryItem * viewer_inv_item = (LLViewerInventoryItem *) inv_item;

	gInventory.changeItemParent(
					   viewer_inv_item,
					   dest_folder,
					   false);
}

void copy_folder_to_outbox(LLInventoryCategory* inv_cat, const LLUUID& dest_folder, const LLUUID& top_level_folder, S32 operation_id)
{
	LLUUID new_folder_id = gInventory.createNewCategory(dest_folder, LLFolderType::FT_NONE, inv_cat->getName());
	gInventory.notifyObservers();

	LLInventoryModel::cat_array_t* cat_array;
	LLInventoryModel::item_array_t* item_array;
	gInventory.getDirectDescendentsOf(inv_cat->getUUID(),cat_array,item_array);

	// copy the vector because otherwise the iterator won't be happy if we delete from it
	LLInventoryModel::item_array_t item_array_copy = *item_array;

	for (LLInventoryModel::item_array_t::iterator iter = item_array_copy.begin(); iter != item_array_copy.end(); iter++)
	{
		LLInventoryItem* item = *iter;
		copy_item_to_outbox(item, new_folder_id, top_level_folder, operation_id);
	}

	LLInventoryModel::cat_array_t cat_array_copy = *cat_array;

	for (LLInventoryModel::cat_array_t::iterator iter = cat_array_copy.begin(); iter != cat_array_copy.end(); iter++)
	{
		LLViewerInventoryCategory* category = *iter;
		copy_folder_to_outbox(category, new_folder_id, top_level_folder, operation_id);
	}

	open_outbox();
}

///----------------------------------------------------------------------------
// Marketplace functions
//
// Handles Copy and Move to or within the Marketplace listings folder.
// Handles creation of stock folders, nesting of listings and version folders,
// permission checking and listings validation.
///----------------------------------------------------------------------------

S32 depth_nesting_in_marketplace(LLUUID cur_uuid)
{
    // Get the marketplace listings root, exit with -1 (i.e. not under the marketplace listings root) if none
    const LLUUID marketplace_listings_uuid = gInventory.findCategoryUUIDForType(LLFolderType::FT_MARKETPLACE_LISTINGS, false);
    if (marketplace_listings_uuid.isNull())
    {
        return -1;
    }
    // If not a descendent of the marketplace listings root, then the nesting depth is -1 by definition
    if (!gInventory.isObjectDescendentOf(cur_uuid, marketplace_listings_uuid))
    {
        return -1;
    }
    
    // Iterate through the parents till we hit the marketplace listings root
    // Note that the marketplace listings root itself will return 0
    S32 depth = 0;
    LLInventoryObject* cur_object = gInventory.getObject(cur_uuid);
    while (cur_uuid != marketplace_listings_uuid)
    {
        depth++;
        cur_uuid = cur_object->getParentUUID();
        cur_object = gInventory.getCategory(cur_uuid);
    }
    return depth;
}

// Returns the UUID of the marketplace listing this object is in
LLUUID nested_parent_id(LLUUID cur_uuid, S32 depth)
{
    LLInventoryObject* cur_object = gInventory.getObject(cur_uuid);
    cur_uuid = (depth < 1 ? LLUUID::null : cur_uuid);
    while (depth > 1)
    {
        depth--;
        cur_uuid = cur_object->getParentUUID();
        cur_object = gInventory.getCategory(cur_uuid);
    }
    return cur_uuid;
}

S32 compute_stock_count(LLUUID cat_uuid)
{
    // Handle the case of the folder being a stock folder immediately
    LLViewerInventoryCategory* cat = gInventory.getCategory(cat_uuid);
    if (!cat)
    {
        // Not a category so no stock count to speak of
        return -1;
    }
    if (cat->getPreferredType() == LLFolderType::FT_MARKETPLACE_STOCK)
    {
        // Note: stock folders are *not* supposed to have nested subfolders so we stop recursion here
        // Note: we *always* give a stock count for stock folders, it's useful even if the listing is unassociated
        return cat->getDescendentCount();
    }

    // Grab marketplace data for this folder
    S32 depth = depth_nesting_in_marketplace(cat_uuid);
    LLUUID listing_uuid = nested_parent_id(cat_uuid, depth);
    if (!LLMarketplaceData::instance().isListed(listing_uuid))
    {
        // If not listed, the notion of stock is meaningless so it won't be computed for any level
        return -1;
    }

    LLUUID version_folder_uuid = LLMarketplaceData::instance().getVersionFolderID(listing_uuid);
    // Handle the case of the first 2 levels : listing and version folders
    if (depth == 1)
    {
        if (version_folder_uuid.notNull())
        {
            // If there is a version folder, the stock value for the listing is the version folder stock
            return compute_stock_count(version_folder_uuid);
        }
    }
    else if (depth == 2)
    {
        if (version_folder_uuid.notNull() && (version_folder_uuid != cat_uuid))
        {
            // If there is a version folder but we're not it, our stock count is meaningless
            return -1;
        }
    }
    
    // In all other cases, the stock count is the min of stock folders count found in the descendents
	LLInventoryModel::cat_array_t* cat_array;
	LLInventoryModel::item_array_t* item_array;
	gInventory.getDirectDescendentsOf(cat_uuid,cat_array,item_array);
    
    // "-1" denotes a folder that doesn't countain any stock folders in its descendents
    S32 curr_count = -1;

    // Note: marketplace listings have a maximum depth nesting of 4
    LLInventoryModel::cat_array_t cat_array_copy = *cat_array;
    for (LLInventoryModel::cat_array_t::iterator iter = cat_array_copy.begin(); iter != cat_array_copy.end(); iter++)
    {
        LLInventoryCategory* category = *iter;
        S32 count = compute_stock_count(category->getUUID());
        if ((curr_count == -1) || ((count != -1) && (count < curr_count)))
        {
            curr_count = count;
        }
    }
    
    return curr_count;
}

void move_item_to_marketplacelistings(LLInventoryItem* inv_item, LLUUID dest_folder, bool copy)
{
    // Get the marketplace listings, exit with error if none
    const LLUUID marketplace_listings_uuid = gInventory.findCategoryUUIDForType(LLFolderType::FT_MARKETPLACE_LISTINGS, false);
    if (marketplace_listings_uuid.isNull())
    {
        llinfos << "Merov : Marketplace error : There is no marketplace listings folder -> move aborted!" << llendl;
        return;
    }

    // We will collapse links into items/folders
	LLViewerInventoryItem * viewer_inv_item = (LLViewerInventoryItem *) inv_item;
	LLViewerInventoryCategory * linked_category = viewer_inv_item->getLinkedCategory();
    
	if (linked_category != NULL)
	{
        // Move the linked folder directly
		move_folder_to_marketplacelistings(linked_category, dest_folder, copy);
	}
	else
	{
        // Grab the linked item if any
		LLViewerInventoryItem * linked_item = viewer_inv_item->getLinkedItem();
        viewer_inv_item = (linked_item != NULL ? linked_item : viewer_inv_item);
    
        // Check that the agent has transfer permission on the item: this is required as a resident cannot
        // put on sale items she cannot transfer. Proceed with move if we have permission.
        if (viewer_inv_item->getPermissions().allowOperationBy(PERM_TRANSFER, gAgent.getID(), gAgent.getGroupID()))
        {
            // When moving an isolated item directly under the marketplace listings root, we create a new folder with that name
            if (dest_folder == marketplace_listings_uuid)
            {
                dest_folder = create_folder_for_item(inv_item, dest_folder);
            }
			LLViewerInventoryCategory* dest_cat = gInventory.getCategory(dest_folder);
            
            // Verify we can have this item in that destination category
            if (!dest_cat->acceptItem(viewer_inv_item))
            {
                llinfos << "Merov : Marketplace error : Cannot move item in that stock folder -> move aborted!" << llendl;
                return;
            }
            
            // When moving a no copy item into a first level listing folder, we create a stock folder for it
            if (!viewer_inv_item->getPermissions().allowOperationBy(PERM_COPY, gAgent.getID(), gAgent.getGroupID()) && (dest_cat->getParentUUID() == marketplace_listings_uuid))
            {
                dest_folder = create_folder_for_item(inv_item, dest_folder);
            }
	
            // Get the parent folder of the moved item : we may have to update it
            LLUUID src_folder = viewer_inv_item->getParentUUID();

            if (copy)
            {
                // Copy the item
                LLPointer<LLInventoryCallback> cb = new LLBoostFuncInventoryCallback(boost::bind(update_folder_cb, dest_folder));
                copy_inventory_item(
                                    gAgent.getID(),
                                    viewer_inv_item->getPermissions().getOwner(),
                                    viewer_inv_item->getUUID(),
                                    dest_folder,
                                    std::string(),
                                    cb);
            }
            else
            {
                // Reparent the item
                gInventory.changeItemParent(viewer_inv_item, dest_folder, false);
            }
            
            // Validate the destination : note that this will run the validation code only on one listing folder at most...
            validate_marketplacelistings(dest_cat);

            // Update the modified folders
            update_marketplace_category(src_folder);
            update_marketplace_category(dest_folder);
        }
        else
        {
            // *TODO : signal an error to the user (UI for this TBD)
            llinfos << "Merov : Marketplace error : User doesn't have the correct permission to put this item on sale -> move aborted!" << llendl;
        }
    }
}

void move_folder_to_marketplacelistings(LLInventoryCategory* inv_cat, const LLUUID& dest_folder, bool copy)
{
    // Check that we have adequate permission on all items being moved. Proceed if we do.
    if (has_correct_permissions_for_sale(inv_cat))
    {
        // Get the destination folder
        LLViewerInventoryCategory* dest_cat = gInventory.getCategory(dest_folder);

        // Check it's not a stock folder
        if (dest_cat->getPreferredType() == LLFolderType::FT_MARKETPLACE_STOCK)
        {
            llinfos << "Merov : Marketplace error : Cannot move folder in stock folder -> move aborted!" << llendl;
            return;
        }
        
        // Get the parent folder of the moved item : we may have to update it
        LLUUID src_folder = inv_cat->getParentUUID();

        LLViewerInventoryCategory * viewer_inv_cat = (LLViewerInventoryCategory *) inv_cat;
        if (copy)
        {
            // Copy the folder
            copy_inventory_category(&gInventory, viewer_inv_cat, dest_folder);
        }
        else
        {
            // Reparent the folder
            gInventory.changeCategoryParent(viewer_inv_cat, dest_folder, false);
        }

        // Check the destination folder recursively for no copy items and promote the including folders if any
        validate_marketplacelistings(dest_cat);

        // Update the modified folders
        update_marketplace_category(src_folder);
        update_marketplace_category(dest_folder);
    }
}

// Returns true if all items within the argument folder are fit for sale, false otherwise
bool has_correct_permissions_for_sale(LLInventoryCategory* cat)
{
	LLInventoryModel::cat_array_t* cat_array;
	LLInventoryModel::item_array_t* item_array;
	gInventory.getDirectDescendentsOf(cat->getUUID(),cat_array,item_array);
    
	LLInventoryModel::item_array_t item_array_copy = *item_array;

	for (LLInventoryModel::item_array_t::iterator iter = item_array_copy.begin(); iter != item_array_copy.end(); iter++)
	{
		LLInventoryItem* item = *iter;
        LLViewerInventoryItem * viewer_inv_item = (LLViewerInventoryItem *) item;
        LLViewerInventoryCategory * linked_category = viewer_inv_item->getLinkedCategory();
		LLViewerInventoryItem * linked_item = viewer_inv_item->getLinkedItem();
        // Linked items and folders cannot be put for sale
        if (linked_category || linked_item)
        {
            llinfos << "Merov : linked items in this folder -> not allowed to sell!" << llendl;
            return false;
        }
        // Check that the agent has transfer permission on the item: this is required as a resident cannot
        // put on sale items she cannot transfer. Proceed with move if we have permission.
        if (!viewer_inv_item->getPermissions().allowOperationBy(PERM_TRANSFER, gAgent.getID(), gAgent.getGroupID()))
        {
            llinfos << "Merov : wrong permissions on items in this folder -> not allowed to sell!" << llendl;
            return false;
        }
	}
    
	LLInventoryModel::cat_array_t cat_array_copy = *cat_array;
    
	for (LLInventoryModel::cat_array_t::iterator iter = cat_array_copy.begin(); iter != cat_array_copy.end(); iter++)
	{
		LLInventoryCategory* category = *iter;
		if (!has_correct_permissions_for_sale(category))
        {
            return false;
        }
	}
    return true;
}

// Make all relevant business logic checks on the marketplace listings starting with the folder as argument
// This function does no deletion of listings but a mere audit and raises issues to the user
// The only thing that's done is to move and sort folders containing no-copy items to stock folders
// *TODO : Signal the errors to the user somewhat (UI still TBD)
// *TODO : Add the rest of the SLM/AIS business logic (limit of nesting depth, stock folder consistency, overall limit on listings, etc...)
void validate_marketplacelistings(LLInventoryCategory* cat)
{
    // Special case a stock folder depth issue
    LLViewerInventoryCategory * viewer_cat = (LLViewerInventoryCategory *) (cat);
	const LLFolderType::EType folder_type = cat->getPreferredType();
    S32 depth = depth_nesting_in_marketplace(cat->getUUID());
    if ((folder_type == LLFolderType::FT_MARKETPLACE_STOCK) && (depth == 1))
    {
        // Nest the stock folder one level deeper in a normal folder and restart from there
        LLUUID parent_uuid = gInventory.findCategoryUUIDForType(LLFolderType::FT_MARKETPLACE_LISTINGS, false);
        LLUUID folder_uuid = gInventory.createNewCategory(parent_uuid, LLFolderType::FT_NONE, cat->getName());
        LLInventoryCategory* new_cat = gInventory.getCategory(folder_uuid);
        gInventory.changeCategoryParent(viewer_cat, folder_uuid, false);
        validate_marketplacelistings(new_cat);
        return;
    }
    
	LLInventoryModel::cat_array_t* cat_array;
	LLInventoryModel::item_array_t* item_array;
	gInventory.getDirectDescendentsOf(cat->getUUID(),cat_array,item_array);
    
    // Stock items : sorting and moving the various stock items is complicated as the set of constraints is high
    // For each folder, we need to:
    // * separate non stock items, stock items per types in different folders
    // * have stock items nested at depth 2 at least
    // * never ever move the non-stock items
    
    std::vector<std::vector<LLViewerInventoryItem*> > items_vector;
    items_vector.resize(LLInventoryType::IT_COUNT+1);

    // Parse the items and create vectors of items to sort copyable items and stock items of various types
	LLInventoryModel::item_array_t item_array_copy = *item_array;
	for (LLInventoryModel::item_array_t::iterator iter = item_array_copy.begin(); iter != item_array_copy.end(); iter++)
	{
		LLInventoryItem* item = *iter;
        LLViewerInventoryItem * viewer_inv_item = (LLViewerInventoryItem *) item;
        LLViewerInventoryCategory * linked_category = viewer_inv_item->getLinkedCategory();
		LLViewerInventoryItem * linked_item = viewer_inv_item->getLinkedItem();
        // Skip items that shouldn't be there to start with, raise an error message for those
        if (linked_category || linked_item)
        {
            llinfos << "Merov : Validation error: skipping linked item : " << viewer_inv_item->getName() << llendl;
            continue;
        }
        if (!viewer_inv_item->getPermissions().allowOperationBy(PERM_TRANSFER, gAgent.getID(), gAgent.getGroupID()))
        {
            llinfos << "Merov : Validation error: skipping item with incorrect permissions : " << viewer_inv_item->getName() << llendl;
            continue;
        }
        // Update the appropriate vector item for that type
        LLInventoryType::EType type = LLInventoryType::IT_COUNT;    // Default value for non stock items
        if (!viewer_inv_item->getPermissions().allowOperationBy(PERM_COPY, gAgent.getID(), gAgent.getGroupID()))
        {
            // Get the item type for stock items
            type = viewer_inv_item->getInventoryType();
        }
        items_vector[type].push_back(viewer_inv_item);
	}
    // How many types of folders? Which type is it if only one?
    S32 count = 0;
    LLInventoryType::EType type = LLInventoryType::IT_COUNT;
    for (S32 i = 0; i <= LLInventoryType::IT_COUNT; i++)
    {
        if (!items_vector[i].empty())
        {
            count++;
            type = (LLInventoryType::EType)(i);
        }
    }
    // If we have one kind only, in the correct folder type at the right depth -> all OK
    if ((count <= 1) && ((type == LLInventoryType::IT_COUNT) || ((folder_type == LLFolderType::FT_MARKETPLACE_STOCK) && (depth >= 2))))
    {
        // Done with that folder!
        llinfos << "Merov : Validation log: folder validates : " << viewer_cat->getName() << llendl;
    }
    else
    {
        // Create one folder per vector of the stock kind at the right depth
        // Note: we *intentionally* skip the non stock items at the end, those should not be moved around
        for (S32 i = 0; i < LLInventoryType::IT_COUNT; i++)
        {
            if (!items_vector[i].empty())
            {
                // Create a new folder
                llinfos << "Merov : Validation log: creating stock folder : " << viewer_cat->getName() << ", type = " << i << llendl;
                LLUUID parent_uuid = (depth >=2 ? viewer_cat->getParentUUID() : viewer_cat->getUUID());
                LLUUID folder_uuid = gInventory.createNewCategory(parent_uuid, LLFolderType::FT_MARKETPLACE_STOCK, viewer_cat->getName());
                // Move each item to the new folder
                while (!items_vector[i].empty())
                {
                    LLViewerInventoryItem* viewer_inv_item = items_vector[i].back();
                    llinfos << "Merov : Validation log: moving item : " << viewer_inv_item->getName() << llendl;
                    gInventory.changeItemParent(viewer_inv_item, folder_uuid, false);
                    items_vector[i].pop_back();
                }
                update_marketplace_category(folder_uuid);
            }
        }
        // Clean up
        if (viewer_cat->getDescendentCount() == 0)
        {
            // Remove the current folder if it ends up empty
            llinfos << "Merov : Validation warning : folder content completely moved to stock folder -> remove empty folder!" << llendl;
            gInventory.removeCategory(cat->getUUID());
            gInventory.notifyObservers();
            return;
        }
        else
        {
            // Update the current folder
            update_marketplace_category(cat->getUUID());
        }
    }

    // Recursion : Perform the same validation on each nested folder
	LLInventoryModel::cat_array_t cat_array_copy = *cat_array;
    
	for (LLInventoryModel::cat_array_t::iterator iter = cat_array_copy.begin(); iter != cat_array_copy.end(); iter++)
	{
		LLInventoryCategory* category = *iter;
		validate_marketplacelistings(category);
	}
}

///----------------------------------------------------------------------------
/// LLInventoryCollectFunctor implementations
///----------------------------------------------------------------------------

// static
bool LLInventoryCollectFunctor::itemTransferCommonlyAllowed(const LLInventoryItem* item)
{
	if (!item)
		return false;

	switch(item->getType())
	{
		case LLAssetType::AT_OBJECT:
		case LLAssetType::AT_BODYPART:
		case LLAssetType::AT_CLOTHING:
			if (!get_is_item_worn(item->getUUID()))
				return true;
			break;
		default:
			return true;
			break;
	}
	return false;
}

bool LLIsType::operator()(LLInventoryCategory* cat, LLInventoryItem* item)
{
	if(mType == LLAssetType::AT_CATEGORY)
	{
		if(cat) return TRUE;
	}
	if(item)
	{
		if(item->getType() == mType) return TRUE;
	}
	return FALSE;
}

bool LLIsNotType::operator()(LLInventoryCategory* cat, LLInventoryItem* item)
{
	if(mType == LLAssetType::AT_CATEGORY)
	{
		if(cat) return FALSE;
	}
	if(item)
	{
		if(item->getType() == mType) return FALSE;
		else return TRUE;
	}
	return TRUE;
}

bool LLIsOfAssetType::operator()(LLInventoryCategory* cat, LLInventoryItem* item)
{
	if(mType == LLAssetType::AT_CATEGORY)
	{
		if(cat) return TRUE;
	}
	if(item)
	{
		if(item->getActualType() == mType) return TRUE;
	}
	return FALSE;
}

bool LLIsTypeWithPermissions::operator()(LLInventoryCategory* cat, LLInventoryItem* item)
{
	if(mType == LLAssetType::AT_CATEGORY)
	{
		if(cat) 
		{
			return TRUE;
		}
	}
	if(item)
	{
		if(item->getType() == mType)
		{
			LLPermissions perm = item->getPermissions();
			if ((perm.getMaskBase() & mPerm) == mPerm)
			{
				return TRUE;
			}
		}
	}
	return FALSE;
}

bool LLBuddyCollector::operator()(LLInventoryCategory* cat,
								  LLInventoryItem* item)
{
	if(item)
	{
		if((LLAssetType::AT_CALLINGCARD == item->getType())
		   && (!item->getCreatorUUID().isNull())
		   && (item->getCreatorUUID() != gAgent.getID()))
		{
			return true;
		}
	}
	return false;
}


bool LLUniqueBuddyCollector::operator()(LLInventoryCategory* cat,
										LLInventoryItem* item)
{
	if(item)
	{
		if((LLAssetType::AT_CALLINGCARD == item->getType())
 		   && (item->getCreatorUUID().notNull())
 		   && (item->getCreatorUUID() != gAgent.getID()))
		{
			mSeen.insert(item->getCreatorUUID());
			return true;
		}
	}
	return false;
}


bool LLParticularBuddyCollector::operator()(LLInventoryCategory* cat,
											LLInventoryItem* item)
{
	if(item)
	{
		if((LLAssetType::AT_CALLINGCARD == item->getType())
		   && (item->getCreatorUUID() == mBuddyID))
		{
			return TRUE;
		}
	}
	return FALSE;
}


bool LLNameCategoryCollector::operator()(
	LLInventoryCategory* cat, LLInventoryItem* item)
{
	if(cat)
	{
		if (!LLStringUtil::compareInsensitive(mName, cat->getName()))
		{
			return true;
		}
	}
	return false;
}

bool LLFindCOFValidItems::operator()(LLInventoryCategory* cat,
									 LLInventoryItem* item)
{
	// Valid COF items are:
	// - links to wearables (body parts or clothing)
	// - links to attachments
	// - links to gestures
	// - links to ensemble folders
	LLViewerInventoryItem *linked_item = ((LLViewerInventoryItem*)item)->getLinkedItem();
	if (linked_item)
	{
		LLAssetType::EType type = linked_item->getType();
		return (type == LLAssetType::AT_CLOTHING ||
				type == LLAssetType::AT_BODYPART ||
				type == LLAssetType::AT_GESTURE ||
				type == LLAssetType::AT_OBJECT);
	}
	else
	{
		LLViewerInventoryCategory *linked_category = ((LLViewerInventoryItem*)item)->getLinkedCategory();
		// BAP remove AT_NONE support after ensembles are fully working?
		return (linked_category &&
				((linked_category->getPreferredType() == LLFolderType::FT_NONE) ||
				 (LLFolderType::lookupIsEnsembleType(linked_category->getPreferredType()))));
	}
}

bool LLFindWearables::operator()(LLInventoryCategory* cat,
								 LLInventoryItem* item)
{
	if(item)
	{
		if((item->getType() == LLAssetType::AT_CLOTHING)
		   || (item->getType() == LLAssetType::AT_BODYPART))
		{
			return TRUE;
		}
	}
	return FALSE;
}

LLFindWearablesEx::LLFindWearablesEx(bool is_worn, bool include_body_parts)
:	mIsWorn(is_worn)
,	mIncludeBodyParts(include_body_parts)
{}

bool LLFindWearablesEx::operator()(LLInventoryCategory* cat, LLInventoryItem* item)
{
	LLViewerInventoryItem *vitem = dynamic_cast<LLViewerInventoryItem*>(item);
	if (!vitem) return false;

	// Skip non-wearables.
	if (!vitem->isWearableType() && vitem->getType() != LLAssetType::AT_OBJECT)
	{
		return false;
	}

	// Skip body parts if requested.
	if (!mIncludeBodyParts && vitem->getType() == LLAssetType::AT_BODYPART)
	{
		return false;
	}

	// Skip broken links.
	if (vitem->getIsBrokenLink())
	{
		return false;
	}

	return (bool) get_is_item_worn(item->getUUID()) == mIsWorn;
}

bool LLFindWearablesOfType::operator()(LLInventoryCategory* cat, LLInventoryItem* item)
{
	if (!item) return false;
	if (item->getType() != LLAssetType::AT_CLOTHING &&
		item->getType() != LLAssetType::AT_BODYPART)
	{
		return false;
	}

	LLViewerInventoryItem *vitem = dynamic_cast<LLViewerInventoryItem*>(item);
	if (!vitem || vitem->getWearableType() != mWearableType) return false;

	return true;
}

void LLFindWearablesOfType::setType(LLWearableType::EType type)
{
	mWearableType = type;
}

bool LLFindNonRemovableObjects::operator()(LLInventoryCategory* cat, LLInventoryItem* item)
{
	if (item)
	{
		return !get_is_item_removable(&gInventory, item->getUUID());
	}
	if (cat)
	{
		return !get_is_category_removable(&gInventory, cat->getUUID());
	}

	llwarns << "Not a category and not an item?" << llendl;
	return false;
}

///----------------------------------------------------------------------------
/// LLAssetIDMatches 
///----------------------------------------------------------------------------
bool LLAssetIDMatches::operator()(LLInventoryCategory* cat, LLInventoryItem* item)
{
	return (item && item->getAssetUUID() == mAssetID);
}

///----------------------------------------------------------------------------
/// LLLinkedItemIDMatches 
///----------------------------------------------------------------------------
bool LLLinkedItemIDMatches::operator()(LLInventoryCategory* cat, LLInventoryItem* item)
{
	return (item && 
			(item->getIsLinkType()) &&
			(item->getLinkedUUID() == mBaseItemID)); // A linked item's assetID will be the compared-to item's itemID.
}

void LLSaveFolderState::setApply(BOOL apply)
{
	mApply = apply; 
	// before generating new list of open folders, clear the old one
	if(!apply) 
	{
		clearOpenFolders(); 
	}
}

void LLSaveFolderState::doFolder(LLFolderViewFolder* folder)
{
	LLInvFVBridge* bridge = (LLInvFVBridge*)folder->getViewModelItem();
	if(!bridge) return;
	
	if(mApply)
	{
		// we're applying the open state
		LLUUID id(bridge->getUUID());
		if(mOpenFolders.find(id) != mOpenFolders.end())
		{
			if (!folder->isOpen())
			{
				folder->setOpen(TRUE);
			}
		}
		else
		{
			// keep selected filter in its current state, this is less jarring to user
			if (!folder->isSelected() && folder->isOpen())
			{
				folder->setOpen(FALSE);
			}
		}
	}
	else
	{
		// we're recording state at this point
		if(folder->isOpen())
		{
			mOpenFolders.insert(bridge->getUUID());
		}
	}
}

void LLOpenFilteredFolders::doItem(LLFolderViewItem *item)
{
	if (item->passedFilter())
	{
		item->getParentFolder()->setOpenArrangeRecursively(TRUE, LLFolderViewFolder::RECURSE_UP);
	}
}

void LLOpenFilteredFolders::doFolder(LLFolderViewFolder* folder)
{
	if (folder->LLFolderViewItem::passedFilter() && folder->getParentFolder())
	{
		folder->getParentFolder()->setOpenArrangeRecursively(TRUE, LLFolderViewFolder::RECURSE_UP);
	}
	// if this folder didn't pass the filter, and none of its descendants did
	else if (!folder->getViewModelItem()->passedFilter() && !folder->getViewModelItem()->descendantsPassedFilter())
	{
		folder->setOpenArrangeRecursively(FALSE, LLFolderViewFolder::RECURSE_NO);
	}
}

void LLSelectFirstFilteredItem::doItem(LLFolderViewItem *item)
{
	if (item->passedFilter() && !mItemSelected)
	{
		item->getRoot()->setSelection(item, FALSE, FALSE);
		if (item->getParentFolder())
		{
			item->getParentFolder()->setOpenArrangeRecursively(TRUE, LLFolderViewFolder::RECURSE_UP);
		}
		mItemSelected = TRUE;
	}
}

void LLSelectFirstFilteredItem::doFolder(LLFolderViewFolder* folder)
{
	// Skip if folder or item already found, if not filtered or if no parent (root folder is not selectable)
	if (!mFolderSelected && !mItemSelected && folder->LLFolderViewItem::passedFilter() && folder->getParentFolder())
	{
		folder->getRoot()->setSelection(folder, FALSE, FALSE);
		folder->getParentFolder()->setOpenArrangeRecursively(TRUE, LLFolderViewFolder::RECURSE_UP);
		mFolderSelected = TRUE;
	}
}

void LLOpenFoldersWithSelection::doItem(LLFolderViewItem *item)
{
	if (item->getParentFolder() && item->isSelected())
	{
		item->getParentFolder()->setOpenArrangeRecursively(TRUE, LLFolderViewFolder::RECURSE_UP);
	}
}

void LLOpenFoldersWithSelection::doFolder(LLFolderViewFolder* folder)
{
	if (folder->getParentFolder() && folder->isSelected())
	{
		folder->getParentFolder()->setOpenArrangeRecursively(TRUE, LLFolderViewFolder::RECURSE_UP);
	}
}

void LLInventoryAction::doToSelected(LLInventoryModel* model, LLFolderView* root, const std::string& action)
{
	if ("rename" == action)
	{
		root->startRenamingSelectedItem();
		return;
	}
	if ("delete" == action)
	{
		LLSD args;
		args["QUESTION"] = LLTrans::getString(root->getSelectedCount() > 1 ? "DeleteItems" :  "DeleteItem");
		LLNotificationsUtil::add("DeleteItems", args, LLSD(), boost::bind(&LLInventoryAction::onItemsRemovalConfirmation, _1, _2, root));
		return;
	}
	if (("copy" == action) || ("cut" == action))
	{	
		// Clear the clipboard before we start adding things on it
		LLClipboard::instance().reset();
	}

	static const std::string change_folder_string = "change_folder_type_";
	if (action.length() > change_folder_string.length() && 
		(action.compare(0,change_folder_string.length(),"change_folder_type_") == 0))
	{
		LLFolderType::EType new_folder_type = LLViewerFolderType::lookupTypeFromXUIName(action.substr(change_folder_string.length()));
		LLFolderViewModelItemInventory* inventory_item = static_cast<LLFolderViewModelItemInventory*>(root->getViewModelItem());
		LLViewerInventoryCategory *cat = model->getCategory(inventory_item->getUUID());
		if (!cat) return;
		cat->changeType(new_folder_type);
		return;
	}


	std::set<LLFolderViewItem*> selected_items = root->getSelectionList();

	LLMultiPreview* multi_previewp = NULL;
	LLMultiProperties* multi_propertiesp = NULL;

	if (("task_open" == action  || "open" == action) && selected_items.size() > 1)
	{
		multi_previewp = new LLMultiPreview();
		gFloaterView->addChild(multi_previewp);

		LLFloater::setFloaterHost(multi_previewp);

	}
	else if (("task_properties" == action || "properties" == action) && selected_items.size() > 1)
	{
		multi_propertiesp = new LLMultiProperties();
		gFloaterView->addChild(multi_propertiesp);

		LLFloater::setFloaterHost(multi_propertiesp);
	}

	std::set<LLFolderViewItem*>::iterator set_iter;

	for (set_iter = selected_items.begin(); set_iter != selected_items.end(); ++set_iter)
	{
		LLFolderViewItem* folder_item = *set_iter;
		if(!folder_item) continue;
		LLInvFVBridge* bridge = (LLInvFVBridge*)folder_item->getViewModelItem();
		if(!bridge) continue;
		bridge->performAction(model, action);
	}

	LLFloater::setFloaterHost(NULL);
	if (multi_previewp)
	{
		multi_previewp->openFloater(LLSD());
	}
	else if (multi_propertiesp)
	{
		multi_propertiesp->openFloater(LLSD());
	}
}

void LLInventoryAction::removeItemFromDND(LLFolderView* root)
{
    if(gAgent.isDoNotDisturb())
    {
        //Get selected items
        LLFolderView::selected_items_t selectedItems = root->getSelectedItems();
        LLFolderViewModelItemInventory * viewModel = NULL;

        //If user is in DND and deletes item, make sure the notification is not displayed by removing the notification
        //from DND history and .xml file. Once this is done, upon exit of DND mode the item deleted will not show a notification.
        for(LLFolderView::selected_items_t::iterator it = selectedItems.begin(); it != selectedItems.end(); ++it)
        {
            viewModel = dynamic_cast<LLFolderViewModelItemInventory *>((*it)->getViewModelItem());

            if(viewModel && viewModel->getUUID().notNull())
            {
                //Will remove the item offer notification
                LLDoNotDisturbNotificationStorage::instance().removeNotification(LLDoNotDisturbNotificationStorage::offerName, viewModel->getUUID());
            }
        }
    }
}

void LLInventoryAction::onItemsRemovalConfirmation( const LLSD& notification, const LLSD& response, LLFolderView* root )
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if (option == 0)
	{
        //Need to remove item from DND before item is removed from root folder view
        //because once removed from root folder view the item is no longer a selected item
        removeItemFromDND(root);
		root->removeSelectedItems();
	}
}
