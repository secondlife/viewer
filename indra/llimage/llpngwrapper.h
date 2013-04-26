/*
 * @file llpngwrapper.h
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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

#ifndef LL_LLPNGWRAPPER_H
#define LL_LLPNGWRAPPER_H

#include "png.h"
#include "llimage.h"

class LLPngWrapper
{
public:
	LLPngWrapper();
	virtual ~LLPngWrapper();

public:
	struct ImageInfo
	{
		U16 mWidth;
		U16 mHeight;
		S8  mComponents;
	};

	BOOL isValidPng(U8* src);
	BOOL readPng(U8* src, LLImageRaw* rawImage, ImageInfo *infop = NULL);
	BOOL writePng(const LLImageRaw* rawImage, U8* dst);
	U32  getFinalSize();
	const std::string& getErrorMessage();

protected:
	void normalizeImage();
	void updateMetaData();

private:

	// Structure for writing/reading PNG data to/from memory
	// as opposed to using a file.
	struct PngDataInfo
	{
		U8 *mData;
		U32 mOffset;
	};

	static void writeFlush(png_structp png_ptr);
	static void errorHandler(png_structp png_ptr, png_const_charp msg);
	static void readDataCallback(png_structp png_ptr, png_bytep dest, png_size_t length);
	static void writeDataCallback(png_structp png_ptr, png_bytep src, png_size_t length);

	void releaseResources();

	png_structp mReadPngPtr;
	png_infop mReadInfoPtr;
	png_structp mWritePngPtr;
	png_infop mWriteInfoPtr;

	U8 **mRowPointers;

	png_uint_32 mWidth;
	png_uint_32 mHeight;
	S32 mBitDepth;
	S32 mColorType;
	S32 mChannels;
	S32 mInterlaceType;
	S32 mCompressionType;
	S32 mFilterMethod;

	U32 mFinalSize;

	F64 mGamma;

	std::string mErrorMessage;
};

#endif
