/**
 * @file   lluilistener.cpp
 * @author Nat Goodspeed
 * @date   2009-08-18
 * @brief  Implementation for lluilistener.
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

// Precompiled header
#include "llviewerprecompiledheaders.h"
// associated header
#include "lluilistener.h"
// STL headers
// std headers
// external library headers
// other Linden headers
#include "llui.h" // getRootView(), resolvePath()
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
        LLSD().with("function", LLSD()));

    add("getValue",
        "For the UI control identified by the path in [\"path\"], return the control's\n"
        "current value as [\"value\"] reply.",
        &LLUIListener::getValue,
        LLSDMap("path", LLSD())("reply", LLSD()));
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

void LLUIListener::getValue(const LLSD&event) const
{
    LLSD reply = LLSD::emptyMap();

    const LLView* root = LLUI::getInstance()->getRootView();
    const LLView* view = LLUI::getInstance()->resolvePath(root, event["path"].asString());
    const LLUICtrl* ctrl(dynamic_cast<const LLUICtrl*>(view));

    if (ctrl)
    {
        reply["value"] = ctrl->getValue();
    }
    else
    {
        // *TODO: ??? return something indicating failure to resolve
    }

    sendReply(reply, event);
}
