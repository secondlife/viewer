/** 
 * @file llmemorystream.h
 * @author Phoenix
 * @date 2005-06-03
 * @brief Implementation of a simple fixed memory stream
 *
 * Copyright (c) 2005-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLMEMORYSTREAM_H
#define LL_LLMEMORYSTREAM_H

/** 
 * This is a simple but effective optimization when you want to treat
 * a chunk of memory as an istream. I wrote this to avoid turing a
 * buffer into a string, and then throwing the string into an
 * iostringstream just to parse it into another datatype, eg, LLSD.
 */

#include <iostream>

/** 
 * @class LLMemoryStreamBuf
 * @brief This implements a wrapper around a piece of memory for istreams
 *
 * The memory passed in is NOT owned by an instance. The caller must
 * be careful to always pass in a valid memory location that exists
 * for at least as long as this streambuf.
 */
class LLMemoryStreamBuf : public std::streambuf
{
public:
	LLMemoryStreamBuf(const U8* start, S32 length);
	~LLMemoryStreamBuf();

	void reset(const U8* start, S32 length);

protected:
	int underflow();
	//std::streamsize xsgetn(char* dest, std::streamsize n);
};


/** 
 * @class LLMemoryStream
 * @brief This implements a wrapper around a piece of memory for istreams
 *
 * The memory passed in is NOT owned by an instance. The caller must
 * be careful to always pass in a valid memory location that exists
 * for at least as long as this streambuf.
 */
class LLMemoryStream : public std::istream
{
public:
	LLMemoryStream(const U8* start, S32 length);
	~LLMemoryStream();

protected:
	LLMemoryStreamBuf mStreamBuf;
};

#endif // LL_LLMEMORYSTREAM_H
