/** 
 * @file llsaveoutfitcombobtn.h
 * @brief Represents outfit save/save as combo button.
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

#ifndef LL_LLFLYOUTCOMBOBTN_H
#define LL_LLFLYOUTCOMBOBTN_H

/*TODO: Make this button generic */

class LLButton;

#include "lltoggleablemenu.h"

class LLFlyoutComboBtnCtrl
{
    LOG_CLASS(LLFlyoutComboBtnCtrl);
public:
    LLFlyoutComboBtnCtrl(LLPanel* parent,
                         const std::string &action_button,
                         const std::string &flyout_button,
                         const std::string &menu_file,
                         bool apply_immediately = true);

	void setMenuItemEnabled(const std::string &item, bool enabled);
	void setShownBtnEnabled(bool enabled);
    void setMenuItemVisible(const std::string &item, bool visible);
    void setMenuItemLabel(const std::string &item, const std::string &label);

    U32 getItemCount();
    void setSelectedItem(S32 itemno);
    void setSelectedItem(const std::string &item);

    void setAction(LLUICtrl::commit_callback_t cb);

protected:
    void onFlyoutButton(LLUICtrl *, const LLSD &);
    void onFlyoutItemSelected(LLUICtrl *, const LLSD &);
    bool onFlyoutItemCheck(LLUICtrl *, const LLSD &);
    void onFlyoutAction(LLUICtrl *, const LLSD &);

    void setSelectedItem(LLMenuItemGL *pitem);

private:
	LLPanel *                   mParent;
	LLToggleableMenu *          mFlyoutMenu;
    std::string                 mActionButton;
    std::string                 mFlyoutButton;

    std::string                 mSelectedName;
    bool                        mApplyImmediately;

    LLUICtrl::commit_signal_t   mActionSignal;
};
#endif // LL_LLFLYOUTCOMBOBTN_H
