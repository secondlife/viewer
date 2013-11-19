/** 
 * @file llavatarpropertiesprocessor.cpp
 * @brief LLAvatarPropertiesProcessor class implementation
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#include "llavatarpropertiesprocessor.h"

// Viewer includes
#include "llagent.h"
#include "llagentpicksinfo.h"
#include "lldateutil.h"
#include "llviewergenericmessage.h"
#include "llstartup.h"

// Linden library includes
#include "llavatarconstants.h"	// AVATAR_TRANSACTED, etc.
#include "lldate.h"
#include "lltrans.h"
#include "llui.h"				// LLUI::getLanguage()
#include "message.h"

LLAvatarPropertiesProcessor::LLAvatarPropertiesProcessor()
{
}

LLAvatarPropertiesProcessor::~LLAvatarPropertiesProcessor()
{
}

void LLAvatarPropertiesProcessor::addObserver(const LLUUID& avatar_id, LLAvatarPropertiesObserver* observer)
{
	// Check if that observer is already in mObservers for that avatar_id
	observer_multimap_t::iterator it;

	// IAN BUG this should update the observer's UUID if this is a dupe - sent to PE
	it = mObservers.find(avatar_id);
	while (it != mObservers.end())
	{
		if (it->second == observer)
		{
			return;
		}
		else
		{
			++it;
		}
	}

	mObservers.insert(std::pair<LLUUID, LLAvatarPropertiesObserver*>(avatar_id, observer));
}

void LLAvatarPropertiesProcessor::removeObserver(const LLUUID& avatar_id, LLAvatarPropertiesObserver* observer)
{
	if (!observer)
	{
		return;
	}

	observer_multimap_t::iterator it;
	it = mObservers.find(avatar_id);
	while (it != mObservers.end())
	{
		if (it->second == observer)
		{
			mObservers.erase(it);
			break;
		}
		else
		{
			++it;
		}
	}
}


void LLAvatarPropertiesProcessor::sendGenericRequest(const LLUUID& avatar_id, EAvatarProcessorType type, const std::string method)
{
	// Suppress duplicate requests while waiting for a response from the network
	if (isPendingRequest(avatar_id, type))
	{
		// waiting for a response, don't re-request
		return;
	}
	// indicate we're going to make a request
	addPendingRequest(avatar_id, type);

	std::vector<std::string> strings;
	strings.push_back( avatar_id.asString() );
	send_generic_message(method, strings);
}

void LLAvatarPropertiesProcessor::sendAvatarPropertiesRequest(const LLUUID& avatar_id)
{
	// this is the startup state when send_complete_agent_movement() message is sent.
	// Before this, the AvatarPropertiesRequest message  
	// won't work so don't bother trying
	if (LLStartUp::getStartupState() <= STATE_AGENT_SEND)
	{
		return;
	}

	if (isPendingRequest(avatar_id, APT_PROPERTIES))
	{
		// waiting for a response, don't re-request
		return;
	}
	// indicate we're going to make a request
	addPendingRequest(avatar_id, APT_PROPERTIES);

	LLMessageSystem *msg = gMessageSystem;

	msg->newMessageFast(_PREHASH_AvatarPropertiesRequest);
	msg->nextBlockFast( _PREHASH_AgentData);
	msg->addUUIDFast(   _PREHASH_AgentID, gAgent.getID() );
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->addUUIDFast(   _PREHASH_AvatarID, avatar_id);
	gAgent.sendReliableMessage();
}

void LLAvatarPropertiesProcessor::sendAvatarPicksRequest(const LLUUID& avatar_id)
{
	sendGenericRequest(avatar_id, APT_PICKS, "avatarpicksrequest");
}

void LLAvatarPropertiesProcessor::sendAvatarNotesRequest(const LLUUID& avatar_id)
{
	sendGenericRequest(avatar_id, APT_NOTES, "avatarnotesrequest");
}

void LLAvatarPropertiesProcessor::sendAvatarGroupsRequest(const LLUUID& avatar_id)
{
	sendGenericRequest(avatar_id, APT_GROUPS, "avatargroupsrequest");
}

void LLAvatarPropertiesProcessor::sendAvatarTexturesRequest(const LLUUID& avatar_id)
{
	sendGenericRequest(avatar_id, APT_TEXTURES, "avatartexturesrequest");
	// No response expected.
	removePendingRequest(avatar_id, APT_TEXTURES);
}

void LLAvatarPropertiesProcessor::sendAvatarClassifiedsRequest(const LLUUID& avatar_id)
{
	sendGenericRequest(avatar_id, APT_CLASSIFIEDS, "avatarclassifiedsrequest");
}

void LLAvatarPropertiesProcessor::sendAvatarPropertiesUpdate(const LLAvatarData* avatar_props)
{
	if (!gAgent.isInitialized() || (gAgent.getID() == LLUUID::null))
	{
		llwarns << "Sending avatarinfo update DENIED - invalid agent" << llendl;
		return;
	}

	llinfos << "Sending avatarinfo update" << llendl;

	// This value is required by sendAvatarPropertiesUpdate method.
	//A profile should never be mature. (From the original code)
	BOOL mature = FALSE;

	LLMessageSystem *msg = gMessageSystem;

	msg->newMessageFast	(_PREHASH_AvatarPropertiesUpdate);
	msg->nextBlockFast	(_PREHASH_AgentData);
	msg->addUUIDFast		(_PREHASH_AgentID,		gAgent.getID() );
	msg->addUUIDFast		(_PREHASH_SessionID,	gAgent.getSessionID() );
	msg->nextBlockFast	(_PREHASH_PropertiesData);

	msg->addUUIDFast		(_PREHASH_ImageID,		avatar_props->image_id);
	msg->addUUIDFast		(_PREHASH_FLImageID,	avatar_props->fl_image_id);
	msg->addStringFast	(_PREHASH_AboutText,	avatar_props->about_text);
	msg->addStringFast	(_PREHASH_FLAboutText,	avatar_props->fl_about_text);

	msg->addBOOL(_PREHASH_AllowPublish, avatar_props->allow_publish);
	msg->addBOOL(_PREHASH_MaturePublish, mature);
	msg->addString(_PREHASH_ProfileURL, avatar_props->profile_url);

	gAgent.sendReliableMessage();
}



//static
std::string LLAvatarPropertiesProcessor::accountType(const LLAvatarData* avatar_data)
{
	// If you have a special account, like M Linden ("El Jefe!")
	// return an untranslated "special" string
	if (!avatar_data->caption_text.empty())
	{
		return avatar_data->caption_text;
	}
	const char* const ACCT_TYPE[] = {
		"AcctTypeResident",
		"AcctTypeTrial",
		"AcctTypeCharterMember",
		"AcctTypeEmployee"
	};
	U8 caption_max = (U8)LL_ARRAY_SIZE(ACCT_TYPE)-1;
	U8 caption_index = llclamp(avatar_data->caption_index, (U8)0, caption_max);
	return LLTrans::getString(ACCT_TYPE[caption_index]);
}

//static
std::string LLAvatarPropertiesProcessor::paymentInfo(const LLAvatarData* avatar_data)
{
	// Special accounts like M Linden don't have payment info revealed.
	if (!avatar_data->caption_text.empty()) return "";

	// Linden employees don't have payment info revealed
	const S32 LINDEN_EMPLOYEE_INDEX = 3;
	if (avatar_data->caption_index == LINDEN_EMPLOYEE_INDEX) return "";

	BOOL transacted = (avatar_data->flags & AVATAR_TRANSACTED);
	BOOL identified = (avatar_data->flags & AVATAR_IDENTIFIED);
	// Not currently getting set in dataserver/lldataavatar.cpp for privacy considerations
	//BOOL age_verified = (avatar_data->flags & AVATAR_AGEVERIFIED); 

	const char* payment_text;
	if(transacted)
	{
		payment_text = "PaymentInfoUsed";
	}
	else if (identified)
	{
		payment_text = "PaymentInfoOnFile";
	}
	else
	{
		payment_text = "NoPaymentInfoOnFile";
	}
	return LLTrans::getString(payment_text);
}

void LLAvatarPropertiesProcessor::processAvatarPropertiesReply(LLMessageSystem* msg, void**)
{
	LLAvatarData avatar_data;
	std::string birth_date;

	msg->getUUIDFast(	_PREHASH_AgentData,			_PREHASH_AgentID, 		avatar_data.agent_id);
	msg->getUUIDFast(	_PREHASH_AgentData,			_PREHASH_AvatarID, 		avatar_data.avatar_id);
	msg->getUUIDFast(  	_PREHASH_PropertiesData,	_PREHASH_ImageID,		avatar_data.image_id);
	msg->getUUIDFast(  	_PREHASH_PropertiesData,	_PREHASH_FLImageID,		avatar_data.fl_image_id);
	msg->getUUIDFast(	_PREHASH_PropertiesData,	_PREHASH_PartnerID,		avatar_data.partner_id);
	msg->getStringFast(	_PREHASH_PropertiesData,	_PREHASH_AboutText,		avatar_data.about_text);
	msg->getStringFast(	_PREHASH_PropertiesData,	_PREHASH_FLAboutText,	avatar_data.fl_about_text);
	msg->getStringFast(	_PREHASH_PropertiesData,	_PREHASH_BornOn,		birth_date);
	msg->getString(		_PREHASH_PropertiesData,	_PREHASH_ProfileURL,	avatar_data.profile_url);
	msg->getU32Fast(	_PREHASH_PropertiesData,	_PREHASH_Flags,			avatar_data.flags);


	LLDateUtil::dateFromPDTString(avatar_data.born_on, birth_date);
	avatar_data.caption_index = 0;

	S32 charter_member_size = 0;
	charter_member_size = msg->getSize(_PREHASH_PropertiesData, _PREHASH_CharterMember);
	if(1 == charter_member_size)
	{
		msg->getBinaryData(_PREHASH_PropertiesData, _PREHASH_CharterMember, &avatar_data.caption_index, 1);
	}
	else if(1 < charter_member_size)
	{
		msg->getString(_PREHASH_PropertiesData, _PREHASH_CharterMember, avatar_data.caption_text);
	}
	LLAvatarPropertiesProcessor* self = getInstance();
	// Request processed, no longer pending
	self->removePendingRequest(avatar_data.avatar_id, APT_PROPERTIES);
	self->notifyObservers(avatar_data.avatar_id,&avatar_data,APT_PROPERTIES);
}

void LLAvatarPropertiesProcessor::processAvatarInterestsReply(LLMessageSystem* msg, void**)
{
/*
	AvatarInterestsReply is automatically sent by the server in response to the 
	AvatarPropertiesRequest sent when the panel is opened (in addition to the AvatarPropertiesReply message). 
	If the interests panel is no longer part of the design (?) we should just register the message 
	to a handler function that does nothing. 
	That will suppress the warnings and be compatible with old server versions.
	WARNING: LLTemplateMessageReader::decodeData: Message from 216.82.37.237:13000 with no handler function received: AvatarInterestsReply
*/
}

