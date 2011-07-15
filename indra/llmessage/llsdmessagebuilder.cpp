/** 
 * @file llsdmessagebuilder.cpp
 * @brief LLSDMessageBuilder class implementation.
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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

#include "linden_common.h"

#include "llsdmessagebuilder.h"

#include "llmessagetemplate.h"
#include "llmath.h"
#include "llquaternion.h"
#include "llsdutil.h"
#include "llsdutil_math.h"
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
			const LLMsgVarData& mvci = *dit;
			const char* varname = mvci.getName();

			switch(mvci.getType())
			{
			case MVT_FIXED:
				addBinaryData(varname, mvci.getData(), mvci.getSize());
				break;

			case MVT_VARIABLE:
				{
					const char end = ((const char*)mvci.getData())[mvci.getSize()-1]; // Ensure null terminated
					if (mvci.getDataSize() == 1 && end == 0) 
					{
						addString(varname, (const char*)mvci.getData());
					}
					else
					{
						addBinaryData(varname, mvci.getData(), mvci.getSize());
					}
					break;
				}

			case MVT_U8:
				addU8(varname, *(U8*)mvci.getData());
				break;

			case MVT_U16:
				addU16(varname, *(U16*)mvci.getData());
				break;

			case MVT_U32:
				addU32(varname, *(U32*)mvci.getData());
				break;

			case MVT_U64:
				addU64(varname, *(U64*)mvci.getData());
				break;

			case MVT_S8:
				addS8(varname, *(S8*)mvci.getData());
				break;

			case MVT_S16:
				addS16(varname, *(S16*)mvci.getData());
				break;

			case MVT_S32:
				addS32(varname, *(S32*)mvci.getData());
				break;

			// S64 not supported in LLSD so we just truncate it
			case MVT_S64:
				addS32(varname, *(S64*)mvci.getData());
				break;

			case MVT_F32:
				addF32(varname, *(F32*)mvci.getData());
				break;

			case MVT_F64:
				addF64(varname, *(F64*)mvci.getData());
				break;

			case MVT_LLVector3:
				addVector3(varname, *(LLVector3*)mvci.getData());
				break;

			case MVT_LLVector3d:
				addVector3d(varname, *(LLVector3d*)mvci.getData());
				break;

			case MVT_LLVector4:
				addVector4(varname, *(LLVector4*)mvci.getData());
				break;

			case MVT_LLQuaternion:
				{
					LLVector3 v = *(LLVector3*)mvci.getData();
					LLQuaternion q;
					q.unpackFromVector3(v);
					addQuat(varname, q);
					break;
				}

			case MVT_LLUUID:
				addUUID(varname, *(LLUUID*)mvci.getData());
				break;	

			case MVT_BOOL:
				addBOOL(varname, *(BOOL*)mvci.getData());
				break;

			case MVT_IP_ADDR:
				addIPAddr(varname, *(U32*)mvci.getData());
				break;

			case MVT_IP_PORT:
				addIPPort(varname, *(U16*)mvci.getData());
				break;

			case MVT_U16Vec3:
				//treated as an array of 6 bytes
				addBinaryData(varname, mvci.getData(), 6);
				break;

			case MVT_U16Quat:
				//treated as an array of 8 bytes
				addBinaryData(varname, mvci.getData(), 8);
				break;

			case MVT_S16Array:
				addBinaryData(varname, mvci.getData(), mvci.getSize());
				break;

			default:
				llwarns << "Unknown type in conversion of message to LLSD" << llendl;
				break;
			}
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
