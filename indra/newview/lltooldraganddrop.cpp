/** 
 * @file lltooldraganddrop.cpp
 * @brief LLToolDragAndDrop class implementation
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
#include "lltooldraganddrop.h"

// library headers
#include "llnotificationsutil.h"
// project headers
#include "llagent.h"
#include "llagentcamera.h"
#include "llagentwearables.h"
#include "llappearancemgr.h"
#include "lldictionary.h"
#include "llfloaterreg.h"
#include "llfloatertools.h"
#include "llgesturemgr.h"
#include "llgiveinventory.h"
#include "llhudmanager.h"
#include "llhudeffecttrail.h"
#include "llimview.h"
#include "llinventorybridge.h"
#include "llinventorydefines.h"
#include "llinventoryfunctions.h"
#include "llpreviewnotecard.h"
#include "llrootview.h"
#include "llselectmgr.h"
#include "lltoolmgr.h"
#include "lltooltip.h"
#include "lltrans.h"
#include "llviewerobjectlist.h"
#include "llviewerregion.h"
#include "llviewerstats.h"
#include "llviewerwindow.h"
#include "llvoavatarself.h"
#include "llworld.h"
#include "llclipboard.h"

// syntactic sugar
#define callMemberFunction(object,ptrToMember)  ((object).*(ptrToMember))

class LLNoPreferredType : public LLInventoryCollectFunctor
{
public:
	LLNoPreferredType() {}
	virtual ~LLNoPreferredType() {}
	virtual bool operator()(LLInventoryCategory* cat,
							LLInventoryItem* item)
	{
		if (cat && (cat->getPreferredType() == LLFolderType::FT_NONE))
		{
			return true;
		}
		return false;
	}
};

class LLNoPreferredTypeOrItem : public LLInventoryCollectFunctor
{
public:
	LLNoPreferredTypeOrItem() {}
	virtual ~LLNoPreferredTypeOrItem() {}
	virtual bool operator()(LLInventoryCategory* cat,
							LLInventoryItem* item)
	{
		if (item) return true;
		if (cat && (cat->getPreferredType() == LLFolderType::FT_NONE))
		{
			return true;
		}
		return false;
	}
};

class LLDroppableItem : public LLInventoryCollectFunctor
{
public:
	LLDroppableItem(BOOL is_transfer) :
		mCountLosing(0), mIsTransfer(is_transfer) {}
	virtual ~LLDroppableItem() {}
	virtual bool operator()(LLInventoryCategory* cat,
							LLInventoryItem* item);
	S32 countNoCopy() const { return mCountLosing; }

protected:
	S32 mCountLosing;
	BOOL mIsTransfer;
};

bool LLDroppableItem::operator()(LLInventoryCategory* cat,
				 LLInventoryItem* item)
{
	bool allowed = false;
	if (item)
	{
		allowed = itemTransferCommonlyAllowed(item);

		if (allowed
		   && mIsTransfer
		   && !item->getPermissions().allowOperationBy(PERM_TRANSFER,
							       gAgent.getID()))
		{
			allowed = false;
		}
		if (allowed && !item->getPermissions().allowCopyBy(gAgent.getID()))
		{
			++mCountLosing;
		}
	}
	return allowed;
}

class LLDropCopyableItems : public LLInventoryCollectFunctor
{
public:
	LLDropCopyableItems() {}
	virtual ~LLDropCopyableItems() {}
	virtual bool operator()(LLInventoryCategory* cat, LLInventoryItem* item);
};


bool LLDropCopyableItems::operator()(
	LLInventoryCategory* cat,
	LLInventoryItem* item)
{
	bool allowed = false;
	if (item)
	{
		allowed = itemTransferCommonlyAllowed(item);
		if (allowed &&
		   !item->getPermissions().allowCopyBy(gAgent.getID()))
		{
			// whoops, can't copy it - don't allow it.
			allowed = false;
		}
	}
	return allowed;
}

// Starts a fetch on folders and items.  This is really not used 
// as an observer in the traditional sense; we're just using it to
// request a fetch and we don't care about when/if the response arrives.
class LLCategoryFireAndForget : public LLInventoryFetchComboObserver
{
public:
	LLCategoryFireAndForget(const uuid_vec_t& folder_ids,
							const uuid_vec_t& item_ids) :
		LLInventoryFetchComboObserver(folder_ids, item_ids)
	{}
	~LLCategoryFireAndForget() {}
	virtual void done()
	{
		/* no-op: it's fire n forget right? */
		lldebugs << "LLCategoryFireAndForget::done()" << llendl;
	}
};

class LLCategoryDropObserver : public LLInventoryFetchItemsObserver
{
public:
	LLCategoryDropObserver(
		const uuid_vec_t& ids,
		const LLUUID& obj_id, LLToolDragAndDrop::ESource src) :
		LLInventoryFetchItemsObserver(ids),
		mObjectID(obj_id),
		mSource(src)
	{}
	~LLCategoryDropObserver() {}
	virtual void done();

protected:
	LLUUID mObjectID;
	LLToolDragAndDrop::ESource mSource;
};

void LLCategoryDropObserver::done()
{
	gInventory.removeObserver(this);
	LLViewerObject* dst_obj = gObjectList.findObject(mObjectID);
	if (dst_obj)
	{
		// *FIX: coalesce these...
 		LLInventoryItem* item = NULL;
  		uuid_vec_t::iterator it = mComplete.begin();
  		uuid_vec_t::iterator end = mComplete.end();
  		for(; it < end; ++it)
  		{
 			item = gInventory.getItem(*it);
 			if (item)
 			{
 				LLToolDragAndDrop::dropInventory(
 					dst_obj,
 					item,
 					mSource,
 					LLUUID::null);
 			}
  		}
	}
	delete this;
}
/* Doesn't seem to be used anymore.
class LLCategoryDropDescendentsObserver : public LLInventoryFetchDescendentsObserver
{
public:
	LLCategoryDropDescendentsObserver(
		const LLUUID& obj_id, LLToolDragAndDrop::ESource src) :
		mObjectID(obj_id),
		mSource(src)
	{}
	~LLCategoryDropDescendentsObserver() {}
	virtual void done();

protected:
	LLUUID mObjectID;
	LLToolDragAndDrop::ESource mSource;
};

void LLCategoryDropDescendentsObserver::done()
{

	gInventory.removeObserver(this);
	uuid_vec_t::iterator it = mComplete.begin();
	uuid_vec_t::iterator end = mComplete.end();
	LLViewerInventoryCategory::cat_array_t cats;
	LLViewerInventoryItem::item_array_t items;
	for(; it != end; ++it)
	{
		gInventory.collectDescendents(
			(*it),
			cats,
			items,
			LLInventoryModel::EXCLUDE_TRASH);
	}

	S32 count = items.count();
	if (count)
	{
		std::set<LLUUID> unique_ids;
		for(S32 i = 0; i < count; ++i)
		{
			unique_ids.insert(items.get(i)->getUUID());
		}
		uuid_vec_t ids;
		std::back_insert_iterator<uuid_vec_t> copier(ids);
		std::copy(unique_ids.begin(), unique_ids.end(), copier);
		LLCategoryDropObserver* dropper;
		dropper = new LLCategoryDropObserver(ids, mObjectID, mSource);
		dropper->startFetch();
		if (dropper->isDone())
		{
			dropper->done();
		}
		else
		{
			gInventory.addObserver(dropper);
		}
	}
	delete this;
}
*/

S32 LLToolDragAndDrop::sOperationId = 0;

LLToolDragAndDrop::DragAndDropEntry::DragAndDropEntry(dragOrDrop3dImpl f_none,
													  dragOrDrop3dImpl f_self,
													  dragOrDrop3dImpl f_avatar,
													  dragOrDrop3dImpl f_object,
													  dragOrDrop3dImpl f_land) :
	LLDictionaryEntry("")
{
	mFunctions[DT_NONE] = f_none;
	mFunctions[DT_SELF] = f_self;
	mFunctions[DT_AVATAR] = f_avatar;
	mFunctions[DT_OBJECT] = f_object;
	mFunctions[DT_LAND] = f_land;
}

LLToolDragAndDrop::dragOrDrop3dImpl LLToolDragAndDrop::LLDragAndDropDictionary::get(EDragAndDropType dad_type, LLToolDragAndDrop::EDropTarget drop_target)
{
	const DragAndDropEntry *entry = lookup(dad_type);
	if (entry)
	{
		return (entry->mFunctions[(U8)drop_target]);
	}
	return &LLToolDragAndDrop::dad3dNULL;
}

