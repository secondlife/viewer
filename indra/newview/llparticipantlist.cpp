/** 
 * @file llparticipantlist.cpp
 * @brief LLParticipantList intended to update view(LLAvatarList) according to incoming messages
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * 
 * Copyright (c) 2009, Linden Research, Inc.
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

// common includes
#include "lltrans.h"
#include "llavataractions.h"
#include "llagent.h"
#include "llimview.h"

#include "llparticipantlist.h"
#include "llavatarlist.h"
#include "llspeakers.h"
#include "llviewermenu.h"
#include "llvoiceclient.h"

//LLParticipantList retrieves add, clear and remove events and updates view accordingly 
#if LL_MSVC
#pragma warning (disable : 4355) // 'this' used in initializer list: yes, intentionally
#endif

static const LLAvatarItemAgentOnTopComparator AGENT_ON_TOP_NAME_COMPARATOR;

LLParticipantList::LLParticipantList(LLSpeakerMgr* data_source, LLAvatarList* avatar_list,  bool use_context_menu/* = true*/,
		bool exclude_agent /*= true*/):
	mSpeakerMgr(data_source),
	mAvatarList(avatar_list),
	mSortOrder(E_SORT_BY_NAME)
,	mParticipantListMenu(NULL)
,	mExcludeAgent(exclude_agent)
{
	mSpeakerAddListener = new SpeakerAddListener(*this);
	mSpeakerRemoveListener = new SpeakerRemoveListener(*this);
	mSpeakerClearListener = new SpeakerClearListener(*this);
	mSpeakerModeratorListener = new SpeakerModeratorUpdateListener(*this);
	mSpeakerMuteListener = new SpeakerMuteListener(*this);

	mSpeakerMgr->addListener(mSpeakerAddListener, "add");
	mSpeakerMgr->addListener(mSpeakerRemoveListener, "remove");
	mSpeakerMgr->addListener(mSpeakerClearListener, "clear");
	mSpeakerMgr->addListener(mSpeakerModeratorListener, "update_moderator");

	mAvatarList->setNoItemsCommentText(LLTrans::getString("LoadingData"));
	mAvatarListDoubleClickConnection = mAvatarList->setDoubleClickCallback(boost::bind(&LLParticipantList::onAvatarListDoubleClicked, this, mAvatarList));
	mAvatarListRefreshConnection = mAvatarList->setRefreshCompleteCallback(boost::bind(&LLParticipantList::onAvatarListRefreshed, this, _1, _2));
    // Set onAvatarListDoubleClicked as default on_return action.
	mAvatarListReturnConnection = mAvatarList->setReturnCallback(boost::bind(&LLParticipantList::onAvatarListDoubleClicked, this, mAvatarList));

	if (use_context_menu)
	{
		mParticipantListMenu = new LLParticipantListMenu(*this);
		mAvatarList->setContextMenu(mParticipantListMenu);
	}
	else
	{
		mAvatarList->setContextMenu(NULL);
	}

	//Lets fill avatarList with existing speakers
	LLAvatarList::uuid_vector_t& group_members = mAvatarList->getIDs();

	LLSpeakerMgr::speaker_list_t speaker_list;
	mSpeakerMgr->getSpeakerList(&speaker_list, true);
	for(LLSpeakerMgr::speaker_list_t::iterator it = speaker_list.begin(); it != speaker_list.end(); it++)
	{
		const LLPointer<LLSpeaker>& speakerp = *it;

		addAvatarIDExceptAgent(group_members, speakerp->mID);
		if ( speakerp->mIsModerator )
		{
			mModeratorList.insert(speakerp->mID);
		}
	}
	// we need to exclude agent id for non group chat
	mAvatarList->setDirty(true);
	sort();
}

LLParticipantList::~LLParticipantList()
{
	mAvatarListDoubleClickConnection.disconnect();
	mAvatarListRefreshConnection.disconnect();
	mAvatarListReturnConnection.disconnect();

	// It is possible Participant List will be re-created from LLCallFloater::onCurrentChannelChanged()
	// See ticket EXT-3427
	// hide menu before deleting it to stop enable and check handlers from triggering.
	if(mParticipantListMenu && !LLApp::isExiting())
	{
		mParticipantListMenu->hide();
	}

	if (mParticipantListMenu)
	{
		delete mParticipantListMenu;
		mParticipantListMenu = NULL;
	}
}

