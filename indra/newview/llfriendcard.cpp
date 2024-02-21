/** 
 * @file llfriendcard.cpp
 * @brief Implementation of classes to process Friends Cards
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

#include "llfriendcard.h"

#include "llagent.h"
#include "llavatarnamecache.h"
#include "llinventory.h"
#include "llinventoryfunctions.h"
#include "llinventoryobserver.h"
#include "lltrans.h"

#include "llcallingcard.h" // for LLAvatarTracker
#include "llviewerinventory.h"
#include "llinventorymodel.h"
#include "llcallbacklist.h"

// Constants;

static const std::string INVENTORY_STRING_FRIENDS_SUBFOLDER = "Friends";
static const std::string INVENTORY_STRING_FRIENDS_ALL_SUBFOLDER = "All";

// helper functions

// NOTE: For now Friends & All folders are created as protected folders of the LLFolderType::FT_CALLINGCARD type.
// So, their names will be processed in the LLFolderViewItem::refresh() to be localized
// using "InvFolder LABEL_NAME" as LLTrans::findString argument.

// We must use in this file their hard-coded names to ensure found them on different locales. EXT-5829.
// These hard-coded names will be stored in InventoryItems but shown localized in FolderViewItems

// If hack in the LLFolderViewItem::refresh() to localize protected folder is removed
// or these folders are not protected these names should be localized in another place/way.
inline const std::string get_friend_folder_name()
{
	return INVENTORY_STRING_FRIENDS_SUBFOLDER;
}

inline const std::string get_friend_all_subfolder_name()
{
	return INVENTORY_STRING_FRIENDS_ALL_SUBFOLDER;
}

void move_from_to_arrays(LLInventoryModel::cat_array_t& from, LLInventoryModel::cat_array_t& to)
{
	while (from.size() > 0)
	{
		to.push_back(from.at(0));
		from.erase(from.begin());
	}
}

const LLUUID& get_folder_uuid(const LLUUID& parentFolderUUID, LLInventoryCollectFunctor& matchFunctor)
{
	LLInventoryModel::cat_array_t cats;
	LLInventoryModel::item_array_t items;

	gInventory.collectDescendentsIf(parentFolderUUID, cats, items,
		LLInventoryModel::EXCLUDE_TRASH, matchFunctor);

	S32 cats_count = cats.size();

	if (cats_count > 1)
	{
		LL_WARNS_ONCE("LLFriendCardsManager")
			<< "There is more than one Friend card folder."
			<< "The first folder will be used."
			<< LL_ENDL;
	}

	return (cats_count >= 1) ? cats.at(0)->getUUID() : LLUUID::null;
}

/**
 * Class LLFindAgentCallingCard
 *
 * An inventory collector functor for checking that agent's own calling card
 * exists within the Calling Cards category and its sub-folders.
 */
class LLFindAgentCallingCard : public LLInventoryCollectFunctor
{
public:
	LLFindAgentCallingCard() : mIsAgentCallingCardFound(false) {}
	virtual ~LLFindAgentCallingCard() {}
	virtual bool operator()(LLInventoryCategory* cat, LLInventoryItem* item);
	bool isAgentCallingCardFound() { return mIsAgentCallingCardFound; }

private:
	bool mIsAgentCallingCardFound;
};

bool LLFindAgentCallingCard::operator()(LLInventoryCategory* cat, LLInventoryItem* item)
{
	if (mIsAgentCallingCardFound) return true;

	if (item && item->getType() == LLAssetType::AT_CALLINGCARD && item->getCreatorUUID() == gAgentID)
	{
		mIsAgentCallingCardFound = true;
	}

	return mIsAgentCallingCardFound;
}

/**
 * Class for fetching initial friend cards data
 *
 * Implemented to fix an issue when Inventory folders are in incomplete state.
 * See EXT-2320, EXT-2061, EXT-1935, EXT-813.
 * Uses a callback to sync Inventory Friends/All folder with agent's Friends List.
 */
class LLInitialFriendCardsFetch : public LLInventoryFetchDescendentsObserver
{
public:
	typedef boost::function<void()> callback_t;

	LLInitialFriendCardsFetch(const LLUUID& folder_id,
							  callback_t cb) :
		LLInventoryFetchDescendentsObserver(folder_id),
		mCheckFolderCallback(cb)	
	{}

	/* virtual */ void done();

private:
	callback_t		mCheckFolderCallback;
};

void LLInitialFriendCardsFetch::done()
{
	// This observer is no longer needed.
	gInventory.removeObserver(this);

	doOnIdleOneTime(mCheckFolderCallback);

	delete this;
}

