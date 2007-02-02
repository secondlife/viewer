/** 
 * @file llinstantmessage.cpp
 * @author Phoenix
 * @date 2005-08-29
 * @brief Constants and functions used in IM.
 *
 * Copyright (c) 2005-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "linden_common.h"

#include "lldbstrings.h"
#include "llinstantmessage.h"
#include "llhost.h"
#include "lluuid.h"
#include "llsd.h"
#include "llsdserialize.h"
#include "llmemory.h"
#include "message.h"

#include "message.h"

const U8 IM_ONLINE = 0;
const U8 IM_OFFLINE = 1;

const S32 VOTE_YES = 1;
const S32 VOTE_NO = 0;
const S32 VOTE_ABSTAIN = -1;

const S32 VOTE_MAJORITY = 0;
const S32 VOTE_SUPER_MAJORITY = 1;
const S32 VOTE_UNANIMOUS = 2;

const char EMPTY_BINARY_BUCKET[] = "";
const S32 EMPTY_BINARY_BUCKET_SIZE = 1;
const U32 NO_TIMESTAMP = 0;
const char SYSTEM_FROM[] = "Second Life";
const S32 IM_TTL = 1;


/**
 * LLIMInfo
 */
LLIMInfo::LLIMInfo() :
	mParentEstateID(0),
	mOffline(0),
	mViewerThinksToIsOnline(false),
	mTimeStamp(0),
	mSource(IM_FROM_SIM),
	mTTL(IM_TTL)
{
}

LLIMInfo::LLIMInfo(
	const LLUUID& from_id,
	BOOL from_group,
	const LLUUID& to_id,
	EInstantMessage im_type, 
	const std::string& name,
	const std::string& message,
	const LLUUID& id,
	U32 parent_estate_id,
	const LLUUID& region_id,
	const LLVector3& position,
	LLSD data,
	U8 offline,
	U32 timestamp,
	EIMSource source,
	S32 ttl) :
	mFromID(from_id),
	mFromGroup(from_group),
	mToID(to_id),
	mParentEstateID(0),
	mRegionID(region_id),
	mPosition(position),
	mOffline(offline),
	mViewerThinksToIsOnline(false),
	mIMType(im_type),
	mID(id),
	mTimeStamp(timestamp),
	mName(name),
	mMessage(message),
	mData(data),
	mSource(source),
	mTTL(ttl)
{
}

LLIMInfo::LLIMInfo(LLMessageSystem* msg, EIMSource source, S32 ttl) :
	mViewerThinksToIsOnline(false),
	mSource(source),
	mTTL(ttl)
{
	unpackMessageBlock(msg);
}

LLIMInfo::~LLIMInfo()
{
}

void LLIMInfo::packInstantMessage(LLMessageSystem* msg) const
{
	lldebugs << "LLIMInfo::packInstantMessage()" << llendl;
	msg->newMessageFast(_PREHASH_ImprovedInstantMessage);
	packMessageBlock(msg);
}

void LLIMInfo::packMessageBlock(LLMessageSystem* msg) const
{
	// Construct binary bucket
	std::vector<U8> bucket;
	if (mData.has("binary_bucket"))
	{
		bucket = mData["binary_bucket"].asBinary();
	}
	pack_instant_message_block(
		msg,
		mFromID,
		mFromGroup,
		LLUUID::null,
		mToID,
		mName.c_str(),
		mMessage.c_str(),
		mOffline,
		mIMType,
		mID,
		mParentEstateID,
		mRegionID,
		mPosition,
		mTimeStamp,
		&bucket[0],
		bucket.size());
}

void pack_instant_message(
	LLMessageSystem* msg,
	const LLUUID& from_id,
	BOOL from_group,
	const LLUUID& session_id,
	const LLUUID& to_id,
	const char* name,
	const char* message,
	U8 offline,
	EInstantMessage dialog,
	const LLUUID& id,
	U32 parent_estate_id,
	const LLUUID& region_id,
	const LLVector3& position,
	U32 timestamp, 
	const U8* binary_bucket,
	S32 binary_bucket_size)
{
	lldebugs << "pack_instant_message()" << llendl;
	msg->newMessageFast(_PREHASH_ImprovedInstantMessage);
	pack_instant_message_block(
		msg,
		from_id,
		from_group,
		session_id,
		to_id,
		name,
		message,
		offline,
		dialog,
		id,
		parent_estate_id,
		region_id,
		position,
		timestamp,
		binary_bucket,
		binary_bucket_size);
}

