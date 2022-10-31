/**
 * @file llpanelonlinestatus.h
 * @brief Represents a class of online status tip toast panels.
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


#include "llpaneltiptoast.h"

#ifndef LL_PANELONLINESTATUS_H
#define LL_PANELONLINESTATUS_H

/**
 * Represents online tip toast panel.
 */
class LLPanelOnlineStatus : public LLPanelTipToast
{
    // disallow instantiation of this class
private:
    // grant privileges to instantiate this class to LLToastPanel
    friend class LLToastPanel;

    LLPanelOnlineStatus(const LLNotificationPtr& notification);
    virtual ~LLPanelOnlineStatus() {}
};

#endif /* LL_PANELONLINESTATUS_H */
