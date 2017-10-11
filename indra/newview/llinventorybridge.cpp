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
#include "llfavoritesbar.h" // management of favorites folder
#include "llfloateropenobject.h"
#include "llfloaterreg.h"
#include "llfloatermarketplacelistings.h"
#include "llfloateroutfitphotopreview.h"
#include "llfloatersidepanelcontainer.h"
#include "llfloaterworldmap.h"
#include "llfolderview.h"
#include "llfriendcard.h"
#include "llgesturemgr.h"
#include "llgiveinventory.h" 
#include "llfloaterimcontainer.h"
#include "llimview.h"
#include "llclipboard.h"
#include "llinventorydefines.h"
#include "llinventoryfunctions.h"
#include "llinventoryicon.h"
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
#include "llurlaction.h"
#include "llviewerassettype.h"
#include "llviewerfoldertype.h"
#include "llviewermenu.h"
#include "llviewermessage.h"
#include "llviewerobjectlist.h"
#include "llviewerregion.h"
#include "llviewerwindow.h"
#include "llvoavatarself.h"
#include "llwearablelist.h"
#include "llwearableitemslist.h"
#include "lllandmarkactions.h"
#include "llpanellandmarks.h"

#include <boost/shared_ptr.hpp>

void copy_slurl_to_clipboard_callback_inv(const std::string& slurl);

typedef std::pair<LLUUID, LLUUID> two_uuids_t;
typedef std::list<two_uuids_t> two_uuids_list_t;

const F32 SOUND_GAIN = 1.0f;

struct LLMoveInv
{
	LLUUID mObjectID;
	LLUUID mCategoryID;
	two_uuids_list_t mMoveList;
	void (*mCallback)(S32, void*);
	void* mUserData;
};

using namespace LLOldEvents;

// Function declarations
bool move_task_inventory_callback(const LLSD& notification, const LLSD& response, boost::shared_ptr<LLMoveInv>);
bool confirm_attachment_rez(const LLSD& notification, const LLSD& response);
void teleport_via_landmark(const LLUUID& asset_id);
static BOOL can_move_to_outfit(LLInventoryItem* inv_item, BOOL move_is_into_current_outfit);
static BOOL can_move_to_landmarks(LLInventoryItem* inv_item);
static bool check_category(LLInventoryModel* model,
						   const LLUUID& cat_id,
						   LLInventoryPanel* active_panel,
						   LLInventoryFilter* filter);
static bool check_item(const LLUUID& item_id,
					   LLInventoryPanel* active_panel,
					   LLInventoryFilter* filter);

// Helper functions

bool isAddAction(const std::string& action)
{
	return ("wear" == action || "attach" == action || "activate" == action);
}

bool isRemoveAction(const std::string& action)
{
	return ("take_off" == action || "detach" == action);
}

bool isMarketplaceSendAction(const std::string& action)
{
	return ("send_to_marketplace" == action);
}

// Used by LLFolderBridge as callback for directory fetching recursion
class LLRightClickInventoryFetchDescendentsObserver : public LLInventoryFetchDescendentsObserver
{
public:
	LLRightClickInventoryFetchDescendentsObserver(const uuid_vec_t& ids) : LLInventoryFetchDescendentsObserver(ids) {}
	~LLRightClickInventoryFetchDescendentsObserver() {}
	virtual void execute(bool clear_observer = false);
	virtual void done()
	{
		execute(true);
	}
};

// Used by LLFolderBridge as callback for directory content items fetching
class LLRightClickInventoryFetchObserver : public LLInventoryFetchItemsObserver
{
public:
	LLRightClickInventoryFetchObserver(const uuid_vec_t& ids) : LLInventoryFetchItemsObserver(ids) { };
	~LLRightClickInventoryFetchObserver() {}
	void execute(bool clear_observer = false)
	{
		if (clear_observer)
		{
			gInventory.removeObserver(this);
			delete this;
		}
		// we've downloaded all the items, so repaint the dialog
		LLFolderBridge::staticFolderOptionsMenu();
	}
	virtual void done()
	{
		execute(true);
	}
};

// +=================================================+
// |        LLInvFVBridge                            |
// +=================================================+

LLInvFVBridge::LLInvFVBridge(LLInventoryPanel* inventory, 
							 LLFolderView* root,
							 const LLUUID& uuid) :
	mUUID(uuid), 
	mRoot(root),
	mInvType(LLInventoryType::IT_NONE),
	mIsLink(FALSE),
	LLFolderViewModelItemInventory(inventory->getRootViewModel())
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
	if(mDisplayName.empty())
	{
		buildDisplayName();
	}
	return mDisplayName;
}

std::string LLInvFVBridge::getSearchableDescription() const
{
	const LLInventoryModel* model = getInventoryModel();
	if (model)
	{
		const LLInventoryItem *item = model->getItem(mUUID);
		if(item)
		{
			std::string desc = item->getDescription();
			LLStringUtil::toUpper(desc);
			return desc;
		}
	}
	return LLStringUtil::null;
}

std::string LLInvFVBridge::getSearchableCreatorName() const
{
	const LLInventoryModel* model = getInventoryModel();
	if (model)
	{
		const LLInventoryItem *item = model->getItem(mUUID);
		if(item)
		{
			LLAvatarName av_name;
			if (LLAvatarNameCache::get(item->getCreatorUUID(), &av_name))
			{
				std::string username = av_name.getUserName();
				LLStringUtil::toUpper(username);
				return username;
			}
		}
	}
	return LLStringUtil::null;
}

std::string LLInvFVBridge::getSearchableUUIDString() const
{
	const LLInventoryModel* model = getInventoryModel();
	if (model)
	{
		const LLViewerInventoryItem *item = model->getItem(mUUID);
		if(item && (item->getIsFullPerm() || gAgent.isGodlikeWithoutAdminMenuFakery()))
		{
			std::string uuid = item->getAssetUUID().asString();
			LLStringUtil::toUpper(uuid);
			return uuid;
		}
	}
	return LLStringUtil::null;
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
	LLInventoryObject* objectp = getInventoryObject();
	if (objectp)
	{
		return objectp->getCreationDate();
	}
	return (time_t)0;
}

void LLInvFVBridge::setCreationDate(time_t creation_date_utc)
{
	LLInventoryObject* objectp = getInventoryObject();
	if (objectp)
	{
		objectp->setCreationDate(creation_date_utc);
	}
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

BOOL LLInvFVBridge::isLibraryItem() const
{
	return gInventory.isObjectDescendentOf(getUUID(),gInventory.getLibraryRootFolderID());
}

/*virtual*/
/**
 * @brief Adds this item into clipboard storage
 */
BOOL LLInvFVBridge::cutToClipboard()
{
	const LLInventoryObject* obj = gInventory.getObject(mUUID);
	if (obj && isItemMovable() && isItemRemovable())
	{
        const LLUUID &marketplacelistings_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_MARKETPLACE_LISTINGS, false);
        const BOOL cut_from_marketplacelistings = gInventory.isObjectDescendentOf(mUUID, marketplacelistings_id);
            
        if (cut_from_marketplacelistings && (LLMarketplaceData::instance().isInActiveFolder(mUUID) ||
                                             LLMarketplaceData::instance().isListedAndActive(mUUID)))
        {
            LLUUID parent_uuid = obj->getParentUUID();
            BOOL result = perform_cutToClipboard();
            gInventory.addChangedMask(LLInventoryObserver::STRUCTURE, parent_uuid);
            return result;
        }
        else
        {
            // Otherwise just perform the cut
            return perform_cutToClipboard();
        }
    }
	return FALSE;
}

// virtual
bool LLInvFVBridge::isCutToClipboard()
{
    if (LLClipboard::instance().isCutMode())
    {
        return LLClipboard::instance().isOnClipboard(mUUID);
    }
    return false;
}

// Callback for cutToClipboard if DAMA required...
BOOL LLInvFVBridge::callback_cutToClipboard(const LLSD& notification, const LLSD& response)
{
    S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
    if (option == 0) // YES
    {
		return perform_cutToClipboard();
    }
    return FALSE;
}

BOOL LLInvFVBridge::perform_cutToClipboard()
{
	const LLInventoryObject* obj = gInventory.getObject(mUUID);
	if (obj && isItemMovable() && isItemRemovable())
	{
		LLClipboard::instance().setCutMode(true);
		return LLClipboard::instance().addToClipboard(mUUID);
	}
	return FALSE;
}

BOOL LLInvFVBridge::copyToClipboard() const
{
	const LLInventoryObject* obj = gInventory.getObject(mUUID);
	if (obj && isItemCopyable())
	{
		return LLClipboard::instance().addToClipboard(mUUID);
	}
	return FALSE;
}

void LLInvFVBridge::showProperties()
{
	if (isMarketplaceListingsFolder())
    {
        LLFloaterReg::showInstance("item_properties", LLSD().with("id",mUUID),TRUE);
        // Force it to show on top as this floater has a tendency to hide when confirmation dialog shows up
        LLFloater* floater_properties = LLFloaterReg::findInstance("item_properties", LLSD().with("id",mUUID));
        if (floater_properties)
        {
            floater_properties->setVisibleAndFrontmost();
        }
    }
    else
    {
        show_item_profile(mUUID);
    }
}

void LLInvFVBridge::removeBatch(std::vector<LLFolderViewModelItem*>& batch)
{
	// Deactivate gestures when moving them into Trash
	LLInvFVBridge* bridge;
	LLInventoryModel* model = getInventoryModel();
	LLViewerInventoryItem* item = NULL;
	LLViewerInventoryCategory* cat = NULL;
	LLInventoryModel::cat_array_t	descendent_categories;
	LLInventoryModel::item_array_t	descendent_items;
	S32 count = batch.size();
	S32 i,j;
	for(i = 0; i < count; ++i)
	{
		bridge = (LLInvFVBridge*)(batch[i]);
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
		bridge = (LLInvFVBridge*)(batch[i]);
		if(!bridge || !bridge->isItemRemovable()) continue;
		cat = (LLViewerInventoryCategory*)model->getCategory(bridge->getUUID());
		if (cat)
		{
			gInventory.collectDescendents( cat->getUUID(), descendent_categories, descendent_items, FALSE );
			for (j=0; j<descendent_items.size(); j++)
			{
				if(LLAssetType::AT_GESTURE == descendent_items[j]->getType())
				{
					LLGestureMgr::instance().deactivateGesture(descendent_items[j]->getUUID());
				}
			}
		}
	}
	removeBatchNoCheck(batch);
	model->checkTrashOverflow();
}

void  LLInvFVBridge::removeBatchNoCheck(std::vector<LLFolderViewModelItem*>&  batch)
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
	S32 count = batch.size();
	S32 i;

	// first, hide any 'preview' floaters that correspond to the items
	// being deleted.
	for(i = 0; i < count; ++i)
	{
		bridge = (LLInvFVBridge*)(batch[i]);
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
		bridge = (LLInvFVBridge*)(batch[i]);
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
		bridge = (LLInvFVBridge*)(batch[i]);
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
		LLViewerInventoryItem* item = gInventory.getItem(*it);
		if (item)
		{
			model->updateItem(item);
		}
	}

	// notify inventory observers.
	model->notifyObservers();
}

BOOL LLInvFVBridge::isClipboardPasteable() const
{
	// Return FALSE on degenerated cases: empty clipboard, no inventory, no agent
	if (!LLClipboard::instance().hasContents() || !isAgentInventory())
	{
		return FALSE;
	}
	LLInventoryModel* model = getInventoryModel();
	if (!model)
	{
		return FALSE;
	}

	// In cut mode, whatever is on the clipboard is always pastable
	if (LLClipboard::instance().isCutMode())
	{
		return TRUE;
	}

	// In normal mode, we need to check each element of the clipboard to know if we can paste or not
	std::vector<LLUUID> objects;
	LLClipboard::instance().pasteFromClipboard(objects);
	S32 count = objects.size();
	for(S32 i = 0; i < count; i++)
	{
		const LLUUID &item_id = objects.at(i);

		// Folders are pastable if all items in there are copyable
		const LLInventoryCategory *cat = model->getCategory(item_id);
		if (cat)
		{
			LLFolderBridge cat_br(mInventoryPanel.get(), mRoot, item_id);
			if (!cat_br.isItemCopyable())
			return FALSE;
			// Skip to the next item in the clipboard
			continue;
		}

		// Each item must be copyable to be pastable
		LLItemBridge item_br(mInventoryPanel.get(), mRoot, item_id);
		if (!item_br.isItemCopyable())
		{
			return FALSE;
		}
	}
	return TRUE;
}

BOOL LLInvFVBridge::isClipboardPasteableAsLink() const
{
	if (!LLClipboard::instance().hasContents() || !isAgentInventory())
	{
		return FALSE;
	}
	const LLInventoryModel* model = getInventoryModel();
	if (!model)
	{
		return FALSE;
	}

	std::vector<LLUUID> objects;
	LLClipboard::instance().pasteFromClipboard(objects);
	S32 count = objects.size();
	for(S32 i = 0; i < count; i++)
	{
		const LLInventoryItem *item = model->getItem(objects.at(i));
		if (item)
		{
			if (!LLAssetType::lookupCanLink(item->getActualType()))
			{
				return FALSE;
			}
		}
		const LLViewerInventoryCategory *cat = model->getCategory(objects.at(i));
		if (cat && LLFolderType::lookupIsProtectedType(cat->getPreferredType()))
		{
			return FALSE;
		}
	}
	return TRUE;
}

void disable_context_entries_if_present(LLMenuGL& menu,
                                        const menuentry_vec_t &disabled_entries)
{
	const LLView::child_list_t *list = menu.getChildList();
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
			disable_context_entries_if_present(*branchp->getBranch(), disabled_entries);
		}

		bool found = false;
		menuentry_vec_t::const_iterator itor2;
		for (itor2 = disabled_entries.begin(); itor2 != disabled_entries.end(); ++itor2)
		{
			if (*itor2 == name)
			{
				found = true;
				break;
			}
		}

        if (found)
        {
			menu_item->setVisible(TRUE);
			// A bit of a hack so we can remember that some UI element explicitly set this to be visible
			// so that some other UI element from multi-select doesn't later set this invisible.
			menu_item->pushVisible(TRUE);

			menu_item->setEnabled(FALSE);
        }
    }
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
				if (!isItemRenameable() || ((flags & FIRST_SELECTED_ITEM) == 0))
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

			items.push_back(std::string("Cut"));
			if (!isItemMovable() || !isItemRemovable())
			{
				disabled_items.push_back(std::string("Cut"));
			}

			if (canListOnMarketplace() && !isMarketplaceListingsFolder() && !isInboxFolder())
			{
				items.push_back(std::string("Marketplace Separator"));

                if (gMenuHolder->getChild<LLView>("MarketplaceListings")->getVisible())
                {
                    items.push_back(std::string("Marketplace Copy"));
                    items.push_back(std::string("Marketplace Move"));
                    if (!canListOnMarketplaceNow())
                    {
                        disabled_items.push_back(std::string("Marketplace Copy"));
                        disabled_items.push_back(std::string("Marketplace Move"));
                    }
                }
			}
		}
	}

	// Don't allow items to be pasted directly into the COF or the inbox
	if (!isCOFFolder() && !isInboxFolder())
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

	LLInventoryPanel *active_panel = LLInventoryPanel::getActiveInventoryPanel(FALSE);
	if (active_panel && (active_panel->getName() != "All Items"))
	{
		items.push_back(std::string("Show in Main Panel"));
	}
}

void LLInvFVBridge::buildContextMenu(LLMenuGL& menu, U32 flags)
{
	LL_DEBUGS() << "LLInvFVBridge::buildContextMenu()" << LL_ENDL;
	menuentry_vec_t items;
	menuentry_vec_t disabled_items;
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
		
		addOpenRightClickMenuOption(items);
		items.push_back(std::string("Properties"));

		getClipboardEntries(true, items, disabled_items, flags);
	}
	addLinkReplaceMenuOption(items, disabled_items);
	hide_context_entries(menu, items, disabled_items);
}

