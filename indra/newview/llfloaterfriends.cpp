/** 
 * @file llfloaterfriends.cpp
 * @author Phoenix
 * @date 2005-01-13
 * @brief Implementation of the friends floater
 *
 * $LicenseInfo:firstyear=2005&license=viewergpl$
 * 
 * Copyright (c) 2005-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

#include "llfloaterfriends.h"

#include <sstream>

#include "lldir.h"

#include "llagent.h"
#include "llfloateravatarpicker.h"
#include "llviewerwindow.h"
#include "llbutton.h"
#include "llcallingcard.h"
#include "llfloateravatarinfo.h"
#include "llinventorymodel.h"
#include "llnamelistctrl.h"
#include "llnotify.h"
#include "llresmgr.h"
#include "llimview.h"
#include "llvieweruictrlfactory.h"
#include "llmenucommands.h"
#include "llviewercontrol.h"
#include "llviewermessage.h"
#include "lltimer.h"
#include "lltextbox.h"

//Maximum number of people you can select to do an operation on at once.
#define MAX_FRIEND_SELECT 20
#define RIGHTS_CHANGE_TIMEOUT 5.0

// simple class to observe the calling cards.
class LLLocalFriendsObserver : public LLFriendObserver
{
public:
	LLLocalFriendsObserver(LLPanelFriends* floater) : mFloater(floater) {}
	virtual ~LLLocalFriendsObserver() { mFloater = NULL; }
	virtual void changed(U32 mask)
	{
		mFloater->updateFriends(mask);
	}
protected:
	LLPanelFriends* mFloater;
};

LLPanelFriends::LLPanelFriends() :
	LLPanel(),
	LLEventTimer(1000000),
	mObserver(NULL),
	mMenuState(0),
	mShowMaxSelectWarning(TRUE),
	mAllowRightsChange(TRUE),
	mNumRightsChanged(0)
{
	mEventTimer.stop();
	mObserver = new LLLocalFriendsObserver(this);
	LLAvatarTracker::instance().addObserver(mObserver);
}

LLPanelFriends::~LLPanelFriends()
{
	LLAvatarTracker::instance().removeObserver(mObserver);
	delete mObserver;
}

BOOL LLPanelFriends::tick()
{
	mEventTimer.stop();
	mPeriod = 1000000;
	mAllowRightsChange = TRUE;
	updateFriends(LLFriendObserver::ADD);
	return FALSE;
}

void LLPanelFriends::updateFriends(U32 changed_mask)
{
	LLUUID selected_id;
	LLCtrlListInterface *friends_list = childGetListInterface("friend_list");
	if (!friends_list) return;
	LLCtrlScrollInterface *friends_scroll = childGetScrollInterface("friend_list");
	if (!friends_scroll) return;
	
	// We kill the selection warning, otherwise we'll spam with warning popups
	// if the maximum amount of friends are selected
	mShowMaxSelectWarning = false;

	LLDynamicArray<LLUUID> selected_friends = getSelectedIDs();
	if(changed_mask & (LLFriendObserver::ADD | LLFriendObserver::REMOVE | LLFriendObserver::ONLINE))
	{
		refreshNames();
	}
	else if(changed_mask & LLFriendObserver::POWERS)
	{
		--mNumRightsChanged;
		if(mNumRightsChanged > 0)
		{
			mPeriod = RIGHTS_CHANGE_TIMEOUT;	
			mEventTimer.start();
			mEventTimer.reset();
			mAllowRightsChange = FALSE;
		}
		else
		{
			tick();
		}
	}
	if(selected_friends.size() > 0)
	{
		// only non-null if friends was already found. This may fail,
		// but we don't really care here, because refreshUI() will
		// clean up the interface.
		friends_list->setCurrentByID(selected_id);
		for(LLDynamicArray<LLUUID>::iterator itr = selected_friends.begin(); itr != selected_friends.end(); ++itr)
		{
			friends_list->setSelectedByValue(*itr, true);
		}
	}

	refreshUI();
	mShowMaxSelectWarning = true;
}

// virtual
BOOL LLPanelFriends::postBuild()
{
	mFriendsList = LLUICtrlFactory::getScrollListByName(this, "friend_list");
	mFriendsList->setMaxSelectable(MAX_FRIEND_SELECT);
	mFriendsList->setMaxiumumSelectCallback(onMaximumSelect);
	mFriendsList->setCommitOnSelectionChange(TRUE);
	childSetCommitCallback("friend_list", onSelectName, this);
	childSetDoubleClickCallback("friend_list", onClickIM);

	refreshNames();

	childSetCommitCallback("online_status_cb", onClickOnlineStatus, this);
	childSetCommitCallback("map_status_cb", onClickMapStatus, this);
	childSetCommitCallback("modify_status_cb", onClickModifyStatus, this);
	childSetAction("im_btn", onClickIM, this);
	childSetAction("profile_btn", onClickProfile, this);
	childSetAction("offer_teleport_btn", onClickOfferTeleport, this);
	childSetAction("pay_btn", onClickPay, this);
	childSetAction("add_btn", onClickAddFriend, this);
	childSetAction("remove_btn", onClickRemove, this);

	setDefaultBtn("im_btn");

	updateFriends(LLFriendObserver::ADD);
	refreshUI();

	return TRUE;
}


void LLPanelFriends::addFriend(const std::string& name, const LLUUID& agent_id)
{
	LLAvatarTracker& at = LLAvatarTracker::instance();
	const LLRelationship* relationInfo = at.getBuddyInfo(agent_id);
	if(!relationInfo) return;
	BOOL online = relationInfo->isOnline();

	LLSD element;
	element["id"] = agent_id;
	element["columns"][LIST_FRIEND_NAME]["column"] = "friend_name";
	element["columns"][LIST_FRIEND_NAME]["value"] = name.c_str();
	element["columns"][LIST_FRIEND_NAME]["font"] = "SANSSERIF";
	element["columns"][LIST_FRIEND_NAME]["font-style"] = "NORMAL";	
	element["columns"][LIST_ONLINE_STATUS]["column"] = "icon_online_status";
	element["columns"][LIST_ONLINE_STATUS]["type"] = "text";
	if (online)
	{
		element["columns"][LIST_FRIEND_NAME]["font-style"] = "BOLD";	
		element["columns"][LIST_ONLINE_STATUS]["type"] = "icon";
		element["columns"][LIST_ONLINE_STATUS]["value"] = gViewerArt.getString("icon_avatar_online.tga");		
	}


	element["columns"][LIST_VISIBLE_ONLINE]["column"] = "icon_visible_online";
	element["columns"][LIST_VISIBLE_ONLINE]["type"] = "text";
	if(relationInfo->isRightGrantedTo(LLRelationship::GRANT_ONLINE_STATUS))
	{
		element["columns"][LIST_VISIBLE_ONLINE]["type"] = "icon";
		element["columns"][LIST_VISIBLE_ONLINE]["value"] = gViewerArt.getString("ff_visible_online.tga");
	}
	element["columns"][LIST_VISIBLE_MAP]["column"] = "icon_visible_map";
	element["columns"][LIST_VISIBLE_MAP]["type"] = "text";
	if(relationInfo->isRightGrantedTo(LLRelationship::GRANT_MAP_LOCATION))
	{
		element["columns"][LIST_VISIBLE_MAP]["type"] = "icon";
		element["columns"][LIST_VISIBLE_MAP]["value"] = gViewerArt.getString("ff_visible_map.tga");
	}
	element["columns"][LIST_EDIT_MINE]["column"] = "icon_edit_mine";
	element["columns"][LIST_EDIT_MINE]["type"] = "text";
	if(relationInfo->isRightGrantedTo(LLRelationship::GRANT_MODIFY_OBJECTS))
	{
		element["columns"][LIST_EDIT_MINE]["type"] = "icon";
		element["columns"][LIST_EDIT_MINE]["value"] = gViewerArt.getString("ff_edit_mine.tga");
	}
	element["columns"][LIST_EDIT_THEIRS]["column"] = "icon_edit_theirs";
	element["columns"][LIST_EDIT_THEIRS]["type"] = "text";
	if(relationInfo->isRightGrantedFrom(LLRelationship::GRANT_MODIFY_OBJECTS))
	{
		element["columns"][LIST_EDIT_THEIRS]["type"] = "icon";
		element["columns"][LIST_EDIT_THEIRS]["value"] = gViewerArt.getString("ff_edit_theirs.tga");
	}
	mFriendsList->addElement(element, ADD_BOTTOM);
}

void LLPanelFriends::refreshRightsChangeList()
{
	LLDynamicArray<LLUUID> friends = getSelectedIDs();
	S32 num_selected = friends.size();

	LLSD row;
	bool can_offer_teleport = num_selected >= 1;

	// aggregate permissions over all selected friends
	bool friends_see_online = true;
	bool friends_see_on_map = true;
	bool friends_modify_objects = true;

	// do at least some of the friends selected have these rights?
	bool some_friends_see_online = false;
	bool some_friends_see_on_map = false;
	bool some_friends_modify_objects = false;

	bool selected_friends_online = true;

	LLTextBox* processing_label = LLUICtrlFactory::getTextBoxByName(this, "process_rights_label");

	if(!mAllowRightsChange)
	{
		if(processing_label)
		{
			processing_label->setVisible(true);
			// ignore selection for now
			friends.clear();
			num_selected = 0;
		}
	}
	else
	{
		if(processing_label)
		{
			processing_label->setVisible(false);
		}
	}

	const LLRelationship* friend_status = NULL;
	for(LLDynamicArray<LLUUID>::iterator itr = friends.begin(); itr != friends.end(); ++itr)
	{
		friend_status = LLAvatarTracker::instance().getBuddyInfo(*itr);
		if (friend_status)
		{
			bool can_see_online = friend_status->isRightGrantedTo(LLRelationship::GRANT_ONLINE_STATUS);
			bool can_see_on_map = friend_status->isRightGrantedTo(LLRelationship::GRANT_MAP_LOCATION);
			bool can_modify_objects = friend_status->isRightGrantedTo(LLRelationship::GRANT_MODIFY_OBJECTS);

			// aggregate rights of this friend into total selection
			friends_see_online &= can_see_online;
			friends_see_on_map &= can_see_on_map;
			friends_modify_objects &= can_modify_objects;

			// can at least one of your selected friends do any of these?
			some_friends_see_online |= can_see_online;
			some_friends_see_on_map |= can_see_on_map;
			some_friends_modify_objects |= can_modify_objects;

			if(!friend_status->isOnline())
			{
				can_offer_teleport = false;
				selected_friends_online = false;
			}
		}
		else // missing buddy info, don't allow any operations
		{
			can_offer_teleport = false;

			friends_see_online = false;
			friends_see_on_map = false;
			friends_modify_objects = false;

			some_friends_see_online = false;
			some_friends_see_on_map = false;
			some_friends_modify_objects = false;
		}
	}
	

	// seeing a friend on the map requires seeing online status as a prerequisite
	friends_see_on_map &= friends_see_online;

	mMenuState = 0;

	// make checkboxes visible after we have finished processing rights
	childSetVisible("online_status_cb", mAllowRightsChange);
	childSetVisible("map_status_cb", mAllowRightsChange);
	childSetVisible("modify_status_cb", mAllowRightsChange);

	if (num_selected == 0)  // nothing selected
	{
		childSetEnabled("im_btn", FALSE);
		childSetEnabled("offer_teleport_btn", FALSE);

		childSetEnabled("online_status_cb", FALSE);
		childSetValue("online_status_cb", FALSE);
		childSetTentative("online_status_cb", FALSE);

		childSetEnabled("map_status_cb", FALSE);
		childSetValue("map_status_cb", FALSE);
		childSetTentative("map_status_cb", FALSE);

		childSetEnabled("modify_status_cb", FALSE);
		childSetValue("modify_status_cb", FALSE);
		childSetTentative("modify_status_cb", FALSE);
	}
	else // we have at least one friend selected...
	{
		// only allow IMs to groups when everyone in the group is online
		// to be consistent with context menus in inventory and because otherwise
		// offline friends would be silently dropped from the session
		childSetEnabled("im_btn", selected_friends_online || num_selected == 1);

		childSetEnabled("offer_teleport_btn", can_offer_teleport);

		childSetEnabled("online_status_cb", TRUE);
		childSetValue("online_status_cb", some_friends_see_online);
		childSetTentative("online_status_cb", some_friends_see_online != friends_see_online);
		if (friends_see_online)
		{
			mMenuState |= LLRelationship::GRANT_ONLINE_STATUS;
		}

		childSetEnabled("map_status_cb", TRUE);
		childSetValue("map_status_cb", some_friends_see_on_map);
		childSetTentative("map_status_cb", some_friends_see_on_map != friends_see_on_map);
		if(friends_see_on_map) 
		{
			mMenuState |= LLRelationship::GRANT_MAP_LOCATION;
		}

		// for now, don't allow modify rights change for multiple select
		childSetEnabled("modify_status_cb", num_selected == 1);
		childSetValue("modify_status_cb", some_friends_modify_objects);
		childSetTentative("modify_status_cb", some_friends_modify_objects != friends_modify_objects);
		if(friends_modify_objects) 
		{
			mMenuState |= LLRelationship::GRANT_MODIFY_OBJECTS;
		}
	}
}

void LLPanelFriends::refreshNames()
{
	LLDynamicArray<LLUUID> selected_ids = getSelectedIDs();	
	S32 pos = mFriendsList->getScrollPos();	
	mFriendsList->operateOnAll(LLCtrlListInterface::OP_DELETE);
	
	LLCollectAllBuddies collect;
	LLAvatarTracker::instance().applyFunctor(collect);
	LLCollectAllBuddies::buddy_map_t::const_iterator it = collect.mOnline.begin();
	LLCollectAllBuddies::buddy_map_t::const_iterator end = collect.mOnline.end();
	
	for ( ; it != end; ++it)
	{
		const std::string& name = it->first;
		const LLUUID& agent_id = it->second;
		addFriend(name, agent_id);
	}
	it = collect.mOffline.begin();
	end = collect.mOffline.end();
	for ( ; it != end; ++it)
	{
		const std::string& name = it->first;
		const LLUUID& agent_id = it->second;
		addFriend(name, agent_id);
	}
	mFriendsList->selectMultiple(selected_ids);
	mFriendsList->setScrollPos(pos);
}


void LLPanelFriends::refreshUI()
{	
	BOOL single_selected = FALSE;
	BOOL multiple_selected = FALSE;
	int num_selected = mFriendsList->getAllSelected().size();
	if(num_selected > 0)
	{
		single_selected = TRUE;
		if(num_selected > 1)
		{
			childSetText("friend_name_label", childGetText("Multiple"));
			multiple_selected = TRUE;		
		}
		else
		{			
			childSetText("friend_name_label", mFriendsList->getFirstSelected()->getColumn(LIST_FRIEND_NAME)->getText() + "...");
		}
	}
	else
	{
		childSetText("friend_name_label", LLString::null);
	}


	//Options that can only be performed with one friend selected
	childSetEnabled("profile_btn", single_selected && !multiple_selected);
	childSetEnabled("pay_btn", single_selected && !multiple_selected);

	//Options that can be performed with up to MAX_FRIEND_SELECT friends selected
	//(single_selected will always be true in this situations)
	childSetEnabled("remove_btn", single_selected);
	childSetEnabled("im_btn", single_selected);
	childSetEnabled("friend_rights", single_selected);

	refreshRightsChangeList();
}

LLDynamicArray<LLUUID> LLPanelFriends::getSelectedIDs()
{
	LLUUID selected_id;
	LLDynamicArray<LLUUID> friend_ids;
	std::vector<LLScrollListItem*> selected = mFriendsList->getAllSelected();
	for(std::vector<LLScrollListItem*>::iterator itr = selected.begin(); itr != selected.end(); ++itr)
	{
		friend_ids.push_back((*itr)->getUUID());
	}
	return friend_ids;
}

// static
void LLPanelFriends::onSelectName(LLUICtrl* ctrl, void* user_data)
{
	LLPanelFriends* panelp = (LLPanelFriends*)user_data;

	if(panelp)
	{
		panelp->refreshUI();
	}
}

//static
void LLPanelFriends::onMaximumSelect(void* user_data)
{
	LLString::format_map_t args;
	args["[MAX_SELECT]"] = llformat("%d", MAX_FRIEND_SELECT);
	LLNotifyBox::showXml("MaxListSelectMessage", args);
};

// static
void LLPanelFriends::onClickProfile(void* user_data)
{
	LLPanelFriends* panelp = (LLPanelFriends*)user_data;

	//llinfos << "LLPanelFriends::onClickProfile()" << llendl;
	LLDynamicArray<LLUUID> ids = panelp->getSelectedIDs();
	if(ids.size() > 0)
	{
		LLUUID agent_id = ids[0];
		BOOL online;
		online = LLAvatarTracker::instance().isBuddyOnline(agent_id);
		LLFloaterAvatarInfo::showFromFriend(agent_id, online);
	}
}

// static
void LLPanelFriends::onClickIM(void* user_data)
{
	LLPanelFriends* panelp = (LLPanelFriends*)user_data;

	//llinfos << "LLPanelFriends::onClickIM()" << llendl;
	LLDynamicArray<LLUUID> ids = panelp->getSelectedIDs();
	if(ids.size() > 0)
	{
		if(ids.size() == 1)
		{
			LLUUID agent_id = ids[0];
			const LLRelationship* info = LLAvatarTracker::instance().getBuddyInfo(agent_id);
			char first[DB_FIRST_NAME_BUF_SIZE];	/* Flawfinder: ignore */
			char last[DB_LAST_NAME_BUF_SIZE];	/* Flawfinder: ignore */
			if(info && gCacheName->getName(agent_id, first, last))
			{
				char buffer[MAX_STRING];	/* Flawfinder: ignore */
				snprintf(buffer, MAX_STRING, "%s %s", first, last);	/* Flawfinder: ignore */
				gIMMgr->setFloaterOpen(TRUE);
				gIMMgr->addSession(
					buffer,
					IM_NOTHING_SPECIAL,
					agent_id);
			}		
		}
		else
		{
			gIMMgr->setFloaterOpen(TRUE);
			gIMMgr->addSession("Friends Conference",
								IM_SESSION_CONFERENCE_START,
								ids[0],
								ids);
		}
		make_ui_sound("UISndStartIM");
	}
}

