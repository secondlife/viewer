/** 
 * @file llappearancemgr.cpp
 * @brief Manager for initiating appearance changes on the viewer
 *
 * $LicenseInfo:firstyear=2004&license=viewergpl$
 * 
 * Copyright (c) 2004-2009, Linden Research, Inc.
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

#include "llappearancemgr.h"
#include "llinventorymodel.h"
#include "llnotifications.h"
#include "llgesturemgr.h"
#include "llinventorybridge.h"
#include "llwearablelist.h"
#include "llagentwearables.h"
#include "llagent.h"
#include "llvoavatar.h"
#include "llvoavatarself.h"
#include "llviewerregion.h"
#include "llfloatercustomize.h"

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
			LLAppearanceManager::wearInventoryCategoryOnAvatar(gInventory.getCategory(mCatID), mAppend);
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
			pid = gInventory.getRootFolderID();
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
		// BAP fixes a lag in display of created dir.
		gInventory.notifyObservers();
	}
	else
	{
		// Wear the inventory category.
		LLAppearanceManager::wearInventoryCategoryOnAvatar(gInventory.getCategory(mCatID), mAppend);
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
		//dec_busy_count();
		gInventory.removeObserver(this);
		delete this;
		return;
	}

	LLOutfitObserver* outfit_observer = new LLOutfitObserver(mCompleteFolders.front(), mCopyItems, mAppend);
	LLInventoryFetchObserver::item_ref_t ids;
	for(S32 i = 0; i < count; ++i)
	{
		ids.push_back(item_array.get(i)->getUUID());
	}

	// clean up, and remove this as an observer since the call to the
	// outfit could notify observers and throw us into an infinite
	// loop.
	//dec_busy_count();
	gInventory.removeObserver(this);
	delete this;

	// increment busy count and either tell the inventory to check &
	// call done, or add this object to the inventory for observation.
	//inc_busy_count();

	// do the fetch
	outfit_observer->fetchItems(ids);
	if(outfit_observer->isEverythingComplete())
	{
		// everything is already here - call done.
		outfit_observer->done();
	}
	else
	{
		// it's all on it's way - add an observer, and the inventory
		// will call done for us when everything is here.
		gInventory.addObserver(outfit_observer);
	}
}

class LLUpdateAppearanceOnDestroy: public LLInventoryCallback
{
public:
	LLUpdateAppearanceOnDestroy():
		mFireCount(0)
	{
	}

	virtual ~LLUpdateAppearanceOnDestroy()
	{
		LLAppearanceManager::updateAppearanceFromCOF();
	}

	/* virtual */ void fire(const LLUUID& inv_item)
	{
		mFireCount++;
	}
private:
	U32 mFireCount;
};

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
	bool append;
};

/* static */ void removeDuplicateItems(LLInventoryModel::item_array_t& items)
{
	LLInventoryModel::item_array_t new_items;
	std::set<LLUUID> items_seen;
	for (S32 i=0; i<items.count(); i++)
	{
		LLViewerInventoryItem *item = items.get(i);
		LLUUID item_id = item->getLinkedUUID();
		if (items_seen.find(item_id)!=items_seen.end())
			continue;
		items_seen.insert(item_id);
		new_items.push_back(item);
	}
	items = new_items;
}

void removeDuplicateItems(LLInventoryModel::item_array_t& dst, const LLInventoryModel::item_array_t& src)
{
	LLInventoryModel::item_array_t new_dst;
	std::set<LLUUID> mark_inventory;

	S32 inventory_dups = 0;
	
	for (LLInventoryModel::item_array_t::const_iterator src_pos = src.begin();
		  src_pos != src.end();
		  ++src_pos)
	{
		LLUUID src_item_id = (*src_pos)->getLinkedUUID();
		mark_inventory.insert(src_item_id);
	}

	for (LLInventoryModel::item_array_t::const_iterator dst_pos = dst.begin();
		  dst_pos != dst.end();
		  ++dst_pos)
	{
		LLUUID dst_item_id = (*dst_pos)->getLinkedUUID();

		if (mark_inventory.find(dst_item_id) == mark_inventory.end())
		{
			// Item is not already present in COF.
			new_dst.put(*dst_pos);
			mark_inventory.insert(dst_item_id);
		}
		else
		{
			inventory_dups++;
		}
	}
	llinfos << "removeDups, original " << dst.count() << " final " << new_dst.count()
			<< " inventory dups " << inventory_dups << llendl;
	
	dst = new_dst;
}

