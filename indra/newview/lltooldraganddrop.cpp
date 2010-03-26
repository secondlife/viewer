/** 
 * @file lltooldraganddrop.cpp
 * @brief LLToolDragAndDrop class implementation
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
#include "lltooldraganddrop.h"

// library headers
#include "llnotificationsutil.h"
// project headers
#include "llagent.h"
#include "llagentcamera.h"
#include "llagentui.h"
#include "llagentwearables.h"
#include "llappearancemgr.h"
#include "lldictionary.h"
//#include "llfirstuse.h"
#include "llfloaterreg.h"
#include "llfloatertools.h"
#include "llgesturemgr.h"
#include "llhudmanager.h"
#include "llhudeffecttrail.h"
#include "llimview.h"
#include "llinventorybridge.h"
#include "llmutelist.h"
#include "llpreviewnotecard.h"
#include "llrecentpeople.h"
#include "llrootview.h"
#include "llselectmgr.h"
#include "lltoolmgr.h"
#include "lltooltip.h"
#include "lltrans.h"
#include "llviewerobjectlist.h"
#include "llviewerstats.h"
#include "llviewerwindow.h"
#include "llvoavatarself.h"
#include "llworld.h"

// MAX ITEMS is based on (sizeof(uuid)+2) * count must be < MTUBYTES
// or 18 * count < 1200 => count < 1200/18 => 66. I've cut it down a
// bit from there to give some pad.
const S32 MAX_ITEMS = 42;

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

class LLUncopyableItems : public LLInventoryCollectFunctor
{
public:
	LLUncopyableItems() {}
	virtual ~LLUncopyableItems() {}
	virtual bool operator()(LLInventoryCategory* cat, LLInventoryItem* item);
};

bool LLUncopyableItems::operator()(LLInventoryCategory* cat,
				   LLInventoryItem* item)
{
	bool uncopyable = false;
	if (item)
	{
		if (itemTransferCommonlyAllowed(item) &&
		   !item->getPermissions().allowCopyBy(gAgent.getID()))
		{
			uncopyable = true;
		}
	}
	return uncopyable;
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

class LLGiveable : public LLInventoryCollectFunctor
{
public:
	LLGiveable() : mCountLosing(0) {}
	virtual ~LLGiveable() {}
	virtual bool operator()(LLInventoryCategory* cat, LLInventoryItem* item);

	S32 countNoCopy() const { return mCountLosing; }
protected:
	S32 mCountLosing;
};

bool LLGiveable::operator()(LLInventoryCategory* cat, LLInventoryItem* item)
{
	// All categories can be given.
	if (cat)
		return true;

	bool allowed = false;
	if (item)
	{
		allowed = itemTransferCommonlyAllowed(item);
		if (allowed &&
		   !item->getPermissions().allowOperationBy(PERM_TRANSFER,
							    gAgent.getID()))
		{
			allowed = FALSE;
		}
		if (allowed &&
		   !item->getPermissions().allowCopyBy(gAgent.getID()))
		{
			++mCountLosing;
		}
	}
	return allowed;
}

class LLCategoryFireAndForget : public LLInventoryFetchComboObserver
{
public:
	LLCategoryFireAndForget() {}
	~LLCategoryFireAndForget() {}
	virtual void done()
	{
		/* no-op: it's fire n forget right? */
		lldebugs << "LLCategoryFireAndForget::done()" << llendl;
	}
};

