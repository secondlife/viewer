/** 
 * @file llavataractions.h
 * @brief Friend-related actions (add, remove, offer teleport, etc)
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

#ifndef LL_LLAVATARACTIONS_H
#define LL_LLAVATARACTIONS_H

#include "lldarray.h"
#include "llsd.h"
#include "lluuid.h"

#include <string>
#include <vector>

class LLInventoryPanel;

/**
 * Friend-related actions (add, remove, offer teleport, etc)
 */
class LLAvatarActions
{
public:
	/**
	 * Show a dialog explaining what friendship entails, then request friendship.
	 */
	static void requestFriendshipDialog(const LLUUID& id, const std::string& name);

	/**
	 * Show a dialog explaining what friendship entails, then request friendship.
	 */
	static void requestFriendshipDialog(const LLUUID& id);

	/**
	 * Show a friend removal dialog.
	 */
	static void removeFriendDialog(const LLUUID& id);
	static void removeFriendsDialog(const uuid_vec_t& ids);
	
	/**
	 * Show teleport offer dialog.
	 */
	static void offerTeleport(const LLUUID& invitee);
	static void offerTeleport(const uuid_vec_t& ids);

	/**
	 * Start instant messaging session.
	 */
	static void startIM(const LLUUID& id);

	/**
	 * End instant messaging session.
	 */
	static void endIM(const LLUUID& id);

	/**
	 * Start an avatar-to-avatar voice call with another user
	 */
	static void startCall(const LLUUID& id);

	/**
	 * Start an ad-hoc conference voice call with multiple users
	 */
	static void startAdhocCall(const uuid_vec_t& ids);

	/**
	 * Start conference chat with the given avatars.
	 */
	static void startConference(const uuid_vec_t& ids);

	/**
	 * Show avatar profile.
	 */
	static void showProfile(const LLUUID& id);
	static void hideProfile(const LLUUID& id);
	static bool profileVisible(const LLUUID& id);

	/**
	 * Show avatar on world map.
	 */
	static void showOnMap(const LLUUID& id);

	/**
	 * Give money to the avatar.
	 */
	static void pay(const LLUUID& id);

	/**
	 * Share items with the avatar.
	 */
	static void share(const LLUUID& id);

	/**
	 * Share items with the picked avatars.
	 */
	static void shareWithAvatars();

	/**
	 * Block/unblock the avatar.
	 */
	static void toggleBlock(const LLUUID& id);

	/**
	 * Return true if avatar with "id" is a friend
	 */
	static bool isFriend(const LLUUID& id);

	/**
	 * @return true if the avatar is blocked
	 */
	static bool isBlocked(const LLUUID& id);

	/**
	 * @return true if you can block the avatar
	 */
	static bool canBlock(const LLUUID& id);

	/**
	 * Return true if the avatar is in a P2P voice call with a given user
	 */
	/* AD *TODO: Is this function needed any more?
		I fixed it a bit(added check for canCall), but it appears that it is not used
		anywhere. Maybe it should be removed?
	static bool isCalling(const LLUUID &id);*/

	/**
	 * @return true if call to the resident can be made
	 */

	static bool canCall();
	/**
	 * Invite avatar to a group.
	 */	
	static void inviteToGroup(const LLUUID& id);
	
	/**
	 * Kick avatar off grid
	 */	
	static void kick(const LLUUID& id);

	/**
	 * Freeze avatar
	 */	
	static void freeze(const LLUUID& id);

	/**
	 * Unfreeze avatar
	 */	
	static void unfreeze(const LLUUID& id);

	/**
	 * Open csr page for avatar
	 */	
	static void csr(const LLUUID& id, std::string name);

	/**
	 * Checks whether we can offer a teleport to the avatar, only offline friends
	 * cannot be offered a teleport.
	 *
	 * @return false if avatar is a friend and not visibly online
	 */
	static bool canOfferTeleport(const LLUUID& id);

	/**
	 * @return false if any one of the specified avatars a friend and not visibly online
	 */
	static bool canOfferTeleport(const uuid_vec_t& ids);

	/**
	 * Checks whether all items selected in the given inventory panel can be shared
	 *
	 * @param inv_panel Inventory panel to get selection from. If NULL, the active inventory panel is used.
	 *
	 * @return false if the selected items cannot be shared or the active inventory panel cannot be obtained
	 */
	static bool canShareSelectedItems(LLInventoryPanel* inv_panel = NULL);

private:
	static bool callbackAddFriendWithMessage(const LLSD& notification, const LLSD& response);
	static bool handleRemove(const LLSD& notification, const LLSD& response);
	static bool handlePay(const LLSD& notification, const LLSD& response, LLUUID avatar_id);
	static bool handleKick(const LLSD& notification, const LLSD& response);
	static bool handleFreeze(const LLSD& notification, const LLSD& response);
	static bool handleUnfreeze(const LLSD& notification, const LLSD& response);
	static void callback_invite_to_group(LLUUID group_id, LLUUID id);

	// Just request friendship, no dialog.
	static void requestFriendship(const LLUUID& target_id, const std::string& target_name, const std::string& message);
};

#endif // LL_LLAVATARACTIONS_H
