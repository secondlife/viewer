/** 
 * @file llconversationmodel.cpp
 * @brief Implementation of conversations list
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

#include "llagent.h"
#include "llavatarnamecache.h"
#include "llavataractions.h"
#include "llevents.h"
#include "llfloaterimsession.h"
#include "llsdutil.h"
#include "llconversationmodel.h"
#include "llimview.h" //For LLIMModel
#include "lltrans.h"

#include <boost/foreach.hpp>

//
// Conversation items : common behaviors
//

LLConversationItem::LLConversationItem(std::string display_name, const LLUUID& uuid, LLFolderViewModelInterface& root_view_model) :
	LLFolderViewModelItemCommon(root_view_model),
	mName(display_name),
	mUUID(uuid),
	mNeedsRefresh(true),
	mConvType(CONV_UNKNOWN),
	mLastActiveTime(0.0),
	mDisplayModeratorOptions(false),
	mDisplayGroupBanOptions(false),
	mAvatarNameCacheConnection()
{
}

LLConversationItem::LLConversationItem(const LLUUID& uuid, LLFolderViewModelInterface& root_view_model) :
	LLFolderViewModelItemCommon(root_view_model),
	mName(""),
	mUUID(uuid),
	mNeedsRefresh(true),
	mConvType(CONV_UNKNOWN),
	mLastActiveTime(0.0),
	mDisplayModeratorOptions(false),
	mDisplayGroupBanOptions(false),
	mAvatarNameCacheConnection()
{
}

LLConversationItem::LLConversationItem(LLFolderViewModelInterface& root_view_model) :
	LLFolderViewModelItemCommon(root_view_model),
	mName(""),
	mUUID(),
	mNeedsRefresh(true),
	mConvType(CONV_UNKNOWN),
	mLastActiveTime(0.0),
	mDisplayModeratorOptions(false),
	mDisplayGroupBanOptions(false),
	mAvatarNameCacheConnection()
{
}

LLConversationItem::~LLConversationItem()
{
	// Disconnect any previous avatar name cache connection to ensure
	// that the callback method is not called after destruction
	if (mAvatarNameCacheConnection.connected())
	{
		mAvatarNameCacheConnection.disconnect();
	}
}

void LLConversationItem::postEvent(const std::string& event_type, LLConversationItemSession* session, LLConversationItemParticipant* participant)
{
	LLUUID session_id = (session ? session->getUUID() : LLUUID());
	LLUUID participant_id = (participant ? participant->getUUID() : LLUUID());
	LLSD event(LLSDMap("type", event_type)("session_uuid", session_id)("participant_uuid", participant_id));
	LLEventPumps::instance().obtain("ConversationsEvents").post(event);
}

// Virtual action callbacks
void LLConversationItem::performAction(LLInventoryModel* model, std::string action)
{
}

void LLConversationItem::openItem( void )
{
}

void LLConversationItem::closeItem( void )
{
}

void LLConversationItem::previewItem( void )
{
}

void LLConversationItem::showProperties(void)
{
}

void LLConversationItem::buildParticipantMenuOptions(menuentry_vec_t& items, U32 flags)
{
	if (flags & ITEM_IN_MULTI_SELECTION)
	{
		items.push_back(std::string("im"));
		items.push_back(std::string("offer_teleport"));
		items.push_back(std::string("voice_call"));
		items.push_back(std::string("remove_friends"));
	}
	else 
	{
		items.push_back(std::string("view_profile"));
		items.push_back(std::string("im"));
		items.push_back(std::string("offer_teleport"));
		items.push_back(std::string("request_teleport"));

		if (getType() != CONV_SESSION_1_ON_1)
		{
			items.push_back(std::string("voice_call"));
		}
		else
		{
			LLVoiceChannel* voice_channel = LLIMModel::getInstance() ? LLIMModel::getInstance()->getVoiceChannel(this->getUUID()) : NULL;
			if(voice_channel != LLVoiceChannel::getCurrentVoiceChannel())
			{
				items.push_back(std::string("voice_call"));
			}
			else
			{
				items.push_back(std::string("disconnect_from_voice"));
			}
		}

		items.push_back(std::string("chat_history"));
		items.push_back(std::string("separator_chat_history"));
		items.push_back(std::string("add_friend"));
		items.push_back(std::string("remove_friend"));
		items.push_back(std::string("invite_to_group"));
		items.push_back(std::string("separator_invite_to_group"));
		if (static_cast<LLConversationItem*>(mParent)->getType() == CONV_SESSION_NEARBY)
			items.push_back(std::string("zoom_in"));
		items.push_back(std::string("map"));
		items.push_back(std::string("share"));
		items.push_back(std::string("pay"));
		items.push_back(std::string("block_unblock"));
		items.push_back(std::string("MuteText"));

		if ((getType() != CONV_SESSION_1_ON_1) && mDisplayModeratorOptions)
		{
			items.push_back(std::string("Moderator Options Separator"));
			items.push_back(std::string("Moderator Options"));
			items.push_back(std::string("AllowTextChat"));
			items.push_back(std::string("moderate_voice_separator"));
			items.push_back(std::string("ModerateVoiceMuteSelected"));
			items.push_back(std::string("ModerateVoiceUnMuteSelected"));
			items.push_back(std::string("ModerateVoiceMute"));
			items.push_back(std::string("ModerateVoiceUnmute"));
		}

		if ((getType() != CONV_SESSION_1_ON_1) && mDisplayGroupBanOptions)
		{
			items.push_back(std::string("Group Ban Separator"));
			items.push_back(std::string("BanMember"));
		}
	}
}

// method does subscription to changes in avatar name cache for current session/participant conversation item.
void LLConversationItem::fetchAvatarName(bool isParticipant /*= true*/)
{
	LLUUID item_id = getUUID();

	// item should not be null for participants
	if (isParticipant)
	{
		llassert(item_id.notNull());
	}

	// disconnect any previous avatar name cache connection
	if (mAvatarNameCacheConnection.connected())
	{
		mAvatarNameCacheConnection.disconnect();
	}

	// exclude nearby chat item
	if (item_id.notNull())
	{
		// for P2P session item, override it as item of called agent
		if (CONV_SESSION_1_ON_1 == getType())
		{
			item_id = LLIMModel::getInstance()->getOtherParticipantID(item_id);
		}

		// subscribe on avatar name cache changes for participant and session items
		mAvatarNameCacheConnection = LLAvatarNameCache::get(item_id, boost::bind(&LLConversationItem::onAvatarNameCache, this, _2));
	}
}

