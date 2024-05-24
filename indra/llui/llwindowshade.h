/**
 * @file llwindowshade.h
 * @brief Notification dialog that slides down and optionally disabled a piece of UI
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

#ifndef LL_LLWINDOWSHADE_H
#define LL_LLWINDOWSHADE_H

#include "lluictrl.h"
#include "llnotifications.h"
#include "lluiimage.h"

class LLWindowShade : public LLUICtrl
{
public:
    struct Params : public LLInitParam::Block<Params, LLUICtrl::Params>
    {
        Optional<LLUIImage*>            bg_image;
        Optional<LLUIColor>             text_color,
                                        shade_color;
        Optional<bool>                  modal,
                                        can_close;

        Params();
    };

    void show(LLNotificationPtr);
    /*virtual*/ void draw();
    void hide();

    bool isShown() const;

    void setBackgroundImage(LLUIImage* image);
    void setTextColor(LLColor4 color);
    void setCanClose(bool can_close);

private:
    void displayLatestNotification();
    LLNotificationPtr getCurrentNotification();
    friend class LLUICtrlFactory;

    LLWindowShade(const Params& p);
    void initFromParams(const Params& params);

    void onCloseNotification();
    void onClickNotificationButton(const std::string& name);
    void onEnterNotificationText(LLUICtrl* ctrl, const std::string& name);
    void onClickIgnore(LLUICtrl* ctrl);

    std::vector<LLNotificationPtr>  mNotifications;
    LLSD                mNotificationResponse;
    bool                mModal;
    S32                 mFormHeight;
    LLUIColor           mTextColor;
};

#endif // LL_LLWINDOWSHADE_H
