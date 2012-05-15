/** 
 * @file llparticipantlist.cpp
 * @brief LLParticipantList intended to update view(LLAvatarList) according to incoming messages
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

#include "llviewerprecompiledheaders.h"

// common includes
#include "lltrans.h"
#include "llavataractions.h"
#include "llagent.h"

#include "llimview.h"
#include "llpanelpeoplemenus.h"
#include "llnotificationsutil.h"
#include "llparticipantlist.h"
#include "llspeakers.h"
#include "llviewercontrol.h"
#include "llviewermenu.h"
#include "llvoiceclient.h"

//LLParticipantList retrieves add, clear and remove events and updates view accordingly 
#if LL_MSVC
#pragma warning (disable : 4355) // 'this' used in initializer list: yes, intentionally
#endif

static const LLAvatarItemAgentOnTopComparator AGENT_ON_TOP_NAME_COMPARATOR;

// helper function to update AvatarList Item's indicator in the voice participant list
static void update_speaker_indicator(const LLAvatarList* const avatar_list, const LLUUID& avatar_uuid, bool is_muted)
{
	LLAvatarListItem* item = dynamic_cast<LLAvatarListItem*>(avatar_list->getItemByValue(avatar_uuid));
	if (item)
	{
		LLOutputMonitorCtrl* indicator = item->getChild<LLOutputMonitorCtrl>("speaking_indicator");
		indicator->setIsMuted(is_muted);
	}
}


// See EXT-4301.
/**
 * class LLAvalineUpdater - observe the list of voice participants in session and check
 *  presence of Avaline Callers among them.
 *
 * LLAvalineUpdater is a LLVoiceClientParticipantObserver. It provides two kinds of validation:
 *	- whether Avaline caller presence among participants;
 *	- whether watched Avaline caller still exists in voice channel.
 * Both validations have callbacks which will notify subscriber if any of event occur.
 *
 * @see findAvalineCaller()
 * @see checkIfAvalineCallersExist()
 */
class LLAvalineUpdater : public LLVoiceClientParticipantObserver
{
public:
	typedef boost::function<void(const LLUUID& speaker_id)> process_avaline_callback_t;

	LLAvalineUpdater(process_avaline_callback_t found_cb, process_avaline_callback_t removed_cb)
		: mAvalineFoundCallback(found_cb)
		, mAvalineRemovedCallback(removed_cb)
	{
		LLVoiceClient::getInstance()->addObserver(this);
	}
	~LLAvalineUpdater()
	{
		if (LLVoiceClient::instanceExists())
		{
			LLVoiceClient::getInstance()->removeObserver(this);
		}
	}

	/**
	 * Adds UUID of Avaline caller to watch.
	 *
	 * @see checkIfAvalineCallersExist().
	 */
	void watchAvalineCaller(const LLUUID& avaline_caller_id)
	{
		mAvalineCallers.insert(avaline_caller_id);
	}

	void onParticipantsChanged()
	{
		uuid_set_t participant_uuids;
		LLVoiceClient::getInstance()->getParticipantList(participant_uuids);


		// check whether Avaline caller exists among voice participants
		// and notify Participant List
		findAvalineCaller(participant_uuids);

		// check whether watched Avaline callers still present among voice participant
		// and remove if absents.
		checkIfAvalineCallersExist(participant_uuids);
	}

private:
	typedef std::set<LLUUID> uuid_set_t;

