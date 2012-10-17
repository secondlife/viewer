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
#include "llavatarnamecache.h"
#include "llavatarname.h"
#include "llagent.h"

#include "llimview.h"
#include "llimfloatercontainer.h"
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
	if (avatar_list)
	{
		LLAvatarListItem* item = dynamic_cast<LLAvatarListItem*>(avatar_list->getItemByValue(avatar_uuid));
		if (item)
		{
			LLOutputMonitorCtrl* indicator = item->getChild<LLOutputMonitorCtrl>("speaking_indicator");
			indicator->setIsMuted(is_muted);
		}
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
									 LLFolderViewModelInterface& root_view_model,
									 bool use_context_menu/* = true*/,
									 bool exclude_agent /*= true*/,
									 bool can_toggle_icons /*= true*/) :
	LLConversationItemSession(data_source->getSessionID(), root_view_model),
	mSpeakerMgr(data_source),
	mAvatarList(avatar_list),
	mExcludeAgent(exclude_agent),
	mValidateSpeakerCallback(NULL)
{

	mAvalineUpdater = new LLAvalineUpdater(boost::bind(&LLParticipantList::onAvalineCallerFound, this, _1),
										   boost::bind(&LLParticipantList::onAvalineCallerRemoved, this, _1));

	mSpeakerAddListener = new SpeakerAddListener(*this);
	mSpeakerRemoveListener = new SpeakerRemoveListener(*this);
	mSpeakerClearListener = new SpeakerClearListener(*this);
	mSpeakerModeratorListener = new SpeakerModeratorUpdateListener(*this);
	mSpeakerUpdateListener = new SpeakerUpdateListener(*this);
	mSpeakerMuteListener = new SpeakerMuteListener(*this);

	mSpeakerMgr->addListener(mSpeakerAddListener, "add");
	mSpeakerMgr->addListener(mSpeakerRemoveListener, "remove");
	mSpeakerMgr->addListener(mSpeakerClearListener, "clear");
	mSpeakerMgr->addListener(mSpeakerModeratorListener, "update_moderator");
	mSpeakerMgr->addListener(mSpeakerUpdateListener, "update_speaker");

	setSessionID(mSpeakerMgr->getSessionID());
	
	if (mAvatarList)
	{
		mAvatarList->setNoItemsCommentText(LLTrans::getString("LoadingData"));
		LL_DEBUGS("SpeakingIndicator") << "Set session for speaking indicators: " << mSpeakerMgr->getSessionID() << LL_ENDL;
		mAvatarList->setSessionID(mSpeakerMgr->getSessionID());
		mAvatarListDoubleClickConnection = mAvatarList->setItemDoubleClickCallback(boost::bind(&LLParticipantList::onAvatarListDoubleClicked, this, _1));
		mAvatarListRefreshConnection = mAvatarList->setRefreshCompleteCallback(boost::bind(&LLParticipantList::onAvatarListRefreshed, this, _1, _2));
		// Set onAvatarListDoubleClicked as default on_return action.
		mAvatarListReturnConnection = mAvatarList->setReturnCallback(boost::bind(&LLParticipantList::onAvatarListDoubleClicked, this, mAvatarList));

		if (use_context_menu)
		{
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
	
	// Identify and store what kind of session we are
	LLIMModel::LLIMSession* im_session = LLIMModel::getInstance()->findIMSession(data_source->getSessionID());
	if (im_session)
	{
		// By default, sessions that can't be identified as group or ad-hoc will be considered P2P (i.e. 1 on 1)
		mConvType = CONV_SESSION_1_ON_1;
		if (im_session->isAdHocSessionType())
		{
			mConvType = CONV_SESSION_AD_HOC;
		}
		else if (im_session->isGroupSessionType())
		{
			mConvType = CONV_SESSION_GROUP;
		}
	}
	else 
	{
		// That's the only session that doesn't get listed in the LLIMModel as a session...
		mConvType = CONV_SESSION_NEARBY;
	}
}

LLParticipantList::~LLParticipantList()
{
	if (mAvatarList)
	{
		mAvatarListDoubleClickConnection.disconnect();
		mAvatarListRefreshConnection.disconnect();
		mAvatarListReturnConnection.disconnect();
		mAvatarListToggleIconsConnection.disconnect();
	}

	if (mAvatarList)
	{
		mAvatarList->setContextMenu(NULL);
		mAvatarList->setComparator(NULL);
	}

	delete mAvalineUpdater;
}

void LLParticipantList::setSpeakingIndicatorsVisible(BOOL visible)
{
	if (mAvatarList)
	{
		mAvatarList->setSpeakingIndicatorsVisible(visible);
	}
}

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
	const std::string moderator_indicator(LLTrans::getString("IM_moderator_label")); 
	const std::size_t moderator_indicator_len = moderator_indicator.length();

	// Firstly remove moderators indicator
	std::set<LLUUID>::const_iterator
		moderator_list_it = mModeratorToRemoveList.begin(),
		moderator_list_end = mModeratorToRemoveList.end();
	for (;moderator_list_it != moderator_list_end; ++moderator_list_it)
	{
		LLAvatarListItem* item = (list ? dynamic_cast<LLAvatarListItem*> (list->getItemByValue(*moderator_list_it)) : NULL);
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
		setParticipantIsModerator(*moderator_list_it,false);
	}

	mModeratorToRemoveList.clear();

	// Add moderators indicator
	moderator_list_it = mModeratorList.begin();
	moderator_list_end = mModeratorList.end();
	for (;moderator_list_it != moderator_list_end; ++moderator_list_it)
	{
		LLAvatarListItem* item = (list ? dynamic_cast<LLAvatarListItem*> (list->getItemByValue(*moderator_list_it)) : NULL);
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
		setParticipantIsModerator(*moderator_list_it,true);
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
			setParticipantIsMuted(speakerp->mID, speakerp->mModeratorMutedVoice);
			update_speaker_indicator(list, speakerp->mID, speakerp->mModeratorMutedVoice);
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
	if (mAvatarList)
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
	}
	
	LLConversationItemParticipant* participant = findParticipant(participant_id);
	if (participant)
	{
		removeParticipant(participant);
	}

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

	// Need to resort the participant list if it's in sort by recent speaker order.
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
	return (mAvatarList ? mAvatarList->calcScreenRect().pointInRect(x, y) : false);
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
	LLUUID avatar_id = event->getValue().asUUID();
	if (mAvatarList)
	{
		uuid_vec_t& group_members = mAvatarList->getIDs();
		uuid_vec_t::iterator pos = std::find(group_members.begin(), group_members.end(), avatar_id);
		if(pos != group_members.end())
		{
			group_members.erase(pos);
			mAvatarList->setDirty();
		}
	}
	removeParticipant(avatar_id);
	return true;
}

bool LLParticipantList::onClearListEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata)
{
	if (mAvatarList)
	{
		uuid_vec_t& group_members = mAvatarList->getIDs();
		group_members.clear();
		mAvatarList->setDirty();
	}
	clearParticipants();
	return true;
}

