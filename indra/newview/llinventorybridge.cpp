/**
 * @file llinventorybridge.cpp
 * @brief Implementation of the Inventory-Folder-View-Bridge classes.
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
#include "llinventorybridge.h"

// external projects
#include "lltransfersourceasset.h" 
#include "llavatarnamecache.h"	// IDEVO

#include "llagent.h"
#include "llagentcamera.h"
#include "llagentwearables.h"
#include "llappearancemgr.h"
#include "llattachmentsmgr.h"
#include "llavataractions.h" 
#include "llfloateropenobject.h"
#include "llfloaterreg.h"
#include "llfloatersidepanelcontainer.h"
#include "llfloaterworldmap.h"
#include "llfolderview.h"
#include "llfriendcard.h"
#include "llgesturemgr.h"
#include "llgiveinventory.h" 
#include "llimfloater.h"
#include "llimview.h"
#include "llinventoryclipboard.h"
#include "llinventorydefines.h"
#include "llinventoryfunctions.h"
#include "llinventorymodel.h"
#include "llinventorymodelbackgroundfetch.h"
#include "llinventorypanel.h"
#include "llmarketplacefunctions.h"
#include "llnotifications.h"
#include "llnotificationsutil.h"
#include "llpreviewanim.h"
#include "llpreviewgesture.h"
#include "llpreviewtexture.h"
#include "llselectmgr.h"
#include "llsidepanelappearance.h"
#include "lltooldraganddrop.h"
#include "lltrans.h"
#include "llviewerassettype.h"
#include "llviewerfoldertype.h"
#include "llviewermenu.h"
#include "llviewermessage.h"
#include "llviewerobjectlist.h"
#include "llviewerwindow.h"
#include "llvoavatarself.h"
#include "llwearablelist.h"

// Marketplace outbox current disabled
#define ENABLE_MERCHANT_OUTBOX_CONTEXT_MENU	1
#define ENABLE_MERCHANT_SEND_TO_MARKETPLACE_CONTEXT_MENU 0
#define BLOCK_WORN_ITEMS_IN_OUTBOX 1

typedef std::pair<LLUUID, LLUUID> two_uuids_t;
typedef std::list<two_uuids_t> two_uuids_list_t;

struct LLMoveInv
{
	LLUUID mObjectID;
	LLUUID mCategoryID;
	two_uuids_list_t mMoveList;
	void (*mCallback)(S32, void*);
	void* mUserData;
};

using namespace LLOldEvents;

// Helpers
// bug in busy count inc/dec right now, logic is complex... do we really need it?
void inc_busy_count()
{
// 	gViewerWindow->getWindow()->incBusyCount();
//  check balance of these calls if this code is changed to ever actually
//  *do* something!
}
void dec_busy_count()
{
// 	gViewerWindow->getWindow()->decBusyCount();
//  check balance of these calls if this code is changed to ever actually
//  *do* something!
}

// Function declarations
void remove_inventory_category_from_avatar(LLInventoryCategory* category);
void remove_inventory_category_from_avatar_step2( BOOL proceed, LLUUID category_id);
bool move_task_inventory_callback(const LLSD& notification, const LLSD& response, LLMoveInv*);
bool confirm_attachment_rez(const LLSD& notification, const LLSD& response);
void teleport_via_landmark(const LLUUID& asset_id);
static BOOL can_move_to_outfit(LLInventoryItem* inv_item, BOOL move_is_into_current_outfit);
static bool check_category(LLInventoryModel* model,
						   const LLUUID& cat_id,
						   LLFolderView* active_folder_view,
						   LLInventoryFilter* filter);
static bool check_item(const LLUUID& item_id,
					   LLFolderView* active_folder_view,
					   LLInventoryFilter* filter);

// Helper functions

bool isAddAction(const std::string& action)
{
	return ("wear" == action || "attach" == action || "activate" == action);
}

bool isRemoveAction(const std::string& action)
{
	return ("take_off" == action || "detach" == action || "deactivate" == action);
}

bool isMarketplaceCopyAction(const std::string& action)
{
	return (("copy_to_outbox" == action) || ("move_to_outbox" == action));
}

bool isMarketplaceSendAction(const std::string& action)
{
	return ("send_to_marketplace" == action);
}

// +=================================================+
// |        LLInvFVBridge                            |
// +=================================================+

LLInvFVBridge::LLInvFVBridge(LLInventoryPanel* inventory, 
							 LLFolderView* root,
							 const LLUUID& uuid) :
	mUUID(uuid), 
	mRoot(root),
	mInvType(LLInventoryType::IT_NONE),
	mIsLink(FALSE)
{
	mInventoryPanel = inventory->getInventoryPanelHandle();
	const LLInventoryObject* obj = getInventoryObject();
	mIsLink = obj && obj->getIsLinkType();
}

const std::string& LLInvFVBridge::getName() const
{
	const LLInventoryObject* obj = getInventoryObject();
	if(obj)
	{
		return obj->getName();
	}
	return LLStringUtil::null;
}

const std::string& LLInvFVBridge::getDisplayName() const
{
	return getName();
}

// Folders have full perms
PermissionMask LLInvFVBridge::getPermissionMask() const
{
	return PERM_ALL;
}

// virtual
LLFolderType::EType LLInvFVBridge::getPreferredType() const
{
	return LLFolderType::FT_NONE;
}


// Folders don't have creation dates.
time_t LLInvFVBridge::getCreationDate() const
{
	return 0;
}

// Can be destroyed (or moved to trash)
BOOL LLInvFVBridge::isItemRemovable() const
{
	return get_is_item_removable(getInventoryModel(), mUUID);
}

// Can be moved to another folder
BOOL LLInvFVBridge::isItemMovable() const
{
	return TRUE;
}

BOOL LLInvFVBridge::isLink() const
{
	return mIsLink;
}

/*virtual*/
/**
 * @brief Adds this item into clipboard storage
 */
void LLInvFVBridge::cutToClipboard()
{
	if(isItemMovable())
	{
		LLInventoryClipboard::instance().cut(mUUID);
	}
}
// *TODO: make sure this does the right thing
void LLInvFVBridge::showProperties()
{
	show_item_profile(mUUID);

	// Disable old properties floater; this is replaced by the sidepanel.
	/*
	  LLFloaterReg::showInstance("properties", mUUID);
	*/
}

void LLInvFVBridge::removeBatch(LLDynamicArray<LLFolderViewEventListener*>& batch)
{
	// Deactivate gestures when moving them into Trash
	LLInvFVBridge* bridge;
	LLInventoryModel* model = getInventoryModel();
	LLViewerInventoryItem* item = NULL;
	LLViewerInventoryCategory* cat = NULL;
	LLInventoryModel::cat_array_t	descendent_categories;
	LLInventoryModel::item_array_t	descendent_items;
	S32 count = batch.count();
	S32 i,j;
	for(i = 0; i < count; ++i)
	{
		bridge = (LLInvFVBridge*)(batch.get(i));
		if(!bridge || !bridge->isItemRemovable()) continue;
		item = (LLViewerInventoryItem*)model->getItem(bridge->getUUID());
		if (item)
		{
			if(LLAssetType::AT_GESTURE == item->getType())
			{
				LLGestureMgr::instance().deactivateGesture(item->getUUID());
			}
		}
	}
	for(i = 0; i < count; ++i)
	{
		bridge = (LLInvFVBridge*)(batch.get(i));
		if(!bridge || !bridge->isItemRemovable()) continue;
		cat = (LLViewerInventoryCategory*)model->getCategory(bridge->getUUID());
		if (cat)
		{
			gInventory.collectDescendents( cat->getUUID(), descendent_categories, descendent_items, FALSE );
			for (j=0; j<descendent_items.count(); j++)
			{
				if(LLAssetType::AT_GESTURE == descendent_items[j]->getType())
				{
					LLGestureMgr::instance().deactivateGesture(descendent_items[j]->getUUID());
				}
			}
		}
	}
	removeBatchNoCheck(batch);
}

void LLInvFVBridge::removeBatchNoCheck(LLDynamicArray<LLFolderViewEventListener*>& batch)
{
	// this method moves a bunch of items and folders to the trash. As
	// per design guidelines for the inventory model, the message is
	// built and the accounting is performed first. After all of that,
	// we call LLInventoryModel::moveObject() to move everything
	// around.
	LLInvFVBridge* bridge;
	LLInventoryModel* model = getInventoryModel();
	if(!model) return;
	LLMessageSystem* msg = gMessageSystem;
	const LLUUID trash_id = model->findCategoryUUIDForType(LLFolderType::FT_TRASH);
	LLViewerInventoryItem* item = NULL;
	uuid_vec_t move_ids;
	LLInventoryModel::update_map_t update;
	bool start_new_message = true;
	S32 count = batch.count();
	S32 i;

	// first, hide any 'preview' floaters that correspond to the items
	// being deleted.
	for(i = 0; i < count; ++i)
	{
		bridge = (LLInvFVBridge*)(batch.get(i));
		if(!bridge || !bridge->isItemRemovable()) continue;
		item = (LLViewerInventoryItem*)model->getItem(bridge->getUUID());
		if(item)
		{
			LLPreview::hide(item->getUUID());
		}
	}

	// do the inventory move to trash

	for(i = 0; i < count; ++i)
	{
		bridge = (LLInvFVBridge*)(batch.get(i));
		if(!bridge || !bridge->isItemRemovable()) continue;
		item = (LLViewerInventoryItem*)model->getItem(bridge->getUUID());
		if(item)
		{
			if(item->getParentUUID() == trash_id) continue;
			move_ids.push_back(item->getUUID());
			--update[item->getParentUUID()];
			++update[trash_id];
			if(start_new_message)
			{
				start_new_message = false;
				msg->newMessageFast(_PREHASH_MoveInventoryItem);
				msg->nextBlockFast(_PREHASH_AgentData);
				msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
				msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
				msg->addBOOLFast(_PREHASH_Stamp, TRUE);
			}
			msg->nextBlockFast(_PREHASH_InventoryData);
			msg->addUUIDFast(_PREHASH_ItemID, item->getUUID());
			msg->addUUIDFast(_PREHASH_FolderID, trash_id);
			msg->addString("NewName", NULL);
			if(msg->isSendFullFast(_PREHASH_InventoryData))
			{
				start_new_message = true;
				gAgent.sendReliableMessage();
				gInventory.accountForUpdate(update);
				update.clear();
			}
		}
	}
	if(!start_new_message)
	{
		start_new_message = true;
		gAgent.sendReliableMessage();
		gInventory.accountForUpdate(update);
		update.clear();
	}

	for(i = 0; i < count; ++i)
	{
		bridge = (LLInvFVBridge*)(batch.get(i));
		if(!bridge || !bridge->isItemRemovable()) continue;
		LLViewerInventoryCategory* cat = (LLViewerInventoryCategory*)model->getCategory(bridge->getUUID());
		if(cat)
		{
			if(cat->getParentUUID() == trash_id) continue;
			move_ids.push_back(cat->getUUID());
			--update[cat->getParentUUID()];
			++update[trash_id];
			if(start_new_message)
			{
				start_new_message = false;
				msg->newMessageFast(_PREHASH_MoveInventoryFolder);
				msg->nextBlockFast(_PREHASH_AgentData);
				msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
				msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
				msg->addBOOL("Stamp", TRUE);
			}
			msg->nextBlockFast(_PREHASH_InventoryData);
			msg->addUUIDFast(_PREHASH_FolderID, cat->getUUID());
			msg->addUUIDFast(_PREHASH_ParentID, trash_id);
			if(msg->isSendFullFast(_PREHASH_InventoryData))
			{
				start_new_message = true;
				gAgent.sendReliableMessage();
				gInventory.accountForUpdate(update);
				update.clear();
			}
		}
	}
	if(!start_new_message)
	{
		gAgent.sendReliableMessage();
		gInventory.accountForUpdate(update);
	}

	// move everything.
	uuid_vec_t::iterator it = move_ids.begin();
	uuid_vec_t::iterator end = move_ids.end();
	for(; it != end; ++it)
	{
		gInventory.moveObject((*it), trash_id);
	}

	// notify inventory observers.
	model->notifyObservers();
}

BOOL LLInvFVBridge::isClipboardPasteable() const
{
	if (!LLInventoryClipboard::instance().hasContents() || !isAgentInventory())
	{
		return FALSE;
	}
	LLInventoryModel* model = getInventoryModel();
	if (!model)
	{
		return FALSE;
	}

	const LLUUID &agent_id = gAgent.getID();

	LLDynamicArray<LLUUID> objects;
	LLInventoryClipboard::instance().retrieve(objects);
	S32 count = objects.count();
	for(S32 i = 0; i < count; i++)
	{
		const LLUUID &item_id = objects.get(i);

		// Can't paste folders
		const LLInventoryCategory *cat = model->getCategory(item_id);
		if (cat)
		{
			return FALSE;
		}

		const LLInventoryItem *item = model->getItem(item_id);
		if (item)
		{
			if (!item->getPermissions().allowCopyBy(agent_id))
			{
				return FALSE;
			}
		}
	}
	return TRUE;
}

BOOL LLInvFVBridge::isClipboardPasteableAsLink() const
{
	if (!LLInventoryClipboard::instance().hasContents() || !isAgentInventory())
	{
		return FALSE;
	}
	const LLInventoryModel* model = getInventoryModel();
	if (!model)
	{
		return FALSE;
	}

	LLDynamicArray<LLUUID> objects;
	LLInventoryClipboard::instance().retrieve(objects);
	S32 count = objects.count();
	for(S32 i = 0; i < count; i++)
	{
		const LLInventoryItem *item = model->getItem(objects.get(i));
		if (item)
		{
			if (!LLAssetType::lookupCanLink(item->getActualType()))
			{
				return FALSE;
			}
		}
		const LLViewerInventoryCategory *cat = model->getCategory(objects.get(i));
		if (cat && LLFolderType::lookupIsProtectedType(cat->getPreferredType()))
		{
			return FALSE;
		}
	}
	return TRUE;
}

void hide_context_entries(LLMenuGL& menu, 
						  const menuentry_vec_t &entries_to_show,
						  const menuentry_vec_t &disabled_entries)
{
	const LLView::child_list_t *list = menu.getChildList();

	// For removing double separators or leading separator.  Start at true so that
	// if the first element is a separator, it will not be shown.
	bool is_previous_entry_separator = true;

	for (LLView::child_list_t::const_iterator itor = list->begin(); 
		 itor != list->end(); 
		 ++itor)
	{
		LLView *menu_item = (*itor);
		std::string name = menu_item->getName();

		// descend into split menus:
		LLMenuItemBranchGL* branchp = dynamic_cast<LLMenuItemBranchGL*>(menu_item);
		if ((name == "More") && branchp)
		{
			hide_context_entries(*branchp->getBranch(), entries_to_show, disabled_entries);
		}

		bool found = false;
		menuentry_vec_t::const_iterator itor2;
		for (itor2 = entries_to_show.begin(); itor2 != entries_to_show.end(); ++itor2)
		{
			if (*itor2 == name)
			{
				found = true;
				break;
			}
		}

		// Don't allow multiple separators in a row (e.g. such as if there are no items
		// between two separators).
		if (found)
		{
			const bool is_entry_separator = (dynamic_cast<LLMenuItemSeparatorGL *>(menu_item) != NULL);
			found = !(is_entry_separator && is_previous_entry_separator);
			is_previous_entry_separator = is_entry_separator;
		}
		
		if (!found)
		{
			if (!menu_item->getLastVisible())
			{
				menu_item->setVisible(FALSE);
			}

			menu_item->setEnabled(FALSE);
		}
		else
		{
			menu_item->setVisible(TRUE);
			// A bit of a hack so we can remember that some UI element explicitly set this to be visible
			// so that some other UI element from multi-select doesn't later set this invisible.
			menu_item->pushVisible(TRUE);

			bool enabled = (menu_item->getEnabled() == TRUE);
			for (itor2 = disabled_entries.begin(); enabled && (itor2 != disabled_entries.end()); ++itor2)
			{
				enabled &= (*itor2 != name);
			}

			menu_item->setEnabled(enabled);
		}
	}
}

// Helper for commonly-used entries
void LLInvFVBridge::getClipboardEntries(bool show_asset_id,
										menuentry_vec_t &items,
										menuentry_vec_t &disabled_items, U32 flags)
{
	const LLInventoryObject *obj = getInventoryObject();

	if (obj)
	{
		if (obj->getIsLinkType())
		{
			items.push_back(std::string("Find Original"));
			if (isLinkedObjectMissing())
			{
				disabled_items.push_back(std::string("Find Original"));
			}
		}
		else
		{
			if (LLAssetType::lookupCanLink(obj->getType()))
			{
				items.push_back(std::string("Find Links"));
			}

			if (!isInboxFolder())
			{
				items.push_back(std::string("Rename"));
				if (!isItemRenameable() || (flags & FIRST_SELECTED_ITEM) == 0)
				{
					disabled_items.push_back(std::string("Rename"));
				}
			}
			
			if (show_asset_id)
			{
				items.push_back(std::string("Copy Asset UUID"));

				bool is_asset_knowable = false;

				LLViewerInventoryItem* inv_item = gInventory.getItem(mUUID);
				if (inv_item)
				{
					is_asset_knowable = LLAssetType::lookupIsAssetIDKnowable(inv_item->getType());
				}
				if ( !is_asset_knowable // disable menu item for Inventory items with unknown asset. EXT-5308
					 || (! ( isItemPermissive() || gAgent.isGodlike() ) )
					 || (flags & FIRST_SELECTED_ITEM) == 0)
				{
					disabled_items.push_back(std::string("Copy Asset UUID"));
				}
			}
			items.push_back(std::string("Copy Separator"));
			
			items.push_back(std::string("Copy"));
			if (!isItemCopyable())
			{
				disabled_items.push_back(std::string("Copy"));
			}

			if (canListOnMarketplace())
			{
				items.push_back(std::string("Marketplace Separator"));

				items.push_back(std::string("Merchant Copy"));
				if (!canListOnMarketplaceNow())
				{
					disabled_items.push_back(std::string("Merchant Copy"));
				}
			}
		}
	}

	// Don't allow items to be pasted directly into the COF or the inbox/outbox
	if (!isCOFFolder() && !isInboxFolder() && !isOutboxFolder())
	{
		items.push_back(std::string("Paste"));
	}
	if (!isClipboardPasteable() || ((flags & FIRST_SELECTED_ITEM) == 0))
	{
		disabled_items.push_back(std::string("Paste"));
	}

	if (gSavedSettings.getBOOL("InventoryLinking"))
	{
		items.push_back(std::string("Paste As Link"));
		if (!isClipboardPasteableAsLink() || (flags & FIRST_SELECTED_ITEM) == 0)
		{
			disabled_items.push_back(std::string("Paste As Link"));
		}
	}

	items.push_back(std::string("Paste Separator"));

	addDeleteContextMenuOptions(items, disabled_items);

	// If multiple items are selected, disable properties (if it exists).
	if ((flags & FIRST_SELECTED_ITEM) == 0)
	{
		disabled_items.push_back(std::string("Properties"));
	}
}

void LLInvFVBridge::buildContextMenu(LLMenuGL& menu, U32 flags)
{
	lldebugs << "LLInvFVBridge::buildContextMenu()" << llendl;
	menuentry_vec_t items;
	menuentry_vec_t disabled_items;
	if(isItemInTrash())
	{
		addTrashContextMenuOptions(items, disabled_items);
	}	
	else if(isOutboxFolder())
	{
		addOutboxContextMenuOptions(flags, items, disabled_items);
	}
	else
	{
		items.push_back(std::string("Share"));
		if (!canShare())
		{
			disabled_items.push_back(std::string("Share"));
		}
		
		addOpenRightClickMenuOption(items);
		items.push_back(std::string("Properties"));

		getClipboardEntries(true, items, disabled_items, flags);
	}
	hide_context_entries(menu, items, disabled_items);
}

void LLInvFVBridge::addTrashContextMenuOptions(menuentry_vec_t &items,
											   menuentry_vec_t &disabled_items)
{
	const LLInventoryObject *obj = getInventoryObject();
	if (obj && obj->getIsLinkType())
	{
		items.push_back(std::string("Find Original"));
		if (isLinkedObjectMissing())
		{
			disabled_items.push_back(std::string("Find Original"));
		}
	}
	items.push_back(std::string("Purge Item"));
	if (!isItemRemovable())
	{
		disabled_items.push_back(std::string("Purge Item"));
	}
	items.push_back(std::string("Restore Item"));
}

void LLInvFVBridge::addDeleteContextMenuOptions(menuentry_vec_t &items,
												menuentry_vec_t &disabled_items)
{

	const LLInventoryObject *obj = getInventoryObject();

	// Don't allow delete as a direct option from COF folder.
	if (obj && obj->getIsLinkType() && isCOFFolder() && get_is_item_worn(mUUID))
	{
		return;
	}

	// "Remove link" and "Delete" are the same operation.
	if (obj && obj->getIsLinkType() && !get_is_item_worn(mUUID))
	{
		items.push_back(std::string("Remove Link"));
	}
	else
	{
		items.push_back(std::string("Delete"));
	}

	if (!isItemRemovable())
	{
		disabled_items.push_back(std::string("Delete"));
	}
}

void LLInvFVBridge::addOpenRightClickMenuOption(menuentry_vec_t &items)
{
	const LLInventoryObject *obj = getInventoryObject();
	const BOOL is_link = (obj && obj->getIsLinkType());

	if (is_link)
		items.push_back(std::string("Open Original"));
	else
		items.push_back(std::string("Open"));
}

void LLInvFVBridge::addOutboxContextMenuOptions(U32 flags,
												menuentry_vec_t &items,
												menuentry_vec_t &disabled_items)
{
	items.push_back(std::string("Rename"));
	items.push_back(std::string("Delete"));
	
	if ((flags & FIRST_SELECTED_ITEM) == 0)
	{
		disabled_items.push_back(std::string("Rename"));
	}
	
#if ENABLE_MERCHANT_SEND_TO_MARKETPLACE_CONTEXT_MENU
	if (isOutboxFolderDirectParent())
	{
		items.push_back(std::string("Marketplace Separator"));
		items.push_back(std::string("Marketplace Send"));
		
		if ((flags & FIRST_SELECTED_ITEM) == 0)
		{
			disabled_items.push_back(std::string("Marketplace Send"));
		}
	}
#endif // ENABLE_MERCHANT_SEND_TO_MARKETPLACE_CONTEXT_MENU
}

// *TODO: remove this
BOOL LLInvFVBridge::startDrag(EDragAndDropType* type, LLUUID* id) const
{
	BOOL rv = FALSE;

	const LLInventoryObject* obj = getInventoryObject();

	if(obj)
	{
		*type = LLViewerAssetType::lookupDragAndDropType(obj->getActualType());
		if(*type == DAD_NONE)
		{
			return FALSE;
		}

		*id = obj->getUUID();
		//object_ids.put(obj->getUUID());

		if (*type == DAD_CATEGORY)
		{
			LLInventoryModelBackgroundFetch::instance().start(obj->getUUID());
		}

		rv = TRUE;
	}

	return rv;
}

LLInventoryObject* LLInvFVBridge::getInventoryObject() const
{
	LLInventoryObject* obj = NULL;
	LLInventoryModel* model = getInventoryModel();
	if(model)
	{
		obj = (LLInventoryObject*)model->getObject(mUUID);
	}
	return obj;
}

LLInventoryModel* LLInvFVBridge::getInventoryModel() const
{
	LLInventoryPanel* panel = mInventoryPanel.get();
	return panel ? panel->getModel() : NULL;
}

BOOL LLInvFVBridge::isItemInTrash() const
{
	LLInventoryModel* model = getInventoryModel();
	if(!model) return FALSE;
	const LLUUID trash_id = model->findCategoryUUIDForType(LLFolderType::FT_TRASH);
	return model->isObjectDescendentOf(mUUID, trash_id);
}

