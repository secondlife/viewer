/**
 * @file appearance_utility.cpp
 * @author Don Kjer <don@lindenlab.com>, Nyx Linden
 * @brief Utility for processing avatar appearance without a full viewer implementation.
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

#include "linden_common.h"

#include "llapp.h"
#include "llapr.h"
#include "llerrorcontrol.h"
#include "llsd.h"
#include "llsdserialize.h"
#include "llsdutil.h"

enum EResult
{
	RV_SUCCESS = 0,
	RV_UNKNOWN_ERROR,
	RV_BAD_ARGUMENTS,
	RV_UNABLE_OPEN,
	RV_UNABLE_TO_PARSE,
};

static const std::string MESSAGE_RV_UNKNOWN("Unknown error.");
static const std::string MESSAGE_RV_ARGUMENTS 
("Invalid arguments: ");
static const std::string MESSAGE_RV_UNABLE_OPEN("Unable to open file: ");
static const std::string MESSAGE_RV_UNABLE_TO_PARSE("Unable to parse input LLSD.");

static const apr_getopt_option_t APPEARANCE_UTILITY_OPTIONS[] =
{
	{"tbd", 't', 0, "Extract dump information from a mesh asset."},
	{"output", 'o', 1, "The output file to write to.  Default is stdout"},
	{"agent-id", 'a', 1, "The agent-id of the user."},
	{"grid", 'g', 1, "The grid."},
	{"help", 'h', 0, "Print the help message."},
	{0, 0, 0, 0}
};

const std::string NOTHING_EXTRA("");

/**
 * Helper to return the standard error based on the return value.
 */
LLSD spit_error(EResult rv, const std::string& extra = NOTHING_EXTRA);

/**
 * Helper to generate an error output based on code and message.
 */
LLSD spit_error(const std::string& key, const std::string& message);



LLSD spit_error(EResult value, const std::string& extra)
{
	LLSD rv;
	switch(value)
	{
	case RV_UNKNOWN_ERROR:
		rv = spit_error("unknown", MESSAGE_RV_UNKNOWN);
	case RV_BAD_ARGUMENTS:
		rv = spit_error("arguments", MESSAGE_RV_ARGUMENTS + extra);
		break;
	case RV_UNABLE_OPEN:
		rv = spit_error("file", MESSAGE_RV_UNABLE_OPEN + extra);
		break;
	case RV_UNABLE_TO_PARSE:
		rv = spit_error("input", MESSAGE_RV_UNABLE_TO_PARSE);
		break;
	default:
		rv = spit_error("arguments", "Invalid arguments to spit_error");
		break;
	}
	return rv;
}

LLSD spit_error(const std::string& key, const std::string& message)
{
	LLSD rv;
	rv["success"] = false;
	rv["error"]["key"] = key;
	rv["error"]["message"] = message;
	return rv;
}


std::string usage(const char* command)
{
	std::ostringstream ostr;
	ostr << "Utilities for processing agent appearance data."
		<< std::endl << std::endl
		<< "Usage:" << std::endl
		<< "\t" << command << " [options] filename" << std::endl << std::endl
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
	return ostr.str();
}

EResult process_tbd(LLSD& input, std::ostream& output, LLSD& error_llsd)
{
	EResult rv = RV_SUCCESS;

	LLSD result;
	result["success"] = true;
	result["input"] = input;
	output << LLSDOStreamer<LLSDXMLFormatter>(result);

	return rv;
}

/**
 * @brief Called by main() to ensure proper cleanup. Basically main().
 */
