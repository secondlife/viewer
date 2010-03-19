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

#include "llagent.h"
#include "llagentwearables.h"
#include "llappearancemgr.h"
#include "llcommandhandler.h"
#include "llfloatercustomize.h"
#include "llgesturemgr.h"
#include "llinventorybridge.h"
#include "llinventoryobserver.h"
#include "llnotificationsutil.h"
#include "llsidepanelappearance.h"
#include "llsidetray.h"
#include "llvoavatar.h"
#include "llvoavatarself.h"
#include "llviewerregion.h"
#include "llwearablelist.h"

LLUUID findDescendentCategoryIDByName(const LLUUID& parent_id,const std::string& name)
{
	LLInventoryModel::cat_array_t cat_array;
	LLInventoryModel::item_array_t item_array;
	LLNameCategoryCollector has_name(name);
	gInventory.collectDescendentsIf(parent_id,
									cat_array,
									item_array,
									LLInventoryModel::EXCLUDE_TRASH,
									has_name);
	if (0 == cat_array.count())
		return LLUUID();
	else
	{
		LLViewerInventoryCategory *cat = cat_array.get(0);
		if (cat)
			return cat->getUUID();
		else
		{
			llwarns << "null cat" << llendl;
			return LLUUID();
		}
	}
}

// support for secondlife:///app/appearance SLapps
class LLAppearanceHandler : public LLCommandHandler
{
public:
	// requests will be throttled from a non-trusted browser
	LLAppearanceHandler() : LLCommandHandler("appearance", UNTRUSTED_THROTTLE) {}

	bool handle(const LLSD& params, const LLSD& query_map, LLMediaCtrl* web)
	{
		// support secondlife:///app/appearance/show, but for now we just
		// make all secondlife:///app/appearance SLapps behave this way
		LLSideTray::getInstance()->showPanel("sidepanel_appearance", LLSD());
		return true;
	}
};
LLAppearanceHandler gAppearanceHandler;

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
		llinfos << "done all inventory callbacks" << llendl;
		
		// Is the destructor called by ordinary dereference, or because the app's shutting down?
		// If the inventory callback manager goes away, we're shutting down, no longer want the callback.
		if( LLInventoryCallbackManager::is_instantiated() )
		{
			LLAppearanceManager::instance().wearInventoryCategoryOnAvatar(gInventory.getCategory(mCatID), mAppend);
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
	virtual void done();
	void doWearCategory();

protected:
	LLUUID mCatID;
	bool mCopyItems;
	bool mAppend;
};

void LLOutfitObserver::done()
{
	llinfos << "done 2nd stage fetch" << llendl;
	gInventory.removeObserver(this);
	doOnIdle(boost::bind(&LLOutfitObserver::doWearCategory,this));
}

void LLOutfitObserver::doWearCategory()
{
	llinfos << "starting" << llendl;
	
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
					pid = gInventory.findCategoryUUIDForType(LLFolderType::FT_GESTURE);
				}
				else
				{
					pid = gInventory.findCategoryUUIDForType(LLFolderType::FT_CLOTHING);
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
			LLFolderType::FT_NONE,
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
		LLAppearanceManager::instance().wearInventoryCategoryOnAvatar(gInventory.getCategory(mCatID), mAppend);
	}
	delete this;
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
	llinfos << "done first stage fetch" << llendl;
	
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
	delete this;
}

LLUpdateAppearanceOnDestroy::LLUpdateAppearanceOnDestroy():
	mFireCount(0)
{
}

LLUpdateAppearanceOnDestroy::~LLUpdateAppearanceOnDestroy()
{
	llinfos << "done update appearance on destroy" << llendl;
	
	if (!LLApp::isExiting())
	{
		LLAppearanceManager::instance().updateAppearanceFromCOF();
	}
}

void LLUpdateAppearanceOnDestroy::fire(const LLUUID& inv_item)
{
	llinfos << "callback fired" << llendl;
	mFireCount++;
}

struct LLFoundData
{
	LLFoundData() :
		mAssetType(LLAssetType::AT_NONE),
		mWearableType(WT_INVALID),
		mWearable(NULL) {}

	LLFoundData(const LLUUID& item_id,
				const LLUUID& asset_id,
				const std::string& name,
				const LLAssetType::EType& asset_type,
				const EWearableType& wearable_type
		) :
		mItemID(item_id),
		mAssetID(asset_id),
		mName(name),
		mAssetType(asset_type),
		mWearableType(wearable_type),
		mWearable( NULL ) {}
	
	LLUUID mItemID;
	LLUUID mAssetID;
	std::string mName;
	LLAssetType::EType mAssetType;
	EWearableType mWearableType;
	LLWearable* mWearable;
};

	
class LLWearableHoldingPattern
{
public:
	LLWearableHoldingPattern();
	~LLWearableHoldingPattern();

	bool pollFetchCompletion();
	void onFetchCompletion();
	bool isFetchCompleted();
	bool isTimedOut();

	void checkMissingWearables();
	bool pollMissingWearables();
	bool isMissingCompleted();
	void recoverMissingWearable(EWearableType type);
	void clearCOFLinksForMissingWearables();
	
	void onAllComplete();
	
