/** 
 * @file llfixedbuffer.cpp
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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


void LLFixedBuffer::addLine(const std::string& utf8line)
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
	while ((S32)mLines.size() > llmax((S32)0, (S32)(mMaxLines - 1)))
	{
		mLines.pop_front();
		mAddTimes.pop_front();
		mLineLengths.pop_front();
	}
}
