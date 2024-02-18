/** 
 * @file lltemplatemessagereader.cpp
 * @brief LLTemplateMessageReader class implementation.
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
#include "lltemplatemessagereader.h"

#include "llfasttimer.h"
#include "llmessagebuilder.h"
#include "llmessagetemplate.h"
#include "llmath.h"
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
		LL_ERRS() << "No message waiting for decode 2!" << LL_ENDL;
		return;
	}

	if (!mCurrentRMessageData)
	{
		LL_ERRS() << "Invalid mCurrentMessageData in getData!" << LL_ENDL;
		return;
	}

	char *bnamep = (char *)blockname + blocknum; // this works because it's just a hash.  The bnamep is never derefference
	char *vnamep = (char *)varname; 

	LLMsgData::msg_blk_data_map_t::const_iterator iter = mCurrentRMessageData->mMemberBlocks.find(bnamep);

	if (iter == mCurrentRMessageData->mMemberBlocks.end())
	{
		LL_ERRS() << "Block " << blockname << " #" << blocknum
			<< " not in message " << mCurrentRMessageData->mName << LL_ENDL;
		return;
	}

	LLMsgBlkData *msg_block_data = iter->second;
	LLMsgBlkData::msg_var_data_map_t &var_data_map = msg_block_data->mMemberVarData;

	if (var_data_map.find(vnamep) == var_data_map.end())
	{
		LL_ERRS() << "Variable "<< vnamep << " not in message "
			<< mCurrentRMessageData->mName<< " block " << bnamep << LL_ENDL;
		return;
	}

	LLMsgVarData& vardata = msg_block_data->mMemberVarData[vnamep];

	if (size && size != vardata.getSize())
	{
		LL_ERRS() << "Msg " << mCurrentRMessageData->mName 
			<< " variable " << vnamep
			<< " is size " << vardata.getSize()
			<< " but copying into buffer of size " << size
			<< LL_ENDL;
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
		LL_WARNS() << "Msg " << mCurrentRMessageData->mName 
			<< " variable " << vnamep
			<< " is size " << vardata.getSize()
			<< " but truncated to max size of " << max_size
			<< LL_ENDL;

		memcpy(datap, vardata.getData(), max_size);
	}
}

S32 LLTemplateMessageReader::getNumberOfBlocks(const char *blockname)
{
	// is there a message ready to go?
	if (mReceiveSize == -1)
	{
		LL_ERRS() << "No message waiting for decode 3!" << LL_ENDL;
		return -1;
	}

	if (!mCurrentRMessageData)
	{
		LL_ERRS() << "Invalid mCurrentRMessageData in getData!" << LL_ENDL;
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
		LL_ERRS() << "No message waiting for decode 4!" << LL_ENDL;
		return LL_MESSAGE_ERROR;
	}

	if (!mCurrentRMessageData)
	{	// This is a serious error - crash
		LL_ERRS() << "Invalid mCurrentRMessageData in getData!" << LL_ENDL;
		return LL_MESSAGE_ERROR;
	}

	char *bnamep = (char *)blockname; 

	LLMsgData::msg_blk_data_map_t::const_iterator iter = mCurrentRMessageData->mMemberBlocks.find(bnamep);
	
	if (iter == mCurrentRMessageData->mMemberBlocks.end())
	{	// don't crash
		LL_INFOS() << "Block " << bnamep << " not in message "
			<< mCurrentRMessageData->mName << LL_ENDL;
		return LL_BLOCK_NOT_IN_MESSAGE;
	}

	char *vnamep = (char *)varname; 

	LLMsgBlkData* msg_data = iter->second;
	LLMsgVarData& vardata = msg_data->mMemberVarData[vnamep];
	
	if (!vardata.getName())
	{	// don't crash
		LL_INFOS() << "Variable " << varname << " not in message "
			<< mCurrentRMessageData->mName << " block " << bnamep << LL_ENDL;
		return LL_VARIABLE_NOT_IN_BLOCK;
	}

	if (mCurrentRMessageTemplate->mMemberBlocks[bnamep]->mType != MBT_SINGLE)
	{	// This is a serious error - crash
		LL_ERRS() << "Block " << bnamep << " isn't type MBT_SINGLE,"
			" use getSize with blocknum argument!" << LL_ENDL;
		return LL_MESSAGE_ERROR;
	}

	return vardata.getSize();
}

S32 LLTemplateMessageReader::getSize(const char *blockname, S32 blocknum, const char *varname)
{
	// is there a message ready to go?
	if (mReceiveSize == -1)
	{	// This is a serious error - crash
		LL_ERRS() << "No message waiting for decode 5!" << LL_ENDL;
		return LL_MESSAGE_ERROR;
	}

	if (!mCurrentRMessageData)
	{	// This is a serious error - crash
		LL_ERRS() << "Invalid mCurrentRMessageData in getData!" << LL_ENDL;
		return LL_MESSAGE_ERROR;
	}

	char *bnamep = (char *)blockname + blocknum; 
	char *vnamep = (char *)varname; 

	LLMsgData::msg_blk_data_map_t::const_iterator iter = mCurrentRMessageData->mMemberBlocks.find(bnamep);
	
	if (iter == mCurrentRMessageData->mMemberBlocks.end())
	{	// don't crash
		LL_INFOS() << "Block " << bnamep << " not in message " 
			<< mCurrentRMessageData->mName << LL_ENDL;
		return LL_BLOCK_NOT_IN_MESSAGE;
	}

	LLMsgBlkData* msg_data = iter->second;
	LLMsgVarData& vardata = msg_data->mMemberVarData[vnamep];
	
	if (!vardata.getName())
	{	// don't crash
		LL_INFOS() << "Variable " << vnamep << " not in message "
			<<  mCurrentRMessageData->mName << " block " << bnamep << LL_ENDL;
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
										  bool &b, S32 blocknum )
{
	U8 value;
	getData(block, var, &value, sizeof(U8), blocknum);
	b = (bool)value;
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
		LL_WARNS() << "non-finite in getF32Fast " << block << " " << var 
				<< LL_ENDL;
		d = 0;
	}
}

void LLTemplateMessageReader::getF64(const char *block, const char *var, 
									 F64 &d, S32 blocknum)
{
	getData(block, var, &d, sizeof(F64), blocknum);

	if( !llfinite( d ) )
	{
		LL_WARNS() << "non-finite in getF64Fast " << block << " " << var 
				<< LL_ENDL;
		d = 0;
	}
}

void LLTemplateMessageReader::getVector3(const char *block, const char *var, 
										 LLVector3 &v, S32 blocknum )
{
	getData(block, var, &v.mV[0], sizeof(v.mV), blocknum);

	if( !v.isFinite() )
	{
		LL_WARNS() << "non-finite in getVector3Fast " << block << " " 
				<< var << LL_ENDL;
		v.zeroVec();
	}
}

void LLTemplateMessageReader::getVector4(const char *block, const char *var, 
										 LLVector4 &v, S32 blocknum)
{
	getData(block, var, &v.mV[0], sizeof(v.mV), blocknum);

	if( !v.isFinite() )
	{
		LL_WARNS() << "non-finite in getVector4Fast " << block << " " 
				<< var << LL_ENDL;
		v.zeroVec();
	}
}

void LLTemplateMessageReader::getVector3d(const char *block, const char *var, 
										  LLVector3d &v, S32 blocknum )
{
	getData(block, var, &v.mdV[0], sizeof(v.mdV), blocknum);

	if( !v.isFinite() )
	{
		LL_WARNS() << "non-finite in getVector3dFast " << block << " " 
				<< var << LL_ENDL;
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
		LL_WARNS() << "non-finite in getQuatFast " << block << " " << var 
				<< LL_ENDL;
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
	char s[MTUBYTES + 1]= {0}; // every element is initialized with 0
	getData(block, var, s, 0, blocknum, MTUBYTES);
	s[MTUBYTES] = '\0';
	outstr = s;
}

//virtual 
S32 LLTemplateMessageReader::getMessageSize() const
{
	return mReceiveSize;
}

// Returns template for the message contained in buffer
bool LLTemplateMessageReader::decodeTemplate(
		const U8* buffer, S32 buffer_size,  // inputs
		LLMessageTemplate** msg_template ) // outputs
{
	const U8* header = buffer + LL_PACKET_ID_SIZE;

	// is there a message ready to go?
	if (buffer_size <= 0)
	{
		LL_WARNS() << "No message waiting for decode!" << LL_ENDL;
		return(false);
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
		LL_WARNS() << "Packet with unusable length received (too short): "
				<< buffer_size << LL_ENDL;
		return(false);
	}

	LLMessageTemplate* temp = get_ptr_in_map(mMessageNumbers,num);
	if (temp)
	{
		*msg_template = temp;
	}
	else
	{
        // MAINT-7482 - make viewer more tolerant of unknown messages.
		LL_WARNS_ONCE() << "Message #" << std::hex << num << std::dec
                        << " received but not registered!" << LL_ENDL;
		//gMessageSystem->callExceptionFunc(MX_UNREGISTERED_MESSAGE);
		return(false);
	}

	return(true);
}

void LLTemplateMessageReader::logRanOffEndOfPacket( const LLHost& host, const S32 where, const S32 wanted )
{
	// we've run off the end of the packet!
	LL_WARNS() << "Ran off end of packet " << mCurrentRMessageTemplate->mName
//			<< " with id " << mCurrentRecvPacketID 
			<< " from " << host
			<< " trying to read " << wanted
			<< " bytes at position " << where
			<< " going past packet end at " << mReceiveSize
			<< LL_ENDL;
	if(gMessageSystem->mVerboseLog)
	{
		LL_INFOS() << "MSG: -> " << host << "\tREAD PAST END:\t"
//				<< mCurrentRecvPacketID << " "
				<< getMessageName() << LL_ENDL;
	}
	gMessageSystem->callExceptionFunc(MX_RAN_OFF_END_OF_PACKET);
}

static LLTrace::BlockTimerStatHandle FTM_PROCESS_MESSAGES("Process Messages");

// decode a given message
bool LLTemplateMessageReader::decodeData(const U8* buffer, const LLHost& sender )
{
    LL_RECORD_BLOCK_TIME(FTM_PROCESS_MESSAGES);

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
			LL_ERRS() << "Unknown block type" << LL_ENDL;
			return false;
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
							htolememcpy(&tsizeb, &buffer[decode_pos], MVT_U8, 1);
							tsize = tsizeb;
							break;
						case 2:
							htolememcpy(&tsizeh, &buffer[decode_pos], MVT_U16, 2);
							tsize = tsizeh;
							break;
						case 4:
							htolememcpy(&tsize, &buffer[decode_pos], MVT_U32, 4);
							break;
						default:
							LL_ERRS() << "Attempting to read variable field with unknown size of " << data_size << LL_ENDL;
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
		LL_DEBUGS() << "Empty message '" << mCurrentRMessageTemplate->mName << "' (no blocks)" << LL_ENDL;
		return false;
	}

	{
		static LLTimer decode_timer;

		if(LLMessageReader::getTimeDecodes() || gMessageSystem->getTimingCallback())
		{
			decode_timer.reset();
		}

		if( !mCurrentRMessageTemplate->callHandlerFunc(gMessageSystem) )
		{
			LL_WARNS() << "Message from " << sender << " with no handler function received: " << mCurrentRMessageTemplate->mName << LL_ENDL;
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
					LL_DEBUGS() << "--------- Message " << mCurrentRMessageTemplate->mName << " decode took " << decode_time << " seconds. (" <<
						mCurrentRMessageTemplate->mMaxDecodeTimePerMsg << " max, " <<
						(mCurrentRMessageTemplate->mTotalDecodeTime / mCurrentRMessageTemplate->mTotalDecoded) << " avg)" << LL_ENDL;
				}
			}
		}
	}
	return true;
}

bool LLTemplateMessageReader::validateMessage(const U8* buffer,
											  S32 buffer_size,
											  const LLHost& sender,
											  bool trusted)
{
	mReceiveSize = buffer_size;
	bool valid = decodeTemplate(buffer, buffer_size, &mCurrentRMessageTemplate );
	if(valid)
	{
		mCurrentRMessageTemplate->mReceiveCount++;
		//LL_DEBUGS() << "MessageRecvd:"
		//						 << mCurrentRMessageTemplate->mName 
		//						 << " from " << sender << LL_ENDL;
	}

	if (valid && isBanned(trusted))
	{
		LL_WARNS("Messaging") << "LLMessageSystem::checkMessages "
			<< "received banned message "
			<< getMessageName()
			<< " from "
			<< ((trusted) ? "trusted " : "untrusted ")
			<< sender << LL_ENDL;
		valid = false;
	}

	if(valid && isUdpBanned())
	{
		LL_WARNS() << "Received UDP black listed message "
				<<  getMessageName()
				<< " from " << sender << LL_ENDL;
		valid = false;
	}
	return valid;
}

bool LLTemplateMessageReader::readMessage(const U8* buffer,
										  const LLHost& sender)
{
	return decodeData(buffer, sender);
}

//virtual 
const char* LLTemplateMessageReader::getMessageName() const
{
	if (!mCurrentRMessageTemplate)
	{
		// no message currently being read
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