/* static */ 
LLUUID LLAppearanceManager::getCOF()
{
	return gInventory.findCategoryUUIDForType(LLAssetType::AT_CURRENT_OUTFIT);
}

// Update appearance from outfit folder.
/* static */ 
void LLAppearanceManager::changeOutfit(bool proceed, const LLUUID& category, bool append)
{
	if (!proceed)
		return;

#if 1 
	updateCOF(category,append);
#else
	if (append)
	{
		updateCOFFromCategory(category, append); // append is true - add non-duplicates to COF.
	}
	else
	{
		LLViewerInventoryCategory* catp = gInventory.getCategory(category);
		if (catp->getPreferredType() == LLAssetType::AT_NONE ||
			LLAssetType::lookupIsEnsembleCategoryType(catp->getPreferredType()))
		{
			updateCOFFromCategory(category, append);  // append is false - rebuild COF.
		}
		else if (catp->getPreferredType() == LLAssetType::AT_OUTFIT)
		{
			rebuildCOFFromOutfit(category);
		}
	}
#endif
}

// Append to current COF contents by recursively traversing a folder.
/* static */ 
void LLAppearanceManager::updateCOFFromCategory(const LLUUID& category, bool append)
{
		// BAP consolidate into one "get all 3 types of descendents" function, use both places.
	LLInventoryModel::item_array_t wear_items;
	LLInventoryModel::item_array_t obj_items;
	LLInventoryModel::item_array_t gest_items;
	bool follow_folder_links = false;
	getUserDescendents(category, wear_items, obj_items, gest_items, follow_folder_links);

	// Find all the wearables that are in the category's subtree.	
	lldebugs << "appendCOFFromCategory()" << llendl;
	if( !wear_items.count() && !obj_items.count() && !gest_items.count())
	{
		LLNotifications::instance().add("CouldNotPutOnOutfit");
		return;
	}
		
	const LLUUID &current_outfit_id = gInventory.findCategoryUUIDForType(LLAssetType::AT_CURRENT_OUTFIT);
	// Processes that take time should show the busy cursor
	//inc_busy_count();
		
	LLInventoryModel::cat_array_t cof_cats;
	LLInventoryModel::item_array_t cof_items;
	gInventory.collectDescendents(current_outfit_id, cof_cats, cof_items,
								  LLInventoryModel::EXCLUDE_TRASH);
	// Remove duplicates
	if (append)
	{
		removeDuplicateItems(wear_items, cof_items);
		removeDuplicateItems(obj_items, cof_items);
		removeDuplicateItems(gest_items, cof_items);
	}

	S32 total_links = gest_items.count() + wear_items.count() + obj_items.count();

	if (!append && total_links > 0)
	{
		purgeCOFBeforeRebuild(category);
	}

	LLPointer<LLUpdateAppearanceOnDestroy> link_waiter = new LLUpdateAppearanceOnDestroy;
	
	// Link all gestures in this folder
	if (gest_items.count() > 0)
	{
		llinfos << "Linking " << gest_items.count() << " gestures" << llendl;
		for (S32 i = 0; i < gest_items.count(); ++i)
		{
			const LLInventoryItem* gest_item = gest_items.get(i).get();
			link_inventory_item(gAgent.getID(), gest_item->getLinkedUUID(), current_outfit_id,
								gest_item->getName(),
								LLAssetType::AT_LINK, link_waiter);
		}
	}

	// Link all wearables
	if(wear_items.count() > 0)
	{
		llinfos << "Linking " << wear_items.count() << " wearables" << llendl;
		for(S32 i = 0; i < wear_items.count(); ++i)
		{
			// Populate the current outfit folder with links to the newly added wearables
			const LLInventoryItem* wear_item = wear_items.get(i).get();
			link_inventory_item(gAgent.getID(), 
								wear_item->getLinkedUUID(), // If this item is a link, then we'll use the linked item's UUID.
								current_outfit_id, 
								wear_item->getName(),
								LLAssetType::AT_LINK, 
								link_waiter);
		}
	}

	// Link all attachments.
	if( obj_items.count() > 0 )
	{
		llinfos << "Linking " << obj_items.count() << " attachments" << llendl;
		LLVOAvatar* avatar = gAgent.getAvatarObject();
		if( avatar )
		{
			for(S32 i = 0; i < obj_items.count(); ++i)
			{
				const LLInventoryItem* obj_item = obj_items.get(i).get();
				link_inventory_item(gAgent.getID(), 
									obj_item->getLinkedUUID(), // If this item is a link, then we'll use the linked item's UUID.
									current_outfit_id, 
									obj_item->getName(),
									LLAssetType::AT_LINK, link_waiter);
			}
		}
	}
}

