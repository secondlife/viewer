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

#include "llconversationmodel.h"

//
// Conversation items : common behaviors
//

LLConversationItem::LLConversationItem(std::string display_name, const LLUUID& uuid, LLFolderViewModelInterface& root_view_model) :
	LLFolderViewModelItemCommon(root_view_model),
	mName(display_name),
	mUUID(uuid),
	mNeedsRefresh(true),
	mConvType(CONV_UNKNOWN),
	mLastActiveTime(0.0)
{
}

LLConversationItem::LLConversationItem(const LLUUID& uuid, LLFolderViewModelInterface& root_view_model) :
	LLFolderViewModelItemCommon(root_view_model),
	mName(""),
	mUUID(uuid),
	mNeedsRefresh(true),
	mConvType(CONV_UNKNOWN),
	mLastActiveTime(0.0)
{
}

LLConversationItem::LLConversationItem(LLFolderViewModelInterface& root_view_model) :
	LLFolderViewModelItemCommon(root_view_model),
	mName(""),
	mUUID(),
	mNeedsRefresh(true),
	mConvType(CONV_UNKNOWN),
	mLastActiveTime(0.0)
{
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
}

void LLConversationItemSession::removeParticipant(LLConversationItemParticipant* participant)
{
	removeChild(participant);
	mNeedsRefresh = true;
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
		participant->setIsMuted(is_muted);
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

void LLConversationItemSession::dumpDebugData()
{
	llinfos << "Merov debug : session " << this << ", uuid = " << mUUID << ", name = " << mName << ", is loaded = " << mIsLoaded << llendl;
	LLConversationItemParticipant* participant = NULL;
	child_list_t::iterator iter;
	for (iter = mChildren.begin(); iter != mChildren.end(); iter++)
	{
		participant = dynamic_cast<LLConversationItemParticipant*>(*iter);
		participant->dumpDebugData();
	}
}

//
// LLConversationItemParticipant
// 

LLConversationItemParticipant::LLConversationItemParticipant(std::string display_name, const LLUUID& uuid, LLFolderViewModelInterface& root_view_model) :
	LLConversationItem(display_name,uuid,root_view_model),
	mIsMuted(false),
	mIsModerator(false),
	mDistToAgent(-1.0)
{
	mConvType = CONV_PARTICIPANT;
}

LLConversationItemParticipant::LLConversationItemParticipant(const LLUUID& uuid, LLFolderViewModelInterface& root_view_model) :
	LLConversationItem(uuid,root_view_model),
	mIsMuted(false),
	mIsModerator(false),
	mDistToAgent(-1.0)
{
	mConvType = CONV_PARTICIPANT;
}

void LLConversationItemParticipant::onAvatarNameCache(const LLAvatarName& av_name)
{
	mName = (av_name.mUsername.empty() ? av_name.mDisplayName : av_name.mUsername);
	mDisplayName = (av_name.mDisplayName.empty() ? av_name.mUsername : av_name.mDisplayName);
	mNeedsRefresh = true;
	if (mParent)
	{
		mParent->requestSort();
	}
}

void LLConversationItemParticipant::dumpDebugData()
{
	llinfos << "Merov debug : participant, uuid = " << mUUID << ", name = " << mName << ", display name = " << mDisplayName << ", muted = " << mIsMuted << ", moderator = " << mIsModerator << llendl;
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
		if ((type_a == LLConversationItem::CONV_SESSION_NEARBY) || (type_b == LLConversationItem::CONV_SESSION_NEARBY))
		{
			// If one is the nearby session, put nearby session *always* first
			return (type_a == LLConversationItem::CONV_SESSION_NEARBY);
		}
		else if (sort_order == LLConversationFilter::SO_DATE)
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
