/** 
 * @file lltooldraganddrop.cpp
 * @brief LLToolDragAndDrop class implementation
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

#include "message.h"
#include "lltooldraganddrop.h"

#include "llinstantmessage.h"
#include "lldir.h"

#include "llagent.h"
#include "llviewercontrol.h"
#include "llfirstuse.h"
#include "llfloater.h"
#include "llfloatertools.h"
#include "llgesturemgr.h"
#include "llhudeffecttrail.h"
#include "llhudmanager.h"
#include "llinventorymodel.h"
#include "llinventoryview.h"
#include "llnotify.h"
#include "llpreviewnotecard.h"
#include "llselectmgr.h"
#include "lltoolmgr.h"
#include "llui.h"
#include "llviewerimagelist.h"
#include "llviewerinventory.h"
#include "llviewerobject.h"
#include "llviewerobjectlist.h"
#include "llviewerregion.h"
#include "llviewerstats.h"
#include "llviewerwindow.h"
#include "llvoavatar.h"
#include "llvolume.h"
#include "llworld.h"
#include "object_flags.h"

LLToolDragAndDrop *gToolDragAndDrop = NULL;

// MAX ITEMS is based on (sizeof(uuid)+2) * count must be < MTUBYTES
// or 18 * count < 1200 => count < 1200/18 => 66. I've cut it down a
// bit from there to give some pad.
const S32 MAX_ITEMS = 42;
const char* FOLDER_INCLUDES_ATTACHMENTS_BEING_WORN = 
				"Cannot give folders that contain objects that are attached to you.\n"
				"Detach the object(s) and then try again.";


// syntactic sugar
#define callMemberFunction(object,ptrToMember)  ((object).*(ptrToMember))

/*
const LLUUID MULTI_CONTAINER_TEXTURE("b2181ea2-1937-2ee1-78b8-bf05c43536b7");
LLUUID CONTAINER_TEXTURES[LLAssetType::AT_COUNT];

const char* CONTAINER_TEXTURE_NAMES[LLAssetType::AT_COUNT] =
{
	"container_texture.tga",
	"container_sound.tga",
	"container_many_things.tga",
	"container_landmark.tga",
	"container_script.tga",
	"container_clothing.tga",
	"container_object.tga",
	"container_many_things.tga",
	"container_many_things.tga",
	"container_many_things.tga",
	"container_script.tga",
	"container_script.tga",
	"container_texture.tga",
	"container_bodypart.tga",
	"container_many_things.tga",
	"container_many_things.tga",
	"container_many_things.tga",
	"container_sound.tga",
	"container_texture.tga",
	"container_texture.tga",
	"container_animation.tga",
	"container_gesture.tga"
};
*/