void LLParticipantList::setSpeakingIndicatorsVisible(BOOL visible)
{
	mAvatarList->setSpeakingIndicatorsVisible(visible);
};

void LLParticipantList::onAvatarListDoubleClicked(LLAvatarList* list)
{
	// NOTE(EM): Should we check if there is multiple selection and start conference if it is so?
	LLUUID clicked_id = list->getSelectedUUID();

	if (clicked_id.isNull() || clicked_id == gAgent.getID())
		return;
	
	LLAvatarActions::startIM(clicked_id);
}

void LLParticipantList::onAvatarListRefreshed(LLUICtrl* ctrl, const LLSD& param)
{
	LLAvatarList* list = dynamic_cast<LLAvatarList*>(ctrl);
	if (list)
	{
		const std::string moderator_indicator(LLTrans::getString("IM_moderator_label")); 
		const std::size_t moderator_indicator_len = moderator_indicator.length();

		// Firstly remove moderators indicator
		std::set<LLUUID>::const_iterator
			moderator_list_it = mModeratorToRemoveList.begin(),
			moderator_list_end = mModeratorToRemoveList.end();
		for (;moderator_list_it != moderator_list_end; ++moderator_list_it)
		{
			LLAvatarListItem* item = dynamic_cast<LLAvatarListItem*> (list->getItemByValue(*moderator_list_it));
			if ( item )
			{
				std::string name = item->getAvatarName();
				size_t found = name.find(moderator_indicator);
				if (found == std::string::npos)
				{
					name.erase(found, moderator_indicator_len);
					item->setName(name);
					item->reshapeAvatarName();
				}
			}
		}

		mModeratorToRemoveList.clear();

		// Add moderators indicator
		moderator_list_it = mModeratorList.begin();
		moderator_list_end = mModeratorList.end();
		for (;moderator_list_it != moderator_list_end; ++moderator_list_it)
		{
			LLAvatarListItem* item = dynamic_cast<LLAvatarListItem*> (list->getItemByValue(*moderator_list_it));
			if ( item )
			{
				std::string name = item->getAvatarName();
				size_t found = name.find(moderator_indicator);
				if (found == std::string::npos)
				{
					name += " ";
					name += moderator_indicator;
					item->setName(name);
					item->reshapeAvatarName();
				}
			}
		}
	}
}

void LLParticipantList::setSortOrder(EParticipantSortOrder order)
{
	if ( mSortOrder != order )
	{
		mSortOrder = order;
		sort();
	}
}

void LLParticipantList::refreshVoiceState()
{
	LLSpeakerMgr::speaker_list_t speakers;
	mSpeakerMgr->getSpeakerList(&speakers, TRUE);

	for (LLSpeakerMgr::speaker_list_t::iterator iter = speakers.begin();
		iter != speakers.end(); ++iter)
	{
		LLSpeaker* speakerp = (*iter).get();
		const LLUUID& speaker_id = speakerp->mID;
		LLAvatarListItem* item = dynamic_cast<LLAvatarListItem*> (mAvatarList->getItemByValue(speaker_id));
		if ( item )
		{
			// if voice is disabled for this speaker show non voice speakers as disabled
			bool is_in_voice = speakerp->mStatus > LLSpeaker::STATUS_VOICE_ACTIVE
				&& speakerp->mStatus != LLSpeaker::STATUS_MUTED;
			item->setOnline(!is_in_voice);
		}
	}
}

bool LLParticipantList::onAddItemEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata)
{
	LLAvatarList::uuid_vector_t& group_members = mAvatarList->getIDs();
	LLUUID uu_id = event->getValue().asUUID();

	LLAvatarList::uuid_vector_t::iterator found = std::find(group_members.begin(), group_members.end(), uu_id);
	if(found != group_members.end())
	{
		llinfos << "Already got a buddy" << llendl;
		return true;
	}

	addAvatarIDExceptAgent(group_members, uu_id);
	// Mark AvatarList as dirty one
	mAvatarList->setDirty();
	sort();
	return true;
}

bool LLParticipantList::onRemoveItemEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata)
{
	LLAvatarList::uuid_vector_t& group_members = mAvatarList->getIDs();
	LLAvatarList::uuid_vector_t::iterator pos = std::find(group_members.begin(), group_members.end(), event->getValue().asUUID());
	if(pos != group_members.end())
	{
		group_members.erase(pos);
		mAvatarList->setDirty();
	}
	return true;
}

