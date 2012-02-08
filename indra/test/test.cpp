/** 
 * @file test.cpp
 * @author Phoenix
 * @date 2005-09-26
 * @brief Entry point for the test app.
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
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
#include "stringize.h"

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

#if LL_MSVC
#pragma warning (push)
#pragma warning (disable : 4702) // warning C4702: unreachable code
#endif
#include <boost/iostreams/tee.hpp>
#include <boost/iostreams/stream.hpp>
#if LL_MSVC
#pragma warning (pop)
#endif

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/foreach.hpp>
#include <boost/lambda/lambda.hpp>

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
	// By default, capture a shared_ptr to std::cout, with a no-op "deleter"
	// so that destroying the shared_ptr makes no attempt to delete std::cout.
	mStream(boost::shared_ptr<std::ostream>(&std::cout, boost::lambda::_1))
	{
		if (stream)
		{
			// We want a boost::iostreams::tee_device that will stream to two
			// std::ostreams.
			typedef boost::iostreams::tee_device<std::ostream, std::ostream> TeeDevice;
			// More than that, though, we want an actual stream using that
			// device.
			typedef boost::iostreams::stream<TeeDevice> TeeStream;
			// Allocate and assign in two separate steps, per Herb Sutter.
			// (Until we turn on C++11 support, have to wrap *stream with
			// boost::ref() due to lack of perfect forwarding.)
			boost::shared_ptr<std::ostream> pstream(new TeeStream(std::cout, boost::ref(*stream)));
			mStream = pstream;
		}
	}

	~LLTestCallback()
	{
	}	

	virtual void run_started()
	{
		//std::cout << "run_started" << std::endl;
	}

	virtual void group_started(const std::string& name) {
		*mStream << "Unit test group_started name=" << name << std::endl;
	}

	virtual void group_completed(const std::string& name) {
		*mStream << "Unit test group_completed name=" << name << std::endl;
	}

	virtual void test_completed(const tut::test_result& tr)
	{
		++mTotalTests;
		std::ostringstream out;
		out << "[" << tr.group << ", " << tr.test;
		if (! tr.name.empty())
			out << ": " << tr.name;
		out << "] ";
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
				out << "unknown (tr.result == " << tr.result << ")";
		}
		if(mVerboseMode || (tr.result != tut::test_result::ok))
		{
			*mStream << out.str();
			if(!tr.message.empty())
			{
				*mStream << ": '" << tr.message << "'";
			}
			*mStream << std::endl;
		}
	}

	virtual int getFailedTests() const { return mFailedTests; }

	virtual void run_completed()
	{
		*mStream << "\tTotal Tests:\t" << mTotalTests << std::endl;
		*mStream << "\tPassed Tests:\t" << mPassedTests;
		if (mPassedTests == mTotalTests)
		{
			*mStream << "\tYAY!! \\o/";
		}
		*mStream << std::endl;

		if (mSkippedTests > 0)
		{
			*mStream << "\tSkipped known failures:\t" << mSkippedTests
			<< std::endl;
		}

		if(mFailedTests > 0)
		{
			*mStream << "*********************************" << std::endl;
			*mStream << "Failed Tests:\t" << mFailedTests << std::endl;
			*mStream << "Please report or fix the problem." << std::endl;
			*mStream << "*********************************" << std::endl;
		}
	}

protected:
	bool mVerboseMode;
	int mTotalTests;
	int mPassedTests;
	int mFailedTests;
	int mSkippedTests;
	boost::shared_ptr<std::ostream> mStream;
};

// TeamCity specific class which emits service messages
// http://confluence.jetbrains.net/display/TCD3/Build+Script+Interaction+with+TeamCity;#BuildScriptInteractionwithTeamCity-testReporting

class LLTCTestCallback : public LLTestCallback
{
public:
	LLTCTestCallback(bool verbose_mode, std::ostream *stream) :
		LLTestCallback(verbose_mode, stream)
	{
	}

	~LLTCTestCallback()
	{
	}

	virtual void group_started(const std::string& name) {
		LLTestCallback::group_started(name);
		std::cout << "\n##teamcity[testSuiteStarted name='" << escape(name) << "']" << std::endl;
	}

	virtual void group_completed(const std::string& name) {
		LLTestCallback::group_completed(name);
		std::cout << "##teamcity[testSuiteFinished name='" << escape(name) << "']" << std::endl;
	}

	virtual void test_completed(const tut::test_result& tr)
	{
		std::string testname(STRINGIZE(tr.group << "." << tr.test));
		if (! tr.name.empty())
		{
			testname.append(":");
			testname.append(tr.name);
		}
		testname = escape(testname);

		// Sadly, tut::callback doesn't give us control at test start; have to
		// backfill start message into TC output.
		std::cout << "##teamcity[testStarted name='" << testname << "']" << std::endl;

		// now forward call to base class so any output produced there is in
		// the right TC context
		LLTestCallback::test_completed(tr);

		switch(tr.result)
		{
			case tut::test_result::ok:
				break;

			case tut::test_result::fail:
			case tut::test_result::ex:
			case tut::test_result::warn:
			case tut::test_result::term:
				std::cout << "##teamcity[testFailed name='" << testname
						  << "' message='" << escape(tr.message) << "']" << std::endl;
				break;

			case tut::test_result::skip:
				std::cout << "##teamcity[testIgnored name='" << testname << "']" << std::endl;
				break;

			default:
				break;
		}

		std::cout << "##teamcity[testFinished name='" << testname << "']" << std::endl;
	}

	static std::string escape(const std::string& str)
	{
		// Per http://confluence.jetbrains.net/display/TCD65/Build+Script+Interaction+with+TeamCity#BuildScriptInteractionwithTeamCity-ServiceMessages
		std::string result;
		BOOST_FOREACH(char c, str)
		{
			switch (c)
			{
			case '\'':
				result.append("|'");
				break;
			case '\n':
				result.append("|n");
				break;
			case '\r':
				result.append("|r");
				break;
/*==========================================================================*|
			// These are not possible 'char' values from a std::string.
			case '\u0085':			// next line
				result.append("|x");
				break;
			case '\u2028':			// line separator
				result.append("|l");
				break;
			case '\u2029':			// paragraph separator
				result.append("|p");
				break;
|*==========================================================================*/
			case '|':
				result.append("||");
				break;
			case '[':
				result.append("|[");
				break;
			case ']':
				result.append("|]");
				break;
			default:
				result.push_back(c);
				break;
			}
		}
		return result;
	}
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
	{"suitename", 'x', 1, "Run tests using this suitename"},
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
		std::cerr << "apr_getopt_init() failed" << std::endl;
		return 1;
	}

	// values used for controlling application
	bool verbose_mode = false;
	bool wait_at_exit = false;
	std::string test_group;
	std::string suite_name;

	// values use for options parsing
	apr_status_t apr_err;
	const char* opt_arg = NULL;
	int opt_id = 0;
	boost::scoped_ptr<std::ofstream> output;
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
				output.reset(new std::ofstream);
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
			case 'x':
				suite_name.assign(opt_arg);
				break;
			default:
				stream_usage(std::cerr, argv[0]);
				return 1;
				break;
		}
	}

	// run the tests

	LLTestCallback* mycallback;
	if (getenv("TEAMCITY_PROJECT_NAME"))
	{
		mycallback = new LLTCTestCallback(verbose_mode, output.get());		
	}
	else
	{
		mycallback = new LLTestCallback(verbose_mode, output.get());
	}

	tut::runner.get().set_callback(mycallback);

	if(test_group.empty())
	{
		tut::runner.get().run_tests();
	}
	else
	{
		tut::runner.get().run_tests(test_group);
	}

	bool success = (mycallback->getFailedTests() == 0);

	if (wait_at_exit)
	{
		std::cerr << "Press return to exit..." << std::endl;
		std::cin.get();
	}

	if (touch && success)
	{
		std::ofstream s;
		s.open(touch);
		s << "ok" << std::endl;
		s.close();
	}

	apr_terminate();

	int retval = (success ? 0 : 1);
	return retval;

	//delete mycallback;
}
