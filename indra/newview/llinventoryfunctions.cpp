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
//#include "llfirstuse.h"
#include "llfloaterinventory.h"
#include "llfocusmgr.h"
#include "llfolderview.h"
#include "llgesturemgr.h"
#include "lliconctrl.h"
#include "llimview.h"
#include "llinventorybridge.h"
#include "llinventoryclipboard.h"
#include "llinventorymodel.h"
#include "llinventorypanel.h"
#include "lllineeditor.h"
#include "llmenugl.h"
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
#include "llsidetray.h"
#include "llsidepanelinventory.h"
#include "lltabcontainer.h"
#include "lltooldraganddrop.h"
#include "lluictrlfactory.h"
#include "llviewermessage.h"
#include "llviewerobjectlist.h"
#include "llviewerregion.h"
#include "llviewerwindow.h"
#include "llvoavatarself.h"
#include "llwearablelist.h"

BOOL LLInventoryState::sWearNewClothing = FALSE;
LLUUID LLInventoryState::sWearNewClothingTransactionID;

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

void change_item_parent(LLInventoryModel* model,
						LLViewerInventoryItem* item,
						const LLUUID& new_parent_id,
						BOOL restamp)
{
	if (item->getParentUUID() != new_parent_id)
	{
		LLInventoryModel::update_list_t update;
		LLInventoryModel::LLCategoryUpdate old_folder(item->getParentUUID(),-1);
		update.push_back(old_folder);
		LLInventoryModel::LLCategoryUpdate new_folder(new_parent_id, 1);
		update.push_back(new_folder);
		gInventory.accountForUpdate(update);

		LLPointer<LLViewerInventoryItem> new_item = new LLViewerInventoryItem(item);
		new_item->setParent(new_parent_id);
		new_item->updateParentOnServer(restamp);
		model->updateItem(new_item);
		model->notifyObservers();
	}
}

void change_category_parent(LLInventoryModel* model,
	LLViewerInventoryCategory* cat,
	const LLUUID& new_parent_id,
	BOOL restamp)
{
	if (!model || !cat)
	{
		return;
	}

	// Can't move a folder into a child of itself.
	if (model->isObjectDescendentOf(new_parent_id, cat->getUUID()))
	{
		return;
	}

	LLInventoryModel::update_list_t update;
	LLInventoryModel::LLCategoryUpdate old_folder(cat->getParentUUID(), -1);
	update.push_back(old_folder);
	LLInventoryModel::LLCategoryUpdate new_folder(new_parent_id, 1);
	update.push_back(new_folder);
	model->accountForUpdate(update);

	LLPointer<LLViewerInventoryCategory> new_cat = new LLViewerInventoryCategory(cat);
	new_cat->setParent(new_parent_id);
	new_cat->updateParentOnServer(restamp);
	model->updateCategory(new_cat);
	model->notifyObservers();
}

void remove_category(LLInventoryModel* model, const LLUUID& cat_id)
{
	if (!model || !get_is_category_removable(model, cat_id))
	{
		return;
	}

	// Look for any gestures and deactivate them
	LLInventoryModel::cat_array_t	descendent_categories;
	LLInventoryModel::item_array_t	descendent_items;
	gInventory.collectDescendents(cat_id, descendent_categories, descendent_items, FALSE);

	for (LLInventoryModel::item_array_t::const_iterator iter = descendent_items.begin();
		 iter != descendent_items.end();
		 ++iter)
	{
		const LLViewerInventoryItem* item = (*iter);
		const LLUUID& item_id = item->getUUID();
		if (item->getType() == LLAssetType::AT_GESTURE
			&& LLGestureMgr::instance().isGestureActive(item_id))
		{
			LLGestureMgr::instance().deactivateGesture(item_id);
		}
	}

	LLViewerInventoryCategory* cat = gInventory.getCategory(cat_id);
	if (cat)
	{
		const LLUUID trash_id = model->findCategoryUUIDForType(LLFolderType::FT_TRASH);
		change_category_parent(model, cat, trash_id, TRUE);
	}
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
	LLSideTray::getInstance()->showPanel("sidepanel_inventory", LLSD().with("id", item_uuid).with("object", object_id));
}