	typedef std::list<LLFoundData> found_list_t;
	found_list_t mFoundList;
	LLInventoryModel::item_array_t mObjItems;
	LLInventoryModel::item_array_t mGestItems;
	typedef std::set<S32> type_set_t;
	type_set_t mTypesToRecover;
	type_set_t mTypesToLink;
	S32 mResolved;
	LLTimer mWaitTime;
	bool mFired;
};

LLWearableHoldingPattern::LLWearableHoldingPattern():
	mResolved(0),
	mFired(false)
{
}

LLWearableHoldingPattern::~LLWearableHoldingPattern()
{
}

bool LLWearableHoldingPattern::isFetchCompleted()
{
	return (mResolved >= (S32)mFoundList.size()); // have everything we were waiting for?
}

bool LLWearableHoldingPattern::isTimedOut()
{
	static F32 max_wait_time = 60.0;  // give up if wearable fetches haven't completed in max_wait_time seconds.
	return mWaitTime.getElapsedTimeF32() > max_wait_time; 
}

void LLWearableHoldingPattern::checkMissingWearables()
{
	std::vector<S32> found_by_type(WT_COUNT,0);
	std::vector<S32> requested_by_type(WT_COUNT,0);
	for (found_list_t::iterator it = mFoundList.begin(); it != mFoundList.end(); ++it)
	{
		LLFoundData &data = *it;
		if (data.mWearableType < WT_COUNT)
			requested_by_type[data.mWearableType]++;
		if (data.mWearable)
			found_by_type[data.mWearableType]++;
	}

	for (S32 type = 0; type < WT_COUNT; ++type)
	{
		llinfos << "type " << type << " requested " << requested_by_type[type] << " found " << found_by_type[type] << llendl;
		if (found_by_type[type] > 0)
			continue;
		if (
			// Need to recover if at least one wearable of that type
			// was requested but none was found (prevent missing
			// pants)
			(requested_by_type[type] > 0) ||  
			// or if type is a body part and no wearables were found.
			((type == WT_SHAPE) || (type == WT_SKIN) || (type == WT_HAIR) || (type == WT_EYES)))
		{
			mTypesToRecover.insert(type);
			mTypesToLink.insert(type);
			recoverMissingWearable((EWearableType)type);
			llwarns << "need to replace " << type << llendl; 
		}
	}

	mWaitTime.reset();
	if (!pollMissingWearables())
	{
		doOnIdleRepeating(boost::bind(&LLWearableHoldingPattern::pollMissingWearables,this));
	}
}

void LLWearableHoldingPattern::onAllComplete()
{
	// Activate all gestures in this folder
	if (mGestItems.count() > 0)
	{
		llinfos << "Activating " << mGestItems.count() << " gestures" << llendl;
		
		LLGestureManager::instance().activateGestures(mGestItems);
		
		// Update the inventory item labels to reflect the fact
		// they are active.
		LLViewerInventoryCategory* catp =
			gInventory.getCategory(LLAppearanceManager::instance().getCOF());
		
		if (catp)
		{
			gInventory.updateCategory(catp);
			gInventory.notifyObservers();
		}
	}

	// Update wearables.
	llinfos << "Updating agent wearables with " << mResolved << " wearable items " << llendl;
	LLAppearanceManager::instance().updateAgentWearables(this, false);
	
	// Update attachments to match those requested.
	LLVOAvatar* avatar = gAgent.getAvatarObject();
	if( avatar )
	{
		llinfos << "Updating " << mObjItems.count() << " attachments" << llendl;
		LLAgentWearables::userUpdateAttachments(mObjItems);
	}

	if (isFetchCompleted() && isMissingCompleted())
	{
		// Only safe to delete if all wearable callbacks and all missing wearables completed.
		delete this;
	}
}

void LLWearableHoldingPattern::onFetchCompletion()
{
	checkMissingWearables();
}

// Runs as an idle callback until all wearables are fetched (or we time out).
bool LLWearableHoldingPattern::pollFetchCompletion()
{
	bool completed = isFetchCompleted();
	bool timed_out = isTimedOut();
	bool done = completed || timed_out;

	if (done)
	{
		llinfos << "polling, done status: " << completed << " timed out " << timed_out
				<< " elapsed " << mWaitTime.getElapsedTimeF32() << llendl;

		mFired = true;
		
		if (timed_out)
		{
			llwarns << "Exceeded max wait time for wearables, updating appearance based on what has arrived" << llendl;
		}

		onFetchCompletion();
	}
	return done;
}

class RecoveredItemLinkCB: public LLInventoryCallback
{
public:
	RecoveredItemLinkCB(EWearableType type, LLWearable *wearable, LLWearableHoldingPattern* holder):
		mHolder(holder),
		mWearable(wearable),
		mType(type)
	{
	}
	void fire(const LLUUID& item_id)
	{
		llinfos << "Recovered item link for type " << mType << llendl;
		mHolder->mTypesToLink.erase(mType);
		// Add wearable to FoundData for actual wearing
		LLViewerInventoryItem *item = gInventory.getItem(item_id);
		LLViewerInventoryItem *linked_item = item ? item->getLinkedItem() : NULL;

		if (linked_item)
		{
			gInventory.addChangedMask(LLInventoryObserver::LABEL, linked_item->getUUID());
			
			if (item)
			{
				LLFoundData found(linked_item->getUUID(),
						  linked_item->getAssetUUID(),
						  linked_item->getName(),
						  linked_item->getType(),
						  linked_item->isWearableType() ? linked_item->getWearableType() : WT_INVALID
						  );
				found.mWearable = mWearable;
				mHolder->mFoundList.push_front(found);
			}
			else
			{
				llwarns << "inventory item not found for recovered wearable" << llendl;
			}
		}
		else
		{
			llwarns << "inventory link not found for recovered wearable" << llendl;
		}
	}
private:
	LLWearableHoldingPattern* mHolder;
	LLWearable *mWearable;
	EWearableType mType;
};