/* static */ 
void LLAppearanceManager::shallowCopyCategory(const LLUUID& src_id, const LLUUID& dst_id,
											  LLPointer<LLInventoryCallback> cb)
{
	LLInventoryModel::cat_array_t cats;
	LLInventoryModel::item_array_t items;
	gInventory.collectDescendents(src_id, cats, items,
								  LLInventoryModel::EXCLUDE_TRASH);
	for (S32 i = 0; i < items.count(); ++i)
	{
		const LLViewerInventoryItem* item = items.get(i).get();
		if (item->getActualType() == LLAssetType::AT_LINK)
		{
			link_inventory_item(gAgent.getID(),
								item->getLinkedUUID(),
								dst_id,
								item->getName(),
								LLAssetType::AT_LINK, cb);
		}
		else if (item->getActualType() == LLAssetType::AT_LINK_FOLDER)
		{
			LLViewerInventoryCategory *catp = item->getLinkedCategory();
			// Skip copying outfit links.
			if (catp && catp->getPreferredType() != LLAssetType::AT_OUTFIT)
			{
				link_inventory_item(gAgent.getID(),
									item->getLinkedUUID(),
									dst_id,
									item->getName(),
									LLAssetType::AT_LINK_FOLDER, cb);
			}
		}
		else
		{
			copy_inventory_item(
				gAgent.getID(),
				item->getPermissions().getOwner(),
				item->getUUID(),
				dst_id,
				item->getName(),
				cb);
		}
	}
}
/* static */ void LLAppearanceManager::purgeCategory(const LLUUID& category, bool keep_outfit_links)
{
	LLInventoryModel::cat_array_t cats;
	LLInventoryModel::item_array_t items;
	gInventory.collectDescendents(category, cats, items,
								  LLInventoryModel::EXCLUDE_TRASH);
	for (S32 i = 0; i < items.count(); ++i)
	{
		LLViewerInventoryItem *item = items.get(i);
		if (keep_outfit_links && (item->getActualType() == LLAssetType::AT_LINK_FOLDER))
			continue;
		gInventory.purgeObject(item->getUUID());
	}
}

// Keep the last N wearables of each type.  For viewer 2.0, N is 1 for
// both body parts and clothing items.
/* static */ void LLAppearanceManager::filterWearableItems(
	LLInventoryModel::item_array_t& items, S32 max_per_type)
{
	// Divvy items into arrays by wearable type.
	std::vector<LLInventoryModel::item_array_t> items_by_type(WT_COUNT);
	for (S32 i=0; i<items.count(); i++)
	{
		LLViewerInventoryItem *item = items.get(i);
		// Ignore non-wearables.
		if (!item->isWearableType())
			continue;
		EWearableType type = item->getWearableType();
		items_by_type[type].push_back(item);
	}

	// rebuild items list, retaining the last max_per_type of each array
	items.clear();
	for (S32 i=0; i<WT_COUNT; i++)
	{
		S32 size = items_by_type[i].size();
		if (size <= 0)
			continue;
		S32 start_index = llmax(0,size-max_per_type);
		for (S32 j = start_index; j<size; j++)
		{
			items.push_back(items_by_type[i][j]);
		}
	}
}

// Create links to all listed items.
/* static */ void LLAppearanceManager::linkAll(const LLUUID& category,
											   LLInventoryModel::item_array_t& items,
											   LLPointer<LLInventoryCallback> cb)
{
	for (S32 i=0; i<items.count(); i++)
	{
		const LLInventoryItem* item = items.get(i).get();
		link_inventory_item(gAgent.getID(),
							item->getLinkedUUID(),
							category,
							item->getName(),
							LLAssetType::AT_LINK,
							cb);
	}
}

