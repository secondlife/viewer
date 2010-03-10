/**
 * @file llsidepanelinventory.cpp
 * @brief LLPanelObjectInventory class implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
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

//*****************************************************************************
//
// Implementation of the panel inventory - used to view and control a
// task's inventory.
//
//*****************************************************************************

#include "llviewerprecompiledheaders.h"

#include "llpanelobjectinventory.h"

#include "llmenugl.h"
#include "llnotificationsutil.h"
#include "roles_constants.h"

#include "llagent.h"
#include "llcallbacklist.h"
#include "llfloaterbuycurrency.h"
#include "llfloaterreg.h"
#include "llinventorybridge.h"
#include "llinventoryfilter.h"
#include "llinventoryfunctions.h"
#include "llpreviewanim.h"
#include "llpreviewgesture.h"
#include "llpreviewnotecard.h"
#include "llpreviewscript.h"
#include "llpreviewsound.h"
#include "llpreviewtexture.h"
#include "llscrollcontainer.h"
#include "llselectmgr.h"
#include "llsidetray.h"
#include "llstatusbar.h"
#include "lltrans.h"
#include "llviewerassettype.h"
#include "llviewerregion.h"
#include "llviewerobjectlist.h"
#include "llviewermessage.h"


///----------------------------------------------------------------------------
/// Class LLTaskInvFVBridge
///----------------------------------------------------------------------------

class LLTaskInvFVBridge : public LLFolderViewEventListener
{
protected:
	LLUUID mUUID;
	std::string mName;
	mutable std::string mDisplayName;
	LLPanelObjectInventory* mPanel;
	U32 mFlags;

	LLInventoryItem* findItem() const;

public:
	LLTaskInvFVBridge(
		LLPanelObjectInventory* panel,
		const LLUUID& uuid,
		const std::string& name,
		U32 flags=0);
	virtual ~LLTaskInvFVBridge( void ) {}

	virtual LLFontGL::StyleFlags getLabelStyle() const { return LLFontGL::NORMAL; }
	virtual std::string getLabelSuffix() const { return LLStringUtil::null; }

	static LLTaskInvFVBridge* createObjectBridge(LLPanelObjectInventory* panel,
												 LLInventoryObject* object);
	void showProperties();
	void buyItem();
	S32 getPrice();
	static bool commitBuyItem(const LLSD& notification, const LLSD& response);

	// LLFolderViewEventListener functionality
	virtual const std::string& getName() const;
	virtual const std::string& getDisplayName() const;
	virtual PermissionMask getPermissionMask() const { return PERM_NONE; }
	/*virtual*/ LLFolderType::EType getPreferredType() const { return LLFolderType::FT_NONE; }
	virtual const LLUUID& getUUID() const { return mUUID; }
	virtual time_t getCreationDate() const;
	virtual LLUIImagePtr getIcon() const;
	virtual void openItem();
	virtual void closeItem() {}
	virtual void previewItem();
	virtual void selectItem() {}
	virtual BOOL isItemRenameable() const;
	virtual BOOL renameItem(const std::string& new_name);
	virtual BOOL isItemMovable() const;
	virtual BOOL isItemRemovable() const;
	virtual BOOL removeItem();
	virtual void removeBatch(LLDynamicArray<LLFolderViewEventListener*>& batch);
	virtual void move(LLFolderViewEventListener* parent_listener);
	virtual BOOL isItemCopyable() const;
	virtual BOOL copyToClipboard() const;
	virtual void cutToClipboard();
	virtual BOOL isClipboardPasteable() const;
	virtual void pasteFromClipboard();
	virtual void pasteLinkFromClipboard();
	virtual void buildContextMenu(LLMenuGL& menu, U32 flags);
	virtual void performAction(LLFolderView* folder, LLInventoryModel* model, std::string action);
	virtual BOOL isUpToDate() const { return TRUE; }
	virtual BOOL hasChildren() const { return FALSE; }
	virtual LLInventoryType::EType getInventoryType() const { return LLInventoryType::IT_NONE; }
	// LLDragAndDropBridge functionality
	virtual BOOL startDrag(EDragAndDropType* type, LLUUID* id) const;
	virtual BOOL dragOrDrop(MASK mask, BOOL drop,
							EDragAndDropType cargo_type,
							void* cargo_data);
};

LLTaskInvFVBridge::LLTaskInvFVBridge(
	LLPanelObjectInventory* panel,
	const LLUUID& uuid,
	const std::string& name,
	U32 flags):
	mUUID(uuid),
	mName(name),
	mPanel(panel),
	mFlags(flags)
{

}

LLInventoryItem* LLTaskInvFVBridge::findItem() const
{
	LLViewerObject* object = gObjectList.findObject(mPanel->getTaskUUID());
	if(object)
	{
		return dynamic_cast<LLInventoryItem*>(object->getInventoryObject(mUUID));
	}
	return NULL;
}

void LLTaskInvFVBridge::showProperties()
{
	LLSD key;
	key["object"] = mPanel->getTaskUUID();
	key["id"] = mUUID;
	LLSideTray::getInstance()->showPanel("sidepanel_inventory", key);


	// Disable old properties floater; this is replaced by the sidepanel.
	/*
	LLFloaterProperties* floater = LLFloaterReg::showTypedInstance<LLFloaterProperties>("properties", mUUID);
	if (floater)
	{
		floater->setObjectID(mPanel->getTaskUUID());
	}
	*/
}

struct LLBuyInvItemData
{
	LLUUID mTaskID;
	LLUUID mItemID;
	LLAssetType::EType mType;

	LLBuyInvItemData(const LLUUID& task,
					 const LLUUID& item,
					 LLAssetType::EType type) :
		mTaskID(task), mItemID(item), mType(type)
	{}
};

void LLTaskInvFVBridge::buyItem()
{
	llinfos << "LLTaskInvFVBridge::buyItem()" << llendl;
	LLInventoryItem* item = findItem();
	if(!item || !item->getSaleInfo().isForSale()) return;
	LLBuyInvItemData* inv = new LLBuyInvItemData(mPanel->getTaskUUID(),
												 mUUID,
												 item->getType());

	const LLSaleInfo& sale_info = item->getSaleInfo();
	const LLPermissions& perm = item->getPermissions();
	const std::string owner_name; // no owner name currently... FIXME?

	LLViewerObject* obj;
	if( ( obj = gObjectList.findObject( mPanel->getTaskUUID() ) ) && obj->isAttachment() )
	{
		LLNotificationsUtil::add("Cannot_Purchase_an_Attachment");
		llinfos << "Attempt to purchase an attachment" << llendl;
		delete inv;
	}
	else
	{
        LLSD args;
        args["PRICE"] = llformat("%d",sale_info.getSalePrice());
        args["OWNER"] = owner_name;
        if (sale_info.getSaleType() != LLSaleInfo::FS_CONTENTS)
        {
        	U32 next_owner_mask = perm.getMaskNextOwner();
        	args["MODIFYPERM"] = LLTrans::getString((next_owner_mask & PERM_MODIFY) ? "PermYes" : "PermNo");
        	args["COPYPERM"] = LLTrans::getString((next_owner_mask & PERM_COPY) ? "PermYes" : "PermNo");
        	args["RESELLPERM"] = LLTrans::getString((next_owner_mask & PERM_TRANSFER) ? "PermYes" : "PermNo");
        }

		std::string alertdesc;
       	switch(sale_info.getSaleType())
       	{
       	  case LLSaleInfo::FS_ORIGINAL:
       		alertdesc = owner_name.empty() ? "BuyOriginalNoOwner" : "BuyOriginal";
       		break;
       	  case LLSaleInfo::FS_CONTENTS:
       		alertdesc = owner_name.empty() ? "BuyContentsNoOwner" : "BuyContents";
       		break;
		  case LLSaleInfo::FS_COPY:
       	  default:
       		alertdesc = owner_name.empty() ? "BuyCopyNoOwner" : "BuyCopy";
       		break;
       	}

		LLSD payload;
		payload["task_id"] = inv->mTaskID;
		payload["item_id"] = inv->mItemID;
		payload["type"] = inv->mType;
		LLNotificationsUtil::add(alertdesc, args, payload, LLTaskInvFVBridge::commitBuyItem);
	}
}

