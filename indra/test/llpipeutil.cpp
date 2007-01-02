/** 
 * @file llpipeutil.cpp
 * @date 2006-05-18
 * @brief Utility pipe fittings for injecting and extracting strings
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llpipeutil.h"

#include <stdlib.h>

#include "llbufferstream.h"
#include "llframetimer.h"
#include "llpumpio.h"
#include "llrand.h"
#include "lltimer.h"

F32 pump_loop(LLPumpIO* pump, F32 seconds)
{
	LLTimer timer;
	timer.setTimerExpirySec(seconds);
	while(!timer.hasExpired())
	{
		LLFrameTimer::updateFrameTime();			
		pump->pump();
		pump->callback();
	}
	return timer.getElapsedTimeF32();
}

//virtual 
LLIOPipe::EStatus LLPipeStringInjector::process_impl(
		const LLChannelDescriptors& channels,
		buffer_ptr_t& buffer,
		bool& eos,
		LLSD& context,
		LLPumpIO* pump)
{
	buffer->append(channels.out(), (U8*) mString.data(), mString.size());
	eos = true;
	return STATUS_DONE;
}


LLIOPipe::EStatus LLPipeStringExtractor::process_impl(
	const LLChannelDescriptors& channels,
    buffer_ptr_t& buffer,
    bool& eos,
    LLSD& context,
    LLPumpIO* pump)
{
    if(!eos) return STATUS_BREAK;
    if(!pump || !buffer) return STATUS_PRECONDITION_NOT_MET;

	LLBufferStream istr(channels, buffer.get());
	std::ostringstream ostr;
	while (istr.good())
	{
		char buf[1024];
		istr.read(buf, sizeof(buf));
		ostr.write(buf, istr.gcount());
	}
	mString = ostr.str();
	mDone = true;
	
	return STATUS_DONE;
}


// virtual
LLIOPipe::EStatus LLIOFuzz::process_impl(
	const LLChannelDescriptors& channels,
	buffer_ptr_t& buffer,
	bool& eos,
	LLSD& context,
	LLPumpIO* pump)
{
	while(mByteCount)
	{
		std::vector<U8> data;
		data.reserve(10000);
		int size = llmin(10000, mByteCount);
		std::generate_n(
			std::back_insert_iterator< std::vector<U8> >(data),
			size,
			rand);
		buffer->append(channels.out(), &data[0], size);
		mByteCount -= size;
	}
	return STATUS_OK;
}

struct random_ascii_generator
{
	random_ascii_generator() {}
	U8 operator()()
	{
		int rv = rand();
		rv %= (127 - 32);
		rv += 32;
		return rv;
	}
};

// virtual
LLIOPipe::EStatus LLIOASCIIFuzz::process_impl(
	const LLChannelDescriptors& channels,
	buffer_ptr_t& buffer,
	bool& eos,
	LLSD& context,
	LLPumpIO* pump)
{
	while(mByteCount)
	{
		std::vector<U8> data;
		data.reserve(10000);
		int size = llmin(10000, mByteCount);
		std::generate_n(
			std::back_insert_iterator< std::vector<U8> >(data),
			size,
			random_ascii_generator());
		buffer->append(channels.out(), &data[0], size);
		mByteCount -= size;
	}
	return STATUS_OK;
}

// virtual
LLIOPipe::EStatus LLIONull::process_impl(
	const LLChannelDescriptors& channels,
	buffer_ptr_t& buffer,
	bool& eos,
	LLSD& context,
	LLPumpIO* pump)
{
	return STATUS_OK;
}
