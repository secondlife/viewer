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

void update_marketplace_category(const LLUUID& cur_uuid, bool perform_consistency_enforcement)
{
    // When changing the marketplace status of an item, we usually have to change the status of all
    // folders in the same listing. This is because the display of each folder is affected by the
    // overall status of the whole listing.
    // Consequently, the only way to correctly update an item anywhere in the marketplace is to 
    // update the whole listing from its listing root.
    // This is not as bad as it seems as we only update folders, not items, and the folder nesting depth 
    // is limited to 4.
    // We also take care of degenerated cases so we don't update all folders in the inventory by mistake.

    // Grab marketplace listing data for this item
    S32 depth = depth_nesting_in_marketplace(cur_uuid);
    if (depth > 0)
    {
        // Retrieve the listing uuid this object is in
        LLUUID listing_uuid = nested_parent_id(cur_uuid, depth);
    
        // Verify marketplace data consistency for this listing
        if (perform_consistency_enforcement && LLMarketplaceData::instance().isListed(listing_uuid))
        {
            LLUUID version_folder_uuid = LLMarketplaceData::instance().getVersionFolder(listing_uuid);
            S32 version_depth = depth_nesting_in_marketplace(version_folder_uuid);
            if (version_folder_uuid.notNull() && (!gInventory.isObjectDescendentOf(version_folder_uuid, listing_uuid) || (version_depth != 2)))
            {
                LL_INFOS("SLM") << "Unlist and clear version folder as the version folder is not at the right place anymore!!" << LL_ENDL;
                LLMarketplaceData::instance().setVersionFolder(listing_uuid, LLUUID::null);
            }
        }
    
        // Update all descendents starting from the listing root
        update_marketplace_folder_hierarchy(listing_uuid);
    }
    else if (depth < 0)
    {
        if (perform_consistency_enforcement && LLMarketplaceData::instance().isListed(cur_uuid))
        {
            LL_INFOS("SLM") << "Disassociate as the listing folder is not under the marketplace folder anymore!!" << LL_ENDL;
            LLMarketplaceData::instance().clearListing(cur_uuid);
        }
    }

    return;
}

// Iterate through the marketplace and flag for label change all categories that countain a stock folder (i.e. stock folders and embedding folders up the hierarchy)
void update_all_marketplace_count(const LLUUID& cat_id)
{
    // Get all descendent folders down
	LLInventoryModel::cat_array_t* cat_array;
	LLInventoryModel::item_array_t* item_array;
	gInventory.getDirectDescendentsOf(cat_id,cat_array,item_array);
    
    LLInventoryModel::cat_array_t cat_array_copy = *cat_array;
    for (LLInventoryModel::cat_array_t::iterator iter = cat_array_copy.begin(); iter != cat_array_copy.end(); iter++)
    {
        LLInventoryCategory* category = *iter;
        if (category->getPreferredType() == LLFolderType::FT_MARKETPLACE_STOCK)
        {
            // Listing containing stock folders needs to be updated but not others
            // Note: we take advantage of the fact that stock folder *do not* contain sub folders to avoid a recursive call here
            update_marketplace_category(category->getUUID());
        }
        else
        {
            // Explore the contained folders recursively
            update_all_marketplace_count(category->getUUID());
        }
    }
}

