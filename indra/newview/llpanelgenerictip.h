/** 
 * @file llpanelgenerictip.h
 * @brief Represents a generic panel for a notifytip notifications. As example:
 * "SystemMessageTip", "Cancelled", "UploadWebSnapshotDone".
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

#ifndef LL_PANELGENERICTIP_H
#define LL_PANELGENERICTIP_H

#include "llpaneltiptoast.h"

/**
 * Represents tip toast panel that contains only one child element - message text.
 * This panel can be used for different cases of tip notifications.
 */
class LLPanelGenericTip: public LLPanelTipToast
{
    // disallow instantiation of this class
private:
    // grant privileges to instantiate this class to LLToastPanel
    friend class LLToastPanel;
    /**
     * Generic toast tip panel.
     * This is particular case of toast panel that decoupled from LLToastNotifyPanel.
     * From now LLToastNotifyPanel is deprecated and will be removed after all  panel
     * types are represented in separate classes.
     */
    LLPanelGenericTip(const LLNotificationPtr& notification);
};
#endif /* LL_PANELGENERICTIP_H */
