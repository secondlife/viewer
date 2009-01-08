/** 
 * @file llsdmessagebuilder.cpp
 * @brief LLSDMessageBuilder class implementation.
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007-2009, Linden Research, Inc.
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

#include "llsdmessagebuilder.h"

#include "llmessagetemplate.h"
#include "llquaternion.h"
#include "llsdutil.h"
#include "llsdserialize.h"
#include "u64.h"
#include "v3dmath.h"
#include "v3math.h"
#include "v4math.h"

LLSDMessageBuilder::LLSDMessageBuilder() :
	mCurrentMessage(LLSD::emptyMap()),
	mCurrentBlock(NULL),
	mCurrentMessageName(""),
	mCurrentBlockName(""),
	mbSBuilt(FALSE),
	mbSClear(TRUE)
{
}

//virtual
LLSDMessageBuilder::~LLSDMessageBuilder()
{
}


// virtual
void LLSDMessageBuilder::newMessage(const char* name)
{
	mbSBuilt = FALSE;
	mbSClear = FALSE;

	mCurrentMessage = LLSD::emptyMap();
	mCurrentMessageName = (char*)name;
}

// virtual
void LLSDMessageBuilder::clearMessage()
{
	mbSBuilt = FALSE;
	mbSClear = TRUE;

	mCurrentMessage = LLSD::emptyMap();
	mCurrentMessageName = "";
}

// virtual
void LLSDMessageBuilder::nextBlock(const char* blockname)
{
	LLSD& block = mCurrentMessage[blockname];
	if(block.isUndefined())
	{
		block[0] = LLSD::emptyMap();
		mCurrentBlock = &(block[0]);
	}
	else if(block.isArray())
	{
		block[block.size()] = LLSD::emptyMap();
		mCurrentBlock = &(block[block.size() - 1]);
	}
	else
	{
		llerrs << "existing block not array" << llendl;
	}
}

// TODO: Remove this horror...
BOOL LLSDMessageBuilder::removeLastBlock()
{
	/* TODO: finish implementing this */
	return FALSE;
}

void LLSDMessageBuilder::addBinaryData(
	const char* varname, 
	const void* data,
	S32 size)
{
	std::vector<U8> v;
	v.resize(size);
	memcpy(&(v[0]), reinterpret_cast<const U8*>(data), size);
	(*mCurrentBlock)[varname] = v;
}

void LLSDMessageBuilder::addS8(const char* varname, S8 v)
{
	(*mCurrentBlock)[varname] = v;
}

void LLSDMessageBuilder::addU8(const char* varname, U8 v)
{
	(*mCurrentBlock)[varname] = v;
}

void LLSDMessageBuilder::addS16(const char* varname, S16 v)
{
	(*mCurrentBlock)[varname] = v;
}

void LLSDMessageBuilder::addU16(const char* varname, U16 v)
{
	(*mCurrentBlock)[varname] = v;
}

void LLSDMessageBuilder::addF32(const char* varname, F32 v)
{
	(*mCurrentBlock)[varname] = v;
}

void LLSDMessageBuilder::addS32(const char* varname, S32 v)
{
	(*mCurrentBlock)[varname] = v;
}

void LLSDMessageBuilder::addU32(const char* varname, U32 v)
{
	(*mCurrentBlock)[varname] = ll_sd_from_U32(v);
}

void LLSDMessageBuilder::addU64(const char* varname, U64 v)
{
	(*mCurrentBlock)[varname] = ll_sd_from_U64(v);
}

void LLSDMessageBuilder::addF64(const char* varname, F64 v)
{
	(*mCurrentBlock)[varname] = v;
}

void LLSDMessageBuilder::addIPAddr(const char* varname, U32 v)
{
	(*mCurrentBlock)[varname] = ll_sd_from_ipaddr(v);
}

void LLSDMessageBuilder::addIPPort(const char* varname, U16 v)
{
	(*mCurrentBlock)[varname] = v;
}

void LLSDMessageBuilder::addBOOL(const char* varname, BOOL v)
{
	(*mCurrentBlock)[varname] = (v == TRUE);
}