bool LLParticipantList::onClearListEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata)
{
	LLAvatarList::uuid_vector_t& group_members = mAvatarList->getIDs();
	group_members.clear();
	mAvatarList->setDirty();
	return true;
}

bool LLParticipantList::onModeratorUpdateEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata)
{
	const LLSD& evt_data = event->getValue();
	if ( evt_data.has("id") && evt_data.has("is_moderator") )
	{
		LLUUID id = evt_data["id"];
		bool is_moderator = evt_data["is_moderator"];
		if ( id.notNull() )
		{
			if ( is_moderator )
				mModeratorList.insert(id);
			else
			{
				std::set<LLUUID>::iterator it = mModeratorList.find (id);
				if ( it != mModeratorList.end () )
				{
					mModeratorToRemoveList.insert(id);
					mModeratorList.erase(id);
				}
			}
		}
	}
	return true;
}

bool LLParticipantList::onSpeakerMuteEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata)
{
	LLPointer<LLSpeaker> speakerp = (LLSpeaker*)event->getSource();
	if (speakerp.isNull()) return false;

	// update UI on confirmation of moderator mutes
	if (event->getValue().asString() == "voice")
	{
		LLAvatarListItem* item = dynamic_cast<LLAvatarListItem*>(mAvatarList->getItemByValue(speakerp->mID));
		if (item)
		{
			LLOutputMonitorCtrl* indicator = item->getChild<LLOutputMonitorCtrl>("speaking_indicator");
			indicator->setIsMuted(speakerp->mModeratorMutedVoice);
		}
	}
	return true;
}

void LLParticipantList::sort()
{
	if ( !mAvatarList )
		return;

	// TODO: Implement more sorting orders after specs updating (EM)
	switch ( mSortOrder ) {
	case E_SORT_BY_NAME :
		// if mExcludeAgent == true , then no need to keep agent on top of the list
		if(mExcludeAgent)
		{
			mAvatarList->sortByName();
		}
		else
		{
			mAvatarList->setComparator(&AGENT_ON_TOP_NAME_COMPARATOR);
			mAvatarList->sort();
		}
		break;
	default :
		llwarns << "Unrecognized sort order for " << mAvatarList->getName() << llendl;
		return;
	}
}

void LLParticipantList::addAvatarIDExceptAgent(std::vector<LLUUID>& existing_list, const LLUUID& avatar_id)
{
	if (mExcludeAgent && gAgent.getID() == avatar_id) return;

	existing_list.push_back(avatar_id);
	adjustParticipant(avatar_id);
}

void LLParticipantList::adjustParticipant(const LLUUID& speaker_id)
{
	LLPointer<LLSpeaker> speakerp = mSpeakerMgr->findSpeaker(speaker_id);
	if (speakerp.isNull()) return;

	// add listener to process moderation changes
	speakerp->addListener(mSpeakerMuteListener);
}

//
// LLParticipantList::SpeakerAddListener
//
bool LLParticipantList::SpeakerAddListener::handleEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata)
{
	/**
	 * We need to filter speaking objects. These objects shouldn't appear in the list
	 * @c LLFloaterChat::addChat() in llviewermessage.cpp to get detailed call hierarchy
	 */
	const LLUUID& speaker_id = event->getValue().asUUID();
	LLPointer<LLSpeaker> speaker = mParent.mSpeakerMgr->findSpeaker(speaker_id);
	if(speaker.isNull() || speaker->mType == LLSpeaker::SPEAKER_OBJECT)
	{
		return false;
	}
	return mParent.onAddItemEvent(event, userdata);
}

//
// LLParticipantList::SpeakerRemoveListener
//
bool LLParticipantList::SpeakerRemoveListener::handleEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata)
{
	return mParent.onRemoveItemEvent(event, userdata);
}

//
// LLParticipantList::SpeakerClearListener
//
bool LLParticipantList::SpeakerClearListener::handleEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata)
{
	return mParent.onClearListEvent(event, userdata);
}

//
// LLParticipantList::SpeakerModeratorListener
//
bool LLParticipantList::SpeakerModeratorUpdateListener::handleEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata)
{
		return mParent.onModeratorUpdateEvent(event, userdata);
}

bool LLParticipantList::SpeakerMuteListener::handleEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata)
{
	return mParent.onSpeakerMuteEvent(event, userdata);
}

