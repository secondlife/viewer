/** 
 * @file llinventorybridge.cpp
 * @brief Implementation of the Inventory-Folder-View-Bridge classes.
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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

#include "llviewerprecompiledheaders.h"

#include <utility> // for std::pair<>

#include "llinventoryview.h"
#include "llinventorybridge.h"

#include "message.h"

#include "llagent.h"
#include "llcallingcard.h"
#include "llcheckboxctrl.h"		// for radio buttons
#include "llradiogroup.h"
#include "llspinctrl.h"
#include "lltextbox.h"
#include "llui.h"

#include "llviewercontrol.h"
#include "llfirstuse.h"
#include "llfloateravatarinfo.h"
#include "llfloaterchat.h"
#include "llfloatercustomize.h"
#include "llfloaterproperties.h"
#include "llfloaterworldmap.h"
#include "llfocusmgr.h"
#include "llfolderview.h"
#include "llgesturemgr.h"
#include "lliconctrl.h"
#include "llinventorymodel.h"
#include "llinventoryclipboard.h"
#include "lllineeditor.h"
#include "llmenugl.h"
#include "llpreviewanim.h"
#include "llpreviewgesture.h"
#include "llpreviewlandmark.h"
#include "llpreviewnotecard.h"
#include "llpreviewscript.h"
#include "llpreviewsound.h"
#include "llpreviewtexture.h"
#include "llresmgr.h"
#include "llscrollcontainer.h"
#include "llimview.h"
#include "lltooldraganddrop.h"
#include "llviewerimagelist.h"
#include "llviewerinventory.h"
#include "llviewerobjectlist.h"
#include "llviewerwindow.h"
#include "llvoavatar.h"
#include "llwearable.h"
#include "llwearablelist.h"
#include "llviewermessage.h" 
#include "llviewerregion.h"
#include "lltabcontainer.h"
#include "lluictrlfactory.h"
#include "llselectmgr.h"
#include "llfloateropenobject.h"

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
struct LLWearableHoldingPattern;
void wear_inventory_category_on_avatar(LLInventoryCategory* category, BOOL append);
void wear_inventory_category_on_avatar_step2( BOOL proceed, void* userdata);
void wear_inventory_category_on_avatar_loop(LLWearable* wearable, void*);
void wear_inventory_category_on_avatar_step3(LLWearableHoldingPattern* holder, BOOL append);
void remove_inventory_category_from_avatar(LLInventoryCategory* category);
void remove_inventory_category_from_avatar_step2( BOOL proceed, void* userdata);
bool move_task_inventory_callback(const LLSD& notification, const LLSD& response, LLMoveInv*);
bool confirm_replace_attachment_rez(const LLSD& notification, const LLSD& response);

std::string ICON_NAME[ICON_NAME_COUNT] =
{
	"inv_item_texture.tga",
	"inv_item_sound.tga",
	"inv_item_callingcard_online.tga",
	"inv_item_callingcard_offline.tga",
	"inv_item_landmark.tga",
	"inv_item_landmark_visited.tga",
	"inv_item_script.tga",
	"inv_item_clothing.tga",
	"inv_item_object.tga",
	"inv_item_object_multi.tga",
	"inv_item_notecard.tga",
	"inv_item_skin.tga",
	"inv_item_snapshot.tga",

	"inv_item_shape.tga",
	"inv_item_skin.tga",
	"inv_item_hair.tga",
	"inv_item_eyes.tga",
	"inv_item_shirt.tga",
	"inv_item_pants.tga",
	"inv_item_shoes.tga",
	"inv_item_socks.tga",
	"inv_item_jacket.tga",
	"inv_item_gloves.tga",
	"inv_item_undershirt.tga",
	"inv_item_underpants.tga",
	"inv_item_skirt.tga",

	"inv_item_animation.tga",
	"inv_item_gesture.tga",
};

struct LLWearInfo
{
	LLUUID	mCategoryID;
	BOOL	mAppend;
};

BOOL gAddToOutfit = FALSE;

// +=================================================+
// |        LLInvFVBridge                            |
// +=================================================+

const std::string& LLInvFVBridge::getName() const
{
	LLInventoryObject* obj = getInventoryObject();
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

// Folders don't have creation dates.
time_t LLInvFVBridge::getCreationDate() const
{
	return 0;
}

// Can be destoryed (or moved to trash)
BOOL LLInvFVBridge::isItemRemovable()
{
	LLInventoryModel* model = mInventoryPanel->getModel();
	if(!model) return FALSE;
	if(model->isObjectDescendentOf(mUUID, gAgent.getInventoryRootID()))
	{
		return TRUE;
	}
	return FALSE;
}

// Can be moved to another folder
BOOL LLInvFVBridge::isItemMovable()
{
	return TRUE;
}

// *TODO: make sure this does the right thing
void LLInvFVBridge::showProperties()
{
	LLShowProps::showProperties(mUUID);
}

void LLInvFVBridge::removeBatch(LLDynamicArray<LLFolderViewEventListener*>& batch)
{
	// Deactivate gestures when moving them into Trash
	LLInvFVBridge* bridge;
	LLInventoryModel* model = mInventoryPanel->getModel();
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
				gGestureManager.deactivateGesture(item->getUUID());
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
					gGestureManager.deactivateGesture(descendent_items[j]->getUUID());
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
	LLInventoryModel* model = mInventoryPanel->getModel();
	if(!model) return;
	LLMessageSystem* msg = gMessageSystem;
	LLUUID trash_id = model->findCategoryUUIDForType(LLAssetType::AT_TRASH);
	LLViewerInventoryItem* item = NULL;
	LLViewerInventoryCategory* cat = NULL;
	std::vector<LLUUID> move_ids;
	LLInventoryModel::update_map_t update;
	bool start_new_message = true;
	S32 count = batch.count();
	S32 i;
	for(i = 0; i < count; ++i)
	{
		bridge = (LLInvFVBridge*)(batch.get(i));
		if(!bridge || !bridge->isItemRemovable()) continue;
		item = (LLViewerInventoryItem*)model->getItem(bridge->getUUID());
		if(item)
		{
			if(item->getParentUUID() == trash_id) continue;
			move_ids.push_back(item->getUUID());
			LLPreview::hide(item->getUUID());
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
		cat = (LLViewerInventoryCategory*)model->getCategory(bridge->getUUID());
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
	std::vector<LLUUID>::iterator it = move_ids.begin();
	std::vector<LLUUID>::iterator end = move_ids.end();
	for(; it != end; ++it)
	{
		gInventory.moveObject((*it), trash_id);
	}

	// notify inventory observers.
	model->notifyObservers();
}

BOOL LLInvFVBridge::isClipboardPasteable() const
{
	LLInventoryModel* model = mInventoryPanel->getModel();
	if(!model) return FALSE;
	BOOL is_agent_inventory = model->isObjectDescendentOf(mUUID, gAgent.getInventoryRootID());

	if(LLInventoryClipboard::instance().hasContents() && is_agent_inventory)
	{
		return TRUE;
	}
	return FALSE;
}

void hideContextEntries(LLMenuGL& menu, 
									   const std::vector<std::string> &entries_to_show,
									   const std::vector<std::string> &disabled_entries)
{
	const LLView::child_list_t *list = menu.getChildList();

	LLView::child_list_t::const_iterator itor;
	for (itor = list->begin(); itor != list->end(); ++itor)
	{
		std::string name = (*itor)->getName();

		// descend into split menus:
		LLMenuItemBranchGL* branchp = dynamic_cast<LLMenuItemBranchGL*>(*itor);
		if ((name == "More") && branchp)
		{
			hideContextEntries(*branchp->getBranch(), entries_to_show, disabled_entries);
		}
		
		
		bool found = false;
		std::vector<std::string>::const_iterator itor2;
		for (itor2 = entries_to_show.begin(); itor2 != entries_to_show.end(); ++itor2)
		{
			if (*itor2 == name)
			{
				found = true;
			}
		}
		if (!found)
		{
			(*itor)->setVisible(FALSE);
		}
		else
		{
			for (itor2 = disabled_entries.begin(); itor2 != disabled_entries.end(); ++itor2)
			{
				if (*itor2 == name)
				{
					(*itor)->setEnabled(FALSE);
				}
			}
		}
	}
}

// Helper for commonly-used entries
void LLInvFVBridge::getClipboardEntries(bool show_asset_id, std::vector<std::string> &items, 
		std::vector<std::string> &disabled_items, U32 flags)
{
	// *TODO: Translate	
	items.push_back(std::string("Rename"));
	if (!isItemRenameable() || (flags & FIRST_SELECTED_ITEM) == 0)
	{
		disabled_items.push_back(std::string("Rename"));
	}

	if (show_asset_id)
	{
		items.push_back(std::string("Copy Asset UUID"));
		if ( (! ( isItemPermissive() || gAgent.isGodlike() ) ) 
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

	items.push_back(std::string("Paste"));
	if (!isClipboardPasteable() || (flags & FIRST_SELECTED_ITEM) == 0)
	{
		disabled_items.push_back(std::string("Paste"));
	}

	items.push_back(std::string("Paste Separator"));

	items.push_back(std::string("Delete"));
	if (!isItemRemovable())
	{
		disabled_items.push_back(std::string("Delete"));
	}
}

void LLInvFVBridge::buildContextMenu(LLMenuGL& menu, U32 flags)
{
	// *TODO: Translate
	lldebugs << "LLInvFVBridge::buildContextMenu()" << llendl;
	std::vector<std::string> items;
	std::vector<std::string> disabled_items;
	if(isInTrash())
	{
		items.push_back(std::string("Purge Item"));
		if (!isItemRemovable())
		{
			disabled_items.push_back(std::string("Purge Item"));
		}
		items.push_back(std::string("Restore Item"));
	}
	else
	{
		items.push_back(std::string("Open"));
		items.push_back(std::string("Properties"));

		getClipboardEntries(true, items, disabled_items, flags);
	}
	hideContextEntries(menu, items, disabled_items);
}

// *TODO: remove this
BOOL LLInvFVBridge::startDrag(EDragAndDropType* type, LLUUID* id) const
{
	BOOL rv = FALSE;

	const LLInventoryObject* obj = getInventoryObject();

	if(obj)
	{
		*type = LLAssetType::lookupDragAndDropType(obj->getType());
		if(*type == DAD_NONE)
		{
			return FALSE;
		}
		
		*id = obj->getUUID();
		//object_ids.put(obj->getUUID());

		if (*type == DAD_CATEGORY)
		{
			gInventory.startBackgroundFetch(obj->getUUID());
		}

		rv = TRUE;
	}

	return rv;
}

LLInventoryObject* LLInvFVBridge::getInventoryObject() const
{
	LLInventoryObject* obj = NULL;
	LLInventoryModel* model = mInventoryPanel->getModel();
	if(model)
	{
		obj = (LLInventoryObject*)model->getObject(mUUID);
	}
	return obj;
}

BOOL LLInvFVBridge::isInTrash() const
{
	LLInventoryModel* model = mInventoryPanel->getModel();
	if(!model) return FALSE;
	LLUUID trash_id = model->findCategoryUUIDForType(LLAssetType::AT_TRASH);
	return model->isObjectDescendentOf(mUUID, trash_id);
}

BOOL LLInvFVBridge::isAgentInventory() const
{
	LLInventoryModel* model = mInventoryPanel->getModel();
	if(!model) return FALSE;
	if(gAgent.getInventoryRootID() == mUUID) return TRUE;
	return model->isObjectDescendentOf(mUUID, gAgent.getInventoryRootID());
}

BOOL LLInvFVBridge::isItemPermissive() const
{
	return FALSE;
}

// static
void LLInvFVBridge::changeItemParent(LLInventoryModel* model,
									 LLViewerInventoryItem* item,
									 const LLUUID& new_parent,
									 BOOL restamp)
{
	if(item->getParentUUID() != new_parent)
	{
		LLInventoryModel::update_list_t update;
		LLInventoryModel::LLCategoryUpdate old_folder(item->getParentUUID(),-1);
		update.push_back(old_folder);
		LLInventoryModel::LLCategoryUpdate new_folder(new_parent, 1);
		update.push_back(new_folder);
		gInventory.accountForUpdate(update);

		LLPointer<LLViewerInventoryItem> new_item = new LLViewerInventoryItem(item);
		new_item->setParent(new_parent);
		new_item->updateParentOnServer(restamp);
		model->updateItem(new_item);
		model->notifyObservers();
	}
}

// static
void LLInvFVBridge::changeCategoryParent(LLInventoryModel* model,
										 LLViewerInventoryCategory* cat,
										 const LLUUID& new_parent,
										 BOOL restamp)
{
	if(cat->getParentUUID() != new_parent)
	{
		LLInventoryModel::update_list_t update;
		LLInventoryModel::LLCategoryUpdate old_folder(cat->getParentUUID(), -1);
		update.push_back(old_folder);
		LLInventoryModel::LLCategoryUpdate new_folder(new_parent, 1);
		update.push_back(new_folder);
		gInventory.accountForUpdate(update);

		LLPointer<LLViewerInventoryCategory> new_cat = new LLViewerInventoryCategory(cat);
		new_cat->setParent(new_parent);
		new_cat->updateParentOnServer(restamp);
		model->updateCategory(new_cat);
		model->notifyObservers();
	}
}


const char* safe_inv_type_lookup(LLInventoryType::EType inv_type)
{
	const char* rv = LLInventoryType::lookup(inv_type);
	if(!rv)
	{
		const char* INVALID_TYPE = "<invalid>";
		rv = INVALID_TYPE;
	}
	return rv;
}

LLInvFVBridge* LLInvFVBridge::createBridge(LLAssetType::EType asset_type,
					   LLInventoryType::EType inv_type,
					   LLInventoryPanel* inventory,
					   const LLUUID& uuid,
					   U32 flags)
{
	LLInvFVBridge* new_listener = NULL;
	switch(asset_type)
	{
	case LLAssetType::AT_TEXTURE:
		if(!(inv_type == LLInventoryType::IT_TEXTURE || inv_type == LLInventoryType::IT_SNAPSHOT))
		{
			llwarns << LLAssetType::lookup(asset_type) << " asset has inventory type " << safe_inv_type_lookup(inv_type) << " on uuid " << uuid << llendl;
		}
		new_listener = new LLTextureBridge(inventory, uuid, inv_type);
		break;

	case LLAssetType::AT_SOUND:
		if(!(inv_type == LLInventoryType::IT_SOUND))
		{
			llwarns << LLAssetType::lookup(asset_type) << " asset has inventory type " << safe_inv_type_lookup(inv_type) << " on uuid " << uuid << llendl;
		}
		new_listener = new LLSoundBridge(inventory, uuid);
		break;

	case LLAssetType::AT_LANDMARK:
		if(!(inv_type == LLInventoryType::IT_LANDMARK))
		{
			llwarns << LLAssetType::lookup(asset_type) << " asset has inventory type " << safe_inv_type_lookup(inv_type) << " on uuid " << uuid << llendl;
		}
		new_listener = new LLLandmarkBridge(inventory, uuid, flags);
		break;
		
	case LLAssetType::AT_CALLINGCARD:
		if(!(inv_type == LLInventoryType::IT_CALLINGCARD))
		{
			llwarns << LLAssetType::lookup(asset_type) << " asset has inventory type " << safe_inv_type_lookup(inv_type) << " on uuid " << uuid << llendl;
		}
		new_listener = new LLCallingCardBridge(inventory, uuid);
		break;

	case LLAssetType::AT_SCRIPT:
		if(!(inv_type == LLInventoryType::IT_LSL))
		{
			llwarns << LLAssetType::lookup(asset_type) << " asset has inventory type " << safe_inv_type_lookup(inv_type) << " on uuid " << uuid << llendl;
		}
		new_listener = new LLScriptBridge(inventory, uuid);
		break;

	case LLAssetType::AT_OBJECT:
		if(!(inv_type == LLInventoryType::IT_OBJECT || inv_type == LLInventoryType::IT_ATTACHMENT))
		{
			llwarns << LLAssetType::lookup(asset_type) << " asset has inventory type " << safe_inv_type_lookup(inv_type) << " on uuid " << uuid << llendl;
		}
		new_listener = new LLObjectBridge(inventory, uuid, inv_type, flags);
		break;

	case LLAssetType::AT_NOTECARD:
		if(!(inv_type == LLInventoryType::IT_NOTECARD))
		{
			llwarns << LLAssetType::lookup(asset_type) << " asset has inventory type " << safe_inv_type_lookup(inv_type) << " on uuid " << uuid << llendl;
		}
		new_listener = new LLNotecardBridge(inventory, uuid);
		break;

	case LLAssetType::AT_ANIMATION:
		if(!(inv_type == LLInventoryType::IT_ANIMATION))
		{
			llwarns << LLAssetType::lookup(asset_type) << " asset has inventory type " << safe_inv_type_lookup(inv_type) << " on uuid " << uuid << llendl;
		}
		new_listener = new LLAnimationBridge(inventory, uuid);
		break;

	case LLAssetType::AT_GESTURE:
		if(!(inv_type == LLInventoryType::IT_GESTURE))
		{
			llwarns << LLAssetType::lookup(asset_type) << " asset has inventory type " << safe_inv_type_lookup(inv_type) << " on uuid " << uuid << llendl;
		}
		new_listener = new LLGestureBridge(inventory, uuid);
		break;

	case LLAssetType::AT_LSL_TEXT:
		if(!(inv_type == LLInventoryType::IT_LSL))
		{
			llwarns << LLAssetType::lookup(asset_type) << " asset has inventory type " << safe_inv_type_lookup(inv_type) << " on uuid " << uuid << llendl;
		}
		new_listener = new LLLSLTextBridge(inventory, uuid);
		break;

	case LLAssetType::AT_CLOTHING:
	case LLAssetType::AT_BODYPART:
		if(!(inv_type == LLInventoryType::IT_WEARABLE))
		{
			llwarns << LLAssetType::lookup(asset_type) << " asset has inventory type " << safe_inv_type_lookup(inv_type) << " on uuid " << uuid << llendl;
		}
		new_listener = new LLWearableBridge(inventory, uuid, asset_type, inv_type, (EWearableType)flags);
		break;

	case LLAssetType::AT_CATEGORY:
	case LLAssetType::AT_ROOT_CATEGORY:
		new_listener = new LLFolderBridge(inventory, uuid);
		break;
		
	default:
		llinfos << "Unhandled asset type (llassetstorage.h): "
				<< (S32)asset_type << llendl;
		break;
	}

	if (new_listener)
	{
		new_listener->mInvType = inv_type;
	}

	return new_listener;
}

// +=================================================+
// |        LLItemBridge                             |
// +=================================================+

void LLItemBridge::performAction(LLFolderView* folder, LLInventoryModel* model, std::string action)
{
	if ("open" == action)
	{
		openItem();
	}
	else if ("properties" == action)
	{
		showProperties();
	}
	else if ("purge" == action)
	{
		LLInventoryCategory* cat = model->getCategory(mUUID);
		if(cat)
		{
			model->purgeDescendentsOf(mUUID);
		}
		LLInventoryObject* obj = model->getObject(mUUID);
		if(!obj) return;
		obj->removeFromServer();
		LLPreview::hide(mUUID);
		model->deleteObject(mUUID);
		model->notifyObservers();
	}
	else if ("restoreToWorld" == action)
	{
		restoreToWorld();
	}
	else if ("restore" == action)
	{
		restoreItem();
	}
	else if ("copy_uuid" == action)
	{
		// Single item only
		LLInventoryItem* item = model->getItem(mUUID);
		if(!item) return;
		LLUUID asset_id = item->getAssetUUID();
		std::string buffer;
		asset_id.toString(buffer);

		gViewerWindow->mWindow->copyTextToClipboard(utf8str_to_wstring(buffer));
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

		LLFolderViewItem* folder_view_itemp = folder->getItemByID(itemp->getParentUUID());
		if (!folder_view_itemp) return;

		folder_view_itemp->getListener()->pasteFromClipboard();
		return;
	}
}

void LLItemBridge::selectItem()
{
	LLViewerInventoryItem* item = (LLViewerInventoryItem*)getItem();
	if(item && !item->isComplete())
	{
		item->fetchFromServer();
	}
}

void LLItemBridge::restoreItem()
{
	LLViewerInventoryItem* item = (LLViewerInventoryItem*)getItem();
	if(item)
	{
		LLInventoryModel* model = mInventoryPanel->getModel();
		LLUUID new_parent = model->findCategoryUUIDForType(item->getType());
		// do not restamp on restore.
		LLInvFVBridge::changeItemParent(model, item, new_parent, FALSE);
	}
}

void LLItemBridge::restoreToWorld()
{
	LLViewerInventoryItem* itemp = (LLViewerInventoryItem*)getItem();
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
	}

	//Similar functionality to the drag and drop rez logic
	BOOL remove_from_inventory = FALSE;

	//remove local inventory copy, sim will deal with permissions and removing the item
	//from the actual inventory if its a no-copy etc
	if(!itemp->getPermissions().allowCopyBy(gAgent.getID()))
	{
		remove_from_inventory = TRUE;
	}

	// Check if it's in the trash. (again similar to the normal rez logic)
	LLUUID trash_id;
	trash_id = gInventory.findCategoryUUIDForType(LLAssetType::AT_TRASH);
	if(gInventory.isObjectDescendentOf(itemp->getUUID(), trash_id))
	{
		remove_from_inventory = TRUE;
	}

	if(remove_from_inventory)
	{
		gInventory.deleteObject(itemp->getUUID());
		gInventory.notifyObservers();
	}
}

LLUIImagePtr LLItemBridge::getIcon() const
{
	return LLUI::getUIImage(ICON_NAME[OBJECT_ICON_NAME]);
}

PermissionMask LLItemBridge::getPermissionMask() const
{
	LLViewerInventoryItem* item = getItem();
	PermissionMask perm_mask = 0;
	if(item) 
	{
		BOOL copy = item->getPermissions().allowCopyBy(gAgent.getID());
		BOOL mod = item->getPermissions().allowModifyBy(gAgent.getID());
		BOOL xfer = item->getPermissions().allowOperationBy(PERM_TRANSFER,
															gAgent.getID());

		if (copy) perm_mask |= PERM_COPY;
		if (mod)  perm_mask |= PERM_MODIFY;
		if (xfer) perm_mask |= PERM_TRANSFER;

	}
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

std::string LLItemBridge::getLabelSuffix() const
{
	std::string suffix;
	LLInventoryItem* item = getItem();
	if(item) 
	{
		// it's a bit confusing to put nocopy/nomod/etc on calling cards.
		if(LLAssetType::AT_CALLINGCARD != item->getType()
		   && item->getPermissions().getOwner() == gAgent.getID())
		{
			BOOL copy = item->getPermissions().allowCopyBy(gAgent.getID());
			BOOL mod = item->getPermissions().allowModifyBy(gAgent.getID());
			BOOL xfer = item->getPermissions().allowOperationBy(PERM_TRANSFER,
																gAgent.getID());
			// *TODO: Translate
			const char* EMPTY = "";
			const char* NO_COPY = " (no copy)";
			const char* NO_MOD = " (no modify)";
			const char* NO_XFER = " (no transfer)";
			const char* scopy;
			if(copy) scopy = EMPTY;
			else scopy = NO_COPY;
			const char* smod;
			if(mod) smod = EMPTY;
			else smod = NO_MOD;
			const char* sxfer;
			if(xfer) sxfer = EMPTY;
			else sxfer = NO_XFER;
			suffix = llformat("%s%s%s",scopy,smod,sxfer);
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
		return (item->getPermissions().allowModifyBy(gAgent.getID()));
	}
	return FALSE;
}

BOOL LLItemBridge::renameItem(const std::string& new_name)
{
	if(!isItemRenameable()) return FALSE;
	LLPreview::rename(mUUID, getPrefix() + new_name);
	LLInventoryModel* model = mInventoryPanel->getModel();
	if(!model) return FALSE;
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
	LLInventoryModel* model = mInventoryPanel->getModel();
	if(!model) return FALSE;
	LLUUID trash_id = model->findCategoryUUIDForType(LLAssetType::AT_TRASH);
	LLViewerInventoryItem* item = getItem();

	// if item is not already in trash
	if(item && !model->isObjectDescendentOf(mUUID, trash_id))
	{
		// move to trash, and restamp
		LLInvFVBridge::changeItemParent(model, item, trash_id, TRUE);
		// delete was successful
		return TRUE;
	}
	else
	{
		// tried to delete already item in trash (should purge?)
		return FALSE;
	}
}

BOOL LLItemBridge::isItemCopyable() const
{
	LLViewerInventoryItem* item = getItem();
	if (item)
	{
		// can't copy worn objects. DEV-15183
		LLVOAvatar *avatarp = gAgent.getAvatarObject();
		if( !avatarp )
		{
			return FALSE;
		}

		if( avatarp->isWearingAttachment( mUUID ) )
		{
			return FALSE;
		}
			

		return (item->getPermissions().allowCopyBy(gAgent.getID()));
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
	LLInventoryModel* model = mInventoryPanel->getModel();
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
		U32 mask = item->getPermissions().getMaskBase();
		if((mask & PERM_ITEM_UNRESTRICTED) == PERM_ITEM_UNRESTRICTED)
		{
			return TRUE;
		}
	}
	return FALSE;
}

// +=================================================+
// |        LLFolderBridge                           |
// +=================================================+

LLFolderBridge* LLFolderBridge::sSelf=NULL;

// Can be moved to another folder
BOOL LLFolderBridge::isItemMovable()
{
	LLInventoryObject* obj = getInventoryObject();
	if(obj)
	{
		return (LLAssetType::AT_NONE == ((LLInventoryCategory*)obj)->getPreferredType());
	}
	return FALSE;
}

void LLFolderBridge::selectItem()
{
}


// Can be destroyed (or moved to trash)
BOOL LLFolderBridge::isItemRemovable()
{
	LLInventoryModel* model = mInventoryPanel->getModel();
	if(!model) 
	{
		return FALSE;
	}

	if(!model->isObjectDescendentOf(mUUID, gAgent.getInventoryRootID()))
	{
		return FALSE;
	}

	LLVOAvatar* avatar = gAgent.getAvatarObject();
	if( !avatar )
	{
		return FALSE;
	}

	LLInventoryCategory* category = model->getCategory(mUUID);
	if( !category )
	{
		return FALSE;
	}

	if( LLAssetType::AT_NONE != category->getPreferredType() )
	{
		return FALSE;
	}

	LLInventoryModel::cat_array_t	descendent_categories;
	LLInventoryModel::item_array_t	descendent_items;
	gInventory.collectDescendents( mUUID, descendent_categories, descendent_items, FALSE );

	S32 i;
	for( i = 0; i < descendent_categories.count(); i++ )
	{
		LLInventoryCategory* category = descendent_categories[i];
		if( LLAssetType::AT_NONE != category->getPreferredType() )
		{
			return FALSE;
		}
	}

	for( i = 0; i < descendent_items.count(); i++ )
	{
		LLInventoryItem* item = descendent_items[i];
		if( (item->getType() == LLAssetType::AT_CLOTHING) ||
			(item->getType() == LLAssetType::AT_BODYPART) )
		{
			if( gAgent.isWearingItem( item->getUUID() ) )
			{
				return FALSE;
			}
		}
		else
		if( item->getType() == LLAssetType::AT_OBJECT )
		{
			if( avatar->isWearingAttachment( item->getUUID() ) )
			{
				return FALSE;
			}
		}
	}

	return TRUE;
}

BOOL LLFolderBridge::isUpToDate() const
{
	LLInventoryModel* model = mInventoryPanel->getModel();
	if(!model) return FALSE;
	LLViewerInventoryCategory* category = (LLViewerInventoryCategory*)model->getCategory(mUUID);
	if( !category )
	{
		return FALSE;
	}

	return category->getVersion() != LLViewerInventoryCategory::VERSION_UNKNOWN;
}

BOOL LLFolderBridge::dragCategoryIntoFolder(LLInventoryCategory* inv_cat,
											BOOL drop)
{
	// This should never happen, but if an inventory item is incorrectly parented, 
	// the UI will get confused and pass in a NULL.
	if(!inv_cat) return FALSE;

	LLInventoryModel* model = mInventoryPanel->getModel();
	if(!model) return FALSE;

	LLVOAvatar* avatar = gAgent.getAvatarObject();
	if(!avatar) return FALSE;

	// cannot drag into library
	if(!isAgentInventory())
	{
		return FALSE;
	}

	// check to make sure source is agent inventory, and is represented there.
	LLToolDragAndDrop::ESource source = LLToolDragAndDrop::getInstance()->getSource();
	BOOL is_agent_inventory = (model->getCategory(inv_cat->getUUID()) != NULL)
		&& (LLToolDragAndDrop::SOURCE_AGENT == source);

	BOOL accept = FALSE;
	S32 i;
	LLInventoryModel::cat_array_t	descendent_categories;
	LLInventoryModel::item_array_t	descendent_items;
	if(is_agent_inventory)
	{
		const LLUUID& cat_id = inv_cat->getUUID();

		// Is the destination the trash?
		LLUUID trash_id;
		trash_id = model->findCategoryUUIDForType(LLAssetType::AT_TRASH);
		BOOL move_is_into_trash = (mUUID == trash_id)
				|| model->isObjectDescendentOf(mUUID, trash_id);
		BOOL is_movable = (LLAssetType::AT_NONE == inv_cat->getPreferredType());
		if( is_movable )
		{
			gInventory.collectDescendents( cat_id, descendent_categories, descendent_items, FALSE );

			for( i = 0; i < descendent_categories.count(); i++ )
			{
				LLInventoryCategory* category = descendent_categories[i];
				if( LLAssetType::AT_NONE != category->getPreferredType() )
				{
					// ...can't move "special folders" like Textures
					is_movable = FALSE;
					break;
				}
			}
			
			if( is_movable )
			{
				if( move_is_into_trash )
				{
					for( i = 0; i < descendent_items.count(); i++ )
					{
						LLInventoryItem* item = descendent_items[i];
						if( (item->getType() == LLAssetType::AT_CLOTHING) ||
							(item->getType() == LLAssetType::AT_BODYPART) )
						{
							if( gAgent.isWearingItem( item->getUUID() ) )
							{
								is_movable = FALSE;  // It's generally movable, but not into the trash!
								break;
							}
						}
						else
						if( item->getType() == LLAssetType::AT_OBJECT )
						{
							if( avatar->isWearingAttachment( item->getUUID() ) )
							{
								is_movable = FALSE;  // It's generally movable, but not into the trash!
								break;
							}
						}
					}
				}
			}
		}

		
		accept =	is_movable
					&& (mUUID != cat_id)								// Can't move a folder into itself
					&& (mUUID != inv_cat->getParentUUID())				// Avoid moves that would change nothing
					&& !(model->isObjectDescendentOf(mUUID, cat_id));	// Avoid circularity
		if(accept && drop)
		{
			// Look for any gestures and deactivate them
			if (move_is_into_trash)
			{
				for (i = 0; i < descendent_items.count(); i++)
				{
					LLInventoryItem* item = descendent_items[i];
					if (item->getType() == LLAssetType::AT_GESTURE
						&& gGestureManager.isGestureActive(item->getUUID()))
					{
						gGestureManager.deactivateGesture(item->getUUID());
					}
				}
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
	else if(LLToolDragAndDrop::SOURCE_WORLD == source)
	{
		// content category has same ID as object itself
		LLUUID object_id = inv_cat->getUUID();
		LLUUID category_id = mUUID;
		accept = move_inv_category_world_to_agent(object_id, category_id, drop);
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
	LLNotifications::instance().add(dialog, LLSD(), LLSD(), boost::bind(move_task_inventory_callback, _1, _2, move_inv));
}

// Move/copy all inventory items from the Contents folder of an in-world
// object to the agent's inventory, inside a given category.
BOOL move_inv_category_world_to_agent(const LLUUID& object_id, 
									  const LLUUID& category_id,
									  BOOL drop,
									  void (*callback)(S32, void*),
									  void* user_data)
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
	InventoryObjectList inventory_objects;
	object->getInventoryContents(inventory_objects);

	if (inventory_objects.empty())
	{
		llinfos << "Object contents not found for drop." << llendl;
		return FALSE;
	}
	
	BOOL accept = TRUE;
	BOOL is_move = FALSE;

	// coming from a task. Need to figure out if the person can
	// move/copy this item.
	InventoryObjectList::iterator it = inventory_objects.begin();
	InventoryObjectList::iterator end = inventory_objects.end();
	for ( ; it != end; ++it)
	{
		// coming from a task. Need to figure out if the person can
		// move/copy this item.
		LLPermissions perm(((LLInventoryItem*)((LLInventoryObject*)(*it)))->getPermissions());
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
		else
		{
			accept = FALSE;
			break;
		}
	}

	if(drop && accept)
	{
		it = inventory_objects.begin();
		InventoryObjectList::iterator first_it = inventory_objects.begin();
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
			params.functor(boost::bind(move_task_inventory_callback, _1, _2, move_inv));
			LLNotifications::instance().forceResponse(params, 0);
		}
	}
	return accept;
}

class LLFindWearables : public LLInventoryCollectFunctor
{
public:
	LLFindWearables() {}
	virtual ~LLFindWearables() {}
	virtual bool operator()(LLInventoryCategory* cat,
							LLInventoryItem* item);
};

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

//Used by LLFolderBridge as callback for directory recursion.
class LLRightClickInventoryFetchObserver : public LLInventoryFetchObserver
{
public:
	LLRightClickInventoryFetchObserver() :
		mCopyItems(false)
	{ };
	LLRightClickInventoryFetchObserver(const LLUUID& cat_id, bool copy_items) :
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
	LLRightClickInventoryFetchDescendentsObserver(bool copy_items) : mCopyItems(copy_items) {}
	~LLRightClickInventoryFetchDescendentsObserver() {}
	virtual void done();
protected:
	bool mCopyItems;
};

void LLRightClickInventoryFetchDescendentsObserver::done()
{
	// Avoid passing a NULL-ref as mCompleteFolders.front() down to
	// gInventory.collectDescendents()
	if( mCompleteFolders.empty() )
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
	gInventory.collectDescendents(mCompleteFolders.front(),
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

	LLRightClickInventoryFetchObserver* outfit;
	outfit = new LLRightClickInventoryFetchObserver(mCompleteFolders.front(), mCopyItems);
	LLInventoryFetchObserver::item_ref_t ids;
	for(S32 i = 0; i < count; ++i)
	{
		ids.push_back(item_array.get(i)->getUUID());
	}

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
	outfit->fetchItems(ids);
	outfit->done();				//Not interested in waiting and this will be right 99% of the time.
//Uncomment the following code for laggy Inventory UI.
/*	if(outfit->isEverythingComplete())
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
	LLInventoryCopyAndWearObserver(const LLUUID& cat_id, int count) :mCatID(cat_id), mContentsCount(count), mFolderAdded(FALSE) {}
	virtual ~LLInventoryCopyAndWearObserver() {}
	virtual void changed(U32 mask);

protected:
	LLUUID mCatID;
	int    mContentsCount;
	BOOL   mFolderAdded;
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
					wear_inventory_category(category, FALSE, TRUE);
					delete this;
				}
			}		
		}

	}
}



void LLFolderBridge::performAction(LLFolderView* folder, LLInventoryModel* model, std::string action)
{
	if ("open" == action)
	{
		openItem();
	}
	else if ("paste" == action)
	{
		pasteFromClipboard();
	}
	else if ("properties" == action)
	{
		showProperties();
	}
	else if ("replaceoutfit" == action)
	{
		modifyOutfit(FALSE);
	}
	else if ("addtooutfit" == action)
	{
		modifyOutfit(TRUE);
	}
	else if ("removefromoutfit" == action)
	{
		LLInventoryModel* model = mInventoryPanel->getModel();
		if(!model) return;
		LLViewerInventoryCategory* cat = getCategory();
		if(!cat) return;
		
		remove_inventory_category_from_avatar ( cat );
	}	
	else if ("purge" == action)
	{		
		LLViewerInventoryCategory* cat;
		cat = (LLViewerInventoryCategory*)getCategory();

		if(cat)
		{
			model->purgeDescendentsOf(mUUID);
		}
		LLInventoryObject* obj = model->getObject(mUUID);
		if(!obj) return;
		obj->removeFromServer();
		model->deleteObject(mUUID);
		model->notifyObservers();
	}
	else if ("restore" == action)
	{
		restoreItem();
	}
}

void LLFolderBridge::openItem()
{
	lldebugs << "LLFolderBridge::openItem()" << llendl;
	LLInventoryModel* model = mInventoryPanel->getModel();
	if(!model) return;
	model->fetchDescendentsOf(mUUID);
}

BOOL LLFolderBridge::isItemRenameable() const
{
	LLViewerInventoryCategory* cat = (LLViewerInventoryCategory*)getCategory();
	if(cat && (cat->getPreferredType() == LLAssetType::AT_NONE)
	   && (cat->getOwnerID() == gAgent.getID()))
	{
		return TRUE;
	}
	return FALSE;
}

void LLFolderBridge::restoreItem()
{
	LLViewerInventoryCategory* cat;
	cat = (LLViewerInventoryCategory*)getCategory();
	if(cat)
	{
		LLInventoryModel* model = mInventoryPanel->getModel();
		LLUUID new_parent = model->findCategoryUUIDForType(cat->getType());
		// do not restamp children on restore
		LLInvFVBridge::changeCategoryParent(model, cat, new_parent, FALSE);
	}
}

// Icons for folders are based on the preferred type
LLUIImagePtr LLFolderBridge::getIcon() const
{
	const char* control = NULL;
	LLAssetType::EType preferred_type = LLAssetType::AT_NONE;
	LLViewerInventoryCategory* cat = getCategory();
	if(cat)
	{
		preferred_type = cat->getPreferredType();
	}
	switch(preferred_type)
	{
	case LLAssetType::AT_TEXTURE:
		control = "inv_folder_texture.tga";
		break;
	case LLAssetType::AT_SOUND:
		control = "inv_folder_sound.tga";
		break;
	case LLAssetType::AT_CALLINGCARD:
		control = "inv_folder_callingcard.tga";
		break;
	case LLAssetType::AT_LANDMARK:
		control = "inv_folder_landmark.tga";
		break;
	case LLAssetType::AT_SCRIPT:
	case LLAssetType::AT_LSL_TEXT:
		control = "inv_folder_script.tga";
		break;
	case LLAssetType::AT_OBJECT:
		control = "inv_folder_object.tga";
		break;
	case LLAssetType::AT_NOTECARD:
		control = "inv_folder_notecard.tga";
		break;
	case LLAssetType::AT_CATEGORY:
		control = "inv_folder_plain_closed.tga";
		break;
	case LLAssetType::AT_CLOTHING:
		control = "inv_folder_clothing.tga";
		break;
	case LLAssetType::AT_BODYPART:
		control = "inv_folder_bodypart.tga";
		break;
	case LLAssetType::AT_TRASH:
		control = "inv_folder_trash.tga";
		break;
	case LLAssetType::AT_SNAPSHOT_CATEGORY:
		control = "inv_folder_snapshot.tga";
		break;
	case LLAssetType::AT_LOST_AND_FOUND:
		control = "inv_folder_lostandfound.tga";
		break;
	case LLAssetType::AT_ANIMATION:
		control = "inv_folder_animation.tga";
		break;
	case LLAssetType::AT_GESTURE:
		control = "inv_folder_gesture.tga";
		break;
	default:
		control = "inv_folder_plain_closed.tga";
		break;
	}
	return LLUI::getUIImage(control);
}

BOOL LLFolderBridge::renameItem(const std::string& new_name)
{
	if(!isItemRenameable()) return FALSE;
	LLInventoryModel* model = mInventoryPanel->getModel();
	if(!model) return FALSE;
	LLViewerInventoryCategory* cat = getCategory();
	if(cat && (cat->getName() != new_name))
	{
		LLPointer<LLViewerInventoryCategory> new_cat = new LLViewerInventoryCategory(cat);
		new_cat->rename(new_name);
		new_cat->updateServer(FALSE);
		model->updateCategory(new_cat);
		model->notifyObservers();
	}
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
	// move it to the trash
	LLPreview::hide(mUUID);
	LLInventoryModel* model = mInventoryPanel->getModel();
	if(!model) return FALSE;

	LLUUID trash_id;
	trash_id = model->findCategoryUUIDForType(LLAssetType::AT_TRASH);

	// Look for any gestures and deactivate them
	LLInventoryModel::cat_array_t	descendent_categories;
	LLInventoryModel::item_array_t	descendent_items;
	gInventory.collectDescendents( mUUID, descendent_categories, descendent_items, FALSE );

	S32 i;
	for (i = 0; i < descendent_items.count(); i++)
	{
		LLInventoryItem* item = descendent_items[i];
		if (item->getType() == LLAssetType::AT_GESTURE
			&& gGestureManager.isGestureActive(item->getUUID()))
		{
			gGestureManager.deactivateGesture(item->getUUID());
		}
	}

	// go ahead and do the normal remove if no 'last calling
	// cards' are being removed.
	LLViewerInventoryCategory* cat = getCategory();
	if(cat)
	{
		LLInvFVBridge::changeCategoryParent(model, cat, trash_id, TRUE);
	}

	return TRUE;
}

BOOL LLFolderBridge::isClipboardPasteable() const
{
	if(LLInventoryClipboard::instance().hasContents() && isAgentInventory())
	{
		return TRUE;
	}
	return FALSE;
}

void LLFolderBridge::pasteFromClipboard()
{
	LLInventoryModel* model = mInventoryPanel->getModel();
	if(model && isClipboardPasteable())
	{
		LLInventoryItem* item = NULL;
		LLDynamicArray<LLUUID> objects;
		LLInventoryClipboard::instance().retrieve(objects);
		S32 count = objects.count();
		LLUUID parent_id(mUUID);
		for(S32 i = 0; i < count; i++)
		{
			item = model->getItem(objects.get(i));
			if (item)
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

void LLFolderBridge::staticFolderOptionsMenu()
{
	if (!sSelf) return;
	sSelf->folderOptionsMenu();
}

void LLFolderBridge::folderOptionsMenu()
{
	std::vector<std::string> disabled_items;

	// *TODO: Translate
	
	LLInventoryModel* model = mInventoryPanel->getModel();
	if(!model) return;

	const LLInventoryCategory* category = model->getCategory(mUUID);
	bool is_default_folder = category &&
		(LLAssetType::AT_NONE != category->getPreferredType());
	
	// calling card related functionality for folders.

	// Only enable calling-card related options for non-default folders.
	if (!is_default_folder)
	{
		LLIsType is_callingcard(LLAssetType::AT_CALLINGCARD);
		if (mCallingCards || checkFolderForContentsOfType(model, is_callingcard))
		{
			mItems.push_back(std::string("Calling Card Separator"));
			mItems.push_back(std::string("Conference Chat Folder"));
			mItems.push_back(std::string("IM All Contacts In Folder"));
		}
	}
	
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

		// Only enable add/replace outfit for non-default folders.
		if (!is_default_folder)
		{
			mItems.push_back(std::string("Add To Outfit"));
			mItems.push_back(std::string("Replace Outfit"));
		}
		mItems.push_back(std::string("Take Off Items"));
	}
	hideContextEntries(*mMenu, mItems, disabled_items);
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

// Flags unused
void LLFolderBridge::buildContextMenu(LLMenuGL& menu, U32 flags)
{
	// *TODO: Translate
	lldebugs << "LLFolderBridge::buildContextMenu()" << llendl;
//	std::vector<std::string> disabled_items;
	LLInventoryModel* model = mInventoryPanel->getModel();
	if(!model) return;
	LLUUID trash_id = model->findCategoryUUIDForType(LLAssetType::AT_TRASH);
	LLUUID lost_and_found_id = model->findCategoryUUIDForType(LLAssetType::AT_LOST_AND_FOUND);

	if (lost_and_found_id == mUUID)
	  {
		// This is the lost+found folder.
		  mItems.push_back(std::string("Empty Lost And Found"));
	  }

	if(trash_id == mUUID)
	{
		// This is the trash.
		mItems.push_back(std::string("Empty Trash"));
	}
	else if(model->isObjectDescendentOf(mUUID, trash_id))
	{
		// This is a folder in the trash.
		mItems.clear(); // clear any items that used to exist
		mItems.push_back(std::string("Purge Item"));
		if (!isItemRemovable())
		{
			mDisabledItems.push_back(std::string("Purge Item"));
		}

		mItems.push_back(std::string("Restore Item"));
	}
	else if(isAgentInventory()) // do not allow creating in library
	{
			// only mature accounts can create undershirts/underwear
			/*if (!gAgent.isTeen())
			{
				sub_menu->append(new LLMenuItemCallGL("New Undershirt",
													&createNewUndershirt,
													NULL,
													(void*)this));
				sub_menu->append(new LLMenuItemCallGL("New Underpants",
													&createNewUnderpants,
													NULL,
													(void*)this));
			}*/