bool get_selection_item_uuids(LLFolderView::selected_items_t& selected_items, uuid_vec_t& ids)
{
	uuid_vec_t results;
    S32 non_item = 0;
	for(LLFolderView::selected_items_t::iterator it = selected_items.begin(); it != selected_items.end(); ++it)
	{
		LLItemBridge *view_model = dynamic_cast<LLItemBridge *>((*it)->getViewModelItem());

		if(view_model && view_model->getUUID().notNull())
		{
			results.push_back(view_model->getUUID());
		}
        else
        {
            non_item++;
        }
	}
	if (non_item == 0)
	{
		ids = results;
		return true;
	}
	return false;
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

	items.push_back(std::string("Delete"));

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

void LLInvFVBridge::addMarketplaceContextMenuOptions(U32 flags,
												menuentry_vec_t &items,
												menuentry_vec_t &disabled_items)
{
    S32 depth = depth_nesting_in_marketplace(mUUID);
    if (depth == 1)
    {
        // Options available at the Listing Folder level
        items.push_back(std::string("Marketplace Create Listing"));
        items.push_back(std::string("Marketplace Associate Listing"));
        items.push_back(std::string("Marketplace Check Listing"));
        items.push_back(std::string("Marketplace List"));
        items.push_back(std::string("Marketplace Unlist"));
        if (LLMarketplaceData::instance().isUpdating(mUUID,depth) || ((flags & FIRST_SELECTED_ITEM) == 0))
        {
            // During SLM update, disable all marketplace related options
            // Also disable all if multiple selected items
            disabled_items.push_back(std::string("Marketplace Create Listing"));
            disabled_items.push_back(std::string("Marketplace Associate Listing"));
            disabled_items.push_back(std::string("Marketplace Check Listing"));
            disabled_items.push_back(std::string("Marketplace List"));
            disabled_items.push_back(std::string("Marketplace Unlist"));
        }
        else
        {
            if (gSavedSettings.getBOOL("MarketplaceListingsLogging"))
            {
                items.push_back(std::string("Marketplace Get Listing"));
            }
            if (LLMarketplaceData::instance().isListed(mUUID))
            {
                disabled_items.push_back(std::string("Marketplace Create Listing"));
                disabled_items.push_back(std::string("Marketplace Associate Listing"));
                if (LLMarketplaceData::instance().getVersionFolder(mUUID).isNull())
                {
                    disabled_items.push_back(std::string("Marketplace List"));
                    disabled_items.push_back(std::string("Marketplace Unlist"));
                }
                else
                {
                    if (LLMarketplaceData::instance().getActivationState(mUUID))
                    {
                        disabled_items.push_back(std::string("Marketplace List"));
                    }
                    else
                    {
                        disabled_items.push_back(std::string("Marketplace Unlist"));
                    }
                }
            }
            else
            {
                disabled_items.push_back(std::string("Marketplace List"));
                disabled_items.push_back(std::string("Marketplace Unlist"));
                if (gSavedSettings.getBOOL("MarketplaceListingsLogging"))
                {
                    disabled_items.push_back(std::string("Marketplace Get Listing"));
                }
            }
        }
    }
    if (depth == 2)
    {
        // Options available at the Version Folder levels and only for folders
        LLInventoryCategory* cat = gInventory.getCategory(mUUID);
        if (cat && LLMarketplaceData::instance().isListed(cat->getParentUUID()))
        {
            items.push_back(std::string("Marketplace Activate"));
            items.push_back(std::string("Marketplace Deactivate"));
            if (LLMarketplaceData::instance().isUpdating(mUUID,depth) || ((flags & FIRST_SELECTED_ITEM) == 0))
            {
                // During SLM update, disable all marketplace related options
                // Also disable all if multiple selected items
                disabled_items.push_back(std::string("Marketplace Activate"));
                disabled_items.push_back(std::string("Marketplace Deactivate"));
            }
            else
            {
                if (LLMarketplaceData::instance().isVersionFolder(mUUID))
                {
                    disabled_items.push_back(std::string("Marketplace Activate"));
                    if (LLMarketplaceData::instance().getActivationState(mUUID))
                    {
                        disabled_items.push_back(std::string("Marketplace Deactivate"));
                    }
                }
                else
                {
                    disabled_items.push_back(std::string("Marketplace Deactivate"));
                }
            }
        }
    }

    items.push_back(std::string("Marketplace Edit Listing"));
    LLUUID listing_folder_id = nested_parent_id(mUUID,depth);
    LLUUID version_folder_id = LLMarketplaceData::instance().getVersionFolder(listing_folder_id);

    if (depth >= 2)
    {
        // Prevent creation of new folders if the max count has been reached on this version folder (active or not)
        LLUUID local_version_folder_id = nested_parent_id(mUUID,depth-1);
        LLInventoryModel::cat_array_t categories;
        LLInventoryModel::item_array_t items;
        gInventory.collectDescendents(local_version_folder_id, categories, items, FALSE);
        if (categories.size() >= gSavedSettings.getU32("InventoryOutboxMaxFolderCount"))
        {
            disabled_items.push_back(std::string("New Folder"));
        }
    }
    
    // Options available at all levels on items and categories
    if (!LLMarketplaceData::instance().isListed(listing_folder_id) || version_folder_id.isNull())
    {
        disabled_items.push_back(std::string("Marketplace Edit Listing"));
    }

    // Separator
    items.push_back(std::string("Marketplace Listings Separator"));
}

void LLInvFVBridge::addLinkReplaceMenuOption(menuentry_vec_t& items, menuentry_vec_t& disabled_items)
{
	const LLInventoryObject* obj = getInventoryObject();

	if (isAgentInventory() && obj && obj->getType() != LLAssetType::AT_CATEGORY && obj->getType() != LLAssetType::AT_LINK_FOLDER)
	{
		items.push_back(std::string("Replace Links"));

		if (mRoot->getSelectedCount() != 1)
		{
			disabled_items.push_back(std::string("Replace Links"));
		}
	}
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
		//object_ids.push_back(obj->getUUID());

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

LLInventoryFilter* LLInvFVBridge::getInventoryFilter() const
{
	LLInventoryPanel* panel = mInventoryPanel.get();
	return panel ? &(panel->getFilter()) : NULL;
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

// *TODO : Suppress isInboxFolder() once Merchant Outbox is fully deprecated
BOOL LLInvFVBridge::isInboxFolder() const
{
	const LLUUID inbox_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_INBOX, false);
	
	if (inbox_id.isNull())
	{
		return FALSE;
	}
	
	return gInventory.isObjectDescendentOf(mUUID, inbox_id);
}

BOOL LLInvFVBridge::isMarketplaceListingsFolder() const
{
	const LLUUID folder_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_MARKETPLACE_LISTINGS, false);
	
	if (folder_id.isNull())
	{
		return FALSE;
	}
	
	return gInventory.isObjectDescendentOf(mUUID, folder_id);
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
	model->changeItemParent(item, new_parent_id, restamp);
}

// static
void LLInvFVBridge::changeCategoryParent(LLInventoryModel* model,
										 LLViewerInventoryCategory* cat,
										 const LLUUID& new_parent_id,
										 BOOL restamp)
{
	model->changeCategoryParent(cat, new_parent_id, restamp);
}

LLInvFVBridge* LLInvFVBridge::createBridge(LLAssetType::EType asset_type,
										   LLAssetType::EType actual_asset_type,
										   LLInventoryType::EType inv_type,
										   LLInventoryPanel* inventory,
										   LLFolderViewModelInventory* view_model,
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
				LL_WARNS() << LLAssetType::lookup(asset_type) << " asset has inventory type " << LLInventoryType::lookupHumanReadable(inv_type) << " on uuid " << uuid << LL_ENDL;
			}
			new_listener = new LLTextureBridge(inventory, root, uuid, inv_type);
			break;

		case LLAssetType::AT_SOUND:
			if(!(inv_type == LLInventoryType::IT_SOUND))
			{
				LL_WARNS() << LLAssetType::lookup(asset_type) << " asset has inventory type " << LLInventoryType::lookupHumanReadable(inv_type) << " on uuid " << uuid << LL_ENDL;
			}
			new_listener = new LLSoundBridge(inventory, root, uuid);
			break;

		case LLAssetType::AT_LANDMARK:
			if(!(inv_type == LLInventoryType::IT_LANDMARK))
			{
				LL_WARNS() << LLAssetType::lookup(asset_type) << " asset has inventory type " << LLInventoryType::lookupHumanReadable(inv_type) << " on uuid " << uuid << LL_ENDL;
			}
			new_listener = new LLLandmarkBridge(inventory, root, uuid, flags);
			break;

		case LLAssetType::AT_CALLINGCARD:
			if(!(inv_type == LLInventoryType::IT_CALLINGCARD))
			{
				LL_WARNS() << LLAssetType::lookup(asset_type) << " asset has inventory type " << LLInventoryType::lookupHumanReadable(inv_type) << " on uuid " << uuid << LL_ENDL;
			}
			new_listener = new LLCallingCardBridge(inventory, root, uuid);
			break;

		case LLAssetType::AT_SCRIPT:
			if(!(inv_type == LLInventoryType::IT_LSL))
			{
				LL_WARNS() << LLAssetType::lookup(asset_type) << " asset has inventory type " << LLInventoryType::lookupHumanReadable(inv_type) << " on uuid " << uuid << LL_ENDL;
			}
			new_listener = new LLItemBridge(inventory, root, uuid);
			break;

		case LLAssetType::AT_OBJECT:
			if(!(inv_type == LLInventoryType::IT_OBJECT || inv_type == LLInventoryType::IT_ATTACHMENT))
			{
				LL_WARNS() << LLAssetType::lookup(asset_type) << " asset has inventory type " << LLInventoryType::lookupHumanReadable(inv_type) << " on uuid " << uuid << LL_ENDL;
			}
			new_listener = new LLObjectBridge(inventory, root, uuid, inv_type, flags);
			break;

		case LLAssetType::AT_NOTECARD:
			if(!(inv_type == LLInventoryType::IT_NOTECARD))
			{
				LL_WARNS() << LLAssetType::lookup(asset_type) << " asset has inventory type " << LLInventoryType::lookupHumanReadable(inv_type) << " on uuid " << uuid << LL_ENDL;
			}
			new_listener = new LLNotecardBridge(inventory, root, uuid);
			break;

		case LLAssetType::AT_ANIMATION:
			if(!(inv_type == LLInventoryType::IT_ANIMATION))
			{
				LL_WARNS() << LLAssetType::lookup(asset_type) << " asset has inventory type " << LLInventoryType::lookupHumanReadable(inv_type) << " on uuid " << uuid << LL_ENDL;
			}
			new_listener = new LLAnimationBridge(inventory, root, uuid);
			break;

		case LLAssetType::AT_GESTURE:
			if(!(inv_type == LLInventoryType::IT_GESTURE))
			{
				LL_WARNS() << LLAssetType::lookup(asset_type) << " asset has inventory type " << LLInventoryType::lookupHumanReadable(inv_type) << " on uuid " << uuid << LL_ENDL;
			}
			new_listener = new LLGestureBridge(inventory, root, uuid);
			break;

		case LLAssetType::AT_LSL_TEXT:
			if(!(inv_type == LLInventoryType::IT_LSL))
			{
				LL_WARNS() << LLAssetType::lookup(asset_type) << " asset has inventory type " << LLInventoryType::lookupHumanReadable(inv_type) << " on uuid " << uuid << LL_ENDL;
			}
			new_listener = new LLLSLTextBridge(inventory, root, uuid);
			break;

		case LLAssetType::AT_CLOTHING:
		case LLAssetType::AT_BODYPART:
			if(!(inv_type == LLInventoryType::IT_WEARABLE))
			{
				LL_WARNS() << LLAssetType::lookup(asset_type) << " asset has inventory type " << LLInventoryType::lookupHumanReadable(inv_type) << " on uuid " << uuid << LL_ENDL;
			}
			new_listener = new LLWearableBridge(inventory, root, uuid, asset_type, inv_type, LLWearableType::inventoryFlagsToWearableType(flags));
			break;
		case LLAssetType::AT_CATEGORY:
			if (actual_asset_type == LLAssetType::AT_LINK_FOLDER)
			{
				// Create a link folder handler instead
				new_listener = new LLLinkFolderBridge(inventory, root, uuid);
			}
            else if (actual_asset_type == LLAssetType::AT_MARKETPLACE_FOLDER)
            {
				// Create a marketplace folder handler
				new_listener = new LLMarketplaceFolderBridge(inventory, root, uuid);
            }
            else
            {
                new_listener = new LLFolderBridge(inventory, root, uuid);
            }
			break;
		case LLAssetType::AT_LINK:
		case LLAssetType::AT_LINK_FOLDER:
			// Only should happen for broken links.
			new_listener = new LLLinkItemBridge(inventory, root, uuid);
			break;
	    case LLAssetType::AT_MESH:
			if(!(inv_type == LLInventoryType::IT_MESH))
			{
				LL_WARNS() << LLAssetType::lookup(asset_type) << " asset has inventory type " << LLInventoryType::lookupHumanReadable(inv_type) << " on uuid " << uuid << LL_ENDL;
			}
			new_listener = new LLMeshBridge(inventory, root, uuid);
			break;

		case LLAssetType::AT_IMAGE_TGA:
		case LLAssetType::AT_IMAGE_JPEG:
			//LL_WARNS() << LLAssetType::lookup(asset_type) << " asset type is unhandled for uuid " << uuid << LL_ENDL;
			break;

		default:
			LL_INFOS() << "Unhandled asset type (llassetstorage.h): "
					<< (S32)asset_type << " (" << LLAssetType::lookup(asset_type) << ")" << LL_ENDL;
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
	LLInventoryObject* obj = model->getObject(uuid);
	if (obj)
	{
		remove_inventory_object(uuid, NULL);
	}
}

void LLInvFVBridge::removeObject(LLInventoryModel *model, const LLUUID &uuid)
{
    // Keep track of the parent
    LLInventoryItem* itemp = model->getItem(uuid);
    LLUUID parent_id = (itemp ? itemp->getParentUUID() : LLUUID::null);
    // Remove the object
    model->removeObject(uuid);
    // Get the parent updated
    if (parent_id.notNull())
    {
        LLViewerInventoryCategory* parent_cat = model->getCategory(parent_id);
        model->updateCategory(parent_cat);
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

			const LLUUID trash_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_TRASH);
			if ((mUUID == trash_id) || gInventory.isObjectDescendentOf(mUUID, trash_id))
			{
				can_share = false;
			}
		}
	}

	return can_share;
}

bool LLInvFVBridge::canListOnMarketplace() const
{
	LLInventoryModel * model = getInventoryModel();

	LLViewerInventoryCategory * cat = model->getCategory(mUUID);
	if (cat && LLFolderType::lookupIsProtectedType(cat->getPreferredType()))
	{
		return false;
	}

	if (!isAgentInventory())
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
}

bool LLInvFVBridge::canListOnMarketplaceNow() const
{
	bool can_list = true;
    
	const LLInventoryObject* obj = getInventoryObject();
	can_list &= (obj != NULL);
    
	if (can_list)
	{
		const LLUUID& object_id = obj->getLinkedUUID();
		can_list = object_id.notNull();
        
		if (can_list)
		{
			LLFolderViewFolder * object_folderp =   mInventoryPanel.get() ? mInventoryPanel.get()->getFolderByID(object_id) : NULL;
			if (object_folderp)
			{
				can_list = !static_cast<LLFolderBridge*>(object_folderp->getViewModelItem())->isLoading();
			}
		}
		
		if (can_list)
		{
            std::string error_msg;
            LLInventoryModel* model = getInventoryModel();
            const LLUUID &marketplacelistings_id = model->findCategoryUUIDForType(LLFolderType::FT_MARKETPLACE_LISTINGS, false);
            if (marketplacelistings_id.notNull())
            {
                LLViewerInventoryCategory * master_folder = model->getCategory(marketplacelistings_id);
                LLInventoryCategory *cat = model->getCategory(mUUID);
                if (cat)
                {
                    can_list = can_move_folder_to_marketplace(master_folder, master_folder, cat, error_msg);
                }
                else
                {
                    LLInventoryItem *item = model->getItem(mUUID);
                    can_list = (item ? can_move_item_to_marketplace(master_folder, master_folder, item, error_msg) : false);
                }
            }
            else
            {
                can_list = false;
            }
		}
	}
	
	return can_list;
}

LLToolDragAndDrop::ESource LLInvFVBridge::getDragSource() const
{
	if (gInventory.isObjectDescendentOf(getUUID(),   gInventory.getRootFolderID()))
	{
		return LLToolDragAndDrop::SOURCE_AGENT;
	}
	else if (gInventory.isObjectDescendentOf(getUUID(),   gInventory.getLibraryRootFolderID()))
	{
		return LLToolDragAndDrop::SOURCE_LIBRARY;
	}

	return LLToolDragAndDrop::SOURCE_VIEWER;
}



