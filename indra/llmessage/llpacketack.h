/** 
 * @file llpacketack.h
 * @brief Reliable UDP helpers for the message system.
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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

