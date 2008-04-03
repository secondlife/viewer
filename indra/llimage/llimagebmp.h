/** 
 * @file llimagebmp.h
 * @brief Image implementation for BMP.
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2007, Linden Research, Inc.
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

#ifndef LL_LLIMAGEBMP_H
#define LL_LLIMAGEBMP_H

#include "llimage.h"

// This class compresses and decompressed BMP files

class LLImageBMP : public LLImageFormatted
{
protected:
	virtual ~LLImageBMP();
	
public:
	LLImageBMP();

	/*virtual*/ BOOL updateData();
	/*virtual*/ BOOL decode(LLImageRaw* raw_image, F32 decode_time);
	/*virtual*/ BOOL encode(const LLImageRaw* raw_image, F32 encode_time);

protected:
	BOOL		decodeColorTable8( U8* dst, U8* src );
	BOOL		decodeColorMask16( U8* dst, U8* src );
	BOOL		decodeTruecolor24( U8* dst, U8* src );
	BOOL		decodeColorMask32( U8* dst, U8* src );

	U32			countTrailingZeros( U32 m );

protected:
	S32			mColorPaletteColors;
	U8*			mColorPalette;
	S32			mBitmapOffset;
	S32			mBitsPerPixel;
	U32			mBitfieldMask[4]; // rgba
	BOOL		mOriginAtTop;
};

#endif