LLContextMenu* LLParticipantList::LLParticipantListMenu::createMenu()
{
	// set up the callbacks for all of the avatar menu items
	LLUICtrl::CommitCallbackRegistry::ScopedRegistrar registrar;
	LLUICtrl::EnableCallbackRegistry::ScopedRegistrar enable_registrar;
	
	registrar.add("ParticipantList.ToggleAllowTextChat", boost::bind(&LLParticipantList::LLParticipantListMenu::toggleAllowTextChat, this, _2));
	registrar.add("ParticipantList.ToggleMuteText", boost::bind(&LLParticipantList::LLParticipantListMenu::toggleMuteText, this, _2));

	registrar.add("Avatar.Profile",	boost::bind(&LLAvatarActions::showProfile, mUUIDs.front()));
	registrar.add("Avatar.IM", boost::bind(&LLAvatarActions::startIM, mUUIDs.front()));
	registrar.add("Avatar.AddFriend", boost::bind(&LLAvatarActions::requestFriendshipDialog, mUUIDs.front()));
	registrar.add("Avatar.BlockUnblock", boost::bind(&LLParticipantList::LLParticipantListMenu::toggleMuteVoice, this, _2));
	registrar.add("Avatar.Share", boost::bind(&LLAvatarActions::share, mUUIDs.front()));
	registrar.add("Avatar.Pay",	boost::bind(&LLAvatarActions::pay, mUUIDs.front()));
	registrar.add("Avatar.Call", boost::bind(&LLAvatarActions::startCall, mUUIDs.front()));

	registrar.add("ParticipantList.ModerateVoice", boost::bind(&LLParticipantList::LLParticipantListMenu::moderateVoice, this, _2));

	enable_registrar.add("ParticipantList.EnableItem", boost::bind(&LLParticipantList::LLParticipantListMenu::enableContextMenuItem,	this, _2));
	enable_registrar.add("ParticipantList.CheckItem",  boost::bind(&LLParticipantList::LLParticipantListMenu::checkContextMenuItem,	this, _2));

	// create the context menu from the XUI
	LLContextMenu* main_menu = LLUICtrlFactory::getInstance()->createFromFile<LLContextMenu>(
		"menu_participant_list.xml", LLMenuGL::sMenuContainer, LLViewerMenuHolderGL::child_registry_t::instance());

	main_menu->setItemVisible("Moderator Options", isGroupModerator());
	main_menu->arrangeAndClear();

	return main_menu;
}

void LLParticipantList::LLParticipantListMenu::show(LLView* spawning_view, const std::vector<LLUUID>& uuids, S32 x, S32 y)
{
	LLPanelPeopleMenus::ContextMenu::show(spawning_view, uuids, x, y);

	if (uuids.size() == 0) return;

	const LLUUID speaker_id = mUUIDs.front();
	BOOL is_muted = isMuted(speaker_id);

	if (is_muted)
	{
		LLMenuGL::sMenuContainer->childSetVisible("ModerateVoiceMuteSelected", false);
		LLMenuGL::sMenuContainer->childSetVisible("ModerateVoiceMuteOthers", false);
	}
	else
	{
		LLMenuGL::sMenuContainer->childSetVisible("ModerateVoiceUnMuteSelected", false);
		LLMenuGL::sMenuContainer->childSetVisible("ModerateVoiceUnMuteOthers", false);
	}
}

void LLParticipantList::LLParticipantListMenu::toggleAllowTextChat(const LLSD& userdata)
{

	LLIMSpeakerMgr* mgr = dynamic_cast<LLIMSpeakerMgr*>(mParent.mSpeakerMgr);
	if (mgr)
	{
		const LLUUID speaker_id = mUUIDs.front();
		mgr->toggleAllowTextChat(speaker_id);
	}
}

void LLParticipantList::LLParticipantListMenu::toggleMute(const LLSD& userdata, U32 flags)
{
	const LLUUID speaker_id = mUUIDs.front();
	BOOL is_muted = LLMuteList::getInstance()->isMuted(speaker_id, flags);
	std::string name;

	//fill in name using voice client's copy of name cache
	LLPointer<LLSpeaker> speakerp = mParent.mSpeakerMgr->findSpeaker(speaker_id);
	if (speakerp.isNull())
	{
		return;
	}

	name = speakerp->mDisplayName;

	LLMute mute(speaker_id, name, speakerp->mType == LLSpeaker::SPEAKER_AGENT ? LLMute::AGENT : LLMute::OBJECT);

	if (!is_muted)
	{
		LLMuteList::getInstance()->add(mute, flags);
	}
	else
	{
		LLMuteList::getInstance()->remove(mute, flags);
	}
}

