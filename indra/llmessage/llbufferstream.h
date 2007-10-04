/** 
 * @file llbufferstream.h
 * @author Phoenix
 * @date 2005-10-10
 * @brief Classes to treat an LLBufferArray as a c++ iostream.
 *
 * $LicenseInfo:firstyear=2005&license=viewergpl$
 * 
 * Copyright (c) 2005-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

#ifndef LL_LLBUFFERSTREAM_H
#define LL_LLBUFFERSTREAM_H

#include <iosfwd>
#include <iostream>
#include "llbuffer.h"

/** 
 * @class LLBufferStreamBuf
 * @brief This implements the buffer wrapper for an istream
 *
 * The buffer array passed in is not owned by the stream buf object.
 */
class LLBufferStreamBuf : public std::streambuf
{
public:
	LLBufferStreamBuf(
		const LLChannelDescriptors& channels,
		LLBufferArray* buffer);
	virtual ~LLBufferStreamBuf();

protected:
#if( LL_WINDOWS || __GNUC__ > 2 )
	typedef std::streambuf::pos_type pos_type;
	typedef std::streambuf::off_type off_type;
#endif

	/* @name streambuf vrtual implementations
	 */
	//@{
	/*
	 * @brief called when we hit the end of input
	 *
	 * @return Returns the character at the current position or EOF.
	 */
	virtual int underflow();

	/*
	 * @brief called when we hit the end of output
	 *
	 * @param c The character to store at the current put position
	 * @return Returns EOF if the function failed. Any other value on success.
	 */
	virtual int overflow(int c);

	/*
	 * @brief synchronize the buffer
	 *
	 * @return Returns 0 on success or -1 on failure.
	 */
	virtual int sync();

	/*
	 * @brief Seek to an offset position in a stream.
	 *
	 * @param off Offset value relative to way paramter
	 * @param way The seek direction. One of ios::beg, ios::cur, and ios::end.
	 * @param which Which pointer to modify. One of ios::in, ios::out,
	 * or both masked together.
	 * @return Returns the new position or an invalid position on failure.
	 */
#if( LL_WINDOWS || __GNUC__ > 2)
	virtual pos_type seekoff(
		off_type off,
		std::ios::seekdir way,
		std::ios::openmode which);
#else
	virtual streampos seekoff(
		streamoff off,
		std::ios::seekdir way,
		std::ios::openmode which);
#endif

	/*
	 * @brief Get s sequence of characters from the input
	 *
	 * @param dst Pointer to a block of memory to accept the characters
	 * @param length Number of characters to be read
	 * @return Returns the number of characters read
	 */
	//virtual streamsize xsgetn(char* dst, streamsize length);

	/*
	 * @brief Write some characters to output
	 *
	 * @param src Pointer to a sequence of characters to be output
	 * @param length Number of characters to be put
	 * @return Returns the number of characters written
	 */
	//virtual streamsize xsputn(char* src, streamsize length);
	//@}

protected:
	// This channels we are working on.
	LLChannelDescriptors mChannels;

	// The buffer we work on
	LLBufferArray* mBuffer;
};


/** 
 * @class LLBufferStream
 * @brief This implements an istream based wrapper around an LLBufferArray.
 *
 * This class does not own the buffer array, and does not hold a
 * shared pointer to it. Since the class itself is fairly ligthweight,
 * just make one on the stack when needed and let it fall out of
 * scope.
 */
class LLBufferStream : public std::iostream
{
public:
	LLBufferStream(
		const LLChannelDescriptors& channels,
		LLBufferArray* buffer);
	~LLBufferStream();

protected:
	LLBufferStreamBuf mStreamBuf;
};


#endif // LL_LLBUFFERSTREAM_H