BOOL LLInvFVBridge::isLinkedObjectInTrash() const
{
	if (isItemInTrash()) return TRUE;

	const LLInventoryObject *obj = getInventoryObject();
	if (obj && obj->getIsLinkType())
	{
		LLInventoryModel* model = getInventoryModel();
		if(!model) return FALSE;
		const LLUUID trash_id = model->findCategoryUUIDForType(LLFolderType::FT_TRASH);
		return model->isObjectDescendentOf(obj->getLinkedUUID(), trash_id);
	}
	return FALSE;
}

BOOL LLInvFVBridge::isLinkedObjectMissing() const
{
	const LLInventoryObject *obj = getInventoryObject();
	if (!obj)
	{
		return TRUE;
	}
	if (obj->getIsLinkType() && LLAssetType::lookupIsLinkType(obj->getType()))
	{
		return TRUE;
	}
	return FALSE;
}

BOOL LLInvFVBridge::isAgentInventory() const
{
	const LLInventoryModel* model = getInventoryModel();
	if(!model) return FALSE;
	if(gInventory.getRootFolderID() == mUUID) return TRUE;
	return model->isObjectDescendentOf(mUUID, gInventory.getRootFolderID());
}

BOOL LLInvFVBridge::isCOFFolder() const
{
	return LLAppearanceMgr::instance().getIsInCOF(mUUID);
}

BOOL LLInvFVBridge::isInboxFolder() const
{
	const LLUUID inbox_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_INBOX, false, false);
	
	if (inbox_id.isNull())
	{
		return FALSE;
	}
	
	return gInventory.isObjectDescendentOf(mUUID, inbox_id);
}

BOOL LLInvFVBridge::isOutboxFolder() const
{
	const LLUUID outbox_id = getOutboxFolder();

	if (outbox_id.isNull())
	{
		return FALSE;
	}

	return gInventory.isObjectDescendentOf(mUUID, outbox_id);
}

BOOL LLInvFVBridge::isOutboxFolderDirectParent() const
{
	BOOL outbox_is_parent = FALSE;
	
	const LLInventoryCategory *cat = gInventory.getCategory(mUUID);

	if (cat)
	{
		const LLUUID outbox_id = getOutboxFolder();
		
		outbox_is_parent = (outbox_id.notNull() && (outbox_id == cat->getParentUUID()));
	}
	
	return outbox_is_parent;
}

const LLUUID LLInvFVBridge::getOutboxFolder() const
{
	const LLUUID outbox_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_OUTBOX, false, false);

	return outbox_id;
}

BOOL LLInvFVBridge::isItemPermissive() const
{
	return FALSE;
}

// static
void LLInvFVBridge::changeItemParent(LLInventoryModel* model,
									 LLViewerInventoryItem* item,
									 const LLUUID& new_parent_id,
									 BOOL restamp)
{
	change_item_parent(model, item, new_parent_id, restamp);
}

// static
void LLInvFVBridge::changeCategoryParent(LLInventoryModel* model,
										 LLViewerInventoryCategory* cat,
										 const LLUUID& new_parent_id,
										 BOOL restamp)
{
	change_category_parent(model, cat, new_parent_id, restamp);
}

LLInvFVBridge* LLInvFVBridge::createBridge(LLAssetType::EType asset_type,
										   LLAssetType::EType actual_asset_type,
										   LLInventoryType::EType inv_type,
										   LLInventoryPanel* inventory,
										   LLFolderView* root,
										   const LLUUID& uuid,
										   U32 flags)
{
	LLInvFVBridge* new_listener = NULL;
	switch(asset_type)
	{
		case LLAssetType::AT_TEXTURE:
			if(!(inv_type == LLInventoryType::IT_TEXTURE || inv_type == LLInventoryType::IT_SNAPSHOT))
			{
				llwarns << LLAssetType::lookup(asset_type) << " asset has inventory type " << LLInventoryType::lookupHumanReadable(inv_type) << " on uuid " << uuid << llendl;
			}
			new_listener = new LLTextureBridge(inventory, root, uuid, inv_type);
			break;

		case LLAssetType::AT_SOUND:
			if(!(inv_type == LLInventoryType::IT_SOUND))
			{
				llwarns << LLAssetType::lookup(asset_type) << " asset has inventory type " << LLInventoryType::lookupHumanReadable(inv_type) << " on uuid " << uuid << llendl;
			}
			new_listener = new LLSoundBridge(inventory, root, uuid);
			break;

		case LLAssetType::AT_LANDMARK:
			if(!(inv_type == LLInventoryType::IT_LANDMARK))
			{
				llwarns << LLAssetType::lookup(asset_type) << " asset has inventory type " << LLInventoryType::lookupHumanReadable(inv_type) << " on uuid " << uuid << llendl;
			}
			new_listener = new LLLandmarkBridge(inventory, root, uuid, flags);
			break;

		case LLAssetType::AT_CALLINGCARD:
			if(!(inv_type == LLInventoryType::IT_CALLINGCARD))
			{
				llwarns << LLAssetType::lookup(asset_type) << " asset has inventory type " << LLInventoryType::lookupHumanReadable(inv_type) << " on uuid " << uuid << llendl;
			}
			new_listener = new LLCallingCardBridge(inventory, root, uuid);
			break;

		case LLAssetType::AT_SCRIPT:
			if(!(inv_type == LLInventoryType::IT_LSL))
			{
				llwarns << LLAssetType::lookup(asset_type) << " asset has inventory type " << LLInventoryType::lookupHumanReadable(inv_type) << " on uuid " << uuid << llendl;
			}
			new_listener = new LLItemBridge(inventory, root, uuid);
			break;

		case LLAssetType::AT_OBJECT:
			if(!(inv_type == LLInventoryType::IT_OBJECT || inv_type == LLInventoryType::IT_ATTACHMENT))
			{
				llwarns << LLAssetType::lookup(asset_type) << " asset has inventory type " << LLInventoryType::lookupHumanReadable(inv_type) << " on uuid " << uuid << llendl;
			}
			new_listener = new LLObjectBridge(inventory, root, uuid, inv_type, flags);
			break;

		case LLAssetType::AT_NOTECARD:
			if(!(inv_type == LLInventoryType::IT_NOTECARD))
			{
				llwarns << LLAssetType::lookup(asset_type) << " asset has inventory type " << LLInventoryType::lookupHumanReadable(inv_type) << " on uuid " << uuid << llendl;
			}
			new_listener = new LLNotecardBridge(inventory, root, uuid);
			break;

		case LLAssetType::AT_ANIMATION:
			if(!(inv_type == LLInventoryType::IT_ANIMATION))
			{
				llwarns << LLAssetType::lookup(asset_type) << " asset has inventory type " << LLInventoryType::lookupHumanReadable(inv_type) << " on uuid " << uuid << llendl;
			}
			new_listener = new LLAnimationBridge(inventory, root, uuid);
			break;

		case LLAssetType::AT_GESTURE:
			if(!(inv_type == LLInventoryType::IT_GESTURE))
			{
				llwarns << LLAssetType::lookup(asset_type) << " asset has inventory type " << LLInventoryType::lookupHumanReadable(inv_type) << " on uuid " << uuid << llendl;
			}
			new_listener = new LLGestureBridge(inventory, root, uuid);
			break;

		case LLAssetType::AT_LSL_TEXT:
			if(!(inv_type == LLInventoryType::IT_LSL))
			{
				llwarns << LLAssetType::lookup(asset_type) << " asset has inventory type " << LLInventoryType::lookupHumanReadable(inv_type) << " on uuid " << uuid << llendl;
			}
			new_listener = new LLLSLTextBridge(inventory, root, uuid);
			break;

		case LLAssetType::AT_CLOTHING:
		case LLAssetType::AT_BODYPART:
			if(!(inv_type == LLInventoryType::IT_WEARABLE))
			{
				llwarns << LLAssetType::lookup(asset_type) << " asset has inventory type " << LLInventoryType::lookupHumanReadable(inv_type) << " on uuid " << uuid << llendl;
			}
			new_listener = new LLWearableBridge(inventory, root, uuid, asset_type, inv_type, (LLWearableType::EType)flags);
			break;
		case LLAssetType::AT_CATEGORY:
			if (actual_asset_type == LLAssetType::AT_LINK_FOLDER)
			{
				// Create a link folder handler instead.
				new_listener = new LLLinkFolderBridge(inventory, root, uuid);
				break;
			}
			new_listener = new LLFolderBridge(inventory, root, uuid);
			break;
		case LLAssetType::AT_LINK:
		case LLAssetType::AT_LINK_FOLDER:
			// Only should happen for broken links.
			new_listener = new LLLinkItemBridge(inventory, root, uuid);
			break;
	    case LLAssetType::AT_MESH:
			if(!(inv_type == LLInventoryType::IT_MESH))
			{
				llwarns << LLAssetType::lookup(asset_type) << " asset has inventory type " << LLInventoryType::lookupHumanReadable(inv_type) << " on uuid " << uuid << llendl;
			}
			new_listener = new LLMeshBridge(inventory, root, uuid);
			break;

		case LLAssetType::AT_IMAGE_TGA:
		case LLAssetType::AT_IMAGE_JPEG:
			//llwarns << LLAssetType::lookup(asset_type) << " asset type is unhandled for uuid " << uuid << llendl;
			break;

		default:
			llinfos << "Unhandled asset type (llassetstorage.h): "
					<< (S32)asset_type << " (" << LLAssetType::lookup(asset_type) << ")" << llendl;
			break;
	}

	if (new_listener)
	{
		new_listener->mInvType = inv_type;
	}

	return new_listener;
}

void LLInvFVBridge::purgeItem(LLInventoryModel *model, const LLUUID &uuid)
{
	LLInventoryCategory* cat = model->getCategory(uuid);
	if (cat)
	{
		model->purgeDescendentsOf(uuid);
		model->notifyObservers();
	}
	LLInventoryObject* obj = model->getObject(uuid);
	if (obj)
	{
		model->purgeObject(uuid);
		model->notifyObservers();
	}
}

bool LLInvFVBridge::canShare() const
{
	bool can_share = false;

	if (isAgentInventory())
	{
		const LLInventoryModel* model = getInventoryModel();
		if (model)
		{
			const LLViewerInventoryItem *item = model->getItem(mUUID);
			if (item)
			{
				if (LLInventoryCollectFunctor::itemTransferCommonlyAllowed(item)) 
				{
					can_share = LLGiveInventory::isInventoryGiveAcceptable(item);
				}
			}
			else
			{
				// Categories can be given.
				can_share = (model->getCategory(mUUID) != NULL);
			}
		}
	}

	return can_share;
}

bool LLInvFVBridge::canListOnMarketplace() const
{
#if ENABLE_MERCHANT_OUTBOX_CONTEXT_MENU

	LLInventoryModel * model = getInventoryModel();

	const LLViewerInventoryCategory * cat = model->getCategory(mUUID);
	if (cat && LLFolderType::lookupIsProtectedType(cat->getPreferredType()))
	{
		return false;
	}

	if (!isAgentInventory())
	{
		return false;
	}
	
	if (getOutboxFolder().isNull())
	{
		return false;
	}

	if (isInboxFolder() || isOutboxFolder())
	{
		return false;
	}
	
	LLViewerInventoryItem * item = model->getItem(mUUID);
	if (item)
	{
		if (!item->getPermissions().allowOperationBy(PERM_TRANSFER, gAgent.getID()))
		{
			return false;
		}
		
		if (LLAssetType::AT_CALLINGCARD == item->getType())
		{
			return false;
		}
	}

	return true;

#else
	return false;
#endif
}

bool LLInvFVBridge::canListOnMarketplaceNow() const
{
#if ENABLE_MERCHANT_OUTBOX_CONTEXT_MENU
	
	bool can_list = true;

	// Do not allow listing while import is in progress
	if (LLMarketplaceInventoryImporter::instanceExists())
	{
		can_list = !LLMarketplaceInventoryImporter::instance().isImportInProgress();
	}
	
	const LLInventoryObject* obj = getInventoryObject();
	can_list &= (obj != NULL);

	if (can_list)
	{
		const LLUUID& object_id = obj->getLinkedUUID();
		can_list = object_id.notNull();

		if (can_list)
		{
			LLFolderViewFolder * object_folderp = mRoot->getFolderByID(object_id);
			if (object_folderp)
			{
				can_list = !object_folderp->isLoading();
			}
		}
		
		if (can_list)
		{
			// Get outbox id
			const LLUUID & outbox_id = getInventoryModel()->findCategoryUUIDForType(LLFolderType::FT_OUTBOX, false);
			LLFolderViewItem * outbox_itemp = mRoot->getItemByID(outbox_id);

			if (outbox_itemp)
			{
				MASK mask = 0x0;
				BOOL drop = FALSE;
				EDragAndDropType cargo_type = LLViewerAssetType::lookupDragAndDropType(obj->getActualType());
				void * cargo_data = (void *) obj;
				std::string tooltip_msg;
				
				can_list = outbox_itemp->getListener()->dragOrDrop(mask, drop, cargo_type, cargo_data, tooltip_msg);
			}
		}
	}
	
	return can_list;

#else
	return false;
#endif
}


// +=================================================+
// |        InventoryFVBridgeBuilder                 |
// +=================================================+
LLInvFVBridge* LLInventoryFVBridgeBuilder::createBridge(LLAssetType::EType asset_type,
														LLAssetType::EType actual_asset_type,
														LLInventoryType::EType inv_type,
														LLInventoryPanel* inventory,
														LLFolderView* root,
														const LLUUID& uuid,
														U32 flags /* = 0x00 */) const
{
	return LLInvFVBridge::createBridge(asset_type,
									   actual_asset_type,
									   inv_type,
									   inventory,
									   root,
									   uuid,
									   flags);
}

// +=================================================+
// |        LLItemBridge                             |
// +=================================================+

void LLItemBridge::performAction(LLInventoryModel* model, std::string action)
{
	if ("goto" == action)
	{
		gotoItem();
	}

	if ("open" == action || "open_original" == action)
	{
		openItem();
		return;
	}
	else if ("properties" == action)
	{
		showProperties();
		return;
	}
	else if ("purge" == action)
	{
		purgeItem(model, mUUID);
		return;
	}
	else if ("restoreToWorld" == action)
	{
		restoreToWorld();
		return;
	}
	else if ("restore" == action)
	{
		restoreItem();
		return;
	}
	else if ("copy_uuid" == action)
	{
		// Single item only
		LLViewerInventoryItem* item = static_cast<LLViewerInventoryItem*>(getItem());
		if(!item) return;
		LLUUID asset_id = item->getProtectedAssetUUID();
		std::string buffer;
		asset_id.toString(buffer);

		gViewerWindow->getWindow()->copyTextToClipboard(utf8str_to_wstring(buffer));
		return;
	}
	else if ("copy" == action)
	{
		copyToClipboard();
		return;
	}
	else if ("paste" == action)
	{
		// Single item only
		LLInventoryItem* itemp = model->getItem(mUUID);
		if (!itemp) return;

		LLFolderViewItem* folder_view_itemp = mRoot->getItemByID(itemp->getParentUUID());
		if (!folder_view_itemp) return;

		folder_view_itemp->getListener()->pasteFromClipboard();
		return;
	}
	else if ("paste_link" == action)
	{
		// Single item only
		LLInventoryItem* itemp = model->getItem(mUUID);
		if (!itemp) return;

		LLFolderViewItem* folder_view_itemp = mRoot->getItemByID(itemp->getParentUUID());
		if (!folder_view_itemp) return;

		folder_view_itemp->getListener()->pasteLinkFromClipboard();
		return;
	}
	else if (isMarketplaceCopyAction(action))
	{
		llinfos << "Copy item to marketplace action!" << llendl;

		LLInventoryItem* itemp = model->getItem(mUUID);
		if (!itemp) return;

		const LLUUID outbox_id = getInventoryModel()->findCategoryUUIDForType(LLFolderType::FT_OUTBOX, false, false);
		copy_item_to_outbox(itemp, outbox_id, LLUUID::null, LLToolDragAndDrop::getOperationId());
	}
}

void LLItemBridge::selectItem()
{
	LLViewerInventoryItem* item = static_cast<LLViewerInventoryItem*>(getItem());
	if(item && !item->isFinished())
	{
		//item->fetchFromServer();
		LLInventoryModelBackgroundFetch::instance().start(item->getUUID(), false);
	}
}

void LLItemBridge::restoreItem()
{
	LLViewerInventoryItem* item = static_cast<LLViewerInventoryItem*>(getItem());
	if(item)
	{
		LLInventoryModel* model = getInventoryModel();
		const LLUUID new_parent = model->findCategoryUUIDForType(LLFolderType::assetTypeToFolderType(item->getType()));
		// do not restamp on restore.
		LLInvFVBridge::changeItemParent(model, item, new_parent, FALSE);
	}
}

void LLItemBridge::restoreToWorld()
{
	//Similar functionality to the drag and drop rez logic
	bool remove_from_inventory = false;

	LLViewerInventoryItem* itemp = static_cast<LLViewerInventoryItem*>(getItem());
	if (itemp)
	{
		LLMessageSystem* msg = gMessageSystem;
		msg->newMessage("RezRestoreToWorld");
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());

		msg->nextBlockFast(_PREHASH_InventoryData);
		itemp->packMessage(msg);
		msg->sendReliable(gAgent.getRegion()->getHost());

		//remove local inventory copy, sim will deal with permissions and removing the item
		//from the actual inventory if its a no-copy etc
		if(!itemp->getPermissions().allowCopyBy(gAgent.getID()))
		{
			remove_from_inventory = true;
		}
		
		// Check if it's in the trash. (again similar to the normal rez logic)
		const LLUUID trash_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_TRASH);
		if(gInventory.isObjectDescendentOf(itemp->getUUID(), trash_id))
		{
			remove_from_inventory = true;
		}
	}

	if(remove_from_inventory)
	{
		gInventory.deleteObject(itemp->getUUID());
		gInventory.notifyObservers();
	}
}

void LLItemBridge::gotoItem()
{
	LLInventoryObject *obj = getInventoryObject();
	if (obj && obj->getIsLinkType())
	{
		LLInventoryPanel *active_panel = LLInventoryPanel::getActiveInventoryPanel();
		if (active_panel)
		{
			active_panel->setSelection(obj->getLinkedUUID(), TAKE_FOCUS_NO);
		}
	}
}

LLUIImagePtr LLItemBridge::getIcon() const
{
	LLInventoryObject *obj = getInventoryObject();
	if (obj) 
	{
		return LLInventoryIcon::getIcon(obj->getType(),
										LLInventoryType::IT_NONE,
										mIsLink);
	}
	
	return LLInventoryIcon::getIcon(LLInventoryIcon::ICONNAME_OBJECT);
}

PermissionMask LLItemBridge::getPermissionMask() const
{
	LLViewerInventoryItem* item = getItem();
	PermissionMask perm_mask = 0;
	if (item) perm_mask = item->getPermissionMask();
	return perm_mask;
}

const std::string& LLItemBridge::getDisplayName() const
{
	if(mDisplayName.empty())
	{
		buildDisplayName(getItem(), mDisplayName);
	}
	return mDisplayName;
}

void LLItemBridge::buildDisplayName(LLInventoryItem* item, std::string& name)
{
	if(item)
	{
		name.assign(item->getName());
	}
	else
	{
		name.assign(LLStringUtil::null);
	}
}

LLFontGL::StyleFlags LLItemBridge::getLabelStyle() const
{
	U8 font = LLFontGL::NORMAL;
	const LLViewerInventoryItem* item = getItem();

	if (get_is_item_worn(mUUID))
	{
		// llinfos << "BOLD" << llendl;
		font |= LLFontGL::BOLD;
	}
	else if(item && item->getIsLinkType())
	{
		font |= LLFontGL::ITALIC;
	}

	return (LLFontGL::StyleFlags)font;
}

std::string LLItemBridge::getLabelSuffix() const
{
	// String table is loaded before login screen and inventory items are
	// loaded after login, so LLTrans should be ready.
	static std::string NO_COPY = LLTrans::getString("no_copy");
	static std::string NO_MOD = LLTrans::getString("no_modify");
	static std::string NO_XFER = LLTrans::getString("no_transfer");
	static std::string LINK = LLTrans::getString("link");
	static std::string BROKEN_LINK = LLTrans::getString("broken_link");
	std::string suffix;
	LLInventoryItem* item = getItem();
	if(item)
	{
		// Any type can have the link suffix...
		BOOL broken_link = LLAssetType::lookupIsLinkType(item->getType());
		if (broken_link) return BROKEN_LINK;

		BOOL link = item->getIsLinkType();
		if (link) return LINK;

		// ...but it's a bit confusing to put nocopy/nomod/etc suffixes on calling cards.
		if(LLAssetType::AT_CALLINGCARD != item->getType()
		   && item->getPermissions().getOwner() == gAgent.getID())
		{
			BOOL copy = item->getPermissions().allowCopyBy(gAgent.getID());
			if (!copy)
			{
				suffix += NO_COPY;
			}
			BOOL mod = item->getPermissions().allowModifyBy(gAgent.getID());
			if (!mod)
			{
				suffix += NO_MOD;
			}
			BOOL xfer = item->getPermissions().allowOperationBy(PERM_TRANSFER,
																gAgent.getID());
			if (!xfer)
			{
				suffix += NO_XFER;
			}
		}
	}
	return suffix;
}

time_t LLItemBridge::getCreationDate() const
{
	LLViewerInventoryItem* item = getItem();
	if (item)
	{
		return item->getCreationDate();
	}
	return 0;
}


BOOL LLItemBridge::isItemRenameable() const
{
	LLViewerInventoryItem* item = getItem();
	if(item)
	{
		// (For now) Don't allow calling card rename since that may confuse users as to
		// what the calling card points to.
		if (item->getInventoryType() == LLInventoryType::IT_CALLINGCARD)
		{
			return FALSE;
		}

		if (!item->isFinished()) // EXT-8662
		{
			return FALSE;
		}

		if (isInboxFolder())
		{
			return FALSE;
		}

		return (item->getPermissions().allowModifyBy(gAgent.getID()));
	}
	return FALSE;
}

BOOL LLItemBridge::renameItem(const std::string& new_name)
{
	if(!isItemRenameable())
		return FALSE;
	LLPreview::dirty(mUUID);
	LLInventoryModel* model = getInventoryModel();
	if(!model)
		return FALSE;
	LLViewerInventoryItem* item = getItem();
	if(item && (item->getName() != new_name))
	{
		LLPointer<LLViewerInventoryItem> new_item = new LLViewerInventoryItem(item);
		new_item->rename(new_name);
		buildDisplayName(new_item, mDisplayName);
		new_item->updateServer(FALSE);
		model->updateItem(new_item);

		model->notifyObservers();
	}
	// return FALSE because we either notified observers (& therefore
	// rebuilt) or we didn't update.
	return FALSE;
}


BOOL LLItemBridge::removeItem()
{
	if(!isItemRemovable())
	{
		return FALSE;
	}

	
	// move it to the trash
	LLPreview::hide(mUUID, TRUE);
	LLInventoryModel* model = getInventoryModel();
	if(!model) return FALSE;
	const LLUUID& trash_id = model->findCategoryUUIDForType(LLFolderType::FT_TRASH);
	LLViewerInventoryItem* item = getItem();
	if (!item) return FALSE;

	// Already in trash
	if (model->isObjectDescendentOf(mUUID, trash_id)) return FALSE;

	LLNotification::Params params("ConfirmItemDeleteHasLinks");
	params.functor.function(boost::bind(&LLItemBridge::confirmRemoveItem, this, _1, _2));
	
	// Check if this item has any links.  If generic inventory linking is enabled,
	// we can't do this check because we may have items in a folder somewhere that is
	// not yet in memory, so we don't want false negatives.  (If disabled, then we 
	// know we only have links in the Outfits folder which we explicitly fetch.)
	if (!gSavedSettings.getBOOL("InventoryLinking"))
	{
		if (!item->getIsLinkType())
		{
			LLInventoryModel::cat_array_t cat_array;
			LLInventoryModel::item_array_t item_array;
			LLLinkedItemIDMatches is_linked_item_match(mUUID);
			gInventory.collectDescendentsIf(gInventory.getRootFolderID(),
											cat_array,
											item_array,
											LLInventoryModel::INCLUDE_TRASH,
											is_linked_item_match);

			const U32 num_links = cat_array.size() + item_array.size();
			if (num_links > 0)
			{
				// Warn if the user is will break any links when deleting this item.
				LLNotifications::instance().add(params);
				return FALSE;
			}
		}
	}
	
	LLNotifications::instance().forceResponse(params, 0);
	return TRUE;
}

