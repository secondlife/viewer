/** 
 * @file llfloaterfriends.cpp
 * @author Phoenix
 * @date 2005-01-13
 * @brief Implementation of the friends floater
 *
 * Copyright (c) 2005-$CurrentYear$, Linden Research, Inc.
 * $License$
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
	LLLocalFriendsObserver(LLFloaterFriends* floater) : mFloater(floater) {}
	virtual ~LLLocalFriendsObserver() { mFloater = NULL; }
	virtual void changed(U32 mask)
	{
		mFloater->updateFriends(mask);
	}
protected:
	LLFloaterFriends* mFloater;
};

LLFloaterFriends* LLFloaterFriends::sInstance = NULL;

LLFloaterFriends::LLFloaterFriends() :
	LLFloater("Friends"),
	LLEventTimer(1000000),
	mObserver(NULL),
	mMenuState(0),
	mShowMaxSelectWarning(TRUE),
	mAllowRightsChange(TRUE),
	mNumRightsChanged(0)
{
	mTimer.stop();
	sInstance = this;
	mObserver = new LLLocalFriendsObserver(this);
	LLAvatarTracker::instance().addObserver(mObserver);
	gSavedSettings.setBOOL("ShowFriends", TRUE);
	// Builds and adds to gFloaterView
	gUICtrlFactory->buildFloater(this, "floater_friends.xml");
}

LLFloaterFriends::~LLFloaterFriends()
{
	LLAvatarTracker::instance().removeObserver(mObserver);
	delete mObserver;
	sInstance = NULL;
	gSavedSettings.setBOOL("ShowFriends", FALSE);
}

void LLFloaterFriends::tick()
{
	mTimer.stop();
	mPeriod = 1000000;
	mAllowRightsChange = TRUE;
	updateFriends(LLFriendObserver::ADD);
}

// static
void LLFloaterFriends::show(void*)
{
	if(sInstance)
	{
		sInstance->open();
	}
	else
	{
		LLFloaterFriends* self = new LLFloaterFriends;
		self->open();
	}
}


// static
BOOL LLFloaterFriends::visible(void*)
{
	return sInstance && sInstance->getVisible();
}


// static
void LLFloaterFriends::toggle(void*)
{
	if (sInstance)
	{
		sInstance->close();
	}
	else
	{
		show();
	}
}


void LLFloaterFriends::updateFriends(U32 changed_mask)
{
	LLUUID selected_id;
	LLCtrlListInterface *friends_list = sInstance->childGetListInterface("friend_list");
	if (!friends_list) return;
	LLCtrlScrollInterface *friends_scroll = sInstance->childGetScrollInterface("friend_list");
	if (!friends_scroll) return;
	
	// We kill the selection warning, otherwise we'll spam with warning popups
	// if the maximum amount of friends are selected
	mShowMaxSelectWarning = false;

	LLDynamicArray<LLUUID> selected_friends = sInstance->getSelectedIDs();
	if(changed_mask & (LLFriendObserver::ADD | LLFriendObserver::REMOVE | LLFriendObserver::ONLINE))
	{
		refreshNames();
	}
	else if(LLFriendObserver::POWERS)
	{
		--mNumRightsChanged;
		if(mNumRightsChanged > 0)
		{
			mPeriod = RIGHTS_CHANGE_TIMEOUT;	
			mTimer.start();
			mTimer.reset();
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
BOOL LLFloaterFriends::postBuild()
{
	
	mFriendsList = LLUICtrlFactory::getScrollListByName(this, "friend_list");
	mFriendsList->setMaxSelectable(MAX_FRIEND_SELECT);
	mFriendsList->setMaxiumumSelectCallback(onMaximumSelect);
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
	childSetAction("close_btn", onClickClose, this);

	refreshUI();
	return TRUE;
}


void LLFloaterFriends::addFriend(const std::string& name, const LLUUID& agent_id)
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

void LLFloaterFriends::reloadNames()
{
}

void LLFloaterFriends::refreshRightsChangeList(U8 state)
{
	LLDynamicArray<LLUUID> friends = getSelectedIDs();
	const LLRelationship* friend_status = NULL;
	if(friends.size() > 0) friend_status = LLAvatarTracker::instance().getBuddyInfo(friends[0]);

	LLSD row;
	bool can_change_visibility = false;
	bool can_change_modify = false;
	bool can_change_online_multiple = true;
	bool can_change_map_multiple = true;
	LLTextBox* processing_label = LLUICtrlFactory::getTextBoxByName(this, "process_rights_label");
	if(!mAllowRightsChange)
	{
		if(processing_label)
		{
			processing_label->setVisible(true);
			state = 0;
		}
	}
	else
	{
		if(processing_label)
		{
			processing_label->setVisible(false);
		}
	}
	if(state == 1)
	{
		if(!friend_status->isOnline())
		{
			childSetEnabled("offer_teleport_btn", false);
		}
		can_change_visibility = true;
		can_change_modify = true;
	}
	else if (state == 2)
	{
		can_change_visibility = true;
		can_change_modify = false;
		for(LLDynamicArray<LLUUID>::iterator itr = friends.begin(); itr != friends.end(); ++itr)
		{
			friend_status = LLAvatarTracker::instance().getBuddyInfo(*itr);
			if(!friend_status->isOnline())
			{
				childSetEnabled("offer_teleport_btn", false);
			}
			if(!friend_status->isRightGrantedTo(LLRelationship::GRANT_ONLINE_STATUS))
			{
				can_change_online_multiple = false;
				can_change_map_multiple = false;
			}
			else if(!friend_status->isRightGrantedTo(LLRelationship::GRANT_MAP_LOCATION))
			{
				can_change_map_multiple = false;
			}
		}
		
	}
	

	LLCheckboxCtrl* check;
	mMenuState = 0;

	check = LLUICtrlFactory::getCheckBoxByName(this, "online_status_cb");
	check->setEnabled(can_change_visibility);
	check->set(FALSE);
	if(!mAllowRightsChange) check->setVisible(FALSE);
	else check->setVisible(TRUE);
	if(friend_status)
	{
		check->set(friend_status->isRightGrantedTo(LLRelationship::GRANT_ONLINE_STATUS) && can_change_online_multiple);
		if(friend_status->isRightGrantedTo(LLRelationship::GRANT_ONLINE_STATUS)) mMenuState |= LLRelationship::GRANT_ONLINE_STATUS;
	}
	
	check = LLUICtrlFactory::getCheckBoxByName(this, "map_status_cb");
	check->setEnabled(false);
	check->set(FALSE);
	if(!mAllowRightsChange) check->setVisible(FALSE);
	else check->setVisible(TRUE);
	if(friend_status)
	{
		check->setEnabled(friend_status->isRightGrantedTo(LLRelationship::GRANT_ONLINE_STATUS) && can_change_visibility && can_change_online_multiple);
		check->set(friend_status->isRightGrantedTo(LLRelationship::GRANT_MAP_LOCATION) && can_change_map_multiple);
		if(friend_status->isRightGrantedTo(LLRelationship::GRANT_MAP_LOCATION)) mMenuState |= LLRelationship::GRANT_MAP_LOCATION;
	}

	check = LLUICtrlFactory::getCheckBoxByName(this, "modify_status_cb");
	check->setEnabled(can_change_modify);
	check->set(FALSE);
	if(!mAllowRightsChange) check->setVisible(FALSE);
	else check->setVisible(TRUE);
	if(can_change_modify) 
	{
		if(friend_status)
		{
			check->set(friend_status->isRightGrantedTo(LLRelationship::GRANT_MODIFY_OBJECTS));
			if(friend_status->isRightGrantedTo(LLRelationship::GRANT_MODIFY_OBJECTS)) mMenuState |= LLRelationship::GRANT_MODIFY_OBJECTS;
		}
	}
}

void LLFloaterFriends::refreshNames()
{
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
	mFriendsList->setScrollPos(pos);
}


void LLFloaterFriends::refreshUI()
{	
	BOOL single_selected = FALSE;
	BOOL multiple_selected = FALSE;
	int num_selected = mFriendsList->getAllSelected().size();
	if(num_selected > 0)
	{
		single_selected = TRUE;
		if(num_selected > 1) multiple_selected = TRUE;		
	}
	//Options that can only be performed with one friend selected
	childSetEnabled("profile_btn", single_selected && !multiple_selected);
	childSetEnabled("pay_btn", single_selected && !multiple_selected);

	//Options that can be performed with up to MAX_FRIEND_SELECT friends selected
	//(single_selected will always be true in this situations)
	childSetEnabled("remove_btn", single_selected);
	childSetEnabled("im_btn", single_selected);
	childSetEnabled("friend_rights", single_selected);

	//Note: We reset this in refreshRightsChangeList since we already have to iterate
	//through all selected friends there
	childSetEnabled("offer_teleport_btn", single_selected);

	refreshRightsChangeList((single_selected + multiple_selected));
}

// static
LLDynamicArray<LLUUID> LLFloaterFriends::getSelectedIDs()
{
	LLUUID selected_id;
	LLDynamicArray<LLUUID> friend_ids;
	if(sInstance)
	{
		std::vector<LLScrollListItem*> selected = sInstance->mFriendsList->getAllSelected();
		for(std::vector<LLScrollListItem*>::iterator itr = selected.begin(); itr != selected.end(); ++itr)
		{
			friend_ids.push_back((*itr)->getUUID());
		}
	}
	return friend_ids;
}

// static
void LLFloaterFriends::onSelectName(LLUICtrl* ctrl, void* user_data)
{
	if(sInstance)
	{
		sInstance->refreshUI();
	}
}

//static
void LLFloaterFriends::onMaximumSelect(void* user_data)
{
	LLString::format_map_t args;
	args["[MAX_SELECT]"] = llformat("%d", MAX_FRIEND_SELECT);
	LLNotifyBox::showXml("MaxListSelectMessage", args);
};

// static
void LLFloaterFriends::onClickProfile(void* user_data)
{
	//llinfos << "LLFloaterFriends::onClickProfile()" << llendl;
	LLDynamicArray<LLUUID> ids = getSelectedIDs();
	if(ids.size() > 0)
	{
		LLUUID agent_id = ids[0];
		BOOL online;
		online = LLAvatarTracker::instance().isBuddyOnline(agent_id);
		LLFloaterAvatarInfo::showFromFriend(agent_id, online);
	}
}

// static
void LLFloaterFriends::onClickIM(void* user_data)
{
	//llinfos << "LLFloaterFriends::onClickIM()" << llendl;
	LLDynamicArray<LLUUID> ids = getSelectedIDs();
	if(ids.size() > 0)
	{
		if(ids.size() == 1)
		{
			LLUUID agent_id = ids[0];
			const LLRelationship* info = LLAvatarTracker::instance().getBuddyInfo(agent_id);
			char first[DB_FIRST_NAME_BUF_SIZE];
			char last[DB_LAST_NAME_BUF_SIZE];
			if(info && gCacheName->getName(agent_id, first, last))
			{
				char buffer[MAX_STRING];
				snprintf(buffer, MAX_STRING, "%s %s", first, last);
				gIMView->setFloaterOpen(TRUE);
				gIMView->addSession(
					buffer,
					IM_NOTHING_SPECIAL,
					agent_id);
			}		
		}
		else
		{
			LLUUID session_id;
			session_id.generate();
			gIMView->setFloaterOpen(TRUE);
			gIMView->addSession(
				"Friends Conference",
				IM_SESSION_ADD,
				session_id,
				ids);
		}
	}
}

// static
void LLFloaterFriends::requestFriendship(const LLUUID& target_id, const LLString& target_name)
{
	// HACK: folder id stored as "message"
	LLUUID calling_card_folder_id = gInventory.findCategoryUUIDForType(LLAssetType::AT_CALLINGCARD);
	std::string message = calling_card_folder_id.getString();

	send_improved_im(target_id,
					 target_name.c_str(),
					 message.c_str(),
					 IM_ONLINE,
					 IM_FRIENDSHIP_OFFERED);
}

// static
void LLFloaterFriends::callbackAddFriend(S32 option, void* user_data)
{
	if (option == 0)
	{
		LLFloaterFriends* self = (LLFloaterFriends*)user_data;
		requestFriendship(self->mAddFriendID, self->mAddFriendName);
	}
}

// static
void LLFloaterFriends::onPickAvatar(const std::vector<std::string>& names,
									const std::vector<LLUUID>& ids,
									void* user_data)
{
	if (names.empty()) return;
	if (ids.empty()) return;

	LLFloaterFriends* self = (LLFloaterFriends*)user_data;
	self->mAddFriendID = ids[0];
	self->mAddFriendName = names[0];

	if(ids[0] == gAgentID)
	{
		LLNotifyBox::showXml("AddSelfFriend");
		return;
	}

	// TODO: accept a line of text with this dialog
	LLString::format_map_t args;
	args["[NAME]"] = names[0];
	gViewerWindow->alertXml("AddFriend", args, callbackAddFriend, user_data);
}

// static
void LLFloaterFriends::onClickAddFriend(void* user_data)
{
	LLFloaterAvatarPicker::show(onPickAvatar, user_data, FALSE, TRUE);
}

// static
void LLFloaterFriends::onClickRemove(void* user_data)
{
	//llinfos << "LLFloaterFriends::onClickRemove()" << llendl;
	LLDynamicArray<LLUUID> ids = getSelectedIDs();
	LLStringBase<char>::format_map_t args;
	if(ids.size() > 0)
	{
		LLString msgType = "RemoveFromFriends";
		if(ids.size() == 1)
		{
			LLUUID agent_id = ids[0];
			char first[DB_FIRST_NAME_BUF_SIZE];
			char last[DB_LAST_NAME_BUF_SIZE];
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
void LLFloaterFriends::onClickOfferTeleport(void*)
{
	LLDynamicArray<LLUUID> ids = getSelectedIDs();
	if(ids.size() > 0)
	{	
		handle_lure(ids);
	}
}

// static
void LLFloaterFriends::onClickPay(void*)
{
	LLDynamicArray<LLUUID> ids = getSelectedIDs();
	if(ids.size() == 1)
	{	
		handle_pay_by_id(ids[0]);
	}
}

// static
void LLFloaterFriends::onClickClose(void* user_data)
{
	//llinfos << "LLFloaterFriends::onClickClose()" << llendl;
	if(sInstance)
	{
		sInstance->onClose(false);
	}
}

void LLFloaterFriends::onClickOnlineStatus(LLUICtrl* ctrl, void* user_data)
{
	bool checked = ctrl->getValue();
	sInstance->updateMenuState(LLRelationship::GRANT_ONLINE_STATUS, checked);
	sInstance->applyRightsToFriends(LLRelationship::GRANT_ONLINE_STATUS, checked);
}

void LLFloaterFriends::onClickMapStatus(LLUICtrl* ctrl, void* user_data)
{
	bool checked = ctrl->getValue();
	sInstance->updateMenuState(LLRelationship::GRANT_MAP_LOCATION, checked);
	sInstance->applyRightsToFriends(LLRelationship::GRANT_MAP_LOCATION, checked);
}

void LLFloaterFriends::onClickModifyStatus(LLUICtrl* ctrl, void* user_data)
{
	bool checked = ctrl->getValue();
	LLDynamicArray<LLUUID> ids = getSelectedIDs();
	LLStringBase<char>::format_map_t args;
	if(ids.size() > 0)
	{
		if(ids.size() == 1)
		{
			LLUUID agent_id = ids[0];
			char first[DB_FIRST_NAME_BUF_SIZE];
			char last[DB_LAST_NAME_BUF_SIZE];
			if(gCacheName->getName(agent_id, first, last))
			{
				args["[FIRST_NAME]"] = first;
				args["[LAST_NAME]"] = last;	
			}
			if(checked) gViewerWindow->alertXml("GrantModifyRights", args, handleModifyRights, NULL);
			else gViewerWindow->alertXml("RevokeModifyRights", args, handleModifyRights, NULL);
		}
		else return;
	}
}

void LLFloaterFriends::handleModifyRights(S32 option, void* user_data)
{
	if(sInstance)
	{
		if(!option)
		{
			sInstance->updateMenuState(LLRelationship::GRANT_MODIFY_OBJECTS, !((sInstance->getMenuState() & LLRelationship::GRANT_MODIFY_OBJECTS) != 0));
			sInstance->applyRightsToFriends(LLRelationship::GRANT_MODIFY_OBJECTS, ((sInstance->getMenuState() & LLRelationship::GRANT_MODIFY_OBJECTS) != 0));
		}
		sInstance->refreshUI();
	}
}

void LLFloaterFriends::updateMenuState(S32 flag, BOOL value)
{
	if(value) mMenuState |= flag;
	else mMenuState &= ~flag;
}

void LLFloaterFriends::applyRightsToFriends(S32 flag, BOOL value)
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
void LLFloaterFriends::handleRemove(S32 option, void* user_data)
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
