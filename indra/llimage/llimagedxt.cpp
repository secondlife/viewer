/** 
 * @file llimagedxt.cpp
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

#include "llimagedxt.h"
#include "llmemory.h"

//static
void LLImageDXT::checkMinWidthHeight(EFileFormat format, S32& width, S32& height)
{
	S32 mindim = (format >= FORMAT_DXT1 && format <= FORMAT_DXR5) ? 4 : 1;
	width = llmax(width, mindim);
	height = llmax(height, mindim);
}

//static
S32 LLImageDXT::formatBits(EFileFormat format)
{
	switch (format)
	{
 	  case FORMAT_DXT1: 	return 4;
	  case FORMAT_DXR1: 	return 4;
	  case FORMAT_I8:		return 8;
	  case FORMAT_A8:		return 8;
 	  case FORMAT_DXT3:		return 8;
	  case FORMAT_DXR3:		return 8;
 	  case FORMAT_DXR5:		return 8;
 	  case FORMAT_DXT5:		return 8;
	  case FORMAT_RGB8:		return 24;
	  case FORMAT_RGBA8:	return 32;
	  default:
		llerrs << "LLImageDXT::Unknown format: " << format << llendl;
		return 0;
	}
};

//static
S32 LLImageDXT::formatBytes(EFileFormat format, S32 width, S32 height)
{
	checkMinWidthHeight(format, width, height);
	S32 bytes = ((width*height*formatBits(format)+7)>>3);
	S32 aligned = (bytes+3)&~3;
	return aligned;
}

//static
S32 LLImageDXT::formatComponents(EFileFormat format)
{
	switch (format)
	{
 	  case FORMAT_DXT1: 	return 3;
	  case FORMAT_DXR1: 	return 3;
	  case FORMAT_I8:		return 1;
	  case FORMAT_A8:		return 1;
 	  case FORMAT_DXT3:		return 4;
	  case FORMAT_DXR3:		return 4;
 	  case FORMAT_DXT5:		return 4;
 	  case FORMAT_DXR5:		return 4;
	  case FORMAT_RGB8:		return 3;
	  case FORMAT_RGBA8:	return 4;
	  default:
		llerrs << "LLImageDXT::Unknown format: " << format << llendl;
		return 0;
	}
};

// static 
LLImageDXT::EFileFormat LLImageDXT::getFormat(S32 fourcc)
{
	switch(fourcc)
	{
		case 0x20203849: return FORMAT_I8;
		case 0x20203841: return FORMAT_A8;
		case 0x20424752: return FORMAT_RGB8;
		case 0x41424752: return FORMAT_RGBA8;
		case 0x31525844: return FORMAT_DXR1;
		case 0x32525844: return FORMAT_DXR2;
		case 0x33525844: return FORMAT_DXR3;
		case 0x34525844: return FORMAT_DXR4;
		case 0x35525844: return FORMAT_DXR5;
		case 0x31545844: return FORMAT_DXT1;
		case 0x32545844: return FORMAT_DXT2;
		case 0x33545844: return FORMAT_DXT3;
		case 0x34545844: return FORMAT_DXT4;
		case 0x35545844: return FORMAT_DXT5;
		default: return FORMAT_UNKNOWN;
	}
}

//static
S32 LLImageDXT::getFourCC(EFileFormat format)
{
	switch(format)
	{
		case FORMAT_I8: return 0x20203849;
		case FORMAT_A8: return 0x20203841;
		case FORMAT_RGB8: return 0x20424752;
		case FORMAT_RGBA8: return 0x41424752;
		case FORMAT_DXR1: return 0x31525844;
		case FORMAT_DXR2: return 0x32525844;
		case FORMAT_DXR3: return 0x33525844;
		case FORMAT_DXR4: return 0x34525844;
		case FORMAT_DXR5: return 0x35525844;
		case FORMAT_DXT1: return 0x31545844;
		case FORMAT_DXT2: return 0x32545844;
		case FORMAT_DXT3: return 0x33545844;
		case FORMAT_DXT4: return 0x34545844;
		case FORMAT_DXT5: return 0x35545844;
		default: return 0x00000000;
	}
}

//static
void LLImageDXT::calcDiscardWidthHeight(S32 discard_level, EFileFormat format, S32& width, S32& height)
{
	while (discard_level > 0 && width > 1 && height > 1)
	{
		discard_level--;
		width >>= 1;
		height >>= 1;
	}
	checkMinWidthHeight(format, width, height);
}

//static
S32 LLImageDXT::calcNumMips(S32 width, S32 height)
{
	S32 nmips = 0;
	while (width > 0 && height > 0)
	{
		width >>= 1;
		height >>= 1;
		nmips++;
	}
	return nmips;
}

//============================================================================

LLImageDXT::LLImageDXT()
	: LLImageFormatted(IMG_CODEC_DXT),
	  mFileFormat(FORMAT_UNKNOWN),
	  mHeaderSize(0)
{
}

LLImageDXT::~LLImageDXT()
{
}

// virtual
BOOL LLImageDXT::updateData()
{
	resetLastError();

	U8* data = getData();
	S32 data_size = getDataSize();
	
	if (!data || !data_size)
	{
		setLastError("LLImageDXT uninitialized");
		return FALSE;
	}

	S32 width, height, miplevelmax;
	dxtfile_header_t* header = (dxtfile_header_t*)data;
	if (header->fourcc != 0x20534444)
	{
		dxtfile_header_old_t* oldheader = (dxtfile_header_old_t*)header;
		mHeaderSize = sizeof(dxtfile_header_old_t);
		mFileFormat = EFileFormat(oldheader->format);
		miplevelmax = llmin(oldheader->maxlevel,MAX_IMAGE_MIP);
		width = oldheader->maxwidth;
		height = oldheader->maxheight;
	}
	else
	{
		mHeaderSize = sizeof(dxtfile_header_t);
		mFileFormat = getFormat(header->pixel_fmt.fourcc);
		miplevelmax = llmin(header->num_mips-1,MAX_IMAGE_MIP);
		width = header->maxwidth;
		height = header->maxheight;
	}

	if (data_size < mHeaderSize)
	{
		llerrs << "LLImageDXT: not enough data" << llendl;
	}
	S32 ncomponents = formatComponents(mFileFormat);
	setSize(width, height, ncomponents);

	S32 discard = calcDiscardLevelBytes(data_size);
	discard = llmin(discard, miplevelmax);
	setDiscardLevel(discard);

	return TRUE;
}

// discard: 0 = largest (last) mip
S32 LLImageDXT::getMipOffset(S32 discard)
{
	if (mFileFormat >= FORMAT_DXT1 && mFileFormat <= FORMAT_DXT5)
	{
		llerrs << "getMipOffset called with old (unsupported) format" << llendl;
	}
	S32 width = getWidth(), height = getHeight();
	S32 num_mips = calcNumMips(width, height);
	discard = llclamp(discard, 0, num_mips-1);
	S32 last_mip = num_mips-1-discard;
	llassert(mHeaderSize > 0);
	S32 offset = mHeaderSize;
	for (S32 mipidx = num_mips-1; mipidx >= 0; mipidx--)
	{
		if (mipidx < last_mip)
		{
			offset += formatBytes(mFileFormat, width, height);
		}
		width >>= 1;
		height >>= 1;
	}
	return offset;
}

void LLImageDXT::setFormat()
{
	S32 ncomponents = getComponents();
	switch (ncomponents)
	{
	  case 3: mFileFormat = FORMAT_DXR1; break;
	  case 4: mFileFormat = FORMAT_DXR3; break;
	  default: llerrs << "LLImageDXT::setFormat called with ncomponents = " << ncomponents << llendl;
	}
	mHeaderSize = calcHeaderSize();
}
		
// virtual
BOOL LLImageDXT::decode(LLImageRaw* raw_image, F32 time)
{
	// *TODO: Test! This has been tweaked since its intial inception,
	//  but we don't use it any more!
	llassert_always(raw_image);
	
	if (mFileFormat >= FORMAT_DXT1 && mFileFormat <= FORMAT_DXR5)
	{
		llwarns << "Attempt to decode compressed LLImageDXT to Raw (unsupported)" << llendl;
		return FALSE;
	}
	
	S32 width = getWidth(), height = getHeight();
	S32 ncomponents = getComponents();
	U8* data = NULL;
	if (mDiscardLevel >= 0)
	{
		data = getData() + getMipOffset(mDiscardLevel);
		calcDiscardWidthHeight(mDiscardLevel, mFileFormat, width, height);
	}
	else
	{
		data = getData() + getMipOffset(0);
	}
	S32 image_size = formatBytes(mFileFormat, width, height);
	
	if ((!getData()) || (data + image_size > getData() + getDataSize()))
	{
		setLastError("LLImageDXT trying to decode an image with not enough data!");
		return FALSE;
	}

	raw_image->resize(width, height, ncomponents);
	memcpy(raw_image->getData(), data, image_size);	/* Flawfinder: ignore */

	return TRUE;
}