BOOL LLItemBridge::confirmRemoveItem(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if (option != 0) return FALSE;

	LLInventoryModel* model = getInventoryModel();
	if (!model) return FALSE;

	LLViewerInventoryItem* item = getItem();
	if (!item) return FALSE;

	const LLUUID& trash_id = model->findCategoryUUIDForType(LLFolderType::FT_TRASH);
	// if item is not already in trash
	if(item && !model->isObjectDescendentOf(mUUID, trash_id))
	{
		// move to trash, and restamp
		LLInvFVBridge::changeItemParent(model, item, trash_id, TRUE);
		// delete was successful
		return TRUE;
	}
	return FALSE;
}

BOOL LLItemBridge::isItemCopyable() const
{
	LLViewerInventoryItem* item = getItem();
	if (item)
	{
		// Can't copy worn objects. DEV-15183
		if(get_is_item_worn(mUUID))
		{
			return FALSE;
		}

		// You can never copy a link.
		if (item->getIsLinkType())
		{
			return FALSE;
		}

		return item->getPermissions().allowCopyBy(gAgent.getID()) || gSavedSettings.getBOOL("InventoryLinking");
	}
	return FALSE;
}

BOOL LLItemBridge::copyToClipboard() const
{
	if(isItemCopyable())
	{
		LLInventoryClipboard::instance().add(mUUID);
		return TRUE;
	}
	return FALSE;
}

LLViewerInventoryItem* LLItemBridge::getItem() const
{
	LLViewerInventoryItem* item = NULL;
	LLInventoryModel* model = getInventoryModel();
	if(model)
	{
		item = (LLViewerInventoryItem*)model->getItem(mUUID);
	}
	return item;
}

BOOL LLItemBridge::isItemPermissive() const
{
	LLViewerInventoryItem* item = getItem();
	if(item)
	{
		return item->getIsFullPerm();
	}
	return FALSE;
}

// +=================================================+
// |        LLFolderBridge                           |
// +=================================================+

LLHandle<LLFolderBridge> LLFolderBridge::sSelf;

// Can be moved to another folder
BOOL LLFolderBridge::isItemMovable() const
{
	LLInventoryObject* obj = getInventoryObject();
	if(obj)
	{
		return (!LLFolderType::lookupIsProtectedType(((LLInventoryCategory*)obj)->getPreferredType()));
	}
	return FALSE;
}

void LLFolderBridge::selectItem()
{
}


// Iterate through a folder's children to determine if
// all the children are removable.
class LLIsItemRemovable : public LLFolderViewFunctor
{
public:
	LLIsItemRemovable() : mPassed(TRUE) {}
	virtual void doFolder(LLFolderViewFolder* folder)
	{
		mPassed &= folder->getListener()->isItemRemovable();
	}
	virtual void doItem(LLFolderViewItem* item)
	{
		mPassed &= item->getListener()->isItemRemovable();
	}
	BOOL mPassed;
};

// Can be destroyed (or moved to trash)
BOOL LLFolderBridge::isItemRemovable() const
{
	if (!get_is_category_removable(getInventoryModel(), mUUID))
	{
		return FALSE;
	}

	LLInventoryPanel* panel = mInventoryPanel.get();
	LLFolderViewFolder* folderp = dynamic_cast<LLFolderViewFolder*>(panel ? panel->getRootFolder()->getItemByID(mUUID) : NULL);
	if (folderp)
	{
		LLIsItemRemovable folder_test;
		folderp->applyFunctorToChildren(folder_test);
		if (!folder_test.mPassed)
		{
			return FALSE;
		}
	}

	return TRUE;
}

BOOL LLFolderBridge::isUpToDate() const
{
	LLInventoryModel* model = getInventoryModel();
	if(!model) return FALSE;
	LLViewerInventoryCategory* category = (LLViewerInventoryCategory*)model->getCategory(mUUID);
	if( !category )
	{
		return FALSE;
	}

	return category->getVersion() != LLViewerInventoryCategory::VERSION_UNKNOWN;
}

BOOL LLFolderBridge::isItemCopyable() const
{
	// Can copy folders to paste-as-link, but not for straight paste.
	return gSavedSettings.getBOOL("InventoryLinking");
}

BOOL LLFolderBridge::copyToClipboard() const
{
	if(isItemCopyable())
	{
		LLInventoryClipboard::instance().add(mUUID);
		return TRUE;
	}
	return FALSE;
}

BOOL LLFolderBridge::isClipboardPasteable() const
{
	if ( ! LLInvFVBridge::isClipboardPasteable() )
		return FALSE;

	// Don't allow pasting duplicates to the Calling Card/Friends subfolders, see bug EXT-1599
	if ( LLFriendCardsManager::instance().isCategoryInFriendFolder( getCategory() ) )
	{
		LLInventoryModel* model = getInventoryModel();
		if ( !model )
		{
			return FALSE;
		}

		LLDynamicArray<LLUUID> objects;
		LLInventoryClipboard::instance().retrieve(objects);
		const LLViewerInventoryCategory *current_cat = getCategory();

		// Search for the direct descendent of current Friends subfolder among all pasted items,
		// and return false if is found.
		for(S32 i = objects.count() - 1; i >= 0; --i)
		{
			const LLUUID &obj_id = objects.get(i);
			if ( LLFriendCardsManager::instance().isObjDirectDescendentOfCategory(model->getObject(obj_id), current_cat) )
			{
				return FALSE;
			}
		}

	}
	return TRUE;
}

BOOL LLFolderBridge::isClipboardPasteableAsLink() const
{
	// Check normal paste-as-link permissions
	if (!LLInvFVBridge::isClipboardPasteableAsLink())
	{
		return FALSE;
	}

	const LLInventoryModel* model = getInventoryModel();
	if (!model)
	{
		return FALSE;
	}

	const LLViewerInventoryCategory *current_cat = getCategory();
	if (current_cat)
	{
		const BOOL is_in_friend_folder = LLFriendCardsManager::instance().isCategoryInFriendFolder( current_cat );
		const LLUUID &current_cat_id = current_cat->getUUID();
		LLDynamicArray<LLUUID> objects;
		LLInventoryClipboard::instance().retrieve(objects);
		S32 count = objects.count();
		for(S32 i = 0; i < count; i++)
		{
			const LLUUID &obj_id = objects.get(i);
			const LLInventoryCategory *cat = model->getCategory(obj_id);
			if (cat)
			{
				const LLUUID &cat_id = cat->getUUID();
				// Don't allow recursive pasting
				if ((cat_id == current_cat_id) ||
					model->isObjectDescendentOf(current_cat_id, cat_id))
				{
					return FALSE;
				}
			}
			// Don't allow pasting duplicates to the Calling Card/Friends subfolders, see bug EXT-1599
			if ( is_in_friend_folder )
			{
				// If object is direct descendent of current Friends subfolder than return false.
				// Note: We can't use 'const LLInventoryCategory *cat', because it may be null
				// in case type of obj_id is LLInventoryItem.
				if ( LLFriendCardsManager::instance().isObjDirectDescendentOfCategory(model->getObject(obj_id), current_cat) )
				{
					return FALSE;
				}
			}
		}
	}
	return TRUE;

}

static BOOL can_move_to_outbox(LLInventoryItem* inv_item, std::string& tooltip_msg)
{
	// Collapse links directly to items/folders
	LLViewerInventoryItem * viewer_inv_item = (LLViewerInventoryItem *) inv_item;
	LLViewerInventoryItem * linked_item = viewer_inv_item->getLinkedItem();
	if (linked_item != NULL)
	{
		inv_item = linked_item;
	}
	
	bool allow_transfer = inv_item->getPermissions().allowOperationBy(PERM_TRANSFER, gAgent.getID());
	if (!allow_transfer)
	{
		tooltip_msg = LLTrans::getString("TooltipOutboxNoTransfer");
		return false;
	}

#if BLOCK_WORN_ITEMS_IN_OUTBOX
	bool worn = get_is_item_worn(inv_item->getUUID());
	if (worn)
	{
		tooltip_msg = LLTrans::getString("TooltipOutboxWorn");
		return false;
	}
#endif
	
	bool calling_card = (LLAssetType::AT_CALLINGCARD == inv_item->getType());
	if (calling_card)
	{
		tooltip_msg = LLTrans::getString("TooltipOutboxCallingCard");
		return false;
	}
	
	return true;
}


int get_folder_levels(LLInventoryCategory* inv_cat)
{
	LLInventoryModel::cat_array_t* cats;
	LLInventoryModel::item_array_t* items;
	gInventory.getDirectDescendentsOf(inv_cat->getUUID(), cats, items);

	int max_child_levels = 0;

	for (S32 i=0; i < cats->count(); ++i)
	{
		LLInventoryCategory* category = cats->get(i);
		max_child_levels = llmax(max_child_levels, get_folder_levels(category));
	}

	return 1 + max_child_levels;
}

int get_folder_path_length(const LLUUID& ancestor_id, const LLUUID& descendant_id)
{
	int depth = 0;

	if (ancestor_id == descendant_id) return depth;

	const LLInventoryCategory* category = gInventory.getCategory(descendant_id);

	while(category)
	{
		LLUUID parent_id = category->getParentUUID();

		if (parent_id.isNull()) break;

		depth++;

		if (parent_id == ancestor_id) return depth;

		category = gInventory.getCategory(parent_id);
	}

	llwarns << "get_folder_path_length() couldn't trace a path from the descendant to the ancestor" << llendl;
	return -1;
}

BOOL LLFolderBridge::dragCategoryIntoFolder(LLInventoryCategory* inv_cat,
											BOOL drop,
											std::string& tooltip_msg)
{

	LLInventoryModel* model = getInventoryModel();

	if (!inv_cat) return FALSE; // shouldn't happen, but in case item is incorrectly parented in which case inv_cat will be NULL
	if (!model) return FALSE;
	if (!isAgentAvatarValid()) return FALSE;
	if (!isAgentInventory()) return FALSE; // cannot drag categories into library

	LLInventoryPanel* destination_panel = mInventoryPanel.get();
	if (!destination_panel) return false;

	LLInventoryFilter* filter = destination_panel->getFilter();
	if (!filter) return false;

	const LLUUID &cat_id = inv_cat->getUUID();
	const LLUUID &current_outfit_id = model->findCategoryUUIDForType(LLFolderType::FT_CURRENT_OUTFIT, false);
	const LLUUID &outbox_id = model->findCategoryUUIDForType(LLFolderType::FT_OUTBOX, false);
	
	const BOOL move_is_into_current_outfit = (mUUID == current_outfit_id);
	const BOOL move_is_into_outbox = model->isObjectDescendentOf(mUUID, outbox_id); 
	const BOOL move_is_from_outbox = model->isObjectDescendentOf(cat_id, outbox_id);

	// check to make sure source is agent inventory, and is represented there.
	LLToolDragAndDrop::ESource source = LLToolDragAndDrop::getInstance()->getSource();
	const BOOL is_agent_inventory = (model->getCategory(cat_id) != NULL)
		&& (LLToolDragAndDrop::SOURCE_AGENT == source);

	BOOL accept = FALSE;
	if (is_agent_inventory)
	{
		const LLUUID &trash_id = model->findCategoryUUIDForType(LLFolderType::FT_TRASH, false);
		const LLUUID &landmarks_id = model->findCategoryUUIDForType(LLFolderType::FT_LANDMARK, false);

		const BOOL move_is_into_trash = (mUUID == trash_id) || model->isObjectDescendentOf(mUUID, trash_id);
		const BOOL move_is_into_outfit = getCategory() && (getCategory()->getPreferredType() == LLFolderType::FT_OUTFIT);
		const BOOL move_is_into_landmarks = (mUUID == landmarks_id) || model->isObjectDescendentOf(mUUID, landmarks_id);

		//--------------------------------------------------------------------------------
		// Determine if folder can be moved.
		//

		BOOL is_movable = TRUE;

		if (is_movable && (mUUID == cat_id))
		{
			is_movable = FALSE;
			tooltip_msg = LLTrans::getString("TooltipDragOntoSelf");
		}
		if (is_movable && (model->isObjectDescendentOf(mUUID, cat_id)))
		{
			is_movable = FALSE;
			tooltip_msg = LLTrans::getString("TooltipDragOntoOwnChild");
		}
		if (is_movable && LLFolderType::lookupIsProtectedType(inv_cat->getPreferredType()))
		{
			is_movable = FALSE;
			// tooltip?
		}
		if (is_movable && move_is_into_outfit)
		{
			is_movable = FALSE;
			// tooltip?
		}
		if (is_movable && (mUUID == model->findCategoryUUIDForType(LLFolderType::FT_FAVORITE)))
		{
			is_movable = FALSE;
			// tooltip?
		}
		
		LLInventoryModel::cat_array_t descendent_categories;
		LLInventoryModel::item_array_t descendent_items;
		if (is_movable)
		{
			model->collectDescendents(cat_id, descendent_categories, descendent_items, FALSE);
			for (S32 i=0; i < descendent_categories.count(); ++i)
			{
				LLInventoryCategory* category = descendent_categories[i];
				if(LLFolderType::lookupIsProtectedType(category->getPreferredType()))
				{
					// Can't move "special folders" (e.g. Textures Folder).
					is_movable = FALSE;
					break;
				}
			}
		}
		if (is_movable && move_is_into_trash)
		{
			for (S32 i=0; i < descendent_items.count(); ++i)
			{
				LLInventoryItem* item = descendent_items[i];
				if (get_is_item_worn(item->getUUID()))
				{
					is_movable = FALSE;
					break; // It's generally movable, but not into the trash.
				}
			}
		}
		if (is_movable && move_is_into_landmarks)
		{
			for (S32 i=0; i < descendent_items.count(); ++i)
			{
				LLViewerInventoryItem* item = descendent_items[i];

				// Don't move anything except landmarks and categories into Landmarks folder.
				// We use getType() instead of getActua;Type() to allow links to landmarks and folders.
				if (LLAssetType::AT_LANDMARK != item->getType() && LLAssetType::AT_CATEGORY != item->getType())
				{
					is_movable = FALSE;
					break; // It's generally movable, but not into Landmarks.
				}
			}
		}
		if (is_movable && move_is_into_outbox)
		{
			const int nested_folder_levels = get_folder_path_length(outbox_id, mUUID) + get_folder_levels(inv_cat);
			
			if (nested_folder_levels > gSavedSettings.getU32("InventoryOutboxMaxFolderDepth"))
			{
				tooltip_msg = LLTrans::getString("TooltipOutboxFolderLevels");
				is_movable = FALSE;
			}
			else
			{
				int dragged_folder_count = descendent_categories.count();
				int existing_item_count = 0;
				int existing_folder_count = 0;
				
				const LLViewerInventoryCategory * master_folder = model->getFirstDescendantOf(outbox_id, mUUID);
				
				if (master_folder != NULL)
				{
					if (model->isObjectDescendentOf(cat_id, master_folder->getUUID()))
					{
						// Don't use count because we're already inside the same category anyway
						dragged_folder_count = 0;
					}
					else
					{
						existing_folder_count = 1; // Include the master folder in the count!

						// If we're in the drop operation as opposed to the drag without drop, we are doing a
						// single category at a time so don't block based on the total amount of cargo data items
						if (drop)
						{
							dragged_folder_count += 1;
						}
						else
						{
							// NOTE: The cargo id's count is a total of categories AND items but we err on the side of
							//       prevention rather than letting too many folders into the hierarchy of the outbox,
							//       when we're dragging the item to a new parent
							dragged_folder_count += LLToolDragAndDrop::instance().getCargoCount();
						}
					}
					
					// Tally the total number of categories and items inside the master folder

					LLInventoryModel::cat_array_t existing_categories;
					LLInventoryModel::item_array_t existing_items;

					model->collectDescendents(master_folder->getUUID(), existing_categories, existing_items, FALSE);
					
					existing_folder_count += existing_categories.count();
					existing_item_count += existing_items.count();
				}
				else
				{
					// Assume a single category is being dragged to the outbox since we evaluate one at a time
					// when not putting them under a parent item.
					dragged_folder_count += 1;
				}

				const int nested_folder_count = existing_folder_count + dragged_folder_count;
				const int nested_item_count = existing_item_count + descendent_items.count();
				
				if (nested_folder_count > gSavedSettings.getU32("InventoryOutboxMaxFolderCount"))
				{
					tooltip_msg = LLTrans::getString("TooltipOutboxTooManyFolders");
					is_movable = FALSE;
				}
				else if (nested_item_count > gSavedSettings.getU32("InventoryOutboxMaxItemCount"))
				{
					tooltip_msg = LLTrans::getString("TooltipOutboxTooManyObjects");
					is_movable = FALSE;
				}
				
				if (is_movable == TRUE)
				{
					for (S32 i=0; i < descendent_items.count(); ++i)
					{
						LLInventoryItem* item = descendent_items[i];
						if (!can_move_to_outbox(item, tooltip_msg))
						{
							is_movable = FALSE;
							break; 
						}
					}
				}
			}
		}

		if (is_movable)
		{
			LLInventoryPanel* active_panel = LLInventoryPanel::getActiveInventoryPanel(FALSE);
			is_movable = active_panel != NULL;

			// For a folder to pass the filter all its descendants are required to pass.
			// We make this exception to allow reordering folders within an inventory panel,
			// which has a filter applied, like Recent tab for example.
			// There may be folders which are displayed because some of their descendants pass
			// the filter, but other don't, and thus remain hidden. Without this check,
			// such folders would not be allowed to be moved within a panel.
			if (destination_panel == active_panel)
			{
				is_movable = true;
			}
			else
			{
				LLFolderView* active_folder_view = NULL;

				if (is_movable)
				{
					active_folder_view = active_panel->getRootFolder();
					is_movable = active_folder_view != NULL;
				}

				if (is_movable)
				{
					// Check whether the folder being dragged from active inventory panel
					// passes the filter of the destination panel.
					is_movable = check_category(model, cat_id, active_folder_view, filter);
				}
			}
		}
		// 
		//--------------------------------------------------------------------------------

		accept = is_movable;

		if (accept && drop)
		{
			// Look for any gestures and deactivate them
			if (move_is_into_trash)
			{
				for (S32 i=0; i < descendent_items.count(); i++)
				{
					LLInventoryItem* item = descendent_items[i];
					if (item->getType() == LLAssetType::AT_GESTURE
						&& LLGestureMgr::instance().isGestureActive(item->getUUID()))
					{
						LLGestureMgr::instance().deactivateGesture(item->getUUID());
					}
				}
			}
			// if target is an outfit or current outfit folder we use link
			if (move_is_into_current_outfit || move_is_into_outfit)
			{
				if (inv_cat->getPreferredType() == LLFolderType::FT_NONE)
				{
					if (move_is_into_current_outfit)
					{
						// traverse category and add all contents to currently worn.
						BOOL append = true;
						LLAppearanceMgr::instance().wearInventoryCategory(inv_cat, false, append);
					}
					else
					{
						// Recursively create links in target outfit.
						LLInventoryModel::cat_array_t cats;
						LLInventoryModel::item_array_t items;
						model->collectDescendents(cat_id, cats, items, LLInventoryModel::EXCLUDE_TRASH);
						LLAppearanceMgr::instance().linkAll(mUUID,items,NULL);
					}
				}
				else
				{
#if SUPPORT_ENSEMBLES
					// BAP - should skip if dup.
					if (move_is_into_current_outfit)
					{
						LLAppearanceMgr::instance().addEnsembleLink(inv_cat);
					}
					else
					{
						LLPointer<LLInventoryCallback> cb = NULL;
						const std::string empty_description = "";
						link_inventory_item(
							gAgent.getID(),
							cat_id,
							mUUID,
							inv_cat->getName(),
							empty_description,
							LLAssetType::AT_LINK_FOLDER,
							cb);
					}
#endif
				}
			}
			else if (move_is_into_outbox && !move_is_from_outbox)
			{
				copy_folder_to_outbox(inv_cat, mUUID, cat_id, LLToolDragAndDrop::getOperationId());
			}
			else
			{
				if (model->isObjectDescendentOf(cat_id, model->findCategoryUUIDForType(LLFolderType::FT_INBOX, false, false)))
				{
					set_dad_inbox_object(cat_id);
				}

				// Reparent the folder and restamp children if it's moving
				// into trash.
				LLInvFVBridge::changeCategoryParent(
					model,
					(LLViewerInventoryCategory*)inv_cat,
					mUUID,
					move_is_into_trash);
			}
		}
	}
	else if (LLToolDragAndDrop::SOURCE_WORLD == source)
	{
		if (move_is_into_outbox)
		{
			tooltip_msg = LLTrans::getString("TooltipOutboxNotInInventory");
			accept = FALSE;
		}
		else
		{
			accept = move_inv_category_world_to_agent(cat_id, mUUID, drop, NULL, NULL, filter);
		}
	}
	else if (LLToolDragAndDrop::SOURCE_LIBRARY == source)
	{
		if (move_is_into_outbox)
		{
			tooltip_msg = LLTrans::getString("TooltipOutboxNotInInventory");
			accept = FALSE;
		}
		else
		{
			// Accept folders that contain complete outfits.
			accept = move_is_into_current_outfit && LLAppearanceMgr::instance().getCanMakeFolderIntoOutfit(cat_id);
		}		

		if (accept && drop)
		{
			LLAppearanceMgr::instance().wearInventoryCategory(inv_cat, true, false);
		}
	}

	return accept;
}

void warn_move_inventory(LLViewerObject* object, LLMoveInv* move_inv)
{
	const char* dialog = NULL;
	if (object->flagScripted())
	{
		dialog = "MoveInventoryFromScriptedObject";
	}
	else
	{
		dialog = "MoveInventoryFromObject";
	}
	LLNotificationsUtil::add(dialog, LLSD(), LLSD(), boost::bind(move_task_inventory_callback, _1, _2, move_inv));
}

// Move/copy all inventory items from the Contents folder of an in-world
// object to the agent's inventory, inside a given category.
BOOL move_inv_category_world_to_agent(const LLUUID& object_id,
									  const LLUUID& category_id,
									  BOOL drop,
									  void (*callback)(S32, void*),
									  void* user_data,
									  LLInventoryFilter* filter)
{
	// Make sure the object exists. If we allowed dragging from
	// anonymous objects, it would be possible to bypass
	// permissions.
	// content category has same ID as object itself
	LLViewerObject* object = gObjectList.findObject(object_id);
	if(!object)
	{
		llinfos << "Object not found for drop." << llendl;
		return FALSE;
	}

	// this folder is coming from an object, as there is only one folder in an object, the root,
	// we need to collect the entire contents and handle them as a group
	LLInventoryObject::object_list_t inventory_objects;
	object->getInventoryContents(inventory_objects);

	if (inventory_objects.empty())
	{
		llinfos << "Object contents not found for drop." << llendl;
		return FALSE;
	}

	BOOL accept = FALSE;
	BOOL is_move = FALSE;

	// coming from a task. Need to figure out if the person can
	// move/copy this item.
	LLInventoryObject::object_list_t::iterator it = inventory_objects.begin();
	LLInventoryObject::object_list_t::iterator end = inventory_objects.end();
	for ( ; it != end; ++it)
	{
		LLInventoryItem* item = dynamic_cast<LLInventoryItem*>(it->get());
		if (!item)
		{
			llwarns << "Invalid inventory item for drop" << llendl;
			continue;
		}

		// coming from a task. Need to figure out if the person can
		// move/copy this item.
		LLPermissions perm(item->getPermissions());
		if((perm.allowCopyBy(gAgent.getID(), gAgent.getGroupID())
			&& perm.allowTransferTo(gAgent.getID())))
//			|| gAgent.isGodlike())
		{
			accept = TRUE;
		}
		else if(object->permYouOwner())
		{
			// If the object cannot be copied, but the object the
			// inventory is owned by the agent, then the item can be
			// moved from the task to agent inventory.
			is_move = TRUE;
			accept = TRUE;
		}

		if (filter && accept)
		{
			accept = filter->check(item);
		}

		if (!accept)
		{
			break;
		}
	}

	if(drop && accept)
	{
		it = inventory_objects.begin();
		LLInventoryObject::object_list_t::iterator first_it = inventory_objects.begin();
		LLMoveInv* move_inv = new LLMoveInv;
		move_inv->mObjectID = object_id;
		move_inv->mCategoryID = category_id;
		move_inv->mCallback = callback;
		move_inv->mUserData = user_data;

		for ( ; it != end; ++it)
		{
			two_uuids_t two(category_id, (*it)->getUUID());
			move_inv->mMoveList.push_back(two);
		}

		if(is_move)
		{
			// Callback called from within here.
			warn_move_inventory(object, move_inv);
		}
		else
		{
			LLNotification::Params params("MoveInventoryFromObject");
			params.functor.function(boost::bind(move_task_inventory_callback, _1, _2, move_inv));
			LLNotifications::instance().forceResponse(params, 0);
		}
	}
	return accept;
}