//
// LLConversationItemSession
// 

LLConversationItemSession::LLConversationItemSession(std::string display_name, const LLUUID& uuid, LLFolderViewModelInterface& root_view_model) :
	LLConversationItem(display_name,uuid,root_view_model),
	mIsLoaded(false)
{
	mConvType = CONV_SESSION_UNKNOWN;
}

LLConversationItemSession::LLConversationItemSession(const LLUUID& uuid, LLFolderViewModelInterface& root_view_model) :
	LLConversationItem(uuid,root_view_model)
{
	mConvType = CONV_SESSION_UNKNOWN;
}

bool LLConversationItemSession::hasChildren() const
{
	return getChildrenCount() > 0;
}

void LLConversationItemSession::addParticipant(LLConversationItemParticipant* participant)
{
	addChild(participant);
	mIsLoaded = true;
	mNeedsRefresh = true;
	updateName(participant);
	postEvent("add_participant", this, participant);
}

void LLConversationItemSession::updateName(LLConversationItemParticipant* participant)
{
	EConversationType conversation_type = getType();
	// We modify the session name only in the case of an ad-hoc session or P2P session, exit otherwise (nothing to do)
	if ((conversation_type != CONV_SESSION_AD_HOC) && (conversation_type != CONV_SESSION_1_ON_1))
	{
		return;
	}

	// Avoid changing the default name if no participant present yet
	if (mChildren.size() == 0)
	{
		return;
	}

	uuid_vec_t temp_uuids; // uuids vector for building the added participants' names string
	if (conversation_type == CONV_SESSION_AD_HOC || conversation_type == CONV_SESSION_1_ON_1)
	{
		// Build a string containing the participants UUIDs (minus own agent) and check if ready for display (we don't want "(waiting)" in there)
		// Note: we don't bind ourselves to the LLAvatarNameCache event as updateParticipantName() is called by
		// onAvatarNameCache() which is itself attached to the same event.

		// In the case of a P2P conversation, we need to grab the name of the other participant in the session instance itself
		// as we do not create participants for such a session.

		LLFolderViewModelItem * itemp;
		BOOST_FOREACH(itemp, mChildren)
		{
			LLConversationItem* current_participant = dynamic_cast<LLConversationItem*>(itemp);
			// Add the avatar uuid to the list (except if it's the own agent uuid)
			if (current_participant->getUUID() != gAgentID)
			{
				LLAvatarName av_name;
				if (LLAvatarNameCache::get(current_participant->getUUID(), &av_name))
				{
					temp_uuids.push_back(current_participant->getUUID());

					if (conversation_type == CONV_SESSION_1_ON_1)
					{
						break;
					}
				}
			}
		}
	}

	if (temp_uuids.size() != 0)
	{
		std::string new_session_name;
		LLAvatarActions::buildResidentsString(temp_uuids, new_session_name);
		renameItem(new_session_name);
		postEvent("update_session", this, NULL);
	}
}

