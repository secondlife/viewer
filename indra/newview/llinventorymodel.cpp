/** 
 * @file llinventorymodel.cpp
 * @brief Implementation of the inventory model used to track agent inventory.
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2014, Linden Research, Inc.
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

#include <typeinfo>

#include "llinventorymodel.h"

#include "llaisapi.h"
#include "llagent.h"
#include "llagentwearables.h"
#include "llappearancemgr.h"
#include "llavatarnamecache.h"
#include "llclipboard.h"
#include "llinventorypanel.h"
#include "llinventorybridge.h"
#include "llinventoryfunctions.h"
#include "llinventoryobserver.h"
#include "llinventorypanel.h"
#include "llnotificationsutil.h"
#include "llmarketplacefunctions.h"
#include "llwindow.h"
#include "llviewercontrol.h"
#include "llviewernetwork.h"
#include "llpreview.h" 
#include "llviewermessage.h"
#include "llviewerfoldertype.h"
#include "llviewerwindow.h"
#include "llappviewer.h"
#include "llviewerregion.h"
#include "llcallbacklist.h"
#include "llvoavatarself.h"
#include "llgesturemgr.h"
#include "llsdutil.h"
#include "bufferarray.h"
#include "bufferstream.h"
#include "llcorehttputil.h"

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
static const char PRODUCTION_CACHE_FORMAT_STRING[] = "%s.inv";
static const char GRID_CACHE_FORMAT_STRING[] = "%s.%s.inv";
static const char * const LOG_INV("Inventory");

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
			S32 descendents_actual = c->getViewerDescendentCount();
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
:   // These are now ordered, keep them that way.
	mBacklinkMMap(),
	mIsAgentInvUsable(false),
	mRootFolderID(),
	mLibraryRootFolderID(),
	mLibraryOwnerID(),
	mCategoryMap(),
	mItemMap(),
	mParentChildCategoryTree(),
	mParentChildItemTree(),
	mLastItem(NULL),
	mIsNotifyObservers(FALSE),
	mModifyMask(LLInventoryObserver::ALL),
	mChangedItemIDs(),
	mObservers(),
	mHttpRequestFG(NULL),
	mHttpRequestBG(NULL),
	mHttpOptions(),
	mHttpHeaders(),
	mHttpPolicyClass(LLCore::HttpRequest::DEFAULT_POLICY_ID),
	mHttpPriorityFG(0),
	mHttpPriorityBG(0),
	mCategoryLock(),
	mItemLock()
{}


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

	// Run down HTTP transport
    mHttpHeaders.reset();
    mHttpOptions.reset();

	delete mHttpRequestFG;
	mHttpRequestFG = NULL;
	delete mHttpRequestBG;
	mHttpRequestBG = NULL;
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

bool LLInventoryModel::getObjectTopmostAncestor(const LLUUID& object_id, LLUUID& result) const
{
	LLInventoryObject *object = getObject(object_id);
	while (object && object->getParentUUID().notNull())
	{
		LLInventoryObject *parent_object = getObject(object->getParentUUID());
		if (!parent_object)
		{
			LL_WARNS(LOG_INV) << "unable to trace topmost ancestor, missing item for uuid " << object->getParentUUID() << LL_ENDL;
			return false;
		}
		object = parent_object;
	}
	result = object->getUUID();
	return true;
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

void LLInventoryModel::consolidateForType(const LLUUID& main_id, LLFolderType::EType type)
{
    // Make a list of folders that are not "main_id" and are of "type"
    std::vector<LLUUID> folder_ids;
    for (cat_map_t::iterator cit = mCategoryMap.begin(); cit != mCategoryMap.end(); ++cit)
    {
        LLViewerInventoryCategory* cat = cit->second;
        if ((cat->getPreferredType() == type) && (cat->getUUID() != main_id))
        {
            folder_ids.push_back(cat->getUUID());
        }
    }

    // Iterate through those folders
	for (std::vector<LLUUID>::iterator folder_ids_it = folder_ids.begin(); folder_ids_it != folder_ids.end(); ++folder_ids_it)
	{
		LLUUID folder_id = (*folder_ids_it);
        
        // Get the content of this folder
        cat_array_t* cats;
        item_array_t* items;
        getDirectDescendentsOf(folder_id, cats, items);
        
        // Move all items to the main folder
        // Note : we get the list of UUIDs and iterate on them instead of iterating directly on item_array_t
        // elements. This is because moving elements modify the maps and, consequently, invalidate iterators on them.
        // This "gather and iterate" method is verbose but resilient.
        std::vector<LLUUID> list_uuids;
        for (item_array_t::const_iterator it = items->begin(); it != items->end(); ++it)
        {
            list_uuids.push_back((*it)->getUUID());
        }
        for (std::vector<LLUUID>::const_iterator it = list_uuids.begin(); it != list_uuids.end(); ++it)
        {
            LLViewerInventoryItem* item = getItem(*it);
            changeItemParent(item, main_id, TRUE);
        }

        // Move all folders to the main folder
        list_uuids.clear();
        for (cat_array_t::const_iterator it = cats->begin(); it != cats->end(); ++it)
        {
            list_uuids.push_back((*it)->getUUID());
        }
        for (std::vector<LLUUID>::const_iterator it = list_uuids.begin(); it != list_uuids.end(); ++it)
        {
            LLViewerInventoryCategory* cat = getCategory(*it);
            changeCategoryParent(cat, main_id, TRUE);
        }
        
        // Purge the emptied folder
        // Note: we'd like to use purgeObject() but it doesn't cleanly eliminate the folder
        // which leads to issues further down the road when the folder is found again
        //purgeObject(folder_id);
        // We remove the folder and empty the trash instead which seems to work
		removeCategory(folder_id);
        gInventory.emptyFolderType("", LLFolderType::FT_TRASH);
	}
}

const LLUUID LLInventoryModel::findCategoryUUIDForTypeInRoot(
	LLFolderType::EType preferred_type,
	bool create_folder,
	const LLUUID& root_id)
{
	LLUUID rv = LLUUID::null;
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
			S32 count = cats->size();
			for(S32 i = 0; i < count; ++i)
			{
				if(cats->at(i)->getPreferredType() == preferred_type)
				{
					const LLUUID& folder_id = cats->at(i)->getUUID();
					if (rv.isNull() || folder_id < rv)
					{
						rv = folder_id;
					}
				}
			}
		}
	}
	
	if(rv.isNull() && isInventoryUsable() && create_folder)
	{
		if(root_id.notNull())
		{
			return createNewCategory(root_id, preferred_type, LLStringUtil::null);
		}
	}
	return rv;
}

// findCategoryUUIDForType() returns the uuid of the category that
// specifies 'type' as what it defaults to containing. The category is
// not necessarily only for that type. *NOTE: This will create a new
// inventory category on the fly if one does not exist.
const LLUUID LLInventoryModel::findCategoryUUIDForType(LLFolderType::EType preferred_type, bool create_folder)
{
	return findCategoryUUIDForTypeInRoot(preferred_type, create_folder, gInventory.getRootFolderID());
}

const LLUUID LLInventoryModel::findUserDefinedCategoryUUIDForType(LLFolderType::EType preferred_type)
{
    LLUUID cat_id;
    switch (preferred_type)
    {
    case LLFolderType::FT_OBJECT:
    {
        cat_id = LLUUID(gSavedPerAccountSettings.getString("ModelUploadFolder"));
        break;
    }
    case LLFolderType::FT_TEXTURE:
    {
        cat_id = LLUUID(gSavedPerAccountSettings.getString("TextureUploadFolder"));
        break;
    }
    case LLFolderType::FT_SOUND:
    {
        cat_id = LLUUID(gSavedPerAccountSettings.getString("SoundUploadFolder"));
        break;
    }
    case LLFolderType::FT_ANIMATION:
    {
        cat_id = LLUUID(gSavedPerAccountSettings.getString("AnimationUploadFolder"));
        break;
    }
    default:
        break;
    }
    
    if (cat_id.isNull() || !getCategory(cat_id))
    {
        cat_id = findCategoryUUIDForTypeInRoot(preferred_type, true, getRootFolderID());
    }
    return cat_id;
}

const LLUUID LLInventoryModel::findLibraryCategoryUUIDForType(LLFolderType::EType preferred_type, bool create_folder)
{
	return findCategoryUUIDForTypeInRoot(preferred_type, create_folder, gInventory.getLibraryRootFolderID());
}

// Convenience function to create a new category. You could call
// updateCategory() with a newly generated UUID category, but this
// version will take care of details like what the name should be
// based on preferred type. Returns the UUID of the new category.
LLUUID LLInventoryModel::createNewCategory(const LLUUID& parent_id,
										   LLFolderType::EType preferred_type,
										   const std::string& pname,
										   inventory_func_type callback)
{
	
	LLUUID id;
	if(!isInventoryUsable())
	{
		LL_WARNS(LOG_INV) << "Inventory is broken." << LL_ENDL;
		return id;
	}

	if(LLFolderType::lookup(preferred_type) == LLFolderType::badLookup())
	{
		LL_DEBUGS(LOG_INV) << "Attempt to create undefined category." << LL_ENDL;
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
	
	LLViewerRegion* viewer_region = gAgent.getRegion();
	std::string url;
	if ( viewer_region )
		url = viewer_region->getCapability("CreateInventoryCategory");
	
	if (!url.empty() && callback)
	{
		//Let's use the new capability.
		
		LLSD request, body;
		body["folder_id"] = id;
		body["parent_id"] = parent_id;
		body["type"] = (LLSD::Integer) preferred_type;
		body["name"] = name;
		
		request["message"] = "CreateInventoryCategory";
		request["payload"] = body;

		LL_DEBUGS(LOG_INV) << "create category request: " << ll_pretty_print_sd(request) << LL_ENDL;
        LLCoros::instance().launch("LLInventoryModel::createNewCategoryCoro",
            boost::bind(&LLInventoryModel::createNewCategoryCoro, this, url, body, callback));

		return LLUUID::null;
	}

	// Add the category to the internal representation
	LLPointer<LLViewerInventoryCategory> cat =
		new LLViewerInventoryCategory(id, parent_id, preferred_type, name, gAgent.getID());
	cat->setVersion(LLViewerInventoryCategory::VERSION_INITIAL - 1); // accountForUpdate() will icrease version by 1
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

void LLInventoryModel::createNewCategoryCoro(std::string url, LLSD postData, inventory_func_type callback)
{
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("createNewCategoryCoro", httpPolicy));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);
    LLCore::HttpOptions::ptr_t httpOpts(new LLCore::HttpOptions);
    

    httpOpts->setWantHeaders(true);

    LL_INFOS("HttpCoroutineAdapter", "genericPostCoro") << "Generic POST for " << url << LL_ENDL;

    LLSD result = httpAdapter->postAndSuspend(httpRequest, url, postData, httpOpts);

    LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    if (!status)
    {
        LL_WARNS() << "HTTP failure attempting to create category." << LL_ENDL;
        return;
    }

    if (!result.has("folder_id"))
    {
        LL_WARNS() << "Malformed response contents" << ll_pretty_print_sd(result) << LL_ENDL;
        return;
    }

    LLUUID categoryId = result["folder_id"].asUUID();

    // Add the category to the internal representation
    LLPointer<LLViewerInventoryCategory> cat = new LLViewerInventoryCategory(categoryId,
        result["parent_id"].asUUID(), (LLFolderType::EType)result["type"].asInteger(),
        result["name"].asString(), gAgent.getID());

    cat->setVersion(LLViewerInventoryCategory::VERSION_INITIAL - 1); // accountForUpdate() will icrease version by 1
    cat->setDescendentCount(0);
    LLInventoryModel::LLCategoryUpdate update(cat->getParentUUID(), 1);
    
    accountForUpdate(update);
    updateCategory(cat);

    if (callback)
    {
        callback(categoryId);
    }

}

// This is optimized for the case that we just want to know whether a
// category has any immediate children meeting a condition, without
// needing to recurse or build up any lists.
bool LLInventoryModel::hasMatchingDirectDescendent(const LLUUID& cat_id,
												   LLInventoryCollectFunctor& filter)
{
	LLInventoryModel::cat_array_t *cats;
	LLInventoryModel::item_array_t *items;
	getDirectDescendentsOf(cat_id, cats, items);
	if (cats)
	{
		for (LLInventoryModel::cat_array_t::const_iterator it = cats->begin();
			 it != cats->end(); ++it)
		{
			if (filter(*it,NULL))
			{
				return true;
			}
		}
	}
	if (items)
	{
		for (LLInventoryModel::item_array_t::const_iterator it = items->begin();
			 it != items->end(); ++it)
		{
			if (filter(NULL,*it))
			{
				return true;
			}
		}
	}
	return false;
}
												  
// Starting with the object specified, add its descendents to the
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
		const LLUUID trash_id = findCategoryUUIDForType(LLFolderType::FT_TRASH);
		if(trash_id.notNull() && (trash_id == id))
			return;
	}
	cat_array_t* cat_array = get_ptr_in_map(mParentChildCategoryTree, id);
	if(cat_array)
	{
		S32 count = cat_array->size();
		for(S32 i = 0; i < count; ++i)
		{
			LLViewerInventoryCategory* cat = cat_array->at(i);
			if(add(cat,NULL))
			{
				cats.push_back(cat);
			}
			collectDescendentsIf(cat->getUUID(), cats, items, include_trash, add);
		}
	}

	LLViewerInventoryItem* item = NULL;
	item_array_t* item_array = get_ptr_in_map(mParentChildItemTree, id);

	// Move onto items
	if(item_array)
	{
		S32 count = item_array->size();
		for(S32 i = 0; i < count; ++i)
		{
			item = item_array->at(i);
			if(add(NULL, item))
			{
				items.push_back(item);
			}
		}
	}
}

U32 LLInventoryModel::getDescendentsCountRecursive(const LLUUID& id, U32 max_item_limit)
{
	LLInventoryModel::cat_array_t cats;
	LLInventoryModel::item_array_t items;
	gInventory.collectDescendents(id, cats, items, LLInventoryModel::INCLUDE_TRASH);

	U32 items_found = items.size() + cats.size();

	for (U32 i = 0; i < cats.size() && items_found <= max_item_limit; ++i)
	{
		items_found += getDescendentsCountRecursive(cats[i]->getUUID(), max_item_limit - items_found);
	}

	return items_found;
}

void LLInventoryModel::addChangedMaskForLinks(const LLUUID& object_id, U32 mask)
{
	const LLInventoryObject *obj = getObject(object_id);
	if (!obj || obj->getIsLinkType())
		return;

	LLInventoryModel::item_array_t item_array = collectLinksTo(object_id);
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

LLInventoryModel::item_array_t LLInventoryModel::collectLinksTo(const LLUUID& id)
{
	// Get item list via collectDescendents (slow!)
	item_array_t items;
	const LLInventoryObject *obj = getObject(id);
	// FIXME - should be as in next line, but this is causing a
	// stack-smashing crash of cause TBD... check in the REBUILD code.
	//if (obj && obj->getIsLinkType())
	if (!obj || obj->getIsLinkType())
		return items;
	
	std::pair<backlink_mmap_t::iterator, backlink_mmap_t::iterator> range = mBacklinkMMap.equal_range(id);
	for (backlink_mmap_t::iterator it = range.first; it != range.second; ++it)
	{
		LLViewerInventoryItem *item = getItem(it->second);
		if (item)
		{
			items.push_back(item);
		}
	}

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
U32 LLInventoryModel::updateItem(const LLViewerInventoryItem* item, U32 mask)
{
	if(item->getUUID().isNull())
	{
		return mask;
	}

	if(!isInventoryUsable())
	{
		LL_WARNS(LOG_INV) << "Inventory is broken." << LL_ENDL;
		return mask;
	}

	// We're hiding mesh types
#if 0
	if (item->getType() == LLAssetType::AT_MESH)
	{
		return mask;
	}
#endif

	LLPointer<LLViewerInventoryItem> old_item = getItem(item->getUUID());
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
				vector_replace_with_last(*item_array, old_item);
			}
			item_array = get_ptr_in_map(mParentChildItemTree, new_parent_id);
			if(item_array)
			{
				item_array->push_back(old_item);
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
				LLInventoryModel::LLCategoryUpdate update(category_id, 1);
				gInventory.accountForUpdate(update);

				// *FIX: bit of a hack to call update server from here...
				new_item->updateParentOnServer(FALSE);
				item_array->push_back(new_item);
			}
			else
			{
				LL_WARNS(LOG_INV) << "Couldn't find parent-child item tree for " << new_item->getName() << LL_ENDL;
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
				item_array->push_back(new_item);
			}
			else
			{
				// Whoops! No such parent, make one.
				LL_INFOS(LOG_INV) << "Lost item: " << new_item->getUUID() << " - "
								  << new_item->getName() << LL_ENDL;
				parent_id = findCategoryUUIDForType(LLFolderType::FT_LOST_AND_FOUND);
				new_item->setParent(parent_id);
				item_array = get_ptr_in_map(mParentChildItemTree, parent_id);
				if(item_array)
				{
					LLInventoryModel::LLCategoryUpdate update(parent_id, 1);
					gInventory.accountForUpdate(update);
					// *FIX: bit of a hack to call update server from
					// here...
					new_item->updateParentOnServer(FALSE);
					item_array->push_back(new_item);
				}
				else
				{
					LL_WARNS(LOG_INV) << "Lost and found Not there!!" << LL_ENDL;
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
			LLAvatarName av_name;

			if (LLAvatarNameCache::get(id, &av_name))
			{
				new_item->rename(av_name.getUserName());
				mask |= LLInventoryObserver::LABEL;
			}
			else
			{
				// Fetch the current name
				LLAvatarNameCache::get(id,
					boost::bind(&LLViewerInventoryItem::onCallingCardNameLookup, new_item.get(),
					_1, _2));
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
void LLInventoryModel::updateCategory(const LLViewerInventoryCategory* cat, U32 mask)
{
	if(!cat || cat->getUUID().isNull())
	{
		return;
	}

	if(!isInventoryUsable())
	{
		LL_WARNS(LOG_INV) << "Inventory is broken." << LL_ENDL;
		return;
	}

	LLPointer<LLViewerInventoryCategory> old_cat = getCategory(cat->getUUID());
	if(old_cat)
	{
		// We already have an old category, modify its values
		LLUUID old_parent_id = old_cat->getParentUUID();
		LLUUID new_parent_id = cat->getParentUUID();
		if(old_parent_id != new_parent_id)
		{
			// need to update the parent-child tree
			cat_array_t* cat_array;
			cat_array = getUnlockedCatArray(old_parent_id);
			if(cat_array)
			{
				vector_replace_with_last(*cat_array, old_cat);
			}
			cat_array = getUnlockedCatArray(new_parent_id);
			if(cat_array)
			{
				cat_array->push_back(old_cat);
			}
			mask |= LLInventoryObserver::STRUCTURE;
            mask |= LLInventoryObserver::INTERNAL;
		}
		if(old_cat->getName() != cat->getName())
		{
			mask |= LLInventoryObserver::LABEL;
		}
        // Under marketplace, category labels are quite complex and need extra upate
        const LLUUID marketplace_id = findCategoryUUIDForType(LLFolderType::FT_MARKETPLACE_LISTINGS, false);
        if (marketplace_id.notNull() && isObjectDescendentOf(cat->getUUID(), marketplace_id))
        {
			mask |= LLInventoryObserver::LABEL;
        }
        old_cat->copyViewerCategory(cat);
		addChangedMask(mask, cat->getUUID());
	}
	else
	{
		// add this category
		LLPointer<LLViewerInventoryCategory> new_cat = new LLViewerInventoryCategory(cat->getOwnerID());
		new_cat->copyViewerCategory(cat);
		addCategory(new_cat);

		// make sure this category is correctly referenced by its parent.
		cat_array_t* cat_array;
		cat_array = getUnlockedCatArray(cat->getParentUUID());
		if(cat_array)
		{
			cat_array->push_back(new_cat);
		}

		// make space in the tree for this category's children.
		llassert_always(mCategoryLock[new_cat->getUUID()] == false);
		llassert_always(mItemLock[new_cat->getUUID()] == false);
		cat_array_t* catsp = new cat_array_t;
		item_array_t* itemsp = new item_array_t;
		mParentChildCategoryTree[new_cat->getUUID()] = catsp;
		mParentChildItemTree[new_cat->getUUID()] = itemsp;
		mask |= LLInventoryObserver::ADD;
		addChangedMask(mask, cat->getUUID());
	}
}

void LLInventoryModel::moveObject(const LLUUID& object_id, const LLUUID& cat_id)
{
	LL_DEBUGS(LOG_INV) << "LLInventoryModel::moveObject()" << LL_ENDL;
	if(!isInventoryUsable())
	{
		LL_WARNS(LOG_INV) << "Inventory is broken." << LL_ENDL;
		return;
	}

	if((object_id == cat_id) || !is_in_map(mCategoryMap, cat_id))
	{
		LL_WARNS(LOG_INV) << "Could not move inventory object " << object_id << " to "
						  << cat_id << LL_ENDL;
		return;
	}
	LLPointer<LLViewerInventoryCategory> cat = getCategory(object_id);
	if(cat && (cat->getParentUUID() != cat_id))
	{
		cat_array_t* cat_array;
		cat_array = getUnlockedCatArray(cat->getParentUUID());
		if(cat_array) vector_replace_with_last(*cat_array, cat);
		cat_array = getUnlockedCatArray(cat_id);
		cat->setParent(cat_id);
		if(cat_array) cat_array->push_back(cat);
		addChangedMask(LLInventoryObserver::STRUCTURE, object_id);
		return;
	}
	LLPointer<LLViewerInventoryItem> item = getItem(object_id);
	if(item && (item->getParentUUID() != cat_id))
	{
		item_array_t* item_array;
		item_array = getUnlockedItemArray(item->getParentUUID());
		if(item_array) vector_replace_with_last(*item_array, item);
		item_array = getUnlockedItemArray(cat_id);
		item->setParent(cat_id);
		if(item_array) item_array->push_back(item);
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
		LL_DEBUGS(LOG_INV) << "'" << item->getName() << "' (" << item->getUUID()
						   << ") is already in folder " << new_parent_id << LL_ENDL;
	}
	else
	{
		LL_INFOS(LOG_INV) << "Moving '" << item->getName() << "' (" << item->getUUID()
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

void LLInventoryModel::onAISUpdateReceived(const std::string& context, const LLSD& update)
{
	LLTimer timer;
	if (gSavedSettings.getBOOL("DebugAvatarAppearanceMessage"))
	{
		dump_sequential_xml(gAgentAvatarp->getFullname() + "_ais_update", update);
	}

	AISUpdate ais_update(update); // parse update llsd into stuff to do.
	ais_update.doUpdate(); // execute the updates in the appropriate order.
	LL_INFOS(LOG_INV) << "elapsed: " << timer.getElapsedTimeF32() << LL_ENDL;
}

// Does not appear to be used currently.
void LLInventoryModel::onItemUpdated(const LLUUID& item_id, const LLSD& updates, bool update_parent_version)
{
	U32 mask = LLInventoryObserver::NONE;

	LLPointer<LLViewerInventoryItem> item = gInventory.getItem(item_id);
	LL_DEBUGS(LOG_INV) << "item_id: [" << item_id << "] name " << (item ? item->getName() : "(NOT FOUND)") << LL_ENDL;
	if(item)
	{
		for (LLSD::map_const_iterator it = updates.beginMap();
			 it != updates.endMap(); ++it)
		{
			if (it->first == "name")
			{
				LL_INFOS(LOG_INV) << "Updating name from " << item->getName() << " to " << it->second.asString() << LL_ENDL;
				item->rename(it->second.asString());
				mask |= LLInventoryObserver::LABEL;
			}
			else if (it->first == "desc")
			{
				LL_INFOS(LOG_INV) << "Updating description from " << item->getActualDescription()
								  << " to " << it->second.asString() << LL_ENDL;
				item->setDescription(it->second.asString());
			}
			else
			{
				LL_ERRS(LOG_INV) << "unhandled updates for field: " << it->first << LL_ENDL;
			}
		}
		mask |= LLInventoryObserver::INTERNAL;
		addChangedMask(mask, item->getUUID());
		if (update_parent_version)
		{
			// Descendent count is unchanged, but folder version incremented.
			LLInventoryModel::LLCategoryUpdate up(item->getParentUUID(), 0);
			accountForUpdate(up);
		}
		notifyObservers(); // do we want to be able to make this optional?
	}
}

// Not used?
void LLInventoryModel::onCategoryUpdated(const LLUUID& cat_id, const LLSD& updates)
{
	U32 mask = LLInventoryObserver::NONE;

	LLPointer<LLViewerInventoryCategory> cat = gInventory.getCategory(cat_id);
	LL_DEBUGS(LOG_INV) << "cat_id: [" << cat_id << "] name " << (cat ? cat->getName() : "(NOT FOUND)") << LL_ENDL;
	if(cat)
	{
		for (LLSD::map_const_iterator it = updates.beginMap();
			 it != updates.endMap(); ++it)
		{
			if (it->first == "name")
			{
				LL_INFOS(LOG_INV) << "Updating name from " << cat->getName() << " to " << it->second.asString() << LL_ENDL;
				cat->rename(it->second.asString());
				mask |= LLInventoryObserver::LABEL;
			}
			else
			{
				LL_ERRS(LOG_INV) << "unhandled updates for field: " << it->first << LL_ENDL;
			}
		}
		mask |= LLInventoryObserver::INTERNAL;
		addChangedMask(mask, cat->getUUID());
		notifyObservers(); // do we want to be able to make this optional?
	}
}

// Update model after descendents have been purged.
void LLInventoryModel::onDescendentsPurgedFromServer(const LLUUID& object_id, bool fix_broken_links)
{
	LLPointer<LLViewerInventoryCategory> cat = getCategory(object_id);
	if (cat.notNull())
	{
		// do the cache accounting
		S32 descendents = cat->getDescendentCount();
		if(descendents > 0)
		{
			LLInventoryModel::LLCategoryUpdate up(object_id, -descendents);
			accountForUpdate(up);
		}

		// we know that descendent count is 0, however since the
		// accounting may actually not do an update, we should force
		// it here.
		cat->setDescendentCount(0);

		// unceremoniously remove anything we have locally stored.
		LLInventoryModel::cat_array_t categories;
		LLInventoryModel::item_array_t items;
		collectDescendents(object_id,
						   categories,
						   items,
						   LLInventoryModel::INCLUDE_TRASH);
		S32 count = items.size();

		LLUUID uu_id;
		for(S32 i = 0; i < count; ++i)
		{
			uu_id = items.at(i)->getUUID();

			// This check prevents the deletion of a previously deleted item.
			// This is necessary because deletion is not done in a hierarchical
			// order. The current item may have been already deleted as a child
			// of its deleted parent.
			if (getItem(uu_id))
			{
				deleteObject(uu_id, fix_broken_links);
			}
		}

		count = categories.size();
		// Slightly kludgy way to make sure categories are removed
		// only after their child categories have gone away.

		// FIXME: Would probably make more sense to have this whole
		// descendent-clearing thing be a post-order recursive
		// function to get the leaf-up behavior automatically.
		S32 deleted_count;
		S32 total_deleted_count = 0;
		do
		{
			deleted_count = 0;
			for(S32 i = 0; i < count; ++i)
			{
				uu_id = categories.at(i)->getUUID();
				if (getCategory(uu_id))
				{
					cat_array_t* cat_list = getUnlockedCatArray(uu_id);
					if (!cat_list || (cat_list->size() == 0))
					{
						deleteObject(uu_id, fix_broken_links);
						deleted_count++;
					}
				}
			}
			total_deleted_count += deleted_count;
		}
		while (deleted_count > 0);
		if (total_deleted_count != count)
		{
			LL_WARNS(LOG_INV) << "Unexpected count of categories deleted, got "
							  << total_deleted_count << " expected " << count << LL_ENDL;
		}
		//gInventory.validate();
	}
}

// Update model after an item is confirmed as removed from
// server. Works for categories or items.
void LLInventoryModel::onObjectDeletedFromServer(const LLUUID& object_id, bool fix_broken_links, bool update_parent_version, bool do_notify_observers)
{
	LLPointer<LLInventoryObject> obj = getObject(object_id);
	if(obj)
	{
		if (getCategory(object_id))
		{
			// For category, need to delete/update all children first.
			onDescendentsPurgedFromServer(object_id, fix_broken_links);
		}


		// From item/cat removeFromServer()
		if (update_parent_version)
		{
			LLInventoryModel::LLCategoryUpdate up(obj->getParentUUID(), -1);
			accountForUpdate(up);
		}

		// From purgeObject()
		LLViewerInventoryItem *item = getItem(object_id);
		if (item && (item->getType() != LLAssetType::AT_LSL_TEXT))
		{
			LLPreview::hide(object_id, TRUE);
		}
		deleteObject(object_id, fix_broken_links, do_notify_observers);
	}
}


// Delete a particular inventory object by ID.
void LLInventoryModel::deleteObject(const LLUUID& id, bool fix_broken_links, bool do_notify_observers)
{
	LL_DEBUGS(LOG_INV) << "LLInventoryModel::deleteObject()" << LL_ENDL;
	LLPointer<LLInventoryObject> obj = getObject(id);
	if (!obj) 
	{
		LL_WARNS(LOG_INV) << "Deleting non-existent object [ id: " << id << " ] " << LL_ENDL;
		return;
	}
	
	LL_DEBUGS(LOG_INV) << "Deleting inventory object " << id << LL_ENDL;
	mLastItem = NULL;
	LLUUID parent_id = obj->getParentUUID();
	mCategoryMap.erase(id);
	mItemMap.erase(id);
	//mInventory.erase(id);
	item_array_t* item_list = getUnlockedItemArray(parent_id);
	if(item_list)
	{
		LLPointer<LLViewerInventoryItem> item = (LLViewerInventoryItem*)((LLInventoryObject*)obj);
		vector_replace_with_last(*item_list, item);
	}
	cat_array_t* cat_list = getUnlockedCatArray(parent_id);
	if(cat_list)
	{
		LLPointer<LLViewerInventoryCategory> cat = (LLViewerInventoryCategory*)((LLInventoryObject*)obj);
		vector_replace_with_last(*cat_list, cat);
	}
    
    // Note : We need to tell the inventory observers that those things are going to be deleted *before* the tree is cleared or they won't know what to delete (in views and view models)
	addChangedMask(LLInventoryObserver::REMOVE, id);
	gInventory.notifyObservers();
    
	item_list = getUnlockedItemArray(id);
	if(item_list)
	{
		if (item_list->size())
		{
			LL_WARNS(LOG_INV) << "Deleting cat " << id << " while it still has child items" << LL_ENDL;
		}
		delete item_list;
		mParentChildItemTree.erase(id);
	}
	cat_list = getUnlockedCatArray(id);
	if(cat_list)
	{
		if (cat_list->size())
		{
			LL_WARNS(LOG_INV) << "Deleting cat " << id << " while it still has child cats" << LL_ENDL;
		}
		delete cat_list;
		mParentChildCategoryTree.erase(id);
	}
	addChangedMask(LLInventoryObserver::REMOVE, id);

	bool is_link_type = obj->getIsLinkType();
	if (is_link_type)
	{
		removeBacklinkInfo(obj->getUUID(), obj->getLinkedUUID());
	}

	// Can't have links to links, so there's no need for this update
	// if the item removed is a link. Can also skip if source of the
	// update is getting broken link info separately.
	obj = NULL; // delete obj
	if (fix_broken_links && !is_link_type)
	{
		updateLinkedObjectsFromPurge(id);
	}
	if (do_notify_observers)
	{
		notifyObservers();
	}
}

void LLInventoryModel::updateLinkedObjectsFromPurge(const LLUUID &baseobj_id)
{
	LLInventoryModel::item_array_t item_array = collectLinksTo(baseobj_id);

	// REBUILD is expensive, so clear the current change list first else
	// everything else on the changelist will also get rebuilt.
	if (item_array.size() > 0)
	{
		notifyObservers();
		for (LLInventoryModel::item_array_t::const_iterator iter = item_array.begin();
			iter != item_array.end();
			iter++)
		{
			const LLViewerInventoryItem *linked_item = (*iter);
			const LLUUID &item_id = linked_item->getUUID();
			if (item_id == baseobj_id) continue;
			addChangedMask(LLInventoryObserver::REBUILD, item_id);
		}
		notifyObservers();
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
	// *FIX:  Think I want this conditional or moved elsewhere...
	handleResponses(true);
	
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
		LL_WARNS(LOG_INV) << "Call was made to notifyObservers within notifyObservers!" << LL_ENDL;
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
	mAddedItemIDs.clear();
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
		LL_WARNS(LOG_INV) << "Adding changed mask within notify observers!  Change will likely be lost." << LL_ENDL;
		LLViewerInventoryItem *item = getItem(referent);
		if (item)
		{
			LL_WARNS(LOG_INV) << "Item " << item->getName() << LL_ENDL;
		}
		else
		{
			LLViewerInventoryCategory *cat = getCategory(referent);
			if (cat)
			{
				LL_WARNS(LOG_INV) << "Category " << cat->getName() << LL_ENDL;
			}
		}
	}
	
	mModifyMask |= mask;
	if (referent.notNull() && (mChangedItemIDs.find(referent) == mChangedItemIDs.end()))
	{
		mChangedItemIDs.insert(referent);
        update_marketplace_category(referent, false);

		if (mask & LLInventoryObserver::ADD)
		{
			mAddedItemIDs.insert(referent);
		}
	
		// Update all linked items.  Starting with just LABEL because I'm
		// not sure what else might need to be accounted for this.
		if (mask & LLInventoryObserver::LABEL)
		{
			addChangedMaskForLinks(referent, LLInventoryObserver::LABEL);
		}
	}
}

bool LLInventoryModel::fetchDescendentsOf(const LLUUID& folder_id) const
{
	if(folder_id.isNull()) 
	{
		LL_WARNS(LOG_INV) << "Calling fetch descendents on NULL folder id!" << LL_ENDL;
		return false;
	}
	LLViewerInventoryCategory* cat = getCategory(folder_id);
	if(!cat)
	{
		LL_WARNS(LOG_INV) << "Asked to fetch descendents of non-existent folder: "
						  << folder_id << LL_ENDL;
		return false;
	}
	//S32 known_descendents = 0;
	///cat_array_t* categories = get_ptr_in_map(mParentChildCategoryTree, folder_id);
	//item_array_t* items = get_ptr_in_map(mParentChildItemTree, folder_id);
	//if(categories)
	//{
	//	known_descendents += categories->size();
	//}
	//if(items)
	//{
	//	known_descendents += items->size();
	//}
	return cat->fetch();
}

//static
std::string LLInventoryModel::getInvCacheAddres(const LLUUID& owner_id)
{
    std::string inventory_addr;
    std::string owner_id_str;
    owner_id.toString(owner_id_str);
    std::string path(gDirUtilp->getExpandedFilename(LL_PATH_CACHE, owner_id_str));
    if (LLGridManager::getInstance()->isInProductionGrid())
    {
        inventory_addr = llformat(PRODUCTION_CACHE_FORMAT_STRING, path.c_str());
    }
    else
    {
        // NOTE: The inventory cache filenames now include the grid name.
        // Add controls against directory traversal or problematic pathname lengths
        // if your viewer uses grid names from an untrusted source.
        const std::string& grid_id_str = LLGridManager::getInstance()->getGridId();
        const std::string& grid_id_lower = utf8str_tolower(grid_id_str);
        inventory_addr = llformat(GRID_CACHE_FORMAT_STRING, path.c_str(), grid_id_lower.c_str());
    }
    return inventory_addr;
}

void LLInventoryModel::cache(
	const LLUUID& parent_folder_id,
	const LLUUID& agent_id)
{
	LL_DEBUGS(LOG_INV) << "Caching " << parent_folder_id << " for " << agent_id
					   << LL_ENDL;
	LLViewerInventoryCategory* root_cat = getCategory(parent_folder_id);
	if(!root_cat) return;
	cat_array_t categories;
	categories.push_back(root_cat);
	item_array_t items;

	LLCanCache can_cache(this);
	can_cache(root_cat, NULL);
	collectDescendentsIf(
		parent_folder_id,
		categories,
		items,
		INCLUDE_TRASH,
		can_cache);
	std::string inventory_filename = getInvCacheAddres(agent_id);
	saveToFile(inventory_filename, categories, items);
	std::string gzip_filename(inventory_filename);
	gzip_filename.append(".gz");
	if(gzip_file(inventory_filename, gzip_filename))
	{
		LL_DEBUGS(LOG_INV) << "Successfully compressed " << inventory_filename << LL_ENDL;
		LLFile::remove(inventory_filename);
	}
	else
	{
		LL_WARNS(LOG_INV) << "Unable to compress " << inventory_filename << LL_ENDL;
	}
}


void LLInventoryModel::addCategory(LLViewerInventoryCategory* category)
{
	//LL_INFOS(LOG_INV) << "LLInventoryModel::addCategory()" << LL_ENDL;
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

bool LLInventoryModel::hasBacklinkInfo(const LLUUID& link_id, const LLUUID& target_id) const
{
	std::pair <backlink_mmap_t::const_iterator, backlink_mmap_t::const_iterator> range;
	range = mBacklinkMMap.equal_range(target_id);
	for (backlink_mmap_t::const_iterator it = range.first; it != range.second; ++it)
	{
		if (it->second == link_id)
		{
			return true;
		}
	}
	return false;
}

void LLInventoryModel::addBacklinkInfo(const LLUUID& link_id, const LLUUID& target_id)
{
	if (!hasBacklinkInfo(link_id, target_id))
	{
		mBacklinkMMap.insert(std::make_pair(target_id, link_id));
	}
}

void LLInventoryModel::removeBacklinkInfo(const LLUUID& link_id, const LLUUID& target_id)
{
	std::pair <backlink_mmap_t::iterator, backlink_mmap_t::iterator> range;
	range = mBacklinkMMap.equal_range(target_id);
	for (backlink_mmap_t::iterator it = range.first; it != range.second; )
	{
		if (it->second == link_id)
		{
			backlink_mmap_t::iterator delete_it = it; // iterator will be invalidated by erase.
			++it;
			mBacklinkMMap.erase(delete_it);
		}
		else
		{
			++it;
		}
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
			LL_WARNS(LOG_INV) << "Got bad asset type for item [ name: " << item->getName()
							  << " type: " << item->getType()
							  << " inv-type: " << item->getInventoryType() << " ], ignoring." << LL_ENDL;
			return;
		}

		// This condition means that we tried to add a link without the baseobj being in memory.
		// The item will show up as a broken link.
		if (item->getIsBrokenLink())
		{
			LL_INFOS(LOG_INV) << "Adding broken link [ name: " << item->getName()
							  << " itemID: " << item->getUUID()
							  << " assetID: " << item->getAssetUUID() << " )  parent: " << item->getParentUUID() << LL_ENDL;
		}
		if (item->getIsLinkType())
		{
			// Add back-link from linked-to UUID.
			const LLUUID& link_id = item->getUUID();
			const LLUUID& target_id = item->getLinkedUUID();
			addBacklinkInfo(link_id, target_id);
		}
		mItemMap[item->getUUID()] = item;
	}
}

// Empty the entire contents
void LLInventoryModel::empty()
{
//	LL_INFOS(LOG_INV) << "LLInventoryModel::empty()" << LL_ENDL;
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
	mBacklinkMMap.clear(); // forget all backlink information.
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
		S32 version = cat->getVersion();
		if(version != LLViewerInventoryCategory::VERSION_UNKNOWN)
		{
			S32 descendents_server = cat->getDescendentCount();
			S32 descendents_actual = cat->getViewerDescendentCount();
			if(descendents_server == descendents_actual)
			{
				descendents_actual += update.mDescendentDelta;
				cat->setDescendentCount(descendents_actual);
				cat->setVersion(++version);
				LL_DEBUGS(LOG_INV) << "accounted: '" << cat->getName() << "' "
								   << version << " with " << descendents_actual
								   << " descendents." << LL_ENDL;
			}
			else
			{
				// Error condition, this means that the category did not register that
				// it got new descendents (perhaps because it is still being loaded)
				// which means its descendent count will be wrong.
				LL_WARNS(LOG_INV) << "Accounting failed for '" << cat->getName() << "' version:"
								  << version << " due to mismatched descendent count:  server == "
								  << descendents_server << ", viewer == " << descendents_actual << LL_ENDL;
			}
		}
		else
		{
			LL_WARNS(LOG_INV) << "Accounting failed for '" << cat->getName() << "' version: unknown (" 
							  << version << ")" << LL_ENDL;
		}
	}
	else
	{
		LL_WARNS(LOG_INV) << "No category found for update " << update.mCategoryID << LL_ENDL;
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
	if (cat_it != mParentChildCategoryTree.end() && cat_it->second->size() > 0)
	{
		return CHILDREN_YES;
	}
	parent_item_map_t::const_iterator item_it = mParentChildItemTree.find(cat->getUUID());
	if (item_it != mParentChildItemTree.end() && item_it->second->size() > 0)
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
		S32 descendents_actual = cat->getViewerDescendentCount();
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
	LL_DEBUGS(LOG_INV) << "importing inventory skeleton for " << owner_id << LL_ENDL;

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
			LL_WARNS(LOG_INV) << "Unable to import near " << name.asString() << LL_ENDL;
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
		std::string inventory_filename = getInvCacheAddres(owner_id);
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
				LL_INFOS(LOG_INV) << "Unable to gunzip " << gzip_filename << LL_ENDL;
			}
		}
		bool is_cache_obsolete = false;
		if(loadFromFile(inventory_filename, categories, items, is_cache_obsolete))
		{
			// We were able to find a cache of files. So, use what we
			// found to generate a set of categories we should add. We
			// will go through each category loaded and if the version
			// does not match, invalidate the version.
			S32 count = categories.size();
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
				if (cit == not_cached)
				{
					continue;
				}
				else if (cat->getVersion() != tcat->getVersion())
				{
					// if the cached version does not match the server version,
					// throw away the version we have so we can fetch the
					// correct contents the next time the viewer opens the folder.
					tcat->setVersion(NO_VERSION);
				}
                else if (tcat->getPreferredType() == LLFolderType::FT_MARKETPLACE_STOCK)
                {
                    // Do not trust stock folders being updated
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
							LL_DEBUGS(LOG_INV) << "Attempted to add cached link item without baseobj present ( name: "
											   << item->getName() << " itemID: " << item->getUUID()
											   << " assetID: " << item->getAssetUUID()
											   << " ).  Ignoring and invalidating " << cat->getName() << " . " << LL_ENDL;
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
						//LL_INFOS(LOG_INV) << "link still broken: " << item->getName() << " in folder " << cat->getName() << LL_ENDL;
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

 				LL_INFOS(LOG_INV) << "Attempted to add " << bad_link_count
								  << " cached link items without baseobj present. "
								  << good_link_count << " link items were successfully added. "
								  << recovered_link_count << " links added in recovery. "
								  << "The corresponding categories were invalidated." << LL_ENDL;
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
			LL_DEBUGS(LOG_INV) << "Invalidating category name: " << cat->getName() << " UUID: " << cat->getUUID() << " due to invalid descendents cache" << LL_ENDL;
		}
		LL_INFOS(LOG_INV) << "Invalidated " << invalid_categories.size() << " categories due to invalid descendents cache" << LL_ENDL;

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
			LL_WARNS(LOG_INV) << "Inv cache out of date, removing" << LL_ENDL;
			LLFile::remove(gzip_filename);
		}
		categories.clear(); // will unref and delete entries
	}

	LL_INFOS(LOG_INV) << "Successfully loaded " << cached_category_count
					  << " categories and " << cached_item_count << " items from cache."
					  << LL_ENDL;

	return rv;
}

// This is a brute force method to rebuild the entire parent-child
// relations. The overall operation has O(NlogN) performance, which
// should be sufficient for our needs. 
void LLInventoryModel::buildParentChildMap()
{
	LL_INFOS(LOG_INV) << "LLInventoryModel::buildParentChildMap()" << LL_ENDL;

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
		cats.push_back(cat);
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
	S32 count = cats.size();
	S32 i;
	S32 lost = 0;
	cat_array_t lost_cats;
	for(i = 0; i < count; ++i)
	{
		LLViewerInventoryCategory* cat = cats.at(i);
		catsp = getUnlockedCatArray(cat->getParentUUID());
		if(catsp &&
		   // Only the two root folders should be children of null.
		   // Others should go to lost & found.
		   (cat->getParentUUID().notNull() || 
			cat->getPreferredType() == LLFolderType::FT_ROOT_INVENTORY ))
		{
			catsp->push_back(cat);
		}
		else
		{
			// *NOTE: This process could be a lot more efficient if we
			// used the new MoveInventoryFolder message, but we would
			// have to continue to do the update & build here. So, to
			// implement it, we would need a set or map of uuid pairs
			// which would be (folder_id, new_parent_id) to be sent up
			// to the server.
			LL_INFOS(LOG_INV) << "Lost category: " << cat->getUUID() << " - "
							  << cat->getName() << LL_ENDL;
			++lost;
			lost_cats.push_back(cat);
		}
	}
	if(lost)
	{
		LL_WARNS(LOG_INV) << "Found  " << lost << " lost categories." << LL_ENDL;
	}

	// Do moves in a separate pass to make sure we've properly filed
	// the FT_LOST_AND_FOUND category before we try to find its UUID.
	for(i = 0; i<lost_cats.size(); ++i)
	{
		LLViewerInventoryCategory *cat = lost_cats.at(i);

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
		// FIXME note that updateServer() fails with protected
		// types, so this will not work as intended in that case.
		cat->updateServer(TRUE);
		catsp = getUnlockedCatArray(cat->getParentUUID());
		if(catsp)
		{
			catsp->push_back(cat);
		}
		else
		{		
			LL_WARNS(LOG_INV) << "Lost and found Not there!!" << LL_ENDL;
		}
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
			items.push_back(item);
		}
	}
	count = items.size();
	lost = 0;
	uuid_vec_t lost_item_ids;
	for(i = 0; i < count; ++i)
	{
		LLPointer<LLViewerInventoryItem> item;
		item = items.at(i);
		itemsp = getUnlockedItemArray(item->getParentUUID());
		if(itemsp)
		{
			itemsp->push_back(item);
		}
		else
		{
			LL_INFOS(LOG_INV) << "Lost item: " << item->getUUID() << " - "
							  << item->getName() << LL_ENDL;
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
				itemsp->push_back(item);
			}
			else
			{
				LL_WARNS(LOG_INV) << "Lost and found Not there!!" << LL_ENDL;
			}
		}
	}
	if(lost)
	{
		LL_WARNS(LOG_INV) << "Found " << lost << " lost items." << LL_ENDL;
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

			// notifyObservers() has been moved to
			// llstartup/idle_startup() after this func completes.
			// Allows some system categories to be created before
			// observers start firing.
		}
	}

	if (!gInventory.validate())
	{
	 	LL_WARNS(LOG_INV) << "model failed validity check!" << LL_ENDL;
	}
}

// Would normally do this at construction but that's too early
// in the process for gInventory.  Have the first requestPost()
// call set things up.
void LLInventoryModel::initHttpRequest()
{
	if (! mHttpRequestFG)
	{
		// Haven't initialized, get to it
		LLAppCoreHttp & app_core_http(LLAppViewer::instance()->getAppCoreHttp());

		mHttpRequestFG = new LLCore::HttpRequest;
		mHttpRequestBG = new LLCore::HttpRequest;
		mHttpOptions = LLCore::HttpOptions::ptr_t(new LLCore::HttpOptions);
		mHttpOptions->setTransferTimeout(300);
		mHttpOptions->setUseRetryAfter(true);
		// mHttpOptions->setTrace(2);		// Do tracing of requests
        mHttpHeaders = LLCore::HttpHeaders::ptr_t(new LLCore::HttpHeaders);
		mHttpHeaders->append(HTTP_OUT_HEADER_CONTENT_TYPE, HTTP_CONTENT_LLSD_XML);
		mHttpHeaders->append(HTTP_OUT_HEADER_ACCEPT, HTTP_CONTENT_LLSD_XML);
		mHttpPolicyClass = app_core_http.getPolicy(LLAppCoreHttp::AP_INVENTORY);
	}
}

void LLInventoryModel::handleResponses(bool foreground)
{
	if (foreground && mHttpRequestFG)
	{
		mHttpRequestFG->update(0);
	}
	else if (! foreground && mHttpRequestBG)
	{
		mHttpRequestBG->update(50000L);
	}
}

LLCore::HttpHandle LLInventoryModel::requestPost(bool foreground,
												 const std::string & url,
												 const LLSD & body,
												 const LLCore::HttpHandler::ptr_t &handler,
												 const char * const message)
{
	if (! mHttpRequestFG)
	{
		// We do the initialization late and lazily as this class is
		// statically-constructed and not all the bits are ready at
		// that time.
		initHttpRequest();
	}
	
	LLCore::HttpRequest * request(foreground ? mHttpRequestFG : mHttpRequestBG);
	LLCore::HttpHandle handle(LLCORE_HTTP_HANDLE_INVALID);
		
	handle = LLCoreHttpUtil::requestPostWithLLSD(request,
												 mHttpPolicyClass,
												 (foreground ? mHttpPriorityFG : mHttpPriorityBG),
												 url,
												 body,
												 mHttpOptions,
												 mHttpHeaders,
												 handler);
	if (LLCORE_HTTP_HANDLE_INVALID == handle)
	{
		LLCore::HttpStatus status(request->getStatus());
		LL_WARNS(LOG_INV) << "HTTP POST request failed for " << message
						  << ", Status: " << status.toTerseString()
						  << " Reason: '" << status.toString() << "'"
						  << LL_ENDL;
	}
	return handle;
}

void LLInventoryModel::createCommonSystemCategories()
{
	gInventory.findCategoryUUIDForType(LLFolderType::FT_TRASH,true);
	gInventory.findCategoryUUIDForType(LLFolderType::FT_FAVORITE,true);
	gInventory.findCategoryUUIDForType(LLFolderType::FT_CALLINGCARD,true);
	gInventory.findCategoryUUIDForType(LLFolderType::FT_MY_OUTFITS,true);
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
		LL_ERRS(LOG_INV) << "Filename is Null!" << LL_ENDL;
		return false;
	}
	LL_INFOS(LOG_INV) << "LLInventoryModel::loadFromFile(" << filename << ")" << LL_ENDL;
	LLFILE* file = LLFile::fopen(filename, "rb");		/*Flawfinder: ignore*/
	if(!file)
	{
		LL_INFOS(LOG_INV) << "unable to load inventory from: " << filename << LL_ENDL;
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
				categories.push_back(inv_cat);
			}
			else
			{
				LL_WARNS(LOG_INV) << "loadInventoryFromFile().  Ignoring invalid inventory category: " << inv_cat->getName() << LL_ENDL;
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
					LL_WARNS(LOG_INV) << "Ignoring inventory with null item id: "
									  << inv_item->getName() << LL_ENDL;
						
				}
				else
				{
					items.push_back(inv_item);
				}
			}
			else
			{
				LL_WARNS(LOG_INV) << "loadInventoryFromFile().  Ignoring invalid inventory item: " << inv_item->getName() << LL_ENDL;
				//delete inv_item; // automatic when inv_cat is reassigned or destroyed
			}
		}
		else
		{
			LL_WARNS(LOG_INV) << "Unknown token in inventory file '" << keyword << "'"
							  << LL_ENDL;
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
		LL_ERRS(LOG_INV) << "Filename is Null!" << LL_ENDL;
		return false;
	}
	LL_INFOS(LOG_INV) << "LLInventoryModel::saveToFile(" << filename << ")" << LL_ENDL;
	LLFILE* file = LLFile::fopen(filename, "wb");		/*Flawfinder: ignore*/
	if(!file)
	{
		LL_WARNS(LOG_INV) << "unable to save inventory to: " << filename << LL_ENDL;
		return false;
	}

	fprintf(file, "\tinv_cache_version\t%d\n",sCurrentInvCacheVersion);
	S32 count = categories.size();
	S32 i;
	for(i = 0; i < count; ++i)
	{
		LLViewerInventoryCategory* cat = categories[i];
		if(cat->getVersion() != LLViewerInventoryCategory::VERSION_UNKNOWN)
		{
			cat->exportFileLocal(file);
		}
	}

	count = items.size();
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
	msg->setHandlerFuncFast(_PREHASH_SaveAssetIntoInventory,
						processSaveAssetIntoInventory,
						NULL);
	msg->setHandlerFuncFast(_PREHASH_BulkUpdateInventory,
							processBulkUpdateInventory,
							NULL);
	msg->setHandlerFunc("MoveInventoryItem", processMoveInventoryItem);
}


