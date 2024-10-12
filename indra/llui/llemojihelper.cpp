/**
* @file llemojihelper.h
* @brief Header file for LLEmojiHelper
*
* $LicenseInfo:firstyear=2014&license=viewerlgpl$
* Second Life Viewer Source Code
* Copyright (C) 2014, Linden Research, Inc.
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

#include "llemojidictionary.h"
#include "llemojihelper.h"
#include "llfloater.h"
#include "llfloaterreg.h"
#include "lluictrl.h"

// ============================================================================
// Constants
//

constexpr char DEFAULT_EMOJI_HELPER_FLOATER[] = "emoji_picker";
constexpr S32 HELPER_FLOATER_OFFSET_X = 0;
constexpr S32 HELPER_FLOATER_OFFSET_Y = 0;

// ============================================================================
// LLEmojiHelper
//

std::string LLEmojiHelper::getToolTip(llwchar ch) const
{
    return LLEmojiDictionary::instance().getNameFromEmoji(ch);
}

bool LLEmojiHelper::isActive(const LLUICtrl* ctrl_p) const
{
    return mHostHandle.get() == ctrl_p;
}

// static
bool LLEmojiHelper::isCursorInEmojiCode(const LLWString& wtext, S32 cursorPos, S32* pShortCodePos)
{
    // If the cursor is currently on a colon start the check one character further back
    S32 shortCodePos = (cursorPos == 0 || L':' != wtext[cursorPos - 1]) ? cursorPos : cursorPos - 1;

    auto isPartOfShortcode = [](llwchar ch) {
        switch (ch)
        {
            case L'-':
            case L'_':
            case L'+':
                return true;
            default:
                return LLStringOps::isAlnum(ch);
        }
    };
    while (shortCodePos > 1 && isPartOfShortcode(wtext[shortCodePos - 1]))
    {
        shortCodePos--;
    }

    bool isShortCode = (cursorPos - shortCodePos >= 2) && (L':' == wtext[shortCodePos - 1]);
    if(isShortCode && (shortCodePos >= 2) && isdigit(wtext[shortCodePos - 2])) // <TS:3T> Add qualifier to avoid emoji pop-up when typing times.
        isShortCode = false;
    if (pShortCodePos)
        *pShortCodePos = (isShortCode) ? shortCodePos - 1 : -1;
    return isShortCode;
}

void LLEmojiHelper::showHelper(LLUICtrl* hostctrl_p, S32 local_x, S32 local_y, const std::string& short_code, std::function<void(llwchar)> cb)
{
    // Commit immediately if the user already typed a full shortcode
    if (const auto* emojiDescrp = LLEmojiDictionary::instance().getDescriptorFromShortCode(short_code))
    {
        cb(emojiDescrp->Character);
        hideHelper();
        return;
    }

    if (mHelperHandle.isDead())
    {
        LLFloater* pHelperFloater = LLFloaterReg::getInstance(DEFAULT_EMOJI_HELPER_FLOATER);
        mHelperHandle = pHelperFloater->getHandle();
        mHelperCommitConn = pHelperFloater->setCommitCallback(std::bind([&](const LLSD& sdValue) { onCommitEmoji(utf8str_to_wstring(sdValue.asStringRef())[0]); }, std::placeholders::_2));
        mHelperCloseConn = pHelperFloater->setCloseCallback([this](LLUICtrl* ctrl, const LLSD& param) { onCloseHelper(ctrl, param); });
    }
    setHostCtrl(hostctrl_p);
    mEmojiCommitCb = cb;

    S32 floater_x, floater_y;
    if (!hostctrl_p->localPointToOtherView(local_x, local_y, &floater_x, &floater_y, gFloaterView))
    {
        LL_ERRS() << "Cannot show emoji helper for non-floater controls." << LL_ENDL;
        return;
    }

    LLFloater* pHelperFloater = mHelperHandle.get();
    LLRect rect = pHelperFloater->getRect();
    S32 left = floater_x - HELPER_FLOATER_OFFSET_X;
    S32 top = floater_y - HELPER_FLOATER_OFFSET_Y + rect.getHeight();
    rect.setLeftTopAndSize(left, top, rect.getWidth(), rect.getHeight());
    pHelperFloater->setRect(rect);
    pHelperFloater->openFloater(LLSD().with("hint", short_code));
}

void LLEmojiHelper::hideHelper(const LLUICtrl* ctrl_p, bool strict)
{
    mIsHideDisabled &= !strict;
    if (mIsHideDisabled || (ctrl_p && !isActive(ctrl_p)))
    {
        return;
    }

    setHostCtrl(nullptr);
}

bool LLEmojiHelper::handleKey(const LLUICtrl* ctrl_p, KEY key, MASK mask)
{
    if (mHelperHandle.isDead() || !isActive(ctrl_p))
    {
        return false;
    }

    return mHelperHandle.get()->handleKey(key, mask, true);
}

void LLEmojiHelper::onCommitEmoji(llwchar emoji)
{
    if (!mHostHandle.isDead() && mEmojiCommitCb)
    {
        mEmojiCommitCb(emoji);
    }
}

void LLEmojiHelper::onCloseHelper(LLUICtrl* ctrl, const LLSD& param)
{
    mCloseSignal(ctrl, param);
}

boost::signals2::connection LLEmojiHelper::setCloseCallback(const commit_signal_t::slot_type& cb)
{
    return mCloseSignal.connect(cb);
}

void LLEmojiHelper::setHostCtrl(LLUICtrl* hostctrl_p)
{
    const LLUICtrl* pCurHostCtrl = mHostHandle.get();
    if (pCurHostCtrl != hostctrl_p)
    {
        mHostCtrlFocusLostConn.disconnect();
        mHostHandle.markDead();
        mEmojiCommitCb = {};

        if (!mHelperHandle.isDead())
        {
            mHelperHandle.get()->closeFloater();
        }

        if (hostctrl_p)
        {
            mHostHandle = hostctrl_p->getHandle();
            mHostCtrlFocusLostConn = hostctrl_p->setFocusLostCallback(std::bind([&]() { hideHelper(getHostCtrl()); }));
        }
    }
}