LLToolDragAndDrop::LLDragAndDropDictionary::LLDragAndDropDictionary()
{
 	//       										 DT_NONE                         DT_SELF                                        DT_AVATAR                   					DT_OBJECT                       					DT_LAND		
	//      										|-------------------------------|----------------------------------------------|-----------------------------------------------|---------------------------------------------------|--------------------------------|
	addEntry(DAD_NONE, 			new DragAndDropEntry(&LLToolDragAndDrop::dad3dNULL,	&LLToolDragAndDrop::dad3dNULL,					&LLToolDragAndDrop::dad3dNULL,					&LLToolDragAndDrop::dad3dNULL,						&LLToolDragAndDrop::dad3dNULL));
	addEntry(DAD_TEXTURE, 		new DragAndDropEntry(&LLToolDragAndDrop::dad3dNULL,	&LLToolDragAndDrop::dad3dNULL,					&LLToolDragAndDrop::dad3dGiveInventory,			&LLToolDragAndDrop::dad3dTextureObject,				&LLToolDragAndDrop::dad3dNULL));
	addEntry(DAD_SOUND, 		new DragAndDropEntry(&LLToolDragAndDrop::dad3dNULL,	&LLToolDragAndDrop::dad3dNULL,					&LLToolDragAndDrop::dad3dGiveInventory,			&LLToolDragAndDrop::dad3dUpdateInventory,			&LLToolDragAndDrop::dad3dNULL));
	addEntry(DAD_CALLINGCARD, 	new DragAndDropEntry(&LLToolDragAndDrop::dad3dNULL,	&LLToolDragAndDrop::dad3dNULL,					&LLToolDragAndDrop::dad3dGiveInventory, 		&LLToolDragAndDrop::dad3dUpdateInventory, 			&LLToolDragAndDrop::dad3dNULL));
	addEntry(DAD_LANDMARK, 		new DragAndDropEntry(&LLToolDragAndDrop::dad3dNULL, &LLToolDragAndDrop::dad3dNULL, 					&LLToolDragAndDrop::dad3dGiveInventory, 		&LLToolDragAndDrop::dad3dUpdateInventory, 			&LLToolDragAndDrop::dad3dNULL));
	addEntry(DAD_SCRIPT, 		new DragAndDropEntry(&LLToolDragAndDrop::dad3dNULL, &LLToolDragAndDrop::dad3dNULL, 					&LLToolDragAndDrop::dad3dGiveInventory, 		&LLToolDragAndDrop::dad3dRezScript, 				&LLToolDragAndDrop::dad3dNULL));
	addEntry(DAD_CLOTHING, 		new DragAndDropEntry(&LLToolDragAndDrop::dad3dNULL, &LLToolDragAndDrop::dad3dWearItem, 				&LLToolDragAndDrop::dad3dGiveInventory, 		&LLToolDragAndDrop::dad3dUpdateInventory, 			&LLToolDragAndDrop::dad3dNULL));
	addEntry(DAD_OBJECT, 		new DragAndDropEntry(&LLToolDragAndDrop::dad3dNULL, &LLToolDragAndDrop::dad3dRezAttachmentFromInv,	&LLToolDragAndDrop::dad3dGiveInventoryObject,	&LLToolDragAndDrop::dad3dRezObjectOnObject, 		&LLToolDragAndDrop::dad3dRezObjectOnLand));
	addEntry(DAD_NOTECARD, 		new DragAndDropEntry(&LLToolDragAndDrop::dad3dNULL, &LLToolDragAndDrop::dad3dNULL, 					&LLToolDragAndDrop::dad3dGiveInventory, 		&LLToolDragAndDrop::dad3dUpdateInventory, 			&LLToolDragAndDrop::dad3dNULL));
	addEntry(DAD_CATEGORY, 		new DragAndDropEntry(&LLToolDragAndDrop::dad3dNULL, &LLToolDragAndDrop::dad3dWearCategory,			&LLToolDragAndDrop::dad3dGiveInventoryCategory,	&LLToolDragAndDrop::dad3dUpdateInventoryCategory,	&LLToolDragAndDrop::dad3dNULL));
	addEntry(DAD_ROOT_CATEGORY, new DragAndDropEntry(&LLToolDragAndDrop::dad3dNULL,	&LLToolDragAndDrop::dad3dNULL,					&LLToolDragAndDrop::dad3dNULL,					&LLToolDragAndDrop::dad3dNULL,						&LLToolDragAndDrop::dad3dNULL));
	addEntry(DAD_BODYPART, 		new DragAndDropEntry(&LLToolDragAndDrop::dad3dNULL,	&LLToolDragAndDrop::dad3dWearItem,				&LLToolDragAndDrop::dad3dGiveInventory,			&LLToolDragAndDrop::dad3dUpdateInventory,			&LLToolDragAndDrop::dad3dNULL));
	addEntry(DAD_ANIMATION, 	new DragAndDropEntry(&LLToolDragAndDrop::dad3dNULL,	&LLToolDragAndDrop::dad3dNULL,					&LLToolDragAndDrop::dad3dGiveInventory,			&LLToolDragAndDrop::dad3dUpdateInventory,			&LLToolDragAndDrop::dad3dNULL));
	addEntry(DAD_GESTURE, 		new DragAndDropEntry(&LLToolDragAndDrop::dad3dNULL,	&LLToolDragAndDrop::dad3dActivateGesture,		&LLToolDragAndDrop::dad3dGiveInventory,			&LLToolDragAndDrop::dad3dUpdateInventory,			&LLToolDragAndDrop::dad3dNULL));
	addEntry(DAD_LINK, 			new DragAndDropEntry(&LLToolDragAndDrop::dad3dNULL,	&LLToolDragAndDrop::dad3dNULL,					&LLToolDragAndDrop::dad3dNULL,					&LLToolDragAndDrop::dad3dNULL,						&LLToolDragAndDrop::dad3dNULL));
	addEntry(DAD_MESH, 			new DragAndDropEntry(&LLToolDragAndDrop::dad3dNULL,	&LLToolDragAndDrop::dad3dNULL,					&LLToolDragAndDrop::dad3dGiveInventory,			&LLToolDragAndDrop::dad3dMeshObject,				&LLToolDragAndDrop::dad3dNULL));
	// TODO: animation on self could play it?  edit it?
	// TODO: gesture on self could play it?  edit it?
};

LLToolDragAndDrop::LLToolDragAndDrop()
:	 LLTool(std::string("draganddrop"), NULL),
	 mDragStartX(0),
	 mDragStartY(0),
	 mSource(SOURCE_AGENT),
	 mCursor(UI_CURSOR_NO),
	 mLastAccept(ACCEPT_NO),
	 mDrop(FALSE),
	 mCurItemIndex(0)
{

}

void LLToolDragAndDrop::setDragStart(S32 x, S32 y)
{
	mDragStartX = x;
	mDragStartY = y;
}

BOOL LLToolDragAndDrop::isOverThreshold(S32 x,S32 y)
{
	static LLCachedControl<S32> drag_and_drop_threshold(gSavedSettings,"DragAndDropDistanceThreshold");
	
	S32 mouse_delta_x = x - mDragStartX;
	S32 mouse_delta_y = y - mDragStartY;
	
	return (mouse_delta_x * mouse_delta_x) + (mouse_delta_y * mouse_delta_y) > drag_and_drop_threshold * drag_and_drop_threshold;
}

void LLToolDragAndDrop::beginDrag(EDragAndDropType type,
								  const LLUUID& cargo_id,
								  ESource source,
								  const LLUUID& source_id,
								  const LLUUID& object_id)
{
	if (type == DAD_NONE)
	{
		llwarns << "Attempted to start drag without a cargo type" << llendl;
		return;
	}
	mCargoTypes.clear();
	mCargoTypes.push_back(type);
	mCargoIDs.clear();
	mCargoIDs.push_back(cargo_id);
	mSource = source;
	mSourceID = source_id;
	mObjectID = object_id;

	setMouseCapture( TRUE );
	LLToolMgr::getInstance()->setTransientTool( this );
	mCursor = UI_CURSOR_NO;
	if ((mCargoTypes[0] == DAD_CATEGORY)
	   && ((mSource == SOURCE_AGENT) || (mSource == SOURCE_LIBRARY)))
	{
		LLInventoryCategory* cat = gInventory.getCategory(cargo_id);
		// go ahead and fire & forget the descendents if we are not
		// dragging a protected folder.
		if (cat)
		{
			LLViewerInventoryCategory::cat_array_t cats;
			LLViewerInventoryItem::item_array_t items;
			LLNoPreferredTypeOrItem is_not_preferred;
			uuid_vec_t folder_ids;
			uuid_vec_t item_ids;
			if (is_not_preferred(cat, NULL))
			{
				folder_ids.push_back(cargo_id);
			}
			gInventory.collectDescendentsIf(
				cargo_id,
				cats,
				items,
				LLInventoryModel::EXCLUDE_TRASH,
				is_not_preferred);
			S32 count = cats.count();
			S32 i;
			for(i = 0; i < count; ++i)
			{
				folder_ids.push_back(cats.get(i)->getUUID());
			}
			count = items.count();
			for(i = 0; i < count; ++i)
			{
				item_ids.push_back(items.get(i)->getUUID());
			}
			if (!folder_ids.empty() || !item_ids.empty())
			{
				LLCategoryFireAndForget *fetcher = new LLCategoryFireAndForget(folder_ids, item_ids);
				fetcher->startFetch();
				delete fetcher;
			}
		}
	}
}

void LLToolDragAndDrop::beginMultiDrag(
	const std::vector<EDragAndDropType> types,
	const uuid_vec_t& cargo_ids,
	ESource source,
	const LLUUID& source_id)
{
	// assert on public api is evil
	//llassert( type != DAD_NONE );

	std::vector<EDragAndDropType>::const_iterator types_it;
	for (types_it = types.begin(); types_it != types.end(); ++types_it)
	{
		if (DAD_NONE == *types_it)
		{
			llwarns << "Attempted to start drag without a cargo type" << llendl;
			return;
		}
	}
	mCargoTypes = types;
	mCargoIDs = cargo_ids;
	mSource = source;
	mSourceID = source_id;

	setMouseCapture( TRUE );
	LLToolMgr::getInstance()->setTransientTool( this );
	mCursor = UI_CURSOR_NO;
	if ((mSource == SOURCE_AGENT) || (mSource == SOURCE_LIBRARY))
	{
		// find categories (i.e. inventory folders) in the cargo.
		LLInventoryCategory* cat = NULL;
		S32 count = llmin(cargo_ids.size(), types.size());
		std::set<LLUUID> cat_ids;
		for(S32 i = 0; i < count; ++i)
		{
			cat = gInventory.getCategory(cargo_ids[i]);
			if (cat)
			{
				LLViewerInventoryCategory::cat_array_t cats;
				LLViewerInventoryItem::item_array_t items;
				LLNoPreferredType is_not_preferred;
				if (is_not_preferred(cat, NULL))
				{
					cat_ids.insert(cat->getUUID());
				}
				gInventory.collectDescendentsIf(
					cat->getUUID(),
					cats,
					items,
					LLInventoryModel::EXCLUDE_TRASH,
					is_not_preferred);
				S32 cat_count = cats.count();
				for(S32 i = 0; i < cat_count; ++i)
				{
					cat_ids.insert(cat->getUUID());
				}
			}
		}
		if (!cat_ids.empty())
		{
			uuid_vec_t folder_ids;
			uuid_vec_t item_ids;
			std::back_insert_iterator<uuid_vec_t> copier(folder_ids);
			std::copy(cat_ids.begin(), cat_ids.end(), copier);
			LLCategoryFireAndForget fetcher(folder_ids, item_ids);
		}
	}
}

void LLToolDragAndDrop::endDrag()
{
	mEndDragSignal();
	LLSelectMgr::getInstance()->unhighlightAll();
	setMouseCapture(FALSE);
}

void LLToolDragAndDrop::onMouseCaptureLost()
{
	// Called whenever the drag ends or if mouse capture is simply lost
	LLToolMgr::getInstance()->clearTransientTool();
	mCargoTypes.clear();
	mCargoIDs.clear();
	mSource = SOURCE_AGENT;
	mSourceID.setNull();
	mObjectID.setNull();
}

BOOL LLToolDragAndDrop::handleMouseUp( S32 x, S32 y, MASK mask )
{
	if (hasMouseCapture())
	{
		EAcceptance acceptance = ACCEPT_NO;
		dragOrDrop( x, y, mask, TRUE, &acceptance );
		endDrag();
	}
	return TRUE;
}

ECursorType LLToolDragAndDrop::acceptanceToCursor( EAcceptance acceptance )
{
	switch (acceptance)
	{
	case ACCEPT_YES_MULTI:
		if (mCargoIDs.size() > 1)
		{
			mCursor = UI_CURSOR_ARROWDRAGMULTI;
		}
		else
		{
			mCursor = UI_CURSOR_ARROWDRAG;
		}
		break;
	case ACCEPT_YES_SINGLE:
		if (mCargoIDs.size() > 1)
		{
			mToolTipMsg = LLTrans::getString("TooltipMustSingleDrop");
			mCursor = UI_CURSOR_NO;
		}
		else
		{
			mCursor = UI_CURSOR_ARROWDRAG;
		}
		break;

	case ACCEPT_NO_LOCKED:
		mCursor = UI_CURSOR_NOLOCKED;
		break;

	case ACCEPT_NO:
		mCursor = UI_CURSOR_NO;
		break;

	case ACCEPT_YES_COPY_MULTI:
		if (mCargoIDs.size() > 1)
		{
			mCursor = UI_CURSOR_ARROWCOPYMULTI;
		}
		else
		{
			mCursor = UI_CURSOR_ARROWCOPY;
		}
		break;
	case ACCEPT_YES_COPY_SINGLE:
		if (mCargoIDs.size() > 1)
		{
			mToolTipMsg = LLTrans::getString("TooltipMustSingleDrop");
			mCursor = UI_CURSOR_NO;
		}
		else
		{
			mCursor = UI_CURSOR_ARROWCOPY;
		}
		break;
	case ACCEPT_POSTPONED:
		break;
	default:
		llassert( FALSE );
	}

	return mCursor;
}

