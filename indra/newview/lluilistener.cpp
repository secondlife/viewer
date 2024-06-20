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

#define THROTTLE_PERIOD 1.5 // required seconds between throttled functions
#define MIN_THROTTLE 0.5

LLUIListener::LLUIListener():
    LLEventAPI("UI",
               "LLUICtrl::CommitCallbackRegistry listener.\n"
               "Capable of invoking any function (with parameter) you can specify in XUI.")
{
    add("call",
        "Invoke the operation named by [\"function\"], passing [\"parameter\"],\n"
        "as if from a user gesture on a menu -- or a button click.",
        &LLUIListener::call,
        llsd::map("function", LLSD(), "reply", LLSD()));

    add("getValue",
        "For the UI control identified by the path in [\"path\"], return the control's\n"
        "current value as [\"value\"] reply.",
        &LLUIListener::getValue,
        llsd::map("path", LLSD(), "reply", LLSD()));
}

typedef LLUICtrl::CommitCallbackInfo cb_info;
void LLUIListener::call(const LLSD& event)
{
    Response response(LLSD(), event);
    LLUICtrl::CommitCallbackInfo *info = LLUICtrl::CommitCallbackRegistry::getValue(event["function"]);
    if (!info || !info->callback_func)
    {
        return response.error(stringize("Function ", std::quoted(event["function"].asString()), " was not found"));
    }
    if (info->handle_untrusted == cb_info::UNTRUSTED_BLOCK) 
    {
        return response.error(stringize("Function ", std::quoted(event["function"].asString()), " may not be called from the script"));
    }

    //Separate UNTRUSTED_THROTTLE and UNTRUSTED_ALLOW functions to have different timeout
    F64 *throttlep, period;
    if (info->handle_untrusted == cb_info::UNTRUSTED_THROTTLE)
    {
        throttlep = &mLastUntrustedThrottle;
        period = THROTTLE_PERIOD;
    }
    else
    {
        throttlep = &mLastMinThrottle;
        period = MIN_THROTTLE;
    }

    F64 cur_time = LLTimer::getElapsedSeconds();
    F64 time_delta = *throttlep + period;
    if (cur_time < time_delta)
    {
        LL_WARNS("LLUIListener") << "Throttled function " << std::quoted(event["function"].asString()) << LL_ENDL;
        return;
    }
    *throttlep = cur_time;

    // Interestingly, view_listener_t::addMenu() (addCommit(),
    // addEnable()) constructs a commit_callback_t callable that accepts
    // two parameters but discards the first. Only the second is passed to
    // handleEvent(). Therefore we feel completely safe passing NULL for
    // the first parameter.
    (info->callback_func)(NULL, event["parameter"]);
}

void LLUIListener::getValue(const LLSD&event) const
{
    Response response(LLSD(), event);

    const LLView* root = LLUI::getInstance()->getRootView();
    const LLView* view = LLUI::getInstance()->resolvePath(root, event["path"].asString());
    const LLUICtrl* ctrl(dynamic_cast<const LLUICtrl*>(view));

    if (ctrl) 
    {
        response["value"] = ctrl->getValue();
    }
    else
    {
        response.error(stringize("UI control ", std::quoted(event["path"].asString()), " was not found"));
    }
}