// +=================================================+
// |        InventoryFVBridgeBuilder                 |
// +=================================================+
LLInvFVBridge* LLInventoryFolderViewModelBuilder::createBridge(LLAssetType::EType asset_type,
														LLAssetType::EType actual_asset_type,
														LLInventoryType::EType inv_type,
														LLInventoryPanel* inventory,
														LLFolderViewModelInventory* view_model,
														LLFolderView* root,
														const LLUUID& uuid,
														U32 flags /* = 0x00 */) const
{
	return LLInvFVBridge::createBridge(asset_type,
									   actual_asset_type,
									   inv_type,
									   inventory,
									   view_model,
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
	else if ("show_in_main_panel" == action)
	{
		LLInventoryPanel::openInventoryPanelAndSetSelection(TRUE, mUUID, TRUE);
		return;
	}
	else if ("cut" == action)
	{
		cutToClipboard();
		return;
	}
	else if ("copy" == action)
	{
		copyToClipboard();
		return;
	}
	else if ("paste" == action)
	{
		LLInventoryItem* itemp = model->getItem(mUUID);
		if (!itemp) return;

		LLFolderViewItem* folder_view_itemp =   mInventoryPanel.get()->getItemByID(itemp->getParentUUID());
		if (!folder_view_itemp) return;

		folder_view_itemp->getViewModelItem()->pasteFromClipboard();
		return;
	}
	else if ("paste_link" == action)
	{
		// Single item only
		LLInventoryItem* itemp = model->getItem(mUUID);
		if (!itemp) return;

		LLFolderViewItem* folder_view_itemp =   mInventoryPanel.get()->getItemByID(itemp->getParentUUID());
		if (!folder_view_itemp) return;

		folder_view_itemp->getViewModelItem()->pasteLinkFromClipboard();
		return;
	}
	else if (("move_to_marketplace_listings" == action) || ("copy_to_marketplace_listings" == action) || ("copy_or_move_to_marketplace_listings" == action))
	{
		LLInventoryItem* itemp = model->getItem(mUUID);
		if (!itemp) return;
        const LLUUID &marketplacelistings_id = model->findCategoryUUIDForType(LLFolderType::FT_MARKETPLACE_LISTINGS, false);
        // Note: For a single item, if it's not a copy, then it's a move
        move_item_to_marketplacelistings(itemp, marketplacelistings_id, ("copy_to_marketplace_listings" == action));
    }
	else if ("copy_slurl" == action)
	{
		LLViewerInventoryItem* item = static_cast<LLViewerInventoryItem*>(getItem());
		if(item)
		{
			LLUUID asset_id = item->getAssetUUID();
			LLLandmark* landmark = gLandmarkList.getAsset(asset_id);
			if (landmark)
			{
				LLVector3d global_pos;
				landmark->getGlobalPos(global_pos);
				LLLandmarkActions::getSLURLfromPosGlobal(global_pos, &copy_slurl_to_clipboard_callback_inv, true);
			}
		}
	}
	else if ("show_on_map" == action)
	{
		doActionOnCurSelectedLandmark(boost::bind(&LLItemBridge::doShowOnMap, this, _1));
	}
	else if ("marketplace_edit_listing" == action)
	{
        std::string url = LLMarketplaceData::instance().getListingURL(mUUID);
        LLUrlAction::openURL(url);
	}
}

void LLItemBridge::doActionOnCurSelectedLandmark(LLLandmarkList::loaded_callback_t cb)
{
	LLViewerInventoryItem* cur_item = getItem();
	if(cur_item && cur_item->getInventoryType() == LLInventoryType::IT_LANDMARK)
	{ 
		LLLandmark* landmark = LLLandmarkActions::getLandmark(cur_item->getUUID(), cb);
		if (landmark)
		{
			cb(landmark);
		}
	}
}

void LLItemBridge::doShowOnMap(LLLandmark* landmark)
{
	LLVector3d landmark_global_pos;
	// landmark has already been tested for NULL by calling routine
	if (landmark->getGlobalPos(landmark_global_pos))
	{
		LLFloaterWorldMap* worldmap_instance = LLFloaterWorldMap::getInstance();
		if (!landmark_global_pos.isExactlyZero() && worldmap_instance)
		{
			worldmap_instance->trackLocation(landmark_global_pos);
			LLFloaterReg::showInstance("world_map", "center");
		}
	}
}

void copy_slurl_to_clipboard_callback_inv(const std::string& slurl)
{
	gViewerWindow->getWindow()->copyTextToClipboard(utf8str_to_wstring(slurl));
	LLSD args;
	args["SLURL"] = slurl;
	LLNotificationsUtil::add("CopySLURL", args);
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
		bool is_snapshot = (item->getInventoryType() == LLInventoryType::IT_SNAPSHOT);

		const LLUUID new_parent = model->findCategoryUUIDForType(is_snapshot? LLFolderType::FT_SNAPSHOT_CATEGORY : LLFolderType::assetTypeToFolderType(item->getType()));
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
	
	return LLInventoryIcon::getIcon(LLInventoryType::ICONNAME_OBJECT);
}

LLUIImagePtr LLItemBridge::getIconOverlay() const
{
	if (getItem() && getItem()->getIsLinkType())
	{
		return LLUI::getUIImage("Inv_Link");
	}
	return NULL;
}

PermissionMask LLItemBridge::getPermissionMask() const
{
	LLViewerInventoryItem* item = getItem();
	PermissionMask perm_mask = 0;
	if (item) perm_mask = item->getPermissionMask();
	return perm_mask;
}

void LLItemBridge::buildDisplayName() const
{
	if(getItem())
	{
		mDisplayName.assign(getItem()->getName());
	}
	else
	{
		mDisplayName.assign(LLStringUtil::null);
	}
	S32 old_length = mSearchableName.length();
	S32 new_length = mDisplayName.length() + getLabelSuffix().length();

	mSearchableName.assign(mDisplayName);
	mSearchableName.append(getLabelSuffix());
	LLStringUtil::toUpper(mSearchableName);
	
	if ((old_length > new_length) && getInventoryFilter())
	{
		getInventoryFilter()->setModified(LLFolderViewFilter::FILTER_MORE_RESTRICTIVE);
	}
	//Name set, so trigger a sort
	if(mParent)
	{
		mParent->requestSort();
	}
}

LLFontGL::StyleFlags LLItemBridge::getLabelStyle() const
{
	U8 font = LLFontGL::NORMAL;
	const LLViewerInventoryItem* item = getItem();

	if (get_is_item_worn(mUUID))
	{
		// LL_INFOS() << "BOLD" << LL_ENDL;
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
		LLSD updates;
		updates["name"] = new_name;
		update_inventory_item(item->getUUID(),updates, NULL);
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
	LLInventoryModel* model = getInventoryModel();
	if(!model) return FALSE;
	const LLUUID& trash_id = model->findCategoryUUIDForType(LLFolderType::FT_TRASH);
	LLViewerInventoryItem* item = getItem();
	if (!item) return FALSE;
	if (item->getType() != LLAssetType::AT_LSL_TEXT)
	{
		LLPreview::hide(mUUID, TRUE);
	}
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
			LLInventoryModel::item_array_t item_array = gInventory.collectLinksTo(mUUID);
			const U32 num_links = item_array.size();
			if (num_links > 0)
			{
				// Warn if the user is will break any links when deleting this item.
				LLNotifications::instance().add(params);
				return FALSE;
			}
		}
	}
	
	LLNotifications::instance().forceResponse(params, 0);
	model->checkTrashOverflow();
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
		// If it's a protected type folder, we can't move it
		if (LLFolderType::lookupIsProtectedType(((LLInventoryCategory*)obj)->getPreferredType()))
			return FALSE;
		return TRUE;
	}
	return FALSE;
}

void LLFolderBridge::selectItem()
{
	// Have no fear: the first thing start() does is to test if everything for that folder has been fetched...
	LLInventoryModelBackgroundFetch::instance().start(getUUID(), true);
}

void LLFolderBridge::buildDisplayName() const
{
	LLFolderType::EType preferred_type = getPreferredType();

	// *TODO: to be removed when database supports multi language. This is a
	// temporary attempt to display the inventory folder in the user locale.
	// mantipov: *NOTE: be sure this code is synchronized with LLFriendCardsManager::findChildFolderUUID
	//		it uses the same way to find localized string

	// HACK: EXT - 6028 ([HARD CODED]? Inventory > Library > "Accessories" folder)
	// Translation of Accessories folder in Library inventory folder
	bool accessories = false;
	if(getName() == "Accessories")
	{
		//To ensure that Accessories folder is in Library we have to check its parent folder.
		//Due to parent LLFolderViewFloder is not set to this item yet we have to check its parent via Inventory Model
		LLInventoryCategory* cat = gInventory.getCategory(getUUID());
		if(cat)
		{
			const LLUUID& parent_folder_id = cat->getParentUUID();
			accessories = (parent_folder_id == gInventory.getLibraryRootFolderID());
		}
	}

	//"Accessories" inventory category has folder type FT_NONE. So, this folder
	//can not be detected as protected with LLFolderType::lookupIsProtectedType
	mDisplayName.assign(getName());
	if (accessories || LLFolderType::lookupIsProtectedType(preferred_type))
	{
		LLTrans::findString(mDisplayName, std::string("InvFolder ") + getName(), LLSD());
	}

	mSearchableName.assign(mDisplayName);
	mSearchableName.append(getLabelSuffix());
	LLStringUtil::toUpper(mSearchableName);

    //Name set, so trigger a sort
    if(mParent)
    {
        mParent->requestSort();
    }
}

std::string LLFolderBridge::getLabelSuffix() const
{
    static LLCachedControl<F32> folder_loading_message_delay(gSavedSettings, "FolderLoadingMessageWaitTime", 0.5f);
    
    if (mIsLoading && mTimeSinceRequestStart.getElapsedTimeF32() >= folder_loading_message_delay())
    {
        return llformat(" ( %s ) ", LLTrans::getString("LoadingData").c_str());
    }
    
    return LLInvFVBridge::getLabelSuffix();
}

LLFontGL::StyleFlags LLFolderBridge::getLabelStyle() const
{
    return LLFontGL::NORMAL;
}

void LLFolderBridge::update()
{
	// we know we have children but  haven't  fetched them (doesn't obey filter)
	bool loading = !isUpToDate() && hasChildren() && mFolderViewItem->isOpen();

	if (loading != mIsLoading)
	{
		if ( loading )
		{
			// Measure how long we've been in the loading state
			mTimeSinceRequestStart.reset();
		}
		mIsLoading = loading;

		mFolderViewItem->refresh();
	}
}


