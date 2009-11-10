/** 
 * @file llfriendcard.cpp
 * @brief Implementation of classes to process Friends Cards
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

#include "llinventory.h"
#include "lltrans.h"

#include "llfriendcard.h"

#include "llcallingcard.h" // for LLAvatarTracker
#include "llviewerinventory.h"
#include "llinventorymodel.h"

// Constants;

static const std::string INVENTORY_STRING_FRIENDS_SUBFOLDER = "Friends";
static const std::string INVENTORY_STRING_FRIENDS_ALL_SUBFOLDER = "All";

// helper functions

/*
mantipov *NOTE: unable to use 
LLTrans::getString("InvFolder Friends"); or
LLTrans::getString("InvFolder FriendsAll");
in next two functions to set localized folders' names because of there is a hack in the
LLFolderViewItem::refreshFromListener() method for protected asset types.
So, localized names will be got from the strings with "InvFolder LABEL_NAME" in the strings.xml
*/
inline const std::string& get_friend_folder_name()
{
	return INVENTORY_STRING_FRIENDS_SUBFOLDER;
}

inline const std::string& get_friend_all_subfolder_name()
{
	return INVENTORY_STRING_FRIENDS_ALL_SUBFOLDER;
}

void move_from_to_arrays(LLInventoryModel::cat_array_t& from, LLInventoryModel::cat_array_t& to)
{
	while (from.count() > 0)
	{
		to.put(from.get(0));
		from.remove(0);
	}
}

const LLUUID& get_folder_uuid(const LLUUID& parentFolderUUID, LLInventoryCollectFunctor& matchFunctor)
{
	LLInventoryModel::cat_array_t cats;
	LLInventoryModel::item_array_t items;

	gInventory.collectDescendentsIf(parentFolderUUID, cats, items, 
		LLInventoryModel::EXCLUDE_TRASH, matchFunctor);

	if (cats.count() == 1)
	{
		return cats.get(0)->getUUID();
	}

	return LLUUID::null;
}


// LLViewerInventoryCategory::fetchDescendents has it own period of fetching.
// for now it is FETCH_TIMER_EXPIRY = 10.0f; So made our period a bit more.
const F32 FETCH_FRIENDS_DESCENDENTS_PERIOD = 11.0f;


/**
 * Intended to call passed callback after the specified period of time.
 *
 * Implemented to fix an issue when Inventory folders are in incomplete state. See EXT-2061, EXT-1935, EXT-813.
 * For now it uses to periodically sync Inventory Friends/All folder with a Agent's Friends List
 * until it is complete.
 */ 
class FriendListUpdater : public LLEventTimer
{
public:
	typedef boost::function<bool()> callback_t;

	FriendListUpdater(callback_t cb, F32 period)
		:	LLEventTimer(period)
		,	mCallback(cb)
	{
		mEventTimer.start();
	}

	virtual BOOL tick() // from LLEventTimer
	{
		return mCallback();
	}

private:
	callback_t		mCallback;
};


// LLFriendCardsManager Constructor / Destructor
LLFriendCardsManager::LLFriendCardsManager()
: mFriendsAllFolderCompleted(true)
{
	LLAvatarTracker::instance().addObserver(this);
}

LLFriendCardsManager::~LLFriendCardsManager()
{
	LLAvatarTracker::instance().removeObserver(this);
}

void LLFriendCardsManager::putAvatarData(const LLUUID& avatarID)
{
	llinfos << "Store avatar data, avatarID: " << avatarID << llendl;
	std::pair< avatar_uuid_set_t::iterator, bool > pr;
	pr = mBuddyIDSet.insert(avatarID);
	if (pr.second == false)
	{
		llwarns << "Trying to add avatar UUID for the stored avatar: " 
			<< avatarID
			<< llendl;
	}
}

const LLUUID LLFriendCardsManager::extractAvatarID(const LLUUID& avatarID)
{
	LLUUID rv;
	avatar_uuid_set_t::iterator it = mBuddyIDSet.find(avatarID);
	if (mBuddyIDSet.end() == it)
	{
		llwarns << "Call method for non-existent avatar name in the map: " << avatarID << llendl;
	}
	else
	{
		rv = (*it);
		mBuddyIDSet.erase(it);
	}
	return rv;
}

// be sure LLInventoryModel::buildParentChildMap() has been called before it.
// and this method must be called before any actions with friend list
void LLFriendCardsManager::ensureFriendFoldersExist()
{
	const LLUUID callingCardsFolderID = gInventory.findCategoryUUIDForType(LLFolderType::FT_CALLINGCARD);

	LLUUID friendFolderUUID = findFriendFolderUUIDImpl();

	if (friendFolderUUID.isNull())
	{
		friendFolderUUID = gInventory.createNewCategory(callingCardsFolderID,
			LLFolderType::FT_CALLINGCARD, get_friend_folder_name());
	}

	LLUUID friendAllSubfolderUUID = findFriendAllSubfolderUUIDImpl();

	if (friendAllSubfolderUUID.isNull())
	{
		friendAllSubfolderUUID = gInventory.createNewCategory(friendFolderUUID,
			LLFolderType::FT_CALLINGCARD, get_friend_all_subfolder_name());
	}
}