BOOL LLToolDragAndDrop::handleHover( S32 x, S32 y, MASK mask )
{
	EAcceptance acceptance = ACCEPT_NO;
	dragOrDrop( x, y, mask, FALSE, &acceptance );

	ECursorType cursor = acceptanceToCursor(acceptance);
	gViewerWindow->getWindow()->setCursor( cursor );

	lldebugst(LLERR_USER_INPUT) << "hover handled by LLToolDragAndDrop" << llendl;
	return TRUE;
}

BOOL LLToolDragAndDrop::handleKey(KEY key, MASK mask)
{
	if (key == KEY_ESCAPE)
	{
		// cancel drag and drop operation
		endDrag();
		return TRUE;
	}

	return FALSE;
}

BOOL LLToolDragAndDrop::handleToolTip(S32 x, S32 y, MASK mask)
{
	if (!mToolTipMsg.empty())
	{
		LLToolTipMgr::instance().unblockToolTips();
		LLToolTipMgr::instance().show(LLToolTip::Params()
			.message(mToolTipMsg)
			.delay_time(gSavedSettings.getF32( "DragAndDropToolTipDelay" )));
		return TRUE;
	}
	return FALSE;
}

void LLToolDragAndDrop::handleDeselect()
{
	mToolTipMsg.clear();

	LLToolTipMgr::instance().blockToolTips();
}

// protected
void LLToolDragAndDrop::dragOrDrop( S32 x, S32 y, MASK mask, BOOL drop, 
								   EAcceptance* acceptance)
{
	*acceptance = ACCEPT_YES_MULTI;

	BOOL handled = FALSE;

	LLView* top_view = gFocusMgr.getTopCtrl();
	LLViewerInventoryItem* item;
	LLViewerInventoryCategory* cat;

	mToolTipMsg.clear();

	// Increment the operation id for every drop
	if (drop)
	{
		sOperationId++;
	}

	if (top_view)
	{
		handled = TRUE;

		for (mCurItemIndex = 0; mCurItemIndex < (S32)mCargoIDs.size(); mCurItemIndex++)
		{
			LLInventoryObject* cargo = locateInventory(item, cat);

			if (cargo)
			{
				S32 local_x, local_y;
				top_view->screenPointToLocal( x, y, &local_x, &local_y );
				EAcceptance item_acceptance = ACCEPT_NO;
				handled = handled && top_view->handleDragAndDrop(local_x, local_y, mask, FALSE,
													mCargoTypes[mCurItemIndex],
													(void*)cargo,
													&item_acceptance,
													mToolTipMsg);
				if (handled)
				{
					// use sort order to determine priority of acceptance
					*acceptance = (EAcceptance)llmin((U32)item_acceptance, (U32)*acceptance);
				}
			}
			else
			{
				return;		
			}
		}

		// all objects passed, go ahead and perform drop if necessary
		if (handled && drop && (U32)*acceptance >= ACCEPT_YES_COPY_SINGLE)
		{
			if ((U32)*acceptance < ACCEPT_YES_COPY_MULTI &&
			    mCargoIDs.size() > 1)
			{
				// tried to give multi-cargo to a single-acceptor - refuse and return.
				*acceptance = ACCEPT_NO;
				return;
			}

			for (mCurItemIndex = 0; mCurItemIndex < (S32)mCargoIDs.size(); mCurItemIndex++)
			{
				LLInventoryObject* cargo = locateInventory(item, cat);

				if (cargo)
				{
					S32 local_x, local_y;

					EAcceptance item_acceptance;
					top_view->screenPointToLocal( x, y, &local_x, &local_y );
					handled = handled && top_view->handleDragAndDrop(local_x, local_y, mask, TRUE,
														mCargoTypes[mCurItemIndex],
														(void*)cargo,
														&item_acceptance,
														mToolTipMsg);
				}
			}
		}
		if (handled)
		{
			mLastAccept = (EAcceptance)*acceptance;
		}
	}

	if (!handled)
	{
		handled = TRUE;

		LLRootView* root_view = gViewerWindow->getRootView();

		for (mCurItemIndex = 0; mCurItemIndex < (S32)mCargoIDs.size(); mCurItemIndex++)
		{
			LLInventoryObject* cargo = locateInventory(item, cat);

			// fix for EXT-3191
			if (NULL == cargo) return;

			EAcceptance item_acceptance = ACCEPT_NO;
			handled = handled && root_view->handleDragAndDrop(x, y, mask, FALSE,
												mCargoTypes[mCurItemIndex],
												(void*)cargo,
												&item_acceptance,
												mToolTipMsg);
			if (handled)
			{
				// use sort order to determine priority of acceptance
				*acceptance = (EAcceptance)llmin((U32)item_acceptance, (U32)*acceptance);
			}
		}
		// all objects passed, go ahead and perform drop if necessary
		if (handled && drop && (U32)*acceptance > ACCEPT_NO_LOCKED)
		{	
			if ((U32)*acceptance < ACCEPT_YES_COPY_MULTI &&
			    mCargoIDs.size() > 1)
			{
				// tried to give multi-cargo to a single-acceptor - refuse and return.
				*acceptance = ACCEPT_NO;
				return;
			}

			for (mCurItemIndex = 0; mCurItemIndex < (S32)mCargoIDs.size(); mCurItemIndex++)
			{
				LLInventoryObject* cargo = locateInventory(item, cat);

				if (cargo)
				{
					EAcceptance item_acceptance;
					handled = handled && root_view->handleDragAndDrop(x, y, mask, TRUE,
											  mCargoTypes[mCurItemIndex],
											  (void*)cargo,
											  &item_acceptance,
											  mToolTipMsg);
				}
			}
		}

		if (handled)
		{
			mLastAccept = (EAcceptance)*acceptance;
		}
	}

	if (!handled)
	{
		// Disallow drag and drop to 3D from the outbox
		const LLUUID outbox_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_OUTBOX, false, false);
		if (outbox_id.notNull())
		{
			for (S32 item_index = 0; item_index < (S32)mCargoIDs.size(); item_index++)
			{
				if (gInventory.isObjectDescendentOf(mCargoIDs[item_index], outbox_id))
				{
					*acceptance = ACCEPT_NO;
					mToolTipMsg = LLTrans::getString("TooltipOutboxDragToWorld");
					return;
				}
			}
		}
		
		dragOrDrop3D( x, y, mask, drop, acceptance );
	}
}

void LLToolDragAndDrop::dragOrDrop3D( S32 x, S32 y, MASK mask, BOOL drop, EAcceptance* acceptance )
{
	mDrop = drop;
	if (mDrop)
	{
		// don't allow drag and drop onto transparent objects
		pick(gViewerWindow->pickImmediate(x, y, FALSE));
	}
	else
	{
		// don't allow drag and drop onto transparent objects
		gViewerWindow->pickAsync(x, y, mask, pickCallback, FALSE);
	}

	*acceptance = mLastAccept;
}

void LLToolDragAndDrop::pickCallback(const LLPickInfo& pick_info)
{
	if (getInstance() != NULL)
	{
		getInstance()->pick(pick_info);
	}
}

void LLToolDragAndDrop::pick(const LLPickInfo& pick_info)
{
	EDropTarget target = DT_NONE;
	S32	hit_face = -1;

	LLViewerObject* hit_obj = pick_info.getObject();
	LLSelectMgr::getInstance()->unhighlightAll();
	bool highlight_object = false;
	// Treat attachments as part of the avatar they are attached to.
	if (hit_obj != NULL)
	{
		// don't allow drag and drop on grass, trees, etc.
		if (pick_info.mPickType == LLPickInfo::PICK_FLORA)
		{
			mCursor = UI_CURSOR_NO;
			gViewerWindow->getWindow()->setCursor( mCursor );
			return;
		}

		if (hit_obj->isAttachment() && !hit_obj->isHUDAttachment())
		{
			LLVOAvatar* avatar = LLVOAvatar::findAvatarFromAttachment( hit_obj );
			if (!avatar)
			{
				mLastAccept = ACCEPT_NO;
				mCursor = UI_CURSOR_NO;
				gViewerWindow->getWindow()->setCursor( mCursor );
				return;
			}
			hit_obj = avatar;
		}

		if (hit_obj->isAvatar())
		{
			if (((LLVOAvatar*) hit_obj)->isSelf())
			{
				target = DT_SELF;
				hit_face = -1;
			}
			else
			{
				target = DT_AVATAR;
				hit_face = -1;
			}
		}
		else
		{
			target = DT_OBJECT;
			hit_face = pick_info.mObjectFace;
			highlight_object = true;
		}
	}
	else if (pick_info.mPickType == LLPickInfo::PICK_LAND)
	{
		target = DT_LAND;
		hit_face = -1;
	}

	mLastAccept = ACCEPT_YES_MULTI;

	for (mCurItemIndex = 0; mCurItemIndex < (S32)mCargoIDs.size(); mCurItemIndex++)
	{
		const S32 item_index = mCurItemIndex;
		const EDragAndDropType dad_type = mCargoTypes[item_index];
		// Call the right implementation function
		mLastAccept = (EAcceptance)llmin(
			(U32)mLastAccept,
			(U32)callMemberFunction(*this, 
									LLDragAndDropDictionary::instance().get(dad_type, target))
				(hit_obj, hit_face, pick_info.mKeyMask, FALSE));
	}

	if (mDrop && ((U32)mLastAccept >= ACCEPT_YES_COPY_SINGLE))
	{
		// if target allows multi-drop or there is only one item being dropped, go ahead
		if ((mLastAccept >= ACCEPT_YES_COPY_MULTI) || (mCargoIDs.size() == 1))
		{
			// Target accepts multi, or cargo is a single-drop
			for (mCurItemIndex = 0; mCurItemIndex < (S32)mCargoIDs.size(); mCurItemIndex++)
			{
				const S32 item_index = mCurItemIndex;
				const EDragAndDropType dad_type = mCargoTypes[item_index];
				// Call the right implementation function
				(U32)callMemberFunction(*this,
										LLDragAndDropDictionary::instance().get(dad_type, target))
					(hit_obj, hit_face, pick_info.mKeyMask, TRUE);
			}
		}
		else
		{
			// Target does not accept multi, but cargo is multi
			mLastAccept = ACCEPT_NO;
		}
	}

	if (highlight_object && mLastAccept > ACCEPT_NO_LOCKED)
	{
		// if any item being dragged will be applied to the object under our cursor
		// highlight that object
		for (S32 i = 0; i < (S32)mCargoIDs.size(); i++)
		{
			if (mCargoTypes[i] != DAD_OBJECT || (pick_info.mKeyMask & MASK_CONTROL))
			{
				LLSelectMgr::getInstance()->highlightObjectAndFamily(hit_obj);
				break;
			}
		}
	}
	ECursorType cursor = acceptanceToCursor( mLastAccept );
	gViewerWindow->getWindow()->setCursor( cursor );

	mLastHitPos = pick_info.mPosGlobal;
	mLastCameraPos = gAgentCamera.getCameraPositionGlobal();
}