S32 LLTaskInvFVBridge::getPrice()
{
	LLInventoryItem* item = findItem();
	if(item)
	{
		return item->getSaleInfo().getSalePrice();
	}
	else
	{
		return -1;
	}
}

// static
bool LLTaskInvFVBridge::commitBuyItem(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if(0 == option)
	{
		LLViewerObject* object = gObjectList.findObject(notification["payload"]["task_id"].asUUID());
		if(!object || !object->getRegion()) return false;


		LLMessageSystem* msg = gMessageSystem;
		msg->newMessageFast(_PREHASH_BuyObjectInventory);
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		msg->nextBlockFast(_PREHASH_Data);
		msg->addUUIDFast(_PREHASH_ObjectID, notification["payload"]["task_id"].asUUID());
		msg->addUUIDFast(_PREHASH_ItemID, notification["payload"]["item_id"].asUUID());
		msg->addUUIDFast(_PREHASH_FolderID,
			gInventory.findCategoryUUIDForType((LLFolderType::EType)notification["payload"]["type"].asInteger()));
		msg->sendReliable(object->getRegion()->getHost());
	}
	return false;
}

const std::string& LLTaskInvFVBridge::getName() const
{
	return mName;
}

const std::string& LLTaskInvFVBridge::getDisplayName() const
{
	LLInventoryItem* item = findItem();

	if(item)
	{
		if(item->getParentUUID().isNull())
		{
			if(item->getName() == "Contents")
			{
				mDisplayName.assign(LLTrans::getString("ViewerObjectContents"));
			}
			else
			{
				mDisplayName.assign(item->getName());
			}
		}
		else
		{
			mDisplayName.assign(item->getName());
		}
		const LLPermissions& perm(item->getPermissions());
		BOOL copy = gAgent.allowOperation(PERM_COPY, perm, GP_OBJECT_MANIPULATE);
		BOOL mod  = gAgent.allowOperation(PERM_MODIFY, perm, GP_OBJECT_MANIPULATE);
		BOOL xfer = gAgent.allowOperation(PERM_TRANSFER, perm, GP_OBJECT_MANIPULATE);

		if(!copy)
		{
			mDisplayName.append(LLTrans::getString("no_copy"));
		}
		if(!mod)
		{
			mDisplayName.append(LLTrans::getString("no_modify"));
		}
		if(!xfer)
		{
			mDisplayName.append(LLTrans::getString("no_transfer"));
		}
	}

	return mDisplayName;
}

// BUG: No creation dates for task inventory
time_t LLTaskInvFVBridge::getCreationDate() const
{
	return 0;
}

LLUIImagePtr LLTaskInvFVBridge::getIcon() const
{
	BOOL item_is_multi = FALSE;
	if ( mFlags & LLInventoryItem::II_FLAGS_OBJECT_HAS_MULTIPLE_ITEMS )
	{
		item_is_multi = TRUE;
	}

	return get_item_icon(LLAssetType::AT_OBJECT, LLInventoryType::IT_OBJECT, 0, item_is_multi );
}

void LLTaskInvFVBridge::openItem()
{
	// no-op.
	lldebugs << "LLTaskInvFVBridge::openItem()" << llendl;
}

void LLTaskInvFVBridge::previewItem()
{
	openItem();
}

BOOL LLTaskInvFVBridge::isItemRenameable() const
{
	if(gAgent.isGodlike()) return TRUE;
	LLViewerObject* object = gObjectList.findObject(mPanel->getTaskUUID());
	if(object)
	{
		LLInventoryItem* item;
		item = (LLInventoryItem*)(object->getInventoryObject(mUUID));
		if(item && gAgent.allowOperation(PERM_MODIFY, item->getPermissions(),
										 GP_OBJECT_MANIPULATE, GOD_LIKE))
		{
			return TRUE;
		}
	}
	return FALSE;
}

BOOL LLTaskInvFVBridge::renameItem(const std::string& new_name)
{
	LLViewerObject* object = gObjectList.findObject(mPanel->getTaskUUID());
	if(object)
	{
		LLViewerInventoryItem* item = NULL;
		item = (LLViewerInventoryItem*)object->getInventoryObject(mUUID);
		if(item && (gAgent.allowOperation(PERM_MODIFY, item->getPermissions(),
										GP_OBJECT_MANIPULATE, GOD_LIKE)))
		{
			LLPointer<LLViewerInventoryItem> new_item = new LLViewerInventoryItem(item);
			new_item->rename(new_name);
			object->updateInventory(
				new_item,
				TASK_INVENTORY_ITEM_KEY,
				false);
		}
	}
	return TRUE;
}

BOOL LLTaskInvFVBridge::isItemMovable() const
{
	//LLViewerObject* object = gObjectList.findObject(mPanel->getTaskUUID());
	//if(object && (object->permModify() || gAgent.isGodlike()))
	//{
	//	return TRUE;
	//}
	//return FALSE;
	return TRUE;
}

BOOL LLTaskInvFVBridge::isItemRemovable() const
{
	const LLViewerObject* object = gObjectList.findObject(mPanel->getTaskUUID());
	if(object
	   && (object->permModify() || object->permYouOwner()))
	{
		return TRUE;
	}
	return FALSE;
}

bool remove_task_inventory_callback(const LLSD& notification, const LLSD& response, LLPanelObjectInventory* panel)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	LLViewerObject* object = gObjectList.findObject(notification["payload"]["task_id"].asUUID());
	if(option == 0 && object)
	{
		// yes
		LLSD::array_const_iterator list_end = notification["payload"]["inventory_ids"].endArray();
		for (LLSD::array_const_iterator list_it = notification["payload"]["inventory_ids"].beginArray();
			list_it != list_end; 
			++list_it)
		{
			object->removeInventory(list_it->asUUID());
		}

		// refresh the UI.
		panel->refresh();
	}
	return false;
}

// helper for remove
// ! REFACTOR ! two_uuids_list_t is also defined in llinventorybridge.h, but differently.
typedef std::pair<LLUUID, std::list<LLUUID> > panel_two_uuids_list_t;
typedef std::pair<LLPanelObjectInventory*, panel_two_uuids_list_t> remove_data_t;
BOOL LLTaskInvFVBridge::removeItem()
{
	if(isItemRemovable() && mPanel)
	{
		LLViewerObject* object = gObjectList.findObject(mPanel->getTaskUUID());
		if(object)
		{
			if(object->permModify())
			{
				// just do it.
				object->removeInventory(mUUID);
				return TRUE;
			}
			else
			{
				LLSD payload;
				payload["task_id"] = mPanel->getTaskUUID();
				payload["inventory_ids"].append(mUUID);
				LLNotificationsUtil::add("RemoveItemWarn", LLSD(), payload, boost::bind(&remove_task_inventory_callback, _1, _2, mPanel));
				return FALSE;
			}
		}
	}
	return FALSE;
}