BOOL LLImageDXT::getMipData(LLPointer<LLImageRaw>& raw, S32 discard)
{
	if (discard < 0)
	{
		discard = mDiscardLevel;
	}
	else if (discard < mDiscardLevel)
	{
		llerrs << "Request for invalid discard level" << llendl;
	}
	U8* data = getData() + getMipOffset(discard);
	S32 width = 0;
	S32 height = 0;
	calcDiscardWidthHeight(discard, mFileFormat, width, height);
	raw = new LLImageRaw(data, width, height, getComponents());
	return TRUE;
}

BOOL LLImageDXT::encodeDXT(const LLImageRaw* raw_image, F32 time, bool explicit_mips)
{
	llassert_always(raw_image);
	
	S32 ncomponents = raw_image->getComponents();
	EFileFormat format;
	switch (ncomponents)
	{
	  case 1:
		format = FORMAT_A8;
		break;
	  case 3:
		format = FORMAT_RGB8;
		break;
	  case 4:
		format = FORMAT_RGBA8;
		break;
	  default:
		llerrs << "LLImageDXT::encode: Unhandled channel number: " << ncomponents << llendl;
		return 0;
	}

	S32 width = raw_image->getWidth();
	S32 height = raw_image->getHeight();

	if (explicit_mips)
	{
		height = (height/3)*2;
	}

	setSize(width, height, ncomponents);
	mHeaderSize = sizeof(dxtfile_header_t);
	mFileFormat = format;

	S32 nmips = calcNumMips(width, height);
	S32 w = width;
	S32 h = height;

	S32 totbytes = mHeaderSize;
	for (S32 mip=0; mip<nmips; mip++)
	{
		totbytes += formatBytes(format,w,h);
		w >>= 1;
		h >>= 1;
	}

	allocateData(totbytes);

	U8* data = getData();
	dxtfile_header_t* header = (dxtfile_header_t*)data;
	llassert(mHeaderSize > 0);
	memset(header, 0, mHeaderSize);
	header->fourcc = 0x20534444;
	header->pixel_fmt.fourcc = getFourCC(format);
	header->num_mips = nmips;
	header->maxwidth = width;
	header->maxheight = height;

	U8* prev_mipdata = 0;
	w = width, h = height;
	for (S32 mip=0; mip<nmips; mip++)
	{
		U8* mipdata = data + getMipOffset(mip);
		S32 bytes = formatBytes(format, w, h);
		if (mip==0)
		{
			memcpy(mipdata, raw_image->getData(), bytes);	/* Flawfinder: ignore */
		}
		else if (explicit_mips)
		{
			extractMip(raw_image->getData(), mipdata, width, height, w, h, format);
		}
		else
		{
			generateMip(prev_mipdata, mipdata, w, h, ncomponents);
		}
		w >>= 1;
		h >>= 1;
		checkMinWidthHeight(format, w, h);
		prev_mipdata = mipdata;
	}
	
	return TRUE;
}