class LLCategoryDropObserver : public LLInventoryFetchObserver
{
public:
	LLCategoryDropObserver(
		const LLUUID& obj_id, LLToolDragAndDrop::ESource src) :
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
  		item_ref_t::iterator it = mComplete.begin();
  		item_ref_t::iterator end = mComplete.end();
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
	folder_ref_t::iterator it = mCompleteFolders.begin();
	folder_ref_t::iterator end = mCompleteFolders.end();
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
		LLInventoryFetchObserver::item_ref_t ids;
		std::back_insert_iterator<LLInventoryFetchObserver::item_ref_t> copier(ids);
		std::copy(unique_ids.begin(), unique_ids.end(), copier);
		LLCategoryDropObserver* dropper;
		dropper = new LLCategoryDropObserver(mObjectID, mSource);
		dropper->fetchItems(ids);
		if (dropper->isEverythingComplete())
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
 	//       										 DT_NONE        DT_SELF                     DT_AVATAR                   DT_OBJECT                       DT_LAND		
	//      										|--------------|---------------------------|---------------------------|-------------------------------|--------------|
	addEntry(DAD_NONE, 			new DragAndDropEntry(&LLToolDragAndDrop::dad3dNULL,	&LLToolDragAndDrop::dad3dNULL,					&LLToolDragAndDrop::dad3dNULL,					&LLToolDragAndDrop::dad3dNULL,						&LLToolDragAndDrop::dad3dNULL));
	addEntry(DAD_TEXTURE, 		new DragAndDropEntry(&LLToolDragAndDrop::dad3dNULL,	&LLToolDragAndDrop::dad3dNULL,					&LLToolDragAndDrop::dad3dGiveInventory,		&LLToolDragAndDrop::dad3dTextureObject,			&LLToolDragAndDrop::dad3dNULL));
	addEntry(DAD_SOUND, 		new DragAndDropEntry(&LLToolDragAndDrop::dad3dNULL,	&LLToolDragAndDrop::dad3dNULL,					&LLToolDragAndDrop::dad3dGiveInventory,		&LLToolDragAndDrop::dad3dUpdateInventory,			&LLToolDragAndDrop::dad3dNULL));
	addEntry(DAD_CALLINGCARD, 	new DragAndDropEntry(&LLToolDragAndDrop::dad3dNULL,	&LLToolDragAndDrop::dad3dNULL,					&LLToolDragAndDrop::dad3dGiveInventory, 		&LLToolDragAndDrop::dad3dUpdateInventory, 			&LLToolDragAndDrop::dad3dNULL));
	addEntry(DAD_LANDMARK, 		new DragAndDropEntry(&LLToolDragAndDrop::dad3dNULL, 	&LLToolDragAndDrop::dad3dNULL, 				&LLToolDragAndDrop::dad3dGiveInventory, 		&LLToolDragAndDrop::dad3dUpdateInventory, 			&LLToolDragAndDrop::dad3dNULL));
	addEntry(DAD_SCRIPT, 		new DragAndDropEntry(&LLToolDragAndDrop::dad3dNULL, 	&LLToolDragAndDrop::dad3dNULL, 				&LLToolDragAndDrop::dad3dGiveInventory, 		&LLToolDragAndDrop::dad3dRezScript, 				&LLToolDragAndDrop::dad3dNULL));
	addEntry(DAD_CLOTHING, 		new DragAndDropEntry(&LLToolDragAndDrop::dad3dNULL, 	&LLToolDragAndDrop::dad3dWearItem, 			&LLToolDragAndDrop::dad3dGiveInventory, 		&LLToolDragAndDrop::dad3dUpdateInventory, 			&LLToolDragAndDrop::dad3dNULL));
	addEntry(DAD_OBJECT, 		new DragAndDropEntry(&LLToolDragAndDrop::dad3dNULL, 	&LLToolDragAndDrop::dad3dRezAttachmentFromInv,	&LLToolDragAndDrop::dad3dGiveInventoryObject,	&LLToolDragAndDrop::dad3dRezObjectOnObject, 		&LLToolDragAndDrop::dad3dRezObjectOnLand));
	addEntry(DAD_NOTECARD, 		new DragAndDropEntry(&LLToolDragAndDrop::dad3dNULL, 	&LLToolDragAndDrop::dad3dNULL, 				&LLToolDragAndDrop::dad3dGiveInventory, 		&LLToolDragAndDrop::dad3dUpdateInventory, 			&LLToolDragAndDrop::dad3dNULL));
	addEntry(DAD_CATEGORY, 		new DragAndDropEntry(&LLToolDragAndDrop::dad3dNULL, 	&LLToolDragAndDrop::dad3dWearCategory,			&LLToolDragAndDrop::dad3dGiveInventoryCategory,&LLToolDragAndDrop::dad3dUpdateInventoryCategory,	&LLToolDragAndDrop::dad3dNULL));
	addEntry(DAD_ROOT_CATEGORY, new DragAndDropEntry(&LLToolDragAndDrop::dad3dNULL,	&LLToolDragAndDrop::dad3dNULL,					&LLToolDragAndDrop::dad3dNULL,					&LLToolDragAndDrop::dad3dNULL,						&LLToolDragAndDrop::dad3dNULL));
	addEntry(DAD_BODYPART, 		new DragAndDropEntry(&LLToolDragAndDrop::dad3dNULL,	&LLToolDragAndDrop::dad3dWearItem,				&LLToolDragAndDrop::dad3dGiveInventory,		&LLToolDragAndDrop::dad3dUpdateInventory,			&LLToolDragAndDrop::dad3dNULL));
	addEntry(DAD_ANIMATION, 	new DragAndDropEntry(&LLToolDragAndDrop::dad3dNULL,	&LLToolDragAndDrop::dad3dNULL,					&LLToolDragAndDrop::dad3dGiveInventory,		&LLToolDragAndDrop::dad3dUpdateInventory,			&LLToolDragAndDrop::dad3dNULL));
	addEntry(DAD_GESTURE, 		new DragAndDropEntry(&LLToolDragAndDrop::dad3dNULL,	&LLToolDragAndDrop::dad3dActivateGesture,		&LLToolDragAndDrop::dad3dGiveInventory,		&LLToolDragAndDrop::dad3dUpdateInventory,			&LLToolDragAndDrop::dad3dNULL));
	addEntry(DAD_LINK, 			new DragAndDropEntry(&LLToolDragAndDrop::dad3dNULL,	&LLToolDragAndDrop::dad3dNULL,					&LLToolDragAndDrop::dad3dNULL,					&LLToolDragAndDrop::dad3dNULL,						&LLToolDragAndDrop::dad3dNULL));
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
	const S32 MIN_MANHATTAN_DIST = 3;
	S32 manhattan_dist = llabs( x - mDragStartX ) + llabs( y - mDragStartY );
	return manhattan_dist >= MIN_MANHATTAN_DIST;
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
			LLInventoryFetchComboObserver::folder_ref_t folder_ids;
			LLInventoryFetchComboObserver::item_ref_t item_ids;
			if (is_not_preferred(cat, NULL))
			{
				folder_ids.push_back(cargo_id);
			}
			gInventory.collectDescendentsIf (
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
				LLCategoryFireAndForget fetcher;
				fetcher.fetch(folder_ids, item_ids);
			}
		}
	}
}

