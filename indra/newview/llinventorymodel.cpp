/** 
 * @file llinventorymodel.cpp
 * @brief Implementation of the inventory model used to track agent inventory.
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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
#include "llinventorymodel.h"

#include "llagent.h"
#include "llagentwearables.h"
#include "llappearancemgr.h"
#include "llclipboard.h"
#include "llinventorypanel.h"
#include "llinventorybridge.h"
#include "llinventoryfunctions.h"
#include "llinventoryobserver.h"
#include "llinventorypanel.h"
#include "llnotificationsutil.h"
#include "llwindow.h"
#include "llviewercontrol.h"
#include "llpreview.h" 
#include "llviewermessage.h"
#include "llviewerfoldertype.h"
#include "llviewerwindow.h"
#include "llappviewer.h"
#include "llviewerregion.h"
#include "llcallbacklist.h"
#include "llvoavatarself.h"
#include "llgesturemgr.h"
#include <typeinfo>

//#define DIFF_INVENTORY_FILES
#ifdef DIFF_INVENTORY_FILES
#include "process.h"
#endif

// Increment this if the inventory contents change in a non-backwards-compatible way.
// For viewer 2, the addition of link items makes a pre-viewer-2 cache incorrect.
const S32 LLInventoryModel::sCurrentInvCacheVersion = 2;
BOOL LLInventoryModel::sFirstTimeInViewer2 = TRUE;

///----------------------------------------------------------------------------
/// Local function declarations, constants, enums, and typedefs
///----------------------------------------------------------------------------

//BOOL decompress_file(const char* src_filename, const char* dst_filename);
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
	mIsNotifyObservers(FALSE),
	mIsAgentInvUsable(false)
{
}

// Destroys the object
LLInventoryModel::~LLInventoryModel()
{
	cleanupInventory();
}

void LLInventoryModel::cleanupInventory()
{
	empty();
	// Deleting one observer might erase others from the list, so always pop off the front
	while (!mObservers.empty())
	{
		observer_list_t::iterator iter = mObservers.begin();
		LLInventoryObserver* observer = *iter;
		mObservers.erase(iter);
		delete observer;
	}
	mObservers.clear();
}

// This is a convenience function to check if one object has a parent
// chain up to the category specified by UUID.
BOOL LLInventoryModel::isObjectDescendentOf(const LLUUID& obj_id,
											const LLUUID& cat_id) const
{
	if (obj_id == cat_id) return TRUE;

	const LLInventoryObject* obj = getObject(obj_id);
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

const LLViewerInventoryCategory *LLInventoryModel::getFirstNondefaultParent(const LLUUID& obj_id) const
{
	const LLInventoryObject* obj = getObject(obj_id);

	// Search up the parent chain until we get to root or an acceptable folder.
	// This assumes there are no cycles in the tree else we'll get a hang.
	LLUUID parent_id = obj->getParentUUID();
	while (!parent_id.isNull())
	{
		const LLViewerInventoryCategory *cat = getCategory(parent_id);
		if (!cat) break;
		const LLFolderType::EType folder_type = cat->getPreferredType();
		if (folder_type != LLFolderType::FT_NONE &&
			folder_type != LLFolderType::FT_ROOT_INVENTORY &&
			!LLFolderType::lookupIsEnsembleType(folder_type))
		{
			return cat;
		}
		parent_id = cat->getParentUUID();
	}
	return NULL;
}

//
// Search up the parent chain until we get to the specified parent, then return the first child category under it
//
const LLViewerInventoryCategory* LLInventoryModel::getFirstDescendantOf(const LLUUID& master_parent_id, const LLUUID& obj_id) const
{
	if (master_parent_id == obj_id)
	{
		return NULL;
	}

	const LLViewerInventoryCategory* current_cat = getCategory(obj_id);

	if (current_cat == NULL)
	{
		current_cat = getCategory(getObject(obj_id)->getParentUUID());
	}
	
	while (current_cat != NULL)
	{
		const LLUUID& current_parent_id = current_cat->getParentUUID();
		
		if (current_parent_id == master_parent_id)
		{
			return current_cat;
		}
		
		current_cat = getCategory(current_parent_id);
	}

	return NULL;
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

LLMD5 LLInventoryModel::hashDirectDescendentNames(const LLUUID& cat_id) const
{
	LLInventoryModel::cat_array_t* cat_array;
	LLInventoryModel::item_array_t* item_array;
	getDirectDescendentsOf(cat_id,cat_array,item_array);
	LLMD5 item_name_hash;
	if (!item_array)
	{
		item_name_hash.finalize();
		return item_name_hash;
	}
	for (LLInventoryModel::item_array_t::const_iterator iter = item_array->begin();
		 iter != item_array->end();
		 iter++)
	{
		const LLViewerInventoryItem *item = (*iter);
		if (!item)
			continue;
		item_name_hash.update(item->getName());
	}
	item_name_hash.finalize();
	return item_name_hash;
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
const LLUUID LLInventoryModel::findCategoryUUIDForType(LLFolderType::EType preferred_type, 
													   bool create_folder, 
													   bool find_in_library)
{
	LLUUID rv = LLUUID::null;
	
	const LLUUID &root_id = (find_in_library) ? gInventory.getLibraryRootFolderID() : gInventory.getRootFolderID();
	if(LLFolderType::FT_ROOT_INVENTORY == preferred_type)
	{
		rv = root_id;
	}
	else if (root_id.notNull())
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
					rv = cats->get(i)->getUUID();
					break;
				}
			}
		}
	}
	
	if(rv.isNull() && isInventoryUsable() && (create_folder && !find_in_library))
	{
		if(root_id.notNull())
		{
			return createNewCategory(root_id, preferred_type, LLStringUtil::null);
		}
	}
	return rv;
}

class LLCreateInventoryCategoryResponder : public LLHTTPClient::Responder
{
public:
	LLCreateInventoryCategoryResponder(LLInventoryModel* model, 
									   void (*callback)(const LLSD&, void*),
									   void* user_data) :
										mModel(model),
										mCallback(callback), 
										mData(user_data) 
	{
	}
	
	virtual void error(U32 status, const std::string& reason)
	{
		LL_WARNS("InvAPI") << "CreateInventoryCategory failed.   status = " << status << ", reasion = \"" << reason << "\"" << LL_ENDL;
	}
	
	virtual void result(const LLSD& content)
	{
		//Server has created folder.
		
		LLUUID category_id = content["folder_id"].asUUID();
		
		
		// Add the category to the internal representation
		LLPointer<LLViewerInventoryCategory> cat =
		new LLViewerInventoryCategory( category_id, 
									  content["parent_id"].asUUID(),
									  (LLFolderType::EType)content["type"].asInteger(),
									  content["name"].asString(), 
									  gAgent.getID() );
		cat->setVersion(LLViewerInventoryCategory::VERSION_INITIAL);
		cat->setDescendentCount(0);
		LLInventoryModel::LLCategoryUpdate update(cat->getParentUUID(), 1);
		mModel->accountForUpdate(update);
		mModel->updateCategory(cat);
		
		if (mCallback && mData)
		{
			mCallback(content, mData);
		}
		
	}
	
private:
	void (*mCallback)(const LLSD&, void*);
	void* mData;
	LLInventoryModel* mModel;
};

// Convenience function to create a new category. You could call
// updateCategory() with a newly generated UUID category, but this
// version will take care of details like what the name should be
// based on preferred type. Returns the UUID of the new category.
LLUUID LLInventoryModel::createNewCategory(const LLUUID& parent_id,
										   LLFolderType::EType preferred_type,
										   const std::string& pname,
										   void (*callback)(const LLSD&, void*),	//Default to NULL
										   void* user_data)							//Default to NULL
{
	
	LLUUID id;
	if(!isInventoryUsable())
	{
		llwarns << "Inventory is broken." << llendl;
		return id;
	}

	if(LLFolderType::lookup(preferred_type) == LLFolderType::badLookup())
	{
		lldebugs << "Attempt to create undefined category." << llendl;
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
		name.assign(LLViewerFolderType::lookupNewCategoryName(preferred_type));
	}
	
	if ( callback && user_data )  //callback required for acked message.
	{
		LLViewerRegion* viewer_region = gAgent.getRegion();
		std::string url;
		if ( viewer_region )
			url = viewer_region->getCapability("CreateInventoryCategory");
		
		if (!url.empty())
		{
			//Let's use the new capability.
			
			LLSD request, body;
			body["folder_id"] = id;
			body["parent_id"] = parent_id;
			body["type"] = (LLSD::Integer) preferred_type;
			body["name"] = name;
			
			request["message"] = "CreateInventoryCategory";
			request["payload"] = body;
			
	//		viewer_region->getCapAPI().post(request);
			LLHTTPClient::post(
							   url,
							   body,
							   new LLCreateInventoryCategoryResponder(this, callback, user_data) );
			return LLUUID::null;
		}
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
											LLInventoryCollectFunctor& add,
											BOOL follow_folder_links)
{
	// Start with categories
	if(!include_trash)
	{
		const LLUUID trash_id = findCategoryUUIDForType(LLFolderType::FT_TRASH);
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

	LLViewerInventoryItem* item = NULL;
	item_array_t* item_array = get_ptr_in_map(mParentChildItemTree, id);

	// Follow folder links recursively.  Currently never goes more
	// than one level deep (for current outfit support)
	// Note: if making it fully recursive, need more checking against infinite loops.
	if (follow_folder_links && item_array)
	{
		S32 count = item_array->count();
		for(S32 i = 0; i < count; ++i)
		{
			item = item_array->get(i);
			if (item && item->getActualType() == LLAssetType::AT_LINK_FOLDER)
			{
				LLViewerInventoryCategory *linked_cat = item->getLinkedCategory();
				if (linked_cat && linked_cat->getPreferredType() != LLFolderType::FT_OUTFIT)
					// BAP - was 
					// LLAssetType::lookupIsEnsembleCategoryType(linked_cat->getPreferredType()))
					// Change back once ensemble typing is in place.
				{
					if(add(linked_cat,NULL))
					{
						// BAP should this be added here?  May not
						// matter if it's only being used in current
						// outfit traversal.
						cats.put(LLPointer<LLViewerInventoryCategory>(linked_cat));
					}
					collectDescendentsIf(linked_cat->getUUID(), cats, items, include_trash, add, FALSE);
				}
			}
		}
	}
	
	// Move onto items
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

void LLInventoryModel::addChangedMaskForLinks(const LLUUID& object_id, U32 mask)
{
	const LLInventoryObject *obj = getObject(object_id);
	if (!obj || obj->getIsLinkType())
		return;

	LLInventoryModel::cat_array_t cat_array;
	LLInventoryModel::item_array_t item_array;
	LLLinkedItemIDMatches is_linked_item_match(object_id);
	collectDescendentsIf(gInventory.getRootFolderID(),
						 cat_array,
						 item_array,
						 LLInventoryModel::INCLUDE_TRASH,
						 is_linked_item_match);
	if (cat_array.empty() && item_array.empty())
	{
		return;
	}
	for (LLInventoryModel::cat_array_t::iterator cat_iter = cat_array.begin();
		 cat_iter != cat_array.end();
		 cat_iter++)
	{
		LLViewerInventoryCategory *linked_cat = (*cat_iter);
		addChangedMask(mask, linked_cat->getUUID());
	};

	for (LLInventoryModel::item_array_t::iterator iter = item_array.begin();
		 iter != item_array.end();
		 iter++)
	{
		LLViewerInventoryItem *linked_item = (*iter);
		addChangedMask(mask, linked_item->getUUID());
	};
}

const LLUUID& LLInventoryModel::getLinkedItemID(const LLUUID& object_id) const
{
	const LLInventoryItem *item = gInventory.getItem(object_id);
	if (!item)
	{
		return object_id;
	}

	// Find the base item in case this a link (if it's not a link,
	// this will just be inv_item_id)
	return item->getLinkedUUID();
}

LLViewerInventoryItem* LLInventoryModel::getLinkedItem(const LLUUID& object_id) const
{
	return object_id.notNull() ? getItem(getLinkedItemID(object_id)) : NULL;
}

LLInventoryModel::item_array_t LLInventoryModel::collectLinkedItems(const LLUUID& id,
																	const LLUUID& start_folder_id)
{
	item_array_t items;
	LLInventoryModel::cat_array_t cat_array;
	LLLinkedItemIDMatches is_linked_item_match(id);
	collectDescendentsIf((start_folder_id == LLUUID::null ? gInventory.getRootFolderID() : start_folder_id),
						 cat_array,
						 items,
						 LLInventoryModel::INCLUDE_TRASH,
						 is_linked_item_match);
	return items;
}

bool LLInventoryModel::isInventoryUsable() const
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

	// We're hiding mesh types
#if 0
	if (item->getType() == LLAssetType::AT_MESH)
	{
		return mask;
	}
#endif

	LLViewerInventoryItem* old_item = getItem(item->getUUID());
	LLPointer<LLViewerInventoryItem> new_item;
	if(old_item)
	{
		// We already have an old item, modify its values
		new_item = old_item;
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
		new_item = new LLViewerInventoryItem(item);
		addItem(new_item);

		if(item->getParentUUID().isNull())
		{
			const LLUUID category_id = findCategoryUUIDForType(LLFolderType::assetTypeToFolderType(new_item->getType()));
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
				parent_id = findCategoryUUIDForType(LLFolderType::FT_LOST_AND_FOUND);
				new_item->setParent(parent_id);
				LLInventoryModel::update_list_t update;
				LLInventoryModel::LLCategoryUpdate new_folder(parent_id, 1);
				update.push_back(new_folder);
				accountForUpdate(update);

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
				parent_id = findCategoryUUIDForType(LLFolderType::FT_LOST_AND_FOUND);
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
	if(new_item->getType() == LLAssetType::AT_CALLINGCARD)
	{
		mask |= LLInventoryObserver::CALLING_CARD;
		// Handle user created calling cards.
		// Target ID is stored in the description field of the card.
		LLUUID id;
		std::string desc = new_item->getDescription();
		BOOL isId = desc.empty() ? FALSE : id.set(desc, FALSE);
		if (isId)
		{
			// Valid UUID; set the item UUID and rename it
			new_item->setCreator(id);
			std::string avatar_name;

			if (gCacheName->getFullName(id, avatar_name))
			{
				new_item->rename(avatar_name);
				mask |= LLInventoryObserver::LABEL;
			}
			else
			{
				// Fetch the current name
				gCacheName->get(id, FALSE,
					boost::bind(&LLViewerInventoryItem::onCallingCardNameLookup, new_item.get(),
					_1, _2, _3));
			}

		}
	}
	else if (new_item->getType() == LLAssetType::AT_GESTURE)
	{
		mask |= LLInventoryObserver::GESTURE;
	}
	addChangedMask(mask, new_item->getUUID());
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

// Migrated from llinventoryfunctions
void LLInventoryModel::changeItemParent(LLViewerInventoryItem* item,
										const LLUUID& new_parent_id,
										BOOL restamp)
{
	if (item->getParentUUID() == new_parent_id)
	{
		LL_DEBUGS("Inventory") << "'" << item->getName() << "' (" << item->getUUID()
							   << ") is already in folder " << new_parent_id << LL_ENDL;
	}
	else
	{
		LL_INFOS("Inventory") << "Moving '" << item->getName() << "' (" << item->getUUID()
							  << ") from " << item->getParentUUID() << " to folder "
							  << new_parent_id << LL_ENDL;
		LLInventoryModel::update_list_t update;
		LLInventoryModel::LLCategoryUpdate old_folder(item->getParentUUID(),-1);
		update.push_back(old_folder);
		LLInventoryModel::LLCategoryUpdate new_folder(new_parent_id, 1);
		update.push_back(new_folder);
		accountForUpdate(update);

		LLPointer<LLViewerInventoryItem> new_item = new LLViewerInventoryItem(item);
		new_item->setParent(new_parent_id);
		new_item->updateParentOnServer(restamp);
		updateItem(new_item);
		notifyObservers();
	}
}

// Migrated from llinventoryfunctions
void LLInventoryModel::changeCategoryParent(LLViewerInventoryCategory* cat,
											const LLUUID& new_parent_id,
											BOOL restamp)
{
	if (!cat)
	{
		return;
	}

	// Can't move a folder into a child of itself.
	if (isObjectDescendentOf(new_parent_id, cat->getUUID()))
	{
		return;
	}

	LLInventoryModel::update_list_t update;
	LLInventoryModel::LLCategoryUpdate old_folder(cat->getParentUUID(), -1);
	update.push_back(old_folder);
	LLInventoryModel::LLCategoryUpdate new_folder(new_parent_id, 1);
	update.push_back(new_folder);
	accountForUpdate(update);

	LLPointer<LLViewerInventoryCategory> new_cat = new LLViewerInventoryCategory(cat);
	new_cat->setParent(new_parent_id);
	new_cat->updateParentOnServer(restamp);
	updateCategory(new_cat);
	notifyObservers();
}

// Delete a particular inventory object by ID.
void LLInventoryModel::deleteObject(const LLUUID& id)
{
	lldebugs << "LLInventoryModel::deleteObject()" << llendl;
	LLPointer<LLInventoryObject> obj = getObject(id);
	if (!obj) 
	{
		llwarns << "Deleting non-existent object [ id: " << id << " ] " << llendl;
		return;
	}
	
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
	updateLinkedObjectsFromPurge(id);
	gInventory.notifyObservers();
}

// Delete a particular inventory item by ID, and remove it from the server.
void LLInventoryModel::purgeObject(const LLUUID &id)
{
	lldebugs << "LLInventoryModel::purgeObject() [ id: " << id << " ] " << llendl;
	LLPointer<LLInventoryObject> obj = getObject(id);
	if(obj)
	{
		obj->removeFromServer();
		LLPreview::hide(id);
		deleteObject(id);
	}
}

void LLInventoryModel::updateLinkedObjectsFromPurge(const LLUUID &baseobj_id)
{
	LLInventoryModel::item_array_t item_array = collectLinkedItems(baseobj_id);

	// REBUILD is expensive, so clear the current change list first else
	// everything else on the changelist will also get rebuilt.
	gInventory.notifyObservers();
	for (LLInventoryModel::item_array_t::const_iterator iter = item_array.begin();
		 iter != item_array.end();
		 iter++)
	{
		const LLViewerInventoryItem *linked_item = (*iter);
		const LLUUID &item_id = linked_item->getUUID();
		if (item_id == baseobj_id) continue;
		addChangedMask(LLInventoryObserver::REBUILD, item_id);
	}
	gInventory.notifyObservers();
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
	if (cat.notNull())
	{
		if (LLClipboard::instance().hasContents() && LLClipboard::instance().isCutMode())
		{
			// Something on the clipboard is in "cut mode" and needs to be preserved
			llinfos << "LLInventoryModel::purgeDescendentsOf " << cat->getName()
			<< " iterate and purge non hidden items" << llendl;
			cat_array_t* categories;
			item_array_t* items;
			// Get the list of direct descendants in tha categoy passed as argument
			getDirectDescendentsOf(id, categories, items);
			std::vector<LLUUID> list_uuids;
			// Make a unique list with all the UUIDs of the direct descendants (items and categories are not treated differently)
			// Note: we need to do that shallow copy as purging things will invalidate the categories or items lists
			for (cat_array_t::const_iterator it = categories->begin(); it != categories->end(); ++it)
			{
				list_uuids.push_back((*it)->getUUID());
			}
			for (item_array_t::const_iterator it = items->begin(); it != items->end(); ++it)
			{
				list_uuids.push_back((*it)->getUUID());
			}
			// Iterate through the list and only purge the UUIDs that are not on the clipboard
			for (std::vector<LLUUID>::const_iterator it = list_uuids.begin(); it != list_uuids.end(); ++it)
			{
				if (!LLClipboard::instance().isOnClipboard(*it))
				{
					purgeObject(*it);
				}
			}
		}
		else
		{
			// Fast purge
			// do the cache accounting
			llinfos << "LLInventoryModel::purgeDescendentsOf " << cat->getName()
				<< llendl;
			S32 descendents = cat->getDescendentCount();
			if(descendents > 0)
			{
				LLCategoryUpdate up(id, -descendents);
				accountForUpdate(up);
			}

			// we know that descendent count is 0, however since the
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
			for(S32 i = 0; i < count; ++i)
			{
				deleteObject(items.get(i)->getUUID());
			}
			count = categories.count();
			for(S32 i = 0; i < count; ++i)
			{
				deleteObject(categories.get(i)->getUUID());
			}
		}
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

BOOL LLInventoryModel::containsObserver(LLInventoryObserver* observer) const
{
	return mObservers.find(observer) != mObservers.end();
}

void LLInventoryModel::idleNotifyObservers()
{
	if (mModifyMask == LLInventoryObserver::NONE && (mChangedItemIDs.size() == 0))
	{
		return;
	}
	notifyObservers();
}

// Call this method when it's time to update everyone on a new state.
void LLInventoryModel::notifyObservers()
{
	if (mIsNotifyObservers)
	{
		// Within notifyObservers, something called notifyObservers
		// again.  This type of recursion is unsafe because it causes items to be 
		// processed twice, and this can easily lead to infinite loops.
		llwarns << "Call was made to notifyObservers within notifyObservers!" << llendl;
		return;
	}

	mIsNotifyObservers = TRUE;
	for (observer_list_t::iterator iter = mObservers.begin();
		 iter != mObservers.end(); )
	{
		LLInventoryObserver* observer = *iter;
		observer->changed(mModifyMask);

		// safe way to increment since changed may delete entries! (@!##%@!@&*!)
		iter = mObservers.upper_bound(observer); 
	}

	mModifyMask = LLInventoryObserver::NONE;
	mChangedItemIDs.clear();
	mIsNotifyObservers = FALSE;
}

// store flag for change
// and id of object change applies to
void LLInventoryModel::addChangedMask(U32 mask, const LLUUID& referent) 
{ 
	if (mIsNotifyObservers)
	{
		// Something marked an item for change within a call to notifyObservers
		// (which is in the process of processing the list of items marked for change).
		// This means the change may fail to be processed.
		llwarns << "Adding changed mask within notify observers!  Change will likely be lost." << llendl;
	}
	
	mModifyMask |= mask; 
	if (referent.notNull())
	{
		mChangedItemIDs.insert(referent);
	}
	
	// Update all linked items.  Starting with just LABEL because I'm
	// not sure what else might need to be accounted for this.
	if (mModifyMask & LLInventoryObserver::LABEL)
	{
		addChangedMaskForLinks(referent, LLInventoryObserver::LABEL);
	}
}

// If we get back a normal response, handle it here
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
	}

	U32 changes = 0x0;
	//as above, this loop never seems to loop more than once per call
	for (item_array_t::iterator it = items.begin(); it != items.end(); ++it)
	{
		changes |= gInventory.updateItem(*it);
	}
	gInventory.notifyObservers();
	gViewerWindow->getWindow()->decBusyCount();
}

//If we get back an error (not found, etc...), handle it here
void LLInventoryModel::fetchInventoryResponder::error(U32 status, const std::string& reason)
{
	llinfos << "fetchInventory::error "
		<< status << ": " << reason << llendl;
	gInventory.notifyObservers();
}

bool LLInventoryModel::fetchDescendentsOf(const LLUUID& folder_id) const
{
	if(folder_id.isNull()) 
	{
		llwarns << "Calling fetch descendents on NULL folder id!" << llendl;
		return false;
	}
	LLViewerInventoryCategory* cat = getCategory(folder_id);
	if(!cat)
	{
		llwarns << "Asked to fetch descendents of non-existent folder: "
				<< folder_id << llendl;
		return false;
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
	return cat->fetch();
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
		// We aren't displaying the Meshes folder
		if (category->mPreferredType == LLFolderType::FT_MESH)
		{
			return;
		}

		// try to localize default names first. See EXT-8319, EXT-7051.
		category->localizeName();

		// Insert category uniquely into the map
		mCategoryMap[category->getUUID()] = category; // LLPointer will deref and delete the old one
		//mInventory[category->getUUID()] = category;
	}
}

void LLInventoryModel::addItem(LLViewerInventoryItem* item)
{
	llassert(item);
	if(item)
	{
		// This can happen if assettype enums from llassettype.h ever change.
		// For example, there is a known backwards compatibility issue in some viewer prototypes prior to when 
		// the AT_LINK enum changed from 23 to 24.
		if ((item->getType() == LLAssetType::AT_NONE)
		    || LLAssetType::lookup(item->getType()) == LLAssetType::badLookup())
		{
			llwarns << "Got bad asset type for item [ name: " << item->getName() << " type: " << item->getType() << " inv-type: " << item->getInventoryType() << " ], ignoring." << llendl;
			return;
		}

		// This condition means that we tried to add a link without the baseobj being in memory.
		// The item will show up as a broken link.
		if (item->getIsBrokenLink())
		{
			llinfos << "Adding broken link [ name: " << item->getName() << " itemID: " << item->getUUID() << " assetID: " << item->getAssetUUID() << " )  parent: " << item->getParentUUID() << llendl;
		}

		mItemMap[item->getUUID()] = item;
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

void LLInventoryModel::accountForUpdate(const LLCategoryUpdate& update) const
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
				lldebugs << "accounted: '" << cat->getName() << "' "
						 << version << " with " << descendents_actual
						 << " descendents." << llendl;
			}
		}
		if(!accounted)
		{
			// Error condition, this means that the category did not register that
			// it got new descendents (perhaps because it is still being loaded)
			// which means its descendent count will be wrong.
			llwarns << "Accounting failed for '" << cat->getName() << "' version:"
					 << version << llendl;
		}
	}
	else
	{
		llwarns << "No category found for update " << update.mCategoryID << llendl;
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

			LLFolderType::EType preferred_type = LLFolderType::FT_NONE;
			LLSD type_default = (*it)["type_default"];
			if(type_default.isDefined())
            {
				preferred_type = (LLFolderType::EType)type_default.asInteger();
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
		item_array_t possible_broken_links;
		cat_set_t invalid_categories; // Used to mark categories that weren't successfully loaded.
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
		bool is_cache_obsolete = false;
		if(loadFromFile(inventory_filename, categories, items, is_cache_obsolete))
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
				// not sent down in the skeleton. Must have been removed from inventory.
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
			std::set<LLUUID>::const_iterator not_cached_id = cached_ids.end();
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
			S32 bad_link_count = 0;
			S32 good_link_count = 0;
			S32 recovered_link_count = 0;
			cat_map_t::iterator unparented = mCategoryMap.end();
			for(item_array_t::const_iterator item_iter = items.begin();
				item_iter != items.end();
				++item_iter)
			{
				LLViewerInventoryItem *item = (*item_iter).get();
				const cat_map_t::iterator cit = mCategoryMap.find(item->getParentUUID());
				
				if(cit != unparented)
				{
					const LLViewerInventoryCategory* cat = cit->second.get();
					if(cat->getVersion() != NO_VERSION)
					{
						// This can happen if the linked object's baseobj is removed from the cache but the linked object is still in the cache.
						if (item->getIsBrokenLink())
						{
							//bad_link_count++;
							lldebugs << "Attempted to add cached link item without baseobj present ( name: "
									 << item->getName() << " itemID: " << item->getUUID()
									 << " assetID: " << item->getAssetUUID()
									 << " ).  Ignoring and invalidating " << cat->getName() << " . " << llendl;
							possible_broken_links.push_back(item);
							continue;
						}
						else if (item->getIsLinkType())
						{
							good_link_count++;
						}
						addItem(item);
						cached_item_count += 1;
						++child_counts[cat->getUUID()];
					}
				}
			}
			if (possible_broken_links.size() > 0)
			{
				for(item_array_t::const_iterator item_iter = possible_broken_links.begin();
				    item_iter != possible_broken_links.end();
				    ++item_iter)
				{
					LLViewerInventoryItem *item = (*item_iter).get();
					const cat_map_t::iterator cit = mCategoryMap.find(item->getParentUUID());
					const LLViewerInventoryCategory* cat = cit->second.get();
					if (item->getIsBrokenLink())
					{
						bad_link_count++;
						invalid_categories.insert(cit->second);
						//llinfos << "link still broken: " << item->getName() << " in folder " << cat->getName() << llendl;
					}
					else
					{
						// was marked as broken because of loading order, its actually fine to load
						addItem(item);
						cached_item_count += 1;
						++child_counts[cat->getUUID()];
						recovered_link_count++;
					}
				}

 				llinfos << "Attempted to add " << bad_link_count
 						<< " cached link items without baseobj present. "
					    << good_link_count << " link items were successfully added. "
					    << recovered_link_count << " links added in recovery. "
 						<< "The corresponding categories were invalidated." << llendl;
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

		// Invalidate all categories that failed fetching descendents for whatever
		// reason (e.g. one of the descendents was a broken link).
		for (cat_set_t::iterator invalid_cat_it = invalid_categories.begin();
			 invalid_cat_it != invalid_categories.end();
			 invalid_cat_it++)
		{
			LLViewerInventoryCategory* cat = (*invalid_cat_it).get();
			cat->setVersion(NO_VERSION);
			llinfos << "Invalidating category name: " << cat->getName() << " UUID: " << cat->getUUID() << " due to invalid descendents cache" << llendl;
		}

		// At this point, we need to set the known descendents for each
		// category which successfully cached so that we do not
		// needlessly fetch descendents for categories which we have.
		update_map_t::const_iterator no_child_counts = child_counts.end();
		for(cat_set_t::iterator it = temp_cats.begin(); it != temp_cats.end(); ++it)
		{
			LLViewerInventoryCategory* cat = (*it).get();
			if(cat->getVersion() != NO_VERSION)
			{
				update_map_t::const_iterator the_count = child_counts.find(cat->getUUID());
				if(the_count != no_child_counts)
				{
					const S32 num_descendents = (*the_count).second.mValue;
					cat->setDescendentCount(num_descendents);
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
		if(is_cache_obsolete)
		{
			// If out of date, remove the gzipped file too.
			llwarns << "Inv cache out of date, removing" << llendl;
			LLFile::remove(gzip_filename);
		}
		categories.clear(); // will unref and delete entries
	}

	llinfos << "Successfully loaded " << cached_category_count
			<< " categories and " << cached_item_count << " items from cache."
			<< llendl;

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
			LLFolderType::EType pref = cat->getPreferredType();
			if(LLFolderType::FT_NONE == pref)
			{
				cat->setParent(findCategoryUUIDForType(LLFolderType::FT_LOST_AND_FOUND));
			}
			else if(LLFolderType::FT_ROOT_INVENTORY == pref)
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

	const BOOL COF_exists = (findCategoryUUIDForType(LLFolderType::FT_CURRENT_OUTFIT, FALSE) != LLUUID::null);
	sFirstTimeInViewer2 = !COF_exists || gAgent.isFirstLogin();


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
	uuid_vec_t lost_item_ids;
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
			item->setParent(findCategoryUUIDForType(LLFolderType::FT_LOST_AND_FOUND));
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
		const LLUUID lnf = findCategoryUUIDForType(LLFolderType::FT_LOST_AND_FOUND);
		for(uuid_vec_t::iterator it = lost_item_ids.begin() ; it < lost_item_ids.end(); ++it)
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

	const LLUUID &agent_inv_root_id = gInventory.getRootFolderID();
	if (agent_inv_root_id.notNull())
	{
		cat_array_t* catsp = get_ptr_in_map(mParentChildCategoryTree, agent_inv_root_id);
		if(catsp)
		{
			// *HACK - fix root inventory folder
			// some accounts has pbroken inventory root folders
			
			std::string name = "My Inventory";
			LLUUID prev_root_id = mRootFolderID;
			for (parent_cat_map_t::const_iterator it = mParentChildCategoryTree.begin(),
					 it_end = mParentChildCategoryTree.end(); it != it_end; ++it)
			{
				cat_array_t* cat_array = it->second;
				for (cat_array_t::const_iterator cat_it = cat_array->begin(),
						 cat_it_end = cat_array->end(); cat_it != cat_it_end; ++cat_it)
					{
					LLPointer<LLViewerInventoryCategory> category = *cat_it;

					if(category && category->getPreferredType() != LLFolderType::FT_ROOT_INVENTORY)
						continue;
					if ( category && 0 == LLStringUtil::compareInsensitive(name, category->getName()) )
					{
						if(category->getUUID()!=mRootFolderID)
						{
							LLUUID& new_inv_root_folder_id = const_cast<LLUUID&>(mRootFolderID);
							new_inv_root_folder_id = category->getUUID();
						}
					}
				}
			}

			// 'My Inventory',
			// root of the agent's inv found.
			// The inv tree is built.
			mIsAgentInvUsable = true;

			llinfos << "Inventory initialized, notifying observers" << llendl;
			addChangedMask(LLInventoryObserver::ALL, LLUUID::null);
			notifyObservers();
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

// static
bool LLInventoryModel::loadFromFile(const std::string& filename,
									LLInventoryModel::cat_array_t& categories,
									LLInventoryModel::item_array_t& items,
									bool &is_cache_obsolete)
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
	char value[MAX_STRING];			/*Flawfinder: ignore*/
	is_cache_obsolete = true;  		// Obsolete until proven current
	while(!feof(file) && fgets(buffer, MAX_STRING, file)) 
	{
		sscanf(buffer, " %126s %126s", keyword, value);	/* Flawfinder: ignore */
		if(0 == strcmp("inv_cache_version", keyword))
		{
			S32 version;
			int succ = sscanf(value,"%d",&version);
			if ((1 == succ) && (version == sCurrentInvCacheVersion))
			{
				// Cache is up to date
				is_cache_obsolete = false;
				continue;
			}
			else
			{
				// Cache is out of date
				break;
			}
		}
		else if(0 == strcmp("inv_category", keyword))
		{
			if (is_cache_obsolete)
				break;
			
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
			if (is_cache_obsolete)
				break;

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
	if (is_cache_obsolete)
		return false;
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

	fprintf(file, "\tinv_cache_version\t%d\n",sCurrentInvCacheVersion);
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
	msg->setHandlerFuncFast(_PREHASH_RemoveInventoryObjects,
							processRemoveInventoryObjects,
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
void LLInventoryModel::removeInventoryItem(LLUUID agent_id, LLMessageSystem* msg, const char* msg_label)
{
	LLUUID item_id;
	S32 count = msg->getNumberOfBlocksFast(msg_label);
	lldebugs << "Message has " << count << " item blocks" << llendl;
	uuid_vec_t item_ids;
	update_map_t update;
	for(S32 i = 0; i < count; ++i)
	{
		msg->getUUIDFast(msg_label, _PREHASH_ItemID, item_id, i);
		lldebugs << "Checking for item-to-be-removed " << item_id << llendl;
		LLViewerInventoryItem* itemp = gInventory.getItem(item_id);
		if(itemp)
		{
		lldebugs << "Item will be removed " << item_id << llendl;
			// we only bother with the delete and account if we found
			// the item - this is usually a back-up for permissions,
			// so frequently the item will already be gone.
			--update[itemp->getParentUUID()];
			item_ids.push_back(item_id);
		}
	}
	gInventory.accountForUpdate(update);
	for(uuid_vec_t::iterator it = item_ids.begin(); it != item_ids.end(); ++it)
	{
		lldebugs << "Calling deleteObject " << *it << llendl;
		gInventory.deleteObject(*it);
	}
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
	LLInventoryModel::removeInventoryItem(agent_id, msg, _PREHASH_InventoryData);
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
		tfolder->setPreferredType(LLFolderType::FT_NONE);
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
	LLInventoryPanel *active_panel = LLInventoryPanel::getActiveInventoryPanel();
	if (active_panel)
	{
		active_panel->setSelection(lastfolder->getUUID(), TAKE_FOCUS_NO);
	}
}

// 	static
void LLInventoryModel::removeInventoryFolder(LLUUID agent_id,
											 LLMessageSystem* msg)
{
	LLUUID folder_id;
	uuid_vec_t folder_ids;
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
	for(uuid_vec_t::iterator it = folder_ids.begin(); it != folder_ids.end(); ++it)
	{
		gInventory.deleteObject(*it);
	}
}

// 	static
void LLInventoryModel::processRemoveInventoryFolder(LLMessageSystem* msg,
													void**)
{
	lldebugs << "LLInventoryModel::processRemoveInventoryFolder()" << llendl;
	LLUUID agent_id, session_id;
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_AgentID, agent_id);
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_SessionID, session_id);
	if(agent_id != gAgent.getID())
	{
		llwarns << "Got a RemoveInventoryFolder for the wrong agent."
		<< llendl;
		return;
	}
	LLInventoryModel::removeInventoryFolder( agent_id, msg );
	gInventory.notifyObservers();
}

// 	static
void LLInventoryModel::processRemoveInventoryObjects(LLMessageSystem* msg,
													void**)
{
	lldebugs << "LLInventoryModel::processRemoveInventoryObjects()" << llendl;
	LLUUID agent_id, session_id;
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_AgentID, agent_id);
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_SessionID, session_id);
	if(agent_id != gAgent.getID())
	{
		llwarns << "Got a RemoveInventoryObjects for the wrong agent."
		<< llendl;
		return;
	}
	LLInventoryModel::removeInventoryFolder( agent_id, msg );
	LLInventoryModel::removeInventoryItem( agent_id, msg, _PREHASH_ItemData );
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
#ifndef LL_RELEASE_FOR_DOWNLOAD
	llinfos << "Bulk inventory: " << tid << llendl;