// Iterate through a folder's children to determine if
// all the children are removable.
class LLIsItemRemovable : public LLFolderViewFunctor
{
public:
	LLIsItemRemovable() : mPassed(TRUE) {}
	virtual void doFolder(LLFolderViewFolder* folder)
	{
		mPassed &= folder->getViewModelItem()->isItemRemovable();
	}
	virtual void doItem(LLFolderViewItem* item)
	{
		mPassed &= item->getViewModelItem()->isItemRemovable();
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
	LLFolderViewFolder* folderp = dynamic_cast<LLFolderViewFolder*>(panel ?   panel->getItemByID(mUUID) : NULL);
	if (folderp)
	{
		LLIsItemRemovable folder_test;
		folderp->applyFunctorToChildren(folder_test);
		if (!folder_test.mPassed)
		{
			return FALSE;
		}
	}
    
    if (isMarketplaceListingsFolder() && LLMarketplaceData::instance().getActivationState(mUUID))
    {
        return FALSE;
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
	// Folders are copyable if items in them are, recursively, copyable.
	
	// Get the content of the folder
	LLInventoryModel::cat_array_t* cat_array;
	LLInventoryModel::item_array_t* item_array;
	gInventory.getDirectDescendentsOf(mUUID,cat_array,item_array);

	// Check the items
	LLInventoryModel::item_array_t item_array_copy = *item_array;
	for (LLInventoryModel::item_array_t::iterator iter = item_array_copy.begin(); iter != item_array_copy.end(); iter++)
	{
		LLInventoryItem* item = *iter;
		LLItemBridge item_br(mInventoryPanel.get(), mRoot, item->getUUID());
		if (!item_br.isItemCopyable())
			return FALSE;
}

	// Check the folders
	LLInventoryModel::cat_array_t cat_array_copy = *cat_array;
	for (LLInventoryModel::cat_array_t::iterator iter = cat_array_copy.begin(); iter != cat_array_copy.end(); iter++)
{
		LLViewerInventoryCategory* category = *iter;
		LLFolderBridge cat_br(mInventoryPanel.get(), mRoot, category->getUUID());
		if (!cat_br.isItemCopyable())
			return FALSE;
	}
	
		return TRUE;
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

		std::vector<LLUUID> objects;
		LLClipboard::instance().pasteFromClipboard(objects);
		const LLViewerInventoryCategory *current_cat = getCategory();

		// Search for the direct descendent of current Friends subfolder among all pasted items,
		// and return false if is found.
		for(S32 i = objects.size() - 1; i >= 0; --i)
		{
			const LLUUID &obj_id = objects.at(i);
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
		std::vector<LLUUID> objects;
		LLClipboard::instance().pasteFromClipboard(objects);
		S32 count = objects.size();
		for(S32 i = 0; i < count; i++)
		{
			const LLUUID &obj_id = objects.at(i);
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


BOOL LLFolderBridge::dragCategoryIntoFolder(LLInventoryCategory* inv_cat,
											BOOL drop,
											std::string& tooltip_msg,
											BOOL is_link,
											BOOL user_confirm)
{

	LLInventoryModel* model = getInventoryModel();

	if (!inv_cat) return FALSE; // shouldn't happen, but in case item is incorrectly parented in which case inv_cat will be NULL
	if (!model) return FALSE;
	if (!isAgentAvatarValid()) return FALSE;
	if (!isAgentInventory()) return FALSE; // cannot drag categories into library

	LLInventoryPanel* destination_panel = mInventoryPanel.get();
	if (!destination_panel) return false;

	LLInventoryFilter* filter = getInventoryFilter();
	if (!filter) return false;

	const LLUUID &cat_id = inv_cat->getUUID();
	const LLUUID &current_outfit_id = model->findCategoryUUIDForType(LLFolderType::FT_CURRENT_OUTFIT, false);
	const LLUUID &marketplacelistings_id = model->findCategoryUUIDForType(LLFolderType::FT_MARKETPLACE_LISTINGS, false);
    const LLUUID from_folder_uuid = inv_cat->getParentUUID();
	
	const BOOL move_is_into_current_outfit = (mUUID == current_outfit_id);
	const BOOL move_is_into_marketplacelistings = model->isObjectDescendentOf(mUUID, marketplacelistings_id);
    const BOOL move_is_from_marketplacelistings = model->isObjectDescendentOf(cat_id, marketplacelistings_id);

	// check to make sure source is agent inventory, and is represented there.
	LLToolDragAndDrop::ESource source = LLToolDragAndDrop::getInstance()->getSource();
	const BOOL is_agent_inventory = (model->getCategory(cat_id) != NULL)
		&& (LLToolDragAndDrop::SOURCE_AGENT == source);

	BOOL accept = FALSE;
	U64 filter_types = filter->getFilterTypes();
	BOOL use_filter = filter_types && (filter_types&LLInventoryFilter::FILTERTYPE_DATE || (filter_types&LLInventoryFilter::FILTERTYPE_OBJECT)==0);

	if (is_agent_inventory)
	{
		const LLUUID &trash_id = model->findCategoryUUIDForType(LLFolderType::FT_TRASH, false);
		const LLUUID &landmarks_id = model->findCategoryUUIDForType(LLFolderType::FT_LANDMARK, false);
		const LLUUID &my_outifts_id = model->findCategoryUUIDForType(LLFolderType::FT_MY_OUTFITS, false);

		const BOOL move_is_into_trash = (mUUID == trash_id) || model->isObjectDescendentOf(mUUID, trash_id);
		const BOOL move_is_into_my_outfits = (mUUID == my_outifts_id) || model->isObjectDescendentOf(mUUID, my_outifts_id);
		const BOOL move_is_into_outfit = move_is_into_my_outfits || (getCategory() && getCategory()->getPreferredType()==LLFolderType::FT_OUTFIT);
		const BOOL move_is_into_current_outfit = (getCategory() && getCategory()->getPreferredType()==LLFolderType::FT_CURRENT_OUTFIT);
		const BOOL move_is_into_landmarks = (mUUID == landmarks_id) || model->isObjectDescendentOf(mUUID, landmarks_id);

		//--------------------------------------------------------------------------------
		// Determine if folder can be moved.
		//

		BOOL is_movable = TRUE;

        if (is_movable && (marketplacelistings_id == cat_id))
        {
            is_movable = FALSE;
            tooltip_msg = LLTrans::getString("TooltipOutboxCannotMoveRoot");
        }
        if (is_movable && move_is_from_marketplacelistings && LLMarketplaceData::instance().getActivationState(cat_id))
        {
            // If the incoming folder is listed and active (and is therefore either the listing or the version folder),
            // then moving is *not* allowed
            is_movable = FALSE;
            tooltip_msg = LLTrans::getString("TooltipOutboxDragActive");
        }
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
			if((mUUID == my_outifts_id) || (getCategory() && getCategory()->getPreferredType() == LLFolderType::FT_NONE))
			{
				is_movable = ((inv_cat->getPreferredType() == LLFolderType::FT_NONE) || (inv_cat->getPreferredType() == LLFolderType::FT_OUTFIT));
			}
			else
			{
				is_movable = false;
			}
		}
		if(is_movable && move_is_into_current_outfit && is_link)
		{
			is_movable = FALSE;
		}
		if (is_movable && (mUUID == model->findCategoryUUIDForType(LLFolderType::FT_FAVORITE)))
		{
			is_movable = FALSE;
			// tooltip?
		}
		if (is_movable && (getPreferredType() == LLFolderType::FT_MARKETPLACE_STOCK))
		{
            // One cannot move a folder into a stock folder
			is_movable = FALSE;
			// tooltip?
        }
		
		LLInventoryModel::cat_array_t descendent_categories;
		LLInventoryModel::item_array_t descendent_items;
		if (is_movable)
		{
			model->collectDescendents(cat_id, descendent_categories, descendent_items, FALSE);
			for (S32 i=0; i < descendent_categories.size(); ++i)
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
		U32 max_items_to_wear = gSavedSettings.getU32("WearFolderLimit");
		if (is_movable
			&& move_is_into_current_outfit
			&& descendent_items.size() > max_items_to_wear)
		{
			LLInventoryModel::cat_array_t cats;
			LLInventoryModel::item_array_t items;
			LLFindWearablesEx not_worn(/*is_worn=*/ false, /*include_body_parts=*/ false);
			gInventory.collectDescendentsIf(cat_id,
				cats,
				items,
				LLInventoryModel::EXCLUDE_TRASH,
				not_worn);

			if (items.size() > max_items_to_wear)
			{
				// Can't move 'large' folders into current outfit: MAINT-4086
				is_movable = FALSE;
				LLStringUtil::format_map_t args;
				args["AMOUNT"] = llformat("%d", max_items_to_wear);
				tooltip_msg = LLTrans::getString("TooltipTooManyWearables",args);
			}
		}
		if (is_movable && move_is_into_trash)
		{
			for (S32 i=0; i < descendent_items.size(); ++i)
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
			for (S32 i=0; i < descendent_items.size(); ++i)
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
        
		if (is_movable && move_is_into_marketplacelistings)
		{
            const LLViewerInventoryCategory * master_folder = model->getFirstDescendantOf(marketplacelistings_id, mUUID);
            LLViewerInventoryCategory * dest_folder = getCategory();
            S32 bundle_size = (drop ? 1 : LLToolDragAndDrop::instance().getCargoCount());
            is_movable = can_move_folder_to_marketplace(master_folder, dest_folder, inv_cat, tooltip_msg, bundle_size);
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

				if (is_movable && use_filter)
				{
					// Check whether the folder being dragged from active inventory panel
					// passes the filter of the destination panel.
					is_movable = check_category(model, cat_id, active_panel, filter);
				}
			}
		}
		// 
		//--------------------------------------------------------------------------------

		accept = is_movable;

		if (accept && drop)
		{
            // Dropping in or out of marketplace needs (sometimes) confirmation
            if (user_confirm && (move_is_from_marketplacelistings || move_is_into_marketplacelistings))
            {
                if (move_is_from_marketplacelistings && (LLMarketplaceData::instance().isInActiveFolder(cat_id) ||
                                                         LLMarketplaceData::instance().isListedAndActive(cat_id)))
                {
                    if (LLMarketplaceData::instance().isListed(cat_id) || LLMarketplaceData::instance().isVersionFolder(cat_id))
                    {
                        // Move the active version folder or listing folder itself outside marketplace listings will unlist the listing so ask that question specifically
                        LLNotificationsUtil::add("ConfirmMerchantUnlist", LLSD(), LLSD(), boost::bind(&LLFolderBridge::callback_dropCategoryIntoFolder, this, _1, _2, inv_cat));
                    }
                    else
                    {
                        // Any other case will simply modify but not unlist an active listed listing
                        LLNotificationsUtil::add("ConfirmMerchantActiveChange", LLSD(), LLSD(), boost::bind(&LLFolderBridge::callback_dropCategoryIntoFolder, this, _1, _2, inv_cat));
                    }
                    return true;
                }
                if (move_is_from_marketplacelistings && LLMarketplaceData::instance().isVersionFolder(cat_id))
                {
                    // Moving the version folder from its location will deactivate it. Ask confirmation.
                    LLNotificationsUtil::add("ConfirmMerchantClearVersion", LLSD(), LLSD(), boost::bind(&LLFolderBridge::callback_dropCategoryIntoFolder, this, _1, _2, inv_cat));
                    return true;
                }
                if (move_is_into_marketplacelistings && LLMarketplaceData::instance().isInActiveFolder(mUUID))
                {
                    // Moving something in an active listed listing will modify it. Ask confirmation.
                    LLNotificationsUtil::add("ConfirmMerchantActiveChange", LLSD(), LLSD(), boost::bind(&LLFolderBridge::callback_dropCategoryIntoFolder, this, _1, _2, inv_cat));
                    return true;
                }
                if (move_is_from_marketplacelistings && LLMarketplaceData::instance().isListed(cat_id))
                {
                    // Moving a whole listing folder will result in archival of SLM data. Ask confirmation.
                    LLNotificationsUtil::add("ConfirmListingCutOrDelete", LLSD(), LLSD(), boost::bind(&LLFolderBridge::callback_dropCategoryIntoFolder, this, _1, _2, inv_cat));
                    return true;
                }
                if (move_is_into_marketplacelistings && !move_is_from_marketplacelistings)
                {
                    LLNotificationsUtil::add("ConfirmMerchantMoveInventory", LLSD(), LLSD(), boost::bind(&LLFolderBridge::callback_dropCategoryIntoFolder, this, _1, _2, inv_cat));
                    return true;
                }
            }
			// Look for any gestures and deactivate them
			if (move_is_into_trash)
			{
				for (S32 i=0; i < descendent_items.size(); i++)
				{
					LLInventoryItem* item = descendent_items[i];
					if (item->getType() == LLAssetType::AT_GESTURE
						&& LLGestureMgr::instance().isGestureActive(item->getUUID()))
					{
						LLGestureMgr::instance().deactivateGesture(item->getUUID());
					}
				}
			}

			// if target is current outfit folder we use link
			if (move_is_into_current_outfit &&
				(inv_cat->getPreferredType() == LLFolderType::FT_NONE ||
				inv_cat->getPreferredType() == LLFolderType::FT_OUTFIT))
			{
				// traverse category and add all contents to currently worn.
				BOOL append = true;
				LLAppearanceMgr::instance().wearInventoryCategory(inv_cat, false, append);
			}
			else if (move_is_into_marketplacelistings)
			{
				move_folder_to_marketplacelistings(inv_cat, mUUID);
			}
			else
			{
				if (model->isObjectDescendentOf(cat_id, model->findCategoryUUIDForType(LLFolderType::FT_INBOX, false)))
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
            if (move_is_from_marketplacelistings)
            {
                // If we are moving a folder at the listing folder level (i.e. its parent is the marketplace listings folder)
                if (from_folder_uuid == marketplacelistings_id)
                {
                    // Clear the folder from the marketplace in case it is a listing folder
                    if (LLMarketplaceData::instance().isListed(cat_id))
                    {
                        LLMarketplaceData::instance().clearListing(cat_id);
                    }
                }
                else
                {
                    // If we move from within an active (listed) listing, checks that it's still valid, if not, unlist
                    LLUUID version_folder_id = LLMarketplaceData::instance().getActiveFolder(from_folder_uuid);
                    if (version_folder_id.notNull())
                    {
                        LLViewerInventoryCategory* cat = gInventory.getCategory(version_folder_id);
                        if (!validate_marketplacelistings(cat,NULL))
                        {
                            LLMarketplaceData::instance().activateListing(version_folder_id,false);
                        }
                    }
                    // In all cases, update the listing we moved from so suffix are updated
                    update_marketplace_category(from_folder_uuid);
                }
            }
		}
	}
	else if (LLToolDragAndDrop::SOURCE_WORLD == source)
	{
		if (move_is_into_marketplacelistings)
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
		if (move_is_into_marketplacelistings)
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

void warn_move_inventory(LLViewerObject* object, boost::shared_ptr<LLMoveInv> move_inv)
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

    static LLNotificationPtr notification_ptr;
    static boost::shared_ptr<LLMoveInv> inv_ptr;

    // Notification blocks user from interacting with inventories so everything that comes after first message
    // is part of this message - don'r show it again
    // Note: workaround for MAINT-5495 untill proper refactoring and warning system for Drag&Drop can be made.
    if (notification_ptr == NULL
        || !notification_ptr->isActive()
        || LLNotificationsUtil::find(notification_ptr->getID()) == NULL
        || inv_ptr->mCategoryID != move_inv->mCategoryID
        || inv_ptr->mObjectID != move_inv->mObjectID)
    {
        notification_ptr = LLNotificationsUtil::add(dialog, LLSD(), LLSD(), boost::bind(move_task_inventory_callback, _1, _2, move_inv));
        inv_ptr = move_inv;
    }
    else
    {
        // Notification is alive and not responded, operating inv_ptr should be safe so attach new data
        two_uuids_list_t::iterator move_it;
        for (move_it = move_inv->mMoveList.begin();
            move_it != move_inv->mMoveList.end();
            ++move_it)
        {
            inv_ptr->mMoveList.push_back(*move_it);
        }
        move_inv.reset();
    }
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
		LL_INFOS() << "Object not found for drop." << LL_ENDL;
		return FALSE;
	}

	// this folder is coming from an object, as there is only one folder in an object, the root,
	// we need to collect the entire contents and handle them as a group
	LLInventoryObject::object_list_t inventory_objects;
	object->getInventoryContents(inventory_objects);

	if (inventory_objects.empty())
	{
		LL_INFOS() << "Object contents not found for drop." << LL_ENDL;
		return FALSE;
	}

	BOOL accept = FALSE;
	BOOL is_move = FALSE;
	BOOL use_filter = FALSE;
	if (filter)
	{
		U64 filter_types = filter->getFilterTypes();
		use_filter = filter_types && (filter_types&LLInventoryFilter::FILTERTYPE_DATE || (filter_types&LLInventoryFilter::FILTERTYPE_OBJECT)==0);
	}

	// coming from a task. Need to figure out if the person can
	// move/copy this item.
	LLInventoryObject::object_list_t::iterator it = inventory_objects.begin();
	LLInventoryObject::object_list_t::iterator end = inventory_objects.end();
	for ( ; it != end; ++it)
	{
		LLInventoryItem* item = dynamic_cast<LLInventoryItem*>(it->get());
		if (!item)
		{
			LL_WARNS() << "Invalid inventory item for drop" << LL_ENDL;
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

		if (accept && use_filter)
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
        boost::shared_ptr<LLMoveInv> move_inv(new LLMoveInv);
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

void LLRightClickInventoryFetchDescendentsObserver::execute(bool clear_observer)
{
	// Bail out immediately if no descendents
	if( mComplete.empty() )
	{
		LL_WARNS() << "LLRightClickInventoryFetchDescendentsObserver::done with empty mCompleteFolders" << LL_ENDL;
		if (clear_observer)
		{
		gInventory.removeObserver(this);
		delete this;
		}
		return;
	}

	// Copy the list of complete fetched folders while "this" is still valid
	uuid_vec_t completed_folder = mComplete;
	
	// Clean up, and remove this as an observer now since recursive calls
	// could notify observers and throw us into an infinite loop.
	if (clear_observer)
	{
		gInventory.removeObserver(this);
		delete this;
	}

	for (uuid_vec_t::iterator current_folder = completed_folder.begin(); current_folder != completed_folder.end(); ++current_folder)
	{
		// Get the information on the fetched folder items and subfolders and fetch those 
		LLInventoryModel::cat_array_t* cat_array;
		LLInventoryModel::item_array_t* item_array;
		gInventory.getDirectDescendentsOf(*current_folder, cat_array, item_array);

		S32 item_count(0);
		if( item_array )
		{			
			item_count = item_array->size();
		}
		
		S32 cat_count(0);
		if( cat_array )
		{			
			cat_count = cat_array->size();
		}

		// Move to next if current folder empty
		if ((item_count == 0) && (cat_count == 0))
		{
			continue;
		}

		uuid_vec_t ids;
		LLRightClickInventoryFetchObserver* outfit = NULL;
		LLRightClickInventoryFetchDescendentsObserver* categories = NULL;

		// Fetch the items
		if (item_count)
		{
			for (S32 i = 0; i < item_count; ++i)
			{
				ids.push_back(item_array->at(i)->getUUID());
			}
			outfit = new LLRightClickInventoryFetchObserver(ids);
		}
		// Fetch the subfolders
		if (cat_count)
		{
			for (S32 i = 0; i < cat_count; ++i)
			{
				ids.push_back(cat_array->at(i)->getUUID());
			}
			categories = new LLRightClickInventoryFetchDescendentsObserver(ids);
		}

		// Perform the item fetch
		if (outfit)
		{
	outfit->startFetch();
			outfit->execute();				// Not interested in waiting and this will be right 99% of the time.
			delete outfit;
//Uncomment the following code for laggy Inventory UI.
			/*
			 if (outfit->isFinished())
	{
	// everything is already here - call done.
				outfit->execute();
				delete outfit;
	}
	else
	{
				// it's all on its way - add an observer, and the inventory
	// will call done for us when everything is here.
	gInventory.addObserver(outfit);
			}
			*/
		}
		// Perform the subfolders fetch : this is where we truly recurse down the folder hierarchy
		if (categories)
		{
			categories->startFetch();
			if (categories->isFinished())
			{
				// everything is already here - call done.
				categories->execute();
				delete categories;
			}
			else
			{
				// it's all on its way - add an observer, and the inventory
				// will call done for us when everything is here.
				gInventory.addObserver(categories);
			}
		}
	}
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
	LLInventoryCopyAndWearObserver(const LLUUID& cat_id, int count, bool folder_added=false, bool replace=false) :
		mCatID(cat_id), mContentsCount(count), mFolderAdded(folder_added), mReplace(replace){}
	virtual ~LLInventoryCopyAndWearObserver() {}
	virtual void changed(U32 mask);

protected:
	LLUUID mCatID;
	int    mContentsCount;
	bool   mFolderAdded;
	bool   mReplace;
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
				LL_WARNS() << "gInventory.getCategory(" << mCatID
						<< ") was NULL" << LL_ENDL;
			}
			else
			{
				if (category->getDescendentCount() ==
				    mContentsCount)
				{
					gInventory.removeObserver(this);
					LLAppearanceMgr::instance().wearInventoryCategory(category, FALSE, !mReplace);
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
		LLFolderViewFolder *f = dynamic_cast<LLFolderViewFolder   *>(mInventoryPanel.get()->getItemByID(mUUID));
		if (f)
		{
			f->toggleOpen();
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
	else if ("addtooutfit" == action)
	{
		modifyOutfit(TRUE);
		return;
	}
	else if ("show_in_main_panel" == action)
	{
		LLInventoryPanel::openInventoryPanelAndSetSelection(TRUE, mUUID, TRUE);
		return;
	}
	else if ("cut" == action)
	{
		cutToClipboard();
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

		LLAppearanceMgr::instance().takeOffOutfit( cat->getLinkedUUID() );
		return;
	}
	else if ("copyoutfittoclipboard" == action)
	{
		copyOutfitToClipboard();
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
	else if ("marketplace_list" == action)
	{
        if (depth_nesting_in_marketplace(mUUID) == 1)
        {
            LLUUID version_folder_id = LLMarketplaceData::instance().getVersionFolder(mUUID);
            LLViewerInventoryCategory* cat = gInventory.getCategory(version_folder_id);
            mMessage = "";
            if (!validate_marketplacelistings(cat,boost::bind(&LLFolderBridge::gatherMessage, this, _1, _2, _3)))
            {
                LLSD subs;
                subs["[ERROR_CODE]"] = mMessage;
                LLNotificationsUtil::add("MerchantListingFailed", subs);
            }
            else
            {
                LLMarketplaceData::instance().activateListing(mUUID,true);
            }
        }
		return;
	}
	else if ("marketplace_activate" == action)
	{
        if (depth_nesting_in_marketplace(mUUID) == 2)
        {
			LLInventoryCategory* category = gInventory.getCategory(mUUID);
            mMessage = "";
            if (!validate_marketplacelistings(category,boost::bind(&LLFolderBridge::gatherMessage, this, _1, _2, _3),false,2))
            {
                LLSD subs;
                subs["[ERROR_CODE]"] = mMessage;
                LLNotificationsUtil::add("MerchantFolderActivationFailed", subs);
            }
            else
            {
                LLMarketplaceData::instance().setVersionFolder(category->getParentUUID(), mUUID);
            }
        }
		return;
	}
	else if ("marketplace_unlist" == action)
	{
        if (depth_nesting_in_marketplace(mUUID) == 1)
        {
            LLMarketplaceData::instance().activateListing(mUUID,false,1);
        }
		return;
	}
	else if ("marketplace_deactivate" == action)
	{
        if (depth_nesting_in_marketplace(mUUID) == 2)
        {
			LLInventoryCategory* category = gInventory.getCategory(mUUID);
            LLMarketplaceData::instance().setVersionFolder(category->getParentUUID(), LLUUID::null, 1);
        }
		return;
	}
	else if ("marketplace_create_listing" == action)
	{
        LLViewerInventoryCategory* cat = gInventory.getCategory(mUUID);
        mMessage = "";
        bool validates = validate_marketplacelistings(cat,boost::bind(&LLFolderBridge::gatherMessage, this, _1, _2, _3),false);
        if (!validates)
        {
            mMessage = "";
            validates = validate_marketplacelistings(cat,boost::bind(&LLFolderBridge::gatherMessage, this, _1, _2, _3),true);
            if (validates)
            {
                LLNotificationsUtil::add("MerchantForceValidateListing");
            }
        }
        
        if (!validates)
        {
            LLSD subs;
            subs["[ERROR_CODE]"] = mMessage;
            LLNotificationsUtil::add("MerchantListingFailed", subs);
        }
        else
        {
            LLMarketplaceData::instance().createListing(mUUID);
        }
		return;
	}
    else if ("marketplace_disassociate_listing" == action)
    {
        LLMarketplaceData::instance().clearListing(mUUID);
		return;
    }
    else if ("marketplace_get_listing" == action)
    {
        // This is used only to exercise the SLM API but won't be shown to end users
        LLMarketplaceData::instance().getListing(mUUID);
		return;
    }
	else if ("marketplace_associate_listing" == action)
	{
        LLFloaterAssociateListing::show(mUUID);
		return;
	}
	else if ("marketplace_check_listing" == action)
	{
        LLSD data(mUUID);
        LLFloaterReg::showInstance("marketplace_validation", data);
		return;
	}
	else if ("marketplace_edit_listing" == action)
	{
        std::string url = LLMarketplaceData::instance().getListingURL(mUUID);
        if (!url.empty())
        {
            LLUrlAction::openURL(url);
        }
		return;
	}
#ifndef LL_RELEASE_FOR_DOWNLOAD
	else if ("delete_system_folder" == action)
	{
		removeSystemFolder();
	}
#endif
	else if (("move_to_marketplace_listings" == action) || ("copy_to_marketplace_listings" == action) || ("copy_or_move_to_marketplace_listings" == action))
	{
		LLInventoryCategory * cat = gInventory.getCategory(mUUID);
		if (!cat) return;
        const LLUUID &marketplacelistings_id = model->findCategoryUUIDForType(LLFolderType::FT_MARKETPLACE_LISTINGS, false);
        move_folder_to_marketplacelistings(cat, marketplacelistings_id, ("move_to_marketplace_listings" != action), (("copy_or_move_to_marketplace_listings" == action)));
    }
}

void LLFolderBridge::gatherMessage(std::string& message, S32 depth, LLError::ELevel log_level)
{
    if (log_level >= LLError::LEVEL_ERROR)
    {
        if (!mMessage.empty())
        {
            // Currently, we do not gather all messages as it creates very long alerts
            // Users can get to the whole list of errors on a listing using the "Check for Errors" audit button or "Check listing" right click menu
            //mMessage += "\n";
            return;
        }
        // Take the leading spaces out...
        std::string::size_type start = message.find_first_not_of(" ");
        // Append the message
        mMessage += message.substr(start, message.length() - start);
    }
}

void LLFolderBridge::copyOutfitToClipboard()
{
	std::string text;

	LLInventoryModel::cat_array_t* cat_array;
	LLInventoryModel::item_array_t* item_array;
	gInventory.getDirectDescendentsOf(mUUID, cat_array, item_array);

	S32 item_count(0);
	if( item_array )
	{			
		item_count = item_array->size();
	}

	if (item_count)
	{
		for (S32 i = 0; i < item_count;)
		{
			LLSD uuid =item_array->at(i)->getUUID();
			LLViewerInventoryItem* item = gInventory.getItem(uuid);

			i++;
			if (item != NULL)
			{
				// Append a newline to all but the last line
				text += i != item_count ? item->getName() + "\n" : item->getName();
			}
		}
	}

	LLClipboard::instance().copyToClipboard(utf8str_to_wstring(text),0,text.size());
}

void LLFolderBridge::openItem()
{
	LL_DEBUGS() << "LLFolderBridge::openItem()" << LL_ENDL;
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
	return getFolderIcon(FALSE);
}

LLUIImagePtr LLFolderBridge::getIconOpen() const
{
	return getFolderIcon(TRUE);
}

LLUIImagePtr LLFolderBridge::getFolderIcon(BOOL is_open) const
{
	LLFolderType::EType preferred_type = getPreferredType();
	return LLUI::getUIImage(LLViewerFolderType::lookupIconName(preferred_type, is_open));
}

// static : use by LLLinkFolderBridge to get the closed type icons
LLUIImagePtr LLFolderBridge::getIcon(LLFolderType::EType preferred_type)
{
	return LLUI::getUIImage(LLViewerFolderType::lookupIconName(preferred_type, FALSE));
}

LLUIImagePtr LLFolderBridge::getIconOverlay() const
{
	if (getInventoryObject() && getInventoryObject()->getIsLinkType())
	{
		return LLUI::getUIImage("Inv_Link");
	}
	return NULL;
}

BOOL LLFolderBridge::renameItem(const std::string& new_name)
{

	LLScrollOnRenameObserver *observer = new LLScrollOnRenameObserver(mUUID, mRoot);
	gInventory.addObserver(observer);

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
		getInventoryModel()->removeCategory(mUUID);
		return TRUE;
	}
	return FALSE;
}

//Recursively update the folder's creation date
void LLFolderBridge::updateHierarchyCreationDate(time_t date)
{
    if(getCreationDate() < date)
    {
        setCreationDate(date);
        if(mParent)
        {
            static_cast<LLFolderBridge *>(mParent)->updateHierarchyCreationDate(date);
        }
    }
}

void LLFolderBridge::pasteFromClipboard()
{
	LLInventoryModel* model = getInventoryModel();
	if (model && isClipboardPasteable())
	{
        const LLUUID &marketplacelistings_id = model->findCategoryUUIDForType(LLFolderType::FT_MARKETPLACE_LISTINGS, false);
        const BOOL paste_into_marketplacelistings = model->isObjectDescendentOf(mUUID, marketplacelistings_id);
        
        BOOL cut_from_marketplacelistings = FALSE;
        if (LLClipboard::instance().isCutMode())
        {
            //Items are not removed from folder on "cut", so we need update listing folder on "paste" operation
            std::vector<LLUUID> objects;
            LLClipboard::instance().pasteFromClipboard(objects);
            for (std::vector<LLUUID>::const_iterator iter = objects.begin(); iter != objects.end(); ++iter)
            {
                const LLUUID& item_id = (*iter);
                if(gInventory.isObjectDescendentOf(item_id, marketplacelistings_id) && (LLMarketplaceData::instance().isInActiveFolder(item_id) ||
                    LLMarketplaceData::instance().isListedAndActive(item_id)))
                {
                    cut_from_marketplacelistings = TRUE;
                    break;
                }
            }
        }
        if (cut_from_marketplacelistings || (paste_into_marketplacelistings && !LLMarketplaceData::instance().isListed(mUUID) && LLMarketplaceData::instance().isInActiveFolder(mUUID)))
        {
            // Prompt the user if pasting in a marketplace active version listing (note that pasting right under the listing folder root doesn't need a prompt)
            LLNotificationsUtil::add("ConfirmMerchantActiveChange", LLSD(), LLSD(), boost::bind(&LLFolderBridge::callback_pasteFromClipboard, this, _1, _2));
        }
        else
        {
            // Otherwise just do the paste
            perform_pasteFromClipboard();
        }
	}
}

// Callback for pasteFromClipboard if DAMA required...
void LLFolderBridge::callback_pasteFromClipboard(const LLSD& notification, const LLSD& response)
{
    S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
    if (option == 0) // YES
    {
        std::vector<LLUUID> objects;
        std::set<LLUUID> parent_folders;
        LLClipboard::instance().pasteFromClipboard(objects);
        for (std::vector<LLUUID>::const_iterator iter = objects.begin(); iter != objects.end(); ++iter)
        {
            const LLInventoryObject* obj = gInventory.getObject(*iter);
            parent_folders.insert(obj->getParentUUID());
        }
        perform_pasteFromClipboard();
        for (std::set<LLUUID>::const_iterator iter = parent_folders.begin(); iter != parent_folders.end(); ++iter)
        {
            gInventory.addChangedMask(LLInventoryObserver::STRUCTURE, *iter);
        }

    }
}

void LLFolderBridge::perform_pasteFromClipboard()
{
	LLInventoryModel* model = getInventoryModel();
	if (model && isClipboardPasteable())
	{
        const LLUUID &current_outfit_id = model->findCategoryUUIDForType(LLFolderType::FT_CURRENT_OUTFIT, false);
        const LLUUID &marketplacelistings_id = model->findCategoryUUIDForType(LLFolderType::FT_MARKETPLACE_LISTINGS, false);
		const LLUUID &favorites_id = model->findCategoryUUIDForType(LLFolderType::FT_FAVORITE, false);
		const LLUUID &my_outifts_id = model->findCategoryUUIDForType(LLFolderType::FT_MY_OUTFITS, false);

		const BOOL move_is_into_current_outfit = (mUUID == current_outfit_id);
		const BOOL move_is_into_my_outfits = (mUUID == my_outifts_id) || model->isObjectDescendentOf(mUUID, my_outifts_id);
		const BOOL move_is_into_outfit = move_is_into_my_outfits || (getCategory() && getCategory()->getPreferredType()==LLFolderType::FT_OUTFIT);
        const BOOL move_is_into_marketplacelistings = model->isObjectDescendentOf(mUUID, marketplacelistings_id);
		const BOOL move_is_into_favorites = (mUUID == favorites_id);

		std::vector<LLUUID> objects;
		LLClipboard::instance().pasteFromClipboard(objects);
        
        LLViewerInventoryCategory * dest_folder = getCategory();
		if (move_is_into_marketplacelistings)
		{
            std::string error_msg;
            const LLViewerInventoryCategory * master_folder = model->getFirstDescendantOf(marketplacelistings_id, mUUID);
            int index = 0;
            for (std::vector<LLUUID>::const_iterator iter = objects.begin(); iter != objects.end(); ++iter)
            {
                const LLUUID& item_id = (*iter);
                LLInventoryItem *item = model->getItem(item_id);
                LLInventoryCategory *cat = model->getCategory(item_id);
                
                if (item && !can_move_item_to_marketplace(master_folder, dest_folder, item, error_msg, objects.size() - index, true))
                {
                    break;
                }
                if (cat && !can_move_folder_to_marketplace(master_folder, dest_folder, cat, error_msg, objects.size() - index, true, true))
                {
                    break;
                }
                ++index;
			}
            if (!error_msg.empty())
            {
                LLSD subs;
                subs["[ERROR_CODE]"] = error_msg;
                LLNotificationsUtil::add("MerchantPasteFailed", subs);
                return;
            }
		}
        else
        {
            // Check that all items can be moved into that folder : for the moment, only stock folder mismatch is checked
            for (std::vector<LLUUID>::const_iterator iter = objects.begin(); iter != objects.end(); ++iter)
            {
                const LLUUID& item_id = (*iter);
                LLInventoryItem *item = model->getItem(item_id);
                LLInventoryCategory *cat = model->getCategory(item_id);

                if ((item && !dest_folder->acceptItem(item)) || (cat && (dest_folder->getPreferredType() == LLFolderType::FT_MARKETPLACE_STOCK)))
                {
                    std::string error_msg = LLTrans::getString("TooltipOutboxMixedStock");
                    LLSD subs;
                    subs["[ERROR_CODE]"] = error_msg;
                    LLNotificationsUtil::add("StockPasteFailed", subs);
                    return;
                }
            }
        }
        
		const LLUUID parent_id(mUUID);
        
		for (std::vector<LLUUID>::const_iterator iter = objects.begin();
			 iter != objects.end();
			 ++iter)
		{
			const LLUUID& item_id = (*iter);
            
			LLInventoryItem *item = model->getItem(item_id);
			LLInventoryObject *obj = model->getObject(item_id);
			if (obj)
			{
				if (move_is_into_current_outfit || move_is_into_outfit)
				{
					if (item && can_move_to_outfit(item, move_is_into_current_outfit))
					{
						dropToOutfit(item, move_is_into_current_outfit);
					}
				}
				else if (move_is_into_favorites)
				{
					if (item && can_move_to_landmarks(item))
					{
						dropToFavorites(item);
					}
				}
				else if (LLClipboard::instance().isCutMode())
				{
					// Do a move to "paste" a "cut"
					// move_inventory_item() is not enough, as we have to update inventory locally too
					if (LLAssetType::AT_CATEGORY == obj->getType())
					{
						LLViewerInventoryCategory* vicat = (LLViewerInventoryCategory *) model->getCategory(item_id);
						llassert(vicat);
						if (vicat)
						{
                            // Clear the cut folder from the marketplace if it is a listing folder
                            if (LLMarketplaceData::instance().isListed(item_id))
                            {
                                LLMarketplaceData::instance().clearListing(item_id);
                            }
                            if (move_is_into_marketplacelistings)
                            {
                                move_folder_to_marketplacelistings(vicat, parent_id);
                            }
                            else
                            {
                                //changeCategoryParent() implicity calls dirtyFilter
                                changeCategoryParent(model, vicat, parent_id, FALSE);
                            }
						}
					}
					else
                    {
                        LLViewerInventoryItem* viitem = dynamic_cast<LLViewerInventoryItem*>(item);
                        llassert(viitem);
                        if (viitem)
                        {
                            if (move_is_into_marketplacelistings)
                            {
                                if (!move_item_to_marketplacelistings(viitem, parent_id))
                                {
                                    // Stop pasting into the marketplace as soon as we get an error
                                    break;
                                }
                            }
                            else
                            {
                                //changeItemParent() implicity calls dirtyFilter
                                changeItemParent(model, viitem, parent_id, FALSE);
                            }
                        }
                    }
				}
				else
				{
					// Do a "copy" to "paste" a regular copy clipboard
					if (LLAssetType::AT_CATEGORY == obj->getType())
					{
						LLViewerInventoryCategory* vicat = (LLViewerInventoryCategory *) model->getCategory(item_id);
						llassert(vicat);
						if (vicat)
						{
                            if (move_is_into_marketplacelistings)
                            {
                                move_folder_to_marketplacelistings(vicat, parent_id, true);
                            }
                            else
                            {
                                copy_inventory_category(model, vicat, parent_id);
                            }
						}
					}
                    else
                    {
                        LLViewerInventoryItem* viitem = dynamic_cast<LLViewerInventoryItem*>(item);
                        llassert(viitem);
                        if (viitem)
                        {
                            if (move_is_into_marketplacelistings)
                            {
                                if (!move_item_to_marketplacelistings(viitem, parent_id, true))
                                {
                                    // Stop pasting into the marketplace as soon as we get an error
                                    break;
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
        }
		// Change mode to paste for next paste
		LLClipboard::instance().setCutMode(false);
	}
}

void LLFolderBridge::pasteLinkFromClipboard()
{
	LLInventoryModel* model = getInventoryModel();
	if(model)
	{
		const LLUUID &current_outfit_id = model->findCategoryUUIDForType(LLFolderType::FT_CURRENT_OUTFIT, false);
        const LLUUID &marketplacelistings_id = model->findCategoryUUIDForType(LLFolderType::FT_MARKETPLACE_LISTINGS, false);
		const LLUUID &my_outifts_id = model->findCategoryUUIDForType(LLFolderType::FT_MY_OUTFITS, false);

		const BOOL move_is_into_current_outfit = (mUUID == current_outfit_id);
		const BOOL move_is_into_my_outfits = (mUUID == my_outifts_id) || model->isObjectDescendentOf(mUUID, my_outifts_id);
		const BOOL move_is_into_outfit = move_is_into_my_outfits || (getCategory() && getCategory()->getPreferredType()==LLFolderType::FT_OUTFIT);
        const BOOL move_is_into_marketplacelistings = model->isObjectDescendentOf(mUUID, marketplacelistings_id);

		if (move_is_into_marketplacelistings)
		{
			// Notify user of failure somehow -- play error sound?  modal dialog?
			return;
		}

		const LLUUID parent_id(mUUID);

		std::vector<LLUUID> objects;
		LLClipboard::instance().pasteFromClipboard(objects);
		for (std::vector<LLUUID>::const_iterator iter = objects.begin();
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
			else if (LLConstPointer<LLInventoryObject> obj = model->getObject(object_id))
			{
				link_inventory_object(parent_id, obj, LLPointer<LLInventoryCallback>(NULL));
			}
		}
		// Change mode to paste for next paste
		LLClipboard::instance().setCutMode(false);
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
	return ((item_array.size() > 0) ? TRUE : FALSE );
}

void LLFolderBridge::buildContextMenuOptions(U32 flags, menuentry_vec_t&   items, menuentry_vec_t& disabled_items)
{
	LLInventoryModel* model = getInventoryModel();
	llassert(model != NULL);

	const LLUUID &trash_id = model->findCategoryUUIDForType(LLFolderType::FT_TRASH);
	const LLUUID &lost_and_found_id = model->findCategoryUUIDForType(LLFolderType::FT_LOST_AND_FOUND);
	const LLUUID &favorites = model->findCategoryUUIDForType(LLFolderType::FT_FAVORITE);
	const LLUUID &marketplace_listings_id = model->findCategoryUUIDForType(LLFolderType::FT_MARKETPLACE_LISTINGS, false);

	if (lost_and_found_id == mUUID)
	{
		// This is the lost+found folder.
		items.push_back(std::string("Empty Lost And Found"));

		LLInventoryModel::cat_array_t* cat_array;
		LLInventoryModel::item_array_t* item_array;
		gInventory.getDirectDescendentsOf(mUUID, cat_array, item_array);
		// Enable Empty menu item only when there is something to act upon.
		if (0 == cat_array->size() && 0 == item_array->size())
		{
			disabled_items.push_back(std::string("Empty Lost And Found"));
		}

		disabled_items.push_back(std::string("New Folder"));
		disabled_items.push_back(std::string("New Script"));
		disabled_items.push_back(std::string("New Note"));
		disabled_items.push_back(std::string("New Gesture"));
		disabled_items.push_back(std::string("New Clothes"));
		disabled_items.push_back(std::string("New Body Parts"));
		disabled_items.push_back(std::string("upload_def"));
	}
	if (favorites == mUUID)
	{
		disabled_items.push_back(std::string("New Folder"));
	}
    if (isMarketplaceListingsFolder())
    {
		addMarketplaceContextMenuOptions(flags, items, disabled_items);
        if (LLMarketplaceData::instance().isUpdating(mUUID))
        {
            disabled_items.push_back(std::string("New Folder"));
            disabled_items.push_back(std::string("Rename"));
            disabled_items.push_back(std::string("Cut"));
            disabled_items.push_back(std::string("Copy"));
            disabled_items.push_back(std::string("Paste"));
            disabled_items.push_back(std::string("Delete"));
        }
    }
    if (getPreferredType() == LLFolderType::FT_MARKETPLACE_STOCK)
    {
        disabled_items.push_back(std::string("New Folder"));
		disabled_items.push_back(std::string("New Script"));
		disabled_items.push_back(std::string("New Note"));
		disabled_items.push_back(std::string("New Gesture"));
		disabled_items.push_back(std::string("New Clothes"));
		disabled_items.push_back(std::string("New Body Parts"));
		disabled_items.push_back(std::string("upload_def"));
    }
    if (marketplace_listings_id == mUUID)
    {
		disabled_items.push_back(std::string("New Folder"));
        disabled_items.push_back(std::string("Rename"));
        disabled_items.push_back(std::string("Cut"));
        disabled_items.push_back(std::string("Delete"));
    }
	if(trash_id == mUUID)
	{
		bool is_recent_panel = false;
		LLInventoryPanel *active_panel = LLInventoryPanel::getActiveInventoryPanel(FALSE);
		if (active_panel && (active_panel->getName() == "Recent Items"))
		{
			is_recent_panel = true;
		}

		// This is the trash.
		items.push_back(std::string("Empty Trash"));

		LLInventoryModel::cat_array_t* cat_array;
		LLInventoryModel::item_array_t* item_array;
		gInventory.getDirectDescendentsOf(mUUID, cat_array, item_array);
		LLViewerInventoryCategory *trash = getCategory();
		// Enable Empty menu item only when there is something to act upon.
		// Also don't enable menu if folder isn't fully fetched
		if ((0 == cat_array->size() && 0 == item_array->size())
			|| is_recent_panel
			|| !trash
			|| trash->getVersion() == LLViewerInventoryCategory::VERSION_UNKNOWN
			|| trash->getDescendentCount() == LLViewerInventoryCategory::VERSION_UNKNOWN)
		{
			disabled_items.push_back(std::string("Empty Trash"));
		}
	}
	else if(isItemInTrash())
	{
		// This is a folder in the trash.
		items.clear(); // clear any items that used to exist
		addTrashContextMenuOptions(items, disabled_items);
	}
	else if(isAgentInventory()) // do not allow creating in library
	{
		LLViewerInventoryCategory *cat = getCategory();
		// BAP removed protected check to re-enable standard ops in untyped folders.
		// Not sure what the right thing is to do here.
		if (!isCOFFolder() && cat && (cat->getPreferredType() != LLFolderType::FT_OUTFIT))
		{
			if (!isInboxFolder()) // don't allow creation in inbox
			{
				// Do not allow to create 2-level subfolder in the Calling Card/Friends folder. EXT-694.
				if (!LLFriendCardsManager::instance().isCategoryInFriendFolder(cat))
				{
					items.push_back(std::string("New Folder"));
				}
                if (!isMarketplaceListingsFolder())
                {
                    items.push_back(std::string("New Script"));
                    items.push_back(std::string("New Note"));
                    items.push_back(std::string("New Gesture"));
                    items.push_back(std::string("New Clothes"));
                    items.push_back(std::string("New Body Parts"));
                    items.push_back(std::string("upload_def"));
                }
			}
			getClipboardEntries(false, items, disabled_items, flags);
		}
		else
		{
			// Want some but not all of the items from getClipboardEntries for outfits.
			if (cat && (cat->getPreferredType() == LLFolderType::FT_OUTFIT))
			{
				items.push_back(std::string("Rename"));

				addDeleteContextMenuOptions(items, disabled_items);
				// EXT-4030: disallow deletion of currently worn outfit
				const LLViewerInventoryItem *base_outfit_link = LLAppearanceMgr::instance().getBaseOutfitLink();
				if (base_outfit_link && (cat == base_outfit_link->getLinkedCategory()))
				{
					disabled_items.push_back(std::string("Delete"));
				}
			}
		}

		if (model->findCategoryUUIDForType(LLFolderType::FT_CURRENT_OUTFIT) == mUUID)
		{
			items.push_back(std::string("Copy outfit list to clipboard"));
		}

		//Added by aura to force inventory pull on right-click to display folder options correctly. 07-17-06
		mCallingCards = mWearables = FALSE;

		LLIsType is_callingcard(LLAssetType::AT_CALLINGCARD);
		if (checkFolderForContentsOfType(model, is_callingcard))
		{
			mCallingCards=TRUE;
		}

		LLFindWearables is_wearable;
		LLIsType is_object( LLAssetType::AT_OBJECT );
		LLIsType is_gesture( LLAssetType::AT_GESTURE );

		if (checkFolderForContentsOfType(model, is_wearable) ||
            checkFolderForContentsOfType(model, is_object)   ||
            checkFolderForContentsOfType(model, is_gesture)    )
		{
			mWearables=TRUE;
		}
	}
	else
	{
		// Mark wearables and allow copy from library
		LLInventoryModel* model = getInventoryModel();
		if(!model) return;
		const LLInventoryCategory* category = model->getCategory(mUUID);
		if (!category) return;
		LLFolderType::EType type = category->getPreferredType();
		const bool is_system_folder = LLFolderType::lookupIsProtectedType(type);

		LLFindWearables is_wearable;
		LLIsType is_object(LLAssetType::AT_OBJECT);
		LLIsType is_gesture(LLAssetType::AT_GESTURE);

		if (checkFolderForContentsOfType(model, is_wearable) ||
			checkFolderForContentsOfType(model, is_object) ||
			checkFolderForContentsOfType(model, is_gesture))
		{
			mWearables = TRUE;
		}

		if (!is_system_folder)
		{
			items.push_back(std::string("Copy"));
			if (!isItemCopyable())
			{
				// For some reason there are items in library that can't be copied directly
				disabled_items.push_back(std::string("Copy"));
			}
		}
	}

	// Preemptively disable system folder removal if more than one item selected.
	if ((flags & FIRST_SELECTED_ITEM) == 0)
	{
		disabled_items.push_back(std::string("Delete System Folder"));
	}

	if (isAgentInventory() && !isMarketplaceListingsFolder())
	{
		items.push_back(std::string("Share"));
		if (!canShare())
		{
			disabled_items.push_back(std::string("Share"));
		}
	}

	

	// Add menu items that are dependent on the contents of the folder.
	LLViewerInventoryCategory* category = (LLViewerInventoryCategory *) model->getCategory(mUUID);
	if (category && (marketplace_listings_id != mUUID))
	{
		uuid_vec_t folders;
		folders.push_back(category->getUUID());

		sSelf = getHandle();
		LLRightClickInventoryFetchDescendentsObserver* fetch = new LLRightClickInventoryFetchDescendentsObserver(folders);
		fetch->startFetch();
		if (fetch->isFinished())
		{
			// Do not call execute() or done() here as if the folder is here, there's likely no point drilling down 
			// This saves lots of time as buildContextMenu() is called a lot
			delete fetch;
			buildContextMenuFolderOptions(flags, items, disabled_items);
		}
		else
		{
			// it's all on its way - add an observer, and the inventory will call done for us when everything is here.
			gInventory.addObserver(fetch);
        }
    }
}

void LLFolderBridge::buildContextMenuFolderOptions(U32 flags,   menuentry_vec_t& items, menuentry_vec_t& disabled_items)
{
	// Build folder specific options back up
	LLInventoryModel* model = getInventoryModel();
	if(!model) return;

	const LLInventoryCategory* category = model->getCategory(mUUID);
	if(!category) return;

	const LLUUID trash_id = model->findCategoryUUIDForType(LLFolderType::FT_TRASH);
	if (trash_id == mUUID) return;
	if (isItemInTrash()) return;
    
	if (!isItemRemovable())
	{
		disabled_items.push_back(std::string("Delete"));
	}
    if (isMarketplaceListingsFolder()) return;

	LLFolderType::EType type = category->getPreferredType();
	const bool is_system_folder = LLFolderType::lookupIsProtectedType(type);
	// BAP change once we're no longer treating regular categories as ensembles.
	const bool is_agent_inventory = isAgentInventory();

	// Only enable calling-card related options for non-system folders.
	if (!is_system_folder && is_agent_inventory)
	{
		LLIsType is_callingcard(LLAssetType::AT_CALLINGCARD);
		if (mCallingCards || checkFolderForContentsOfType(model, is_callingcard))
		{
			items.push_back(std::string("Calling Card Separator"));
			items.push_back(std::string("Conference Chat Folder"));
			items.push_back(std::string("IM All Contacts In Folder"));
		}
	}

#ifndef LL_RELEASE_FOR_DOWNLOAD
	if (LLFolderType::lookupIsProtectedType(type) && is_agent_inventory)
	{
		items.push_back(std::string("Delete System Folder"));
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
		// Only enable add/replace outfit for non-system folders.
		if (!is_system_folder)
		{
			// Adding an outfit onto another (versus replacing) doesn't make sense.
			if (type != LLFolderType::FT_OUTFIT)
			{
				items.push_back(std::string("Add To Outfit"));
			}

			items.push_back(std::string("Replace Outfit"));
		}
		if (is_agent_inventory)
		{
			items.push_back(std::string("Folder Wearables Separator"));
			items.push_back(std::string("Remove From Outfit"));
			if (!LLAppearanceMgr::getCanRemoveFromCOF(mUUID))
			{
					disabled_items.push_back(std::string("Remove From Outfit"));
			}
		}
		if (!LLAppearanceMgr::instance().getCanReplaceCOF(mUUID))
		{
			disabled_items.push_back(std::string("Replace Outfit"));
		}
		if (!LLAppearanceMgr::instance().getCanAddToCOF(mUUID))
		{
			disabled_items.push_back(std::string("Add To Outfit"));
		}
		items.push_back(std::string("Outfit Separator"));

	}
}

// Flags unused
void LLFolderBridge::buildContextMenu(LLMenuGL& menu, U32 flags)
{
	sSelf.markDead();

	// fetch contents of this folder, as context menu can depend on contents
	// still, user would have to open context menu again to see the changes
	gInventory.fetchDescendentsOf(getUUID());


	menuentry_vec_t items;
	menuentry_vec_t disabled_items;

	LL_DEBUGS() << "LLFolderBridge::buildContextMenu()" << LL_ENDL;

	LLInventoryModel* model = getInventoryModel();
	if(!model) return;

	buildContextMenuOptions(flags, items, disabled_items);
    hide_context_entries(menu, items, disabled_items);

	// Reposition the menu, in case we're adding items to an existing menu.
	menu.needsArrange();
	menu.arrangeAndClear();
}

bool LLFolderBridge::hasChildren() const
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

	//LL_INFOS() << "LLFolderBridge::dragOrDrop()" << LL_ENDL;
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
					accept = dragCategoryIntoFolder((LLInventoryCategory*)linked_category, drop, tooltip_msg, TRUE);
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
			LL_WARNS() << "Unhandled cargo type for drag&drop " << cargo_type << LL_ENDL;
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

EInventorySortGroup LLFolderBridge::getSortGroup() const
{
	LLFolderType::EType preferred_type = getPreferredType();

	if (preferred_type == LLFolderType::FT_TRASH)
	{
		return SG_TRASH_FOLDER;
	}

	if(LLFolderType::lookupIsProtectedType(preferred_type))
	{
		return SG_SYSTEM_FOLDER;
	}

	return SG_NORMAL_FOLDER;
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

	// checking amount of items to wear
	U32 max_items = gSavedSettings.getU32("WearFolderLimit");
	LLInventoryModel::cat_array_t cats;
	LLInventoryModel::item_array_t items;
	LLFindWearablesEx not_worn(/*is_worn=*/ false, /*include_body_parts=*/ false);
	gInventory.collectDescendentsIf(cat->getUUID(),
		cats,
		items,
		LLInventoryModel::EXCLUDE_TRASH,
		not_worn);

	if (items.size() > max_items)
	{
		LLSD args;
		args["AMOUNT"] = llformat("%d", max_items);
		LLNotificationsUtil::add("TooManyWearables", args);
		return;
	}

	if (isAgentInventory())
	{
		LLAppearanceMgr::instance().wearInventoryCategory(cat, FALSE, append);
	}
	else
	{
		// Library, we need to copy content first
		LLAppearanceMgr::instance().wearInventoryCategory(cat, TRUE, append);
	}
}


// +=================================================+
// |        LLMarketplaceFolderBridge                |
// +=================================================+

// LLMarketplaceFolderBridge is a specialized LLFolderBridge for use in Marketplace Inventory panels
LLMarketplaceFolderBridge::LLMarketplaceFolderBridge(LLInventoryPanel* inventory,
                          LLFolderView* root,
                          const LLUUID& uuid) :
LLFolderBridge(inventory, root, uuid)
{
    m_depth = depth_nesting_in_marketplace(mUUID);
    m_stockCountCache = COMPUTE_STOCK_NOT_EVALUATED;
}

LLUIImagePtr LLMarketplaceFolderBridge::getIcon() const
{
	return getMarketplaceFolderIcon(FALSE);
}

LLUIImagePtr LLMarketplaceFolderBridge::getIconOpen() const
{
	return getMarketplaceFolderIcon(TRUE);
}

LLUIImagePtr LLMarketplaceFolderBridge::getMarketplaceFolderIcon(BOOL is_open) const
{
	LLFolderType::EType preferred_type = getPreferredType();
    if (!LLMarketplaceData::instance().isUpdating(getUUID()))
    {
        // Skip computation (expensive) if we're waiting for updates. Use the old value in that case.
        m_depth = depth_nesting_in_marketplace(mUUID);
    }
    if ((preferred_type == LLFolderType::FT_NONE) && (m_depth == 2))
    {
        // We override the type when in the marketplace listings folder and only for version folder
        preferred_type = LLFolderType::FT_MARKETPLACE_VERSION;
    }
	return LLUI::getUIImage(LLViewerFolderType::lookupIconName(preferred_type, is_open));
}

std::string LLMarketplaceFolderBridge::getLabelSuffix() const
{
    static LLCachedControl<F32> folder_loading_message_delay(gSavedSettings, "FolderLoadingMessageWaitTime", 0.5f);
    
    if (mIsLoading && mTimeSinceRequestStart.getElapsedTimeF32() >= folder_loading_message_delay())
    {
        return llformat(" ( %s ) ", LLTrans::getString("LoadingData").c_str());
    }
    
    std::string suffix = "";
    // Listing folder case
    if (LLMarketplaceData::instance().isListed(getUUID()))
    {
        suffix = llformat("%d",LLMarketplaceData::instance().getListingID(getUUID()));
        if (suffix.empty())
        {
            suffix = LLTrans::getString("MarketplaceNoID");
        }
        suffix = " (" +  suffix + ")";
        if (LLMarketplaceData::instance().getActivationState(getUUID()))
        {
            suffix += " (" +  LLTrans::getString("MarketplaceLive") + ")";
        }
    }
    // Version folder case
    else if (LLMarketplaceData::instance().isVersionFolder(getUUID()))
    {
        suffix += " (" +  LLTrans::getString("MarketplaceActive") + ")";
    }
    // Add stock amount
    bool updating = LLMarketplaceData::instance().isUpdating(getUUID());
    if (!updating)
    {
        // Skip computation (expensive) if we're waiting for update anyway. Use the old value in that case.
        m_stockCountCache = compute_stock_count(getUUID());
    }
    if (m_stockCountCache == 0)
    {
        suffix += " (" +  LLTrans::getString("MarketplaceNoStock") + ")";
    }
    else if (m_stockCountCache != COMPUTE_STOCK_INFINITE)
    {
        if (getPreferredType() == LLFolderType::FT_MARKETPLACE_STOCK)
        {
            suffix += " (" +  LLTrans::getString("MarketplaceStock");
        }
        else
        {
            suffix += " (" +  LLTrans::getString("MarketplaceMax");
        }
        if (m_stockCountCache == COMPUTE_STOCK_NOT_EVALUATED)
        {
            suffix += "=" + LLTrans::getString("MarketplaceUpdating") + ")";
        }
        else
        {
            suffix +=  "=" + llformat("%d", m_stockCountCache) + ")";
        }
    }
    // Add updating suffix
    if (updating)
    {
        suffix += " (" +  LLTrans::getString("MarketplaceUpdating") + ")";
    }
    return LLInvFVBridge::getLabelSuffix() + suffix;
}

LLFontGL::StyleFlags LLMarketplaceFolderBridge::getLabelStyle() const
{
    return (LLMarketplaceData::instance().getActivationState(getUUID()) ? LLFontGL::BOLD : LLFontGL::NORMAL);
}




// helper stuff
bool move_task_inventory_callback(const LLSD& notification, const LLSD& response, boost::shared_ptr<LLMoveInv> move_inv)
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
			LLInventoryCopyAndWearObserver* inventoryObserver = new LLInventoryCopyAndWearObserver(cat_and_wear->mCatID, contents_count, cat_and_wear->mFolderResponded,
																									cat_and_wear->mReplace);
			
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

	move_inv.reset(); //since notification will persist
	return false;
}

// Returns true if the item can be moved to Current Outfit or any outfit folder.
static BOOL can_move_to_outfit(LLInventoryItem* inv_item, BOOL move_is_into_current_outfit)
{
	LLInventoryType::EType inv_type = inv_item->getInventoryType();
	if ((inv_type != LLInventoryType::IT_WEARABLE) &&
		(inv_type != LLInventoryType::IT_GESTURE) &&
		(inv_type != LLInventoryType::IT_ATTACHMENT) &&
		(inv_type != LLInventoryType::IT_OBJECT) &&
		(inv_type != LLInventoryType::IT_SNAPSHOT) &&
		(inv_type != LLInventoryType::IT_TEXTURE))
	{
		return FALSE;
	}

	U32 flags = inv_item->getFlags();
	if(flags & LLInventoryItemFlags::II_FLAGS_OBJECT_HAS_MULTIPLE_ITEMS)
	{
		return FALSE;
	}

	if((inv_type == LLInventoryType::IT_TEXTURE) || (inv_type == LLInventoryType::IT_SNAPSHOT))
	{
		return !move_is_into_current_outfit;
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
	LLFolderViewModelItemInventory* view_model = drag_over_item ? static_cast<LLFolderViewModelItemInventory*>(drag_over_item->getViewModelItem()) : NULL;
	if (view_model)
	{
		cb.get()->setTargetLandmarkId(view_model->getUUID());
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
	if((inv_item->getInventoryType() == LLInventoryType::IT_TEXTURE) || (inv_item->getInventoryType() == LLInventoryType::IT_SNAPSHOT))
	{
		const LLUUID &my_outifts_id = getInventoryModel()->findCategoryUUIDForType(LLFolderType::FT_MY_OUTFITS, false);
		if(mUUID != my_outifts_id)
		{
			LLFloaterOutfitPhotoPreview* photo_preview  = LLFloaterReg::showTypedInstance<LLFloaterOutfitPhotoPreview>("outfit_photo_preview", inv_item->getUUID());
			if(photo_preview)
			{
				photo_preview->setOutfitID(mUUID);
			}
		}
		return;
	}

	// BAP - should skip if dup.
	if (move_is_into_current_outfit)
	{
		LLAppearanceMgr::instance().wearItemOnAvatar(inv_item->getUUID(), true, true);
	}
	else
	{
		LLPointer<LLInventoryCallback> cb = NULL;
		link_inventory_object(mUUID, LLConstPointer<LLInventoryObject>(inv_item), cb);
	}
}

// Callback for drop item if DAMA required...
void LLFolderBridge::callback_dropItemIntoFolder(const LLSD& notification, const LLSD& response, LLInventoryItem* inv_item)
{
    S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
    if (option == 0) // YES
    {
        std::string tooltip_msg;
        dragItemIntoFolder(inv_item, TRUE, tooltip_msg, FALSE);
    }
}

// Callback for drop category if DAMA required...
void LLFolderBridge::callback_dropCategoryIntoFolder(const LLSD& notification, const LLSD& response, LLInventoryCategory* inv_category)
{
    S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
    if (option == 0) // YES
    {
        std::string tooltip_msg;
		dragCategoryIntoFolder(inv_category, TRUE, tooltip_msg, FALSE, FALSE);
    }
}

// This is used both for testing whether an item can be dropped
// into the folder, as well as performing the actual drop, depending
// if drop == TRUE.
BOOL LLFolderBridge::dragItemIntoFolder(LLInventoryItem* inv_item,
										BOOL drop,
										std::string& tooltip_msg,
                                        BOOL user_confirm)
{
	LLInventoryModel* model = getInventoryModel();

	if (!model || !inv_item) return FALSE;
	if (!isAgentInventory()) return FALSE; // cannot drag into library
	if (!isAgentAvatarValid()) return FALSE;

	LLInventoryPanel* destination_panel = mInventoryPanel.get();
	if (!destination_panel) return false;

	LLInventoryFilter* filter = getInventoryFilter();
	if (!filter) return false;

	const LLUUID &current_outfit_id = model->findCategoryUUIDForType(LLFolderType::FT_CURRENT_OUTFIT, false);
	const LLUUID &favorites_id = model->findCategoryUUIDForType(LLFolderType::FT_FAVORITE, false);
	const LLUUID &landmarks_id = model->findCategoryUUIDForType(LLFolderType::FT_LANDMARK, false);
	const LLUUID &marketplacelistings_id = model->findCategoryUUIDForType(LLFolderType::FT_MARKETPLACE_LISTINGS, false);
	const LLUUID &my_outifts_id = model->findCategoryUUIDForType(LLFolderType::FT_MY_OUTFITS, false);
    const LLUUID from_folder_uuid = inv_item->getParentUUID();

	const BOOL move_is_into_current_outfit = (mUUID == current_outfit_id);
	const BOOL move_is_into_favorites = (mUUID == favorites_id);
	const BOOL move_is_into_my_outfits = (mUUID == my_outifts_id) || model->isObjectDescendentOf(mUUID, my_outifts_id);
	const BOOL move_is_into_outfit = move_is_into_my_outfits || (getCategory() && getCategory()->getPreferredType()==LLFolderType::FT_OUTFIT);
	const BOOL move_is_into_landmarks = (mUUID == landmarks_id) || model->isObjectDescendentOf(mUUID, landmarks_id);
    const BOOL move_is_into_marketplacelistings = model->isObjectDescendentOf(mUUID, marketplacelistings_id);
    const BOOL move_is_from_marketplacelistings = model->isObjectDescendentOf(inv_item->getUUID(), marketplacelistings_id);

	LLToolDragAndDrop::ESource source = LLToolDragAndDrop::getInstance()->getSource();
	BOOL accept = FALSE;
	U64 filter_types = filter->getFilterTypes();
	// We shouldn't allow to drop non recent items into recent tab (or some similar transactions)
	// while we are allowing to interact with regular filtered inventory
	BOOL use_filter = filter_types && (filter_types&LLInventoryFilter::FILTERTYPE_DATE || (filter_types&LLInventoryFilter::FILTERTYPE_OBJECT)==0);
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
		// Note: if user_confirm is false, we already went through those accept logic test and can skip them

		accept = TRUE;

		if (user_confirm && !is_movable)
		{
			accept = FALSE;
		}
		else if (user_confirm && (mUUID == inv_item->getParentUUID()) && !move_is_into_favorites)
		{
			accept = FALSE;
		}
		else if (user_confirm && (move_is_into_current_outfit || move_is_into_outfit))
		{
			accept = can_move_to_outfit(inv_item, move_is_into_current_outfit);
		}
		else if (user_confirm && (move_is_into_favorites || move_is_into_landmarks))
		{
			accept = can_move_to_landmarks(inv_item);
		}
		else if (user_confirm && move_is_into_marketplacelistings)
		{
            const LLViewerInventoryCategory * master_folder = model->getFirstDescendantOf(marketplacelistings_id, mUUID);
            LLViewerInventoryCategory * dest_folder = getCategory();
            accept = can_move_item_to_marketplace(master_folder, dest_folder, inv_item, tooltip_msg, LLToolDragAndDrop::instance().getCargoCount() - LLToolDragAndDrop::instance().getCargoIndex());
		}

        // Check that the folder can accept this item based on folder/item type compatibility (e.g. stock folder compatibility)
        if (user_confirm && accept)
        {
            LLViewerInventoryCategory * dest_folder = getCategory();
            accept = dest_folder->acceptItem(inv_item);
        }
        
		LLInventoryPanel* active_panel = LLInventoryPanel::getActiveInventoryPanel(FALSE);

		// Check whether the item being dragged from active inventory panel
		// passes the filter of the destination panel.
		if (user_confirm && accept && active_panel && use_filter)
		{
			LLFolderViewItem* fv_item =   active_panel->getItemByID(inv_item->getUUID());
			if (!fv_item) return false;

			accept = filter->check(fv_item->getViewModelItem());
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
                        // RN: a better solution would be to deselect automatically when an   item is moved
			// and then select any item that is dropped only in the panel that it   is dropped in
			if (active_panel && (destination_panel != active_panel))
            {
                active_panel->unSelectAll();
            }
            // Dropping in or out of marketplace needs (sometimes) confirmation
            if (user_confirm && (move_is_from_marketplacelistings || move_is_into_marketplacelistings))
            {
                if ((move_is_from_marketplacelistings && (LLMarketplaceData::instance().isInActiveFolder(inv_item->getUUID())
                                                       || LLMarketplaceData::instance().isListedAndActive(inv_item->getUUID()))) ||
                    (move_is_into_marketplacelistings && LLMarketplaceData::instance().isInActiveFolder(mUUID)))
                {
                    LLNotificationsUtil::add("ConfirmMerchantActiveChange", LLSD(), LLSD(), boost::bind(&LLFolderBridge::callback_dropItemIntoFolder, this, _1, _2, inv_item));
                    return true;
                }
                if (move_is_into_marketplacelistings && !move_is_from_marketplacelistings)
                {
                    LLNotificationsUtil::add("ConfirmMerchantMoveInventory", LLSD(), LLSD(), boost::bind(&LLFolderBridge::callback_dropItemIntoFolder, this, _1, _2, inv_item));
                    return true;
                }
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
					LLUUID destItemId = static_cast<LLFolderViewModelItemInventory*>(itemp->getViewModelItem())->getUUID();
					LLFavoritesOrderStorage::instance().rearrangeFavoriteLandmarks(srcItemId, destItemId);
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
            // MARKETPLACE LISTINGS folder
            // Move the item
            else if (move_is_into_marketplacelistings)
            {
                move_item_to_marketplacelistings(inv_item, mUUID);
            }
			// NORMAL or TRASH folder
			// (move the item, restamp if into trash)
			else
			{
				// set up observer to select item once drag and drop from inbox is complete 
				if (gInventory.isObjectDescendentOf(inv_item->getUUID(), gInventory.findCategoryUUIDForType(LLFolderType::FT_INBOX, false)))
				{
					set_dad_inbox_object(inv_item->getUUID());
				}

				LLInvFVBridge::changeItemParent(
					model,
					(LLViewerInventoryItem*)inv_item,
					mUUID,
					move_is_into_trash);
			}
            
            if (move_is_from_marketplacelistings)
            {
                // If we move from an active (listed) listing, checks that it's still valid, if not, unlist
                LLUUID version_folder_id = LLMarketplaceData::instance().getActiveFolder(from_folder_uuid);
                if (version_folder_id.notNull())
                {
                    LLViewerInventoryCategory* cat = gInventory.getCategory(version_folder_id);
                    if (!validate_marketplacelistings(cat,NULL))
                    {
                        LLMarketplaceData::instance().activateListing(version_folder_id,false);
                    }
                }
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
			LL_INFOS() << "Object not found for drop." << LL_ENDL;
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
		else if (move_is_into_marketplacelistings)
		{
			tooltip_msg = LLTrans::getString("TooltipOutboxNotInInventory");
			accept = FALSE;
		}
		
		// Check whether the item being dragged from in world
		// passes the filter of the destination panel.
		if (accept && use_filter)
		{
			accept = filter->check(inv_item);
		}

		if (accept && drop)
		{
            boost::shared_ptr<LLMoveInv> move_inv (new LLMoveInv());
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
		if (move_is_into_marketplacelistings)
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
		if (accept && use_filter)
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

			if (move_is_into_marketplacelistings)
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
			if (accept && active_panel && use_filter)
			{
				LLFolderViewItem* fv_item =   active_panel->getItemByID(inv_item->getUUID());
				if (!fv_item) return false;

				accept = filter->check(fv_item->getViewModelItem());
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
		LL_WARNS() << "unhandled drag source" << LL_ENDL;
	}
	return accept;
}

// static
bool check_category(LLInventoryModel* model,
					const LLUUID& cat_id,
					LLInventoryPanel* active_panel,
					LLInventoryFilter* filter)
{
	if (!model || !active_panel || !filter)
		return false;

	if (!filter->checkFolder(cat_id))
	{
		return false;
	}

	LLInventoryModel::cat_array_t descendent_categories;
	LLInventoryModel::item_array_t descendent_items;
	model->collectDescendents(cat_id, descendent_categories, descendent_items, TRUE);

	S32 num_descendent_categories = descendent_categories.size();
	S32 num_descendent_items = descendent_items.size();

	if (num_descendent_categories + num_descendent_items == 0)
	{
		// Empty folder should be checked as any other folder view item.
		// If we are filtering by date the folder should not pass because
		// it doesn't have its own creation date. See LLInvFVBridge::getCreationDate().
		return check_item(cat_id, active_panel, filter);
	}

	for (S32 i = 0; i < num_descendent_categories; ++i)
	{
		LLInventoryCategory* category = descendent_categories[i];
		if(!check_category(model, category->getUUID(), active_panel, filter))
		{
			return false;
		}
	}

	for (S32 i = 0; i < num_descendent_items; ++i)
	{
		LLViewerInventoryItem* item = descendent_items[i];
		if(!check_item(item->getUUID(), active_panel, filter))
		{
			return false;
		}
	}

	return true;
}

// static
bool check_item(const LLUUID& item_id,
				LLInventoryPanel* active_panel,
				LLInventoryFilter* filter)
{
	if (!active_panel || !filter) return false;

	LLFolderViewItem* fv_item = active_panel->getItemByID(item_id);
	if (!fv_item) return false;

	return filter->check(fv_item->getViewModelItem());
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
	LL_DEBUGS() << "LLTextureBridge::buildContextMenu()" << LL_ENDL;
	menuentry_vec_t items;
	menuentry_vec_t disabled_items;
	if(isItemInTrash())
	{
		addTrashContextMenuOptions(items, disabled_items);
	}	
    else if (isMarketplaceListingsFolder())
    {
		addMarketplaceContextMenuOptions(flags, items, disabled_items);
		items.push_back(std::string("Properties"));
		getClipboardEntries(false, items, disabled_items, flags);
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
	addLinkReplaceMenuOption(items, disabled_items);
	hide_context_entries(menu, items, disabled_items);	
}

// virtual
void LLTextureBridge::performAction(LLInventoryModel* model, std::string action)
{
	if ("save_as" == action)
	{
		LLPreviewTexture* preview_texture = LLFloaterReg::getTypedInstance<LLPreviewTexture>("preview_texture", mUUID);
		if (preview_texture)
		{
			preview_texture->openToSave();
			preview_texture->saveAs();
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

void LLSoundBridge::openSoundPreview(void* which)
{
	LLSoundBridge *me = (LLSoundBridge *)which;
	LLFloaterReg::showInstance("preview_sound", LLSD(me->mUUID), TAKE_FOCUS_YES);
}

void LLSoundBridge::buildContextMenu(LLMenuGL& menu, U32 flags)
{
	LL_DEBUGS() << "LLSoundBridge::buildContextMenu()" << LL_ENDL;
	menuentry_vec_t items;
	menuentry_vec_t disabled_items;

    if (isMarketplaceListingsFolder())
    {
		addMarketplaceContextMenuOptions(flags, items, disabled_items);
		items.push_back(std::string("Properties"));
		getClipboardEntries(false, items, disabled_items, flags);
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

	addLinkReplaceMenuOption(items, disabled_items);
	hide_context_entries(menu, items, disabled_items);
}

void LLSoundBridge::performAction(LLInventoryModel* model, std::string action)
{
	if ("sound_play" == action)
	{
		LLViewerInventoryItem* item = getItem();
		if(item)
		{
			send_sound_trigger(item->getAssetUUID(), SOUND_GAIN);
		}
	}
	else if ("open" == action)
	{
		openSoundPreview((void*)this);
	}
	else LLItemBridge::performAction(model, action);
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

	LL_DEBUGS() << "LLLandmarkBridge::buildContextMenu()" << LL_ENDL;
    if (isMarketplaceListingsFolder())
    {
		addMarketplaceContextMenuOptions(flags, items, disabled_items);
		items.push_back(std::string("Properties"));
		getClipboardEntries(false, items, disabled_items, flags);
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
		items.push_back(std::string("url_copy"));
		items.push_back(std::string("About Landmark"));
		items.push_back(std::string("show_on_map"));
	}

	// Disable "About Landmark" menu item for
	// multiple landmarks selected. Only one landmark
	// info panel can be shown at a time.
	if ((flags & FIRST_SELECTED_ITEM) == 0)
	{
		disabled_items.push_back(std::string("url_copy"));
		disabled_items.push_back(std::string("About Landmark"));
	}

	addLinkReplaceMenuOption(items, disabled_items);
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
		if (mask & LLFriendObserver::ONLINE)
		{
			mBridgep->checkSearchBySuffixChanges();
		}
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
	LLFolderViewItem* itemp = panel ? panel->getItemByID(mUUID) : NULL;
	if (itemp)
	{
		itemp->refresh();
	}
}

void LLCallingCardBridge::checkSearchBySuffixChanges()
{
	if (!mDisplayName.empty())
	{
		// changes in mDisplayName are processed by rename function and here it will be always same
		// suffixes are also of fixed length, and we are processing change of one at a time,
		// so it should be safe to use length (note: mSearchableName is capitalized)
		S32 old_length = mSearchableName.length();
		S32 new_length = mDisplayName.length() + getLabelSuffix().length();
		if (old_length == new_length)
		{
			return;
		}
		mSearchableName.assign(mDisplayName);
		mSearchableName.append(getLabelSuffix());
		LLStringUtil::toUpper(mSearchableName);
		if (new_length<old_length)
		{
			LLInventoryFilter* filter = getInventoryFilter();
			if (filter && mPassedFilter && mSearchableName.find(filter->getFilterSubString()) == std::string::npos)
			{
				// string no longer contains substring 
				// we either have to update all parents manually or restart filter.
				// dirtyFilter will not work here due to obsolete descendants' generations 
				getInventoryFilter()->setModified(LLFolderViewFilter::FILTER_MORE_RESTRICTIVE);
			}
		}
		else
		{
			if (getInventoryFilter())
			{
				// mSearchableName became longer, we gained additional suffix and need to repeat filter check.
				dirtyFilter();
			}
		}
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
			std::string callingcard_name = LLCacheName::getDefaultName();
			LLAvatarName av_name;
			if (LLAvatarNameCache::get(item->getCreatorUUID(), &av_name))
			{
				callingcard_name = av_name.getCompleteName();
			}
			LLUUID session_id = gIMMgr->addSession(callingcard_name, IM_NOTHING_SPECIAL, item->getCreatorUUID());
			if (session_id != LLUUID::null)
			{
				LLFloaterIMContainer::getInstance()->showConversation(session_id);
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
	else if ("request_lure" == action)
	{
		LLViewerInventoryItem *item = getItem();
		if (item && (item->getCreatorUUID() != gAgent.getID()) &&
			(!item->getCreatorUUID().isNull()))
		{
			LLAvatarActions::teleportRequest(item->getCreatorUUID());
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
	LL_DEBUGS() << "LLCallingCardBridge::buildContextMenu()" << LL_ENDL;
	menuentry_vec_t items;
	menuentry_vec_t disabled_items;

	if(isItemInTrash())
	{
		addTrashContextMenuOptions(items, disabled_items);
	}	
    else if (isMarketplaceListingsFolder())
    {
		addMarketplaceContextMenuOptions(flags, items, disabled_items);
		items.push_back(std::string("Properties"));
		getClipboardEntries(false, items, disabled_items, flags);
    }
	else
	{
		items.push_back(std::string("Share"));
		if (!canShare())
		{
			disabled_items.push_back(std::string("Share"));
		}
		if ((flags & FIRST_SELECTED_ITEM) == 0)
		{
		disabled_items.push_back(std::string("Open"));
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
		items.push_back(std::string("Request Teleport..."));
		items.push_back(std::string("Conference Chat"));

		if (!good_card)
		{
			disabled_items.push_back(std::string("Send Instant Message"));
		}
		if (!good_card || !user_online)
		{
			disabled_items.push_back(std::string("Offer Teleport..."));
			disabled_items.push_back(std::string("Request Teleport..."));
			disabled_items.push_back(std::string("Conference Chat"));
		}
	}
	addLinkReplaceMenuOption(items, disabled_items);
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

void LLNotecardBridge::buildContextMenu(LLMenuGL& menu, U32 flags)
{
	LL_DEBUGS() << "LLNotecardBridge::buildContextMenu()" << LL_ENDL;
    
    if (isMarketplaceListingsFolder())
    {
        menuentry_vec_t items;
        menuentry_vec_t disabled_items;
		addMarketplaceContextMenuOptions(flags, items, disabled_items);
		items.push_back(std::string("Properties"));
		getClipboardEntries(false, items, disabled_items, flags);
        hide_context_entries(menu, items, disabled_items);
    }
	else
	{
        LLItemBridge::buildContextMenu(menu, flags);
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
	else if ("deactivate" == action || isRemoveAction(action))
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
	LL_DEBUGS() << "LLGestureBridge::buildContextMenu()" << LL_ENDL;
	menuentry_vec_t items;
	menuentry_vec_t disabled_items;
	if(isItemInTrash())
	{
		addTrashContextMenuOptions(items, disabled_items);
	}
    else if (isMarketplaceListingsFolder())
    {
		addMarketplaceContextMenuOptions(flags, items, disabled_items);
		items.push_back(std::string("Properties"));
		getClipboardEntries(false, items, disabled_items, flags);
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
	addLinkReplaceMenuOption(items, disabled_items);
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

	LL_DEBUGS() << "LLAnimationBridge::buildContextMenu()" << LL_ENDL;
    if (isMarketplaceListingsFolder())
    {
		addMarketplaceContextMenuOptions(flags, items, disabled_items);
		items.push_back(std::string("Properties"));
		getClipboardEntries(false, items, disabled_items, flags);
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

	addLinkReplaceMenuOption(items, disabled_items);
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
			LLPointer<LLInventoryCallback> cb = new LLBoostFuncInventoryCallback(boost::bind(rez_attachment_cb, _1, (LLViewerJointAttachment*)0));
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
		LLAppearanceMgr::instance().removeItemFromAvatar(mUUID);
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
		std::string attachment_point_name;
		if (gAgentAvatarp->getAttachedPointName(mUUID, attachment_point_name))
		{
			LLStringUtil::format_map_t args;
			args["[ATTACHMENT_POINT]"] =  LLTrans::getString(attachment_point_name);

			return LLItemBridge::getLabelSuffix() + LLTrans::getString("WornOnAttachmentPoint", args);
		}
		else
		{
			LLStringUtil::format_map_t args;
			args["[ATTACHMENT_ERROR]"] =  LLTrans::getString(attachment_point_name);
			return LLItemBridge::getLabelSuffix() + LLTrans::getString("AttachmentErrorMessage", args);
		}
	}
	return LLItemBridge::getLabelSuffix();
}

void rez_attachment(LLViewerInventoryItem* item, LLViewerJointAttachment* attachment, bool replace)
{
	const LLUUID& item_id = item->getLinkedUUID();

	// Check for duplicate request.
	if (isAgentAvatarValid() &&
		gAgentAvatarp->isWearingAttachment(item_id))
	{
		LL_WARNS() << "ATT duplicate attachment request, ignoring" << LL_ENDL;
		return;
	}

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
			// Queue up attachments to be sent in next idle tick, this way the
			// attachments are batched up all into one message versus each attachment
			// being sent in its own separate attachments message.
			U8 attachment_pt = notification["payload"]["attachment_point"].asInteger();
			BOOL is_add = notification["payload"]["is_add"].asBoolean();

			LL_DEBUGS("Avatar") << "ATT calling addAttachmentRequest " << (itemp ? itemp->getName() : "UNKNOWN") << " id " << item_id << LL_ENDL;
			LLAttachmentsMgr::instance().addAttachmentRequest(item_id, attachment_pt, is_add);
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
    else if (isMarketplaceListingsFolder())
    {
		addMarketplaceContextMenuOptions(flags, items, disabled_items);
		items.push_back(std::string("Properties"));
		getClipboardEntries(false, items, disabled_items, flags);
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
						items.push_back(p.name);
					}
				}
			}
		}
	}
	addLinkReplaceMenuOption(items, disabled_items);
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
		new_item->updateServer(FALSE);
		model->updateItem(new_item);
		model->notifyObservers();
		buildDisplayName();

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
	performAction(getInventoryModel(),
			      get_is_item_worn(mUUID) ? "take_off" : "wear");
}

void LLWearableBridge::buildContextMenu(LLMenuGL& menu, U32 flags)
{
	LL_DEBUGS() << "LLWearableBridge::buildContextMenu()" << LL_ENDL;
	menuentry_vec_t items;
	menuentry_vec_t disabled_items;
	if(isItemInTrash())
	{
		addTrashContextMenuOptions(items, disabled_items);
	}
    else if (isMarketplaceListingsFolder())
    {
		addMarketplaceContextMenuOptions(flags, items, disabled_items);
		items.push_back(std::string("Properties"));
		getClipboardEntries(false, items, disabled_items, flags);
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

		bool modifiable = !gAgentWearables.isWearableModifiable(item->getUUID());
		if (((flags & FIRST_SELECTED_ITEM) == 0) || modifiable)
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
						if (!gAgentWearables.canAddWearable(mWearableType))
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
	addLinkReplaceMenuOption(items, disabled_items);
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
void LLWearableBridge::onWearOnAvatarArrived( LLViewerWearable* wearable, void* userdata )
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
				LL_INFOS() << "By the time wearable asset arrived, its inv item already pointed to a different asset." << LL_ENDL;
			}
		}
	}
	delete item_id;
}

// static
// BAP remove the "add" code path once everything is fully COF-ified.
void LLWearableBridge::onWearAddOnAvatarArrived( LLViewerWearable* wearable, void* userdata )
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
				LL_INFOS() << "By the time wearable asset arrived, its inv item already pointed to a different asset." << LL_ENDL;
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

void LLWearableBridge::removeFromAvatar()
{
	LL_WARNS() << "safe to remove?" << LL_ENDL;
	if (get_is_item_worn(mUUID))
	{
		LLAppearanceMgr::instance().removeItemFromAvatar(mUUID);
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
	LL_DEBUGS() << "LLLink::buildContextMenu()" << LL_ENDL;
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
	addLinkReplaceMenuOption(items, disabled_items);
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

void LLMeshBridge::buildContextMenu(LLMenuGL& menu, U32 flags)
{
	LL_DEBUGS() << "LLMeshBridge::buildContextMenu()" << LL_ENDL;
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
    else if (isMarketplaceListingsFolder())
    {
		addMarketplaceContextMenuOptions(flags, items, disabled_items);
		items.push_back(std::string("Properties"));
		getClipboardEntries(false, items, disabled_items, flags);
    }
	else
	{
		items.push_back(std::string("Properties"));

		getClipboardEntries(true, items, disabled_items, flags);
	}

	addLinkReplaceMenuOption(items, disabled_items);
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
	LL_DEBUGS() << "LLLink::buildContextMenu()" << LL_ENDL;
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
		LLFolderViewItem *base_folder = mInventoryPanel.get()->getItemByID(cat_uuid);
		if (base_folder)
		{
			if (LLInventoryModel* model = getInventoryModel())
			{
				model->fetchDescendentsOf(cat_uuid);
			}
			base_folder->setOpen(TRUE);
			mRoot->setSelection(base_folder,TRUE);
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
			send_sound_trigger(item->getAssetUUID(), SOUND_GAIN);
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
	menuentry_vec_t disabled_items, items;
        buildContextMenuOptions(flags, items, disabled_items);

	items.erase(std::remove(items.begin(), items.end(), std::string("New Folder")), items.end());

	hide_context_entries(menu, items, disabled_items);
}

LLInvFVBridge* LLRecentInventoryBridgeBuilder::createBridge(
	LLAssetType::EType asset_type,
	LLAssetType::EType actual_asset_type,
	LLInventoryType::EType inv_type,
	LLInventoryPanel* inventory,
	LLFolderViewModelInventory* view_model,
	LLFolderView* root,
	const LLUUID& uuid,
	U32 flags /*= 0x00*/ ) const
{
	LLInvFVBridge* new_listener = NULL;
	if (asset_type == LLAssetType::AT_CATEGORY 
		&& actual_asset_type != LLAssetType::AT_LINK_FOLDER)
	{
		new_listener = new LLRecentItemsFolderBridge(inv_type, inventory, root, uuid);
	}
	else
		{
		new_listener = LLInventoryFolderViewModelBuilder::createBridge(asset_type,
				actual_asset_type,
				inv_type,
				inventory,
																view_model,
				root,
				uuid,
				flags);
		}
	return new_listener;
}

LLFolderViewGroupedItemBridge::LLFolderViewGroupedItemBridge()
{
}

void LLFolderViewGroupedItemBridge::groupFilterContextMenu(folder_view_item_deque& selected_items, LLMenuGL& menu)
{
    uuid_vec_t ids;
	menuentry_vec_t disabled_items;
    if (get_selection_item_uuids(selected_items, ids))
    {
        if (!LLAppearanceMgr::instance().canAddWearables(ids))
        {
			disabled_items.push_back(std::string("Wearable Add"));
        }
    }
	disable_context_entries_if_present(menu, disabled_items);
}

// EOF