bool LLFriendCardsManager::isItemInAnyFriendsList(const LLViewerInventoryItem* item)
{
	if (item->getType() != LLAssetType::AT_CALLINGCARD)
		return false;

	LLInventoryModel::item_array_t items;
	findMatchedFriendCards(item->getCreatorUUID(), items);

	return items.count() > 0;
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
				for ( S32 i = items->count() - 1; i >= 0; --i )
				{
					cur_item = items->get(i);
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
			for ( S32 i = items->count() - 1; i >= 0; --i )
			{
				cur_item = items->get(i);
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
		for ( S32 i = cats->count() - 1; i >= 0; --i )
		{
			cur_cat = cats->get(i);
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
	return TRUE == gInventory.isObjectDescendentOf(cat->getUUID(), findFriendFolderUUIDImpl());
}

bool LLFriendCardsManager::isAnyFriendCategory(const LLUUID& catID) const
{
	const LLUUID& friendFolderID = findFriendFolderUUIDImpl();
	if (catID == friendFolderID)
		return true;

	return TRUE == gInventory.isObjectDescendentOf(catID, friendFolderID);
}

bool LLFriendCardsManager::syncFriendsFolder()
{
	//lets create "Friends" and "Friends/All" in the Inventory "Calling Cards" if they are absent
	LLFriendCardsManager::instance().ensureFriendFoldersExist();

	LLAvatarTracker::buddy_map_t all_buddies;
	LLAvatarTracker::instance().copyBuddyList(all_buddies);

	// 1. Remove Friend Cards for non-friends
	LLInventoryModel::cat_array_t cats;
	LLInventoryModel::item_array_t items;

	gInventory.collectDescendents(findFriendAllSubfolderUUIDImpl(), cats, items, LLInventoryModel::EXCLUDE_TRASH);
	
	LLInventoryModel::item_array_t::const_iterator it;
	for (it = items.begin(); it != items.end(); ++it)
	{
		lldebugs << "Check if buddy is in list: " << (*it)->getName() << " " << (*it)->getCreatorUUID() << llendl;
		if (NULL == get_ptr_in_map(all_buddies, (*it)->getCreatorUUID()))
		{
			lldebugs << "NONEXISTS, so remove it" << llendl;
			removeFriendCardFromInventory((*it)->getCreatorUUID());
		}
	}

	// 2. Add missing Friend Cards for friends
	LLAvatarTracker::buddy_map_t::const_iterator buddy_it = all_buddies.begin();
	llinfos << "try to build friends, count: " << all_buddies.size() << llendl; 
	mFriendsAllFolderCompleted = true;
	for(; buddy_it != all_buddies.end(); ++buddy_it)
	{
		const LLUUID& buddy_id = (*buddy_it).first;
		addFriendCardToInventory(buddy_id);
	}

	if (!mFriendsAllFolderCompleted)
	{
		forceFriendListIsLoaded(findFriendAllSubfolderUUIDImpl());

		static bool timer_started = false;
		if (!timer_started)
		{
			lldebugs << "Create and start timer to sync Inventory Friends All folder with Friends list" << llendl;

			// do not worry about destruction of the FriendListUpdater. 
			// It will be deleted by LLEventTimer::updateClass when FriendListUpdater::tick() returns true.
			new FriendListUpdater(boost::bind(&LLFriendCardsManager::syncFriendsFolder, this),
				FETCH_FRIENDS_DESCENDENTS_PERIOD);
		}
		timer_started = true;
	}
	else
	{
		lldebugs << "Friends/All Inventory folder is synchronized with the Agent's Friends List" << llendl;
	}

	return mFriendsAllFolderCompleted;
}

void LLFriendCardsManager::collectFriendsLists(folderid_buddies_map_t& folderBuddiesMap) const
{
	folderBuddiesMap.clear();

	LLInventoryModel::cat_array_t* listFolders;
	LLInventoryModel::item_array_t* items;

	// get folders in the Friend folder. Items should be NULL due to Cards should be in lists.
	gInventory.getDirectDescendentsOf(findFriendFolderUUIDImpl(), listFolders, items);

	if (NULL == listFolders)
		return;

	LLInventoryModel::cat_array_t::const_iterator itCats;	// to iterate Friend Lists (categories)
	LLInventoryModel::item_array_t::const_iterator itBuddy;	// to iterate Buddies in each List
	LLInventoryModel::cat_array_t* fakeCatsArg;
	for (itCats = listFolders->begin(); itCats != listFolders->end(); ++itCats)
	{
		if (items)
			items->clear();

		// *HACK: Only Friends/All content will be shown for now
		// *TODO: Remove this hack, implement sorting if it will be needded by spec.
		if ((*itCats)->getUUID() != findFriendAllSubfolderUUIDImpl())
			continue;

		gInventory.getDirectDescendentsOf((*itCats)->getUUID(), fakeCatsArg, items);

		if (NULL == items)
			continue;

		std::vector<LLUUID> buddyUUIDs;
		for (itBuddy = items->begin(); itBuddy != items->end(); ++itBuddy)
		{
			buddyUUIDs.push_back((*itBuddy)->getCreatorUUID());
		}

		folderBuddiesMap.insert(make_pair((*itCats)->getUUID(), buddyUUIDs));
	}
}


/************************************************************************/
/*		Private Methods                                                 */
/************************************************************************/
const LLUUID& LLFriendCardsManager::findFriendFolderUUIDImpl() const
{
	const LLUUID callingCardsFolderID = gInventory.findCategoryUUIDForType(LLFolderType::FT_CALLINGCARD);

	std::string friendFolderName = get_friend_folder_name();

	return findChildFolderUUID(callingCardsFolderID, friendFolderName);
}

const LLUUID& LLFriendCardsManager::findFriendAllSubfolderUUIDImpl() const
{
	LLUUID friendFolderUUID = findFriendFolderUUIDImpl();

	std::string friendAllSubfolderName = get_friend_all_subfolder_name();

	return findChildFolderUUID(friendFolderUUID, friendAllSubfolderName);
}

const LLUUID& LLFriendCardsManager::findChildFolderUUID(const LLUUID& parentFolderUUID, const std::string& folderLabel) const
{
	// mantipov *HACK: get localaized name in the same way like in the LLFolderViewItem::refreshFromListener() method.
	// be sure these both methods are synchronized.
	// see also get_friend_folder_name() and get_friend_all_subfolder_name() functions
	std::string localizedName = LLTrans::getString("InvFolder " + folderLabel);

	LLNameCategoryCollector matchFolderFunctor(localizedName);

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

	while (subFolders.count() > 0)
	{
		LLViewerInventoryCategory* cat = subFolders.get(0);
		subFolders.remove(0);

		gInventory.collectDescendentsIf(cat->getUUID(), cats, items, 
			LLInventoryModel::EXCLUDE_TRASH, matchFunctor);

		move_from_to_arrays(cats, subFolders);
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

bool LLFriendCardsManager::addFriendCardToInventory(const LLUUID& avatarID)
{
	LLInventoryModel* invModel = &gInventory;

	bool shouldBeAdded = true;
	std::string name;
	gCacheName->getFullName(avatarID, name);

	lldebugs << "Processing buddy name: " << name 
		<< ", id: " << avatarID
		<< llendl; 

	if (shouldBeAdded && findFriendCardInventoryUUIDImpl(avatarID).notNull())
	{
		shouldBeAdded = false;
		lldebugs << "is found in Inventory: " << name << llendl; 
	}

	if (shouldBeAdded && isAvatarDataStored(avatarID))
	{
		shouldBeAdded = false;
		lldebugs << "is found in sentRequests: " << name << llendl; 
	}

	LLUUID friendListFolderID = findFriendAllSubfolderUUIDImpl();
	if (friendListFolderID.notNull() && shouldBeAdded && !invModel->isCategoryComplete(friendListFolderID))
	{
		mFriendsAllFolderCompleted = false;
		shouldBeAdded = false;
		lldebugs << "Friends/All category is not completed" << llendl; 
	}
	if (shouldBeAdded)
	{
		putAvatarData(avatarID);
		lldebugs << "Sent create_inventory_item for " << avatarID << ", " << name << llendl;

		// TODO: mantipov: Is CreateFriendCardCallback really needed? Probably not
		LLPointer<LLInventoryCallback> cb = new CreateFriendCardCallback();

		create_inventory_callingcard(avatarID, friendListFolderID, cb);
	}

	return shouldBeAdded;
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
			const std::set<LLUUID>& changed_items = at.getChangedIDs();
			std::set<LLUUID>::const_iterator id_it = changed_items.begin();
			std::set<LLUUID>::const_iterator id_end = changed_items.end();
			for (;id_it != id_end; ++id_it)
			{
				LLFriendCardsManager::instance().addFriendCardToInventory(*id_it);
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

void LLFriendCardsManager::forceFriendListIsLoaded(const LLUUID& folder_id) const
{
	bool fetching_inventory = gInventory.fetchDescendentsOf(folder_id);
	lldebugs << "Trying to fetch descendants of Friends/All Inventory folder, fetched: "
		<< fetching_inventory << llendl;
}

// EOF
