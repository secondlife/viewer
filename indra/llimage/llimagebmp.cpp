/** 
 * @file llimagebmp.cpp
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

#include "linden_common.h"

#include "llimagebmp.h"
#include "llerror.h"

#include "llendianswizzle.h"


/**
 * @struct LLBMPHeader
 *
 * This struct helps deal with bmp files.
 */
struct LLBMPHeader
{
	S32 mSize;
	S32 mWidth;
	S32 mHeight;
	S16 mPlanes;
	S16 mBitsPerPixel;
	S16 mCompression;
	S16 mAlignmentPadding; // pads out to next word boundary
	S32 mImageSize;
	S32 mHorzPelsPerMeter;
	S32 mVertPelsPerMeter;
	S32 mNumColors;
	S32 mNumColorsImportant;
};

/**
 * @struct Win95BmpHeaderExtension
 */
struct Win95BmpHeaderExtension
{
	U32 mReadMask;
	U32 mGreenMask;
	U32 mBlueMask;
	U32 mAlphaMask;
	U32 mColorSpaceType;
	U16	mRed[3];	// Red CIE endpoint
	U16 mGreen[3];	// Green CIE endpoint
	U16 mBlue[3];	// Blue CIE endpoint
	U32 mGamma[3];  // Gamma scale for r g and b
};

/**
 * LLImageBMP
 */
LLImageBMP::LLImageBMP() 
	:
	LLImageFormatted(IMG_CODEC_BMP),
	mColorPaletteColors( 0 ),
	mColorPalette( NULL ),
	mBitmapOffset( 0 ),
	mBitsPerPixel( 0 ),
	mOriginAtTop( false )
{
	mBitfieldMask[0] = 0;
	mBitfieldMask[1] = 0;
	mBitfieldMask[2] = 0;
	mBitfieldMask[3] = 0;
}

LLImageBMP::~LLImageBMP()
{
	delete[] mColorPalette;
}


