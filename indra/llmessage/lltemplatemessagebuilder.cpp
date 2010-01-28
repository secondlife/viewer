/** 
 * @file lltemplatemessagebuilder.cpp
 * @brief LLTemplateMessageBuilder class implementation.
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

#include "lltemplatemessagebuilder.h"

#include "llmessagetemplate.h"
#include "llquaternion.h"
#include "u64.h"
#include "v3dmath.h"
#include "v3math.h"
#include "v4math.h"

LLTemplateMessageBuilder::LLTemplateMessageBuilder(const message_template_name_map_t& name_template_map) :
	mCurrentSMessageData(NULL),
	mCurrentSMessageTemplate(NULL),
	mCurrentSDataBlock(NULL),
	mCurrentSMessageName(NULL),
	mCurrentSBlockName(NULL),
	mbSBuilt(FALSE),
	mbSClear(TRUE),
	mCurrentSendTotal(0),
	mMessageTemplates(name_template_map)
{
}

//virtual
LLTemplateMessageBuilder::~LLTemplateMessageBuilder()
{
	delete mCurrentSMessageData;
	mCurrentSMessageData = NULL;
}

// virtual
void LLTemplateMessageBuilder::newMessage(const char *name)
{
	mbSBuilt = FALSE;
	mbSClear = FALSE;

	mCurrentSendTotal = 0;

	delete mCurrentSMessageData;
	mCurrentSMessageData = NULL;

	char* namep = (char*)name; 
	if (mMessageTemplates.count(namep) > 0)
	{
		mCurrentSMessageTemplate = mMessageTemplates.find(name)->second;
		mCurrentSMessageData = new LLMsgData(namep);
		mCurrentSMessageName = namep;
		mCurrentSDataBlock = NULL;
		mCurrentSBlockName = NULL;

		// add at one of each block
		const LLMessageTemplate* msg_template = mMessageTemplates.find(name)->second;

		if (msg_template->getDeprecation() != MD_NOTDEPRECATED)
		{
			llwarns << "Sending deprecated message " << namep << llendl;
		}
		
		LLMessageTemplate::message_block_map_t::const_iterator iter;
		for(iter = msg_template->mMemberBlocks.begin();
			iter != msg_template->mMemberBlocks.end();
			++iter)
		{
			LLMessageBlock* ci = *iter;
			LLMsgBlkData* tblockp = new LLMsgBlkData(ci->mName, 0);
			mCurrentSMessageData->addBlock(tblockp);
		}
	}
	else
	{
		llerrs << "newMessage - Message " << name << " not registered" << llendl;
	}
}

// virtual
void LLTemplateMessageBuilder::clearMessage()
{
	mbSBuilt = FALSE;
	mbSClear = TRUE;

	mCurrentSendTotal = 0;

	mCurrentSMessageTemplate = NULL;

	delete mCurrentSMessageData;
	mCurrentSMessageData = NULL;

	mCurrentSMessageName = NULL;
	mCurrentSDataBlock = NULL;
	mCurrentSBlockName = NULL;
}

// virtual
void LLTemplateMessageBuilder::nextBlock(const char* blockname)
{
	char *bnamep = (char *)blockname; 

	if (!mCurrentSMessageTemplate)
	{
		llerrs << "newMessage not called prior to setBlock" << llendl;
		return;
	}

	// now, does this block exist?
	const LLMessageBlock* template_data = mCurrentSMessageTemplate->getBlock(bnamep);
	if (!template_data)
	{
		llerrs << "LLTemplateMessageBuilder::nextBlock " << bnamep
			<< " not a block in " << mCurrentSMessageTemplate->mName << llendl;
		return;
	}
	
	// ok, have we already set this block?
	LLMsgBlkData* block_data = mCurrentSMessageData->mMemberBlocks[bnamep];
	if (block_data->mBlockNumber == 0)
	{
		// nope! set this as the current block
		block_data->mBlockNumber = 1;
		mCurrentSDataBlock = block_data;
		mCurrentSBlockName = bnamep;

		// add placeholders for each of the variables
		for (LLMessageBlock::message_variable_map_t::const_iterator iter = template_data->mMemberVariables.begin();
			 iter != template_data->mMemberVariables.end(); iter++)
		{
			LLMessageVariable& ci = **iter;
			mCurrentSDataBlock->addVariable(ci.getName(), ci.getType());
		}
		return;
	}
	else
	{
		// already have this block. . . 
		// are we supposed to have a new one?

		// if the block is type MBT_SINGLE this is bad!
		if (template_data->mType == MBT_SINGLE)
		{
			llerrs << "LLTemplateMessageBuilder::nextBlock called multiple times"
				<< " for " << bnamep << " but is type MBT_SINGLE" << llendl;
			return;
		}


		// if the block is type MBT_MULTIPLE then we need a known number, 
		// make sure that we're not exceeding it
		if (  (template_data->mType == MBT_MULTIPLE)
			&&(mCurrentSDataBlock->mBlockNumber == template_data->mNumber))
		{
			llerrs << "LLTemplateMessageBuilder::nextBlock called "
				<< mCurrentSDataBlock->mBlockNumber << " times for " << bnamep
				<< " exceeding " << template_data->mNumber
				<< " specified in type MBT_MULTIPLE." << llendl;
			return;
		}

		// ok, we can make a new one
		// modify the name to avoid name collision by adding number to end
		S32  count = block_data->mBlockNumber;

		// incrememt base name's count
		block_data->mBlockNumber++;

		if (block_data->mBlockNumber > MAX_BLOCKS)
		{
			llerrs << "Trying to pack too many blocks into MBT_VARIABLE type "
				   << "(limited to " << MAX_BLOCKS << ")" << llendl;
		}

		// create new name
		// Nota Bene: if things are working correctly, 
		// mCurrentMessageData->mMemberBlocks[blockname]->mBlockNumber == 
		// mCurrentDataBlock->mBlockNumber + 1

		char *nbnamep = bnamep + count;
	
		mCurrentSDataBlock = new LLMsgBlkData(bnamep, count);
		mCurrentSDataBlock->mName = nbnamep;
		mCurrentSMessageData->mMemberBlocks[nbnamep] = mCurrentSDataBlock;

		// add placeholders for each of the variables
		for (LLMessageBlock::message_variable_map_t::const_iterator
				 iter = template_data->mMemberVariables.begin(),
				 end = template_data->mMemberVariables.end();
			 iter != end; iter++)
		{
			LLMessageVariable& ci = **iter;
			mCurrentSDataBlock->addVariable(ci.getName(), ci.getType());
		}
		return;
	}
}

// TODO: Remove this horror...
BOOL LLTemplateMessageBuilder::removeLastBlock()
{
	if (mCurrentSBlockName)
	{
		if (  (mCurrentSMessageData)
			&&(mCurrentSMessageTemplate))
		{
			if (mCurrentSMessageData->mMemberBlocks[mCurrentSBlockName]->mBlockNumber >= 1)
			{
				// At least one block for the current block name.

				// Store the current block name for future reference.
				char *block_name = mCurrentSBlockName;

				// Decrement the sent total by the size of the
				// data in the message block that we're currently building.

				const LLMessageBlock* template_data = mCurrentSMessageTemplate->getBlock(mCurrentSBlockName);
				
				for (LLMessageBlock::message_variable_map_t::const_iterator iter = template_data->mMemberVariables.begin();
					 iter != template_data->mMemberVariables.end(); iter++)
				{
					LLMessageVariable& ci = **iter;
					mCurrentSendTotal -= ci.getSize();
				}


				// Now we want to find the block that we're blowing away.

				// Get the number of blocks.
				LLMsgBlkData* block_data = mCurrentSMessageData->mMemberBlocks[block_name];
				S32 num_blocks = block_data->mBlockNumber;

				// Use the same (suspect?) algorithm that's used to generate
				// the names in the nextBlock method to find it.
				char *block_getting_whacked = block_name + num_blocks - 1;
				LLMsgBlkData* whacked_data = mCurrentSMessageData->mMemberBlocks[block_getting_whacked];
				delete whacked_data;
				mCurrentSMessageData->mMemberBlocks.erase(block_getting_whacked);

				if (num_blocks <= 1)
				{
					// we just blew away the last one, so return FALSE
					llwarns << "not blowing away the only block of message "
							<< mCurrentSMessageName
							<< ". Block: " << block_name
							<< ". Number: " << num_blocks
							<< llendl;
					return FALSE;
				}
				else
				{
					// Decrement the counter.
					block_data->mBlockNumber--;
					return TRUE;
				}
			}
		}
	}
	return FALSE;
}

// add data to variable in current block
void LLTemplateMessageBuilder::addData(const char *varname, const void *data, EMsgVariableType type, S32 size)
{
	char *vnamep = (char *)varname; 

	// do we have a current message?
	if (!mCurrentSMessageTemplate)
	{
		llerrs << "newMessage not called prior to addData" << llendl;
		return;
	}

	// do we have a current block?
	if (!mCurrentSDataBlock)
	{
		llerrs << "setBlock not called prior to addData" << llendl;
		return;
	}

	// kewl, add the data if it exists
	const LLMessageVariable* var_data = mCurrentSMessageTemplate->getBlock(mCurrentSBlockName)->getVariable(vnamep);
	if (!var_data || !var_data->getName())
	{
		llerrs << vnamep << " not a variable in block " << mCurrentSBlockName << " of " << mCurrentSMessageTemplate->mName << llendl;
		return;
	}

	// ok, it seems ok. . . are we the correct size?
	if (var_data->getType() == MVT_VARIABLE)
	{
		// Variable 1 can only store 255 bytes, make sure our data is smaller
		if ((var_data->getSize() == 1) &&
			(size > 255))
		{
			llwarns << "Field " << varname << " is a Variable 1 but program "
			       << "attempted to stuff more than 255 bytes in "
			       << "(" << size << ").  Clamping size and truncating data." << llendl;
			size = 255;
			char *truncate = (char *)data;
			truncate[255] = 0;
		}

		// no correct size for MVT_VARIABLE, instead we need to tell how many bytes the size will be encoded as
		mCurrentSDataBlock->addData(vnamep, data, size, type, var_data->getSize());
		mCurrentSendTotal += size;
	}
	else
	{
		if (size != var_data->getSize())
		{
			llerrs << varname << " is type MVT_FIXED but request size " << size << " doesn't match template size "
				   << var_data->getSize() << llendl;
			return;
		}
		// alright, smash it in
		mCurrentSDataBlock->addData(vnamep, data, size, type);
		mCurrentSendTotal += size;
	}
}

// add data to variable in current block - fails if variable isn't MVT_FIXED
void LLTemplateMessageBuilder::addData(const char *varname, const void *data, EMsgVariableType type)
{
	char *vnamep = (char *)varname; 

	// do we have a current message?
	if (!mCurrentSMessageTemplate)
	{
		llerrs << "newMessage not called prior to addData" << llendl;
		return;
	}

	// do we have a current block?
	if (!mCurrentSDataBlock)
	{
		llerrs << "setBlock not called prior to addData" << llendl;
		return;
	}

	// kewl, add the data if it exists
	const LLMessageVariable* var_data = mCurrentSMessageTemplate->getBlock(mCurrentSBlockName)->getVariable(vnamep);
	if (!var_data->getName())
	{
		llerrs << vnamep << " not a variable in block " << mCurrentSBlockName << " of " << mCurrentSMessageTemplate->mName << llendl;
		return;
	}

	// ok, it seems ok. . . are we MVT_VARIABLE?
	if (var_data->getType() == MVT_VARIABLE)
	{
		// nope
		llerrs << vnamep << " is type MVT_VARIABLE. Call using addData(name, data, size)" << llendl;
		return;
	}
	else
	{
		mCurrentSDataBlock->addData(vnamep, data, var_data->getSize(), type);
		mCurrentSendTotal += var_data->getSize();
	}
}

void LLTemplateMessageBuilder::addBinaryData(const char *varname, 
											const void *data, S32 size)
{
	addData(varname, data, MVT_FIXED, size);
}

void LLTemplateMessageBuilder::addS8(const char *varname, S8 s)
{
	addData(varname, &s, MVT_S8, sizeof(s));
}

void LLTemplateMessageBuilder::addU8(const char *varname, U8 u)
{
	addData(varname, &u, MVT_U8, sizeof(u));
}

void LLTemplateMessageBuilder::addS16(const char *varname, S16 i)
{
	addData(varname, &i, MVT_S16, sizeof(i));
}

void LLTemplateMessageBuilder::addU16(const char *varname, U16 i)
{
	addData(varname, &i, MVT_U16, sizeof(i));
}

void LLTemplateMessageBuilder::addF32(const char *varname, F32 f)
{
	addData(varname, &f, MVT_F32, sizeof(f));
}

void LLTemplateMessageBuilder::addS32(const char *varname, S32 s)
{
	addData(varname, &s, MVT_S32, sizeof(s));
}

void LLTemplateMessageBuilder::addU32(const char *varname, U32 u)
{
	addData(varname, &u, MVT_U32, sizeof(u));
}

void LLTemplateMessageBuilder::addU64(const char *varname, U64 lu)
{
	addData(varname, &lu, MVT_U64, sizeof(lu));
}

void LLTemplateMessageBuilder::addF64(const char *varname, F64 d)
{
	addData(varname, &d, MVT_F64, sizeof(d));
}

void LLTemplateMessageBuilder::addIPAddr(const char *varname, U32 u)
{
	addData(varname, &u, MVT_IP_ADDR, sizeof(u));
}

void LLTemplateMessageBuilder::addIPPort(const char *varname, U16 u)
{
	u = htons(u);
	addData(varname, &u, MVT_IP_PORT, sizeof(u));
}

void LLTemplateMessageBuilder::addBOOL(const char* varname, BOOL b)
{
	// Can't just cast a BOOL (actually a U32) to a U8.
	// In some cases the low order bits will be zero.
	U8 temp = (b != 0);
	addData(varname, &temp, MVT_BOOL, sizeof(temp));
}

void LLTemplateMessageBuilder::addString(const char* varname, const char* s)
{
	if (s)
		addData( varname, (void *)s, MVT_VARIABLE, (S32)strlen(s) + 1);  /* Flawfinder: ignore */  
	else
		addData( varname, NULL, MVT_VARIABLE, 0); 
}

