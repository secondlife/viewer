/**
 * @file lltoastalertpanel.h
 * @brief Panel for alert toasts.
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#ifndef LL_TOASTALERTPANEL_H
#define LL_TOASTALERTPANEL_H

#include "lltoastpanel.h"
#include "llfloater.h"
#include "llui.h"
#include "llnotificationptr.h"
#include "llerror.h"

class LLButton;
class LLCheckBoxCtrl;
class LLLineEditor;

/**
 * Toast panel for alert notification.
 * Alerts notifications doesn't require user interaction.
 *
 * Replaces class LLAlertDialog.
 * https://wiki.lindenlab.com/mediawiki/index.php?title=LLAlertDialog&oldid=81388
 */

class LLToastAlertPanel
    : public LLCheckBoxToastPanel
{
    LOG_CLASS(LLToastAlertPanel);
public:
    typedef bool (*display_callback_t)(S32 modal);

public:
    // User's responsibility to call show() after creating these.
    LLToastAlertPanel( LLNotificationPtr notep, bool is_modal );

    virtual bool    handleKeyHere(KEY key, MASK mask );

    virtual void    draw();
    virtual void    setVisible( bool visible );

    void            setCaution(bool val = true) { mCaution = val; }
    // If mUnique==true only one copy of this message should exist
    void            setUnique(bool val = true) { mUnique = val; }
    void            setEditTextArgs(const LLSD& edit_args);

    void onButtonPressed(const LLSD& data, S32 button);

private:
    static std::map<std::string, LLToastAlertPanel*> sUniqueActiveMap;

    virtual ~LLToastAlertPanel();
    // No you can't kill it.  It can only kill itself.

    // Does it have a readable title label, or minimize or close buttons?
    bool hasTitleBar() const;

private:
    static LLControlGroup* sSettings;

    struct ButtonData
    {
        LLButton* mButton = nullptr;
        std::string mURL;
        U32 mURLExternal = 0;
        S32 mWidth = 0;
    };
    std::vector<ButtonData> mButtonData;

    S32             mDefaultOption;
    bool            mCaution;
    bool            mUnique;
    LLUIString      mLabel;
    LLFrameTimer    mDefaultBtnTimer;
    // For Dialogs that take a line as text as input:
    LLLineEditor* mLineEditor;
    LLHandle<LLView>    mPreviouslyFocusedView;

};

#endif  // LL_TOASTALERTPANEL_H
