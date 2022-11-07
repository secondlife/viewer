/**
 * @file llpaneltiptoast.cpp
 * @brief Represents a base class of tip toast panels.
 *
 * $LicenseInfo:firstyear=2010&license=viewerlgpl$
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

#include "llviewerprecompiledheaders.h"

#include "llpaneltiptoast.h"

BOOL LLPanelTipToast::postBuild()
{
    mMessageText= findChild<LLUICtrl>("message");

    if (mMessageText != NULL)
    {
        mMessageText->setMouseUpCallback(boost::bind(&LLPanelTipToast::onMessageTextClick,this));
        setMouseUpCallback(boost::bind(&LLPanelTipToast::onPanelClick, this, _2, _3, _4));
    }
    else
    {
        llassert(!"Can't find child 'message' text box.");
        return FALSE;
    }

    return TRUE;
}

void LLPanelTipToast::onMessageTextClick()
{
    // notify parent toast about need hide
    LLSD info;
    info["action"] = "hide_toast";
    notifyParent(info);
}

void LLPanelTipToast::onPanelClick(S32 x, S32 y, MASK mask)
{
    if (!mMessageText->getRect().pointInRect(x, y))
    {
        onMessageTextClick();
    }
}
