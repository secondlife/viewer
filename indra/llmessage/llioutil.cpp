/** 
 * @file llioutil.cpp
 * @author Phoenix
 * @date 2005-10-05
 * @brief Utility functionality for the io pipes.
 *
 * $LicenseInfo:firstyear=2005&license=viewergpl$
 * 
 * Copyright (c) 2005-2009, Linden Research, Inc.
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
	if(mSeconds > 0.0)
	{
		if(pump) pump->sleepChain(mSeconds);
		mSeconds = 0.0;
		return STATUS_BREAK;
	}
	return STATUS_DONE;
}

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