/* static */ void LLAppearanceManager::updateCOF(const LLUUID& category, bool append)
{
	const LLUUID cof = getCOF();

	// Collect and filter descendents to determine new COF contents.

	// - Body parts: always include COF contents as a fallback in case any
	// required parts are missing.
	LLInventoryModel::item_array_t body_items;
	getDescendentsOfAssetType(cof, body_items, LLAssetType::AT_BODYPART, false);
	getDescendentsOfAssetType(category, body_items, LLAssetType::AT_BODYPART, false);
	// Reduce body items to max of one per type.
	removeDuplicateItems(body_items);
	filterWearableItems(body_items, 1);

	// - Wearables: include COF contents only if appending.
	LLInventoryModel::item_array_t wear_items;
	if (append)
		getDescendentsOfAssetType(cof, wear_items, LLAssetType::AT_CLOTHING, false);
	getDescendentsOfAssetType(category, wear_items, LLAssetType::AT_CLOTHING, false);
	// Reduce wearables to max of one per type.
	removeDuplicateItems(wear_items);
	filterWearableItems(wear_items, 1);

	// - Attachments: include COF contents only if appending.
	LLInventoryModel::item_array_t obj_items;
	if (append)
		getDescendentsOfAssetType(cof, obj_items, LLAssetType::AT_OBJECT, false);
	getDescendentsOfAssetType(category, obj_items, LLAssetType::AT_OBJECT, false);
	removeDuplicateItems(obj_items);

	// - Gestures: include COF contents only if appending.
	LLInventoryModel::item_array_t gest_items;
	if (append)
		getDescendentsOfAssetType(cof, gest_items, LLAssetType::AT_GESTURE, false);
	getDescendentsOfAssetType(category, gest_items, LLAssetType::AT_GESTURE, false);
	removeDuplicateItems(gest_items);
	
	// Remove current COF contents.
	bool keep_outfit_links = append;
	purgeCategory(cof, keep_outfit_links);
	gInventory.notifyObservers();

	// Create links to new COF contents.
	LLPointer<LLInventoryCallback> link_waiter = new LLUpdateAppearanceOnDestroy;

	linkAll(cof, body_items, link_waiter);
	linkAll(cof, wear_items, link_waiter);
	linkAll(cof, obj_items, link_waiter);
	linkAll(cof, gest_items, link_waiter);

	// Add link to outfit if category is an outfit. 
	LLViewerInventoryCategory* catp = gInventory.getCategory(category);
	if (!append && catp && catp->getPreferredType() == LLAssetType::AT_OUTFIT)
	{
		link_inventory_item(gAgent.getID(), category, cof, catp->getName(),
							LLAssetType::AT_LINK_FOLDER, link_waiter);
	}
							  
}

/* static */ 
bool LLAppearanceManager::isMandatoryWearableType(EWearableType type)
{
	return (type==WT_SHAPE) || (type==WT_SKIN) || (type== WT_HAIR) || (type==WT_EYES);
}

// For mandatory body parts.
/* static */ 
void LLAppearanceManager::checkMandatoryWearableTypes(const LLUUID& category, std::set<EWearableType>& types_found)
{
	LLInventoryModel::cat_array_t new_cats;
	LLInventoryModel::item_array_t new_items;
	gInventory.collectDescendents(category, new_cats, new_items,
								  LLInventoryModel::EXCLUDE_TRASH);
	std::set<EWearableType> wt_types_found;
	for (S32 i = 0; i < new_items.count(); ++i)
	{
		LLViewerInventoryItem *itemp = new_items.get(i);
		if (itemp->isWearableType())
		{
			EWearableType type = itemp->getWearableType();
			if (isMandatoryWearableType(type))
			{
				types_found.insert(type);
			}
		}
	}
}