bool LLImageBMP::updateData()
{
	resetLastError();

	LLImageDataLock lock(this);

	// Check to make sure that this instance has been initialized with data
	U8* mdata = getData();
	if (!mdata || (0 == getDataSize()))
	{
		setLastError("Uninitialized instance of LLImageBMP");
		return false;
	}

	// Read the bitmap headers in order to get all the useful info
	// about this image

	////////////////////////////////////////////////////////////////////
	// Part 1: "File Header"
	// 14 bytes consisting of
	// 2 bytes: either BM or BA
	// 4 bytes: file size in bytes
	// 4 bytes: reserved (always 0)
	// 4 bytes: bitmap offset (starting position of image data in bytes)
	const S32 FILE_HEADER_SIZE = 14;
	if ((mdata[0] != 'B') || (mdata[1] != 'M'))
    {
		if ((mdata[0] != 'B') || (mdata[1] != 'A'))
		{
			setLastError("OS/2 bitmap array BMP files are not supported");
			return false;
		}
		else
		{
			setLastError("Does not appear to be a bitmap file");
			return false;
		}
	}

	mBitmapOffset = mdata[13];
	mBitmapOffset <<= 8; mBitmapOffset += mdata[12];
	mBitmapOffset <<= 8; mBitmapOffset += mdata[11];
	mBitmapOffset <<= 8; mBitmapOffset += mdata[10];


	////////////////////////////////////////////////////////////////////
	// Part 2: "Bitmap Header"
	const S32 BITMAP_HEADER_SIZE = 40;
	LLBMPHeader header;
	llassert( sizeof( header ) == BITMAP_HEADER_SIZE );

	memcpy(	/* Flawfinder: ignore */
		(void*)&header,
		mdata + FILE_HEADER_SIZE,
		BITMAP_HEADER_SIZE);

	// convert BMP header from little endian (no-op on little endian builds)
	llendianswizzleone(header.mSize);
	llendianswizzleone(header.mWidth);
	llendianswizzleone(header.mHeight);
	llendianswizzleone(header.mPlanes);
	llendianswizzleone(header.mBitsPerPixel);
	llendianswizzleone(header.mCompression);
	llendianswizzleone(header.mAlignmentPadding);
	llendianswizzleone(header.mImageSize);
	llendianswizzleone(header.mHorzPelsPerMeter);
	llendianswizzleone(header.mVertPelsPerMeter);
	llendianswizzleone(header.mNumColors);
	llendianswizzleone(header.mNumColorsImportant);

	bool windows_nt_version = false;
	bool windows_95_version = false;
	if( 12 == header.mSize )
	{
		setLastError("Windows 2.x and OS/2 1.x BMP files are not supported");
		return false;
	}
	else
	if( 40 == header.mSize )
	{
		if( 3 == header.mCompression )
		{
			// Windows NT
			windows_nt_version = true;
		}
		else
		{
			// Windows 3.x
		}
	}
	else
	if( 12 <= header.mSize && header.mSize <= 64 )
	{
		setLastError("OS/2 2.x BMP files are not supported");
		return false;
	}
	else
	if( 108 == header.mSize )
	{
		// BITMAPV4HEADER
		windows_95_version = true;
	}
	else
	if( 108 < header.mSize )
	{
		// BITMAPV5HEADER or greater
		// Should work as long at Microsoft maintained backwards compatibility (which they did in V4 and V5)
		windows_95_version = true;
	}

	S32 width = header.mWidth;
	S32 height = header.mHeight;
	if (height < 0)
	{
		mOriginAtTop = true;
		height = -height;
	}
	else
	{
		mOriginAtTop = false;
	}

	mBitsPerPixel = header.mBitsPerPixel;
	S32 components;
	switch( mBitsPerPixel )
	{
	case 8:
		components = 1;
		break;
	case 24:
	case 32:
		components = 3;
		break;
	case 1:
	case 4:
	case 16: // Started work on 16, but doesn't work yet
		// These are legal, but we don't support them yet.
		setLastError("Unsupported bit depth");
		return false;
	default:
		setLastError("Unrecognized bit depth");
		return false;
	}

	setSize(width, height, components);
	
	switch( header.mCompression )
	{
	case 0:
		// Uncompressed
		break;

	case 1:
		setLastError("8 bit RLE compression not supported.");
		return false;

	case 2: 
		setLastError("4 bit RLE compression not supported.");
		return false;

	case 3:
		// Windows NT or Windows 95
		break;

	default:
		setLastError("Unsupported compression format.");
		return false;
	}

	////////////////////////////////////////////////////////////////////
	// Part 3: Bitfield Masks and other color data
	S32 extension_size = 0;
	if( windows_nt_version )
	{
		if( (16 != header.mBitsPerPixel) && (32 != header.mBitsPerPixel) )
		{
			setLastError("Bitfield encoding requires 16 or 32 bits per pixel.");
			return false;
		}

		if( 0 != header.mNumColors )
		{
			setLastError("Bitfield encoding is not compatible with a color table.");
			return false;
		}

		
		extension_size = 4 * 3;
		memcpy( mBitfieldMask, mdata + FILE_HEADER_SIZE + BITMAP_HEADER_SIZE, extension_size);	/* Flawfinder: ignore */
	}
	else
	if( windows_95_version )
	{
		Win95BmpHeaderExtension win_95_extension;
		extension_size = sizeof( win_95_extension );

		llassert( sizeof( win_95_extension ) + BITMAP_HEADER_SIZE == 108 );
		memcpy( &win_95_extension, mdata + FILE_HEADER_SIZE + BITMAP_HEADER_SIZE, sizeof( win_95_extension ) );	/* Flawfinder: ignore */

		if( 3 == header.mCompression )
		{
			memcpy( mBitfieldMask, mdata + FILE_HEADER_SIZE + BITMAP_HEADER_SIZE, 4 * 4);	/* Flawfinder: ignore */
		}

		// Color correction ignored for now
	}
	

	////////////////////////////////////////////////////////////////////
	// Part 4: Color Palette (optional)
	// Note: There's no color palette if there are 16 or more bits per pixel
	S32 color_palette_size = 0;
	mColorPaletteColors = 0;
	if( header.mBitsPerPixel < 16 )
	{
		if( 0 == header.mNumColors )
		{
			mColorPaletteColors = (1 << header.mBitsPerPixel);
		}
		else
		{
			mColorPaletteColors = header.mNumColors;
		}
	}
	color_palette_size = mColorPaletteColors * 4;

	if( 0 != mColorPaletteColors )
	{
		mColorPalette = new(std::nothrow) U8[color_palette_size];
		if (!mColorPalette)
		{
            LLError::LLUserWarningMsg::showOutOfMemory();
			LL_ERRS() << "Out of memory in LLImageBMP::updateData()" << LL_ENDL;
			return false;
		}
		memcpy( mColorPalette, mdata + FILE_HEADER_SIZE + BITMAP_HEADER_SIZE + extension_size, color_palette_size );	/* Flawfinder: ignore */
	}

	return true;
}