bool LLParticipantList::onSpeakerUpdateEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata)
{
	const LLSD& evt_data = event->getValue();
	if ( evt_data.has("id") )
	{
		LLUUID participant_id = evt_data["id"];
		LLIMFloaterContainer* im_box = LLIMFloaterContainer::findInstance();
		if (im_box)
		{
			im_box->setTimeNow(mUUID,participant_id);
		}
	}
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
		setParticipantIsMuted(speakerp->mID, speakerp->mModeratorMutedVoice);
		update_speaker_indicator(mAvatarList, speakerp->mID, speakerp->mModeratorMutedVoice);
	}
	return true;
}

void LLParticipantList::sort()
{
	// *TODO : Merov : Need to plan for sort() for LLConversationModel
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
	// Do not add if already in there or excluded for some reason
	if (mExcludeAgent && gAgent.getID() == avatar_id) return;
	if (mAvatarList && mAvatarList->contains(avatar_id)) return;
	if (findParticipant(avatar_id)) return;

	bool is_avatar = LLVoiceClient::getInstance()->isParticipantAvatar(avatar_id);

	LLConversationItemParticipant* participant = NULL;
	
	if (is_avatar)
	{
		// Create a participant view model instance
		LLAvatarName avatar_name;
		bool has_name = LLAvatarNameCache::get(avatar_id, &avatar_name);
		participant = new LLConversationItemParticipant(!has_name ? LLTrans::getString("AvatarNameWaiting") : avatar_name.mDisplayName , avatar_id, mRootViewModel);
		// Binds avatar's name update callback
		LLAvatarNameCache::get(avatar_id, boost::bind(&LLConversationItemParticipant::onAvatarNameCache, participant, _2));
		if (mAvatarList)
		{
			mAvatarList->getIDs().push_back(avatar_id);
			mAvatarList->setDirty();
		}
	}
	else
	{
		std::string display_name = LLVoiceClient::getInstance()->getDisplayName(avatar_id);
		// Create a participant view model instance
		participant = new LLConversationItemParticipant(display_name.empty() ? LLTrans::getString("AvatarNameWaiting") : display_name, avatar_id, mRootViewModel);
		if (mAvatarList)
		{
			mAvatarList->addAvalineItem(avatar_id, mSpeakerMgr->getSessionID(), display_name.empty() ? LLTrans::getString("AvatarNameWaiting") : display_name);
		}
		mAvalineUpdater->watchAvalineCaller(avatar_id);
	}

	// *TODO : Merov : need to update the online/offline status of the participant.
	// Hack for this: LLAvatarTracker::instance().isBuddyOnline(avatar_id))

	// Add the participant model to the session's children list
	addParticipant(participant);

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
// LLParticipantList::SpeakerUpdateListener
//
bool LLParticipantList::SpeakerUpdateListener::handleEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata)
{
	return mParent.onSpeakerUpdateEvent(event, userdata);
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