void show_item_profile(const LLUUID& item_uuid)
{
	LLUUID linked_uuid = gInventory.getLinkedItemID(item_uuid);
	LLSideTray::getInstance()->showPanel("sidepanel_inventory", LLSD().with("id", linked_uuid));
}

void show_item_original(const LLUUID& item_uuid)
{
	//sidetray inventory panel
	LLSidepanelInventory *sidepanel_inventory =
		dynamic_cast<LLSidepanelInventory *>(LLSideTray::getInstance()->getPanel("sidepanel_inventory"));

	bool reset_inventory_filter = !LLSideTray::getInstance()->isPanelActive("sidepanel_inventory");

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
	
	if(reset_inventory_filter)
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
		if(sidepanel_inventory && !floater_inventory_visible)
		{
			LLPanelMainInventory* main_inventory = sidepanel_inventory->getMainInventoryPanel();

			main_inventory->onFilterEdit("");
		}
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
		case LLAssetType::AT_CALLINGCARD:
			return false;
			break;
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
	LLMemType mt(LLMemType::MTYPE_INVENTORY_DO_FOLDER);
	if(mApply)
	{
		// we're applying the open state
		LLInvFVBridge* bridge = (LLInvFVBridge*)folder->getListener();
		if(!bridge) return;
		LLUUID id(bridge->getUUID());
		if(mOpenFolders.find(id) != mOpenFolders.end())
		{
			folder->setOpen(TRUE);
		}
		else
		{
			// keep selected filter in its current state, this is less jarring to user
			if (!folder->isSelected())
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
			LLInvFVBridge* bridge = (LLInvFVBridge*)folder->getListener();
			if(!bridge) return;
			mOpenFolders.insert(bridge->getUUID());
		}
	}
}

void LLOpenFilteredFolders::doItem(LLFolderViewItem *item)
{
	if (item->getFiltered())
	{
		item->getParentFolder()->setOpenArrangeRecursively(TRUE, LLFolderViewFolder::RECURSE_UP);
	}
}

void LLOpenFilteredFolders::doFolder(LLFolderViewFolder* folder)
{
	if (folder->getFiltered() && folder->getParentFolder())
	{
		folder->getParentFolder()->setOpenArrangeRecursively(TRUE, LLFolderViewFolder::RECURSE_UP);
	}
	// if this folder didn't pass the filter, and none of its descendants did
	else if (!folder->getFiltered() && !folder->hasFilteredDescendants())
	{
		folder->setOpenArrangeRecursively(FALSE, LLFolderViewFolder::RECURSE_NO);
	}
}

void LLSelectFirstFilteredItem::doItem(LLFolderViewItem *item)
{
	if (item->getFiltered() && !mItemSelected)
	{
		item->getRoot()->setSelection(item, FALSE, FALSE);
		if (item->getParentFolder())
		{
			item->getParentFolder()->setOpenArrangeRecursively(TRUE, LLFolderViewFolder::RECURSE_UP);
		}
		item->getRoot()->scrollToShowSelection();
		mItemSelected = TRUE;
	}
}

void LLSelectFirstFilteredItem::doFolder(LLFolderViewFolder* folder)
{
	if (folder->getFiltered() && !mItemSelected)
	{
		folder->getRoot()->setSelection(folder, FALSE, FALSE);
		if (folder->getParentFolder())
		{
			folder->getParentFolder()->setOpenArrangeRecursively(TRUE, LLFolderViewFolder::RECURSE_UP);
		}
		folder->getRoot()->scrollToShowSelection();
		mItemSelected = TRUE;
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

