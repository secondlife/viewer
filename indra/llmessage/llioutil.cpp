/** 
 * @file llioutil.cpp
 * @author Phoenix
 * @date 2005-10-05
 * @brief Utility functionality for the io pipes.
 *
 * Copyright (c) 2005-$CurrentYear$, Linden Research, Inc.
 * $License$
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
