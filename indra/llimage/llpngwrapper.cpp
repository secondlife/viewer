/*
 * @file llpngwrapper.cpp
 * @brief Encapsulates libpng read/write functionality.
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

#include "linden_common.h"
#include "stdtypes.h"
#include "llerror.h"

#include "llimage.h"
#include "llpngwrapper.h"

// ---------------------------------------------------------------------------
// LLPngWrapper
// ---------------------------------------------------------------------------

LLPngWrapper::LLPngWrapper()
	: mReadPngPtr( NULL ),
	  mReadInfoPtr( NULL ),
	  mWritePngPtr( NULL ),
	  mWriteInfoPtr( NULL ),
	  mRowPointers( NULL ),
	  mWidth( 0 ),
	  mHeight( 0 ),
	  mBitDepth( 0 ),
	  mColorType( 0 ),
	  mChannels( 0 ),
	  mInterlaceType( 0 ),
	  mCompressionType( 0 ),
	  mFilterMethod( 0 ),
	  mFinalSize( 0 ),
	  mGamma(0.f)
{
}

LLPngWrapper::~LLPngWrapper()
{
	releaseResources();
}

// Checks the src for a valid PNG header
BOOL LLPngWrapper::isValidPng(U8* src)
{
	const int PNG_BYTES_TO_CHECK = 8;

	int sig = png_sig_cmp((png_bytep)src, (png_size_t)0, PNG_BYTES_TO_CHECK);
	if (sig != 0)
	{
		mErrorMessage = "Invalid or corrupt PNG file";
		return FALSE;
	}

	return TRUE;
}

// Called by the libpng library when a fatal encoding or decoding error
// occurs.  We simply throw the error message and let our try/catch
// block clean up.
void LLPngWrapper::errorHandler(png_structp png_ptr, png_const_charp msg)
{
	throw msg;
}

// Called by the libpng library when reading (decoding) the PNG file. We
// copy the PNG data from our internal buffer into the PNG's data buffer.
void LLPngWrapper::readDataCallback(png_structp png_ptr, png_bytep dest, png_size_t length)
{
	PngDataInfo *dataInfo = (PngDataInfo *) png_get_io_ptr(png_ptr);
	U8 *src = &dataInfo->mData[dataInfo->mOffset];
	memcpy(dest, src, length);
	dataInfo->mOffset += static_cast<U32>(length);
}

// Called by the libpng library when writing (encoding) the PNG file. We
// copy the encoded result into our data buffer.
void LLPngWrapper::writeDataCallback(png_structp png_ptr, png_bytep src, png_size_t length)
{
	PngDataInfo *dataInfo = (PngDataInfo *) png_get_io_ptr(png_ptr);
	U8 *dest = &dataInfo->mData[dataInfo->mOffset];
	memcpy(dest, src, length);
	dataInfo->mOffset += static_cast<U32>(length);
}

// Flush the write output pointer
void LLPngWrapper::writeFlush(png_structp png_ptr)
{
	// no-op since we're just writing to memory
}

// Read the PNG file using the libpng.  The low-level interface is used here
// because we want to do various transformations (including applying gama)
// which can't be done with the high-level interface.
// The scanline also begins at the bottom of
// the image (per SecondLife conventions) instead of at the top, so we
// must assign row-pointers in "reverse" order.
BOOL LLPngWrapper::readPng(U8* src, LLImageRaw* rawImage, ImageInfo *infop)
{
	try
	{
		// Create and initialize the png structures
		mReadPngPtr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
			this, &errorHandler, NULL);
		if (mReadPngPtr == NULL)
		{
			throw "Problem creating png read structure";
		}

		// Allocate/initialize the memory for image information.
		mReadInfoPtr = png_create_info_struct(mReadPngPtr);

		// Set up the input control
		PngDataInfo dataPtr;
		dataPtr.mData = src;
		dataPtr.mOffset = 0;

		png_set_read_fn(mReadPngPtr, &dataPtr, &readDataCallback);
		png_set_sig_bytes(mReadPngPtr, 0);

		// setup low-level read and get header information
		png_read_info(mReadPngPtr, mReadInfoPtr);
		png_get_IHDR(mReadPngPtr, mReadInfoPtr, &mWidth, &mHeight,
			&mBitDepth, &mColorType, &mInterlaceType,
			&mCompressionType, &mFilterMethod);

		// Normalize the image, then get updated image information
		// after transformations have been applied
		normalizeImage();
		updateMetaData();

		// If a raw object is supplied, read the PNG image into its
		// data space
		if (rawImage != NULL)
		{
			rawImage->resize(static_cast<U16>(mWidth),
				static_cast<U16>(mHeight), mChannels);
			U8 *dest = rawImage->getData();
			int offset = mWidth * mChannels;

			// Set up the row pointers and read the image
			mRowPointers = new U8* [mHeight];
			for (U32 i=0; i < mHeight; i++)
			{
				mRowPointers[i] = &dest[(mHeight-i-1)*offset];
			}

			png_read_image(mReadPngPtr, mRowPointers);

			// Finish up, ensures all metadata are updated
			png_read_end(mReadPngPtr, NULL);
		}

		// If an info object is supplied, copy the relevant info
		if (infop != NULL)
		{
			infop->mHeight = static_cast<U16>(mHeight);
			infop->mWidth = static_cast<U16>(mWidth);
			infop->mComponents = mChannels;
		}

		mFinalSize = dataPtr.mOffset;
	}
	catch (png_const_charp msg)
	{
		mErrorMessage = msg;
		releaseResources();
		return (FALSE);
	}

	// Clean up and return
	releaseResources();
	return (TRUE);
}

// Do transformations to normalize the input to 8-bpp RGBA
void LLPngWrapper::normalizeImage()
{
	//		1. Expand any palettes
	//		2. Convert grayscales to RGB
	//		3. Create alpha layer from transparency
	//		4. Ensure 8-bpp for all images
	//		5. Set (or guess) gamma

	if (mColorType == PNG_COLOR_TYPE_PALETTE)
	{
		png_set_palette_to_rgb(mReadPngPtr);
	}
    if (mColorType == PNG_COLOR_TYPE_GRAY && mBitDepth < 8)
	{
		png_set_expand_gray_1_2_4_to_8(mReadPngPtr);
	}
	if (mColorType == PNG_COLOR_TYPE_GRAY
		|| mColorType == PNG_COLOR_TYPE_GRAY_ALPHA)
	{
		png_set_gray_to_rgb(mReadPngPtr);
	}
	if (png_get_valid(mReadPngPtr, mReadInfoPtr, PNG_INFO_tRNS))
	{
		png_set_tRNS_to_alpha(mReadPngPtr);
	}
	if (mBitDepth < 8)
	{
		png_set_packing(mReadPngPtr);
	}
	else if (mBitDepth == 16)
	{
		png_set_strip_16(mReadPngPtr);
	}

#if LL_DARWIN
	const F64 SCREEN_GAMMA = 1.8;
#else
	const F64 SCREEN_GAMMA = 2.2;
#endif

	if (png_get_gAMA(mReadPngPtr, mReadInfoPtr, &mGamma))
	{
		png_set_gamma(mReadPngPtr, SCREEN_GAMMA, mGamma);
	}
	else
	{
		png_set_gamma(mReadPngPtr, SCREEN_GAMMA, 1/SCREEN_GAMMA);
	}
}

// Read out the image meta-data
void LLPngWrapper::updateMetaData()
{
	png_read_update_info(mReadPngPtr, mReadInfoPtr);
    mWidth = png_get_image_width(mReadPngPtr, mReadInfoPtr);
    mHeight = png_get_image_height(mReadPngPtr, mReadInfoPtr);
    mBitDepth = png_get_bit_depth(mReadPngPtr, mReadInfoPtr);
    mColorType = png_get_color_type(mReadPngPtr, mReadInfoPtr);
	mChannels = png_get_channels(mReadPngPtr, mReadInfoPtr);
}

// Method to write raw image into PNG at dest. The raw scanline begins
// at the bottom of the image per SecondLife conventions.
BOOL LLPngWrapper::writePng(const LLImageRaw* rawImage, U8* dest)
{
	try
	{
		S8 numComponents = rawImage->getComponents();
		switch (numComponents)
		{
		case 1:
			mColorType = PNG_COLOR_TYPE_GRAY;
			break;
		case 2:
			mColorType = PNG_COLOR_TYPE_GRAY_ALPHA;
			break;
		case 3:
			mColorType = PNG_COLOR_TYPE_RGB;
			break;
		case 4:
			mColorType = PNG_COLOR_TYPE_RGB_ALPHA;
			break;
		default:
			mColorType = -1;
		}

		if (mColorType == -1)
		{
			throw "Unsupported image: unexpected number of channels";
		}

		mWritePngPtr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
			NULL, &errorHandler, NULL);
		if (!mWritePngPtr)
		{
			throw "Problem creating png write structure";
		}

		mWriteInfoPtr = png_create_info_struct(mWritePngPtr);

		// Setup write function
		PngDataInfo dataPtr;
		dataPtr.mData = dest;
		dataPtr.mOffset = 0;
		png_set_write_fn(mWritePngPtr, &dataPtr, &writeDataCallback, &writeFlush);

		// Setup image params
		mWidth = rawImage->getWidth();
		mHeight = rawImage->getHeight();
		mBitDepth = 8;	// Fixed to 8-bpp in SL
		mChannels = numComponents;
		mInterlaceType = PNG_INTERLACE_NONE;
		mCompressionType = PNG_COMPRESSION_TYPE_DEFAULT;
		mFilterMethod = PNG_FILTER_TYPE_DEFAULT;

		// Write header
		png_set_IHDR(mWritePngPtr, mWriteInfoPtr, mWidth, mHeight,
			mBitDepth, mColorType, mInterlaceType,
			mCompressionType, mFilterMethod);

		// Get data and compute row size
		const U8* data = rawImage->getData();
		int offset = mWidth * mChannels;

		// Ready to write, start with the header
		png_write_info(mWritePngPtr, mWriteInfoPtr);

		// Write image (sorry, must const-cast for libpng)
		const U8 * rowPointer;
		for (U32 i=0; i < mHeight; i++)
		{
			rowPointer = &data[(mHeight-1-i)*offset];
			png_write_row(mWritePngPtr, const_cast<png_bytep>(rowPointer));
		}

		// Finish up
		png_write_end(mWritePngPtr, mWriteInfoPtr);
		mFinalSize = dataPtr.mOffset;
	}
	catch (png_const_charp msg)
	{
		mErrorMessage = msg;
		releaseResources();
		return (FALSE);
	}

	releaseResources();
	return TRUE;
}

// Cleanup various internal structures
void LLPngWrapper::releaseResources()
{
	if (mReadPngPtr || mReadInfoPtr)
	{
		png_destroy_read_struct(&mReadPngPtr, &mReadInfoPtr, NULL);
		mReadPngPtr = NULL;
		mReadInfoPtr = NULL;
	}

	if (mWritePngPtr || mWriteInfoPtr)
	{
		png_destroy_write_struct(&mWritePngPtr, &mWriteInfoPtr);
		mWritePngPtr = NULL;
		mWriteInfoPtr = NULL;
	}

	if (mRowPointers)
	{
		delete[] mRowPointers;
		mRowPointers = NULL;
	}
}

// Get final image size after compression
U32 LLPngWrapper::getFinalSize()
{
	return mFinalSize;
}

// Get last error message, if any
const std::string& LLPngWrapper::getErrorMessage()
{
	return mErrorMessage;
}
