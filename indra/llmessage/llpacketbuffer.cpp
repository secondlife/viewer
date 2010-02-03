/** 
 * @file llpacketbuffer.cpp
 * @brief implementation of LLPacketBuffer class for a packet.
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

#include "linden_common.h"

#include "llpacketbuffer.h"

#include "net.h"
#include "timing.h"
#include "llhost.h"

///////////////////////////////////////////////////////////

LLPacketBuffer::LLPacketBuffer(const LLHost &host, const char *datap, const S32 size) : mHost(host)
{
	mSize = 0;
	mData[0] = '!';

	if (size > NET_BUFFER_SIZE)
	{
		llerrs << "Sending packet > " << NET_BUFFER_SIZE << " of size " << size << llendl;
	}
	else
	{
		if (datap != NULL)
		{
			memcpy(mData, datap, size);
			mSize = size;
		}
	}
	
}

LLPacketBuffer::LLPacketBuffer (S32 hSocket)
{
	init(hSocket);
}

///////////////////////////////////////////////////////////

LLPacketBuffer::~LLPacketBuffer ()
{
}

///////////////////////////////////////////////////////////

void LLPacketBuffer::init (S32 hSocket)
{
	mSize = receive_packet(hSocket, mData);
	mHost = ::get_sender();
	mReceivingIF = ::get_receiving_interface();
}

