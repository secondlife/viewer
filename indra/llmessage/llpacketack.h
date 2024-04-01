/** 
 * @file llpacketack.h
 * @brief Reliable UDP helpers for the message system.
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

#ifndef LL_LLPACKETACK_H
#define LL_LLPACKETACK_H

#include "llhost.h"
#include "llunits.h"

class LLReliablePacketParams
{
public:
	LLHost mHost;
	S32 mRetries;
	bool mPingBasedRetry;
	F32Seconds mTimeout;
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
		mPingBasedRetry = true;
		mTimeout = F32Seconds(0.f);
		mCallback = NULL;
		mCallbackData = NULL;
		mMessageName = NULL;
	};

	void set(
		const LLHost& host,
		S32 retries,
		bool ping_based_retry,
		F32Seconds timeout, 
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
	bool mPingBasedRetry;
	F32Seconds mTimeout;
	void (*mCallback)(void**,S32);
	void** mCallbackData;
	char* mMessageName;

	U8* mBuffer;
	S32 mBufferLength;

	TPACKETID mPacketID;

	F64Seconds mExpirationTime;
};

#endif