//Used by LLFolderBridge as callback for directory recursion.
class LLRightClickInventoryFetchObserver : public LLInventoryFetchItemsObserver
{
public:
	LLRightClickInventoryFetchObserver(const uuid_vec_t& ids) :
		LLInventoryFetchItemsObserver(ids),
		mCopyItems(false)
	{ };
	LLRightClickInventoryFetchObserver(const uuid_vec_t& ids,
									   const LLUUID& cat_id, 
									   bool copy_items) :
		LLInventoryFetchItemsObserver(ids),
		mCatID(cat_id),
		mCopyItems(copy_items)
	{ };
	virtual void done()
	{
		// we've downloaded all the items, so repaint the dialog
		LLFolderBridge::staticFolderOptionsMenu();

		gInventory.removeObserver(this);
		delete this;
	}

protected:
	LLUUID mCatID;
	bool mCopyItems;

};

//Used by LLFolderBridge as callback for directory recursion.
class LLRightClickInventoryFetchDescendentsObserver : public LLInventoryFetchDescendentsObserver
{
public:
	LLRightClickInventoryFetchDescendentsObserver(const uuid_vec_t& ids,
												  bool copy_items) : 
		LLInventoryFetchDescendentsObserver(ids),
		mCopyItems(copy_items) 
	{}
	~LLRightClickInventoryFetchDescendentsObserver() {}
	virtual void done();
protected:
	bool mCopyItems;
};

void LLRightClickInventoryFetchDescendentsObserver::done()
{
	// Avoid passing a NULL-ref as mCompleteFolders.front() down to
	// gInventory.collectDescendents()
	if( mComplete.empty() )
	{
		llwarns << "LLRightClickInventoryFetchDescendentsObserver::done with empty mCompleteFolders" << llendl;
		dec_busy_count();
		gInventory.removeObserver(this);
		delete this;
		return;
	}

	// What we do here is get the complete information on the items in
	// the library, and set up an observer that will wait for that to
	// happen.
	LLInventoryModel::cat_array_t cat_array;
	LLInventoryModel::item_array_t item_array;
	gInventory.collectDescendents(mComplete.front(),
								  cat_array,
								  item_array,
								  LLInventoryModel::EXCLUDE_TRASH);
	S32 count = item_array.count();
#if 0 // HACK/TODO: Why?
	// This early causes a giant menu to get produced, and doesn't seem to be needed.
	if(!count)
	{
		llwarns << "Nothing fetched in category " << mCompleteFolders.front()
				<< llendl;
		dec_busy_count();
		gInventory.removeObserver(this);
		delete this;
		return;
	}
#endif

	uuid_vec_t ids;
	for(S32 i = 0; i < count; ++i)
	{
		ids.push_back(item_array.get(i)->getUUID());
	}

	LLRightClickInventoryFetchObserver* outfit = new LLRightClickInventoryFetchObserver(ids, mComplete.front(), mCopyItems);

	// clean up, and remove this as an observer since the call to the
	// outfit could notify observers and throw us into an infinite
	// loop.
	dec_busy_count();
	gInventory.removeObserver(this);
	delete this;

	// increment busy count and either tell the inventory to check &
	// call done, or add this object to the inventory for observation.
	inc_busy_count();

	// do the fetch
	outfit->startFetch();
	outfit->done();				//Not interested in waiting and this will be right 99% of the time.
//Uncomment the following code for laggy Inventory UI.
/*	if(outfit->isFinished())
	{
	// everything is already here - call done.
	outfit->done();
	}
	else
	{
	// it's all on it's way - add an observer, and the inventory
	// will call done for us when everything is here.
	gInventory.addObserver(outfit);
	}*/
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLInventoryWearObserver
//
// Observer for "copy and wear" operation to support knowing
// when the all of the contents have been added to inventory.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class LLInventoryCopyAndWearObserver : public LLInventoryObserver
{
public:
	LLInventoryCopyAndWearObserver(const LLUUID& cat_id, int count, bool folder_added=false) :
		mCatID(cat_id), mContentsCount(count), mFolderAdded(folder_added) {}
	virtual ~LLInventoryCopyAndWearObserver() {}
	virtual void changed(U32 mask);

protected:
	LLUUID mCatID;
	int    mContentsCount;
	bool   mFolderAdded;
};



void LLInventoryCopyAndWearObserver::changed(U32 mask)
{
	if((mask & (LLInventoryObserver::ADD)) != 0)
	{
		if (!mFolderAdded)
		{
			const std::set<LLUUID>& changed_items = gInventory.getChangedIDs();

			std::set<LLUUID>::const_iterator id_it = changed_items.begin();
			std::set<LLUUID>::const_iterator id_end = changed_items.end();
			for (;id_it != id_end; ++id_it)
			{
				if ((*id_it) == mCatID)
				{
					mFolderAdded = TRUE;
					break;
				}
			}
		}

		if (mFolderAdded)
		{
			LLViewerInventoryCategory* category = gInventory.getCategory(mCatID);
			if (NULL == category)
			{
				llwarns << "gInventory.getCategory(" << mCatID
						<< ") was NULL" << llendl;
			}
			else
			{
				if (category->getDescendentCount() ==
				    mContentsCount)
				{
					gInventory.removeObserver(this);
					LLAppearanceMgr::instance().wearInventoryCategory(category, FALSE, FALSE);
					delete this;
				}
			}
		}

	}
}



void LLFolderBridge::performAction(LLInventoryModel* model, std::string action)
{
	if ("open" == action)
	{
		LLFolderViewFolder *f = dynamic_cast<LLFolderViewFolder *>(mRoot->getItemByID(mUUID));
		if (f)
		{
			f->setOpen(TRUE);
		}
		
		return;
	}
	else if ("paste" == action)
	{
		pasteFromClipboard();
		return;
	}
	else if ("paste_link" == action)
	{
		pasteLinkFromClipboard();
		return;
	}
	else if ("properties" == action)
	{
		showProperties();
		return;
	}
	else if ("replaceoutfit" == action)
	{
		modifyOutfit(FALSE);
		return;
	}
#if SUPPORT_ENSEMBLES
	else if ("wearasensemble" == action)
	{
		LLInventoryModel* model = getInventoryModel();
		if(!model) return;
		LLViewerInventoryCategory* cat = getCategory();
		if(!cat) return;
		LLAppearanceMgr::instance().addEnsembleLink(cat,true);
		return;
	}
#endif
	else if ("addtooutfit" == action)
	{
		modifyOutfit(TRUE);
		return;
	}
	else if ("copy" == action)
	{
		copyToClipboard();
		return;
	}
	else if ("removefromoutfit" == action)
	{
		LLInventoryModel* model = getInventoryModel();
		if(!model) return;
		LLViewerInventoryCategory* cat = getCategory();
		if(!cat) return;

		remove_inventory_category_from_avatar ( cat );
		return;
	}
	else if ("purge" == action)
	{
		purgeItem(model, mUUID);
		return;
	}
	else if ("restore" == action)
	{
		restoreItem();
		return;
	}
#ifndef LL_RELEASE_FOR_DOWNLOAD
	else if ("delete_system_folder" == action)
	{
		removeSystemFolder();
	}
#endif
	else if (isMarketplaceCopyAction(action))
	{
		llinfos << "Copy folder to marketplace action!" << llendl;

		LLInventoryCategory * cat = gInventory.getCategory(mUUID);
		if (!cat) return;

		const LLUUID outbox_id = getInventoryModel()->findCategoryUUIDForType(LLFolderType::FT_OUTBOX, false, false);
		copy_folder_to_outbox(cat, outbox_id, cat->getUUID(), LLToolDragAndDrop::getOperationId());
	}
#if ENABLE_MERCHANT_SEND_TO_MARKETPLACE_CONTEXT_MENU
	else if (isMarketplaceSendAction(action))
	{
		llinfos << "Send to marketplace action!" << llendl;

		LLInventoryCategory * cat = gInventory.getCategory(mUUID);
		if (!cat) return;
		
		send_to_marketplace(cat);
	}
#endif // ENABLE_MERCHANT_SEND_TO_MARKETPLACE_CONTEXT_MENU
}

void LLFolderBridge::openItem()
{
	lldebugs << "LLFolderBridge::openItem()" << llendl;
	LLInventoryModel* model = getInventoryModel();
	if(!model) return;
	if(mUUID.isNull()) return;
	bool fetching_inventory = model->fetchDescendentsOf(mUUID);
	// Only change folder type if we have the folder contents.
	if (!fetching_inventory)
	{
		// Disabling this for now, it's causing crash when new items are added to folders
		// since folder type may change before new item item has finished processing.
		// determineFolderType();
	}
}

void LLFolderBridge::closeItem()
{
	determineFolderType();
}

void LLFolderBridge::determineFolderType()
{
	if (isUpToDate())
	{
		LLInventoryModel* model = getInventoryModel();
		LLViewerInventoryCategory* category = model->getCategory(mUUID);
		if (category)
		{
			category->determineFolderType();
		}
	}
}

BOOL LLFolderBridge::isItemRenameable() const
{
	return get_is_category_renameable(getInventoryModel(), mUUID);
}

void LLFolderBridge::restoreItem()
{
	LLViewerInventoryCategory* cat;
	cat = (LLViewerInventoryCategory*)getCategory();
	if(cat)
	{
		LLInventoryModel* model = getInventoryModel();
		const LLUUID new_parent = model->findCategoryUUIDForType(LLFolderType::assetTypeToFolderType(cat->getType()));
		// do not restamp children on restore
		LLInvFVBridge::changeCategoryParent(model, cat, new_parent, FALSE);
	}
}

LLFolderType::EType LLFolderBridge::getPreferredType() const
{
	LLFolderType::EType preferred_type = LLFolderType::FT_NONE;
	LLViewerInventoryCategory* cat = getCategory();
	if(cat)
	{
		preferred_type = cat->getPreferredType();
	}

	return preferred_type;
}

// Icons for folders are based on the preferred type
LLUIImagePtr LLFolderBridge::getIcon() const
{
	LLFolderType::EType preferred_type = LLFolderType::FT_NONE;
	LLViewerInventoryCategory* cat = getCategory();
	if(cat)
	{
		preferred_type = cat->getPreferredType();
	}
	return getIcon(preferred_type);
}

// static
LLUIImagePtr LLFolderBridge::getIcon(LLFolderType::EType preferred_type)
{
	return LLUI::getUIImage(LLViewerFolderType::lookupIconName(preferred_type, FALSE));
		/*case LLAssetType::AT_MESH:
			control = "inv_folder_mesh.tga";
			break;*/
}

LLUIImagePtr LLFolderBridge::getOpenIcon() const
{
	return LLUI::getUIImage(LLViewerFolderType::lookupIconName(getPreferredType(), TRUE));

}

BOOL LLFolderBridge::renameItem(const std::string& new_name)
{
	rename_category(getInventoryModel(), mUUID, new_name);

	// return FALSE because we either notified observers (& therefore
	// rebuilt) or we didn't update.
	return FALSE;
}

BOOL LLFolderBridge::removeItem()
{
	if(!isItemRemovable())
	{
		return FALSE;
	}
	const LLViewerInventoryCategory *cat = getCategory();
	
	LLSD payload;
	LLSD args;
	args["FOLDERNAME"] = cat->getName();

	LLNotification::Params params("ConfirmDeleteProtectedCategory");
	params.payload(payload).substitutions(args).functor.function(boost::bind(&LLFolderBridge::removeItemResponse, this, _1, _2));
	LLNotifications::instance().forceResponse(params, 0);
	return TRUE;
}


BOOL LLFolderBridge::removeSystemFolder()
{
	const LLViewerInventoryCategory *cat = getCategory();
	if (!LLFolderType::lookupIsProtectedType(cat->getPreferredType()))
	{
		return FALSE;
	}

	LLSD payload;
	LLSD args;
	args["FOLDERNAME"] = cat->getName();

	LLNotification::Params params("ConfirmDeleteProtectedCategory");
	params.payload(payload).substitutions(args).functor.function(boost::bind(&LLFolderBridge::removeItemResponse, this, _1, _2));
	{
		LLNotifications::instance().add(params);
	}
	return TRUE;
}

bool LLFolderBridge::removeItemResponse(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotification::getSelectedOption(notification, response);

	// if they choose delete, do it.  Otherwise, don't do anything
	if(option == 0) 
	{
		// move it to the trash
		LLPreview::hide(mUUID);
		remove_category(getInventoryModel(), mUUID);
		return TRUE;
	}
	return FALSE;
}

void LLFolderBridge::pasteFromClipboard()
{
	LLInventoryModel* model = getInventoryModel();
	if(model && isClipboardPasteable())
	{
		const LLUUID &current_outfit_id = model->findCategoryUUIDForType(LLFolderType::FT_CURRENT_OUTFIT, false);
		const LLUUID &outbox_id = model->findCategoryUUIDForType(LLFolderType::FT_OUTBOX, false);

		const BOOL move_is_into_current_outfit = (mUUID == current_outfit_id);
		const BOOL move_is_into_outfit = (getCategory() && getCategory()->getPreferredType()==LLFolderType::FT_OUTFIT);
		const BOOL move_is_into_outbox = model->isObjectDescendentOf(mUUID, outbox_id);

		LLDynamicArray<LLUUID> objects;
		LLInventoryClipboard::instance().retrieve(objects);

		if (move_is_into_outbox)
		{
			LLFolderViewItem * outbox_itemp = mRoot->getItemByID(mUUID);

			if (outbox_itemp)
			{
				LLToolDragAndDrop::instance().setCargoCount(objects.size());

				BOOL can_list = TRUE;

				for (LLDynamicArray<LLUUID>::const_iterator iter = objects.begin();
					(iter != objects.end()) && (can_list == TRUE);
					++iter)
				{
					const LLUUID& item_id = (*iter);
					LLInventoryItem *item = model->getItem(item_id);

					if (item)
					{
						MASK mask = 0x0;
						BOOL drop = FALSE;
						EDragAndDropType cargo_type = LLViewerAssetType::lookupDragAndDropType(item->getActualType());
						void * cargo_data = (void *) item;
						std::string tooltip_msg;

						can_list = outbox_itemp->getListener()->dragOrDrop(mask, drop, cargo_type, cargo_data, tooltip_msg);
					}
				}

				LLToolDragAndDrop::instance().resetCargoCount();

				if (can_list == FALSE)
				{
					// Notify user of failure somehow -- play error sound?  modal dialog?
					return;
				}
			}
		}

		const LLUUID parent_id(mUUID);

		for (LLDynamicArray<LLUUID>::const_iterator iter = objects.begin();
			 iter != objects.end();
			 ++iter)
		{
			const LLUUID& item_id = (*iter);

			LLInventoryItem *item = model->getItem(item_id);
			if (item)
			{
				if (move_is_into_current_outfit || move_is_into_outfit)
				{
					if (can_move_to_outfit(item, move_is_into_current_outfit))
					{
						dropToOutfit(item, move_is_into_current_outfit);
					}
				}
				else if(LLInventoryClipboard::instance().isCutMode())
				{
					// move_inventory_item() is not enough,
					//we have to update inventory locally too
					LLViewerInventoryItem* viitem = dynamic_cast<LLViewerInventoryItem*>(item);
					llassert(viitem);
					if (viitem)
					{
						changeItemParent(model, viitem, parent_id, FALSE);
					}
				}
				else
				{
					copy_inventory_item(
						gAgent.getID(),
						item->getPermissions().getOwner(),
						item->getUUID(),
						parent_id,
						std::string(),
						LLPointer<LLInventoryCallback>(NULL));
				}
			}
		}
	}
}

void LLFolderBridge::pasteLinkFromClipboard()
{
	LLInventoryModel* model = getInventoryModel();
	if(model)
	{
		const LLUUID &current_outfit_id = model->findCategoryUUIDForType(LLFolderType::FT_CURRENT_OUTFIT, false);
		const LLUUID &outbox_id = model->findCategoryUUIDForType(LLFolderType::FT_OUTBOX, false);

		const BOOL move_is_into_current_outfit = (mUUID == current_outfit_id);
		const BOOL move_is_into_outfit = (getCategory() && getCategory()->getPreferredType()==LLFolderType::FT_OUTFIT);
		const BOOL move_is_into_outbox = model->isObjectDescendentOf(mUUID, outbox_id);

		if (move_is_into_outbox)
		{
			// Notify user of failure somehow -- play error sound?  modal dialog?
			return;
		}

		const LLUUID parent_id(mUUID);

		LLDynamicArray<LLUUID> objects;
		LLInventoryClipboard::instance().retrieve(objects);
		for (LLDynamicArray<LLUUID>::const_iterator iter = objects.begin();
			 iter != objects.end();
			 ++iter)
		{
			const LLUUID &object_id = (*iter);
			if (move_is_into_current_outfit || move_is_into_outfit)
			{
				LLInventoryItem *item = model->getItem(object_id);
				if (item && can_move_to_outfit(item, move_is_into_current_outfit))
				{
					dropToOutfit(item, move_is_into_current_outfit);
				}
			}
			else if (LLInventoryCategory *cat = model->getCategory(object_id))
			{
				const std::string empty_description = "";
				link_inventory_item(
					gAgent.getID(),
					cat->getUUID(),
					parent_id,
					cat->getName(),
					empty_description,
					LLAssetType::AT_LINK_FOLDER,
					LLPointer<LLInventoryCallback>(NULL));
			}
			else if (LLInventoryItem *item = model->getItem(object_id))
			{
				link_inventory_item(
					gAgent.getID(),
					item->getLinkedUUID(),
					parent_id,
					item->getName(),
					item->getDescription(),
					LLAssetType::AT_LINK,
					LLPointer<LLInventoryCallback>(NULL));
			}
		}
	}
}

void LLFolderBridge::staticFolderOptionsMenu()
{
	LLFolderBridge* selfp = sSelf.get();

	if (selfp && selfp->mRoot)
	{
		selfp->mRoot->updateMenu();
	}
}

BOOL LLFolderBridge::checkFolderForContentsOfType(LLInventoryModel* model, LLInventoryCollectFunctor& is_type)
{
	LLInventoryModel::cat_array_t cat_array;
	LLInventoryModel::item_array_t item_array;
	model->collectDescendentsIf(mUUID,
								cat_array,
								item_array,
								LLInventoryModel::EXCLUDE_TRASH,
								is_type);
	return ((item_array.count() > 0) ? TRUE : FALSE );
}

void LLFolderBridge::buildContextMenuBaseOptions(U32 flags)
{
	LLInventoryModel* model = getInventoryModel();
	llassert(model != NULL);

	const LLUUID trash_id = model->findCategoryUUIDForType(LLFolderType::FT_TRASH);
	const LLUUID lost_and_found_id = model->findCategoryUUIDForType(LLFolderType::FT_LOST_AND_FOUND);

	if (lost_and_found_id == mUUID)
	{
		// This is the lost+found folder.
		mItems.push_back(std::string("Empty Lost And Found"));

		mDisabledItems.push_back(std::string("New Folder"));
		mDisabledItems.push_back(std::string("New Script"));
		mDisabledItems.push_back(std::string("New Note"));
		mDisabledItems.push_back(std::string("New Gesture"));
		mDisabledItems.push_back(std::string("New Clothes"));
		mDisabledItems.push_back(std::string("New Body Parts"));
	}

	if(trash_id == mUUID)
	{
		// This is the trash.
		mItems.push_back(std::string("Empty Trash"));
	}
	else if(isItemInTrash())
	{
		// This is a folder in the trash.
		mItems.clear(); // clear any items that used to exist
		addTrashContextMenuOptions(mItems, mDisabledItems);
	}
	else if(isOutboxFolder())
	{
		addOutboxContextMenuOptions(flags, mItems, mDisabledItems);
	}
	else if(isAgentInventory()) // do not allow creating in library
	{
		LLViewerInventoryCategory *cat = getCategory();
		// BAP removed protected check to re-enable standard ops in untyped folders.
		// Not sure what the right thing is to do here.
		if (!isCOFFolder() && cat && (cat->getPreferredType() != LLFolderType::FT_OUTFIT))
		{
			if (!isInboxFolder() && !isOutboxFolder()) // don't allow creation in inbox or outbox
			{
				// Do not allow to create 2-level subfolder in the Calling Card/Friends folder. EXT-694.
				if (!LLFriendCardsManager::instance().isCategoryInFriendFolder(cat))
				{
					mItems.push_back(std::string("New Folder"));
				}

				mItems.push_back(std::string("New Script"));
				mItems.push_back(std::string("New Note"));
				mItems.push_back(std::string("New Gesture"));
				mItems.push_back(std::string("New Clothes"));
				mItems.push_back(std::string("New Body Parts"));
			}
#if SUPPORT_ENSEMBLES
			// Changing folder types is an unfinished unsupported feature
			// and can lead to unexpected behavior if enabled.
			mItems.push_back(std::string("Change Type"));
			const LLViewerInventoryCategory *cat = getCategory();
			if (cat && LLFolderType::lookupIsProtectedType(cat->getPreferredType()))
			{
				mDisabledItems.push_back(std::string("Change Type"));
			}
#endif
			getClipboardEntries(false, mItems, mDisabledItems, flags);
		}
		else
		{
			// Want some but not all of the items from getClipboardEntries for outfits.
			if (cat && (cat->getPreferredType() == LLFolderType::FT_OUTFIT))
			{
				mItems.push_back(std::string("Rename"));

				addDeleteContextMenuOptions(mItems, mDisabledItems);
				// EXT-4030: disallow deletion of currently worn outfit
				const LLViewerInventoryItem *base_outfit_link = LLAppearanceMgr::instance().getBaseOutfitLink();
				if (base_outfit_link && (cat == base_outfit_link->getLinkedCategory()))
				{
					mDisabledItems.push_back(std::string("Delete"));
				}
			}
		}

		//Added by spatters to force inventory pull on right-click to display folder options correctly. 07-17-06
		mCallingCards = mWearables = FALSE;

		LLIsType is_callingcard(LLAssetType::AT_CALLINGCARD);
		if (checkFolderForContentsOfType(model, is_callingcard))
		{
			mCallingCards=TRUE;
		}

		LLFindWearables is_wearable;
		LLIsType is_object( LLAssetType::AT_OBJECT );
		LLIsType is_gesture( LLAssetType::AT_GESTURE );

		if (checkFolderForContentsOfType(model, is_wearable)  ||
			checkFolderForContentsOfType(model, is_object) ||
			checkFolderForContentsOfType(model, is_gesture) )
		{
			mWearables=TRUE;
		}
	}

	// Preemptively disable system folder removal if more than one item selected.
	if ((flags & FIRST_SELECTED_ITEM) == 0)
	{
		mDisabledItems.push_back(std::string("Delete System Folder"));
	}

	if (!isOutboxFolder())
	{
		mItems.push_back(std::string("Share"));
		if (!canShare())
		{
			mDisabledItems.push_back(std::string("Share"));
		}
	}
}

void LLFolderBridge::buildContextMenuFolderOptions(U32 flags)
{
	// Build folder specific options back up
	LLInventoryModel* model = getInventoryModel();
	if(!model) return;

	const LLInventoryCategory* category = model->getCategory(mUUID);
	if(!category) return;

	const LLUUID trash_id = model->findCategoryUUIDForType(LLFolderType::FT_TRASH);
	if (trash_id == mUUID) return;
	if (isItemInTrash()) return;
	if (!isAgentInventory()) return;
	if (isOutboxFolder()) return;

	LLFolderType::EType type = category->getPreferredType();
	const bool is_system_folder = LLFolderType::lookupIsProtectedType(type);
	// BAP change once we're no longer treating regular categories as ensembles.
	const bool is_ensemble = (type == LLFolderType::FT_NONE ||
		LLFolderType::lookupIsEnsembleType(type));

	// Only enable calling-card related options for non-system folders.
	if (!is_system_folder)
	{
		LLIsType is_callingcard(LLAssetType::AT_CALLINGCARD);
		if (mCallingCards || checkFolderForContentsOfType(model, is_callingcard))
		{
			mItems.push_back(std::string("Calling Card Separator"));
			mItems.push_back(std::string("Conference Chat Folder"));
			mItems.push_back(std::string("IM All Contacts In Folder"));
		}
	}

	if (!isItemRemovable())
	{
		mDisabledItems.push_back(std::string("Delete"));
	}

#ifndef LL_RELEASE_FOR_DOWNLOAD
	if (LLFolderType::lookupIsProtectedType(type))
	{
		mItems.push_back(std::string("Delete System Folder"));
	}
#endif

	// wearables related functionality for folders.
	//is_wearable
	LLFindWearables is_wearable;
	LLIsType is_object( LLAssetType::AT_OBJECT );
	LLIsType is_gesture( LLAssetType::AT_GESTURE );

	if (mWearables ||
		checkFolderForContentsOfType(model, is_wearable)  ||
		checkFolderForContentsOfType(model, is_object) ||
		checkFolderForContentsOfType(model, is_gesture) )
	{
		mItems.push_back(std::string("Folder Wearables Separator"));

		// Only enable add/replace outfit for non-system folders.
		if (!is_system_folder)
		{
			// Adding an outfit onto another (versus replacing) doesn't make sense.
			if (type != LLFolderType::FT_OUTFIT)
			{
				mItems.push_back(std::string("Add To Outfit"));
			}

			mItems.push_back(std::string("Replace Outfit"));
		}
		if (is_ensemble)
		{
			mItems.push_back(std::string("Wear As Ensemble"));
		}
		mItems.push_back(std::string("Remove From Outfit"));
		if (!LLAppearanceMgr::getCanRemoveFromCOF(mUUID))
		{
			mDisabledItems.push_back(std::string("Remove From Outfit"));
		}
		if (!LLAppearanceMgr::instance().getCanReplaceCOF(mUUID))
		{
			mDisabledItems.push_back(std::string("Replace Outfit"));
		}
		mItems.push_back(std::string("Outfit Separator"));
	}
}

// Flags unused
void LLFolderBridge::buildContextMenu(LLMenuGL& menu, U32 flags)
{
	sSelf.markDead();

	mItems.clear();
	mDisabledItems.clear();

	lldebugs << "LLFolderBridge::buildContextMenu()" << llendl;

	LLInventoryModel* model = getInventoryModel();
	if(!model) return;

	buildContextMenuBaseOptions(flags);

	// Add menu items that are dependent on the contents of the folder.
	LLViewerInventoryCategory* category = (LLViewerInventoryCategory *) model->getCategory(mUUID);
	if (category)
	{
		uuid_vec_t folders;
		folders.push_back(category->getUUID());

		sSelf = getHandle();
		LLRightClickInventoryFetchDescendentsObserver* fetch = new LLRightClickInventoryFetchDescendentsObserver(folders, FALSE);
		fetch->startFetch();
		inc_busy_count();
		if (fetch->isFinished())
		{
			buildContextMenuFolderOptions(flags);
		}
		else
		{
			// it's all on its way - add an observer, and the inventory will call done for us when everything is here.
			gInventory.addObserver(fetch);
		}
	}

	hide_context_entries(menu, mItems, mDisabledItems);

	// Reposition the menu, in case we're adding items to an existing menu.
	menu.needsArrange();
	menu.arrangeAndClear();
}

BOOL LLFolderBridge::hasChildren() const
{
	LLInventoryModel* model = getInventoryModel();
	if(!model) return FALSE;
	LLInventoryModel::EHasChildren has_children;
	has_children = gInventory.categoryHasChildren(mUUID);
	return has_children != LLInventoryModel::CHILDREN_NO;
}

BOOL LLFolderBridge::dragOrDrop(MASK mask, BOOL drop,
								EDragAndDropType cargo_type,
								void* cargo_data,
								std::string& tooltip_msg)
{
	LLInventoryItem* inv_item = (LLInventoryItem*)cargo_data;

	//llinfos << "LLFolderBridge::dragOrDrop()" << llendl;
	BOOL accept = FALSE;
	switch(cargo_type)
	{
		case DAD_TEXTURE:
		case DAD_SOUND:
		case DAD_CALLINGCARD:
		case DAD_LANDMARK:
		case DAD_SCRIPT:
		case DAD_CLOTHING:
		case DAD_OBJECT:
		case DAD_NOTECARD:
		case DAD_BODYPART:
		case DAD_ANIMATION:
		case DAD_GESTURE:
		case DAD_MESH:
			accept = dragItemIntoFolder(inv_item, drop, tooltip_msg);
			break;
		case DAD_LINK:
			// DAD_LINK type might mean one of two asset types: AT_LINK or AT_LINK_FOLDER.
			// If we have an item of AT_LINK_FOLDER type we should process the linked
			// category being dragged or dropped into folder.
			if (inv_item && LLAssetType::AT_LINK_FOLDER == inv_item->getActualType())
			{
				LLInventoryCategory* linked_category = gInventory.getCategory(inv_item->getLinkedUUID());
				if (linked_category)
				{
					accept = dragCategoryIntoFolder((LLInventoryCategory*)linked_category, drop, tooltip_msg);
				}
			}
			else
			{
				accept = dragItemIntoFolder(inv_item, drop, tooltip_msg);
			}
			break;
		case DAD_CATEGORY:
			if (LLFriendCardsManager::instance().isAnyFriendCategory(mUUID))
			{
				accept = FALSE;
			}
			else
			{
				accept = dragCategoryIntoFolder((LLInventoryCategory*)cargo_data, drop, tooltip_msg);
			}
			break;
		case DAD_ROOT_CATEGORY:
		case DAD_NONE:
			break;
		default:
			llwarns << "Unhandled cargo type for drag&drop " << cargo_type << llendl;
			break;
	}
	return accept;
}

LLViewerInventoryCategory* LLFolderBridge::getCategory() const
{
	LLViewerInventoryCategory* cat = NULL;
	LLInventoryModel* model = getInventoryModel();
	if(model)
	{
		cat = (LLViewerInventoryCategory*)model->getCategory(mUUID);
	}
	return cat;
}


// static
void LLFolderBridge::pasteClipboard(void* user_data)
{
	LLFolderBridge* self = (LLFolderBridge*)user_data;
	if(self) self->pasteFromClipboard();
}

void LLFolderBridge::createNewCategory(void* user_data)
{
	LLFolderBridge* bridge = (LLFolderBridge*)user_data;
	if(!bridge) return;
	LLInventoryPanel* panel = bridge->mInventoryPanel.get();
	if (!panel) return;
	LLInventoryModel* model = panel->getModel();
	if(!model) return;
	LLUUID id;
	id = model->createNewCategory(bridge->getUUID(),
								  LLFolderType::FT_NONE,
								  LLStringUtil::null);
	model->notifyObservers();

	// At this point, the bridge has probably been deleted, but the
	// view is still there.
	panel->setSelection(id, TAKE_FOCUS_YES);
}

void LLFolderBridge::createNewShirt(void* user_data)
{
	LLFolderBridge::createWearable((LLFolderBridge*)user_data, LLWearableType::WT_SHIRT);
}

void LLFolderBridge::createNewPants(void* user_data)
{
	LLFolderBridge::createWearable((LLFolderBridge*)user_data, LLWearableType::WT_PANTS);
}

void LLFolderBridge::createNewShoes(void* user_data)
{
	LLFolderBridge::createWearable((LLFolderBridge*)user_data, LLWearableType::WT_SHOES);
}

void LLFolderBridge::createNewSocks(void* user_data)
{
	LLFolderBridge::createWearable((LLFolderBridge*)user_data, LLWearableType::WT_SOCKS);
}

void LLFolderBridge::createNewJacket(void* user_data)
{
	LLFolderBridge::createWearable((LLFolderBridge*)user_data, LLWearableType::WT_JACKET);
}

void LLFolderBridge::createNewSkirt(void* user_data)
{
	LLFolderBridge::createWearable((LLFolderBridge*)user_data, LLWearableType::WT_SKIRT);
}

void LLFolderBridge::createNewGloves(void* user_data)
{
	LLFolderBridge::createWearable((LLFolderBridge*)user_data, LLWearableType::WT_GLOVES);
}

void LLFolderBridge::createNewUndershirt(void* user_data)
{
	LLFolderBridge::createWearable((LLFolderBridge*)user_data, LLWearableType::WT_UNDERSHIRT);
}

void LLFolderBridge::createNewUnderpants(void* user_data)
{
	LLFolderBridge::createWearable((LLFolderBridge*)user_data, LLWearableType::WT_UNDERPANTS);
}

void LLFolderBridge::createNewShape(void* user_data)
{
	LLFolderBridge::createWearable((LLFolderBridge*)user_data, LLWearableType::WT_SHAPE);
}

void LLFolderBridge::createNewSkin(void* user_data)
{
	LLFolderBridge::createWearable((LLFolderBridge*)user_data, LLWearableType::WT_SKIN);
}

void LLFolderBridge::createNewHair(void* user_data)
{
	LLFolderBridge::createWearable((LLFolderBridge*)user_data, LLWearableType::WT_HAIR);
}

void LLFolderBridge::createNewEyes(void* user_data)
{
	LLFolderBridge::createWearable((LLFolderBridge*)user_data, LLWearableType::WT_EYES);
}

// static
void LLFolderBridge::createWearable(LLFolderBridge* bridge, LLWearableType::EType type)
{
	if(!bridge) return;
	LLUUID parent_id = bridge->getUUID();
	LLAgentWearables::createWearable(type, false, parent_id);
}

void LLFolderBridge::modifyOutfit(BOOL append)
{
	LLInventoryModel* model = getInventoryModel();
	if(!model) return;
	LLViewerInventoryCategory* cat = getCategory();
	if(!cat) return;

	LLAppearanceMgr::instance().wearInventoryCategory( cat, FALSE, append );
}

// helper stuff
bool move_task_inventory_callback(const LLSD& notification, const LLSD& response, LLMoveInv* move_inv)
{
	LLFloaterOpenObject::LLCatAndWear* cat_and_wear = (LLFloaterOpenObject::LLCatAndWear* )move_inv->mUserData;
	LLViewerObject* object = gObjectList.findObject(move_inv->mObjectID);
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);

	if(option == 0 && object)
	{
		if (cat_and_wear && cat_and_wear->mWear) // && !cat_and_wear->mFolderResponded)
		{
			LLInventoryObject::object_list_t inventory_objects;
			object->getInventoryContents(inventory_objects);
			int contents_count = inventory_objects.size()-1; //subtract one for containing folder
			LLInventoryCopyAndWearObserver* inventoryObserver = new LLInventoryCopyAndWearObserver(cat_and_wear->mCatID, contents_count, cat_and_wear->mFolderResponded);
			
			gInventory.addObserver(inventoryObserver);
		}

		two_uuids_list_t::iterator move_it;
		for (move_it = move_inv->mMoveList.begin();
			 move_it != move_inv->mMoveList.end();
			 ++move_it)
		{
			object->moveInventory(move_it->first, move_it->second);
		}

		// update the UI.
		dialog_refresh_all();
	}

	if (move_inv->mCallback)
	{
		move_inv->mCallback(option, move_inv->mUserData);
	}

	delete move_inv;
	return false;
}