// virtual
BOOL LLImageDXT::encode(const LLImageRaw* raw_image, F32 time)
{
	return encodeDXT(raw_image, time, false);
}

// virtual
bool LLImageDXT::convertToDXR()
{
	EFileFormat newformat = FORMAT_UNKNOWN;
	switch (mFileFormat)
	{
	  case FORMAT_DXR1:
	  case FORMAT_DXR2:
	  case FORMAT_DXR3:
	  case FORMAT_DXR4:
	  case FORMAT_DXR5:
		return false; // nothing to do
	  case FORMAT_DXT1: newformat = FORMAT_DXR1; break;
	  case FORMAT_DXT2: newformat = FORMAT_DXR2; break;
	  case FORMAT_DXT3: newformat = FORMAT_DXR3; break;
	  case FORMAT_DXT4: newformat = FORMAT_DXR4; break;
	  case FORMAT_DXT5: newformat = FORMAT_DXR5; break;
	  default:
		llwarns << "convertToDXR: can not convert format: " << llformat("0x%08x",getFourCC(mFileFormat)) << llendl;
		return false;
	}
	mFileFormat = newformat;
	S32 width = getWidth(), height = getHeight();
	S32 nmips = calcNumMips(width,height);
	S32 total_bytes = getDataSize();
	U8* olddata = getData();
	U8* newdata = (U8*)ALLOCATE_MEM(LLImageBase::getPrivatePool(), total_bytes);
	if (!newdata)
	{
		llerrs << "Out of memory in LLImageDXT::convertToDXR()" << llendl;
		return false;
	}
	llassert(total_bytes > 0);
	memset(newdata, 0, total_bytes);
	memcpy(newdata, olddata, mHeaderSize);	/* Flawfinder: ignore */
	for (S32 mip=0; mip<nmips; mip++)
	{
		S32 bytes = formatBytes(mFileFormat, width, height);
		S32 newoffset = getMipOffset(mip);
		S32 oldoffset = mHeaderSize + (total_bytes - newoffset - bytes);
		memcpy(newdata + newoffset, olddata + oldoffset, bytes);	/* Flawfinder: ignore */
		width >>= 1;
		height >>= 1;
	}
	dxtfile_header_t* header = (dxtfile_header_t*)newdata;
	header->pixel_fmt.fourcc = getFourCC(newformat);
	setData(newdata, total_bytes);
	updateData();
	return true;
}