void LLTaskInvFVBridge::removeBatch(LLDynamicArray<LLFolderViewEventListener*>& batch)
{
	if (!mPanel)
	{
		return;
	}

	LLViewerObject* object = gObjectList.findObject(mPanel->getTaskUUID());
	if (!object)
	{
		return;
	}

	if (!object->permModify())
	{
		LLSD payload;
		payload["task_id"] = mPanel->getTaskUUID();
		for (S32 i = 0; i < (S32)batch.size(); i++)
		{
			LLTaskInvFVBridge* itemp = (LLTaskInvFVBridge*)batch[i];
			payload["inventory_ids"].append(itemp->getUUID());
		}
		LLNotificationsUtil::add("RemoveItemWarn", LLSD(), payload, boost::bind(&remove_task_inventory_callback, _1, _2, mPanel));
		
	}
	else
	{
		for (S32 i = 0; i < (S32)batch.size(); i++)
		{
			LLTaskInvFVBridge* itemp = (LLTaskInvFVBridge*)batch[i];

			if(itemp->isItemRemovable())
			{
				// just do it.
				object->removeInventory(itemp->getUUID());
			}
		}
	}
}

void LLTaskInvFVBridge::move(LLFolderViewEventListener* parent_listener)
{
}

BOOL LLTaskInvFVBridge::isItemCopyable() const
{
	LLInventoryItem* item = findItem();
	if(!item) return FALSE;
	return gAgent.allowOperation(PERM_COPY, item->getPermissions(),
								GP_OBJECT_MANIPULATE);
}

BOOL LLTaskInvFVBridge::copyToClipboard() const
{
	return FALSE;
}

void LLTaskInvFVBridge::cutToClipboard()
{
}

BOOL LLTaskInvFVBridge::isClipboardPasteable() const
{
	return FALSE;
}

void LLTaskInvFVBridge::pasteFromClipboard()
{
}

void LLTaskInvFVBridge::pasteLinkFromClipboard()
{
}

BOOL LLTaskInvFVBridge::startDrag(EDragAndDropType* type, LLUUID* id) const
{
	//llinfos << "LLTaskInvFVBridge::startDrag()" << llendl;
	if(mPanel)
	{
		LLViewerObject* object = gObjectList.findObject(mPanel->getTaskUUID());
		if(object)
		{
			LLInventoryItem* inv = NULL;
			if((inv = (LLInventoryItem*)object->getInventoryObject(mUUID)))
			{
				const LLPermissions& perm = inv->getPermissions();
				bool can_copy = gAgent.allowOperation(PERM_COPY, perm,
														GP_OBJECT_MANIPULATE);
				if (object->isAttachment() && !can_copy)
				{
                    //RN: no copy contents of attachments cannot be dragged out
                    // due to a race condition and possible exploit where
                    // attached objects do not update their inventory items
                    // when their contents are manipulated
                    return FALSE;
				}
				if((can_copy && perm.allowTransferTo(gAgent.getID()))
				   || object->permYouOwner())
//				   || gAgent.isGodlike())

				{
					*type = LLViewerAssetType::lookupDragAndDropType(inv->getType());

					*id = inv->getUUID();
					return TRUE;
				}
			}
		}
	}
	return FALSE;
}

BOOL LLTaskInvFVBridge::dragOrDrop(MASK mask, BOOL drop,
								   EDragAndDropType cargo_type,
								   void* cargo_data)
{
	//llinfos << "LLTaskInvFVBridge::dragOrDrop()" << llendl;
	return FALSE;
}

// virtual
void LLTaskInvFVBridge::performAction(LLFolderView* folder, LLInventoryModel* model, std::string action)
{
	if (action == "task_buy")
	{
		// Check the price of the item.
		S32 price = getPrice();
		if (-1 == price)
		{
			llwarns << "label_buy_task_bridged_item: Invalid price" << llendl;
		}
		else
		{
			if (price > 0 && price > gStatusBar->getBalance())
			{
				LLFloaterBuyCurrency::buyCurrency("This costs", price);
			}
			else
			{
				buyItem();
			}
		}
	}
	else if (action == "task_open")
	{
		openItem();
	}
	else if (action == "task_properties")
	{
		showProperties();
	}
}

void LLTaskInvFVBridge::buildContextMenu(LLMenuGL& menu, U32 flags)
{
	LLInventoryItem* item = findItem();
	std::vector<std::string> items;
	std::vector<std::string> disabled_items;
	
	if (!item)
	{
		hide_context_entries(menu, items, disabled_items);
		return;
	}

	if(gAgent.allowOperation(PERM_OWNER, item->getPermissions(),
							 GP_OBJECT_MANIPULATE)
	   && item->getSaleInfo().isForSale())
	{
		items.push_back(std::string("Task Buy"));

		std::string label= LLTrans::getString("Buy");
		// Check the price of the item.
		S32 price = getPrice();
		if (-1 == price)
		{
			llwarns << "label_buy_task_bridged_item: Invalid price" << llendl;
		}
		else
		{
			std::ostringstream info;
			info << LLTrans::getString("BuyforL$") << price;
			label.assign(info.str());
		}

		const LLView::child_list_t *list = menu.getChildList();
		LLView::child_list_t::const_iterator itor;
		for (itor = list->begin(); itor != list->end(); ++itor)
		{
			std::string name = (*itor)->getName();
			LLMenuItemCallGL* menu_itemp = dynamic_cast<LLMenuItemCallGL*>(*itor);
			if (name == "Task Buy" && menu_itemp)
			{
				menu_itemp->setLabel(label);
			}
		}
	}
	else
	{
		items.push_back(std::string("Task Open"));
		if (!isItemCopyable())
		{
			disabled_items.push_back(std::string("Task Open"));
		}
	}
	items.push_back(std::string("Task Properties"));
	if(isItemRenameable())
	{
		items.push_back(std::string("Task Rename"));
	}
	if(isItemRemovable())
	{
		items.push_back(std::string("Task Remove"));
	}

	hide_context_entries(menu, items, disabled_items);
}


///----------------------------------------------------------------------------
/// Class LLTaskFolderBridge
///----------------------------------------------------------------------------

class LLTaskCategoryBridge : public LLTaskInvFVBridge
{
public:
	LLTaskCategoryBridge(
		LLPanelObjectInventory* panel,
		const LLUUID& uuid,
		const std::string& name);

	virtual LLUIImagePtr getIcon() const;
	virtual const std::string& getDisplayName() const { return getName(); }
	virtual BOOL isItemRenameable() const;
	// virtual BOOL isItemCopyable() const { return FALSE; }
	virtual BOOL renameItem(const std::string& new_name);
	virtual BOOL isItemRemovable() const;
	virtual void buildContextMenu(LLMenuGL& menu, U32 flags);
	virtual BOOL hasChildren() const;
	virtual BOOL startDrag(EDragAndDropType* type, LLUUID* id) const;
	virtual BOOL dragOrDrop(MASK mask, BOOL drop,
							EDragAndDropType cargo_type,
							void* cargo_data);
};

LLTaskCategoryBridge::LLTaskCategoryBridge(
	LLPanelObjectInventory* panel,
	const LLUUID& uuid,
	const std::string& name) :
	LLTaskInvFVBridge(panel, uuid, name)
{
}

LLUIImagePtr LLTaskCategoryBridge::getIcon() const
{
	return LLUI::getUIImage("Inv_FolderClosed");
}

BOOL LLTaskCategoryBridge::isItemRenameable() const
{
	return FALSE;
}

BOOL LLTaskCategoryBridge::renameItem(const std::string& new_name)
{
	return FALSE;
}

BOOL LLTaskCategoryBridge::isItemRemovable() const
{
	return FALSE;
}

void LLTaskCategoryBridge::buildContextMenu(LLMenuGL& menu, U32 flags)
{
	std::vector<std::string> items;
	std::vector<std::string> disabled_items;
	items.push_back(std::string("Task Open"));
	hide_context_entries(menu, items, disabled_items);
}

BOOL LLTaskCategoryBridge::hasChildren() const
{
	// return TRUE if we have or do know know if we have children.
	// *FIX: For now, return FALSE - we will know for sure soon enough.
	return FALSE;
}