// Remove everything from the COF that we safely can before replacing
// with contents of new category.  This means preserving any mandatory
// body parts that aren't present in the new category, and getting rid
// of everything else.
/* static */ 
void LLAppearanceManager::purgeCOFBeforeRebuild(const LLUUID& category)
{
	// See which mandatory body types are present in the new category.
	std::set<EWearableType> wt_types_found;
	checkMandatoryWearableTypes(category,wt_types_found);
	
	LLInventoryModel::cat_array_t cof_cats;
	LLInventoryModel::item_array_t cof_items;
	gInventory.collectDescendents(getCOF(), cof_cats, cof_items,
								  LLInventoryModel::EXCLUDE_TRASH);
	for (S32 i = 0; i < cof_items.count(); ++i)
	{
		LLViewerInventoryItem *itemp = cof_items.get(i);
		if (itemp->isWearableType())
		{
			EWearableType type = itemp->getWearableType();
			if (!isMandatoryWearableType(type) || (wt_types_found.find(type) != wt_types_found.end()))
			{
				// Not mandatory or supplied by the new category - OK to delete
				gInventory.purgeObject(cof_items.get(i)->getUUID());
			}
		}
		else
		{
			// Not a wearable - always purge
			gInventory.purgeObject(cof_items.get(i)->getUUID());
		}
	}
	gInventory.notifyObservers();
}

// Replace COF contents from a given outfit folder.
/* static */ 
void LLAppearanceManager::rebuildCOFFromOutfit(const LLUUID& category)
{
	lldebugs << "rebuildCOFFromOutfit()" << llendl;

	dumpCat(category,"start, source outfit");
	dumpCat(getCOF(),"start, COF");

	// Find all the wearables that are in the category's subtree.	
	LLInventoryModel::item_array_t items;
	getCOFValidDescendents(category, items);

	if( items.count() == 0)
	{
		LLNotifications::instance().add("CouldNotPutOnOutfit");
		return;
	}
		
	// Processes that take time should show the busy cursor
	//inc_busy_count();

	//dumpCat(current_outfit_id,"COF before remove:");

	//dumpCat(current_outfit_id,"COF after remove:");

	purgeCOFBeforeRebuild(category);
	
	LLPointer<LLInventoryCallback> link_waiter = new LLUpdateAppearanceOnDestroy;
	LLUUID current_outfit_id = getCOF();
	LLAppearanceManager::shallowCopyCategory(category, current_outfit_id, link_waiter);

	//dumpCat(current_outfit_id,"COF after shallow copy:");

	// Create a link to the outfit that we wore.
	LLViewerInventoryCategory* catp = gInventory.getCategory(category);
	if (catp && catp->getPreferredType() == LLAssetType::AT_OUTFIT)
	{
		link_inventory_item(gAgent.getID(), category, current_outfit_id, catp->getName(),
							LLAssetType::AT_LINK_FOLDER, link_waiter);
	}
}

/* static */
void LLAppearanceManager::onWearableAssetFetch(LLWearable* wearable, void* data)
{
	LLWearableHoldingPattern* holder = (LLWearableHoldingPattern*)data;
	bool append = holder->append;
	
	if(wearable)
	{
		for (LLWearableHoldingPattern::found_list_t::iterator iter = holder->mFoundList.begin();
			 iter != holder->mFoundList.end(); ++iter)
		{
			LLFoundData* data = *iter;
			if(wearable->getAssetID() == data->mAssetID)
			{
				data->mWearable = wearable;
				break;
			}
		}
	}
	holder->mResolved += 1;
	if(holder->mResolved >= (S32)holder->mFoundList.size())
	{
		LLAppearanceManager::updateAgentWearables(holder, append);
	}
}

/* static */
void LLAppearanceManager::updateAgentWearables(LLWearableHoldingPattern* holder, bool append)
{
	lldebugs << "updateAgentWearables()" << llendl;
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
				if( item && (item->getAssetUUID() == wearable->getAssetID()) )
				{
					items.put(item);
					wearables.put(wearable);
				}
				break;
			}
		}
	}

	if(wearables.count() > 0)
	{
		gAgentWearables.setWearableOutfit(items, wearables, !append);
		gInventory.notifyObservers();
	}

	delete holder;

//	dec_busy_count();
}