// static
BOOL LLToolDragAndDrop::handleDropTextureProtections(LLViewerObject* hit_obj,
													 LLInventoryItem* item,
													 LLToolDragAndDrop::ESource source,
													 const LLUUID& src_id)
{
	// Always succeed if....
	// texture is from the library 
	// or already in the contents of the object
	if (SOURCE_LIBRARY == source)
	{
		// dropping a texture from the library always just works.
		return TRUE;
	}

	// In case the inventory has not been updated (e.g. due to some recent operation
	// causing a dirty inventory), stall the user while fetching the inventory.
	if (hit_obj->isInventoryDirty())
	{
		hit_obj->fetchInventoryFromServer();
		LLSD args;
		args["ERROR_MESSAGE"] = "Unable to add texture.\nPlease wait a few seconds and try again.";
		LLNotificationsUtil::add("ErrorMessage", args);
		return FALSE;
	}
	if (hit_obj->getInventoryItemByAsset(item->getAssetUUID()))
	{
		// if the asset is already in the object's inventory 
		// then it can always be added to a side.
		// This saves some work if the task's inventory is already loaded
		// and ensures that the texture item is only added once.
		return TRUE;
	}

	if (!item) return FALSE;
	
	LLPointer<LLViewerInventoryItem> new_item = new LLViewerInventoryItem(item);
	if (!item->getPermissions().allowOperationBy(PERM_COPY, gAgent.getID()))
	{
		// Check that we can add the texture as inventory to the object
		if (willObjectAcceptInventory(hit_obj,item) < ACCEPT_YES_COPY_SINGLE )
		{
			return FALSE;
		}
		// make sure the object has the texture in it's inventory.
		if (SOURCE_AGENT == source)
		{
			// Remove the texture from local inventory. The server
			// will actually remove the item from agent inventory.
			gInventory.deleteObject(item->getUUID());
			gInventory.notifyObservers();
		}
		else if (SOURCE_WORLD == source)
		{
			// *FIX: if the objects are in different regions, and the
			// source region has crashed, you can bypass these
			// permissions.
			LLViewerObject* src_obj = gObjectList.findObject(src_id);
			if (src_obj)
			{
				src_obj->removeInventory(item->getUUID());
			}
			else
			{
				llwarns << "Unable to find source object." << llendl;
				return FALSE;
			}
		}
		// Add the texture item to the target object's inventory.
		hit_obj->updateInventory(new_item, TASK_INVENTORY_ITEM_KEY, true);
 		// TODO: Check to see if adding the item was successful; if not, then
		// we should return false here.
	}
	else if (!item->getPermissions().allowOperationBy(PERM_TRANSFER,
													 gAgent.getID()))
	{
		// Check that we can add the texture as inventory to the object
		if (willObjectAcceptInventory(hit_obj,item) < ACCEPT_YES_COPY_SINGLE )
		{
			return FALSE;
		}
		// *FIX: may want to make sure agent can paint hit_obj.

		// Add the texture item to the target object's inventory.
		hit_obj->updateInventory(new_item, TASK_INVENTORY_ITEM_KEY, true);
		// Force the object to update its refetch its inventory so it has this texture.
		hit_obj->fetchInventoryFromServer();
 		// TODO: Check to see if adding the item was successful; if not, then
		// we should return false here.
	}
	return TRUE;
}

void LLToolDragAndDrop::dropTextureAllFaces(LLViewerObject* hit_obj,
											LLInventoryItem* item,
											LLToolDragAndDrop::ESource source,
											const LLUUID& src_id)
{
	if (!item)
	{
		llwarns << "LLToolDragAndDrop::dropTextureAllFaces no texture item." << llendl;
		return;
	}
	LLUUID asset_id = item->getAssetUUID();
	BOOL success = handleDropTextureProtections(hit_obj, item, source, src_id);
	if (!success)
	{
		return;
	}
	LLViewerTexture* image = LLViewerTextureManager::getFetchedTexture(asset_id);
	LLViewerStats::getInstance()->incStat(LLViewerStats::ST_EDIT_TEXTURE_COUNT );
	S32 num_faces = hit_obj->getNumTEs();
	for( S32 face = 0; face < num_faces; face++ )
	{

		// update viewer side image in anticipation of update from simulator
		hit_obj->setTEImage(face, image);
		dialog_refresh_all();
	}
	// send the update to the simulator
	hit_obj->sendTEUpdate();
}

void LLToolDragAndDrop::dropMesh(LLViewerObject* hit_obj,
								 LLInventoryItem* item,
								 LLToolDragAndDrop::ESource source,
								 const LLUUID& src_id)
{
	if (!item)
	{
		llwarns << "no inventory item." << llendl;
		return;
	}
	LLUUID asset_id = item->getAssetUUID();
	BOOL success = handleDropTextureProtections(hit_obj, item, source, src_id);
	if(!success)
	{
		return;
	}

	LLSculptParams sculpt_params;
	sculpt_params.setSculptTexture(asset_id);
	sculpt_params.setSculptType(LL_SCULPT_TYPE_MESH);
	hit_obj->setParameterEntry(LLNetworkData::PARAMS_SCULPT, sculpt_params, TRUE);
	
	dialog_refresh_all();
}

/*
void LLToolDragAndDrop::dropTextureOneFaceAvatar(LLVOAvatar* avatar, S32 hit_face, LLInventoryItem* item)
{
	if (hit_face == -1) return;
	LLViewerTexture* image = LLViewerTextureManager::getFetchedTexture(item->getAssetUUID());
	
	avatar->userSetOptionalTE( hit_face, image);
}
*/

void LLToolDragAndDrop::dropTextureOneFace(LLViewerObject* hit_obj,
										   S32 hit_face,
										   LLInventoryItem* item,
										   LLToolDragAndDrop::ESource source,
										   const LLUUID& src_id)
{
	if (hit_face == -1) return;
	if (!item)
	{
		llwarns << "LLToolDragAndDrop::dropTextureOneFace no texture item." << llendl;
		return;
	}
	LLUUID asset_id = item->getAssetUUID();
	BOOL success = handleDropTextureProtections(hit_obj, item, source, src_id);
	if (!success)
	{
		return;
	}
	// update viewer side image in anticipation of update from simulator
	LLViewerTexture* image = LLViewerTextureManager::getFetchedTexture(asset_id);
	LLViewerStats::getInstance()->incStat(LLViewerStats::ST_EDIT_TEXTURE_COUNT );
	hit_obj->setTEImage(hit_face, image);
	dialog_refresh_all();

	// send the update to the simulator
	hit_obj->sendTEUpdate();
}


void LLToolDragAndDrop::dropScript(LLViewerObject* hit_obj,
								   LLInventoryItem* item,
								   BOOL active,
								   ESource source,
								   const LLUUID& src_id)
{
	// *HACK: In order to resolve SL-22177, we need to block drags
	// from notecards and objects onto other objects.
	if ((SOURCE_WORLD == LLToolDragAndDrop::getInstance()->mSource)
	   || (SOURCE_NOTECARD == LLToolDragAndDrop::getInstance()->mSource))
	{
		llwarns << "Call to LLToolDragAndDrop::dropScript() from world"
			<< " or notecard." << llendl;
		return;
	}
	if (hit_obj && item)
	{
		LLPointer<LLViewerInventoryItem> new_script = new LLViewerInventoryItem(item);
		if (!item->getPermissions().allowCopyBy(gAgent.getID()))
		{
			if (SOURCE_AGENT == source)
			{
				// Remove the script from local inventory. The server
				// will actually remove the item from agent inventory.
				gInventory.deleteObject(item->getUUID());
				gInventory.notifyObservers();
			}
			else if (SOURCE_WORLD == source)
			{
				// *FIX: if the objects are in different regions, and
				// the source region has crashed, you can bypass
				// these permissions.
				LLViewerObject* src_obj = gObjectList.findObject(src_id);
				if (src_obj)
				{
					src_obj->removeInventory(item->getUUID());
				}
				else
				{
					llwarns << "Unable to find source object." << llendl;
					return;
				}
			}
		}
		hit_obj->saveScript(new_script, active, true);
		gFloaterTools->dirty();

		// VEFFECT: SetScript
		LLHUDEffectSpiral *effectp = (LLHUDEffectSpiral *)LLHUDManager::getInstance()->createViewerEffect(LLHUDObject::LL_HUD_EFFECT_BEAM, TRUE);
		effectp->setSourceObject(gAgentAvatarp);
		effectp->setTargetObject(hit_obj);
		effectp->setDuration(LL_HUD_DUR_SHORT);
		effectp->setColor(LLColor4U(gAgent.getEffectColor()));
	}
}

void LLToolDragAndDrop::dropObject(LLViewerObject* raycast_target,
				   BOOL bypass_sim_raycast,
				   BOOL from_task_inventory,
				   BOOL remove_from_inventory)
{
	LLViewerRegion* regionp = LLWorld::getInstance()->getRegionFromPosGlobal(mLastHitPos);
	if (!regionp)
	{
		llwarns << "Couldn't find region to rez object" << llendl;
		return;
	}

	//llinfos << "Rezzing object" << llendl;
	make_ui_sound("UISndObjectRezIn");
	LLViewerInventoryItem* item;
	LLViewerInventoryCategory* cat;
	locateInventory(item, cat);
	if (!item || !item->isFinished()) return;
	
	//if (regionp
	//	&& (regionp->getRegionFlags() & REGION_FLAGS_SANDBOX))
	//{
	//	LLFirstUse::useSandbox();
	//}
	// check if it cannot be copied, and mark as remove if it is -
	// this will remove the object from inventory after rez. Only
	// bother with this check if we would not normally remove from
	// inventory.
	if (!remove_from_inventory
		&& !item->getPermissions().allowCopyBy(gAgent.getID()))
	{
		remove_from_inventory = TRUE;
	}

	// Limit raycast to a single object.  
	// Speeds up server raycast + avoid problems with server ray
	// hitting objects that were clipped by the near plane or culled
	// on the viewer.
	LLUUID ray_target_id;
	if (raycast_target)
	{
		ray_target_id = raycast_target->getID();
	}
	else
	{
		ray_target_id.setNull();
	}

	// Check if it's in the trash.
	bool is_in_trash = false;
	const LLUUID trash_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_TRASH);
	if (gInventory.isObjectDescendentOf(item->getUUID(), trash_id))
	{
		is_in_trash = true;
		remove_from_inventory = TRUE;
	}

	LLUUID source_id = from_task_inventory ? mSourceID : LLUUID::null;

	// Select the object only if we're editing.
	BOOL rez_selected = LLToolMgr::getInstance()->inEdit();


	LLVector3 ray_start = regionp->getPosRegionFromGlobal(mLastCameraPos);
	LLVector3 ray_end   = regionp->getPosRegionFromGlobal(mLastHitPos);
	// currently the ray's end point is an approximation,
	// and is sometimes too short (causing failure.)  so we
	// double the ray's length:
	if (bypass_sim_raycast == FALSE)
	{
		LLVector3 ray_direction = ray_start - ray_end;
		ray_end = ray_end - ray_direction;
	}
	
	
	// Message packing code should be it's own uninterrupted block
	LLMessageSystem* msg = gMessageSystem;
	if (mSource == SOURCE_NOTECARD)
	{
		msg->newMessageFast(_PREHASH_RezObjectFromNotecard);
	}
	else
	{
		msg->newMessageFast(_PREHASH_RezObject);
	}
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID,  gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID,  gAgent.getSessionID());
	msg->addUUIDFast(_PREHASH_GroupID, gAgent.getGroupID());

	msg->nextBlock("RezData");
	// if it's being rezzed from task inventory, we need to enable
	// saving it back into the task inventory.
	// *FIX: We can probably compress this to a single byte, since I
	// think folderid == mSourceID. This will be a later
	// optimization.
	msg->addUUIDFast(_PREHASH_FromTaskID, source_id);
	msg->addU8Fast(_PREHASH_BypassRaycast, (U8) bypass_sim_raycast);
	msg->addVector3Fast(_PREHASH_RayStart, ray_start);
	msg->addVector3Fast(_PREHASH_RayEnd, ray_end);
	msg->addUUIDFast(_PREHASH_RayTargetID, ray_target_id );
	msg->addBOOLFast(_PREHASH_RayEndIsIntersection, FALSE);
	msg->addBOOLFast(_PREHASH_RezSelected, rez_selected);
	msg->addBOOLFast(_PREHASH_RemoveItem, remove_from_inventory);

	// deal with permissions slam logic
	pack_permissions_slam(msg, item->getFlags(), item->getPermissions());

	LLUUID folder_id = item->getParentUUID();
	if ((SOURCE_LIBRARY == mSource) || (is_in_trash))
	{
		// since it's coming from the library or trash, we want to not
		// 'take' it back to the same place.
		item->setParent(LLUUID::null);
		// *TODO this code isn't working - the parent (FolderID) is still
		// set when the object is "taken".  so code on the "take" side is
		// checking for trash and library as well (llviewermenu.cpp)
	}
	if (mSource == SOURCE_NOTECARD)
	{
		msg->nextBlockFast(_PREHASH_NotecardData);
		msg->addUUIDFast(_PREHASH_NotecardItemID, mSourceID);
		msg->addUUIDFast(_PREHASH_ObjectID, mObjectID);
		msg->nextBlockFast(_PREHASH_InventoryData);
		msg->addUUIDFast(_PREHASH_ItemID, item->getUUID());
	}
	else
	{
		msg->nextBlockFast(_PREHASH_InventoryData);
		item->packMessage(msg);
	}
	msg->sendReliable(regionp->getHost());
	// back out the change. no actual internal changes take place.
	item->setParent(folder_id); 

	// If we're going to select it, get ready for the incoming
	// selected object.
	if (rez_selected)
	{
		LLSelectMgr::getInstance()->deselectAll();
		gViewerWindow->getWindow()->incBusyCount();
	}

	if (remove_from_inventory)
	{
		// Delete it from inventory immediately so that users cannot
		// easily bypass copy protection in laggy situations. If the
		// rez fails, we will put it back on the server.
		gInventory.deleteObject(item->getUUID());
		gInventory.notifyObservers();
	}

	// VEFFECT: DropObject
	LLHUDEffectSpiral *effectp = (LLHUDEffectSpiral *)LLHUDManager::getInstance()->createViewerEffect(LLHUDObject::LL_HUD_EFFECT_BEAM, TRUE);
	effectp->setSourceObject(gAgentAvatarp);
	effectp->setPositionGlobal(mLastHitPos);
	effectp->setDuration(LL_HUD_DUR_SHORT);
	effectp->setColor(LLColor4U(gAgent.getEffectColor()));

	LLViewerStats::getInstance()->incStat(LLViewerStats::ST_REZ_COUNT);
}

