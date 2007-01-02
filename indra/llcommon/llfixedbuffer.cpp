/** 
 * @file llfixedbuffer.cpp
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */
#include "linden_common.h"

#include "llfixedbuffer.h"

LLFixedBuffer::LLFixedBuffer(const U32 max_lines)
{
	mMaxLines = max_lines;
	mTimer.reset();
}


LLFixedBuffer::~LLFixedBuffer()
{
	clear();
}


void LLFixedBuffer::clear()
{
	mLines.clear();
	mAddTimes.clear();
	mLineLengths.clear();

	mTimer.reset();
}


void LLFixedBuffer::addLine(const LLString& utf8line)
{
	LLWString wstring = utf8str_to_wstring(utf8line);
	LLFixedBuffer::addLine(wstring);
}

void LLFixedBuffer::addLine(const LLWString& line)
{
	if (line.empty())
	{
		return;
	}

	removeExtraLines();

	mLines.push_back(line);
	mLineLengths.push_back((S32)line.length());
	mAddTimes.push_back(mTimer.getElapsedTimeF32());
}


void LLFixedBuffer::setMaxLines(S32 max_lines)
{
	mMaxLines = max_lines;

	removeExtraLines();
}


void LLFixedBuffer::removeExtraLines()
{
	while ((S32)mLines.size() > llmax(0, (S32)(mMaxLines - 1)))
	{
		mLines.pop_front();
		mAddTimes.pop_front();
		mLineLengths.pop_front();
	}
}