void LLAvatarPropertiesProcessor::processAvatarClassifiedsReply(LLMessageSystem* msg, void**)
{
	LLAvatarClassifieds classifieds;

	msg->getUUID(_PREHASH_AgentData, _PREHASH_AgentID, classifieds.agent_id);
	msg->getUUID(_PREHASH_AgentData, _PREHASH_TargetID, classifieds.target_id);

	S32 block_count = msg->getNumberOfBlocks(_PREHASH_Data);

	for(int n = 0; n < block_count; ++n)
	{
		LLAvatarClassifieds::classified_data data;

		msg->getUUID(_PREHASH_Data, _PREHASH_ClassifiedID, data.classified_id, n);
		msg->getString(_PREHASH_Data, _PREHASH_Name, data.name, n);

		classifieds.classifieds_list.push_back(data);
	}

	LLAvatarPropertiesProcessor* self = getInstance();
	// Request processed, no longer pending
	self->removePendingRequest(classifieds.target_id, APT_CLASSIFIEDS);
	self->notifyObservers(classifieds.target_id,&classifieds,APT_CLASSIFIEDS);
}

void LLAvatarPropertiesProcessor::processClassifiedInfoReply(LLMessageSystem* msg, void**)
{
	LLAvatarClassifiedInfo c_info;

	msg->getUUID(_PREHASH_AgentData, _PREHASH_AgentID, c_info.agent_id);

	msg->getUUID(_PREHASH_Data, _PREHASH_ClassifiedID, c_info.classified_id);
	msg->getUUID(_PREHASH_Data, _PREHASH_CreatorID, c_info.creator_id);
	msg->getU32(_PREHASH_Data, _PREHASH_CreationDate, c_info.creation_date);
	msg->getU32(_PREHASH_Data, _PREHASH_ExpirationDate, c_info.expiration_date);
	msg->getU32(_PREHASH_Data, _PREHASH_Category, c_info.category);
	msg->getString(_PREHASH_Data, _PREHASH_Name, c_info.name);
	msg->getString(_PREHASH_Data, _PREHASH_Desc, c_info.description);
	msg->getUUID(_PREHASH_Data, _PREHASH_ParcelID, c_info.parcel_id);
	msg->getU32(_PREHASH_Data, _PREHASH_ParentEstate, c_info.parent_estate);
	msg->getUUID(_PREHASH_Data, _PREHASH_SnapshotID, c_info.snapshot_id);
	msg->getString(_PREHASH_Data, _PREHASH_SimName, c_info.sim_name);
	msg->getVector3d(_PREHASH_Data, _PREHASH_PosGlobal, c_info.pos_global);
	msg->getString(_PREHASH_Data, _PREHASH_ParcelName, c_info.parcel_name);
	msg->getU8(_PREHASH_Data, _PREHASH_ClassifiedFlags, c_info.flags);
	msg->getS32(_PREHASH_Data, _PREHASH_PriceForListing, c_info.price_for_listing);

	LLAvatarPropertiesProcessor* self = getInstance();
	// Request processed, no longer pending
	self->removePendingRequest(c_info.creator_id, APT_CLASSIFIED_INFO);
	self->notifyObservers(c_info.creator_id, &c_info, APT_CLASSIFIED_INFO);
}


