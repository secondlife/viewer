/** 
 * @file lltoolindividual.cpp
 * @brief LLToolIndividual class implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

//*****************************************************************************
//
// This is a tool for selecting individual object from the
// toolbox. Handy for when you want to deal with child object
// inventory...
//
//*****************************************************************************

#include "llviewerprecompiledheaders.h"
#include "lltoolindividual.h"

#include "llfloaterreg.h"
#include "llselectmgr.h"
#include "llviewerobject.h"
#include "llviewerwindow.h"
#include "llfloatertools.h"

///----------------------------------------------------------------------------
/// Globals
///----------------------------------------------------------------------------


///----------------------------------------------------------------------------
/// Local function declarations, constants, enums, and typedefs
///----------------------------------------------------------------------------


///----------------------------------------------------------------------------
/// Class LLToolIndividual
///----------------------------------------------------------------------------

// Default constructor
LLToolIndividual::LLToolIndividual()
: LLTool(std::string("Individual"))
{
}

// Destroys the object
LLToolIndividual::~LLToolIndividual()
{
}

BOOL LLToolIndividual::handleMouseDown(S32 x, S32 y, MASK mask)
{
    gViewerWindow->pickAsync(x, y, mask, pickCallback);
    return TRUE;
}

void LLToolIndividual::pickCallback(const LLPickInfo& pick_info)
{
    LLViewerObject* obj = pick_info.getObject();
    LLSelectMgr::getInstance()->deselectAll();
    if(obj)
    {
        LLSelectMgr::getInstance()->selectObjectOnly(obj);
    }
}

BOOL LLToolIndividual::handleDoubleClick(S32 x, S32 y, MASK mask)
{
    if(!LLSelectMgr::getInstance()->getSelection()->isEmpty())
    {
        // You should already have an object selected from the mousedown.
        // If so, show its inventory. 
        LLFloaterReg::showInstance("build", "Content");
        return TRUE;
    }
    else
    {
        // Nothing selected means the first mouse click was probably
        // bad, so try again.
        return FALSE;
    }
}

void LLToolIndividual::handleSelect()
{
    const BOOL children_ok = TRUE;
    LLViewerObject* obj = LLSelectMgr::getInstance()->getSelection()->getFirstRootObject(children_ok);
    LLSelectMgr::getInstance()->deselectAll();
    if(obj)
    {
        LLSelectMgr::getInstance()->selectObjectOnly(obj);
    }
}

///----------------------------------------------------------------------------
/// Local function definitions
///----------------------------------------------------------------------------