void LLToolDragAndDrop::dropInventory(LLViewerObject* hit_obj,
									  LLInventoryItem* item,
									  LLToolDragAndDrop::ESource source,
									  const LLUUID& src_id)
{
	// *HACK: In order to resolve SL-22177, we need to block drags
	// from notecards and objects onto other objects.
	if ((SOURCE_WORLD == LLToolDragAndDrop::getInstance()->mSource)
	   || (SOURCE_NOTECARD == LLToolDragAndDrop::getInstance()->mSource))
	{
		llwarns << "Call to LLToolDragAndDrop::dropInventory() from world"
			<< " or notecard." << llendl;
		return;
	}

	LLPointer<LLViewerInventoryItem> new_item = new LLViewerInventoryItem(item);
	time_t creation_date = time_corrected();
	new_item->setCreationDate(creation_date);

	if (!item->getPermissions().allowCopyBy(gAgent.getID()))
	{
		if (SOURCE_AGENT == source)
		{
			// Remove the inventory item from local inventory. The
			// server will actually remove the item from agent
			// inventory.
			gInventory.deleteObject(item->getUUID());
			gInventory.notifyObservers();
		}
		else if (SOURCE_WORLD == source)
		{
			// *FIX: if the objects are in different regions, and the
			// source region has crashed, you can bypass these
			// permissions.
			LLViewerObject* src_obj = gObjectList.findObject(src_id);
			if (src_obj)
			{
				src_obj->removeInventory(item->getUUID());
			}
			else
			{
				llwarns << "Unable to find source object." << llendl;
				return;
			}
		}
	}
	hit_obj->updateInventory(new_item, TASK_INVENTORY_ITEM_KEY, true);
	if (LLFloaterReg::instanceVisible("build"))
	{
		// *FIX: only show this if panel not expanded?
		LLFloaterReg::showInstance("build", "Content");
	}

	// VEFFECT: AddToInventory
	LLHUDEffectSpiral *effectp = (LLHUDEffectSpiral *)LLHUDManager::getInstance()->createViewerEffect(LLHUDObject::LL_HUD_EFFECT_BEAM, TRUE);
	effectp->setSourceObject(gAgentAvatarp);
	effectp->setTargetObject(hit_obj);
	effectp->setDuration(LL_HUD_DUR_SHORT);
	effectp->setColor(LLColor4U(gAgent.getEffectColor()));
	gFloaterTools->dirty();
}

// accessor that looks at permissions, copyability, and names of
// inventory items to determine if a drop would be ok.
EAcceptance LLToolDragAndDrop::willObjectAcceptInventory(LLViewerObject* obj, LLInventoryItem* item)
{
	// check the basics
	if (!item || !obj) return ACCEPT_NO;
	// HACK: downcast
	LLViewerInventoryItem* vitem = (LLViewerInventoryItem*)item;
	if (!vitem->isFinished()) return ACCEPT_NO;
	if (vitem->getIsLinkType()) return ACCEPT_NO; // No giving away links

	// deny attempts to drop from an object onto itself. This is to
	// help make sure that drops that are from an object to an object
	// don't have to worry about order of evaluation. Think of this
	// like check for self in assignment.
	if(obj->getID() == item->getParentUUID())
	{
		return ACCEPT_NO;
	}
	
	//BOOL copy = (perm.allowCopyBy(gAgent.getID(),
	//							  gAgent.getGroupID())
	//			 && (obj->mPermModify || obj->mFlagAllowInventoryAdd));
	BOOL worn = FALSE;
	LLVOAvatarSelf* my_avatar = NULL;
	switch(item->getType())
	{
	case LLAssetType::AT_OBJECT:
		my_avatar = gAgentAvatarp;
		if(my_avatar && my_avatar->isWearingAttachment(item->getUUID()))
		{
				worn = TRUE;
		}
		break;
	case LLAssetType::AT_BODYPART:
	case LLAssetType::AT_CLOTHING:
		if(gAgentWearables.isWearingItem(item->getUUID()))
		{
			worn = TRUE;
		}
		break;
	case LLAssetType::AT_CALLINGCARD:
		// Calling Cards in object are disabled for now
		// because of incomplete LSL support. See STORM-1117.
		return ACCEPT_NO;
	default:
			break;
	}
	const LLPermissions& perm = item->getPermissions();
	BOOL modify = (obj->permModify() || obj->flagAllowInventoryAdd());
	BOOL transfer = FALSE;
	if((obj->permYouOwner() && (perm.getOwner() == gAgent.getID()))
	   || perm.allowOperationBy(PERM_TRANSFER, gAgent.getID()))
	{
		transfer = TRUE;
	}
	BOOL volume = (LL_PCODE_VOLUME == obj->getPCode());
	BOOL attached = obj->isAttachment();
	BOOL unrestricted = ((perm.getMaskBase() & PERM_ITEM_UNRESTRICTED) == PERM_ITEM_UNRESTRICTED) ? TRUE : FALSE;
	if(attached && !unrestricted)
	{
		return ACCEPT_NO_LOCKED;
	}
	else if(modify && transfer && volume && !worn)
	{
		return ACCEPT_YES_MULTI;
	}
	else if(!modify)
	{
		return ACCEPT_NO_LOCKED;
	}
	return ACCEPT_NO;
}


static void give_inventory_cb(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	// if Cancel pressed
	if (option == 1)
	{
		return;
	}

	LLSD payload = notification["payload"];
	const LLUUID& session_id = payload["session_id"];
	const LLUUID& agent_id = payload["agent_id"];
	LLViewerInventoryItem * inv_item =  gInventory.getItem(payload["item_id"]);
	if (NULL == inv_item)
	{
		llassert(NULL != inv_item);
		return;
	}

	if (LLGiveInventory::doGiveInventoryItem(agent_id, inv_item, session_id))
	{
		if ("avatarpicker" == payload["d&d_dest"].asString())
		{
			LLFloaterReg::hideInstance("avatar_picker");
		}
		LLNotificationsUtil::add("ItemsShared");
	}
}

static void show_item_sharing_confirmation(const std::string name,
					   LLViewerInventoryItem* inv_item,
					   const LLSD& dest,
					   const LLUUID& dest_agent,
					   const LLUUID& session_id = LLUUID::null)
{
	if (!inv_item)
	{
		llassert(NULL != inv_item);
		return;
	}

	LLSD substitutions;
	substitutions["RESIDENTS"] = name;
	substitutions["ITEMS"] = inv_item->getName();
	LLSD payload;
	payload["agent_id"] = dest_agent;
	payload["item_id"] = inv_item->getUUID();
	payload["session_id"] = session_id;
	payload["d&d_dest"] = dest.asString();
	LLNotificationsUtil::add("ShareItemsConfirmation", substitutions, payload, &give_inventory_cb);
}

static void get_name_cb(const LLUUID& id,
						const std::string& full_name,
						LLViewerInventoryItem* inv_item,
						const LLSD& dest,
						const LLUUID& dest_agent)
{
	show_item_sharing_confirmation(full_name,
								   inv_item,
								   dest,
								   id,
								   LLUUID::null);
}