// LLFriendCardsManager Constructor / Destructor
LLFriendCardsManager::LLFriendCardsManager()
:   mState(INIT)
{
	LLAvatarTracker::instance().addObserver(this);
}

LLFriendCardsManager::~LLFriendCardsManager()
{
	LLAvatarTracker::instance().removeObserver(this);
}

void LLFriendCardsManager::putAvatarData(const LLUUID& avatarID)
{
	LL_INFOS() << "Store avatar data, avatarID: " << avatarID << LL_ENDL;
	std::pair< avatar_uuid_set_t::iterator, bool > pr;
	pr = mBuddyIDSet.insert(avatarID);
	if (pr.second == false)
	{
		LL_WARNS() << "Trying to add avatar UUID for the stored avatar: " 
			<< avatarID
			<< LL_ENDL;
	}
}

const LLUUID LLFriendCardsManager::extractAvatarID(const LLUUID& avatarID)
{
	LLUUID rv;
	avatar_uuid_set_t::iterator it = mBuddyIDSet.find(avatarID);
	if (mBuddyIDSet.end() == it)
	{
		LL_WARNS() << "Call method for non-existent avatar name in the map: " << avatarID << LL_ENDL;
	}
	else
	{
		rv = (*it);
		mBuddyIDSet.erase(it);
	}
	return rv;
}

bool LLFriendCardsManager::isItemInAnyFriendsList(const LLViewerInventoryItem* item)
{
	if (item->getType() != LLAssetType::AT_CALLINGCARD)
		return false;

	LLInventoryModel::item_array_t items;
	findMatchedFriendCards(item->getCreatorUUID(), items);

	return items.size() > 0;
}


bool LLFriendCardsManager::isObjDirectDescendentOfCategory(const LLInventoryObject* obj, 
	const LLViewerInventoryCategory* cat) const
{
	// we need both params to proceed.
	if ( !obj || !cat )
		return false;

	// Need to check that target category is in the Calling Card/Friends folder. 
	// In other case function returns unpredictable result.
	if ( !isCategoryInFriendFolder(cat) )
		return false;

	bool result = false;

	LLInventoryModel::item_array_t* items;
	LLInventoryModel::cat_array_t* cats;

	gInventory.lockDirectDescendentArrays(cat->getUUID(), cats, items);
	if ( items )
	{
		if ( obj->getType() == LLAssetType::AT_CALLINGCARD )
		{
			// For CALLINGCARD compare items by creator's id, if they are equal assume
			// that it is same card and return true. Note: UUID's of compared items
			// may be not equal. Also, we already know that obj should be type of LLInventoryItem,
			// but in case inventory database is broken check what dynamic_cast returns.
			const LLInventoryItem* item = dynamic_cast < const LLInventoryItem* > (obj);
			if ( item )
			{
				LLUUID creator_id = item->getCreatorUUID();
				LLViewerInventoryItem* cur_item = NULL;
				for ( S32 i = items->size() - 1; i >= 0; --i )
				{
					cur_item = items->at(i);
					if ( creator_id == cur_item->getCreatorUUID() )
					{
						result = true;
						break;
					}
				}
			}
		}
		else
		{
			// Else check that items have same type and name.
			// Note: UUID's of compared items also may be not equal.
			std::string obj_name = obj->getName();
			LLViewerInventoryItem* cur_item = NULL;
			for ( S32 i = items->size() - 1; i >= 0; --i )
			{
				cur_item = items->at(i);
				if ( obj->getType() != cur_item->getType() )
					continue;
				if ( obj_name == cur_item->getName() )
				{
					result = true;
					break;
				}
			}
		}
	}
	if ( !result && cats )
	{
		// There is no direct descendent in items, so check categories.
		// If target obj and descendent category have same type and name
		// then return true. Note: UUID's of compared items also may be not equal.
		std::string obj_name = obj->getName();
		LLViewerInventoryCategory* cur_cat = NULL;
		for ( S32 i = cats->size() - 1; i >= 0; --i )
		{
			cur_cat = cats->at(i);
			if ( obj->getType() != cur_cat->getType() )
				continue;
			if ( obj_name == cur_cat->getName() )
			{
				result = true;
				break;
			}
		}
	}
	gInventory.unlockDirectDescendentArrays(cat->getUUID());

	return result;
}


bool LLFriendCardsManager::isCategoryInFriendFolder(const LLViewerInventoryCategory* cat) const
{
	if (NULL == cat)
		return false;
	return true == gInventory.isObjectDescendentOf(cat->getUUID(), findFriendFolderUUIDImpl());
}

