/** 
 * @file llinventorymodel.cpp
 * @brief Implementation of the inventory model used to track agent inventory.
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

#include "llinventorymodel.h"

#include "llassetstorage.h"
#include "llcrc.h"
#include "lldir.h"
#include "llsys.h"
#include "llxfermanager.h"
#include "message.h"

#include "llagent.h"
#include "llagentwearables.h"
#include "llfloater.h"
#include "llfocusmgr.h"
#include "llinventorybridge.h"
#include "llfloaterinventory.h"
#include "llviewerinventory.h"
#include "llviewermessage.h"
#include "llviewerwindow.h"
#include "llviewerregion.h"
#include "llappviewer.h"
#include "lldbstrings.h"
#include "llviewerstats.h"
#include "llmutelist.h"
#include "llnotifications.h"
#include "llcallbacklist.h"
#include "llpreview.h"
#include "llviewercontrol.h"
#include "llvoavatarself.h"
#include "llsdutil.h"
#include <deque>

//#define DIFF_INVENTORY_FILES
#ifdef DIFF_INVENTORY_FILES
#include "process.h"
#endif

BOOL LLInventoryModel::sBackgroundFetchActive = FALSE;
BOOL LLInventoryModel::sAllFoldersFetched = FALSE;
BOOL LLInventoryModel::sFullFetchStarted = FALSE;
S32  LLInventoryModel::sNumFetchRetries = 0;
F32  LLInventoryModel::sMinTimeBetweenFetches = 0.3f;
F32  LLInventoryModel::sMaxTimeBetweenFetches = 10.f;
BOOL LLInventoryModel::sTimelyFetchPending = FALSE;
LLFrameTimer LLInventoryModel::sFetchTimer;
S16 LLInventoryModel::sBulkFetchCount = 0;

// RN: for some reason, using std::queue in the header file confuses the compiler which things it's an xmlrpc_queue
static std::deque<LLUUID> sFetchQueue;

///----------------------------------------------------------------------------
/// Local function declarations, constants, enums, and typedefs
///----------------------------------------------------------------------------

//BOOL decompress_file(const char* src_filename, const char* dst_filename);
const F32 MAX_TIME_FOR_SINGLE_FETCH = 10.f;
const S32 MAX_FETCH_RETRIES = 10;
const char CACHE_FORMAT_STRING[] = "%s.inv"; 

struct InventoryIDPtrLess
{
	bool operator()(const LLViewerInventoryCategory* i1, const LLViewerInventoryCategory* i2) const
	{
		return (i1->getUUID() < i2->getUUID());
	}
};

class LLCanCache : public LLInventoryCollectFunctor 
{
public:
	LLCanCache(LLInventoryModel* model) : mModel(model) {}
	virtual ~LLCanCache() {}
	virtual bool operator()(LLInventoryCategory* cat, LLInventoryItem* item);
protected:
	LLInventoryModel* mModel;
	std::set<LLUUID> mCachedCatIDs;
};

bool LLCanCache::operator()(LLInventoryCategory* cat, LLInventoryItem* item)
{
	bool rv = false;
	if(item)
	{
		if(mCachedCatIDs.find(item->getParentUUID()) != mCachedCatIDs.end())
		{
			rv = true;
		}
	}
	else if(cat)
	{
		// HACK: downcast
		LLViewerInventoryCategory* c = (LLViewerInventoryCategory*)cat;
		if(c->getVersion() != LLViewerInventoryCategory::VERSION_UNKNOWN)
		{
			S32 descendents_server = c->getDescendentCount();
			LLInventoryModel::cat_array_t* cats;
			LLInventoryModel::item_array_t* items;
			mModel->getDirectDescendentsOf(
				c->getUUID(),
				cats,
				items);
			S32 descendents_actual = 0;
			if(cats && items)
			{
				descendents_actual = cats->count() + items->count();
			}
			if(descendents_server == descendents_actual)
			{
				mCachedCatIDs.insert(c->getUUID());
				rv = true;
			}
		}
	}
	return rv;
}

///----------------------------------------------------------------------------
/// Class LLInventoryModel
///----------------------------------------------------------------------------

// global for the agent inventory.
LLInventoryModel gInventory;

// Default constructor
LLInventoryModel::LLInventoryModel()
:	mModifyMask(LLInventoryObserver::ALL),
	mChangedItemIDs(),
	mCategoryMap(),
	mItemMap(),
	mCategoryLock(),
	mItemLock(),
	mLastItem(NULL),
	mParentChildCategoryTree(),
	mParentChildItemTree(),
	mObservers(),
	mRootFolderID(),
	mLibraryRootFolderID(),
	mLibraryOwnerID(),
	mIsAgentInvUsable(false)
{
}

// Destroys the object
LLInventoryModel::~LLInventoryModel()
{
	empty();
	for (observer_list_t::iterator iter = mObservers.begin();
		 iter != mObservers.end(); ++iter)
	{
		delete *iter;
	}
}

// This is a convenience function to check if one object has a parent
// chain up to the category specified by UUID.
BOOL LLInventoryModel::isObjectDescendentOf(const LLUUID& obj_id,
											const LLUUID& cat_id)
{
	LLInventoryObject* obj = getObject(obj_id);
	while(obj)
	{
		const LLUUID& parent_id = obj->getParentUUID();
		if( parent_id.isNull() )
		{
			return FALSE;
		}
		if(parent_id == cat_id)
		{
			return TRUE;
		}
		// Since we're scanning up the parents, we only need to check
		// in the category list.
		obj = getCategory(parent_id);
	}
	return FALSE;
}

// Get the object by id. Returns NULL if not found.
LLInventoryObject* LLInventoryModel::getObject(const LLUUID& id) const
{
	LLViewerInventoryCategory* cat = getCategory(id);
	if (cat)
	{
		return cat;
	}
	LLViewerInventoryItem* item = getItem(id);
	if (item)
	{
		return item;
	}
	return NULL;
}

// Get the item by id. Returns NULL if not found.
LLViewerInventoryItem* LLInventoryModel::getItem(const LLUUID& id) const
{
	LLViewerInventoryItem* item = NULL;
	if(mLastItem.notNull() && mLastItem->getUUID() == id)
	{
		item = mLastItem;
	}
	else
	{
		item_map_t::const_iterator iter = mItemMap.find(id);
		if (iter != mItemMap.end())
		{
			item = iter->second;
			mLastItem = item;
		}
	}
	return item;
}

// Get the category by id. Returns NULL if not found
LLViewerInventoryCategory* LLInventoryModel::getCategory(const LLUUID& id) const
{
	LLViewerInventoryCategory* category = NULL;
	cat_map_t::const_iterator iter = mCategoryMap.find(id);
	if (iter != mCategoryMap.end())
	{
		category = iter->second;
	}
	return category;
}

S32 LLInventoryModel::getItemCount() const
{
	return mItemMap.size();
}

S32 LLInventoryModel::getCategoryCount() const
{
	return mCategoryMap.size();
}

// Return the direct descendents of the id provided. The array
// provided points straight into the guts of this object, and
// should only be used for read operations, since modifications
// may invalidate the internal state of the inventory. Set passed
// in values to NULL if the call fails.
void LLInventoryModel::getDirectDescendentsOf(const LLUUID& cat_id,
											  cat_array_t*& categories,
											  item_array_t*& items) const
{
	categories = get_ptr_in_map(mParentChildCategoryTree, cat_id);
	items = get_ptr_in_map(mParentChildItemTree, cat_id);
}

// SJB: Added version to lock the arrays to catch potential logic bugs
void LLInventoryModel::lockDirectDescendentArrays(const LLUUID& cat_id,
												  cat_array_t*& categories,
												  item_array_t*& items)
{
	getDirectDescendentsOf(cat_id, categories, items);
	if (categories)
	{
		mCategoryLock[cat_id] = true;
	}
	if (items)
	{
		mItemLock[cat_id] = true;
	}
}

void LLInventoryModel::unlockDirectDescendentArrays(const LLUUID& cat_id)
{
	mCategoryLock[cat_id] = false;
	mItemLock[cat_id] = false;
}

// findCategoryUUIDForType() returns the uuid of the category that
// specifies 'type' as what it defaults to containing. The category is
// not necessarily only for that type. *NOTE: This will create a new
// inventory category on the fly if one does not exist.
LLUUID LLInventoryModel::findCategoryUUIDForType(LLAssetType::EType t, bool create_folder)
{
	LLUUID rv = findCatUUID(t);
	if(rv.isNull() && isInventoryUsable() && create_folder)
	{
		LLUUID root_id = gInventory.getRootFolderID();
		if(root_id.notNull())
		{
			rv = createNewCategory(root_id, t, LLStringUtil::null);
		}
	}
	return rv;
}

// Internal method which looks for a category with the specified
// preferred type. Returns LLUUID::null if not found.
LLUUID LLInventoryModel::findCatUUID(LLAssetType::EType preferred_type)
{
	LLUUID root_id = gInventory.getRootFolderID();
	if(LLAssetType::AT_CATEGORY == preferred_type)
	{
		return root_id;
	}
	if(root_id.notNull())
	{
		cat_array_t* cats = NULL;
		cats = get_ptr_in_map(mParentChildCategoryTree, root_id);
		if(cats)
		{
			S32 count = cats->count();
			for(S32 i = 0; i < count; ++i)
			{
				if(cats->get(i)->getPreferredType() == preferred_type)
				{
					return cats->get(i)->getUUID();
				}
			}
		}
	}
	return LLUUID::null;
}

// Convenience function to create a new category. You could call
// updateCategory() with a newly generated UUID category, but this
// version will take care of details like what the name should be
// based on preferred type. Returns the UUID of the new category.
LLUUID LLInventoryModel::createNewCategory(const LLUUID& parent_id,
										   LLAssetType::EType preferred_type,
										   const std::string& pname)
{
	LLUUID id;
	if(!isInventoryUsable())
	{
		llwarns << "Inventory is broken." << llendl;
		return id;
	}

	if(preferred_type == LLAssetType::AT_SIMSTATE)
	{
		lldebugs << "Attempt to create simstate category." << llendl;
		return id;
	}

	id.generate();
	std::string name = pname;
	if(!pname.empty())
	{
		name.assign(pname);
	}
	else
	{
		name.assign(LLAssetType::lookupCategoryName(preferred_type));
	}

	// Add the category to the internal representation
	LLPointer<LLViewerInventoryCategory> cat =
		new LLViewerInventoryCategory(id, parent_id, preferred_type, name, gAgent.getID());
	cat->setVersion(LLViewerInventoryCategory::VERSION_INITIAL);
	cat->setDescendentCount(0);
	LLCategoryUpdate update(cat->getParentUUID(), 1);
	accountForUpdate(update);
	updateCategory(cat);

	// Create the category on the server. We do this to prevent people
	// from munging their protected folders.
	LLMessageSystem* msg = gMessageSystem;
	msg->newMessage("CreateInventoryFolder");
	msg->nextBlock("AgentData");
	msg->addUUID("AgentID", gAgent.getID());
	msg->addUUID(_PREHASH_SessionID, gAgent.getSessionID());
	msg->nextBlock("FolderData");
	cat->packMessage(msg);
	gAgent.sendReliableMessage();

	// return the folder id of the newly created folder
	return id;
}

// Starting with the object specified, add it's descendents to the
// array provided, but do not add the inventory object specified by
// id. There is no guaranteed order. Neither array will be erased
// before adding objects to it. Do not store a copy of the pointers
// collected - use them, and collect them again later if you need to
// reference the same objects.

class LLAlwaysCollect : public LLInventoryCollectFunctor
{
public:
	virtual ~LLAlwaysCollect() {}
	virtual bool operator()(LLInventoryCategory* cat,
							LLInventoryItem* item)
	{
		return TRUE;
	}
};

void LLInventoryModel::collectDescendents(const LLUUID& id,
										  cat_array_t& cats,
										  item_array_t& items,
										  BOOL include_trash)
{
	LLAlwaysCollect always;
	collectDescendentsIf(id, cats, items, include_trash, always);
}

void LLInventoryModel::collectDescendentsIf(const LLUUID& id,
											cat_array_t& cats,
											item_array_t& items,
											BOOL include_trash,
											LLInventoryCollectFunctor& add)
{
	// Start with categories
	if(!include_trash)
	{
		LLUUID trash_id(findCatUUID(LLAssetType::AT_TRASH));
		if(trash_id.notNull() && (trash_id == id))
			return;
	}
	cat_array_t* cat_array = get_ptr_in_map(mParentChildCategoryTree, id);
	if(cat_array)
	{
		S32 count = cat_array->count();
		for(S32 i = 0; i < count; ++i)
		{
			LLViewerInventoryCategory* cat = cat_array->get(i);
			if(add(cat,NULL))
			{
				cats.put(cat);
			}
			collectDescendentsIf(cat->getUUID(), cats, items, include_trash, add);
		}
	}

	// Move onto items
	LLViewerInventoryItem* item = NULL;
	item_array_t* item_array = get_ptr_in_map(mParentChildItemTree, id);
	if(item_array)
	{
		S32 count = item_array->count();
		for(S32 i = 0; i < count; ++i)
		{
			item = item_array->get(i);
			if(add(NULL, item))
			{
				items.put(item);
			}
		}
	}
}

void LLInventoryModel::updateLinkedObjects(const LLUUID& object_id)
{
	LLInventoryModel::cat_array_t cat_array;
	LLInventoryModel::item_array_t item_array;
	LLLinkedItemIDMatches is_linked_item_match(object_id);
	collectDescendentsIf(gInventory.getRootFolderID(),
						 cat_array,
						 item_array,
						 LLInventoryModel::INCLUDE_TRASH,
						 is_linked_item_match);

	for (LLInventoryModel::cat_array_t::iterator cat_iter = cat_array.begin();
		 cat_iter != cat_array.end();
		 cat_iter++)
	{
		LLViewerInventoryCategory *linked_cat = (*cat_iter);
		addChangedMask(LLInventoryObserver::LABEL, linked_cat->getUUID());
	};

	for (LLInventoryModel::item_array_t::iterator iter = item_array.begin();
		 iter != item_array.end();
		 iter++)
	{
		LLViewerInventoryItem *linked_item = (*iter);
		addChangedMask(LLInventoryObserver::LABEL, linked_item->getUUID());
	};
	notifyObservers();
}

void LLInventoryModel::collectLinkedItems(const LLUUID& id,
										  item_array_t& items)
{
	LLInventoryModel::cat_array_t cat_array;
	LLLinkedItemIDMatches is_linked_item_match(id);
	collectDescendentsIf(gInventory.getRootFolderID(),
						 cat_array,
						 items,
						 LLInventoryModel::INCLUDE_TRASH,
						 is_linked_item_match);
}

// Generates a string containing the path to the item specified by
// item_id.
void LLInventoryModel::appendPath(const LLUUID& id, std::string& path)
{
	std::string temp;
	LLInventoryObject* obj = getObject(id);
	LLUUID parent_id;
	if(obj) parent_id = obj->getParentUUID();
	std::string forward_slash("/");
	while(obj)
	{
		obj = getCategory(parent_id);
		if(obj)
		{
			temp.assign(forward_slash + obj->getName() + temp);
			parent_id = obj->getParentUUID();
		}
	}
	path.append(temp);
}

bool LLInventoryModel::isInventoryUsable()
{
	bool result = false;
	if(gInventory.getRootFolderID().notNull() && mIsAgentInvUsable)
	{
		result = true;
	}
	return result;	
}

// Calling this method with an inventory item will either change an
// existing item with a matching item_id, or will add the item to the
// current inventory.
U32 LLInventoryModel::updateItem(const LLViewerInventoryItem* item)
{
	U32 mask = LLInventoryObserver::NONE;
	if(item->getUUID().isNull())
	{
		return mask;
	}

	if(!isInventoryUsable())
	{
		llwarns << "Inventory is broken." << llendl;
		return mask;
	}

	LLViewerInventoryItem* old_item = getItem(item->getUUID());
	if(old_item)
	{
		// We already have an old item, modify its values
		LLUUID old_parent_id = old_item->getParentUUID();
		LLUUID new_parent_id = item->getParentUUID();
		if(old_parent_id != new_parent_id)
		{
			// need to update the parent-child tree
			item_array_t* item_array;
			item_array = get_ptr_in_map(mParentChildItemTree, old_parent_id);
			if(item_array)
			{
				item_array->removeObj(old_item);
			}
			item_array = get_ptr_in_map(mParentChildItemTree, new_parent_id);
			if(item_array)
			{
				item_array->put(old_item);
			}
			mask |= LLInventoryObserver::STRUCTURE;
		}
		if(old_item->getName() != item->getName())
		{
			mask |= LLInventoryObserver::LABEL;
		}
		old_item->copyViewerItem(item);
		mask |= LLInventoryObserver::INTERNAL;
	}
	else
	{
		// Simply add this item
		LLPointer<LLViewerInventoryItem> new_item = new LLViewerInventoryItem(item);
		addItem(new_item);

		if(item->getParentUUID().isNull())
		{
			LLUUID category_id = findCategoryUUIDForType(new_item->getType());
			new_item->setParent(category_id);
			item_array_t* item_array = get_ptr_in_map(mParentChildItemTree, category_id);
			if( item_array )
			{
				// *FIX: bit of a hack to call update server from here...
				new_item->updateServer(TRUE);
				item_array->put(new_item);
			}
			else
			{
				llwarns << "Couldn't find parent-child item tree for " << new_item->getName() << llendl;
			}
		}
		else
		{
			// *NOTE: The general scheme is that if every byte of the
			// uuid is 0, except for the last one or two,the use the
			// last two bytes of the parent id, and match that up
			// against the type. For now, we're only worried about
			// lost & found.
			LLUUID parent_id = item->getParentUUID();
			if(parent_id == CATEGORIZE_LOST_AND_FOUND_ID)
			{
				parent_id = findCategoryUUIDForType(LLAssetType::AT_LOST_AND_FOUND);
				new_item->setParent(parent_id);
			}
			item_array_t* item_array = get_ptr_in_map(mParentChildItemTree, parent_id);
			if(item_array)
			{
				item_array->put(new_item);
			}
			else
			{
				// Whoops! No such parent, make one.
				llinfos << "Lost item: " << new_item->getUUID() << " - "
						<< new_item->getName() << llendl;
				parent_id = findCategoryUUIDForType(LLAssetType::AT_LOST_AND_FOUND);
				new_item->setParent(parent_id);
				item_array = get_ptr_in_map(mParentChildItemTree, parent_id);
				if(item_array)
				{
					// *FIX: bit of a hack to call update server from
					// here...
					new_item->updateServer(TRUE);
					item_array->put(new_item);
				}
				else
				{
					llwarns << "Lost and found Not there!!" << llendl;
				}
			}
		}
		mask |= LLInventoryObserver::ADD;
	}
	if(item->getType() == LLAssetType::AT_CALLINGCARD)
	{
		mask |= LLInventoryObserver::CALLING_CARD;
	}
	addChangedMask(mask, item->getUUID());
	return mask;
}

LLInventoryModel::cat_array_t* LLInventoryModel::getUnlockedCatArray(const LLUUID& id)
{
	cat_array_t* cat_array = get_ptr_in_map(mParentChildCategoryTree, id);
	if (cat_array)
	{
		llassert_always(mCategoryLock[id] == false);
	}
	return cat_array;
}

LLInventoryModel::item_array_t* LLInventoryModel::getUnlockedItemArray(const LLUUID& id)
{
	item_array_t* item_array = get_ptr_in_map(mParentChildItemTree, id);
	if (item_array)
	{
		llassert_always(mItemLock[id] == false);
	}
	return item_array;
}

// Calling this method with an inventory category will either change
// an existing item with the matching id, or it will add the category.
void LLInventoryModel::updateCategory(const LLViewerInventoryCategory* cat)
{
	if(cat->getUUID().isNull())
	{
		return;
	}

	if(!isInventoryUsable())
	{
		llwarns << "Inventory is broken." << llendl;
		return;
	}

	LLViewerInventoryCategory* old_cat = getCategory(cat->getUUID());
	if(old_cat)
	{
		// We already have an old category, modify it's values
		U32 mask = LLInventoryObserver::NONE;
		LLUUID old_parent_id = old_cat->getParentUUID();
		LLUUID new_parent_id = cat->getParentUUID();
		if(old_parent_id != new_parent_id)
		{
			// need to update the parent-child tree
			cat_array_t* cat_array;
			cat_array = getUnlockedCatArray(old_parent_id);
			if(cat_array)
			{
				cat_array->removeObj(old_cat);
			}
			cat_array = getUnlockedCatArray(new_parent_id);
			if(cat_array)
			{
				cat_array->put(old_cat);
			}
			mask |= LLInventoryObserver::STRUCTURE;
		}
		if(old_cat->getName() != cat->getName())
		{
			mask |= LLInventoryObserver::LABEL;
		}
		old_cat->copyViewerCategory(cat);
		addChangedMask(mask, cat->getUUID());
	}
	else
	{
		// add this category
		LLPointer<LLViewerInventoryCategory> new_cat = new LLViewerInventoryCategory(cat->getParentUUID());
		new_cat->copyViewerCategory(cat);
		addCategory(new_cat);

		// make sure this category is correctly referenced by it's parent.
		cat_array_t* cat_array;
		cat_array = getUnlockedCatArray(cat->getParentUUID());
		if(cat_array)
		{
			cat_array->put(new_cat);
		}

		// make space in the tree for this category's children.
		llassert_always(mCategoryLock[new_cat->getUUID()] == false);
		llassert_always(mItemLock[new_cat->getUUID()] == false);
		cat_array_t* catsp = new cat_array_t;
		item_array_t* itemsp = new item_array_t;
		mParentChildCategoryTree[new_cat->getUUID()] = catsp;
		mParentChildItemTree[new_cat->getUUID()] = itemsp;
		addChangedMask(LLInventoryObserver::ADD, cat->getUUID());
	}
}

void LLInventoryModel::moveObject(const LLUUID& object_id, const LLUUID& cat_id)
{
	lldebugs << "LLInventoryModel::moveObject()" << llendl;
	if(!isInventoryUsable())
	{
		llwarns << "Inventory is broken." << llendl;
		return;
	}

	if((object_id == cat_id) || !is_in_map(mCategoryMap, cat_id))
	{
		llwarns << "Could not move inventory object " << object_id << " to "
				<< cat_id << llendl;
		return;
	}
	LLViewerInventoryCategory* cat = getCategory(object_id);
	if(cat && (cat->getParentUUID() != cat_id))
	{
		cat_array_t* cat_array;
		cat_array = getUnlockedCatArray(cat->getParentUUID());
		if(cat_array) cat_array->removeObj(cat);
		cat_array = getUnlockedCatArray(cat_id);
		cat->setParent(cat_id);
		if(cat_array) cat_array->put(cat);
		addChangedMask(LLInventoryObserver::STRUCTURE, object_id);
		return;
	}
	LLViewerInventoryItem* item = getItem(object_id);
	if(item && (item->getParentUUID() != cat_id))
	{
		item_array_t* item_array;
		item_array = getUnlockedItemArray(item->getParentUUID());
		if(item_array) item_array->removeObj(item);
		item_array = getUnlockedItemArray(cat_id);
		item->setParent(cat_id);
		if(item_array) item_array->put(item);
		addChangedMask(LLInventoryObserver::STRUCTURE, object_id);
		return;
	}
}

// Delete a particular inventory object by ID.
void LLInventoryModel::deleteObject(const LLUUID& id)
{
	purgeLinkedObjects(id);
	lldebugs << "LLInventoryModel::deleteObject()" << llendl;
	LLPointer<LLInventoryObject> obj = getObject(id);
	if(obj)
	{
		lldebugs << "Deleting inventory object " << id << llendl;
		mLastItem = NULL;
		LLUUID parent_id = obj->getParentUUID();
		mCategoryMap.erase(id);
		mItemMap.erase(id);
		//mInventory.erase(id);
		item_array_t* item_list = getUnlockedItemArray(parent_id);
		if(item_list)
		{
			LLViewerInventoryItem* item = (LLViewerInventoryItem*)((LLInventoryObject*)obj);
			item_list->removeObj(item);
		}
		cat_array_t* cat_list = getUnlockedCatArray(parent_id);
		if(cat_list)
		{
			LLViewerInventoryCategory* cat = (LLViewerInventoryCategory*)((LLInventoryObject*)obj);
			cat_list->removeObj(cat);
		}
		item_list = getUnlockedItemArray(id);
		if(item_list)
		{
			delete item_list;
			mParentChildItemTree.erase(id);
		}
		cat_list = getUnlockedCatArray(id);
		if(cat_list)
		{
			delete cat_list;
			mParentChildCategoryTree.erase(id);
		}
		addChangedMask(LLInventoryObserver::REMOVE, id);
		obj = NULL; // delete obj
	}
}

// Delete a particular inventory item by ID, and remove it from the server.
void LLInventoryModel::purgeObject(const LLUUID &id)
{
	lldebugs << "LLInventoryModel::purgeObject()" << llendl;
	LLPointer<LLInventoryObject> obj = getObject(id);
	if(obj)
	{
		obj->removeFromServer();
		LLPreview::hide(id);
		deleteObject(id);
	}
}

void LLInventoryModel::purgeLinkedObjects(const LLUUID &id)
{
	LLInventoryObject* objectp = getObject(id);
	if (!objectp) return;

	if (LLAssetType::lookupIsLinkType(objectp->getActualType()))
	{
		return;
	}

	LLInventoryModel::item_array_t item_array;
	collectLinkedItems(id, item_array);
	
	for (LLInventoryModel::item_array_t::iterator iter = item_array.begin();
		 iter != item_array.end();
		 iter++)
	{
		LLViewerInventoryItem *linked_item = (*iter);
		if (linked_item->getUUID() == id) continue;
		purgeObject(linked_item->getUUID());
	}
}

// This is a method which collects the descendents of the id
// provided. If the category is not found, no action is
// taken. This method goes through the long winded process of
// cancelling any calling cards, removing server representation of
// folders, items, etc in a fairly efficient manner.
void LLInventoryModel::purgeDescendentsOf(const LLUUID& id)
{
	EHasChildren children = categoryHasChildren(id);
	if(children == CHILDREN_NO)
	{
		llinfos << "Not purging descendents of " << id << llendl;
		return;
	}
	LLPointer<LLViewerInventoryCategory> cat = getCategory(id);
	if(cat.notNull())
	{
		// do the cache accounting
		llinfos << "LLInventoryModel::purgeDescendentsOf " << cat->getName()
				<< llendl;
		S32 descendents = cat->getDescendentCount();
		if(descendents > 0)
		{
			LLCategoryUpdate up(id, -descendents);
			accountForUpdate(up);
		}

		// we know that descendent count is 0, aide since the
		// accounting may actually not do an update, we should force
		// it here.
		cat->setDescendentCount(0);

		// send it upstream
		LLMessageSystem* msg = gMessageSystem;
		msg->newMessage("PurgeInventoryDescendents");
		msg->nextBlock("AgentData");
		msg->addUUID("AgentID", gAgent.getID());
		msg->addUUID("SessionID", gAgent.getSessionID());
		msg->nextBlock("InventoryData");
		msg->addUUID("FolderID", id);
		gAgent.sendReliableMessage();

		// unceremoniously remove anything we have locally stored.
		cat_array_t categories;
		item_array_t items;
		collectDescendents(id,
						   categories,
						   items,
						   INCLUDE_TRASH);
		S32 count = items.count();
		S32 i;
		for(i = 0; i < count; ++i)
		{
			deleteObject(items.get(i)->getUUID());
		}
		count = categories.count();
		for(i = 0; i < count; ++i)
		{
			deleteObject(categories.get(i)->getUUID());
		}
	}
}

void LLInventoryModel::deleteFromServer(LLDynamicArray<LLUUID>& category_ids,
										LLDynamicArray<LLUUID>& item_ids)
{
	// Store off tre UUIDS of parents which are being deleted (thus no
	// need to increment) and the parents which are being modified. We
	// have to increment the version of the parent with each message
	// sent upstream since the dataserver will increment each unique
	// parent per update message.
	std::set<LLUUID> ignore_parents;
	update_map_t inc_parents;

	S32 i;
	S32 count = category_ids.count();
	BOOL start_new_message = TRUE;
	LLMessageSystem* msg = gMessageSystem;
	LLPointer<LLViewerInventoryCategory> cat;
	for(i = 0; i < count; i++)
	{
		if(start_new_message)
		{
			start_new_message = FALSE;
			msg->newMessageFast(_PREHASH_RemoveInventoryObjects);
			msg->nextBlockFast(_PREHASH_AgentData);
			msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
			msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		}
		LLUUID cat_id = category_ids.get(i);

		msg->nextBlockFast(_PREHASH_FolderData);
		msg->addUUIDFast(_PREHASH_FolderID, cat_id);
		cat = getCategory(cat_id);
		ignore_parents.insert(cat_id);
		addChangedMask(LLInventoryObserver::REMOVE | LLInventoryObserver::STRUCTURE, cat_id);
		if(cat.notNull() && (ignore_parents.find(cat->getParentUUID())==ignore_parents.end()))
		{
			--inc_parents[cat->getParentUUID()];
		}
		if(msg->isSendFullFast(_PREHASH_FolderData))
		{
			start_new_message = TRUE;
			msg->nextBlockFast(_PREHASH_ItemData);
			msg->addUUIDFast(_PREHASH_ItemID, LLUUID::null);
			gAgent.sendReliableMessage();
			accountForUpdate(inc_parents);
			inc_parents.clear();
		}
	}

	count = item_ids.count();
	std::set<LLUUID>::iterator not_ignored = ignore_parents.end();
	LLPointer<LLViewerInventoryItem> item;
	if((0 == count) && (!start_new_message))
	{
		msg->nextBlockFast(_PREHASH_ItemData);
		msg->addUUIDFast(_PREHASH_ItemID, LLUUID::null);
	}
	for(i = 0; i < count; i++)
	{
		if(start_new_message)
		{
			start_new_message = FALSE;
			msg->newMessageFast(_PREHASH_RemoveInventoryObjects);
			msg->nextBlockFast(_PREHASH_AgentData);
			msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
			msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
			msg->nextBlockFast(_PREHASH_FolderData);
			msg->addUUIDFast(_PREHASH_FolderID, LLUUID::null);
		}
		LLUUID item_id = item_ids.get(i);
		msg->nextBlockFast(_PREHASH_ItemData);
		msg->addUUIDFast(_PREHASH_ItemID, item_id);
		item = getItem(item_id);
		addChangedMask(LLInventoryObserver::REMOVE | LLInventoryObserver::STRUCTURE, item_id);
		if(item.notNull() && (ignore_parents.find(item->getParentUUID()) == not_ignored))
		{
			--inc_parents[item->getParentUUID()];
		}
		if(msg->isSendFullFast(_PREHASH_ItemData))
		{
			start_new_message = TRUE;
			gAgent.sendReliableMessage();
			accountForUpdate(inc_parents);
			inc_parents.clear();
		}
	}
	if(!start_new_message)
	{
		gAgent.sendReliableMessage();
		accountForUpdate(inc_parents);
	}
}

// Add/remove an observer. If the observer is destroyed, be sure to
// remove it.
void LLInventoryModel::addObserver(LLInventoryObserver* observer)
{
	mObservers.insert(observer);
}
	
void LLInventoryModel::removeObserver(LLInventoryObserver* observer)
{
	mObservers.erase(observer);
}

BOOL LLInventoryModel::containsObserver(LLInventoryObserver* observer)
{
	return mObservers.find(observer) != mObservers.end();
}

// Call this method when it's time to update everyone on a new state,
// by default, the inventory model will not update observers
// automatically.
// The optional argument 'service_name' is used by Agent Inventory Service [DEV-20328]
void LLInventoryModel::notifyObservers(const std::string service_name)
{
	for (observer_list_t::iterator iter = mObservers.begin();
		 iter != mObservers.end(); )
	{
		LLInventoryObserver* observer = *iter;
		
		if (service_name.empty())
		{
			observer->changed(mModifyMask);
		}
		else
		{
			observer->mMessageName = service_name;
			observer->changed(mModifyMask);
		}

		// safe way to incrament since changed may delete entries! (@!##%@!@&*!)
		iter = mObservers.upper_bound(observer); 
	}

	mModifyMask = LLInventoryObserver::NONE;
	mChangedItemIDs.clear();
}

// store flag for change
// and id of object change applies to
void LLInventoryModel::addChangedMask(U32 mask, const LLUUID& referent) 
{ 
	mModifyMask |= mask; 
	if (referent.notNull())
	{
		mChangedItemIDs.insert(referent);
	}
}

// This method to prepares a set of mock inventory which provides
// minimal functionality before the actual arrival of inventory.
/*
void LLInventoryModel::mock(const LLUUID& root_id)
{
	llinfos << "LLInventoryModel::mock() " << root_id << llendl;
	if(root_id.isNull())
	{
		llwarns << "Not a valid root id" << llendl;
		return;
	}
	LLPointer<LLViewerInventoryCategory> cat = new LLViewerInventoryCategory(
		root_id,
		LLUUID::null,
		LLAssetType::AT_CATEGORY,
		LLAssetType::lookupCategoryName(LLAssetType::AT_ROOT_CATEGORY),
		gAgent.getID());
	addCategory(cat);
	gInventory.buildParentChildMap();
}
*/

