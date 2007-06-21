/*
 * @file llimagepng.cpp
 * @brief LLImageFormatted glue to encode / decode PNG files.
 *
 * Copyright (c) 2007 Peekay Semyorka.
 * Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
 * $License$
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