bool LLFriendCardsManager::isAnyFriendCategory(const LLUUID& catID) const
{
	const LLUUID& friendFolderID = findFriendFolderUUIDImpl();
	if (catID == friendFolderID)
		return true;

	return true == gInventory.isObjectDescendentOf(catID, friendFolderID);
}

void LLFriendCardsManager::syncFriendCardsFolders()
{
	const LLUUID callingCardsFolderID = gInventory.findCategoryUUIDForType(LLFolderType::FT_CALLINGCARD);

	fetchAndCheckFolderDescendents(callingCardsFolderID,
			boost::bind(&LLFriendCardsManager::ensureFriendsFolderExists, this));
}


/************************************************************************/
/*		Private Methods                                                 */
/************************************************************************/
const LLUUID& LLFriendCardsManager::findFirstCallingCardSubfolder(const LLUUID &parent_id) const
{
    if (parent_id.isNull())
    {
        return LLUUID::null;
    }

    LLInventoryModel::cat_array_t* cats;
    LLInventoryModel::item_array_t* items;
    gInventory.getDirectDescendentsOf(parent_id, cats, items);

    if (!cats || !items || cats->size() == 0)
    {
        // call failed
        return LLUUID::null;
    }

    if (cats->size() > 1)
    {
        const LLViewerInventoryCategory* friendFolder = gInventory.getCategory(parent_id);
        if (friendFolder)
        {
            LL_WARNS_ONCE() << friendFolder->getName() << " folder contains more than one folder" << LL_ENDL;
        }
    }

    for (LLInventoryModel::cat_array_t::const_iterator iter = cats->begin();
        iter != cats->end();
        ++iter)
    {
        const LLInventoryCategory* category = (*iter);
        if (category->getPreferredType() == LLFolderType::FT_CALLINGCARD)
        {
            return category->getUUID();
        }
    }

    return LLUUID::null;
}

// Inventorry ->
//   Calling Cards - >
//     Friends - > (the only expected folder)
//       All (the only expected folder)

const LLUUID& LLFriendCardsManager::findFriendFolderUUIDImpl() const
{
    const LLUUID callingCardsFolderID = gInventory.findCategoryUUIDForType(LLFolderType::FT_CALLINGCARD);

    return findFirstCallingCardSubfolder(callingCardsFolderID);
}

const LLUUID& LLFriendCardsManager::findFriendAllSubfolderUUIDImpl() const
{
    LLUUID friendFolderUUID = findFriendFolderUUIDImpl();

    return findFirstCallingCardSubfolder(friendFolderUUID);
}

const LLUUID& LLFriendCardsManager::findChildFolderUUID(const LLUUID& parentFolderUUID, const std::string& nonLocalizedName) const
{
	LLNameCategoryCollector matchFolderFunctor(nonLocalizedName);

	return get_folder_uuid(parentFolderUUID, matchFolderFunctor);
}
const LLUUID& LLFriendCardsManager::findFriendCardInventoryUUIDImpl(const LLUUID& avatarID)
{
	LLUUID friendAllSubfolderUUID = findFriendAllSubfolderUUIDImpl();
	LLInventoryModel::cat_array_t cats;
	LLInventoryModel::item_array_t items;
	LLInventoryModel::item_array_t::const_iterator it;

	// it is not necessary to check friendAllSubfolderUUID against NULL. It will be processed by collectDescendents
	gInventory.collectDescendents(friendAllSubfolderUUID, cats, items, LLInventoryModel::EXCLUDE_TRASH);
	for (it = items.begin(); it != items.end(); ++it)
	{
		if ((*it)->getCreatorUUID() == avatarID)
			return (*it)->getUUID();
	}

	return LLUUID::null;
}

void LLFriendCardsManager::findMatchedFriendCards(const LLUUID& avatarID, LLInventoryModel::item_array_t& items) const
{
	LLInventoryModel::cat_array_t cats;
	LLUUID friendFolderUUID = findFriendFolderUUIDImpl();


	LLViewerInventoryCategory* friendFolder = gInventory.getCategory(friendFolderUUID);
	if (NULL == friendFolder)
		return;

	LLParticularBuddyCollector matchFunctor(avatarID);
	LLInventoryModel::cat_array_t subFolders;
	subFolders.push_back(friendFolder);

	while (subFolders.size() > 0)
	{
		LLViewerInventoryCategory* cat = subFolders.at(0);
		subFolders.erase(subFolders.begin());

		gInventory.collectDescendentsIf(cat->getUUID(), cats, items, 
			LLInventoryModel::EXCLUDE_TRASH, matchFunctor);

		move_from_to_arrays(cats, subFolders);
	}
}