void LLTemplateMessageBuilder::addString(const char* varname, const std::string& s)
{
	if (s.size())
		addData( varname, (void *)s.c_str(), MVT_VARIABLE, (S32)(s.size()) + 1); 
	else
		addData( varname, NULL, MVT_VARIABLE, 0); 
}

void LLTemplateMessageBuilder::addVector3(const char *varname, const LLVector3& vec)
{
	addData(varname, vec.mV, MVT_LLVector3, sizeof(vec.mV));
}

void LLTemplateMessageBuilder::addVector4(const char *varname, const LLVector4& vec)
{
	addData(varname, vec.mV, MVT_LLVector4, sizeof(vec.mV));
}

void LLTemplateMessageBuilder::addVector3d(const char *varname, const LLVector3d& vec)
{
	addData(varname, vec.mdV, MVT_LLVector3d, sizeof(vec.mdV));
}

void LLTemplateMessageBuilder::addQuat(const char *varname, const LLQuaternion& quat)
{
	addData(varname, quat.packToVector3().mV, MVT_LLQuaternion, sizeof(LLVector3));
}

void LLTemplateMessageBuilder::addUUID(const char *varname, const LLUUID& uuid)
{
	addData(varname, uuid.mData, MVT_LLUUID, sizeof(uuid.mData));
}

