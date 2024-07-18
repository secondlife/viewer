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
#include "boost/bind.hpp"

// linden includes
#include "linden_common.h"

#include "llapr.h"
#include "llerrorcontrol.h"
#include "llfasttimer.h"
#include "llgl.h"
#include "llmd5.h"
#include "llsd.h"
#include "llsdserialize.h"
#include "llsdutil.h"
#include "llquantize.h"
#include "lltreeiterators.h"
#include "v3color.h"
#include "llwindow.h"
#include "lltrace.h"

// appearance includes
#include "llavatarappearance.h"
#include "llwearabletype.h"

// project includes
#include "llappappearanceutility.h"
#include "llbakingprocess.h"
#include "llprocessparams.h"
#include "llprocesstexture.h"
#include "llprocessskin.h"

const std::string NOTHING_EXTRA("");

////////////////////////////////////////////
// LLAppException
////////////////////////////////////////////

static const std::string MESSAGE_RV_UNKNOWN("Unknown error.");
static const std::string MESSAGE_RV_ARGUMENTS 
("Invalid arguments: ");
static const std::string MESSAGE_RV_UNABLE_OPEN("Unable to open file: ");
static const std::string MESSAGE_RV_UNABLE_TO_PARSE("Unable to parse input LLSD.");
static const std::string MESSAGE_RV_UNABLE_TO_DECODE("Unable to decode input J2C.");
static const std::string MESSAGE_RV_UNABLE_TO_ENCODE("Unable to encode output J2C.");
static const std::string MESSAGE_RV_UNABLE_TO_INIT_GL("Unable to initialize GL.");
static const std::string MESSAGE_RV_UNABLE_TO_BAKE("Unable to bake texture.");
static const std::string MESSAGE_RV_INVALID_SKIN_BLOCK("Invalid skin block.");
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
		printErrorLLSD("input", MESSAGE_RV_UNABLE_TO_PARSE + extra);
		break;
	case RV_UNABLE_TO_DECODE:
		printErrorLLSD("input", MESSAGE_RV_UNABLE_TO_DECODE + extra);
		break;
	case RV_UNABLE_TO_ENCODE:
		printErrorLLSD("input", MESSAGE_RV_UNABLE_TO_ENCODE + extra);
		break;
	case RV_UNABLE_TO_INIT_GL:
		printErrorLLSD("input", MESSAGE_RV_UNABLE_TO_INIT_GL + extra);
		break;
	case RV_UNABLE_TO_BAKE:
		printErrorLLSD("input", MESSAGE_RV_UNABLE_TO_BAKE + extra);
		break;
	case RV_INVALID_SKIN_BLOCK:
		printErrorLLSD("input", MESSAGE_RV_INVALID_SKIN_BLOCK + extra);
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
	/*virtual*/ void process(std::ostream& output)
	{
		mApp->usage(output);
	}
};