void LLAvatarPropertiesProcessor::processAvatarNotesReply(LLMessageSystem* msg, void**)
{
	LLAvatarNotes avatar_notes;

	msg->getUUID(_PREHASH_AgentData, _PREHASH_AgentID, avatar_notes.agent_id);
	msg->getUUID(_PREHASH_Data, _PREHASH_TargetID, avatar_notes.target_id);
	msg->getString(_PREHASH_Data, _PREHASH_Notes, avatar_notes.notes);

	LLAvatarPropertiesProcessor* self = getInstance();
	// Request processed, no longer pending
	self->removePendingRequest(avatar_notes.target_id, APT_NOTES);
	self->notifyObservers(avatar_notes.target_id,&avatar_notes,APT_NOTES);
}

void LLAvatarPropertiesProcessor::processAvatarPicksReply(LLMessageSystem* msg, void**)
{
	LLAvatarPicks avatar_picks;
	msg->getUUID(_PREHASH_AgentData, _PREHASH_AgentID, avatar_picks.target_id);
	msg->getUUID(_PREHASH_AgentData, _PREHASH_TargetID, avatar_picks.target_id);

	S32 block_count = msg->getNumberOfBlocks(_PREHASH_Data);
	for (int block = 0; block < block_count; ++block)
	{
		LLUUID pick_id;
		std::string pick_name;

		msg->getUUID(_PREHASH_Data, _PREHASH_PickID, pick_id, block);
		msg->getString(_PREHASH_Data, _PREHASH_PickName, pick_name, block);

		avatar_picks.picks_list.push_back(std::make_pair(pick_id,pick_name));
	}
	LLAvatarPropertiesProcessor* self = getInstance();
	// Request processed, no longer pending
	self->removePendingRequest(avatar_picks.target_id, APT_PICKS);
	self->notifyObservers(avatar_picks.target_id,&avatar_picks,APT_PICKS);
}