// virtual
S32 LLImageDXT::calcHeaderSize()
{
	return llmax(sizeof(dxtfile_header_old_t), sizeof(dxtfile_header_t));
}

// virtual
S32 LLImageDXT::calcDataSize(S32 discard_level)
{
	if (mFileFormat == FORMAT_UNKNOWN)
	{
		llerrs << "calcDataSize called with unloaded LLImageDXT" << llendl;
		return 0;
	}
	if (discard_level < 0)
	{
		discard_level = mDiscardLevel;
	}
	S32 bytes = getMipOffset(discard_level); // size of header + previous mips
	S32 w = getWidth() >> discard_level;
	S32 h = getHeight() >> discard_level;
	bytes += formatBytes(mFileFormat,w,h);
	return bytes;
}

//============================================================================

//static
void LLImageDXT::extractMip(const U8 *indata, U8* mipdata, int width, int height,
							int mip_width, int mip_height, EFileFormat format)
{
	int initial_offset = formatBytes(format, width, height);
	int line_width = formatBytes(format, width, 1);
	int mip_line_width = formatBytes(format, mip_width, 1);
	int line_offset = 0;

	for (int ww=width>>1; ww>mip_width; ww>>=1)
	{
		line_offset += formatBytes(format, ww, 1);
	}

	for (int h=0;h<mip_height;++h)
	{
		int start_offset = initial_offset + line_width * h + line_offset;
		memcpy(mipdata + mip_line_width*h, indata + start_offset, mip_line_width);	/* Flawfinder: ignore */
	}
}

//============================================================================
