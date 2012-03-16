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
#include "lltimer.h"

#include "llimage_libtest.h"

// Linden library includes
#include "llimage.h"
#include "llimagejpeg.h"
#include "llimagepng.h"
#include "llimagebmp.h"
#include "llimagetga.h"
#include "llimagej2c.h"
#include "lldir.h"
#include "lldiriterator.h"

// system libraries
#include <iostream>

// doc string provided when invoking the program with --help 
static const char USAGE[] = "\n"
"usage:\tllimage_libtest [options]\n"
"\n"
" -h, --help\n"
"        Print this help\n"
" -i, --input <file1 .. file2>\n"
"        List of image files to load and convert. Patterns with wild cards can be used.\n"
" -o, --output <file1 .. file2> OR <type>\n"
"        List of image files to create (assumes same order as for input files)\n"
"        OR 3 letters file type extension to convert each input file into.\n"
" -load, --load_size <n>\n"
"        Portion of the input file to load, in bytes."
"        If (load == 0), it will load the whole file."
"        If (load == -1), it will load the size relevant to reach the requested discard level (see -d)."
"        Only valid for j2c images. Default is 0 (load whole file).\n"
" -r, --region <x0, y0, x1, y1>\n"
"        Crop region applied to the input files in pixels.\n"
"        Only used for j2c images. Default is no region cropping.\n"
" -d, --discard_level <n>\n"
"        Discard level max used on input. 0 is highest resolution. Max discard level is 5.\n"
"        This allows the input image to be clamped in resolution when loading.\n"
"        Only valid for j2c images. Default is no discard.\n"
" -p, --precincts <n>\n"
"        Dimension of precincts in pixels. Precincts are assumed square and identical for\n"
"        all levels. Note that this option also add PLT and tile markers to the codestream, \n"
"        and uses RPCL order. Power of 2 must be used.\n"
"        Only valid for output j2c images. Default is no precincts used.\n"
" -b, --blocks <n>\n"
"        Dimension of coding blocks in pixels. Blocks are assumed square. Power of 2 must\n"
"        be used. Blocks must be smaller than precincts. Like precincts, this option adds\n"
"        PLT, tile markers and uses RPCL.\n"
"        Only valid for output j2c images. Default is 64.\n"
" -l, --levels <n>\n"
"        Number of decomposition levels (aka discard levels) in the output image.\n"
"        The maximum number of levels authorized is 32.\n"
"        Only valid for output j2c images. Default is 5.\n"
" -rev, --reversible\n"
"        Set the compression to be lossless (reversible in j2c parlance).\n"
"        Only valid for output j2c images.\n"
" -log, --logmetrics <metric>\n"
"        Log performance data for <metric>. Results in <metric>.slp\n"
"        Note: so far, only ImageCompressionTester has been tested.\n"
" -a, --analyzeperformance\n"
"        Create a report comparing <metric>_baseline.slp with current <metric>.slp\n"
"        Results in <metric>_report.csv\n"
" -s, --image-stats\n"
"        Output stats for each input and output image.\n"
"\n";

// true when all image loading is done. Used by metric logging thread to know when to stop the thread.
static bool sAllDone = false;

// Create an empty formatted image instance of the correct type from the filename
LLPointer<LLImageFormatted> create_image(const std::string &filename)
{
	std::string exten = gDirUtilp->getExtension(filename);	
	LLPointer<LLImageFormatted> image = LLImageFormatted::createFromExtension(exten);
	return image;
}

void output_image_stats(LLPointer<LLImageFormatted> image, const std::string &filename)
{
	// Print out some statistical data on the image
	std::cout << "Image stats for : " << filename << ", extension : " << image->getExtension() << std::endl;

	std::cout << "    with : " << (int)(image->getWidth())       << ", height : " << (int)(image->getHeight())   << std::endl;
	std::cout << "    comp : " << (int)(image->getComponents())  << ", levels : " << (int)(image->getLevels())   << std::endl;
	std::cout << "    head : " << (int)(image->calcHeaderSize()) << ",   data : " << (int)(image->getDataSize()) << std::endl;

	return;
}