static const apr_getopt_option_t APPEARANCE_UTILITY_OPTIONS[] =
{
	{"params", 'p', 0, "Generate appearance parameters for an agent."},
	{"texture", 't', 0, "Generate baked texture for a slot."},
	{"output", 'o', 1, "The output file to write to.  Default is stdout"},
	{"agent-id", 'a', 1, "The agent-id of the user."},
	{"bake-size", 'b', 1, "The bake texture size. eg use 512 for 512*512 textures, 1024 for 1024*1024 textures" },
	//{"grid", 'g', 1, "The grid."},
	{"debug", 'd', 0, "Enable debug spam.  Default is warn/info spam only."},
	{"treemap", 'm', 1, "Output LLFrameTimer to specified file in graphviz treemap/pachwork format."},
	{"threshold", 's', 1, "Percent threshold of max LLFrameTimer time in order to appear on treemap. Default is 1%."},
	{"joint-offsets",'j',0,"Extract joint positions from skin."},
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
	mAppName(argv[0]),
	mDebugMode(FALSE),
	mTreeMapThreshold(1),
	mBakeTextureSize(512)
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
		case 't':
			verifyNoProcess();
			mProcess = new LLProcessTexture(this);
			break;
		case 'j':
			verifyNoProcess();
			mProcess = new LLProcessSkin( this );
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
		case 'b':
			mBakeTextureSize = atoi(opt_arg);
			break;
		//case 'g':
		//	grid = opt_arg;
		//	break;
		case 'd':
			mDebugMode = TRUE;
			break;
		case 'm':
			mTreeMapFilename.assign(opt_arg);
			break;
		case 's':
			mTreeMapThreshold = atoi(opt_arg);
			break;
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
	if (mProcess)
	{
		mProcess->parseInput( *mInput );
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
	if (mDebugMode)
	{
		mRecording.start();
		LLError::setDefaultLevel(LLError::LEVEL_DEBUG);
	}
	else
	{
		LLError::setDefaultLevel(LLError::LEVEL_WARN);
	}

	validateArguments();

	LL_DEBUGS() << "BakeSize: " << mBakeTextureSize << LL_ENDL;

	// Initialize classes.
	// Values taken from settings.xml.
	const BOOL USE_TEXTURE_NEW_BYTE_RANGE=TRUE;
	const S32 TEXTURE_REVERSE_BYTE_RANGE=50;
	LLImage::initClass(USE_TEXTURE_NEW_BYTE_RANGE, TEXTURE_REVERSE_BYTE_RANGE);
	const BOOL SKIP_ANALYZE_ALPHA=TRUE;
	LLImageGL::initClass(LLGLTexture::MAX_GL_IMAGE_CATEGORY, SKIP_ANALYZE_ALPHA);
	LLWearableType::initClass(new LLPassthroughTranslationBridge());

	// *TODO: Create a texture bridge?
	LLAvatarAppearance::initClass();

	initializeIO();
	if (mProcess)
	{
		mProcess->init();
	}
	return true;
}


void add_cluster(LLTrace::Recording& recording, std::ostream& tree, LLTrace::BlockTimerStatHandle& node, std::vector < S32 > & clusters, F64Milliseconds threshold)
{
	LLMD5 hash;
	hash.update((const unsigned char*)node.getName().c_str(), node.getName().size());
	hash.finalize();
	char buf[33];
	hash.hex_digest(buf);
	buf[6] = 0;
	LLColor3 color(buf);
	if (color.brightness() < 0.25f)
	{
		color.normalize();
	}
	std::ostringstream color_str;
	color_str << "#" << std::hex << std::setfill('0') << std::setw(2)
			  << (S32) F32_to_U8(color.mV[0], 0.0f, 1.0f)
			  << (S32) F32_to_U8(color.mV[1], 0.0f, 1.0f)
			  << (S32) F32_to_U8(color.mV[2], 0.0f, 1.0f);

	std::vector<S32>::iterator iter = clusters.begin();
	std::vector<S32>::iterator end  = clusters.end();
	std::ostringstream padding;
	for(; iter != end; ++iter)
	{
		padding << "  ";
	}

	std::ostringstream node_id;
	bool first = true;
	iter = clusters.begin();
	for(; iter != end; ++iter)
	{
		if (!first)
		{
			node_id << "_";
		}
		first = false;
		node_id << (*iter);
	}

	if (node.getChildren().size() == 0)
	{
		F64Milliseconds leaf_time_ms(recording.getSum(node));
		if (leaf_time_ms > threshold)
		{
			tree << padding.str() << "n" << node_id.str() << " ["
				 << "label=\"" << node.getName() << " (" << leaf_time_ms.value() << ")\" "
				 << "fillcolor=\"" << color_str.str() << "\" "
				 << "area=" << leaf_time_ms.value() / 10 << "]" << std::endl;
		}
	}
	else
	{
		if (clusters.size())
		{
			tree << padding.str() << "subgraph cluster" << node_id.str();
			tree << " {" << std::endl;
		}

		S32Milliseconds node_area(recording.getSum(node));
		std::vector<LLTrace::BlockTimerStatHandle*>::iterator child_iter = node.getChildren().begin();
		for (S32 num=0; child_iter != node.getChildren().end(); ++child_iter, ++num)
		{
			clusters.push_back(num);
			add_cluster(recording, tree, *(*child_iter), clusters, threshold);
			clusters.pop_back();
			node_area -= recording.getSum(*(*child_iter));
		}

		S32Milliseconds node_time_ms(node_area);
		if (node_time_ms > threshold)
		{
			tree << padding.str() << "n" << node_id.str() << " ["
				 << "label=\"" << node.getName() << " (" << node_time_ms.value() << ")\" "
				 << "fillcolor=\"" << color_str.str() << "\" "
				 << "area=" << node_time_ms.value() / 10 << "]" << std::endl;
		}
		if (clusters.size())
		{
			tree << padding.str() << "}" << std::endl;;
		}
	}
}

bool LLAppAppearanceUtility::cleanup()
{
	if (mProcess)
	{
		mProcess->cleanup();
	}

	// Spam fast timer information in debug mode.
	if (mDebugMode)
	{
		mRecording.stop();
		LLTrace::BlockTimer::processTimes();

		S32Milliseconds max_time_ms(0);
		for (LLTrace::block_timer_tree_df_iterator_t it = LLTrace::begin_block_timer_tree_df(LLTrace::BlockTimer::getRootTimeBlock());
			it != LLTrace::end_block_timer_tree_df();
			++it)
		{
			LLTrace::BlockTimerStatHandle* idp = (*it);
			// Skip near-zero time leafs.
			S32Milliseconds leaf_time_ms(mRecording.getSum(*idp));
			if (leaf_time_ms > max_time_ms) max_time_ms = leaf_time_ms;
			if ((S32Milliseconds)0 == leaf_time_ms) continue;

			std::vector< LLTrace::BlockTimerStatHandle*  > parents;
			LLTrace::BlockTimerStatHandle* parentp = idp->getParent();
			while (parentp)
			{
				parents.push_back(parentp);
				if (parentp->getParent() == parentp) break;
				parentp = parentp->getParent();
			}

			std::ostringstream fullname;
			bool is_first = true;
			for ( std::vector< LLTrace::BlockTimerStatHandle*  >::reverse_iterator iter = parents.rbegin();
				  iter != parents.rend(); ++iter)
			{
				// Skip root
				if (is_first)
				{
					is_first = false;
					continue;
				}
				LLTrace::BlockTimerStatHandle* parent_idp = (*iter);
				U32Milliseconds time_ms(mRecording.getSum(parent_idp->selfTime()));
				fullname << parent_idp->getName() << " ";
				fullname << "(";
				if (time_ms > (U32Milliseconds)0)
				{
					fullname << time_ms.value() << " ms, ";
				}
				fullname << mRecording.getSum(parent_idp->callCount()) << " call)-> ";
			}
			LL_DEBUGS() << fullname.str() << LL_ENDL;
		}
		if (!mTreeMapFilename.empty())
		{
			std::ofstream tree(mTreeMapFilename.c_str());
			tree << "graph G {" << std::endl;
			tree << "  node[style=filled]" << std::endl;

			LLTrace::BlockTimerStatHandle& root = LLTrace::BlockTimer::getRootTimeBlock();
			std::vector<S32> clusters;
			add_cluster(mRecording, tree, root, clusters, (((F32) mTreeMapThreshold / 100.f) * max_time_ms) );

			tree << "}" << std::endl;

			LL_DEBUGS() << "To generate a treemap of LLFrameTimer results, run:" << LL_ENDL;
			LL_DEBUGS() << "patchwork " << mTreeMapFilename << " -Tpng > rendered.png" << LL_ENDL;
		}
	}

	LLAvatarAppearance::cleanupClass();
	LLWearableType::cleanupClass();
	LLImageGL::cleanupClass();
	LLImage::cleanupClass();

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

bool LLAppAppearanceUtility::frame()
{
	// This isn't really a loop, for this application.  We just execute the requested command.
	mProcess->process(*mOutput);
	return true;
}

