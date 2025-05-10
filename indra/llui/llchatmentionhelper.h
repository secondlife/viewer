/**
* @file llchatmentionhelper.h
* @brief Header file for LLChatMentionHelper
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

#pragma once

#include "llhandle.h"
#include "llsingleton.h"

#include <boost/signals2.hpp>

class LLFloater;
class LLUICtrl;

class LLChatMentionHelper : public LLSingleton<LLChatMentionHelper>
{
    LLSINGLETON(LLChatMentionHelper) {}
    ~LLChatMentionHelper() override {}

public:

    bool isActive(const LLUICtrl* ctrl) const;
    bool isCursorInNameMention(const LLWString& wtext, S32 cursor_pos, S32* mention_start_pos = nullptr) const;
    void showHelper(LLUICtrl* host_ctrl, S32 local_x, S32 local_y, const std::string& av_name, std::function<void(std::string)> commit_cb);
    void hideHelper(const LLUICtrl* ctrl = nullptr);

    bool handleKey(const LLUICtrl* ctrl, KEY key, MASK mask);
    void onCommitName(std::string name_url);

    void updateAvatarList(std::vector<std::string> av_names) { mAvatarNames = av_names; }

protected:
    void setHostCtrl(LLUICtrl* host_ctrl);
    LLUICtrl* getHostCtrl() const { return mHostHandle.get(); }

private:
    LLHandle<LLUICtrl>  mHostHandle;
    LLHandle<LLFloater> mHelperHandle;
    boost::signals2::connection mHostCtrlFocusLostConn;
    boost::signals2::connection mHelperCommitConn;
    std::function<void(std::string)> mNameCommitCb;

    std::vector<std::string> mAvatarNames;
};
