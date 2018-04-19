/** 
 * @file llxfer.cpp
 * @brief implementation of LLXfer class for a single xfer.
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

#include "linden_common.h"

#include "llxfer.h"
#include "lluuid.h"
#include "llerror.h"
#include "llmath.h"
#include "u64.h"

//number of bytes sent in each message
const U32 LL_XFER_CHUNK_SIZE = 1000;

const U32 LLXfer::XFER_FILE = 1;
const U32 LLXfer::XFER_VFILE = 2;
const U32 LLXfer::XFER_MEM = 3;

///////////////////////////////////////////////////////////

LLXfer::LLXfer (S32 chunk_size)
{
	init(chunk_size);
}

///////////////////////////////////////////////////////////

LLXfer::~LLXfer ()
{
	cleanup();
}

///////////////////////////////////////////////////////////

void LLXfer::init (S32 chunk_size)
{
	mID = 0;

	mPacketNum = -1; // there's a preincrement before sending the zeroth packet
	mXferSize = 0;

	mStatus = e_LL_XFER_UNINITIALIZED;
	mNext = NULL;
	mWaitingForACK = FALSE;
	
	mCallback = NULL;
	mCallbackDataHandle = NULL;
	mCallbackResult = 0;

	mBufferContainsEOF = FALSE;
	mBuffer = NULL;
	mBufferLength = 0;
	mBufferStartOffset = 0;

	mRetries = 0;

	if (chunk_size < 1)
	{
		chunk_size = LL_XFER_CHUNK_SIZE;
	}
	mChunkSize = chunk_size;
}
	
///////////////////////////////////////////////////////////

void LLXfer::cleanup ()
{
	if (mBuffer)
	{
		delete[] mBuffer;
		mBuffer = NULL;
	}
}

///////////////////////////////////////////////////////////

S32 LLXfer::startSend (U64 xfer_id, const LLHost &remote_host)
{
	LL_WARNS("Xfer") << "unexpected call to base class LLXfer::startSend for " << getFileName() << LL_ENDL;
	return (-1);
}

///////////////////////////////////////////////////////////

void LLXfer::closeFileHandle()
{
	LL_WARNS("Xfer") << "unexpected call to base class LLXfer::closeFileHandle for " << getFileName() << LL_ENDL;
}

///////////////////////////////////////////////////////////

S32 LLXfer::reopenFileHandle()
{
	LL_WARNS("Xfer") << "unexpected call to base class LLXfer::reopenFileHandle for " << getFileName() << LL_ENDL;
	return (-1);
}

///////////////////////////////////////////////////////////

void LLXfer::setXferSize (S32 xfer_size)
{	
	mXferSize = xfer_size;
//	cout << "starting transfer of size: " << xfer_size << endl;
}

///////////////////////////////////////////////////////////

S32 LLXfer::startDownload()
{
	LL_WARNS() << "undifferentiated LLXfer::startDownload for " << getFileName()
			<< LL_ENDL;
	return (-1);
}

///////////////////////////////////////////////////////////

S32 LLXfer::receiveData (char *datap, S32 data_size)
{
	S32 retval = 0;

	if (((S32) mBufferLength + data_size) > getMaxBufferSize())
	{	// Write existing data to disk if it's larger than the buffer size
		retval = flush();
	}

	if (!retval)
	{
		if (datap != NULL)
		{	// Append new data to mBuffer
			memcpy(&mBuffer[mBufferLength],datap,data_size);	/*Flawfinder: ignore*/
			mBufferLength += data_size;
		}
		else
		{
			LL_ERRS() << "NULL data passed in receiveData" << LL_ENDL;
		}
	}

	return (retval);
}

///////////////////////////////////////////////////////////

S32 LLXfer::flush()
{
	// only files have somewhere to flush to
	// if we get called with a flush it means we've blown past our
	// allocated buffer size

	return (-1);
}


///////////////////////////////////////////////////////////

S32 LLXfer::suck(S32 start_position)
{
	LL_WARNS() << "Attempted to send a packet outside the buffer bounds in LLXfer::suck()" << LL_ENDL;
	return (-1);
}

///////////////////////////////////////////////////////////

