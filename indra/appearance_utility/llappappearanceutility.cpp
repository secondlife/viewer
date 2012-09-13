/** 
 * @file llappappearanceutility.cpp
 * @brief Implementation of LLAppAppearanceUtility class.
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2012, Linden Research, Inc.
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

#include <iostream>
#include <sstream>
#include <string>

// linden includes
#include "linden_common.h"

#include "llapr.h"
#include "llerrorcontrol.h"
#include "llsd.h"
#include "llsdserialize.h"
#include "llsdutil.h"

// appearance includes
#include "llavatarappearance.h"
#include "llwearabletype.h"

// project includes
#include "llappappearanceutility.h"
#include "llbakingprocess.h"
#include "llprocessparams.h"

const std::string NOTHING_EXTRA("");

////////////////////////////////////////////
// LLAppException
////////////////////////////////////////////

static const std::string MESSAGE_RV_UNKNOWN("Unknown error.");
static const std::string MESSAGE_RV_ARGUMENTS 
("Invalid arguments: ");
static const std::string MESSAGE_RV_UNABLE_OPEN("Unable to open file: ");
static const std::string MESSAGE_RV_UNABLE_TO_PARSE("Unable to parse input LLSD.");
static const std::string MESSAGE_DUPLICATE_MODES = "Cannot specify more than one process mode.";


LLAppException::LLAppException(EResult status_code, const std::string& extra) :
	mStatusCode(status_code)
{
	switch(status_code)
	{
	case RV_UNKNOWN_ERROR:
		printErrorLLSD("unknown", MESSAGE_RV_UNKNOWN);
	case RV_BAD_ARGUMENTS:
		printErrorLLSD("arguments", MESSAGE_RV_ARGUMENTS + extra);
		break;
	case RV_UNABLE_OPEN:
		printErrorLLSD("file", MESSAGE_RV_UNABLE_OPEN + extra);
		break;
	case RV_UNABLE_TO_PARSE:
		printErrorLLSD("input", MESSAGE_RV_UNABLE_TO_PARSE);
		break;
	default:
		printErrorLLSD("arguments", "Unknown exception.");
		break;
	}
}

void LLAppException::printErrorLLSD(const std::string& key, const std::string& message)
{
	LLSD error_llsd;
	error_llsd["success"] = false;
	error_llsd["error"]["key"] = key;
	error_llsd["error"]["message"] = message;

	std::cerr << LLSDOStreamer<LLSDXMLFormatter>(error_llsd);
}



////////////////////////////////////////////
// LLAppAppearanceUtility
////////////////////////////////////////////

///////// Option Parsing /////////

// Simple usage command.
class LLProcessUsage : public LLBakingProcess
{
public:
	LLProcessUsage(LLAppAppearanceUtility* app) :
		LLBakingProcess(app) {}
	/*virtual*/ void process(LLSD& input, std::ostream& output)
	{
		mApp->usage(output);
	}
};


static const apr_getopt_option_t APPEARANCE_UTILITY_OPTIONS[] =
{
	{"params", 'p', 0, "Generate appearance parameters for an agent."},
	{"output", 'o', 1, "The output file to write to.  Default is stdout"},
	{"agent-id", 'a', 1, "The agent-id of the user."},
	//{"grid", 'g', 1, "The grid."},
	{"help", 'h', 0, "Print the help message."},
	{0, 0, 0, 0}
};

void LLAppAppearanceUtility::usage(std::ostream& ostr)
{
	ostr << "Utilities for processing agent appearance data."
		<< std::endl << std::endl
		<< "Usage:" << std::endl
		<< "\t" << mAppName << " [options] filename" << std::endl << std::endl
		<< "Will read from stdin if filename is set to '-'." << std::endl << std::endl
		<< "Options:" << std::endl;
	const apr_getopt_option_t* option = &APPEARANCE_UTILITY_OPTIONS[0];
	while(option->name)
	{
		ostr << "\t--" << option->name << "\t\t"
			<< option->description << std::endl;
		++option;
	}
	ostr << std::endl << "Return Values:" << std::endl
		<< "\t0\t\tSuccess." << std::endl
		<< "\t1\t\tUnknown error." << std::endl
		<< "\t2\t\tBad arguments." << std::endl
		<< "\t3\t\tUnable to open file. Possibly wrong filename"
		<< " or bad permissions." << std::endl
		<< "\t4\t\tUnable to parse input LLSD." << std::endl
		<< std::endl
		<< "Output:" << std::endl
		<< "If a non-zero status code is returned, additional error information"
		<< " will be returned on stderr." << std::endl
		<< "* This will be in the form of an LLSD document." << std::endl
		<< "* Check ['error']['message'] to get a human readable message." << std::endl
		<< "If a zero status code is returned, processed output will be written"
		<< " to the file specified by --out (or stdout, if not specified)." << std::endl
		<< std::endl
		<< std::endl;
}


