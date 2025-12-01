/**
 * @file lltest_harness.h
 * @brief Shared test harness bootstrap for APR, logging, and LLTrace.
 *
 * $LicenseInfo:firstyear=2025&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2025,
 * Linden Research, Inc.
 * $/LicenseInfo$
 */

#ifndef LL_LLTEST_HARNESS_H
#define LL_LLTEST_HARNESS_H

#include <iosfwd>
#include <memory>
#include <string>

class LLReplayLog
{
public:
    LLReplayLog() {}
    virtual ~LLReplayLog() {}

    virtual void reset() {}
    virtual void replay(std::ostream&) {}
};

void lltest_init_apr();

void lltest_shutdown_apr();

/**
 * Initialize logging for the test harness.
 *
 * @param app_name   Name of the test executable (typically argv[0]).
 * @param logtest    Effective LOGTEST level (environment and CLI overrides).
 * @param logfail    LOGFAIL level from the environment, may be null.
 *
 * Returns the replay logger instance to be used by test callbacks.
 *
 * This variant installs the fatal handler used by the TUT runner.
 */
std::shared_ptr<LLReplayLog> lltest_init_logging(
    const std::string& app_name,
    const char* logtest,
    const char* logfail);

/**
 * Initialize logging for the test harness without installing a fatal handler.
 *
 * Intended for doctest-based executables that do not want LL_ERRS to abort
 * the entire test run via TUT's fail mechanism.
 */
std::shared_ptr<LLReplayLog> lltest_init_logging_no_fatal(
    const std::string& app_name,
    const char* logtest,
    const char* logfail);

/// Initialize LLTrace master thread recorder for tests.
void lltest_init_trace();

#endif // LL_LLTEST_HARNESS_H