	/**
	 * Finds Avaline callers among voice participants and calls mAvalineFoundCallback.
	 *
	 * When Avatar is in group call with Avaline caller and then ends call Avaline caller stays
	 * in Group Chat floater (exists in LLSpeakerMgr). If Avatar starts call with that group again
	 * Avaline caller is added to voice channel AFTER Avatar is connected to group call.
	 * But Voice Control Panel (VCP) is filled from session LLSpeakerMgr and there is no information
	 * if a speaker is Avaline caller.
	 *
	 * In this case this speaker is created as avatar and will be recreated when it appears in
	 * Avatar's Voice session.
	 *
	 * @see LLParticipantList::onAvalineCallerFound()
	 */
	void findAvalineCaller(const uuid_set_t& participant_uuids)
	{
		uuid_set_t::const_iterator it = participant_uuids.begin(), it_end = participant_uuids.end();

		for(; it != it_end; ++it)
		{
			const LLUUID& participant_id = *it;
			if (!LLVoiceClient::getInstance()->isParticipantAvatar(participant_id))
			{
				LL_DEBUGS("Avaline") << "Avaline caller found among voice participants: " << participant_id << LL_ENDL;

				if (mAvalineFoundCallback)
				{
					mAvalineFoundCallback(participant_id);
				}
			}
		}
	}

	/**
	 * Finds Avaline callers which are not anymore among voice participants and calls mAvalineRemovedCallback.
	 *
	 * The problem is when Avaline caller ends a call it is removed from Voice Client session but
	 * still exists in LLSpeakerMgr. Server does not send such information.
	 * This method implements a HUCK to notify subscribers that watched Avaline callers by class
	 * are not anymore in the call.
	 *
	 * @see LLParticipantList::onAvalineCallerRemoved()
	 */
	void checkIfAvalineCallersExist(const uuid_set_t& participant_uuids)
	{
		uuid_set_t::iterator it = mAvalineCallers.begin();
		uuid_set_t::const_iterator participants_it_end = participant_uuids.end();

		while (it != mAvalineCallers.end())
		{
			const LLUUID participant_id = *it;
			LL_DEBUGS("Avaline") << "Check avaline caller: " << participant_id << LL_ENDL;
			bool not_found = participant_uuids.find(participant_id) == participants_it_end;
			if (not_found)
			{
				LL_DEBUGS("Avaline") << "Watched Avaline caller is not found among voice participants: " << participant_id << LL_ENDL;

				// notify Participant List
				if (mAvalineRemovedCallback)
				{
					mAvalineRemovedCallback(participant_id);
				}

				// remove from the watch list
				mAvalineCallers.erase(it++);
			}
			else
			{
				++it;
			}
		}
	}

	process_avaline_callback_t mAvalineFoundCallback;
	process_avaline_callback_t mAvalineRemovedCallback;

	uuid_set_t mAvalineCallers;
};

LLParticipantList::LLParticipantList(LLSpeakerMgr* data_source,
									 LLAvatarList* avatar_list,
									 bool use_context_menu/* = true*/,
									 bool exclude_agent /*= true*/,
									 bool can_toggle_icons /*= true*/) :
	mSpeakerMgr(data_source),
	mAvatarList(avatar_list),
	mParticipantListMenu(NULL),
	mExcludeAgent(exclude_agent),
	mValidateSpeakerCallback(NULL)
{

	mAvalineUpdater = new LLAvalineUpdater(boost::bind(&LLParticipantList::onAvalineCallerFound, this, _1),
										   boost::bind(&LLParticipantList::onAvalineCallerRemoved, this, _1));

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
	LL_DEBUGS("SpeakingIndicator") << "Set session for speaking indicators: " << mSpeakerMgr->getSessionID() << LL_ENDL;
	mAvatarList->setSessionID(mSpeakerMgr->getSessionID());
	mAvatarListDoubleClickConnection = mAvatarList->setItemDoubleClickCallback(boost::bind(&LLParticipantList::onAvatarListDoubleClicked, this, _1));
	mAvatarListRefreshConnection = mAvatarList->setRefreshCompleteCallback(boost::bind(&LLParticipantList::onAvatarListRefreshed, this, _1, _2));
    // Set onAvatarListDoubleClicked as default on_return action.
	mAvatarListReturnConnection = mAvatarList->setReturnCallback(boost::bind(&LLParticipantList::onAvatarListDoubleClicked, this, mAvatarList));

	if (use_context_menu)
	{
		//mParticipantListMenu = new LLParticipantListMenu(*this);
		//mAvatarList->setContextMenu(mParticipantListMenu);
		mAvatarList->setContextMenu(&LLPanelPeopleMenus::gNearbyMenu);
	}
	else
	{
		mAvatarList->setContextMenu(NULL);
	}

	if (use_context_menu && can_toggle_icons)
	{
		mAvatarList->setShowIcons("ParticipantListShowIcons");
		mAvatarListToggleIconsConnection = gSavedSettings.getControl("ParticipantListShowIcons")->getSignal()->connect(boost::bind(&LLAvatarList::toggleIcons, mAvatarList));
	}

	//Lets fill avatarList with existing speakers
	LLSpeakerMgr::speaker_list_t speaker_list;
	mSpeakerMgr->getSpeakerList(&speaker_list, true);
	for(LLSpeakerMgr::speaker_list_t::iterator it = speaker_list.begin(); it != speaker_list.end(); it++)
	{
		const LLPointer<LLSpeaker>& speakerp = *it;

		addAvatarIDExceptAgent(speakerp->mID);
		if ( speakerp->mIsModerator )
		{
			mModeratorList.insert(speakerp->mID);
		}
		else
		{
			mModeratorToRemoveList.insert(speakerp->mID);
		}
	}
	// we need to exclude agent id for non group chat
	sort();
}