bool LLImageBMP::decode(LLImageRaw* raw_image, F32 decode_time)
{
	llassert_always(raw_image);
	
	resetLastError();

	LLImageDataLock lockIn(this);
	LLImageDataLock lockOut(raw_image);

	// Check to make sure that this instance has been initialized with data
	const U8* mdata = getData();
	if (!mdata || (0 == getDataSize()))
	{
		setLastError("llimagebmp trying to decode an image with no data!");
		return false;
	}
	
	if (!raw_image->resize(getWidth(), getHeight(), 3))
	{
		setLastError("llimagebmp failed to resize image!");
		return false;
	}

	const U8* src = mdata + mBitmapOffset;
	U8* dst = raw_image->getData();

	bool success = false;

	switch( mBitsPerPixel )
	{
	case 8:
		if( mColorPaletteColors >= 256 )
		{
			success = decodeColorTable8( dst, src );
		}
		break;
	
	case 16:
		success = decodeColorMask16( dst, src );
		break;
	
	case 24:
		success = decodeTruecolor24( dst, src );
		break;

	case 32:
		success = decodeColorMask32( dst, src );
		break;
	}

	if( success && mOriginAtTop )
	{
		raw_image->verticalFlip();
	}

	return success;
}

U32 LLImageBMP::countTrailingZeros( U32 m )
{
	U32 shift_count = 0;
	while( !(m & 1) )
	{
		shift_count++;
		m >>= 1;
	}
	return shift_count;
}


bool LLImageBMP::decodeColorMask16( U8* dst, const U8* src )
{
	llassert( 16 == mBitsPerPixel );

	if( !mBitfieldMask[0] && !mBitfieldMask[1] && !mBitfieldMask[2] )
	{
		// Use default values
		mBitfieldMask[0] = 0x00007C00;
		mBitfieldMask[1] = 0x000003E0;
		mBitfieldMask[2] = 0x0000001F;
	}

	S32 src_row_span = getWidth() * 2;
	S32 alignment_bytes = (3 * src_row_span) % 4;  // round up to nearest multiple of 4

	U32 r_shift = countTrailingZeros( mBitfieldMask[2] );
	U32 g_shift = countTrailingZeros( mBitfieldMask[1] );
	U32 b_shift = countTrailingZeros( mBitfieldMask[0] );

	for( S32 row = 0; row < getHeight(); row++ )
	{
		for( S32 col = 0; col < getWidth(); col++ )
		{
			U32 value = *((U16*)src);
			dst[0] = U8((value & mBitfieldMask[2]) >> r_shift); // Red
			dst[1] = U8((value & mBitfieldMask[1]) >> g_shift); // Green
			dst[2] = U8((value & mBitfieldMask[0]) >> b_shift); // Blue
			src += 2;
			dst += 3;
		}
		src += alignment_bytes;
	}

	return true;
}