void update_all_marketplace_count()
{
    // Get the marketplace root and launch the recursive exploration
    const LLUUID marketplace_listings_uuid = gInventory.findCategoryUUIDForType(LLFolderType::FT_MARKETPLACE_LISTINGS, false);
    if (!marketplace_listings_uuid.isNull())
    {
        update_all_marketplace_count(marketplace_listings_uuid);
    }
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

	LLSD updates;
	updates["name"] = new_name;
	update_inventory_category(cat_id, updates, NULL);
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
		LL_WARNS() << "Could not find My Inventory floater" << LL_ENDL;
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

    LLUUID version_folder_uuid = LLMarketplaceData::instance().getVersionFolder(listing_uuid);
    // Handle the case of the first 2 levels : listing and version folders
    if (depth == 1)
    {
        if (version_folder_uuid.notNull())
        {
            // If there is a version folder, the stock value for the listing is the version folder stock
            return compute_stock_count(version_folder_uuid);
        }
        else
        {
            // If there's no version folder associated, the notion of stock count has no meaning
            return -1;
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

// local helper
bool can_move_to_marketplace(LLInventoryItem* inv_item, std::string& tooltip_msg, bool resolve_links)
{
	// Collapse links directly to items/folders
	LLViewerInventoryItem * viewer_inv_item = (LLViewerInventoryItem *) inv_item;
	LLViewerInventoryItem * linked_item = viewer_inv_item->getLinkedItem();
    LLViewerInventoryCategory * linked_category = viewer_inv_item->getLinkedCategory();

    // Linked items and folders cannot be put for sale if caller can't resolve them
    if (!resolve_links && (linked_category || linked_item))
    {
		tooltip_msg = LLTrans::getString("TooltipOutboxLinked");
        return false;
    }
	
    // A category is always considered as passing...
    if (linked_category != NULL)
	{
        return true;
	}
    
    // Take the linked item if necessary
    if (linked_item != NULL)
	{
		inv_item = linked_item;
	}
	
    // Check that the agent has transfer permission on the item: this is required as a resident cannot
    // put on sale items she cannot transfer. Proceed with move if we have permission.
	bool allow_transfer = inv_item->getPermissions().allowOperationBy(PERM_TRANSFER, gAgent.getID());
	if (!allow_transfer)
	{
		tooltip_msg = LLTrans::getString("TooltipOutboxNoTransfer");
		return false;
	}
    
    // Check worn/not worn status: worn items cannot be put on the marketplace
	bool worn = get_is_item_worn(inv_item->getUUID());
	if (worn)
	{
		tooltip_msg = LLTrans::getString("TooltipOutboxWorn");
		return false;
	}
	
    // Check type: for the moment, calling cards cannot be put on the marketplace
	bool calling_card = (LLAssetType::AT_CALLINGCARD == inv_item->getType());
	if (calling_card)
	{
		tooltip_msg = LLTrans::getString("TooltipOutboxCallingCard");
		return false;
	}
	
	return true;
}

// local helper
// Returns the max tree length (in folder nodes) down from the argument folder
int get_folder_levels(LLInventoryCategory* inv_cat)
{
	LLInventoryModel::cat_array_t* cats;
	LLInventoryModel::item_array_t* items;
	gInventory.getDirectDescendentsOf(inv_cat->getUUID(), cats, items);
    
	int max_child_levels = 0;
    
	for (S32 i=0; i < cats->size(); ++i)
	{
		LLInventoryCategory* category = cats->at(i);
		max_child_levels = llmax(max_child_levels, get_folder_levels(category));
	}
    
	return 1 + max_child_levels;
}

// local helper
// Returns the distance (in folder nodes) between the ancestor and its descendant. Returns -1 if not related.
int get_folder_path_length(const LLUUID& ancestor_id, const LLUUID& descendant_id)
{
	int depth = 0;
    
	if (ancestor_id == descendant_id) return depth;
    
	const LLInventoryCategory* category = gInventory.getCategory(descendant_id);
    
	while (category)
	{
		LLUUID parent_id = category->getParentUUID();
        
		if (parent_id.isNull()) break;
        
		depth++;
        
		if (parent_id == ancestor_id) return depth;
        
		category = gInventory.getCategory(parent_id);
	}
    
	LL_WARNS("SLM") << "get_folder_path_length() couldn't trace a path from the descendant to the ancestor" << LL_ENDL;
	return -1;
}

// local helper
// Returns true if all items within the argument folder are fit for sale, false otherwise
bool has_correct_permissions_for_sale(LLInventoryCategory* cat, std::string& error_msg)
{
	LLInventoryModel::cat_array_t* cat_array;
	LLInventoryModel::item_array_t* item_array;
	gInventory.getDirectDescendentsOf(cat->getUUID(),cat_array,item_array);
    
	LLInventoryModel::item_array_t item_array_copy = *item_array;
    
	for (LLInventoryModel::item_array_t::iterator iter = item_array_copy.begin(); iter != item_array_copy.end(); iter++)
	{
		LLInventoryItem* item = *iter;
        if (!can_move_to_marketplace(item, error_msg, false))
        {
            return false;
        }
	}
    
	LLInventoryModel::cat_array_t cat_array_copy = *cat_array;
    
	for (LLInventoryModel::cat_array_t::iterator iter = cat_array_copy.begin(); iter != cat_array_copy.end(); iter++)
	{
		LLInventoryCategory* category = *iter;
		if (!has_correct_permissions_for_sale(category, error_msg))
        {
            return false;
        }
	}
    return true;
}

// Returns true if inv_item can be dropped in dest_folder, a folder nested in marketplace listings (or merchant inventory) under the root_folder root
// If returns is false, tooltip_msg contains an error message to display to the user (localized and all).
// bundle_size is the amount of sibling items that are getting moved to the marketplace at the same time.
bool can_move_item_to_marketplace(const LLInventoryCategory* root_folder, LLInventoryCategory* dest_folder, LLInventoryItem* inv_item, std::string& tooltip_msg, S32 bundle_size)
{
    // Check stock folder type matches item type in marketplace listings or merchant outbox (even if of no use there for the moment)
    LLViewerInventoryCategory* view_folder = dynamic_cast<LLViewerInventoryCategory*>(dest_folder);
    bool accept = (view_folder && view_folder->acceptItem(inv_item));

    // Check that the item has the right type and persimssions to be sold on the marketplace
    if (accept)
    {
        accept = can_move_to_marketplace(inv_item, tooltip_msg, true);
    }
    
    // Check that the total amount of items won't violate the max limit on the marketplace
    if (accept)
    {
        int existing_item_count = bundle_size;
        
        // Get the version folder: that's where the counts start from
        const LLViewerInventoryCategory * version_folder = ((root_folder && (root_folder != dest_folder)) ? gInventory.getFirstDescendantOf(root_folder->getUUID(), dest_folder->getUUID()) : NULL);

        if (version_folder)
        {
            LLInventoryModel::cat_array_t existing_categories;
            LLInventoryModel::item_array_t existing_items;
            
            gInventory.collectDescendents(version_folder->getUUID(), existing_categories, existing_items, FALSE);
            
            existing_item_count += existing_items.size();
        }
        
        if (existing_item_count > gSavedSettings.getU32("InventoryOutboxMaxItemCount"))
        {
            LLStringUtil::format_map_t args;
            U32 amount = gSavedSettings.getU32("InventoryOutboxMaxItemCount");
            args["[AMOUNT]"] = llformat("%d",amount);
            tooltip_msg = LLTrans::getString("TooltipOutboxTooManyObjects", args);
            accept = false;
        }
    }

    return accept;
}

// Returns true if inv_cat can be dropped in dest_folder, a folder nested in marketplace listings (or merchant inventory) under the root_folder root
// If returns is false, tooltip_msg contains an error message to display to the user (localized and all).
// bundle_size is the amount of sibling items that are getting moved to the marketplace at the same time.
bool can_move_folder_to_marketplace(const LLInventoryCategory* root_folder, LLInventoryCategory* dest_folder, LLInventoryCategory* inv_cat, std::string& tooltip_msg, S32 bundle_size)
{
    bool accept = true;
    
    // Compute the nested folders level we'll add into with that incoming folder
    int incoming_folder_depth = get_folder_levels(inv_cat);
    // Compute the nested folders level we're inserting ourselves in
    // Note: add 1 when inserting under a listing folder as we need to take the root listing folder in the count
    int insertion_point_folder_depth = (root_folder ? get_folder_path_length(root_folder->getUUID(), dest_folder->getUUID()) + 1 : 0);

    // Get the version folder: that's where the folders and items counts start from
    const LLViewerInventoryCategory * version_folder = (insertion_point_folder_depth >= 2 ? gInventory.getFirstDescendantOf(root_folder->getUUID(), dest_folder->getUUID()) : NULL);
    
    // Compare the whole with the nested folders depth limit
    // Note: substract 2 as we leave root and version folder out of the count threshold
    if ((incoming_folder_depth + insertion_point_folder_depth - 2) > gSavedSettings.getU32("InventoryOutboxMaxFolderDepth"))
    {
        LLStringUtil::format_map_t args;
        U32 amount = gSavedSettings.getU32("InventoryOutboxMaxFolderDepth");
        args["[AMOUNT]"] = llformat("%d",amount);
        tooltip_msg = LLTrans::getString("TooltipOutboxFolderLevels", args);
        accept = false;
    }
    
    if (accept)
    {
        LLInventoryModel::cat_array_t descendent_categories;
        LLInventoryModel::item_array_t descendent_items;
        gInventory.collectDescendents(inv_cat->getUUID(), descendent_categories, descendent_items, FALSE);
    
        int dragged_folder_count = descendent_categories.size() + bundle_size;  // Note: We assume that we're moving a bunch of folders in. That might be wrong...
        int dragged_item_count = descendent_items.size();
        int existing_item_count = 0;
        int existing_folder_count = 0;
    
        if (version_folder)
        {
            if (gInventory.isObjectDescendentOf(inv_cat->getUUID(), version_folder->getUUID()))
            {
                // Clear those counts or they will be counted twice because we're already inside the version category
                dragged_folder_count = 0;
                dragged_item_count = 0;
            }
        
            // Tally the total number of categories and items inside the root folder
            LLInventoryModel::cat_array_t existing_categories;
            LLInventoryModel::item_array_t existing_items;
            gInventory.collectDescendents(version_folder->getUUID(), existing_categories, existing_items, FALSE);
        
            existing_folder_count += existing_categories.size();
            existing_item_count += existing_items.size();
        }
    
        const int total_folder_count = existing_folder_count + dragged_folder_count;
        const int total_item_count = existing_item_count + dragged_item_count;
    
        if (total_folder_count > gSavedSettings.getU32("InventoryOutboxMaxFolderCount"))
        {
            LLStringUtil::format_map_t args;
            U32 amount = gSavedSettings.getU32("InventoryOutboxMaxFolderCount");
            args["[AMOUNT]"] = llformat("%d",amount);
            tooltip_msg = LLTrans::getString("TooltipOutboxTooManyFolders", args);
            accept = false;
        }
        else if (total_item_count > gSavedSettings.getU32("InventoryOutboxMaxItemCount"))
        {
            LLStringUtil::format_map_t args;
            U32 amount = gSavedSettings.getU32("InventoryOutboxMaxItemCount");
            args["[AMOUNT]"] = llformat("%d",amount);
            tooltip_msg = LLTrans::getString("TooltipOutboxTooManyObjects", args);
            accept = false;
        }
        
        // Now check that each item in the folder can be moved in the marketplace
        if (accept)
        {
            for (S32 i=0; i < descendent_items.size(); ++i)
            {
                LLInventoryItem* item = descendent_items[i];
                if (!can_move_to_marketplace(item, tooltip_msg, false))
                {
                    accept = false;
                    break;
                }
            }
        }
    }
    
    return accept;
}

bool move_item_to_marketplacelistings(LLInventoryItem* inv_item, LLUUID dest_folder, bool copy)
{
    // Get the marketplace listings depth of the destination folder, exit with error if not under marketplace
    S32 depth = depth_nesting_in_marketplace(dest_folder);
    if (depth < 0)
    {
		LLSD subs;
		subs["[ERROR_CODE]"] = LLTrans::getString("Marketplace Error Prefix") + LLTrans::getString("Marketplace Error Not Merchant");
		LLNotificationsUtil::add("MerchantPasteFailed", subs);
        return false;
    }

    // We will collapse links into items/folders
	LLViewerInventoryItem * viewer_inv_item = (LLViewerInventoryItem *) inv_item;
	LLViewerInventoryCategory * linked_category = viewer_inv_item->getLinkedCategory();
    
	if (linked_category != NULL)
	{
        // Move the linked folder directly
		return move_folder_to_marketplacelistings(linked_category, dest_folder, copy);
	}
	else
	{
        // Grab the linked item if any
		LLViewerInventoryItem * linked_item = viewer_inv_item->getLinkedItem();
        viewer_inv_item = (linked_item != NULL ? linked_item : viewer_inv_item);
    
        // Check that the agent has transfer permission on the item: this is required as a resident cannot
        // put on sale items she cannot transfer. Proceed with move if we have permission.
        std::string error_msg;
        if (can_move_to_marketplace(inv_item, error_msg, true))
        {
            // When moving an isolated item, we might need to create the folder structure to support it
            if (depth == 0)
            {
                // We need a listing folder
                dest_folder = gInventory.createNewCategory(dest_folder, LLFolderType::FT_NONE, viewer_inv_item->getName());
                depth++;
            }
            if (depth == 1)
            {
                // We need a version folder
                dest_folder = gInventory.createNewCategory(dest_folder, LLFolderType::FT_NONE, viewer_inv_item->getName());
                depth++;
            }
			LLViewerInventoryCategory* dest_cat = gInventory.getCategory(dest_folder);
            if (!viewer_inv_item->getPermissions().allowOperationBy(PERM_COPY, gAgent.getID(), gAgent.getGroupID()) &&
                (dest_cat->getPreferredType() != LLFolderType::FT_MARKETPLACE_STOCK))
            {
                // We need a stock folder
                dest_folder = gInventory.createNewCategory(dest_folder, LLFolderType::FT_MARKETPLACE_STOCK, viewer_inv_item->getName());
                dest_cat = gInventory.getCategory(dest_folder);
                depth++;
            }
            
            // Verify we can have this item in that destination category
            if (!dest_cat->acceptItem(viewer_inv_item))
            {
                LLSD subs;
                subs["[ERROR_CODE]"] = LLTrans::getString("Marketplace Error Prefix") + LLTrans::getString("Marketplace Error Not Accepted");
                LLNotificationsUtil::add("MerchantPasteFailed", subs);
                return false;
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
                gInventory.changeItemParent(viewer_inv_item, dest_folder, true);
            }
            
            // Update the modified folders
            update_marketplace_category(src_folder);
            update_marketplace_category(dest_folder);
            gInventory.notifyObservers();
        }
        else
        {
            LLSD subs;
            subs["[ERROR_CODE]"] = LLTrans::getString("Marketplace Error Prefix") + error_msg;
            LLNotificationsUtil::add("MerchantPasteFailed", subs);
            return false;
        }
    }
    return true;
}

bool move_folder_to_marketplacelistings(LLInventoryCategory* inv_cat, const LLUUID& dest_folder, bool copy)
{
    // Check that we have adequate permission on all items being moved. Proceed if we do.
    std::string error_msg;
    if (has_correct_permissions_for_sale(inv_cat, error_msg))
    {
        // Get the destination folder
        LLViewerInventoryCategory* dest_cat = gInventory.getCategory(dest_folder);

        // Check it's not a stock folder
        if (dest_cat->getPreferredType() == LLFolderType::FT_MARKETPLACE_STOCK)
        {
            LLSD subs;
            subs["[ERROR_CODE]"] = LLTrans::getString("Marketplace Error Prefix") + LLTrans::getString("Marketplace Error Not Accepted");
            LLNotificationsUtil::add("MerchantPasteFailed", subs);
            return false;
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
        gInventory.notifyObservers();
    }
    else
    {
        LLSD subs;
        subs["[ERROR_CODE]"] = LLTrans::getString("Marketplace Error Prefix") + error_msg;
        LLNotificationsUtil::add("MerchantPasteFailed", subs);
        return false;
    }
    return true;
}

// Make all relevant business logic checks on the marketplace listings starting with the folder as argument
// This function does no deletion of listings but a mere audit and raises issues to the user
// The only thing that's done is to move and sort folders containing no-copy items to stock folders
// *TODO : Add the rest of the SLM/AIS business logic (limit of nesting depth, stock folder consistency, overall limit on listings, etc...)
void validate_marketplacelistings(LLInventoryCategory* cat, validation_callback_t cb)
{
   // Special case a stock folder depth issue
    LLViewerInventoryCategory * viewer_cat = (LLViewerInventoryCategory *) (cat);
	const LLFolderType::EType folder_type = cat->getPreferredType();
    S32 depth = depth_nesting_in_marketplace(cat->getUUID());
    if (depth < 0)
    {
        // If the folder is not under the marketplace listings root, validation should not be applied
        // *TODO: Should we call update_marketplace_category(cat->getUUID()) ?
        return;
    }
    if (depth == 1)
    {
        if (cb)
        {
            std::string message = LLTrans::getString("Marketplace Validation Intro") + cat->getName();
            cb(message);
        }
        std::string message;
        bool is_ok = can_move_folder_to_marketplace(cat, cat, cat, message, 0);
        if (cb && !is_ok)
        {
            message = LLTrans::getString("Marketplace Validation Error") + message;
            cb(message);
        }
    }
    std::string indent;
    for (int i = 1; i < depth; i++)
    {
        indent += "    ";
    }
    if ((folder_type == LLFolderType::FT_MARKETPLACE_STOCK) && (depth <= 2))
    {
        // Nest the stock folder one level deeper in a normal folder and restart from there
        //LLUUID parent_uuid = gInventory.findCategoryUUIDForType(LLFolderType::FT_MARKETPLACE_LISTINGS, false);
        LLUUID parent_uuid = cat->getParentUUID();
        LLUUID folder_uuid = gInventory.createNewCategory(parent_uuid, LLFolderType::FT_NONE, cat->getName());
        if (cb)
        {
            std::string message = indent + LLTrans::getString("Marketplace Validation Warning Stock") + cat->getName();
            cb(message);
        }
        LLInventoryCategory* new_cat = gInventory.getCategory(folder_uuid);
        gInventory.changeCategoryParent(viewer_cat, folder_uuid, false);
        validate_marketplacelistings(new_cat, cb);
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
        
        // Skip items that shouldn't be there to start with, raise an error message for those
        std::string error_msg;
        if (!can_move_to_marketplace(item, error_msg, false))
        {
            if (cb)
            {
                std::string message = indent + LLTrans::getString("Marketplace Validation Error") + error_msg + " : " + viewer_inv_item->getName();
                cb(message);
            }
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
    // If we have no items in there (only folders) -> all OK
    if (count == 0)
    {
        // We warn if there's really nothing in the folder (may be it's a mistake or an under construction listing)
        if ((cat_array->size() == 0) && cb)
        {
            std::string message = indent + LLTrans::getString("Marketplace Validation Warning Empty") + cat->getName();
            cb(message);
        }
    }
    // If we have one kind only, in the correct folder type at the right depth -> all OK
    else if ((count == 1) && (((type == LLInventoryType::IT_COUNT) && (depth > 1)) || ((folder_type == LLFolderType::FT_MARKETPLACE_STOCK) && (depth > 2))))
    {
        // Done with that folder!
        if (cb)
        {
            std::string message = indent + LLTrans::getString("Marketplace Validation Log") + cat->getName();
            cb(message);
        }
    }
    else
    {
        // Create one folder per vector at the right depth and of the right type
        for (S32 i = 0; i <= LLInventoryType::IT_COUNT; i++)
        {
            if (!items_vector[i].empty())
            {
                // Create a new folder
                LLUUID parent_uuid = (depth > 2 ? viewer_cat->getParentUUID() : viewer_cat->getUUID());
                LLFolderType::EType new_folder_type = (i == LLInventoryType::IT_COUNT ? LLFolderType::FT_NONE : LLFolderType::FT_MARKETPLACE_STOCK);
                if (cb)
                {
                    std::string message = "";
                    if (new_folder_type == LLFolderType::FT_MARKETPLACE_STOCK)
                    {
                        message = indent + LLTrans::getString("Marketplace Validation Warning Create Stock") + viewer_cat->getName();
                    }
                    else
                    {
                        message = indent + LLTrans::getString("Marketplace Validation Warning Create Version") + viewer_cat->getName();
                    }
                    cb(message);
                }
                LLUUID folder_uuid = gInventory.createNewCategory(parent_uuid, new_folder_type, viewer_cat->getName());
                // Move each item to the new folder
                while (!items_vector[i].empty())
                {
                    LLViewerInventoryItem* viewer_inv_item = items_vector[i].back();
                    if (cb)
                    {
                        std::string message = indent + LLTrans::getString("Marketplace Validation Warning Move") + viewer_inv_item->getName();
                        cb(message);
                    }
                    gInventory.changeItemParent(viewer_inv_item, folder_uuid, true);
                    items_vector[i].pop_back();
                }
                update_marketplace_category(folder_uuid);
                gInventory.notifyObservers();
            }
        }
        // Clean up
        if (viewer_cat->getDescendentCount() == 0)
        {
            // Remove the current folder if it ends up empty
            if (cb)
            {
                std::string message = indent + LLTrans::getString("Marketplace Validation Warning Delete") + viewer_cat->getName();
                cb(message);
            }
            gInventory.removeCategory(cat->getUUID());
            gInventory.notifyObservers();
            return;
        }
        else
        {
            // Update the current folder
            update_marketplace_category(cat->getUUID());
            gInventory.notifyObservers();
        }
    }

    // Recursion : Perform the same validation on each nested folder
	LLInventoryModel::cat_array_t cat_array_copy = *cat_array;
    
	for (LLInventoryModel::cat_array_t::iterator iter = cat_array_copy.begin(); iter != cat_array_copy.end(); iter++)
	{
		LLInventoryCategory* category = *iter;
		validate_marketplacelistings(category, cb);
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

bool LLIsValidItemLink::operator()(LLInventoryCategory* cat, LLInventoryItem* item)
{
	LLViewerInventoryItem *vitem = dynamic_cast<LLViewerInventoryItem*>(item);
	if (!vitem) return false;
	return (vitem->getActualType() == LLAssetType::AT_LINK  && !vitem->getIsBrokenLink());
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

	LL_WARNS() << "Not a category and not an item?" << LL_ENDL;
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

// Callback for doToSelected if DAMA required...
void LLInventoryAction::callback_doToSelected(const LLSD& notification, const LLSD& response, class LLInventoryModel* model, class LLFolderView* root, const std::string& action)
{
    S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
    if (option == 0) // YES
    {
        doToSelected(model, root, action, FALSE);
    }
}

void LLInventoryAction::doToSelected(LLInventoryModel* model, LLFolderView* root, const std::string& action, BOOL user_confirm)
{
	std::set<LLFolderViewItem*> selected_items = root->getSelectionList();
        
    // Prompt the user for some marketplace active listing edits
	if (user_confirm && (("delete" == action) || ("rename" == action) || ("properties" == action) || ("task_properties" == action)))
    {
        std::set<LLFolderViewItem*>::iterator set_iter = selected_items.begin();
        LLFolderViewModelItemInventory * viewModel = NULL;
        for (; set_iter != selected_items.end(); ++set_iter)
        {
            viewModel = dynamic_cast<LLFolderViewModelItemInventory *>((*set_iter)->getViewModelItem());
            if (viewModel && LLMarketplaceData::instance().isInActiveFolder(viewModel->getUUID()))
            {
                break;
            }
        }
        if (set_iter != selected_items.end())
        {
            if ((("cut" == action) || ("delete" == action)) && (LLMarketplaceData::instance().isListed(viewModel->getUUID()) || LLMarketplaceData::instance().isVersionFolder(viewModel->getUUID())))
            {
                // Cut or delete of the active version folder or listing folder itself will unlist the listing so ask that question specifically
                LLNotificationsUtil::add("ConfirmMerchantUnlist", LLSD(), LLSD(), boost::bind(&LLInventoryAction::callback_doToSelected, _1, _2, model, root, action));
            }
            else
            {
                // Any other case will simply modify but not unlist a listing
                LLNotificationsUtil::add("ConfirmMerchantActiveChange", LLSD(), LLSD(), boost::bind(&LLInventoryAction::callback_doToSelected, _1, _2, model, root, action));
            }
            return;
        }
    }
    
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