// Load an image from file and return a raw (decompressed) instance of its data
LLPointer<LLImageRaw> load_image(const std::string &src_filename, int discard_level, int* region, int load_size, bool output_stats)
{
	LLPointer<LLImageFormatted> image = create_image(src_filename);
	
	// We support partial loading only for j2c images
	if (image->getCodec() == IMG_CODEC_J2C)
	{
		// Load the header
		if (!image->load(src_filename, 600))
		{
			return NULL;
		}
		S32 h = ((LLImageJ2C*)(image.get()))->calcHeaderSize();
		S32 d = (load_size > 0 ? ((LLImageJ2C*)(image.get()))->calcDiscardLevelBytes(load_size) : 0);
		S8  r = ((LLImageJ2C*)(image.get()))->getRawDiscardLevel();
		std::cout << "Merov debug : header = " << h << ", load_size = " << load_size << ", discard level = " << d << ", raw discard level = " << r << std::endl;
		for (d = 0; d < MAX_DISCARD_LEVEL; d++)
		{
			S32 data_size = ((LLImageJ2C*)(image.get()))->calcDataSize(d);
			std::cout << "Merov debug : discard_level = " << d << ", data_size = " << data_size << std::endl;
		}
		if (load_size < 0)
		{
			load_size = (discard_level != -1 ? ((LLImageJ2C*)(image.get()))->calcDataSize(discard_level) : 0);
		}
		// Load the requested byte range
		if (!image->load(src_filename, load_size))
		{
			return NULL;
		}
	}
	else 
	{
		// This just loads the image file stream into a buffer. No decoding done.
		if (!image->load(src_filename))
		{
			return NULL;
		}
	}
	
	if(	(image->getComponents() != 3) && (image->getComponents() != 4) )
	{
		std::cout << "Image files with less than 3 or more than 4 components are not supported\n";
		return NULL;
	}
	
	if (output_stats)
	{
		output_image_stats(image, src_filename);
	}
	
	LLPointer<LLImageRaw> raw_image = new LLImageRaw;
	
	// Set the image restriction on load in the case of a j2c image
	if ((image->getCodec() == IMG_CODEC_J2C) && ((discard_level != -1) || (region != NULL)))
	{
		// That method doesn't exist (and likely, doesn't make sense) for any other image file format
		// hence the required cryptic cast.
		((LLImageJ2C*)(image.get()))->initDecode(*raw_image, discard_level, region);
	}
	
	if (!image->decode(raw_image, 0.0f))
	{
		return NULL;
	}
	
	return raw_image;
}

