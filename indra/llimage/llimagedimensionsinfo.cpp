/** 
 * @file llimagedimensionsinfo.cpp
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#include "llimagejpeg.h"

#include "llimagedimensionsinfo.h"

// Value is true if one of Libjpeg's functions has encountered an error while working.
static bool sJpegErrorEncountered = false;

bool LLImageDimensionsInfo::load(const std::string& src_filename,U32 codec)
{
	clean();

	mSrcFilename = src_filename;

	S32 file_size = 0;
	apr_status_t s = mInfile.open(src_filename, LL_APR_RB, NULL, &file_size);

	if (s != APR_SUCCESS)
	{
		setLastError("Unable to open file for reading", src_filename);
		return false;
	}

	if (file_size == 0)
	{
		setLastError("File is empty",src_filename);
		return false;
	}

	switch (codec)
	{
	case IMG_CODEC_BMP:
		return getImageDimensionsBmp();
	case IMG_CODEC_TGA:
		return getImageDimensionsTga();
	case IMG_CODEC_JPEG:
		return getImageDimensionsJpeg();
	case IMG_CODEC_PNG:
		return getImageDimensionsPng();
	default:
		return false;

	}
}


bool LLImageDimensionsInfo::getImageDimensionsBmp()
{
	const S32 BMP_FILE_HEADER_SIZE = 14;

	mInfile.seek(APR_CUR,BMP_FILE_HEADER_SIZE+4);
	mWidth = read_reverse_s32();
	mHeight = read_reverse_s32();

	return true;
}

bool LLImageDimensionsInfo::getImageDimensionsTga()
{
	const S32 TGA_FILE_HEADER_SIZE = 12;

	mInfile.seek(APR_CUR,TGA_FILE_HEADER_SIZE);
	mWidth = read_byte() | read_byte() << 8;
	mHeight = read_byte() | read_byte() << 8;

	return true;
}

bool LLImageDimensionsInfo::getImageDimensionsPng()
{
	const S32 PNG_FILE_MARKER_SIZE = 8;

	mInfile.seek(APR_CUR,PNG_FILE_MARKER_SIZE + 8/*header offset+chunk length+chunk type*/);
	mWidth = read_s32();
	mHeight = read_s32();

	return true;
}

// Called instead of exit() if Libjpeg encounters an error.
void on_jpeg_error(j_common_ptr cinfo)
{
	(void) cinfo;
	sJpegErrorEncountered = true;
	llwarns << "Libjpeg has encountered an error!" << llendl;
}

bool LLImageDimensionsInfo::getImageDimensionsJpeg()
{
	sJpegErrorEncountered = false;
	clean();
	FILE *fp = fopen (mSrcFilename.c_str(), "rb");
	if (fp == NULL) 
	{
		setLastError("Unable to open file for reading", mSrcFilename);
		return false;
	}
	/* Init jpeg */
	jpeg_error_mgr jerr;
	jpeg_decompress_struct cinfo;
	cinfo.err = jpeg_std_error(&jerr);
	// Call our function instead of exit() if Libjpeg encounters an error.
	// This is done to avoid crash in this case (STORM-472).
	cinfo.err->error_exit = on_jpeg_error;

	jpeg_create_decompress	(&cinfo);
	jpeg_stdio_src		(&cinfo, fp);
	jpeg_read_header	(&cinfo, TRUE);
	cinfo.out_color_space = JCS_RGB;
	jpeg_start_decompress	(&cinfo);

	mHeight = cinfo.output_width;
	mHeight = cinfo.output_height;

	jpeg_destroy_decompress(&cinfo);
	fclose(fp);

	return !sJpegErrorEncountered;
}

