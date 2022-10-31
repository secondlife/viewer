/** 
 * @file llfloaternotificationsconsole.h
 * @brief Debugging console for unified notifications.
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

#ifndef LL_LLFLOATER_NOTIFICATIONS_CONSOLE_H
#define LL_LLFLOATER_NOTIFICATIONS_CONSOLE_H

#include "llfloater.h"
#include "lllayoutstack.h"
//#include "llnotificationsutil.h"

class LLNotification;

class LLFloaterNotificationConsole : 
    public LLFloater
{
    friend class LLFloaterReg;

public:

    // LLPanel
    BOOL postBuild();

    void addChannel(const std::string& type, bool open = false);
    void updateResizeLimits(LLLayoutStack &stack);

    void removeChannel(const std::string& type);
    void updateResizeLimits();

private:
    LLFloaterNotificationConsole(const LLSD& key);  
    void onClickAdd();
};


/*
 * @brief Pop-up debugging view of a generic new notification.
 */
class LLFloaterNotification : public LLFloater
{
public:
    LLFloaterNotification(LLNotification* note);

    // LLPanel
    BOOL postBuild();
    void respond();

private:
    static void onCommitResponse(LLUICtrl* ctrl, void* data) { ((LLFloaterNotification*)data)->respond(); }
    LLNotification* mNote;
};
#endif