LLParticipantList::~LLParticipantList()
{
	mAvatarListDoubleClickConnection.disconnect();
	mAvatarListRefreshConnection.disconnect();
	mAvatarListReturnConnection.disconnect();
	mAvatarListToggleIconsConnection.disconnect();

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

	mAvatarList->setContextMenu(NULL);
	mAvatarList->setComparator(NULL);

	delete mAvalineUpdater;
}

void LLParticipantList::setSpeakingIndicatorsVisible(BOOL visible)
{
	mAvatarList->setSpeakingIndicatorsVisible(visible);
};

void LLParticipantList::onAvatarListDoubleClicked(LLUICtrl* ctrl)
{
	LLAvatarListItem* item = dynamic_cast<LLAvatarListItem*>(ctrl);
	if(!item)
	{
		return;
	}

	LLUUID clicked_id = item->getAvatarId();

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
				std::string tooltip = item->getAvatarToolTip();
				size_t found = name.find(moderator_indicator);
				if (found != std::string::npos)
				{
					name.erase(found, moderator_indicator_len);
					item->setAvatarName(name);
				}
				found = tooltip.find(moderator_indicator);
				if (found != tooltip.npos)
				{
					tooltip.erase(found, moderator_indicator_len);
					item->setAvatarToolTip(tooltip);
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
				std::string tooltip = item->getAvatarToolTip();
				size_t found = name.find(moderator_indicator);
				if (found == std::string::npos)
				{
					name += " ";
					name += moderator_indicator;
					item->setAvatarName(name);
				}
				found = tooltip.find(moderator_indicator);
				if (found == std::string::npos)
				{
					tooltip += " ";
					tooltip += moderator_indicator;
					item->setAvatarToolTip(tooltip);
				}
			}
		}

		// update voice mute state of all items. See EXT-7235
		LLSpeakerMgr::speaker_list_t speaker_list;

		// Use also participants which are not in voice session now (the second arg is TRUE).
		// They can already have mModeratorMutedVoice set from the previous voice session
		// and LLSpeakerVoiceModerationEvent will not be sent when speaker manager is updated next time.
		mSpeakerMgr->getSpeakerList(&speaker_list, TRUE);
		for(LLSpeakerMgr::speaker_list_t::iterator it = speaker_list.begin(); it != speaker_list.end(); it++)
		{
			const LLPointer<LLSpeaker>& speakerp = *it;

			if (speakerp->mStatus == LLSpeaker::STATUS_TEXT_ONLY)
			{
				update_speaker_indicator(list, speakerp->mID, speakerp->mModeratorMutedVoice);
			}
		}
	}
}