void LLConversationItemSession::removeParticipant(LLConversationItemParticipant* participant)
{
	removeChild(participant);
	mNeedsRefresh = true;
	updateName(participant);
	postEvent("remove_participant", this, participant);
}

void LLConversationItemSession::removeParticipant(const LLUUID& participant_id)
{
	LLConversationItemParticipant* participant = findParticipant(participant_id);
	if (participant)
	{
		removeParticipant(participant);
	}
}

void LLConversationItemSession::clearParticipants()
{
	clearChildren();
	mIsLoaded = false;
	mNeedsRefresh = true;
}

LLConversationItemParticipant* LLConversationItemSession::findParticipant(const LLUUID& participant_id)
{
	// This is *not* a general tree parsing algorithm. It assumes that a session contains only 
	// items (LLConversationItemParticipant) that have themselve no children.
	LLConversationItemParticipant* participant = NULL;
	child_list_t::iterator iter;
	for (iter = mChildren.begin(); iter != mChildren.end(); iter++)
	{
		participant = dynamic_cast<LLConversationItemParticipant*>(*iter);
		if (participant->hasSameValue(participant_id))
		{
			break;
		}
	}
	return (iter == mChildren.end() ? NULL : participant);
}

void LLConversationItemSession::setParticipantIsMuted(const LLUUID& participant_id, bool is_muted)
{
	LLConversationItemParticipant* participant = findParticipant(participant_id);
	if (participant)
	{
		participant->muteVoice(is_muted);
	}
}

void LLConversationItemSession::setParticipantIsModerator(const LLUUID& participant_id, bool is_moderator)
{
	LLConversationItemParticipant* participant = findParticipant(participant_id);
	if (participant)
	{
		participant->setIsModerator(is_moderator);
	}
}

void LLConversationItemSession::setTimeNow(const LLUUID& participant_id)
{
	mLastActiveTime = LLFrameTimer::getElapsedSeconds();
	mNeedsRefresh = true;
	LLConversationItemParticipant* participant = findParticipant(participant_id);
	if (participant)
	{
		participant->setTimeNow();
	}
}

