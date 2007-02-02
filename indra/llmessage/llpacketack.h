/** 
 * @file llpacketack.h
 * @brief Reliable UDP helpers for the message system.
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLPACKETACK_H
#define LL_LLPACKETACK_H

#include <cstring>
#include <stdio.h>

#include "llerror.h"
#include "lltimer.h"
#include "llhost.h"

//class LLPacketAck
//{
//public:
//	LLHost mHost;
//	TPACKETID          mPacketID;
//public:
//	LLPacketAck(const LLHost &host, TPACKETID packet_id)
//		{
//			mHost = host;
//			mPacketID = packet_id;
//		};
//	~LLPacketAck(){};
//};

class LLReliablePacketParams
{
public:
	LLHost	mHost;
	S32				mRetries;
	BOOL			mPingBasedRetry;
	F32				mTimeout;
	void			(*mCallback)(void **,S32);
	void			**mCallbackData;
	char			*mMessageName;

public:
	LLReliablePacketParams()
	{
		mRetries = 0;
		mPingBasedRetry = TRUE;
		mTimeout = 0.f;
		mCallback = NULL;
		mCallbackData = NULL;
		mMessageName = NULL;
	};

	~LLReliablePacketParams() { };

	void set (	const LLHost &host, S32 retries, BOOL ping_based_retry,
				F32 timeout, 
				void (*callback)(void **,S32), void **callback_data, char *name )
	{
		mHost = host;
		mRetries = retries;
		mPingBasedRetry = ping_based_retry;
		mTimeout = timeout;
		mCallback = callback;
		mCallbackData = callback_data;
		mMessageName = name;
	};
};

class LLReliablePacket
{
public:
	LLReliablePacket(S32 socket, U8 *buf_ptr, S32 buf_len, LLReliablePacketParams *params) :
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
		mPacketID = buf_ptr[1] + ((buf_ptr[0] & 0x0f ) * 256);
		if (sizeof(TPACKETID) == 4)
		{
			mPacketID *= 256;
			mPacketID += buf_ptr[2];
			mPacketID *= 256;
			mPacketID += buf_ptr[3];
		}

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
	};
	~LLReliablePacket(){ delete [] mBuffer; };

	friend class LLCircuitData;
protected:
	S32				mSocket;
	LLHost	        mHost;
	S32				mRetries;
	BOOL			mPingBasedRetry;
	F32				mTimeout;
	void			(*mCallback)(void **,S32);
	void			**mCallbackData;
	char			*mMessageName;
	
	U8				*mBuffer;
	S32				mBufferLength;

	TPACKETID				mPacketID;

	F64				mExpirationTime;
	
};

#endif