void LLAvatarPropertiesProcessor::processPickInfoReply(LLMessageSystem* msg, void**)
{
	LLPickData pick_data;

	// Extract the agent id and verify the message is for this
	// client.
	msg->getUUID(_PREHASH_AgentData, _PREHASH_AgentID, pick_data.agent_id );
	msg->getUUID(_PREHASH_Data, _PREHASH_PickID, pick_data.pick_id);
	msg->getUUID(_PREHASH_Data, _PREHASH_CreatorID, pick_data.creator_id);

	// ** top_pick should be deleted, not being used anymore - angela
	msg->getBOOL(_PREHASH_Data, _PREHASH_TopPick, pick_data.top_pick);
	msg->getUUID(_PREHASH_Data, _PREHASH_ParcelID, pick_data.parcel_id);
	msg->getString(_PREHASH_Data, _PREHASH_Name, pick_data.name);
	msg->getString(_PREHASH_Data, _PREHASH_Desc, pick_data.desc);
	msg->getUUID(_PREHASH_Data, _PREHASH_SnapshotID, pick_data.snapshot_id);

	msg->getString(_PREHASH_Data, _PREHASH_User, pick_data.user_name);
	msg->getString(_PREHASH_Data, _PREHASH_OriginalName, pick_data.original_name);
	msg->getString(_PREHASH_Data, _PREHASH_SimName, pick_data.sim_name);
	msg->getVector3d(_PREHASH_Data, _PREHASH_PosGlobal, pick_data.pos_global);

	msg->getS32(_PREHASH_Data, _PREHASH_SortOrder, pick_data.sort_order);
	msg->getBOOL(_PREHASH_Data, _PREHASH_Enabled, pick_data.enabled);

	LLAvatarPropertiesProcessor* self = getInstance();
	// don't need to remove pending request as we don't track pick info
	self->notifyObservers(pick_data.creator_id, &pick_data, APT_PICK_INFO);
}