//If we get back a normal response, handle it here
void  LLInventoryModel::fetchInventoryResponder::result(const LLSD& content)
{	
	start_new_inventory_observer();

	/*LLUUID agent_id;
	agent_id = content["agent_id"].asUUID();
	if(agent_id != gAgent.getID())
	{
		llwarns << "Got a inventory update for the wrong agent: " << agent_id
				<< llendl;
		return;
	}*/
	item_array_t items;
	update_map_t update;
	S32 count = content["items"].size();
	bool all_one_folder = true;
	LLUUID folder_id;
	// Does this loop ever execute more than once?
	for(S32 i = 0; i < count; ++i)
	{
		LLPointer<LLViewerInventoryItem> titem = new LLViewerInventoryItem;
		titem->unpackMessage(content["items"][i]);
		
		lldebugs << "LLInventoryModel::messageUpdateCore() item id:"
				 << titem->getUUID() << llendl;
		items.push_back(titem);
		// examine update for changes.
		LLViewerInventoryItem* itemp = gInventory.getItem(titem->getUUID());
		if(itemp)
		{
			if(titem->getParentUUID() == itemp->getParentUUID())
			{
				update[titem->getParentUUID()];
			}
			else
			{
				++update[titem->getParentUUID()];
				--update[itemp->getParentUUID()];
			}
		}
		else
		{
			++update[titem->getParentUUID()];
		}
		if (folder_id.isNull())
		{
			folder_id = titem->getParentUUID();
		}
		else
		{
			all_one_folder = false;
		}
	}

	U32 changes = 0x0;
	//as above, this loop never seems to loop more than once per call
	for (item_array_t::iterator it = items.begin(); it != items.end(); ++it)
	{
		changes |= gInventory.updateItem(*it);
	}
	gInventory.notifyObservers("fetchinventory");
	gViewerWindow->getWindow()->decBusyCount();
}

