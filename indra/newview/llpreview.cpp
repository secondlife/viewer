/** 
 * @file llpreview.cpp
 * @brief LLPreview class implementation
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

#include "llviewerprecompiledheaders.h"
#include "stdenums.h"

#include "llpreview.h"

#include "lllineeditor.h"
#include "llinventory.h"
#include "llinventorymodel.h"
#include "llresmgr.h"
#include "lltextbox.h"
#include "llfloaterreg.h"
#include "llfocusmgr.h"
#include "lltooldraganddrop.h"
#include "llradiogroup.h"
#include "llassetstorage.h"
#include "llviewerassettype.h"
#include "llviewerobject.h"
#include "llviewerobjectlist.h"
#include "lldbstrings.h"
#include "llagent.h"
#include "llvoavatarself.h"
#include "llselectmgr.h"
#include "llviewerinventory.h"
#include "llviewerwindow.h"
#include "lltrans.h"

// Constants

LLPreview::LLPreview(const LLSD& key)
:	LLFloater(key),
	mItemUUID(key.asUUID()),
	mObjectUUID(),			// set later by setObjectID()
	mCopyToInvBtn( NULL ),
	mForceClose(FALSE),
	mUserResized(FALSE),
	mCloseAfterSave(FALSE),
	mAssetStatus(PREVIEW_ASSET_UNLOADED),
	mDirty(TRUE)
{
	mAuxItem = new LLInventoryItem;
	// don't necessarily steal focus on creation -- sometimes these guys pop up without user action
	setAutoFocus(FALSE);

	gInventory.addObserver(this);
	
	refreshFromItem();
}

BOOL LLPreview::postBuild()
{
	refreshFromItem();
	return TRUE;
}

LLPreview::~LLPreview()
{
	gFocusMgr.releaseFocusIfNeeded( this ); // calls onCommit()
	gInventory.removeObserver(this);
}

void LLPreview::setObjectID(const LLUUID& object_id)
{
	mObjectUUID = object_id;
	if (getAssetStatus() == PREVIEW_ASSET_UNLOADED)
	{
		loadAsset();
	}
}

void LLPreview::setItem( LLInventoryItem* item )
{
	mItem = item;
	if (mItem && getAssetStatus() == PREVIEW_ASSET_UNLOADED)
	{
		loadAsset();
	}
}

const LLInventoryItem *LLPreview::getItem() const
{
	const LLInventoryItem *item = NULL;
	if (mItem.notNull())
	{
		item = mItem;
	}
	else if (mObjectUUID.isNull())
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
			item = dynamic_cast<LLInventoryItem*>(object->getInventoryObject(mItemUUID));
		}
	}
	return item;
}

// Sub-classes should override this function if they allow editing
void LLPreview::onCommit()
{
	const LLViewerInventoryItem *item = dynamic_cast<const LLViewerInventoryItem*>(getItem());
	if(item)
	{
		if (!item->isComplete())
		{
			// We are attempting to save an item that was never loaded
			llwarns << "LLPreview::onCommit() called with mIsComplete == FALSE"
					<< " Type: " << item->getType()
					<< " ID: " << item->getUUID()
					<< llendl;
			return;
		}
		
		LLPointer<LLViewerInventoryItem> new_item = new LLViewerInventoryItem(item);
		new_item->setDescription(childGetText("desc"));

		std::string new_name = childGetText("name");
		if ( (new_item->getName() != new_name) && !new_name.empty())
		{
			new_item->rename(childGetText("name"));
		}

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
			gInventory.notifyObservers();

			// If the item is an attachment that is currently being worn,
			// update the object itself.
			if( item->getType() == LLAssetType::AT_OBJECT )
			{
				LLVOAvatarSelf* avatarp = gAgent.getAvatarObject();
				if(avatarp)
				{
					LLViewerObject* obj = avatarp->getWornAttachment( item->getUUID() );
					if( obj )
					{
						LLSelectMgr::getInstance()->deselectAll();
						LLSelectMgr::getInstance()->addAsIndividual( obj, SELECT_ALL_TES, FALSE );
						LLSelectMgr::getInstance()->selectionSetObjectDescription( childGetText("desc") );

						LLSelectMgr::getInstance()->deselectAll();
					}
				}
			}
		}
	}
}

void LLPreview::changed(U32 mask)
{
	mDirty = TRUE;
}

void LLPreview::setNotecardInfo(const LLUUID& notecard_inv_id, 
								const LLUUID& object_id)
{
	mNotecardInventoryID = notecard_inv_id;
	mNotecardObjectID = object_id;
}

void LLPreview::draw()
{
	LLFloater::draw();
	if (mDirty)
	{
		mDirty = FALSE;
		refreshFromItem();
	}
}

void LLPreview::refreshFromItem()
{
	const LLInventoryItem* item = getItem();
	if (!item)
	{
		return;
	}
	if (hasString("Title"))
	{
		LLStringUtil::format_map_t args;
		args["[NAME]"] = item->getName();
		LLUIString title = getString("Title", args);
		setTitle(title.getString());
	}
	childSetText("desc",item->getDescription());

	BOOL can_agent_manipulate = item->getPermissions().allowModifyBy(gAgent.getID());
	childSetEnabled("desc",can_agent_manipulate);
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
void LLPreview::hide(const LLUUID& item_uuid, BOOL no_saving /* = FALSE */ )
{
	LLFloater* floater = LLFloaterReg::findInstance("preview", LLSD(item_uuid));
	if (!floater) floater = LLFloaterReg::findInstance("preview_avatar", LLSD(item_uuid));
	
	LLPreview* preview = dynamic_cast<LLPreview*>(floater);
	if (preview)
	{
		if ( no_saving )
		{
			preview->mForceClose = TRUE;
		}
		preview->closeFloater();
	}
}