void LLConversationItemSession::setDistance(const LLUUID& participant_id, F64 dist)
{
	LLConversationItemParticipant* participant = findParticipant(participant_id);
	if (participant)
	{
		participant->setDistance(dist);
		mNeedsRefresh = true;
	}
}

void LLConversationItemSession::buildContextMenu(LLMenuGL& menu, U32 flags)
{
    LL_DEBUGS() << "LLConversationItemParticipant::buildContextMenu()" << LL_ENDL;
    menuentry_vec_t items;
    menuentry_vec_t disabled_items;
    if((flags & ITEM_IN_MULTI_SELECTION) && (this->getType() != CONV_SESSION_NEARBY))
    {
    	items.push_back(std::string("close_selected_conversations"));
    }
    if(this->getType() == CONV_SESSION_1_ON_1)
    {
        items.push_back(std::string("close_conversation"));
        items.push_back(std::string("separator_disconnect_from_voice"));
        buildParticipantMenuOptions(items, flags);
    }
    else if(this->getType() == CONV_SESSION_GROUP)
    {
        items.push_back(std::string("close_conversation"));
        addVoiceOptions(items);
        items.push_back(std::string("chat_history"));
        items.push_back(std::string("separator_chat_history"));
        items.push_back(std::string("group_profile"));
        items.push_back(std::string("activate_group"));
        items.push_back(std::string("leave_group"));
    }
    else if(this->getType() == CONV_SESSION_AD_HOC)
    {
        items.push_back(std::string("close_conversation"));
        addVoiceOptions(items);
        items.push_back(std::string("chat_history"));
    }
    else if(this->getType() == CONV_SESSION_NEARBY)
    {
        items.push_back(std::string("chat_history"));
    }

    hide_context_entries(menu, items, disabled_items);
}

void LLConversationItemSession::addVoiceOptions(menuentry_vec_t& items)
{
    LLVoiceChannel* voice_channel = LLIMModel::getInstance() ? LLIMModel::getInstance()->getVoiceChannel(this->getUUID()) : NULL;

    if(voice_channel != LLVoiceChannel::getCurrentVoiceChannel())
    {
        items.push_back(std::string("open_voice_conversation"));
    }
    else
    {
        items.push_back(std::string("disconnect_from_voice"));
    }
}

// The time of activity of a session is the time of the most recent activity, session and participants included
const bool LLConversationItemSession::getTime(F64& time) const
{
	F64 most_recent_time = mLastActiveTime;
	bool has_time = (most_recent_time > 0.1);
	LLConversationItemParticipant* participant = NULL;
	child_list_t::const_iterator iter;
	for (iter = mChildren.begin(); iter != mChildren.end(); iter++)
	{
		participant = dynamic_cast<LLConversationItemParticipant*>(*iter);
		F64 participant_time;
		if (participant->getTime(participant_time))
		{
			has_time = true;
			most_recent_time = llmax(most_recent_time,participant_time);
		}
	}
	if (has_time)
	{
		time = most_recent_time;
	}
	return has_time;
}

void LLConversationItemSession::dumpDebugData(bool dump_children)
{
	// Session info
	LL_INFOS() << "Merov debug : session " << this << ", uuid = " << mUUID << ", name = " << mName << ", is loaded = " << mIsLoaded << LL_ENDL;
	// Children info
	if (dump_children)
	{
		for (child_list_t::iterator iter = mChildren.begin(); iter != mChildren.end(); iter++)
		{
			LLConversationItemParticipant* participant = dynamic_cast<LLConversationItemParticipant*>(*iter);
			if (participant)
			{
				participant->dumpDebugData();
			}
		}
	}
}

// should be invoked only for P2P sessions
void LLConversationItemSession::onAvatarNameCache(const LLAvatarName& av_name)
{
	if (mAvatarNameCacheConnection.connected())
	{
		mAvatarNameCacheConnection.disconnect();
	}

	renameItem(av_name.getDisplayName());
	postEvent("update_session", this, NULL);
}