BOOL LLTaskCategoryBridge::startDrag(EDragAndDropType* type, LLUUID* id) const
{
	//llinfos << "LLTaskInvFVBridge::startDrag()" << llendl;
	if(mPanel)
	{
		LLViewerObject* object = gObjectList.findObject(mPanel->getTaskUUID());
		if(object)
		{
			const LLInventoryItem *inv = dynamic_cast<LLInventoryItem*>(object->getInventoryObject(mUUID));
			if (inv)
			{
				const LLPermissions& perm = inv->getPermissions();
				bool can_copy = gAgent.allowOperation(PERM_COPY, perm,
														GP_OBJECT_MANIPULATE);
				if((can_copy && perm.allowTransferTo(gAgent.getID()))
				   || object->permYouOwner())
//				   || gAgent.isGodlike())

				{
					*type = LLViewerAssetType::lookupDragAndDropType(inv->getType());

					*id = inv->getUUID();
					return TRUE;
				}
			}
		}
	}
	return FALSE;
}

BOOL LLTaskCategoryBridge::dragOrDrop(MASK mask, BOOL drop,
									  EDragAndDropType cargo_type,
									  void* cargo_data)
{
	//llinfos << "LLTaskCategoryBridge::dragOrDrop()" << llendl;
	BOOL accept = FALSE;
	LLViewerObject* object = gObjectList.findObject(mPanel->getTaskUUID());
	if(object)
	{
		switch(cargo_type)
		{
		case DAD_CATEGORY:
			accept = LLToolDragAndDrop::getInstance()->dadUpdateInventoryCategory(object,drop);
			break;
		case DAD_TEXTURE:
		case DAD_SOUND:
		case DAD_LANDMARK:
		case DAD_OBJECT:
		case DAD_NOTECARD:
		case DAD_CLOTHING:
		case DAD_BODYPART:
		case DAD_ANIMATION:
		case DAD_GESTURE:
		case DAD_CALLINGCARD:
			accept = LLToolDragAndDrop::isInventoryDropAcceptable(object, (LLViewerInventoryItem*)cargo_data);
			if(accept && drop)
			{
				LLToolDragAndDrop::dropInventory(object,
												 (LLViewerInventoryItem*)cargo_data,
												 LLToolDragAndDrop::getInstance()->getSource(),
												 LLToolDragAndDrop::getInstance()->getSourceID());
			}
			break;
		case DAD_SCRIPT:
			// *HACK: In order to resolve SL-22177, we need to block
			// drags from notecards and objects onto other
			// objects. uncomment the simpler version when we have
			// that right.
			//accept = LLToolDragAndDrop::isInventoryDropAcceptable(object, (LLViewerInventoryItem*)cargo_data);
			if(LLToolDragAndDrop::isInventoryDropAcceptable(
				   object, (LLViewerInventoryItem*)cargo_data)
			   && (LLToolDragAndDrop::SOURCE_WORLD != LLToolDragAndDrop::getInstance()->getSource())
			   && (LLToolDragAndDrop::SOURCE_NOTECARD != LLToolDragAndDrop::getInstance()->getSource()))
			{
				accept = TRUE;
			}
			if(accept && drop)
			{
				LLViewerInventoryItem* item = (LLViewerInventoryItem*)cargo_data;
				// rez in the script active by default, rez in
				// inactive if the control key is being held down.
				BOOL active = ((mask & MASK_CONTROL) == 0);
				LLToolDragAndDrop::dropScript(object, item, active,
											  LLToolDragAndDrop::getInstance()->getSource(),
											  LLToolDragAndDrop::getInstance()->getSourceID());
			}
			break;
		default:
			break;
		}
	}
	return accept;
}

///----------------------------------------------------------------------------
/// Class LLTaskTextureBridge
///----------------------------------------------------------------------------

class LLTaskTextureBridge : public LLTaskInvFVBridge
{
public:
	LLTaskTextureBridge(
		LLPanelObjectInventory* panel,
		const LLUUID& uuid,
		const std::string& name,
		LLInventoryType::EType it);

	virtual LLUIImagePtr getIcon() const;
	virtual void openItem();
protected:
	LLInventoryType::EType mInventoryType;
};

LLTaskTextureBridge::LLTaskTextureBridge(
	LLPanelObjectInventory* panel,
	const LLUUID& uuid,
	const std::string& name,
	LLInventoryType::EType it) :
	LLTaskInvFVBridge(panel, uuid, name),
	mInventoryType(it)
{
}

LLUIImagePtr LLTaskTextureBridge::getIcon() const
{
	return get_item_icon(LLAssetType::AT_TEXTURE, mInventoryType, 0, FALSE);
}

void LLTaskTextureBridge::openItem()
{
	llinfos << "LLTaskTextureBridge::openItem()" << llendl;
	LLPreviewTexture* preview = LLFloaterReg::showTypedInstance<LLPreviewTexture>("preview_texture", LLSD(mUUID), TAKE_FOCUS_YES);
	if(preview)
	{
		preview->setObjectID(mPanel->getTaskUUID());
	}
}


///----------------------------------------------------------------------------
/// Class LLTaskSoundBridge
///----------------------------------------------------------------------------

class LLTaskSoundBridge : public LLTaskInvFVBridge
{
public:
	LLTaskSoundBridge(
		LLPanelObjectInventory* panel,
		const LLUUID& uuid,
		const std::string& name);

	virtual LLUIImagePtr getIcon() const;
	virtual void openItem();
	virtual void performAction(LLFolderView* folder, LLInventoryModel* model, std::string action);
	virtual void buildContextMenu(LLMenuGL& menu, U32 flags);
	static void openSoundPreview(void* data);
};

LLTaskSoundBridge::LLTaskSoundBridge(
	LLPanelObjectInventory* panel,
	const LLUUID& uuid,
	const std::string& name) :
	LLTaskInvFVBridge(panel, uuid, name)
{
}

LLUIImagePtr LLTaskSoundBridge::getIcon() const
{
	return get_item_icon(LLAssetType::AT_SOUND, LLInventoryType::IT_SOUND, 0, FALSE);
}

void LLTaskSoundBridge::openItem()
{
	openSoundPreview((void*)this);
}

void LLTaskSoundBridge::openSoundPreview(void* data)
{
	LLTaskSoundBridge* self = (LLTaskSoundBridge*)data;
	if(!self)
		return;

	LLPreviewSound* preview = LLFloaterReg::showTypedInstance<LLPreviewSound>("preview_sound", LLSD(self->mUUID), TAKE_FOCUS_YES);
	if (preview)
	{
		preview->setObjectID(self->mPanel->getTaskUUID());
	}
}

// virtual
void LLTaskSoundBridge::performAction(LLFolderView* folder, LLInventoryModel* model, std::string action)
{
	if (action == "task_play")
	{
		LLInventoryItem* item = findItem();
		if(item)
		{
			send_sound_trigger(item->getAssetUUID(), 1.0);
		}
	}
	LLTaskInvFVBridge::performAction(folder, model, action);
}