//If we get back an error (not found, etc...), handle it here
void LLInventoryModel::fetchInventoryResponder::error(U32 status, const std::string& reason)
{
	llinfos << "fetchInventory::error "
		<< status << ": " << reason << llendl;
	gInventory.notifyObservers("fetchinventory");
}

void LLInventoryModel::fetchDescendentsOf(const LLUUID& folder_id)
{
	LLViewerInventoryCategory* cat = getCategory(folder_id);
	if(!cat)
	{
		llwarns << "Asked to fetch descendents of non-existent folder: "
				<< folder_id << llendl;
		return;
	}
	//S32 known_descendents = 0;
	///cat_array_t* categories = get_ptr_in_map(mParentChildCategoryTree, folder_id);
	//item_array_t* items = get_ptr_in_map(mParentChildItemTree, folder_id);
	//if(categories)
	//{
	//	known_descendents += categories->count();
	//}
	//if(items)
	//{
	//	known_descendents += items->count();
	//}
	if(!cat->fetchDescendents())
	{
		//llinfos << "Not fetching descendents" << llendl;
	}
}

//Initialize statics.
bool LLInventoryModel::isBulkFetchProcessingComplete()
{
	return ( (sFetchQueue.empty() 
			&& sBulkFetchCount<=0)  ?  TRUE : FALSE ) ;
}

class fetchDescendentsResponder: public LLHTTPClient::Responder
{
	public:
		fetchDescendentsResponder(const LLSD& request_sd) : mRequestSD(request_sd) {};
		//fetchDescendentsResponder() {};
		void result(const LLSD& content);
		void error(U32 status, const std::string& reason);
	public:
		typedef std::vector<LLViewerInventoryCategory*> folder_ref_t;
	protected:
		LLSD mRequestSD;
};

//If we get back a normal response, handle it here
void  fetchDescendentsResponder::result(const LLSD& content)
{
	if (content.has("folders"))	
	{

		for(LLSD::array_const_iterator folder_it = content["folders"].beginArray();
			folder_it != content["folders"].endArray();
			++folder_it)
		{	
			LLSD folder_sd = *folder_it;
			

			//LLUUID agent_id = folder_sd["agent_id"];

			//if(agent_id != gAgent.getID())	//This should never happen.
			//{
			//	llwarns << "Got a UpdateInventoryItem for the wrong agent."
			//			<< llendl;
			//	break;
			//}

			LLUUID parent_id = folder_sd["folder_id"];
			LLUUID owner_id = folder_sd["owner_id"];
			S32    version  = (S32)folder_sd["version"].asInteger();
			S32    descendents = (S32)folder_sd["descendents"].asInteger();
			LLPointer<LLViewerInventoryCategory> tcategory = new LLViewerInventoryCategory(owner_id);

            if (parent_id.isNull())
            {
			    LLPointer<LLViewerInventoryItem> titem = new LLViewerInventoryItem;
			    for(LLSD::array_const_iterator item_it = folder_sd["items"].beginArray();
				    item_it != folder_sd["items"].endArray();
				    ++item_it)
			    {	
                    LLUUID lost_uuid = gInventory.findCategoryUUIDForType(LLAssetType::AT_LOST_AND_FOUND);
                    if (lost_uuid.notNull())
                    {
				        LLSD item = *item_it;
				        titem->unpackMessage(item);
				
                        LLInventoryModel::update_list_t update;
                        LLInventoryModel::LLCategoryUpdate new_folder(lost_uuid, 1);
                        update.push_back(new_folder);
                        gInventory.accountForUpdate(update);

                        titem->setParent(lost_uuid);
                        titem->updateParentOnServer(FALSE);
                        gInventory.updateItem(titem);
                        gInventory.notifyObservers("fetchDescendents");
                        
                    }
                }
            }

	        LLViewerInventoryCategory* pcat = gInventory.getCategory(parent_id);
			if (!pcat)
			{
				continue;
			}

			for(LLSD::array_const_iterator category_it = folder_sd["categories"].beginArray();
				category_it != folder_sd["categories"].endArray();
				++category_it)
			{	
				LLSD category = *category_it;
				tcategory->fromLLSD(category); 
							
				if (LLInventoryModel::sFullFetchStarted)
				{
					sFetchQueue.push_back(tcategory->getUUID());
				}
				else if ( !gInventory.isCategoryComplete(tcategory->getUUID()) )
				{
					gInventory.updateCategory(tcategory);
				}

			}
			LLPointer<LLViewerInventoryItem> titem = new LLViewerInventoryItem;
			for(LLSD::array_const_iterator item_it = folder_sd["items"].beginArray();
				item_it != folder_sd["items"].endArray();
				++item_it)
			{	
				LLSD item = *item_it;
				titem->unpackMessage(item);
				
				gInventory.updateItem(titem);
			}

			// set version and descendentcount according to message.
			LLViewerInventoryCategory* cat = gInventory.getCategory(parent_id);
			if(cat)
			{
				cat->setVersion(version);
				cat->setDescendentCount(descendents);
			}

		}
	}
		
	if (content.has("bad_folders"))
	{
		for(LLSD::array_const_iterator folder_it = content["bad_folders"].beginArray();
			folder_it != content["bad_folders"].endArray();
			++folder_it)
		{	
			LLSD folder_sd = *folder_it;
			
			//These folders failed on the dataserver.  We probably don't want to retry them.
			llinfos << "Folder " << folder_sd["folder_id"].asString() 
					<< "Error: " << folder_sd["error"].asString() << llendl;
		}
	}

	LLInventoryModel::incrBulkFetch(-1);
	
	if (LLInventoryModel::isBulkFetchProcessingComplete())
	{
		llinfos << "Inventory fetch completed" << llendl;
		if (LLInventoryModel::sFullFetchStarted)
		{
			LLInventoryModel::sAllFoldersFetched = TRUE;
		}
		LLInventoryModel::stopBackgroundFetch();
	}
	
	gInventory.notifyObservers("fetchDescendents");
}

//If we get back an error (not found, etc...), handle it here
void fetchDescendentsResponder::error(U32 status, const std::string& reason)
{
	llinfos << "fetchDescendentsResponder::error "
		<< status << ": " << reason << llendl;
						
	LLInventoryModel::incrBulkFetch(-1);

	if (status==499)		//timed out.  Let's be awesome!
	{
		for(LLSD::array_const_iterator folder_it = mRequestSD["folders"].beginArray();
			folder_it != mRequestSD["folders"].endArray();
			++folder_it)
		{	
			LLSD folder_sd = *folder_it;
			LLUUID folder_id = folder_sd["folder_id"];
			sFetchQueue.push_front(folder_id);
		}
	}
	else
	{
		if (LLInventoryModel::isBulkFetchProcessingComplete())
		{
			if (LLInventoryModel::sFullFetchStarted)
			{
				LLInventoryModel::sAllFoldersFetched = TRUE;
			}
			LLInventoryModel::stopBackgroundFetch();
		}
	}
	gInventory.notifyObservers("fetchDescendents");
}

//static   Bundle up a bunch of requests to send all at once.
void LLInventoryModel::bulkFetch(std::string url)
{
	//Background fetch is called from gIdleCallbacks in a loop until background fetch is stopped.
	//If there are items in sFetchQueue, we want to check the time since the last bulkFetch was 
	//sent.  If it exceeds our retry time, go ahead and fire off another batch.  
	//Stopbackgroundfetch will be run from the Responder instead of here.  

	S16 max_concurrent_fetches=8;
	F32 new_min_time = 0.5f;			//HACK!  Clean this up when old code goes away entirely.
	if (sMinTimeBetweenFetches < new_min_time) sMinTimeBetweenFetches=new_min_time;  //HACK!  See above.
	
	if(gDisconnected 
	|| sBulkFetchCount > max_concurrent_fetches
	|| sFetchTimer.getElapsedTimeF32() < sMinTimeBetweenFetches)
	{
		return; // just bail if we are disconnected.
	}	

	U32 folder_count=0;
	U32 max_batch_size=5;

	U32 sort_order = gSavedSettings.getU32("InventorySortOrder") & 0x1;

	LLSD body;
	LLSD body_lib;
	while( !(sFetchQueue.empty() ) && (folder_count < max_batch_size) )
	{
        if (sFetchQueue.front().isNull()) //DEV-17797
        {
			LLSD folder_sd;
			folder_sd["folder_id"]		= LLUUID::null.asString();
			folder_sd["owner_id"]		= gAgent.getID();
			folder_sd["sort_order"]		= (LLSD::Integer)sort_order;
			folder_sd["fetch_folders"]	= (LLSD::Boolean)FALSE;
			folder_sd["fetch_items"]	= (LLSD::Boolean)TRUE;
			body["folders"].append(folder_sd);
            folder_count++;
        }
        else
        {
				

		    LLViewerInventoryCategory* cat = gInventory.getCategory(sFetchQueue.front());
		
		    if (cat)
		    {
			    if ( LLViewerInventoryCategory::VERSION_UNKNOWN == cat->getVersion())
			    {
				    LLSD folder_sd;
				    folder_sd["folder_id"]		= cat->getUUID();
				    folder_sd["owner_id"]		= cat->getOwnerID();
				    folder_sd["sort_order"]		= (LLSD::Integer)sort_order;
				    folder_sd["fetch_folders"]	= TRUE; //(LLSD::Boolean)sFullFetchStarted;
				    folder_sd["fetch_items"]	= (LLSD::Boolean)TRUE;
				    
				    if (ALEXANDRIA_LINDEN_ID == cat->getOwnerID())
					    body_lib["folders"].append(folder_sd);
				    else
					    body["folders"].append(folder_sd);
				    folder_count++;
			    }
			    if (sFullFetchStarted)
			    {	//Already have this folder but append child folders to list.
				    // add all children to queue
				    parent_cat_map_t::iterator cat_it = gInventory.mParentChildCategoryTree.find(cat->getUUID());
				    if (cat_it != gInventory.mParentChildCategoryTree.end())
				    {
					    cat_array_t* child_categories = cat_it->second;
    
					    for (S32 child_num = 0; child_num < child_categories->count(); child_num++)
					    {
						    sFetchQueue.push_back(child_categories->get(child_num)->getUUID());
					    }
				    }
    
			    }
		    }
        }
		sFetchQueue.pop_front();
	}
		
		if (folder_count > 0)
		{
			sBulkFetchCount++;
			if (body["folders"].size())
			{
				LLHTTPClient::post(url, body, new fetchDescendentsResponder(body),300.0);
			}
			if (body_lib["folders"].size())
			{
				std::string url_lib = gAgent.getRegion()->getCapability("FetchLibDescendents");
				LLHTTPClient::post(url_lib, body_lib, new fetchDescendentsResponder(body_lib),300.0);
			}
			sFetchTimer.reset();
		}
	else if (isBulkFetchProcessingComplete())
	{
		if (sFullFetchStarted)
		{
			sAllFoldersFetched = TRUE;
		}
		stopBackgroundFetch();
	}	
}

