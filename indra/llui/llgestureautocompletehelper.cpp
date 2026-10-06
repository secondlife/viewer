/**
 * @file llgestureautocompletehelper.cpp
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2026, Linden Research, Inc.
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

#include "linden_common.h"

#include "llgestureautocompletehelper.h"

#include "llfloater.h"
#include "llfloaterreg.h"
#include "llfocusmgr.h"
#include "lluictrl.h"

constexpr char GESTURE_AUTOCOMPLETE_FLOATER[] = "gesture_autocomplete_picker";

bool LLGestureAutocompleteHelper::isActive(const LLUICtrl* ctrl) const
{
    return mHostHandle.get() == ctrl;
}

void LLGestureAutocompleteHelper::showHelper(
    LLUICtrl* host_ctrl,
    const std::vector<Row>& rows,
    size_t total,
    std::function<void(std::string)> commit_cb)
{
    if (mHelperHandle.isDead())
    {
        LLFloater* helper_floater = LLFloaterReg::getInstance(GESTURE_AUTOCOMPLETE_FLOATER);
        mHelperHandle = helper_floater->getHandle();
        mHelperCommitConn = helper_floater->setCommitCallback(
            [this](LLUICtrl*, const LLSD& param) { onCommitGesture(param.asString()); });
    }

    setHostCtrl(host_ctrl);
    mRows = rows;
    mTotal = total;
    mGestureCommitCb = commit_cb;

    S32 floater_x, floater_y;
    LLRect host_rect = host_ctrl->getRect();
    if (!host_ctrl->localPointToOtherView(0, host_rect.getHeight(), &floater_x, &floater_y, gFloaterView))
    {
        LL_WARNS() << "Cannot show gesture autocomplete helper for non-floater controls." << LL_ENDL;
        return;
    }

    LLFloater* helper_floater = mHelperHandle.get();
    LLRect rect = helper_floater->getRect();
    rect.setLeftTopAndSize(floater_x, floater_y + rect.getHeight(), rect.getWidth(), rect.getHeight());
    helper_floater->setRect(rect);

    refreshPicker();
}

void LLGestureAutocompleteHelper::hideHelper(const LLUICtrl* ctrl)
{
    if (ctrl && !isActive(ctrl))
    {
        return;
    }

    setHostCtrl(nullptr);
}

bool LLGestureAutocompleteHelper::handleKey(const LLUICtrl* ctrl, KEY key, MASK mask)
{
    if (mHelperHandle.isDead() || !isActive(ctrl))
    {
        return false;
    }

    return mHelperHandle.get()->handleKey(key, mask, true);
}

void LLGestureAutocompleteHelper::onCommitGesture(const std::string& trigger)
{
    if (!mHostHandle.isDead() && mGestureCommitCb)
    {
        mGestureCommitCb(trigger);
    }

    hideHelper(getHostCtrl());
}

void LLGestureAutocompleteHelper::refreshPicker()
{
    if (mHelperHandle.isDead())
    {
        return;
    }

    LLFloater* helper_floater = mHelperHandle.get();

    if (helper_floater->isShown())
    {
        helper_floater->onOpen(LLSD());
    }
    else
    {
        helper_floater->openFloater(LLSD());
    }
}

void LLGestureAutocompleteHelper::setHostCtrl(LLUICtrl* host_ctrl)
{
    const LLUICtrl* cur_host_ctrl = mHostHandle.get();

    if (cur_host_ctrl != host_ctrl)
    {
        mHostCtrlFocusLostConn.disconnect();
        mHostHandle.markDead();
        mGestureCommitCb = {};
        mRows.clear();
        mTotal = 0;

        if (!mHelperHandle.isDead())
        {
            mHelperHandle.get()->closeFloater();
        }

        if (host_ctrl)
        {
            mHostHandle = host_ctrl->getHandle();
            mHostCtrlFocusLostConn = host_ctrl->setFocusLostCallback(
                [this](auto*)
                {
                    // Scroll list grabs focus on click.
                    // Keep focus on the host when the click was ours.
                    LLFloater* helper_floater = mHelperHandle.get();
                    if (helper_floater && gFocusMgr.childHasKeyboardFocus(helper_floater))
                    {
                        if (LLUICtrl* host = getHostCtrl())
                        {
                            host->setFocus(true);
                        }
                        return;
                    }

                    hideHelper(getHostCtrl());
                });
        }
    }
}
