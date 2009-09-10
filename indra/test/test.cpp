/** 
 * @file test.cpp
 * @author Phoenix
 * @date 2005-09-26
 * @brief Entry point for the test app.
 *
 * $LicenseInfo:firstyear=2005&license=viewergpl$
 * 
 * Copyright (c) 2005-2009, Linden Research, Inc.
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

/** 
 *
 * You can add tests by creating a new cpp file in this directory, and
 * rebuilding. There are at most 50 tests per testgroup without a
 * little bit of template parameter and makefile tweaking.
 *
 */

#include "linden_common.h"
#include "llerrorcontrol.h"
#include "lltut.h"

#include "apr_pools.h"
#include "apr_getopt.h"

// the CTYPE_WORKAROUND is needed for linux dev stations that don't
// have the broken libc6 packages needed by our out-of-date static 
// libs (such as libcrypto and libcurl). -- Leviathan 20060113
#ifdef CTYPE_WORKAROUND
#	include "ctype_workaround.h"
#endif

#ifndef LL_WINDOWS
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#endif

namespace tut
{
	std::string sSourceDir;

    test_runner_singleton runner;
}

class LLTestCallback : public tut::callback
{
public:
	LLTestCallback(bool verbose_mode, std::ostream *stream) :
		mVerboseMode(verbose_mode),
		mTotalTests(0),
		mPassedTests(0),
		mFailedTests(0),
		mSkippedTests(0),
		mStream(stream)
	{
	}

	void run_started()
	{
		//std::cout << "run_started" << std::endl;
	}

	void test_completed(const tut::test_result& tr)
	{
		++mTotalTests;
		std::ostringstream out;
		out << "[" << tr.group << ", " << tr.test << "] ";
		switch(tr.result)
		{
		case tut::test_result::ok:
			++mPassedTests;
			out << "ok";
			break;
		case tut::test_result::fail:
			++mFailedTests;
			out << "fail";
			break;
		case tut::test_result::ex:
			++mFailedTests;
			out << "exception";
			break;
		case tut::test_result::warn:
			++mFailedTests;
			out << "test destructor throw";
			break;
		case tut::test_result::term:
			++mFailedTests;
			out << "abnormal termination";
			break;
		case tut::test_result::skip:
			++mSkippedTests;
			out << "skipped known failure";
			break;
		default:
			++mFailedTests;
			out << "unknown";
		}
		if(mVerboseMode || (tr.result != tut::test_result::ok))
		{
			if(!tr.message.empty())
			{
				out << ": '" << tr.message << "'";
			}
			if (mStream)
			{
				*mStream << out.str() << std::endl;
			}
			
			std::cout << out.str() << std::endl;
		}
	}

	void run_completed()
	{
		if (mStream)
		{
			run_completed_(*mStream);
		}
		run_completed_(std::cout);

		if (mFailedTests > 0)
		{
			exit(1);
		}
	}
	
private:
	void run_completed_(std::ostream &stream)
	{
		stream << std::endl;
		stream << "Total Tests:   " << mTotalTests << std::endl;
		stream << "Passed Tests: " << mPassedTests << std::endl;

		stream << std::endl;
		stream << "Total Tests:   " << mTotalTests << std::endl;
		stream << "Passed Tests: " << mPassedTests << std::endl;
		
		if (mSkippedTests > 0)
		{
			stream << "Skipped known failures: " << mSkippedTests
				<< std::endl;
		}

		if(mFailedTests > 0)
		{
			stream << "*********************************" << std::endl;
			stream << "Failed Tests:   " << mFailedTests << std::endl;
			stream << "Please report or fix the problem." << std::endl;
			stream << "*********************************" << std::endl;
		}
	}

protected:
	bool mVerboseMode;
	int mTotalTests;
	int mPassedTests;
	int mFailedTests;
	int mSkippedTests;
	std::ostream *mStream;
};

static const apr_getopt_option_t TEST_CL_OPTIONS[] =
{
	{"help", 'h', 0, "Print the help message."},
	{"list", 'l', 0, "List available test groups."},
	{"verbose", 'v', 0, "Verbose output."},
	{"group", 'g', 1, "Run test group specified by option argument."},
	{"output", 'o', 1, "Write output to the named file."},
	{"sourcedir", 's', 1, "Project source file directory from CMake."},
	{"touch", 't', 1, "Touch the given file if all tests succeed"},
	{"wait", 'w', 0, "Wait for input before exit."},
	{"debug", 'd', 0, "Emit full debug logs."},
	{0, 0, 0, 0}
};