void LLXfer::sendPacket(S32 packet_num)
{
	char fdata_buf[LL_XFER_LARGE_PAYLOAD+4];		/* Flawfinder: ignore */
	S32 fdata_size = mChunkSize;
	BOOL last_packet = FALSE;
	S32 num_copy = 0;

	// if the desired packet is not in our current buffered excerpt from the file. . . 
	if (((U32)packet_num*fdata_size < mBufferStartOffset) 
		|| ((U32)llmin((U32)mXferSize,(U32)((U32)(packet_num+1)*fdata_size)) > mBufferStartOffset + mBufferLength))
	
	{
		if (suck(packet_num*fdata_size))  // returns non-zero on failure
		{
			abort(LL_ERR_EOF);
			return;
		}	
	}
		
	S32 desired_read_position = 0;
		
	desired_read_position = packet_num * fdata_size - mBufferStartOffset;
	
	fdata_size = llmin((S32)mBufferLength-desired_read_position, mChunkSize);

	if (fdata_size < 0)
	{
		LL_WARNS() << "negative data size in xfer send, aborting" << LL_ENDL;
		abort(LL_ERR_EOF);
		return;
	}

	if (((U32)(desired_read_position + fdata_size) >= (U32)mBufferLength) && (mBufferContainsEOF))
	{
		last_packet = TRUE;
	}
		
	if (packet_num)
	{ 
		num_copy = llmin(fdata_size, (S32)sizeof(fdata_buf));
		num_copy = llmin(num_copy, (S32)(mBufferLength - desired_read_position));
		if (num_copy > 0)
		{
			memcpy(fdata_buf,&mBuffer[desired_read_position],num_copy);	/*Flawfinder: ignore*/
		}
	}
	else  
	{
		// if we're the first packet, encode size as an additional S32
		// at start of data.
		num_copy = llmin(fdata_size, (S32)(sizeof(fdata_buf)-sizeof(S32)));
		num_copy = llmin(
			num_copy,
			(S32)(mBufferLength - desired_read_position));
		if (num_copy > 0)
		{
			memcpy(	/*Flawfinder: ignore*/
				fdata_buf + sizeof(S32),
				&mBuffer[desired_read_position],
				num_copy);
		}
		fdata_size += sizeof(S32);
		htonmemcpy(fdata_buf,&mXferSize, MVT_S32, sizeof(S32));
	}

	S32 encoded_packetnum = encodePacketNum(packet_num,last_packet);
		
	if (fdata_size)
	{
		// send the packet 
		gMessageSystem->newMessageFast(_PREHASH_SendXferPacket);
		gMessageSystem->nextBlockFast(_PREHASH_XferID);
		
		gMessageSystem->addU64Fast(_PREHASH_ID, mID);
		gMessageSystem->addU32Fast(_PREHASH_Packet, encoded_packetnum);
		
		gMessageSystem->nextBlockFast(_PREHASH_DataPacket);
		gMessageSystem->addBinaryDataFast(_PREHASH_Data, &fdata_buf,fdata_size);
			
		S32 sent_something = gMessageSystem->sendMessage(mRemoteHost);
		if (sent_something == 0)
		{
			abort(LL_ERR_CIRCUIT_GONE);
			return;
		}

		ACKTimer.reset();
		mWaitingForACK = TRUE;
	}
	if (last_packet)
	{
		mStatus = e_LL_XFER_COMPLETE;	
	}
	else
	{
		mStatus = e_LL_XFER_IN_PROGRESS;
	}
}

///////////////////////////////////////////////////////////

void LLXfer::sendNextPacket()
{
	mRetries = 0;
	sendPacket(++mPacketNum);
}

///////////////////////////////////////////////////////////

void LLXfer::resendLastPacket()
{
	mRetries++;
	sendPacket(mPacketNum);
}

///////////////////////////////////////////////////////////

S32 LLXfer::processEOF()
{
	S32 retval = 0;

	mStatus = e_LL_XFER_COMPLETE;

	if (LL_ERR_NOERR == mCallbackResult)
	{
		LL_INFOS() << "xfer from " << mRemoteHost << " complete: " << getFileName()
				<< LL_ENDL;
	}
	else
	{
		LL_INFOS() << "xfer from " << mRemoteHost << " failed or aborted, code "
				<< mCallbackResult << ": " << getFileName() << LL_ENDL;
	}

	if (mCallback)
	{
		mCallback(mCallbackDataHandle,mCallbackResult,LL_EXSTAT_NONE);
	}

	return(retval);
}

///////////////////////////////////////////////////////////

S32 LLXfer::encodePacketNum(S32 packet_num, BOOL is_EOF)
{
	if (is_EOF)
	{
		packet_num |= 0x80000000;
	}
	return packet_num;
}

///////////////////////////////////////////////////////////

void LLXfer::abort (S32 result_code)
{
	mCallbackResult = result_code;

	LL_INFOS() << "Aborting xfer from " << mRemoteHost << " named " << getFileName()
			<< " - error: " << result_code << LL_ENDL;

	if (result_code != LL_ERR_CIRCUIT_GONE)
	{
		gMessageSystem->newMessageFast(_PREHASH_AbortXfer);
		gMessageSystem->nextBlockFast(_PREHASH_XferID);
		gMessageSystem->addU64Fast(_PREHASH_ID, mID);
		gMessageSystem->addS32Fast(_PREHASH_Result, result_code);

		gMessageSystem->sendMessage(mRemoteHost);
	}

	mStatus = e_LL_XFER_ABORTED;
}


///////////////////////////////////////////////////////////

std::string LLXfer::getFileName() 
{
	return U64_to_str(mID);
}

///////////////////////////////////////////////////////////

U32 LLXfer::getXferTypeTag()
{
	return 0;
}

///////////////////////////////////////////////////////////

S32 LLXfer::getMaxBufferSize ()
{
	return(mXferSize);
}


std::ostream& operator<< (std::ostream& os, LLXfer &hh)
{
	os << hh.getFileName() ;
	return os;
}