/* static */ 
void LLAppearanceManager::updateAppearanceFromCOF()
{
	dumpCat(getCOF(),"COF, start");

	bool follow_folder_links = true;
	LLUUID current_outfit_id = getCOF();

	// Find all the wearables that are in the COF's subtree.	
	lldebugs << "LLAppearanceManager::updateFromCOF()" << llendl;
	LLInventoryModel::item_array_t wear_items;
	LLInventoryModel::item_array_t obj_items;
	LLInventoryModel::item_array_t gest_items;
	getUserDescendents(current_outfit_id, wear_items, obj_items, gest_items, follow_folder_links);
	
	if( !wear_items.count() && !obj_items.count() && !gest_items.count())
	{
		LLNotifications::instance().add("CouldNotPutOnOutfit");
		return;
	}
		
	// Processes that take time should show the busy cursor
	//inc_busy_count(); // BAP this is currently a no-op in llinventorybridge.cpp - do we need it?
		
	// Activate all gestures in this folder
	if (gest_items.count() > 0)
	{
		llinfos << "Activating " << gest_items.count() << " gestures" << llendl;

		LLGestureManager::instance().activateGestures(gest_items);

		// Update the inventory item labels to reflect the fact
		// they are active.
		LLViewerInventoryCategory* catp = gInventory.getCategory(current_outfit_id);
		if (catp)
		{
			gInventory.updateCategory(catp);
			gInventory.notifyObservers();
		}
	}

	if(wear_items.count() > 0)
	{
		// Note: can't do normal iteration, because if all the
		// wearables can be resolved immediately, then the
		// callback will be called (and this object deleted)
		// before the final getNextData().
		LLWearableHoldingPattern* holder = new LLWearableHoldingPattern;
		LLFoundData* found;
		LLDynamicArray<LLFoundData*> found_container;
		for(S32 i = 0; i  < wear_items.count(); ++i)
		{
			found = new LLFoundData(wear_items.get(i)->getLinkedUUID(), // Wear the base item, not the link
									wear_items.get(i)->getAssetUUID(),
									wear_items.get(i)->getName(),
									wear_items.get(i)->getType());
			holder->mFoundList.push_front(found);
			found_container.put(found);
		}
		for(S32 i = 0; i < wear_items.count(); ++i)
		{
			holder->append = false;
			found = found_container.get(i);
				
			// Fetch the wearables about to be worn.
			LLWearableList::instance().getAsset(found->mAssetID,
												found->mName,
												found->mAssetType,
												LLAppearanceManager::onWearableAssetFetch,
												(void*)holder);
		}
	}

	// Update attachments to match those requested.
	LLVOAvatar* avatar = gAgent.getAvatarObject();
	if( avatar )
	{
		LLAgentWearables::userUpdateAttachments(obj_items);
	}
}

/* static */ 
void LLAppearanceManager::getCOFValidDescendents(const LLUUID& category,
												 LLInventoryModel::item_array_t& items)
{
	LLInventoryModel::cat_array_t cats;
	LLFindCOFValidItems is_cof_valid;
	bool follow_folder_links = false;
	gInventory.collectDescendentsIf(category,
									cats, 
									items, 
									LLInventoryModel::EXCLUDE_TRASH,
									is_cof_valid, 
									follow_folder_links);
}

/* static */
void LLAppearanceManager::getDescendentsOfAssetType(const LLUUID& category,
													LLInventoryModel::item_array_t& items,
													LLAssetType::EType type,
													bool follow_folder_links)
{
	LLInventoryModel::cat_array_t cats;
	LLIsType is_of_type(type);
	gInventory.collectDescendentsIf(category,
									cats,
									items,
									LLInventoryModel::EXCLUDE_TRASH,
									is_of_type,
									follow_folder_links);
}

/* static */ 
void LLAppearanceManager::getUserDescendents(const LLUUID& category, 
											 LLInventoryModel::item_array_t& wear_items,
											 LLInventoryModel::item_array_t& obj_items,
											 LLInventoryModel::item_array_t& gest_items,
											 bool follow_folder_links)
{
	LLInventoryModel::cat_array_t wear_cats;
	LLFindWearables is_wearable;
	gInventory.collectDescendentsIf(category,
									wear_cats,
									wear_items,
									LLInventoryModel::EXCLUDE_TRASH,
									is_wearable,
									follow_folder_links);

	LLInventoryModel::cat_array_t obj_cats;
	LLIsType is_object( LLAssetType::AT_OBJECT );
	gInventory.collectDescendentsIf(category,
									obj_cats,
									obj_items,
									LLInventoryModel::EXCLUDE_TRASH,
									is_object,
									follow_folder_links);

	// Find all gestures in this folder
	LLInventoryModel::cat_array_t gest_cats;
	LLIsType is_gesture( LLAssetType::AT_GESTURE );
	gInventory.collectDescendentsIf(category,
									gest_cats,
									gest_items,
									LLInventoryModel::EXCLUDE_TRASH,
									is_gesture,
									follow_folder_links);
}