class RecoveredItemCB: public LLInventoryCallback
{
public:
	RecoveredItemCB(EWearableType type, LLWearable *wearable, LLWearableHoldingPattern* holder):
		mHolder(holder),
		mWearable(wearable),
		mType(type)
	{
	}
	void fire(const LLUUID& item_id)
	{
		llinfos << "Recovered item for type " << mType << llendl;
		LLViewerInventoryItem *itemp = gInventory.getItem(item_id);
		mWearable->setItemID(item_id);
		LLPointer<LLInventoryCallback> cb = new RecoveredItemLinkCB(mType,mWearable,mHolder);
		mHolder->mTypesToRecover.erase(mType);
		llassert(itemp);
		if (itemp)
		{
			link_inventory_item( gAgent.getID(),
					     item_id,
					     LLAppearanceManager::instance().getCOF(),
					     itemp->getName(),
					     LLAssetType::AT_LINK,
					     cb);
		}
	}
private:
	LLWearableHoldingPattern* mHolder;
	LLWearable *mWearable;
	EWearableType mType;
};

void LLWearableHoldingPattern::recoverMissingWearable(EWearableType type)
{
		// Try to recover by replacing missing wearable with a new one.
	LLNotificationsUtil::add("ReplacedMissingWearable");
	lldebugs << "Wearable " << LLWearableDictionary::getTypeLabel(type)
			 << " could not be downloaded.  Replaced inventory item with default wearable." << llendl;
	LLWearable* wearable = LLWearableList::instance().createNewWearable(type);

	// Add a new one in the lost and found folder.
	const LLUUID lost_and_found_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_LOST_AND_FOUND);
	LLPointer<LLInventoryCallback> cb = new RecoveredItemCB(type,wearable,this);

	create_inventory_item(gAgent.getID(),
						  gAgent.getSessionID(),
						  lost_and_found_id,
						  wearable->getTransactionID(),
						  wearable->getName(),
						  wearable->getDescription(),
						  wearable->getAssetType(),
						  LLInventoryType::IT_WEARABLE,
						  wearable->getType(),
						  wearable->getPermissions().getMaskNextOwner(),
						  cb);
}

bool LLWearableHoldingPattern::isMissingCompleted()
{
	return mTypesToLink.size()==0 && mTypesToRecover.size()==0;
}

void LLWearableHoldingPattern::clearCOFLinksForMissingWearables()
{
	for (found_list_t::iterator it = mFoundList.begin(); it != mFoundList.end(); ++it)
	{
		LLFoundData &data = *it;
		if ((data.mWearableType < WT_COUNT) && (!data.mWearable))
		{
			// Wearable link that was never resolved; remove links to it from COF
			llinfos << "removing link for unresolved item " << data.mItemID.asString() << llendl;
			LLAppearanceManager::instance().removeCOFItemLinks(data.mItemID,false);
		}
	}
}

bool LLWearableHoldingPattern::pollMissingWearables()
{
	bool timed_out = isTimedOut();
	bool missing_completed = isMissingCompleted();
	bool done = timed_out || missing_completed;
	
	llinfos << "polling missing wearables, waiting for items " << mTypesToRecover.size()
			<< " links " << mTypesToLink.size()
			<< " wearables, timed out " << timed_out
			<< " elapsed " << mWaitTime.getElapsedTimeF32()
			<< " done " << done << llendl;

	if (done)
	{
		clearCOFLinksForMissingWearables();
		onAllComplete();
	}
	return done;
}

static void onWearableAssetFetch(LLWearable* wearable, void* data)
{
	LLWearableHoldingPattern* holder = (LLWearableHoldingPattern*)data;
	holder->mResolved += 1;  // just counting callbacks, not successes.
	llinfos << "onWearableAssetFetch, resolved count " << holder->mResolved << " of requested " << holder->mFoundList.size() << llendl;
	if (wearable)
	{
		llinfos << "wearable found, type " << wearable->getType() << " asset " << wearable->getAssetID() << llendl;
	}
	else
	{
		llwarns << "no wearable found" << llendl;
	}

	if (holder->mFired)
	{
		llwarns << "called after holder fired" << llendl;
		return;
	}

	if (!wearable)
	{
		return;
	}

	for (LLWearableHoldingPattern::found_list_t::iterator iter = holder->mFoundList.begin();
		 iter != holder->mFoundList.end(); ++iter)
	{
		LLFoundData& data = *iter;
		if(wearable->getAssetID() == data.mAssetID)
		{
			data.mWearable = wearable;
			// Failing this means inventory or asset server are corrupted in a way we don't handle.
			llassert((data.mWearableType < WT_COUNT) && (wearable->getType() == data.mWearableType));
			break;
		}
	}
}


