/**
 * @file   wrapllerrs.h
 * @author Nat Goodspeed
 * @date   2009-03-11
 * @brief  Define a class useful for unit tests that engage llerrs (LL_ERRS) functionality
 * 
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * Copyright (c) 2009, Linden Research, Inc.
 * $/LicenseInfo$
 */

#if ! defined(LL_WRAPLLERRS_H)
#define LL_WRAPLLERRS_H

#include "llerrorcontrol.h"

struct WrapLL_ERRS
{
    WrapLL_ERRS():
        // Resetting Settings discards the default Recorder that writes to
        // stderr. Otherwise, expected llerrs (LL_ERRS) messages clutter the
        // console output of successful tests, potentially confusing things.
        mPriorErrorSettings(LLError::saveAndResetSettings()),
        // Save shutdown function called by LL_ERRS
        mPriorFatal(LLError::getFatalFunction())
    {
        // Make LL_ERRS call our own operator() method
        LLError::setFatalFunction(boost::bind(&WrapLL_ERRS::operator(), this, _1));
    }

    ~WrapLL_ERRS()
    {
        LLError::setFatalFunction(mPriorFatal);
        LLError::restoreSettings(mPriorErrorSettings);
    }

    struct FatalException: public std::runtime_error
    {
        FatalException(const std::string& what): std::runtime_error(what) {}
    };

    void operator()(const std::string& message)
    {
        // Save message for later in case consumer wants to sense the result directly
        error = message;
        // Also throw an appropriate exception since calling code is likely to
        // assume that control won't continue beyond LL_ERRS.
        throw FatalException(message);
    }

    std::string error;
    LLError::Settings* mPriorErrorSettings;
    LLError::FatalFunction mPriorFatal;
};

#endif /* ! defined(LL_WRAPLLERRS_H) */