void LLAppearanceManager::wearInventoryCategory(LLInventoryCategory* category, bool copy, bool append)
{
	if(!category) return;

	lldebugs << "wearInventoryCategory( " << category->getName()
			 << " )" << llendl;
	// What we do here is get the complete information on the items in
	// the inventory, and set up an observer that will wait for that to
	// happen.
	LLOutfitFetch* outfit_fetcher = new LLOutfitFetch(copy, append);
	LLInventoryFetchDescendentsObserver::folder_ref_t folders;
	folders.push_back(category->getUUID());
	outfit_fetcher->fetchDescendents(folders);
	//inc_busy_count();
	if(outfit_fetcher->isEverythingComplete())
	{
		// everything is already here - call done.
		outfit_fetcher->done();
	}
	else
	{
		// it's all on it's way - add an observer, and the inventory
		// will call done for us when everything is here.
		gInventory.addObserver(outfit_fetcher);
	}
}

// *NOTE: hack to get from avatar inventory to avatar
/* static */
void LLAppearanceManager::wearInventoryCategoryOnAvatar( LLInventoryCategory* category, bool append )
{
	// Avoid unintentionally overwriting old wearables.  We have to do
	// this up front to avoid having to deal with the case of multiple
	// wearables being dirty.
	if(!category) return;
	lldebugs << "wearInventoryCategoryOnAvatar( " << category->getName()
			 << " )" << llendl;
			 	
	if( gFloaterCustomize )
	{
		gFloaterCustomize->askToSaveIfDirty(boost::bind(LLAppearanceManager::changeOutfit, _1, category->getUUID(), append));
	}
	else
	{
		LLAppearanceManager::changeOutfit(TRUE, category->getUUID(), append);
	}
}

/* static */
void LLAppearanceManager::wearOutfitByName(const std::string& name)
{
	llinfos << "Wearing category " << name << llendl;
	//inc_busy_count();

	LLInventoryModel::cat_array_t cat_array;
	LLInventoryModel::item_array_t item_array;
	LLNameCategoryCollector has_name(name);
	gInventory.collectDescendentsIf(gInventory.getRootFolderID(),
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
		LLAppearanceManager::wearInventoryCategory(cat, copy_items, false);
	}
	else
	{
		llwarns << "Couldn't find outfit " <<name<< " in wearOutfitByName()"
				<< llendl;
	}

	//dec_busy_count();
}

/* static */
void LLAppearanceManager::wearItem( LLInventoryItem* item, bool do_update )
{
	LLInventoryModel::cat_array_t cat_array;
	LLInventoryModel::item_array_t item_array;
	gInventory.collectDescendents(LLAppearanceManager::getCOF(),
								  cat_array,
								  item_array,
								  LLInventoryModel::EXCLUDE_TRASH);
	bool linked_already = false;
	for (S32 i=0; i<item_array.count(); i++)
	{
		const LLInventoryItem* inv_item = item_array.get(i).get();
		if (inv_item->getLinkedUUID() == item->getLinkedUUID())
		{
			linked_already = true;
			break;
		}
	}
	if (linked_already)
	{
		if (do_update)
			LLAppearanceManager::updateAppearanceFromCOF();
	}
	else
	{
		LLPointer<LLInventoryCallback> cb = do_update ? new ModifiedCOFCallback : 0;
		link_inventory_item( gAgent.getID(),
							 item->getLinkedUUID(),
							 getCOF(),
							 item->getName(),
							 LLAssetType::AT_LINK,
							 cb);
	}
}

/* static */
void LLAppearanceManager::wearEnsemble( LLInventoryCategory* cat, bool do_update )
{
#if SUPPORT_ENSEMBLES
	// BAP add check for already in COF.
	LLPointer<LLInventoryCallback> cb = do_update ? new ModifiedCOFCallback : 0;
	link_inventory_item( gAgent.getID(),
						 cat->getLinkedUUID(),
						 getCOF(),
						 cat->getName(),
						 LLAssetType::AT_LINK_FOLDER,
						 cb);
#endif
}