/////// LLApp Interface ////////

LLAppAppearanceUtility::LLAppAppearanceUtility(int argc, char** argv) :
	LLApp(),
	mArgc(argc),
	mArgv(argv),
	mProcess(NULL),
	mInput(NULL),
	mOutput(NULL),
	mAppName(argv[0])
{
}

// virtual
LLAppAppearanceUtility::~LLAppAppearanceUtility()
{
}

void LLAppAppearanceUtility::verifyNoProcess()
{
	if (mProcess)
	{
		std::cerr << "Invalid arguments. " << MESSAGE_DUPLICATE_MODES << std::endl;
		usage(std::cerr);
		throw LLAppException(RV_BAD_ARGUMENTS, MESSAGE_DUPLICATE_MODES);
	}
}

void LLAppAppearanceUtility::parseArguments()
{
	////// BEGIN OPTION PARSING //////
	// Check for '-' as last option, since apr doesn't seem to like that.
	if (std::string(mArgv[mArgc-1]) == "-")
	{
		mInputFilename.assign("-");
		mArgc--;
	}

	apr_status_t apr_err;
	const char* opt_arg = NULL;
	int opt_id = 0;
	apr_getopt_t* os = NULL;
	if(APR_SUCCESS != apr_getopt_init(&os, gAPRPoolp, mArgc, mArgv))
	{
		std::cerr << "Unable to initialize apr" << std::endl;
		throw LLAppException(RV_UNKNOWN_ERROR);
	}

	//std::string grid;
	while(true)
	{
		apr_err = apr_getopt_long(os, APPEARANCE_UTILITY_OPTIONS, &opt_id, &opt_arg);
		if(APR_STATUS_IS_EOF(apr_err)) break;
		if(apr_err)
		{
			char buf[MAX_STRING];		/* Flawfinder: ignore */
			std::cerr << "Error parsing options: "
				<< apr_strerror(apr_err, buf, MAX_STRING) << std::endl;
			usage(std::cerr);
			throw LLAppException(RV_BAD_ARGUMENTS, buf);
		}
		switch (opt_id)
		{
		case 'h':
			verifyNoProcess();
			mProcess = new LLProcessUsage(this);
			break;
		case 'p':
			verifyNoProcess();
			mProcess = new LLProcessParams(this);
			break;
		case 'o':
			mOutputFilename.assign(opt_arg);
			break;
		case 'a':
			mAgentID.set(opt_arg);
			if (mAgentID.isNull())
			{
				const char* INVALID_AGENT_ID="agent-id must be a valid uuid.";
				std::cerr << "Invalid arguments. " << INVALID_AGENT_ID << std::endl;
				usage(std::cerr);
				throw LLAppException(RV_BAD_ARGUMENTS, INVALID_AGENT_ID);
			}
			break;
		//case 'g':
		//	grid = opt_arg;
		//	break;
		default:
			usage(std::cerr);
			throw LLAppException(RV_BAD_ARGUMENTS, "Unknown option.");
		}
	}

	if ("-" != mInputFilename)
	{
		bool valid_input_filename = false;
		// Try to grab the input filename.
		if (os->argv && os->argv[os->ind])
		{
			mInputFilename.assign(os->argv[os->ind]);
			if (! mInputFilename.empty() )
			{
				valid_input_filename = true;
			}
		}
		if (!valid_input_filename)
		{
			const char* INVALID_FILENAME="Must specify input file.";
			std::cerr << "Invalid arguments. " << INVALID_FILENAME << std::endl;
			usage(std::cerr);
			throw LLAppException(RV_BAD_ARGUMENTS, INVALID_FILENAME);
		}
	}

	////// END OPTION PARSING //////
}

