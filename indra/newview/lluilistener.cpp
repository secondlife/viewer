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
#include "llviewerwindow.h" // to get root view
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
        LLSD().with("path", LLSD()));
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

const LLView* resolve_path(const LLView* context, const std::string path) 
{
	std::vector<std::string> parts;
	const std::string delims("/");
	LLStringUtilBase<char>::getTokens(path, parts, delims);
	
	bool recurse = false;
	for (std::vector<std::string>::iterator it = parts.begin();
		 it != parts.end() && context; it++) 
    {
		std::string part = *it;
		
		if (part.length() == 0) 
        {
			// Allow "foo//bar" meaning "descendant named bar"
			recurse = true;
        }
		else
        {
			const LLView* found = context->findChildView(part, recurse);
			if (!found) 
            {
				return NULL;
            }
			else 
            {
				context = found;
            }
			recurse = false;
        }
    }
	
    return NULL;
}

void LLUIListener::getValue(const LLSD&event) const
{
    LLSD reply;
    
    const LLView* root = (LLView*)(gViewerWindow->getRootView());
    const LLView* view = resolve_path(root, event["path"].asString());
    const LLUICtrl* ctrl(dynamic_cast<const LLUICtrl*>(view));

    if (ctrl) 
    {
        reply["value"] = ctrl->getValue();
    }
    else
    {
        // *TODO: ??? return something indicating failure to resolve
    }
    
    sendReply(reply, event, "reply");
}
