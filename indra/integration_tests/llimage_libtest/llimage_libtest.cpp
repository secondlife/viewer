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

// doc string provided when invoking the program with --help 
static const char USAGE[] = "\n"
"usage:\tllimage_libtest [options]\n"
"\n"
" --help                           print this help\n"
" --in <file1 .. file2>            list of image files to load and convert, patterns can be used\n"
" --out <file1 .. file2> OR <type> list of image files to create (assumes same order as --in files)\n"
"                                  OR 3 letters file type extension to convert each input file into\n"
"\n";

// Create an empty formatted image instance of the correct type from the filename
LLPointer<LLImageFormatted> create_image(const std::string &filename)
{
	std::string exten = gDirUtilp->getExtension(filename);
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
	
	return image;
}

// Load an image from file and return a raw (decompressed) instance of its data
LLPointer<LLImageRaw> load_image(const std::string &src_filename)
{
	LLPointer<LLImageFormatted> image = create_image(src_filename);

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

// Save a raw image instance into a file
bool save_image(const std::string &dest_filename, LLPointer<LLImageRaw> raw_image)
{
	LLPointer<LLImageFormatted> image = create_image(dest_filename);
	
	if (!image->encode(raw_image, 0.0f))
	{
		return false;
	}
	
	return image->save(dest_filename);
}

void store_input_file(std::list<std::string> &input_filenames, const std::string &path)
{
	// Break the incoming path in its components
	std::string dir = gDirUtilp->getDirName(path);
	std::string name = gDirUtilp->getBaseFileName(path);
	std::string exten = gDirUtilp->getExtension(path);

	// std::cout << "store_input_file : " << path << ", dir : " << dir << ", name : " << name << ", exten : " << exten << std::endl;
	
	// If extension is not an image type or "*", exit
	// Note: we don't support complex patterns for the extension like "j??"
	// Note: on most shells, the pattern expansion is done by the shell so that pattern matching limitation is actually not a problem
	if ((exten.compare("*") != 0) && (LLImageBase::getCodecFromExtension(exten) == IMG_CODEC_INVALID))
	{
		return;
	}

	if ((name.find('*') != -1) || ((name.find('?') != -1)))
	{
		// If file name is a pattern, iterate to get each file name and store
		std::string next_name;
		while (gDirUtilp->getNextFileInDir(dir,name,next_name))
		{
			std::string file_name = dir + gDirUtilp->getDirDelimiter() + next_name;
			input_filenames.push_back(file_name);
		}
	}
	else
	{
		// Verify that the file does exist before storing 
		if (gDirUtilp->fileExists(path))
		{
			input_filenames.push_back(path);
		}
		else
		{
			std::cout << "store_input_file : the file " << path << " could not be found" << std::endl;
		}
	}	
}

void store_output_file(std::list<std::string> &output_filenames, std::list<std::string> &input_filenames, const std::string &path)
{
	// Break the incoming path in its components
	std::string dir = gDirUtilp->getDirName(path);
	std::string name = gDirUtilp->getBaseFileName(path);
	std::string exten = gDirUtilp->getExtension(path);
	
	// std::cout << "store_output_file : " << path << ", dir : " << dir << ", name : " << name << ", exten : " << exten << std::endl;
	
	if (dir.empty() && exten.empty())
	{
		// If dir and exten are empty, we interpret the name as a file extension type name and will iterate through input list to populate the output list
		exten = name;
		// Make sure the extension is an image type
		if (LLImageBase::getCodecFromExtension(exten) == IMG_CODEC_INVALID)
		{
			return;
		}
		std::string delim = gDirUtilp->getDirDelimiter();
		std::list<std::string>::iterator in_file  = input_filenames.begin();
		std::list<std::string>::iterator end = input_filenames.end();
		for (; in_file != end; ++in_file)
		{
			dir = gDirUtilp->getDirName(*in_file);
			name = gDirUtilp->getBaseFileName(*in_file,true);
			std::string file_name = dir + delim + name + "." + exten;
			output_filenames.push_back(file_name);
		}
	}
	else
	{
		// Make sure the extension is an image type
		if (LLImageBase::getCodecFromExtension(exten) == IMG_CODEC_INVALID)
		{
			return;
		}
		// Store the path
		output_filenames.push_back(path);
	}
}

int main(int argc, char** argv)
{
	// List of input and output files
	std::list<std::string> input_filenames;
	std::list<std::string> output_filenames;

	// Init whatever is necessary
	ll_init_apr();
	LLImage::initClass();

	// Analyze command line arguments
	for (int arg = 1; arg < argc; ++arg)
	{
		if (!strcmp(argv[arg], "--help"))
		{
            // Send the usage to standard out
            std::cout << USAGE << std::endl;
			return 0;
		}
		else if (!strcmp(argv[arg], "--in") && arg < argc-1)
		{
			std::string file_name = argv[arg+1];
			while (file_name[0] != '-')		// if arg starts with '-', we consider it's not a file name but some other argument
			{
				// std::cout << "input file name : " << file_name << std::endl;				
				store_input_file(input_filenames, file_name);
				arg += 1;					// Skip that arg now we know it's a file name
				if ((arg + 1) == argc)		// Break out of the loop if we reach the end of the arg list
					break;
				file_name = argv[arg+1];	// Next argument and loop over
			}
		}
		else if (!strcmp(argv[arg], "--out") && arg < argc-1)
		{
			std::string file_name = argv[arg+1];
			while (file_name[0] != '-')		// if arg starts with '-', we consider it's not a file name but some other argument
			{
				// std::cout << "output file name : " << file_name << std::endl;				
				store_output_file(output_filenames, input_filenames, file_name);
				arg += 1;					// Skip that arg now we know it's a file name
				if ((arg + 1) == argc)		// Break out of the loop if we reach the end of the arg list
					break;
				file_name = argv[arg+1];	// Next argument and loop over
			}
		}		
	}
		
	// Analyze the list of (input,output) files
	if (input_filenames.size() == 0)
	{
		std::cout << "No input file, nothing to do -> exit" << std::endl;
		return 0;
	}

	// Perform action on each input file
	std::list<std::string>::iterator in_file  = input_filenames.begin();
	std::list<std::string>::iterator out_file = output_filenames.begin();
	std::list<std::string>::iterator in_end = input_filenames.end();
	std::list<std::string>::iterator out_end = output_filenames.end();
	for (; in_file != in_end; ++in_file)
	{
		// Load file
		LLPointer<LLImageRaw> raw_image = load_image(*in_file);
		if (!raw_image)
		{
			std::cout << "Error: Image " << *in_file << " could not be loaded" << std::endl;
			continue;
		}
	
		// Save file
		if (out_file != out_end)
		{
			if (!save_image(*out_file, raw_image))
			{
				std::cout << "Error: Image " << *out_file << " could not be saved" << std::endl;
			}
			else
			{
				std::cout << *in_file << " -> " << *out_file << std::endl;
			}
			++out_file;
		}

		// Output stats on each file
	}
	
	// Output perf data if required by user
	
	// Cleanup and exit
	LLImage::cleanupClass();
	
	return 0;
}