void LLAvatarPropertiesProcessor::processAvatarGroupsReply(LLMessageSystem* msg, void**)
{
	LLAvatarGroups avatar_groups;
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_AgentID, avatar_groups.agent_id );
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_AvatarID, avatar_groups.avatar_id );

	S32 group_count = msg->getNumberOfBlocksFast(_PREHASH_GroupData);
	for(S32 i = 0; i < group_count; ++i)
	{
		LLAvatarGroups::LLGroupData group_data;

		msg->getU64(    _PREHASH_GroupData, _PREHASH_GroupPowers,	group_data.group_powers, i );
		msg->getStringFast(_PREHASH_GroupData, _PREHASH_GroupTitle,	group_data.group_title, i );
		msg->getUUIDFast(  _PREHASH_GroupData, _PREHASH_GroupID,	group_data.group_id, i);
		msg->getStringFast(_PREHASH_GroupData, _PREHASH_GroupName,	group_data.group_name, i );
		msg->getUUIDFast(  _PREHASH_GroupData, _PREHASH_GroupInsigniaID, group_data.group_insignia_id, i );

		avatar_groups.group_list.push_back(group_data);
	}

	LLAvatarPropertiesProcessor* self = getInstance();
	self->removePendingRequest(avatar_groups.avatar_id, APT_GROUPS);
	self->notifyObservers(avatar_groups.avatar_id,&avatar_groups,APT_GROUPS);
}

void LLAvatarPropertiesProcessor::notifyObservers(const LLUUID& id,void* data, EAvatarProcessorType type)
{
	// Copy the map (because observers may delete themselves when updated?)
	LLAvatarPropertiesProcessor::observer_multimap_t observers = mObservers;

	observer_multimap_t::iterator oi = observers.begin();
	observer_multimap_t::iterator end = observers.end();
	for (; oi != end; ++oi)
	{
		// only notify observers for the same agent, or if the observer
		// didn't know the agent ID and passed a NULL id.
		const LLUUID &agent_id = oi->first;
		if (agent_id == id || agent_id.isNull())
		{
			oi->second->processProperties(data,type);
		}
	}
}

