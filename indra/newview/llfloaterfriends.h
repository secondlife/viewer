/** 
 * @file llfloaterfriends.h
 * @author Phoenix
 * @date 2005-01-13
 * @brief Declaration of class for displaying the local agent's friends.
 *
 * $LicenseInfo:firstyear=2005&license=viewergpl$
 * 
 * Copyright (c) 2005-2009, Linden Research, Inc.
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

#ifndef LL_LLFLOATERFRIENDS_H
#define LL_LLFLOATERFRIENDS_H

#include "llpanel.h"
#include "llstring.h"
#include "lluuid.h"
#include "lltimer.h"
#include "llcallingcard.h"

class LLFriendObserver;
class LLRelationship;
class LLScrollListItem;
class LLScrollListCtrl;

/** 
 * @class LLPanelFriends
 * @brief An instance of this class is used for displaying your friends
 * and gives you quick access to all agents which a user relationship.
 *
 * @sa LLFloater
 */
class LLPanelFriends : public LLPanel, public LLEventTimer
{
public:
	LLPanelFriends();
	virtual ~LLPanelFriends();

	/** 
	 * @brief This method either creates or brings to the front the
	 * current instantiation of this floater. There is only once since
	 * you can currently only look at your local friends.
	 */
	virtual BOOL tick();

	/** 
	 * @brief This method is called in response to the LLAvatarTracker
	 * sending out a changed() message.
	 */
	void updateFriends(U32 changed_mask);

	virtual BOOL postBuild();

	// *HACK Made public to remove friends from LLAvatarIconCtrl context menu
	static bool handleRemove(const LLSD& notification, const LLSD& response);

private:

	enum FRIENDS_COLUMN_ORDER
	{
		LIST_ONLINE_STATUS,
		LIST_FRIEND_NAME,
		LIST_VISIBLE_ONLINE,
		LIST_VISIBLE_MAP,
		LIST_EDIT_MINE,
		LIST_EDIT_THEIRS,
		LIST_FRIEND_UPDATE_GEN
	};

	// protected members
	typedef std::map<LLUUID, S32> rights_map_t;
	void refreshNames(U32 changed_mask);
	BOOL refreshNamesSync(const LLAvatarTracker::buddy_map_t & all_buddies);
	BOOL refreshNamesPresence(const LLAvatarTracker::buddy_map_t & all_buddies);
	void refreshUI();
	void refreshRightsChangeList();
	void applyRightsToFriends();
	BOOL addFriend(const LLUUID& agent_id);	
	BOOL updateFriendItem(const LLUUID& agent_id, const LLRelationship* relationship);

	typedef enum 
	{
		GRANT,
		REVOKE
	} EGrantRevoke;
	void confirmModifyRights(rights_map_t& ids, EGrantRevoke command);
	void sendRightsGrant(rights_map_t& ids);

	// return empty vector if nothing is selected
	std::vector<LLUUID> getSelectedIDs();

	// callback methods
	static void onSelectName(LLUICtrl* ctrl, void* user_data);
	static bool callbackAddFriend(const LLSD& notification, const LLSD& response);
	static bool callbackAddFriendWithMessage(const LLSD& notification, const LLSD& response);
	static void onPickAvatar(const std::vector<std::string>& names, const std::vector<LLUUID>& ids);
	static void onMaximumSelect();

	static void onClickIM(void* user_data);
	static void onClickProfile(void* user_data);
	static void onClickAddFriend(void* user_data);
	static void onClickRemove(void* user_data);

	static void onClickOfferTeleport(void* user_data);
	static void onClickPay(void* user_data);

	static void onClickModifyStatus(LLUICtrl* ctrl, void* user_data);

	bool modifyRightsConfirmation(const LLSD& notification, const LLSD& response, rights_map_t* rights);

private:
	// member data
	LLFriendObserver* mObserver;
	LLUUID mAddFriendID;
	std::string mAddFriendName;
	LLScrollListCtrl* mFriendsList;
	BOOL mShowMaxSelectWarning;
	BOOL mAllowRightsChange;
	S32 mNumRightsChanged;
};


#endif // LL_LLFLOATERFRIENDS_H