// static
void LLPanelFriends::requestFriendship(const LLUUID& target_id, const LLString& target_name)
{
	// HACK: folder id stored as "message"
	LLUUID calling_card_folder_id = gInventory.findCategoryUUIDForType(LLAssetType::AT_CALLINGCARD);
	std::string message = calling_card_folder_id.asString();
	send_improved_im(target_id,
					 target_name.c_str(),
					 message.c_str(),
					 IM_ONLINE,
					 IM_FRIENDSHIP_OFFERED);
}

struct LLAddFriendData
{
	LLUUID mID;
	std::string mName;
};

// static
void LLPanelFriends::callbackAddFriend(S32 option, void* data)
{
	LLAddFriendData* add = (LLAddFriendData*)data;
	if (option == 0)
	{
		requestFriendship(add->mID, add->mName);
	}
	delete add;
}

// static
void LLPanelFriends::onPickAvatar(const std::vector<std::string>& names,
									const std::vector<LLUUID>& ids,
									void* )
{
	if (names.empty()) return;
	if (ids.empty()) return;
	requestFriendshipDialog(ids[0], names[0]);
}

// static
void LLPanelFriends::requestFriendshipDialog(const LLUUID& id,
											   const std::string& name)
{
	if(id == gAgentID)
	{
		LLNotifyBox::showXml("AddSelfFriend");
		return;
	}

	LLAddFriendData* data = new LLAddFriendData();
	data->mID = id;
	data->mName = name;
	
	// TODO: accept a line of text with this dialog
	LLString::format_map_t args;
	args["[NAME]"] = name;
	gViewerWindow->alertXml("AddFriend", args, callbackAddFriend, data);
}

