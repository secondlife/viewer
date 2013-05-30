/** 
 * @file llioutil.cpp
 * @author Phoenix
 * @date 2005-10-05
 * @brief Utility functionality for the io pipes.
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
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
#include "llioutil.h"

/**
 * LLIOFlush
 */
LLIOPipe::EStatus LLIOFlush::process_impl(
	const LLChannelDescriptors& channels,
	buffer_ptr_t& buffer,
	bool& eos,
	LLSD& context,
	LLPumpIO* pump)
{
	eos = true;
	return STATUS_OK;
}


static LLFastTimer::DeclareTimer FTM_PROCESS_SLEEP("IO Sleep");
/** 
 * @class LLIOSleep
 */
LLIOPipe::EStatus LLIOSleep::process_impl(
	const LLChannelDescriptors& channels,
	buffer_ptr_t& buffer,
	bool& eos,
	LLSD& context,
	LLPumpIO* pump)
{
	LLFastTimer t(FTM_PROCESS_SLEEP);
	if(mSeconds > 0.0)
	{
		if(pump) pump->sleepChain(mSeconds);
		mSeconds = 0.0;
		return STATUS_BREAK;
	}
	return STATUS_DONE;
}

static LLFastTimer::DeclareTimer FTM_PROCESS_ADD_CHAIN("Add Chain");
/** 
 * @class LLIOAddChain
 */
LLIOPipe::EStatus LLIOAddChain::process_impl(
	const LLChannelDescriptors& channels,
	buffer_ptr_t& buffer,
	bool& eos,
	LLSD& context,
	LLPumpIO* pump)
{
	LLFastTimer t(FTM_PROCESS_ADD_CHAIN);
	pump->addChain(mChain, mTimeout);
	return STATUS_DONE;
}

/**
 * LLChangeChannel
 */
LLChangeChannel::LLChangeChannel(S32 is, S32 becomes) :
	mIs(is),
	mBecomes(becomes)
{
}

void LLChangeChannel::operator()(LLSegment& segment)
{
	if(segment.isOnChannel(mIs))
	{
		segment.setChannel(mBecomes);
	}
}
