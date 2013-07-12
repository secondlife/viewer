/** 
 * @file llfixedbuffer.h
 * @brief A fixed size buffer of lines.
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

#ifndef LL_LLFIXEDBUFFER_H
#define LL_LLFIXEDBUFFER_H

#include "timer.h"
#include <deque>
#include <string>
#include "llstring.h"
#include "llthread.h"
#include "llerrorcontrol.h"

//  fixed buffer implementation
class LL_COMMON_API LLFixedBuffer : public LLLineBuffer
{
public:
	LLFixedBuffer(const U32 max_lines = 20);
	~LLFixedBuffer();

	LLTimer	mTimer;
	U32		mMaxLines;
	std::deque<LLWString>	mLines;
	std::deque<F32>			mAddTimes;
	std::deque<S32>			mLineLengths;

	/*virtual*/ void clear(); // Clear the buffer, and reset it.

	/*virtual*/ void addLine(const std::string& utf8line);

	void setMaxLines(S32 max_lines);
	
protected:
	void removeExtraLines();
	void addWLine(const LLWString& line);

protected:
	LLMutex mMutex ;
};

#endif //LL_FIXED_BUFFER_H
