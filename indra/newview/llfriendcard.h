/** 
 * @file llfriendcard.h
 * @brief Definition of classes to process Friends Cards
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

#ifndef LL_LLFRIENDCARD_H
#define LL_LLFRIENDCARD_H


#include "llcallingcard.h"
#include "llinventorymodel.h" // for LLInventoryModel::item_array_t

class LLViewerInventoryItem;

class LLFriendCardsManager
	: public LLSingleton<LLFriendCardsManager>
	, public LLFriendObserver
{
	LOG_CLASS(LLFriendCardsManager);

	friend class LLSingleton<LLFriendCardsManager>;
	friend class CreateFriendCardCallback;

public:
	typedef std::map<LLUUID, uuid_vec_t > folderid_buddies_map_t;

	// LLFriendObserver implementation
	void changed(U32 mask)
	{
		onFriendListUpdate(mask);
	}

	/**
	 *	Determines if specified Inventory Calling Card exists in any of lists 
	 *	in the Calling Card/Friends/ folder (Default, or Custom)
	 */
	bool isItemInAnyFriendsList(const LLViewerInventoryItem* item);

	/**
	 *	Checks if specified category is contained in the Calling Card/Friends folder and 
	 *	determines if specified Inventory Object exists in that category.
	 */
	bool isObjDirectDescendentOfCategory(const LLInventoryObject* obj, const LLViewerInventoryCategory* cat) const;

	/**
	 *	Checks is the specified category is in the Calling Card/Friends folder
	 */
	bool isCategoryInFriendFolder(const LLViewerInventoryCategory* cat) const;

	/**
	 *	Checks is the specified category is a Friend folder or any its subfolder
	 */
	bool isAnyFriendCategory(const LLUUID& catID) const;

	/**
	 *	Checks whether "Friends" and "Friends/All" folders exist in "Calling Cards" category
	 *	(creates them otherwise) and fetches their contents to synchronize with Agent's Friends List.
	 */
	void syncFriendCardsFolders();

private:
	typedef boost::function<void()> callback_t;

	LLFriendCardsManager();
	~LLFriendCardsManager();


	/**
	 *	Stores buddy id to avoid sent create_inventory_callingcard several time for the same Avatar
	 */
	void putAvatarData(const LLUUID& avatarID);

	/**
	 *	Extracts buddy id of Created Friend Card
	 */
	const LLUUID extractAvatarID(const LLUUID& avatarID);

	bool isAvatarDataStored(const LLUUID& avatarID) const
	{
		return (mBuddyIDSet.end() != mBuddyIDSet.find(avatarID));
	}

	const LLUUID& findChildFolderUUID(const LLUUID& parentFolderUUID, const std::string& nonLocalizedName) const;
	const LLUUID& findFriendFolderUUIDImpl() const;
	const LLUUID& findFriendAllSubfolderUUIDImpl() const;
	const LLUUID& findFriendCardInventoryUUIDImpl(const LLUUID& avatarID);
	void findMatchedFriendCards(const LLUUID& avatarID, LLInventoryModel::item_array_t& items) const;

	void fetchAndCheckFolderDescendents(const LLUUID& folder_id, callback_t cb);

	/**
	 *	Checks whether "Calling Cards/Friends" folder exists. If not, creates it with "All"
	 *	sub-folder and synchronizes its contents with buddies list.
	 */
	void ensureFriendsFolderExists();

	/**
	 *	Checks whether "Calling Cards/Friends/All" folder exists. If not, creates it and
	 *	synchronizes its contents with buddies list.
	 */
	void ensureFriendsAllFolderExists();

	/**
	 *	Synchronizes content of the Calling Card/Friends/All Global Inventory folder with Agent's Friend List
	 */
	void syncFriendsFolder();

	/**
	 *	Adds avatar specified by its UUID into the Calling Card/Friends/All Global Inventory folder
	 */
	void addFriendCardToInventory(const LLUUID& avatarID);

	/**
	 *	Removes an avatar specified by its UUID from the Calling Card/Friends/All Global Inventory folder
	 *		and from the all Custom Folders
	 */
	void removeFriendCardFromInventory(const LLUUID& avatarID);

	void onFriendListUpdate(U32 changed_mask);


private:
	typedef std::set<LLUUID> avatar_uuid_set_t;

	avatar_uuid_set_t mBuddyIDSet;
};

#endif // LL_LLFRIENDCARD_H
