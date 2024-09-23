/**
 * @file lltoolpipette.cpp
 * @brief LLToolPipette class implementation
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
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

/**
 * A tool to pick texture entry infro from objects in world (color/texture)
 */

#include "llviewerprecompiledheaders.h"

// File includes
#include "lltoolpipette.h"

// Library includes
#include "lltooltip.h"

// Viewer includes
#include "llviewerobjectlist.h"
#include "llviewerwindow.h"
#include "llselectmgr.h"
#include "lltoolmgr.h"

//
// Member functions
//

LLToolPipette::LLToolPipette()
:   LLTool(std::string("Pipette")),
    mSuccess(true)
{
}


LLToolPipette::~LLToolPipette()
{ }


bool LLToolPipette::handleMouseDown(S32 x, S32 y, MASK mask)
{
    mSuccess = true;
    mTooltipMsg.clear();
    setMouseCapture(true);
    gViewerWindow->pickAsync(x, y, mask, pickCallback);
    return true;
}

bool LLToolPipette::handleMouseUp(S32 x, S32 y, MASK mask)
{
    mSuccess = true;
    LLSelectMgr::getInstance()->unhighlightAll();
    // *NOTE: This assumes the pipette tool is a transient tool.
    LLToolMgr::getInstance()->clearTransientTool();
    setMouseCapture(false);
    return true;
}

bool LLToolPipette::handleHover(S32 x, S32 y, MASK mask)
{
    gViewerWindow->setCursor(mSuccess ? UI_CURSOR_PIPETTE : UI_CURSOR_NO);
    if (hasMouseCapture()) // mouse button is down
    {
        gViewerWindow->pickAsync(x, y, mask, pickCallback);
        return true;
    }
    return false;
}

bool LLToolPipette::handleToolTip(S32 x, S32 y, MASK mask)
{
    if (mTooltipMsg.empty())
    {
        return false;
    }

    LLRect sticky_rect;
    sticky_rect.setCenterAndSize(x, y, 20, 20);
    LLToolTipMgr::instance().show(LLToolTip::Params()
        .message(mTooltipMsg)
        .sticky_rect(sticky_rect));

    return true;
}

void LLToolPipette::setTextureEntry(const LLTextureEntry* entry)
{
    if (entry)
    {
        mTextureEntry = *entry;
        mSignal(mTextureEntry);
    }
}

void LLToolPipette::pickCallback(const LLPickInfo& pick_info)
{
    LLViewerObject* hit_obj = pick_info.getObject();
    LLSelectMgr::getInstance()->unhighlightAll();

    // if we clicked on a face of a valid prim, save off texture entry data
    if (hit_obj &&
        hit_obj->getPCode() == LL_PCODE_VOLUME &&
        pick_info.mObjectFace != -1)
    {
        //TODO: this should highlight the selected face only
        LLSelectMgr::getInstance()->highlightObjectOnly(hit_obj);
        const LLTextureEntry* entry = hit_obj->getTE(pick_info.mObjectFace);
        LLToolPipette::getInstance()->setTextureEntry(entry);
    }
}

void LLToolPipette::setResult(bool success, const std::string& msg)
{
    mTooltipMsg = msg;
    mSuccess = success;
}
