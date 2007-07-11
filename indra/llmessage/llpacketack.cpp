/** 
 * @file llpacketack.cpp
 * @author Phoenix
 * @date 2007-05-09
 * @brief Implementation of the LLReliablePacket.
 *
 * Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "linden_common.h"
#include "llpacketack.h"

#if !LL_WINDOWS
#include <netinet/in.h>
#else
#include "winsock2.h"
#endif

#include "message.h"

LLReliablePacket::LLReliablePacket(
	S32 socket,
	U8* buf_ptr,
	S32 buf_len,
	LLReliablePacketParams* params) :
	mBuffer(NULL),
	mBufferLength(0)
{
	if (params)
	{
		mHost = params->mHost;
		mRetries = params->mRetries;
		mPingBasedRetry = params->mPingBasedRetry;
		mTimeout = params->mTimeout;
		mCallback = params->mCallback;
		mCallbackData = params->mCallbackData;
		mMessageName = params->mMessageName;
	}
	else
	{
		mRetries = 0;
		mPingBasedRetry = TRUE;
		mTimeout = 0.f;
		mCallback = NULL;
		mCallbackData = NULL;
		mMessageName = NULL;
	}

	mExpirationTime = (F64)((S64)totalTime())/1000000.0 + mTimeout;
	mPacketID = ntohl(*((U32*)(&buf_ptr[PHL_PACKET_ID])));

	mSocket = socket;
	if (mRetries)
	{
		mBuffer = new U8[buf_len];
		if (mBuffer != NULL)
		{
			memcpy(mBuffer,buf_ptr,buf_len);	/*Flawfinder: ignore*/
			mBufferLength = buf_len;
		}
			
	}
}