/*		BOOL contains_calling_cards = FALSE;
		LLInventoryModel::cat_array_t cat_array;
		LLInventoryModel::item_array_t item_array;

		LLIsType is_callingcard(LLAssetType::AT_CALLINGCARD);
		model->collectDescendentsIf(mUUID,
									cat_array,
									item_array,
									LLInventoryModel::EXCLUDE_TRASH,
									is_callingcard);
		if(item_array.count() > 0) contains_calling_cards = TRUE;
*/
		mItems.push_back(std::string("New Folder"));
		mItems.push_back(std::string("New Script"));
		mItems.push_back(std::string("New Note"));
		mItems.push_back(std::string("New Gesture"));
		mItems.push_back(std::string("New Clothes"));
		mItems.push_back(std::string("New Body Parts"));

		getClipboardEntries(false, mItems, mDisabledItems, flags);

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
		
		mMenu = &menu;
		sSelf = this;
		LLRightClickInventoryFetchDescendentsObserver* fetch = new LLRightClickInventoryFetchDescendentsObserver(FALSE);

		LLInventoryFetchDescendentsObserver::folder_ref_t folders;
		LLViewerInventoryCategory* category = (LLViewerInventoryCategory*)model->getCategory(mUUID);
		folders.push_back(category->getUUID());
		fetch->fetchDescendents(folders);
		inc_busy_count();
		if(fetch->isEverythingComplete())
		{
			// everything is already here - call done.
			fetch->done();
		}
		else
		{
			// it's all on it's way - add an observer, and the inventory
			// will call done for us when everything is here.
			gInventory.addObserver(fetch);
		}
	}
	else
	{
		mItems.push_back(std::string("--no options--"));
		mDisabledItems.push_back(std::string("--no options--"));
	}
	hideContextEntries(menu, mItems, mDisabledItems);
}