void stream_usage(std::ostream& s, const char* app)
{
	s << "Usage: " << app << " [OPTIONS]" << std::endl
	  << std::endl;

	s << "This application runs the unit tests." << std::endl << std::endl;

	s << "Options: " << std::endl;
	const apr_getopt_option_t* option = &TEST_CL_OPTIONS[0];
	while(option->name)
	{
		s << "  ";
		s << "  -" << (char)option->optch << ", --" << option->name
		  << std::endl;
		s << "\t" << option->description << std::endl << std::endl;
		++option;
	}

	s << "Examples:" << std::endl;
	s << "  " << app << " --verbose" << std::endl;
	s << "\tRun all the tests and report all results." << std::endl;
	s << "  " << app << " --list" << std::endl;
	s << "\tList all available test groups." << std::endl;
	s << "  " << app << " --group=uuid" << std::endl;
	s << "\tRun the test group 'uuid'." << std::endl;
}

void stream_groups(std::ostream& s, const char* app)
{
	s << "Registered test groups:" << std::endl;
	tut::groupnames gl = tut::runner.get().list_groups();
	tut::groupnames::const_iterator it = gl.begin();
	tut::groupnames::const_iterator end = gl.end();
	for(; it != end; ++it)
	{
		s << "  " << *(it) << std::endl;
	}
}

void wouldHaveCrashed(const std::string& message)
{
	tut::fail("llerrs message: " + message);
}

int main(int argc, char **argv)
{
	// The following line must be executed to initialize Google Mock
	// (and Google Test) before running the tests.
#ifndef LL_WINDOWS
	::testing::InitGoogleMock(&argc, argv);
#endif
	LLError::initForApplication(".");
	LLError::setFatalFunction(wouldHaveCrashed);
	LLError::setDefaultLevel(LLError::LEVEL_ERROR);
		//< *TODO: should come from error config file. Note that we
		// have a command line option that sets this to debug.
	
#ifdef CTYPE_WORKAROUND
	ctype_workaround();
#endif

	apr_initialize();
	apr_pool_t* pool = NULL;
	if(APR_SUCCESS != apr_pool_create(&pool, NULL))
	{
		std::cerr << "Unable to initialize pool" << std::endl;
		return 1;
	}
	apr_getopt_t* os = NULL;
	if(APR_SUCCESS != apr_getopt_init(&os, pool, argc, argv))
	{
		std::cerr << "Unable to  pool" << std::endl;
		return 1;
	}

	// values used for controlling application
	bool verbose_mode = false;
	bool wait_at_exit = false;
	std::string test_group;

	// values use for options parsing
	apr_status_t apr_err;
	const char* opt_arg = NULL;
	int opt_id = 0;
	std::ofstream *output = NULL;
	const char *touch = NULL;
	
	while(true)
	{
		apr_err = apr_getopt_long(os, TEST_CL_OPTIONS, &opt_id, &opt_arg);
		if(APR_STATUS_IS_EOF(apr_err)) break;
		if(apr_err)
		{
			char buf[255];		/* Flawfinder: ignore */
			std::cerr << "Error parsing options: "
					  << apr_strerror(apr_err, buf, 255) << std::endl;
			return 1;
		}
		switch (opt_id)
		{
		case 'g':
			test_group.assign(opt_arg);
			break;
		case 'h':
			stream_usage(std::cout, argv[0]);
			return 0;
			break;
		case 'l':
			stream_groups(std::cout, argv[0]);
			return 0;
		case 'v':
			verbose_mode = true;
			break;
		case 'o':
			output = new std::ofstream;
			output->open(opt_arg);
			break;
		case 's':	// --sourcedir
			tut::sSourceDir = opt_arg;
			// For convenience, so you can use tut::sSourceDir + "myfile"
			tut::sSourceDir += '/';
			break;
		case 't':
			touch = opt_arg;
			break;
		case 'w':
			wait_at_exit = true;
			break;
		case 'd':
			// *TODO: should come from error config file. We set it to
			// ERROR by default, so this allows full debug levels.
			LLError::setDefaultLevel(LLError::LEVEL_DEBUG);
			break;
		default:
			stream_usage(std::cerr, argv[0]);
			return 1;
			break;
		}
	}

	// run the tests
	LLTestCallback callback(verbose_mode, output);
	tut::runner.get().set_callback(&callback);
	
	if(test_group.empty())
	{
		tut::runner.get().run_tests();
	}
	else
	{
		tut::runner.get().run_tests(test_group);
	}

	if (wait_at_exit)
	{
		std::cerr << "Waiting for input before exiting..." << std::endl;
		std::cin.get();
	}
	
	if (output)
	{
		output->close();
		delete output;
	}

	if (touch)
	{
		std::ofstream s;
		s.open(touch);
		s << "ok" << std::endl;
		s.close();
	}
	
	apr_terminate();
	return 0;
}