// Returns true if the item can be moved to Current Outfit or any outfit folder.
static BOOL can_move_to_outfit(LLInventoryItem* inv_item, BOOL move_is_into_current_outfit)
{
	if ((inv_item->getInventoryType() != LLInventoryType::IT_WEARABLE) &&
		(inv_item->getInventoryType() != LLInventoryType::IT_GESTURE) &&
		(inv_item->getInventoryType() != LLInventoryType::IT_ATTACHMENT) &&
		(inv_item->getInventoryType() != LLInventoryType::IT_OBJECT))
	{
		return FALSE;
	}

	if (move_is_into_current_outfit && get_is_item_worn(inv_item->getUUID()))
	{
		return FALSE;
	}

	return TRUE;
}

// Returns TRUE if item is a landmark or a link to a landmark
// and can be moved to Favorites or Landmarks folder.
static BOOL can_move_to_landmarks(LLInventoryItem* inv_item)
{
	// Need to get the linked item to know its type because LLInventoryItem::getType()
	// returns actual type AT_LINK for links, not the asset type of a linked item.
	if (LLAssetType::AT_LINK == inv_item->getType())
	{
		LLInventoryItem* linked_item = gInventory.getItem(inv_item->getLinkedUUID());
		if (linked_item)
		{
			return LLAssetType::AT_LANDMARK == linked_item->getType();
		}
	}

	return LLAssetType::AT_LANDMARK == inv_item->getType();
}

void LLFolderBridge::dropToFavorites(LLInventoryItem* inv_item)
{
	// use callback to rearrange favorite landmarks after adding
	// to have new one placed before target (on which it was dropped). See EXT-4312.
	LLPointer<AddFavoriteLandmarkCallback> cb = new AddFavoriteLandmarkCallback();
	LLInventoryPanel* panel = mInventoryPanel.get();
	LLFolderViewItem* drag_over_item = panel ? panel->getRootFolder()->getDraggingOverItem() : NULL;
	if (drag_over_item && drag_over_item->getListener())
	{
		cb.get()->setTargetLandmarkId(drag_over_item->getListener()->getUUID());
	}

	copy_inventory_item(
		gAgent.getID(),
		inv_item->getPermissions().getOwner(),
		inv_item->getUUID(),
		mUUID,
		std::string(),
		cb);
}

void LLFolderBridge::dropToOutfit(LLInventoryItem* inv_item, BOOL move_is_into_current_outfit)
{
	// BAP - should skip if dup.
	if (move_is_into_current_outfit)
	{
		LLAppearanceMgr::instance().wearItemOnAvatar(inv_item->getUUID(), true, true);
	}
	else
	{
		LLPointer<LLInventoryCallback> cb = NULL;
		link_inventory_item(
			gAgent.getID(),
			inv_item->getLinkedUUID(),
			mUUID,
			inv_item->getName(),
			inv_item->getDescription(),
			LLAssetType::AT_LINK,
			cb);
	}
}

// This is used both for testing whether an item can be dropped
// into the folder, as well as performing the actual drop, depending
// if drop == TRUE.
BOOL LLFolderBridge::dragItemIntoFolder(LLInventoryItem* inv_item,
										BOOL drop,
										std::string& tooltip_msg)
{
	LLInventoryModel* model = getInventoryModel();

	if (!model || !inv_item) return FALSE;
	if (!isAgentInventory()) return FALSE; // cannot drag into library
	if (!isAgentAvatarValid()) return FALSE;

	LLInventoryPanel* destination_panel = mInventoryPanel.get();
	if (!destination_panel) return false;

	LLInventoryFilter* filter = destination_panel->getFilter();
	if (!filter) return false;

	const LLUUID &current_outfit_id = model->findCategoryUUIDForType(LLFolderType::FT_CURRENT_OUTFIT, false);
	const LLUUID &favorites_id = model->findCategoryUUIDForType(LLFolderType::FT_FAVORITE, false);
	const LLUUID &landmarks_id = model->findCategoryUUIDForType(LLFolderType::FT_LANDMARK, false);
	const LLUUID &outbox_id = model->findCategoryUUIDForType(LLFolderType::FT_OUTBOX, false);

	const BOOL move_is_into_current_outfit = (mUUID == current_outfit_id);
	const BOOL move_is_into_favorites = (mUUID == favorites_id);
	const BOOL move_is_into_outfit = (getCategory() && getCategory()->getPreferredType()==LLFolderType::FT_OUTFIT);
	const BOOL move_is_into_landmarks = (mUUID == landmarks_id) || model->isObjectDescendentOf(mUUID, landmarks_id);
	const BOOL move_is_into_outbox = model->isObjectDescendentOf(mUUID, outbox_id);
	const BOOL move_is_from_outbox = model->isObjectDescendentOf(inv_item->getUUID(), outbox_id);

	LLToolDragAndDrop::ESource source = LLToolDragAndDrop::getInstance()->getSource();
	BOOL accept = FALSE;
	LLViewerObject* object = NULL;
	if(LLToolDragAndDrop::SOURCE_AGENT == source)
	{
		const LLUUID &trash_id = model->findCategoryUUIDForType(LLFolderType::FT_TRASH, false);

		const BOOL move_is_into_trash = (mUUID == trash_id) || model->isObjectDescendentOf(mUUID, trash_id);
		const BOOL move_is_outof_current_outfit = LLAppearanceMgr::instance().getIsInCOF(inv_item->getUUID());

		//--------------------------------------------------------------------------------
		// Determine if item can be moved.
		//

		BOOL is_movable = TRUE;

		switch (inv_item->getActualType())
		{
			case LLAssetType::AT_CATEGORY:
				is_movable = !LLFolderType::lookupIsProtectedType(((LLInventoryCategory*)inv_item)->getPreferredType());
				break;
			default:
				break;
		}
		// Can't explicitly drag things out of the COF.
		if (move_is_outof_current_outfit)
		{
			is_movable = FALSE;
		}
		if (move_is_into_trash)
		{
			is_movable &= inv_item->getIsLinkType() || !get_is_item_worn(inv_item->getUUID());
		}
		if (is_movable)
		{
			// Don't allow creating duplicates in the Calling Card/Friends
			// subfolders, see bug EXT-1599. Check is item direct descendent
			// of target folder and forbid item's movement if it so.
			// Note: isItemDirectDescendentOfCategory checks if
			// passed category is in the Calling Card/Friends folder
			is_movable &= !LLFriendCardsManager::instance().isObjDirectDescendentOfCategory(inv_item, getCategory());
		}

		// 
		//--------------------------------------------------------------------------------
		
		//--------------------------------------------------------------------------------
		// Determine if item can be moved & dropped
		//

		accept = TRUE;

		if (!is_movable) 
		{
			accept = FALSE;
		}
		else if ((mUUID == inv_item->getParentUUID()) && !move_is_into_favorites)
		{
			accept = FALSE;
		}
		else if (move_is_into_current_outfit || move_is_into_outfit)
		{
			accept = can_move_to_outfit(inv_item, move_is_into_current_outfit);
		}
		else if (move_is_into_favorites || move_is_into_landmarks)
		{
			accept = can_move_to_landmarks(inv_item);
		}
		else if (move_is_into_outbox)
		{
			accept = can_move_to_outbox(inv_item, tooltip_msg);
			
			if (accept)
			{
				const LLViewerInventoryCategory * master_folder = model->getFirstDescendantOf(outbox_id, mUUID);
				
				int existing_item_count = LLToolDragAndDrop::instance().getCargoCount();
				
				if (master_folder != NULL)
				{
					LLInventoryModel::cat_array_t existing_categories;
					LLInventoryModel::item_array_t existing_items;
					
					gInventory.collectDescendents(master_folder->getUUID(), existing_categories, existing_items, FALSE);
					
					existing_item_count += existing_items.count();
				}
				
				if (existing_item_count > gSavedSettings.getU32("InventoryOutboxMaxItemCount"))
				{
					tooltip_msg = LLTrans::getString("TooltipOutboxTooManyObjects");
					accept = FALSE;
				}
			}
		}

		LLInventoryPanel* active_panel = LLInventoryPanel::getActiveInventoryPanel(FALSE);

		// Check whether the item being dragged from active inventory panel
		// passes the filter of the destination panel.
		if (accept && active_panel)
		{
			LLFolderView* active_folder_view = active_panel->getRootFolder();
			if (!active_folder_view) return false;

			LLFolderViewItem* fv_item = active_folder_view->getItemByID(inv_item->getUUID());
			if (!fv_item) return false;

			accept = filter->check(fv_item);
		}

		if (accept && drop)
		{
			if (inv_item->getType() == LLAssetType::AT_GESTURE
				&& LLGestureMgr::instance().isGestureActive(inv_item->getUUID()) && move_is_into_trash)
			{
				LLGestureMgr::instance().deactivateGesture(inv_item->getUUID());
			}
			// If an item is being dragged between windows, unselect everything in the active window 
			// so that we don't follow the selection to its new location (which is very annoying).
			if (active_panel && (destination_panel != active_panel))
				{
					active_panel->unSelectAll();
				}

			//--------------------------------------------------------------------------------
			// Destination folder logic
			// 

			// REORDER
			// (only reorder the item in Favorites folder)
			if ((mUUID == inv_item->getParentUUID()) && move_is_into_favorites)
			{
				LLFolderViewItem* itemp = destination_panel->getRootFolder()->getDraggingOverItem();
				if (itemp)
				{
					LLUUID srcItemId = inv_item->getUUID();
					LLUUID destItemId = itemp->getListener()->getUUID();
					gInventory.rearrangeFavoriteLandmarks(srcItemId, destItemId);
				}
			}

			// FAVORITES folder
			// (copy the item)
			else if (move_is_into_favorites)
			{
				dropToFavorites(inv_item);
			}
			// CURRENT OUTFIT or OUTFIT folder
			// (link the item)
			else if (move_is_into_current_outfit || move_is_into_outfit)
			{
				dropToOutfit(inv_item, move_is_into_current_outfit);
			}
			else if (move_is_into_outbox)
			{
				if (move_is_from_outbox)
				{
					move_item_within_outbox(inv_item, mUUID, LLToolDragAndDrop::getOperationId());
				}
				else
				{
					copy_item_to_outbox(inv_item, mUUID, LLUUID::null, LLToolDragAndDrop::getOperationId());
				}
			}
			// NORMAL or TRASH folder
			// (move the item, restamp if into trash)
			else
			{
				// set up observer to select item once drag and drop from inbox is complete 
				if (gInventory.isObjectDescendentOf(inv_item->getUUID(), gInventory.findCategoryUUIDForType(LLFolderType::FT_INBOX, false, false)))
				{
					set_dad_inbox_object(inv_item->getUUID());
				}

				LLInvFVBridge::changeItemParent(
					model,
					(LLViewerInventoryItem*)inv_item,
					mUUID,
					move_is_into_trash);
			}

			// 
			//--------------------------------------------------------------------------------
		}
	}
	else if (LLToolDragAndDrop::SOURCE_WORLD == source)
	{
		// Make sure the object exists. If we allowed dragging from
		// anonymous objects, it would be possible to bypass
		// permissions.
		object = gObjectList.findObject(inv_item->getParentUUID());
		if (!object)
		{
			llinfos << "Object not found for drop." << llendl;
			return FALSE;
		}

		// coming from a task. Need to figure out if the person can
		// move/copy this item.
		LLPermissions perm(inv_item->getPermissions());
		BOOL is_move = FALSE;
		if ((perm.allowCopyBy(gAgent.getID(), gAgent.getGroupID())
			&& perm.allowTransferTo(gAgent.getID())))
			// || gAgent.isGodlike())
		{
			accept = TRUE;
		}
		else if(object->permYouOwner())
		{
			// If the object cannot be copied, but the object the
			// inventory is owned by the agent, then the item can be
			// moved from the task to agent inventory.
			is_move = TRUE;
			accept = TRUE;
		}

		// Don't allow placing an original item into Current Outfit or an outfit folder
		// because they must contain only links to wearable items.
		// *TODO: Probably we should create a link to an item if it was dragged to outfit or COF.
		if (move_is_into_current_outfit || move_is_into_outfit)
		{
			accept = FALSE;
		}
		// Don't allow to move a single item to Favorites or Landmarks
		// if it is not a landmark or a link to a landmark.
		else if ((move_is_into_favorites || move_is_into_landmarks)
				 && !can_move_to_landmarks(inv_item))
		{
			accept = FALSE;
		}
		else if (move_is_into_outbox)
		{
			tooltip_msg = LLTrans::getString("TooltipOutboxNotInInventory");
			accept = FALSE;
		}
		
		// Check whether the item being dragged from in world
		// passes the filter of the destination panel.
		if (accept)
		{
			accept = filter->check(inv_item);
		}

		if (accept && drop)
		{
			LLMoveInv* move_inv = new LLMoveInv;
			move_inv->mObjectID = inv_item->getParentUUID();
			two_uuids_t item_pair(mUUID, inv_item->getUUID());
			move_inv->mMoveList.push_back(item_pair);
			move_inv->mCallback = NULL;
			move_inv->mUserData = NULL;
			if(is_move)
			{
				warn_move_inventory(object, move_inv);
			}
			else
			{
				// store dad inventory item to select added one later. See EXT-4347
				set_dad_inventory_item(inv_item, mUUID);

				LLNotification::Params params("MoveInventoryFromObject");
				params.functor.function(boost::bind(move_task_inventory_callback, _1, _2, move_inv));
				LLNotifications::instance().forceResponse(params, 0);
			}
		}
	}
	else if(LLToolDragAndDrop::SOURCE_NOTECARD == source)
	{
		if (move_is_into_outbox)
		{
			tooltip_msg = LLTrans::getString("TooltipOutboxNotInInventory");
			accept = FALSE;
		}
		else
		{
			// Don't allow placing an original item from a notecard to Current Outfit or an outfit folder
			// because they must contain only links to wearable items.
			accept = !(move_is_into_current_outfit || move_is_into_outfit);
		}
		
		// Check whether the item being dragged from notecard
		// passes the filter of the destination panel.
		if (accept)
		{
			accept = filter->check(inv_item);
		}

		if (accept && drop)
		{
			copy_inventory_from_notecard(mUUID,  // Drop to the chosen destination folder
										 LLToolDragAndDrop::getInstance()->getObjectID(),
										 LLToolDragAndDrop::getInstance()->getSourceID(),
										 inv_item);
		}
	}
	else if(LLToolDragAndDrop::SOURCE_LIBRARY == source)
	{
		LLViewerInventoryItem* item = (LLViewerInventoryItem*)inv_item;
		if(item && item->isFinished())
		{
			accept = TRUE;

			if (move_is_into_outbox)
			{
				tooltip_msg = LLTrans::getString("TooltipOutboxNotInInventory");
				accept = FALSE;
			}
			else if (move_is_into_current_outfit || move_is_into_outfit)
			{
				accept = can_move_to_outfit(inv_item, move_is_into_current_outfit);
			}
			// Don't allow to move a single item to Favorites or Landmarks
			// if it is not a landmark or a link to a landmark.
			else if (move_is_into_favorites || move_is_into_landmarks)
			{
				accept = can_move_to_landmarks(inv_item);
			}

			LLInventoryPanel* active_panel = LLInventoryPanel::getActiveInventoryPanel(FALSE);

			// Check whether the item being dragged from the library
			// passes the filter of the destination panel.
			if (accept && active_panel)
			{
				LLFolderView* active_folder_view = active_panel->getRootFolder();
				if (!active_folder_view) return false;

				LLFolderViewItem* fv_item = active_folder_view->getItemByID(inv_item->getUUID());
				if (!fv_item) return false;

				accept = filter->check(fv_item);
			}

			if (accept && drop)
			{
				// FAVORITES folder
				// (copy the item)
				if (move_is_into_favorites)
				{
					dropToFavorites(inv_item);
				}
				// CURRENT OUTFIT or OUTFIT folder
				// (link the item)
				else if (move_is_into_current_outfit || move_is_into_outfit)
				{
					dropToOutfit(inv_item, move_is_into_current_outfit);
				}
				else
				{
					copy_inventory_item(
						gAgent.getID(),
						inv_item->getPermissions().getOwner(),
						inv_item->getUUID(),
						mUUID,
						std::string(),
						LLPointer<LLInventoryCallback>(NULL));
				}
			}
		}
	}
	else
	{
		llwarns << "unhandled drag source" << llendl;
	}
	return accept;
}