// static
void LLPanelFriends::onClickAddFriend(void* user_data)
{
	LLPanelFriends* panelp = (LLPanelFriends*)user_data;
	LLFloater* root_floater = gFloaterView->getParentFloater(panelp);
	LLFloaterAvatarPicker* picker = LLFloaterAvatarPicker::show(onPickAvatar, user_data, FALSE, TRUE);
	if (root_floater)
	{
		root_floater->addDependentFloater(picker);
	}
}

// static
void LLPanelFriends::onClickRemove(void* user_data)
{
	LLPanelFriends* panelp = (LLPanelFriends*)user_data;

	//llinfos << "LLPanelFriends::onClickRemove()" << llendl;
	LLDynamicArray<LLUUID> ids = panelp->getSelectedIDs();
	LLStringBase<char>::format_map_t args;
	if(ids.size() > 0)
	{
		LLString msgType = "RemoveFromFriends";
		if(ids.size() == 1)
		{
			LLUUID agent_id = ids[0];
			char first[DB_FIRST_NAME_BUF_SIZE];		/*Flawfinder: ignore*/
			char last[DB_LAST_NAME_BUF_SIZE];		/*Flawfinder: ignore*/
			if(gCacheName->getName(agent_id, first, last))
			{
				args["[FIRST_NAME]"] = first;
				args["[LAST_NAME]"] = last;	
			}
		}
		else
		{
			msgType = "RemoveMultipleFromFriends";
		}
		gViewerWindow->alertXml(msgType,
			args,
			&handleRemove,
			(void*)new LLDynamicArray<LLUUID>(ids));
	}
}

