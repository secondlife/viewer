/**
 * @file   commoncontrol.h
 * @author Nat Goodspeed
 * @date   2022-06-08
 * @brief  Access LLViewerControl LLEventAPI, if process has one.
 *
 * $LicenseInfo:firstyear=2022&license=viewerlgpl$
 * Copyright (c) 2022, Linden Research, Inc.
 * $/LicenseInfo$
 */

#if ! defined(LL_COMMONCONTROL_H)
#define LL_COMMONCONTROL_H

#include <vector>
#include "llexception.h"
#include "llsd.h"

namespace LL
{
    class CommonControl
    {
    public:
        struct Error: public LLException
        {
            Error(const std::string& what): LLException(what) {}
        };

        /// Exception thrown if there's no LLViewerControl LLEventAPI
        struct NoListener: public Error
        {
            NoListener(const std::string& what): Error(what) {}
        };

        struct ParamError: public Error
        {
            ParamError(const std::string& what): Error(what) {}
        };

        /// set control group.key to defined default value
        static
        LLSD set_default(const std::string& group, const std::string& key);

        /// set control group.key to specified value
        static
        LLSD set(const std::string& group, const std::string& key, const LLSD& value);

        /// toggle boolean control group.key
        static
        LLSD toggle(const std::string& group, const std::string& key);

        /// get the definition for control group.key, (! isDefined()) if bad
        /// ["name"], ["type"], ["value"], ["comment"]
        static
        LLSD get_def(const std::string& group, const std::string& key);

        /// get the value of control group.key
        static
        LLSD get(const std::string& group, const std::string& key);

        /// get defined groups
        static
        std::vector<std::string> get_groups();

        /// get definitions for all variables in group
        static
        LLSD get_vars(const std::string& group);

    private:
        static
        LLSD access(const LLSD& params);
    };
} // namespace LL

#endif /* ! defined(LL_COMMONCONTROL_H) */