// static
bool check_category(LLInventoryModel* model,
					const LLUUID& cat_id,
					LLFolderView* active_folder_view,
					LLInventoryFilter* filter)
{
	if (!model || !active_folder_view || !filter)
		return false;

	if (!filter->checkFolder(cat_id))
	{
		return false;
	}

	LLInventoryModel::cat_array_t descendent_categories;
	LLInventoryModel::item_array_t descendent_items;
	model->collectDescendents(cat_id, descendent_categories, descendent_items, TRUE);

	S32 num_descendent_categories = descendent_categories.count();
	S32 num_descendent_items = descendent_items.count();

	if (num_descendent_categories + num_descendent_items == 0)
	{
		// Empty folder should be checked as any other folder view item.
		// If we are filtering by date the folder should not pass because
		// it doesn't have its own creation date. See LLInvFVBridge::getCreationDate().
		return check_item(cat_id, active_folder_view, filter);
	}

	for (S32 i = 0; i < num_descendent_categories; ++i)
	{
		LLInventoryCategory* category = descendent_categories[i];
		if(!check_category(model, category->getUUID(), active_folder_view, filter))
		{
			return false;
		}
	}

	for (S32 i = 0; i < num_descendent_items; ++i)
	{
		LLViewerInventoryItem* item = descendent_items[i];
		if(!check_item(item->getUUID(), active_folder_view, filter))
		{
			return false;
		}
	}

	return true;
}

// static
bool check_item(const LLUUID& item_id,
				LLFolderView* active_folder_view,
				LLInventoryFilter* filter)
{
	if (!active_folder_view || !filter) return false;

	LLFolderViewItem* fv_item = active_folder_view->getItemByID(item_id);
	if (!fv_item) return false;

	return filter->check(fv_item);
}

// +=================================================+
// |        LLTextureBridge                          |
// +=================================================+

LLUIImagePtr LLTextureBridge::getIcon() const
{
	return LLInventoryIcon::getIcon(LLAssetType::AT_TEXTURE, mInvType);
}

void LLTextureBridge::openItem()
{
	LLViewerInventoryItem* item = getItem();

	if (item)
	{
		LLInvFVBridgeAction::doAction(item->getType(),mUUID,getInventoryModel());
	}
}

bool LLTextureBridge::canSaveTexture(void)
{
	const LLInventoryModel* model = getInventoryModel();
	if(!model) 
	{
		return false;
	}
	
	const LLViewerInventoryItem *item = model->getItem(mUUID);
	if (item)
	{
		return item->checkPermissionsSet(PERM_ITEM_UNRESTRICTED);
	}
	return false;
}

void LLTextureBridge::buildContextMenu(LLMenuGL& menu, U32 flags)
{
	lldebugs << "LLTextureBridge::buildContextMenu()" << llendl;
	menuentry_vec_t items;
	menuentry_vec_t disabled_items;
	if(isItemInTrash())
	{
		addTrashContextMenuOptions(items, disabled_items);
	}	
	else if(isOutboxFolder())
	{
		addOutboxContextMenuOptions(flags, items, disabled_items);
	}
	else
	{
		items.push_back(std::string("Share"));
		if (!canShare())
		{
			disabled_items.push_back(std::string("Share"));
		}

		addOpenRightClickMenuOption(items);
		items.push_back(std::string("Properties"));

		getClipboardEntries(true, items, disabled_items, flags);

		items.push_back(std::string("Texture Separator"));
		items.push_back(std::string("Save As"));
		if (!canSaveTexture())
		{
			disabled_items.push_back(std::string("Save As"));
		}
	}
	hide_context_entries(menu, items, disabled_items);	
}

// virtual
void LLTextureBridge::performAction(LLInventoryModel* model, std::string action)
{
	if ("save_as" == action)
	{
		LLFloaterReg::showInstance("preview_texture", LLSD(mUUID), TAKE_FOCUS_YES);
		LLPreviewTexture* preview_texture = LLFloaterReg::findTypedInstance<LLPreviewTexture>("preview_texture", mUUID);
		if (preview_texture)
		{
			preview_texture->openToSave();
		}
	}
	else LLItemBridge::performAction(model, action);
}

// +=================================================+
// |        LLSoundBridge                            |
// +=================================================+

void LLSoundBridge::openItem()
{
	const LLViewerInventoryItem* item = getItem();
	if (item)
	{
		LLInvFVBridgeAction::doAction(item->getType(),mUUID,getInventoryModel());
	}
}

void LLSoundBridge::previewItem()
{
	LLViewerInventoryItem* item = getItem();
	if(item)
	{
		send_sound_trigger(item->getAssetUUID(), 1.0);
	}
}

void LLSoundBridge::openSoundPreview(void* which)
{
	LLSoundBridge *me = (LLSoundBridge *)which;
	LLFloaterReg::showInstance("preview_sound", LLSD(me->mUUID), TAKE_FOCUS_YES);
}

void LLSoundBridge::buildContextMenu(LLMenuGL& menu, U32 flags)
{
	lldebugs << "LLSoundBridge::buildContextMenu()" << llendl;
	menuentry_vec_t items;
	menuentry_vec_t disabled_items;

	if (isOutboxFolder())
	{
		addOutboxContextMenuOptions(flags, items, disabled_items);
	}
	else
	{
		if (isItemInTrash())
		{
			addTrashContextMenuOptions(items, disabled_items);
		}	
		else
		{
			items.push_back(std::string("Share"));
			if (!canShare())
			{
				disabled_items.push_back(std::string("Share"));
			}
			items.push_back(std::string("Sound Open"));
			items.push_back(std::string("Properties"));

			getClipboardEntries(true, items, disabled_items, flags);
		}

		items.push_back(std::string("Sound Separator"));
		items.push_back(std::string("Sound Play"));
	}

	hide_context_entries(menu, items, disabled_items);
}

// +=================================================+
// |        LLLandmarkBridge                         |
// +=================================================+

LLLandmarkBridge::LLLandmarkBridge(LLInventoryPanel* inventory, 
								   LLFolderView* root,
								   const LLUUID& uuid, 
								   U32 flags/* = 0x00*/) :
	LLItemBridge(inventory, root, uuid)
{
	mVisited = FALSE;
	if (flags & LLInventoryItemFlags::II_FLAGS_LANDMARK_VISITED)
	{
		mVisited = TRUE;
	}
}

LLUIImagePtr LLLandmarkBridge::getIcon() const
{
	return LLInventoryIcon::getIcon(LLAssetType::AT_LANDMARK, LLInventoryType::IT_LANDMARK, mVisited, FALSE);
}

void LLLandmarkBridge::buildContextMenu(LLMenuGL& menu, U32 flags)
{
	menuentry_vec_t items;
	menuentry_vec_t disabled_items;

	lldebugs << "LLLandmarkBridge::buildContextMenu()" << llendl;
	if(isOutboxFolder())
	{
		addOutboxContextMenuOptions(flags, items, disabled_items);
	}
	else
	{
		if(isItemInTrash())
		{
			addTrashContextMenuOptions(items, disabled_items);
		}	
		else
		{
			items.push_back(std::string("Share"));
			if (!canShare())
			{
				disabled_items.push_back(std::string("Share"));
			}
			items.push_back(std::string("Landmark Open"));
			items.push_back(std::string("Properties"));

			getClipboardEntries(true, items, disabled_items, flags);
		}

		items.push_back(std::string("Landmark Separator"));
		items.push_back(std::string("About Landmark"));
	}

	// Disable "About Landmark" menu item for
	// multiple landmarks selected. Only one landmark
	// info panel can be shown at a time.
	if ((flags & FIRST_SELECTED_ITEM) == 0)
	{
		disabled_items.push_back(std::string("About Landmark"));
	}

	hide_context_entries(menu, items, disabled_items);
}

// Convenience function for the two functions below.
void teleport_via_landmark(const LLUUID& asset_id)
{
	gAgent.teleportViaLandmark( asset_id );

	// we now automatically track the landmark you're teleporting to
	// because you'll probably arrive at a telehub instead
	LLFloaterWorldMap* floater_world_map = LLFloaterWorldMap::getInstance();
	if( floater_world_map )
	{
		floater_world_map->trackLandmark( asset_id );
	}
}

// virtual
void LLLandmarkBridge::performAction(LLInventoryModel* model, std::string action)
{
	if ("teleport" == action)
	{
		LLViewerInventoryItem* item = getItem();
		if(item)
		{
			teleport_via_landmark(item->getAssetUUID());
		}
	}
	else if ("about" == action)
	{
		LLViewerInventoryItem* item = getItem();
		if(item)
		{
			LLSD key;
			key["type"] = "landmark";
			key["id"] = item->getUUID();

			LLFloaterSidePanelContainer::showPanel("places", key);
		}
	}
	else
	{
		LLItemBridge::performAction(model, action);
	}
}

static bool open_landmark_callback(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);

	LLUUID asset_id = notification["payload"]["asset_id"].asUUID();
	if (option == 0)
	{
		teleport_via_landmark(asset_id);
	}

	return false;
}
static LLNotificationFunctorRegistration open_landmark_callback_reg("TeleportFromLandmark", open_landmark_callback);


void LLLandmarkBridge::openItem()
{
	LLViewerInventoryItem* item = getItem();

	if (item)
	{
		LLInvFVBridgeAction::doAction(item->getType(),mUUID,getInventoryModel());
	}
}


// +=================================================+
// |        LLCallingCardObserver                    |
// +=================================================+
class LLCallingCardObserver : public LLFriendObserver
{
public:
	LLCallingCardObserver(LLCallingCardBridge* bridge) : mBridgep(bridge) {}
	virtual ~LLCallingCardObserver() { mBridgep = NULL; }
	virtual void changed(U32 mask)
	{
		mBridgep->refreshFolderViewItem();
	}
protected:
	LLCallingCardBridge* mBridgep;
};

// +=================================================+
// |        LLCallingCardBridge                      |
// +=================================================+

LLCallingCardBridge::LLCallingCardBridge(LLInventoryPanel* inventory, 
										 LLFolderView* root,
										 const LLUUID& uuid ) :
	LLItemBridge(inventory, root, uuid)
{
	mObserver = new LLCallingCardObserver(this);
	LLAvatarTracker::instance().addObserver(mObserver);
}

LLCallingCardBridge::~LLCallingCardBridge()
{
	LLAvatarTracker::instance().removeObserver(mObserver);
	delete mObserver;
}

void LLCallingCardBridge::refreshFolderViewItem()
{
	LLInventoryPanel* panel = mInventoryPanel.get();
	LLFolderViewItem* itemp = panel ? panel->getRootFolder()->getItemByID(mUUID) : NULL;
	if (itemp)
	{
		itemp->refresh();
	}
}

// virtual
void LLCallingCardBridge::performAction(LLInventoryModel* model, std::string action)
{
	if ("begin_im" == action)
	{
		LLViewerInventoryItem *item = getItem();
		if (item && (item->getCreatorUUID() != gAgent.getID()) &&
			(!item->getCreatorUUID().isNull()))
		{
			std::string callingcard_name;
			gCacheName->getFullName(item->getCreatorUUID(), callingcard_name);
			// IDEVO
			LLAvatarName av_name;
			if (LLAvatarNameCache::useDisplayNames()
				&& LLAvatarNameCache::get(item->getCreatorUUID(), &av_name))
			{
				callingcard_name = av_name.mDisplayName + " (" + av_name.mUsername + ")";
			}
			LLUUID session_id = gIMMgr->addSession(callingcard_name, IM_NOTHING_SPECIAL, item->getCreatorUUID());
			if (session_id != LLUUID::null)
			{
				LLIMFloater::show(session_id);
			}
		}
	}
	else if ("lure" == action)
	{
		LLViewerInventoryItem *item = getItem();
		if (item && (item->getCreatorUUID() != gAgent.getID()) &&
			(!item->getCreatorUUID().isNull()))
		{
			LLAvatarActions::offerTeleport(item->getCreatorUUID());
		}
	}
	else LLItemBridge::performAction(model, action);
}

LLUIImagePtr LLCallingCardBridge::getIcon() const
{
	BOOL online = FALSE;
	LLViewerInventoryItem* item = getItem();
	if(item)
	{
		online = LLAvatarTracker::instance().isBuddyOnline(item->getCreatorUUID());
	}
	return LLInventoryIcon::getIcon(LLAssetType::AT_CALLINGCARD, LLInventoryType::IT_CALLINGCARD, online, FALSE);
}

std::string LLCallingCardBridge::getLabelSuffix() const
{
	LLViewerInventoryItem* item = getItem();
	if( item && LLAvatarTracker::instance().isBuddyOnline(item->getCreatorUUID()) )
	{
		return LLItemBridge::getLabelSuffix() + " (online)";
	}
	else
	{
		return LLItemBridge::getLabelSuffix();
	}
}

void LLCallingCardBridge::openItem()
{
	LLViewerInventoryItem* item = getItem();

	if (item)
	{
		LLInvFVBridgeAction::doAction(item->getType(),mUUID,getInventoryModel());
	}
/*
  LLViewerInventoryItem* item = getItem();
  if(item && !item->getCreatorUUID().isNull())
  {
  LLAvatarActions::showProfile(item->getCreatorUUID());
  }
*/
}

void LLCallingCardBridge::buildContextMenu(LLMenuGL& menu, U32 flags)
{
	lldebugs << "LLCallingCardBridge::buildContextMenu()" << llendl;
	menuentry_vec_t items;
	menuentry_vec_t disabled_items;

	if(isItemInTrash())
	{
		addTrashContextMenuOptions(items, disabled_items);
	}	
	else if(isOutboxFolder())
	{
		items.push_back(std::string("Delete"));
	}
	else
	{
		items.push_back(std::string("Share"));
		if (!canShare())
		{
			disabled_items.push_back(std::string("Share"));
		}
		addOpenRightClickMenuOption(items);
		items.push_back(std::string("Properties"));

		getClipboardEntries(true, items, disabled_items, flags);

		LLInventoryItem* item = getItem();
		BOOL good_card = (item
						  && (LLUUID::null != item->getCreatorUUID())
						  && (item->getCreatorUUID() != gAgent.getID()));
		BOOL user_online = FALSE;
		if (item)
		{
			user_online = (LLAvatarTracker::instance().isBuddyOnline(item->getCreatorUUID()));
		}
		items.push_back(std::string("Send Instant Message Separator"));
		items.push_back(std::string("Send Instant Message"));
		items.push_back(std::string("Offer Teleport..."));
		items.push_back(std::string("Conference Chat"));

		if (!good_card)
		{
			disabled_items.push_back(std::string("Send Instant Message"));
		}
		if (!good_card || !user_online)
		{
			disabled_items.push_back(std::string("Offer Teleport..."));
			disabled_items.push_back(std::string("Conference Chat"));
		}
	}
	hide_context_entries(menu, items, disabled_items);
}

BOOL LLCallingCardBridge::dragOrDrop(MASK mask, BOOL drop,
									 EDragAndDropType cargo_type,
									 void* cargo_data,
									 std::string& tooltip_msg)
{
	LLViewerInventoryItem* item = getItem();
	BOOL rv = FALSE;
	if(item)
	{
		// check the type
		switch(cargo_type)
		{
			case DAD_TEXTURE:
			case DAD_SOUND:
			case DAD_LANDMARK:
			case DAD_SCRIPT:
			case DAD_CLOTHING:
			case DAD_OBJECT:
			case DAD_NOTECARD:
			case DAD_BODYPART:
			case DAD_ANIMATION:
			case DAD_GESTURE:
			case DAD_MESH:
			{
				LLInventoryItem* inv_item = (LLInventoryItem*)cargo_data;
				const LLPermissions& perm = inv_item->getPermissions();
				if(gInventory.getItem(inv_item->getUUID())
				   && perm.allowOperationBy(PERM_TRANSFER, gAgent.getID()))
				{
					rv = TRUE;
					if(drop)
					{
						LLGiveInventory::doGiveInventoryItem(item->getCreatorUUID(),
														 (LLInventoryItem*)cargo_data);
					}
				}
				else
				{
					// It's not in the user's inventory (it's probably in
					// an object's contents), so disallow dragging it here.
					// You can't give something you don't yet have.
					rv = FALSE;
				}
				break;
			}
			case DAD_CATEGORY:
			{
				LLInventoryCategory* inv_cat = (LLInventoryCategory*)cargo_data;
				if( gInventory.getCategory( inv_cat->getUUID() ) )
				{
					rv = TRUE;
					if(drop)
					{
						LLGiveInventory::doGiveInventoryCategory(
							item->getCreatorUUID(),
							inv_cat);
					}
				}
				else
				{
					// It's not in the user's inventory (it's probably in
					// an object's contents), so disallow dragging it here.
					// You can't give something you don't yet have.
					rv = FALSE;
				}
				break;
			}
			default:
				break;
		}
	}
	return rv;
}

// +=================================================+
// |        LLNotecardBridge                         |
// +=================================================+

void LLNotecardBridge::openItem()
{
	LLViewerInventoryItem* item = getItem();
	if (item)
	{
		LLInvFVBridgeAction::doAction(item->getType(),mUUID,getInventoryModel());
	}
}

// +=================================================+
// |        LLGestureBridge                          |
// +=================================================+

LLFontGL::StyleFlags LLGestureBridge::getLabelStyle() const
{
	if( LLGestureMgr::instance().isGestureActive(mUUID) )
	{
		return LLFontGL::BOLD;
	}
	else
	{
		return LLFontGL::NORMAL;
	}
}

std::string LLGestureBridge::getLabelSuffix() const
{
	if( LLGestureMgr::instance().isGestureActive(mUUID) )
	{
		LLStringUtil::format_map_t args;
		args["[GESLABEL]"] =  LLItemBridge::getLabelSuffix();
		return  LLTrans::getString("ActiveGesture", args);
	}
	else
	{
		return LLItemBridge::getLabelSuffix();
	}
}

// virtual
void LLGestureBridge::performAction(LLInventoryModel* model, std::string action)
{
	if (isAddAction(action))
	{
		LLGestureMgr::instance().activateGesture(mUUID);

		LLViewerInventoryItem* item = gInventory.getItem(mUUID);
		if (!item) return;

		// Since we just changed the suffix to indicate (active)
		// the server doesn't need to know, just the viewer.
		gInventory.updateItem(item);
		gInventory.notifyObservers();
	}
	else if (isRemoveAction(action))
	{
		LLGestureMgr::instance().deactivateGesture(mUUID);

		LLViewerInventoryItem* item = gInventory.getItem(mUUID);
		if (!item) return;

		// Since we just changed the suffix to indicate (active)
		// the server doesn't need to know, just the viewer.
		gInventory.updateItem(item);
		gInventory.notifyObservers();
	}
	else if("play" == action)
	{
		if(!LLGestureMgr::instance().isGestureActive(mUUID))
		{
			// we need to inform server about gesture activating to be consistent with LLPreviewGesture and  LLGestureComboList.
			BOOL inform_server = TRUE;
			BOOL deactivate_similar = FALSE;
			LLGestureMgr::instance().setGestureLoadedCallback(mUUID, boost::bind(&LLGestureBridge::playGesture, mUUID));
			LLViewerInventoryItem* item = gInventory.getItem(mUUID);
			llassert(item);
			if (item)
			{
				LLGestureMgr::instance().activateGestureWithAsset(mUUID, item->getAssetUUID(), inform_server, deactivate_similar);
			}
		}
		else
		{
			playGesture(mUUID);
		}
	}
	else LLItemBridge::performAction(model, action);
}

