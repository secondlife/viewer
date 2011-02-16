/** 
 * @file llimage_libtest.cpp
 * @author Merov Linden
 * @brief Integration test for the llimage library
 *
 * $LicenseInfo:firstyear=2011&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2011, Linden Research, Inc.
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
#include "llpointer.h"

#include "llimage_libtest.h"

// Linden library includes
#include "llimage.h"
#include "llimagejpeg.h"
#include "llimagepng.h"
#include "llimagebmp.h"
#include "llimagetga.h"
#include "llimagej2c.h"
#include "lldir.h"

// system libraries
#include <iostream>

static const char USAGE[] = "\n"
"usage:\tllimage_libtest [options]\n"
"\n"
" --help                         print this help\n"
"\n";

LLPointer<LLImageRaw> load_image(const std::string &src_filename)
{
	std::string exten = gDirUtilp->getExtension(src_filename);
	U32 codec = LLImageBase::getCodecFromExtension(exten);
	
	LLPointer<LLImageFormatted> image;
	switch (codec)
	{
		case IMG_CODEC_BMP:
			image = new LLImageBMP();
			break;
		case IMG_CODEC_TGA:
			image = new LLImageTGA();
			break;
		case IMG_CODEC_JPEG:
			image = new LLImageJPEG();
			break;
		case IMG_CODEC_J2C:
			image = new LLImageJ2C();
			break;
		case IMG_CODEC_PNG:
			image = new LLImagePNG();
			break;
		default:
			return NULL;
	}

	if (!image->load(src_filename))
	{
		return NULL;
	}
	
	if(	(image->getComponents() != 3) && (image->getComponents() != 4) )
	{
		std::cout << "Image files with less than 3 or more than 4 components are not supported\n";
		return NULL;
	}
	
	LLPointer<LLImageRaw> raw_image = new LLImageRaw;
	if (!image->decode(raw_image, 0.0f))
	{
		return NULL;
	}
	
	return raw_image;
}

bool save_image(const std::string &filepath, LLPointer<LLImageRaw> raw_image)
{
	std::string exten = gDirUtilp->getExtension(filepath);
	U32 codec = LLImageBase::getCodecFromExtension(exten);

	LLPointer<LLImageFormatted> image;
	switch (codec)
	{
		case IMG_CODEC_BMP:
			image = new LLImageBMP();
			break;
		case IMG_CODEC_TGA:
			image = new LLImageTGA();
			break;
		case IMG_CODEC_JPEG:
			image = new LLImageJPEG();
			break;
		case IMG_CODEC_J2C:
			image = new LLImageJ2C();
			break;
		case IMG_CODEC_PNG:
			image = new LLImagePNG();
			break;
		default:
			return NULL;
	}
	
	if (!image->encode(raw_image, 0.0f))
	{
		return false;
	}
	
	return image->save(filepath);
}

int main(int argc, char** argv)
{
	// Init whatever is necessary
	ll_init_apr();
	LLImage::initClass();

	// Analyze command line arguments
	for (int arg=1; arg<argc; ++arg)
	{
		if (!strcmp(argv[arg], "--help"))
		{
            // always send the usage to standard out
            std::cout << USAGE << std::endl;
			return 0;
		}
		
	}
		
	// List of (input,output) files
	
	// Load file
	LLPointer<LLImageRaw> raw_image = load_image("lolcat-monorail.jpg");
	if (raw_image)
	{
		std::cout << "Image loaded\n" << std::endl;
	}
	else
	{
		std::cout << "Image not found\n" << std::endl;
	}
	
	// Save file
	if (raw_image)
	{
		save_image("monorail.png",raw_image);
	}
		
	// Output stats on each file
	
	// Cleanup and exit
	LLImage::cleanupClass();
	
	return 0;
}
