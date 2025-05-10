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

#pragma once

#include "llhandle.h"
#include "llsingleton.h"

#include <boost/signals2.hpp>

class LLFloater;
class LLUICtrl;

class LLEmojiHelper : public LLSingleton<LLEmojiHelper>
{
    LLSINGLETON(LLEmojiHelper) {}
    ~LLEmojiHelper() override {}

public:
    // General
    std::string getToolTip(llwchar ch) const;
    bool        isActive(const LLUICtrl* ctrl_p) const;
    static bool isCursorInEmojiCode(const LLWString& wtext, S32 cursor_pos, S32* short_code_pos_p = nullptr);
    void        showHelper(LLUICtrl* hostctrl_p, S32 local_x, S32 local_y, const std::string& short_code, std::function<void(llwchar)> commit_cb);
    void        hideHelper(const LLUICtrl* ctrl_p = nullptr, bool strict = false);
    void        setIsHideDisabled(bool disabled) { mIsHideDisabled = disabled; };

    // Eventing
    bool handleKey(const LLUICtrl* ctrl_p, KEY key, MASK mask);
    void onCommitEmoji(llwchar emoji);
    void onCloseHelper(LLUICtrl* ctrl, const LLSD& param);

    typedef boost::signals2::signal<void(LLUICtrl* ctrl, const LLSD& param)> commit_signal_t;
    boost::signals2::connection setCloseCallback(const commit_signal_t::slot_type& cb);

protected:
    LLUICtrl* getHostCtrl() const { return mHostHandle.get(); }
    void      setHostCtrl(LLUICtrl* hostctrl_p);

private:
    commit_signal_t mCloseSignal;

    LLHandle<LLUICtrl>  mHostHandle;
    LLHandle<LLFloater> mHelperHandle;
    boost::signals2::connection mHostCtrlFocusLostConn;
    boost::signals2::connection mHelperCommitConn;
    boost::signals2::connection mHelperCloseConn;
    std::function<void(llwchar)> mEmojiCommitCb;
    bool mIsHideDisabled;
};