void LLFriendCardsManager::fetchAndCheckFolderDescendents(const LLUUID& folder_id,  callback_t cb)
{
	// This instance will be deleted in LLInitialFriendCardsFetch::done().
	LLInitialFriendCardsFetch* fetch = new LLInitialFriendCardsFetch(folder_id, cb);
	fetch->startFetch();
	if(fetch->isFinished())
	{
		// everything is already here - call done.
		fetch->done();
	}
	else
	{
		// it's all on it's way - add an observer, and the inventory
		// will call done for us when everything is here.
		gInventory.addObserver(fetch);
	}
}

// Make sure LLInventoryModel::buildParentChildMap() has been called before it.
// This method must be called before any actions with friends list.
void LLFriendCardsManager::ensureFriendsFolderExists()
{
	const LLUUID calling_cards_folder_ID = gInventory.findCategoryUUIDForType(LLFolderType::FT_CALLINGCARD);

	// If "Friends" folder exists in "Calling Cards" we should check if "All" sub-folder
	// exists in "Friends", otherwise we create it.
	LLUUID friends_folder_ID = findFriendFolderUUIDImpl();
	if (friends_folder_ID.notNull())
	{
        mState = LOADING_FRIENDS_FOLDER;
		fetchAndCheckFolderDescendents(friends_folder_ID,
				boost::bind(&LLFriendCardsManager::ensureFriendsAllFolderExists, this));
	}
	else
	{
		if (!gInventory.isCategoryComplete(calling_cards_folder_ID))
		{
			LLViewerInventoryCategory* cat = gInventory.getCategory(calling_cards_folder_ID);
			std::string cat_name = cat ? cat->getName() : "unknown";
			LL_WARNS() << "Failed to find \"" << cat_name << "\" category descendents in Category Tree." << LL_ENDL;
		}

		gInventory.createNewCategory(
            calling_cards_folder_ID,
			LLFolderType::FT_CALLINGCARD,
            get_friend_folder_name(),
            [](const LLUUID &new_category_id)
        {
            gInventory.createNewCategory(
                new_category_id,
                LLFolderType::FT_CALLINGCARD,
                get_friend_all_subfolder_name(),
                [](const LLUUID &new_category_id)
            {
                // Now when we have all needed folders we can sync their contents with buddies list.
                LLFriendCardsManager::getInstance()->syncFriendsFolder();
            }
            );
        }
        );
	}
}

// Make sure LLFriendCardsManager::ensureFriendsFolderExists() has been called before it.
void LLFriendCardsManager::ensureFriendsAllFolderExists()
{
	LLUUID friends_all_folder_ID = findFriendAllSubfolderUUIDImpl();
	if (friends_all_folder_ID.notNull())
	{
        mState = LOADING_ALL_FOLDER;
		fetchAndCheckFolderDescendents(friends_all_folder_ID,
				boost::bind(&LLFriendCardsManager::syncFriendsFolder, this));
	}
	else
	{
		LLUUID friends_folder_ID = findFriendFolderUUIDImpl();

		if (!gInventory.isCategoryComplete(friends_folder_ID))
		{
			LLViewerInventoryCategory* cat = gInventory.getCategory(friends_folder_ID);
			std::string cat_name = cat ? cat->getName() : "unknown";
			LL_WARNS() << "Failed to find \"" << cat_name << "\" category descendents in Category Tree." << LL_ENDL;
		}

        gInventory.createNewCategory(
            friends_folder_ID,
			LLFolderType::FT_CALLINGCARD,
            get_friend_all_subfolder_name(),
            [](const LLUUID &new_cat_id)
        {
            // Now when we have all needed folders we can sync their contents with buddies list.
            LLFriendCardsManager::getInstance()->syncFriendsFolder();
        }
        );
	}
}

void LLFriendCardsManager::syncFriendsFolder()
{
	LLAvatarTracker::buddy_map_t all_buddies;
	LLAvatarTracker::instance().copyBuddyList(all_buddies);

	// 1. Check if own calling card exists
	const LLUUID calling_cards_folder_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_CALLINGCARD);

	LLInventoryModel::cat_array_t cats;
	LLInventoryModel::item_array_t items;
	LLFindAgentCallingCard collector;
	gInventory.collectDescendentsIf(calling_cards_folder_id, cats, items, LLInventoryModel::EXCLUDE_TRASH, collector);

	// Create own calling card if it was not found in Friends/All folder
	if (!collector.isAgentCallingCardFound())
	{
		create_inventory_callingcard(gAgentID, calling_cards_folder_id);
	}

    // All folders created and updated.
    mState = MANAGER_READY;

	// 2. Add missing Friend Cards for friends
	LLAvatarTracker::buddy_map_t::const_iterator buddy_it = all_buddies.begin();
	LL_INFOS() << "try to build friends, count: " << all_buddies.size() << LL_ENDL;
	for(; buddy_it != all_buddies.end(); ++buddy_it)
	{
		const LLUUID& buddy_id = (*buddy_it).first;
		addFriendCardToInventory(buddy_id);
	}
}