// static
void LLPanelFriends::onClickOfferTeleport(void* user_data)
{
	LLPanelFriends* panelp = (LLPanelFriends*)user_data;

	LLDynamicArray<LLUUID> ids = panelp->getSelectedIDs();
	if(ids.size() > 0)
	{	
		handle_lure(ids);
	}
}

// static
void LLPanelFriends::onClickPay(void* user_data)
{
	LLPanelFriends* panelp = (LLPanelFriends*)user_data;

	LLDynamicArray<LLUUID> ids = panelp->getSelectedIDs();
	if(ids.size() == 1)
	{	
		handle_pay_by_id(ids[0]);
	}
}

void LLPanelFriends::onClickOnlineStatus(LLUICtrl* ctrl, void* user_data)
{
	LLPanelFriends* panelp = (LLPanelFriends*)user_data;

	bool checked = ctrl->getValue();
	panelp->updateMenuState(LLRelationship::GRANT_ONLINE_STATUS, checked);
	panelp->applyRightsToFriends(LLRelationship::GRANT_ONLINE_STATUS, checked);
}

void LLPanelFriends::onClickMapStatus(LLUICtrl* ctrl, void* user_data)
{
	LLPanelFriends* panelp = (LLPanelFriends*)user_data;
	bool checked = ctrl->getValue();
	panelp->updateMenuState(LLRelationship::GRANT_MAP_LOCATION, checked);
	panelp->applyRightsToFriends(LLRelationship::GRANT_MAP_LOCATION, checked);
}