BOOL LLFolderBridge::hasChildren() const
{
	LLInventoryModel* model = mInventoryPanel->getModel();
	if(!model) return FALSE;
	LLInventoryModel::EHasChildren has_children;
	has_children = gInventory.categoryHasChildren(mUUID);
	return has_children != LLInventoryModel::CHILDREN_NO;
}

BOOL LLFolderBridge::dragOrDrop(MASK mask, BOOL drop,
								EDragAndDropType cargo_type,
								void* cargo_data)
{
	//llinfos << "LLFolderBridge::dragOrDrop()" << llendl;
	BOOL accept = FALSE;
	switch(cargo_type)
	{
	case DAD_TEXTURE:
	case DAD_SOUND:
	case DAD_CALLINGCARD:
	case DAD_LANDMARK:
	case DAD_SCRIPT:
	case DAD_OBJECT:
	case DAD_NOTECARD:
	case DAD_CLOTHING:
	case DAD_BODYPART:
	case DAD_ANIMATION:
	case DAD_GESTURE:
		accept = dragItemIntoFolder((LLInventoryItem*)cargo_data,
									drop);
		break;
	case DAD_CATEGORY:
		accept = dragCategoryIntoFolder((LLInventoryCategory*)cargo_data,
										drop);
		break;
	default:
		break;
	}
	return accept;
}

