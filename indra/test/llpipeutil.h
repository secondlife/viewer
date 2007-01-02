/** 
 * @file llpipeutil.h
 * @date 2006-05-18
 * @brief Utility pipe fittings for injecting and extracting strings
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
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

#endif // LL_LLPIPEUTIL_H
