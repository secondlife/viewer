/** 
 * @file lltransientdockablefloater.h
 * @brief Creates a panel of a specific kind for a toast.
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
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

#ifndef LL_TRANSIENTDOCKABLEFLOATER_H
#define LL_TRANSIENTDOCKABLEFLOATER_H

#include "llerror.h"
#include "llfloater.h"
#include "lldockcontrol.h"
#include "lldockablefloater.h"
#include "lltransientfloatermgr.h"

/**
 * Represents floater that can dock and managed by transient floater manager.
 * Transient floaters should be hidden if user click anywhere except defined view list.
 */
class LLTransientDockableFloater : public LLDockableFloater, LLTransientFloater
{
public:
    LOG_CLASS(LLTransientDockableFloater);
    LLTransientDockableFloater(LLDockControl* dockControl, bool uniqueDocking,
            const LLSD& key, const Params& params = getDefaultParams());
    virtual ~LLTransientDockableFloater();

    /*virtual*/ void setVisible(BOOL visible);
    /* virtual */void setDocked(bool docked, bool pop_on_undock = true);
    virtual LLTransientFloaterMgr::ETransientGroup getGroup() { return LLTransientFloaterMgr::GLOBAL; }
};

#endif /* LL_TRANSIENTDOCKABLEFLOATER_H */
