/**
* @file llchatmentionhelper.cpp
*
* $LicenseInfo:firstyear=2025&license=viewerlgpl$
* Second Life Viewer Source Code
* Copyright (C) 2025, Linden Research, Inc.
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

#include "llchatmentionhelper.h"
#include "llfloater.h"
#include "llfloaterreg.h"
#include "lluictrl.h"

constexpr char CHAT_MENTION_HELPER_FLOATER[] = "chat_mention_picker";

bool LLChatMentionHelper::isActive(const LLUICtrl* ctrl) const
{
    return mHostHandle.get() == ctrl;
}

bool LLChatMentionHelper::isCursorInNameMention(const LLWString& wtext, S32 cursor_pos, S32* mention_start_pos) const
{
    if (cursor_pos <= 0 || cursor_pos > static_cast<S32>(wtext.size()))
        return false;

    // Find the beginning of the current word
    S32 start = cursor_pos - 1;
    while (start > 0 && wtext[start - 1] != U32(' ') && wtext[start - 1] != U32('\n'))
    {
        --start;
    }

    if (wtext[start] != U32('@'))
        return false;

    if (mention_start_pos)
        *mention_start_pos = start;

    S32 word_length = cursor_pos - start;

    if (word_length == 1)
    {
        return true;
    }

    // Get the name after '@'
    std::string name = wstring_to_utf8str(wtext.substr(start + 1, word_length - 1));
    LLStringUtil::toLower(name);
    for (const auto& av_name : mAvatarNames)
    {
        if (av_name == name || av_name.find(name) == 0)
        {
            return true;
        }
    }

    return false;
}

void LLChatMentionHelper::showHelper(LLUICtrl* host_ctrl, S32 local_x, S32 local_y, const std::string& av_name, std::function<void(std::string)> cb)
{
    if (mHelperHandle.isDead())
    {
        LLFloater* av_picker_floater = LLFloaterReg::getInstance(CHAT_MENTION_HELPER_FLOATER);
        mHelperHandle = av_picker_floater->getHandle();
        mHelperCommitConn = av_picker_floater->setCommitCallback([&](LLUICtrl* ctrl, const LLSD& param) { onCommitName(param.asString()); });
    }
    setHostCtrl(host_ctrl);
    mNameCommitCb = cb;

    S32 floater_x, floater_y;
    if (!host_ctrl->localPointToOtherView(local_x, local_y, &floater_x, &floater_y, gFloaterView))
    {
        LL_WARNS() << "Cannot show helper for non-floater controls." << LL_ENDL;
        return;
    }

    LLFloater* av_picker_floater = mHelperHandle.get();
    LLRect rect = av_picker_floater->getRect();
    rect.setLeftTopAndSize(floater_x, floater_y + rect.getHeight(), rect.getWidth(), rect.getHeight());
    av_picker_floater->setRect(rect);
    if (av_picker_floater->isShown())
    {
        av_picker_floater->onOpen(LLSD().with("av_name", av_name));
    }
    else
    {
        av_picker_floater->openFloater(LLSD().with("av_name", av_name));
    }
}

void LLChatMentionHelper::hideHelper(const LLUICtrl* ctrl)
{
    if ((ctrl && !isActive(ctrl)))
    {
        return;
    }
    setHostCtrl(nullptr);
}

bool LLChatMentionHelper::handleKey(const LLUICtrl* ctrl, KEY key, MASK mask)
{
    if (mHelperHandle.isDead() || !isActive(ctrl))
    {
        return false;
    }

    return mHelperHandle.get()->handleKey(key, mask, true);
}

void LLChatMentionHelper::onCommitName(std::string name_url)
{
    if (!mHostHandle.isDead() && mNameCommitCb)
    {
        mNameCommitCb(name_url);
    }
}

void LLChatMentionHelper::setHostCtrl(LLUICtrl* host_ctrl)
{
    const LLUICtrl* pCurHostCtrl = mHostHandle.get();
    if (pCurHostCtrl != host_ctrl)
    {
        mHostCtrlFocusLostConn.disconnect();
        mHostHandle.markDead();
        mNameCommitCb = {};

        if (!mHelperHandle.isDead())
        {
            mHelperHandle.get()->closeFloater();
        }

        if (host_ctrl)
        {
            mHostHandle = host_ctrl->getHandle();
            mHostCtrlFocusLostConn = host_ctrl->setFocusLostCallback(std::bind([&]() { hideHelper(getHostCtrl()); }));
        }
    }
}