// function used as drag-and-drop handler for simple agent give inventory requests
//static
bool LLToolDragAndDrop::handleGiveDragAndDrop(LLUUID dest_agent, LLUUID session_id, BOOL drop,
											  EDragAndDropType cargo_type,
											  void* cargo_data,
											  EAcceptance* accept,
											  const LLSD& dest)
{
	// check the type
	switch(cargo_type)
	{
	case DAD_TEXTURE:
	case DAD_SOUND:
	case DAD_LANDMARK:
	case DAD_SCRIPT:
	case DAD_OBJECT:
	case DAD_NOTECARD:
	case DAD_CLOTHING:
	case DAD_BODYPART:
	case DAD_ANIMATION:
	case DAD_GESTURE:
	case DAD_CALLINGCARD:
	case DAD_MESH:
	{
		LLViewerInventoryItem* inv_item = (LLViewerInventoryItem*)cargo_data;
		if(gInventory.getItem(inv_item->getUUID())
			&& LLGiveInventory::isInventoryGiveAcceptable(inv_item))
		{
			// *TODO: get multiple object transfers working
			*accept = ACCEPT_YES_COPY_SINGLE;
			if(drop)
			{
				LLIMModel::LLIMSession * session = LLIMModel::instance().findIMSession(session_id);

				// If no IM session found get the destination agent's name by id.
				if (NULL == session)
				{
					std::string fullname;

					// If destination agent's name is found in cash proceed to showing the confirmation dialog.
					// Otherwise set up a callback to show the dialog when the name arrives.
					if (gCacheName->getFullName(dest_agent, fullname))
					{
						show_item_sharing_confirmation(fullname, inv_item, dest, dest_agent, LLUUID::null);
					}
					else
					{
						gCacheName->get(dest_agent, false, boost::bind(&get_name_cb, _1, _2, inv_item, dest, dest_agent));
					}

					return true;
				}

				// If an IM session with destination agent is found item offer will be logged in this session.
				show_item_sharing_confirmation(session->mName, inv_item, dest, dest_agent, session_id);
			}
		}
		else
		{
			// It's not in the user's inventory (it's probably
			// in an object's contents), so disallow dragging
			// it here.  You can't give something you don't
			// yet have.
			*accept = ACCEPT_NO;
		}
		break;
	}
	case DAD_CATEGORY:
	{
		LLViewerInventoryCategory* inv_cat = (LLViewerInventoryCategory*)cargo_data;
		if( gInventory.getCategory( inv_cat->getUUID() ) )
		{
			// *TODO: get multiple object transfers working
			*accept = ACCEPT_YES_COPY_SINGLE;
			if(drop)
			{
				LLGiveInventory::doGiveInventoryCategory(dest_agent, inv_cat, session_id);
			}
		}
		else
		{
			// It's not in the user's inventory (it's probably
			// in an object's contents), so disallow dragging
			// it here.  You can't give something you don't
			// yet have.
			*accept = ACCEPT_NO;
		}
		break;
	}
	default:
		*accept = ACCEPT_NO;
		break;
	}

	return TRUE;
}



///
/// Methods called in the drag & drop array
///

EAcceptance LLToolDragAndDrop::dad3dNULL(
	LLViewerObject*, S32, MASK, BOOL)
{
	lldebugs << "LLToolDragAndDrop::dad3dNULL()" << llendl;
	return ACCEPT_NO;
}

EAcceptance LLToolDragAndDrop::dad3dRezAttachmentFromInv(
	LLViewerObject* obj, S32 face, MASK mask, BOOL drop)
{
	lldebugs << "LLToolDragAndDrop::dad3dRezAttachmentFromInv()" << llendl;
	// must be in the user's inventory
	if(mSource != SOURCE_AGENT && mSource != SOURCE_LIBRARY)
	{
		return ACCEPT_NO;
	}

	LLViewerInventoryItem* item;
	LLViewerInventoryCategory* cat;
	locateInventory(item, cat);
	if (!item || !item->isFinished()) return ACCEPT_NO;

	// must not be in the trash
	const LLUUID trash_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_TRASH);
	if( gInventory.isObjectDescendentOf( item->getUUID(), trash_id ) )
	{
		return ACCEPT_NO;
	}

	// must not be already wearing it
	LLVOAvatarSelf* avatar = gAgentAvatarp;
	if( !avatar || avatar->isWearingAttachment(item->getUUID()) )
	{
		return ACCEPT_NO;
	}

	const LLUUID &outbox_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_OUTBOX, false);
	if(gInventory.isObjectDescendentOf(item->getUUID(), outbox_id))
	{
		return ACCEPT_NO;
	}


	if( drop )
	{
		if(mSource == SOURCE_LIBRARY)
		{
			LLPointer<LLInventoryCallback> cb = new RezAttachmentCallback(0);
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
			rez_attachment(item, 0);
		}
	}
	return ACCEPT_YES_SINGLE;
}


EAcceptance LLToolDragAndDrop::dad3dRezObjectOnLand(
	LLViewerObject* obj, S32 face, MASK mask, BOOL drop)
{
	if (mSource == SOURCE_WORLD)
	{
		return dad3dRezFromObjectOnLand(obj, face, mask, drop);
	}

	lldebugs << "LLToolDragAndDrop::dad3dRezObjectOnLand()" << llendl;
	LLViewerInventoryItem* item;
	LLViewerInventoryCategory* cat;
	locateInventory(item, cat);
	if (!item || !item->isFinished()) return ACCEPT_NO;

	LLVOAvatarSelf* my_avatar = gAgentAvatarp;
	if( !my_avatar || my_avatar->isWearingAttachment( item->getUUID() ) )
	{
		return ACCEPT_NO;
	}

	EAcceptance accept;
	BOOL remove_inventory;

	// Get initial settings based on shift key
	if (mask & MASK_SHIFT)
	{
		// For now, always make copy
		//accept = ACCEPT_YES_SINGLE;
		//remove_inventory = TRUE;
		accept = ACCEPT_YES_COPY_SINGLE;
		remove_inventory = FALSE;
	}
	else
	{
		accept = ACCEPT_YES_COPY_SINGLE;
		remove_inventory = FALSE;
	}

	// check if the item can be copied. If not, send that to the sim
	// which will remove the inventory item.
	if(!item->getPermissions().allowCopyBy(gAgent.getID()))
	{
		accept = ACCEPT_YES_SINGLE;
		remove_inventory = TRUE;
	}

	// Check if it's in the trash.
	const LLUUID trash_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_TRASH);
	if(gInventory.isObjectDescendentOf(item->getUUID(), trash_id))
	{
		accept = ACCEPT_YES_SINGLE;
		remove_inventory = TRUE;
	}

	if(drop)
	{
		dropObject(obj, TRUE, FALSE, remove_inventory);
	}

	return accept;
}

EAcceptance LLToolDragAndDrop::dad3dRezObjectOnObject(
	LLViewerObject* obj, S32 face, MASK mask, BOOL drop)
{
	// handle objects coming from object inventory
	if (mSource == SOURCE_WORLD)
	{
		return dad3dRezFromObjectOnObject(obj, face, mask, drop);
	}

	lldebugs << "LLToolDragAndDrop::dad3dRezObjectOnObject()" << llendl;
	LLViewerInventoryItem* item;
	LLViewerInventoryCategory* cat;
	locateInventory(item, cat);
	if (!item || !item->isFinished()) return ACCEPT_NO;
	LLVOAvatarSelf* my_avatar = gAgentAvatarp;
	if( !my_avatar || my_avatar->isWearingAttachment( item->getUUID() ) )
	{
		return ACCEPT_NO;
	}

	if((mask & MASK_CONTROL))
	{
		// *HACK: In order to resolve SL-22177, we need to block drags
		// from notecards and objects onto other objects.
		if(mSource == SOURCE_NOTECARD)
		{
			return ACCEPT_NO;
		}

		EAcceptance rv = willObjectAcceptInventory(obj, item);
		if(drop && (ACCEPT_YES_SINGLE <= rv))
		{
			dropInventory(obj, item, mSource, mSourceID);
		}
		return rv;
	}
	
	EAcceptance accept;
	BOOL remove_inventory;

	if (mask & MASK_SHIFT)
	{
		// For now, always make copy
		//accept = ACCEPT_YES_SINGLE;
		//remove_inventory = TRUE;
		accept = ACCEPT_YES_COPY_SINGLE;
		remove_inventory = FALSE;
	}
	else
	{
		accept = ACCEPT_YES_COPY_SINGLE;
		remove_inventory = FALSE;
	}
	
	// check if the item can be copied. If not, send that to the sim
	// which will remove the inventory item.
	if(!item->getPermissions().allowCopyBy(gAgent.getID()))
	{
		accept = ACCEPT_YES_SINGLE;
		remove_inventory = TRUE;
	}

	// Check if it's in the trash.
	const LLUUID trash_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_TRASH);
	if(gInventory.isObjectDescendentOf(item->getUUID(), trash_id))
	{
		accept = ACCEPT_YES_SINGLE;
		remove_inventory = TRUE;
	}

	if(drop)
	{
		dropObject(obj, FALSE, FALSE, remove_inventory);
	}

	return accept;
}

EAcceptance LLToolDragAndDrop::dad3dRezScript(
	LLViewerObject* obj, S32 face, MASK mask, BOOL drop)
{
	lldebugs << "LLToolDragAndDrop::dad3dRezScript()" << llendl;

	// *HACK: In order to resolve SL-22177, we need to block drags
	// from notecards and objects onto other objects.
	if((SOURCE_WORLD == mSource) || (SOURCE_NOTECARD == mSource))
	{
		return ACCEPT_NO;
	}

	LLViewerInventoryItem* item;
	LLViewerInventoryCategory* cat;
	locateInventory(item, cat);
	if (!item || !item->isFinished()) return ACCEPT_NO;
	EAcceptance rv = willObjectAcceptInventory(obj, item);
	if(drop && (ACCEPT_YES_SINGLE <= rv))
	{
		// rez in the script active by default, rez in inactive if the
		// control key is being held down.
		BOOL active = ((mask & MASK_CONTROL) == 0);
	
		LLViewerObject* root_object = obj;
		if (obj && obj->getParent())
		{
			LLViewerObject* parent_obj = (LLViewerObject*)obj->getParent();
			if (!parent_obj->isAvatar())
			{
				root_object = parent_obj;
			}
		}

		dropScript(root_object, item, active, mSource, mSourceID);
	}	
	return rv;
}

EAcceptance LLToolDragAndDrop::dad3dApplyToObject(
	LLViewerObject* obj, S32 face, MASK mask, BOOL drop, EDragAndDropType cargo_type)
{
	lldebugs << "LLToolDragAndDrop::dad3dApplyToObject()" << llendl;

	// *HACK: In order to resolve SL-22177, we need to block drags
	// from notecards and objects onto other objects.
	if((SOURCE_WORLD == mSource) || (SOURCE_NOTECARD == mSource))
	{
		return ACCEPT_NO;
	}
	
	LLViewerInventoryItem* item;
	LLViewerInventoryCategory* cat;
	locateInventory(item, cat);
	if (!item || !item->isFinished()) return ACCEPT_NO;
	EAcceptance rv = willObjectAcceptInventory(obj, item);
	if((mask & MASK_CONTROL))
	{
		if((ACCEPT_YES_SINGLE <= rv) && drop)
		{
			dropInventory(obj, item, mSource, mSourceID);
		}
		return rv;
	}
	if(!obj->permModify())
	{
		return ACCEPT_NO_LOCKED;
	}
	//If texture !copyable don't texture or you'll never get it back.
	if(!item->getPermissions().allowCopyBy(gAgent.getID()))
	{
		return ACCEPT_NO;
	}

	if(drop && (ACCEPT_YES_SINGLE <= rv))
	{
		if (cargo_type == DAD_TEXTURE)
		{
			if((mask & MASK_SHIFT))
			{
				dropTextureAllFaces(obj, item, mSource, mSourceID);
			}
			else
			{
				dropTextureOneFace(obj, face, item, mSource, mSourceID);
			}
		}
		else if (cargo_type == DAD_MESH)
		{
			dropMesh(obj, item, mSource, mSourceID);
		}
		else
		{
			llwarns << "unsupported asset type" << llendl;
		}
		
		// VEFFECT: SetTexture
		LLHUDEffectSpiral *effectp = (LLHUDEffectSpiral *)LLHUDManager::getInstance()->createViewerEffect(LLHUDObject::LL_HUD_EFFECT_BEAM, TRUE);
		effectp->setSourceObject(gAgentAvatarp);
		effectp->setTargetObject(obj);
		effectp->setDuration(LL_HUD_DUR_SHORT);
		effectp->setColor(LLColor4U(gAgent.getEffectColor()));
	}

	// enable multi-drop, although last texture will win
	return ACCEPT_YES_MULTI;
}


