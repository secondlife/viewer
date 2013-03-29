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
	// Make sure the file is long enough.
	const S32 DATA_LEN = 26; // BMP header (14) + DIB header size (4) + width (4) + height (4)
	if (!checkFileLength(DATA_LEN))
	{
		llwarns << "Premature end of file" << llendl;
		return false;
	}

	// Read BMP signature.
	U8 signature[2];
	mInfile.read((void*)signature, sizeof(signature)/sizeof(signature[0]));

	// Make sure this is actually a BMP file.
	// We only support Windows bitmaps (BM), according to LLImageBMP::updateData().
	if (signature[0] != 'B' || signature[1] != 'M')
	{
		llwarns << "Not a BMP" << llendl;
		return false;
	}

	// Read image dimensions.
	mInfile.seek(APR_CUR, 16);
	mWidth = read_reverse_s32();
	mHeight = read_reverse_s32();

	return true;
}

bool LLImageDimensionsInfo::getImageDimensionsTga()
{
	const S32 TGA_FILE_HEADER_SIZE = 12;

	// Make sure the file is long enough.
	if (!checkFileLength(TGA_FILE_HEADER_SIZE + 1 /* width */ + 1 /* height */))
	{
		llwarns << "Premature end of file" << llendl;
		return false;
	}

	// *TODO: Detect non-TGA files somehow.
	mInfile.seek(APR_CUR,TGA_FILE_HEADER_SIZE);
	mWidth = read_byte() | read_byte() << 8;
	mHeight = read_byte() | read_byte() << 8;

	return true;
}

bool LLImageDimensionsInfo::getImageDimensionsPng()
{
	const S32 PNG_MAGIC_SIZE = 8;

	// Make sure the file is long enough.
	if (!checkFileLength(PNG_MAGIC_SIZE + 8 + sizeof(S32) * 2 /* width, height */))
	{
		llwarns << "Premature end of file" << llendl;
		return false;
	}

	// Read PNG signature.
	const U8 png_magic[PNG_MAGIC_SIZE] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
	U8 signature[PNG_MAGIC_SIZE];
	mInfile.read((void*)signature, PNG_MAGIC_SIZE);

	// Make sure it's a PNG file.
	if (memcmp(signature, png_magic, PNG_MAGIC_SIZE) != 0)
	{
		llwarns << "Not a PNG" << llendl;
		return false;
	}

	// Read image dimensions.
	mInfile.seek(APR_CUR, 8 /* chunk length + chunk type */);
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

	/* Make sure this is a JPEG file. */
	const size_t JPEG_MAGIC_SIZE = 2;
	const uint8_t jpeg_magic[JPEG_MAGIC_SIZE] = {0xFF, 0xD8};
	uint8_t signature[JPEG_MAGIC_SIZE];

	if (fread(signature, sizeof(signature), 1, fp) != 1)
	{
		llwarns << "Premature end of file" << llendl;
		return false;
	}
	if (memcmp(signature, jpeg_magic, JPEG_MAGIC_SIZE) != 0)
	{
		llwarns << "Not a JPEG" << llendl;
		return false;
	}
	fseek(fp, 0, SEEK_SET); // go back to start of the file

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

bool LLImageDimensionsInfo::checkFileLength(S32 min_len)
{
	// Make sure the file is not shorter than min_len bytes.
	// so that we don't have to check value returned by each read() or seek().
	char* buf = new char[min_len];
	int nread = mInfile.read(buf, min_len);
	delete[] buf;
	mInfile.seek(APR_SET, 0);
	return nread == min_len;
}