// static
bool LLInventoryModel::isEverythingFetched()
{
	return (sAllFoldersFetched ? true : false);
}

//static
BOOL LLInventoryModel::backgroundFetchActive()
{
	return sBackgroundFetchActive;
}

//static 
void LLInventoryModel::startBackgroundFetch(const LLUUID& cat_id)
{
	if (!sAllFoldersFetched)
	{
		sBackgroundFetchActive = TRUE;
		if (cat_id.isNull())
		{
			if (!sFullFetchStarted)
			{
				sFullFetchStarted = TRUE;
				sFetchQueue.push_back(gInventory.getLibraryRootFolderID());
				sFetchQueue.push_back(gInventory.getRootFolderID());
				gIdleCallbacks.addFunction(&LLInventoryModel::backgroundFetch, NULL);
			}
		}
		else
		{
			// specific folder requests go to front of queue
			if (sFetchQueue.empty() || sFetchQueue.front() != cat_id)
			{
				sFetchQueue.push_front(cat_id);
				gIdleCallbacks.addFunction(&LLInventoryModel::backgroundFetch, NULL);
			}
		}
	}
}

//static
void LLInventoryModel::findLostItems()
{
	sBackgroundFetchActive = TRUE;
    sFetchQueue.push_back(LLUUID::null);
    gIdleCallbacks.addFunction(&LLInventoryModel::backgroundFetch, NULL);
}

//static
void LLInventoryModel::stopBackgroundFetch()
{
	if (sBackgroundFetchActive)
	{
		sBackgroundFetchActive = FALSE;
		gIdleCallbacks.deleteFunction(&LLInventoryModel::backgroundFetch, NULL);
		sBulkFetchCount=0;
		sMinTimeBetweenFetches=0.0f;
//		sFullFetchStarted=FALSE;
	}
}

//static 
void LLInventoryModel::backgroundFetch(void*)
{
	if (sBackgroundFetchActive && gAgent.getRegion())
	{
		//If we'll be using the capability, we'll be sending batches and the background thing isn't as important.
		std::string url = gAgent.getRegion()->getCapability("WebFetchInventoryDescendents");   
		if (!url.empty()) 
		{
			bulkFetch(url);
			return;
		}
		
		//DEPRECATED OLD CODE FOLLOWS.
		// no more categories to fetch, stop fetch process
		if (sFetchQueue.empty())
		{
			llinfos << "Inventory fetch completed" << llendl;
			if (sFullFetchStarted)
			{
				sAllFoldersFetched = TRUE;
			}
			stopBackgroundFetch();
			return;
		}

		F32 fast_fetch_time = lerp(sMinTimeBetweenFetches, sMaxTimeBetweenFetches, 0.1f);
		F32 slow_fetch_time = lerp(sMinTimeBetweenFetches, sMaxTimeBetweenFetches, 0.5f);
		if (sTimelyFetchPending && sFetchTimer.getElapsedTimeF32() > slow_fetch_time)
		{
			// double timeouts on failure
			sMinTimeBetweenFetches = llmin(sMinTimeBetweenFetches * 2.f, 10.f);
			sMaxTimeBetweenFetches = llmin(sMaxTimeBetweenFetches * 2.f, 120.f);
			llinfos << "Inventory fetch times grown to (" << sMinTimeBetweenFetches << ", " << sMaxTimeBetweenFetches << ")" << llendl;
			// fetch is no longer considered "timely" although we will wait for full time-out
			sTimelyFetchPending = FALSE;
		}

		while(1)
		{
			if (sFetchQueue.empty())
			{
				break;
			}

			if(gDisconnected)
			{
				// just bail if we are disconnected.
				break;
			}

			LLViewerInventoryCategory* cat = gInventory.getCategory(sFetchQueue.front());

			// category has been deleted, remove from queue.
			if (!cat)
			{
				sFetchQueue.pop_front();
				continue;
			}
			
			if (sFetchTimer.getElapsedTimeF32() > sMinTimeBetweenFetches && 
				LLViewerInventoryCategory::VERSION_UNKNOWN == cat->getVersion())
			{
				// category exists but has no children yet, fetch the descendants
				// for now, just request every time and rely on retry timer to throttle
				if (cat->fetchDescendents())
				{
					sFetchTimer.reset();
					sTimelyFetchPending = TRUE;
				}
				else
				{
					//  The catagory also tracks if it has expired and here it says it hasn't
					//  yet.  Get out of here because nothing is going to happen until we
					//  update the timers.
					break;
				}
			}
			// do I have all my children?
			else if (gInventory.isCategoryComplete(sFetchQueue.front()))
			{
				// finished with this category, remove from queue
				sFetchQueue.pop_front();

				// add all children to queue
				parent_cat_map_t::iterator cat_it = gInventory.mParentChildCategoryTree.find(cat->getUUID());
				if (cat_it != gInventory.mParentChildCategoryTree.end())
				{
					cat_array_t* child_categories = cat_it->second;

					for (S32 child_num = 0; child_num < child_categories->count(); child_num++)
					{
						sFetchQueue.push_back(child_categories->get(child_num)->getUUID());
					}
				}

				// we received a response in less than the fast time
				if (sTimelyFetchPending && sFetchTimer.getElapsedTimeF32() < fast_fetch_time)
				{
					// shrink timeouts based on success
					sMinTimeBetweenFetches = llmax(sMinTimeBetweenFetches * 0.8f, 0.3f);
					sMaxTimeBetweenFetches = llmax(sMaxTimeBetweenFetches * 0.8f, 10.f);
					//llinfos << "Inventory fetch times shrunk to (" << sMinTimeBetweenFetches << ", " << sMaxTimeBetweenFetches << ")" << llendl;
				}

				sTimelyFetchPending = FALSE;
				continue;
			}
			else if (sFetchTimer.getElapsedTimeF32() > sMaxTimeBetweenFetches)
			{
				// received first packet, but our num descendants does not match db's num descendants
				// so try again later
				LLUUID fetch_id = sFetchQueue.front();
				sFetchQueue.pop_front();

				if (sNumFetchRetries++ < MAX_FETCH_RETRIES)
				{
					// push on back of queue
					sFetchQueue.push_back(fetch_id);
				}
				sTimelyFetchPending = FALSE;
				sFetchTimer.reset();
				break;
			}

			// not enough time has elapsed to do a new fetch
			break;
		}
	}
}

void LLInventoryModel::cache(
	const LLUUID& parent_folder_id,
	const LLUUID& agent_id)
{
	lldebugs << "Caching " << parent_folder_id << " for " << agent_id
			 << llendl;
	LLViewerInventoryCategory* root_cat = getCategory(parent_folder_id);
	if(!root_cat) return;
	cat_array_t categories;
	categories.put(root_cat);
	item_array_t items;

	LLCanCache can_cache(this);
	can_cache(root_cat, NULL);
	collectDescendentsIf(
		parent_folder_id,
		categories,
		items,
		INCLUDE_TRASH,
		can_cache);
	std::string agent_id_str;
	std::string inventory_filename;
	agent_id.toString(agent_id_str);
	std::string path(gDirUtilp->getExpandedFilename(LL_PATH_CACHE, agent_id_str));
	inventory_filename = llformat(CACHE_FORMAT_STRING, path.c_str());
	saveToFile(inventory_filename, categories, items);
	std::string gzip_filename(inventory_filename);
	gzip_filename.append(".gz");
	if(gzip_file(inventory_filename, gzip_filename))
	{
		lldebugs << "Successfully compressed " << inventory_filename << llendl;
		LLFile::remove(inventory_filename);
	}
	else
	{
		llwarns << "Unable to compress " << inventory_filename << llendl;
	}
}


void LLInventoryModel::addCategory(LLViewerInventoryCategory* category)
{
	//llinfos << "LLInventoryModel::addCategory()" << llendl;
	if(category)
	{
		// Insert category uniquely into the map
		mCategoryMap[category->getUUID()] = category; // LLPointer will deref and delete the old one
		//mInventory[category->getUUID()] = category;
	}
}

void LLInventoryModel::addItem(LLViewerInventoryItem* item)
{
	//llinfos << "LLInventoryModel::addItem()" << llendl;
	if(item)
	{
		mItemMap[item->getUUID()] = item;
		//mInventory[item->getUUID()] = item;
	}
}

// Empty the entire contents
void LLInventoryModel::empty()
{
//	llinfos << "LLInventoryModel::empty()" << llendl;
	std::for_each(
		mParentChildCategoryTree.begin(),
		mParentChildCategoryTree.end(),
		DeletePairedPointer());
	mParentChildCategoryTree.clear();
	std::for_each(
		mParentChildItemTree.begin(),
		mParentChildItemTree.end(),
		DeletePairedPointer());
	mParentChildItemTree.clear();
	mCategoryMap.clear(); // remove all references (should delete entries)
	mItemMap.clear(); // remove all references (should delete entries)
	mLastItem = NULL;
	//mInventory.clear();
}

void LLInventoryModel::accountForUpdate(const LLCategoryUpdate& update)
{
	LLViewerInventoryCategory* cat = getCategory(update.mCategoryID);
	if(cat)
	{
		bool accounted = false;
		S32 version = cat->getVersion();
		if(version != LLViewerInventoryCategory::VERSION_UNKNOWN)
		{
			S32 descendents_server = cat->getDescendentCount();
			LLInventoryModel::cat_array_t* cats;
			LLInventoryModel::item_array_t* items;
			getDirectDescendentsOf(update.mCategoryID, cats, items);
			S32 descendents_actual = 0;
			if(cats && items)
			{
				descendents_actual = cats->count() + items->count();
			}
			if(descendents_server == descendents_actual)
			{
				accounted = true;
				descendents_actual += update.mDescendentDelta;
				cat->setDescendentCount(descendents_actual);
				cat->setVersion(++version);
				llinfos << "accounted: '" << cat->getName() << "' "
						<< version << " with " << descendents_actual
						<< " descendents." << llendl;
			}
		}
		if(!accounted)
		{
			lldebugs << "No accounting for: '" << cat->getName() << "' "
					 << version << llendl;
		}
	}
	else
	{
		llwarns << "No category found for update " << update.mCategoryID
				<< llendl;
	}
}

void LLInventoryModel::accountForUpdate(
	const LLInventoryModel::update_list_t& update)
{
	update_list_t::const_iterator it = update.begin();
	update_list_t::const_iterator end = update.end();
	for(; it != end; ++it)
	{
		accountForUpdate(*it);
	}
}

void LLInventoryModel::accountForUpdate(
	const LLInventoryModel::update_map_t& update)
{
	LLCategoryUpdate up;
	update_map_t::const_iterator it = update.begin();
	update_map_t::const_iterator end = update.end();
	for(; it != end; ++it)
	{
		up.mCategoryID = (*it).first;
		up.mDescendentDelta = (*it).second.mValue;
		accountForUpdate(up);
	}
}


/*
void LLInventoryModel::incrementCategoryVersion(const LLUUID& category_id)
{
	LLViewerInventoryCategory* cat = getCategory(category_id);
	if(cat)
	{
		S32 version = cat->getVersion();
		if(LLViewerInventoryCategory::VERSION_UNKNOWN != version)
		{
			cat->setVersion(version + 1);
			llinfos << "IncrementVersion: " << cat->getName() << " "
					<< cat->getVersion() << llendl;
		}
		else
		{
			llinfos << "Attempt to increment version when unknown: "
					<< category_id << llendl;
		}
	}
	else
	{
		llinfos << "Attempt to increment category: " << category_id << llendl;
	}
}
void LLInventoryModel::incrementCategorySetVersion(
	const std::set<LLUUID>& categories)
{
	if(!categories.empty())
	{ 
		std::set<LLUUID>::const_iterator it = categories.begin();
		std::set<LLUUID>::const_iterator end = categories.end();
		for(; it != end; ++it)
		{
			incrementCategoryVersion(*it);
		}
	}
}
*/


LLInventoryModel::EHasChildren LLInventoryModel::categoryHasChildren(
	const LLUUID& cat_id) const
{
	LLViewerInventoryCategory* cat = getCategory(cat_id);
	if(!cat) return CHILDREN_NO;
	if(cat->getDescendentCount() > 0)
	{
		return CHILDREN_YES;
	}
	if(cat->getDescendentCount() == 0)
	{
		return CHILDREN_NO;
	}
	if((cat->getDescendentCount() == LLViewerInventoryCategory::DESCENDENT_COUNT_UNKNOWN)
	   || (cat->getVersion() == LLViewerInventoryCategory::VERSION_UNKNOWN))
	{
		return CHILDREN_MAYBE;
	}

	// Shouldn't have to run this, but who knows.
	parent_cat_map_t::const_iterator cat_it = mParentChildCategoryTree.find(cat->getUUID());
	if (cat_it != mParentChildCategoryTree.end() && cat_it->second->count() > 0)
	{
		return CHILDREN_YES;
	}
	parent_item_map_t::const_iterator item_it = mParentChildItemTree.find(cat->getUUID());
	if (item_it != mParentChildItemTree.end() && item_it->second->count() > 0)
	{
		return CHILDREN_YES;
	}

	return CHILDREN_NO;
}

bool LLInventoryModel::isCategoryComplete(const LLUUID& cat_id) const
{
	LLViewerInventoryCategory* cat = getCategory(cat_id);
	if(cat && (cat->getVersion()!=LLViewerInventoryCategory::VERSION_UNKNOWN))
	{
		S32 descendents_server = cat->getDescendentCount();
		LLInventoryModel::cat_array_t* cats;
		LLInventoryModel::item_array_t* items;
		getDirectDescendentsOf(cat_id, cats, items);
		S32 descendents_actual = 0;
		if(cats && items)
		{
			descendents_actual = cats->count() + items->count();
		}
		if(descendents_server == descendents_actual)
		{
			return true;
		}
	}
	return false;
}

