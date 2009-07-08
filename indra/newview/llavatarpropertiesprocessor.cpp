/** 
 * @file llavatarpropertiesprocessor.cpp
 * @brief LLAvatarPropertiesProcessor class implementation
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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

#include "llavatarpropertiesprocessor.h"

#include "message.h"
#include "llagent.h"
#include "llviewergenericmessage.h"

LLAvatarPropertiesProcessor::LLAvatarPropertiesProcessor()
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

void LLAvatarPropertiesProcessor::sendDataRequest(const LLUUID& avatar_id, EAvatarProcessorType type, 
	const void * data)
{
	switch(type)
	{
	case APT_PROPERTIES:
		sendAvatarPropertiesRequest(avatar_id);
		break;
	case APT_PICKS:
		sendGenericRequest(avatar_id, "avatarpicksrequest");
		break;
	case APT_PICK_INFO:
		if (data) {
			sendPickInfoRequest(avatar_id, *static_cast<const LLUUID*>(data));
		}
		break;
	case APT_NOTES:
		sendGenericRequest(avatar_id, "avatarnotesrequest");
		break;
	case APT_GROUPS:
		sendGenericRequest(avatar_id, "avatargroupsrequest");
		break;
	default:
		break;
	}
}

void LLAvatarPropertiesProcessor::sendGenericRequest(const LLUUID& avatar_id, const std::string method)
{
	std::vector<std::string> strings;
	strings.push_back( avatar_id.asString() );
	send_generic_message(method, strings);
}

void LLAvatarPropertiesProcessor::sendAvatarPropertiesRequest(const LLUUID& avatar_id)
{
	LLMessageSystem *msg = gMessageSystem;

	msg->newMessageFast(_PREHASH_AvatarPropertiesRequest);
	msg->nextBlockFast( _PREHASH_AgentData);
	msg->addUUIDFast(   _PREHASH_AgentID, gAgent.getID() );
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->addUUIDFast(   _PREHASH_AvatarID, avatar_id);
	gAgent.sendReliableMessage();
}

void LLAvatarPropertiesProcessor::sendDataUpdate(const void* data, EAvatarProcessorType type)
{
	switch(type)
	{
	case APT_PROPERTIES:
		sendAvatarPropertiesUpdate(data);
		break;
	case APT_PICK_INFO:
		sendPicInfoUpdate(data);
	case APT_PICKS:
//		sendGenericRequest(avatar_id, "avatarpicksrequest");
		break;
	case APT_NOTES:
//		sendGenericRequest(avatar_id, "avatarnotesrequest");
		break;
	case APT_GROUPS:
//		sendGenericRequest(avatar_id, "avatargroupsrequest");
		break;
	default:
		break;
	}

}
void LLAvatarPropertiesProcessor::sendAvatarPropertiesUpdate(const void* data)
{
	llinfos << "Sending avatarinfo update" << llendl;

	const LLAvatarData* avatar_props = static_cast<const LLAvatarData*>(data);
	// This value is required by sendAvatarPropertiesUpdate method.
	//A profile should never be mature. (From the original code)
	BOOL mature = FALSE;



	LLMessageSystem *msg = gMessageSystem;

	msg->newMessageFast(_PREHASH_AvatarPropertiesUpdate);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(	_PREHASH_AgentID,		gAgent.getID() );
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID() );
	msg->nextBlockFast(_PREHASH_PropertiesData);

	msg->addUUIDFast(	_PREHASH_ImageID,	avatar_props->image_id);
	msg->addUUIDFast(	_PREHASH_FLImageID,		avatar_props->fl_image_id);
	msg->addStringFast(	_PREHASH_AboutText,		avatar_props->about_text);
	msg->addStringFast(	_PREHASH_FLAboutText,	avatar_props->fl_about_text);

	msg->addBOOL(_PREHASH_AllowPublish, avatar_props->allow_publish);
	msg->addBOOL(_PREHASH_MaturePublish, mature);
	msg->addString(_PREHASH_ProfileURL, avatar_props->profile_url);
	gAgent.sendReliableMessage();
}

void LLAvatarPropertiesProcessor::processAvatarPropertiesReply(LLMessageSystem* msg, void**)
{
	LLAvatarData avatar_data;

	msg->getUUIDFast(	_PREHASH_AgentData,			_PREHASH_AgentID, 		avatar_data.agent_id);
	msg->getUUIDFast(	_PREHASH_AgentData,			_PREHASH_AvatarID, 		avatar_data.avatar_id);
	msg->getUUIDFast(  	_PREHASH_PropertiesData,	_PREHASH_ImageID,		avatar_data.image_id);
	msg->getUUIDFast(  	_PREHASH_PropertiesData,	_PREHASH_FLImageID,		avatar_data.fl_image_id);
	msg->getUUIDFast(	_PREHASH_PropertiesData,	_PREHASH_PartnerID,		avatar_data.partner_id);
	msg->getStringFast(	_PREHASH_PropertiesData,	_PREHASH_AboutText,		avatar_data.about_text);
	msg->getStringFast(	_PREHASH_PropertiesData,	_PREHASH_FLAboutText,	avatar_data.fl_about_text);
	msg->getStringFast(	_PREHASH_PropertiesData,	_PREHASH_BornOn,		avatar_data.born_on);
	msg->getString(		_PREHASH_PropertiesData,	_PREHASH_ProfileURL,	avatar_data.profile_url);
	msg->getU32Fast(	_PREHASH_PropertiesData,	_PREHASH_Flags,			avatar_data.flags);


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
	notifyObservers(avatar_data.avatar_id,&avatar_data,APT_PROPERTIES);
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
void LLAvatarPropertiesProcessor::processAvatarClassifiedReply(LLMessageSystem* msg, void**)
{
	// avatarclassifiedsrequest is not sent according to new UI design but
	// keep this method according to resolved issues. 
}
void LLAvatarPropertiesProcessor::processAvatarNotesReply(LLMessageSystem* msg, void**)
{
	LLAvatarNotes avatar_notes;

	msg->getUUID(_PREHASH_AgentData, _PREHASH_AgentID, avatar_notes.agent_id);
	msg->getUUID(_PREHASH_Data, _PREHASH_TargetID, avatar_notes.target_id);
	msg->getString(_PREHASH_Data, _PREHASH_Notes, avatar_notes.notes);

	notifyObservers(avatar_notes.target_id,&avatar_notes,APT_NOTES);
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
	notifyObservers(avatar_picks.target_id,&avatar_picks,APT_PICKS);
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

	// "Location text" is actually the owner name, the original
	// name that owner gave the parcel, and the location.
	msg->getString(_PREHASH_Data, _PREHASH_User, pick_data.location_text);
	pick_data.location_text.append(", ");

	msg->getString(_PREHASH_Data, _PREHASH_OriginalName, pick_data.original_name);
	if (!pick_data.original_name.empty())
	{
		pick_data.location_text.append(pick_data.original_name);
		pick_data.location_text.append(", ");
	}

	msg->getString(_PREHASH_Data, _PREHASH_SimName, pick_data.sim_name);
	pick_data.location_text.append(pick_data.sim_name);
	pick_data.location_text.append(" ");

	msg->getVector3d(_PREHASH_Data, _PREHASH_PosGlobal, pick_data.pos_global);
	S32 region_x = llround((F32)pick_data.pos_global.mdV[VX]) % REGION_WIDTH_UNITS;
	S32 region_y = llround((F32)pick_data.pos_global.mdV[VY]) % REGION_WIDTH_UNITS;
	S32 region_z = llround((F32)pick_data.pos_global.mdV[VZ]);
	pick_data.location_text.append(llformat("(%d, %d, %d)", region_x, region_y, region_z));

	msg->getS32(_PREHASH_Data, _PREHASH_SortOrder, pick_data.sort_order);
	msg->getBOOL(_PREHASH_Data, _PREHASH_Enabled, pick_data.enabled);

	notifyObservers(pick_data.creator_id, &pick_data, APT_PICK_INFO);
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

	notifyObservers(avatar_groups.avatar_id,&avatar_groups,APT_GROUPS);
}

void LLAvatarPropertiesProcessor::notifyObservers(const LLUUID& id,void* data, EAvatarProcessorType type)
{
	LLAvatarPropertiesProcessor::observer_multimap_t observers = LLAvatarPropertiesProcessor::getInstance()->mObservers;

	observer_multimap_t::iterator oi = observers.lower_bound(id);
	observer_multimap_t::iterator end = observers.upper_bound(id);
	for (; oi != end; ++oi)
	{
		oi->second->processProperties(data,type);
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
}

void LLAvatarPropertiesProcessor::sendPicInfoUpdate(const void* pick_data)
{
	if (!pick_data) return;
	const LLPickData *new_pick = static_cast<const LLPickData*>(pick_data);
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