LLViewerInventoryCategory* LLFolderBridge::getCategory() const
{
	LLViewerInventoryCategory* cat = NULL;
	LLInventoryModel* model = mInventoryPanel->getModel();
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
	LLInventoryPanel* panel = bridge->mInventoryPanel;
	LLInventoryModel* model = panel->getModel();
	if(!model) return;
	LLUUID id;
	id = model->createNewCategory(bridge->getUUID(),
								  LLAssetType::AT_NONE,
								  LLStringUtil::null);
	model->notifyObservers();

	// At this point, the bridge has probably been deleted, but the
	// view is still there.
	panel->setSelection(id, TAKE_FOCUS_YES);
}

void LLFolderBridge::createNewShirt(void* user_data)
{
	LLFolderBridge::createWearable((LLFolderBridge*)user_data, WT_SHIRT);
}

void LLFolderBridge::createNewPants(void* user_data)
{
	LLFolderBridge::createWearable((LLFolderBridge*)user_data, WT_PANTS);
}

void LLFolderBridge::createNewShoes(void* user_data)
{
	LLFolderBridge::createWearable((LLFolderBridge*)user_data, WT_SHOES);
}

void LLFolderBridge::createNewSocks(void* user_data)
{
	LLFolderBridge::createWearable((LLFolderBridge*)user_data, WT_SOCKS);
}

void LLFolderBridge::createNewJacket(void* user_data)
{
	LLFolderBridge::createWearable((LLFolderBridge*)user_data, WT_JACKET);
}

void LLFolderBridge::createNewSkirt(void* user_data)
{
	LLFolderBridge::createWearable((LLFolderBridge*)user_data, WT_SKIRT);
}

void LLFolderBridge::createNewGloves(void* user_data)
{
	LLFolderBridge::createWearable((LLFolderBridge*)user_data, WT_GLOVES);
}

void LLFolderBridge::createNewUndershirt(void* user_data)
{
	LLFolderBridge::createWearable((LLFolderBridge*)user_data, WT_UNDERSHIRT);
}

void LLFolderBridge::createNewUnderpants(void* user_data)
{
	LLFolderBridge::createWearable((LLFolderBridge*)user_data, WT_UNDERPANTS);
}

void LLFolderBridge::createNewShape(void* user_data)
{
	LLFolderBridge::createWearable((LLFolderBridge*)user_data, WT_SHAPE);
}

void LLFolderBridge::createNewSkin(void* user_data)
{
	LLFolderBridge::createWearable((LLFolderBridge*)user_data, WT_SKIN);
}

void LLFolderBridge::createNewHair(void* user_data)
{
	LLFolderBridge::createWearable((LLFolderBridge*)user_data, WT_HAIR);
}

void LLFolderBridge::createNewEyes(void* user_data)
{
	LLFolderBridge::createWearable((LLFolderBridge*)user_data, WT_EYES);
}

// static
void LLFolderBridge::createWearable(LLFolderBridge* bridge, EWearableType type)
{
	if(!bridge) return;
	LLUUID parent_id = bridge->getUUID();
	createWearable(parent_id, type);
}

// Separate function so can be called by global menu as well as right-click
// menu.
// static
void LLFolderBridge::createWearable(LLUUID parent_id, EWearableType type)
{
	LLWearable* wearable = gWearableList.createNewWearable(type);
	LLAssetType::EType asset_type = wearable->getAssetType();
	LLInventoryType::EType inv_type = LLInventoryType::IT_WEARABLE;
	create_inventory_item(gAgent.getID(), gAgent.getSessionID(),
		parent_id, wearable->getTransactionID(), wearable->getName(),
		wearable->getDescription(), asset_type, inv_type, wearable->getType(),
		wearable->getPermissions().getMaskNextOwner(),
		LLPointer<LLInventoryCallback>(NULL));
}

void LLFolderBridge::modifyOutfit(BOOL append)
{
	LLInventoryModel* model = mInventoryPanel->getModel();
	if(!model) return;
	LLViewerInventoryCategory* cat = getCategory();
	if(!cat) return;
	
	wear_inventory_category_on_avatar( cat, append );
}