bool LLInventoryModel::loadSkeleton(
	const LLSD& options,
	const LLUUID& owner_id)
{
	lldebugs << "importing inventory skeleton for " << owner_id << llendl;

	typedef std::set<LLPointer<LLViewerInventoryCategory>, InventoryIDPtrLess> cat_set_t;
	cat_set_t temp_cats;
	bool rv = true;

	for(LLSD::array_const_iterator it = options.beginArray(),
		end = options.endArray(); it != end; ++it)
	{
		LLSD name = (*it)["name"];
		LLSD folder_id = (*it)["folder_id"];
		LLSD parent_id = (*it)["parent_id"];
		LLSD version = (*it)["version"];
		if(name.isDefined()
			&& folder_id.isDefined()
			&& parent_id.isDefined()
			&& version.isDefined()
			&& folder_id.asUUID().notNull() // if an id is null, it locks the viewer.
			) 		
		{
			LLPointer<LLViewerInventoryCategory> cat = new LLViewerInventoryCategory(owner_id);
			cat->rename(name.asString());
			cat->setUUID(folder_id.asUUID());
			cat->setParent(parent_id.asUUID());

			LLAssetType::EType preferred_type = LLAssetType::AT_NONE;
			LLSD type_default = (*it)["type_default"];
			if(type_default.isDefined())
            {
				preferred_type = (LLAssetType::EType)type_default.asInteger();
            }
            cat->setPreferredType(preferred_type);
			cat->setVersion(version.asInteger());
            temp_cats.insert(cat);
		}
		else
		{
			llwarns << "Unable to import near " << name.asString() << llendl;
            rv = false;
		}
	}

	S32 cached_category_count = 0;
	S32 cached_item_count = 0;
	if(!temp_cats.empty())
	{
		update_map_t child_counts;
		cat_array_t categories;
		item_array_t items;
		std::string owner_id_str;
		owner_id.toString(owner_id_str);
		std::string path(gDirUtilp->getExpandedFilename(LL_PATH_CACHE, owner_id_str));
		std::string inventory_filename;
		inventory_filename = llformat(CACHE_FORMAT_STRING, path.c_str());
		const S32 NO_VERSION = LLViewerInventoryCategory::VERSION_UNKNOWN;
		std::string gzip_filename(inventory_filename);
		gzip_filename.append(".gz");
		LLFILE* fp = LLFile::fopen(gzip_filename, "rb");
		bool remove_inventory_file = false;
		if(fp)
		{
			fclose(fp);
			fp = NULL;
			if(gunzip_file(gzip_filename, inventory_filename))
			{
				// we only want to remove the inventory file if it was
				// gzipped before we loaded, and we successfully
				// gunziped it.
				remove_inventory_file = true;
			}
			else
			{
				llinfos << "Unable to gunzip " << gzip_filename << llendl;
			}
		}
		if(loadFromFile(inventory_filename, categories, items))
		{
			// We were able to find a cache of files. So, use what we
			// found to generate a set of categories we should add. We
			// will go through each category loaded and if the version
			// does not match, invalidate the version.
			S32 count = categories.count();
			cat_set_t::iterator not_cached = temp_cats.end();
			std::set<LLUUID> cached_ids;
			for(S32 i = 0; i < count; ++i)
			{
				LLViewerInventoryCategory* cat = categories[i];
				cat_set_t::iterator cit = temp_cats.find(cat);
				if (cit == temp_cats.end())
				{
					continue; // cache corruption?? not sure why this happens -SJB
				}
				LLViewerInventoryCategory* tcat = *cit;
				
				// we can safely ignore anything loaded from file, but
				// not sent down in the skeleton.
				if(cit == not_cached)
				{
					continue;
				}
				if(cat->getVersion() != tcat->getVersion())
				{
					// if the cached version does not match the server version,
					// throw away the version we have so we can fetch the
					// correct contents the next time the viewer opens the folder.
					tcat->setVersion(NO_VERSION);
				}
				else
				{
					cached_ids.insert(tcat->getUUID());
				}
			}

			// go ahead and add the cats returned during the download
			std::set<LLUUID>::iterator not_cached_id = cached_ids.end();
			cached_category_count = cached_ids.size();
			for(cat_set_t::iterator it = temp_cats.begin(); it != temp_cats.end(); ++it)
			{
				if(cached_ids.find((*it)->getUUID()) == not_cached_id)
				{
					// this check is performed so that we do not
					// mark new folders in the skeleton (and not in cache)
					// as being cached.
					LLViewerInventoryCategory *llvic = (*it);
					llvic->setVersion(NO_VERSION);
				}
				addCategory(*it);
				++child_counts[(*it)->getParentUUID()];
			}

			// Add all the items loaded which are parented to a
			// category with a correctly cached parent
			count = items.count();
			cat_map_t::iterator unparented = mCategoryMap.end();
			for(int i = 0; i < count; ++i)
			{
				cat_map_t::iterator cit = mCategoryMap.find(items[i]->getParentUUID());
				
				if(cit != unparented)
				{
					LLViewerInventoryCategory* cat = cit->second;
					if(cat->getVersion() != NO_VERSION)
					{
						addItem(items[i]);
						cached_item_count += 1;
						++child_counts[cat->getUUID()];
					}
				}
			}
		}
		else
		{
			// go ahead and add everything after stripping the version
			// information.
			for(cat_set_t::iterator it = temp_cats.begin(); it != temp_cats.end(); ++it)
			{
				LLViewerInventoryCategory *llvic = (*it);
				llvic->setVersion(NO_VERSION);
				addCategory(*it);
			}
		}

		// At this point, we need to set the known descendents for each
		// category which successfully cached so that we do not
		// needlessly fetch descendents for categories which we have.
		update_map_t::iterator no_child_counts = child_counts.end();
		update_map_t::iterator the_count;
		for(cat_set_t::iterator it = temp_cats.begin(); it != temp_cats.end(); ++it)
		{
			LLViewerInventoryCategory* cat = (*it);
			if(cat->getVersion() != NO_VERSION)
			{
				the_count = child_counts.find(cat->getUUID());
				if(the_count != no_child_counts)
				{
					cat->setDescendentCount((*the_count).second.mValue);
				}
				else
				{
					cat->setDescendentCount(0);
				}
			}
		}

		if(remove_inventory_file)
		{
			// clean up the gunzipped file.
			LLFile::remove(inventory_filename);
		}
		categories.clear(); // will unref and delete entries
	}

	llinfos << "Successfully loaded " << cached_category_count
			<< " categories and " << cached_item_count << " items from cache."
			<< llendl;

	return rv;
}

bool LLInventoryModel::loadMeat(const LLSD& options, const LLUUID& owner_id)
{
	llinfos << "importing inventory for " << owner_id << llendl;
	bool rv = true;
	for(LLSD::array_const_iterator it = options.beginArray(),
		end = options.endArray(); it != end; ++it)
	{
		LLSD name = (*it)["name"];
		LLSD item_id = (*it)["item_id"];
		LLSD parent_id = (*it)["parent_id"];
		LLSD asset_type = (*it)["type"];
		LLSD data_id = (*it)["data_id"];
		if(name.isDefined() 
			&& item_id.isDefined()
			&& parent_id.isDefined()
			&& asset_type.isDefined()
			&& data_id.isDefined())
		{
			LLPointer<LLViewerInventoryItem> item = new LLViewerInventoryItem;
			item->rename(name.asString());
			item->setUUID(item_id.asUUID());
			item->setParent(parent_id.asUUID());
			LLAssetType::EType type = (LLAssetType::EType)asset_type.asInteger();
            item->setType(type);

			LLSD llsd_inv_type = (*it)["inv_type"];
			if(llsd_inv_type.isDefined())
            {
				LLInventoryType::EType inv_type = (LLInventoryType::EType)llsd_inv_type.asInteger();
                item->setInventoryType(inv_type);
            }

            if(LLAssetType::AT_CALLINGCARD == type)
            {
                LLPermissions perm;
				perm.init(data_id.asUUID(), owner_id, LLUUID::null, LLUUID::null);
                item->setPermissions(perm);
            }
            else
            {
				LLPermissions default_perm;
				default_perm.init(LLUUID::null, owner_id, LLUUID::null, LLUUID::null);
				LLSD llsd_perm_mask = (*it)["perm_mask"];
				if(llsd_perm_mask.isDefined())
                {
					PermissionMask perm_mask = llsd_perm_mask.asInteger();
					default_perm.initMasks(
						perm_mask, perm_mask, perm_mask, perm_mask, perm_mask);
				}
				else
				{
					default_perm.initMasks(
						PERM_NONE, PERM_NONE, PERM_NONE, PERM_NONE, PERM_NONE);
				}
				item->setPermissions(default_perm);
				item->setAssetUUID(data_id.asUUID());
            }

			LLSD flags = (*it)["flags"];
			if(flags.isDefined())
            {
				// Not sure how well LLSD.asInteger() maps to 
				// unsigned long - using strtoul()
				item->setFlags(strtoul(flags.asString().c_str(), NULL, 0));
            }

			LLSD time = (*it)["time"];
			if(time.isDefined())
            {
				item->setCreationDate(time.asInteger());
            }
            addItem(item);
		}
		else
		{
			llwarns << "Unable to import near " << name.asString() << llendl;
            rv = false;
		}
	}
	return rv;
}

// This is a brute force method to rebuild the entire parent-child
// relations. The overall operation has O(NlogN) performance, which
// should be sufficient for our needs. 
void LLInventoryModel::buildParentChildMap()
{
	llinfos << "LLInventoryModel::buildParentChildMap()" << llendl;

	// *NOTE: I am skipping the logic around folder version
	// synchronization here because it seems if a folder is lost, we
	// might actually want to invalidate it at that point - not
	// attempt to cache. More time & thought is necessary.

	// First the categories. We'll copy all of the categories into a
	// temporary container to iterate over (oh for real iterators.)
	// While we're at it, we'll allocate the arrays in the trees.
	cat_array_t cats;
	cat_array_t* catsp;
	item_array_t* itemsp;
	
	for(cat_map_t::iterator cit = mCategoryMap.begin(); cit != mCategoryMap.end(); ++cit)
	{
		LLViewerInventoryCategory* cat = cit->second;
		cats.put(cat);
		if (mParentChildCategoryTree.count(cat->getUUID()) == 0)
		{
			llassert_always(mCategoryLock[cat->getUUID()] == false);
			catsp = new cat_array_t;
			mParentChildCategoryTree[cat->getUUID()] = catsp;
		}
		if (mParentChildItemTree.count(cat->getUUID()) == 0)
		{
			llassert_always(mItemLock[cat->getUUID()] == false);
			itemsp = new item_array_t;
			mParentChildItemTree[cat->getUUID()] = itemsp;
		}
	}

	// Insert a special parent for the root - so that lookups on
	// LLUUID::null as the parent work correctly. This is kind of a
	// blatent wastes of space since we allocate a block of memory for
	// the array, but whatever - it's not that much space.
	if (mParentChildCategoryTree.count(LLUUID::null) == 0)
	{
		catsp = new cat_array_t;
		mParentChildCategoryTree[LLUUID::null] = catsp;
	}

	// Now we have a structure with all of the categories that we can
	// iterate over and insert into the correct place in the child
	// category tree. 
	S32 count = cats.count();
	S32 i;
	S32 lost = 0;
	for(i = 0; i < count; ++i)
	{
		LLViewerInventoryCategory* cat = cats.get(i);
		catsp = getUnlockedCatArray(cat->getParentUUID());
		if(catsp)
		{
			catsp->put(cat);
		}
		else
		{
			// *NOTE: This process could be a lot more efficient if we
			// used the new MoveInventoryFolder message, but we would
			// have to continue to do the update & build here. So, to
			// implement it, we would need a set or map of uuid pairs
			// which would be (folder_id, new_parent_id) to be sent up
			// to the server.
			llinfos << "Lost categroy: " << cat->getUUID() << " - "
					<< cat->getName() << llendl;
			++lost;
			// plop it into the lost & found.
			LLAssetType::EType pref = cat->getPreferredType();
			if(LLAssetType::AT_NONE == pref)
			{
				cat->setParent(findCategoryUUIDForType(LLAssetType::AT_LOST_AND_FOUND));
			}
			else if(LLAssetType::AT_CATEGORY == pref)
			{
				// it's the root
				cat->setParent(LLUUID::null);
			}
			else
			{
				// it's a protected folder.
				cat->setParent(gInventory.getRootFolderID());
			}
			cat->updateServer(TRUE);
			catsp = getUnlockedCatArray(cat->getParentUUID());
			if(catsp)
			{
				catsp->put(cat);
			}
			else
			{		
				llwarns << "Lost and found Not there!!" << llendl;
			}
		}
	}
	if(lost)
	{
		llwarns << "Found  " << lost << " lost categories." << llendl;
	}

	// Now the items. We allocated in the last step, so now all we
	// have to do is iterate over the items and put them in the right
	// place.
	item_array_t items;
	if(!mItemMap.empty())
	{
		LLPointer<LLViewerInventoryItem> item;
		for(item_map_t::iterator iit = mItemMap.begin(); iit != mItemMap.end(); ++iit)
		{
			item = (*iit).second;
			items.put(item);
		}
	}
	count = items.count();
	lost = 0;
	std::vector<LLUUID> lost_item_ids;
	for(i = 0; i < count; ++i)
	{
		LLPointer<LLViewerInventoryItem> item;
		item = items.get(i);
		itemsp = getUnlockedItemArray(item->getParentUUID());
		if(itemsp)
		{
			itemsp->put(item);
		}
		else
		{
			llinfos << "Lost item: " << item->getUUID() << " - "
					<< item->getName() << llendl;
			++lost;
			// plop it into the lost & found.
			//
			item->setParent(findCategoryUUIDForType(LLAssetType::AT_LOST_AND_FOUND));
			// move it later using a special message to move items. If
			// we update server here, the client might crash.
			//item->updateServer();
			lost_item_ids.push_back(item->getUUID());
			itemsp = getUnlockedItemArray(item->getParentUUID());
			if(itemsp)
			{
				itemsp->put(item);
			}
			else
			{
				llwarns << "Lost and found Not there!!" << llendl;
			}
		}
	}
	if(lost)
	{
		llwarns << "Found " << lost << " lost items." << llendl;
		LLMessageSystem* msg = gMessageSystem;
		BOOL start_new_message = TRUE;
		LLUUID lnf = findCategoryUUIDForType(LLAssetType::AT_LOST_AND_FOUND);
		for(std::vector<LLUUID>::iterator it = lost_item_ids.begin() ; it < lost_item_ids.end(); ++it)
		{
			if(start_new_message)
			{
				start_new_message = FALSE;
				msg->newMessageFast(_PREHASH_MoveInventoryItem);
				msg->nextBlockFast(_PREHASH_AgentData);
				msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
				msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
				msg->addBOOLFast(_PREHASH_Stamp, FALSE);
			}
			msg->nextBlockFast(_PREHASH_InventoryData);
			msg->addUUIDFast(_PREHASH_ItemID, (*it));
			msg->addUUIDFast(_PREHASH_FolderID, lnf);
			msg->addString("NewName", NULL);
			if(msg->isSendFull(NULL))
			{
				start_new_message = TRUE;
				gAgent.sendReliableMessage();
			}
		}
		if(!start_new_message)
		{
			gAgent.sendReliableMessage();
		}
	}

	LLUUID agent_inv_root_id = gInventory.getRootFolderID();
	if (agent_inv_root_id.notNull())
	{
		cat_array_t* catsp = get_ptr_in_map(mParentChildCategoryTree, agent_inv_root_id);
		if(catsp)
		{
			// 'My Inventory',
			// root of the agent's inv found.
			// The inv tree is built.
			mIsAgentInvUsable = true;
		}
	}
}

struct LLUUIDAndName
{
	LLUUIDAndName() {}
	LLUUIDAndName(const LLUUID& id, const std::string& name);
	bool operator==(const LLUUIDAndName& rhs) const;
	bool operator<(const LLUUIDAndName& rhs) const;
	bool operator>(const LLUUIDAndName& rhs) const;

	LLUUID mID;
	std::string mName;
};

LLUUIDAndName::LLUUIDAndName(const LLUUID& id, const std::string& name) :
	mID(id), mName(name)
{
}

bool LLUUIDAndName::operator==(const LLUUIDAndName& rhs) const
{
	return ((mID == rhs.mID) && (mName == rhs.mName));
}

bool LLUUIDAndName::operator<(const LLUUIDAndName& rhs) const
{
	return (mID < rhs.mID);
}

bool LLUUIDAndName::operator>(const LLUUIDAndName& rhs) const
{
	return (mID > rhs.mID);
}

