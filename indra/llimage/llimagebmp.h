/** 
 * @file llimagebmp.h
 * @brief Image implementation for BMP.
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

    /*virtual*/ std::string getExtension() { return std::string("bmp"); }
    /*virtual*/ bool updateData();
    /*virtual*/ bool decode(LLImageRaw* raw_image, F32 decode_time);
    /*virtual*/ bool encode(const LLImageRaw* raw_image, F32 encode_time);

protected:
    bool        decodeColorTable8( U8* dst, U8* src );
    bool        decodeColorMask16( U8* dst, U8* src );
    bool        decodeTruecolor24( U8* dst, U8* src );
    bool        decodeColorMask32( U8* dst, U8* src );

    U32         countTrailingZeros( U32 m );

protected:
    S32         mColorPaletteColors;
    U8*         mColorPalette;
    S32         mBitmapOffset;
    S32         mBitsPerPixel;
    U32         mBitfieldMask[4]; // rgba
    bool        mOriginAtTop;
};

#endif