// static
void LLPreview::dirty(const LLUUID& item_uuid)
{
	LLFloater* floater = LLFloaterReg::findInstance("preview", LLSD(item_uuid));
	if (!floater) floater = LLFloaterReg::findInstance("preview_avatar", LLSD(item_uuid));
	
	LLPreview* preview = dynamic_cast<LLPreview*>(floater);
	if(preview)
	{
		preview->mDirty = TRUE;
	}
}

BOOL LLPreview::handleMouseDown(S32 x, S32 y, MASK mask)
{
	if(mClientRect.pointInRect(x, y))
	{
		// No handler needed for focus lost since this class has no
		// state that depends on it.
		bringToFront(x, y);
		gFocusMgr.setMouseCapture(this);
		S32 screen_x;
		S32 screen_y;
		localPointToScreen(x, y, &screen_x, &screen_y );
		LLToolDragAndDrop::getInstance()->setDragStart(screen_x, screen_y);
		return TRUE;
	}
	return LLFloater::handleMouseDown(x, y, mask);
}

BOOL LLPreview::handleMouseUp(S32 x, S32 y, MASK mask)
{
	if(hasMouseCapture())
	{
		gFocusMgr.setMouseCapture(NULL);
		return TRUE;
	}
	return LLFloater::handleMouseUp(x, y, mask);
}

BOOL LLPreview::handleHover(S32 x, S32 y, MASK mask)
{
	if(hasMouseCapture())
	{
		S32 screen_x;
		S32 screen_y;
		const LLInventoryItem *item = getItem();

		localPointToScreen(x, y, &screen_x, &screen_y );
		if(item
		   && item->getPermissions().allowCopyBy(gAgent.getID(),
												 gAgent.getGroupID())
		   && LLToolDragAndDrop::getInstance()->isOverThreshold(screen_x, screen_y))
		{
			EDragAndDropType type;
			type = LLViewerAssetType::lookupDragAndDropType(item->getType());
			LLToolDragAndDrop::ESource src = LLToolDragAndDrop::SOURCE_LIBRARY;
			if(!mObjectUUID.isNull())
			{
				src = LLToolDragAndDrop::SOURCE_WORLD;
			}
			else if(item->getPermissions().getOwner() == gAgent.getID())
			{
				src = LLToolDragAndDrop::SOURCE_AGENT;
			}
			LLToolDragAndDrop::getInstance()->beginDrag(type,
										item->getUUID(),
										src,
										mObjectUUID);
			return LLToolDragAndDrop::getInstance()->handleHover(x, y, mask );
		}
	}
	return LLFloater::handleHover(x,y,mask);
}