EAcceptance LLToolDragAndDrop::dad3dTextureObject(
	LLViewerObject* obj, S32 face, MASK mask, BOOL drop)
{
	return dad3dApplyToObject(obj, face, mask, drop, DAD_TEXTURE);
}

EAcceptance LLToolDragAndDrop::dad3dMeshObject(
	LLViewerObject* obj, S32 face, MASK mask, BOOL drop)
{
	return dad3dApplyToObject(obj, face, mask, drop, DAD_MESH);
}


/*
EAcceptance LLToolDragAndDrop::dad3dTextureSelf(
	LLViewerObject* obj, S32 face, MASK mask, BOOL drop)
{
	lldebugs << "LLToolDragAndDrop::dad3dTextureAvatar()" << llendl;
	if(drop)
	{
		if( !(mask & MASK_SHIFT) )
		{
			dropTextureOneFaceAvatar( (LLVOAvatar*)obj, face, (LLInventoryItem*)mCargoData);
		}
	}
	return (mask & MASK_SHIFT) ? ACCEPT_NO : ACCEPT_YES_SINGLE;
}
*/

EAcceptance LLToolDragAndDrop::dad3dWearItem(
	LLViewerObject* obj, S32 face, MASK mask, BOOL drop)
{
	lldebugs << "LLToolDragAndDrop::dad3dWearItem()" << llendl;
	LLViewerInventoryItem* item;
	LLViewerInventoryCategory* cat;
	locateInventory(item, cat);
	if (!item || !item->isFinished()) return ACCEPT_NO;

	if(mSource == SOURCE_AGENT || mSource == SOURCE_LIBRARY)
	{
		// it's in the agent inventory
		const LLUUID trash_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_TRASH);
		if( gInventory.isObjectDescendentOf( item->getUUID(), trash_id ) )
		{
			return ACCEPT_NO;
		}

		if( drop )
		{
			// TODO: investigate wearables may not be loaded at this point EXT-8231

			LLAppearanceMgr::instance().wearItemOnAvatar(item->getUUID(),true, !(mask & MASK_CONTROL));
		}
		return ACCEPT_YES_MULTI;
	}
	else
	{
		// TODO: copy/move item to avatar's inventory and then wear it.
		return ACCEPT_NO;
	}
}

EAcceptance LLToolDragAndDrop::dad3dActivateGesture(
	LLViewerObject* obj, S32 face, MASK mask, BOOL drop)
{
	lldebugs << "LLToolDragAndDrop::dad3dActivateGesture()" << llendl;
	LLViewerInventoryItem* item;
	LLViewerInventoryCategory* cat;
	locateInventory(item, cat);
	if (!item || !item->isFinished()) return ACCEPT_NO;

	if(mSource == SOURCE_AGENT || mSource == SOURCE_LIBRARY)
	{
		// it's in the agent inventory
		const LLUUID trash_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_TRASH);
		if( gInventory.isObjectDescendentOf( item->getUUID(), trash_id ) )
		{
			return ACCEPT_NO;
		}

		if( drop )
		{
			LLUUID item_id;
			if(mSource == SOURCE_LIBRARY)
			{
				// create item based on that one, and put it on if that
				// was a success.
				LLPointer<LLInventoryCallback> cb = new ActivateGestureCallback();
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
				LLGestureMgr::instance().activateGesture(item->getUUID());
				gInventory.updateItem(item);
				gInventory.notifyObservers();
			}
		}
		return ACCEPT_YES_MULTI;
	}
	else
	{
		return ACCEPT_NO;
	}
}

EAcceptance LLToolDragAndDrop::dad3dWearCategory(
	LLViewerObject* obj, S32 face, MASK mask, BOOL drop)
{
	lldebugs << "LLToolDragAndDrop::dad3dWearCategory()" << llendl;
	LLViewerInventoryItem* item;
	LLViewerInventoryCategory* category;
	locateInventory(item, category);
	if(!category) return ACCEPT_NO;

	if (drop)
	{
		// TODO: investigate wearables may not be loaded at this point EXT-8231
	}

	if(mSource == SOURCE_AGENT)
	{
		const LLUUID trash_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_TRASH);
		if( gInventory.isObjectDescendentOf( category->getUUID(), trash_id ) )
		{
			return ACCEPT_NO;
		}

		const LLUUID &outbox_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_OUTBOX, false);
		if(gInventory.isObjectDescendentOf(category->getUUID(), outbox_id))
		{
			return ACCEPT_NO;
		}

		if(drop)
		{
			BOOL append = ( (mask & MASK_SHIFT) ? TRUE : FALSE );
			LLAppearanceMgr::instance().wearInventoryCategory(category, false, append);
		}
		return ACCEPT_YES_MULTI;
	}
	else if(mSource == SOURCE_LIBRARY)
	{
		if(drop)
		{
			LLAppearanceMgr::instance().wearInventoryCategory(category, true, false);
		}
		return ACCEPT_YES_MULTI;
	}
	else
	{
		// TODO: copy/move category to avatar's inventory and then wear it.
		return ACCEPT_NO;
	}
}


EAcceptance LLToolDragAndDrop::dad3dUpdateInventory(
	LLViewerObject* obj, S32 face, MASK mask, BOOL drop)
{
	lldebugs << "LLToolDragAndDrop::dadUpdateInventory()" << llendl;

	// *HACK: In order to resolve SL-22177, we need to block drags
	// from notecards and objects onto other objects.
	if((SOURCE_WORLD == mSource) || (SOURCE_NOTECARD == mSource))
	{
		return ACCEPT_NO;
	}

	LLViewerInventoryItem* item;
	LLViewerInventoryCategory* cat;
	locateInventory(item, cat);
	if (!item || !item->isFinished()) return ACCEPT_NO;
	LLViewerObject* root_object = obj;
	if (obj && obj->getParent())
	{
		LLViewerObject* parent_obj = (LLViewerObject*)obj->getParent();
		if (!parent_obj->isAvatar())
		{
			root_object = parent_obj;
		}
	}

	EAcceptance rv = willObjectAcceptInventory(root_object, item);
	if(root_object && drop && (ACCEPT_YES_COPY_SINGLE <= rv))
	{
		dropInventory(root_object, item, mSource, mSourceID);
	}
	return rv;
}

BOOL LLToolDragAndDrop::dadUpdateInventory(LLViewerObject* obj, BOOL drop)
{
	EAcceptance rv = dad3dUpdateInventory(obj, -1, MASK_NONE, drop);
	return (rv >= ACCEPT_YES_COPY_SINGLE);
}

EAcceptance LLToolDragAndDrop::dad3dUpdateInventoryCategory(
	LLViewerObject* obj, S32 face, MASK mask, BOOL drop)
{
	lldebugs << "LLToolDragAndDrop::dad3dUpdateInventoryCategory()" << llendl;
	if (obj == NULL)
	{
		llwarns << "obj is NULL; aborting func with ACCEPT_NO" << llendl;
		return ACCEPT_NO;
	}

	if ((mSource != SOURCE_AGENT) && (mSource != SOURCE_LIBRARY))
	{
		return ACCEPT_NO;
	}
	if (obj->isAttachment())
	{
		return ACCEPT_NO_LOCKED;
	}

	LLViewerInventoryItem* item = NULL;
	LLViewerInventoryCategory* cat = NULL;
	locateInventory(item, cat);
	if (!cat) 
	{
		return ACCEPT_NO;
	}

	// Find all the items in the category
	LLDroppableItem droppable(!obj->permYouOwner());
	LLInventoryModel::cat_array_t cats;
	LLInventoryModel::item_array_t items;
	gInventory.collectDescendentsIf(cat->getUUID(),
					cats,
					items,
					LLInventoryModel::EXCLUDE_TRASH,
					droppable);
	cats.put(cat);
 	if (droppable.countNoCopy() > 0)
 	{
 		llwarns << "*** Need to confirm this step" << llendl;
 	}
	LLViewerObject* root_object = obj;
	if (obj->getParent())
	{
		LLViewerObject* parent_obj = (LLViewerObject*)obj->getParent();
		if (!parent_obj->isAvatar())
		{
			root_object = parent_obj;
		}
	}

	EAcceptance rv = ACCEPT_NO;

	// Check for accept
	for (LLInventoryModel::cat_array_t::const_iterator cat_iter = cats.begin();
		 cat_iter != cats.end();
		 ++cat_iter)
	{
		const LLViewerInventoryCategory *cat = (*cat_iter);
		rv = gInventory.isCategoryComplete(cat->getUUID()) ? ACCEPT_YES_MULTI : ACCEPT_NO;
		if(rv < ACCEPT_YES_SINGLE)
		{
			lldebugs << "Category " << cat->getUUID() << "is not complete." << llendl;
			break;
		}
	}
	if (ACCEPT_YES_COPY_SINGLE <= rv)
	{
		for (LLInventoryModel::item_array_t::const_iterator item_iter = items.begin();
			 item_iter != items.end();
			 ++item_iter)
		{
			LLViewerInventoryItem *item = (*item_iter);
			/*
			// Pass the base objects, not the links.
			if (item && item->getIsLinkType())
			{
				item = item->getLinkedItem();
				(*item_iter) = item;
			}
			*/
			rv = willObjectAcceptInventory(root_object, item);
			if (rv < ACCEPT_YES_COPY_SINGLE)
			{
				lldebugs << "Object will not accept " << item->getUUID() << llendl;
				break;
			}
		}
	}

	// If every item is accepted, send it on
	if (drop && (ACCEPT_YES_COPY_SINGLE <= rv))
	{
		uuid_vec_t ids;
		for (LLInventoryModel::item_array_t::const_iterator item_iter = items.begin();
			 item_iter != items.end();
			 ++item_iter)
		{
			const LLViewerInventoryItem *item = (*item_iter);
			ids.push_back(item->getUUID());
		}
		LLCategoryDropObserver* dropper = new LLCategoryDropObserver(ids, obj->getID(), mSource);
		dropper->startFetch();
		if (dropper->isFinished())
		{
			dropper->done();
		}
		else
		{
			gInventory.addObserver(dropper);
		}
	}
	return rv;
}

BOOL LLToolDragAndDrop::dadUpdateInventoryCategory(LLViewerObject* obj,
												   BOOL drop)
{
	EAcceptance rv = dad3dUpdateInventoryCategory(obj, -1, MASK_NONE, drop);
	return (rv >= ACCEPT_YES_COPY_SINGLE);
}

EAcceptance LLToolDragAndDrop::dad3dGiveInventoryObject(
	LLViewerObject* obj, S32 face, MASK mask, BOOL drop)
{
	lldebugs << "LLToolDragAndDrop::dad3dGiveInventoryObject()" << llendl;

	// item has to be in agent inventory.
	if(mSource != SOURCE_AGENT) return ACCEPT_NO;

	// find the item now.
	LLViewerInventoryItem* item;
	LLViewerInventoryCategory* cat;
	locateInventory(item, cat);
	if (!item || !item->isFinished()) return ACCEPT_NO;
	if(!item->getPermissions().allowOperationBy(PERM_TRANSFER, gAgent.getID()))
	{
		// cannot give away no-transfer objects
		return ACCEPT_NO;
	}
	LLVOAvatarSelf* avatar = gAgentAvatarp;
	if(avatar && avatar->isWearingAttachment( item->getUUID() ) )
	{
		// You can't give objects that are attached to you
		return ACCEPT_NO;
	}
	if( obj && avatar )
	{
		if(drop)
		{
			LLGiveInventory::doGiveInventoryItem(obj->getID(), item );
		}
		// *TODO: deal with all the issues surrounding multi-object
		// inventory transfers.
		return ACCEPT_YES_SINGLE;
	}
	return ACCEPT_NO;
}