// helper stuff
bool move_task_inventory_callback(const LLSD& notification, const LLSD& response, LLMoveInv* move_inv)
{
	LLFloaterOpenObject::LLCatAndWear* cat_and_wear = (LLFloaterOpenObject::LLCatAndWear* )move_inv->mUserData;
	LLViewerObject* object = gObjectList.findObject(move_inv->mObjectID);
	S32 option = LLNotification::getSelectedOption(notification, response);

	if(option == 0 && object)
	{
		if (cat_and_wear && cat_and_wear->mWear)
		{
			InventoryObjectList inventory_objects;
			object->getInventoryContents(inventory_objects);
			int contents_count = inventory_objects.size()-1; //subtract one for containing folder

			LLInventoryCopyAndWearObserver* inventoryObserver = new LLInventoryCopyAndWearObserver(cat_and_wear->mCatID, contents_count);
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

BOOL LLFolderBridge::dragItemIntoFolder(LLInventoryItem* inv_item,
										BOOL drop)
{
	LLInventoryModel* model = mInventoryPanel->getModel();
	if(!model) return FALSE;

	// cannot drag into library
	if(!isAgentInventory())
	{
		return FALSE;
	}

	LLVOAvatar* avatar = gAgent.getAvatarObject();
	if(!avatar) return FALSE;

	LLToolDragAndDrop::ESource source = LLToolDragAndDrop::getInstance()->getSource();
	BOOL accept = FALSE;
	LLViewerObject* object = NULL;
	if(LLToolDragAndDrop::SOURCE_AGENT == source)
	{

		BOOL is_movable = TRUE;
		switch( inv_item->getType() )
		{
		case LLAssetType::AT_ROOT_CATEGORY:
			is_movable = FALSE;
			break;

		case LLAssetType::AT_CATEGORY:
			is_movable = ( LLAssetType::AT_NONE == ((LLInventoryCategory*)inv_item)->getPreferredType() );
			break;
		default:
			break;
		}

		LLUUID trash_id = model->findCategoryUUIDForType(LLAssetType::AT_TRASH);
		BOOL move_is_into_trash = (mUUID == trash_id) || model->isObjectDescendentOf(mUUID, trash_id);
		if(is_movable && move_is_into_trash)
		{
			switch(inv_item->getType())
			{
			case LLAssetType::AT_CLOTHING:
			case LLAssetType::AT_BODYPART:
				is_movable = !gAgent.isWearingItem(inv_item->getUUID());
				break;

			case LLAssetType::AT_OBJECT:
				is_movable = !avatar->isWearingAttachment(inv_item->getUUID());
				break;
			default:
				break;
			}
		}
 
		accept = is_movable && (mUUID != inv_item->getParentUUID());
		if(accept && drop)
		{
			if (inv_item->getType() == LLAssetType::AT_GESTURE
				&& gGestureManager.isGestureActive(inv_item->getUUID()) && move_is_into_trash)
			{
				gGestureManager.deactivateGesture(inv_item->getUUID());
			}
			// If an item is being dragged between windows, unselect
			// everything in the active window so that we don't follow
			// the selection to its new location (which is very
			// annoying).
			if (LLInventoryView::getActiveInventory())
			{
				LLInventoryPanel* active_panel = LLInventoryView::getActiveInventory()->getPanel();
				if (active_panel && (mInventoryPanel != active_panel))
				{
					active_panel->unSelectAll();
				}
			}

			// restamp if the move is into the trash.
			LLInvFVBridge::changeItemParent(
				model,
				(LLViewerInventoryItem*)inv_item,
				mUUID,
				move_is_into_trash);
		}
	}
	else if(LLToolDragAndDrop::SOURCE_WORLD == source)
	{
		// Make sure the object exists. If we allowed dragging from
		// anonymous objects, it would be possible to bypass
		// permissions.
		object = gObjectList.findObject(inv_item->getParentUUID());
		if(!object)
		{
			llinfos << "Object not found for drop." << llendl;
			return FALSE;
		}

		// coming from a task. Need to figure out if the person can
		// move/copy this item.
		LLPermissions perm(inv_item->getPermissions());
		BOOL is_move = FALSE;
		if((perm.allowCopyBy(gAgent.getID(), gAgent.getGroupID())
			&& perm.allowTransferTo(gAgent.getID())))
//		   || gAgent.isGodlike())
			
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
		if(drop && accept)
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
				LLNotification::Params params("MoveInventoryFromObject");
				params.functor(boost::bind(move_task_inventory_callback, _1, _2, move_inv));
				LLNotifications::instance().forceResponse(params, 0);
			}
		}
		
	}
	else if(LLToolDragAndDrop::SOURCE_NOTECARD == source)
	{
		accept = TRUE;
		if(drop)
		{
			copy_inventory_from_notecard(LLToolDragAndDrop::getInstance()->getObjectID(),
				LLToolDragAndDrop::getInstance()->getSourceID(), inv_item);
		}
	}
	else if(LLToolDragAndDrop::SOURCE_LIBRARY == source)
	{
		LLViewerInventoryItem* item = (LLViewerInventoryItem*)inv_item;
		if(item && item->isComplete())
		{
			accept = TRUE;
			if(drop)
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
	else
	{
		llwarns << "unhandled drag source" << llendl;
	}
	return accept;
}

// +=================================================+
// |        LLScriptBridge (DEPRECTED)               |
// +=================================================+

LLUIImagePtr LLScriptBridge::getIcon() const
{
	return get_item_icon(LLAssetType::AT_SCRIPT, LLInventoryType::IT_LSL, 0, FALSE);
}

// +=================================================+
// |        LLTextureBridge                          |
// +=================================================+

std::string LLTextureBridge::sPrefix("Texture: ");


LLUIImagePtr LLTextureBridge::getIcon() const
{
	return get_item_icon(LLAssetType::AT_TEXTURE, mInvType, 0, FALSE);
}
	
void open_texture(const LLUUID& item_id, 
				   const std::string& title,
				   BOOL show_keep_discard,
				   const LLUUID& source_id,
				   BOOL take_focus)
{
	// See if we can bring an exiting preview to the front
	if( !LLPreview::show( item_id, take_focus ) )
	{
		// There isn't one, so make a new preview
		S32 left, top;
		gFloaterView->getNewFloaterPosition(&left, &top);
		LLRect rect = gSavedSettings.getRect("PreviewTextureRect");
		rect.translate( left - rect.mLeft, top - rect.mTop );

		LLPreviewTexture* preview;
		preview = new LLPreviewTexture("preview texture",
										  rect,
										  title,
										  item_id,
										  LLUUID::null,
										  show_keep_discard);
		preview->setSourceID(source_id);
		if(take_focus) preview->setFocus(TRUE);

		gFloaterView->adjustToFitScreen(preview, FALSE);
	}
}

void LLTextureBridge::openItem()
{
	LLViewerInventoryItem* item = getItem();
	if(item)
	{
		open_texture(mUUID, getPrefix() + item->getName(), FALSE);
	}
}

// +=================================================+
// |        LLSoundBridge                            |
// +=================================================+

std::string LLSoundBridge::sPrefix("Sound: ");


LLUIImagePtr LLSoundBridge::getIcon() const
{
	return get_item_icon(LLAssetType::AT_SOUND, LLInventoryType::IT_SOUND, 0, FALSE);
}

void LLSoundBridge::openItem()
{
// Changed this back to the way it USED to work:
// only open the preview dialog through the contextual right-click menu
// double-click just plays the sound

	LLViewerInventoryItem* item = getItem();
	if(item)
	{
		openSoundPreview((void*)this);
		//send_uuid_sound_trigger(item->getAssetUUID(), 1.0);
	}

//	if(!LLPreview::show(mUUID))
//	{
//		S32 left, top;
//		gFloaterView->getNewFloaterPosition(&left, &top);
//		LLRect rect = gSavedSettings.getRect("PreviewSoundRect");
//		rect.translate(left - rect.mLeft, top - rect.mTop);
//			new LLPreviewSound("preview sound",
//							   rect,
//							   getPrefix() + getName(),
//							   mUUID));
//	}
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
	if(!LLPreview::show(me->mUUID))
	{
		S32 left, top;
		gFloaterView->getNewFloaterPosition(&left, &top);
		LLRect rect = gSavedSettings.getRect("PreviewSoundRect");
		rect.translate(left - rect.mLeft, top - rect.mTop);
		LLPreviewSound* preview = new LLPreviewSound("preview sound",
										   rect,
										   me->getPrefix() + me->getName(),
										   me->mUUID);
		preview->setFocus(TRUE);
		// Keep entirely onscreen.
		gFloaterView->adjustToFitScreen(preview, FALSE);
	}
}

void LLSoundBridge::buildContextMenu(LLMenuGL& menu, U32 flags)
{
	lldebugs << "LLTextureBridge::buildContextMenu()" << llendl;
	std::vector<std::string> items;
	std::vector<std::string> disabled_items;

	// *TODO: Translate
	if(isInTrash())
	{
		items.push_back(std::string("Purge Item"));
		if (!isItemRemovable())
		{
			disabled_items.push_back(std::string("Purge Item"));
		}

		items.push_back(std::string("Restore Item"));
	}
	else
	{
		items.push_back(std::string("Sound Open"));
		items.push_back(std::string("Properties"));

		getClipboardEntries(true, items, disabled_items, flags);
	}

	items.push_back(std::string("Sound Separator"));
	items.push_back(std::string("Sound Play"));

	hideContextEntries(menu, items, disabled_items);
}

// +=================================================+
// |        LLLandmarkBridge                         |
// +=================================================+

std::string LLLandmarkBridge::sPrefix("Landmark:  ");

LLUIImagePtr LLLandmarkBridge::getIcon() const
{
	return get_item_icon(LLAssetType::AT_LANDMARK, LLInventoryType::IT_LANDMARK, mVisited, FALSE);
}

void LLLandmarkBridge::buildContextMenu(LLMenuGL& menu, U32 flags)
{
	std::vector<std::string> items;
	std::vector<std::string> disabled_items;

	// *TODO: Translate
	lldebugs << "LLLandmarkBridge::buildContextMenu()" << llendl;
	if(isInTrash())
	{
		items.push_back(std::string("Purge Item"));
		if (!isItemRemovable())
		{
			disabled_items.push_back(std::string("Purge Item"));
		}

		items.push_back(std::string("Restore Item"));
	}
	else
	{
		items.push_back(std::string("Landmark Open"));
		items.push_back(std::string("Properties"));

		getClipboardEntries(true, items, disabled_items, flags);
	}

	items.push_back(std::string("Landmark Separator"));
	items.push_back(std::string("Teleport To Landmark"));

	hideContextEntries(menu, items, disabled_items);

}

// virtual
void LLLandmarkBridge::performAction(LLFolderView* folder, LLInventoryModel* model, std::string action)
{
	if ("teleport" == action)
	{
		LLViewerInventoryItem* item = getItem();
		if(item)
		{
			gAgent.teleportViaLandmark(item->getAssetUUID());

			// we now automatically track the landmark you're teleporting to
			// because you'll probably arrive at a telehub instead
			if( gFloaterWorldMap )
			{
				gFloaterWorldMap->trackLandmark( item->getAssetUUID() );
			}
		}
	}
	if ("about" == action)
	{
		LLViewerInventoryItem* item = getItem();
		if(item)
		{
			open_landmark(item, std::string("  ") + getPrefix() + item->getName(), FALSE);
		}
	}
	else LLItemBridge::performAction(folder, model, action);
}

void open_landmark(LLViewerInventoryItem* inv_item,
				   const std::string& title,
				   BOOL show_keep_discard,
				   const LLUUID& source_id,
				   BOOL take_focus)
{
	// See if we can bring an exiting preview to the front
	if( !LLPreview::show( inv_item->getUUID(), take_focus ) )
	{
		// There isn't one, so make a new preview
		S32 left, top;
		gFloaterView->getNewFloaterPosition(&left, &top);
		LLRect rect = gSavedSettings.getRect("PreviewLandmarkRect");
		rect.translate( left - rect.mLeft, top - rect.mTop );

		LLPreviewLandmark* preview = new LLPreviewLandmark(title,
								  rect,
								  title,
								  inv_item->getUUID(),
								  show_keep_discard,
								  inv_item);
		preview->setSourceID(source_id);
		if(take_focus) preview->setFocus(TRUE);
		// keep onscreen
		gFloaterView->adjustToFitScreen(preview, FALSE);
	}
}

static bool open_landmark_callback(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotification::getSelectedOption(notification, response);

	LLUUID asset_id = notification["payload"]["asset_id"].asUUID();
	if (option == 0)
	{
		// HACK: This is to demonstrate teleport on double click for landmarks
		gAgent.teleportViaLandmark( asset_id );

		// we now automatically track the landmark you're teleporting to
		// because you'll probably arrive at a telehub instead
		if( gFloaterWorldMap )
		{
			gFloaterWorldMap->trackLandmark( asset_id );
		}
	}

	return false;
}
static LLNotificationFunctorRegistration open_landmark_callback_reg("TeleportFromLandmark", open_landmark_callback);


void LLLandmarkBridge::openItem()
{
	LLViewerInventoryItem* item = getItem();
	if( item )
	{
		// Opening (double-clicking) a landmark immediately teleports,
		// but warns you the first time.
		// open_landmark(item, std::string("  ") + getPrefix() + item->getName(), FALSE);
		LLSD payload;
		payload["asset_id"] = item->getAssetUUID();
		LLNotifications::instance().add("TeleportFromLandmark", LLSD(), payload);
	}
}


// +=================================================+
// |        LLCallingCardObserver                    |
// +=================================================+
void LLCallingCardObserver::changed(U32 mask)
{
	mBridgep->refreshFolderViewItem();
}

// +=================================================+
// |        LLCallingCardBridge                      |
// +=================================================+

std::string LLCallingCardBridge::sPrefix("Calling Card: ");

LLCallingCardBridge::LLCallingCardBridge( LLInventoryPanel* inventory, const LLUUID& uuid ) :
	LLItemBridge(inventory, uuid)
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
	LLFolderViewItem* itemp = mInventoryPanel->getRootFolder()->getItemByID(mUUID);
	if (itemp)
	{
		itemp->refresh();
	}
}

// virtual
void LLCallingCardBridge::performAction(LLFolderView* folder, LLInventoryModel* model, std::string action)
{
	if ("begin_im" == action)
	{
		LLViewerInventoryItem *item = getItem();
		if (item && (item->getCreatorUUID() != gAgent.getID()) &&
			(!item->getCreatorUUID().isNull()))
		{
			gIMMgr->setFloaterOpen(TRUE);
			gIMMgr->addSession(item->getName(), IM_NOTHING_SPECIAL, item->getCreatorUUID());
		}
	}
	else if ("lure" == action)
	{
		LLViewerInventoryItem *item = getItem();
		if (item && (item->getCreatorUUID() != gAgent.getID()) &&
			(!item->getCreatorUUID().isNull()))
		{
			handle_lure(item->getCreatorUUID());
		}
	}
	else LLItemBridge::performAction(folder, model, action);
}

LLUIImagePtr LLCallingCardBridge::getIcon() const
{
	BOOL online = FALSE;
	LLViewerInventoryItem* item = getItem();
	if(item)
	{
		online = LLAvatarTracker::instance().isBuddyOnline(item->getCreatorUUID());
	}
	return get_item_icon(LLAssetType::AT_CALLINGCARD, LLInventoryType::IT_CALLINGCARD, online, FALSE);
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
	if(item && !item->getCreatorUUID().isNull())
	{
		BOOL online;
		online = LLAvatarTracker::instance().isBuddyOnline(item->getCreatorUUID());
		LLFloaterAvatarInfo::showFromFriend(item->getCreatorUUID(), online);
	}
}

void LLCallingCardBridge::buildContextMenu(LLMenuGL& menu, U32 flags)
{
	// *TODO: Translate
	lldebugs << "LLCallingCardBridge::buildContextMenu()" << llendl;
	std::vector<std::string> items;
	std::vector<std::string> disabled_items;

	if(isInTrash())
	{
		items.push_back(std::string("Purge Item"));
		if (!isItemRemovable())
		{
			disabled_items.push_back(std::string("Purge Item"));
		}

		items.push_back(std::string("Restore Item"));
	}
	else
	{
		items.push_back(std::string("Open"));
		items.push_back(std::string("Properties"));

		getClipboardEntries(true, items, disabled_items, flags);

		LLInventoryItem* item = getItem();
		BOOL good_card = (item
						  && (LLUUID::null != item->getCreatorUUID())
						  && (item->getCreatorUUID() != gAgent.getID()));
		BOOL user_online = (LLAvatarTracker::instance().isBuddyOnline(item->getCreatorUUID()));
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
	hideContextEntries(menu, items, disabled_items);
}

BOOL LLCallingCardBridge::dragOrDrop(MASK mask, BOOL drop,
									 EDragAndDropType cargo_type,
									 void* cargo_data)
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
			{
				LLInventoryItem* inv_item = (LLInventoryItem*)cargo_data;
				const LLPermissions& perm = inv_item->getPermissions();
				if(gInventory.getItem(inv_item->getUUID())
				   && perm.allowOperationBy(PERM_TRANSFER, gAgent.getID()))
				{
					rv = TRUE;
					if(drop)
					{
						LLToolDragAndDrop::giveInventory(item->getCreatorUUID(),
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
						LLToolDragAndDrop::giveInventoryCategory(
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

std::string LLNotecardBridge::sPrefix("Note: ");


LLUIImagePtr LLNotecardBridge::getIcon() const
{
	return get_item_icon(LLAssetType::AT_NOTECARD, LLInventoryType::IT_NOTECARD, 0, FALSE);
}

void open_notecard(LLViewerInventoryItem* inv_item,
				   const std::string& title,
				   const LLUUID& object_id,
				   BOOL show_keep_discard,
				   const LLUUID& source_id,
				   BOOL take_focus)
{
	// See if we can bring an existing preview to the front
	if(!LLPreview::show(inv_item->getUUID(), take_focus))
	{
		// There isn't one, so make a new preview
		S32 left, top;
		gFloaterView->getNewFloaterPosition(&left, &top);
		LLRect rect = gSavedSettings.getRect("NotecardEditorRect");
		rect.translate(left - rect.mLeft, top - rect.mTop);
		LLPreviewNotecard* preview;
		preview = new LLPreviewNotecard("preview notecard", rect, title,
						inv_item->getUUID(), object_id, inv_item->getAssetUUID(),
						show_keep_discard, inv_item);
		preview->setSourceID(source_id);
		if(take_focus) preview->setFocus(TRUE);
		// Force to be entirely onscreen.
		gFloaterView->adjustToFitScreen(preview, FALSE);

		//if (source_id.notNull())
		//{
		//	// look for existing tabbed view for content from same source
		//	LLPreview* existing_preview = LLPreview::getPreviewForSource(source_id);
		//	if (existing_preview)
		//	{
		//		// found existing preview from this source
		//		// is it already hosted in a multi-preview window?
		//		LLMultiPreview* preview_hostp = (LLMultiPreview*)existing_preview->getHost();
		//		if (!preview_hostp)
		//		{
		//			// create new multipreview if it doesn't exist
		//			LLMultiPreview* preview_hostp = new LLMultiPreview(existing_preview->getRect());
		//			preview_hostp->addFloater(existing_preview);
		//		}
		//		// add this preview to existing host
		//		preview_hostp->addFloater(preview);
		//	}
		//}
	}
}


void LLNotecardBridge::openItem()
{
	LLViewerInventoryItem* item = getItem();
	if (item)
	{
		open_notecard(item, getPrefix() + item->getName(), LLUUID::null, FALSE);
	}
}


// +=================================================+
// |        LLGestureBridge                          |
// +=================================================+

std::string LLGestureBridge::sPrefix("Gesture: ");

LLUIImagePtr LLGestureBridge::getIcon() const
{
	return get_item_icon(LLAssetType::AT_GESTURE, LLInventoryType::IT_GESTURE, 0, FALSE);
}

LLFontGL::StyleFlags LLGestureBridge::getLabelStyle() const
{
	if( gGestureManager.isGestureActive(mUUID) )
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
	if( gGestureManager.isGestureActive(mUUID) )
	{
		return LLItemBridge::getLabelSuffix() + " (active)";
	}
	else
	{
		return LLItemBridge::getLabelSuffix();
	}
}

// virtual
void LLGestureBridge::performAction(LLFolderView* folder, LLInventoryModel* model, std::string action)
{
	if ("activate" == action)
	{
		gGestureManager.activateGesture(mUUID);

		LLViewerInventoryItem* item = gInventory.getItem(mUUID);
		if (!item) return;

		// Since we just changed the suffix to indicate (active)
		// the server doesn't need to know, just the viewer.
		gInventory.updateItem(item);
		gInventory.notifyObservers();
	}
	else if ("deactivate" == action)
	{
		gGestureManager.deactivateGesture(mUUID);

		LLViewerInventoryItem* item = gInventory.getItem(mUUID);
		if (!item) return;

		// Since we just changed the suffix to indicate (active)
		// the server doesn't need to know, just the viewer.
		gInventory.updateItem(item);
		gInventory.notifyObservers();
	}
	else LLItemBridge::performAction(folder, model, action);
}

void LLGestureBridge::openItem()
{
	LLViewerInventoryItem* item = getItem();
	if (!item) return;

	// See if we can bring an existing preview to the front
	if(!LLPreview::show(mUUID))
	{
		LLUUID item_id = mUUID;
		std::string title = getPrefix() + item->getName();
		LLUUID object_id = LLUUID::null;

		// TODO: save the rectangle
		LLPreviewGesture* preview = LLPreviewGesture::show(title, item_id, object_id);
		preview->setFocus(TRUE);

		// Force to be entirely onscreen.
		gFloaterView->adjustToFitScreen(preview, FALSE);
	}
}

BOOL LLGestureBridge::removeItem()
{
	// Force close the preview window, if it exists
	gGestureManager.deactivateGesture(mUUID);
	return LLItemBridge::removeItem();
}

void LLGestureBridge::buildContextMenu(LLMenuGL& menu, U32 flags)
{
	// *TODO: Translate
	lldebugs << "LLGestureBridge::buildContextMenu()" << llendl;
	std::vector<std::string> items;
	std::vector<std::string> disabled_items;
	if(isInTrash())
	{
		items.push_back(std::string("Purge Item"));
		if (!isItemRemovable())
		{
			disabled_items.push_back(std::string("Purge Item"));
		}

		items.push_back(std::string("Restore Item"));
	}
	else
	{
		items.push_back(std::string("Open"));
		items.push_back(std::string("Properties"));

		getClipboardEntries(true, items, disabled_items, flags);

		items.push_back(std::string("Gesture Separator"));
		items.push_back(std::string("Activate"));
		items.push_back(std::string("Deactivate"));

		/*menu.append(new LLMenuItemCallGL("Activate",
										 handleActivateGesture,
										 enableActivateGesture,
										 (void*)this));
		menu.append(new LLMenuItemCallGL("Deactivate",
										 handleDeactivateGesture,
										 enableDeactivateGesture,
										 (void*)this));*/
	}
	hideContextEntries(menu, items, disabled_items);
}

// +=================================================+
// |        LLAnimationBridge                        |
// +=================================================+

std::string LLAnimationBridge::sPrefix("Animation: ");


LLUIImagePtr LLAnimationBridge::getIcon() const
{
	return get_item_icon(LLAssetType::AT_ANIMATION, LLInventoryType::IT_ANIMATION, 0, FALSE);
}

void LLAnimationBridge::buildContextMenu(LLMenuGL& menu, U32 flags)
{
	// *TODO: Translate
	std::vector<std::string> items;
	std::vector<std::string> disabled_items;

	lldebugs << "LLAnimationBridge::buildContextMenu()" << llendl;
	if(isInTrash())
	{
		items.push_back(std::string("Purge Item"));
		if (!isItemRemovable())
		{
			disabled_items.push_back(std::string("Purge Item"));
		}

		items.push_back(std::string("Restore Item"));
	}
	else
	{
		items.push_back(std::string("Animation Open"));
		items.push_back(std::string("Properties"));

		getClipboardEntries(true, items, disabled_items, flags);
	}

	items.push_back(std::string("Animation Separator"));
	items.push_back(std::string("Animation Play"));
	items.push_back(std::string("Animation Audition"));

	hideContextEntries(menu, items, disabled_items);

}

// virtual
void LLAnimationBridge::performAction(LLFolderView* folder, LLInventoryModel* model, std::string action)
{
	S32 activate = 0;

	if ((action == "playworld") || (action == "playlocal"))
	{
	
		if ("playworld" == action) activate = 1;
		if ("playlocal" == action) activate = 2;

		// See if we can bring an existing preview to the front
		if( !LLPreview::show( mUUID ) )
		{
			// There isn't one, so make a new preview
			LLViewerInventoryItem* item = getItem();
			if( item )
			{
				S32 left, top;
				gFloaterView->getNewFloaterPosition(&left, &top);
				LLRect rect = gSavedSettings.getRect("PreviewAnimRect");
				rect.translate( left - rect.mLeft, top - rect.mTop );
				LLPreviewAnim* preview = new LLPreviewAnim("preview anim",
										rect,
										getPrefix() + item->getName(),
										mUUID,
										activate);
				// Force to be entirely onscreen.
				gFloaterView->adjustToFitScreen(preview, FALSE);
			}
		}
	}
	else
	{
		LLItemBridge::performAction(folder, model, action);
	}
}

void LLAnimationBridge::openItem()
{
	// See if we can bring an existing preview to the front
	if( !LLPreview::show( mUUID ) )
	{
		// There isn't one, so make a new preview
		LLViewerInventoryItem* item = getItem();
		if( item )
		{
			S32 left, top;
			gFloaterView->getNewFloaterPosition(&left, &top);
			LLRect rect = gSavedSettings.getRect("PreviewAnimRect");
			rect.translate( left - rect.mLeft, top - rect.mTop );
			LLPreviewAnim* preview = new LLPreviewAnim("preview anim",
									rect,
									getPrefix() + item->getName(),
									mUUID,
									0);
			preview->setFocus(TRUE);
			// Force to be entirely onscreen.
			gFloaterView->adjustToFitScreen(preview, FALSE);
		}
	}
}

// +=================================================+
// |        LLObjectBridge                           |
// +=================================================+

// static
std::string LLObjectBridge::sPrefix("Object: ");

// static
LLUUID LLObjectBridge::sContextMenuItemID;

BOOL LLObjectBridge::isItemRemovable()
{
	LLVOAvatar* avatar = gAgent.getAvatarObject();
	if(!avatar) return FALSE;
	if(avatar->isWearingAttachment(mUUID)) return FALSE;
	return LLInvFVBridge::isItemRemovable();
}

LLUIImagePtr LLObjectBridge::getIcon() const
{
	return get_item_icon(LLAssetType::AT_OBJECT, mInvType, mAttachPt, mIsMultiObject );
}

void rez_attachment(LLViewerInventoryItem* item, LLViewerJointAttachment* attachment);

// virtual
void LLObjectBridge::performAction(LLFolderView* folder, LLInventoryModel* model, std::string action)
{
	if ("attach" == action)
	{
		LLUUID object_id = mUUID;
		LLViewerInventoryItem* item;
		item = (LLViewerInventoryItem*)gInventory.getItem(object_id);
		if(item && gInventory.isObjectDescendentOf(object_id, gAgent.getInventoryRootID()))
		{
			rez_attachment(item, NULL);
		}
		else if(item && item->isComplete())
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
	else if ("detach" == action)
	{
		LLInventoryItem* item = gInventory.getItem(mUUID);
		if( item )
		{
			gMessageSystem->newMessageFast(_PREHASH_DetachAttachmentIntoInv);
			gMessageSystem->nextBlockFast(_PREHASH_ObjectData );
			gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
			gMessageSystem->addUUIDFast(_PREHASH_ItemID, item->getUUID() );

			gMessageSystem->sendReliable( gAgent.getRegion()->getHost() );
		}
		// this object might have been selected, so let the selection manager know it's gone now
		LLViewerObject *found_obj =
			gObjectList.findObject(item->getUUID());
		if (found_obj)
		{
			LLSelectMgr::getInstance()->remove(found_obj);
		}
		else
		{
			llwarns << "object not found - ignoring" << llendl;
		}
	}
	else LLItemBridge::performAction(folder, model, action);
}

void LLObjectBridge::openItem()
{
	/* Disabled -- this preview isn't useful. JC */
	// CP: actually, this code is required - made changes to match LLAnimationBridge::openItem() idiom
	// The properties preview is useful, converting to show object properties. - DaveP
	LLShowProps::showProperties(mUUID);
}

LLFontGL::StyleFlags LLObjectBridge::getLabelStyle() const
{ 
	LLVOAvatar* avatar = gAgent.getAvatarObject();
	if( avatar && avatar->isWearingAttachment( mUUID ) )
	{
		return LLFontGL::BOLD;
	}
	else
	{
		return LLFontGL::NORMAL;
	}
}

std::string LLObjectBridge::getLabelSuffix() const
{
	LLVOAvatar* avatar = gAgent.getAvatarObject();
	if( avatar && avatar->isWearingAttachment( mUUID ) )
	{
		std::string attachment_point_name = avatar->getAttachedPointName(mUUID);
		LLStringUtil::toLower(attachment_point_name);
		return LLItemBridge::getLabelSuffix() + std::string(" (worn on ") + attachment_point_name + std::string(")");
	}
	else
	{
		return LLItemBridge::getLabelSuffix();
	}
}

void rez_attachment(LLViewerInventoryItem* item, LLViewerJointAttachment* attachment)
{
	LLSD payload;
	payload["item_id"] = item->getUUID();

	S32 attach_pt = 0;
	if (gAgent.getAvatarObject() && attachment)
	{
		for (LLVOAvatar::attachment_map_t::iterator iter = gAgent.getAvatarObject()->mAttachmentPoints.begin();
			 iter != gAgent.getAvatarObject()->mAttachmentPoints.end(); ++iter)
		{
			if (iter->second == attachment)
			{
				attach_pt = iter->first;
				break;
			}
		}
	}

	payload["attachment_point"] = attach_pt;

	if (attachment && attachment->getObject())
	{
		LLNotifications::instance().add("ReplaceAttachment", LLSD(), payload, confirm_replace_attachment_rez);
	}
	else
	{
		LLNotifications::instance().forceResponse(LLNotification::Params("ReplaceAttachment").payload(payload), 0/*YES*/);
	}
}

bool confirm_replace_attachment_rez(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotification::getSelectedOption(notification, response);
	if (option == 0/*YES*/)
	{
		LLViewerInventoryItem* itemp = gInventory.getItem(notification["payload"]["item_id"].asUUID());
		
		if (itemp)
		{
			LLMessageSystem* msg = gMessageSystem;
			msg->newMessageFast(_PREHASH_RezSingleAttachmentFromInv);
			msg->nextBlockFast(_PREHASH_AgentData);
			msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
			msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
			msg->nextBlockFast(_PREHASH_ObjectData);
			msg->addUUIDFast(_PREHASH_ItemID, itemp->getUUID());
			msg->addUUIDFast(_PREHASH_OwnerID, itemp->getPermissions().getOwner());
			msg->addU8Fast(_PREHASH_AttachmentPt, notification["payload"]["attachment_point"].asInteger());
			pack_permissions_slam(msg, itemp->getFlags(), itemp->getPermissions());
			msg->addStringFast(_PREHASH_Name, itemp->getName());
			msg->addStringFast(_PREHASH_Description, itemp->getDescription());
			msg->sendReliable(gAgent.getRegion()->getHost());
		}
	}
	return false;
}
static LLNotificationFunctorRegistration confirm_replace_attachment_rez_reg("ReplaceAttachment", confirm_replace_attachment_rez);

void LLObjectBridge::buildContextMenu(LLMenuGL& menu, U32 flags)
{
	// *TODO: Translate
	std::vector<std::string> items;
	std::vector<std::string> disabled_items;
	if(isInTrash())
	{
		items.push_back(std::string("Purge Item"));
		if (!isItemRemovable())
		{
			disabled_items.push_back(std::string("Purge Item"));
		}

		items.push_back(std::string("Restore Item"));
	}
	else
	{
		items.push_back(std::string("Properties"));

		getClipboardEntries(true, items, disabled_items, flags);

		LLObjectBridge::sContextMenuItemID = mUUID;

		LLInventoryItem* item = getItem();
		if(item)
		{
			LLVOAvatar *avatarp = gAgent.getAvatarObject();
			if( !avatarp )
			{
				return;
			}
			
			if( avatarp->isWearingAttachment( mUUID ) )
			{
				items.push_back(std::string("Detach From Yourself"));
			}
			else
			if( !isInTrash() )
			{
				items.push_back(std::string("Attach Separator"));
				items.push_back(std::string("Object Wear"));
				items.push_back(std::string("Attach To"));
				items.push_back(std::string("Attach To HUD"));
				items.push_back(std::string("Restore to Last Position"));

				LLMenuGL* attach_menu = menu.getChildMenuByName("Attach To", TRUE);
				LLMenuGL* attach_hud_menu = menu.getChildMenuByName("Attach To HUD", TRUE);
				LLVOAvatar *avatarp = gAgent.getAvatarObject();
				if (attach_menu && (attach_menu->getChildCount() == 0) &&
					attach_hud_menu && (attach_hud_menu->getChildCount() == 0) &&
					avatarp)
				{
					for (LLVOAvatar::attachment_map_t::iterator iter = avatarp->mAttachmentPoints.begin(); 
						 iter != avatarp->mAttachmentPoints.end(); )
					{
						LLVOAvatar::attachment_map_t::iterator curiter = iter++;
						LLViewerJointAttachment* attachment = curiter->second;
						LLMenuItemCallGL *new_item;
						if (attachment->getIsHUDAttachment())
						{
							attach_hud_menu->append(new_item = new LLMenuItemCallGL(attachment->getName(), 
								NULL, //&LLObjectBridge::attachToAvatar, 
								NULL, &attach_label, (void*)attachment));
						}
						else
						{
							attach_menu->append(new_item = new LLMenuItemCallGL(attachment->getName(), 
								NULL, //&LLObjectBridge::attachToAvatar,
								NULL, &attach_label, (void*)attachment));
						}

						LLSimpleListener* callback = mInventoryPanel->getListenerByName("Inventory.AttachObject");

						if (callback)
						{
							new_item->addListener(callback, "on_click", LLSD(attachment->getName()));
						}
					}
				}
			}
		}
	}
	hideContextEntries(menu, items, disabled_items);
}

BOOL LLObjectBridge::renameItem(const std::string& new_name)
{
	if(!isItemRenameable()) return FALSE;
	LLPreview::rename(mUUID, getPrefix() + new_name);
	LLInventoryModel* model = mInventoryPanel->getModel();
	if(!model) return FALSE;
	LLViewerInventoryItem* item = getItem();
	if(item && (item->getName() != new_name))
	{
		LLPointer<LLViewerInventoryItem> new_item = new LLViewerInventoryItem(item);
		new_item->rename(new_name);
		buildDisplayName(new_item, mDisplayName);
		new_item->updateServer(FALSE);
		model->updateItem(new_item);
		model->notifyObservers();

		LLVOAvatar* avatar = gAgent.getAvatarObject();
		if( avatar )
		{
			LLViewerObject* obj = avatar->getWornAttachment( item->getUUID() );
			if( obj )
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

std::string LLLSLTextBridge::sPrefix("Script: ");

LLUIImagePtr LLLSLTextBridge::getIcon() const
{
	return get_item_icon(LLAssetType::AT_SCRIPT, LLInventoryType::IT_LSL, 0, FALSE);
}

void LLLSLTextBridge::openItem()
{
	// See if we can bring an exiting preview to the front
	if(!LLPreview::show(mUUID))
	{
		LLViewerInventoryItem* item = getItem();
		if (item)
		{
			// There isn't one, so make a new preview
			S32 left, top;
			gFloaterView->getNewFloaterPosition(&left, &top);
			LLRect rect = gSavedSettings.getRect("PreviewScriptRect");
			rect.translate(left - rect.mLeft, top - rect.mTop);
			
			LLPreviewLSL* preview =	new LLPreviewLSL("preview lsl text",
											 rect,
											 getPrefix() + item->getName(),
											 mUUID);
			preview->setFocus(TRUE);
			// keep onscreen
			gFloaterView->adjustToFitScreen(preview, FALSE);
		}
	}
}

// +=================================================+
// |        LLWearableBridge                         |
// +=================================================+

// *NOTE: hack to get from avatar inventory to avatar
void wear_inventory_item_on_avatar( LLInventoryItem* item )
{
	if(item)
	{
		lldebugs << "wear_inventory_item_on_avatar( " << item->getName()
				 << " )" << llendl;
			
		gWearableList.getAsset(item->getAssetUUID(),
							   item->getName(),
							   item->getType(),
							   LLWearableBridge::onWearOnAvatarArrived,
							   new LLUUID(item->getUUID()));
	}
}

struct LLFoundData
{
	LLFoundData(const LLUUID& item_id,
				const LLUUID& asset_id,
				const std::string& name,
				LLAssetType::EType asset_type) :
		mItemID(item_id),
		mAssetID(asset_id),
		mName(name),
		mAssetType(asset_type),
		mWearable( NULL ) {}
	
	LLUUID mItemID;
	LLUUID mAssetID;
	std::string mName;
	LLAssetType::EType mAssetType;
	LLWearable* mWearable;
};

struct LLWearableHoldingPattern
{
	LLWearableHoldingPattern() : mResolved(0) {}
	~LLWearableHoldingPattern()
	{
		for_each(mFoundList.begin(), mFoundList.end(), DeletePointer());
		mFoundList.clear();
	}
	typedef std::list<LLFoundData*> found_list_t;
	found_list_t mFoundList;
	S32 mResolved;
};


class LLOutfitObserver : public LLInventoryFetchObserver
{
public:
	LLOutfitObserver(const LLUUID& cat_id, bool copy_items, bool append) :
		mCatID(cat_id),
		mCopyItems(copy_items),
		mAppend(append)
	{}
	~LLOutfitObserver() {}
	virtual void done(); //public

protected:
	LLUUID mCatID;
	bool mCopyItems;
	bool mAppend;
};

class LLWearInventoryCategoryCallback : public LLInventoryCallback
{
public:
	LLWearInventoryCategoryCallback(const LLUUID& cat_id, bool append)
	{
		mCatID = cat_id;
		mAppend = append;
	}
	void fire(const LLUUID& item_id)
	{
		/*
		 * Do nothing.  We only care about the destructor
		 *
		 * The reason for this is that this callback is used in a hack where the
		 * same callback is given to dozens of items, and the destructor is called
		 * after the last item has fired the event and dereferenced it -- if all
		 * the events actually fire!
		 */
	}

protected:
	~LLWearInventoryCategoryCallback()
	{
		// Is the destructor called by ordinary dereference, or because the app's shutting down?
		// If the inventory callback manager goes away, we're shutting down, no longer want the callback.
		if( LLInventoryCallbackManager::is_instantiated() )
		{
			wear_inventory_category_on_avatar(gInventory.getCategory(mCatID), mAppend);
		}
		else
		{
			llwarns << "Dropping unhandled LLWearInventoryCategoryCallback" << llendl;
		}
	}

private:
	LLUUID mCatID;
	bool mAppend;
};

void LLOutfitObserver::done()
{
	// We now have an outfit ready to be copied to agent inventory. Do
	// it, and wear that outfit normally.
	if(mCopyItems)
	{
		LLInventoryCategory* cat = gInventory.getCategory(mCatID);
		std::string name;
		if(!cat)
		{
			// should never happen.
			name = "New Outfit";
		}
		else
		{
			name = cat->getName();
		}
		LLViewerInventoryItem* item = NULL;
		item_ref_t::iterator it = mComplete.begin();
		item_ref_t::iterator end = mComplete.end();
		LLUUID pid;
		for(; it < end; ++it)
		{
			item = (LLViewerInventoryItem*)gInventory.getItem(*it);
			if(item)
			{
				if(LLInventoryType::IT_GESTURE == item->getInventoryType())
				{
					pid = gInventory.findCategoryUUIDForType(LLAssetType::AT_GESTURE);
				}
				else
				{
					pid = gInventory.findCategoryUUIDForType(LLAssetType::AT_CLOTHING);
				}
				break;
			}
		}
		if(pid.isNull())
		{
			pid = gAgent.getInventoryRootID();
		}
		
		LLUUID cat_id = gInventory.createNewCategory(
			pid,
			LLAssetType::AT_NONE,
			name);
		mCatID = cat_id;
		LLPointer<LLInventoryCallback> cb = new LLWearInventoryCategoryCallback(mCatID, mAppend);
		it = mComplete.begin();
		for(; it < end; ++it)
		{
			item = (LLViewerInventoryItem*)gInventory.getItem(*it);
			if(item)
			{
				copy_inventory_item(
					gAgent.getID(),
					item->getPermissions().getOwner(),
					item->getUUID(),
					cat_id,
					std::string(),
					cb);
			}
		}
	}
	else
	{
		// Wear the inventory category.
		wear_inventory_category_on_avatar(gInventory.getCategory(mCatID), mAppend);
	}
}

class LLOutfitFetch : public LLInventoryFetchDescendentsObserver
{
public:
	LLOutfitFetch(bool copy_items, bool append) : mCopyItems(copy_items), mAppend(append) {}
	~LLOutfitFetch() {}
	virtual void done();
protected:
	bool mCopyItems;
	bool mAppend;
};

void LLOutfitFetch::done()
{
	// What we do here is get the complete information on the items in
	// the library, and set up an observer that will wait for that to
	// happen.
	LLInventoryModel::cat_array_t cat_array;
	LLInventoryModel::item_array_t item_array;
	gInventory.collectDescendents(mCompleteFolders.front(),
								  cat_array,
								  item_array,
								  LLInventoryModel::EXCLUDE_TRASH);
	S32 count = item_array.count();
	if(!count)
	{
		llwarns << "Nothing fetched in category " << mCompleteFolders.front()
				<< llendl;
		dec_busy_count();
		gInventory.removeObserver(this);
		delete this;
		return;
	}

	LLOutfitObserver* outfit;
	outfit = new LLOutfitObserver(mCompleteFolders.front(), mCopyItems, mAppend);
	LLInventoryFetchObserver::item_ref_t ids;
	for(S32 i = 0; i < count; ++i)
	{
		ids.push_back(item_array.get(i)->getUUID());
	}

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
	outfit->fetchItems(ids);
	if(outfit->isEverythingComplete())
	{
		// everything is already here - call done.
		outfit->done();
	}
	else
	{
		// it's all on it's way - add an observer, and the inventory
		// will call done for us when everything is here.
		gInventory.addObserver(outfit);
	}
}

void wear_outfit_by_name(const std::string& name)
{
	llinfos << "Wearing category " << name << llendl;
	inc_busy_count();

	LLInventoryModel::cat_array_t cat_array;
	LLInventoryModel::item_array_t item_array;
	LLNameCategoryCollector has_name(name);
	gInventory.collectDescendentsIf(gAgent.getInventoryRootID(),
									cat_array,
									item_array,
									LLInventoryModel::EXCLUDE_TRASH,
									has_name);
	bool copy_items = false;
	LLInventoryCategory* cat = NULL;
	if (cat_array.count() > 0)
	{
		// Just wear the first one that matches
		cat = cat_array.get(0);
	}
	else
	{
		gInventory.collectDescendentsIf(LLUUID::null,
										cat_array,
										item_array,
										LLInventoryModel::EXCLUDE_TRASH,
										has_name);
		if(cat_array.count() > 0)
		{
			cat = cat_array.get(0);
			copy_items = true;
		}
	}

	if(cat)
	{
		wear_inventory_category(cat, copy_items, false);
	}
	else
	{
		llwarns << "Couldn't find outfit " <<name<< " in wear_outfit_by_name()"
				<< llendl;
	}

	dec_busy_count();
}

void wear_inventory_category(LLInventoryCategory* category, bool copy, bool append)
{
	if(!category) return;

	lldebugs << "wear_inventory_category( " << category->getName()
			 << " )" << llendl;
	// What we do here is get the complete information on the items in
	// the inventory, and set up an observer that will wait for that to
	// happen.
	LLOutfitFetch* outfit;
	outfit = new LLOutfitFetch(copy, append);
	LLInventoryFetchDescendentsObserver::folder_ref_t folders;
	folders.push_back(category->getUUID());
	outfit->fetchDescendents(folders);
	inc_busy_count();
	if(outfit->isEverythingComplete())
	{
		// everything is already here - call done.
		outfit->done();
	}
	else
	{
		// it's all on it's way - add an observer, and the inventory
		// will call done for us when everything is here.
		gInventory.addObserver(outfit);
	}
}

// *NOTE: hack to get from avatar inventory to avatar
void wear_inventory_category_on_avatar( LLInventoryCategory* category, BOOL append )
{
	// Avoid unintentionally overwriting old wearables.  We have to do
	// this up front to avoid having to deal with the case of multiple
	// wearables being dirty.
	if(!category) return;
	lldebugs << "wear_inventory_category_on_avatar( " << category->getName()
			 << " )" << llendl;
			 	
	LLWearInfo* userdata = new LLWearInfo;
	userdata->mAppend = append;
	userdata->mCategoryID = category->getUUID();

	if( gFloaterCustomize )
	{
		gFloaterCustomize->askToSaveIfDirty(
			wear_inventory_category_on_avatar_step2,
			userdata);
	}
	else
	{
		wear_inventory_category_on_avatar_step2(
			TRUE,
			userdata );
	}
}


void wear_inventory_category_on_avatar_step2( BOOL proceed, void* userdata )
{
	LLWearInfo* wear_info = (LLWearInfo*)userdata;
	if (!wear_info) return;

	// Find all the wearables that are in the category's subtree.	
	lldebugs << "wear_inventory_category_on_avatar_step2()" << llendl;
	if(proceed)
	{
		LLInventoryModel::cat_array_t cat_array;
		LLInventoryModel::item_array_t item_array;
		LLFindWearables is_wearable;
		gInventory.collectDescendentsIf(wear_info->mCategoryID,
										cat_array,
										item_array,
										LLInventoryModel::EXCLUDE_TRASH,
										is_wearable);
		S32 i;
		S32 wearable_count = item_array.count();

		LLInventoryModel::cat_array_t	obj_cat_array;
		LLInventoryModel::item_array_t	obj_item_array;
		LLIsType is_object( LLAssetType::AT_OBJECT );
		gInventory.collectDescendentsIf(wear_info->mCategoryID,
										obj_cat_array,
										obj_item_array,
										LLInventoryModel::EXCLUDE_TRASH,
										is_object);
		S32 obj_count = obj_item_array.count();

		// Find all gestures in this folder
		LLInventoryModel::cat_array_t	gest_cat_array;
		LLInventoryModel::item_array_t	gest_item_array;
		LLIsType is_gesture( LLAssetType::AT_GESTURE );
		gInventory.collectDescendentsIf(wear_info->mCategoryID,
										gest_cat_array,
										gest_item_array,
										LLInventoryModel::EXCLUDE_TRASH,
										is_gesture);
		S32 gest_count = gest_item_array.count();

		if( !wearable_count && !obj_count && !gest_count)
		{
			LLNotifications::instance().add("CouldNotPutOnOutfit");
			delete wear_info;
			return;
		}

		// Processes that take time should show the busy cursor
		if (wearable_count > 0 || obj_count > 0)
		{
			inc_busy_count();
		}

		// Activate all gestures in this folder
		if (gest_count > 0)
		{
			llinfos << "Activating " << gest_count << " gestures" << llendl;

			gGestureManager.activateGestures(gest_item_array);

			// Update the inventory item labels to reflect the fact
			// they are active.
			LLViewerInventoryCategory* catp = gInventory.getCategory(wear_info->mCategoryID);
			if (catp)
			{
				gInventory.updateCategory(catp);
				gInventory.notifyObservers();
			}
		}

		if(wearable_count > 0)
		{
			// Note: can't do normal iteration, because if all the
			// wearables can be resolved immediately, then the
			// callback will be called (and this object deleted)
			// before the final getNextData().
			LLWearableHoldingPattern* holder = new LLWearableHoldingPattern;
			LLFoundData* found;
			LLDynamicArray<LLFoundData*> found_container;
			for(i = 0; i  < wearable_count; ++i)
			{
				found = new LLFoundData(item_array.get(i)->getUUID(),
										item_array.get(i)->getAssetUUID(),
										item_array.get(i)->getName(),
										item_array.get(i)->getType());
				holder->mFoundList.push_front(found);
				found_container.put(found);
			}
			for(i = 0; i < wearable_count; ++i)
			{
				gAddToOutfit = wear_info->mAppend;

				found = found_container.get(i);
				gWearableList.getAsset(found->mAssetID,
										found->mName,
									   found->mAssetType,
									   wear_inventory_category_on_avatar_loop,
									   (void*)holder);
			}
		}


		//If not appending and the folder doesn't contain only gestures, take off all attachments.
		if (!wear_info->mAppend 
			&& !(wearable_count == 0 && obj_count == 0 && gest_count > 0) )
		{
			LLAgent::userRemoveAllAttachments(NULL);
		}

		if( obj_count > 0 )
		{
			// We've found some attachements.  Add these.

			LLVOAvatar* avatar = gAgent.getAvatarObject();
			if( avatar )
			{
				// Build a compound message to send all the objects that need to be rezzed.

				// Limit number of packets to send
				const S32 MAX_PACKETS_TO_SEND = 10;
				const S32 OBJECTS_PER_PACKET = 4;
				const S32 MAX_OBJECTS_TO_SEND = MAX_PACKETS_TO_SEND * OBJECTS_PER_PACKET;
				if( obj_count > MAX_OBJECTS_TO_SEND )
				{
					obj_count = MAX_OBJECTS_TO_SEND;
				}
				
				// Create an id to keep the parts of the compound message together
				LLUUID compound_msg_id;
				compound_msg_id.generate();
				LLMessageSystem* msg = gMessageSystem;

				for(i = 0; i < obj_count; ++i)
				{
					if( 0 == (i % OBJECTS_PER_PACKET) )
					{
						// Start a new message chunk
						msg->newMessageFast(_PREHASH_RezMultipleAttachmentsFromInv);
						msg->nextBlockFast(_PREHASH_AgentData);
						msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
						msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
						msg->nextBlockFast(_PREHASH_HeaderData);
						msg->addUUIDFast(_PREHASH_CompoundMsgID, compound_msg_id );
						msg->addU8Fast(_PREHASH_TotalObjects, obj_count );
						msg->addBOOLFast(_PREHASH_FirstDetachAll, !wear_info->mAppend );
					}

					LLInventoryItem* item = obj_item_array.get(i);
					msg->nextBlockFast(_PREHASH_ObjectData );
					msg->addUUIDFast(_PREHASH_ItemID, item->getUUID() );
					msg->addUUIDFast(_PREHASH_OwnerID, item->getPermissions().getOwner());
					msg->addU8Fast(_PREHASH_AttachmentPt, 0 );	// Wear at the previous or default attachment point
					pack_permissions_slam(msg, item->getFlags(), item->getPermissions());
					msg->addStringFast(_PREHASH_Name, item->getName());
					msg->addStringFast(_PREHASH_Description, item->getDescription());

					if( (i+1 == obj_count) || ((OBJECTS_PER_PACKET-1) == (i % OBJECTS_PER_PACKET)) )
					{
						// End of message chunk
						msg->sendReliable( gAgent.getRegion()->getHost() );
					}
				}
			}
		}
	}
	delete wear_info;
	wear_info = NULL;
}

void wear_inventory_category_on_avatar_loop(LLWearable* wearable, void* data)
{
	LLWearableHoldingPattern* holder = (LLWearableHoldingPattern*)data;
	BOOL append= gAddToOutfit;
	
	if(wearable)
	{
		for (LLWearableHoldingPattern::found_list_t::iterator iter = holder->mFoundList.begin();
			 iter != holder->mFoundList.end(); ++iter)
		{
			LLFoundData* data = *iter;
			if(wearable->getID() == data->mAssetID)
			{
				data->mWearable = wearable;
				break;
			}
		}
	}
	holder->mResolved += 1;
	if(holder->mResolved >= (S32)holder->mFoundList.size())
	{
		wear_inventory_category_on_avatar_step3(holder, append);
	}
}

void wear_inventory_category_on_avatar_step3(LLWearableHoldingPattern* holder, BOOL append)
{
	lldebugs << "wear_inventory_category_on_avatar_step3()" << llendl;
	LLInventoryItem::item_array_t items;
	LLDynamicArray< LLWearable* > wearables;

	// For each wearable type, find the first instance in the category
	// that we recursed through.
	for( S32 i = 0; i < WT_COUNT; i++ )
	{
		for (LLWearableHoldingPattern::found_list_t::iterator iter = holder->mFoundList.begin();
			 iter != holder->mFoundList.end(); ++iter)
		{
			LLFoundData* data = *iter;
			LLWearable* wearable = data->mWearable;
			if( wearable && ((S32)wearable->getType() == i) )
			{
				LLViewerInventoryItem* item;
				item = (LLViewerInventoryItem*)gInventory.getItem(data->mItemID);
				if( item && (item->getAssetUUID() == wearable->getID()) )
				{
				//RN: after discussing with Brashears, I disabled this code
				//Metadata should reside in the item, not the asset
				//And this code does not handle failed asset uploads properly
//					if(!wearable->isMatchedToInventoryItem(item ))
//					{
//						wearable = gWearableList.createWearableMatchedToInventoryItem( wearable, item );
//						// Now that we have an asset that matches the
//						// item, update the item to point to the new
//						// asset.
//						item->setAssetUUID(wearable->getID());
//						item->updateAssetOnServer();
//					}
					items.put(item);
					wearables.put(wearable);
				}
				break;
			}
		}
	}

	if(wearables.count() > 0)
	{
		gAgent.setWearableOutfit(items, wearables, !append);
		gInventory.notifyObservers();
	}

	delete holder;

	dec_busy_count();
}

void remove_inventory_category_from_avatar( LLInventoryCategory* category )
{
	if(!category) return;
	lldebugs << "remove_inventory_category_from_avatar( " << category->getName()
			 << " )" << llendl;
			 
	
	LLUUID* uuid	= new LLUUID(category->getUUID());

	if( gFloaterCustomize )
	{
		gFloaterCustomize->askToSaveIfDirty(
			remove_inventory_category_from_avatar_step2,
			uuid);
	}
	else
	{
		remove_inventory_category_from_avatar_step2(
			TRUE,
			uuid );
	}
}


void remove_inventory_category_from_avatar_step2( BOOL proceed, void* userdata)
{

	// Find all the wearables that are in the category's subtree.
	LLUUID* category_id = (LLUUID *)userdata;
	
	lldebugs << "remove_inventory_category_from_avatar_step2()" << llendl;
	if(proceed)
	{
		LLInventoryModel::cat_array_t cat_array;
		LLInventoryModel::item_array_t item_array;
		LLFindWearables is_wearable;
		gInventory.collectDescendentsIf(*category_id,
										cat_array,
										item_array,
										LLInventoryModel::EXCLUDE_TRASH,
										is_wearable);
		S32 i;
		S32 wearable_count = item_array.count();

		LLInventoryModel::cat_array_t	obj_cat_array;
		LLInventoryModel::item_array_t	obj_item_array;
		LLIsType is_object( LLAssetType::AT_OBJECT );
		gInventory.collectDescendentsIf(*category_id,
										obj_cat_array,
										obj_item_array,
										LLInventoryModel::EXCLUDE_TRASH,
										is_object);
		S32 obj_count = obj_item_array.count();

		// Find all gestures in this folder
		LLInventoryModel::cat_array_t	gest_cat_array;
		LLInventoryModel::item_array_t	gest_item_array;
		LLIsType is_gesture( LLAssetType::AT_GESTURE );
		gInventory.collectDescendentsIf(*category_id,
										gest_cat_array,
										gest_item_array,
										LLInventoryModel::EXCLUDE_TRASH,
										is_gesture);
		S32 gest_count = gest_item_array.count();

		if (wearable_count > 0)	//Loop through wearables.  If worn, remove.
		{
			for(i = 0; i  < wearable_count; ++i)
			{
				if( gAgent.isWearingItem (item_array.get(i)->getUUID()) )
				{
					gWearableList.getAsset(item_array.get(i)->getAssetUUID(),
									item_array.get(i)->getName(),
								   item_array.get(i)->getType(),
								    LLWearableBridge::onRemoveFromAvatarArrived,
								   new LLUUID(item_array.get(i)->getUUID()));

				}
			}
		}
		
		
		if (obj_count > 0)
		{
			for(i = 0; i  < obj_count; ++i)
			{
				gMessageSystem->newMessageFast(_PREHASH_DetachAttachmentIntoInv);
				gMessageSystem->nextBlockFast(_PREHASH_ObjectData );
				gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
				gMessageSystem->addUUIDFast(_PREHASH_ItemID, obj_item_array.get(i)->getUUID() );

				gMessageSystem->sendReliable( gAgent.getRegion()->getHost() );

				// this object might have been selected, so let the selection manager know it's gone now
				LLViewerObject *found_obj = gObjectList.findObject( obj_item_array.get(i)->getUUID());
				if (found_obj)
				{
					LLSelectMgr::getInstance()->remove(found_obj);
				}
				else
				{
					llwarns << "object not found, ignoring" << llendl;
				}
			}
		}

		if (gest_count > 0)
		{
			for(i = 0; i  < gest_count; ++i)
			{
				if ( gGestureManager.isGestureActive( gest_item_array.get(i)->getUUID()) )
				{
					gGestureManager.deactivateGesture( gest_item_array.get(i)->getUUID() );
					gInventory.updateItem( gest_item_array.get(i) );
					gInventory.notifyObservers();
				}

			}
		}
	}
	delete category_id;
	category_id = NULL;
}

BOOL LLWearableBridge::renameItem(const std::string& new_name)
{
	if( gAgent.isWearingItem( mUUID ) )
	{
		gAgent.setWearableName( mUUID, new_name );
	}
	return LLItemBridge::renameItem(new_name);
}

BOOL LLWearableBridge::isItemRemovable()
{
	if(gAgent.isWearingItem(mUUID)) return FALSE;
	return LLInvFVBridge::isItemRemovable();
}

LLFontGL::StyleFlags LLWearableBridge::getLabelStyle() const
{ 
	if( gAgent.isWearingItem( mUUID ) )
	{
		// llinfos << "BOLD" << llendl;
		return LLFontGL::BOLD;
	}
	else
	{
		return LLFontGL::NORMAL;
	}
}

std::string LLWearableBridge::getLabelSuffix() const
{
	if( gAgent.isWearingItem( mUUID ) )
	{
		return LLItemBridge::getLabelSuffix() + " (worn)";
	}
	else
	{
		return LLItemBridge::getLabelSuffix();
	}
}

LLUIImagePtr LLWearableBridge::getIcon() const
{
	return get_item_icon(mAssetType, mInvType, mWearableType, FALSE);
}

// virtual
void LLWearableBridge::performAction(LLFolderView* folder, LLInventoryModel* model, std::string action)
{
	if ("wear" == action)
	{
		wearOnAvatar();
	}
	else if ("edit" == action)
	{
		editOnAvatar();
		return;
	}
	else if ("take_off" == action)
	{
		if(gAgent.isWearingItem(mUUID))
		{
			LLViewerInventoryItem* item = getItem();
			if (item)
			{
				gWearableList.getAsset(item->getAssetUUID(),
										item->getName(),
									item->getType(),
									LLWearableBridge::onRemoveFromAvatarArrived,
									new LLUUID(mUUID));
			}
		}
	}
	else LLItemBridge::performAction(folder, model, action);
}

void LLWearableBridge::openItem()
{
	if( isInTrash() )
	{
		LLNotifications::instance().add("CannotWearTrash");
	}
	else if(isAgentInventory())
	{
		if( !gAgent.isWearingItem( mUUID ) )
		{
			wearOnAvatar();
		}
	}
	else
	{
		// must be in the inventory library. copy it to our inventory
		// and put it on right away.
		LLViewerInventoryItem* item = getItem();
		if(item && item->isComplete())
		{
			LLPointer<LLInventoryCallback> cb = new WearOnAvatarCallback();
			copy_inventory_item(
				gAgent.getID(),
				item->getPermissions().getOwner(),
				item->getUUID(),
				LLUUID::null,
				std::string(),
				cb);
		}
		else if(item)
		{
			// *TODO: We should fetch the item details, and then do
			// the operation above.
			LLNotifications::instance().add("CannotWearInfoNotComplete");
		}
	}
}

void LLWearableBridge::buildContextMenu(LLMenuGL& menu, U32 flags)
{
	// *TODO: Translate
	lldebugs << "LLWearableBridge::buildContextMenu()" << llendl;
	std::vector<std::string> items;
	std::vector<std::string> disabled_items;
	if(isInTrash())
	{
		items.push_back(std::string("Purge Item"));
		if (!isItemRemovable())
		{
			disabled_items.push_back(std::string("Purge Item"));
		}

		items.push_back(std::string("Restore Item"));
	}
	else
	{	// FWIW, it looks like SUPPRESS_OPEN_ITEM is not set anywhere
		BOOL no_open = ((flags & SUPPRESS_OPEN_ITEM) == SUPPRESS_OPEN_ITEM);

		// If we have clothing, don't add "Open" as it's the same action as "Wear"   SL-18976
		LLViewerInventoryItem* item = getItem();
		if( !no_open && item )
		{
			no_open = (item->getType() == LLAssetType::AT_CLOTHING) ||
					  (item->getType() == LLAssetType::AT_BODYPART);
		}
		if (!no_open)
		{
			items.push_back(std::string("Open"));
		}

		items.push_back(std::string("Properties"));

		getClipboardEntries(true, items, disabled_items, flags);

		items.push_back(std::string("Wearable Separator"));
		
		items.push_back(std::string("Wearable Wear"));
		items.push_back(std::string("Wearable Edit"));

		if ((flags & FIRST_SELECTED_ITEM) == 0)
		{
			disabled_items.push_back(std::string("Wearable Edit"));
		}
		//menu.appendSeparator();
		//menu.append(new LLMenuItemCallGL("Wear",
		//								 LLWearableBridge::onWearOnAvatar,
		//								 LLWearableBridge::canWearOnAvatar,
		//								 (void*)this));
		//menu.append(new LLMenuItemCallGL("Edit",
		//								 LLWearableBridge::onEditOnAvatar,
		//								 LLWearableBridge::canEditOnAvatar,
		//								 (void*)this));

		if( item && (item->getType() == LLAssetType::AT_CLOTHING) )
		{
			items.push_back(std::string("Take Off"));
			/*menu.append(new LLMenuItemCallGL("Take Off",
											 LLWearableBridge::onRemoveFromAvatar,
											 LLWearableBridge::canRemoveFromAvatar,
											 (void*)this));*/
		}
	}
	hideContextEntries(menu, items, disabled_items);
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
		if(!item || !item->isComplete()) return FALSE;
	}
	return (!gAgent.isWearingItem(self->mUUID));
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
	// Don't wear anything until initial wearables are loaded, can
	// destroy clothing items.
	if (!gAgent.areWearablesLoaded()) 
	{
		LLNotifications::instance().add("CanNotChangeAppearanceUntilLoaded");
		return;
	}

	LLViewerInventoryItem* item = getItem();
	if(item)
	{
		if(!isAgentInventory())
		{
			LLPointer<LLInventoryCallback> cb = new WearOnAvatarCallback();
			copy_inventory_item(
				gAgent.getID(),
				item->getPermissions().getOwner(),
				item->getUUID(),
				LLUUID::null,
				std::string(),
				cb);
		}
		else
		{
			wear_inventory_item_on_avatar(item);
		}
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
			if(item->getAssetUUID() == wearable->getID())
			{
				//RN: after discussing with Brashears, I disabled this code
				//Metadata should reside in the item, not the asset
				//And this code does not handle failed asset uploads properly

//				if(!wearable->isMatchedToInventoryItem(item))
//				{
//					LLWearable* new_wearable = gWearableList.createWearableMatchedToInventoryItem( wearable, item );
//
//					// Now that we have an asset that matches the
//					// item, update the item to point to the new
//					// asset.
//					item->setAssetUUID(new_wearable->getID());
//					item->updateAssetOnServer();
//					wearable = new_wearable;
//				}
				gAgent.setWearable(item, wearable);
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

	return (gAgent.isWearingItem(self->mUUID));
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
	LLWearable* wearable = gAgent.getWearableFromWearableItem(mUUID);
	if( wearable )
	{
		// Set the tab to the right wearable.
		LLFloaterCustomize::setCurrentWearableType( wearable->getType() );

		if( CAMERA_MODE_CUSTOMIZE_AVATAR != gAgent.getCameraMode() )
		{
			// Start Avatar Customization
			gAgent.changeCameraToCustomizeAvatar();
		}
	}
}

// static
BOOL LLWearableBridge::canRemoveFromAvatar(void* user_data)
{
	LLWearableBridge* self = (LLWearableBridge*)user_data;
	if( self && (LLAssetType::AT_BODYPART != self->mAssetType) )
	{
		return gAgent.isWearingItem( self->mUUID );
	}
	return FALSE;
}

// static 
void LLWearableBridge::onRemoveFromAvatar(void* user_data)
{
	LLWearableBridge* self = (LLWearableBridge*)user_data;
	if(!self) return;
	if(gAgent.isWearingItem(self->mUUID))
	{
		LLViewerInventoryItem* item = self->getItem();
		if (item)
		{
			gWearableList.getAsset(item->getAssetUUID(),
									item->getName(),
								   item->getType(),
								   onRemoveFromAvatarArrived,
								   new LLUUID(self->mUUID));
		}
	}
}

// static
void LLWearableBridge::onRemoveFromAvatarArrived(LLWearable* wearable,
												 void* userdata)
{
	LLUUID* item_id = (LLUUID*) userdata;
	if(wearable)
	{
		if( gAgent.isWearingItem( *item_id ) )
		{
			EWearableType type = wearable->getType();
	
			if( !(type==WT_SHAPE || type==WT_SKIN || type==WT_HAIR || type==WT_EYES ) ) //&&
				//!((!gAgent.isTeen()) && ( type==WT_UNDERPANTS || type==WT_UNDERSHIRT )) )
			{
				gAgent.removeWearable( type );
			}
		}
	}
	delete item_id;
}