void LLPanelFriends::onClickModifyStatus(LLUICtrl* ctrl, void* user_data)
{
	LLPanelFriends* panelp = (LLPanelFriends*)user_data;

	bool checked = ctrl->getValue();
	LLDynamicArray<LLUUID> ids = panelp->getSelectedIDs();
	LLStringBase<char>::format_map_t args;
	if(ids.size() > 0)
	{
		if(ids.size() == 1)
		{
			LLUUID agent_id = ids[0];
			char first[DB_FIRST_NAME_BUF_SIZE];		/*Flawfinder: ignore*/
			char last[DB_LAST_NAME_BUF_SIZE];		/*Flawfinder: ignore*/
			if(gCacheName->getName(agent_id, first, last))
			{
				args["[FIRST_NAME]"] = first;
				args["[LAST_NAME]"] = last;	
			}
			if(checked) gViewerWindow->alertXml("GrantModifyRights", args, handleModifyRights, user_data);
			else gViewerWindow->alertXml("RevokeModifyRights", args, handleModifyRights, user_data);
		}
		else return;
	}
}

void LLPanelFriends::handleModifyRights(S32 option, void* user_data)
{
	LLPanelFriends* panelp = (LLPanelFriends*)user_data;

	if(panelp)
	{
		if(!option)
		{
			panelp->updateMenuState(LLRelationship::GRANT_MODIFY_OBJECTS, !((panelp->getMenuState() & LLRelationship::GRANT_MODIFY_OBJECTS) != 0));
			panelp->applyRightsToFriends(LLRelationship::GRANT_MODIFY_OBJECTS, ((panelp->getMenuState() & LLRelationship::GRANT_MODIFY_OBJECTS) != 0));
		}
		panelp->refreshUI();
	}
}