void LLParticipantList::LLParticipantListMenu::toggleMuteText(const LLSD& userdata)
{
	toggleMute(userdata, LLMute::flagTextChat);
}

void LLParticipantList::LLParticipantListMenu::toggleMuteVoice(const LLSD& userdata)
{
	toggleMute(userdata, LLMute::flagVoiceChat);
}

bool LLParticipantList::LLParticipantListMenu::isGroupModerator()
{
	// Agent is in Group Call
	if(gAgent.isInGroup(mParent.mSpeakerMgr->getSessionID()))
	{
		// Agent is Moderator
		return mParent.mSpeakerMgr->findSpeaker(gAgentID)->mIsModerator;
	}
	return false;
}

bool LLParticipantList::LLParticipantListMenu::isMuted(const LLUUID& avatar_id)
{
	LLPointer<LLSpeaker> selected_speakerp = mParent.mSpeakerMgr->findSpeaker(avatar_id);
	if (!selected_speakerp) return true;

	return selected_speakerp->mStatus == LLSpeaker::STATUS_MUTED;
}

void LLParticipantList::LLParticipantListMenu::moderateVoice(const LLSD& userdata)
{
	if (!gAgent.getRegion()) return;

	bool moderate_selected = userdata.asString() == "selected";
	const LLUUID& selected_avatar_id = mUUIDs.front();
	bool is_muted = isMuted(selected_avatar_id);

	if (moderate_selected)
	{
		moderateVoiceParticipant(selected_avatar_id, is_muted);
	}
	else
	{
		moderateVoiceOtherParticipants(selected_avatar_id, is_muted);
	}
}

void LLParticipantList::LLParticipantListMenu::moderateVoiceParticipant(const LLUUID& avatar_id, bool unmute)
{
	LLIMSpeakerMgr* mgr = dynamic_cast<LLIMSpeakerMgr*>(mParent.mSpeakerMgr);
	if (mgr)
	{
		mgr->moderateVoiceParticipant(avatar_id, unmute);
	}
}

void LLParticipantList::LLParticipantListMenu::moderateVoiceOtherParticipants(const LLUUID& excluded_avatar_id, bool unmute)
{
	LLIMSpeakerMgr* mgr = dynamic_cast<LLIMSpeakerMgr*>(mParent.mSpeakerMgr);
	if (mgr)
	{
		mgr->moderateVoiceOtherParticipants(excluded_avatar_id, unmute);
	}
}

bool LLParticipantList::LLParticipantListMenu::enableContextMenuItem(const LLSD& userdata)
{
	std::string item = userdata.asString();
	if (item == "can_mute_text" || "can_block" == item)
	{
		return mUUIDs.front() != gAgentID;
	}
	else
		if (item == "can_allow_text_chat" || "can_moderate_voice" == item)
		{
			return isGroupModerator();
		}
	else if (item == std::string("can_add"))
		{
			// We can add friends if:
			// - there are selected people
			// - and there are no friends among selection yet.

			bool result = (mUUIDs.size() > 0);

			std::vector<LLUUID>::const_iterator
				id = mUUIDs.begin(),
				uuids_end = mUUIDs.end();

			for (;id != uuids_end; ++id)
			{
				if ( LLAvatarActions::isFriend(*id) )
				{
					result = false;
					break;
				}
			}
			return result;
		}
	else if (item == "can_call")
	{
		return LLVoiceClient::voiceEnabled();
	}

	return true;
}

bool LLParticipantList::LLParticipantListMenu::checkContextMenuItem(const LLSD& userdata)
{
	std::string item = userdata.asString();
	const LLUUID& id = mUUIDs.front();

	if (item == "is_muted")
	{
		return LLMuteList::getInstance()->isMuted(id, LLMute::flagTextChat);
	}
	else if (item == "is_allowed_text_chat")
	{
		LLPointer<LLSpeaker> selected_speakerp = mParent.mSpeakerMgr->findSpeaker(id);

		if (selected_speakerp.notNull())
		{
			return !selected_speakerp->mModeratorMutedText;
		}
	}
	else if(item == "is_blocked")
	{
		return LLMuteList::getInstance()->isMuted(id, LLMute::flagVoiceChat);
	}

	return false;
}

//EOF
