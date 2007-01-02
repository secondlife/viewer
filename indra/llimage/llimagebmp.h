/** 
 * @file llimagebmp.h
 * @brief Image implementation for BMP.
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
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
	/*virtual*/ BOOL decode(LLImageRaw* raw_image, F32 time=0.0);
	/*virtual*/ BOOL encode(const LLImageRaw* raw_image, F32 time=0.0);

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
