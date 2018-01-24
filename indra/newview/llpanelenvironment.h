/** 
 * @file llpanelenvironment.h
 * @brief LLPanelExperiences class definition
 *
 * $LicenseInfo:firstyear=2013&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2013, Linden Research, Inc.
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

#ifndef LL_LLPANELENVIRONMENT_H
#define LL_LLPANELENVIRONMENT_H

#include "llaccordionctrltab.h"
#include "llradiogroup.h"
#include "llcheckboxctrl.h"
#include "llsliderctrl.h"
#include "llsettingsdaycycle.h"

class LLViewerRegion;

class LLPanelEnvironmentInfo : public LLPanel
{
public:
    LLPanelEnvironmentInfo();

    // LLPanel
    /*virtual*/ BOOL postBuild();
    /*virtual*/ void onOpen(const LLSD& key);

    // LLView
    /*virtual*/ void onVisibilityChange(BOOL new_visibility);


    virtual void refresh();

protected:
    LOG_CLASS(LLPanelEnvironmentInfo);

    void setControlsEnabled(bool enabled);
    void setApplyProgress(bool started);
    void setDirty(bool dirty);

    void onSwitchDefaultSelection();

    void onBtnApply();
    void onBtnCancel();
    void onBtnEdit();

    void onEditiCommited(LLSettingsDay::ptr_t newday);

    virtual void doApply() = 0;
    virtual void doEditCommited(LLSettingsDay::ptr_t &newday);

    /// New environment settings that are being applied to the region.
    //	LLEnvironmentSettings	mNewRegionSettings;

    bool			mEnableEditing;

    LLRadioGroup*	mRegionSettingsRadioGroup;
    LLSliderCtrl*   mDayLengthSlider;
    LLSliderCtrl*   mDayOffsetSlider;
    LLCheckBoxCtrl* mAllowOverRide;

    LLSettingsDay::ptr_t mEditingDayCycle;
};
#endif // LL_LLPANELEXPERIENCES_H