EResult process_command(int argc, char** argv, LLSD& error_llsd)
{
	EResult rv = RV_SUCCESS;

	////// BEGIN OPTION PARSING //////
	// Check for '-' as last option, since apr doesn't seem to like that.
	bool read_stdin = false;
	bool write_stdout = true;
	if (std::string(argv[argc-1]) == "-")
	{
		read_stdin = true;
		argc--;
	}

	apr_status_t apr_err;
	const char* opt_arg = NULL;
	int opt_id = 0;
	apr_getopt_t* os = NULL;
	if(APR_SUCCESS != apr_getopt_init(&os, gAPRPoolp, argc, argv))
	{
		std::cerr << "Unable to initialize apr" << std::endl;
		rv = RV_UNKNOWN_ERROR;
		error_llsd = spit_error(rv);
		return rv;
	}

	bool tbd = false;
	LLUUID agent_id;
	std::string output_filename;
	std::string grid;
	while(true)
	{
		apr_err = apr_getopt_long(os, APPEARANCE_UTILITY_OPTIONS, &opt_id, &opt_arg);
		if(APR_STATUS_IS_EOF(apr_err)) break;
		if(apr_err)
		{
			char buf[MAX_STRING];		/* Flawfinder: ignore */
			std::cerr << "Error parsing options: "
				<< apr_strerror(apr_err, buf, MAX_STRING) << std::endl;
			std::cerr << usage(os->argv[0]) <<  std::endl;
			rv = RV_BAD_ARGUMENTS;
			error_llsd = spit_error(rv, buf);
			return rv;
		}
		switch (opt_id)
		{
		case 't':
			tbd = true;
			break;
		case 'o':
			output_filename.assign(opt_arg);
			write_stdout=false;
			break;
		case 'a':
			agent_id.set(opt_arg);
			if (agent_id.isNull())
			{
				const char* INVALID_AGENT_ID="agent-id must be a valid uuid.";
				std::cerr << "Incorrect arguments. " << INVALID_AGENT_ID
					<< std::endl << usage(argv[0]) <<  std::endl;
				rv = RV_BAD_ARGUMENTS;
				error_llsd = spit_error(rv, INVALID_AGENT_ID);
				return rv;
			}
			break;
		case 'g':
			grid = opt_arg;
			break;
		case 'h':
			std::cout << usage(os->argv[0]);
			return RV_SUCCESS;
		default:
			std::cerr << usage(os->argv[0]);
			rv = RV_BAD_ARGUMENTS;
			error_llsd = spit_error(rv, "Unknown option.");
			return rv;
		}
	}
	////// END OPTION PARSING //////

	/////  BEGIN ARGUMENT VALIDATION /////
	std::string input_filename;

	// Make sure we don't have more than one command specified.
	if (!tbd)
	{
		const char* INVALID_MODE="Must specify mode. (tbd)";
		std::cerr << "Incorrect arguments. " << INVALID_MODE
				  << std::endl;
		std::cerr << usage(os->argv[0]);
		rv = RV_BAD_ARGUMENTS;
		error_llsd = spit_error(rv, INVALID_MODE);
		return rv;
	}

	// *TODO: Add debug mode(s).  Skip this in debug mode.
	LLError::setDefaultLevel(LLError::LEVEL_WARN);

	if (!read_stdin)
	{
		bool valid_input_filename = false;
		// Try to grab the input filename.
		if (os->argv && os->argv[os->ind])
		{
			input_filename.assign(os->argv[os->ind]);
			if (! input_filename.empty() )
			{
				valid_input_filename = true;
			}
		}
		if (!valid_input_filename)
		{
			const char* INVALID_FILENAME="Must specify input file.";
			std::cerr << "Incorrect arguments. " << INVALID_FILENAME
				<< std::endl << usage(os->argv[0]) <<  std::endl;
			rv = RV_BAD_ARGUMENTS;
			error_llsd = spit_error(rv, INVALID_FILENAME);
			return rv;
		}
	}

	/////  END ARGUMENT VALIDATION /////

	/////  BEGIN OPEN INPUT FILE ////
	std::ifstream input_file;
	std::stringstream data;
	std::istream* input = NULL;

	if (read_stdin)
	{
		// Read unformated data from stdin in to memory.
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
					rv = RV_UNKNOWN_ERROR;
					error_llsd = spit_error(rv);
					return rv;
				}
				else
				{
					// Output normally.
					data.write(buffer, std::cin.gcount());
					if (std::cin.eof()) break;

					// Clear this problem.  We have handled it.
					std::cin.clear();
				}
			}
			else
			{
				data.write(buffer, std::cin.gcount());
				if (std::cin.eof()) break;
			}
		}
		input = &data;
	}
	else
	{
		// Make sure we can open the input file.
		input_file.open( input_filename.c_str(), std::fstream::in );
		if ( input_file.fail())
		{
			std::cerr << "Couldn't open input file '" << input_filename << "'." << std::endl;
			rv = RV_UNABLE_OPEN;
			error_llsd = spit_error(rv, input_filename);
			return rv;
		}
		input = &input_file;
	}
	/////  END OPEN INPUT FILE ////

	/////  BEGIN OPEN OUTPUT FILE ////
	std::fstream output_file;
	std::ostream* output = NULL;

	// Make sure we can open the output file.
	if (write_stdout)
	{
		output = &std::cout;
	}
	else
	{
		output_file.open( output_filename.c_str(), std::fstream::out );
		if ( output_file.fail() )
		{
			std::cerr << "Couldn't open output file '" << output_filename << "'." << std::endl;
			rv = RV_UNABLE_OPEN;
			error_llsd = spit_error(rv, output_filename);
			if (!read_stdin) input_file.close();
			return rv;
		}
		output = &output_file;
	}
	/////  END OPEN OUTPUT FILE ////

	/////  BEGIN INPUT PARSING ////
	LLSD input_llsd;
	LLSDSerialize::fromXML( input_llsd, *input );
	if (input_llsd.isUndefined())
	{
		rv = RV_UNABLE_TO_PARSE;
		error_llsd = spit_error(rv);
		return rv;
	}

	/////  END INPUT PARSING ////

	if (tbd)
	{
		rv = process_tbd(input_llsd, *output, error_llsd);
	}

	if (!write_stdout) output_file.close();
	if (!read_stdin) input_file.close();

	return rv;
}

class LLAppAppearanceUtility : public LLApp
{
public:
	LLAppAppearanceUtility() {}
	virtual ~LLAppAppearanceUtility() {}
	/*virtual*/ bool init();
	/*virtual*/ bool cleanup();
	/*virtual*/ bool mainLoop() { return true;}
};

bool LLAppAppearanceUtility::init()
{
	return true;
}

bool LLAppAppearanceUtility::cleanup()
{
	return true;
}

int main(int argc, char** argv)
{
	ll_init_apr();
	LLAppAppearanceUtility* app = new LLAppAppearanceUtility();
	app->init();
	LLSD error_llsd;
	EResult rv = process_command(argc, argv, error_llsd);
	if (RV_SUCCESS != rv)
	{
		std::cerr << LLSDOStreamer<LLSDXMLFormatter>(error_llsd);
	}
	app->cleanup();
	delete app;
	ll_cleanup_apr();
	return (int) rv;
}