void LLGestureBridge::openItem()
{
	LLViewerInventoryItem* item = getItem();

	if (item)
	{
		LLInvFVBridgeAction::doAction(item->getType(),mUUID,getInventoryModel());
	}
/*
  LLViewerInventoryItem* item = getItem();
  if (item)
  {
  LLPreviewGesture* preview = LLPreviewGesture::show(mUUID, LLUUID::null);
  preview->setFocus(TRUE);
  }
*/
}

BOOL LLGestureBridge::removeItem()
{
	// Grab class information locally since *this may be deleted
	// within this function.  Not a great pattern...
	const LLInventoryModel* model = getInventoryModel();
	if(!model)
	{
		return FALSE;
	}
	const LLUUID item_id = mUUID;
	
	// This will also force close the preview window, if it exists.
	// This may actually delete *this, if mUUID is in the COF.
	LLGestureMgr::instance().deactivateGesture(item_id);
	
	// If deactivateGesture deleted *this, then return out immediately.
	if (!model->getObject(item_id))
	{
		return TRUE;
	}

	return LLItemBridge::removeItem();
}

void LLGestureBridge::buildContextMenu(LLMenuGL& menu, U32 flags)
{
	lldebugs << "LLGestureBridge::buildContextMenu()" << llendl;
	menuentry_vec_t items;
	menuentry_vec_t disabled_items;
	if(isItemInTrash())
	{
		addTrashContextMenuOptions(items, disabled_items);
	}
	else if(isOutboxFolder())
	{
		items.push_back(std::string("Delete"));
	}
	else
	{
		items.push_back(std::string("Share"));
		if (!canShare())
		{
			disabled_items.push_back(std::string("Share"));
		}

		addOpenRightClickMenuOption(items);
		items.push_back(std::string("Properties"));

		getClipboardEntries(true, items, disabled_items, flags);

		items.push_back(std::string("Gesture Separator"));
		if (LLGestureMgr::instance().isGestureActive(getUUID()))
		{
			items.push_back(std::string("Deactivate"));
		}
		else
		{
			items.push_back(std::string("Activate"));
		}
	}
	hide_context_entries(menu, items, disabled_items);
}

// static
void LLGestureBridge::playGesture(const LLUUID& item_id)
{
	if (LLGestureMgr::instance().isGesturePlaying(item_id))
	{
		LLGestureMgr::instance().stopGesture(item_id);
	}
	else
	{
		LLGestureMgr::instance().playGesture(item_id);
	}
}


// +=================================================+
// |        LLAnimationBridge                        |
// +=================================================+

void LLAnimationBridge::buildContextMenu(LLMenuGL& menu, U32 flags)
{
	menuentry_vec_t items;
	menuentry_vec_t disabled_items;

	lldebugs << "LLAnimationBridge::buildContextMenu()" << llendl;
	if(isOutboxFolder())
	{
		items.push_back(std::string("Delete"));
	}
	else
	{
		if(isItemInTrash())
		{
			addTrashContextMenuOptions(items, disabled_items);
		}	
		else
		{
			items.push_back(std::string("Share"));
			if (!canShare())
			{
				disabled_items.push_back(std::string("Share"));
			}
			items.push_back(std::string("Animation Open"));
			items.push_back(std::string("Properties"));

			getClipboardEntries(true, items, disabled_items, flags);
		}

		items.push_back(std::string("Animation Separator"));
		items.push_back(std::string("Animation Play"));
		items.push_back(std::string("Animation Audition"));
	}

	hide_context_entries(menu, items, disabled_items);
}

// virtual
void LLAnimationBridge::performAction(LLInventoryModel* model, std::string action)
{
	if ((action == "playworld") || (action == "playlocal"))
	{
		if (getItem())
		{
			LLSD::String activate = "NONE";
			if ("playworld" == action) activate = "Inworld";
			if ("playlocal" == action) activate = "Locally";

			LLPreviewAnim* preview = LLFloaterReg::showTypedInstance<LLPreviewAnim>("preview_anim", LLSD(mUUID));
			if (preview)
			{
				preview->play(activate);
			}
		}
	}
	else
	{
		LLItemBridge::performAction(model, action);
	}
}

void LLAnimationBridge::openItem()
{
	LLViewerInventoryItem* item = getItem();

	if (item)
	{
		LLInvFVBridgeAction::doAction(item->getType(),mUUID,getInventoryModel());
	}
/*
  LLViewerInventoryItem* item = getItem();
  if (item)
  {
  LLFloaterReg::showInstance("preview_anim", LLSD(mUUID), TAKE_FOCUS_YES);
  }
*/
}

// +=================================================+
// |        LLObjectBridge                           |
// +=================================================+

// static
LLUUID LLObjectBridge::sContextMenuItemID;

LLObjectBridge::LLObjectBridge(LLInventoryPanel* inventory, 
							   LLFolderView* root,
							   const LLUUID& uuid, 
							   LLInventoryType::EType type, 
							   U32 flags) :
	LLItemBridge(inventory, root, uuid)
{
	mAttachPt = (flags & 0xff); // low bye of inventory flags
	mIsMultiObject = ( flags & LLInventoryItemFlags::II_FLAGS_OBJECT_HAS_MULTIPLE_ITEMS ) ?  TRUE: FALSE;
	mInvType = type;
}

LLUIImagePtr LLObjectBridge::getIcon() const
{
	return LLInventoryIcon::getIcon(LLAssetType::AT_OBJECT, mInvType, mAttachPt, mIsMultiObject);
}

LLInventoryObject* LLObjectBridge::getObject() const
{
	LLInventoryObject* object = NULL;
	LLInventoryModel* model = getInventoryModel();
	if(model)
	{
		object = (LLInventoryObject*)model->getObject(mUUID);
	}
	return object;
}

// virtual
void LLObjectBridge::performAction(LLInventoryModel* model, std::string action)
{
	if (isAddAction(action))
	{
		LLUUID object_id = mUUID;
		LLViewerInventoryItem* item;
		item = (LLViewerInventoryItem*)gInventory.getItem(object_id);
		if(item && gInventory.isObjectDescendentOf(object_id, gInventory.getRootFolderID()))
		{
			rez_attachment(item, NULL, true); // Replace if "Wear"ing.
		}
		else if(item && item->isFinished())
		{
			// must be in library. copy it to our inventory and put it on.
			LLPointer<LLInventoryCallback> cb = new RezAttachmentCallback(0);
			copy_inventory_item(
				gAgent.getID(),
				item->getPermissions().getOwner(),
				item->getUUID(),
				LLUUID::null,
				std::string(),
				cb);
		}
		gFocusMgr.setKeyboardFocus(NULL);
	}
	else if ("wear_add" == action)
	{
		LLAppearanceMgr::instance().wearItemOnAvatar(mUUID, true, false); // Don't replace if adding.
	}
	else if (isRemoveAction(action))
	{
		LLInventoryItem* item = gInventory.getItem(mUUID);
		if(item)
		{
			LLVOAvatarSelf::detachAttachmentIntoInventory(item->getLinkedUUID());
		}
	}
	else LLItemBridge::performAction(model, action);
}

void LLObjectBridge::openItem()
{
	// object double-click action is to wear/unwear object
	performAction(getInventoryModel(),
		      get_is_item_worn(mUUID) ? "detach" : "attach");
}

std::string LLObjectBridge::getLabelSuffix() const
{
	if (get_is_item_worn(mUUID))
	{
		if (!isAgentAvatarValid()) // Error condition, can't figure out attach point
		{
			return LLItemBridge::getLabelSuffix() + LLTrans::getString("worn");
		}
		std::string attachment_point_name = gAgentAvatarp->getAttachedPointName(mUUID);
		if (attachment_point_name == LLStringUtil::null) // Error condition, invalid attach point
		{
			attachment_point_name = "Invalid Attachment";
		}
		// e.g. "(worn on ...)" / "(attached to ...)"
		LLStringUtil::format_map_t args;
		args["[ATTACHMENT_POINT]"] =  LLTrans::getString(attachment_point_name);

		return LLItemBridge::getLabelSuffix() + LLTrans::getString("WornOnAttachmentPoint", args);
	}
	return LLItemBridge::getLabelSuffix();
}

void rez_attachment(LLViewerInventoryItem* item, LLViewerJointAttachment* attachment, bool replace)
{
	const LLUUID& item_id = item->getLinkedUUID();

	// Check for duplicate request.
	if (isAgentAvatarValid() &&
		(gAgentAvatarp->attachmentWasRequested(item_id) ||
		 gAgentAvatarp->isWearingAttachment(item_id)))
	{
		llwarns << "duplicate attachment request, ignoring" << llendl;
		return;
	}
	gAgentAvatarp->addAttachmentRequest(item_id);

	S32 attach_pt = 0;
	if (isAgentAvatarValid() && attachment)
	{
		for (LLVOAvatar::attachment_map_t::iterator iter = gAgentAvatarp->mAttachmentPoints.begin();
			 iter != gAgentAvatarp->mAttachmentPoints.end(); ++iter)
		{
			if (iter->second == attachment)
			{
				attach_pt = iter->first;
				break;
			}
		}
	}

	LLSD payload;
	payload["item_id"] = item_id; // Wear the base object in case this is a link.
	payload["attachment_point"] = attach_pt;
	payload["is_add"] = !replace;

	if (replace &&
		(attachment && attachment->getNumObjects() > 0))
	{
		LLNotificationsUtil::add("ReplaceAttachment", LLSD(), payload, confirm_attachment_rez);
	}
	else
	{
		LLNotifications::instance().forceResponse(LLNotification::Params("ReplaceAttachment").payload(payload), 0/*YES*/);
	}
}

bool confirm_attachment_rez(const LLSD& notification, const LLSD& response)
{
	if (!gAgentAvatarp->canAttachMoreObjects())
	{
		LLSD args;
		args["MAX_ATTACHMENTS"] = llformat("%d", MAX_AGENT_ATTACHMENTS);
		LLNotificationsUtil::add("MaxAttachmentsOnOutfit", args);
		return false;
	}

	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if (option == 0/*YES*/)
	{
		LLUUID item_id = notification["payload"]["item_id"].asUUID();
		LLViewerInventoryItem* itemp = gInventory.getItem(item_id);

		if (itemp)
		{
			/*
			{
				U8 attachment_pt = notification["payload"]["attachment_point"].asInteger();
				
				LLMessageSystem* msg = gMessageSystem;
				msg->newMessageFast(_PREHASH_RezSingleAttachmentFromInv);
				msg->nextBlockFast(_PREHASH_AgentData);
				msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
				msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
				msg->nextBlockFast(_PREHASH_ObjectData);
				msg->addUUIDFast(_PREHASH_ItemID, itemp->getUUID());
				msg->addUUIDFast(_PREHASH_OwnerID, itemp->getPermissions().getOwner());
				msg->addU8Fast(_PREHASH_AttachmentPt, attachment_pt);
				pack_permissions_slam(msg, itemp->getFlags(), itemp->getPermissions());
				msg->addStringFast(_PREHASH_Name, itemp->getName());
				msg->addStringFast(_PREHASH_Description, itemp->getDescription());
				msg->sendReliable(gAgent.getRegion()->getHost());
				return false;
			}
			*/

			// Queue up attachments to be sent in next idle tick, this way the
			// attachments are batched up all into one message versus each attachment
			// being sent in its own separate attachments message.
			U8 attachment_pt = notification["payload"]["attachment_point"].asInteger();
			BOOL is_add = notification["payload"]["is_add"].asBoolean();

			LLAttachmentsMgr::instance().addAttachment(item_id,
													   attachment_pt,
													   is_add);
		}
	}
	return false;
}
static LLNotificationFunctorRegistration confirm_replace_attachment_rez_reg("ReplaceAttachment", confirm_attachment_rez);

void LLObjectBridge::buildContextMenu(LLMenuGL& menu, U32 flags)
{
	menuentry_vec_t items;
	menuentry_vec_t disabled_items;
	if(isItemInTrash())
	{
		addTrashContextMenuOptions(items, disabled_items);
	}	
	else if(isOutboxFolder())
	{
		items.push_back(std::string("Delete"));
	}
	else
	{
		items.push_back(std::string("Share"));
		if (!canShare())
		{
			disabled_items.push_back(std::string("Share"));
		}

		items.push_back(std::string("Properties"));

		getClipboardEntries(true, items, disabled_items, flags);

		LLObjectBridge::sContextMenuItemID = mUUID;

		LLInventoryItem *item = getItem();
		if(item)
		{
			if (!isAgentAvatarValid()) return;

			if( get_is_item_worn( mUUID ) )
			{
				items.push_back(std::string("Wearable And Object Separator"));
				items.push_back(std::string("Detach From Yourself"));
			}
			else if (!isItemInTrash() && !isLinkedObjectInTrash() && !isLinkedObjectMissing() && !isCOFFolder())
			{
				items.push_back(std::string("Wearable And Object Separator"));
				items.push_back(std::string("Wearable And Object Wear"));
				items.push_back(std::string("Wearable Add"));
				items.push_back(std::string("Attach To"));
				items.push_back(std::string("Attach To HUD"));
				// commented out for DEV-32347
				//items.push_back(std::string("Restore to Last Position"));

				if (!gAgentAvatarp->canAttachMoreObjects())
				{
					disabled_items.push_back(std::string("Wearable And Object Wear"));
					disabled_items.push_back(std::string("Wearable Add"));
					disabled_items.push_back(std::string("Attach To"));
					disabled_items.push_back(std::string("Attach To HUD"));
				}
				LLMenuGL* attach_menu = menu.findChildMenuByName("Attach To", TRUE);
				LLMenuGL* attach_hud_menu = menu.findChildMenuByName("Attach To HUD", TRUE);
				if (attach_menu
					&& (attach_menu->getChildCount() == 0)
					&& attach_hud_menu
					&& (attach_hud_menu->getChildCount() == 0)
					&& isAgentAvatarValid())
				{
					for (LLVOAvatar::attachment_map_t::iterator iter = gAgentAvatarp->mAttachmentPoints.begin();
						 iter != gAgentAvatarp->mAttachmentPoints.end(); )
					{
						LLVOAvatar::attachment_map_t::iterator curiter = iter++;
						LLViewerJointAttachment* attachment = curiter->second;
						LLMenuItemCallGL::Params p;
						std::string submenu_name = attachment->getName();
						if (LLTrans::getString(submenu_name) != "")
						{
						    p.name = (" ")+LLTrans::getString(submenu_name)+" ";
						}
						else
						{
							p.name = submenu_name;
						}
						LLSD cbparams;
						cbparams["index"] = curiter->first;
						cbparams["label"] = p.name;
						p.on_click.function_name = "Inventory.AttachObject";
						p.on_click.parameter = LLSD(attachment->getName());
						p.on_enable.function_name = "Attachment.Label";
						p.on_enable.parameter = cbparams;
						LLView* parent = attachment->getIsHUDAttachment() ? attach_hud_menu : attach_menu;
						LLUICtrlFactory::create<LLMenuItemCallGL>(p, parent);
					}
				}
			}
		}
	}
	hide_context_entries(menu, items, disabled_items);
}

BOOL LLObjectBridge::renameItem(const std::string& new_name)
{
	if(!isItemRenameable())
		return FALSE;
	LLPreview::dirty(mUUID);
	LLInventoryModel* model = getInventoryModel();
	if(!model)
		return FALSE;
	LLViewerInventoryItem* item = getItem();
	if(item && (item->getName() != new_name))
	{
		LLPointer<LLViewerInventoryItem> new_item = new LLViewerInventoryItem(item);
		new_item->rename(new_name);
		buildDisplayName(new_item, mDisplayName);
		new_item->updateServer(FALSE);
		model->updateItem(new_item);

		model->notifyObservers();

		if (isAgentAvatarValid())
		{
			LLViewerObject* obj = gAgentAvatarp->getWornAttachment( item->getUUID() );
			if(obj)
			{
				LLSelectMgr::getInstance()->deselectAll();
				LLSelectMgr::getInstance()->addAsIndividual( obj, SELECT_ALL_TES, FALSE );
				LLSelectMgr::getInstance()->selectionSetObjectName( new_name );
				LLSelectMgr::getInstance()->deselectAll();
			}
		}
	}
	// return FALSE because we either notified observers (& therefore
	// rebuilt) or we didn't update.
	return FALSE;
}

// +=================================================+
// |        LLLSLTextBridge                          |
// +=================================================+

void LLLSLTextBridge::openItem()
{
	LLViewerInventoryItem* item = getItem();

	if (item)
	{
		LLInvFVBridgeAction::doAction(item->getType(),mUUID,getInventoryModel());
	}
}

// +=================================================+
// |        LLWearableBridge                         |
// +=================================================+

LLWearableBridge::LLWearableBridge(LLInventoryPanel* inventory, 
								   LLFolderView* root, 
								   const LLUUID& uuid, 
								   LLAssetType::EType asset_type, 
								   LLInventoryType::EType inv_type, 
								   LLWearableType::EType  wearable_type) :
	LLItemBridge(inventory, root, uuid),
	mAssetType( asset_type ),
	mWearableType(wearable_type)
{
	mInvType = inv_type;
}

void remove_inventory_category_from_avatar( LLInventoryCategory* category )
{
	if(!category) return;
	lldebugs << "remove_inventory_category_from_avatar( " << category->getName()
			 << " )" << llendl;


	if (gAgentCamera.cameraCustomizeAvatar())
	{
		// switching to outfit editor should automagically save any currently edited wearable
		LLFloaterSidePanelContainer::showPanel("appearance", LLSD().with("type", "edit_outfit"));
	}

	remove_inventory_category_from_avatar_step2(TRUE, category->getUUID() );
}

struct OnRemoveStruct
{
	LLUUID mUUID;
	OnRemoveStruct(const LLUUID& uuid):
		mUUID(uuid)
	{
	}
};

void remove_inventory_category_from_avatar_step2( BOOL proceed, LLUUID category_id)
{

	// Find all the wearables that are in the category's subtree.
	lldebugs << "remove_inventory_category_from_avatar_step2()" << llendl;
	if(proceed)
	{
		LLInventoryModel::cat_array_t cat_array;
		LLInventoryModel::item_array_t item_array;
		LLFindWearables is_wearable;
		gInventory.collectDescendentsIf(category_id,
										cat_array,
										item_array,
										LLInventoryModel::EXCLUDE_TRASH,
										is_wearable);
		S32 i;
		S32 wearable_count = item_array.count();

		LLInventoryModel::cat_array_t	obj_cat_array;
		LLInventoryModel::item_array_t	obj_item_array;
		LLIsType is_object( LLAssetType::AT_OBJECT );
		gInventory.collectDescendentsIf(category_id,
										obj_cat_array,
										obj_item_array,
										LLInventoryModel::EXCLUDE_TRASH,
										is_object);
		S32 obj_count = obj_item_array.count();

		// Find all gestures in this folder
		LLInventoryModel::cat_array_t	gest_cat_array;
		LLInventoryModel::item_array_t	gest_item_array;
		LLIsType is_gesture( LLAssetType::AT_GESTURE );
		gInventory.collectDescendentsIf(category_id,
										gest_cat_array,
										gest_item_array,
										LLInventoryModel::EXCLUDE_TRASH,
										is_gesture);
		S32 gest_count = gest_item_array.count();

		if (wearable_count > 0)	//Loop through wearables.  If worn, remove.
		{
			for(i = 0; i  < wearable_count; ++i)
			{
				LLViewerInventoryItem *item = item_array.get(i);
				if (item->getType() == LLAssetType::AT_BODYPART)
					continue;
				if (gAgent.isTeen() && item->isWearableType() &&
					(item->getWearableType() == LLWearableType::WT_UNDERPANTS || item->getWearableType() == LLWearableType::WT_UNDERSHIRT))
					continue;
				if (get_is_item_worn(item->getUUID()))
				{
					LLWearableList::instance().getAsset(item->getAssetUUID(),
														item->getName(),
														item->getType(),
														LLWearableBridge::onRemoveFromAvatarArrived,
														new OnRemoveStruct(item->getLinkedUUID()));
				}
			}
		}

		if (obj_count > 0)
		{
			for(i = 0; i  < obj_count; ++i)
			{
				LLViewerInventoryItem *obj_item = obj_item_array.get(i);
				if (get_is_item_worn(obj_item->getUUID()))
				{
					LLVOAvatarSelf::detachAttachmentIntoInventory(obj_item->getLinkedUUID());
				}
			}
		}

		if (gest_count > 0)
		{
			for(i = 0; i  < gest_count; ++i)
			{
				LLViewerInventoryItem *gest_item = gest_item_array.get(i);
				if (get_is_item_worn(gest_item->getUUID()))
				{
					LLGestureMgr::instance().deactivateGesture( gest_item->getLinkedUUID() );
					gInventory.updateItem( gest_item );
					gInventory.notifyObservers();
				}

			}
		}
	}
}

BOOL LLWearableBridge::renameItem(const std::string& new_name)
{
	if (get_is_item_worn(mUUID))
	{
		gAgentWearables.setWearableName( mUUID, new_name );
	}
	return LLItemBridge::renameItem(new_name);
}

std::string LLWearableBridge::getLabelSuffix() const
{
	if (get_is_item_worn(mUUID))
	{
		// e.g. "(worn)" 
		return LLItemBridge::getLabelSuffix() + LLTrans::getString("worn");
	}
	else
	{
		return LLItemBridge::getLabelSuffix();
	}
}

LLUIImagePtr LLWearableBridge::getIcon() const
{
	return LLInventoryIcon::getIcon(mAssetType, mInvType, mWearableType, FALSE);
}

// virtual
void LLWearableBridge::performAction(LLInventoryModel* model, std::string action)
{
	if (isAddAction(action))
	{
		wearOnAvatar();
	}
	else if ("wear_add" == action)
	{
		wearAddOnAvatar();
	}
	else if ("edit" == action)
	{
		editOnAvatar();
		return;
	}
	else if (isRemoveAction(action))
	{
		removeFromAvatar();
		return;
	}
	else LLItemBridge::performAction(model, action);
}

void LLWearableBridge::openItem()
{
	LLViewerInventoryItem* item = getItem();

	if (item)
	{
		LLInvFVBridgeAction::doAction(item->getType(),mUUID,getInventoryModel());
	}
}

void LLWearableBridge::buildContextMenu(LLMenuGL& menu, U32 flags)
{
	lldebugs << "LLWearableBridge::buildContextMenu()" << llendl;
	menuentry_vec_t items;
	menuentry_vec_t disabled_items;
	if(isItemInTrash())
	{
		addTrashContextMenuOptions(items, disabled_items);
	}
	else if(isOutboxFolder())
	{
		items.push_back(std::string("Delete"));
	}
	else
	{	// FWIW, it looks like SUPPRESS_OPEN_ITEM is not set anywhere
		BOOL can_open = ((flags & SUPPRESS_OPEN_ITEM) != SUPPRESS_OPEN_ITEM);

		// If we have clothing, don't add "Open" as it's the same action as "Wear"   SL-18976
		LLViewerInventoryItem* item = getItem();
		if (can_open && item)
		{
			can_open = (item->getType() != LLAssetType::AT_CLOTHING) &&
				(item->getType() != LLAssetType::AT_BODYPART);
		}
		if (isLinkedObjectMissing())
		{
			can_open = FALSE;
		}
		items.push_back(std::string("Share"));
		if (!canShare())
		{
			disabled_items.push_back(std::string("Share"));
		}
		
		if (can_open)
		{
			addOpenRightClickMenuOption(items);
		}
		else
		{
			disabled_items.push_back(std::string("Open"));
			disabled_items.push_back(std::string("Open Original"));
		}

		items.push_back(std::string("Properties"));

		getClipboardEntries(true, items, disabled_items, flags);

		items.push_back(std::string("Wearable And Object Separator"));

		items.push_back(std::string("Wearable Edit"));

		if ((flags & FIRST_SELECTED_ITEM) == 0)
		{
			disabled_items.push_back(std::string("Wearable Edit"));
		}
		// Don't allow items to be worn if their baseobj is in the trash.
		if (isLinkedObjectInTrash() || isLinkedObjectMissing() || isCOFFolder())
		{
			disabled_items.push_back(std::string("Wearable And Object Wear"));
			disabled_items.push_back(std::string("Wearable Add"));
			disabled_items.push_back(std::string("Wearable Edit"));
		}

		// Disable wear and take off based on whether the item is worn.
		if(item)
		{
			switch (item->getType())
			{
				case LLAssetType::AT_CLOTHING:
					items.push_back(std::string("Take Off"));
					// Fallthrough since clothing and bodypart share wear options
				case LLAssetType::AT_BODYPART:
					if (get_is_item_worn(item->getUUID()))
					{
						disabled_items.push_back(std::string("Wearable And Object Wear"));
						disabled_items.push_back(std::string("Wearable Add"));
					}
					else
					{
						items.push_back(std::string("Wearable And Object Wear"));
						disabled_items.push_back(std::string("Take Off"));
						disabled_items.push_back(std::string("Wearable Edit"));
					}

					if (LLWearableType::getAllowMultiwear(mWearableType))
					{
						items.push_back(std::string("Wearable Add"));
						if (gAgentWearables.getWearableCount(mWearableType) >= LLAgentWearables::MAX_CLOTHING_PER_TYPE)
						{
							disabled_items.push_back(std::string("Wearable Add"));
						}
					}
					break;
				default:
					break;
			}
		}
	}
	hide_context_entries(menu, items, disabled_items);
}

