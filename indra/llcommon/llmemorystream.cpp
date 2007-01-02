/** 
 * @file llmemorystream.cpp
 * @author Phoenix
 * @date 2005-06-03
 * @brief Buffer and stream for a fixed linear memory segment.
 *
 * Copyright (c) 2005-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "linden_common.h"
#include "llmemorystream.h"

LLMemoryStreamBuf::LLMemoryStreamBuf(const U8* start, S32 length)
{
	reset(start, length);
}

LLMemoryStreamBuf::~LLMemoryStreamBuf()
{
}

void LLMemoryStreamBuf::reset(const U8* start, S32 length)
{
	setg((char*)start, (char*)start, (char*)start + length);
}

int LLMemoryStreamBuf::underflow()
{
	//lldebugs << "LLMemoryStreamBuf::underflow()" << llendl;
	if(gptr() < egptr())
	{
		return *gptr();
	}
	return EOF;
}

/** 
 * @class LLMemoryStreamBuf
 */

LLMemoryStream::LLMemoryStream(const U8* start, S32 length) :
	std::istream(&mStreamBuf),
	mStreamBuf(start, length)
{
}

LLMemoryStream::~LLMemoryStream()
{
}


