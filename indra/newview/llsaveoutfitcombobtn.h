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

#ifndef LL_LLSAVEOUTFITCOMBOBTN_H
#define LL_LLSAVEOUTFITCOMBOBTN_H

class LLButton;

#include "lltoggleablemenu.h"

/**
 * Represents outfit Save/Save As combo button.
 */
class LLSaveOutfitComboBtn
{
    LOG_CLASS(LLSaveOutfitComboBtn);
public:
    LLSaveOutfitComboBtn(LLPanel* parent, bool saveAsDefaultAction = false);

    void showSaveMenu();
    void saveOutfit(bool as_new = false);
    void setMenuItemEnabled(const std::string& item, bool enabled);
    void setSaveBtnEnabled(bool enabled);

private:
    bool mSaveAsDefaultAction;
    LLPanel* mParent;
    LLToggleableMenu* mSaveMenu;
};

#endif // LL_LLSAVEOUTFITCOMBOBTN_H
