/** 
 * @file llpipeutil.h
 * @date 2006-05-18
 * @brief Utility pipe fittings for injecting and extracting strings
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2007, Linden Research, Inc.
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

#ifndef LL_LLPIPEUTIL_H
#define LL_LLPIPEUTIL_H

#include "lliopipe.h"


/**
 * @brief Simple function which pumps for the specified time.
 */
F32 pump_loop(LLPumpIO* pump, F32 seconds);

/**
 * @brief Simple class which writes a string and then marks the stream
 * as done.
 */
class LLPipeStringInjector : public LLIOPipe
{
public:
	LLPipeStringInjector(const std::string& string)
		: mString(string)
		{ }
	  
protected:
    virtual EStatus process_impl(
		const LLChannelDescriptors& channels,
		buffer_ptr_t& buffer,
		bool& eos,
		LLSD& context,
		LLPumpIO* pump);

private:
	std::string mString;
};


class LLPipeStringExtractor : public LLIOPipe
{
public:
	LLPipeStringExtractor() : mDone(false) { }
	
	bool done() { return mDone; }
	std::string string() { return mString; }
	
protected:
	// LLIOPipe API implementation.
	virtual EStatus process_impl(
        const LLChannelDescriptors& channels,
        LLIOPipe::buffer_ptr_t& buffer,
        bool& eos,
        LLSD& context,
        LLPumpIO* pump);

private:
	bool mDone;
	std::string mString;
};

/**
 * @brief Generate a specified number of bytes of random data
 */
class LLIOFuzz : public LLIOPipe
{
public:
	LLIOFuzz(int byte_count) : mByteCount(byte_count) {}
	  
protected:
    virtual EStatus process_impl(
		const LLChannelDescriptors& channels,
		buffer_ptr_t& buffer,
		bool& eos,
		LLSD& context,
		LLPumpIO* pump);

private:
	int mByteCount;
};

/**
 * @brief Generate some ascii fuz
 */
class LLIOASCIIFuzz : public LLIOPipe
{
public:
	LLIOASCIIFuzz(int byte_count) : mByteCount(byte_count) {}
	  
protected:
    virtual EStatus process_impl(
		const LLChannelDescriptors& channels,
		buffer_ptr_t& buffer,
		bool& eos,
		LLSD& context,
		LLPumpIO* pump);

private:
	int mByteCount;
};


/**
 * @brief Pipe that does nothing except return STATUS_OK
 */
class LLIONull : public LLIOPipe
{
public:
	LLIONull() {}
	  
protected:
    virtual EStatus process_impl(
		const LLChannelDescriptors& channels,
		buffer_ptr_t& buffer,
		bool& eos,
		LLSD& context,
		LLPumpIO* pump);
};

/**
 * @brief Pipe that sleeps, and then responds later.
 */
class LLIOSleeper : public LLIOPipe
{
public:
	LLIOSleeper() : mRespond(false) {}
	  
protected:
    virtual EStatus process_impl(
		const LLChannelDescriptors& channels,
		buffer_ptr_t& buffer,
		bool& eos,
		LLSD& context,
		LLPumpIO* pump);
private:
	bool mRespond;

};

#endif // LL_LLPIPEUTIL_H
