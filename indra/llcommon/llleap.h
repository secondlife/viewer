/**
 * @file   llleap.h
 * @author Nat Goodspeed
 * @date   2012-02-20
 * @brief  Class that implements "LLSD Event API Plugin"
 * 
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Copyright (c) 2012, Linden Research, Inc.
 * $/LicenseInfo$
 */

#if ! defined(LL_LLLEAP_H)
#define LL_LLLEAP_H

#include "llinstancetracker.h"
#include <string>
#include <vector>
#include <stdexcept>

/**
 * LLSD Event API Plugin class. Because instances are managed by
 * LLInstanceTracker, you can instantiate LLLeap and forget the instance
 * unless you need it later. Each instance manages an LLProcess; when the
 * child process terminates, LLLeap deletes itself. We don't require a unique
 * LLInstanceTracker key.
 *
 * The fact that a given LLLeap instance vanishes when its child process
 * terminates makes it problematic to store an LLLeap* anywhere. Any stored
 * LLLeap* pointer should be validated before use by
 * LLLeap::getInstance(LLLeap*) (see LLInstanceTracker).
 */
class LL_COMMON_API LLLeap: public LLInstanceTracker<LLLeap>
{
public:
    /**
     * Pass a brief string description, mostly for logging purposes. The desc
     * need not be unique, but obviously the clearer we can make it, the
     * easier these things will be to debug. The strings are the command line
     * used to launch the desired plugin process.
     *
     * Pass exc=false to suppress LLLeap::Error exception. Obviously in that
     * case the caller cannot discover the nature of the error, merely that an
     * error of some kind occurred (because create() returned NULL). Either
     * way, the error is logged.
     */
    static LLLeap* create(const std::string& desc, const std::vector<std::string>& plugin,
                          bool exc=true);

    /**
     * Pass a brief string description, mostly for logging purposes. The desc
     * need not be unique, but obviously the clearer we can make it, the
     * easier these things will be to debug. Pass a command-line string
     * to launch the desired plugin process.
     *
     * Pass exc=false to suppress LLLeap::Error exception. Obviously in that
     * case the caller cannot discover the nature of the error, merely that an
     * error of some kind occurred (because create() returned NULL). Either
     * way, the error is logged.
     */
    static LLLeap* create(const std::string& desc, const std::string& plugin,
                          bool exc=true);

    /**
     * Exception thrown for invalid create() arguments, e.g. no plugin
     * program. This is more resiliant than an LL_ERRS failure, because the
     * string(s) passed to create() might come from an external source. This
     * way the caller can catch LLLeap::Error and try to recover.
     */
    struct Error: public std::runtime_error
    {
        Error(const std::string& what): std::runtime_error(what) {}
    };

    virtual ~LLLeap();

protected:
    LLLeap();
};

#endif /* ! defined(LL_LLLEAP_H) */