/* static */
void LLAppearanceManager::removeItemLinks(const LLUUID& item_id, bool do_update)
{
	LLInventoryModel::cat_array_t cat_array;
	LLInventoryModel::item_array_t item_array;
	gInventory.collectDescendents(LLAppearanceManager::getCOF(),
								  cat_array,
								  item_array,
								  LLInventoryModel::EXCLUDE_TRASH);
	for (S32 i=0; i<item_array.count(); i++)
	{
		const LLInventoryItem* item = item_array.get(i).get();
		if (item->getLinkedUUID() == item_id)
		{
			gInventory.purgeObject(item_array.get(i)->getUUID());
		}
	}
	if (do_update)
	{
		LLAppearanceManager::updateAppearanceFromCOF();
	}
}

//#define DUMP_CAT_VERBOSE

/* static */
void LLAppearanceManager::dumpCat(const LLUUID& cat_id, const std::string& msg)
{
	LLInventoryModel::cat_array_t cats;
	LLInventoryModel::item_array_t items;
	gInventory.collectDescendents(cat_id, cats, items, LLInventoryModel::EXCLUDE_TRASH);

#ifdef DUMP_CAT_VERBOSE
	llinfos << llendl;
	llinfos << str << llendl;
	S32 hitcount = 0;
	for(S32 i=0; i<items.count(); i++)
	{
		LLViewerInventoryItem *item = items.get(i);
		if (item)
			hitcount++;
		llinfos << i <<" "<< item->getName() <<llendl;
	}
#endif
	llinfos << msg << " count " << items.count() << llendl;
}

/* static */
void LLAppearanceManager::dumpItemArray(const LLInventoryModel::item_array_t& items,
										const std::string& msg)
{
	llinfos << msg << llendl;
	for (S32 i=0; i<items.count(); i++)
	{
		LLViewerInventoryItem *item = items.get(i);
		llinfos << i <<" " << item->getName() << llendl;
	}
	llinfos << llendl;
}


std::set<LLUUID> LLAppearanceManager::sRegisteredAttachments;
bool LLAppearanceManager::sAttachmentInvLinkEnabled(false);

/* static */
void LLAppearanceManager::setAttachmentInvLinkEnable(bool val)
{
	llinfos << "setAttachmentInvLinkEnable => " << (int) val << llendl;
	sAttachmentInvLinkEnabled = val;
}

void dumpAttachmentSet(const std::set<LLUUID>& atts, const std::string& msg)
{
       llinfos << msg << llendl;
       for (std::set<LLUUID>::const_iterator it = atts.begin();
               it != atts.end();
               ++it)
       {
               LLUUID item_id = *it;
               LLViewerInventoryItem *item = gInventory.getItem(item_id);
               if (item)
                       llinfos << "atts " << item->getName() << llendl;
               else
                       llinfos << "atts " << "UNKNOWN[" << item_id.asString() << "]" << llendl;
       }
       llinfos << llendl;
}

/* static */
void LLAppearanceManager::registerAttachment(const LLUUID& item_id)
{
       sRegisteredAttachments.insert(item_id);
       dumpAttachmentSet(sRegisteredAttachments,"after register:");

	   if (sAttachmentInvLinkEnabled)
	   {
		   LLViewerInventoryItem *item = gInventory.getItem(item_id);
		   if (item)
		   {
			   LLAppearanceManager::dumpCat(LLAppearanceManager::getCOF(),"Adding attachment link:");
			   LLAppearanceManager::wearItem(item,false);  // Add COF link for item.
			   gInventory.addChangedMask(LLInventoryObserver::LABEL, item_id);
			   gInventory.notifyObservers();
		   }
	   }
	   else
	   {
		   llinfos << "no link changes, inv link not enabled" << llendl;
	   }
}

/* static */
void LLAppearanceManager::unregisterAttachment(const LLUUID& item_id)
{
       sRegisteredAttachments.erase(item_id);
       dumpAttachmentSet(sRegisteredAttachments,"after unregister:");

	   if (sAttachmentInvLinkEnabled)
	   {
		   LLAppearanceManager::dumpCat(LLAppearanceManager::getCOF(),"Removing attachment link:");
		   LLAppearanceManager::removeItemLinks(item_id, false);
		   // BAP - needs to change for label to track link.
		   gInventory.addChangedMask(LLInventoryObserver::LABEL, item_id);
		   gInventory.notifyObservers();
	   }
	   else
	   {
		   llinfos << "no link changes, inv link not enabled" << llendl;
	   }
}