static S32 zero_code(U8 **data, U32 *data_size)
{
	// Encoded send buffer needs to be slightly larger since the zero
	// coding can potentially increase the size of the send data.
	static U8 encodedSendBuffer[2 * MAX_BUFFER_SIZE];

	S32 count = *data_size;
	
	S32 net_gain = 0;
	U8 num_zeroes = 0;
	
	U8 *inptr = (U8 *)*data;
	U8 *outptr = (U8 *)encodedSendBuffer;

// skip the packet id field

	for (U32 ii = 0; ii < LL_PACKET_ID_SIZE ; ++ii)
	{
		count--;
		*outptr++ = *inptr++;
	}

// build encoded packet, keeping track of net size gain

// sequential zero bytes are encoded as 0 [U8 count] 
// with 0 0 [count] representing wrap (>256 zeroes)

	while (count--)
	{
		if (!(*inptr))   // in a zero count
		{
			if (num_zeroes)
			{
				if (++num_zeroes > 254)
				{
					*outptr++ = num_zeroes;
					num_zeroes = 0;
				}
				net_gain--;   // subseqent zeroes save one
			}
			else
			{
				*outptr++ = 0;
				net_gain++;  // starting a zero count adds one
				num_zeroes = 1;
			}
			inptr++;
		}
		else
		{
			if (num_zeroes)
			{
				*outptr++ = num_zeroes;
				num_zeroes = 0;
			}
			*outptr++ = *inptr++;
		}
	}

	if (num_zeroes)
	{
		*outptr++ = num_zeroes;
	}

	if (net_gain < 0)
	{
		// TODO: babbage: reinstate stat collecting...
		//mCompressedPacketsOut++;
		//mUncompressedBytesOut += *data_size;

		*data = encodedSendBuffer;
		*data_size += net_gain;
		encodedSendBuffer[0] |= LL_ZERO_CODE_FLAG;          // set the head bit to indicate zero coding

		//mCompressedBytesOut += *data_size;

	}
	//mTotalBytesOut += *data_size;

	return(net_gain);
}