void pack_instant_message_block(
	LLMessageSystem* msg,
	const LLUUID& from_id,
	BOOL from_group,
	const LLUUID& session_id,
	const LLUUID& to_id,
	const char* name,
	const char* message,
	U8 offline,
	EInstantMessage dialog,
	const LLUUID& id,
	U32 parent_estate_id,
	const LLUUID& region_id,
	const LLVector3& position,
	U32 timestamp,
	const U8* binary_bucket,
	S32 binary_bucket_size)
{
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, from_id);
	msg->addUUIDFast(_PREHASH_SessionID, session_id);
	msg->nextBlockFast(_PREHASH_MessageBlock);
	msg->addBOOLFast(_PREHASH_FromGroup, from_group);
	msg->addUUIDFast(_PREHASH_ToAgentID, to_id);
	msg->addU32Fast(_PREHASH_ParentEstateID, parent_estate_id);
	msg->addUUIDFast(_PREHASH_RegionID, region_id);
	msg->addVector3Fast(_PREHASH_Position, position);
	msg->addU8Fast(_PREHASH_Offline, offline);
	msg->addU8Fast(_PREHASH_Dialog, (U8) dialog);
	msg->addUUIDFast(_PREHASH_ID, id);
	msg->addU32Fast(_PREHASH_Timestamp, timestamp);
	msg->addStringFast(_PREHASH_FromAgentName, name);
	S32 bytes_left = MTUBYTES;
	if(message)
	{
		char buffer[MTUBYTES];	/*Flawfinder: ignore*/
		bytes_left -= snprintf(buffer, MTUBYTES, "%s", message);	/*Flawfinder: ignore*/
		bytes_left = llmax(0, bytes_left);
		msg->addStringFast(_PREHASH_Message, buffer);
	}
	else
	{
		msg->addStringFast(_PREHASH_Message, NULL);
	}
	const U8* bb;
	if(binary_bucket)
	{
		bb = binary_bucket;
		binary_bucket_size = llmin(bytes_left, binary_bucket_size);
	}
	else
	{
		bb = (const U8*)EMPTY_BINARY_BUCKET;
		binary_bucket_size = EMPTY_BINARY_BUCKET_SIZE;
	}
	msg->addBinaryDataFast(_PREHASH_BinaryBucket, bb, binary_bucket_size);
}

void LLIMInfo::unpackMessageBlock(LLMessageSystem* msg)
{
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_AgentID, mFromID);
	msg->getBOOLFast(_PREHASH_MessageBlock, _PREHASH_FromGroup, mFromGroup);
	msg->getUUIDFast(_PREHASH_MessageBlock, _PREHASH_ToAgentID, mToID);
	msg->getU32Fast(_PREHASH_MessageBlock, _PREHASH_ParentEstateID, mParentEstateID);
	msg->getUUIDFast(_PREHASH_MessageBlock, _PREHASH_RegionID, mRegionID);
	msg->getVector3Fast(_PREHASH_MessageBlock, _PREHASH_Position, mPosition);
	msg->getU8Fast(_PREHASH_MessageBlock, _PREHASH_Offline, mOffline);
	U8 dialog;
	msg->getU8Fast(_PREHASH_MessageBlock, _PREHASH_Dialog, dialog);
	mIMType = (EInstantMessage) dialog;
	msg->getUUIDFast(_PREHASH_MessageBlock, _PREHASH_ID, mID);
	msg->getU32Fast(_PREHASH_MessageBlock, _PREHASH_Timestamp, mTimeStamp);
	char name[DB_FULL_NAME_BUF_SIZE];	/*Flawfinder: ignore*/
	msg->getStringFast(_PREHASH_MessageBlock, _PREHASH_FromAgentName, DB_FULL_NAME_BUF_SIZE, name);
	mName.assign(name);

	char message[DB_IM_MSG_BUF_SIZE];	/*Flawfinder: ignore*/
	msg->getStringFast(_PREHASH_MessageBlock, _PREHASH_Message, DB_IM_MSG_BUF_SIZE, message);
	mMessage.assign(message);

	S32 binary_bucket_size = llmin(
		MTUBYTES,
		msg->getSizeFast(
			_PREHASH_MessageBlock,
			_PREHASH_BinaryBucket));
	if(binary_bucket_size > 0)
	{
		std::vector<U8> bucket;
		bucket.resize(binary_bucket_size);

		msg->getBinaryDataFast(
			_PREHASH_MessageBlock,
			_PREHASH_BinaryBucket,
			&bucket[0],
			0,
			0,
			binary_bucket_size);
		mData["binary_bucket"] = bucket;
	}
	else
	{
		mData.clear();
	}
}

LLPointer<LLIMInfo> LLIMInfo::clone()
{
	return new LLIMInfo(
			mFromID,
			mFromGroup,
			mToID,
			mIMType,
			mName,
			mMessage,
			mID,
			mParentEstateID,
			mRegionID,
			mPosition,
			mData,
			mOffline,
			mTimeStamp,
			mSource,
			mTTL);
}

