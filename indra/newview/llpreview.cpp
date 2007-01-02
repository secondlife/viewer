/** 
 * @file llpreview.cpp
 * @brief LLPreview class implementation
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"
#include "stdenums.h"

#include "llpreview.h"
#include "lllineeditor.h"
#include "llinventory.h"
#include "llinventorymodel.h"
#include "llresmgr.h"
#include "lltextbox.h"
#include "llfocusmgr.h"
#include "lltooldraganddrop.h"
#include "llradiogroup.h"
#include "llassetstorage.h"
#include "llviewerobject.h"
#include "llviewerobjectlist.h"
#include "lldbstrings.h"
#include "llagent.h"
#include "llvoavatar.h"
#include "llselectmgr.h"
#include "llinventoryview.h"
#include "llviewerinventory.h"

// Constants

// Globals and statics
LLPreview::preview_multimap_t LLPreview::sPreviewsBySource;
LLPreview::preview_map_t LLPreview::sInstances;

// Functions
LLPreview::LLPreview(const std::string& name) :
	LLFloater(name),
	mCopyToInvBtn(NULL),
	mForceClose(FALSE),
	mCloseAfterSave(FALSE),
	mAssetStatus(PREVIEW_ASSET_UNLOADED)
{
	// don't add to instance list, since ItemID is null
	mAuxItem = new LLInventoryItem; // (LLPointer is auto-deleted)
	// don't necessarily steal focus on creation -- sometimes these guys pop up without user action
	mAutoFocus = FALSE;
}

LLPreview::LLPreview(const std::string& name, const LLRect& rect, const std::string& title, const LLUUID& item_uuid, const LLUUID& object_uuid, BOOL allow_resize, S32 min_width, S32 min_height )
:	LLFloater(name, rect, title, allow_resize, min_width, min_height ),
	mItemUUID(item_uuid),
	mSourceID(LLUUID::null),
	mObjectUUID(object_uuid),
	mCopyToInvBtn( NULL ),
	mForceClose( FALSE ),
	mCloseAfterSave(FALSE),
	mAssetStatus(PREVIEW_ASSET_UNLOADED)
{
	mAuxItem = new LLInventoryItem;
	// don't necessarily steal focus on creation -- sometimes these guys pop up without user action
	mAutoFocus = FALSE;

	if (mItemUUID.notNull())
	{
		sInstances[mItemUUID] = this;
	}

}

LLPreview::~LLPreview()
{
	gFocusMgr.releaseFocusIfNeeded( this ); // calls onCommit()

	if (mItemUUID.notNull())
	{
		sInstances.erase( mItemUUID );
	}

	if (mSourceID.notNull())
	{
		preview_multimap_t::iterator found_it = sPreviewsBySource.find(mSourceID);
		for (; found_it != sPreviewsBySource.end(); ++found_it)
		{
			if (found_it->second == mViewHandle)
			{
				sPreviewsBySource.erase(found_it);
				break;
			}
		}
	}
}

void LLPreview::setItemID(const LLUUID& item_id)
{
	if (mItemUUID.notNull())
	{
		sInstances.erase(mItemUUID);
	}

	mItemUUID = item_id;

	if (mItemUUID.notNull())
	{
		sInstances[mItemUUID] = this;
	}
}

void LLPreview::setObjectID(const LLUUID& object_id)
{
	mObjectUUID = object_id;
}

void LLPreview::setSourceID(const LLUUID& source_id)
{
	if (mSourceID.notNull())
	{
		// erase old one
		preview_multimap_t::iterator found_it = sPreviewsBySource.find(mSourceID);
		for (; found_it != sPreviewsBySource.end(); ++found_it)
		{
			if (found_it->second == mViewHandle)
			{
				sPreviewsBySource.erase(found_it);
				break;
			}
		}
	}
	mSourceID = source_id;
	sPreviewsBySource.insert(preview_multimap_t::value_type(mSourceID, mViewHandle));
}

LLViewerInventoryItem* LLPreview::getItem() const
{
	LLViewerInventoryItem* item = NULL;
	if(mObjectUUID.isNull())
	{
		// it's an inventory item, so get the item.
		item = gInventory.getItem(mItemUUID);
	}
	else
	{
		// it's an object's inventory item.
		LLViewerObject* object = gObjectList.findObject(mObjectUUID);
		if(object)
		{
			item = (LLViewerInventoryItem*)object->getInventoryObject(mItemUUID);
		}
	}
	return item;
}

// Sub-classes should override this function if they allow editing
void LLPreview::onCommit()
{
	LLViewerInventoryItem* item = getItem();
	if(item)
	{
		LLPointer<LLViewerInventoryItem> new_item = new LLViewerInventoryItem(item);
		BOOL has_sale_info = FALSE;
		LLSaleInfo sale_info;
		new_item->setDescription(childGetText("desc"));
		if(mObjectUUID.notNull())
		{
			// must be in an object
			LLViewerObject* object = gObjectList.findObject(mObjectUUID);
			if(object)
			{
				object->updateInventory(
					new_item,
					TASK_INVENTORY_ITEM_KEY,
					false);
			}
		}
		else if(item->getPermissions().getOwner() == gAgent.getID())
		{
			new_item->updateServer(FALSE);
			gInventory.updateItem(new_item);

			// If the item is an attachment that is currently being worn,
			// update the object itself.
			if( item->getType() == LLAssetType::AT_OBJECT )
			{
				LLVOAvatar* avatar = gAgent.getAvatarObject();
				if( avatar )
				{
					LLViewerObject* obj = avatar->getWornAttachment( item->getUUID() );
					if( obj )
					{
						gSelectMgr->deselectAll();
						gSelectMgr->addAsIndividual( obj, SELECT_ALL_TES, FALSE );
						gSelectMgr->setObjectDescription( childGetText("desc") );
					
						if( has_sale_info )
						{
							gSelectMgr->setObjectSaleInfo( sale_info );
						}

						gSelectMgr->deselectAll();
					}
				}
			}
		}
	}
}

// static 
void LLPreview::onText(LLUICtrl*, void* userdata)
{
	LLPreview* self = (LLPreview*) userdata;
	self->onCommit();
}

// static
void LLPreview::onRadio(LLUICtrl*, void* userdata)
{
	LLPreview* self = (LLPreview*) userdata;
	self->onCommit();
}

// static
LLPreview* LLPreview::find(const LLUUID& item_uuid)
{
	LLPreview* instance = NULL;
	preview_map_t::iterator found_it = LLPreview::sInstances.find(item_uuid);
	if(found_it != LLPreview::sInstances.end())
	{
		instance = found_it->second;
	}
	return instance;
}

// static
LLPreview* LLPreview::show( const LLUUID& item_uuid, BOOL take_focus )
{
	LLPreview* instance = LLPreview::find(item_uuid);
	if(instance)
	{
		if (LLFloater::getFloaterHost() && LLFloater::getFloaterHost() != instance->getHost())
		{
			// this preview window is being opened in a new context
			// needs to be rehosted
			LLFloater::getFloaterHost()->addFloater(instance, TRUE);
		}
		instance->open();
		if (take_focus)
		{
			instance->setFocus(TRUE);
		}
	}

	return instance;
}

// static
bool LLPreview::save( const LLUUID& item_uuid, LLPointer<LLInventoryItem>* itemptr )
{
	bool res = false;
	LLPreview* instance = LLPreview::find(item_uuid);
	if(instance)
	{
		res = instance->saveItem(itemptr);
	}
	if (!res)
	{
		delete itemptr;
	}
	return res;
}

// static
void LLPreview::hide(const LLUUID& item_uuid)
{
	preview_map_t::iterator found_it = LLPreview::sInstances.find(item_uuid);
	if(found_it != LLPreview::sInstances.end())
	{
		LLPreview* instance = found_it->second;
		if( instance->getParent() )
		{
			instance->getParent()->removeChild( instance );
		}

		delete instance;
	}
}

// static
void LLPreview::rename(const LLUUID& item_uuid, const std::string& new_name)
{
	preview_map_t::iterator found_it = LLPreview::sInstances.find(item_uuid);
	if(found_it != LLPreview::sInstances.end())
	{
		LLPreview* instance = found_it->second;
		instance->setTitle( new_name );
	}
}

BOOL LLPreview::handleMouseDown(S32 x, S32 y, MASK mask)
{
	if(mClientRect.pointInRect(x, y))
	{
		// No handler needed for focus lost since this class has no
		// state that depends on it.
		bringToFront(x, y);
		gFocusMgr.setMouseCapture(this, NULL);
		S32 screen_x;
		S32 screen_y;
		localPointToScreen(x, y, &screen_x, &screen_y );
		gToolDragAndDrop->setDragStart(screen_x, screen_y);
		return TRUE;
	}
	return LLFloater::handleMouseDown(x, y, mask);
}

BOOL LLPreview::handleMouseUp(S32 x, S32 y, MASK mask)
{
	if(gFocusMgr.getMouseCapture() == this)
	{
		gFocusMgr.setMouseCapture(NULL, NULL);
		return TRUE;
	}
	return LLFloater::handleMouseUp(x, y, mask);
}

BOOL LLPreview::handleHover(S32 x, S32 y, MASK mask)
{
	if(gFocusMgr.getMouseCapture() == this)
	{
		S32 screen_x;
		S32 screen_y;
		LLViewerInventoryItem *item = getItem();

		localPointToScreen(x, y, &screen_x, &screen_y );
		if(item
		   && item->getPermissions().allowCopyBy(gAgent.getID(),
												 gAgent.getGroupID())
		   && gToolDragAndDrop->isOverThreshold(screen_x, screen_y))
		{
			EDragAndDropType type;
			type = LLAssetType::lookupDragAndDropType(item->getType());
			LLToolDragAndDrop::ESource src = LLToolDragAndDrop::SOURCE_LIBRARY;
			if(!mObjectUUID.isNull())
			{
				src = LLToolDragAndDrop::SOURCE_WORLD;
			}
			else if(item->getPermissions().getOwner() == gAgent.getID())
			{
				src = LLToolDragAndDrop::SOURCE_AGENT;
			}
			gToolDragAndDrop->beginDrag(type,
										item->getUUID(),
										src,
										mObjectUUID);
			return gToolDragAndDrop->handleHover(x, y, mask );
		}
	}
	return LLFloater::handleHover(x,y,mask);
}

void LLPreview::open()
{
	LLMultiFloater* hostp = getHost();
	if (!sHostp && !hostp && getAssetStatus() == PREVIEW_ASSET_UNLOADED)
	{
		loadAsset();
	}
	LLFloater::open();
}

// virtual
bool LLPreview::saveItem(LLPointer<LLInventoryItem>* itemptr)
{
	return false;
}


// static
void LLPreview::onBtnCopyToInv(void* userdata)
{
	LLPreview* self = (LLPreview*) userdata;
	LLInventoryItem *item = self->mAuxItem;

	if(item && item->getUUID().notNull())
	{
		// Copy to inventory
		if (self->mNotecardInventoryID.notNull())
		{
			copy_inventory_from_notecard(self->mObjectID,
				self->mNotecardInventoryID, item);
		}
		else
		{
			LLPointer<LLInventoryCallback> cb = NULL;
			copy_inventory_item(
				gAgent.getID(),
				item->getPermissions().getOwner(),
				item->getUUID(),
				LLUUID::null,
				std::string(),
				cb);
		}
	}
}

// static
void LLPreview::onKeepBtn(void* data)
{
	LLPreview* self = (LLPreview*)data;
	self->close();
}

// static
void LLPreview::onDiscardBtn(void* data)
{
	LLPreview* self = (LLPreview*)data;

	LLViewerInventoryItem* item = self->getItem();
	if (!item) return;

	self->mForceClose = TRUE;
	self->close();

	// Delete the item entirely
	/*
	item->removeFromServer();
	gInventory.deleteObject(item->getUUID());
	gInventory.notifyObservers();
	*/

	// Move the item to the trash
	LLUUID trash_id = gInventory.findCategoryUUIDForType(LLAssetType::AT_TRASH);
	if (item->getParentUUID() != trash_id)
	{
		LLInventoryModel::update_list_t update;
		LLInventoryModel::LLCategoryUpdate old_folder(item->getParentUUID(),-1);
		update.push_back(old_folder);
		LLInventoryModel::LLCategoryUpdate new_folder(trash_id, 1);
		update.push_back(new_folder);
		gInventory.accountForUpdate(update);

		LLPointer<LLViewerInventoryItem> new_item = new LLViewerInventoryItem(item);
		new_item->setParent(trash_id);
		// no need to restamp it though it's a move into trash because
		// it's a brand new item already.
		new_item->updateParentOnServer(FALSE);
		gInventory.updateItem(new_item);
		gInventory.notifyObservers();
	}
}

//static
LLPreview* LLPreview::getFirstPreviewForSource(const LLUUID& source_id)
{
	preview_multimap_t::iterator found_it = sPreviewsBySource.find(source_id);
	if (found_it != sPreviewsBySource.end())
	{
		// just return first one
		return (LLPreview*)LLFloater::getFloaterByHandle(found_it->second);
	}
	return NULL;
}

//
// LLMultiPreview
//

LLMultiPreview::LLMultiPreview(const LLRect& rect) : LLMultiFloater("Preview", rect)
{
}

void LLMultiPreview::open()
{
	LLMultiFloater::open();
	LLPreview* frontmost_preview = (LLPreview*)mTabContainer->getCurrentPanel();
	if (frontmost_preview && frontmost_preview->getAssetStatus() == LLPreview::PREVIEW_ASSET_UNLOADED)
	{
		frontmost_preview->loadAsset();
	}
}

void LLMultiPreview::tabOpen(LLFloater* opened_floater, bool from_click)
{
	LLPreview* opened_preview = (LLPreview*)opened_floater;
	if (opened_preview && opened_preview->getAssetStatus() == LLPreview::PREVIEW_ASSET_UNLOADED)
	{
		opened_preview->loadAsset();
	}
}
