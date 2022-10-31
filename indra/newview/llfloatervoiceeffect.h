/** 
 * @file llfloatervoiceeffect.h
 * @author Aimee
 * @brief Selection and preview of voice effects.
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

#ifndef LL_LLFLOATERVOICEEFFECT_H
#define LL_LLFLOATERVOICEEFFECT_H

#include "llfloater.h"
#include "llvoiceclient.h"

class LLButton;
class LLScrollListCtrl;

class LLFloaterVoiceEffect
    : public LLFloater
    , public LLVoiceEffectObserver
{
public:
    LOG_CLASS(LLFloaterVoiceEffect);

    LLFloaterVoiceEffect(const LLSD& key);
    virtual ~LLFloaterVoiceEffect();

    virtual BOOL postBuild();
    virtual void onClose(bool app_quitting);

private:
    enum ColumnIndex
    {
        NAME_COLUMN = 0,
        DATE_COLUMN = 1,
    };

    void refreshEffectList();
    void updateControls();

    /// Called by voice effect provider when voice effect list is changed.
    virtual void onVoiceEffectChanged(bool effect_list_updated);

    void onClickRecord();
    void onClickPlay();
    void onClickStop();
//  void onClickActivate();

    LLUUID mSelectedID;
    LLScrollListCtrl* mVoiceEffectList;
};

#endif
