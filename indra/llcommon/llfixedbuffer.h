/** 
 * @file llfixedbuffer.h
 * @brief A fixed size buffer of lines.
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLFIXEDBUFFER_H
#define LL_LLFIXEDBUFFER_H

#include "timer.h"
#include <deque>
#include <string>
#include "llstring.h"

// Fixed size buffer for console output and other things.

class LLFixedBuffer
{
public:
	LLFixedBuffer(const U32 max_lines = 20);
	virtual ~LLFixedBuffer();

	LLTimer	mTimer;
	U32		mMaxLines;
	std::deque<LLWString>	mLines;
	std::deque<F32>			mAddTimes;
	std::deque<S32>			mLineLengths;

	void clear(); // Clear the buffer, and reset it.
	virtual void addLine(const LLString& utf8line);
	virtual void addLine(const LLWString& line);

	// Get lines currently in the buffer, up to max_size chars, max_length lines
	char *getLines(U32 max_size = 0, U32 max_length = 0); 
	void setMaxLines(S32 max_lines);
protected:
	virtual void removeExtraLines();
};

const U32 FIXED_BUF_MAX_LINE_LEN = 255; // Not including termnating 0

#endif //LL_FIXED_BUFFER_H