// Called from menus
// static
BOOL LLWearableBridge::canWearOnAvatar(void* user_data)
{
	LLWearableBridge* self = (LLWearableBridge*)user_data;
	if(!self) return FALSE;
	if(!self->isAgentInventory())
	{
		LLViewerInventoryItem* item = (LLViewerInventoryItem*)self->getItem();
		if(!item || !item->isFinished()) return FALSE;
	}
	return (!get_is_item_worn(self->mUUID));
}

// Called from menus
// static
void LLWearableBridge::onWearOnAvatar(void* user_data)
{
	LLWearableBridge* self = (LLWearableBridge*)user_data;
	if(!self) return;
	self->wearOnAvatar();
}

void LLWearableBridge::wearOnAvatar()
{
	// TODO: investigate wearables may not be loaded at this point EXT-8231

	LLViewerInventoryItem* item = getItem();
	if(item)
	{
		LLAppearanceMgr::instance().wearItemOnAvatar(item->getUUID(), true, true);
	}
}

void LLWearableBridge::wearAddOnAvatar()
{
	// TODO: investigate wearables may not be loaded at this point EXT-8231

	LLViewerInventoryItem* item = getItem();
	if(item)
	{
		LLAppearanceMgr::instance().wearItemOnAvatar(item->getUUID(), true, false);
	}
}

// static
void LLWearableBridge::onWearOnAvatarArrived( LLWearable* wearable, void* userdata )
{
	LLUUID* item_id = (LLUUID*) userdata;
	if(wearable)
	{
		LLViewerInventoryItem* item = NULL;
		item = (LLViewerInventoryItem*)gInventory.getItem(*item_id);
		if(item)
		{
			if(item->getAssetUUID() == wearable->getAssetID())
			{
				gAgentWearables.setWearableItem(item, wearable);
				gInventory.notifyObservers();
				//self->getFolderItem()->refreshFromRoot();
			}
			else
			{
				llinfos << "By the time wearable asset arrived, its inv item already pointed to a different asset." << llendl;
			}
		}
	}
	delete item_id;
}

// static
// BAP remove the "add" code path once everything is fully COF-ified.
void LLWearableBridge::onWearAddOnAvatarArrived( LLWearable* wearable, void* userdata )
{
	LLUUID* item_id = (LLUUID*) userdata;
	if(wearable)
	{
		LLViewerInventoryItem* item = NULL;
		item = (LLViewerInventoryItem*)gInventory.getItem(*item_id);
		if(item)
		{
			if(item->getAssetUUID() == wearable->getAssetID())
			{
				bool do_append = true;
				gAgentWearables.setWearableItem(item, wearable, do_append);
				gInventory.notifyObservers();
				//self->getFolderItem()->refreshFromRoot();
			}
			else
			{
				llinfos << "By the time wearable asset arrived, its inv item already pointed to a different asset." << llendl;
			}
		}
	}
	delete item_id;
}

// static
BOOL LLWearableBridge::canEditOnAvatar(void* user_data)
{
	LLWearableBridge* self = (LLWearableBridge*)user_data;
	if(!self) return FALSE;

	return (get_is_item_worn(self->mUUID));
}

// static
void LLWearableBridge::onEditOnAvatar(void* user_data)
{
	LLWearableBridge* self = (LLWearableBridge*)user_data;
	if(self)
	{
		self->editOnAvatar();
	}
}

void LLWearableBridge::editOnAvatar()
{
	LLAgentWearables::editWearable(mUUID);
}

// static
BOOL LLWearableBridge::canRemoveFromAvatar(void* user_data)
{
	LLWearableBridge* self = (LLWearableBridge*)user_data;
	if( self && (LLAssetType::AT_BODYPART != self->mAssetType) )
	{
		return get_is_item_worn( self->mUUID );
	}
	return FALSE;
}

// static
void LLWearableBridge::onRemoveFromAvatar(void* user_data)
{
	LLWearableBridge* self = (LLWearableBridge*)user_data;
	if(!self) return;
	if(get_is_item_worn(self->mUUID))
	{
		LLViewerInventoryItem* item = self->getItem();
		if (item)
		{
			LLUUID parent_id = item->getParentUUID();
			LLWearableList::instance().getAsset(item->getAssetUUID(),
												item->getName(),
												item->getType(),
												onRemoveFromAvatarArrived,
												new OnRemoveStruct(LLUUID(self->mUUID)));
		}
	}
}

// static
void LLWearableBridge::onRemoveFromAvatarArrived(LLWearable* wearable,
												 void* userdata)
{
	OnRemoveStruct *on_remove_struct = (OnRemoveStruct*) userdata;
	const LLUUID &item_id = gInventory.getLinkedItemID(on_remove_struct->mUUID);
	if(wearable)
	{
		if( get_is_item_worn( item_id ) )
		{
			LLWearableType::EType type = wearable->getType();

			if( !(type==LLWearableType::WT_SHAPE || type==LLWearableType::WT_SKIN || type==LLWearableType::WT_HAIR || type==LLWearableType::WT_EYES ) ) //&&
				//!((!gAgent.isTeen()) && ( type==LLWearableType::WT_UNDERPANTS || type==LLWearableType::WT_UNDERSHIRT )) )
			{
				bool do_remove_all = false;
				U32 index = gAgentWearables.getWearableIndex(wearable);
				gAgentWearables.removeWearable( type, do_remove_all, index );
			}
		}
	}

	// Find and remove this item from the COF.
	LLAppearanceMgr::instance().removeCOFItemLinks(item_id,false);
	gInventory.notifyObservers();

	delete on_remove_struct;
}

// static
void LLWearableBridge::removeAllClothesFromAvatar()
{
	// Fetch worn clothes (i.e. the ones in COF).
	LLInventoryModel::item_array_t clothing_items;
	LLInventoryModel::cat_array_t dummy;
	LLIsType is_clothing(LLAssetType::AT_CLOTHING);
	gInventory.collectDescendentsIf(LLAppearanceMgr::instance().getCOF(),
									dummy,
									clothing_items,
									LLInventoryModel::EXCLUDE_TRASH,
									is_clothing,
									false);

	// Take them off by removing from COF.
	for (LLInventoryModel::item_array_t::const_iterator it = clothing_items.begin(); it != clothing_items.end(); ++it)
	{
		LLAppearanceMgr::instance().removeItemFromAvatar((*it)->getUUID());
	}
}

// static
void LLWearableBridge::removeItemFromAvatar(LLViewerInventoryItem *item)
{
	if (item)
	{
		LLWearableList::instance().getAsset(item->getAssetUUID(),
											item->getName(),
											item->getType(),
											LLWearableBridge::onRemoveFromAvatarArrived,
											new OnRemoveStruct(item->getUUID()));
	}
}

void LLWearableBridge::removeFromAvatar()
{
	if (get_is_item_worn(mUUID))
	{
		LLViewerInventoryItem* item = getItem();
		removeItemFromAvatar(item);
	}
}


// +=================================================+
// |        LLLinkItemBridge                         |
// +=================================================+
// For broken item links

std::string LLLinkItemBridge::sPrefix("Link: ");

void LLLinkItemBridge::buildContextMenu(LLMenuGL& menu, U32 flags)
{
	// *TODO: Translate
	lldebugs << "LLLink::buildContextMenu()" << llendl;
	menuentry_vec_t items;
	menuentry_vec_t disabled_items;

	items.push_back(std::string("Find Original"));
	disabled_items.push_back(std::string("Find Original"));
	
	if(isItemInTrash())
	{
		addTrashContextMenuOptions(items, disabled_items);
	}
	else
	{
		items.push_back(std::string("Properties"));
		addDeleteContextMenuOptions(items, disabled_items);
	}
	hide_context_entries(menu, items, disabled_items);
}

// +=================================================+
// |        LLMeshBridge                             |
// +=================================================+

LLUIImagePtr LLMeshBridge::getIcon() const
{
	return LLInventoryIcon::getIcon(LLAssetType::AT_MESH, LLInventoryType::IT_MESH, 0, FALSE);
}

void LLMeshBridge::openItem()
{
	LLViewerInventoryItem* item = getItem();
	
	if (item)
	{
		// open mesh
	}
}

void LLMeshBridge::previewItem()
{
	LLViewerInventoryItem* item = getItem();
	if(item)
	{
		// preview mesh
	}
}


void LLMeshBridge::buildContextMenu(LLMenuGL& menu, U32 flags)
{
	lldebugs << "LLMeshBridge::buildContextMenu()" << llendl;
	std::vector<std::string> items;
	std::vector<std::string> disabled_items;

	if(isItemInTrash())
	{
		items.push_back(std::string("Purge Item"));
		if (!isItemRemovable())
		{
			disabled_items.push_back(std::string("Purge Item"));
		}

		items.push_back(std::string("Restore Item"));
	}
	else if(isOutboxFolder())
	{
		addOutboxContextMenuOptions(flags, items, disabled_items);
	}
	else
	{
		items.push_back(std::string("Properties"));

		getClipboardEntries(true, items, disabled_items, flags);
	}

	hide_context_entries(menu, items, disabled_items);
}


// +=================================================+
// |        LLLinkBridge                             |
// +=================================================+
// For broken folder links.
std::string LLLinkFolderBridge::sPrefix("Link: ");
LLUIImagePtr LLLinkFolderBridge::getIcon() const
{
	LLFolderType::EType folder_type = LLFolderType::FT_NONE;
	const LLInventoryObject *obj = getInventoryObject();
	if (obj)
	{
		LLViewerInventoryCategory* cat = NULL;
		LLInventoryModel* model = getInventoryModel();
		if(model)
		{
			cat = (LLViewerInventoryCategory*)model->getCategory(obj->getLinkedUUID());
			if (cat)
			{
				folder_type = cat->getPreferredType();
			}
		}
	}
	return LLFolderBridge::getIcon(folder_type);
}

void LLLinkFolderBridge::buildContextMenu(LLMenuGL& menu, U32 flags)
{
	// *TODO: Translate
	lldebugs << "LLLink::buildContextMenu()" << llendl;
	menuentry_vec_t items;
	menuentry_vec_t disabled_items;

	if (isItemInTrash())
	{
		addTrashContextMenuOptions(items, disabled_items);
	}
	else
	{
		items.push_back(std::string("Find Original"));
		addDeleteContextMenuOptions(items, disabled_items);
	}
	hide_context_entries(menu, items, disabled_items);
}
void LLLinkFolderBridge::performAction(LLInventoryModel* model, std::string action)
{
	if ("goto" == action)
	{
		gotoItem();
		return;
	}
	LLItemBridge::performAction(model,action);
}
void LLLinkFolderBridge::gotoItem()
{
	const LLUUID &cat_uuid = getFolderID();
	if (!cat_uuid.isNull())
	{
		if (LLFolderViewItem *base_folder = mRoot->getItemByID(cat_uuid))
		{
			if (LLInventoryModel* model = getInventoryModel())
			{
				model->fetchDescendentsOf(cat_uuid);
			}
			base_folder->setOpen(TRUE);
			mRoot->setSelectionFromRoot(base_folder,TRUE);
			mRoot->scrollToShowSelection();
		}
	}
}
const LLUUID &LLLinkFolderBridge::getFolderID() const
{
	if (LLViewerInventoryItem *link_item = getItem())
	{
		if (const LLViewerInventoryCategory *cat = link_item->getLinkedCategory())
		{
			const LLUUID& cat_uuid = cat->getUUID();
			return cat_uuid;
		}
	}
	return LLUUID::null;
}

/********************************************************************************
 **
 **                    BRIDGE ACTIONS
 **/

// static
void LLInvFVBridgeAction::doAction(LLAssetType::EType asset_type,
								   const LLUUID& uuid,
								   LLInventoryModel* model)
{
	// Perform indirection in case of link.
	const LLUUID& linked_uuid = gInventory.getLinkedItemID(uuid);

	LLInvFVBridgeAction* action = createAction(asset_type,linked_uuid,model);
	if(action)
	{
		action->doIt();
		delete action;
	}
}

// static
void LLInvFVBridgeAction::doAction(const LLUUID& uuid, LLInventoryModel* model)
{
	llassert(model);
	LLViewerInventoryItem* item = model->getItem(uuid);
	llassert(item);
	if (item)
	{
		LLAssetType::EType asset_type = item->getType();
		LLInvFVBridgeAction* action = createAction(asset_type,uuid,model);
		if(action)
		{
			action->doIt();
			delete action;
		}
	}
}

LLViewerInventoryItem* LLInvFVBridgeAction::getItem() const
{
	if (mModel)
		return (LLViewerInventoryItem*)mModel->getItem(mUUID);
	return NULL;
}

class LLTextureBridgeAction: public LLInvFVBridgeAction
{
	friend class LLInvFVBridgeAction;
public:
	virtual void doIt()
	{
		if (getItem())
		{
			LLFloaterReg::showInstance("preview_texture", LLSD(mUUID), TAKE_FOCUS_YES);
		}
		LLInvFVBridgeAction::doIt();
	}
	virtual ~LLTextureBridgeAction(){}
protected:
	LLTextureBridgeAction(const LLUUID& id,LLInventoryModel* model) : LLInvFVBridgeAction(id,model) {}
};

class LLSoundBridgeAction: public LLInvFVBridgeAction
{
	friend class LLInvFVBridgeAction;
public:
	virtual void doIt()
	{
		LLViewerInventoryItem* item = getItem();
		if (item)
		{
			LLFloaterReg::showInstance("preview_sound", LLSD(mUUID), TAKE_FOCUS_YES);
		}
		LLInvFVBridgeAction::doIt();
	}
	virtual ~LLSoundBridgeAction(){}
protected:
	LLSoundBridgeAction(const LLUUID& id,LLInventoryModel* model) : LLInvFVBridgeAction(id,model) {}
};

class LLLandmarkBridgeAction: public LLInvFVBridgeAction
{
	friend class LLInvFVBridgeAction;
public:
	virtual void doIt()
	{
		LLViewerInventoryItem* item = getItem();
		if (item)
		{
			// Opening (double-clicking) a landmark immediately teleports,
			// but warns you the first time.
			LLSD payload;
			payload["asset_id"] = item->getAssetUUID();		
			
			LLSD args; 
			args["LOCATION"] = item->getName(); 
			
			LLNotificationsUtil::add("TeleportFromLandmark", args, payload);
		}
		LLInvFVBridgeAction::doIt();
	}
	virtual ~LLLandmarkBridgeAction(){}
protected:
	LLLandmarkBridgeAction(const LLUUID& id,LLInventoryModel* model) : LLInvFVBridgeAction(id,model) {}
};

class LLCallingCardBridgeAction: public LLInvFVBridgeAction
{
	friend class LLInvFVBridgeAction;
public:
	virtual void doIt()
	{
		LLViewerInventoryItem* item = getItem();
		if (item && item->getCreatorUUID().notNull())
		{
			LLAvatarActions::showProfile(item->getCreatorUUID());
		}
		LLInvFVBridgeAction::doIt();
	}
	virtual ~LLCallingCardBridgeAction(){}
protected:
	LLCallingCardBridgeAction(const LLUUID& id,LLInventoryModel* model) : LLInvFVBridgeAction(id,model) {}

};

class LLNotecardBridgeAction
: public LLInvFVBridgeAction
{
	friend class LLInvFVBridgeAction;
public:
	virtual void doIt()
	{
		LLViewerInventoryItem* item = getItem();
		if (item)
		{
			LLFloaterReg::showInstance("preview_notecard", LLSD(item->getUUID()), TAKE_FOCUS_YES);
		}
		LLInvFVBridgeAction::doIt();
	}
	virtual ~LLNotecardBridgeAction(){}
protected:
	LLNotecardBridgeAction(const LLUUID& id,LLInventoryModel* model) : LLInvFVBridgeAction(id,model) {}
};

class LLGestureBridgeAction: public LLInvFVBridgeAction
{
	friend class LLInvFVBridgeAction;
public:
	virtual void doIt()
	{
		LLViewerInventoryItem* item = getItem();
		if (item)
		{
			LLPreviewGesture* preview = LLPreviewGesture::show(mUUID, LLUUID::null);
			preview->setFocus(TRUE);
		}
		LLInvFVBridgeAction::doIt();		
	}
	virtual ~LLGestureBridgeAction(){}
protected:
	LLGestureBridgeAction(const LLUUID& id,LLInventoryModel* model) : LLInvFVBridgeAction(id,model) {}
};

class LLAnimationBridgeAction: public LLInvFVBridgeAction
{
	friend class LLInvFVBridgeAction;
public:
	virtual void doIt()
	{
		LLViewerInventoryItem* item = getItem();
		if (item)
		{
			LLFloaterReg::showInstance("preview_anim", LLSD(mUUID), TAKE_FOCUS_YES);
		}
		LLInvFVBridgeAction::doIt();
	}
	virtual ~LLAnimationBridgeAction(){}
protected:
	LLAnimationBridgeAction(const LLUUID& id,LLInventoryModel* model) : LLInvFVBridgeAction(id,model) {}
};

class LLObjectBridgeAction: public LLInvFVBridgeAction
{
	friend class LLInvFVBridgeAction;
public:
	virtual void doIt()
	{
		/*
		  LLFloaterReg::showInstance("properties", mUUID);
		*/
		LLInvFVBridgeAction::doIt();
	}
	virtual ~LLObjectBridgeAction(){}
protected:
	LLObjectBridgeAction(const LLUUID& id,LLInventoryModel* model) : LLInvFVBridgeAction(id,model) {}
};

class LLLSLTextBridgeAction: public LLInvFVBridgeAction
{
	friend class LLInvFVBridgeAction;
public:
	virtual void doIt()
	{
		LLViewerInventoryItem* item = getItem();
		if (item)
		{
			LLFloaterReg::showInstance("preview_script", LLSD(mUUID), TAKE_FOCUS_YES);
		}
		LLInvFVBridgeAction::doIt();
	}
	virtual ~LLLSLTextBridgeAction(){}
protected:
	LLLSLTextBridgeAction(const LLUUID& id,LLInventoryModel* model) : LLInvFVBridgeAction(id,model) {}
};

class LLWearableBridgeAction: public LLInvFVBridgeAction
{
	friend class LLInvFVBridgeAction;
public:
	virtual void doIt()
	{
		wearOnAvatar();
	}

	virtual ~LLWearableBridgeAction(){}
protected:
	LLWearableBridgeAction(const LLUUID& id,LLInventoryModel* model) : LLInvFVBridgeAction(id,model) {}
	BOOL isItemInTrash() const;
	// return true if the item is in agent inventory. if false, it
	// must be lost or in the inventory library.
	BOOL isAgentInventory() const;
	void wearOnAvatar();
};

BOOL LLWearableBridgeAction::isItemInTrash() const
{
	if(!mModel) return FALSE;
	const LLUUID trash_id = mModel->findCategoryUUIDForType(LLFolderType::FT_TRASH);
	return mModel->isObjectDescendentOf(mUUID, trash_id);
}

BOOL LLWearableBridgeAction::isAgentInventory() const
{
	if(!mModel) return FALSE;
	if(gInventory.getRootFolderID() == mUUID) return TRUE;
	return mModel->isObjectDescendentOf(mUUID, gInventory.getRootFolderID());
}

void LLWearableBridgeAction::wearOnAvatar()
{
	// TODO: investigate wearables may not be loaded at this point EXT-8231

	LLViewerInventoryItem* item = getItem();
	if(item)
	{
		LLAppearanceMgr::instance().wearItemOnAvatar(item->getUUID(), true, true);
	}
}

LLInvFVBridgeAction* LLInvFVBridgeAction::createAction(LLAssetType::EType asset_type,
													   const LLUUID& uuid,
													   LLInventoryModel* model)
{
	LLInvFVBridgeAction* action = NULL;
	switch(asset_type)
	{
		case LLAssetType::AT_TEXTURE:
			action = new LLTextureBridgeAction(uuid,model);
			break;
		case LLAssetType::AT_SOUND:
			action = new LLSoundBridgeAction(uuid,model);
			break;
		case LLAssetType::AT_LANDMARK:
			action = new LLLandmarkBridgeAction(uuid,model);
			break;
		case LLAssetType::AT_CALLINGCARD:
			action = new LLCallingCardBridgeAction(uuid,model);
			break;
		case LLAssetType::AT_OBJECT:
			action = new LLObjectBridgeAction(uuid,model);
			break;
		case LLAssetType::AT_NOTECARD:
			action = new LLNotecardBridgeAction(uuid,model);
			break;
		case LLAssetType::AT_ANIMATION:
			action = new LLAnimationBridgeAction(uuid,model);
			break;
		case LLAssetType::AT_GESTURE:
			action = new LLGestureBridgeAction(uuid,model);
			break;
		case LLAssetType::AT_LSL_TEXT:
			action = new LLLSLTextBridgeAction(uuid,model);
			break;
		case LLAssetType::AT_CLOTHING:
		case LLAssetType::AT_BODYPART:
			action = new LLWearableBridgeAction(uuid,model);
			break;
		default:
			break;
	}
	return action;
}

/**                    Bridge Actions
 **
 ********************************************************************************/

/************************************************************************/
/* Recent Inventory Panel related classes                               */
/************************************************************************/
void LLRecentItemsFolderBridge::buildContextMenu(LLMenuGL& menu, U32 flags)
{
	LLFolderBridge::buildContextMenu(menu, flags);

	menuentry_vec_t disabled_items, items = getMenuItems();

	items.erase(std::remove(items.begin(), items.end(), std::string("New Folder")), items.end());

	hide_context_entries(menu, items, disabled_items);
}

LLInvFVBridge* LLRecentInventoryBridgeBuilder::createBridge(
	LLAssetType::EType asset_type,
	LLAssetType::EType actual_asset_type,
	LLInventoryType::EType inv_type,
	LLInventoryPanel* inventory,
	LLFolderView* root,
	const LLUUID& uuid,
	U32 flags /*= 0x00*/ ) const
{
	LLInvFVBridge* new_listener = NULL;
	switch(asset_type)
	{
	case LLAssetType::AT_CATEGORY:
		if (actual_asset_type == LLAssetType::AT_LINK_FOLDER)
		{
			// *TODO: Create a link folder handler instead if it is necessary
			new_listener = LLInventoryFVBridgeBuilder::createBridge(
				asset_type,
				actual_asset_type,
				inv_type,
				inventory,
				root,
				uuid,
				flags);
			break;
		}
		new_listener = new LLRecentItemsFolderBridge(inv_type, inventory, root, uuid);
		break;
	default:
		new_listener = LLInventoryFVBridgeBuilder::createBridge(
			asset_type,
			actual_asset_type,
			inv_type,
			inventory,
			root,
			uuid,
			flags);
	}
	return new_listener;

}


// EOF
