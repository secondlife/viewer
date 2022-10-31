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
    mSuccess(TRUE)
{ 
}


LLToolPipette::~LLToolPipette()
{ }


BOOL LLToolPipette::handleMouseDown(S32 x, S32 y, MASK mask)
{
    mSuccess = TRUE;
    mTooltipMsg.clear();
    setMouseCapture(TRUE);
    gViewerWindow->pickAsync(x, y, mask, pickCallback);
    return TRUE;
}

BOOL LLToolPipette::handleMouseUp(S32 x, S32 y, MASK mask)
{
    mSuccess = TRUE;
    LLSelectMgr::getInstance()->unhighlightAll();
    // *NOTE: This assumes the pipette tool is a transient tool.
    LLToolMgr::getInstance()->clearTransientTool();
    setMouseCapture(FALSE);
    return TRUE;
}

BOOL LLToolPipette::handleHover(S32 x, S32 y, MASK mask)
{
    gViewerWindow->setCursor(mSuccess ? UI_CURSOR_PIPETTE : UI_CURSOR_NO);
    if (hasMouseCapture()) // mouse button is down
    {
        gViewerWindow->pickAsync(x, y, mask, pickCallback);
        return TRUE;
    }
    return FALSE;
}

BOOL LLToolPipette::handleToolTip(S32 x, S32 y, MASK mask)
{
    if (mTooltipMsg.empty())
    {
        return FALSE;
    }

    LLRect sticky_rect;
    sticky_rect.setCenterAndSize(x, y, 20, 20);
    LLToolTipMgr::instance().show(LLToolTip::Params()
        .message(mTooltipMsg)
        .sticky_rect(sticky_rect));

    return TRUE;
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

void LLToolPipette::setResult(BOOL success, const std::string& msg)
{
    mTooltipMsg = msg;
    mSuccess = success;
}