static void removeDuplicateItems(LLInventoryModel::item_array_t& items)
{
	LLInventoryModel::item_array_t new_items;
	std::set<LLUUID> items_seen;
	std::deque<LLViewerInventoryItem*> tmp_list;
	// Traverse from the front and keep the first of each item
	// encountered, so we actually keep the *last* of each duplicate
	// item.  This is needed to give the right priority when adding
	// duplicate items to an existing outfit.
	for (S32 i=items.count()-1; i>=0; i--)
	{
		LLViewerInventoryItem *item = items.get(i);
		LLUUID item_id = item->getLinkedUUID();
		if (items_seen.find(item_id)!=items_seen.end())
			continue;
		items_seen.insert(item_id);
		tmp_list.push_front(item);
	}
	for (std::deque<LLViewerInventoryItem*>::iterator it = tmp_list.begin();
		 it != tmp_list.end();
		 ++it)
	{
		new_items.put(*it);
	}
	items = new_items;
}

const LLUUID LLAppearanceManager::getCOF() const
{
	return gInventory.findCategoryUUIDForType(LLFolderType::FT_CURRENT_OUTFIT);
}


const LLViewerInventoryItem* LLAppearanceManager::getBaseOutfitLink()
{
	const LLUUID& current_outfit_cat = getCOF();
	LLInventoryModel::cat_array_t cat_array;
	LLInventoryModel::item_array_t item_array;
	// Can't search on FT_OUTFIT since links to categories return FT_CATEGORY for type since they don't
	// return preferred type.
	LLIsType is_category( LLAssetType::AT_CATEGORY ); 
	gInventory.collectDescendentsIf(current_outfit_cat,
									cat_array,
									item_array,
									false,
									is_category,
									false);
	for (LLInventoryModel::item_array_t::const_iterator iter = item_array.begin();
		 iter != item_array.end();
		 iter++)
	{
		const LLViewerInventoryItem *item = (*iter);
		const LLViewerInventoryCategory *cat = item->getLinkedCategory();
		if (cat && cat->getPreferredType() == LLFolderType::FT_OUTFIT)
		{
			return item;
		}
	}
	return NULL;
}

bool LLAppearanceManager::getBaseOutfitName(std::string& name)
{
	const LLViewerInventoryItem* outfit_link = getBaseOutfitLink();
	if(outfit_link)
	{
		const LLViewerInventoryCategory *cat = outfit_link->getLinkedCategory();
		if (cat)
		{
			name = cat->getName();
			return true;
		}
	}
	return false;
}

// Update appearance from outfit folder.
void LLAppearanceManager::changeOutfit(bool proceed, const LLUUID& category, bool append)
{
	if (!proceed)
		return;
	LLAppearanceManager::instance().updateCOF(category,append);
}

// Create a copy of src_id + contents as a subfolder of dst_id.
void LLAppearanceManager::shallowCopyCategory(const LLUUID& src_id, const LLUUID& dst_id,
											  LLPointer<LLInventoryCallback> cb)
{
	LLInventoryCategory *src_cat = gInventory.getCategory(src_id);
	if (!src_cat)
	{
		llwarns << "folder not found for src " << src_id.asString() << llendl;
		return;
	}
	LLUUID parent_id = dst_id;
	if(parent_id.isNull())
	{
		parent_id = gInventory.getRootFolderID();
	}
	LLUUID subfolder_id = gInventory.createNewCategory( parent_id,
														LLFolderType::FT_NONE,
														src_cat->getName());
	shallowCopyCategoryContents(src_id, subfolder_id, cb);

	gInventory.notifyObservers();
}

// Copy contents of src_id to dst_id.
void LLAppearanceManager::shallowCopyCategoryContents(const LLUUID& src_id, const LLUUID& dst_id,
													  LLPointer<LLInventoryCallback> cb)
{
	LLInventoryModel::cat_array_t* cats;
	LLInventoryModel::item_array_t* items;
	gInventory.getDirectDescendentsOf(src_id, cats, items);
	for (LLInventoryModel::item_array_t::const_iterator iter = items->begin();
		 iter != items->end();
		 ++iter)
	{
		const LLViewerInventoryItem* item = (*iter);
		switch (item->getActualType())
		{
			case LLAssetType::AT_LINK:
			{
				link_inventory_item(gAgent.getID(),
									item->getLinkedUUID(),
									dst_id,
									item->getName(),
									LLAssetType::AT_LINK, cb);
				break;
			}
			case LLAssetType::AT_LINK_FOLDER:
			{
				LLViewerInventoryCategory *catp = item->getLinkedCategory();
				// Skip copying outfit links.
				if (catp && catp->getPreferredType() != LLFolderType::FT_OUTFIT)
				{
					link_inventory_item(gAgent.getID(),
										item->getLinkedUUID(),
										dst_id,
										item->getName(),
										LLAssetType::AT_LINK_FOLDER, cb);
				}
				break;
			}
			case LLAssetType::AT_CLOTHING:
			case LLAssetType::AT_OBJECT:
			case LLAssetType::AT_BODYPART:
			case LLAssetType::AT_GESTURE:
			{
				copy_inventory_item(gAgent.getID(),
									item->getPermissions().getOwner(),
									item->getUUID(),
									dst_id,
									item->getName(),
									cb);
				break;
			}
			default:
				// Ignore non-outfit asset types
				break;
		}
	}
}