// Given the current state of the inventory items, figure out the
// clone information. *FIX: This is sub-optimal, since we can insert
// this information snurgically, but this makes sure the implementation
// works before we worry about optimization.
//void LLInventoryModel::recalculateCloneInformation()
//{
//	//dumpInventory();
//
//	// This implements a 'multi-map' like structure to keep track of
//	// how many clones we find.
//	typedef LLDynamicArray<LLViewerInventoryItem*> viewer_item_array_t;
//	typedef std::map<LLUUIDAndName, viewer_item_array_t*> clone_map_t;
//	clone_map_t clone_map;
//	LLUUIDAndName id_and_name;
//	viewer_item_array_t* clones = NULL;
//	LLViewerInventoryItem* item = NULL;
//	for(item = (LLViewerInventoryItem*)mItemMap.getFirstData();
//		item != NULL;
//		item = (LLViewerInventoryItem*)mItemMap.getNextData())
//	{
//		if(item->getType() == LLAssetType::AT_CALLINGCARD)
//		{
//			// if it's a calling card, we key off of the creator id, not
//			// the asset id.
//			id_and_name.mID = item->getCreatorUUID();
//		}
//		else
//		{
//			// if it's not a calling card, we key clones from the
//			// asset id.
//			id_and_name.mID = item->getAssetUUID();
//		}
//		if(id_and_name.mID == LLUUID::null)
//		{
//			continue;
//		}
//		id_and_name.mName = item->getName();
//		if(clone_map.checkData(id_and_name))
//		{
//			clones = clone_map.getData(id_and_name);
//		}
//		else
//		{
//			clones = new viewer_item_array_t;
//			clone_map.addData(id_and_name, clones);
//		}
//		clones->put(item);
//	}
//
//	S32 count = 0;
//	for(clones = clone_map.getFirstData();
//		clones != NULL;
//		clones = clone_map.getNextData())
//	{
//		count = clones->count();
//		for(S32 i = 0; i < count; i++)
//		{
//			item = clones->get(i);
//			item->setCloneCount(count - 1);
//			//clones[i] = NULL;
//		}
//		delete clones;
//	}
//	clone_map.removeAllData();
//	//dumpInventory();
//}

// static
bool LLInventoryModel::loadFromFile(const std::string& filename,
									LLInventoryModel::cat_array_t& categories,
									LLInventoryModel::item_array_t& items)
{
	if(filename.empty())
	{
		llerrs << "Filename is Null!" << llendl;
		return false;
	}
	llinfos << "LLInventoryModel::loadFromFile(" << filename << ")" << llendl;
	LLFILE* file = LLFile::fopen(filename, "rb");		/*Flawfinder: ignore*/
	if(!file)
	{
		llinfos << "unable to load inventory from: " << filename << llendl;
		return false;
	}
	// *NOTE: This buffer size is hard coded into scanf() below.
	char buffer[MAX_STRING];		/*Flawfinder: ignore*/
	char keyword[MAX_STRING];		/*Flawfinder: ignore*/
	while(!feof(file) && fgets(buffer, MAX_STRING, file)) 
	{
		sscanf(buffer, " %254s", keyword);	/* Flawfinder: ignore */
		if(0 == strcmp("inv_category", keyword))
		{
			LLPointer<LLViewerInventoryCategory> inv_cat = new LLViewerInventoryCategory(LLUUID::null);
			if(inv_cat->importFileLocal(file))
			{
				categories.put(inv_cat);
			}
			else
			{
				llwarns << "loadInventoryFromFile().  Ignoring invalid inventory category: " << inv_cat->getName() << llendl;
				//delete inv_cat; // automatic when inv_cat is reassigned or destroyed
			}
		}
		else if(0 == strcmp("inv_item", keyword))
		{
			LLPointer<LLViewerInventoryItem> inv_item = new LLViewerInventoryItem;
			if( inv_item->importFileLocal(file) )
			{
				// *FIX: Need a better solution, this prevents the
				// application from freezing, but breaks inventory
				// caching.
				if(inv_item->getUUID().isNull())
				{
					//delete inv_item; // automatic when inv_cat is reassigned or destroyed
					llwarns << "Ignoring inventory with null item id: "
							<< inv_item->getName() << llendl;
						
				}
				else
				{
					items.put(inv_item);
				}
			}
			else
			{
				llwarns << "loadInventoryFromFile().  Ignoring invalid inventory item: " << inv_item->getName() << llendl;
				//delete inv_item; // automatic when inv_cat is reassigned or destroyed
			}
		}
		else
		{
			llwarns << "Unknown token in inventory file '" << keyword << "'"
					<< llendl;
		}
	}
	fclose(file);
	return true;
}

// static
bool LLInventoryModel::saveToFile(const std::string& filename,
								  const cat_array_t& categories,
								  const item_array_t& items)
{
	if(filename.empty())
	{
		llerrs << "Filename is Null!" << llendl;
		return false;
	}
	llinfos << "LLInventoryModel::saveToFile(" << filename << ")" << llendl;
	LLFILE* file = LLFile::fopen(filename, "wb");		/*Flawfinder: ignore*/
	if(!file)
	{
		llwarns << "unable to save inventory to: " << filename << llendl;
		return false;
	}

	S32 count = categories.count();
	S32 i;
	for(i = 0; i < count; ++i)
	{
		LLViewerInventoryCategory* cat = categories[i];
		if(cat->getVersion() != LLViewerInventoryCategory::VERSION_UNKNOWN)
		{
			cat->exportFileLocal(file);
		}
	}

	count = items.count();
	for(i = 0; i < count; ++i)
	{
		items[i]->exportFile(file);
	}

	fclose(file);
	return true;
}

// message handling functionality
// static
void LLInventoryModel::registerCallbacks(LLMessageSystem* msg)
{
	//msg->setHandlerFuncFast(_PREHASH_InventoryUpdate,
	//					processInventoryUpdate,
	//					NULL);
	//msg->setHandlerFuncFast(_PREHASH_UseCachedInventory,
	//					processUseCachedInventory,
	//					NULL);
	msg->setHandlerFuncFast(_PREHASH_UpdateCreateInventoryItem,
						processUpdateCreateInventoryItem,
						NULL);
	msg->setHandlerFuncFast(_PREHASH_RemoveInventoryItem,
						processRemoveInventoryItem,
						NULL);
	msg->setHandlerFuncFast(_PREHASH_UpdateInventoryFolder,
						processUpdateInventoryFolder,
						NULL);
	msg->setHandlerFuncFast(_PREHASH_RemoveInventoryFolder,
						processRemoveInventoryFolder,
						NULL);
	//msg->setHandlerFuncFast(_PREHASH_ExchangeCallingCard,
	//						processExchangeCallingcard,
	//						NULL);
	//msg->setHandlerFuncFast(_PREHASH_AddCallingCard,
	//					processAddCallingcard,
	//					NULL);
	//msg->setHandlerFuncFast(_PREHASH_DeclineCallingCard,
	//					processDeclineCallingcard,
	//					NULL);
	msg->setHandlerFuncFast(_PREHASH_SaveAssetIntoInventory,
						processSaveAssetIntoInventory,
						NULL);
	msg->setHandlerFuncFast(_PREHASH_BulkUpdateInventory,
							processBulkUpdateInventory,
							NULL);
	msg->setHandlerFunc("InventoryDescendents", processInventoryDescendents);
	msg->setHandlerFunc("MoveInventoryItem", processMoveInventoryItem);
	msg->setHandlerFunc("FetchInventoryReply", processFetchInventoryReply);
}


// 	static
void LLInventoryModel::processUpdateCreateInventoryItem(LLMessageSystem* msg, void**)
{
	// do accounting and highlight new items if they arrive
	if (gInventory.messageUpdateCore(msg, true))
	{
		U32 callback_id;
		LLUUID item_id;
		msg->getUUIDFast(_PREHASH_InventoryData, _PREHASH_ItemID, item_id);
		msg->getU32Fast(_PREHASH_InventoryData, _PREHASH_CallbackID, callback_id);

		gInventoryCallbacks.fire(callback_id, item_id);
	}

}

// static
void LLInventoryModel::processFetchInventoryReply(LLMessageSystem* msg, void**)
{
	// no accounting
	gInventory.messageUpdateCore(msg, false);
}


bool LLInventoryModel::messageUpdateCore(LLMessageSystem* msg, bool account)
{
	//make sure our added inventory observer is active
	start_new_inventory_observer();

	LLUUID agent_id;
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_AgentID, agent_id);
	if(agent_id != gAgent.getID())
	{
		llwarns << "Got a inventory update for the wrong agent: " << agent_id
				<< llendl;
		return false;
	}
	item_array_t items;
	update_map_t update;
	S32 count = msg->getNumberOfBlocksFast(_PREHASH_InventoryData);
	bool all_one_folder = true;
	LLUUID folder_id;
	// Does this loop ever execute more than once?
	for(S32 i = 0; i < count; ++i)
	{
		LLPointer<LLViewerInventoryItem> titem = new LLViewerInventoryItem;
		titem->unpackMessage(msg, _PREHASH_InventoryData, i);
		lldebugs << "LLInventoryModel::messageUpdateCore() item id:"
				 << titem->getUUID() << llendl;
		items.push_back(titem);
		// examine update for changes.
		LLViewerInventoryItem* itemp = gInventory.getItem(titem->getUUID());
		if(itemp)
		{
			if(titem->getParentUUID() == itemp->getParentUUID())
			{
				update[titem->getParentUUID()];
			}
			else
			{
				++update[titem->getParentUUID()];
				--update[itemp->getParentUUID()];
			}
		}
		else
		{
			++update[titem->getParentUUID()];
		}
		if (folder_id.isNull())
		{
			folder_id = titem->getParentUUID();
		}
		else
		{
			all_one_folder = false;
		}
	}
	if(account)
	{
		gInventory.accountForUpdate(update);
	}

	U32 changes = 0x0;
	//as above, this loop never seems to loop more than once per call
	for (item_array_t::iterator it = items.begin(); it != items.end(); ++it)
	{
		changes |= gInventory.updateItem(*it);
	}
	gInventory.notifyObservers();
	gViewerWindow->getWindow()->decBusyCount();

	return true;
}

// 	static
void LLInventoryModel::processRemoveInventoryItem(LLMessageSystem* msg, void**)
{
	lldebugs << "LLInventoryModel::processRemoveInventoryItem()" << llendl;
	LLUUID agent_id, item_id;
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_AgentID, agent_id);
	if(agent_id != gAgent.getID())
	{
		llwarns << "Got a RemoveInventoryItem for the wrong agent."
				<< llendl;
		return;
	}
	S32 count = msg->getNumberOfBlocksFast(_PREHASH_InventoryData);
	std::vector<LLUUID> item_ids;
	update_map_t update;
	for(S32 i = 0; i < count; ++i)
	{
		msg->getUUIDFast(_PREHASH_InventoryData, _PREHASH_ItemID, item_id, i);
		LLViewerInventoryItem* itemp = gInventory.getItem(item_id);
		if(itemp)
		{
			// we only bother with the delete and account if we found
			// the item - this is usually a back-up for permissions,
			// so frequently the item will already be gone.
			--update[itemp->getParentUUID()];
			item_ids.push_back(item_id);
		}
	}
	gInventory.accountForUpdate(update);
	for(std::vector<LLUUID>::iterator it = item_ids.begin(); it != item_ids.end(); ++it)
	{
		gInventory.deleteObject(*it);
	}
	gInventory.notifyObservers();
}

// 	static
void LLInventoryModel::processUpdateInventoryFolder(LLMessageSystem* msg,
													void**)
{
	lldebugs << "LLInventoryModel::processUpdateInventoryFolder()" << llendl;
	LLUUID agent_id, folder_id, parent_id;
	//char name[DB_INV_ITEM_NAME_BUF_SIZE];
	msg->getUUIDFast(_PREHASH_FolderData, _PREHASH_AgentID, agent_id);
	if(agent_id != gAgent.getID())
	{
		llwarns << "Got an UpdateInventoryFolder for the wrong agent."
				<< llendl;
		return;
	}
	LLPointer<LLViewerInventoryCategory> lastfolder; // hack
	cat_array_t folders;
	update_map_t update;
	S32 count = msg->getNumberOfBlocksFast(_PREHASH_FolderData);
	for(S32 i = 0; i < count; ++i)
	{
		LLPointer<LLViewerInventoryCategory> tfolder = new LLViewerInventoryCategory(gAgent.getID());
		lastfolder = tfolder;
		tfolder->unpackMessage(msg, _PREHASH_FolderData, i);
		// make sure it's not a protected folder
		tfolder->setPreferredType(LLAssetType::AT_NONE);
		folders.push_back(tfolder);
		// examine update for changes.
		LLViewerInventoryCategory* folderp = gInventory.getCategory(tfolder->getUUID());
		if(folderp)
		{
			if(tfolder->getParentUUID() == folderp->getParentUUID())
			{
				update[tfolder->getParentUUID()];
			}
			else
			{
				++update[tfolder->getParentUUID()];
				--update[folderp->getParentUUID()];
			}
		}
		else
		{
			++update[tfolder->getParentUUID()];
		}
	}
	gInventory.accountForUpdate(update);
	for (cat_array_t::iterator it = folders.begin(); it != folders.end(); ++it)
	{
		gInventory.updateCategory(*it);
	}
	gInventory.notifyObservers();

	// *HACK: Do the 'show' logic for a new item in the inventory.
	LLFloaterInventory* view = LLFloaterInventory::getActiveInventory();
	if(view)
	{
		view->getPanel()->setSelection(lastfolder->getUUID(), TAKE_FOCUS_NO);
	}
}

// 	static
void LLInventoryModel::processRemoveInventoryFolder(LLMessageSystem* msg,
													void**)
{
	lldebugs << "LLInventoryModel::processRemoveInventoryFolder()" << llendl;
	LLUUID agent_id, folder_id;
	msg->getUUIDFast(_PREHASH_FolderData, _PREHASH_AgentID, agent_id);
	if(agent_id != gAgent.getID())
	{
		llwarns << "Got a RemoveInventoryFolder for the wrong agent."
				<< llendl;
		return;
	}
	std::vector<LLUUID> folder_ids;
	update_map_t update;
	S32 count = msg->getNumberOfBlocksFast(_PREHASH_FolderData);
	for(S32 i = 0; i < count; ++i)
	{
		msg->getUUIDFast(_PREHASH_FolderData, _PREHASH_FolderID, folder_id, i);
		LLViewerInventoryCategory* folderp = gInventory.getCategory(folder_id);
		if(folderp)
		{
			--update[folderp->getParentUUID()];
			folder_ids.push_back(folder_id);
		}
	}
	gInventory.accountForUpdate(update);
	for(std::vector<LLUUID>::iterator it = folder_ids.begin(); it != folder_ids.end(); ++it)
	{
		gInventory.deleteObject(*it);
	}
	gInventory.notifyObservers();
}

// 	static
void LLInventoryModel::processSaveAssetIntoInventory(LLMessageSystem* msg,
													 void**)
{
	LLUUID agent_id;
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_AgentID, agent_id);
	if(agent_id != gAgent.getID())
	{
		llwarns << "Got a SaveAssetIntoInventory message for the wrong agent."
				<< llendl;
		return;
	}

	LLUUID item_id;
	msg->getUUIDFast(_PREHASH_InventoryData, _PREHASH_ItemID, item_id);

	// The viewer ignores the asset id because this message is only
	// used for attachments/objects, so the asset id is not used in
	// the viewer anyway.
	lldebugs << "LLInventoryModel::processSaveAssetIntoInventory itemID="
		<< item_id << llendl;
	LLViewerInventoryItem* item = gInventory.getItem( item_id );
	if( item )
	{
		LLCategoryUpdate up(item->getParentUUID(), 0);
		gInventory.accountForUpdate(up);
		gInventory.addChangedMask( LLInventoryObserver::INTERNAL, item_id);
		gInventory.notifyObservers();
	}
	else
	{
		llinfos << "LLInventoryModel::processSaveAssetIntoInventory item"
			" not found: " << item_id << llendl;
	}
	if(gViewerWindow)
	{
		gViewerWindow->getWindow()->decBusyCount();
	}
}

