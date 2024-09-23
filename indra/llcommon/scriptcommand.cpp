/**
 * @file   scriptcommand.cpp
 * @author Nat Goodspeed
 * @date   2024-09-16
 * @brief  Implementation for scriptcommand.
 *
 * $LicenseInfo:firstyear=2024&license=viewerlgpl$
 * Copyright (c) 2024, Linden Research, Inc.
 * $/LicenseInfo$
 */

// Precompiled header
#include "linden_common.h"
// associated header
#include "scriptcommand.h"
// STL headers
// std headers
#include <sstream>
// external library headers
// other Linden headers
#include "fsyspath.h"
#include "llerror.h"
#include "llsdutil.h"
#include "llstring.h"
#include "stringize.h"

ScriptCommand::ScriptCommand(const std::string& command, const LLSD& path,
                             const std::string& base)
{
    fsyspath basepath(base);
    // Use LLStringUtil::getTokens() to parse the script command line
    args = LLStringUtil::getTokens(command,
                                   " \t\r\n", // drop_delims
                                   "",        // no keep_delims
                                   "\"'",     // either kind of quotes
                                   "\\");     // backslash escape
    // search for args[0] on paths
    if (search(args[0], path, basepath))
    {
        // The first token is in fact the script filename. Now that we've
        // found the script file, we've consumed that token. The rest are
        // command-line arguments.
        args.erase(args.begin());
        return;
    }

    // Parsing the command line produced a script file path we can't find.
    // Maybe that's because there are spaces in the original pathname that
    // were neither quoted nor escaped? See if we can find the whole original
    // command line string.
    if (search(command, path, basepath))
    {
        // Here we found script, using the whole input command line as its
        // pathname. Discard any parts of it we mistook for command-line
        // arguments.
        args.clear();
        return;
    }

    // Couldn't find the script either way. Is it because we can't even check
    // existence?
    if (! mError.empty())
    {
        return;
    }

    // No, existence check works, we just can't find the script.
    std::ostringstream msgstream;
    msgstream << "Can't find script file " << std::quoted(args[0]);
    if (command != args[0])
    {
        msgstream << " or " << std::quoted(command);
    }
    if (path.size() > 0)
    {
        msgstream << " on " << path;
    }
    if (! base.empty())
    {
        msgstream << " relative to " << base;
    }
    mError = msgstream.str();
    LL_WARNS("Lua") << mError << LL_ENDL;
}

bool ScriptCommand::search(const fsyspath& script, const LLSD& paths, const fsyspath& base)
{
    for (const auto& path : llsd::inArray(paths))
    {
        // If a path is already absolute, (otherpath / path) preserves it.
        // Explicitly instantiate fsyspath for every string conversion to
        // properly convert UTF-8 filename strings on Windows.
        fsyspath absscript{ base / fsyspath(path.asString()) / script };
        bool exists;
        try
        {
            exists = std::filesystem::exists(absscript);
        }
        catch (const std::filesystem::filesystem_error& exc)
        {
            mError = stringize("Can't check existence: ", exc.what());
            LL_WARNS("Lua") << mError << LL_ENDL;
            return false;
        }
        if (exists)
        {
            this->script = absscript.string();
            return true;
        }
    }
    return false;
}

std::string ScriptCommand::error() const
{
    return mError;
}