BOOL LLAppearanceManager::getCanMakeFolderIntoOutfit(const LLUUID& folder_id)
{
	// These are the wearable items that are required for considering this
	// folder as containing a complete outfit.
	U32 required_wearables = 0;
	required_wearables |= 1LL << WT_SHAPE;
	required_wearables |= 1LL << WT_SKIN;
	required_wearables |= 1LL << WT_HAIR;
	required_wearables |= 1LL << WT_EYES;

	// These are the wearables that the folder actually contains.
	U32 folder_wearables = 0;
	LLInventoryModel::cat_array_t* cats;
	LLInventoryModel::item_array_t* items;
	gInventory.getDirectDescendentsOf(folder_id, cats, items);
	for (LLInventoryModel::item_array_t::const_iterator iter = items->begin();
		 iter != items->end();
		 ++iter)
	{
		const LLViewerInventoryItem* item = (*iter);
		if (item->isWearableType())
		{
			const EWearableType wearable_type = item->getWearableType();
			folder_wearables |= 1LL << wearable_type;
		}
	}

	// If the folder contains the required wearables, return TRUE.
	return ((required_wearables & folder_wearables) == required_wearables);
}


void LLAppearanceManager::purgeBaseOutfitLink(const LLUUID& category)
{
	LLInventoryModel::cat_array_t cats;
	LLInventoryModel::item_array_t items;
	gInventory.collectDescendents(category, cats, items,
								  LLInventoryModel::EXCLUDE_TRASH);
	for (S32 i = 0; i < items.count(); ++i)
	{
		LLViewerInventoryItem *item = items.get(i);
		if (item->getActualType() != LLAssetType::AT_LINK_FOLDER)
			continue;
		if (item->getIsLinkType())
		{
			LLViewerInventoryCategory* catp = item->getLinkedCategory();
			if(catp && catp->getPreferredType() == LLFolderType::FT_OUTFIT)
			{
				gInventory.purgeObject(item->getUUID());
			}
		}
	}
}

void LLAppearanceManager::purgeCategory(const LLUUID& category, bool keep_outfit_links)
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
		if (item->getIsLinkType())
		{
			gInventory.purgeObject(item->getUUID());
		}
	}
}

