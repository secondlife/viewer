/** 
 * @file llpacketack.cpp
 * @author Phoenix
 * @date 2007-05-09
 * @brief Implementation of the LLReliablePacket.
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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