bool LLImageBMP::decodeColorMask32( U8* dst, const U8* src )
{
	// Note: alpha is not supported

	llassert( 32 == mBitsPerPixel );

	if( !mBitfieldMask[0] && !mBitfieldMask[1] && !mBitfieldMask[2] )
	{
		// Use default values
		mBitfieldMask[0] = 0x00FF0000;
		mBitfieldMask[1] = 0x0000FF00;
		mBitfieldMask[2] = 0x000000FF;
	}

	if (getWidth() * getHeight() * 4 > getDataSize() - mBitmapOffset)
	{ //here we have situation when data size in src less than actually needed
		return false;
	}

	S32 src_row_span = getWidth() * 4;
	S32 alignment_bytes = (3 * src_row_span) % 4;  // round up to nearest multiple of 4

	U32 r_shift = countTrailingZeros( mBitfieldMask[0] );
	U32 g_shift = countTrailingZeros( mBitfieldMask[1] );
	U32 b_shift = countTrailingZeros( mBitfieldMask[2] );

	for( S32 row = 0; row < getHeight(); row++ )
	{
		for( S32 col = 0; col < getWidth(); col++ )
		{
			U32 value = *((U32*)src);
			dst[0] = U8((value & mBitfieldMask[0]) >> r_shift); // Red
			dst[1] = U8((value & mBitfieldMask[1]) >> g_shift); // Green
			dst[2] = U8((value & mBitfieldMask[2]) >> b_shift); // Blue
			src += 4;
			dst += 3;
		}
		src += alignment_bytes;
	}

	return true;
}


bool LLImageBMP::decodeColorTable8( U8* dst, const U8* src )
{
	llassert( (8 == mBitsPerPixel) && (mColorPaletteColors >= 256) );

	S32 src_row_span = getWidth() * 1;
	S32 alignment_bytes = (3 * src_row_span) % 4;  // round up to nearest multiple of 4

	if ((getWidth() * getHeight()) + getHeight() * alignment_bytes > getDataSize() - mBitmapOffset)
	{ //here we have situation when data size in src less than actually needed
		return false;
	}

	for( S32 row = 0; row < getHeight(); row++ )
	{
		for( S32 col = 0; col < getWidth(); col++ )
		{
			S32 index = 4 * src[0];
			dst[0] = mColorPalette[index + 2];	// Red
			dst[1] = mColorPalette[index + 1];	// Green
			dst[2] = mColorPalette[index + 0];	// Blue
			src++;
			dst += 3;
		}
		src += alignment_bytes;
	}

	return true;
}


bool LLImageBMP::decodeTruecolor24( U8* dst, const U8* src )
{
	llassert( 24 == mBitsPerPixel );
	llassert( 3 == getComponents() );
	S32 src_row_span = getWidth() * 3;
	S32 alignment_bytes = (3 * src_row_span) % 4;  // round up to nearest multiple of 4

	if ((getWidth() * getHeight() * 3) + getHeight() * alignment_bytes > getDataSize() - mBitmapOffset)
	{ //here we have situation when data size in src less than actually needed
		return false;
	}

	for( S32 row = 0; row < getHeight(); row++ )
	{
		for( S32 col = 0; col < getWidth(); col++ )
		{
			dst[0] = src[2];	// Red
			dst[1] = src[1];	// Green
			dst[2] = src[0];	// Blue
			src += 3;
			dst += 3;
		}
		src += alignment_bytes;
	}

	return true;
}