struct InventoryCallbackInfo
{
	InventoryCallbackInfo(U32 callback, const LLUUID& inv_id) :
		mCallback(callback), mInvID(inv_id) {}
	U32 mCallback;
	LLUUID mInvID;
};

// static
void LLInventoryModel::processBulkUpdateInventory(LLMessageSystem* msg, void**)
{
	LLUUID agent_id;
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_AgentID, agent_id);
	if(agent_id != gAgent.getID())
	{
		llwarns << "Got a BulkUpdateInventory for the wrong agent." << llendl;
		return;
	}
	LLUUID tid;
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_TransactionID, tid);
	llinfos << "Bulk inventory: " << tid << llendl;

	update_map_t update;
	cat_array_t folders;
	S32 count;
	S32 i;
	count = msg->getNumberOfBlocksFast(_PREHASH_FolderData);
	for(i = 0; i < count; ++i)
	{
		LLPointer<LLViewerInventoryCategory> tfolder = new LLViewerInventoryCategory(gAgent.getID());
		tfolder->unpackMessage(msg, _PREHASH_FolderData, i);
		//llinfos << "unpaked folder '" << tfolder->getName() << "' ("
		//		<< tfolder->getUUID() << ") in " << tfolder->getParentUUID()
		//		<< llendl;
		if(tfolder->getUUID().notNull())
		{
			folders.push_back(tfolder);
			LLViewerInventoryCategory* folderp = gInventory.getCategory(tfolder->getUUID());
			if(folderp)
			{
				if(tfolder->getParentUUID() == folderp->getParentUUID())
				{
					update[tfolder->getParentUUID()];
				}
				else
				{
					++update[tfolder->getParentUUID()];
					--update[folderp->getParentUUID()];
				}
			}
			else
			{
				// we could not find the folder, so it is probably
				// new. However, we only want to attempt accounting
				// for the parent if we can find the parent.
				folderp = gInventory.getCategory(tfolder->getParentUUID());
				if(folderp)
				{
					++update[tfolder->getParentUUID()];
				}
			}
		}
	}


	count = msg->getNumberOfBlocksFast(_PREHASH_ItemData);
	std::vector<LLUUID> wearable_ids;
	item_array_t items;
	std::list<InventoryCallbackInfo> cblist;
	for(i = 0; i < count; ++i)
	{
		LLPointer<LLViewerInventoryItem> titem = new LLViewerInventoryItem;
		titem->unpackMessage(msg, _PREHASH_ItemData, i);
		//llinfos << "unpaked item '" << titem->getName() << "' in "
		//		<< titem->getParentUUID() << llendl;
		U32 callback_id;
		msg->getU32Fast(_PREHASH_ItemData, _PREHASH_CallbackID, callback_id);
		if(titem->getUUID().notNull())
		{
			items.push_back(titem);
			cblist.push_back(InventoryCallbackInfo(callback_id, titem->getUUID()));
			if (titem->getInventoryType() == LLInventoryType::IT_WEARABLE)
			{
				wearable_ids.push_back(titem->getUUID());
			}
			// examine update for changes.
			LLViewerInventoryItem* itemp = gInventory.getItem(titem->getUUID());
			if(itemp)
			{
				if(titem->getParentUUID() == itemp->getParentUUID())
				{
					update[titem->getParentUUID()];
				}
				else
				{
					++update[titem->getParentUUID()];
					--update[itemp->getParentUUID()];
				}
			}
			else
			{
				LLViewerInventoryCategory* folderp = gInventory.getCategory(titem->getParentUUID());
				if(folderp)
				{
					++update[titem->getParentUUID()];
				}
			}
		}
		else
		{
			cblist.push_back(InventoryCallbackInfo(callback_id, LLUUID::null));
		}
	}
	gInventory.accountForUpdate(update);

	for (cat_array_t::iterator cit = folders.begin(); cit != folders.end(); ++cit)
	{
		gInventory.updateCategory(*cit);
	}
	for (item_array_t::iterator iit = items.begin(); iit != items.end(); ++iit)
	{
		gInventory.updateItem(*iit);
	}
	gInventory.notifyObservers();

	// The incoming inventory could span more than one BulkInventoryUpdate packet,
	// so record the transaction ID for this purchase, then wear all clothing
	// that comes in as part of that transaction ID.  JC
	if (LLFloaterInventory::sWearNewClothing)
	{
		LLFloaterInventory::sWearNewClothingTransactionID = tid;
		LLFloaterInventory::sWearNewClothing = FALSE;
	}

	if (tid == LLFloaterInventory::sWearNewClothingTransactionID)
	{
		count = wearable_ids.size();
		for (i = 0; i < count; ++i)
		{
			LLViewerInventoryItem* wearable_item;
			wearable_item = gInventory.getItem(wearable_ids[i]);
			wear_inventory_item_on_avatar(wearable_item);
		}
	}

	std::list<InventoryCallbackInfo>::iterator inv_it;
	for (inv_it = cblist.begin(); inv_it != cblist.end(); ++inv_it)
	{
		InventoryCallbackInfo cbinfo = (*inv_it);
		gInventoryCallbacks.fire(cbinfo.mCallback, cbinfo.mInvID);
	}
	// Don't show the inventory.  We used to call showAgentInventory here.
	//LLFloaterInventory* view = LLFloaterInventory::getActiveInventory();
	//if(view)
	//{
	//	const BOOL take_keyboard_focus = FALSE;
	//	view->setSelection(category.getUUID(), take_keyboard_focus );
	//	LLView* focus_view = gFocusMgr.getKeyboardFocus();
	//	LLFocusMgr::FocusLostCallback callback = gFocusMgr.getFocusCallback();
	//	// HACK to open inventory offers that are accepted.  This information
	//	// really needs to flow through the instant messages and inventory
	//	// transfer/update messages.
	//	if (LLFloaterInventory::sOpenNextNewItem)
	//	{
	//		view->openSelected();
	//		LLFloaterInventory::sOpenNextNewItem = FALSE;
	//	}
	//
	//	// restore keyboard focus
	//	gFocusMgr.setKeyboardFocus(focus_view);
	//}
}

// static
void LLInventoryModel::processInventoryDescendents(LLMessageSystem* msg,void**)
{
	LLUUID agent_id;
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_AgentID, agent_id);
	if(agent_id != gAgent.getID())
	{
		llwarns << "Got a UpdateInventoryItem for the wrong agent."
				<< llendl;
		return;
	}
	LLUUID parent_id;
	msg->getUUID("AgentData", "FolderID", parent_id);
	LLUUID owner_id;
	msg->getUUID("AgentData", "OwnerID", owner_id);
	S32 version;
	msg->getS32("AgentData", "Version", version);
	S32 descendents;
	msg->getS32("AgentData", "Descendents", descendents);
	S32 i;
	S32 count = msg->getNumberOfBlocksFast(_PREHASH_FolderData);
	LLPointer<LLViewerInventoryCategory> tcategory = new LLViewerInventoryCategory(owner_id);
	for(i = 0; i < count; ++i)
	{
		tcategory->unpackMessage(msg, _PREHASH_FolderData, i);
		gInventory.updateCategory(tcategory);
	}

	count = msg->getNumberOfBlocksFast(_PREHASH_ItemData);
	LLPointer<LLViewerInventoryItem> titem = new LLViewerInventoryItem;
	for(i = 0; i < count; ++i)
	{
		titem->unpackMessage(msg, _PREHASH_ItemData, i);
		gInventory.updateItem(titem);
	}

	// set version and descendentcount according to message.
	LLViewerInventoryCategory* cat = gInventory.getCategory(parent_id);
	if(cat)
	{
		cat->setVersion(version);
		cat->setDescendentCount(descendents);
	}
	gInventory.notifyObservers();
}

// static
void LLInventoryModel::processMoveInventoryItem(LLMessageSystem* msg, void**)
{
	lldebugs << "LLInventoryModel::processMoveInventoryItem()" << llendl;
	LLUUID agent_id;
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_AgentID, agent_id);
	if(agent_id != gAgent.getID())
	{
		llwarns << "Got a MoveInventoryItem message for the wrong agent."
				<< llendl;
		return;
	}

	LLUUID item_id;
	LLUUID folder_id;
	std::string new_name;
	bool anything_changed = false;
	S32 count = msg->getNumberOfBlocksFast(_PREHASH_InventoryData);
	for(S32 i = 0; i < count; ++i)
	{
		msg->getUUIDFast(_PREHASH_InventoryData, _PREHASH_ItemID, item_id, i);
		LLViewerInventoryItem* item = gInventory.getItem(item_id);
		if(item)
		{
			LLPointer<LLViewerInventoryItem> new_item = new LLViewerInventoryItem(item);
			msg->getUUIDFast(_PREHASH_InventoryData, _PREHASH_FolderID, folder_id, i);
			msg->getString("InventoryData", "NewName", new_name, i);

			lldebugs << "moving item " << item_id << " to folder "
					 << folder_id << llendl;
			update_list_t update;
			LLCategoryUpdate old_folder(item->getParentUUID(), -1);
			update.push_back(old_folder);
			LLCategoryUpdate new_folder(folder_id, 1);
			update.push_back(new_folder);
			gInventory.accountForUpdate(update);

			new_item->setParent(folder_id);
			if (new_name.length() > 0)
			{
				new_item->rename(new_name);
			}
			gInventory.updateItem(new_item);
			anything_changed = true;
		}
		else
		{
			llinfos << "LLInventoryModel::processMoveInventoryItem item not found: " << item_id << llendl;
		}
	}
	if(anything_changed)
	{
		gInventory.notifyObservers();
	}
}

//----------------------------------------------------------------------------

// Trash: LLAssetType::AT_TRASH, "ConfirmEmptyTrash"
// Lost&Found: LLAssetType::AT_LOST_AND_FOUND, "ConfirmEmptyLostAndFound"

bool LLInventoryModel::callbackEmptyFolderType(const LLSD& notification, const LLSD& response, LLAssetType::EType folder_type)
{
	S32 option = LLNotification::getSelectedOption(notification, response);
	if (option == 0) // YES
	{
		LLUUID folder_id = findCategoryUUIDForType(folder_type);
		purgeDescendentsOf(folder_id);
		notifyObservers();
	}
	return false;
}

void LLInventoryModel::emptyFolderType(const std::string notification, LLAssetType::EType folder_type)
{
	if (!notification.empty())
	{
		LLNotifications::instance().add(notification, LLSD(), LLSD(),
										boost::bind(&LLInventoryModel::callbackEmptyFolderType, this, _1, _2, folder_type));
	}
	else
	{
		LLUUID folder_id = findCategoryUUIDForType(folder_type);
		purgeDescendentsOf(folder_id);
		notifyObservers();
	}
}

//----------------------------------------------------------------------------

void LLInventoryModel::removeItem(const LLUUID& item_id)
{
	LLViewerInventoryItem* item = getItem(item_id);
	const LLUUID& new_parent = findCategoryUUIDForType(LLAssetType::AT_TRASH);
	if (item && item->getParentUUID() != new_parent)
	{
		LLInventoryModel::update_list_t update;
		LLInventoryModel::LLCategoryUpdate old_folder(item->getParentUUID(),-1);
		update.push_back(old_folder);
		LLInventoryModel::LLCategoryUpdate new_folder(new_parent, 1);
		update.push_back(new_folder);
		accountForUpdate(update);

		LLPointer<LLViewerInventoryItem> new_item = new LLViewerInventoryItem(item);
		new_item->setParent(new_parent);
		new_item->updateParentOnServer(TRUE);
		updateItem(new_item);
		notifyObservers();
	}
}

LLUUID LLInventoryModel::getRootFolderID() const
{
	return mRootFolderID;
}

void LLInventoryModel::setRootFolderID(const LLUUID& val)
{
	mRootFolderID = val;
}

LLUUID LLInventoryModel::getLibraryRootFolderID() const
{
	return mLibraryRootFolderID;
}

void LLInventoryModel::setLibraryRootFolderID(const LLUUID& val)
{
	mLibraryRootFolderID = val;
}

LLUUID LLInventoryModel::getLibraryOwnerID() const
{
	return mLibraryOwnerID;
}

void LLInventoryModel::setLibraryOwnerID(const LLUUID& val)
{
	mLibraryOwnerID = val;
}

//----------------------------------------------------------------------------

// *NOTE: DEBUG functionality
void LLInventoryModel::dumpInventory()
{
	llinfos << "\nBegin Inventory Dump\n**********************:" << llendl;
	llinfos << "mCategroy[] contains " << mCategoryMap.size() << " items." << llendl;
	for(cat_map_t::iterator cit = mCategoryMap.begin(); cit != mCategoryMap.end(); ++cit)
	{
		LLViewerInventoryCategory* cat = cit->second;
		if(cat)
		{
			llinfos << "  " <<  cat->getUUID() << " '" << cat->getName() << "' "
					<< cat->getVersion() << " " << cat->getDescendentCount()
					<< llendl;
		}
		else
		{
			llinfos << "  NULL!" << llendl;
		}
	}	
	llinfos << "mItemMap[] contains " << mItemMap.size() << " items." << llendl;
	for(item_map_t::iterator iit = mItemMap.begin(); iit != mItemMap.end(); ++iit)
	{
		LLViewerInventoryItem* item = iit->second;
		if(item)
		{
			llinfos << "  " << item->getUUID() << " "
					<< item->getName() << llendl;
		}
		else
		{
			llinfos << "  NULL!" << llendl;
		}
	}
	llinfos << "\n**********************\nEnd Inventory Dump" << llendl;
}

///----------------------------------------------------------------------------
/// LLInventoryCollectFunctor implementations
///----------------------------------------------------------------------------

// static
bool LLInventoryCollectFunctor::itemTransferCommonlyAllowed(LLInventoryItem* item)
{
	if (!item)
		return false;

	bool allowed = false;
	LLVOAvatarSelf* my_avatar = NULL;

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
		if(!gAgentWearables.isWearingItem(item->getUUID()))
		{
			allowed = true;
		}
		break;
		
	default:
		allowed = true;
		break;
	}

	return allowed;
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


//bool LLIsClone::operator()(LLInventoryCategory* cat, LLInventoryItem* item)
//{
//	if(cat) return FALSE;
//	if(item)
//	{
//		if(mItemMap->getType() == LLAssetType::AT_CALLINGCARD)
//		{
//			if((item->getType() == LLAssetType::AT_CALLINGCARD)
//			   && !(item->getCreatorUUID().isNull())
//			   && (item->getCreatorUUID() == mItemMap->getCreatorUUID()))
//			{
//				return TRUE;
//			}
//		}
//		else
//		{
//			if((item->getType() == mItemMap->getType())
//			   && !(item->getAssetUUID().isNull())
//			   && (item->getAssetUUID() == mItemMap->getAssetUUID())
//			   && (item->getName() == mItemMap->getName()))
//			{
//				return TRUE;
//			}
//		}
//	}
//	return FALSE;
//}

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