/*
  Seems this method is not necessary after onAvalineCallerRemoved was implemented;

  It does nothing because list item is always created with correct class type for Avaline caller.
  For now Avaline Caller is removed from the LLSpeakerMgr List when it is removed from the Voice Client
  session.
  This happens in two cases: if Avaline Caller ends call itself or if Resident ends group call.

  Probably Avaline caller should be removed from the LLSpeakerMgr list ONLY if it ends call itself.
  Asked in EXT-4301.
*/
void LLParticipantList::onAvalineCallerFound(const LLUUID& participant_id)
{
	LLPanel* item = mAvatarList->getItemByValue(participant_id);

	if (NULL == item)
	{
		LL_WARNS("Avaline") << "Something wrong. Unable to find item for: " << participant_id << LL_ENDL;
		return;
	}

	if (typeid(*item) == typeid(LLAvalineListItem))
	{
		LL_DEBUGS("Avaline") << "Avaline caller has already correct class type for: " << participant_id << LL_ENDL;
		// item representing an Avaline caller has a correct type already.
		return;
	}

	LL_DEBUGS("Avaline") << "remove item from the list and re-add it: " << participant_id << LL_ENDL;

	// remove UUID from LLAvatarList::mIDs to be able add it again.
	uuid_vec_t& ids = mAvatarList->getIDs();
	uuid_vec_t::iterator pos = std::find(ids.begin(), ids.end(), participant_id);
	ids.erase(pos);

	// remove item directly
	mAvatarList->removeItem(item);

	// re-add avaline caller with a correct class instance.
	addAvatarIDExceptAgent(participant_id);
}

void LLParticipantList::onAvalineCallerRemoved(const LLUUID& participant_id)
{
	LL_DEBUGS("Avaline") << "Removing avaline caller from the list: " << participant_id << LL_ENDL;

	mSpeakerMgr->removeAvalineSpeaker(participant_id);
}

void LLParticipantList::setSortOrder(EParticipantSortOrder order)
{
	const U32 speaker_sort_order = gSavedSettings.getU32("SpeakerParticipantDefaultOrder");

	if ( speaker_sort_order != order )
	{
		gSavedSettings.setU32("SpeakerParticipantDefaultOrder", (U32)order);
		sort();
	}
}

const LLParticipantList::EParticipantSortOrder LLParticipantList::getSortOrder() const
{
	const U32 speaker_sort_order = gSavedSettings.getU32("SpeakerParticipantDefaultOrder");
	return EParticipantSortOrder(speaker_sort_order);
}

void LLParticipantList::setValidateSpeakerCallback(validate_speaker_callback_t cb)
{
	mValidateSpeakerCallback = cb;
}

void LLParticipantList::update()
{
	mSpeakerMgr->update(true);

	if (E_SORT_BY_RECENT_SPEAKERS == getSortOrder() && !isHovered())
	{
		// Resort avatar list
		sort();
	}
}

bool LLParticipantList::isHovered()
{
	S32 x, y;
	LLUI::getMousePositionScreen(&x, &y);
	return mAvatarList->calcScreenRect().pointInRect(x, y);
}

bool LLParticipantList::onAddItemEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata)
{
	LLUUID uu_id = event->getValue().asUUID();

	if (mValidateSpeakerCallback && !mValidateSpeakerCallback(uu_id))
	{
		return true;
	}

	addAvatarIDExceptAgent(uu_id);
	sort();
	return true;
}

bool LLParticipantList::onRemoveItemEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata)
{
	uuid_vec_t& group_members = mAvatarList->getIDs();
	uuid_vec_t::iterator pos = std::find(group_members.begin(), group_members.end(), event->getValue().asUUID());
	if(pos != group_members.end())
	{
		group_members.erase(pos);
		mAvatarList->setDirty();
	}
	return true;
}