// Keep the last N wearables of each type.  For viewer 2.0, N is 1 for
// both body parts and clothing items.
void LLAppearanceManager::filterWearableItems(
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
		if(type < 0 || type >= WT_COUNT)
		{
			LL_WARNS("Appearance") << "Invalid wearable type. Inventory type does not match wearable flag bitfield." << LL_ENDL;
			continue;
		}
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
void LLAppearanceManager::linkAll(const LLUUID& category,
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

void LLAppearanceManager::updateCOF(const LLUUID& category, bool append)
{
	LLViewerInventoryCategory *pcat = gInventory.getCategory(category);
	llinfos << "starting, cat " << (pcat ? pcat->getName() : "[UNKNOWN]") << llendl;

	const LLUUID cof = getCOF();

	// Deactivate currently active gestures in the COF, if replacing outfit
	if (!append)
	{
		LLInventoryModel::item_array_t gest_items;
		getDescendentsOfAssetType(cof, gest_items, LLAssetType::AT_GESTURE, false);
		for(S32 i = 0; i  < gest_items.count(); ++i)
		{
			LLViewerInventoryItem *gest_item = gest_items.get(i);
			if ( LLGestureManager::instance().isGestureActive( gest_item->getLinkedUUID()) )
			{
				LLGestureManager::instance().deactivateGesture( gest_item->getLinkedUUID() );
			}
		}
	}
	
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
	llinfos << "creating LLUpdateAppearanceOnDestroy" << llendl;
	LLPointer<LLInventoryCallback> link_waiter = new LLUpdateAppearanceOnDestroy;

	linkAll(cof, body_items, link_waiter);
	linkAll(cof, wear_items, link_waiter);
	linkAll(cof, obj_items, link_waiter);
	linkAll(cof, gest_items, link_waiter);

	// Add link to outfit if category is an outfit. 
	if (!append)
	{
		createBaseOutfitLink(category, link_waiter);
	}
	llinfos << "waiting for LLUpdateAppearanceOnDestroy" << llendl;
}

void LLAppearanceManager::updatePanelOutfitName(const std::string& name)
{
	LLSidepanelAppearance* panel_appearance =
		dynamic_cast<LLSidepanelAppearance *>(LLSideTray::getInstance()->getPanel("sidepanel_appearance"));
	if (panel_appearance)
	{
		panel_appearance->refreshCurrentOutfitName(name);
	}
}

void LLAppearanceManager::createBaseOutfitLink(const LLUUID& category, LLPointer<LLInventoryCallback> link_waiter)
{
	const LLUUID cof = getCOF();
	LLViewerInventoryCategory* catp = gInventory.getCategory(category);
	std::string new_outfit_name = "";

	purgeBaseOutfitLink(cof);

	if (catp && catp->getPreferredType() == LLFolderType::FT_OUTFIT)
	{
		link_inventory_item(gAgent.getID(), category, cof, catp->getName(),
							LLAssetType::AT_LINK_FOLDER, link_waiter);
		new_outfit_name = catp->getName();
	}
	
	updatePanelOutfitName(new_outfit_name);
}

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
			LLFoundData& data = *iter;
			LLWearable* wearable = data.mWearable;
			if( wearable && ((S32)wearable->getType() == i) )
			{
				LLViewerInventoryItem* item;
				item = (LLViewerInventoryItem*)gInventory.getItem(data.mItemID);
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
	}

//	dec_busy_count();
}

static void remove_non_link_items(LLInventoryModel::item_array_t &items)
{
	LLInventoryModel::item_array_t pruned_items;
	for (LLInventoryModel::item_array_t::const_iterator iter = items.begin();
		 iter != items.end();
		 ++iter)
	{
 		const LLViewerInventoryItem *item = (*iter);
		if (item && item->getIsLinkType())
		{
			pruned_items.push_back((*iter));
		}
	}
	items = pruned_items;
}

void LLAppearanceManager::updateAppearanceFromCOF()
{
	// update dirty flag to see if the state of the COF matches
	// the saved outfit stored as a folder link
	llinfos << "starting" << llendl;

	updateIsDirty();

	dumpCat(getCOF(),"COF, start");

	bool follow_folder_links = true;
	LLUUID current_outfit_id = getCOF();

	// Find all the wearables that are in the COF's subtree.
	lldebugs << "LLAppearanceManager::updateFromCOF()" << llendl;
	LLInventoryModel::item_array_t wear_items;
	LLInventoryModel::item_array_t obj_items;
	LLInventoryModel::item_array_t gest_items;
	getUserDescendents(current_outfit_id, wear_items, obj_items, gest_items, follow_folder_links);
	// Get rid of non-links in case somehow the COF was corrupted.
	remove_non_link_items(wear_items);
	remove_non_link_items(obj_items);
	remove_non_link_items(gest_items);

	if(!wear_items.count())
	{
		LLNotificationsUtil::add("CouldNotPutOnOutfit");
		return;
	}

	LLWearableHoldingPattern* holder = new LLWearableHoldingPattern;

	holder->mObjItems = obj_items;
	holder->mGestItems = gest_items;
		
	// Note: can't do normal iteration, because if all the
	// wearables can be resolved immediately, then the
	// callback will be called (and this object deleted)
	// before the final getNextData().

	// BAP future cleanup - no point having found_container when
	// mFoundList already has all the info.
	LLDynamicArray<LLFoundData> found_container;
	for(S32 i = 0; i  < wear_items.count(); ++i)
	{
		LLViewerInventoryItem *item = wear_items.get(i);
		LLViewerInventoryItem *linked_item = item ? item->getLinkedItem() : NULL;
		if (item && item->getIsLinkType() && linked_item)
		{
			LLFoundData found(linked_item->getUUID(),
							  linked_item->getAssetUUID(),
							  linked_item->getName(),
							  linked_item->getType(),
							  linked_item->isWearableType() ? linked_item->getWearableType() : WT_INVALID
				);

#if 0
			// Fault injection: uncomment this block to test asset
			// fetch failures (should be replaced by new defaults in
			// lost&found).
			if (found.mWearableType == WT_SHAPE || found.mWearableType == WT_JACKET)
			{
				found.mAssetID.generate(); // Replace with new UUID, guaranteed not to exist in DB
				
			}
#endif

			holder->mFoundList.push_front(found);
			found_container.put(found);
		}
		else
		{
			if (!item)
			{
				llwarns << "Attempt to wear a null item " << llendl;
			}
			else if (!linked_item)
			{
				llwarns << "Attempt to wear a broken link [ name:" << item->getName() << " ] " << llendl;
			}
		}
	}

	for(S32 i = 0; i < found_container.count(); ++i)
	{
		LLFoundData& found = found_container.get(i);

		llinfos << "waiting for onWearableAssetFetch callback, asset " << found.mAssetID.asString() << llendl;

		// Fetch the wearables about to be worn.
		LLWearableList::instance().getAsset(found.mAssetID,
											found.mName,
											found.mAssetType,
											onWearableAssetFetch,
											(void*)holder);

	}

	if (!holder->pollFetchCompletion())
	{
		doOnIdleRepeating(boost::bind(&LLWearableHoldingPattern::pollFetchCompletion,holder));
	}

}

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

	llinfos << "wearInventoryCategory( " << category->getName()
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
void LLAppearanceManager::wearInventoryCategoryOnAvatar( LLInventoryCategory* category, bool append )
{
	// Avoid unintentionally overwriting old wearables.  We have to do
	// this up front to avoid having to deal with the case of multiple
	// wearables being dirty.
	if(!category) return;

	llinfos << "wearInventoryCategoryOnAvatar( " << category->getName()
			 << " )" << llendl;
			 	
	if( gFloaterCustomize )
	{
		gFloaterCustomize->askToSaveIfDirty(boost::bind(&LLAppearanceManager::changeOutfit,
														&LLAppearanceManager::instance(),
														_1, category->getUUID(), append));
	}
	else
	{
		LLAppearanceManager::changeOutfit(TRUE, category->getUUID(), append);
	}
}

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

bool areMatchingWearables(const LLViewerInventoryItem *a, const LLViewerInventoryItem *b)
{
	return (a->isWearableType() && b->isWearableType() &&
			(a->getWearableType() == b->getWearableType()));
}

class LLDeferredCOFLinkObserver: public LLInventoryObserver
{
public:
	LLDeferredCOFLinkObserver(const LLUUID& item_id, bool do_update):
		mItemID(item_id),
		mDoUpdate(do_update)
	{
	}

	~LLDeferredCOFLinkObserver()
	{
	}
	
	/* virtual */ void changed(U32 mask)
	{
		const LLInventoryItem *item = gInventory.getItem(mItemID);
		if (item)
		{
			gInventory.removeObserver(this);
			LLAppearanceManager::instance().addCOFItemLink(item,mDoUpdate);
			delete this;
		}
	}

private:
	const LLUUID mItemID;
	bool mDoUpdate;
};


void LLAppearanceManager::addCOFItemLink(const LLUUID &item_id, bool do_update )
{
	const LLInventoryItem *item = gInventory.getItem(item_id);
	if (!item)
	{
		LLDeferredCOFLinkObserver *observer = new LLDeferredCOFLinkObserver(item_id, do_update);
		gInventory.addObserver(observer);
	}
	else
	{
		addCOFItemLink(item, do_update);
	}
}

void LLAppearanceManager::addCOFItemLink(const LLInventoryItem *item, bool do_update )
{		
	const LLViewerInventoryItem *vitem = dynamic_cast<const LLViewerInventoryItem*>(item);
	if (!vitem)
	{
		llwarns << "not an llviewerinventoryitem, failed" << llendl;
		return;
	}

	gInventory.addChangedMask(LLInventoryObserver::LABEL, vitem->getLinkedUUID());

	LLInventoryModel::cat_array_t cat_array;
	LLInventoryModel::item_array_t item_array;
	gInventory.collectDescendents(LLAppearanceManager::getCOF(),
								  cat_array,
								  item_array,
								  LLInventoryModel::EXCLUDE_TRASH);
	bool linked_already = false;
	for (S32 i=0; i<item_array.count(); i++)
	{
		// Are these links to the same object?
		const LLViewerInventoryItem* inv_item = item_array.get(i).get();
		if (inv_item->getLinkedUUID() == vitem->getLinkedUUID())
		{
			linked_already = true;
		}
		// Are these links to different items of the same wearable
		// type? If so, new item will replace old.
		// MULTI-WEARABLES: revisit if more than one per type is allowed.
		else if (areMatchingWearables(vitem,inv_item))
		{
			if (inv_item->getIsLinkType())
			{
				gInventory.purgeObject(inv_item->getUUID());
			}
		}
	}
	if (linked_already)
	{
		if (do_update)
		{	
			LLAppearanceManager::updateAppearanceFromCOF();
		}
		return;
	}
	else
	{
		LLPointer<LLInventoryCallback> cb = do_update ? new ModifiedCOFCallback : 0;
		link_inventory_item( gAgent.getID(),
							 vitem->getLinkedUUID(),
							 getCOF(),
							 vitem->getName(),
							 LLAssetType::AT_LINK,
							 cb);
	}
	return;
}

void LLAppearanceManager::addEnsembleLink( LLInventoryCategory* cat, bool do_update )
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

void LLAppearanceManager::removeCOFItemLinks(const LLUUID& item_id, bool do_update)
{
	gInventory.addChangedMask(LLInventoryObserver::LABEL, item_id);

	LLInventoryModel::cat_array_t cat_array;
	LLInventoryModel::item_array_t item_array;
	gInventory.collectDescendents(LLAppearanceManager::getCOF(),
								  cat_array,
								  item_array,
								  LLInventoryModel::EXCLUDE_TRASH);
	for (S32 i=0; i<item_array.count(); i++)
	{
		const LLInventoryItem* item = item_array.get(i).get();
		if (item->getIsLinkType() && item->getLinkedUUID() == item_id)
		{
			gInventory.purgeObject(item->getUUID());
		}
	}
	if (do_update)
	{
		LLAppearanceManager::updateAppearanceFromCOF();
	}
}

void LLAppearanceManager::updateIsDirty()
{
	LLUUID cof = getCOF();
	LLUUID base_outfit;

	// find base outfit link 
	const LLViewerInventoryItem* base_outfit_item = getBaseOutfitLink();
	LLViewerInventoryCategory* catp = NULL;
	if (base_outfit_item && base_outfit_item->getIsLinkType())
	{
		catp = base_outfit_item->getLinkedCategory();
	}
	if(catp && catp->getPreferredType() == LLFolderType::FT_OUTFIT)
	{
		base_outfit = catp->getUUID();
	}

	if(base_outfit.isNull())
	{
		// no outfit link found, display "unsaved outfit"
		mOutfitIsDirty = true;
	}
	else
	{
		LLInventoryModel::cat_array_t cof_cats;
		LLInventoryModel::item_array_t cof_items;
		gInventory.collectDescendents(cof, cof_cats, cof_items,
									  LLInventoryModel::EXCLUDE_TRASH);

		LLInventoryModel::cat_array_t outfit_cats;
		LLInventoryModel::item_array_t outfit_items;
		gInventory.collectDescendents(base_outfit, outfit_cats, outfit_items,
									  LLInventoryModel::EXCLUDE_TRASH);

		if(outfit_items.count() != cof_items.count() -1)
		{
			// Current outfit folder should have one more item than the outfit folder.
			// this one item is the link back to the outfit folder itself.
			mOutfitIsDirty = true;
		}
		else
		{
			typedef std::set<LLUUID> item_set_t;
			item_set_t cof_set;
			item_set_t outfit_set;

			// sort COF items by UUID
			for (S32 i = 0; i < cof_items.count(); ++i)
			{
				LLViewerInventoryItem *item = cof_items.get(i);
				// don't add the base outfit link to the list of objects we're comparing
				if(item != base_outfit_item)
				{
					cof_set.insert(item->getLinkedUUID());
				}
			}

			// sort outfit folder by UUID
			for (S32 i = 0; i < outfit_items.count(); ++i)
			{
				LLViewerInventoryItem *item = outfit_items.get(i);
				outfit_set.insert(item->getLinkedUUID());
			}

			mOutfitIsDirty = (outfit_set != cof_set);
		}
	}
}

void LLAppearanceManager::onFirstFullyVisible()
{
	// If this is the very first time the user has logged into viewer2+ (from a legacy viewer, or new account)
	// then auto-populate outfits from the library into the My Outfits folder.

	llinfos << "avatar fully visible" << llendl;

	static bool check_populate_my_outfits = true;
	if (check_populate_my_outfits && 
		(LLInventoryModel::getIsFirstTimeInViewer2() 
		 || gSavedSettings.getBOOL("MyOutfitsAutofill")))
	{
		gAgentWearables.populateMyOutfitsFolder();
	}
	check_populate_my_outfits = false;
}

//#define DUMP_CAT_VERBOSE

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

LLAppearanceManager::LLAppearanceManager():
	mAttachmentInvLinkEnabled(false),
	mOutfitIsDirty(false)
{
}

LLAppearanceManager::~LLAppearanceManager()
{
}

void LLAppearanceManager::setAttachmentInvLinkEnable(bool val)
{
	llinfos << "setAttachmentInvLinkEnable => " << (int) val << llendl;
	mAttachmentInvLinkEnabled = val;
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

void LLAppearanceManager::registerAttachment(const LLUUID& item_id)
{
       mRegisteredAttachments.insert(item_id);
	   gInventory.addChangedMask(LLInventoryObserver::LABEL, item_id);
       //dumpAttachmentSet(mRegisteredAttachments,"after register:");

	   if (mAttachmentInvLinkEnabled)
	   {
		   LLAppearanceManager::addCOFItemLink(item_id, false);  // Add COF link for item.
	   }
	   else
	   {
		   //llinfos << "no link changes, inv link not enabled" << llendl;
	   }
}

void LLAppearanceManager::unregisterAttachment(const LLUUID& item_id)
{
       mRegisteredAttachments.erase(item_id);
	   gInventory.addChangedMask(LLInventoryObserver::LABEL, item_id);

       //dumpAttachmentSet(mRegisteredAttachments,"after unregister:");

	   if (mAttachmentInvLinkEnabled)
	   {
		   //LLAppearanceManager::dumpCat(LLAppearanceManager::getCOF(),"Removing attachment link:");
		   LLAppearanceManager::removeCOFItemLinks(item_id, false);
	   }
	   else
	   {
		   //llinfos << "no link changes, inv link not enabled" << llendl;
	   }
}

void LLAppearanceManager::linkRegisteredAttachments()
{
	for (std::set<LLUUID>::iterator it = mRegisteredAttachments.begin();
		 it != mRegisteredAttachments.end();
		 ++it)
	{
		LLUUID item_id = *it;
		addCOFItemLink(item_id, false);
	}
	mRegisteredAttachments.clear();
}

BOOL LLAppearanceManager::getIsInCOF(const LLUUID& obj_id) const
{
	return gInventory.isObjectDescendentOf(obj_id, getCOF());
}

BOOL LLAppearanceManager::getIsProtectedCOFItem(const LLUUID& obj_id) const
{
	if (!getIsInCOF(obj_id)) return FALSE;

	// If a non-link somehow ended up in COF, allow deletion.
	const LLInventoryObject *obj = gInventory.getObject(obj_id);
	if (obj && !obj->getIsLinkType())
	{
		return FALSE;
	}

	// For now, don't allow direct deletion from the COF.  Instead, force users
	// to choose "Detach" or "Take Off".
	return TRUE;
	/*
	const LLInventoryObject *obj = gInventory.getObject(obj_id);
	if (!obj) return FALSE;

	// Can't delete bodyparts, since this would be equivalent to removing the item.
	if (obj->getType() == LLAssetType::AT_BODYPART) return TRUE;

	// Can't delete the folder link, since this is saved for bookkeeping.
	if (obj->getActualType() == LLAssetType::AT_LINK_FOLDER) return TRUE;

	return FALSE;
	*/
}