void LLPanelFriends::updateMenuState(S32 flag, BOOL value)
{
	if(value) mMenuState |= flag;
	else mMenuState &= ~flag;
}

void LLPanelFriends::applyRightsToFriends(S32 flag, BOOL value)
{
	LLMessageSystem* msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_GrantUserRights);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUID(_PREHASH_AgentID, gAgent.getID());
	msg->addUUID(_PREHASH_SessionID, gAgent.getSessionID());

	LLDynamicArray<LLUUID> ids = getSelectedIDs();
	S32 rights;
	for(LLDynamicArray<LLUUID>::iterator itr = ids.begin(); itr != ids.end(); ++itr)
	{
		rights = LLAvatarTracker::instance().getBuddyInfo(*itr)->getRightsGrantedTo();
		if(LLAvatarTracker::instance().getBuddyInfo(*itr)->isRightGrantedTo(flag) != (bool)value)
		{
			if(value) rights |= flag;
			else rights &= ~flag;
			msg->nextBlockFast(_PREHASH_Rights);
			msg->addUUID(_PREHASH_AgentRelated, *itr);
			msg->addS32(_PREHASH_RelatedRights, rights);
		}
	}
	mNumRightsChanged = ids.size();
	gAgent.sendReliableMessage();
}



// static
void LLPanelFriends::handleRemove(S32 option, void* user_data)
{
	LLDynamicArray<LLUUID>* ids = static_cast<LLDynamicArray<LLUUID>*>(user_data);
	for(LLDynamicArray<LLUUID>::iterator itr = ids->begin(); itr != ids->end(); ++itr)
	{
		LLUUID id = (*itr);
		const LLRelationship* ip = LLAvatarTracker::instance().getBuddyInfo(id);
		if(ip)
		{			
			switch(option)
			{
			case 0: // YES
				if( ip->isRightGrantedTo(LLRelationship::GRANT_MODIFY_OBJECTS))
				{
					LLAvatarTracker::instance().empower(id, FALSE);
					LLAvatarTracker::instance().notifyObservers();
				}
				LLAvatarTracker::instance().terminateBuddy(id);
				LLAvatarTracker::instance().notifyObservers();
				gInventory.addChangedMask(LLInventoryObserver::LABEL | LLInventoryObserver::CALLING_CARD, LLUUID::null);
				gInventory.notifyObservers();
				break;

			case 1: // NO
			default:
				llinfos << "No removal performed." << llendl;
				break;
			}
		}
		
	}
	delete ids;
}
