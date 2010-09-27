/** 
 * @file llpipeutil.cpp
 * @date 2006-05-18
 * @brief Utility pipe fittings for injecting and extracting strings
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2009, Linden Research, Inc.
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
#include "llpipeutil.h"

#include <stdlib.h>
#include <sstream>

#include "llbufferstream.h"
#include "lldefs.h"
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
		char buf[1024];		/* Flawfinder: ignore */
		istr.read(buf, sizeof(buf));	/* Flawfinder: ignore */
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

// virtual
LLIOPipe::EStatus LLIOSleeper::process_impl(
	const LLChannelDescriptors& channels,
	buffer_ptr_t& buffer,
	bool& eos,
	LLSD& context,
	LLPumpIO* pump)
{
	if(!mRespond)
	{
		lldebugs << "LLIOSleeper::process_impl() sleeping." << llendl;
		mRespond = true;
		static const F64 SLEEP_TIME = 2.0;
		pump->sleepChain(SLEEP_TIME);
		return STATUS_BREAK;
	}
	lldebugs << "LLIOSleeper::process_impl() responding." << llendl;
	LLBufferStream ostr(channels, buffer.get());
	ostr << "huh? sorry, I was sleeping." << std::endl;
	return STATUS_DONE;
}
