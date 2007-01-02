/** 
 * @file lltextureanim.h
 * @brief LLTextureAnim base class
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLTEXTUREANIM_H
#define LL_LLTEXTUREANIM_H

#include "stdtypes.h"
#include "llsd.h"

class LLMessageSystem;
class LLDataPacker;

class LLTextureAnim
{
public:
	LLTextureAnim();
	virtual ~LLTextureAnim();

	virtual void reset();
	void packTAMessage(LLMessageSystem *mesgsys) const;
	void packTAMessage(LLDataPacker &dp) const;
	void unpackTAMessage(LLMessageSystem *mesgsys, const S32 block_num);
	void unpackTAMessage(LLDataPacker &dp);
	BOOL equals(const LLTextureAnim &other) const;
	LLSD asLLSD() const;
	operator LLSD() const { return asLLSD(); }
	bool fromLLSD(LLSD& sd);

	enum
	{
		ON				= 0x01,
		LOOP			= 0x02,
		REVERSE			= 0x04,
		PING_PONG		= 0x08,
		SMOOTH			= 0x10,
		ROTATE			= 0x20,
		SCALE			= 0x40,
	};

public:
	U8 mMode;
	S8 mFace;
	U8 mSizeX;
	U8 mSizeY;
	F32 mStart;
	F32 mLength;
	F32 mRate; // Rate in frames per second.
};
#endif