void LLTaskSoundBridge::buildContextMenu(LLMenuGL& menu, U32 flags)
{
	LLInventoryItem* item = findItem();
	if(!item) return;
	std::vector<std::string> items;
	std::vector<std::string> disabled_items;

	if(item->getPermissions().getOwner() != gAgent.getID()
	   && item->getSaleInfo().isForSale())
	{
		items.push_back(std::string("Task Buy"));

		std::string label= LLTrans::getString("Buy");
		// Check the price of the item.
		S32 price = getPrice();
		if (-1 == price)
		{
			llwarns << "label_buy_task_bridged_item: Invalid price" << llendl;
		}
		else
		{
			std::ostringstream info;
			info <<  LLTrans::getString("BuyforL$") << price;
			label.assign(info.str());
		}

		const LLView::child_list_t *list = menu.getChildList();
		LLView::child_list_t::const_iterator itor;
		for (itor = list->begin(); itor != list->end(); ++itor)
		{
			std::string name = (*itor)->getName();
			LLMenuItemCallGL* menu_itemp = dynamic_cast<LLMenuItemCallGL*>(*itor);
			if (name == "Task Buy" && menu_itemp)
			{
				menu_itemp->setLabel(label);
			}
		}
	}
	else
	{
		items.push_back(std::string("Task Open")); 
		if (!isItemCopyable())
		{
			disabled_items.push_back(std::string("Task Open"));
		}
	}
	items.push_back(std::string("Task Properties"));
	if(isItemRenameable())
	{
		items.push_back(std::string("Task Rename"));
	}
	if(isItemRemovable())
	{
		items.push_back(std::string("Task Remove"));
	}

	items.push_back(std::string("Task Play"));


	hide_context_entries(menu, items, disabled_items);
}

///----------------------------------------------------------------------------
/// Class LLTaskLandmarkBridge
///----------------------------------------------------------------------------

class LLTaskLandmarkBridge : public LLTaskInvFVBridge
{
public:
	LLTaskLandmarkBridge(
		LLPanelObjectInventory* panel,
		const LLUUID& uuid,
		const std::string& name);

	virtual LLUIImagePtr getIcon() const;
};

LLTaskLandmarkBridge::LLTaskLandmarkBridge(
	LLPanelObjectInventory* panel,
	const LLUUID& uuid,
	const std::string& name) :
	LLTaskInvFVBridge(panel, uuid, name)
{
}

LLUIImagePtr LLTaskLandmarkBridge::getIcon() const
{
	return get_item_icon(LLAssetType::AT_LANDMARK, LLInventoryType::IT_LANDMARK, 0, FALSE);
}


///----------------------------------------------------------------------------
/// Class LLTaskCallingCardBridge
///----------------------------------------------------------------------------

class LLTaskCallingCardBridge : public LLTaskInvFVBridge
{
public:
	LLTaskCallingCardBridge(
		LLPanelObjectInventory* panel,
		const LLUUID& uuid,
		const std::string& name);

	virtual LLUIImagePtr getIcon() const;
	virtual BOOL isItemRenameable() const;
	virtual BOOL renameItem(const std::string& new_name);
};

LLTaskCallingCardBridge::LLTaskCallingCardBridge(
	LLPanelObjectInventory* panel,
	const LLUUID& uuid,
	const std::string& name) :
	LLTaskInvFVBridge(panel, uuid, name)
{
}

LLUIImagePtr LLTaskCallingCardBridge::getIcon() const
{
	return get_item_icon(LLAssetType::AT_CALLINGCARD, LLInventoryType::IT_CALLINGCARD, 0, FALSE);
}

BOOL LLTaskCallingCardBridge::isItemRenameable() const
{
	return FALSE;
}

BOOL LLTaskCallingCardBridge::renameItem(const std::string& new_name)
{
	return FALSE;
}


///----------------------------------------------------------------------------
/// Class LLTaskScriptBridge
///----------------------------------------------------------------------------

class LLTaskScriptBridge : public LLTaskInvFVBridge
{
public:
	LLTaskScriptBridge(
		LLPanelObjectInventory* panel,
		const LLUUID& uuid,
		const std::string& name);

	virtual LLUIImagePtr getIcon() const;
	//static BOOL enableIfCopyable( void* userdata );
};

LLTaskScriptBridge::LLTaskScriptBridge(
	LLPanelObjectInventory* panel,
	const LLUUID& uuid,
	const std::string& name) :
	LLTaskInvFVBridge(panel, uuid, name)
{
}

LLUIImagePtr LLTaskScriptBridge::getIcon() const
{
	return get_item_icon(LLAssetType::AT_SCRIPT, LLInventoryType::IT_LSL, 0, FALSE);
}


class LLTaskLSLBridge : public LLTaskScriptBridge
{
public:
	LLTaskLSLBridge(
		LLPanelObjectInventory* panel,
		const LLUUID& uuid,
		const std::string& name);

	virtual void openItem();
	virtual BOOL removeItem();
	//virtual void buildContextMenu(LLMenuGL& menu);

	//static void copyToInventory(void* userdata);
};

LLTaskLSLBridge::LLTaskLSLBridge(
	LLPanelObjectInventory* panel,
	const LLUUID& uuid,
	const std::string& name) :
	LLTaskScriptBridge(panel, uuid, name)
{
}

void LLTaskLSLBridge::openItem()
{
	llinfos << "LLTaskLSLBridge::openItem() " << mUUID << llendl;
	LLViewerObject* object = gObjectList.findObject(mPanel->getTaskUUID());
	if(!object || object->isInventoryPending())
	{
		return;
	}
	if (object->permModify() || gAgent.isGodlike())
	{
		LLLiveLSLEditor* preview = LLFloaterReg::showTypedInstance<LLLiveLSLEditor>("preview_scriptedit", LLSD(mUUID), TAKE_FOCUS_YES);
		if (preview)
		{
			preview->setObjectID(mPanel->getTaskUUID());
		}
	}
	else
	{	
		LLNotificationsUtil::add("CannotOpenScriptObjectNoMod");
	}
}

BOOL LLTaskLSLBridge::removeItem()
{
	LLFloaterReg::hideInstance("preview_scriptedit", LLSD(mUUID));
	return LLTaskInvFVBridge::removeItem();
}

///----------------------------------------------------------------------------
/// Class LLTaskObjectBridge
///----------------------------------------------------------------------------

class LLTaskObjectBridge : public LLTaskInvFVBridge
{
public:
	LLTaskObjectBridge(
		LLPanelObjectInventory* panel,
		const LLUUID& uuid,
		const std::string& name);

	virtual LLUIImagePtr getIcon() const;
};

LLTaskObjectBridge::LLTaskObjectBridge(
	LLPanelObjectInventory* panel,
	const LLUUID& uuid,
	const std::string& name) :
	LLTaskInvFVBridge(panel, uuid, name)
{
}

LLUIImagePtr LLTaskObjectBridge::getIcon() const
{
	BOOL item_is_multi = FALSE;
	if ( mFlags & LLInventoryItem::II_FLAGS_OBJECT_HAS_MULTIPLE_ITEMS )
	{
		item_is_multi = TRUE;
	}

	return get_item_icon(LLAssetType::AT_OBJECT, LLInventoryType::IT_OBJECT, 0, item_is_multi);
}

///----------------------------------------------------------------------------
/// Class LLTaskNotecardBridge
///----------------------------------------------------------------------------

class LLTaskNotecardBridge : public LLTaskInvFVBridge
{
public:
	LLTaskNotecardBridge(
		LLPanelObjectInventory* panel,
		const LLUUID& uuid,
		const std::string& name);

	virtual LLUIImagePtr getIcon() const;
	virtual void openItem();
	virtual BOOL removeItem();
};

LLTaskNotecardBridge::LLTaskNotecardBridge(
	LLPanelObjectInventory* panel,
	const LLUUID& uuid,
	const std::string& name) :
	LLTaskInvFVBridge(panel, uuid, name)
{
}

LLUIImagePtr LLTaskNotecardBridge::getIcon() const
{
	return get_item_icon(LLAssetType::AT_NOTECARD, LLInventoryType::IT_NOTECARD, 0, FALSE);
}