void LLSDMessageBuilder::addString(const char* varname, const char* v)
{
	if (v)
		(*mCurrentBlock)[varname] = v;  /* Flawfinder: ignore */  
	else
		(*mCurrentBlock)[varname] = ""; 
}

void LLSDMessageBuilder::addString(const char* varname, const std::string& v)
{
	if (v.size())
		(*mCurrentBlock)[varname] = v; 
	else
		(*mCurrentBlock)[varname] = ""; 
}

void LLSDMessageBuilder::addVector3(const char* varname, const LLVector3& v)
{
	(*mCurrentBlock)[varname] = ll_sd_from_vector3(v);
}

void LLSDMessageBuilder::addVector4(const char* varname, const LLVector4& v)
{
	(*mCurrentBlock)[varname] = ll_sd_from_vector4(v);
}

void LLSDMessageBuilder::addVector3d(const char* varname, const LLVector3d& v)
{
	(*mCurrentBlock)[varname] = ll_sd_from_vector3d(v);
}

void LLSDMessageBuilder::addQuat(const char* varname, const LLQuaternion& v)
{
	(*mCurrentBlock)[varname] = ll_sd_from_quaternion(v);
}

void LLSDMessageBuilder::addUUID(const char* varname, const LLUUID& v)
{
	(*mCurrentBlock)[varname] = v;
}

void LLSDMessageBuilder::compressMessage(U8*& buf_ptr, U32& buffer_length)
{
}

BOOL LLSDMessageBuilder::isMessageFull(const char* blockname) const
{
	return FALSE;
}

U32 LLSDMessageBuilder::buildMessage(U8*, U32, U8)
{
	return 0;
}

void LLSDMessageBuilder::copyFromMessageData(const LLMsgData& data)
{
	// copy the blocks
	// counting variables used to encode multiple block info
	S32 block_count = 0;
    char* block_name = NULL;

	// loop through msg blocks to loop through variables, totalling up size
	// data and filling the new (send) message
	LLMsgData::msg_blk_data_map_t::const_iterator iter = 
		data.mMemberBlocks.begin();
	LLMsgData::msg_blk_data_map_t::const_iterator end = 
		data.mMemberBlocks.end();
	for(; iter != end; ++iter)
	{
		const LLMsgBlkData* mbci = iter->second;
		if(!mbci) continue;

		// do we need to encode a block code?
		if (block_count == 0)
		{
			block_count = mbci->mBlockNumber;
			block_name = (char*)mbci->mName;
		}

		// counting down mutliple blocks
		block_count--;

		nextBlock(block_name);

		// now loop through the variables
		LLMsgBlkData::msg_var_data_map_t::const_iterator dit = mbci->mMemberVarData.begin();
		LLMsgBlkData::msg_var_data_map_t::const_iterator dend = mbci->mMemberVarData.end();
		
		for(; dit != dend; ++dit)
		{
			//const LLMsgVarData& mvci = *dit;

			// TODO: Copy mvci data in to block:
			// (*mCurrentBlock)[varname] = v;
		}
	}
}

//virtual
void LLSDMessageBuilder::copyFromLLSD(const LLSD& msg)
{
	mCurrentMessage = msg;
	lldebugs << LLSDNotationStreamer(mCurrentMessage) << llendl;
}

const LLSD& LLSDMessageBuilder::getMessage() const
{
	 return mCurrentMessage;
}

//virtual
void LLSDMessageBuilder::setBuilt(BOOL b) { mbSBuilt = b; }

//virtual 
BOOL LLSDMessageBuilder::isBuilt() const {return mbSBuilt;}

//virtual 
BOOL LLSDMessageBuilder::isClear() const {return mbSClear;}

//virtual 
S32 LLSDMessageBuilder::getMessageSize()
{
	// babbage: size is unknown as message stored as LLSD.
	// return non-zero if pending data, as send can be skipped for 0 size.
	// return 1 to encourage senders checking size against splitting message.
	return mCurrentMessage.size()? 1 : 0;
}

//virtual 
const char* LLSDMessageBuilder::getMessageName() const 
{
	return mCurrentMessageName.c_str();
}
