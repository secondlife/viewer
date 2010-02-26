/** 
 * @file llinstantmessage.cpp
 * @author Phoenix
 * @date 2005-08-29
 * @brief Constants and functions used in IM.
 *
 * $LicenseInfo:firstyear=2005&license=viewergpl$
 * 
 * Copyright (c) 2005-2009, Linden Research, Inc.
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

#include "linden_common.h"

#include "lldbstrings.h"
#include "llinstantmessage.h"
#include "llhost.h"
#include "lluuid.h"
#include "llsd.h"
#include "llsdserialize.h"
#include "llsdutil_math.h"
#include "llpointer.h"
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
const std::string SYSTEM_FROM("Second Life");
const S32 IM_TTL = 1;


/**
 * LLIMInfo
 */
LLIMInfo::LLIMInfo() :
	mFromGroup(FALSE),
	mParentEstateID(0),
	mOffline(0),
	mViewerThinksToIsOnline(false),
	mIMType(IM_NOTHING_SPECIAL),
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
		mName,
		mMessage,
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
	const std::string& name,
	const std::string& message,
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
	const std::string& name,
	const std::string& message,
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
	if(!message.empty())
	{
		char buffer[MTUBYTES];
		int num_written = snprintf(buffer, MTUBYTES, "%s", message.c_str());	/* Flawfinder: ignore */
		// snprintf returns number of bytes that would have been written
		// had the output not being truncated. In that case, it will
		// return either -1 or value >= passed in size value . So a check needs to be added
		// to detect truncation, and if there is any, only account for the
		// actual number of bytes written..and not what could have been
		// written.
		if (num_written < 0 || num_written >= MTUBYTES)
		{
			num_written = MTUBYTES - 1;
			llwarns << "pack_instant_message_block: message truncated: " << message << llendl;
		}

		bytes_left -= num_written;
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
	msg->getStringFast(_PREHASH_MessageBlock, _PREHASH_FromAgentName, mName);

	msg->getStringFast(_PREHASH_MessageBlock, _PREHASH_Message, mMessage);

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

LLSD im_info_to_llsd(LLPointer<LLIMInfo> im_info)
{
	LLSD param_version;
	param_version["version"] = 1;
	LLSD param_message;
	param_message["from_id"] = im_info->mFromID;
	param_message["from_group"] = im_info->mFromGroup;
	param_message["to_id"] = im_info->mToID;
	param_message["from_name"] = im_info->mName;
	param_message["message"] = im_info->mMessage;
	param_message["type"] = (S32)im_info->mIMType;
	param_message["id"] = im_info->mID;
	param_message["timestamp"] = (S32)im_info->mTimeStamp;
	param_message["offline"] = (S32)im_info->mOffline;
	param_message["parent_estate_id"] = (S32)im_info->mParentEstateID;
	param_message["region_id"] = im_info->mRegionID;
	param_message["position"] = ll_sd_from_vector3(im_info->mPosition);
	param_message["data"] = im_info->mData;
	param_message["source"]= im_info->mSource;
	param_message["ttl"] = im_info->mTTL;

	LLSD param_agent;
	param_agent["agent_id"] = im_info->mFromID;

	LLSD params;
	params["version_params"] = param_version;
	params["message_params"] = param_message;
	params["agent_params"] = param_agent;

	return params;
}

LLPointer<LLIMInfo> llsd_to_im_info(const LLSD& im_info_sd)
{
	LLSD param_message = im_info_sd["message_params"];
	LLSD param_agent = im_info_sd["agent_params"];

	LLPointer<LLIMInfo> im_info = new LLIMInfo(
		param_message["from_id"].asUUID(),
		param_message["from_group"].asBoolean(),
		param_message["to_id"].asUUID(),
		(EInstantMessage) param_message["type"].asInteger(),
		param_message["from_name"].asString(),
		param_message["message"].asString(),
		param_message["id"].asUUID(),
		(U32) param_message["parent_estate_id"].asInteger(),
		param_message["region_id"].asUUID(),
		ll_vector3_from_sd(param_message["position"]),
		param_message["data"],
		(U8) param_message["offline"].asInteger(),
		(U32) param_message["timestamp"].asInteger(),
		(EIMSource)param_message["source"].asInteger(),
		param_message["ttl"].asInteger());

	return im_info;
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