//
// LLConversationItemParticipant
// 

LLConversationItemParticipant::LLConversationItemParticipant(std::string display_name, const LLUUID& uuid, LLFolderViewModelInterface& root_view_model) :
	LLConversationItem(display_name,uuid,root_view_model),
	mIsModerator(false),
	mDisplayModeratorLabel(false),
	mDistToAgent(-1.0)
{
	mDisplayName = display_name;
	mConvType = CONV_PARTICIPANT;
}

LLConversationItemParticipant::LLConversationItemParticipant(const LLUUID& uuid, LLFolderViewModelInterface& root_view_model) :
	LLConversationItem(uuid,root_view_model),
	mIsModerator(false),
	mDisplayModeratorLabel(false),
	mDistToAgent(-1.0)
{
	mConvType = CONV_PARTICIPANT;
}

void LLConversationItemParticipant::updateName()
{
	llassert(getUUID().notNull());
	if (getUUID().notNull())
	{
		LLAvatarName av_name;
		if (LLAvatarNameCache::get(getUUID(),&av_name))
		{
			updateName(av_name);
		}
	}
}

void LLConversationItemParticipant::onAvatarNameCache(const LLAvatarName& av_name)
{
	if (mAvatarNameCacheConnection.connected())
	{
		mAvatarNameCacheConnection.disconnect();
	}

	updateName(av_name);
}

void LLConversationItemParticipant::updateName(const LLAvatarName& av_name)
{
	mName = av_name.getUserName();
	mDisplayName = av_name.getDisplayName();
	
	if (mDisplayModeratorLabel)
	{
		mDisplayName += " " + LLTrans::getString("IM_moderator_label");
	}
	
	renameItem(mDisplayName);
	if (mParent != NULL)
	{
		LLConversationItemSession* parent_session = dynamic_cast<LLConversationItemSession*>(mParent);
		if (parent_session != NULL)
		{
			parent_session->requestSort();
			parent_session->updateName(this);
			postEvent("update_participant", parent_session, this);
		}
	}
}

void LLConversationItemParticipant::buildContextMenu(LLMenuGL& menu, U32 flags)
{
    menuentry_vec_t items;
    menuentry_vec_t disabled_items;
	
	buildParticipantMenuOptions(items, flags);
	
    hide_context_entries(menu, items, disabled_items);
}

LLConversationItemSession* LLConversationItemParticipant::getParentSession()
{
	LLConversationItemSession* parent_session = NULL;
	if (hasParent())
	{
		parent_session = dynamic_cast<LLConversationItemSession*>(mParent);
	}
	return parent_session;
}

void LLConversationItemParticipant::dumpDebugData()
{
	LL_INFOS() << "Merov debug : participant, uuid = " << mUUID << ", name = " << mName << ", display name = " << mDisplayName << ", muted = " << isVoiceMuted() << ", moderator = " << mIsModerator << LL_ENDL;
}

void LLConversationItemParticipant::setDisplayModeratorRole(bool displayRole)
{ 
	if (displayRole != mDisplayModeratorLabel)
	{
		mDisplayModeratorLabel = displayRole;
		updateName();
	}
}

bool LLConversationItemParticipant::isVoiceMuted()
{
	return LLMuteList::getInstance()->isMuted(mUUID, LLMute::flagVoiceChat);
}

void LLConversationItemParticipant::muteVoice(bool mute_voice)
{
	LLAvatarName av_name;
	LLAvatarNameCache::get(mUUID, &av_name);
	LLMuteList * mute_listp = LLMuteList::getInstance();
	bool voice_already_muted = mute_listp->isMuted(mUUID, av_name.getUserName(), LLMute::flagVoiceChat);

	LLMute mute(mUUID, av_name.getUserName(), LLMute::AGENT);
	if (voice_already_muted && !mute_voice)
	{
		mute_listp->remove(mute, LLMute::flagVoiceChat);
	}
	else if (!voice_already_muted && mute_voice)
	{
		mute_listp->add(mute, LLMute::flagVoiceChat);
	}
}