// Save a raw image instance into a file
bool save_image(const std::string &dest_filename, LLPointer<LLImageRaw> raw_image, int blocks_size, int precincts_size, int levels, bool reversible, bool output_stats)
{
	LLPointer<LLImageFormatted> image = create_image(dest_filename);
	
	// Set the image codestream parameters on output in the case of a j2c image
	if (image->getCodec() == IMG_CODEC_J2C)
	{
		// That method doesn't exist (and likely, doesn't make sense) for any other image file format
		// hence the required cryptic cast.
		if ((blocks_size != -1) || (precincts_size != -1) || (levels != 0))
		{
			((LLImageJ2C*)(image.get()))->initEncode(*raw_image, blocks_size, precincts_size, levels);
		}
		((LLImageJ2C*)(image.get()))->setReversible(reversible);
	}
	
	if (!image->encode(raw_image, 0.0f))
	{
		return false;
	}
	
	if (output_stats)
	{
		output_image_stats(image, dest_filename);
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
		LLDirIterator iter(dir, name);
		while (iter.next(next_name))
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
			std::string file_name;
			if (!dir.empty())
			{
				file_name = dir + delim + name + "." + exten;
			}
			else
			{
				file_name = name + "." + exten;
			}
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

// Holds the metric gathering output in a thread safe way
class LogThread : public LLThread
{
public:
	std::string mFile;

	LogThread(std::string& test_name) : LLThread("llimage_libtest log")
	{
		std::string file_name = test_name + std::string(".slp");
		mFile = file_name;
	}
		
	void run()
	{
		std::ofstream os(mFile.c_str());
			
		while (!sAllDone)
		{
			LLFastTimer::writeLog(os);
			os.flush();
			ms_sleep(32);
		}
		LLFastTimer::writeLog(os);
		os.flush();
		os.close();
	}		
};

int main(int argc, char** argv)
{
	// List of input and output files
	std::list<std::string> input_filenames;
	std::list<std::string> output_filenames;
	// Other optional parsed arguments
	bool analyze_performance = false;
	bool image_stats = false;
	int* region = NULL;
	int discard_level = -1;
	int load_size = 0;
	int precincts_size = -1;
	int blocks_size = -1;
	int levels = 0;
	bool reversible = false;

	// Init whatever is necessary
	ll_init_apr();
	LLImage::initClass();
	LogThread* fast_timer_log_thread = NULL;	// For performance and metric gathering

	// Analyze command line arguments
	for (int arg = 1; arg < argc; ++arg)
	{
		if (!strcmp(argv[arg], "--help") || !strcmp(argv[arg], "-h"))
		{
            // Send the usage to standard out
            std::cout << USAGE << std::endl;
			return 0;
		}
		else if ((!strcmp(argv[arg], "--input") || !strcmp(argv[arg], "-i")) && arg < argc-1)
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
		else if ((!strcmp(argv[arg], "--output") || !strcmp(argv[arg], "-o")) && arg < argc-1)
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
		else if ((!strcmp(argv[arg], "--region") || !strcmp(argv[arg], "-r")) && arg < argc-1)
		{
			std::string value_str = argv[arg+1];
			int index = 0;
			region = new int[4];
			while (value_str[0] != '-')		// if arg starts with '-', it's the next option
			{
				int value = atoi(value_str.c_str());
				region[index++] = value;
				arg += 1;					// Definitely skip that arg now we know it's a number
				if ((arg + 1) == argc)		// Break out of the loop if we reach the end of the arg list
					break;
				if (index == 4)				// Break out of the loop if we captured 4 values already
					break;
				value_str = argv[arg+1];	// Next argument and loop over
			}
			if (index != 4)
			{
				std::cout << "--region arguments invalid" << std::endl;
				delete [] region;
				region = NULL;
			}
		}
		else if (!strcmp(argv[arg], "--discard_level") || !strcmp(argv[arg], "-d"))
		{
			std::string value_str;
			if ((arg + 1) < argc)
			{
				value_str = argv[arg+1];
			}
			if (((arg + 1) >= argc) || (value_str[0] == '-'))
			{
				std::cout << "No valid --discard_level argument given, discard_level ignored" << std::endl;
			}
			else
			{
				discard_level = atoi(value_str.c_str());
				// Clamp to the values accepted by the viewer
				discard_level = llclamp(discard_level,0,5);
			}
		}
		else if (!strcmp(argv[arg], "--load_size") || !strcmp(argv[arg], "-load"))
		{
			std::string value_str;
			if ((arg + 1) < argc)
			{
				value_str = argv[arg+1];
			}
			if (((arg + 1) >= argc) || (value_str[0] == '-'))
			{
				std::cout << "No valid --load_size argument given, load_size ignored" << std::endl;
			}
			else
			{
				load_size = atoi(value_str.c_str());
			}
		}
		else if (!strcmp(argv[arg], "--precincts") || !strcmp(argv[arg], "-p"))
		{
			std::string value_str;
			if ((arg + 1) < argc)
			{
				value_str = argv[arg+1];
			}
			if (((arg + 1) >= argc) || (value_str[0] == '-'))
			{
				std::cout << "No valid --precincts argument given, precincts ignored" << std::endl;
			}
			else
			{
				precincts_size = atoi(value_str.c_str());
			}
		}
		else if (!strcmp(argv[arg], "--blocks") || !strcmp(argv[arg], "-b"))
		{
			std::string value_str;
			if ((arg + 1) < argc)
			{
				value_str = argv[arg+1];
			}
			if (((arg + 1) >= argc) || (value_str[0] == '-'))
			{
				std::cout << "No valid --blocks argument given, blocks ignored" << std::endl;
			}
			else
			{
				blocks_size = atoi(value_str.c_str());
			}
		}
		else if (!strcmp(argv[arg], "--levels") || !strcmp(argv[arg], "-l"))
		{
			std::string value_str;
			if ((arg + 1) < argc)
			{
				value_str = argv[arg+1];
			}
			if (((arg + 1) >= argc) || (value_str[0] == '-'))
			{
				std::cout << "No valid --levels argument given, default (5) will be used" << std::endl;
			}
			else
			{
				levels = atoi(value_str.c_str());
			}
		}
		else if (!strcmp(argv[arg], "--reversible") || !strcmp(argv[arg], "-rev"))
		{
			reversible = true;
		}
		else if (!strcmp(argv[arg], "--logmetrics") || !strcmp(argv[arg], "-log"))
		{
			// '--logmetrics' needs to be specified with a named test metric argument
			// Note: for the moment, only ImageCompressionTester has been tested
			std::string test_name;
			if ((arg + 1) < argc)
			{
				test_name = argv[arg+1];
			}
			if (((arg + 1) >= argc) || (test_name[0] == '-'))
			{
				// We don't have an argument left in the arg list or the next argument is another option
				std::cout << "No --logmetrics argument given, no perf data will be gathered" << std::endl;
			}
			else
			{
				LLFastTimer::sMetricLog = TRUE;
				LLFastTimer::sLogName = test_name;
				arg += 1;					// Skip that arg now we know it's a valid test name
				if ((arg + 1) == argc)		// Break out of the loop if we reach the end of the arg list
					break;
			}
		}
		else if (!strcmp(argv[arg], "--analyzeperformance") || !strcmp(argv[arg], "-a"))
		{
			analyze_performance = true;
		}
		else if (!strcmp(argv[arg], "--image-stats") || !strcmp(argv[arg], "-s"))
		{
			image_stats = true;
		}
	}
		
	// Check arguments consistency. Exit with proper message if inconsistent.
	if (input_filenames.size() == 0)
	{
		std::cout << "No input file, nothing to do -> exit" << std::endl;
		return 0;
	}
	if (analyze_performance && !LLFastTimer::sMetricLog)
	{
		std::cout << "Cannot create perf report if no perf gathered (i.e. use argument -log <perf> with -a) -> exit" << std::endl;
		return 0;
	}
	

	// Create the logging thread if required
	if (LLFastTimer::sMetricLog)
	{
		LLFastTimer::sLogLock = new LLMutex(NULL);
		fast_timer_log_thread = new LogThread(LLFastTimer::sLogName);
		fast_timer_log_thread->start();
	}
	
	// Perform action on each input file
	std::list<std::string>::iterator in_file  = input_filenames.begin();
	std::list<std::string>::iterator out_file = output_filenames.begin();
	std::list<std::string>::iterator in_end = input_filenames.end();
	std::list<std::string>::iterator out_end = output_filenames.end();
	for (; in_file != in_end; ++in_file, ++out_file)
	{
		// Load file
		LLPointer<LLImageRaw> raw_image = load_image(*in_file, discard_level, region, load_size, image_stats);
		if (!raw_image)
		{
			std::cout << "Error: Image " << *in_file << " could not be loaded" << std::endl;
			continue;
		}
	
		// Save file
		if (out_file != out_end)
		{
			if (!save_image(*out_file, raw_image, blocks_size, precincts_size, levels, reversible, image_stats))
			{
				std::cout << "Error: Image " << *out_file << " could not be saved" << std::endl;
			}
			else
			{
				std::cout << *in_file << " -> " << *out_file << std::endl;
			}
		}
	}

	// Output perf data if requested by user
	if (analyze_performance)
	{
		std::string baseline_name = LLFastTimer::sLogName + "_baseline.slp";
		std::string current_name  = LLFastTimer::sLogName + ".slp"; 
		std::string report_name   = LLFastTimer::sLogName + "_report.csv";
		
		std::cout << "Analyzing performance, check report in : " << report_name << std::endl;

		LLMetricPerformanceTesterBasic::doAnalysisMetrics(baseline_name, current_name, report_name);
	}
	
	// Stop the perf gathering system if needed
	if (LLFastTimer::sMetricLog)
	{
		LLMetricPerformanceTesterBasic::deleteTester(LLFastTimer::sLogName);
		sAllDone = true;
	}
	
	// Cleanup and exit
	LLImage::cleanupClass();
	if (fast_timer_log_thread)
	{
		fast_timer_log_thread->shutdown();
	}
	
	return 0;
}