void LLTemplateMessageBuilder::compressMessage(U8*& buf_ptr, U32& buffer_length)
{
	if(ME_ZEROCODED == mCurrentSMessageTemplate->getEncoding())
	{
		zero_code(&buf_ptr, &buffer_length);
	}
}

BOOL LLTemplateMessageBuilder::isMessageFull(const char* blockname) const
{
	if(mCurrentSendTotal > MTUBYTES)
	{
		return TRUE;
	}
	if(!blockname)
	{
		return FALSE;
	}
	char* bnamep = (char*)blockname;
	S32 max;

	const LLMessageBlock* template_data = mCurrentSMessageTemplate->getBlock(bnamep);
	
	switch(template_data->mType)
	{
	case MBT_SINGLE:
		max = 1;
		break;
	case MBT_MULTIPLE:
		max = template_data->mNumber;
		break;
	case MBT_VARIABLE:
	default:
		max = MAX_BLOCKS;
		break;
	}
	if(mCurrentSMessageData->mMemberBlocks[bnamep]->mBlockNumber >= max)
	{
		return TRUE;
	}
	return FALSE;
}

static S32 buildBlock(U8* buffer, S32 buffer_size, const LLMessageBlock* template_data, LLMsgData* message_data)
{
	S32 result = 0;
	LLMsgData::msg_blk_data_map_t::const_iterator block_iter = message_data->mMemberBlocks.find(template_data->mName);
	const LLMsgBlkData* mbci = block_iter->second;
		
	// ok, if this is the first block of a repeating pack, set
	// block_count and, if it's type MBT_VARIABLE encode a byte
	// for how many there are
	S32 block_count = mbci->mBlockNumber;
	if (template_data->mType == MBT_VARIABLE)
	{
		// remember that mBlockNumber is a S32
		U8 temp_block_number = (U8)mbci->mBlockNumber;
		if ((S32)(result + sizeof(U8)) < MAX_BUFFER_SIZE)
		{
			memcpy(&buffer[result], &temp_block_number, sizeof(U8));
			result += sizeof(U8);
		}
		else
		{
			// Just reporting error is likely not enough. Need
			// to check how to abort or error out gracefully
			// from this function. XXXTBD
			llerrs << "buildBlock failed. Message excedding "
					<< "sendBuffersize." << llendl;
		}
	}
	else if (template_data->mType == MBT_MULTIPLE)
	{
		if (block_count != template_data->mNumber)
		{
			// nope!  need to fill it in all the way!
			llerrs << "Block " << mbci->mName
				<< " is type MBT_MULTIPLE but only has data for "
				<< block_count << " out of its "
				<< template_data->mNumber << " blocks" << llendl;
		}
	}

	while(block_count > 0)
	{
		// now loop through the variables
		for (LLMsgBlkData::msg_var_data_map_t::const_iterator iter = mbci->mMemberVarData.begin();
			 iter != mbci->mMemberVarData.end(); iter++)
		{
			const LLMsgVarData& mvci = *iter;
			if (mvci.getSize() == -1)
			{
				// oops, this variable wasn't ever set!
				llerrs << "The variable " << mvci.getName() << " in block "
					<< mbci->mName << " of message "
					<< template_data->mName
					<< " wasn't set prior to buildMessage call" << llendl;
			}
			else
			{
				S32 data_size = mvci.getDataSize();
				if(data_size > 0)
				{
					// The type is MVT_VARIABLE, which means that we
					// need to encode a size argument. Otherwise,
					// there is no need.
					S32 size = mvci.getSize();
					U8 sizeb;
					U16 sizeh;
					switch(data_size)
					{
					case 1:
						sizeb = size;
						htonmemcpy(&buffer[result], &sizeb, MVT_U8, 1);
						break;
					case 2:
						sizeh = size;
						htonmemcpy(&buffer[result], &sizeh, MVT_U16, 2);
						break;
					case 4:
						htonmemcpy(&buffer[result], &size, MVT_S32, 4);
						break;
					default:
						llerrs << "Attempting to build variable field with unknown size of " << size << llendl;
						break;
					}
					result += mvci.getDataSize();
				}

				// if there is any data to pack, pack it
				if((mvci.getData() != NULL) && mvci.getSize())
				{
					if(result + mvci.getSize() < buffer_size)
					{
					    memcpy(
							&buffer[result],
							mvci.getData(),
							mvci.getSize());
					    result += mvci.getSize();
					}
					else
					{
					    // Just reporting error is likely not
					    // enough. Need to check how to abort or error
					    // out gracefully from this function. XXXTBD
						llerrs << "buildBlock failed. "
							<< "Attempted to pack "
							<< (result + mvci.getSize())
							<< " bytes into a buffer with size "
							<< buffer_size << "." << llendl;
					}						
				}
			}
		}

		--block_count;
		
		if (block_iter != message_data->mMemberBlocks.end())
		{
			++block_iter;
			if (block_iter != message_data->mMemberBlocks.end())
			{
				mbci = block_iter->second;
			}
		}
	}

	return result;
}