void LLAvatarPropertiesProcessor::sendFriendRights(const LLUUID& avatar_id, S32 rights)
{
	if(!avatar_id.isNull())
	{
		LLMessageSystem* msg = gMessageSystem;

		// setup message header
		msg->newMessageFast(_PREHASH_GrantUserRights);
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUID(_PREHASH_AgentID, gAgent.getID());
		msg->addUUID(_PREHASH_SessionID, gAgent.getSessionID());

		msg->nextBlockFast(_PREHASH_Rights);
		msg->addUUID(_PREHASH_AgentRelated, avatar_id);
		msg->addS32(_PREHASH_RelatedRights, rights);

		gAgent.sendReliableMessage();
	}
}

void LLAvatarPropertiesProcessor::sendNotes(const LLUUID& avatar_id, const std::string notes)
{
	if(!avatar_id.isNull())
	{
		LLMessageSystem* msg = gMessageSystem;

		// setup message header
		msg->newMessageFast(_PREHASH_AvatarNotesUpdate);
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUID(_PREHASH_AgentID, gAgent.getID());
		msg->addUUID(_PREHASH_SessionID, gAgent.getSessionID());

		msg->nextBlockFast(_PREHASH_Data);
		msg->addUUID(_PREHASH_TargetID, avatar_id);
		msg->addString(_PREHASH_Notes, notes);

		gAgent.sendReliableMessage();
	}
}


void LLAvatarPropertiesProcessor::sendPickDelete( const LLUUID& pick_id )
{
	LLMessageSystem* msg = gMessageSystem; 
	msg->newMessage(_PREHASH_PickDelete);
	msg->nextBlock(_PREHASH_AgentData);
	msg->addUUID(_PREHASH_AgentID, gAgent.getID());
	msg->addUUID(_PREHASH_SessionID, gAgent.getSessionID());
	msg->nextBlock(_PREHASH_Data);
	msg->addUUID(_PREHASH_PickID, pick_id);
	gAgent.sendReliableMessage();

	LLAgentPicksInfo::getInstance()->requestNumberOfPicks();
	LLAgentPicksInfo::getInstance()->decrementNumberOfPicks();
}

void LLAvatarPropertiesProcessor::sendClassifiedDelete(const LLUUID& classified_id)
{
	LLMessageSystem* msg = gMessageSystem; 

	msg->newMessage(_PREHASH_ClassifiedDelete);

	msg->nextBlock(_PREHASH_AgentData);
	msg->addUUID(_PREHASH_AgentID, gAgent.getID());
	msg->addUUID(_PREHASH_SessionID, gAgent.getSessionID());

	msg->nextBlock(_PREHASH_Data);
	msg->addUUID(_PREHASH_ClassifiedID, classified_id);

	gAgent.sendReliableMessage();
}

void LLAvatarPropertiesProcessor::sendPickInfoUpdate(const LLPickData* new_pick)
{
	if (!new_pick) return;

	LLMessageSystem* msg = gMessageSystem;

	msg->newMessage(_PREHASH_PickInfoUpdate);
	msg->nextBlock(_PREHASH_AgentData);
	msg->addUUID(_PREHASH_AgentID, gAgent.getID());
	msg->addUUID(_PREHASH_SessionID, gAgent.getSessionID());

	msg->nextBlock(_PREHASH_Data);
	msg->addUUID(_PREHASH_PickID, new_pick->pick_id);
	msg->addUUID(_PREHASH_CreatorID, new_pick->creator_id);

	//legacy var need to be deleted
	msg->addBOOL(_PREHASH_TopPick, FALSE);	

	// fills in on simulator if null
	msg->addUUID(_PREHASH_ParcelID, new_pick->parcel_id);
	msg->addString(_PREHASH_Name, new_pick->name);
	msg->addString(_PREHASH_Desc, new_pick->desc);
	msg->addUUID(_PREHASH_SnapshotID, new_pick->snapshot_id);
	msg->addVector3d(_PREHASH_PosGlobal, new_pick->pos_global);

	// Only top picks have a sort order
	msg->addS32(_PREHASH_SortOrder, 0);

	msg->addBOOL(_PREHASH_Enabled, new_pick->enabled);
	gAgent.sendReliableMessage();

	LLAgentPicksInfo::getInstance()->requestNumberOfPicks();
}