void LLPreview::onOpen(const LLSD& key)
{
	if (!getFloaterHost() && !getHost() && getAssetStatus() == PREVIEW_ASSET_UNLOADED)
	{
		loadAsset();
	}
}

void LLPreview::setAuxItem( const LLInventoryItem* item )
{
	if ( mAuxItem ) 
		mAuxItem->copyItem(item);
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
			copy_inventory_from_notecard(self->mNotecardObjectID,
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
	self->closeFloater();
}

// static
void LLPreview::onKeepBtn(void* data)
{
	LLPreview* self = (LLPreview*)data;
	self->closeFloater();
}

// static
void LLPreview::onDiscardBtn(void* data)
{
	LLPreview* self = (LLPreview*)data;

	const LLInventoryItem* item = self->getItem();
	if (!item) return;

	self->mForceClose = TRUE;
	self->closeFloater();

	// Delete the item entirely
	/*
	item->removeFromServer();
	gInventory.deleteObject(item->getUUID());
	gInventory.notifyObservers();
	*/

	// Move the item to the trash
	const LLUUID trash_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_TRASH);
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

void LLPreview::handleReshape(const LLRect& new_rect, bool by_user)
{
	if(by_user 
		&& (new_rect.getWidth() != getRect().getWidth() || new_rect.getHeight() != getRect().getHeight()))
	{
		userResized();
	}
	LLFloater::handleReshape(new_rect, by_user);
}

//
// LLMultiPreview
//

LLMultiPreview::LLMultiPreview()
	: LLMultiFloater(LLSD())
{
	// *TODO: There should be a .xml file for this
	const LLRect& nextrect = LLFloaterReg::getFloaterRect("preview"); // place where the next preview should show up
	if (nextrect.getWidth() > 0)
	{
		setRect(nextrect);
	}
	else
	{
		// start with a rect in the top-left corner ; will get resized
		LLRect rect;
		rect.setLeftTopAndSize(0, gViewerWindow->getWindowHeightScaled(), 200, 200);
		setRect(rect);
	}
	setTitle(LLTrans::getString("MultiPreviewTitle"));
	buildTabContainer();
	setCanResize(TRUE);
}

void LLMultiPreview::onOpen(const LLSD& key)
{
	LLPreview* frontmost_preview = (LLPreview*)mTabContainer->getCurrentPanel();
	if (frontmost_preview && frontmost_preview->getAssetStatus() == LLPreview::PREVIEW_ASSET_UNLOADED)
	{
		frontmost_preview->loadAsset();
	}
	LLMultiFloater::onOpen(key);
}


void LLMultiPreview::handleReshape(const LLRect& new_rect, bool by_user)
{
	if(new_rect.getWidth() != getRect().getWidth() || new_rect.getHeight() != getRect().getHeight())
	{
		LLPreview* frontmost_preview = (LLPreview*)mTabContainer->getCurrentPanel();
		if (frontmost_preview) frontmost_preview->userResized();
	}
	LLFloater::handleReshape(new_rect, by_user);
}


void LLMultiPreview::tabOpen(LLFloater* opened_floater, bool from_click)
{
	LLPreview* opened_preview = (LLPreview*)opened_floater;
	if (opened_preview && opened_preview->getAssetStatus() == LLPreview::PREVIEW_ASSET_UNLOADED)
	{
		opened_preview->loadAsset();
	}
}


void LLPreview::setAssetId(const LLUUID& asset_id)
{
	const LLViewerInventoryItem* item = dynamic_cast<const LLViewerInventoryItem*>(getItem());
	if(NULL == item)
	{
		return;
	}
	
	if(mObjectUUID.isNull())
	{
		// Update avatar inventory asset_id.
		LLPointer<LLViewerInventoryItem> new_item = new LLViewerInventoryItem(item);
		new_item->setAssetUUID(asset_id);
		gInventory.updateItem(new_item);
		gInventory.notifyObservers();
	}
	else
	{
		// Update object inventory asset_id.
		LLViewerObject* object = gObjectList.findObject(mObjectUUID);
		if(NULL == object)
		{
			return;
		}
		object->updateViewerInventoryAsset(item, asset_id);
	}
}