void LLTaskNotecardBridge::openItem()
{
	LLViewerObject* object = gObjectList.findObject(mPanel->getTaskUUID());
	if(!object || object->isInventoryPending())
	{
		return;
	}
	if(object->permModify() || gAgent.isGodlike())
	{
		LLPreviewNotecard* preview = LLFloaterReg::showTypedInstance<LLPreviewNotecard>("preview_notecard", LLSD(mUUID), TAKE_FOCUS_YES);
		if (preview)
		{
			preview->setObjectID(mPanel->getTaskUUID());
		}
	}
}

BOOL LLTaskNotecardBridge::removeItem()
{
	LLFloaterReg::hideInstance("preview_notecard", LLSD(mUUID));
	return LLTaskInvFVBridge::removeItem();
}

///----------------------------------------------------------------------------
/// Class LLTaskGestureBridge
///----------------------------------------------------------------------------

class LLTaskGestureBridge : public LLTaskInvFVBridge
{
public:
	LLTaskGestureBridge(
		LLPanelObjectInventory* panel,
		const LLUUID& uuid,
		const std::string& name);

	virtual LLUIImagePtr getIcon() const;
	virtual void openItem();
	virtual BOOL removeItem();
};

LLTaskGestureBridge::LLTaskGestureBridge(
	LLPanelObjectInventory* panel,
	const LLUUID& uuid,
	const std::string& name) :
	LLTaskInvFVBridge(panel, uuid, name)
{
}

LLUIImagePtr LLTaskGestureBridge::getIcon() const
{
	return get_item_icon(LLAssetType::AT_GESTURE, LLInventoryType::IT_GESTURE, 0, FALSE);
}

void LLTaskGestureBridge::openItem()
{
	LLViewerObject* object = gObjectList.findObject(mPanel->getTaskUUID());
	if(!object || object->isInventoryPending())
	{
		return;
	}
	LLPreviewGesture::show(mUUID, mPanel->getTaskUUID());
}

BOOL LLTaskGestureBridge::removeItem()
{
	// Don't need to deactivate gesture because gestures inside objects can never be active.
	LLFloaterReg::hideInstance("preview_gesture", LLSD(mUUID));
	return LLTaskInvFVBridge::removeItem();
}

///----------------------------------------------------------------------------
/// Class LLTaskAnimationBridge
///----------------------------------------------------------------------------

class LLTaskAnimationBridge : public LLTaskInvFVBridge
{
public:
	LLTaskAnimationBridge(
		LLPanelObjectInventory* panel,
		const LLUUID& uuid,
		const std::string& name);

	virtual LLUIImagePtr getIcon() const;
	virtual void openItem();
	virtual BOOL removeItem();
};

LLTaskAnimationBridge::LLTaskAnimationBridge(
	LLPanelObjectInventory* panel,
	const LLUUID& uuid,
	const std::string& name) :
	LLTaskInvFVBridge(panel, uuid, name)
{
}

LLUIImagePtr LLTaskAnimationBridge::getIcon() const
{
	return get_item_icon(LLAssetType::AT_ANIMATION, LLInventoryType::IT_ANIMATION, 0, FALSE);
}

void LLTaskAnimationBridge::openItem()
{
	LLViewerObject* object = gObjectList.findObject(mPanel->getTaskUUID());
	if(!object || object->isInventoryPending())
	{
		return;
	}

	LLPreviewAnim* preview = LLFloaterReg::showTypedInstance<LLPreviewAnim>("preview_anim", LLSD(mUUID), TAKE_FOCUS_YES);
	if (preview && (object->permModify() || gAgent.isGodlike()))
	{
		preview->setObjectID(mPanel->getTaskUUID());
	}
}

BOOL LLTaskAnimationBridge::removeItem()
{
	LLFloaterReg::hideInstance("preview_anim", LLSD(mUUID));
	return LLTaskInvFVBridge::removeItem();
}

///----------------------------------------------------------------------------
/// Class LLTaskWearableBridge
///----------------------------------------------------------------------------

class LLTaskWearableBridge : public LLTaskInvFVBridge
{
public:
	LLTaskWearableBridge(
		LLPanelObjectInventory* panel,
		const LLUUID& uuid,
		const std::string& name,
		LLAssetType::EType asset_type,
		U32 flags);

	virtual LLUIImagePtr getIcon() const;

protected:
	LLAssetType::EType		mAssetType;
};

LLTaskWearableBridge::LLTaskWearableBridge(
	LLPanelObjectInventory* panel,
	const LLUUID& uuid,
	const std::string& name,
	LLAssetType::EType asset_type,
	U32 flags) :
	LLTaskInvFVBridge(panel, uuid, name, flags),
	mAssetType( asset_type )
{
}

LLUIImagePtr LLTaskWearableBridge::getIcon() const
{
	return get_item_icon(mAssetType, LLInventoryType::IT_WEARABLE, mFlags, FALSE );
}


///----------------------------------------------------------------------------
/// LLTaskInvFVBridge impl
//----------------------------------------------------------------------------

LLTaskInvFVBridge* LLTaskInvFVBridge::createObjectBridge(LLPanelObjectInventory* panel,
														 LLInventoryObject* object)
{
	LLTaskInvFVBridge* new_bridge = NULL;
	LLAssetType::EType type = object->getType();
	LLInventoryItem* item = NULL;
	switch(type)
	{
	case LLAssetType::AT_TEXTURE:
		item = (LLInventoryItem*)object;
		new_bridge = new LLTaskTextureBridge(panel,
											 object->getUUID(),
											 object->getName(),
											 item->getInventoryType());
		break;
	case LLAssetType::AT_SOUND:
		new_bridge = new LLTaskSoundBridge(panel,
										   object->getUUID(),
										   object->getName());
		break;
	case LLAssetType::AT_LANDMARK:
		new_bridge = new LLTaskLandmarkBridge(panel,
											  object->getUUID(),
											  object->getName());
		break;
	case LLAssetType::AT_CALLINGCARD:
		new_bridge = new LLTaskCallingCardBridge(panel,
												 object->getUUID(),
												 object->getName());
		break;
	case LLAssetType::AT_SCRIPT:
		// OLD SCRIPTS DEPRECATED - JC
		llwarns << "Old script" << llendl;
		//new_bridge = new LLTaskOldScriptBridge(panel,
		//									   object->getUUID(),
		//									   object->getName());
		break;
	case LLAssetType::AT_OBJECT:
		new_bridge = new LLTaskObjectBridge(panel,
											object->getUUID(),
											object->getName());
		break;
	case LLAssetType::AT_NOTECARD:
		new_bridge = new LLTaskNotecardBridge(panel,
											  object->getUUID(),
											  object->getName());
		break;
	case LLAssetType::AT_ANIMATION:
		new_bridge = new LLTaskAnimationBridge(panel,
											  object->getUUID(),
											  object->getName());
		break;
	case LLAssetType::AT_GESTURE:
		new_bridge = new LLTaskGestureBridge(panel,
											  object->getUUID(),
											  object->getName());
		break;
	case LLAssetType::AT_CLOTHING:
	case LLAssetType::AT_BODYPART:
		item = (LLInventoryItem*)object;
		new_bridge = new LLTaskWearableBridge(panel,
											  object->getUUID(),
											  object->getName(),
											  type,
											  item->getFlags());
		break;
	case LLAssetType::AT_CATEGORY:
		new_bridge = new LLTaskCategoryBridge(panel,
											  object->getUUID(),
											  object->getName());
		break;
	case LLAssetType::AT_LSL_TEXT:
		new_bridge = new LLTaskLSLBridge(panel,
										 object->getUUID(),
										 object->getName());
		break;
	
		break;
	default:
		llinfos << "Unhandled inventory type (llassetstorage.h): "
				<< (S32)type << llendl;
		break;
	}
	return new_bridge;
}


///----------------------------------------------------------------------------
/// Class LLPanelObjectInventory
///----------------------------------------------------------------------------