//
// LLConversationSort
// 

// Comparison operator: returns "true" is a comes before b, "false" otherwise
bool LLConversationSort::operator()(const LLConversationItem* const& a, const LLConversationItem* const& b) const
{
	LLConversationItem::EConversationType type_a = a->getType();
	LLConversationItem::EConversationType type_b = b->getType();

	if ((type_a == LLConversationItem::CONV_PARTICIPANT) && (type_b == LLConversationItem::CONV_PARTICIPANT))
	{
		// If both items are participants
		U32 sort_order = getSortOrderParticipants();
		if (sort_order == LLConversationFilter::SO_DATE)
		{
			F64 time_a = 0.0;
			F64 time_b = 0.0;
			bool has_time_a = a->getTime(time_a);
			bool has_time_b = b->getTime(time_b);
			if (has_time_a && has_time_b)
			{
				// Most recent comes first
				return (time_a > time_b);
			}
			else if (has_time_a || has_time_b)
			{
				// If we have only one time available, the element with time must come first
				return has_time_a;
			}
			// If no time available, we'll default to sort by name at the end of this method
		}
		else if (sort_order == LLConversationFilter::SO_DISTANCE)
		{
			F64 dist_a = 0.0;
			F64 dist_b = 0.0;
			bool has_dist_a = a->getDistanceToAgent(dist_a);
			bool has_dist_b = b->getDistanceToAgent(dist_b);
			if (has_dist_a && has_dist_b)
			{
				// Closest comes first
				return (dist_a < dist_b);
			}
			else if (has_dist_a || has_dist_b)
			{
				// If we have only one distance available, the element with it must come first
				return has_dist_a;
			}
			// If no distance available, we'll default to sort by name at the end of this method
		}
	}
	else if ((type_a > LLConversationItem::CONV_PARTICIPANT) && (type_b > LLConversationItem::CONV_PARTICIPANT))
	{
		// If both are sessions
		U32 sort_order = getSortOrderSessions();

		if (sort_order == LLConversationFilter::SO_DATE)
		{
			// Sort by time
			F64 time_a = 0.0;
			F64 time_b = 0.0;
			bool has_time_a = a->getTime(time_a);
			bool has_time_b = b->getTime(time_b);
			if (has_time_a && has_time_b)
			{
				// Most recent comes first
				return (time_a > time_b);
			}
			else if (has_time_a || has_time_b)
			{
				// If we have only one time available, the element with time must come first
				return has_time_a;
			}
			// If no time available, we'll default to sort by name at the end of this method
		}
		else
		{
			if ((type_a == LLConversationItem::CONV_SESSION_NEARBY) || (type_b == LLConversationItem::CONV_SESSION_NEARBY))
			{
				// If one is the nearby session, put nearby session *always* last
				return (type_b == LLConversationItem::CONV_SESSION_NEARBY);
			}
			else if (sort_order == LLConversationFilter::SO_SESSION_TYPE)
			{
				if (type_a != type_b)
				{
					// Lowest types come first. See LLConversationItem definition of types
					return (type_a < type_b);
				}
			// If types are identical, we'll default to sort by name at the end of this method
			}
		}
	}
	else
	{
		// If one item is a participant and the other a session, the session comes before the participant
		// so we simply compare the type
		// Notes: as a consequence, CONV_UNKNOWN (which should never get created...) always come first
		return (type_a > type_b);
	}
	// By default, in all other possible cases (including sort order type LLConversationFilter::SO_NAME of course), 
	// we sort by name
	S32 compare = LLStringUtil::compareDict(a->getName(), b->getName());
	return (compare < 0);
}

//
// LLConversationViewModel
//

void LLConversationViewModel::sort(LLFolderViewFolder* folder) 
{
	base_t::sort(folder);
}

// EOF
