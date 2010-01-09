/** 
 * @file lltemplatemessagereader.cpp
 * @brief LLTemplateMessageReader class implementation.
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
#include "lltemplatemessagereader.h"

#include "llfasttimer.h"
#include "llmessagebuilder.h"
#include "llmessagetemplate.h"
#include "llquaternion.h"
#include "message.h"
#include "u64.h"
#include "v3dmath.h"
#include "v3math.h"
#include "v4math.h"

LLTemplateMessageReader::LLTemplateMessageReader(message_template_number_map_t&
												 number_template_map) :
	mReceiveSize(0),
	mCurrentRMessageTemplate(NULL),
	mCurrentRMessageData(NULL),
	mMessageNumbers(number_template_map)
{
}

//virtual 
LLTemplateMessageReader::~LLTemplateMessageReader()
{
	delete mCurrentRMessageData;
	mCurrentRMessageData = NULL;
}

//virtual
void LLTemplateMessageReader::clearMessage()
{
	mReceiveSize = -1;
	mCurrentRMessageTemplate = NULL;
	delete mCurrentRMessageData;
	mCurrentRMessageData = NULL;
}

void LLTemplateMessageReader::getData(const char *blockname, const char *varname, void *datap, S32 size, S32 blocknum, S32 max_size)
{
	// is there a message ready to go?
	if (mReceiveSize == -1)
	{
		llerrs << "No message waiting for decode 2!" << llendl;
		return;
	}

	if (!mCurrentRMessageData)
	{
		llerrs << "Invalid mCurrentMessageData in getData!" << llendl;
		return;
	}

	char *bnamep = (char *)blockname + blocknum; // this works because it's just a hash.  The bnamep is never derefference
	char *vnamep = (char *)varname; 

	LLMsgData::msg_blk_data_map_t::const_iterator iter = mCurrentRMessageData->mMemberBlocks.find(bnamep);

	if (iter == mCurrentRMessageData->mMemberBlocks.end())
	{
		llerrs << "Block " << blockname << " #" << blocknum
			<< " not in message " << mCurrentRMessageData->mName << llendl;
		return;
	}

	LLMsgBlkData *msg_block_data = iter->second;
	LLMsgVarData& vardata = msg_block_data->mMemberVarData[vnamep];

	if (!vardata.getName())
	{
		llerrs << "Variable "<< vnamep << " not in message "
			<< mCurrentRMessageData->mName<< " block " << bnamep << llendl;
		return;
	}

	if (size && size != vardata.getSize())
	{
		llerrs << "Msg " << mCurrentRMessageData->mName 
			<< " variable " << vnamep
			<< " is size " << vardata.getSize()
			<< " but copying into buffer of size " << size
			<< llendl;
		return;
	}


	const S32 vardata_size = vardata.getSize();
	if( max_size >= vardata_size )
	{   
		switch( vardata_size )
		{ 
		case 1:
			*((U8*)datap) = *((U8*)vardata.getData());
			break;
		case 2:
			*((U16*)datap) = *((U16*)vardata.getData());
			break;
		case 4:
			*((U32*)datap) = *((U32*)vardata.getData());
			break;
		case 8:
			((U32*)datap)[0] = ((U32*)vardata.getData())[0];
			((U32*)datap)[1] = ((U32*)vardata.getData())[1];
			break;
		default:
			memcpy(datap, vardata.getData(), vardata_size);
			break;
		}
	}
	else
	{
		llwarns << "Msg " << mCurrentRMessageData->mName 
			<< " variable " << vnamep
			<< " is size " << vardata.getSize()
			<< " but truncated to max size of " << max_size
			<< llendl;

		memcpy(datap, vardata.getData(), max_size);
	}
}

S32 LLTemplateMessageReader::getNumberOfBlocks(const char *blockname)
{
	// is there a message ready to go?
	if (mReceiveSize == -1)
	{
		llerrs << "No message waiting for decode 3!" << llendl;
		return -1;
	}

	if (!mCurrentRMessageData)
	{
		llerrs << "Invalid mCurrentRMessageData in getData!" << llendl;
		return -1;
	}

	char *bnamep = (char *)blockname; 

	LLMsgData::msg_blk_data_map_t::const_iterator iter = mCurrentRMessageData->mMemberBlocks.find(bnamep);
	
	if (iter == mCurrentRMessageData->mMemberBlocks.end())
	{
		return 0;
	}

	return (iter->second)->mBlockNumber;
}

S32 LLTemplateMessageReader::getSize(const char *blockname, const char *varname)
{
	// is there a message ready to go?
	if (mReceiveSize == -1)
	{	// This is a serious error - crash 
		llerrs << "No message waiting for decode 4!" << llendl;
		return LL_MESSAGE_ERROR;
	}

	if (!mCurrentRMessageData)
	{	// This is a serious error - crash
		llerrs << "Invalid mCurrentRMessageData in getData!" << llendl;
		return LL_MESSAGE_ERROR;
	}

	char *bnamep = (char *)blockname; 

	LLMsgData::msg_blk_data_map_t::const_iterator iter = mCurrentRMessageData->mMemberBlocks.find(bnamep);
	
	if (iter == mCurrentRMessageData->mMemberBlocks.end())
	{	// don't crash
		llinfos << "Block " << bnamep << " not in message "
			<< mCurrentRMessageData->mName << llendl;
		return LL_BLOCK_NOT_IN_MESSAGE;
	}

	char *vnamep = (char *)varname; 

	LLMsgBlkData* msg_data = iter->second;
	LLMsgVarData& vardata = msg_data->mMemberVarData[vnamep];
	
	if (!vardata.getName())
	{	// don't crash
		llinfos << "Variable " << varname << " not in message "
			<< mCurrentRMessageData->mName << " block " << bnamep << llendl;
		return LL_VARIABLE_NOT_IN_BLOCK;
	}

	if (mCurrentRMessageTemplate->mMemberBlocks[bnamep]->mType != MBT_SINGLE)
	{	// This is a serious error - crash
		llerrs << "Block " << bnamep << " isn't type MBT_SINGLE,"
			" use getSize with blocknum argument!" << llendl;
		return LL_MESSAGE_ERROR;
	}

	return vardata.getSize();
}

S32 LLTemplateMessageReader::getSize(const char *blockname, S32 blocknum, const char *varname)
{
	// is there a message ready to go?
	if (mReceiveSize == -1)
	{	// This is a serious error - crash
		llerrs << "No message waiting for decode 5!" << llendl;
		return LL_MESSAGE_ERROR;
	}

	if (!mCurrentRMessageData)
	{	// This is a serious error - crash
		llerrs << "Invalid mCurrentRMessageData in getData!" << llendl;
		return LL_MESSAGE_ERROR;
	}

	char *bnamep = (char *)blockname + blocknum; 
	char *vnamep = (char *)varname; 

	LLMsgData::msg_blk_data_map_t::const_iterator iter = mCurrentRMessageData->mMemberBlocks.find(bnamep);
	
	if (iter == mCurrentRMessageData->mMemberBlocks.end())
	{	// don't crash
		llinfos << "Block " << bnamep << " not in message " 
			<< mCurrentRMessageData->mName << llendl;
		return LL_BLOCK_NOT_IN_MESSAGE;
	}

	LLMsgBlkData* msg_data = iter->second;
	LLMsgVarData& vardata = msg_data->mMemberVarData[vnamep];
	
	if (!vardata.getName())
	{	// don't crash
		llinfos << "Variable " << vnamep << " not in message "
			<<  mCurrentRMessageData->mName << " block " << bnamep << llendl;
		return LL_VARIABLE_NOT_IN_BLOCK;
	}

	return vardata.getSize();
}

void LLTemplateMessageReader::getBinaryData(const char *blockname, 
											const char *varname, void *datap, 
											S32 size, S32 blocknum, 
											S32 max_size)
{
	getData(blockname, varname, datap, size, blocknum, max_size);
}

void LLTemplateMessageReader::getS8(const char *block, const char *var, 
										S8 &u, S32 blocknum)
{
	getData(block, var, &u, sizeof(S8), blocknum);
}

void LLTemplateMessageReader::getU8(const char *block, const char *var, 
										U8 &u, S32 blocknum)
{
	getData(block, var, &u, sizeof(U8), blocknum);
}

void LLTemplateMessageReader::getBOOL(const char *block, const char *var, 
										  BOOL &b, S32 blocknum )
{
	U8 value;
	getData(block, var, &value, sizeof(U8), blocknum);
	b = (BOOL) value;
}

void LLTemplateMessageReader::getS16(const char *block, const char *var, 
										 S16 &d, S32 blocknum)
{
	getData(block, var, &d, sizeof(S16), blocknum);
}

void LLTemplateMessageReader::getU16(const char *block, const char *var, 
										 U16 &d, S32 blocknum)
{
	getData(block, var, &d, sizeof(U16), blocknum);
}

void LLTemplateMessageReader::getS32(const char *block, const char *var, 
										 S32 &d, S32 blocknum)
{
	getData(block, var, &d, sizeof(S32), blocknum);
}

void LLTemplateMessageReader::getU32(const char *block, const char *var, 
									 U32 &d, S32 blocknum)
{
	getData(block, var, &d, sizeof(U32), blocknum);
}

void LLTemplateMessageReader::getU64(const char *block, const char *var, 
									 U64 &d, S32 blocknum)
{
	getData(block, var, &d, sizeof(U64), blocknum);
}

void LLTemplateMessageReader::getF32(const char *block, const char *var, 
									 F32 &d, S32 blocknum)
{
	getData(block, var, &d, sizeof(F32), blocknum);

	if( !llfinite( d ) )
	{
		llwarns << "non-finite in getF32Fast " << block << " " << var 
				<< llendl;
		d = 0;
	}
}

void LLTemplateMessageReader::getF64(const char *block, const char *var, 
									 F64 &d, S32 blocknum)
{
	getData(block, var, &d, sizeof(F64), blocknum);

	if( !llfinite( d ) )
	{
		llwarns << "non-finite in getF64Fast " << block << " " << var 
				<< llendl;
		d = 0;
	}
}

void LLTemplateMessageReader::getVector3(const char *block, const char *var, 
										 LLVector3 &v, S32 blocknum )
{
	getData(block, var, &v.mV[0], sizeof(v.mV), blocknum);

	if( !v.isFinite() )
	{
		llwarns << "non-finite in getVector3Fast " << block << " " 
				<< var << llendl;
		v.zeroVec();
	}
}

void LLTemplateMessageReader::getVector4(const char *block, const char *var, 
										 LLVector4 &v, S32 blocknum)
{
	getData(block, var, &v.mV[0], sizeof(v.mV), blocknum);

	if( !v.isFinite() )
	{
		llwarns << "non-finite in getVector4Fast " << block << " " 
				<< var << llendl;
		v.zeroVec();
	}
}

void LLTemplateMessageReader::getVector3d(const char *block, const char *var, 
										  LLVector3d &v, S32 blocknum )
{
	getData(block, var, &v.mdV[0], sizeof(v.mdV), blocknum);

	if( !v.isFinite() )
	{
		llwarns << "non-finite in getVector3dFast " << block << " " 
				<< var << llendl;
		v.zeroVec();
	}

}

void LLTemplateMessageReader::getQuat(const char *block, const char *var, 
									  LLQuaternion &q, S32 blocknum)
{
	LLVector3 vec;
	getData(block, var, &vec.mV[0], sizeof(vec.mV), blocknum);
	if( vec.isFinite() )
	{
		q.unpackFromVector3( vec );
	}
	else
	{
		llwarns << "non-finite in getQuatFast " << block << " " << var 
				<< llendl;
		q.loadIdentity();
	}
}

void LLTemplateMessageReader::getUUID(const char *block, const char *var, 
									  LLUUID &u, S32 blocknum)
{
	getData(block, var, &u.mData[0], sizeof(u.mData), blocknum);
}

inline void LLTemplateMessageReader::getIPAddr(const char *block, const char *var, U32 &u, S32 blocknum)
{
	getData(block, var, &u, sizeof(U32), blocknum);
}

inline void LLTemplateMessageReader::getIPPort(const char *block, const char *var, U16 &u, S32 blocknum)
{
	getData(block, var, &u, sizeof(U16), blocknum);
	u = ntohs(u);
}

inline void LLTemplateMessageReader::getString(const char *block, const char *var, S32 buffer_size, char *s, S32 blocknum )
{
	s[0] = '\0';
	getData(block, var, s, 0, blocknum, buffer_size);
	s[buffer_size - 1] = '\0';
}

inline void LLTemplateMessageReader::getString(const char *block, const char *var, std::string& outstr, S32 blocknum )
{
	char s[MTUBYTES];
	s[0] = '\0';
	getData(block, var, s, 0, blocknum, MTUBYTES);
	s[MTUBYTES - 1] = '\0';
	outstr = s;
}

//virtual 
S32 LLTemplateMessageReader::getMessageSize() const
{
	return mReceiveSize;
}

// Returns template for the message contained in buffer
BOOL LLTemplateMessageReader::decodeTemplate(  
		const U8* buffer, S32 buffer_size,  // inputs
		LLMessageTemplate** msg_template ) // outputs
{
	const U8* header = buffer + LL_PACKET_ID_SIZE;

	// is there a message ready to go?
	if (buffer_size <= 0)
	{
		llwarns << "No message waiting for decode!" << llendl;
		return(FALSE);
	}

	U32 num = 0;

	if (header[0] != 255)
	{
		// high frequency message
		num = header[0];
	}
	else if ((buffer_size >= ((S32) LL_MINIMUM_VALID_PACKET_SIZE + 1)) && (header[1] != 255))
	{
		// medium frequency message
		num = (255 << 8) | header[1];
	}
	else if ((buffer_size >= ((S32) LL_MINIMUM_VALID_PACKET_SIZE + 3)) && (header[1] == 255))
	{
		// low frequency message
		U16	message_id_U16 = 0;
		// I think this check busts the message system.
		// it appears that if there is a NULL in the message #, it won't copy it....
		// what was the goal?
		//if(header[2])
		memcpy(&message_id_U16, &header[2], 2);

		// dependant on endian-ness:
		//		U32	temp = (255 << 24) | (255 << 16) | header[2];

		// independant of endian-ness:
		message_id_U16 = ntohs(message_id_U16);
		num = 0xFFFF0000 | message_id_U16;
	}
	else // bogus packet received (too short)
	{
		llwarns << "Packet with unusable length received (too short): "
				<< buffer_size << llendl;
		return(FALSE);
	}

	LLMessageTemplate* temp = get_ptr_in_map(mMessageNumbers,num);
	if (temp)
	{
		*msg_template = temp;
	}
	else
	{
		llwarns << "Message #" << std::hex << num << std::dec
			<< " received but not registered!" << llendl;
		gMessageSystem->callExceptionFunc(MX_UNREGISTERED_MESSAGE);
		return(FALSE);
	}

	return(TRUE);
}

void LLTemplateMessageReader::logRanOffEndOfPacket( const LLHost& host, const S32 where, const S32 wanted )
{
	// we've run off the end of the packet!
	llwarns << "Ran off end of packet " << mCurrentRMessageTemplate->mName
//			<< " with id " << mCurrentRecvPacketID 
			<< " from " << host
			<< " trying to read " << wanted
			<< " bytes at position " << where
			<< " going past packet end at " << mReceiveSize
			<< llendl;
	if(gMessageSystem->mVerboseLog)
	{
		llinfos << "MSG: -> " << host << "\tREAD PAST END:\t"
//				<< mCurrentRecvPacketID << " "
				<< getMessageName() << llendl;
	}
	gMessageSystem->callExceptionFunc(MX_RAN_OFF_END_OF_PACKET);
}

static LLFastTimerUtil::DeclareTimer FTM_PROCESS_MESSAGES("Process Messages");

// decode a given message
BOOL LLTemplateMessageReader::decodeData(const U8* buffer, const LLHost& sender )
{
	llassert( mReceiveSize >= 0 );
	llassert( mCurrentRMessageTemplate);
	llassert( !mCurrentRMessageData );
	delete mCurrentRMessageData; // just to make sure

	// The offset tells us how may bytes to skip after the end of the
	// message name.
	U8 offset = buffer[PHL_OFFSET];
	S32 decode_pos = LL_PACKET_ID_SIZE + (S32)(mCurrentRMessageTemplate->mFrequency) + offset;

	// create base working data set
	mCurrentRMessageData = new LLMsgData(mCurrentRMessageTemplate->mName);
	
	// loop through the template building the data structure as we go
	LLMessageTemplate::message_block_map_t::const_iterator iter;
	for(iter = mCurrentRMessageTemplate->mMemberBlocks.begin();
		iter != mCurrentRMessageTemplate->mMemberBlocks.end();
		++iter)
	{
		LLMessageBlock* mbci = *iter;
		U8	repeat_number;
		S32	i;

		// how many of this block?

		if (mbci->mType == MBT_SINGLE)
		{
			// just one
			repeat_number = 1;
		}
		else if (mbci->mType == MBT_MULTIPLE)
		{
			// a known number
			repeat_number = mbci->mNumber;
		}
		else if (mbci->mType == MBT_VARIABLE)
		{
			// need to read the number from the message
			// repeat number is a single byte
			if (decode_pos >= mReceiveSize)
			{
				// commented out - hetgrid says that missing variable blocks
				// at end of message are legal
				// logRanOffEndOfPacket(sender, decode_pos, 1);

				// default to 0 repeats
				repeat_number = 0;
			}
			else
			{
				repeat_number = buffer[decode_pos];
				decode_pos++;
			}
		}
		else
		{
			llerrs << "Unknown block type" << llendl;
			return FALSE;
		}

		LLMsgBlkData* cur_data_block = NULL;

		// now loop through the block
		for (i = 0; i < repeat_number; i++)
		{
			if (i)
			{
				// build new name to prevent collisions
				// TODO: This should really change to a vector
				cur_data_block = new LLMsgBlkData(mbci->mName, repeat_number);
				cur_data_block->mName = mbci->mName + i;
			}
			else
			{
				cur_data_block = new LLMsgBlkData(mbci->mName, repeat_number);
			}

			// add the block to the message
			mCurrentRMessageData->addBlock(cur_data_block);

			// now read the variables
			for (LLMessageBlock::message_variable_map_t::const_iterator iter = 
					 mbci->mMemberVariables.begin();
				 iter != mbci->mMemberVariables.end(); iter++)
			{
				const LLMessageVariable& mvci = **iter;

				// ok, build out the variables
				// add variable block
				cur_data_block->addVariable(mvci.getName(), mvci.getType());

				// what type of variable?
				if (mvci.getType() == MVT_VARIABLE)
				{
					// variable, get the number of bytes to read from the template
					S32 data_size = mvci.getSize();
					U8 tsizeb = 0;
					U16 tsizeh = 0;
					U32 tsize = 0;

					if ((decode_pos + data_size) > mReceiveSize)
					{
						logRanOffEndOfPacket(sender, decode_pos, data_size);

						// default to 0 length variable blocks
						tsize = 0;
					}
					else
					{
						switch(data_size)
						{
						case 1:
							htonmemcpy(&tsizeb, &buffer[decode_pos], MVT_U8, 1);
							tsize = tsizeb;
							break;
						case 2:
							htonmemcpy(&tsizeh, &buffer[decode_pos], MVT_U16, 2);
							tsize = tsizeh;
							break;
						case 4:
							htonmemcpy(&tsize, &buffer[decode_pos], MVT_U32, 4);
							break;
						default:
							llerrs << "Attempting to read variable field with unknown size of " << data_size << llendl;
							break;
						}
					}
					decode_pos += data_size;

					cur_data_block->addData(mvci.getName(), &buffer[decode_pos], tsize, mvci.getType());
					decode_pos += tsize;
				}
				else
				{
					// fixed!
					// so, copy data pointer and set data size to fixed size
					if ((decode_pos + mvci.getSize()) > mReceiveSize)
					{
						logRanOffEndOfPacket(sender, decode_pos, mvci.getSize());

						// default to 0s.
						U32 size = mvci.getSize();
						std::vector<U8> data(size, 0);
						cur_data_block->addData(mvci.getName(), &(data[0]), 
												size, mvci.getType());
					}
					else
					{
						cur_data_block->addData(mvci.getName(), 
												&buffer[decode_pos], 
												mvci.getSize(), 
												mvci.getType());
					}
					decode_pos += mvci.getSize();
				}
			}
		}
	}

	if (mCurrentRMessageData->mMemberBlocks.empty()
		&& !mCurrentRMessageTemplate->mMemberBlocks.empty())
	{
		lldebugs << "Empty message '" << mCurrentRMessageTemplate->mName << "' (no blocks)" << llendl;
		return FALSE;
	}

	{
		static LLTimer decode_timer;

		if(LLMessageReader::getTimeDecodes() || gMessageSystem->getTimingCallback())
		{
			decode_timer.reset();
		}

		{
			LLFastTimer t(FTM_PROCESS_MESSAGES);
			if( !mCurrentRMessageTemplate->callHandlerFunc(gMessageSystem) )
			{
				llwarns << "Message from " << sender << " with no handler function received: " << mCurrentRMessageTemplate->mName << llendl;
			}
		}

		if(LLMessageReader::getTimeDecodes() || gMessageSystem->getTimingCallback())
		{
			F32 decode_time = decode_timer.getElapsedTimeF32();

			if (gMessageSystem->getTimingCallback())
			{
				(gMessageSystem->getTimingCallback())(mCurrentRMessageTemplate->mName,
								decode_time,
								gMessageSystem->getTimingCallbackData());
			}

			if (LLMessageReader::getTimeDecodes())
			{
				mCurrentRMessageTemplate->mDecodeTimeThisFrame += decode_time;

				mCurrentRMessageTemplate->mTotalDecoded++;
				mCurrentRMessageTemplate->mTotalDecodeTime += decode_time;

				if( mCurrentRMessageTemplate->mMaxDecodeTimePerMsg < decode_time )
				{
					mCurrentRMessageTemplate->mMaxDecodeTimePerMsg = decode_time;
				}


				if(decode_time > LLMessageReader::getTimeDecodesSpamThreshold())
				{
					lldebugs << "--------- Message " << mCurrentRMessageTemplate->mName << " decode took " << decode_time << " seconds. (" <<
						mCurrentRMessageTemplate->mMaxDecodeTimePerMsg << " max, " <<
						(mCurrentRMessageTemplate->mTotalDecodeTime / mCurrentRMessageTemplate->mTotalDecoded) << " avg)" << llendl;
				}
			}
		}
	}
	return TRUE;
}

BOOL LLTemplateMessageReader::validateMessage(const U8* buffer, 
											  S32 buffer_size, 
											  const LLHost& sender,
											  bool trusted)
{
	mReceiveSize = buffer_size;
	BOOL valid = decodeTemplate(buffer, buffer_size, &mCurrentRMessageTemplate );
	if(valid)
	{
		mCurrentRMessageTemplate->mReceiveCount++;
		//lldebugs << "MessageRecvd:"
		//						 << mCurrentRMessageTemplate->mName 
		//						 << " from " << sender << llendl;
	}

	if (valid && isBanned(trusted))
	{
		LL_WARNS("Messaging") << "LLMessageSystem::checkMessages "
			<< "received banned message "
			<< getMessageName()
			<< " from "
			<< ((trusted) ? "trusted " : "untrusted ")
			<< sender << llendl;
		valid = FALSE;
	}

	if(valid && isUdpBanned())
	{
		llwarns << "Received UDP black listed message "
				<<  getMessageName()
				<< " from " << sender << llendl;
		valid = FALSE;
	}
	return valid;
}

BOOL LLTemplateMessageReader::readMessage(const U8* buffer, 
										  const LLHost& sender)
{
	return decodeData(buffer, sender);
}

//virtual 
const char* LLTemplateMessageReader::getMessageName() const
{
	if (!mCurrentRMessageTemplate)
	{
		llwarns << "no mCurrentRMessageTemplate" << llendl;
		return "";
	}
	return mCurrentRMessageTemplate->mName;
}

//virtual 
bool LLTemplateMessageReader::isTrusted() const
{
	return mCurrentRMessageTemplate->getTrust() == MT_TRUST;
}

bool LLTemplateMessageReader::isBanned(bool trustedSource) const
{
	return mCurrentRMessageTemplate->isBanned(trustedSource);
}

bool LLTemplateMessageReader::isUdpBanned() const
{
	return mCurrentRMessageTemplate->isUdpBanned();
}

//virtual 
void LLTemplateMessageReader::copyToBuilder(LLMessageBuilder& builder) const
{
	if(NULL == mCurrentRMessageTemplate)
    {
        return;
    }
	builder.copyFromMessageData(*mCurrentRMessageData);
}