// 	static
void LLInventoryModel::processUpdateCreateInventoryItem(LLMessageSystem* msg, void**)
{
	// do accounting and highlight new items if they arrive
	if (gInventory.messageUpdateCore(msg, true, LLInventoryObserver::UPDATE_CREATE))
	{
		U32 callback_id;
		LLUUID item_id;
		msg->getUUIDFast(_PREHASH_InventoryData, _PREHASH_ItemID, item_id);
		msg->getU32Fast(_PREHASH_InventoryData, _PREHASH_CallbackID, callback_id);

		gInventoryCallbacks.fire(callback_id, item_id);
	}

}

bool LLInventoryModel::messageUpdateCore(LLMessageSystem* msg, bool account, U32 mask)
{
	//make sure our added inventory observer is active
	start_new_inventory_observer();

	LLUUID agent_id;
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_AgentID, agent_id);
	if(agent_id != gAgent.getID())
	{
		LL_WARNS(LOG_INV) << "Got a inventory update for the wrong agent: " << agent_id
						  << LL_ENDL;
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
		LL_DEBUGS(LOG_INV) << "LLInventoryModel::messageUpdateCore() item id: "
						   << titem->getUUID() << LL_ENDL;
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
	if (account)
	{
		mask |= LLInventoryObserver::CREATE;
	}
	//as above, this loop never seems to loop more than once per call
	for (item_array_t::iterator it = items.begin(); it != items.end(); ++it)
	{
		changes |= gInventory.updateItem(*it, mask);
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
	LL_DEBUGS(LOG_INV) << "Message has " << count << " item blocks" << LL_ENDL;
	uuid_vec_t item_ids;
	update_map_t update;
	for(S32 i = 0; i < count; ++i)
	{
		msg->getUUIDFast(msg_label, _PREHASH_ItemID, item_id, i);
		LL_DEBUGS(LOG_INV) << "Checking for item-to-be-removed " << item_id << LL_ENDL;
		LLViewerInventoryItem* itemp = gInventory.getItem(item_id);
		if(itemp)
		{
			LL_DEBUGS(LOG_INV) << "Item will be removed " << item_id << LL_ENDL;
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
		LL_DEBUGS(LOG_INV) << "Calling deleteObject " << *it << LL_ENDL;
		gInventory.deleteObject(*it);
	}
}

// 	static
void LLInventoryModel::processRemoveInventoryItem(LLMessageSystem* msg, void**)
{
	LL_DEBUGS(LOG_INV) << "LLInventoryModel::processRemoveInventoryItem()" << LL_ENDL;
	LLUUID agent_id, item_id;
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_AgentID, agent_id);
	if(agent_id != gAgent.getID())
	{
		LL_WARNS(LOG_INV) << "Got a RemoveInventoryItem for the wrong agent."
						  << LL_ENDL;
		return;
	}
	LLInventoryModel::removeInventoryItem(agent_id, msg, _PREHASH_InventoryData);
	gInventory.notifyObservers();
}

// 	static
void LLInventoryModel::processUpdateInventoryFolder(LLMessageSystem* msg,
													void**)
{
	LL_DEBUGS(LOG_INV) << "LLInventoryModel::processUpdateInventoryFolder()" << LL_ENDL;
	LLUUID agent_id, folder_id, parent_id;
	//char name[DB_INV_ITEM_NAME_BUF_SIZE];
	msg->getUUIDFast(_PREHASH_FolderData, _PREHASH_AgentID, agent_id);
	if(agent_id != gAgent.getID())
	{
		LL_WARNS(LOG_INV) << "Got an UpdateInventoryFolder for the wrong agent."
						  << LL_ENDL;
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
	LL_DEBUGS() << "LLInventoryModel::processRemoveInventoryFolder()" << LL_ENDL;
	LLUUID agent_id, session_id;
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_AgentID, agent_id);
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_SessionID, session_id);
	if(agent_id != gAgent.getID())
	{
		LL_WARNS() << "Got a RemoveInventoryFolder for the wrong agent."
		<< LL_ENDL;
		return;
	}
	LLInventoryModel::removeInventoryFolder( agent_id, msg );
	gInventory.notifyObservers();
}