bool LLParticipantList::onClearListEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata)
{
	uuid_vec_t& group_members = mAvatarList->getIDs();
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

			// apply changes immediately
			onAvatarListRefreshed(mAvatarList, LLSD());
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
		update_speaker_indicator(mAvatarList, speakerp->mID, speakerp->mModeratorMutedVoice);
	}
	return true;
}

void LLParticipantList::sort()
{
	if ( !mAvatarList )
		return;

	switch ( getSortOrder() ) 
	{
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
		case E_SORT_BY_RECENT_SPEAKERS:
			if (mSortByRecentSpeakers.isNull())
				mSortByRecentSpeakers = new LLAvatarItemRecentSpeakerComparator(*this);
			mAvatarList->setComparator(mSortByRecentSpeakers.get());
			mAvatarList->sort();
			break;
		default :
			llwarns << "Unrecognized sort order for " << mAvatarList->getName() << llendl;
			return;
	}
}

void LLParticipantList::addAvatarIDExceptAgent(const LLUUID& avatar_id)
{
	if (mExcludeAgent && gAgent.getID() == avatar_id) return;
	if (mAvatarList->contains(avatar_id)) return;

	bool is_avatar = LLVoiceClient::getInstance()->isParticipantAvatar(avatar_id);

	if (is_avatar)
	{
		mAvatarList->getIDs().push_back(avatar_id);
		mAvatarList->setDirty();
	}
	else
	{
		std::string display_name = LLVoiceClient::getInstance()->getDisplayName(avatar_id);
		mAvatarList->addAvalineItem(avatar_id, mSpeakerMgr->getSessionID(), display_name.empty() ? LLTrans::getString("AvatarNameWaiting") : display_name);
		mAvalineUpdater->watchAvalineCaller(avatar_id);
	}
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
	 * @see LLFloaterChat::addChat() in llviewermessage.cpp to get detailed call hierarchy
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

/*LLContextMenu* LLParticipantList::LLParticipantListMenu::createMenu()
{
	// set up the callbacks for all of the avatar menu items
	LLUICtrl::CommitCallbackRegistry::ScopedRegistrar registrar;
	LLUICtrl::EnableCallbackRegistry::ScopedRegistrar enable_registrar;
	
	registrar.add("ParticipantList.Sort", boost::bind(&LLParticipantList::LLParticipantListMenu::sortParticipantList, this, _2));
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
	enable_registrar.add("ParticipantList.EnableItem.Moderate", boost::bind(&LLParticipantList::LLParticipantListMenu::enableModerateContextMenuItem,	this, _2));
	enable_registrar.add("ParticipantList.CheckItem",  boost::bind(&LLParticipantList::LLParticipantListMenu::checkContextMenuItem,	this, _2));

	// create the context menu from the XUI
	LLContextMenu* main_menu = createFromFile("menu_participant_list.xml");

	// Don't show sort options for P2P chat
	bool is_sort_visible = (mParent.mAvatarList && mParent.mAvatarList->size() > 1);
	main_menu->setItemVisible("SortByName", is_sort_visible);
	main_menu->setItemVisible("SortByRecentSpeakers", is_sort_visible);
	main_menu->setItemVisible("Moderator Options Separator", isGroupModerator());
	main_menu->setItemVisible("Moderator Options", isGroupModerator());
	main_menu->setItemVisible("View Icons Separator", mParent.mAvatarListToggleIconsConnection.connected());
	main_menu->setItemVisible("View Icons", mParent.mAvatarListToggleIconsConnection.connected());
	main_menu->arrangeAndClear();

	return main_menu;
}*/

void LLParticipantList::LLParticipantListMenu::show(LLView* spawning_view, const uuid_vec_t& uuids, S32 x, S32 y)
{
	if (uuids.size() == 0) return;

	LLListContextMenu::show(spawning_view, uuids, x, y);

	const LLUUID& speaker_id = mUUIDs.front();
	BOOL is_muted = isMuted(speaker_id);

	if (is_muted)
	{
		LLMenuGL::sMenuContainer->getChildView("ModerateVoiceMuteSelected")->setVisible( false);
	}
	else
	{
		LLMenuGL::sMenuContainer->getChildView("ModerateVoiceUnMuteSelected")->setVisible( false);
	}
}

void LLParticipantList::LLParticipantListMenu::sortParticipantList(const LLSD& userdata)
{
	std::string param = userdata.asString();
	if ("sort_by_name" == param)
	{
		mParent.setSortOrder(E_SORT_BY_NAME);
	}
	else if ("sort_by_recent_speakers" == param)
	{
		mParent.setSortOrder(E_SORT_BY_RECENT_SPEAKERS);
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
		LL_WARNS("Speakers") << "Speaker " << speaker_id << " not found" << llendl;
		return;
	}
	LLAvatarListItem* item = dynamic_cast<LLAvatarListItem*>(mParent.mAvatarList->getItemByValue(speaker_id));
	if (NULL == item) return;

	name = item->getAvatarName();

	LLMute::EType mute_type;
	switch (speakerp->mType)
	{
		case LLSpeaker::SPEAKER_AGENT:
			mute_type = LLMute::AGENT;
			break;
		case LLSpeaker::SPEAKER_OBJECT:
			mute_type = LLMute::OBJECT;
			break;
		case LLSpeaker::SPEAKER_EXTERNAL:
		default:
			mute_type = LLMute::EXTERNAL;
			break;
	}
	LLMute mute(speaker_id, name, mute_type);

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
	if (!mParent.mSpeakerMgr)
	{
		llwarns << "Speaker manager is missing" << llendl;
		return false;
	}

	// Is session a group call/chat?
	if(gAgent.isInGroup(mParent.mSpeakerMgr->getSessionID()))
	{
		LLSpeaker* speaker = mParent.mSpeakerMgr->findSpeaker(gAgentID).get();

		// Is agent a moderator?
		return speaker && speaker->mIsModerator;
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

	if (moderate_selected)
	{
		const LLUUID& selected_avatar_id = mUUIDs.front();
		bool is_muted = isMuted(selected_avatar_id);
		moderateVoiceParticipant(selected_avatar_id, is_muted);
	}
	else
	{
		bool unmute_all = userdata.asString() == "unmute_all";
		moderateVoiceAllParticipants(unmute_all);
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

void LLParticipantList::LLParticipantListMenu::moderateVoiceAllParticipants(bool unmute)
{
	LLIMSpeakerMgr* mgr = dynamic_cast<LLIMSpeakerMgr*>(mParent.mSpeakerMgr);
	if (mgr)
	{
		if (!unmute)
		{
			LLSD payload;
			payload["session_id"] = mgr->getSessionID();
			LLNotificationsUtil::add("ConfirmMuteAll", LLSD(), payload, confirmMuteAllCallback);
			return;
		}

		mgr->moderateVoiceAllParticipants(unmute);
	}
}

// static
void LLParticipantList::LLParticipantListMenu::confirmMuteAllCallback(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	// if Cancel pressed
	if (option == 1)
	{
		return;
	}

	const LLSD& payload = notification["payload"];
	const LLUUID& session_id = payload["session_id"];

	LLIMSpeakerMgr * speaker_manager = dynamic_cast<LLIMSpeakerMgr*> (
		LLIMModel::getInstance()->getSpeakerManager(session_id));
	if (speaker_manager)
	{
		speaker_manager->moderateVoiceAllParticipants(false);
	}

	return;
}


bool LLParticipantList::LLParticipantListMenu::enableContextMenuItem(const LLSD& userdata)
{
	std::string item = userdata.asString();
	const LLUUID& participant_id = mUUIDs.front();

	// For now non of "can_view_profile" action and menu actions listed below except "can_block"
	// can be performed for Avaline callers.
	bool is_participant_avatar = LLVoiceClient::getInstance()->isParticipantAvatar(participant_id);
	if (!is_participant_avatar && "can_block" != item) return false;

	if (item == "can_mute_text" || "can_block" == item || "can_share" == item || "can_im" == item 
		|| "can_pay" == item)
	{
		return mUUIDs.front() != gAgentID;
	}
	else if (item == std::string("can_add"))
	{
		// We can add friends if:
		// - there are selected people
		// - and there are no friends among selection yet.

		bool result = (mUUIDs.size() > 0);

		uuid_vec_t::const_iterator
			id = mUUIDs.begin(),
			uuids_end = mUUIDs.end();

		for (;id != uuids_end; ++id)
		{
			if ( *id == gAgentID || LLAvatarActions::isFriend(*id) )
			{
				result = false;
				break;
			}
		}
		return result;
	}
	else if (item == "can_call")
	{
		bool not_agent = mUUIDs.front() != gAgentID;
		bool can_call = not_agent &&  LLVoiceClient::getInstance()->voiceEnabled() && LLVoiceClient::getInstance()->isVoiceWorking();
		return can_call;
	}

	return true;
}

/*
  Processed menu items with such parameters:
  can_allow_text_chat
  can_moderate_voice
*/
bool LLParticipantList::LLParticipantListMenu::enableModerateContextMenuItem(const LLSD& userdata)
{
	// only group moderators can perform actions related to this "enable callback"
	if (!isGroupModerator()) return false;

	const LLUUID& participant_id = mUUIDs.front();
	LLPointer<LLSpeaker> speakerp = mParent.mSpeakerMgr->findSpeaker(participant_id);

	// not in voice participants can not be moderated
	bool speaker_in_voice = speakerp.notNull() && speakerp->isInVoiceChannel();

	const std::string& item = userdata.asString();

	if ("can_moderate_voice" == item)
	{
		return speaker_in_voice;
	}

	// For now non of menu actions except "can_moderate_voice" can be performed for Avaline callers.
	bool is_participant_avatar = LLVoiceClient::getInstance()->isParticipantAvatar(participant_id);
	if (!is_participant_avatar) return false;

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
	else if(item == "is_sorted_by_name")
	{
		return E_SORT_BY_NAME == mParent.getSortOrder();
	}
	else if(item == "is_sorted_by_recent_speakers")
	{
		return E_SORT_BY_RECENT_SPEAKERS == mParent.getSortOrder();
	}

	return false;
}

bool LLParticipantList::LLAvatarItemRecentSpeakerComparator::doCompare(const LLAvatarListItem* avatar_item1, const LLAvatarListItem* avatar_item2) const
{
	if (mParent.mSpeakerMgr)
	{
		LLPointer<LLSpeaker> lhs = mParent.mSpeakerMgr->findSpeaker(avatar_item1->getAvatarId());
		LLPointer<LLSpeaker> rhs = mParent.mSpeakerMgr->findSpeaker(avatar_item2->getAvatarId());
		if ( lhs.notNull() && rhs.notNull() )
		{
			// Compare by last speaking time
			if( lhs->mLastSpokeTime != rhs->mLastSpokeTime )
				return ( lhs->mLastSpokeTime > rhs->mLastSpokeTime );
			else if ( lhs->mSortIndex != rhs->mSortIndex )
				return ( lhs->mSortIndex < rhs->mSortIndex );
		}
		else if ( lhs.notNull() )
		{
			// True if only avatar_item1 speaker info available
			return true;
		}
		else if ( rhs.notNull() )
		{
			// False if only avatar_item2 speaker info available
			return false;
		}
	}
	// By default compare by name.
	return LLAvatarItemNameComparator::doCompare(avatar_item1, avatar_item2);
}

//EOF