void LLAvatarPropertiesProcessor::sendClassifiedInfoUpdate(const LLAvatarClassifiedInfo* c_data)
{
	if(!c_data)
	{
		return;
	}

	LLMessageSystem* msg = gMessageSystem;

	msg->newMessage(_PREHASH_ClassifiedInfoUpdate);

	msg->nextBlock(_PREHASH_AgentData);
	msg->addUUID(_PREHASH_AgentID, gAgent.getID());
	msg->addUUID(_PREHASH_SessionID, gAgent.getSessionID());

	msg->nextBlock(_PREHASH_Data);
	msg->addUUID(_PREHASH_ClassifiedID, c_data->classified_id);
	msg->addU32(_PREHASH_Category, c_data->category);
	msg->addString(_PREHASH_Name, c_data->name);
	msg->addString(_PREHASH_Desc, c_data->description);
	msg->addUUID(_PREHASH_ParcelID, c_data->parcel_id);
	msg->addU32(_PREHASH_ParentEstate, 0);
	msg->addUUID(_PREHASH_SnapshotID, c_data->snapshot_id);
	msg->addVector3d(_PREHASH_PosGlobal, c_data->pos_global);
	msg->addU8(_PREHASH_ClassifiedFlags, c_data->flags);
	msg->addS32(_PREHASH_PriceForListing, c_data->price_for_listing);

	gAgent.sendReliableMessage();
}

void LLAvatarPropertiesProcessor::sendPickInfoRequest(const LLUUID& creator_id, const LLUUID& pick_id)
{
	// Must ask for a pick based on the creator id because
	// the pick database is distributed to the inventory cluster. JC
	std::vector<std::string> request_params;
	request_params.push_back(creator_id.asString() );
	request_params.push_back(pick_id.asString() );
	send_generic_message("pickinforequest", request_params);
}

void LLAvatarPropertiesProcessor::sendClassifiedInfoRequest(const LLUUID& classified_id)
{
	LLMessageSystem* msg = gMessageSystem;

	msg->newMessage(_PREHASH_ClassifiedInfoRequest);
	msg->nextBlock(_PREHASH_AgentData);
	
	msg->addUUID(_PREHASH_AgentID, gAgent.getID());
	msg->addUUID(_PREHASH_SessionID, gAgent.getSessionID());

	msg->nextBlock(_PREHASH_Data);
	msg->addUUID(_PREHASH_ClassifiedID, classified_id);

	gAgent.sendReliableMessage();
}

bool LLAvatarPropertiesProcessor::isPendingRequest(const LLUUID& avatar_id, EAvatarProcessorType type)
{
	timestamp_map_t::key_type key = std::make_pair(avatar_id, type);
	timestamp_map_t::iterator it = mRequestTimestamps.find(key);

	// Is this a new request?
	if (it == mRequestTimestamps.end()) return false;

	// We found a request, check if it has timed out
	U32 now = time(NULL);
	const U32 REQUEST_EXPIRE_SECS = 5;
	U32 expires = it->second + REQUEST_EXPIRE_SECS;

	// Request is still pending if it hasn't expired yet
	// *NOTE: Expired requests will accumulate in this map, but they are rare,
	// the data is small, and they will be updated if the same data is
	// re-requested
	return (now < expires);
}

void LLAvatarPropertiesProcessor::addPendingRequest(const LLUUID& avatar_id, EAvatarProcessorType type)
{
	timestamp_map_t::key_type key = std::make_pair(avatar_id, type);
	U32 now = time(NULL);
	// Add or update existing (expired) request
	mRequestTimestamps[ key ] = now;
}

void LLAvatarPropertiesProcessor::removePendingRequest(const LLUUID& avatar_id, EAvatarProcessorType type)
{
	timestamp_map_t::key_type key = std::make_pair(avatar_id, type);
	mRequestTimestamps.erase(key);
}
