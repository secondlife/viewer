/** 
 * @file lltransientdockablefloater.cpp
 * @brief Creates a panel of a specific kind for a toast
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
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

#include "lltransientfloatermgr.h"
#include "lltransientdockablefloater.h"
#include "llfloaterreg.h"


LLTransientDockableFloater::LLTransientDockableFloater(LLDockControl* dockControl, bool uniqueDocking,
        const LLSD& key, const Params& params) :
        LLDockableFloater(dockControl, uniqueDocking, key, params)
{
    LLTransientFloaterMgr::getInstance()->registerTransientFloater(this);
    LLTransientFloater::init(this);
}

LLTransientDockableFloater::~LLTransientDockableFloater()
{
    LLTransientFloaterMgr::getInstance()->unregisterTransientFloater(this);
    LLView* dock = getDockWidget();
    LLTransientFloaterMgr::getInstance()->removeControlView(
            LLTransientFloaterMgr::DOCKED, this);
    if (dock != NULL)
    {
        LLTransientFloaterMgr::getInstance()->removeControlView(
                LLTransientFloaterMgr::DOCKED, dock);
    }
}

void LLTransientDockableFloater::setVisible(BOOL visible)
{
    LLView* dock = getDockWidget();
    if(visible && isDocked())
    {
        LLTransientFloaterMgr::getInstance()->addControlView(LLTransientFloaterMgr::DOCKED, this);
        if (dock != NULL)
        {
            LLTransientFloaterMgr::getInstance()->addControlView(LLTransientFloaterMgr::DOCKED, dock);
        }
    }
    else
    {
        LLTransientFloaterMgr::getInstance()->removeControlView(LLTransientFloaterMgr::DOCKED, this);
        if (dock != NULL)
        {
            LLTransientFloaterMgr::getInstance()->removeControlView(LLTransientFloaterMgr::DOCKED, dock);
        }
    }

    LLDockableFloater::setVisible(visible);
}

void LLTransientDockableFloater::setDocked(bool docked, bool pop_on_undock)
{
    LLView* dock = getDockWidget();
    if(docked)
    {
        LLTransientFloaterMgr::getInstance()->addControlView(LLTransientFloaterMgr::DOCKED, this);
        if (dock != NULL)
        {
            LLTransientFloaterMgr::getInstance()->addControlView(LLTransientFloaterMgr::DOCKED, dock);
        }
    }
    else
    {
        LLTransientFloaterMgr::getInstance()->removeControlView(LLTransientFloaterMgr::DOCKED, this);
        if (dock != NULL)
        {
            LLTransientFloaterMgr::getInstance()->removeControlView(LLTransientFloaterMgr::DOCKED, dock);
        }
    }

    LLDockableFloater::setDocked(docked, pop_on_undock);
}
