/**
 * @file lltest_harness.cpp
 * @brief Shared test harness bootstrap for APR, logging, and LLTrace.
 *
 * $LicenseInfo:firstyear=2025&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2025,
 * Linden Research, Inc.
 * $/LicenseInfo$
 */

#include "linden_common.h"

#include "lltest_harness.h"

#include "llerrorcontrol.h"
#include "lltrace.h"
#include "lltracethreadrecorder.h"
#include "namedtempfile.h"

// the CTYPE_WORKAROUND is needed for linux dev stations that don't
// have the broken libc6 packages needed by our out-of-date static
// libs (such as libcrypto and libcurl). -- Leviathan 20060113
#ifdef CTYPE_WORKAROUND
#   include "ctype_workaround.h"
#endif

#include <boost/core/noncopyable.hpp>

#include <fstream>

class RecordToTempFile : public LLError::Recorder, public boost::noncopyable
{
public:
    RecordToTempFile()
        : LLError::Recorder(),
        boost::noncopyable(),
        mTempFile("log", ""),
        mFile(mTempFile.getName().c_str())
    {
    }

    virtual ~RecordToTempFile()
    {
        mFile.close();
    }

    virtual void recordMessage(LLError::ELevel level, const std::string& message)
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
    llofstream mFile;
};

class LLReplayLogReal: public LLReplayLog, public boost::noncopyable
{
public:
    LLReplayLogReal(LLError::ELevel level)
        : LLReplayLog(),
        boost::noncopyable(),
        mOldSettings(LLError::saveAndResetSettings()),
        mRecorder(new RecordToTempFile())
    {
        LLError::setDefaultLevel(level);
        LLError::addRecorder(mRecorder);
    }

    virtual ~LLReplayLogReal()
    {
        LLError::removeRecorder(mRecorder);
        LLError::restoreSettings(mOldSettings);
    }

    virtual void reset()
    {
        std::dynamic_pointer_cast<RecordToTempFile>(mRecorder)->reset();
    }

    virtual void replay(std::ostream& out)
    {
        std::dynamic_pointer_cast<RecordToTempFile>(mRecorder)->replay(out);
    }

private:
    LLError::SettingsStoragePtr mOldSettings;
    LLError::RecorderPtr mRecorder;
};

static LLTrace::ThreadRecorder* sMasterThreadRecorder = nullptr;

void lltest_init_apr()
{
    ll_init_apr();
}

void lltest_shutdown_apr()
{
    ll_cleanup_apr();
}

static std::shared_ptr<LLReplayLog> lltest_init_logging_impl(
    const std::string& app_name,
    const char* logtest,
    const char* logfail)
{
    const char* LOGTEST = logtest;
    const char* LOGFAIL = logfail;

    std::shared_ptr<LLReplayLog> replayer{ std::make_shared<LLReplayLog>() };

    if (LOGTEST && *LOGTEST)
    {
        LLError::initForApplication(".", ".", true);
        LLError::setDefaultLevel(LLError::decodeLevel(LOGTEST));
    }
    else
    {
        LLError::initForApplication(".", ".", false);
        LLError::setDefaultLevel(LLError::LEVEL_DEBUG);
        if (LOGFAIL && *LOGFAIL)
        {
            LLError::ELevel level = LLError::decodeLevel(LOGFAIL);
            replayer.reset(new LLReplayLogReal(level));
        }
    }

    std::string test_log = app_name + ".log";
    LLFile::remove(test_log);
    LLError::logToFile(test_log);

#ifdef CTYPE_WORKAROUND
    ctype_workaround();
#endif

    return replayer;
}

std::shared_ptr<LLReplayLog> lltest_init_logging(
    const std::string& app_name,
    const char* logtest,
    const char* logfail)
{
    return lltest_init_logging_impl(app_name, logtest, logfail);
}

std::shared_ptr<LLReplayLog> lltest_init_logging_no_fatal(
    const std::string& app_name,
    const char* logtest,
    const char* logfail)
{
    return lltest_init_logging_impl(app_name, logtest, logfail);
}

void lltest_init_trace()
{
    if (!sMasterThreadRecorder)
    {
        sMasterThreadRecorder = new LLTrace::ThreadRecorder();
        LLTrace::set_master_thread_recorder(sMasterThreadRecorder);
    }
}