void LLAppAppearanceUtility::validateArguments()
{
	/////  BEGIN ARGUMENT VALIDATION /////

	// Make sure we have a command specified.
	if (!mProcess)
	{
		const char* INVALID_MODE="No process mode specified.";
		std::cerr << "Invalid arguments. " << INVALID_MODE
				  << std::endl;
		usage(std::cerr);
		throw LLAppException(RV_BAD_ARGUMENTS, INVALID_MODE);
	}

	/////  END ARGUMENT VALIDATION /////
}

void LLAppAppearanceUtility::initializeIO()
{
	/////  BEGIN OPEN INPUT FILE ////

	if ( "-" == mInputFilename )
	{
		// Read unformated data from stdin in to memory.
		std::stringstream* data = new std::stringstream();
		const S32 BUFFER_SIZE = BUFSIZ;
		char buffer[BUFFER_SIZE];
		while (true)
		{
			std::cin.read(buffer, BUFFER_SIZE);
			// Check if anything out of the ordinary happened.
			if (!std::cin)
			{
				// See if something 'really bad' happened, or if we just
				// used up all of our buffer.
				if (std::cin.bad())
				{
					std::cerr << "Problem reading standard input." << std::endl;
					delete data;
					throw (RV_UNKNOWN_ERROR);
				}
				else
				{
					// Output normally.
					data->write(buffer, std::cin.gcount());
					if (std::cin.eof()) break;

					// Clear this problem.  We have handled it.
					std::cin.clear();
				}
			}
			else
			{
				data->write(buffer, std::cin.gcount());
				if (std::cin.eof()) break;
			}
		}
		mInput = data;
	}
	else
	{
		// Make sure we can open the input file.
		std::ifstream* input_file = new std::ifstream();
		input_file->open( mInputFilename.c_str(), std::fstream::in );
		if ( input_file->fail())
		{
			std::cerr << "Couldn't open input file '" << mInputFilename << "'." << std::endl;
			delete input_file;
			throw LLAppException(RV_UNABLE_OPEN, mInputFilename);
		}
		mInput = input_file;
	}
	/////  END OPEN INPUT FILE ////

	/////  BEGIN OPEN OUTPUT FILE ////

	if ("" == mOutputFilename)
	{
		mOutput = &std::cout;
	}
	else
	{
		// Make sure we can open the output file.
		std::fstream* output_file = new std::fstream();
		output_file->open( mOutputFilename.c_str(), std::fstream::out );
		if ( output_file->fail() )
		{
			std::cerr << "Couldn't open output file '" << mOutputFilename << "'." << std::endl;
			delete output_file;
			throw LLAppException(RV_UNABLE_OPEN, mOutputFilename);
		}
		mOutput = output_file;
	}
	/////  END OPEN OUTPUT FILE ////

	/////  BEGIN INPUT PARSING ////
	LLSDSerialize::fromXML( mInputData, *mInput );
	if (mInputData.isUndefined())
	{
		throw LLAppException(RV_UNABLE_TO_PARSE);
	}
	/////  END INPUT PARSING ////
}

class LLPassthroughTranslationBridge : public LLTranslationBridge
{
public:
	virtual std::string getString(const std::string &xml_desc)
	{
		// Just pass back the input string.
		return xml_desc;
	}
};


bool LLAppAppearanceUtility::init()
{
	parseArguments();

	bool log_to_stderr = true;
	LLError::initForApplication("", log_to_stderr);
	// *TODO: Add debug mode(s).  Skip this in debug mode.
	LLError::setDefaultLevel(LLError::LEVEL_WARN);

	validateArguments();
	initializeIO();

	// Initialize classes.
	LLWearableType::initClass(new LLPassthroughTranslationBridge());

	// *TODO: Create a texture bridge?
	LLAvatarAppearance::initClass();

	return true;
}

bool LLAppAppearanceUtility::cleanup()
{
	LLAvatarAppearance::cleanupClass();
	LLWearableType::cleanupClass();

	if (mProcess)
	{
		delete mProcess;
		mProcess = NULL;
	}
	if ("-" != mInputFilename && mInput)
	{
		static_cast<std::ifstream*>(mInput)->close();
	}
	if ("" != mOutputFilename && mOutput)
	{
		static_cast<std::ofstream*>(mOutput)->close();
		delete mOutput;
		mOutput = NULL;
	}
	delete mInput;
	mInput = NULL;
	return true;
}

bool LLAppAppearanceUtility::mainLoop()
{
	// This isn't really a loop, for this application.  We just execute the requested command.
	mProcess->process(mInputData, *mOutput);
	return true;
}

