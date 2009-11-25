/**
 * @file   lluilistener.cpp
 * @author Nat Goodspeed
 * @date   2009-08-18
 * @brief  Implementation for lluilistener.
 * 
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * Copyright (c) 2009, Linden Research, Inc.
 * $/LicenseInfo$
 */

// Precompiled header
#include "llviewerprecompiledheaders.h"
// associated header
#include "lluilistener.h"
// STL headers
// std headers
// external library headers
// other Linden headers
#include "lluictrl.h"
#include "llerror.h"

LLUIListener::LLUIListener():
    LLEventAPI("UI",
               "LLUICtrl::CommitCallbackRegistry listener.\n"
               "Capable of invoking any function (with parameter) you can specify in XUI.")
{
    add("call",
        "Invoke the operation named by [\"function\"], passing [\"parameter\"],\n"
        "as if from a user gesture on a menu -- or a button click.",
        &LLUIListener::call,
        LLSD().insert("function", LLSD()));
}

void LLUIListener::call(const LLSD& event) const
{
    LLUICtrl::commit_callback_t* func =
        LLUICtrl::CommitCallbackRegistry::getValue(event["function"]);
    if (! func)
    {
        // This API is intended for use by a script. It's a fire-and-forget
        // API: we provide no reply. Therefore, a typo in the script will
        // provide no feedback whatsoever to that script. To rub the coder's
        // nose in such an error, crump rather than quietly ignoring it.
        LL_ERRS("LLUIListener") << "function '" << event["function"] << "' not found" << LL_ENDL;
    }
    else
    {
        // Interestingly, view_listener_t::addMenu() (addCommit(),
        // addEnable()) constructs a commit_callback_t callable that accepts
        // two parameters but discards the first. Only the second is passed to
        // handleEvent(). Therefore we feel completely safe passing NULL for
        // the first parameter.
        (*func)(NULL, event["parameter"]);
    }
}
