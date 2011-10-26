/** 
 * @file llfixedbuffer.cpp
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */
#include "linden_common.h"

#include "llfixedbuffer.h"

////////////////////////////////////////////////////////////////////////////

LLFixedBuffer::LLFixedBuffer(const U32 max_lines)
	: LLLineBuffer(),
	  mMaxLines(max_lines),
	  mMutex(NULL)
{
	mTimer.reset();
}

LLFixedBuffer::~LLFixedBuffer()
{
	clear();
}

void LLFixedBuffer::clear()
{
	mMutex.lock() ;
	mLines.clear();
	mAddTimes.clear();
	mLineLengths.clear();
	mMutex.unlock() ;

	mTimer.reset();
}


void LLFixedBuffer::addLine(const std::string& utf8line)
{
	LLWString wstring = utf8str_to_wstring(utf8line);
	addWLine(wstring);
}

void LLFixedBuffer::addWLine(const LLWString& line)
{
	if (line.empty())
	{
		return;
	}

	removeExtraLines();

	mMutex.lock() ;
	mLines.push_back(line);
	mLineLengths.push_back((S32)line.length());
	mAddTimes.push_back(mTimer.getElapsedTimeF32());
	mMutex.unlock() ;
}


void LLFixedBuffer::setMaxLines(S32 max_lines)
{
	mMaxLines = max_lines;

	removeExtraLines();
}


void LLFixedBuffer::removeExtraLines()
{
	mMutex.lock() ;
	while ((S32)mLines.size() > llmax((S32)0, (S32)(mMaxLines - 1)))
	{
		mLines.pop_front();
		mAddTimes.pop_front();
		mLineLengths.pop_front();
	}
	mMutex.unlock() ;
}