void LLToolDragAndDrop::beginMultiDrag(
	const std::vector<EDragAndDropType> types,
	const std::vector<LLUUID>& cargo_ids,
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
				gInventory.collectDescendentsIf (
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
			LLInventoryFetchComboObserver::folder_ref_t folder_ids;
			LLInventoryFetchComboObserver::item_ref_t item_ids;
			std::back_insert_iterator<LLInventoryFetchDescendentsObserver::folder_ref_t> copier(folder_ids);
			std::copy(cat_ids.begin(), cat_ids.end(), copier);
			LLCategoryFireAndForget fetcher;
			fetcher.fetch(folder_ids, item_ids);
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
		effectp->setSourceObject(gAgent.getAvatarObject());
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
	if (!item || !item->isComplete()) return;
	
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
	effectp->setSourceObject(gAgent.getAvatarObject());
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
	effectp->setSourceObject(gAgent.getAvatarObject());
	effectp->setTargetObject(hit_obj);
	effectp->setDuration(LL_HUD_DUR_SHORT);
	effectp->setColor(LLColor4U(gAgent.getEffectColor()));
	gFloaterTools->dirty();
}

void LLToolDragAndDrop::giveInventory(const LLUUID& to_agent,
									  LLInventoryItem* item,
									  const LLUUID& im_session_id)
									  
{
	llinfos << "LLToolDragAndDrop::giveInventory()" << llendl;
	if (!isInventoryGiveAcceptable(item))
	{
		return;
	}
	if (item->getPermissions().allowCopyBy(gAgent.getID()))
	{
		// just give it away.
		LLToolDragAndDrop::commitGiveInventoryItem(to_agent, item, im_session_id);
	}
	else
	{
		// ask if the agent is sure.
		LLSD payload;
		payload["agent_id"] = to_agent;
		payload["item_id"] = item->getUUID();
		LLNotificationsUtil::add("CannotCopyWarning", LLSD(), payload, 
		        &LLToolDragAndDrop::handleCopyProtectedItem);
	}
}
// static
bool LLToolDragAndDrop::handleCopyProtectedItem(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	LLInventoryItem* item = NULL;
	switch(option)
	{
	case 0:  // "Yes"
		item = gInventory.getItem(notification["payload"]["item_id"].asUUID());
		if (item)
		{
			LLToolDragAndDrop::commitGiveInventoryItem(notification["payload"]["agent_id"].asUUID(),
													   item);
			// delete it for now - it will be deleted on the server
			// quickly enough.
			gInventory.deleteObject(notification["payload"]["item_id"].asUUID());
			gInventory.notifyObservers();
		}
		else
		{
			LLNotificationsUtil::add("CannotGiveItem");		
		}
		break;

	default: // no, cancel, whatever, who cares, not yes.
		LLNotificationsUtil::add("TransactionCancelled");
		break;
	}
	return false;
}

// static
void LLToolDragAndDrop::commitGiveInventoryItem(const LLUUID& to_agent,
												LLInventoryItem* item,
												const LLUUID& im_session_id)
{
	if (!item) return;
	std::string name;
	LLAgentUI::buildFullname(name);
	LLUUID transaction_id;
	transaction_id.generate();
	const S32 BUCKET_SIZE = sizeof(U8) + UUID_BYTES;
	U8 bucket[BUCKET_SIZE];
	bucket[0] = (U8)item->getType();
	memcpy(&bucket[1], &(item->getUUID().mData), UUID_BYTES);		/* Flawfinder: ignore */
	pack_instant_message(
		gMessageSystem,
		gAgent.getID(),
		FALSE,
		gAgent.getSessionID(),
		to_agent,
		name,
		item->getName(),
		IM_ONLINE,
		IM_INVENTORY_OFFERED,
		transaction_id,
		0,
		LLUUID::null,
		gAgent.getPositionAgent(),
		NO_TIMESTAMP,
		bucket,
		BUCKET_SIZE);
	gAgent.sendReliableMessage(); 

	// VEFFECT: giveInventory
	LLHUDEffectSpiral *effectp = (LLHUDEffectSpiral *)LLHUDManager::getInstance()->createViewerEffect(LLHUDObject::LL_HUD_EFFECT_BEAM, TRUE);
	effectp->setSourceObject(gAgent.getAvatarObject());
	effectp->setTargetObject(gObjectList.findObject(to_agent));
	effectp->setDuration(LL_HUD_DUR_SHORT);
	effectp->setColor(LLColor4U(gAgent.getEffectColor()));
	gFloaterTools->dirty();

	LLMuteList::getInstance()->autoRemove(to_agent, LLMuteList::AR_INVENTORY);

	logInventoryOffer(to_agent, im_session_id);	

	// add buddy to recent people list
	LLRecentPeople::instance().add(to_agent);
}

//static
void LLToolDragAndDrop::logInventoryOffer(const LLUUID& to_agent, const LLUUID &im_session_id)
{
	// compute id of possible IM session with agent that has "to_agent" id
	LLUUID session_id = LLIMMgr::computeSessionID(IM_NOTHING_SPECIAL, to_agent);
	// If this item was given by drag-and-drop into an IM panel, log this action in the IM panel chat.
	if (im_session_id.notNull())
	{
		LLSD args;
		gIMMgr->addSystemMessage(im_session_id, "inventory_item_offered", args);
	}
	// If this item was given by drag-and-drop on avatar while IM panel was open, log this action in the IM panel chat.
	else if (LLIMModel::getInstance()->findIMSession(session_id))
	{
		LLSD args;
		gIMMgr->addSystemMessage(session_id, "inventory_item_offered", args);
	}
	// If this item was given by drag-and-drop on avatar while IM panel wasn't open, log this action to IM history.
	else
	{
		std::string full_name;
		if (gCacheName->getFullName(to_agent, full_name))
		{
			LLIMModel::instance().logToFile(full_name, LLTrans::getString("SECOND_LIFE"), im_session_id, LLTrans::getString("inventory_item_offered-im"));
		}
	}
}

void LLToolDragAndDrop::giveInventoryCategory(const LLUUID& to_agent,
											  LLInventoryCategory* cat,
											  const LLUUID& im_session_id)

{
	if (!cat) return;
	llinfos << "LLToolDragAndDrop::giveInventoryCategory() - "
			<< cat->getUUID() << llendl;

	LLVOAvatarSelf* avatarp = gAgent.getAvatarObject();
	if (!avatarp)
	{
		return;
	}

	// Test out how many items are being given.
	LLViewerInventoryCategory::cat_array_t cats;
	LLViewerInventoryItem::item_array_t items;
	LLGiveable giveable;
	gInventory.collectDescendentsIf (cat->getUUID(),
									cats,
									items,
									LLInventoryModel::EXCLUDE_TRASH,
									giveable);
	S32 count = cats.count();
	bool complete = true;
	for(S32 i = 0; i < count; ++i)
	{
		if (!gInventory.isCategoryComplete(cats.get(i)->getUUID()))
		{
			complete = false;
			break;
		}
	}
	if (!complete)
	{
		LLNotificationsUtil::add("IncompleteInventory");
		return;
	}
 	count = items.count() + cats.count();
 	if (count > MAX_ITEMS)
  	{
		LLNotificationsUtil::add("TooManyItems");
  		return;
  	}
 	else if (count == 0)
  	{
		LLNotificationsUtil::add("NoItems");
  		return;
  	}
	else
	{
		if (0 == giveable.countNoCopy())
		{
			LLToolDragAndDrop::commitGiveInventoryCategory(to_agent, cat, im_session_id);
		}
		else 
		{
			LLSD args;
			args["COUNT"] = llformat("%d",giveable.countNoCopy());
			LLSD payload;
			payload["agent_id"] = to_agent;
			payload["folder_id"] = cat->getUUID();
			LLNotificationsUtil::add("CannotCopyCountItems", args, payload, &LLToolDragAndDrop::handleCopyProtectedCategory);
		}
	}
}


// static
bool LLToolDragAndDrop::handleCopyProtectedCategory(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	LLInventoryCategory* cat = NULL;
	switch(option)
	{
	case 0:  // "Yes"
		cat = gInventory.getCategory(notification["payload"]["folder_id"].asUUID());
		if (cat)
		{
			LLToolDragAndDrop::commitGiveInventoryCategory(notification["payload"]["agent_id"].asUUID(),
														   cat);
			LLViewerInventoryCategory::cat_array_t cats;
			LLViewerInventoryItem::item_array_t items;
			LLUncopyableItems remove;
			gInventory.collectDescendentsIf (cat->getUUID(),
											cats,
											items,
											LLInventoryModel::EXCLUDE_TRASH,
											remove);
			S32 count = items.count();
			for(S32 i = 0; i < count; ++i)
			{
				gInventory.deleteObject(items.get(i)->getUUID());
			}
			gInventory.notifyObservers();
		}
		else
		{
			LLNotificationsUtil::add("CannotGiveCategory");
		}
		break;

	default: // no, cancel, whatever, who cares, not yes.
		LLNotificationsUtil::add("TransactionCancelled");
		break;
	}
	return false;
}

// static
void LLToolDragAndDrop::commitGiveInventoryCategory(const LLUUID& to_agent,
													LLInventoryCategory* cat,
													const LLUUID& im_session_id)

{
	if (!cat) return;
	llinfos << "LLToolDragAndDrop::commitGiveInventoryCategory() - "
			<< cat->getUUID() << llendl;

	// add buddy to recent people list
	LLRecentPeople::instance().add(to_agent);

	// Test out how many items are being given.
	LLViewerInventoryCategory::cat_array_t cats;
	LLViewerInventoryItem::item_array_t items;
	LLGiveable giveable;
	gInventory.collectDescendentsIf (cat->getUUID(),
									cats,
									items,
									LLInventoryModel::EXCLUDE_TRASH,
									giveable);

	// MAX ITEMS is based on (sizeof(uuid)+2) * count must be <
	// MTUBYTES or 18 * count < 1200 => count < 1200/18 =>
	// 66. I've cut it down a bit from there to give some pad.
 	S32 count = items.count() + cats.count();
 	if (count > MAX_ITEMS)
  	{
		LLNotificationsUtil::add("TooManyItems");
  		return;
  	}
 	else if (count == 0)
  	{
		LLNotificationsUtil::add("NoItems");
  		return;
  	}
	else
	{
		std::string name;
		LLAgentUI::buildFullname(name);
		LLUUID transaction_id;
		transaction_id.generate();
		S32 bucket_size = (sizeof(U8) + UUID_BYTES) * (count + 1);
		U8* bucket = new U8[bucket_size];
		U8* pos = bucket;
		U8 type = (U8)cat->getType();
		memcpy(pos, &type, sizeof(U8));		/* Flawfinder: ignore */
		pos += sizeof(U8);
		memcpy(pos, &(cat->getUUID()), UUID_BYTES);		/* Flawfinder: ignore */
		pos += UUID_BYTES;
		S32 i;
		count = cats.count();
		for(i = 0; i < count; ++i)
		{
			memcpy(pos, &type, sizeof(U8));		/* Flawfinder: ignore */
			pos += sizeof(U8);
			memcpy(pos, &(cats.get(i)->getUUID()), UUID_BYTES);		/* Flawfinder: ignore */
			pos += UUID_BYTES;
		}
		count = items.count();
		for(i = 0; i < count; ++i)
		{
			type = (U8)items.get(i)->getType();
			memcpy(pos, &type, sizeof(U8));		/* Flawfinder: ignore */
			pos += sizeof(U8);
			memcpy(pos, &(items.get(i)->getUUID()), UUID_BYTES);		/* Flawfinder: ignore */
			pos += UUID_BYTES;
		}
		pack_instant_message(
			gMessageSystem,
			gAgent.getID(),
			FALSE,
			gAgent.getSessionID(),
			to_agent,
			name,
			cat->getName(),
			IM_ONLINE,
			IM_INVENTORY_OFFERED,
			transaction_id,
			0,
			LLUUID::null,
			gAgent.getPositionAgent(),
			NO_TIMESTAMP,
			bucket,
			bucket_size);
		gAgent.sendReliableMessage();
		delete[] bucket;

		// VEFFECT: giveInventoryCategory
		LLHUDEffectSpiral *effectp = (LLHUDEffectSpiral *)LLHUDManager::getInstance()->createViewerEffect(LLHUDObject::LL_HUD_EFFECT_BEAM, TRUE);
		effectp->setSourceObject(gAgent.getAvatarObject());
		effectp->setTargetObject(gObjectList.findObject(to_agent));
		effectp->setDuration(LL_HUD_DUR_SHORT);
		effectp->setColor(LLColor4U(gAgent.getEffectColor()));
		gFloaterTools->dirty();

		LLMuteList::getInstance()->autoRemove(to_agent, LLMuteList::AR_INVENTORY);

		logInventoryOffer(to_agent, im_session_id);
	}
}

// static
BOOL LLToolDragAndDrop::isInventoryGiveAcceptable(LLInventoryItem* item)
{
	if (!item)
	{
		return FALSE;
	}
	if (!item->getPermissions().allowOperationBy(PERM_TRANSFER, gAgent.getID()))
	{
		return FALSE;
	}
	BOOL copyable = FALSE;
	if (item->getPermissions().allowCopyBy(gAgent.getID())) copyable = TRUE;

	LLVOAvatarSelf* avatarp = gAgent.getAvatarObject();
	if (!avatarp)
	{
		return FALSE;
	}

	BOOL acceptable = TRUE;
	switch(item->getType())
	{
	case LLAssetType::AT_OBJECT:
		if (avatarp->isWearingAttachment(item->getUUID()))
		{
			acceptable = FALSE;
		}
		break;
	case LLAssetType::AT_BODYPART:
	case LLAssetType::AT_CLOTHING:
		if (!copyable && gAgentWearables.isWearingItem(item->getUUID()))
		{
			acceptable = FALSE;
		}
		break;
	default:
		break;
	}
	return acceptable;
}

// Static
BOOL LLToolDragAndDrop::isInventoryGroupGiveAcceptable(LLInventoryItem* item)
{
	if (!item)
	{
		return FALSE;
	}

	// These permissions are double checked in the simulator in
	// LLGroupNoticeInventoryItemFetch::result().
	if (!item->getPermissions().allowOperationBy(PERM_TRANSFER, gAgent.getID()))
	{
		return FALSE;
	}
	if (!item->getPermissions().allowCopyBy(gAgent.getID()))
	{
		return FALSE;
	}

	LLVOAvatarSelf* avatarp = gAgent.getAvatarObject();
	if (!avatarp)
	{
		return FALSE;
	}

	BOOL acceptable = TRUE;
	switch(item->getType())
	{
	case LLAssetType::AT_OBJECT:
		if (avatarp->isWearingAttachment(item->getUUID()))
		{
			acceptable = FALSE;
		}
		break;
	default:
		break;
	}
	return acceptable;
}

// accessor that looks at permissions, copyability, and names of
// inventory items to determine if a drop would be ok.
EAcceptance LLToolDragAndDrop::willObjectAcceptInventory(LLViewerObject* obj, LLInventoryItem* item)
{
	// check the basics
	if (!item || !obj) return ACCEPT_NO;
	// HACK: downcast
	LLViewerInventoryItem* vitem = (LLViewerInventoryItem*)item;
	if (!vitem->isComplete()) return ACCEPT_NO;
	if (vitem->getIsLinkType()) return ACCEPT_NO; // No giving away links

	// deny attempts to drop from an object onto itself. This is to
	// help make sure that drops that are from an object to an object
	// don't have to worry about order of evaluation. Think of this
	// like check for self in assignment.
	if (obj->getID() == item->getParentUUID())
	{
		return ACCEPT_NO;
	}
	
	//BOOL copy = (perm.allowCopyBy(gAgent.getID(),
	//							  gAgent.getGroupID())
	//			 && (obj->mPermModify || obj->mFlagAllowInventoryAdd));
	BOOL worn = FALSE;
	LLVOAvatarSelf* avatarp = NULL;
	switch(item->getType())
	{
	case LLAssetType::AT_OBJECT:
		avatarp = gAgent.getAvatarObject();
		if (avatarp && avatarp->isWearingAttachment(item->getUUID()))
		{
				worn = TRUE;
		}
		break;
	case LLAssetType::AT_BODYPART:
	case LLAssetType::AT_CLOTHING:
		if (gAgentWearables.isWearingItem(item->getUUID()))
		{
			worn = TRUE;
		}
		break;
	default:
			break;
	}
	const LLPermissions& perm = item->getPermissions();
	BOOL modify = (obj->permModify() || obj->flagAllowInventoryAdd());
	BOOL transfer = FALSE;
	if ((obj->permYouOwner() && (perm.getOwner() == gAgent.getID()))
	   || perm.allowOperationBy(PERM_TRANSFER, gAgent.getID()))
	{
		transfer = TRUE;
	}
	BOOL volume = (LL_PCODE_VOLUME == obj->getPCode());
	BOOL attached = obj->isAttachment();
	BOOL unrestricted = ((perm.getMaskBase() & PERM_ITEM_UNRESTRICTED) == PERM_ITEM_UNRESTRICTED) ? TRUE : FALSE;
	if (attached && !unrestricted)
	{
		return ACCEPT_NO_LOCKED;
	}
	else if (modify && transfer && volume && !worn)
	{
		return ACCEPT_YES_MULTI;
	}
	else if (!modify)
	{
		return ACCEPT_NO_LOCKED;
	}
	return ACCEPT_NO;
}


// function used as drag-and-drop handler for simple agent give inventory requests
//static
bool LLToolDragAndDrop::handleGiveDragAndDrop(LLUUID dest_agent, LLUUID session_id, BOOL drop,
											  EDragAndDropType cargo_type,
											  void* cargo_data,
											  EAcceptance* accept)
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
	{
		LLViewerInventoryItem* inv_item = (LLViewerInventoryItem*)cargo_data;
		if (gInventory.getItem(inv_item->getUUID())
		   && LLToolDragAndDrop::isInventoryGiveAcceptable(inv_item))
		{
			// *TODO: get multiple object transfers working
			*accept = ACCEPT_YES_COPY_SINGLE;
			if (drop)
			{
				LLToolDragAndDrop::giveInventory(dest_agent, inv_item, session_id);
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
		if (gInventory.getCategory(inv_cat->getUUID()))
		{
			// *TODO: get multiple object transfers working
			*accept = ACCEPT_YES_COPY_SINGLE;
			if (drop)
			{
				LLToolDragAndDrop::giveInventoryCategory(dest_agent, inv_cat, session_id);
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
	if (mSource != SOURCE_AGENT && mSource != SOURCE_LIBRARY)
	{
		return ACCEPT_NO;
	}

	LLViewerInventoryItem* item;
	LLViewerInventoryCategory* cat;
	locateInventory(item, cat);
	if (!item || !item->isComplete()) return ACCEPT_NO;

	// must not be in the trash
	const LLUUID trash_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_TRASH);
	if (gInventory.isObjectDescendentOf(item->getUUID(), trash_id))
	{
		return ACCEPT_NO;
	}

	// must not be already wearing it
	LLVOAvatarSelf* avatarp = gAgent.getAvatarObject();
	if (!avatarp || avatarp->isWearingAttachment(item->getUUID()))
	{
		return ACCEPT_NO;
	}

	if (drop)
	{
		if (mSource == SOURCE_LIBRARY)
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
	if (!item || !item->isComplete()) return ACCEPT_NO;

	LLVOAvatarSelf* avatarp = gAgent.getAvatarObject();
	if (!avatarp || avatarp->isWearingAttachment(item->getUUID()))
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
	if (!item->getPermissions().allowCopyBy(gAgent.getID()))
	{
		accept = ACCEPT_YES_SINGLE;
		remove_inventory = TRUE;
	}

	// Check if it's in the trash.
	const LLUUID trash_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_TRASH);
	if (gInventory.isObjectDescendentOf(item->getUUID(), trash_id))
	{
		accept = ACCEPT_YES_SINGLE;
		remove_inventory = TRUE;
	}

	if (drop)
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
	if (!item || !item->isComplete()) return ACCEPT_NO;
	LLVOAvatarSelf* avatarp = gAgent.getAvatarObject();
	if (!avatarp || avatarp->isWearingAttachment(item->getUUID()))
	{
		return ACCEPT_NO;
	}

	if ((mask & MASK_CONTROL))
	{
		// *HACK: In order to resolve SL-22177, we need to block drags
		// from notecards and objects onto other objects.
		if (mSource == SOURCE_NOTECARD)
		{
			return ACCEPT_NO;
		}

		EAcceptance rv = willObjectAcceptInventory(obj, item);
		if (drop && (ACCEPT_YES_SINGLE <= rv))
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
	if (!item->getPermissions().allowCopyBy(gAgent.getID()))
	{
		accept = ACCEPT_YES_SINGLE;
		remove_inventory = TRUE;
	}

	// Check if it's in the trash.
	const LLUUID trash_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_TRASH);
	if (gInventory.isObjectDescendentOf(item->getUUID(), trash_id))
	{
		accept = ACCEPT_YES_SINGLE;
		remove_inventory = TRUE;
	}

	if (drop)
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
	if ((SOURCE_WORLD == mSource) || (SOURCE_NOTECARD == mSource))
	{
		return ACCEPT_NO;
	}

	LLViewerInventoryItem* item;
	LLViewerInventoryCategory* cat;
	locateInventory(item, cat);
	if (!item || !item->isComplete()) return ACCEPT_NO;
	EAcceptance rv = willObjectAcceptInventory(obj, item);
	if (drop && (ACCEPT_YES_SINGLE <= rv))
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

EAcceptance LLToolDragAndDrop::dad3dTextureObject(
	LLViewerObject* obj, S32 face, MASK mask, BOOL drop)
{
	lldebugs << "LLToolDragAndDrop::dad3dTextureObject()" << llendl;

	// *HACK: In order to resolve SL-22177, we need to block drags
	// from notecards and objects onto other objects.
	if ((SOURCE_WORLD == mSource) || (SOURCE_NOTECARD == mSource))
	{
		return ACCEPT_NO;
	}
	
	LLViewerInventoryItem* item;
	LLViewerInventoryCategory* cat;
	locateInventory(item, cat);
	if (!item || !item->isComplete()) return ACCEPT_NO;
	EAcceptance rv = willObjectAcceptInventory(obj, item);
	if ((mask & MASK_CONTROL))
	{
		if ((ACCEPT_YES_SINGLE <= rv) && drop)
		{
			dropInventory(obj, item, mSource, mSourceID);
		}
		return rv;
	}
	if (!obj->permModify())
	{
		return ACCEPT_NO_LOCKED;
	}
	//If texture !copyable don't texture or you'll never get it back.
	if (!item->getPermissions().allowCopyBy(gAgent.getID()))
	{
		return ACCEPT_NO;
	}

	if (drop && (ACCEPT_YES_SINGLE <= rv))
	{
		if ((mask & MASK_SHIFT))
		{
			dropTextureAllFaces(obj, item, mSource, mSourceID);
		}
		else
		{
			dropTextureOneFace(obj, face, item, mSource, mSourceID);
		}
		
		// VEFFECT: SetTexture
		LLHUDEffectSpiral *effectp = (LLHUDEffectSpiral *)LLHUDManager::getInstance()->createViewerEffect(LLHUDObject::LL_HUD_EFFECT_BEAM, TRUE);
		effectp->setSourceObject(gAgent.getAvatarObject());
		effectp->setTargetObject(obj);
		effectp->setDuration(LL_HUD_DUR_SHORT);
		effectp->setColor(LLColor4U(gAgent.getEffectColor()));
	}

	// enable multi-drop, although last texture will win
	return ACCEPT_YES_MULTI;
}
/*
EAcceptance LLToolDragAndDrop::dad3dTextureSelf(
	LLViewerObject* obj, S32 face, MASK mask, BOOL drop)
{
	lldebugs << "LLToolDragAndDrop::dad3dTextureAvatar()" << llendl;
	if (drop)
	{
		if (!(mask & MASK_SHIFT))
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
	if (!item || !item->isComplete()) return ACCEPT_NO;

	if (mSource == SOURCE_AGENT || mSource == SOURCE_LIBRARY)
	{
		// it's in the agent inventory
		const LLUUID trash_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_TRASH);
		if (gInventory.isObjectDescendentOf(item->getUUID(), trash_id))
		{
			return ACCEPT_NO;
		}

		if (drop)
		{
			// Don't wear anything until initial wearables are loaded, can
			// destroy clothing items.
			if (!gAgentWearables.areWearablesLoaded()) 
			{
				LLNotificationsUtil::add("CanNotChangeAppearanceUntilLoaded");
				return ACCEPT_NO;
			}

			if (mSource == SOURCE_LIBRARY)
			{
				// create item based on that one, and put it on if that
				// was a success.
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
				wear_inventory_item_on_avatar( item );
			}
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
	if (!item || !item->isComplete()) return ACCEPT_NO;

	if (mSource == SOURCE_AGENT || mSource == SOURCE_LIBRARY)
	{
		// it's in the agent inventory
		const LLUUID trash_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_TRASH);
		if (gInventory.isObjectDescendentOf(item->getUUID(), trash_id))
		{
			return ACCEPT_NO;
		}

		if (drop)
		{
			LLUUID item_id;
			if (mSource == SOURCE_LIBRARY)
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
				LLGestureManager::instance().activateGesture(item->getUUID());
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
	if (!category) return ACCEPT_NO;

	if (drop)
	{
		// Don't wear anything until initial wearables are loaded, can
		// destroy clothing items.
		if (!gAgentWearables.areWearablesLoaded()) 
		{
			LLNotificationsUtil::add("CanNotChangeAppearanceUntilLoaded");
			return ACCEPT_NO;
		}
	}

	if (mSource == SOURCE_AGENT)
	{
		const LLUUID trash_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_TRASH);
		if (gInventory.isObjectDescendentOf(category->getUUID(), trash_id))
		{
			return ACCEPT_NO;
		}

		if (drop)
		{
		    BOOL append = ( (mask & MASK_SHIFT) ? TRUE : FALSE );
			LLAppearanceManager::instance().wearInventoryCategory(category, false, append);
		}
		return ACCEPT_YES_MULTI;
	}
	else if (mSource == SOURCE_LIBRARY)
	{
		if (drop)
		{
			LLAppearanceManager::instance().wearInventoryCategory(category, true, false);
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
	if ((SOURCE_WORLD == mSource) || (SOURCE_NOTECARD == mSource))
	{
		return ACCEPT_NO;
	}

	LLViewerInventoryItem* item;
	LLViewerInventoryCategory* cat;
	locateInventory(item, cat);
	if (!item || !item->isComplete()) return ACCEPT_NO;
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
	if (root_object && drop && (ACCEPT_YES_COPY_SINGLE <= rv))
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
	gInventory.collectDescendentsIf (cat->getUUID(),
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
		if (rv < ACCEPT_YES_SINGLE)
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
		LLInventoryFetchObserver::item_ref_t ids;
		for (LLInventoryModel::item_array_t::const_iterator item_iter = items.begin();
			 item_iter != items.end();
			 ++item_iter)
		{
			const LLViewerInventoryItem *item = (*item_iter);
			ids.push_back(item->getUUID());
		}
		LLCategoryDropObserver* dropper = new LLCategoryDropObserver(obj->getID(), mSource);
		dropper->fetchItems(ids);
		if (dropper->isEverythingComplete())
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
	if (mSource != SOURCE_AGENT) return ACCEPT_NO;

	// find the item now.
	LLViewerInventoryItem* item;
	LLViewerInventoryCategory* cat;
	locateInventory(item, cat);
	if (!item || !item->isComplete()) return ACCEPT_NO;
	if (!item->getPermissions().allowOperationBy(PERM_TRANSFER, gAgent.getID()))
	{
		// cannot give away no-transfer objects
		return ACCEPT_NO;
	}
	LLVOAvatarSelf* avatarp = gAgent.getAvatarObject();
	if (avatarp && avatarp->isWearingAttachment(item->getUUID()))
	{
		// You can't give objects that are attached to you
		return ACCEPT_NO;
	}
	if (obj && avatarp)
	{
		if (drop)
		{
			giveInventory(obj->getID(), item );
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
	if (mSource != SOURCE_AGENT) return ACCEPT_NO;
	LLViewerInventoryItem* item;
	LLViewerInventoryCategory* cat;
	locateInventory(item, cat);
	if (!item || !item->isComplete()) return ACCEPT_NO;
	if (!isInventoryGiveAcceptable(item))
	{
		return ACCEPT_NO;
	}
	if (drop && obj)
	{
		giveInventory(obj->getID(), item);
	}
	// *TODO: deal with all the issues surrounding multi-object
	// inventory transfers.
	return ACCEPT_YES_SINGLE;
}

EAcceptance LLToolDragAndDrop::dad3dGiveInventoryCategory(
	LLViewerObject* obj, S32 face, MASK mask, BOOL drop)
{
	lldebugs << "LLToolDragAndDrop::dad3dGiveInventoryCategory()" << llendl;
	if (drop && obj)
	{
		LLViewerInventoryItem* item;
		LLViewerInventoryCategory* cat;
		locateInventory(item, cat);
		if (!cat) return ACCEPT_NO;
		giveInventoryCategory(obj->getID(), cat);
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
	if (!item || !item->isComplete()) return ACCEPT_NO;

	if (!gAgent.allowOperation(PERM_COPY, item->getPermissions())
		|| !item->getPermissions().allowTransferTo(LLUUID::null))
	{
		return ACCEPT_NO_LOCKED;
	}
	if (drop)
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
	if (!item || !item->isComplete()) return ACCEPT_NO;
	if ((mask & MASK_CONTROL))
	{
		// *HACK: In order to resolve SL-22177, we need to block drags
		// from notecards and objects onto other objects.
		return ACCEPT_NO;

		// *HACK: uncomment this when appropriate
		//EAcceptance rv = willObjectAcceptInventory(obj, item);
		//if (drop && (ACCEPT_YES_SINGLE <= rv))
		//{
		//	dropInventory(obj, item, mSource, mSourceID);
		//}
		//return rv;
	}
	if (!item->getPermissions().allowCopyBy(gAgent.getID(),
										   gAgent.getGroupID())
	   || !item->getPermissions().allowTransferTo(LLUUID::null))
	{
		return ACCEPT_NO_LOCKED;
	}
	if (drop)
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
	if (!cat) return ACCEPT_NO;
	EAcceptance rv = ACCEPT_NO;

	// find all the items in the category
	LLViewerInventoryCategory::cat_array_t cats;
	LLViewerInventoryItem::item_array_t items;
	LLDropCopyableItems droppable;
	gInventory.collectDescendentsIf (cat->getUUID(),
									cats,
									items,
									LLInventoryModel::EXCLUDE_TRASH,
									droppable);
	if (items.count() > 0)
	{
		rv = ACCEPT_YES_SINGLE;
	}
	if ((rv >= ACCEPT_YES_COPY_SINGLE) && drop)
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
	if (!items.count()) return ACCEPT_NO;
	EAcceptance rv = ACCEPT_NO;
	for (S32 i = 0; i < items.count(); i++)
	{
		LLInventoryItem* item = items[i];
		if (item->getPermissions().allowCopyBy(gAgent.getID()))
		{
			copyable_items.put(item);
			rv = ACCEPT_YES_SINGLE;
		}
	}

	if ((rv >= ACCEPT_YES_COPY_SINGLE) && drop)
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
	if (mCargoIDs.empty()) return NULL;
	if ((mSource == SOURCE_AGENT) || (mSource == SOURCE_LIBRARY))
	{
		// The object should be in user inventory.
		item = (LLViewerInventoryItem*)gInventory.getItem(mCargoIDs[mCurItemIndex]);
		cat = (LLViewerInventoryCategory*)gInventory.getCategory(mCargoIDs[mCurItemIndex]);
	}
	else if (mSource == SOURCE_WORLD)
	{
		// This object is in some task inventory somewhere.
		LLViewerObject* obj = gObjectList.findObject(mSourceID);
		if (obj)
		{
			if ((mCargoTypes[mCurItemIndex] == DAD_CATEGORY)
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
	else if (mSource == SOURCE_NOTECARD)
	{
		LLPreviewNotecard* preview = LLFloaterReg::findTypedInstance<LLPreviewNotecard>("preview_notecard", mSourceID);
		if (preview)
		{
			item = (LLViewerInventoryItem*)preview->getDragItem();
		}
	}
	if (item) return item;
	if (cat) return cat;
	return NULL;
}

/*
LLInventoryObject* LLToolDragAndDrop::locateMultipleInventory(LLViewerInventoryCategory::cat_array_t& cats,
															  LLViewerInventoryItem::item_array_t& items)
{
	if (mCargoIDs.count() == 0) return NULL;
	if ((mSource == SOURCE_AGENT) || (mSource == SOURCE_LIBRARY))
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
	else if (mSource == SOURCE_WORLD)
	{
		// This object is in some task inventory somewhere.
		LLViewerObject* obj = gObjectList.findObject(mSourceID);
		if (obj)
		{
			if ((mCargoType == DAD_CATEGORY)
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
	else if (mSource == SOURCE_NOTECARD)
	{
		LLPreviewNotecard* card;
		card = (LLPreviewNotecard*)LLPreview::find(mSourceID);
		if (card)
		{
			items.put((LLInventoryItem*)card->getDragItem());
		}
	}
	if (items.count()) return items[0];
	if (cats.count()) return cats[0];
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