#endif

	update_map_t update;
	cat_array_t folders;
	S32 count;
	S32 i;
	count = msg->getNumberOfBlocksFast(_PREHASH_FolderData);
	for(i = 0; i < count; ++i)
	{
		LLPointer<LLViewerInventoryCategory> tfolder = new LLViewerInventoryCategory(gAgent.getID());
		tfolder->unpackMessage(msg, _PREHASH_FolderData, i);
		llinfos << "unpacked folder '" << tfolder->getName() << "' ("
				<< tfolder->getUUID() << ") in " << tfolder->getParentUUID()
				<< llendl;
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
	uuid_vec_t wearable_ids;
	item_array_t items;
	std::list<InventoryCallbackInfo> cblist;
	for(i = 0; i < count; ++i)
	{
		LLPointer<LLViewerInventoryItem> titem = new LLViewerInventoryItem;
		titem->unpackMessage(msg, _PREHASH_ItemData, i);
		llinfos << "unpacked item '" << titem->getName() << "' in "
				<< titem->getParentUUID() << llendl;
		U32 callback_id;
		msg->getU32Fast(_PREHASH_ItemData, _PREHASH_CallbackID, callback_id);
		if(titem->getUUID().notNull() ) // && callback_id.notNull() )
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
	if (LLInventoryState::sWearNewClothing)
	{
		LLInventoryState::sWearNewClothingTransactionID = tid;
		LLInventoryState::sWearNewClothing = FALSE;
	}

	if (tid.notNull() && tid == LLInventoryState::sWearNewClothingTransactionID)
	{
		count = wearable_ids.size();
		for (i = 0; i < count; ++i)
		{
			LLViewerInventoryItem* wearable_item;
			wearable_item = gInventory.getItem(wearable_ids[i]);
			LLAppearanceMgr::instance().wearItemOnAvatar(wearable_item->getUUID(), true, true);
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
		llwarns << "Got a UpdateInventoryItem for the wrong agent." << llendl;
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
		// If the item has already been added (e.g. from link prefetch), then it doesn't need to be re-added.
		if (gInventory.getItem(titem->getUUID()))
		{
			lldebugs << "Skipping prefetched item [ Name: " << titem->getName() << " | Type: " << titem->getActualType() << " | ItemUUID: " << titem->getUUID() << " ] " << llendl;
			continue;
		}
		gInventory.updateItem(titem);
	}

	// set version and descendentcount according to message.
	LLViewerInventoryCategory* cat = gInventory.getCategory(parent_id);
	if(cat)
	{
		cat->setVersion(version);
		cat->setDescendentCount(descendents);
		// Get this UUID on the changed list so that whatever's listening for it
		// will get triggered.
		gInventory.addChangedMask(LLInventoryObserver::INTERNAL, cat->getUUID());
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

// Trash: LLFolderType::FT_TRASH, "ConfirmEmptyTrash"
// Lost&Found: LLFolderType::FT_LOST_AND_FOUND, "ConfirmEmptyLostAndFound"

bool LLInventoryModel::callbackEmptyFolderType(const LLSD& notification, const LLSD& response, LLFolderType::EType preferred_type)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if (option == 0) // YES
	{
		const LLUUID folder_id = findCategoryUUIDForType(preferred_type);
		purgeDescendentsOf(folder_id);
		notifyObservers();
	}
	return false;
}

void LLInventoryModel::emptyFolderType(const std::string notification, LLFolderType::EType preferred_type)
{
	if (!notification.empty())
	{
		LLNotificationsUtil::add(notification, LLSD(), LLSD(),
										boost::bind(&LLInventoryModel::callbackEmptyFolderType, this, _1, _2, preferred_type));
	}
	else
	{
		const LLUUID folder_id = findCategoryUUIDForType(preferred_type);
		purgeDescendentsOf(folder_id);
		notifyObservers();
	}
}

//----------------------------------------------------------------------------

void LLInventoryModel::removeItem(const LLUUID& item_id)
{
	LLViewerInventoryItem* item = getItem(item_id);
	if (! item)
	{
		LL_WARNS("Inventory") << "couldn't find inventory item " << item_id << LL_ENDL;
	}
	else
	{
		const LLUUID new_parent = findCategoryUUIDForType(LLFolderType::FT_TRASH);
		if (new_parent.notNull())
		{
			LL_INFOS("Inventory") << "Moving to Trash (" << new_parent << "):" << LL_ENDL;
			changeItemParent(item, new_parent, TRUE);
		}
	}
}

void LLInventoryModel::removeCategory(const LLUUID& category_id)
{
	if (! get_is_category_removable(this, category_id))
	{
		return;
	}

	// Look for any gestures and deactivate them
	LLInventoryModel::cat_array_t	descendent_categories;
	LLInventoryModel::item_array_t	descendent_items;
	collectDescendents(category_id, descendent_categories, descendent_items, FALSE);

	for (LLInventoryModel::item_array_t::const_iterator iter = descendent_items.begin();
		 iter != descendent_items.end();
		 ++iter)
	{
		const LLViewerInventoryItem* item = (*iter);
		const LLUUID& item_id = item->getUUID();
		if (item->getType() == LLAssetType::AT_GESTURE
			&& LLGestureMgr::instance().isGestureActive(item_id))
		{
			LLGestureMgr::instance().deactivateGesture(item_id);
		}
	}

	LLViewerInventoryCategory* cat = getCategory(category_id);
	if (cat)
	{
		const LLUUID trash_id = findCategoryUUIDForType(LLFolderType::FT_TRASH);
		if (trash_id.notNull())
		{
			changeCategoryParent(cat, trash_id, TRUE);
		}
	}
}

void LLInventoryModel::removeObject(const LLUUID& object_id)
{
	LLInventoryObject* obj = getObject(object_id);
	if (dynamic_cast<LLViewerInventoryItem*>(obj))
	{
		removeItem(object_id);
	}
	else if (dynamic_cast<LLViewerInventoryCategory*>(obj))
	{
		removeCategory(object_id);
	}
	else if (obj)
	{
		LL_WARNS("Inventory") << "object ID " << object_id
							  << " is an object of unrecognized class "
							  << typeid(*obj).name() << LL_ENDL;
	}
	else
	{
		LL_WARNS("Inventory") << "object ID " << object_id << " not found" << LL_ENDL;
	}
}

const LLUUID &LLInventoryModel::getRootFolderID() const
{
	return mRootFolderID;
}

void LLInventoryModel::setRootFolderID(const LLUUID& val)
{
	mRootFolderID = val;
}

const LLUUID &LLInventoryModel::getLibraryRootFolderID() const
{
	return mLibraryRootFolderID;
}

void LLInventoryModel::setLibraryRootFolderID(const LLUUID& val)
{
	mLibraryRootFolderID = val;
}

const LLUUID &LLInventoryModel::getLibraryOwnerID() const
{
	return mLibraryOwnerID;
}

void LLInventoryModel::setLibraryOwnerID(const LLUUID& val)
{
	mLibraryOwnerID = val;
}

// static
BOOL LLInventoryModel::getIsFirstTimeInViewer2()
{
	// Do not call this before parentchild map is built.
	if (!gInventory.mIsAgentInvUsable)
	{
		llwarns << "Parent Child Map not yet built; guessing as first time in viewer2." << llendl;
		return TRUE;
	}

	return sFirstTimeInViewer2;
}

LLInventoryModel::item_array_t::iterator LLInventoryModel::findItemIterByUUID(LLInventoryModel::item_array_t& items, const LLUUID& id)
{
	LLInventoryModel::item_array_t::iterator curr_item = items.begin();

	while (curr_item != items.end())
	{
		if ((*curr_item)->getUUID() == id)
		{
			break;
		}
		++curr_item;
	}

	return curr_item;
}

// static
// * @param[in, out] items - vector with items to be updated. It should be sorted in a right way
// * before calling this method.
// * @param src_item_id - LLUUID of inventory item to be moved in new position
// * @param dest_item_id - LLUUID of inventory item before (or after) which source item should 
// * be placed.
// * @param insert_before - bool indicating if src_item_id should be placed before or after 
// * dest_item_id. Default is true.
void LLInventoryModel::updateItemsOrder(LLInventoryModel::item_array_t& items, const LLUUID& src_item_id, const LLUUID& dest_item_id, bool insert_before)
{
	LLInventoryModel::item_array_t::iterator it_src = findItemIterByUUID(items, src_item_id);
	LLInventoryModel::item_array_t::iterator it_dest = findItemIterByUUID(items, dest_item_id);

	// If one of the passed UUID is not in the item list, bail out
	if ((it_src == items.end()) || (it_dest == items.end())) 
		return;

	// Erase the source element from the list, keep a copy before erasing.
	LLViewerInventoryItem* src_item = *it_src;
	items.erase(it_src);
	
	// Note: Target iterator is not valid anymore because the container was changed, so update it.
	it_dest = findItemIterByUUID(items, dest_item_id);
	
	// Go to the next element if one wishes to insert after the dest element
	if (!insert_before)
	{
		++it_dest;
	}
	
	// Reinsert the source item in the right place
	if (it_dest != items.end())
	{
		items.insert(it_dest, src_item);
	}
	else 
	{
		// Append to the list if it_dest reached the end
		items.push_back(src_item);
	}
}

//* @param[in] items vector of items in order to be saved.
void LLInventoryModel::saveItemsOrder(const LLInventoryModel::item_array_t& items)
{
	int sortField = 0;

	// current order is saved by setting incremental values (1, 2, 3, ...) for the sort field
	for (item_array_t::const_iterator i = items.begin(); i != items.end(); ++i)
	{
		LLViewerInventoryItem* item = *i;

		item->setSortField(++sortField);
		item->setComplete(TRUE);
		item->updateServer(FALSE);

		updateItem(item);

		// Tell the parent folder to refresh its sort order.
		addChangedMask(LLInventoryObserver::SORT, item->getParentUUID());
	}

	notifyObservers();
}

// See also LLInventorySort where landmarks in the Favorites folder are sorted.
class LLViewerInventoryItemSort
{
public:
	bool operator()(const LLPointer<LLViewerInventoryItem>& a, const LLPointer<LLViewerInventoryItem>& b)
	{
		return a->getSortField() < b->getSortField();
	}
};

/**
 * Sorts passed items by LLViewerInventoryItem sort field.
 *
 * @param[in, out] items - array of items, not sorted.
 */
static void rearrange_item_order_by_sort_field(LLInventoryModel::item_array_t& items)
{
	static LLViewerInventoryItemSort sort_functor;
	std::sort(items.begin(), items.end(), sort_functor);
}

// * @param source_item_id - LLUUID of the source item to be moved into new position
// * @param target_item_id - LLUUID of the target item before which source item should be placed.
void LLInventoryModel::rearrangeFavoriteLandmarks(const LLUUID& source_item_id, const LLUUID& target_item_id)
{
	LLInventoryModel::cat_array_t cats;
	LLInventoryModel::item_array_t items;
	LLIsType is_type(LLAssetType::AT_LANDMARK);
	LLUUID favorites_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_FAVORITE);
	gInventory.collectDescendentsIf(favorites_id, cats, items, LLInventoryModel::EXCLUDE_TRASH, is_type);

	// ensure items are sorted properly before changing order. EXT-3498
	rearrange_item_order_by_sort_field(items);

	// update order
	updateItemsOrder(items, source_item_id, target_item_id);

	saveItemsOrder(items);
}

//----------------------------------------------------------------------------

// *NOTE: DEBUG functionality
void LLInventoryModel::dumpInventory() const
{
	llinfos << "\nBegin Inventory Dump\n**********************:" << llendl;
	llinfos << "mCategory[] contains " << mCategoryMap.size() << " items." << llendl;
	for(cat_map_t::const_iterator cit = mCategoryMap.begin(); cit != mCategoryMap.end(); ++cit)
	{
		const LLViewerInventoryCategory* cat = cit->second;
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
	for(item_map_t::const_iterator iit = mItemMap.begin(); iit != mItemMap.end(); ++iit)
	{
		const LLViewerInventoryItem* item = iit->second;
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