class CreateFriendCardCallback : public LLInventoryCallback
{
public:
	void fire(const LLUUID& inv_item_id)
	{
		LLViewerInventoryItem* item = gInventory.getItem(inv_item_id);

		if (item)
			LLFriendCardsManager::instance().extractAvatarID(item->getCreatorUUID());
	}
};

void LLFriendCardsManager::addFriendCardToInventory(const LLUUID& avatarID)
{

	bool shouldBeAdded = true;
	LLAvatarName av_name;
	LLAvatarNameCache::get(avatarID, &av_name);
	const std::string& name = av_name.getAccountName();

	LL_DEBUGS() << "Processing buddy name: " << name 
		<< ", id: " << avatarID
		<< LL_ENDL; 

    if (shouldBeAdded && !isManagerReady())
    {
        shouldBeAdded = false;
        LL_DEBUGS() << "Calling cards manager not ready, state: " << getManagerState() << LL_ENDL;
    }

	if (shouldBeAdded && findFriendCardInventoryUUIDImpl(avatarID).notNull())
	{
		shouldBeAdded = false;
		LL_DEBUGS() << "is found in Inventory: " << name << LL_ENDL; 
	}

	if (shouldBeAdded && isAvatarDataStored(avatarID))
	{
		shouldBeAdded = false;
		LL_DEBUGS() << "is found in sentRequests: " << name << LL_ENDL; 
	}

	if (shouldBeAdded)
	{
		putAvatarData(avatarID);
		LL_DEBUGS() << "Sent create_inventory_item for " << avatarID << ", " << name << LL_ENDL;

		// TODO: mantipov: Is CreateFriendCardCallback really needed? Probably not
		LLPointer<LLInventoryCallback> cb = new CreateFriendCardCallback;

		create_inventory_callingcard(avatarID, findFriendAllSubfolderUUIDImpl(), cb);
	}
}

void LLFriendCardsManager::removeFriendCardFromInventory(const LLUUID& avatarID)
{
	LLInventoryModel::item_array_t items;
	findMatchedFriendCards(avatarID, items);

	LLInventoryModel::item_array_t::const_iterator it;
	for (it = items.begin(); it != items.end(); ++ it)
	{
		gInventory.removeItem((*it)->getUUID());
	}
}

void LLFriendCardsManager::onFriendListUpdate(U32 changed_mask)
{
	LLAvatarTracker& at = LLAvatarTracker::instance();

	switch(changed_mask) {
	case LLFriendObserver::ADD:
		{
            LLFriendCardsManager& cards_manager = LLFriendCardsManager::instance();
            if (cards_manager.isManagerReady())
            {
                // Try to add cards into inventory.
                // If cards already exist they won't be created.
                const std::set<LLUUID>& changed_items = at.getChangedIDs();
                std::set<LLUUID>::const_iterator id_it = changed_items.begin();
                std::set<LLUUID>::const_iterator id_end = changed_items.end();
                for (; id_it != id_end; ++id_it)
                {
                    cards_manager.addFriendCardToInventory(*id_it);
                }
            }
            else
            {
                // User either removed calling cards' folders and manager is loading them
                // or update came too early, before viewer had chance to load all folders.
                // Either way don't process 'add' operation - manager will recreate all
                // cards after fetching folders.
                LL_INFOS_ONCE() << "Calling cards manager not ready, state: "
                    << cards_manager.getManagerState()
                    << ", postponing update."
                    << LL_ENDL;
            }
		}
		break;
	case LLFriendObserver::REMOVE:
		{
			const std::set<LLUUID>& changed_items = at.getChangedIDs();
			std::set<LLUUID>::const_iterator id_it = changed_items.begin();
			std::set<LLUUID>::const_iterator id_end = changed_items.end();
			for (;id_it != id_end; ++id_it)
			{
				LLFriendCardsManager::instance().removeFriendCardFromInventory(*id_it);
			}
		}

	default:;
	}
}

// EOF