// 	static
void LLInventoryModel::processRemoveInventoryObjects(LLMessageSystem* msg,
													void**)
{
	LL_DEBUGS() << "LLInventoryModel::processRemoveInventoryObjects()" << LL_ENDL;
	LLUUID agent_id, session_id;
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_AgentID, agent_id);
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_SessionID, session_id);
	if(agent_id != gAgent.getID())
	{
		LL_WARNS() << "Got a RemoveInventoryObjects for the wrong agent."
		<< LL_ENDL;
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
		LL_WARNS() << "Got a SaveAssetIntoInventory message for the wrong agent."
				<< LL_ENDL;
		return;
	}

	LLUUID item_id;
	msg->getUUIDFast(_PREHASH_InventoryData, _PREHASH_ItemID, item_id);

	// The viewer ignores the asset id because this message is only
	// used for attachments/objects, so the asset id is not used in
	// the viewer anyway.
	LL_DEBUGS() << "LLInventoryModel::processSaveAssetIntoInventory itemID="
		<< item_id << LL_ENDL;
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
		LL_INFOS() << "LLInventoryModel::processSaveAssetIntoInventory item"
			" not found: " << item_id << LL_ENDL;
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
		LL_WARNS() << "Got a BulkUpdateInventory for the wrong agent." << LL_ENDL;
		return;
	}
	LLUUID tid;
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_TransactionID, tid);
#ifndef LL_RELEASE_FOR_DOWNLOAD
	LL_DEBUGS("Inventory") << "Bulk inventory: " << tid << LL_ENDL;
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
		LL_DEBUGS("Inventory") << "unpacked folder '" << tfolder->getName() << "' ("
							   << tfolder->getUUID() << ") in " << tfolder->getParentUUID()
							   << LL_ENDL;
        
        // If the folder is a listing or a version folder, all we need to do is update the SLM data
        int depth_folder = depth_nesting_in_marketplace(tfolder->getUUID());
        if ((depth_folder == 1) || (depth_folder == 2))
        {
            // Trigger an SLM listing update
            LLUUID listing_uuid = (depth_folder == 1 ? tfolder->getUUID() : tfolder->getParentUUID());
            S32 listing_id = LLMarketplaceData::instance().getListingID(listing_uuid);
            LLMarketplaceData::instance().getListing(listing_id);
            // In that case, there is no item to update so no callback -> we skip the rest of the update
        }
		else if(tfolder->getUUID().notNull())
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
		LL_DEBUGS("Inventory") << "unpacked item '" << titem->getName() << "' in "
							   << titem->getParentUUID() << LL_ENDL;
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
}