static LLDefaultChildRegistry::Register<LLPanelObjectInventory> r("panel_inventory_object");

void do_nothing()
{
}

// Default constructor
LLPanelObjectInventory::LLPanelObjectInventory(const LLPanelObjectInventory::Params& p) :
	LLPanel(p),
	mScroller(NULL),
	mFolders(NULL),
	mHaveInventory(FALSE),
	mIsInventoryEmpty(TRUE),
	mInventoryNeedsUpdate(FALSE)
{
	// Setup context menu callbacks
	mCommitCallbackRegistrar.add("Inventory.DoToSelected", boost::bind(&LLPanelObjectInventory::doToSelected, this, _2));
	mCommitCallbackRegistrar.add("Inventory.EmptyTrash", boost::bind(&LLInventoryModel::emptyFolderType, &gInventory, "ConfirmEmptyTrash", LLFolderType::FT_TRASH));
	mCommitCallbackRegistrar.add("Inventory.EmptyLostAndFound", boost::bind(&LLInventoryModel::emptyFolderType, &gInventory, "ConfirmEmptyLostAndFound", LLFolderType::FT_LOST_AND_FOUND));
	mCommitCallbackRegistrar.add("Inventory.DoCreate", boost::bind(&do_nothing));
	mCommitCallbackRegistrar.add("Inventory.AttachObject", boost::bind(&do_nothing));
	mCommitCallbackRegistrar.add("Inventory.BeginIMSession", boost::bind(&do_nothing));
}

// Destroys the object
LLPanelObjectInventory::~LLPanelObjectInventory()
{
	if (!gIdleCallbacks.deleteFunction(idle, this))
	{
		llwarns << "LLPanelObjectInventory::~LLPanelObjectInventory() failed to delete callback" << llendl;
	}
}

BOOL LLPanelObjectInventory::postBuild()
{
	// clear contents and initialize menus, sets up mFolders
	reset();

	// Register an idle update callback
	gIdleCallbacks.addFunction(idle, this);

	return TRUE;
}

void LLPanelObjectInventory::doToSelected(const LLSD& userdata)
{
	mFolders->doToSelected(&gInventory, userdata);
}

void LLPanelObjectInventory::clearContents()
{
	mHaveInventory = FALSE;
	mIsInventoryEmpty = TRUE;
	if (LLToolDragAndDrop::getInstance() && LLToolDragAndDrop::getInstance()->getSource() == LLToolDragAndDrop::SOURCE_WORLD)
	{
		LLToolDragAndDrop::getInstance()->endDrag();
	}

	if( mScroller )
	{
		// removes mFolders
		removeChild( mScroller ); //*TODO: Really shouldn't do this during draw()/refresh()
		mScroller->die();
		mScroller = NULL;
		mFolders = NULL;
	}
}


void LLPanelObjectInventory::reset()
{
	clearContents();

	//setBorderVisible(FALSE);
	
	mCommitCallbackRegistrar.pushScope(); // push local callbacks
	
	LLRect dummy_rect(0, 1, 1, 0);
	LLFolderView::Params p;
	p.name = "task inventory";
	p.task_id = getTaskUUID();
	p.parent_panel = this;
	p.tool_tip= p.name;
	mFolders = LLUICtrlFactory::create<LLFolderView>(p);
	// this ensures that we never say "searching..." or "no items found"
	mFolders->getFilter()->setShowFolderState(LLInventoryFilter::SHOW_ALL_FOLDERS);
	mFolders->setCallbackRegistrar(&mCommitCallbackRegistrar);

	if (hasFocus())
	{
		LLEditMenuHandler::gEditMenuHandler = mFolders;
	}

	LLRect scroller_rect(0, getRect().getHeight(), getRect().getWidth(), 0);
	LLScrollContainer::Params scroll_p;
	scroll_p.name("task inventory scroller");
	scroll_p.rect(scroller_rect);
	scroll_p.tab_stop(true);
	scroll_p.follows.flags(FOLLOWS_ALL);
	mScroller = LLUICtrlFactory::create<LLScrollContainer>(scroll_p);
	addChild(mScroller);
	mScroller->addChild(mFolders);
	
	mFolders->setScrollContainer( mScroller );
	
	mCommitCallbackRegistrar.popScope();
}

void LLPanelObjectInventory::inventoryChanged(LLViewerObject* object,
										InventoryObjectList* inventory,
										S32 serial_num,
										void* data)
{
	if(!object) return;

	//llinfos << "invetnory arrived: \n"
	//		<< " panel UUID: " << panel->mTaskUUID << "\n"
	//		<< " task  UUID: " << object->mID << llendl;
	if(mTaskUUID == object->mID)
	{
		mInventoryNeedsUpdate = TRUE;
	}

	// refresh any properties floaters that are hanging around.
	if(inventory)
	{
		for (InventoryObjectList::const_iterator iter = inventory->begin();
			 iter != inventory->end(); )
		{
			LLInventoryObject* item = *iter++;
			LLFloaterProperties* floater = LLFloaterReg::findTypedInstance<LLFloaterProperties>("properites", item->getUUID());
			if(floater)
			{
				floater->refresh();
			}
		}
	}
}

void LLPanelObjectInventory::updateInventory()
{
	//llinfos << "inventory arrived: \n"
	//		<< " panel UUID: " << panel->mTaskUUID << "\n"
	//		<< " task  UUID: " << object->mID << llendl;
	// We're still interested in this task's inventory.
	std::set<LLUUID> selected_items;
	BOOL inventory_has_focus = FALSE;
	if (mHaveInventory)
	{
		mFolders->getSelectionList(selected_items);
		inventory_has_focus = gFocusMgr.childHasKeyboardFocus(mFolders);
	}

	reset();

	LLViewerObject* objectp = gObjectList.findObject(mTaskUUID);
	if (objectp)
	{
		LLInventoryObject* inventory_root = objectp->getInventoryRoot();
		InventoryObjectList contents;
		objectp->getInventoryContents(contents);
		if (inventory_root)
		{
			createFolderViews(inventory_root, contents);
			mHaveInventory = TRUE;
			mIsInventoryEmpty = FALSE;
			mFolders->setEnabled(TRUE);
		}
		else
		{
			// TODO: create an empty inventory
			mIsInventoryEmpty = TRUE;
			mHaveInventory = TRUE;
		}
	}
	else
	{
		// TODO: create an empty inventory
		mIsInventoryEmpty = TRUE;
		mHaveInventory = TRUE;
	}

	// restore previous selection
	std::set<LLUUID>::iterator selection_it;
	BOOL first_item = TRUE;
	for (selection_it = selected_items.begin(); selection_it != selected_items.end(); ++selection_it)
	{
		LLFolderViewItem* selected_item = mFolders->getItemByID(*selection_it);
		if (selected_item)
		{
			//HACK: "set" first item then "change" each other one to get keyboard focus right
			if (first_item)
			{
				mFolders->setSelection(selected_item, TRUE, inventory_has_focus);
				first_item = FALSE;
			}
			else
			{
				mFolders->changeSelection(selected_item, TRUE);
			}
		}
	}

	mFolders->requestArrange();
	mInventoryNeedsUpdate = FALSE;
	// Edit menu handler is set in onFocusReceived
}

