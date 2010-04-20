/** 
 * @file llimagedimensionsinfo.cpp
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
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

#include "llimagejpeg.h"

#include "llimagedimensionsinfo.h"

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


bool LLImageDimensionsInfo::getImageDimensionsJpeg()
{
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

	jpeg_create_decompress	(&cinfo);
	jpeg_stdio_src		(&cinfo, fp);
	jpeg_read_header	(&cinfo, TRUE);
	cinfo.out_color_space = JCS_RGB;
	jpeg_start_decompress	(&cinfo);

	mHeight = cinfo.output_width;
	mHeight = cinfo.output_height;

	jpeg_destroy_decompress(&cinfo);
	fclose(fp);

	return true;
}

