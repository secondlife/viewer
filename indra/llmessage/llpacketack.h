/** 
 * @file llpacketack.h
 * @brief Reliable UDP helpers for the message system.
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLPACKETACK_H
#define LL_LLPACKETACK_H

#include "llhost.h"

class LLReliablePacketParams
{
public:
	LLHost mHost;
	S32 mRetries;
	BOOL mPingBasedRetry;
	F32 mTimeout;
	void (*mCallback)(void **,S32);
	void** mCallbackData;
	char* mMessageName;

public:
	LLReliablePacketParams()
	{
		clear();
	};

	~LLReliablePacketParams() { };

	void clear()
	{
		mHost.invalidate();
		mRetries = 0;
		mPingBasedRetry = TRUE;
		mTimeout = 0.f;
		mCallback = NULL;
		mCallbackData = NULL;
		mMessageName = NULL;
	};

	void set(
		const LLHost& host,
		S32 retries,
		BOOL ping_based_retry,
		F32 timeout, 
		void (*callback)(void**,S32),
		void** callback_data, char* name)
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
	LLReliablePacket(
		S32 socket,
		U8* buf_ptr,
		S32 buf_len,
		LLReliablePacketParams* params);
	~LLReliablePacket()
	{ 
		mCallback = NULL;
		delete [] mBuffer;
		mBuffer = NULL;
	};

	friend class LLCircuitData;
protected:
	S32 mSocket;
	LLHost mHost;
	S32 mRetries;
	BOOL mPingBasedRetry;
	F32 mTimeout;
	void (*mCallback)(void**,S32);
	void** mCallbackData;
	char* mMessageName;

	U8* mBuffer;
	S32 mBufferLength;

	TPACKETID mPacketID;

	F64 mExpirationTime;
};

#endif

