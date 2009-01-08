/** 
 * @file llmemorystream.h
 * @author Phoenix
 * @date 2005-06-03
 * @brief Implementation of a simple fixed memory stream
 *
 * $LicenseInfo:firstyear=2005&license=viewergpl$
 * 
 * Copyright (c) 2005-2009, Linden Research, Inc.
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
