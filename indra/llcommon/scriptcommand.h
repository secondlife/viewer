/**
 * @file   scriptcommand.h
 * @author Nat Goodspeed
 * @date   2024-09-16
 * @brief  ScriptCommand parses a string into a script filepath plus arguments.
 *
 * $LicenseInfo:firstyear=2024&license=viewerlgpl$
 * Copyright (c) 2024, Linden Research, Inc.
 * $/LicenseInfo$
 */

#if ! defined(LL_SCRIPTCOMMAND_H)
#define LL_SCRIPTCOMMAND_H

#include "llsd.h"

#include <string>
#include <vector>

class fsyspath;

class ScriptCommand
{
public:
    /**
     * ScriptCommand accepts a command-line string to invoke an existing
     * script, which may or may not include arguments to pass to that script.
     * The constructor parses the command line into tokens using quoting and
     * escaping rules similar to bash. The first token is assumed to be the
     * script name; ScriptCommand tries to find that script on the filesystem,
     * trying each directory listed in the LLSD array of strings 'path'. When
     * it finds the script file, it stores its full pathname in 'script' and
     * the remaining tokens in 'args'.
     *
     * But if the constructor can't find the first token on 'path', it
     * considers the possibility that the whole command-line string is
     * actually a single pathname containing unescaped spaces. If that script
     * file is found on 'path', it stores its full pathname in 'script' and
     * leaves 'args' empty.
     *
     * We accept 'path' as an LLSD array rather than (e.g.)
     * std::vector<std::string> because the primary use case involves
     * retrieving 'path' from settings.
     *
     * If you also pass 'base', any directory on 'path' may be specified
     * relative to 'base'. Otherwise, every directory on 'path' must be
     * absolute.
     */
    ScriptCommand(const std::string& command, const LLSD& path=LLSD::emptyArray(),
                  const std::string& base={});

    std::string script;
    std::vector<std::string> args;

    // returns empty string if no error
    std::string error() const;

private:
    bool search(const fsyspath& script, const LLSD& path, const fsyspath& base);

    std::string mError;
};

#endif /* ! defined(LL_SCRIPTCOMMAND_H) */
