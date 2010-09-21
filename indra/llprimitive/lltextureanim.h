/** 
 * @file lltextureanim.h
 * @brief LLTextureAnim base class
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