EAcceptance LLToolDragAndDrop::dad3dGiveInventory(
	LLViewerObject* obj, S32 face, MASK mask, BOOL drop)
{
	lldebugs << "LLToolDragAndDrop::dad3dGiveInventory()" << llendl;
	// item has to be in agent inventory.
	if(mSource != SOURCE_AGENT) return ACCEPT_NO;
	LLViewerInventoryItem* item;
	LLViewerInventoryCategory* cat;
	locateInventory(item, cat);
	if (!item || !item->isFinished()) return ACCEPT_NO;
	if (!LLGiveInventory::isInventoryGiveAcceptable(item))
	{
		return ACCEPT_NO;
	}
	if (drop && obj)
	{
		LLGiveInventory::doGiveInventoryItem(obj->getID(), item);
	}
	// *TODO: deal with all the issues surrounding multi-object
	// inventory transfers.
	return ACCEPT_YES_SINGLE;
}

EAcceptance LLToolDragAndDrop::dad3dGiveInventoryCategory(
	LLViewerObject* obj, S32 face, MASK mask, BOOL drop)
{
	lldebugs << "LLToolDragAndDrop::dad3dGiveInventoryCategory()" << llendl;
	if(drop && obj)
	{
		LLViewerInventoryItem* item;
		LLViewerInventoryCategory* cat;
		locateInventory(item, cat);
		if(!cat) return ACCEPT_NO;
		LLGiveInventory::doGiveInventoryCategory(obj->getID(), cat);
	}
	// *TODO: deal with all the issues surrounding multi-object
	// inventory transfers.
	return ACCEPT_YES_SINGLE;
}


EAcceptance LLToolDragAndDrop::dad3dRezFromObjectOnLand(
	LLViewerObject* obj, S32 face, MASK mask, BOOL drop)
{
	lldebugs << "LLToolDragAndDrop::dad3dRezFromObjectOnLand()" << llendl;
	LLViewerInventoryItem* item = NULL;
	LLViewerInventoryCategory* cat = NULL;
	locateInventory(item, cat);
	if (!item || !item->isFinished()) return ACCEPT_NO;

	if(!gAgent.allowOperation(PERM_COPY, item->getPermissions())
		|| !item->getPermissions().allowTransferTo(LLUUID::null))
	{
		return ACCEPT_NO_LOCKED;
	}
	if(drop)
	{
		dropObject(obj, TRUE, TRUE, FALSE);
	}
	return ACCEPT_YES_SINGLE;
}

EAcceptance LLToolDragAndDrop::dad3dRezFromObjectOnObject(
	LLViewerObject* obj, S32 face, MASK mask, BOOL drop)
{
	lldebugs << "LLToolDragAndDrop::dad3dRezFromObjectOnObject()" << llendl;
	LLViewerInventoryItem* item;
	LLViewerInventoryCategory* cat;
	locateInventory(item, cat);
	if (!item || !item->isFinished()) return ACCEPT_NO;
	if((mask & MASK_CONTROL))
	{
		// *HACK: In order to resolve SL-22177, we need to block drags
		// from notecards and objects onto other objects.
		return ACCEPT_NO;

		// *HACK: uncomment this when appropriate
		//EAcceptance rv = willObjectAcceptInventory(obj, item);
		//if(drop && (ACCEPT_YES_SINGLE <= rv))
		//{
		//	dropInventory(obj, item, mSource, mSourceID);
		//}
		//return rv;
	}
	if(!item->getPermissions().allowCopyBy(gAgent.getID(),
										   gAgent.getGroupID())
	   || !item->getPermissions().allowTransferTo(LLUUID::null))
	{
		return ACCEPT_NO_LOCKED;
	}
	if(drop)
	{
		dropObject(obj, FALSE, TRUE, FALSE);
	}
	return ACCEPT_YES_SINGLE;
}

EAcceptance LLToolDragAndDrop::dad3dCategoryOnLand(
	LLViewerObject *obj, S32 face, MASK mask, BOOL drop)
{
	return ACCEPT_NO;
	/*
	lldebugs << "LLToolDragAndDrop::dad3dCategoryOnLand()" << llendl;
	LLInventoryItem* item;
	LLInventoryCategory* cat;
	locateInventory(item, cat);
	if(!cat) return ACCEPT_NO;
	EAcceptance rv = ACCEPT_NO;

	// find all the items in the category
	LLViewerInventoryCategory::cat_array_t cats;
	LLViewerInventoryItem::item_array_t items;
	LLDropCopyableItems droppable;
	gInventory.collectDescendentsIf(cat->getUUID(),
									cats,
									items,
									LLInventoryModel::EXCLUDE_TRASH,
									droppable);
	if(items.count() > 0)
	{
		rv = ACCEPT_YES_SINGLE;
	}
	if((rv >= ACCEPT_YES_COPY_SINGLE) && drop)
	{
		createContainer(items, cat->getName());
		return ACCEPT_NO;
	}
	return rv;
	*/
}


// This is based on ALOT of copied, special-cased code
// This shortcuts alot of steps to make a basic object
// w/ an inventory and a special permissions set
EAcceptance LLToolDragAndDrop::dad3dAssetOnLand(
	LLViewerObject *obj, S32 face, MASK mask, BOOL drop)
{
	return ACCEPT_NO;
	/*
	lldebugs << "LLToolDragAndDrop::dad3dAssetOnLand()" << llendl;
	LLViewerInventoryCategory::cat_array_t cats;
	LLViewerInventoryItem::item_array_t items;
	LLViewerInventoryItem::item_array_t copyable_items;
	locateMultipleInventory(items, cats);
	if(!items.count()) return ACCEPT_NO;
	EAcceptance rv = ACCEPT_NO;
	for (S32 i = 0; i < items.count(); i++)
	{
		LLInventoryItem* item = items[i];
		if(item->getPermissions().allowCopyBy(gAgent.getID()))
		{
			copyable_items.put(item);
			rv = ACCEPT_YES_SINGLE;
		}
	}

	if((rv >= ACCEPT_YES_COPY_SINGLE) && drop)
	{
		createContainer(copyable_items, NULL);
	}

	return rv;
	*/
}

LLInventoryObject* LLToolDragAndDrop::locateInventory(
	LLViewerInventoryItem*& item,
	LLViewerInventoryCategory*& cat)
{
	item = NULL;
	cat = NULL;
	if(mCargoIDs.empty()) return NULL;
	if((mSource == SOURCE_AGENT) || (mSource == SOURCE_LIBRARY))
	{
		// The object should be in user inventory.
		item = (LLViewerInventoryItem*)gInventory.getItem(mCargoIDs[mCurItemIndex]);
		cat = (LLViewerInventoryCategory*)gInventory.getCategory(mCargoIDs[mCurItemIndex]);
	}
	else if(mSource == SOURCE_WORLD)
	{
		// This object is in some task inventory somewhere.
		LLViewerObject* obj = gObjectList.findObject(mSourceID);
		if(obj)
		{
			if((mCargoTypes[mCurItemIndex] == DAD_CATEGORY)
			   || (mCargoTypes[mCurItemIndex] == DAD_ROOT_CATEGORY))
			{
				cat = (LLViewerInventoryCategory*)obj->getInventoryObject(mCargoIDs[mCurItemIndex]);
			}
			else
			{
			   item = (LLViewerInventoryItem*)obj->getInventoryObject(mCargoIDs[mCurItemIndex]);
			}
		}
	}
	else if(mSource == SOURCE_NOTECARD)
	{
		LLPreviewNotecard* preview = LLFloaterReg::findTypedInstance<LLPreviewNotecard>("preview_notecard", mSourceID);
		if(preview)
		{
			item = (LLViewerInventoryItem*)preview->getDragItem();
		}
	}
	else if(mSource == SOURCE_VIEWER)
	{
		item = (LLViewerInventoryItem*)gClipboard.getSourceObject();
	}
	if(item) return item;
	if(cat) return cat;
	return NULL;
}

/*
LLInventoryObject* LLToolDragAndDrop::locateMultipleInventory(LLViewerInventoryCategory::cat_array_t& cats,
															  LLViewerInventoryItem::item_array_t& items)
{
	if(mCargoIDs.count() == 0) return NULL;
	if((mSource == SOURCE_AGENT) || (mSource == SOURCE_LIBRARY))
	{
		// The object should be in user inventory.
		for (S32 i = 0; i < mCargoIDs.count(); i++)
		{
			LLInventoryItem* item = gInventory.getItem(mCargoIDs[i]);
			if (item)
			{
				items.put(item);
			}
			LLInventoryCategory* category = gInventory.getCategory(mCargoIDs[i]);
			if (category)
			{
				cats.put(category);
			}
		}
	}
	else if(mSource == SOURCE_WORLD)
	{
		// This object is in some task inventory somewhere.
		LLViewerObject* obj = gObjectList.findObject(mSourceID);
		if(obj)
		{
			if((mCargoType == DAD_CATEGORY)
			   || (mCargoType == DAD_ROOT_CATEGORY))
			{
				// The object should be in user inventory.
				for (S32 i = 0; i < mCargoIDs.count(); i++)
				{
					LLInventoryCategory* category = (LLInventoryCategory*)obj->getInventoryObject(mCargoIDs[i]);
					if (category)
					{
						cats.put(category);
					}
				}
			}
			else
			{
				for (S32 i = 0; i < mCargoIDs.count(); i++)
				{
					LLInventoryItem* item = (LLInventoryItem*)obj->getInventoryObject(mCargoIDs[i]);
					if (item)
					{
						items.put(item);
					}
				}
			}
		}
	}
	else if(mSource == SOURCE_NOTECARD)
	{
		LLPreviewNotecard* card;
		card = (LLPreviewNotecard*)LLPreview::find(mSourceID);
		if(card)
		{
			items.put((LLInventoryItem*)card->getDragItem());
		}
	}
	if(items.count()) return items[0];
	if(cats.count()) return cats[0];
	return NULL;
}
*/

// void LLToolDragAndDrop::createContainer(LLViewerInventoryItem::item_array_t &items, const char* preferred_name )
// {
// 	llwarns << "LLToolDragAndDrop::createContainer()" << llendl;
// 	return;
// }


// utility functions

void pack_permissions_slam(LLMessageSystem* msg, U32 flags, const LLPermissions& perms)
{
	// CRUFT -- the server no longer pays attention to this data
	U32 group_mask		= perms.getMaskGroup();
	U32 everyone_mask	= perms.getMaskEveryone();
	U32 next_owner_mask	= perms.getMaskNextOwner();
	
	msg->addU32Fast(_PREHASH_ItemFlags, flags);
	msg->addU32Fast(_PREHASH_GroupMask, group_mask);
	msg->addU32Fast(_PREHASH_EveryoneMask, everyone_mask);
	msg->addU32Fast(_PREHASH_NextOwnerMask, next_owner_mask);
}
