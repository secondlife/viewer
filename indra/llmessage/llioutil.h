/** 
 * @file llioutil.h
 * @author Phoenix
 * @date 2005-10-05
 * @brief Helper classes for dealing with IOPipes
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

#ifndef LL_LLIOUTIL_H
#define LL_LLIOUTIL_H

#include "llbuffer.h"
#include "lliopipe.h"
#include "llpumpio.h"

/** 
 * @class LLIOFlush
 * @brief This class is used as a mini chain head which drains the buffer.
 * @see LLIOPipe
 *
 * An instance of this class acts as a useful chain head when all data
 * is known, and you simply want to get the chain moving.
 */
class LLIOFlush : public LLIOPipe
{
public:
	LLIOFlush() {}
	virtual ~LLIOFlush() {}

protected:
	/* @name LLIOPipe virtual implementations
	 */
	//@{
	/** 
	 * @brief Process the data in buffer
	 */
	EStatus process_impl(
		const LLChannelDescriptors& channels,
		buffer_ptr_t& buffer,
		bool& eos,
		LLSD& context,
		LLPumpIO* pump);
	//@}
protected:
};

/** 
 * @class LLIOSleep
 * @brief This is a simple helper class which will hold a chain and
 * process it later using pump mechanisms
 * @see LLIOPipe
 */
class LLIOSleep : public LLIOPipe
{
public:
	LLIOSleep(F64 sleep_seconds) : mSeconds(sleep_seconds) {}
	virtual ~LLIOSleep() {}

protected:
	/* @name LLIOPipe virtual implementations
	 */
	//@{
	/** 
	 * @brief Process the data in buffer
	 */
	EStatus process_impl(
		const LLChannelDescriptors& channels,
		buffer_ptr_t& buffer,
		bool& eos,
		LLSD& context,
		LLPumpIO* pump);
	//@}
protected:
	F64 mSeconds;
};

/** 
 * @class LLIOAddChain
 * @brief Simple pipe that just adds a chain to a pump.
 * @see LLIOPipe
 */
class LLIOAddChain : public LLIOPipe
{
public:
	LLIOAddChain(const LLPumpIO::chain_t& chain, F32 timeout) :
		mChain(chain),
		mTimeout(timeout)
	{}
	virtual ~LLIOAddChain() {}

protected:
	/* @name LLIOPipe virtual implementations
	 */
	//@{
	/** 
	 * @brief Process the data in buffer
	 */
	EStatus process_impl(
		const LLChannelDescriptors& channels,
		buffer_ptr_t& buffer,
		bool& eos,
		LLSD& context,
		LLPumpIO* pump);
	//@}

protected:
	LLPumpIO::chain_t mChain;
	F32 mTimeout;
};

/** 
 * @class LLChangeChannel
 * @brief This class changes the channel of segments in the buffer
 * @see LLBufferArray
 *
 * This class is useful for iterating over the segments in a buffer
 * array and changing each channel that matches to a different
 * channel.
 * Example:
 * <code>
 * set_in_to_out(LLChannelDescriptors channels, LLBufferArray* buf)
 * {
 *   std::for_each(
 *     buf->beginSegment(),
 *     buf->endSegment(),
 *     LLChangeChannel(channels.in(), channels.out()));
 * }
 * </code>
 */
class LLChangeChannel //: public unary_function<T, void>
{
public:
	/** 
	 * @brief Constructor for iterating over a segment range to change channel.
	 *
	 * @param is The channel to match when looking at a segment.
	 * @param becomes The channel to set the segment when a match is found.
	 */
	LLChangeChannel(S32 is, S32 becomes);

	/** 
	 * @brief Do the work of changing the channel
	 */
	void operator()(LLSegment& segment);

protected:
	S32 mIs;
	S32 mBecomes;
};


#endif // LL_LLIOUTIL_H