bool LLImageBMP::encode(const LLImageRaw* raw_image, F32 encode_time)
{
	llassert_always(raw_image);
	
	resetLastError();

	LLImageDataSharedLock lockIn(raw_image);
	LLImageDataLock lockOut(this);

	S32 src_components = raw_image->getComponents();
	S32 dst_components =  ( src_components < 3 ) ? 1 : 3;

	if( (2 == src_components) || (4 == src_components) )
	{
		LL_INFOS() << "Dropping alpha information during BMP encoding" << LL_ENDL;
	}

	setSize(raw_image->getWidth(), raw_image->getHeight(), dst_components);
	
	U8 magic[14];
	LLBMPHeader header;
	int header_bytes = 14+sizeof(header);
	llassert(header_bytes == 54);
	if (getComponents() == 1)
	{
		header_bytes += 1024; // Need colour LUT.
	}
	int line_bytes = getComponents() * getWidth();
	int alignment_bytes = (3 * line_bytes) % 4;
	line_bytes += alignment_bytes;
	int file_bytes = line_bytes*getHeight() + header_bytes;

	// Allocate the new buffer for the data.
	if(!allocateData(file_bytes)) //memory allocation failed
	{
		return false ;
	}

	magic[0] = 'B'; magic[1] = 'M';
	magic[2] = (U8) file_bytes;
	magic[3] = (U8)(file_bytes>>8);
	magic[4] = (U8)(file_bytes>>16);
	magic[5] = (U8)(file_bytes>>24);
	magic[6] = magic[7] = magic[8] = magic[9] = 0;
	magic[10] = (U8) header_bytes;
	magic[11] = (U8)(header_bytes>>8);
	magic[12] = (U8)(header_bytes>>16);
	magic[13] = (U8)(header_bytes>>24);
	header.mSize = 40;
	header.mWidth = getWidth();
	header.mHeight = getHeight();
	header.mPlanes = 1;
	header.mBitsPerPixel = (getComponents()==1)?8:24;
	header.mCompression = 0;
	header.mAlignmentPadding = 0;
	header.mImageSize = 0;
#if LL_DARWIN
	header.mHorzPelsPerMeter = header.mVertPelsPerMeter = 2834;	// 72dpi
#else
	header.mHorzPelsPerMeter = header.mVertPelsPerMeter = 0;
#endif
	header.mNumColors = header.mNumColorsImportant = 0;

	// convert BMP header to little endian (no-op on little endian builds)
	llendianswizzleone(header.mSize);
	llendianswizzleone(header.mWidth);
	llendianswizzleone(header.mHeight);
	llendianswizzleone(header.mPlanes);
	llendianswizzleone(header.mBitsPerPixel);
	llendianswizzleone(header.mCompression);
	llendianswizzleone(header.mAlignmentPadding);
	llendianswizzleone(header.mImageSize);
	llendianswizzleone(header.mHorzPelsPerMeter);
	llendianswizzleone(header.mVertPelsPerMeter);
	llendianswizzleone(header.mNumColors);
	llendianswizzleone(header.mNumColorsImportant);

	U8* mdata = getData();
	
	// Output magic, then header, then the palette table, then the data.
	U32 cur_pos = 0;
	memcpy(mdata, magic, 14);
	cur_pos += 14;
	memcpy(mdata+cur_pos, &header, 40);	/* Flawfinder: ignore */
	cur_pos += 40;
	if (getComponents() == 1)
	{
		S32 n;
		for (n=0; n < 256; n++)
		{
			mdata[cur_pos++] = (U8)n;
			mdata[cur_pos++] = (U8)n;
			mdata[cur_pos++] = (U8)n;
			mdata[cur_pos++] = 0;
		}
	}
	
	// Need to iterate through, because we need to flip the RGB.
	const U8* src = raw_image->getData();
	U8* dst = mdata + cur_pos;

	for( S32 row = 0; row < getHeight(); row++ )
	{
		for( S32 col = 0; col < getWidth(); col++ )
		{
			switch( src_components )
			{
			case 1:
				*dst++ = *src++;
				break;
			case 2:
				{
					U32 lum = src[0];
					U32 alpha = src[1];
					*dst++ = (U8)(lum * alpha / 255);
					src += 2;
					break;
				}
			case 3:
			case 4:
				dst[0] = src[2];
				dst[1] = src[1];
				dst[2] = src[0];
				src += src_components;
				dst += 3;
				break;
			}

		}
		for( S32 i = 0; i < alignment_bytes; i++ )
		{
			*dst++ = 0;
		}
	}

	return true;
}