// static
void LLInventoryModel::processMoveInventoryItem(LLMessageSystem* msg, void**)
{
	LL_DEBUGS() << "LLInventoryModel::processMoveInventoryItem()" << LL_ENDL;
	LLUUID agent_id;
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_AgentID, agent_id);
	if(agent_id != gAgent.getID())
	{
		LL_WARNS() << "Got a MoveInventoryItem message for the wrong agent."
				<< LL_ENDL;
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

			LL_DEBUGS() << "moving item " << item_id << " to folder "
					 << folder_id << LL_ENDL;
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
			LL_INFOS() << "LLInventoryModel::processMoveInventoryItem item not found: " << item_id << LL_ENDL;
		}
	}
	if(anything_changed)
	{
		gInventory.notifyObservers();
	}
}

//----------------------------------------------------------------------------

// Trash: LLFolderType::FT_TRASH, "ConfirmEmptyTrash"
// Trash: LLFolderType::FT_TRASH, "TrashIsFull" when trash exceeds maximum capacity
// Lost&Found: LLFolderType::FT_LOST_AND_FOUND, "ConfirmEmptyLostAndFound"

bool LLInventoryModel::callbackEmptyFolderType(const LLSD& notification, const LLSD& response, LLFolderType::EType preferred_type)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if (option == 0) // YES
	{
		const LLUUID folder_id = findCategoryUUIDForType(preferred_type);
		purge_descendents_of(folder_id, NULL);
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
		purge_descendents_of(folder_id, NULL);
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

	checkTrashOverflow();
}

