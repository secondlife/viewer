/*
 * @file llimagepng.cpp
 * @brief LLImageFormatted glue to encode / decode PNG files.
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007-2009, Linden Research, Inc.
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

#include "linden_common.h"
#include "stdtypes.h"
#include "llerror.h"

#include "llimage.h"
#include "llpngwrapper.h"
#include "llimagepng.h"

// ---------------------------------------------------------------------------
// LLImagePNG
// ---------------------------------------------------------------------------
LLImagePNG::LLImagePNG()
    : LLImageFormatted(IMG_CODEC_PNG),
	  mTmpWriteBuffer(NULL)
{
}

LLImagePNG::~LLImagePNG()
{
	if (mTmpWriteBuffer)
	{
		delete[] mTmpWriteBuffer;
	}
}

// Virtual
// Parse PNG image information and set the appropriate
// width, height and component (channel) information.
BOOL LLImagePNG::updateData()
{
    resetLastError();

    // Check to make sure that this instance has been initialized with data
    if (!getData() || (0 == getDataSize()))
    {
        setLastError("Uninitialized instance of LLImagePNG");
        return FALSE;
    }

	// Decode the PNG data and extract sizing information
	LLPngWrapper pngWrapper;
	LLPngWrapper::ImageInfo infop;
	if (! pngWrapper.readPng(getData(), NULL, &infop))
	{
		setLastError(pngWrapper.getErrorMessage());
		return FALSE;
	}

	setSize(infop.mWidth, infop.mHeight, infop.mComponents);

	return TRUE;
}

// Virtual
// Decode an in-memory PNG image into the raw RGB or RGBA format
// used within SecondLife.
BOOL LLImagePNG::decode(LLImageRaw* raw_image, F32 decode_time)
{
	llassert_always(raw_image);

    resetLastError();

    // Check to make sure that this instance has been initialized with data
    if (!getData() || (0 == getDataSize()))
    {
        setLastError("LLImagePNG trying to decode an image with no data!");
        return FALSE;
    }

	// Decode the PNG data into the raw image
	LLPngWrapper pngWrapper;
	if (! pngWrapper.readPng(getData(), raw_image))
	{
		setLastError(pngWrapper.getErrorMessage());
		return FALSE;
	}

	return TRUE;
}

// Virtual
// Encode the in memory RGB image into PNG format.
BOOL LLImagePNG::encode(const LLImageRaw* raw_image, F32 encode_time)
{
	llassert_always(raw_image);

    resetLastError();

	// Image logical size
	setSize(raw_image->getWidth(), raw_image->getHeight(), raw_image->getComponents());

	// Temporary buffer to hold the encoded image. Note: the final image
	// size should be much smaller due to compression.
	if (mTmpWriteBuffer)
	{
		delete[] mTmpWriteBuffer;
	}
	U32 bufferSize = getWidth() * getHeight() * getComponents() + 1024;
    U8* mTmpWriteBuffer = new U8[ bufferSize ];

	// Delegate actual encoding work to wrapper
	LLPngWrapper pngWrapper;
	if (! pngWrapper.writePng(raw_image, mTmpWriteBuffer))
	{
		setLastError(pngWrapper.getErrorMessage());
		return FALSE;
	}

	// Resize internal buffer and copy from temp
	U32 encodedSize = pngWrapper.getFinalSize();
	allocateData(encodedSize);
	memcpy(getData(), mTmpWriteBuffer, encodedSize);

	delete[] mTmpWriteBuffer;

	return TRUE;
}