class LLNoPreferredType : public LLInventoryCollectFunctor
{
public:
	LLNoPreferredType() {}
	virtual ~LLNoPreferredType() {}
	virtual bool operator()(LLInventoryCategory* cat,
							LLInventoryItem* item)
	{
		if(cat && (cat->getPreferredType() == LLAssetType::AT_NONE))
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
		if(item) return true;
		if(cat && (cat->getPreferredType() == LLAssetType::AT_NONE))
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
	if(item)
	{
		LLVOAvatar* my_avatar = NULL;
		switch(item->getType())
		{
		case LLAssetType::AT_CALLINGCARD:
			// not allowed
			break;

		case LLAssetType::AT_OBJECT:
			my_avatar = gAgent.getAvatarObject();
			if(my_avatar && !my_avatar->isWearingAttachment(item->getUUID()))
			{
				allowed = true;
			}
			break;

		case LLAssetType::AT_BODYPART:
		case LLAssetType::AT_CLOTHING:
			if(!gAgent.isWearingItem(item->getUUID()))
			{
				allowed = true;
			}
			break;

		default:
			allowed = true;
			break;
		}
		if(mIsTransfer
		   && !item->getPermissions().allowOperationBy(PERM_TRANSFER,
													   gAgent.getID()))
		{
			allowed = false;
		}
		if(allowed && !item->getPermissions().allowCopyBy(gAgent.getID()))
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
	BOOL uncopyable = FALSE;
	if(item)
	{
		BOOL allowed = FALSE;
		LLVOAvatar* my_avatar = NULL;
		switch(item->getType())
		{
		case LLAssetType::AT_CALLINGCARD:
			// not allowed
			break;

		case LLAssetType::AT_OBJECT:
			my_avatar = gAgent.getAvatarObject();
			if(my_avatar && !my_avatar->isWearingAttachment(item->getUUID()))
			{
				allowed = TRUE;
			}
			break;

		case LLAssetType::AT_BODYPART:
		case LLAssetType::AT_CLOTHING:
			if(!gAgent.isWearingItem(item->getUUID()))
			{
				allowed = TRUE;
			}
			break;

		default:
			allowed = TRUE;
			break;
		}
		if(allowed && !item->getPermissions().allowCopyBy(gAgent.getID()))
		{
			uncopyable = TRUE;
		}
	}
	return (uncopyable ? true : false);
}

class LLDropCopyableItems : public LLInventoryCollectFunctor
{
public:
	LLDropCopyableItems() {}
	virtual ~LLDropCopyableItems() {}
	virtual bool operator()(LLInventoryCategory* cat, LLInventoryItem* item);
};


bool LLDropCopyableItems::operator()(
	LLInventoryCategory* cat, LLInventoryItem* item)
{
	BOOL allowed = FALSE;
	if(item)
	{
		LLVOAvatar* my_avatar = NULL;
		switch(item->getType())
		{
		case LLAssetType::AT_CALLINGCARD:
			// not allowed
			break;

		case LLAssetType::AT_OBJECT:
			my_avatar = gAgent.getAvatarObject();
			if(my_avatar && !my_avatar->isWearingAttachment(item->getUUID()))
			{
				allowed = TRUE;
			}
			break;

		case LLAssetType::AT_BODYPART:
		case LLAssetType::AT_CLOTHING:
			if(!gAgent.isWearingItem(item->getUUID()))
			{
				allowed = TRUE;
			}
			break;

		default:
			allowed = TRUE;
			break;
		}
		if(allowed && !item->getPermissions().allowCopyBy(gAgent.getID()))
		{
			// whoops, can't copy it - don't allow it.
			allowed = FALSE;
		}
	}
	return (allowed ? true : false);
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
	if(cat) return TRUE;
	BOOL allowed = FALSE;
	if(item)
	{
		LLVOAvatar* my_avatar = NULL;
		switch(item->getType())
		{
		case LLAssetType::AT_CALLINGCARD:
			// not allowed
			break;

		case LLAssetType::AT_OBJECT:
			my_avatar = gAgent.getAvatarObject();
			if(my_avatar && !my_avatar->isWearingAttachment(item->getUUID()))
			{
				allowed = TRUE;
			}
			break;

		case LLAssetType::AT_BODYPART:
		case LLAssetType::AT_CLOTHING:
			if(!gAgent.isWearingItem(item->getUUID()))
			{
				allowed = TRUE;
			}
			break;

		default:
			allowed = TRUE;
			break;
		}
		if(!item->getPermissions().allowOperationBy(PERM_TRANSFER,
													gAgent.getID()))
		{
			allowed = FALSE;
		}
		if(allowed && !item->getPermissions().allowCopyBy(gAgent.getID()))
		{
			++mCountLosing;
		}
	}
	return (allowed ? true : false);
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
	if(dst_obj)
	{
		// *FIX: coalesce these...
 		LLInventoryItem* item = NULL;
  		item_ref_t::iterator it = mComplete.begin();
  		item_ref_t::iterator end = mComplete.end();
  		for(; it < end; ++it)
  		{
 			item = gInventory.getItem(*it);
 			if(item)
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
	if(count)
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
		if(dropper->isEverythingComplete())
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

// This array is used to more easily control what happens when a 3d
// drag and drop event occurs. Since there's an array of drop target
// and cargo type, it's implemented as an array of pointers to member
// functions which correctly carry out the actual drop.
LLToolDragAndDrop::dragOrDrop3dImpl LLToolDragAndDrop::sDragAndDrop3d[DAD_COUNT][LLToolDragAndDrop::DT_COUNT] =
{
	//	Source: DAD_NONE
	{
		&LLToolDragAndDrop::dad3dNULL, // Dest: DT_NONE
		&LLToolDragAndDrop::dad3dNULL, // Dest: DT_SELF
		&LLToolDragAndDrop::dad3dNULL, // Dest: DT_AVATAR
		&LLToolDragAndDrop::dad3dNULL, // Dest: DT_OBJECT
		&LLToolDragAndDrop::dad3dNULL, // Dest: DT_LAND
	},
	//	Source: DAD_TEXTURE
	{
		&LLToolDragAndDrop::dad3dNULL, // Dest: DT_NONE
		&LLToolDragAndDrop::dad3dNULL, // Dest: DT_SELF
		&LLToolDragAndDrop::dad3dGiveInventory, // Dest: DT_AVATAR
		&LLToolDragAndDrop::dad3dTextureObject, // Dest: DT_OBJECT
		&LLToolDragAndDrop::dad3dNULL,//dad3dAssetOnLand, // Dest: DT_LAND
	},
	//	Source: DAD_SOUND
	{
		&LLToolDragAndDrop::dad3dNULL, // Dest: DT_NONE
		&LLToolDragAndDrop::dad3dNULL, // Dest: DT_SELF
		&LLToolDragAndDrop::dad3dGiveInventory, // Dest: DT_AVATAR
		&LLToolDragAndDrop::dad3dUpdateInventory, // Dest: DT_OBJECT
		&LLToolDragAndDrop::dad3dNULL,//dad3dAssetOnLand, // Dest: DT_LAND
	},
	//	Source: DAD_CALLINGCARD
	{
		&LLToolDragAndDrop::dad3dNULL, // Dest: DT_NONE
		&LLToolDragAndDrop::dad3dNULL, // Dest: DT_SELF
		&LLToolDragAndDrop::dad3dNULL, // Dest: DT_AVATAR
		&LLToolDragAndDrop::dad3dNULL, // Dest: DT_OBJECT
		&LLToolDragAndDrop::dad3dNULL, // Dest: DT_LAND
	},
	//	Source: DAD_LANDMARK
	{
		&LLToolDragAndDrop::dad3dNULL, // Dest: DT_NONE
		&LLToolDragAndDrop::dad3dNULL, // Dest: DT_SELF
		&LLToolDragAndDrop::dad3dGiveInventory, // Dest: DT_AVATAR
		&LLToolDragAndDrop::dad3dUpdateInventory, // Dest: DT_OBJECT
		&LLToolDragAndDrop::dad3dNULL,//dad3dAssetOnLand, // Dest: DT_LAND
	},
	//	Source: DAD_SCRIPT
	{
		&LLToolDragAndDrop::dad3dNULL, // Dest: DT_NONE
		&LLToolDragAndDrop::dad3dNULL, // Dest: DT_SELF
		&LLToolDragAndDrop::dad3dGiveInventory, // Dest: DT_AVATAR
		&LLToolDragAndDrop::dad3dRezScript, // Dest: DT_OBJECT
		&LLToolDragAndDrop::dad3dNULL,//dad3dAssetOnLand, // Dest: DT_LAND
	},
	//	Source: DAD_CLOTHING
	{
		&LLToolDragAndDrop::dad3dNULL, // Dest: DT_NONE
		&LLToolDragAndDrop::dad3dWearItem, // Dest: DT_SELF
		&LLToolDragAndDrop::dad3dGiveInventory, // Dest: DT_AVATAR
		&LLToolDragAndDrop::dad3dUpdateInventory, // Dest: DT_OBJECT
		&LLToolDragAndDrop::dad3dNULL,//dad3dAssetOnLand, // Dest: DT_LAND
	},
	//	Source: DAD_OBJECT
	{
		&LLToolDragAndDrop::dad3dNULL, // Dest: DT_NONE
		&LLToolDragAndDrop::dad3dRezAttachmentFromInv, // Dest: DT_SELF
		&LLToolDragAndDrop::dad3dGiveInventoryObject, // Dest: DT_AVATAR
		&LLToolDragAndDrop::dad3dRezObjectOnObject, // Dest: DT_OBJECT
		&LLToolDragAndDrop::dad3dRezObjectOnLand, // Dest: DT_LAND
	},
	//	Source: DAD_NOTECARD
	{
		&LLToolDragAndDrop::dad3dNULL, // Dest: DT_NONE
		&LLToolDragAndDrop::dad3dNULL, // Dest: DT_SELF
		&LLToolDragAndDrop::dad3dGiveInventory, // Dest: DT_AVATAR
		&LLToolDragAndDrop::dad3dUpdateInventory, // Dest: DT_OBJECT
		&LLToolDragAndDrop::dad3dNULL,//dad3dAssetOnLand, // Dest: DT_LAND
	},
	//	Source: DAD_CATEGORY
	{
		&LLToolDragAndDrop::dad3dNULL, // Dest: DT_NONE
		&LLToolDragAndDrop::dad3dWearCategory, // Dest: DT_SELF
		&LLToolDragAndDrop::dad3dGiveInventoryCategory, // Dest: DT_AVATAR
		&LLToolDragAndDrop::dad3dUpdateInventoryCategory, // Dest: DT_OBJECT
		&LLToolDragAndDrop::dad3dNULL,//dad3dCategoryOnLand, // Dest: DT_LAND
	},
	//	Source: DAD_ROOT
	{
		&LLToolDragAndDrop::dad3dNULL, // Dest: DT_NONE
		&LLToolDragAndDrop::dad3dNULL, // Dest: DT_SELF
		&LLToolDragAndDrop::dad3dNULL, // Dest: DT_AVATAR
		&LLToolDragAndDrop::dad3dNULL, // Dest: DT_OBJECT
		&LLToolDragAndDrop::dad3dNULL, // Dest: DT_LAND
	},
	//	Source: DAD_BODYPART
	{
		&LLToolDragAndDrop::dad3dNULL, // Dest: DT_NONE
		&LLToolDragAndDrop::dad3dWearItem, // Dest: DT_SELF
		&LLToolDragAndDrop::dad3dGiveInventory, // Dest: DT_AVATAR
		&LLToolDragAndDrop::dad3dUpdateInventory, // Dest: DT_OBJECT
		&LLToolDragAndDrop::dad3dNULL,//dad3dAssetOnLand, // Dest: DT_LAND
	},
	//	Source: DAD_ANIMATION
	// TODO: animation on self could play it?  edit it?
	{
		&LLToolDragAndDrop::dad3dNULL, // Dest: DT_NONE
		&LLToolDragAndDrop::dad3dNULL, // Dest: DT_SELF
		&LLToolDragAndDrop::dad3dGiveInventory, // Dest: DT_AVATAR
		&LLToolDragAndDrop::dad3dUpdateInventory, // Dest: DT_OBJECT
		&LLToolDragAndDrop::dad3dNULL,//dad3dAssetOnLand, // Dest: DT_LAND
	},
	//	Source: DAD_GESTURE
	// TODO: gesture on self could play it?  edit it?
	{
		&LLToolDragAndDrop::dad3dNULL, // Dest: DT_NONE
		&LLToolDragAndDrop::dad3dActivateGesture, // Dest: DT_SELF
		&LLToolDragAndDrop::dad3dGiveInventory, // Dest: DT_AVATAR
		&LLToolDragAndDrop::dad3dUpdateInventory, // Dest: DT_OBJECT
		&LLToolDragAndDrop::dad3dNULL,//dad3dAssetOnLand, // Dest: DT_LAND
	},
};

LLToolDragAndDrop::LLToolDragAndDrop()
	 :
	 LLTool("draganddrop", NULL),
	 mDragStartX(0),
	 mDragStartY(0),
	 mSource(SOURCE_AGENT),
	 mCursor(UI_CURSOR_NO),
	 mLastAccept(ACCEPT_NO),
	 mDrop(FALSE),
	 mCurItemIndex(0)
{
	// setup container texture ids
	//for (S32 i = 0; i < LLAssetType::AT_COUNT; i++)
	//{
	//	CONTAINER_TEXTURES[i].set(gViewerArt.getString(CONTAINER_TEXTURE_NAMES[i]));
	//}
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
	if(type == DAD_NONE)
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
	gToolMgr->setTransientTool( this );
	mCursor = UI_CURSOR_NO;
	if((mCargoTypes[0] == DAD_CATEGORY)
	   && ((mSource == SOURCE_AGENT) || (mSource == SOURCE_LIBRARY)))
	{
		LLInventoryCategory* cat = gInventory.getCategory(cargo_id);
		// go ahead and fire & forget the descendents if we are not
		// dragging a protected folder.
		if(cat)
		{
			LLViewerInventoryCategory::cat_array_t cats;
			LLViewerInventoryItem::item_array_t items;
			LLNoPreferredTypeOrItem is_not_preferred;
			LLInventoryFetchComboObserver::folder_ref_t folder_ids;
			LLInventoryFetchComboObserver::item_ref_t item_ids;
			if(is_not_preferred(cat, NULL))
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
			if(!folder_ids.empty() || !item_ids.empty())
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
		if(DAD_NONE == *types_it)
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
	gToolMgr->setTransientTool( this );
	mCursor = UI_CURSOR_NO;
	if((mSource == SOURCE_AGENT) || (mSource == SOURCE_LIBRARY))
	{
		// find cats in the cargo.
		LLInventoryCategory* cat = NULL;
		S32 count = llmin(cargo_ids.size(), types.size());
		std::set<LLUUID> cat_ids;
		for(S32 i = 0; i < count; ++i)
		{
			cat = gInventory.getCategory(cargo_ids[i]);
			if(cat)
			{
				LLViewerInventoryCategory::cat_array_t cats;
				LLViewerInventoryItem::item_array_t items;
				LLNoPreferredType is_not_preferred;
				if(is_not_preferred(cat, NULL))
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
		if(!cat_ids.empty())
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
	gSelectMgr->unhighlightAll();
	setMouseCapture(FALSE);
}

void LLToolDragAndDrop::onMouseCaptureLost()
{
	// Called whenever the drag ends or if mouse captue is simply lost
	gToolMgr->clearTransientTool();
	mCargoTypes.clear();
	mCargoIDs.clear();
	mSource = SOURCE_AGENT;
	mSourceID.setNull();
	mObjectID.setNull();
}

BOOL LLToolDragAndDrop::handleMouseUp( S32 x, S32 y, MASK mask )
{
	if( hasMouseCapture() )
	{
		EAcceptance acceptance = ACCEPT_NO;
		dragOrDrop( x, y, mask, TRUE, &acceptance );
		endDrag();
	}
	return TRUE;
}

BOOL LLToolDragAndDrop::handleHover( S32 x, S32 y, MASK mask )
{
	EAcceptance acceptance = ACCEPT_NO;
	dragOrDrop( x, y, mask, FALSE, &acceptance );

	switch( acceptance )
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
		mCursor = UI_CURSOR_ARROWDRAG;
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
		mCursor = UI_CURSOR_ARROWCOPY;
		break;
	case ACCEPT_POSTPONED:
		break;
	default:
		llassert( FALSE );
	}

	gViewerWindow->getWindow()->setCursor( mCursor );
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

BOOL LLToolDragAndDrop::handleToolTip(S32 x, S32 y, LLString& msg, LLRect *sticky_rect_screen)
{
	if (!mToolTipMsg.empty())
	{
		msg = mToolTipMsg;
		//*stick_rect_screen = gViewerWindow->getWindowRect();
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

	LLView* top_view = gViewerWindow->getTopCtrl();
	LLViewerInventoryItem* item;
	LLViewerInventoryCategory* cat;

	mToolTipMsg.assign("");

	if(top_view)
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
			// drop all items
			if ((U32)*acceptance >= ACCEPT_YES_COPY_MULTI)
			{
				mCurItemIndex = 0;
			}
			// drop just last item
			else
			{
				mCurItemIndex = mCargoIDs.size() - 1;
			}
			for (; mCurItemIndex < (S32)mCargoIDs.size(); mCurItemIndex++)
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

	if(!handled)
	{
		handled = TRUE;

		LLView* root_view = gViewerWindow->getRootView();

		for (mCurItemIndex = 0; mCurItemIndex < (S32)mCargoIDs.size(); mCurItemIndex++)
		{
			LLInventoryObject* cargo = locateInventory(item, cat);

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
			// drop all items
			if ((U32)*acceptance >= ACCEPT_YES_COPY_MULTI)
			{
				mCurItemIndex = 0;
			}
			// drop just last item
			else
			{
				mCurItemIndex = mCargoIDs.size() - 1;
			}
			for (; mCurItemIndex < (S32)mCargoIDs.size(); mCurItemIndex++)
			{
				LLInventoryObject* cargo = locateInventory(item, cat);

				if (cargo)
				{
					//S32 local_x, local_y;

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

	if ( !handled )
	{
		dragOrDrop3D( x, y, mask, drop, acceptance );
	}
}

void LLToolDragAndDrop::dragOrDrop3D( S32 x, S32 y, MASK mask, BOOL drop, EAcceptance* acceptance )
{
	mDrop = drop;
	if (mDrop)
	{
		gPickFaces = TRUE;
		// don't allow drag and drop onto transparent objects
		gViewerWindow->hitObjectOrLandGlobalImmediate(x, y, pickCallback, FALSE);
	}
	else
	{
		// Don't pick faces during hover.  Nothing currently requires per-face
		// data.
		// don't allow drag and drop onto transparent objects
		gViewerWindow->hitObjectOrLandGlobalAsync(x, y, mask, pickCallback, FALSE);
	}

	*acceptance = mLastAccept;
}

void LLToolDragAndDrop::pickCallback(S32 x, S32 y, MASK mask)
{
	EDropTarget target = DT_NONE;
	S32	hit_face = -1;

	LLViewerObject* hit_obj = gViewerWindow->lastNonFloraObjectHit();
	gSelectMgr->unhighlightAll();

	// Treat attachments as part of the avatar they are attached to.
	if (hit_obj)
	{
		if(hit_obj->isAttachment() && !hit_obj->isHUDAttachment())
		{
			LLVOAvatar* avatar = LLVOAvatar::findAvatarFromAttachment( hit_obj );
			if( !avatar )
			{
				gToolDragAndDrop->mLastAccept = ACCEPT_NO;
				gToolDragAndDrop->mCursor = UI_CURSOR_NO;
				gViewerWindow->getWindow()->setCursor( gToolDragAndDrop->mCursor );
				return;
			}
			
			hit_obj = avatar;
		}

		if(hit_obj->isAvatar())
		{
			if(((LLVOAvatar*) hit_obj)->mIsSelf)
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
			hit_face = gLastHitNonFloraObjectFace;
			// if any item being dragged will be applied to the object under our cursor
			// highlight that object
			for (S32 i = 0; i < (S32)gToolDragAndDrop->mCargoIDs.size(); i++)
			{
				if (gToolDragAndDrop->mCargoTypes[i] != DAD_OBJECT || (mask & MASK_CONTROL))
				{
					gSelectMgr->highlightObjectAndFamily(hit_obj);
					break;
				}
			}
		}
	}
	else if(gLastHitLand)
	{
		target = DT_LAND;
		hit_face = -1;
	}

	gToolDragAndDrop->mLastAccept = ACCEPT_YES_MULTI;

	for (gToolDragAndDrop->mCurItemIndex = 0; gToolDragAndDrop->mCurItemIndex < (S32)gToolDragAndDrop->mCargoIDs.size(); 
		gToolDragAndDrop->mCurItemIndex++)
	{
		// Call the right implementation function
		gToolDragAndDrop->mLastAccept = (EAcceptance)llmin(
			(U32)gToolDragAndDrop->mLastAccept,
			(U32)callMemberFunction((*gToolDragAndDrop), 
				gToolDragAndDrop->sDragAndDrop3d[gToolDragAndDrop->mCargoTypes[gToolDragAndDrop->mCurItemIndex]][target])
				(hit_obj, hit_face, mask, FALSE));
	}

	if (gToolDragAndDrop->mDrop && (U32)gToolDragAndDrop->mLastAccept >= ACCEPT_YES_COPY_SINGLE)
	{
		// if target allows multi-drop, go ahead and start iteration at beginning of cargo list
		if (gToolDragAndDrop->mLastAccept >= ACCEPT_YES_COPY_MULTI)
		{
			gToolDragAndDrop->mCurItemIndex = 0;
		}
		// otherwise start at end, to follow selection rules (last selected item is most current)
		else
		{
			gToolDragAndDrop->mCurItemIndex = gToolDragAndDrop->mCargoIDs.size() - 1;
		}

		for (; gToolDragAndDrop->mCurItemIndex < (S32)gToolDragAndDrop->mCargoIDs.size(); 
			gToolDragAndDrop->mCurItemIndex++)
		{
			// Call the right implementation function
			(U32)callMemberFunction((*gToolDragAndDrop), 
				gToolDragAndDrop->sDragAndDrop3d[gToolDragAndDrop->mCargoTypes[gToolDragAndDrop->mCurItemIndex]][target])
				(hit_obj, hit_face, mask, TRUE);
		}
	}

	switch( gToolDragAndDrop->mLastAccept )
	{
	case ACCEPT_YES_MULTI: 
		if (gToolDragAndDrop->mCargoIDs.size() > 1)
		{
			gToolDragAndDrop->mCursor = UI_CURSOR_ARROWDRAGMULTI;
		}
		else
		{
			gToolDragAndDrop->mCursor = UI_CURSOR_ARROWDRAG;
		}
		break;
	case ACCEPT_YES_SINGLE: 
		gToolDragAndDrop->mCursor = UI_CURSOR_ARROWDRAG;
		break;

	case ACCEPT_NO_LOCKED:
		gToolDragAndDrop->mCursor = UI_CURSOR_NOLOCKED;
		break;

	case ACCEPT_NO:
		gToolDragAndDrop->mCursor = UI_CURSOR_NO;
		break;

	case ACCEPT_YES_COPY_MULTI:
	if (gToolDragAndDrop->mCargoIDs.size() > 1)
		{
			gToolDragAndDrop->mCursor = UI_CURSOR_ARROWCOPYMULTI;
		}
		else
		{
			gToolDragAndDrop->mCursor = UI_CURSOR_ARROWCOPY;
		}
		break;
	case ACCEPT_YES_COPY_SINGLE:
		gToolDragAndDrop->mCursor = UI_CURSOR_ARROWCOPY;
		break;
	case ACCEPT_POSTPONED:
		break;
	default:
		llassert( FALSE );
	}

	gToolDragAndDrop->mLastHitPos = gLastHitPosGlobal + gLastHitObjectOffset;
	gToolDragAndDrop->mLastCameraPos = gAgent.getCameraPositionGlobal();

	gViewerWindow->getWindow()->setCursor( gToolDragAndDrop->mCursor );
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
	if(SOURCE_LIBRARY == source)
	{
		// dropping a texture from the library always just works.
		return TRUE;
	}

	if (hit_obj->getInventoryItemByAsset(item->getAssetUUID()))
	{
		// if the asset is already in the object's inventory 
		// then it can always be added to a side.
		// This saves some work if the task's inventory is already loaded
		return TRUE;
	}

	if (!item) return FALSE;
	
	LLPointer<LLViewerInventoryItem> new_item = new LLViewerInventoryItem(item);
	if(!item->getPermissions().allowOperationBy(PERM_COPY, gAgent.getID()))
	{
		// Check that we can add the texture as inventory to the object
		if (willObjectAcceptInventory(hit_obj,item) < ACCEPT_YES_COPY_SINGLE )
		{
			return FALSE;
		}
		// make sure the object has the texture in it's inventory.
		if(SOURCE_AGENT == source)
		{
			// Remove the texture from local inventory. The server
			// will actually remove the item from agent inventory.
			gInventory.deleteObject(item->getUUID());
			gInventory.notifyObservers();
		}
		else if(SOURCE_WORLD == source)
		{
			// *FIX: if the objects are in different regions, and the
			// source region has crashed, you can bypass these
			// permissions.
			LLViewerObject* src_obj = gObjectList.findObject(src_id);
			if(src_obj)
			{
				src_obj->removeInventory(item->getUUID());
			}
			else
			{
				llwarns << "Unable to find source object." << llendl;
				return FALSE;
			}
		}
		hit_obj->updateInventory(new_item, TASK_INVENTORY_ASSET_KEY, true);
	}
	else if(!item->getPermissions().allowOperationBy(PERM_TRANSFER,
													 gAgent.getID()))
	{
		// Check that we can add the testure as inventory to the object
		if (willObjectAcceptInventory(hit_obj,item) < ACCEPT_YES_COPY_SINGLE )
		{
			return FALSE;
		}
		// *FIX: may want to make sure agent can paint hit_obj.

		// make sure the object has the texture in it's inventory.
		hit_obj->updateInventory(new_item, TASK_INVENTORY_ASSET_KEY, true);
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
	if(!success)
	{
		return;
	}
	LLViewerImage* image = gImageList.getImage(asset_id);
	gViewerStats->incStat(LLViewerStats::ST_EDIT_TEXTURE_COUNT );
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
	LLViewerImage* image = gImageList.getImage(item->getAssetUUID());
	
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
	if(!success)
	{
		return;
	}
	// update viewer side image in anticipation of update from simulator
	LLViewerImage* image = gImageList.getImage(asset_id);
	gViewerStats->incStat(LLViewerStats::ST_EDIT_TEXTURE_COUNT );
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
	if((SOURCE_WORLD == gToolDragAndDrop->mSource)
	   || (SOURCE_NOTECARD == gToolDragAndDrop->mSource))
	{
		llwarns << "Call to LLToolDragAndDrop::dropScript() from world"
			<< " or notecard." << llendl;
		return;
	}
	if(hit_obj && item)
	{
		LLPointer<LLViewerInventoryItem> new_script = new LLViewerInventoryItem(item);
		if(!item->getPermissions().allowCopyBy(gAgent.getID()))
		{
			if(SOURCE_AGENT == source)
			{
				// Remove the script from local inventory. The server
				// will actually remove the item from agent inventory.
				gInventory.deleteObject(item->getUUID());
				gInventory.notifyObservers();
			}
			else if(SOURCE_WORLD == source)
			{
				// *FIX: if the objects are in different regions, and
				// the source region has crashed, you can bypass
				// these permissions.
				LLViewerObject* src_obj = gObjectList.findObject(src_id);
				if(src_obj)
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
		LLHUDEffectSpiral *effectp = (LLHUDEffectSpiral *)gHUDManager->createViewerEffect(LLHUDObject::LL_HUD_EFFECT_BEAM, TRUE);
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
	LLViewerRegion* regionp = gWorldp->getRegionFromPosGlobal(mLastHitPos);
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
	if(!item || !item->isComplete()) return;
	
	if (regionp
		&& (regionp->getRegionFlags() & REGION_FLAGS_SANDBOX))
	{
		LLFirstUse::useSandbox();
	}
	// check if it cannot be copied, and mark as remove if it is -
	// this will remove the object from inventory after rez. Only
	// bother with this check if we would not normally remove from
	// inventory.
	if(!remove_from_inventory
		&& !item->getPermissions().allowCopyBy(gAgent.getID()))
	{
		remove_from_inventory = TRUE;
	}

	// Limit raycast to a single object.  
	// Speeds up server raycast + avoid problems with server ray
	// hitting objects that were clipped by the near plane or culled
	// on the viewer.
	LLUUID ray_target_id;
	if( raycast_target )
	{
		ray_target_id = raycast_target->getID();
	}
	else
	{
		ray_target_id.setNull();
	}

	// Check if it's in the trash.
	bool is_in_trash = false;
	LLUUID trash_id;
	trash_id = gInventory.findCategoryUUIDForType(LLAssetType::AT_TRASH);
	if(gInventory.isObjectDescendentOf(item->getUUID(), trash_id))
	{
		is_in_trash = true;
		remove_from_inventory = TRUE;
	}

	LLUUID source_id = from_task_inventory ? mSourceID : LLUUID::null;

	// Select the object only if we're editing.
	BOOL rez_selected = gToolMgr->inEdit();

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
	msg->addVector3Fast(_PREHASH_RayStart, regionp->getPosRegionFromGlobal(mLastCameraPos));
	msg->addVector3Fast(_PREHASH_RayEnd, regionp->getPosRegionFromGlobal(mLastHitPos));
	msg->addUUIDFast(_PREHASH_RayTargetID, ray_target_id );
	msg->addBOOLFast(_PREHASH_RayEndIsIntersection, FALSE);
	msg->addBOOLFast(_PREHASH_RezSelected, rez_selected);
	msg->addBOOLFast(_PREHASH_RemoveItem, remove_from_inventory);

	// deal with permissions slam logic
	pack_permissions_slam(msg, item->getFlags(), item->getPermissions());

	LLUUID folder_id = item->getParentUUID();
	if((SOURCE_LIBRARY == mSource) || (is_in_trash))
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
		gSelectMgr->deselectAll();
		gViewerWindow->getWindow()->incBusyCount();
	}

	if(remove_from_inventory)
	{
		// Delete it from inventory immediately so that users cannot
		// easily bypass copy protection in laggy situations. If the
		// rez fails, we will put it back on the server.
		gInventory.deleteObject(item->getUUID());
		gInventory.notifyObservers();
	}

	// VEFFECT: DropObject
	LLHUDEffectSpiral *effectp = (LLHUDEffectSpiral *)gHUDManager->createViewerEffect(LLHUDObject::LL_HUD_EFFECT_BEAM, TRUE);
	effectp->setSourceObject(gAgent.getAvatarObject());
	effectp->setPositionGlobal(mLastHitPos);
	effectp->setDuration(LL_HUD_DUR_SHORT);
	effectp->setColor(LLColor4U(gAgent.getEffectColor()));

	gViewerStats->incStat(LLViewerStats::ST_REZ_COUNT);
}

void LLToolDragAndDrop::dropInventory(LLViewerObject* hit_obj,
									  LLInventoryItem* item,
									  LLToolDragAndDrop::ESource source,
									  const LLUUID& src_id)
{
	// *HACK: In order to resolve SL-22177, we need to block drags
	// from notecards and objects onto other objects.
	if((SOURCE_WORLD == gToolDragAndDrop->mSource)
	   || (SOURCE_NOTECARD == gToolDragAndDrop->mSource))
	{
		llwarns << "Call to LLToolDragAndDrop::dropInventory() from world"
			<< " or notecard." << llendl;
		return;
	}

	LLPointer<LLViewerInventoryItem> new_item = new LLViewerInventoryItem(item);
	S32 creation_date = time_corrected();
	new_item->setCreationDate(creation_date);

	if(!item->getPermissions().allowCopyBy(gAgent.getID()))
	{
		if(SOURCE_AGENT == source)
		{
			// Remove the inventory item from local inventory. The
			// server will actually remove the item from agent
			// inventory.
			gInventory.deleteObject(item->getUUID());
			gInventory.notifyObservers();
		}
		else if(SOURCE_WORLD == source)
		{
			// *FIX: if the objects are in different regions, and the
			// source region has crashed, you can bypass these
			// permissions.
			LLViewerObject* src_obj = gObjectList.findObject(src_id);
			if(src_obj)
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
	if (gFloaterTools->getVisible())
	{
		// *FIX: only show this if panel not expanded?
		gFloaterTools->showPanel(LLFloaterTools::PANEL_CONTENTS);
	}

	// VEFFECT: AddToInventory
	LLHUDEffectSpiral *effectp = (LLHUDEffectSpiral *)gHUDManager->createViewerEffect(LLHUDObject::LL_HUD_EFFECT_BEAM, TRUE);
	effectp->setSourceObject(gAgent.getAvatarObject());
	effectp->setTargetObject(hit_obj);
	effectp->setDuration(LL_HUD_DUR_SHORT);
	effectp->setColor(LLColor4U(gAgent.getEffectColor()));
	gFloaterTools->dirty();
}

struct LLGiveInventoryInfo
{
	LLUUID mToAgentID;
	LLUUID mInventoryObjectID;
	LLGiveInventoryInfo(const LLUUID& to_agent, const LLUUID& obj_id) :
		mToAgentID(to_agent), mInventoryObjectID(obj_id) {}
};

void LLToolDragAndDrop::giveInventory(const LLUUID& to_agent,
									  LLInventoryItem* item)
{
	llinfos << "LLToolDragAndDrop::giveInventory()" << llendl;
	if(!isInventoryGiveAcceptable(item))
	{
		return;
	}
	if(item->getPermissions().allowCopyBy(gAgent.getID()))
	{
		// just give it away.
		LLToolDragAndDrop::commitGiveInventoryItem(to_agent, item);
	}
	else
	{
		// ask if the agent is sure.
		LLGiveInventoryInfo* info = new LLGiveInventoryInfo(to_agent,
															item->getUUID());

		gViewerWindow->alertXml("CannotCopyWarning",
								  &LLToolDragAndDrop::handleCopyProtectedItem,
								  (void*)info);
	}
}

// static
void LLToolDragAndDrop::handleCopyProtectedItem(S32 option, void* data)
{
	LLGiveInventoryInfo* info = (LLGiveInventoryInfo*)data;
	LLInventoryItem* item = NULL;
	switch(option)
	{
	case 0:  // "Yes"
		item = gInventory.getItem(info->mInventoryObjectID);
		if(item)
		{
			LLToolDragAndDrop::commitGiveInventoryItem(info->mToAgentID,
													   item);
			// delete it for now - it will be deleted on the server
			// quickly enough.
			gInventory.deleteObject(info->mInventoryObjectID);
			gInventory.notifyObservers();
		}
		else
		{
			gViewerWindow->alertXml("CannotGiveItem");		
		}
		break;

	default: // no, cancel, whatever, who cares, not yes.
		gViewerWindow->alertXml("TransactionCancelled");
		break;
	}
}

// static
void LLToolDragAndDrop::commitGiveInventoryItem(const LLUUID& to_agent,
												LLInventoryItem* item)
{
	if(!item) return;
	std::string name;
	gAgent.buildFullname(name);
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
		name.c_str(),
		item->getName().c_str(),
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
	LLHUDEffectSpiral *effectp = (LLHUDEffectSpiral *)gHUDManager->createViewerEffect(LLHUDObject::LL_HUD_EFFECT_BEAM, TRUE);
	effectp->setSourceObject(gAgent.getAvatarObject());
	effectp->setTargetObject(gObjectList.findObject(to_agent));
	effectp->setDuration(LL_HUD_DUR_SHORT);
	effectp->setColor(LLColor4U(gAgent.getEffectColor()));
	gFloaterTools->dirty();
}

void LLToolDragAndDrop::giveInventoryCategory(const LLUUID& to_agent,
											  LLInventoryCategory* cat)
{
	if(!cat) return;
	llinfos << "LLToolDragAndDrop::giveInventoryCategory() - "
			<< cat->getUUID() << llendl;

	LLVOAvatar* my_avatar = gAgent.getAvatarObject();
	if( !my_avatar )
	{
		return;
	}

	// Test out how many items are being given.
	LLViewerInventoryCategory::cat_array_t cats;
	LLViewerInventoryItem::item_array_t items;
	LLGiveable giveable;
	gInventory.collectDescendentsIf(cat->getUUID(),
									cats,
									items,
									LLInventoryModel::EXCLUDE_TRASH,
									giveable);
	S32 count = cats.count();
	bool complete = true;
	for(S32 i = 0; i < count; ++i)
	{
		if(!gInventory.isCategoryComplete(cats.get(i)->getUUID()))
		{
			complete = false;
			break;
		}
	}
	if(!complete)
	{
		LLNotifyBox::showXml("IncompleteInventory");
		return;
	}
 	count = items.count() + cats.count();
 	if(count > MAX_ITEMS)
  	{
		gViewerWindow->alertXml("TooManyItems");
  		return;
  	}
 	else if(count == 0)
  	{
		gViewerWindow->alertXml("NoItems");
  		return;
  	}
	else
	{
		if(0 == giveable.countNoCopy())
		{
			LLToolDragAndDrop::commitGiveInventoryCategory(to_agent, cat);
		}
		else 
		{
			LLGiveInventoryInfo* info = NULL;
			info = new LLGiveInventoryInfo(to_agent, cat->getUUID());
			LLStringBase<char>::format_map_t args;
			args["[COUNT]"] = llformat("%d",giveable.countNoCopy());
			gViewerWindow->alertXml("CannotCopyCountItems", args,
				&LLToolDragAndDrop::handleCopyProtectedCategory,
				(void*)info);
				
		}
	}
}


// static
void LLToolDragAndDrop::handleCopyProtectedCategory(S32 option, void* data)
{
	LLGiveInventoryInfo* info = (LLGiveInventoryInfo*)data;
	LLInventoryCategory* cat = NULL;
	switch(option)
	{
	case 0:  // "Yes"
		cat = gInventory.getCategory(info->mInventoryObjectID);
		if(cat)
		{
			LLToolDragAndDrop::commitGiveInventoryCategory(info->mToAgentID,
														   cat);
			LLViewerInventoryCategory::cat_array_t cats;
			LLViewerInventoryItem::item_array_t items;
			LLUncopyableItems remove;
			gInventory.collectDescendentsIf(cat->getUUID(),
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
			gViewerWindow->alertXml("CannotGiveCategory");
		}
		break;

	default: // no, cancel, whatever, who cares, not yes.
		gViewerWindow->alertXml("TransactionCancelled");
		break;
	}
}

// static
void LLToolDragAndDrop::commitGiveInventoryCategory(const LLUUID& to_agent,
													LLInventoryCategory* cat)
{
	if(!cat) return;
	llinfos << "LLToolDragAndDrop::commitGiveInventoryCategory() - "
			<< cat->getUUID() << llendl;

	// Test out how many items are being given.
	LLViewerInventoryCategory::cat_array_t cats;
	LLViewerInventoryItem::item_array_t items;
	LLGiveable giveable;
	gInventory.collectDescendentsIf(cat->getUUID(),
									cats,
									items,
									LLInventoryModel::EXCLUDE_TRASH,
									giveable);

	// MAX ITEMS is based on (sizeof(uuid)+2) * count must be <
	// MTUBYTES or 18 * count < 1200 => count < 1200/18 =>
	// 66. I've cut it down a bit from there to give some pad.
 	S32 count = items.count() + cats.count();
 	if(count > MAX_ITEMS)
  	{
		gViewerWindow->alertXml("TooManyItems");
  		return;
  	}
 	else if(count == 0)
  	{
		gViewerWindow->alertXml("NoItems");
  		return;
  	}
	else
	{
		std::string name;
		gAgent.buildFullname(name);
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
			name.c_str(),
			cat->getName().c_str(),
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
		LLHUDEffectSpiral *effectp = (LLHUDEffectSpiral *)gHUDManager->createViewerEffect(LLHUDObject::LL_HUD_EFFECT_BEAM, TRUE);
		effectp->setSourceObject(gAgent.getAvatarObject());
		effectp->setTargetObject(gObjectList.findObject(to_agent));
		effectp->setDuration(LL_HUD_DUR_SHORT);
		effectp->setColor(LLColor4U(gAgent.getEffectColor()));
		gFloaterTools->dirty();
	}
}

// static
BOOL LLToolDragAndDrop::isInventoryGiveAcceptable(LLInventoryItem* item)
{
	if(!item)
	{
		return FALSE;
	}
	if(!item->getPermissions().allowOperationBy(PERM_TRANSFER, gAgent.getID()))
	{
		return FALSE;
	}
	BOOL copyable = FALSE;
	if(item->getPermissions().allowCopyBy(gAgent.getID())) copyable = TRUE;
	LLVOAvatar* my_avatar = gAgent.getAvatarObject();
	if(!my_avatar)
	{
		return FALSE;
	}
	BOOL acceptable = TRUE;
	switch(item->getType())
	{
	case LLAssetType::AT_CALLINGCARD:
		acceptable = FALSE;
		break;
	case LLAssetType::AT_OBJECT:
		if(my_avatar->isWearingAttachment(item->getUUID()))
		{
			acceptable = FALSE;
		}
		break;
	case LLAssetType::AT_BODYPART:
	case LLAssetType::AT_CLOTHING:
		if(!copyable && gAgent.isWearingItem(item->getUUID()))
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
	if(!item)
	{
		return FALSE;
	}

	// These permissions are double checked in the simulator in
	// LLGroupNoticeInventoryItemFetch::result().
	if(!item->getPermissions().allowOperationBy(PERM_TRANSFER, gAgent.getID()))
	{
		return FALSE;
	}
	if(!item->getPermissions().allowCopyBy(gAgent.getID()))
	{
		return FALSE;
	}

	LLVOAvatar* my_avatar = gAgent.getAvatarObject();
	if(!my_avatar)
	{
		return FALSE;
	}

	BOOL acceptable = TRUE;
	switch(item->getType())
	{
	case LLAssetType::AT_CALLINGCARD:
		acceptable = FALSE;
		break;
	case LLAssetType::AT_OBJECT:
		if(my_avatar->isWearingAttachment(item->getUUID()))
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
	if(!item || !obj) return ACCEPT_NO;
	// HACK: downcast
	LLViewerInventoryItem* vitem = (LLViewerInventoryItem*)item;
	if(!vitem->isComplete()) return ACCEPT_NO;

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
	LLVOAvatar* my_avatar = NULL;
	switch(item->getType())
	{
	case LLAssetType::AT_OBJECT:
		my_avatar = gAgent.getAvatarObject();
		if(my_avatar && my_avatar->isWearingAttachment(item->getUUID()))
		{
				worn = TRUE;
		}
		break;
	case LLAssetType::AT_BODYPART:
	case LLAssetType::AT_CLOTHING:
		if(gAgent.isWearingItem(item->getUUID()))
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
	if(!item || !item->isComplete()) return ACCEPT_NO;

	// must not be in the trash
	LLUUID trash_id(gInventory.findCategoryUUIDForType(LLAssetType::AT_TRASH));
	if( gInventory.isObjectDescendentOf( item->getUUID(), trash_id ) )
	{
		return ACCEPT_NO;
	}

	// must not be already wearing it
	LLVOAvatar* avatar = gAgent.getAvatarObject();
	if( !avatar || avatar->isWearingAttachment(item->getUUID()) )
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
	if(!item || !item->isComplete()) return ACCEPT_NO;

	LLVOAvatar* my_avatar = gAgent.getAvatarObject();
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
	LLUUID trash_id;
	trash_id = gInventory.findCategoryUUIDForType(LLAssetType::AT_TRASH);
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
	if(!item || !item->isComplete()) return ACCEPT_NO;
	LLVOAvatar* my_avatar = gAgent.getAvatarObject();
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
	LLUUID trash_id;
	trash_id = gInventory.findCategoryUUIDForType(LLAssetType::AT_TRASH);
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
	if(!item || !item->isComplete()) return ACCEPT_NO;
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

EAcceptance LLToolDragAndDrop::dad3dTextureObject(
	LLViewerObject* obj, S32 face, MASK mask, BOOL drop)
{
	lldebugs << "LLToolDragAndDrop::dad3dTextureObject()" << llendl;

	// *HACK: In order to resolve SL-22177, we need to block drags
	// from notecards and objects onto other objects.
	if((SOURCE_WORLD == mSource) || (SOURCE_NOTECARD == mSource))
	{
		return ACCEPT_NO;
	}
	
	LLViewerInventoryItem* item;
	LLViewerInventoryCategory* cat;
	locateInventory(item, cat);
	if(!item || !item->isComplete()) return ACCEPT_NO;
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
		if((mask & MASK_SHIFT))
		{
			dropTextureAllFaces(obj, item, mSource, mSourceID);
		}
		else
		{
			dropTextureOneFace(obj, face, item, mSource, mSourceID);
		}
		
		// VEFFECT: SetTexture
		LLHUDEffectSpiral *effectp = (LLHUDEffectSpiral *)gHUDManager->createViewerEffect(LLHUDObject::LL_HUD_EFFECT_BEAM, TRUE);
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
	if(!item || !item->isComplete()) return ACCEPT_NO;

	if(mSource == SOURCE_AGENT || mSource == SOURCE_LIBRARY)
	{
		// it's in the agent inventory
		LLUUID trash_id = gInventory.findCategoryUUIDForType(LLAssetType::AT_TRASH);
		if( gInventory.isObjectDescendentOf( item->getUUID(), trash_id ) )
		{
			return ACCEPT_NO;
		}

		if( drop )
		{
			// Don't wear anything until initial wearables are loaded, can
			// destroy clothing items.
			if (!gAgent.areWearablesLoaded()) 
			{
				gViewerWindow->alertXml("CanNotChangeAppearanceUntilLoaded");
				return ACCEPT_NO;
			}

			if(mSource == SOURCE_LIBRARY)
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
	if(!item || !item->isComplete()) return ACCEPT_NO;

	if(mSource == SOURCE_AGENT || mSource == SOURCE_LIBRARY)
	{
		// it's in the agent inventory
		LLUUID trash_id = gInventory.findCategoryUUIDForType(LLAssetType::AT_TRASH);
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
				gGestureManager.activateGesture(item->getUUID());
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
		// Don't wear anything until initial wearables are loaded, can
		// destroy clothing items.
		if (!gAgent.areWearablesLoaded()) 
		{
			gViewerWindow->alertXml("CanNotChangeAppearanceUntilLoaded");
			return ACCEPT_NO;
		}
	}

	if(mSource == SOURCE_AGENT)
	{
		LLUUID trash_id(gInventory.findCategoryUUIDForType(LLAssetType::AT_TRASH));
		if( gInventory.isObjectDescendentOf( category->getUUID(), trash_id ) )
		{
			return ACCEPT_NO;
		}

		if(drop)
		{
		    BOOL append = ( (mask & MASK_SHIFT) ? TRUE : FALSE );
			wear_inventory_category(category, false, append);
		}
		return ACCEPT_YES_MULTI;
	}
	else if(mSource == SOURCE_LIBRARY)
	{
		if(drop)
		{
			wear_inventory_category(category, true, false);
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
	if(!item || !item->isComplete()) return ACCEPT_NO;
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
	if (NULL==obj)
	{
		llwarns << "obj is NULL; aborting func with ACCEPT_NO" << llendl;
		return ACCEPT_NO;
	}

	if (mSource != SOURCE_AGENT && mSource != SOURCE_LIBRARY)
	{
		return ACCEPT_NO;
	}
	if(obj->isAttachment()) return ACCEPT_NO_LOCKED;
	LLViewerInventoryItem* item;
	LLViewerInventoryCategory* cat;
	locateInventory(item, cat);
	if(!cat) return ACCEPT_NO;
	EAcceptance rv = ACCEPT_NO;

	// find all the items in the category
	LLDroppableItem droppable(!obj->permYouOwner());
	LLInventoryModel::cat_array_t cats;
	LLInventoryModel::item_array_t items;
	gInventory.collectDescendentsIf(cat->getUUID(),
					cats,
					items,
					LLInventoryModel::EXCLUDE_TRASH,
					droppable);
	cats.put(cat);
 	if(droppable.countNoCopy() > 0)
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

	// Check for accept
	S32 i;
	S32 count = cats.count();
	for(i = 0; i < count; ++i)
	{
		rv = gInventory.isCategoryComplete(cats.get(i)->getUUID()) ? ACCEPT_YES_MULTI : ACCEPT_NO;
		if(rv < ACCEPT_YES_SINGLE)
		{
			lldebugs << "Category " << cats.get(i)->getUUID()
					 << "is not complete." << llendl;
			break;
		}
	}
	if(ACCEPT_YES_COPY_SINGLE <= rv)
	{
		count = items.count();
		for(i = 0; i < count; ++i)
		{
			rv = willObjectAcceptInventory(root_object, items.get(i));
			if(rv < ACCEPT_YES_COPY_SINGLE)
			{
				lldebugs << "Object will not accept "
						 << items.get(i)->getUUID() << llendl;
				break;
			}
		}
	}

	// if every item is accepted, go ahead and send it on.
	if(drop && (ACCEPT_YES_COPY_SINGLE <= rv))
	{
		S32 count = items.count();
		LLInventoryFetchObserver::item_ref_t ids;
		for(i = 0; i < count; ++i)
		{
			//dropInventory(root_object, items.get(i), mSource, mSourceID);
			ids.push_back(items.get(i)->getUUID());
		}
		LLCategoryDropObserver* dropper;
		dropper = new LLCategoryDropObserver(obj->getID(), mSource);
		dropper->fetchItems(ids);
		if(dropper->isEverythingComplete())
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
	if(!item || !item->isComplete()) return ACCEPT_NO;
	if(!item->getPermissions().allowOperationBy(PERM_TRANSFER, gAgent.getID()))
	{
		// cannot give away no-transfer objects
		return ACCEPT_NO;
	}
	LLVOAvatar* avatar = gAgent.getAvatarObject();
	if(avatar && avatar->isWearingAttachment( item->getUUID() ) )
	{
		// You can't give objects that are attached to you
		return ACCEPT_NO;
	}
	if( obj && avatar )
	{
		if(drop)
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
	if(mSource != SOURCE_AGENT) return ACCEPT_NO;
	LLViewerInventoryItem* item;
	LLViewerInventoryCategory* cat;
	locateInventory(item, cat);
	if(!item || !item->isComplete()) return ACCEPT_NO;
	if(!isInventoryGiveAcceptable(item))
	{
		return ACCEPT_NO;
	}
	if(drop && obj)
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
	if(drop && obj)
	{
		LLViewerInventoryItem* item;
		LLViewerInventoryCategory* cat;
		locateInventory(item, cat);
		if(!cat) return ACCEPT_NO;
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
	if(!item || !item->isComplete()) return ACCEPT_NO;
	if(!item->getPermissions().allowCopyBy(gAgent.getID(),
										   gAgent.getGroupID())
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
	if(!item || !item->isComplete()) return ACCEPT_NO;
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
		LLPreviewNotecard* card;
		card = (LLPreviewNotecard*)LLPreview::find(mSourceID);
		if(card)
		{
			item = (LLViewerInventoryItem*)card->getDragItem();
		}
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

void LLToolDragAndDrop::createContainer(LLViewerInventoryItem::item_array_t &items, const char* preferred_name )
{
	llwarns << "LLToolDragAndDrop::createContainer()" << llendl;
	return;
}


// utility functions

void pack_permissions_slam(LLMessageSystem* msg, U32 flags, const LLPermissions& perms)
{
	U32 group_mask		= perms.getMaskGroup();
	U32 everyone_mask	= perms.getMaskEveryone();
	U32 next_owner_mask	= perms.getMaskNextOwner();
	
	msg->addU32Fast(_PREHASH_ItemFlags, flags);
	msg->addU32Fast(_PREHASH_GroupMask, group_mask);
	msg->addU32Fast(_PREHASH_EveryoneMask, everyone_mask);
	msg->addU32Fast(_PREHASH_NextOwnerMask, next_owner_mask);
}