// *FIX: This is currently a very expensive operation, because we have
// to iterate through the inventory one time for each category. This
// leads to an N^2 based on the category count. This could be greatly
// speeded with an efficient multimap implementation, but we don't
// have that in our current arsenal.
void LLPanelObjectInventory::createFolderViews(LLInventoryObject* inventory_root, InventoryObjectList& contents)
{
	if (!inventory_root)
	{
		return;
	}
	// Create a visible root category.
	LLTaskInvFVBridge* bridge = NULL;
	bridge = LLTaskInvFVBridge::createObjectBridge(this, inventory_root);
	if(bridge)
	{
		LLFolderViewFolder* new_folder = NULL;
		LLFolderViewFolder::Params p;
		p.name = inventory_root->getName();
		p.icon = LLUI::getUIImage("Inv_FolderClosed");
		p.icon_open = LLUI::getUIImage("Inv_FolderOpen");
		p.root = mFolders;
		p.listener = bridge;
		p.tool_tip = p.name;
		new_folder = LLUICtrlFactory::create<LLFolderViewFolder>(p);
		new_folder->addToFolder(mFolders, mFolders);
		new_folder->toggleOpen();

		createViewsForCategory(&contents, inventory_root, new_folder);
	}
}

typedef std::pair<LLInventoryObject*, LLFolderViewFolder*> obj_folder_pair;

void LLPanelObjectInventory::createViewsForCategory(InventoryObjectList* inventory, 
											  LLInventoryObject* parent,
											  LLFolderViewFolder* folder)
{
	// Find all in the first pass
	LLDynamicArray<obj_folder_pair*> child_categories;
	LLTaskInvFVBridge* bridge;
	LLFolderViewItem* view;

	InventoryObjectList::iterator it = inventory->begin();
	InventoryObjectList::iterator end = inventory->end();
	for( ; it != end; ++it)
	{
		LLInventoryObject* obj = *it;

		if(parent->getUUID() == obj->getParentUUID())
		{
			bridge = LLTaskInvFVBridge::createObjectBridge(this, obj);
			if(!bridge)
			{
				continue;
			}
			if(LLAssetType::AT_CATEGORY == obj->getType())
			{
				LLFolderViewFolder::Params p;
				p.name = obj->getName();
				p.icon = LLUI::getUIImage("Inv_FolderClosed");
				p.icon_open = LLUI::getUIImage("Inv_FolderOpen");
				p.root = mFolders;
				p.listener = bridge;
				p.tool_tip = p.name;
				view = LLUICtrlFactory::create<LLFolderViewFolder>(p);
				child_categories.put(new obj_folder_pair(obj,
														 (LLFolderViewFolder*)view));
			}
			else
			{
				LLFolderViewItem::Params params;
				params.name(obj->getName());
				params.icon(bridge->getIcon());
				params.creation_date(bridge->getCreationDate());
				params.root(mFolders);
				params.listener(bridge);
				params.rect(LLRect());
				params.tool_tip = params.name;
				view = LLUICtrlFactory::create<LLFolderViewItem> (params);
			}
			view->addToFolder(folder, mFolders);
		}
	}

	// now, for each category, do the second pass
	for(S32 i = 0; i < child_categories.count(); i++)
	{
		createViewsForCategory(inventory, child_categories[i]->first,
							   child_categories[i]->second );
		delete child_categories[i];
	}
}

void LLPanelObjectInventory::refresh()
{
	//llinfos << "LLPanelObjectInventory::refresh()" << llendl;
	BOOL has_inventory = FALSE;
	const BOOL non_root_ok = TRUE;
	LLSelectNode* node = LLSelectMgr::getInstance()->getSelection()->getFirstRootNode(NULL, non_root_ok);
	if(node)
	{
		LLViewerObject* object = node->getObject();
		if(object && ((LLSelectMgr::getInstance()->getSelection()->getRootObjectCount() == 1)
					  || (LLSelectMgr::getInstance()->getSelection()->getObjectCount() == 1)))
		{
			// determine if we need to make a request. Start with a
			// default based on if we have inventory at all.
			BOOL make_request = !mHaveInventory;

			// If the task id is different than what we've stored,
			// then make the request.
			if(mTaskUUID != object->mID)
			{
				mTaskUUID = object->mID;
				make_request = TRUE;

				// This is a new object so pre-emptively clear the contents
				// Otherwise we show the old stuff until the update comes in
				clearContents();

				// Register for updates from this object,
				registerVOInventoryListener(object,NULL);
			}

			// Based on the node information, we may need to dirty the
			// object inventory and get it again.
			if(node->mValid)
			{
				if(node->mInventorySerial != object->getInventorySerial() || object->isInventoryDirty())
				{
					make_request = TRUE;
				}
			}

			// do the request if necessary.
			if(make_request)
			{
				requestVOInventory();
			}
			has_inventory = TRUE;
		}
	}
	if(!has_inventory)
	{
		mTaskUUID = LLUUID::null;
		removeVOInventoryListener();
		clearContents();
	}
	//llinfos << "LLPanelObjectInventory::refresh() " << mTaskUUID << llendl;
}

void LLPanelObjectInventory::removeSelectedItem()
{
	if(mFolders)
	{
		mFolders->removeSelectedItems();
	}
}

void LLPanelObjectInventory::startRenamingSelectedItem()
{
	if(mFolders)
	{
		mFolders->startRenamingSelectedItem();
	}
}

void LLPanelObjectInventory::draw()
{
	LLPanel::draw();

	if(mIsInventoryEmpty)
	{
		if((LLUUID::null != mTaskUUID) && (!mHaveInventory))
		{
			LLFontGL::getFontSansSerif()->renderUTF8(LLTrans::getString("LoadingContents"), 0,
													 (S32)(getRect().getWidth() * 0.5f),
													 10,
													 LLColor4( 1, 1, 1, 1 ),
													 LLFontGL::HCENTER,
													 LLFontGL::BOTTOM);
		}
		else if(mHaveInventory)
		{
			LLFontGL::getFontSansSerif()->renderUTF8(LLTrans::getString("NoContents"), 0,
													 (S32)(getRect().getWidth() * 0.5f),
													 10,
													 LLColor4( 1, 1, 1, 1 ),
													 LLFontGL::HCENTER,
													 LLFontGL::BOTTOM);
		}
	}
}

void LLPanelObjectInventory::deleteAllChildren()
{
	mScroller = NULL;
	mFolders = NULL;
	LLView::deleteAllChildren();
}

BOOL LLPanelObjectInventory::handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop, EDragAndDropType cargo_type, void *cargo_data, EAcceptance *accept, std::string& tooltip_msg)
{
	if (mFolders && mHaveInventory)
	{
		LLFolderViewItem* folderp = mFolders->getNextFromChild(NULL);
		if (!folderp)
		{
			return FALSE;
		}
		// Try to pass on unmodified mouse coordinates
		S32 local_x = x - mFolders->getRect().mLeft;
		S32 local_y = y - mFolders->getRect().mBottom;

		if (mFolders->pointInView(local_x, local_y))
		{
			return mFolders->handleDragAndDrop(local_x, local_y, mask, drop, cargo_type, cargo_data, accept, tooltip_msg);
		}
		else
		{
			//force mouse coordinates to be inside folder rectangle
			return mFolders->handleDragAndDrop(5, 1, mask, drop, cargo_type, cargo_data, accept, tooltip_msg);
		}
	}
	else
	{
		return FALSE;
	}
}

//static
void LLPanelObjectInventory::idle(void* user_data)
{
	LLPanelObjectInventory* self = (LLPanelObjectInventory*)user_data;


	if (self->mInventoryNeedsUpdate)
	{
		self->updateInventory();
	}
}

void LLPanelObjectInventory::onFocusLost()
{
	// inventory no longer handles cut/copy/paste/delete
	if (LLEditMenuHandler::gEditMenuHandler == mFolders)
	{
		LLEditMenuHandler::gEditMenuHandler = NULL;
	}
	
	LLPanel::onFocusLost();
}

void LLPanelObjectInventory::onFocusReceived()
{
	// inventory now handles cut/copy/paste/delete
	LLEditMenuHandler::gEditMenuHandler = mFolders;
	
	LLPanel::onFocusReceived();
}
