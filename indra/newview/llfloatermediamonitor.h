/**
 * @file llfloatermediamonitor.cpp
 * @author Callum Prentice
 * @brief Debug floater containing a list of active media sources
 *        and data for each such as URL, priority, location etc.
 *
 * $LicenseInfo:firstyear=2008&license=viewerlgpl$
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

#pragma once

#include "llfloater.h"
#include "lluuid.h"

class LLEventTimer;
class LLScrollListCtrl;
class LLButton;
class LLSlider;
class LLToggleableMenu;

class LLFloaterMediaMonitor:
    public LLFloater
{
        friend class LLFloaterReg;
    private:
        LLFloaterMediaMonitor(const LLSD& key);
        bool postBuild() override;
        void onOpen(const LLSD& key) override;
        void onClose(bool app_quitting) override;
        ~LLFloaterMediaMonitor();

        void updateDisplayList();

        // Refreshes mZoomBtn/mUnzoomBtn/mVolumeSlider to match whatever's currently
        // selected (or resets them to a disabled/default state if nothing is), and
        // disables Zoom for non-Prim rows (no single object for the camera to zoom
        // onto). Called both on selection change and after each updateDisplayList()
        // refresh, since the list fully rebuilds itself every tick -- see selectByID()
        // in updateDisplayList() for how the selection itself survives that rebuild.
        void updateSelectedMediaControls();

        void onRefreshBtn();
        void onCloseBtn();
        void onZoomBtn();
        void onUnzoomBtn();
        void onVolumeChange();
        void onRightClickItem(LLUICtrl* ctrl, S32 x, S32 y);
        void onMenuAction(const LLSD& userdata);

        LLScrollListCtrl* mActiveMediaList;
        LLButton* mZoomBtn;
        LLButton* mUnzoomBtn;
        LLSlider* mVolumeSlider;
        LLToggleableMenu* mContextMenu;

        // Which media (if any) our own Zoom button last zoomed the camera onto --
        // deliberately NOT LLViewerMediaFocus::isZoomedOnMedia()/mCurrentZoom, since
        // that's the shared EZoomLevel state machine NMP and the in-world media
        // controls also drive. Tracking our own zoomed-media id keeps Media Monitor's
        // exact-fit zoom (see onZoomBtn()) fully self-contained -- it can't perturb,
        // and isn't perturbed by, zoom state set from those other UIs.
        LLUUID mZoomedMediaId;

        // Started in onOpen(), stopped in onClose() -- no point re-scanning the
        // media list on a timer while the floater isn't even visible.
        LLEventTimer* mRefreshTimer = nullptr;

        // The XUI-declared title (e.g. localized), cached in postBuild() so
        // updateDisplayList() can append " - N instances" without hardcoding
        // the base "Media Monitor" string here.
        std::string mBaseTitle;
};
