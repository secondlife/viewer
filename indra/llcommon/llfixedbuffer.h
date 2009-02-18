/** 
 * @file llfixedbuffer.h
 * @brief A fixed size buffer of lines.
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

#ifndef LL_LLFIXEDBUFFER_H
#define LL_LLFIXEDBUFFER_H

#include "timer.h"
#include <deque>
#include <string>
#include "llstring.h"
#include "llthread.h"

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

	//do not make these two "virtual"
	void addLine(const std::string& utf8line);
	void addLine(const LLWString& line);

	// Get lines currently in the buffer, up to max_size chars, max_length lines
	char *getLines(U32 max_size = 0, U32 max_length = 0); 
	void setMaxLines(S32 max_lines);
protected:
	virtual void removeExtraLines();

protected:
	LLMutex mMutex ;
};

const U32 FIXED_BUF_MAX_LINE_LEN = 255; // Not including termnating 0

#endif //LL_FIXED_BUFFER_H