///----------------------------------------------------------------------------
/// Observers
///----------------------------------------------------------------------------

void LLInventoryCompletionObserver::changed(U32 mask)
{
	// scan through the incomplete items and move or erase them as
	// appropriate.
	if(!mIncomplete.empty())
	{
		for(item_ref_t::iterator it = mIncomplete.begin(); it < mIncomplete.end(); )
		{
			LLViewerInventoryItem* item = gInventory.getItem(*it);
			if(!item)
			{
				it = mIncomplete.erase(it);
				continue;
			}
			if(item->isComplete())
			{
				mComplete.push_back(*it);
				it = mIncomplete.erase(it);
				continue;
			}
			++it;
		}
		if(mIncomplete.empty())
		{
			done();
		}
	}
}

void LLInventoryCompletionObserver::watchItem(const LLUUID& id)
{
	if(id.notNull())
	{
		mIncomplete.push_back(id);
	}
}


void LLInventoryFetchObserver::changed(U32 mask)
{
	// scan through the incomplete items and move or erase them as
	// appropriate.
	if(!mIncomplete.empty())
	{
		for(item_ref_t::iterator it = mIncomplete.begin(); it < mIncomplete.end(); )
		{
			LLViewerInventoryItem* item = gInventory.getItem(*it);
			if(!item)
			{
				// BUG: This can cause done() to get called prematurely below.
				// This happens with the LLGestureInventoryFetchObserver that
				// loads gestures at startup. JC
				it = mIncomplete.erase(it);
				continue;
			}
			if(item->isComplete())
			{
				mComplete.push_back(*it);
				it = mIncomplete.erase(it);
				continue;
			}
			++it;
		}
		if(mIncomplete.empty())
		{
			done();
		}
	}
	//llinfos << "LLInventoryFetchObserver::changed() mComplete size " << mComplete.size() << llendl;
	//llinfos << "LLInventoryFetchObserver::changed() mIncomplete size " << mIncomplete.size() << llendl;
}

bool LLInventoryFetchObserver::isEverythingComplete() const
{
	return mIncomplete.empty();
}

void fetch_items_from_llsd(const LLSD& items_llsd)
{
	if (!items_llsd.size()) return;
	LLSD body;
	body[0]["cap_name"] = "FetchInventory";
	body[1]["cap_name"] = "FetchLib";
	for (S32 i=0; i<items_llsd.size();i++)
	{
		if (items_llsd[i]["owner_id"].asString() == gAgent.getID().asString())
		{
			body[0]["items"].append(items_llsd[i]);
			continue;
		}
		if (items_llsd[i]["owner_id"].asString() == ALEXANDRIA_LINDEN_ID.asString())
		{
			body[1]["items"].append(items_llsd[i]);
			continue;
		}
	}
		
	for (S32 i=0; i<body.size(); i++)
	{
		if (0 >= body[i].size()) continue;
		std::string url = gAgent.getRegion()->getCapability(body[i]["cap_name"].asString());

		if (!url.empty())
		{
			body[i]["agent_id"]	= gAgent.getID();
			LLHTTPClient::post(url, body[i], new LLInventoryModel::fetchInventoryResponder(body[i]));
			break;
		}

		LLMessageSystem* msg = gMessageSystem;
		BOOL start_new_message = TRUE;
		for (S32 j=0; j<body[i]["items"].size(); j++)
		{
			LLSD item_entry = body[i]["items"][j];
			if(start_new_message)
			{
				start_new_message = FALSE;
				msg->newMessageFast(_PREHASH_FetchInventory);
				msg->nextBlockFast(_PREHASH_AgentData);
				msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
				msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
			}
			msg->nextBlockFast(_PREHASH_InventoryData);
			msg->addUUIDFast(_PREHASH_OwnerID, item_entry["owner_id"].asUUID());
			msg->addUUIDFast(_PREHASH_ItemID, item_entry["item_id"].asUUID());
			if(msg->isSendFull(NULL))
			{
				start_new_message = TRUE;
				gAgent.sendReliableMessage();
			}
		}
		if(!start_new_message)
		{
			gAgent.sendReliableMessage();
		}
	}
}

void LLInventoryFetchObserver::fetchItems(
	const LLInventoryFetchObserver::item_ref_t& ids)
{
	LLUUID owner_id;
	LLSD items_llsd;
	for(item_ref_t::const_iterator it = ids.begin(); it < ids.end(); ++it)
	{
		LLViewerInventoryItem* item = gInventory.getItem(*it);
		if(item)
		{
			if(item->isComplete())
			{
				// It's complete, so put it on the complete container.
				mComplete.push_back(*it);
				continue;
			}
			else
			{
				owner_id = item->getPermissions().getOwner();
			}
		}
		else
		{
			// assume it's agent inventory.
			owner_id = gAgent.getID();
		}
		
		// It's incomplete, so put it on the incomplete container, and
		// pack this on the message.
		mIncomplete.push_back(*it);
		
		// Prepare the data to fetch
		LLSD item_entry;
		item_entry["owner_id"] = owner_id;
		item_entry["item_id"] = (*it);
		items_llsd.append(item_entry);
	}
	fetch_items_from_llsd(items_llsd);
}

// virtual
void LLInventoryFetchDescendentsObserver::changed(U32 mask)
{
	for(folder_ref_t::iterator it = mIncompleteFolders.begin(); it < mIncompleteFolders.end();)
	{
		LLViewerInventoryCategory* cat = gInventory.getCategory(*it);
		if(!cat)
		{
			it = mIncompleteFolders.erase(it);
			continue;
		}
		if(isComplete(cat))
		{
			mCompleteFolders.push_back(*it);
			it = mIncompleteFolders.erase(it);
			continue;
		}
		++it;
	}
	if(mIncompleteFolders.empty())
	{
		done();
	}
}

void LLInventoryFetchDescendentsObserver::fetchDescendents(
	const folder_ref_t& ids)
{
	for(folder_ref_t::const_iterator it = ids.begin(); it != ids.end(); ++it)
	{
		LLViewerInventoryCategory* cat = gInventory.getCategory(*it);
		if(!cat) continue;
		if(!isComplete(cat))
		{
			cat->fetchDescendents();		//blindly fetch it without seeing if anything else is fetching it.
			mIncompleteFolders.push_back(*it);	//Add to list of things being downloaded for this observer.
		}
		else
		{
			mCompleteFolders.push_back(*it);
		}
	}
}

bool LLInventoryFetchDescendentsObserver::isEverythingComplete() const
{
	return mIncompleteFolders.empty();
}

bool LLInventoryFetchDescendentsObserver::isComplete(LLViewerInventoryCategory* cat)
{
	S32 version = cat->getVersion();
	S32 descendents = cat->getDescendentCount();
	if((LLViewerInventoryCategory::VERSION_UNKNOWN == version)
	   || (LLViewerInventoryCategory::DESCENDENT_COUNT_UNKNOWN == descendents))
	{
		return false;
	}
	// it might be complete - check known descendents against
	// currently available.
	LLInventoryModel::cat_array_t* cats;
	LLInventoryModel::item_array_t* items;
	gInventory.getDirectDescendentsOf(cat->getUUID(), cats, items);
	if(!cats || !items)
	{
		// bit of a hack - pretend we're done if they are gone or
		// incomplete. should never know, but it would suck if this
		// kept tight looping because of a corrupt memory state.
		return true;
	}
	S32 known = cats->count() + items->count();
	if(descendents == known)
	{
		// hey - we're done.
		return true;
	}
	return false;
}

void LLInventoryFetchComboObserver::changed(U32 mask)
{
	if(!mIncompleteItems.empty())
	{
		for(item_ref_t::iterator it = mIncompleteItems.begin(); it < mIncompleteItems.end(); )
		{
			LLViewerInventoryItem* item = gInventory.getItem(*it);
			if(!item)
			{
				it = mIncompleteItems.erase(it);
				continue;
			}
			if(item->isComplete())
			{
				mCompleteItems.push_back(*it);
				it = mIncompleteItems.erase(it);
				continue;
			}
			++it;
		}
	}
	if(!mIncompleteFolders.empty())
	{
		for(folder_ref_t::iterator it = mIncompleteFolders.begin(); it < mIncompleteFolders.end();)
		{
			LLViewerInventoryCategory* cat = gInventory.getCategory(*it);
			if(!cat)
			{
				it = mIncompleteFolders.erase(it);
				continue;
			}
			if(gInventory.isCategoryComplete(*it))
			{
				mCompleteFolders.push_back(*it);
				it = mIncompleteFolders.erase(it);
				continue;
			}
			++it;
		}
	}
	if(!mDone && mIncompleteItems.empty() && mIncompleteFolders.empty())
	{
		mDone = true;
		done();
	}
}

void LLInventoryFetchComboObserver::fetch(
	const folder_ref_t& folder_ids,
	const item_ref_t& item_ids)
{
	lldebugs << "LLInventoryFetchComboObserver::fetch()" << llendl;
	for(folder_ref_t::const_iterator fit = folder_ids.begin(); fit != folder_ids.end(); ++fit)
	{
		LLViewerInventoryCategory* cat = gInventory.getCategory(*fit);
		if(!cat) continue;
		if(!gInventory.isCategoryComplete(*fit))
		{
			cat->fetchDescendents();
			lldebugs << "fetching folder " << *fit <<llendl;
			mIncompleteFolders.push_back(*fit);
		}
		else
		{
			mCompleteFolders.push_back(*fit);
			lldebugs << "completing folder " << *fit <<llendl;
		}
	}

	// Now for the items - we fetch everything which is not a direct
	// descendent of an incomplete folder because the item will show
	// up in an inventory descendents message soon enough so we do not
	// have to fetch it individually.
	LLSD items_llsd;
	LLUUID owner_id;
	for(item_ref_t::const_iterator iit = item_ids.begin(); iit != item_ids.end(); ++iit)
	{
		LLViewerInventoryItem* item = gInventory.getItem(*iit);
		if(!item)
		{
			lldebugs << "uanble to find item " << *iit << llendl;
			continue;
		}
		if(item->isComplete())
		{
			// It's complete, so put it on the complete container.
			mCompleteItems.push_back(*iit);
			lldebugs << "completing item " << *iit << llendl;
			continue;
		}
		else
		{
			mIncompleteItems.push_back(*iit);
			owner_id = item->getPermissions().getOwner();
		}
		if(std::find(mIncompleteFolders.begin(), mIncompleteFolders.end(), item->getParentUUID()) == mIncompleteFolders.end())
		{
			LLSD item_entry;
			item_entry["owner_id"] = owner_id;
			item_entry["item_id"] = (*iit);
			items_llsd.append(item_entry);
		}
		else
		{
			lldebugs << "not worrying about " << *iit << llendl;
		}
	}
	fetch_items_from_llsd(items_llsd);
}

void LLInventoryExistenceObserver::watchItem(const LLUUID& id)
{
	if(id.notNull())
	{
		mMIA.push_back(id);
	}
}

void LLInventoryExistenceObserver::changed(U32 mask)
{
	// scan through the incomplete items and move or erase them as
	// appropriate.
	if(!mMIA.empty())
	{
		for(item_ref_t::iterator it = mMIA.begin(); it < mMIA.end(); )
		{
			LLViewerInventoryItem* item = gInventory.getItem(*it);
			if(!item)
			{
				++it;
				continue;
			}
			mExist.push_back(*it);
			it = mMIA.erase(it);
		}
		if(mMIA.empty())
		{
			done();
		}
	}
}

void LLInventoryAddedObserver::changed(U32 mask)
{
	if(!(mask & LLInventoryObserver::ADD))
	{
		return;
	}

	// *HACK: If this was in response to a packet off
	// the network, figure out which item was updated.
	LLMessageSystem* msg = gMessageSystem;

	std::string msg_name;
	if (mMessageName.empty())
	{
		msg_name = msg->getMessageName();
	}
	else
	{
		msg_name = mMessageName;
	}

	if (msg_name.empty())
	{
		return;
	}
	
	// We only want newly created inventory items. JC
	if ( msg_name != "UpdateCreateInventoryItem")
	{
		return;
	}

	LLPointer<LLViewerInventoryItem> titem = new LLViewerInventoryItem;
	S32 num_blocks = msg->getNumberOfBlocksFast(_PREHASH_InventoryData);
	for(S32 i = 0; i < num_blocks; ++i)
	{
		titem->unpackMessage(msg, _PREHASH_InventoryData, i);
		if (!(titem->getUUID().isNull()))
		{
			//we don't do anything with null keys
			mAdded.push_back(titem->getUUID());
		}
	}
	if (!mAdded.empty())
	{
		done();
	}
}

LLInventoryTransactionObserver::LLInventoryTransactionObserver(
	const LLTransactionID& transaction_id) :
	mTransactionID(transaction_id)
{
}

void LLInventoryTransactionObserver::changed(U32 mask)
{
	if(mask & LLInventoryObserver::ADD)
	{
		// This could be it - see if we are processing a bulk update
		LLMessageSystem* msg = gMessageSystem;
		if(msg->getMessageName()
		   && (0 == strcmp(msg->getMessageName(), "BulkUpdateInventory")))
		{
			// we have a match for the message - now check the
			// transaction id.
			LLUUID id;
			msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_TransactionID, id);
			if(id == mTransactionID)
			{
				// woo hoo, we found it
				folder_ref_t folders;
				item_ref_t items;
				S32 count;
				count = msg->getNumberOfBlocksFast(_PREHASH_FolderData);
				S32 i;
				for(i = 0; i < count; ++i)
				{
					msg->getUUIDFast(_PREHASH_FolderData, _PREHASH_FolderID, id, i);
					if(id.notNull())
					{
						folders.push_back(id);
					}
				}
				count = msg->getNumberOfBlocksFast(_PREHASH_ItemData);
				for(i = 0; i < count; ++i)
				{
					msg->getUUIDFast(_PREHASH_ItemData, _PREHASH_ItemID, id, i);
					if(id.notNull())
					{
						items.push_back(id);
					}
				}

				// call the derived class the implements this method.
				done(folders, items);
			}
		}
	}
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
			(LLAssetType::lookupIsLinkType(item->getActualType())) &&
			(item->getLinkedUUID() == mBaseItemID)); // A linked item's assetID will be the compared-to item's itemID.
}

///----------------------------------------------------------------------------
/// Local function definitions
///----------------------------------------------------------------------------


/*
BOOL decompress_file(const char* src_filename, const char* dst_filename)
{
	BOOL rv = FALSE;
	gzFile src = NULL;
	U8* buffer = NULL;
	LLFILE* dst = NULL;
	S32 bytes = 0;
	const S32 DECOMPRESS_BUFFER_SIZE = 32000;

	// open the files
	src = gzopen(src_filename, "rb");
	if(!src) goto err_decompress;
	dst = LLFile::fopen(dst_filename, "wb");
	if(!dst) goto err_decompress;

	// decompress.
	buffer = new U8[DECOMPRESS_BUFFER_SIZE + 1];

	do
	{
		bytes = gzread(src, buffer, DECOMPRESS_BUFFER_SIZE);
		if (bytes < 0)
		{
			goto err_decompress;
		}

		fwrite(buffer, bytes, 1, dst);
	} while(gzeof(src) == 0);

	// success
	rv = TRUE;

 err_decompress:
	if(src != NULL) gzclose(src);
	if(buffer != NULL) delete[] buffer;
	if(dst != NULL) fclose(dst);
	return rv;
}
*/
