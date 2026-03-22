/**
 * @file test.cpp
 * @author Phoenix
 * @date 2005-09-26
 * @brief Entry point for the test app – now backed by doctest.
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
 * Add tests by creating a new cpp file in this directory and rebuilding.
 * Each file should define TEST_CASE blocks using the doctest framework.
 * See https://github.com/doctest/doctest for documentation.
 */

// Define the doctest implementation in this translation unit.
#define DOCTEST_CONFIG_IMPLEMENT
#include "lltut.h"

#include "linden_common.h"
#include "llerrorcontrol.h"
#include "stringize.h"
#include "namedtempfile.h"
#include "lltrace.h"
#include "lltracethreadrecorder.h"

#ifdef CTYPE_WORKAROUND
#   include "ctype_workaround.h"
#endif

#include <fstream>
#include <string>

// ---------------------------------------------------------------------------
// tut::sSourceDir – set via --sourcedir flag and used by test files to locate
// test-data files relative to the source tree.
// ---------------------------------------------------------------------------
namespace tut
{
    std::string sSourceDir;
}

// ---------------------------------------------------------------------------
// Register tut::failure as a known exception so doctest prints the message
// instead of "unknown exception".
// ---------------------------------------------------------------------------
REGISTER_EXCEPTION_TRANSLATOR(tut::failure& ex)
{
    return doctest::String(ex.what());
}

REGISTER_EXCEPTION_TRANSLATOR(tut::skip_exception& ex)
{
    // Treat skipped tests as passing (they were explicitly skipped).
    return doctest::String(std::string("SKIPPED: ").append(ex.what()).c_str());
}

// ---------------------------------------------------------------------------
// Log replay helper (replays captured log messages on test failure)
// ---------------------------------------------------------------------------
class LLReplayLog
{
public:
    LLReplayLog() {}
    virtual ~LLReplayLog() {}
    virtual void reset() {}
    virtual void replay(std::ostream&) {}
};

class RecordToTempFile : public LLError::Recorder
{
public:
    RecordToTempFile(const RecordToTempFile&)             = delete;
    RecordToTempFile& operator=(const RecordToTempFile&)  = delete;

    RecordToTempFile()
        : LLError::Recorder()
        , mTempFile("log", "")
        , mFile(mTempFile.getName().c_str())
    {}

    virtual ~RecordToTempFile()
    {
        mFile.close();
    }

    virtual void recordMessage(LLError::ELevel level, const std::string& message) override
    {
        LL_PROFILE_ZONE_SCOPED;
        mFile << message << std::endl;
    }

    void reset()
    {
        mFile.close();
        mFile.open(mTempFile.getName().c_str());
    }

    void replay(std::ostream& out)
    {
        mFile.close();
        std::ifstream inf(mTempFile.getName().c_str());
        std::string line;
        while (std::getline(inf, line))
        {
            out << line << std::endl;
        }
    }

private:
    NamedTempFile mTempFile;
    llofstream    mFile;
};

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
static LLTrace::ThreadRecorder* sMasterThreadRecorder = nullptr;

int main(int argc, char** argv)
{
    // -----------------------------------------------------------------------
    // Pull out our custom flags before handing argv to doctest.
    // Supported flags:
    //   --sourcedir=<path>   Set tut::sSourceDir
    //   --touch=<file>       Touch file on success (legacy CMake integration)
    //   --debug              Emit full debug logs
    // -----------------------------------------------------------------------
    const char* touch    = nullptr;
    bool        debugLog = false;
    const char* LOGTEST  = getenv("LOGTEST");
    const char* LOGFAIL  = getenv("LOGFAIL");

    // We rebuild a cleaned argv for doctest (strip our own flags).
    std::vector<const char*> dt_argv;
    dt_argv.push_back(argv[0]);

    for (int i = 1; i < argc; ++i)
    {
        std::string arg(argv[i]);

        if (arg.substr(0, 12) == "--sourcedir=")
        {
            tut::sSourceDir = arg.substr(12);
            // Ensure trailing slash
            if (!tut::sSourceDir.empty() && tut::sSourceDir.back() != '/')
                tut::sSourceDir += '/';
        }
        else if (arg.substr(0, 8) == "--touch=")
        {
            touch = argv[i] + 8;
        }
        else if (arg == "--debug" || arg == "-d")
        {
            debugLog = true;
        }
        else
        {
            // Pass through to doctest
            dt_argv.push_back(argv[i]);
        }
    }

    int dt_argc = static_cast<int>(dt_argv.size());

    // -----------------------------------------------------------------------
    // Initialise the LL error/logging system
    // -----------------------------------------------------------------------
    ll_init_apr();

    if ((debugLog || (LOGTEST && *LOGTEST)))
    {
        LLError::initForApplication(".", ".", true /* log to stderr */);
        if (debugLog)
            LLError::setDefaultLevel(LLError::LEVEL_DEBUG);
        else
            LLError::setDefaultLevel(LLError::decodeLevel(LOGTEST));
    }
    else
    {
        LLError::initForApplication(".", ".", false /* do not log to stderr */);
        LLError::setDefaultLevel(LLError::LEVEL_DEBUG);
    }

    std::string test_log = std::string(argv[0]) + ".log";
    LLFile::remove(test_log);
    LLError::logToFile(test_log);

#ifdef CTYPE_WORKAROUND
    ctype_workaround();
#endif

    if (!sMasterThreadRecorder)
    {
        sMasterThreadRecorder = new LLTrace::ThreadRecorder();
        LLTrace::set_master_thread_recorder(sMasterThreadRecorder);
    }

    // -----------------------------------------------------------------------
    // Run doctest
    // -----------------------------------------------------------------------
    doctest::Context context(dt_argc, dt_argv.data());

    // Print a summary even when all tests pass (matches legacy behaviour).
    context.setOption("no-exitcode", false);

    int result = context.run();

    // -----------------------------------------------------------------------
    // Touch the sentinel file if everything passed (legacy CMake integration)
    // -----------------------------------------------------------------------
    bool success = (result == 0) && !context.shouldExit();
    if (touch && success)
    {
        llofstream s;
        s.open(touch);
        s << "ok" << std::endl;
        s.close();
    }

    ll_cleanup_apr();

    if (context.shouldExit())
        return result;

    return result;
}