// make sure that all the desired data is in place and then copy the data into MAX_BUFFER_SIZEd buffer
U32 LLTemplateMessageBuilder::buildMessage(
	U8* buffer,
	U32 buffer_size,
	U8 offset_to_data)
{
	// basic algorithm is to loop through the various pieces, building
	// size and offset info if we encounter a -1 for mSize at any
	// point that variable wasn't given data

	// do we have a current message?
	if (!mCurrentSMessageTemplate)
	{
		llerrs << "newMessage not called prior to buildMessage" << llendl;
		return 0;
	}

	// leave room for flags, packet sequence #, and data offset
	// information.
	buffer[PHL_OFFSET] = offset_to_data;
	U32 result = LL_PACKET_ID_SIZE;

	// encode message number and adjust total_offset
	if (mCurrentSMessageTemplate->mFrequency == MFT_HIGH)
	{
// old, endian-dependant way
//		memcpy(&buffer[result], &mCurrentMessageTemplate->mMessageNumber, sizeof(U8));

// new, independant way
		buffer[result] = (U8)mCurrentSMessageTemplate->mMessageNumber;
		result += sizeof(U8);
	}
	else if (mCurrentSMessageTemplate->mFrequency == MFT_MEDIUM)
	{
		U8 temp = 255;
		memcpy(&buffer[result], &temp, sizeof(U8));  /*Flawfinder: ignore*/
		result += sizeof(U8);

		// mask off unsightly bits
		temp = mCurrentSMessageTemplate->mMessageNumber & 255;
		memcpy(&buffer[result], &temp, sizeof(U8));  /*Flawfinder: ignore*/
		result += sizeof(U8);
	}
	else if (mCurrentSMessageTemplate->mFrequency == MFT_LOW)
	{
		U8 temp = 255;
		U16  message_num;
		memcpy(&buffer[result], &temp, sizeof(U8));  /*Flawfinder: ignore*/
		result += sizeof(U8);
		memcpy(&buffer[result], &temp, sizeof(U8));  /*Flawfinder: ignore*/
		result += sizeof(U8);

		// mask off unsightly bits
		message_num = mCurrentSMessageTemplate->mMessageNumber & 0xFFFF;

	    // convert to network byte order
		message_num = htons(message_num);
		memcpy(&buffer[result], &message_num, sizeof(U16)); /*Flawfinder: ignore*/
		result += sizeof(U16);
	}
	else
	{
		llerrs << "unexpected message frequency in buildMessage" << llendl;
		return 0;
	}

	// fast forward through the offset and build the message
	result += offset_to_data;
	for(LLMessageTemplate::message_block_map_t::const_iterator
			iter = mCurrentSMessageTemplate->mMemberBlocks.begin(),
			end = mCurrentSMessageTemplate->mMemberBlocks.end();
		 iter != end;
		++iter)
	{
		result += buildBlock(buffer + result, buffer_size - result, *iter, mCurrentSMessageData);
	}
	mbSBuilt = TRUE;

	return result;
}

void LLTemplateMessageBuilder::copyFromMessageData(const LLMsgData& data)
{
	// copy the blocks
	// counting variables used to encode multiple block info
	S32 block_count = 0;
    char *block_name = NULL;

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
			block_name = (char *)mbci->mName;
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
			addData(mvci.getName(), mvci.getData(), mvci.getType(), mvci.getSize());
		}
	}
}

//virtual 
void LLTemplateMessageBuilder::copyFromLLSD(const LLSD&)
{
	// TODO
}

//virtual
void LLTemplateMessageBuilder::setBuilt(BOOL b) { mbSBuilt = b; }

//virtual 
BOOL LLTemplateMessageBuilder::isBuilt() const {return mbSBuilt;}

//virtual 
BOOL LLTemplateMessageBuilder::isClear() const {return mbSClear;}

//virtual 
S32 LLTemplateMessageBuilder::getMessageSize() {return mCurrentSendTotal;}

//virtual 
const char* LLTemplateMessageBuilder::getMessageName() const 
{
	return mCurrentSMessageName;
}