void LLInventoryModel::removeObject(const LLUUID& object_id)
{
	if(object_id.isNull())
	{
		return;
	}

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

void  LLInventoryModel::checkTrashOverflow()
{
	static const U32 trash_max_capacity = gSavedSettings.getU32("InventoryTrashMaxCapacity");
	const LLUUID trash_id = findCategoryUUIDForType(LLFolderType::FT_TRASH);
	if (getDescendentsCountRecursive(trash_id, trash_max_capacity) >= trash_max_capacity)
	{
		gInventory.emptyFolderType("TrashIsFull", LLFolderType::FT_TRASH);
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
		LL_WARNS() << "Parent Child Map not yet built; guessing as first time in viewer2." << LL_ENDL;
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

// See also LLInventorySort where landmarks in the Favorites folder are sorted.
class LLViewerInventoryItemSort
{
public:
	bool operator()(const LLPointer<LLViewerInventoryItem>& a, const LLPointer<LLViewerInventoryItem>& b)
	{
		return a->getSortField() < b->getSortField();
	}
};

//----------------------------------------------------------------------------

// *NOTE: DEBUG functionality
void LLInventoryModel::dumpInventory() const
{
	LL_INFOS() << "\nBegin Inventory Dump\n**********************:" << LL_ENDL;
	LL_INFOS() << "mCategory[] contains " << mCategoryMap.size() << " items." << LL_ENDL;
	for(cat_map_t::const_iterator cit = mCategoryMap.begin(); cit != mCategoryMap.end(); ++cit)
	{
		const LLViewerInventoryCategory* cat = cit->second;
		if(cat)
		{
			LL_INFOS() << "  " <<  cat->getUUID() << " '" << cat->getName() << "' "
					<< cat->getVersion() << " " << cat->getDescendentCount()
					<< LL_ENDL;
		}
		else
		{
			LL_INFOS() << "  NULL!" << LL_ENDL;
		}
	}	
	LL_INFOS() << "mItemMap[] contains " << mItemMap.size() << " items." << LL_ENDL;
	for(item_map_t::const_iterator iit = mItemMap.begin(); iit != mItemMap.end(); ++iit)
	{
		const LLViewerInventoryItem* item = iit->second;
		if(item)
		{
			LL_INFOS() << "  " << item->getUUID() << " "
					<< item->getName() << LL_ENDL;
		}
		else
		{
			LL_INFOS() << "  NULL!" << LL_ENDL;
		}
	}
	LL_INFOS() << "\n**********************\nEnd Inventory Dump" << LL_ENDL;
}

// Do various integrity checks on model, logging issues found and
// returning an overall good/bad flag.
bool LLInventoryModel::validate() const
{
	bool valid = true;

	if (getRootFolderID().isNull())
	{
		LL_WARNS() << "no root folder id" << LL_ENDL;
		valid = false;
	}
	if (getLibraryRootFolderID().isNull())
	{
		LL_WARNS() << "no root folder id" << LL_ENDL;
		valid = false;
	}

	if (mCategoryMap.size() + 1 != mParentChildCategoryTree.size())
	{
		// ParentChild should be one larger because of the special entry for null uuid.
		LL_INFOS() << "unexpected sizes: cat map size " << mCategoryMap.size()
				<< " parent/child " << mParentChildCategoryTree.size() << LL_ENDL;
		valid = false;
	}
	S32 cat_lock = 0;
	S32 item_lock = 0;
	S32 desc_unknown_count = 0;
	S32 version_unknown_count = 0;
	for(cat_map_t::const_iterator cit = mCategoryMap.begin(); cit != mCategoryMap.end(); ++cit)
	{
		const LLUUID& cat_id = cit->first;
		const LLViewerInventoryCategory *cat = cit->second;
		if (!cat)
		{
			LL_WARNS() << "invalid cat" << LL_ENDL;
			valid = false;
			continue;
		}
		if (cat_id != cat->getUUID())
		{
			LL_WARNS() << "cat id/index mismatch " << cat_id << " " << cat->getUUID() << LL_ENDL;
			valid = false;
		}

		if (cat->getParentUUID().isNull())
		{
			if (cat_id != getRootFolderID() && cat_id != getLibraryRootFolderID())
			{
				LL_WARNS() << "cat " << cat_id << " has no parent, but is not root ("
						<< getRootFolderID() << ") or library root ("
						<< getLibraryRootFolderID() << ")" << LL_ENDL;
			}
		}
		cat_array_t* cats;
		item_array_t* items;
		getDirectDescendentsOf(cat_id,cats,items);
		if (!cats || !items)
		{
			LL_WARNS() << "invalid direct descendents for " << cat_id << LL_ENDL;
			valid = false;
			continue;
		}
		if (cat->getDescendentCount() == LLViewerInventoryCategory::DESCENDENT_COUNT_UNKNOWN)
		{
			desc_unknown_count++;
		}
		else if (cats->size() + items->size() != cat->getDescendentCount())
		{
			LL_WARNS() << "invalid desc count for " << cat_id << " name [" << cat->getName()
					<< "] parent " << cat->getParentUUID()
					<< " cached " << cat->getDescendentCount()
					<< " expected " << cats->size() << "+" << items->size()
					<< "=" << cats->size() +items->size() << LL_ENDL;
			valid = false;
		}
		if (cat->getVersion() == LLViewerInventoryCategory::VERSION_UNKNOWN)
		{
			version_unknown_count++;
		}
		if (mCategoryLock.count(cat_id))
		{
			cat_lock++;
		}
		if (mItemLock.count(cat_id))
		{
			item_lock++;
		}
		for (S32 i = 0; i<items->size(); i++)
		{
			LLViewerInventoryItem *item = items->at(i);

			if (!item)
			{
				LL_WARNS() << "null item at index " << i << " for cat " << cat_id << LL_ENDL;
				valid = false;
				continue;
			}

			const LLUUID& item_id = item->getUUID();
			
			if (item->getParentUUID() != cat_id)
			{
				LL_WARNS() << "wrong parent for " << item_id << " found "
						<< item->getParentUUID() << " expected " << cat_id
						<< LL_ENDL;
				valid = false;
			}


			// Entries in items and mItemMap should correspond.
			item_map_t::const_iterator it = mItemMap.find(item_id);
			if (it == mItemMap.end())
			{
				LL_WARNS() << "item " << item_id << " found as child of "
						<< cat_id << " but not in top level mItemMap" << LL_ENDL;
				valid = false;
			}
			else
			{
				LLViewerInventoryItem *top_item = it->second;
				if (top_item != item)
				{
					LL_WARNS() << "item mismatch, item_id " << item_id
							<< " top level entry is different, uuid " << top_item->getUUID() << LL_ENDL;
				}
			}

			// Topmost ancestor should be root or library.
			LLUUID topmost_ancestor_id;
			bool found = getObjectTopmostAncestor(item_id, topmost_ancestor_id);
			if (!found)
			{
				LL_WARNS() << "unable to find topmost ancestor for " << item_id << LL_ENDL;
				valid = false;
			}
			else
			{
				if (topmost_ancestor_id != getRootFolderID() &&
					topmost_ancestor_id != getLibraryRootFolderID())
				{
					LL_WARNS() << "unrecognized top level ancestor for " << item_id
							<< " got " << topmost_ancestor_id
							<< " expected " << getRootFolderID()
							<< " or " << getLibraryRootFolderID() << LL_ENDL;
					valid = false;
				}
			}
		}

		// Does this category appear as a child of its supposed parent?
		const LLUUID& parent_id = cat->getParentUUID();
		if (!parent_id.isNull())
		{
			cat_array_t* cats;
			item_array_t* items;
			getDirectDescendentsOf(parent_id,cats,items);
			if (!cats)
			{
				LL_WARNS() << "cat " << cat_id << " name [" << cat->getName()
						<< "] orphaned - no child cat array for alleged parent " << parent_id << LL_ENDL;
				valid = false;
			}
			else
			{
				bool found = false;
				for (S32 i = 0; i<cats->size(); i++)
				{
					LLViewerInventoryCategory *kid_cat = cats->at(i);
					if (kid_cat == cat)
					{
						found = true;
						break;
					}
				}
				if (!found)
				{
					LL_WARNS() << "cat " << cat_id << " name [" << cat->getName()
							<< "] orphaned - not found in child cat array of alleged parent " << parent_id << LL_ENDL;
				}
			}
		}
	}

	for(item_map_t::const_iterator iit = mItemMap.begin(); iit != mItemMap.end(); ++iit)
	{
		const LLUUID& item_id = iit->first;
		LLViewerInventoryItem *item = iit->second;
		if (item->getUUID() != item_id)
		{
			LL_WARNS() << "item_id " << item_id << " does not match " << item->getUUID() << LL_ENDL;
			valid = false;
		}

		const LLUUID& parent_id = item->getParentUUID();
		if (parent_id.isNull())
		{
			LL_WARNS() << "item " << item_id << " name [" << item->getName() << "] has null parent id!" << LL_ENDL;
		}
		else
		{
			cat_array_t* cats;
			item_array_t* items;
			getDirectDescendentsOf(parent_id,cats,items);
			if (!items)
			{
				LL_WARNS() << "item " << item_id << " name [" << item->getName()
						<< "] orphaned - alleged parent has no child items list " << parent_id << LL_ENDL;
			}
			else
			{
				bool found = false;
				for (S32 i=0; i<items->size(); ++i)
				{
					if (items->at(i) == item) 
					{
						found = true;
						break;
					}
				}
				if (!found)
				{
					LL_WARNS() << "item " << item_id << " name [" << item->getName()
							<< "] orphaned - not found as child of alleged parent " << parent_id << LL_ENDL;
				}
			}
				
		}
		// Link checking
		if (item->getIsLinkType())
		{
			const LLUUID& link_id = item->getUUID();
			const LLUUID& target_id = item->getLinkedUUID();
			LLViewerInventoryItem *target_item = getItem(target_id);
			LLViewerInventoryCategory *target_cat = getCategory(target_id);
			// Linked-to UUID should have back reference to this link.
			if (!hasBacklinkInfo(link_id, target_id))
			{
				LL_WARNS() << "link " << item->getUUID() << " type " << item->getActualType()
						<< " missing backlink info at target_id " << target_id
						<< LL_ENDL;
			}
			// Links should have referents.
			if (item->getActualType() == LLAssetType::AT_LINK && !target_item)
			{
				LL_WARNS() << "broken item link " << item->getName() << " id " << item->getUUID() << LL_ENDL;
			}
			else if (item->getActualType() == LLAssetType::AT_LINK_FOLDER && !target_cat)
			{
				LL_WARNS() << "broken folder link " << item->getName() << " id " << item->getUUID() << LL_ENDL;
			}
			if (target_item && target_item->getIsLinkType())
			{
				LL_WARNS() << "link " << item->getName() << " references a link item "
						<< target_item->getName() << " " << target_item->getUUID() << LL_ENDL;
			}

			// Links should not have backlinks.
			std::pair<backlink_mmap_t::const_iterator, backlink_mmap_t::const_iterator> range = mBacklinkMMap.equal_range(link_id);
			if (range.first != range.second)
			{
				LL_WARNS() << "Link item " << item->getName() << " has backlinks!" << LL_ENDL;
			}
		}
		else
		{
			// Check the backlinks of a non-link item.
			const LLUUID& target_id = item->getUUID();
			std::pair<backlink_mmap_t::const_iterator, backlink_mmap_t::const_iterator> range = mBacklinkMMap.equal_range(target_id);
			for (backlink_mmap_t::const_iterator it = range.first; it != range.second; ++it)
			{
				const LLUUID& link_id = it->second;
				LLViewerInventoryItem *link_item = getItem(link_id);
				if (!link_item || !link_item->getIsLinkType())
				{
					LL_WARNS() << "invalid backlink from target " << item->getName() << " to " << link_id << LL_ENDL;
				}
			}
		}
	}
	
	if (cat_lock > 0 || item_lock > 0)
	{
		LL_INFOS() << "Found locks on some categories: sub-cat arrays "
				<< cat_lock << ", item arrays " << item_lock << LL_ENDL;
	}
	if (desc_unknown_count != 0)
	{
		LL_INFOS() << "Found " << desc_unknown_count << " cats with unknown descendent count" << LL_ENDL; 
	}
	if (version_unknown_count != 0)
	{
		LL_INFOS() << "Found " << version_unknown_count << " cats with unknown version" << LL_ENDL;
	}

	LL_INFOS() << "Validate done, valid = " << (U32) valid << LL_ENDL;

	return valid;
}

///----------------------------------------------------------------------------
/// Local function definitions
///----------------------------------------------------------------------------


#if 0
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
#endif


///----------------------------------------------------------------------------
/// Class LLInventoryModel::FetchItemHttpHandler
///----------------------------------------------------------------------------

LLInventoryModel::FetchItemHttpHandler::FetchItemHttpHandler(const LLSD & request_sd)
	: LLCore::HttpHandler(),
	  mRequestSD(request_sd)
{}

LLInventoryModel::FetchItemHttpHandler::~FetchItemHttpHandler()
{}

void LLInventoryModel::FetchItemHttpHandler::onCompleted(LLCore::HttpHandle handle,
														 LLCore::HttpResponse * response)
{
	do		// Single-pass do-while used for common exit handling
	{
		LLCore::HttpStatus status(response->getStatus());
		// status = LLCore::HttpStatus(404);				// Dev tool to force error handling
		if (! status)
		{
			processFailure(status, response);
			break;			// Goto common exit
		}

		LLCore::BufferArray * body(response->getBody());
		// body = NULL;									// Dev tool to force error handling
		if (! body || ! body->size())
		{
			LL_WARNS(LOG_INV) << "Missing data in inventory item query." << LL_ENDL;
			processFailure("HTTP response for inventory item query missing body", response);
			break;			// Goto common exit
		}

		// body->write(0, "Garbage Response", 16);		// Dev tool to force error handling
		LLSD body_llsd;
		if (! LLCoreHttpUtil::responseToLLSD(response, true, body_llsd))
		{
			// INFOS-level logging will occur on the parsed failure
			processFailure("HTTP response for inventory item query has malformed LLSD", response);
			break;			// Goto common exit
		}

		// Expect top-level structure to be a map
		// body_llsd = LLSD::emptyArray();				// Dev tool to force error handling
		if (! body_llsd.isMap())
		{
			processFailure("LLSD response for inventory item not a map", response);
			break;			// Goto common exit
		}

		// Check for 200-with-error failures
		//
		// Original Responder-based serivce model didn't check for these errors.
		// It may be more robust to ignore this condition.  With aggregated requests,
		// an error in one inventory item might take down the entire request.
		// So if this instead broke up the aggregated items into single requests,
		// maybe that would make progress.  Or perhaps there's structured information
		// that can tell us what went wrong.  Need to dig into this and firm up
		// the API.
		//
		// body_llsd["error"] = LLSD::emptyMap();		// Dev tool to force error handling
		// body_llsd["error"]["identifier"] = "Development";
		// body_llsd["error"]["message"] = "You left development code in the viewer";
		if (body_llsd.has("error"))
		{
			processFailure("Inventory application error (200-with-error)", response);
			break;			// Goto common exit
		}

		// Okay, process data if possible
		processData(body_llsd, response);
	}
	while (false);
}

void LLInventoryModel::FetchItemHttpHandler::processData(LLSD & content, LLCore::HttpResponse * response)
{
	start_new_inventory_observer();

#if 0
	LLUUID agent_id;
	agent_id = content["agent_id"].asUUID();
	if (agent_id != gAgent.getID())
	{
		LL_WARNS(LOG_INV) << "Got a inventory update for the wrong agent: " << agent_id
						  << LL_ENDL;
		return;
	}
#endif
	
	LLInventoryModel::item_array_t items;
	LLInventoryModel::update_map_t update;
	LLUUID folder_id;
	LLSD content_items(content["items"]);
	const S32 count(content_items.size());

	// Does this loop ever execute more than once?
	for (S32 i(0); i < count; ++i)
	{
		LLPointer<LLViewerInventoryItem> titem = new LLViewerInventoryItem;
		titem->unpackMessage(content_items[i]);
		
		LL_DEBUGS(LOG_INV) << "ItemHttpHandler::httpSuccess item id: "
						   << titem->getUUID() << LL_ENDL;
		items.push_back(titem);
		
		// examine update for changes.
		LLViewerInventoryItem * itemp(gInventory.getItem(titem->getUUID()));

		if (itemp)
		{
			if (titem->getParentUUID() == itemp->getParentUUID())
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

	// as above, this loop never seems to loop more than once per call
	U32 changes(0U);
	for (LLInventoryModel::item_array_t::iterator it = items.begin(); it != items.end(); ++it)
	{
		changes |= gInventory.updateItem(*it);
	}
	// *HUH:  Have computed 'changes', nothing uses it.
	
	gInventory.notifyObservers();
	gViewerWindow->getWindow()->decBusyCount();
}


void LLInventoryModel::FetchItemHttpHandler::processFailure(LLCore::HttpStatus status, LLCore::HttpResponse * response)
{
	const std::string & ct(response->getContentType());
	LL_WARNS(LOG_INV) << "Inventory item fetch failure\n"
					  << "[Status: " << status.toTerseString() << "]\n"
					  << "[Reason: " << status.toString() << "]\n"
					  << "[Content-type: " << ct << "]\n"
					  << "[Content (abridged): "
					  << LLCoreHttpUtil::responseToString(response) << "]" << LL_ENDL;
	gInventory.notifyObservers();
}

void LLInventoryModel::FetchItemHttpHandler::processFailure(const char * const reason, LLCore::HttpResponse * response)
{
	LL_WARNS(LOG_INV) << "Inventory item fetch failure\n"
					  << "[Status: internal error]\n"
					  << "[Reason: " << reason << "]\n"
					  << "[Content (abridged): "
					  << LLCoreHttpUtil::responseToString(response) << "]" << LL_ENDL;
	gInventory.notifyObservers();
}

