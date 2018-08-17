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
#include "llenvironment.h"
#include "llparcel.h"
#include "llsettingspicker.h"
#include "llfloatereditextdaycycle.h"

class LLViewerRegion;

class LLPanelEnvironmentInfo : public LLPanel
{
public:
                                LLPanelEnvironmentInfo();

    // LLPanel
    virtual BOOL                postBuild() override;
    virtual void                onOpen(const LLSD& key) override;

    // LLView
    virtual void                onVisibilityChange(BOOL new_visibility) override;

    virtual void                refresh() override;

    S32                         getCurrentParcelId() const { return mCurrentParcelId; }
    void                        setCurrentParcelId(S32 parcel_id) { mCurrentParcelId = parcel_id; }

    virtual bool                isRegion() const = 0;
    virtual LLParcel *          getParcel() = 0;
    virtual bool                canEdit() = 0;

protected:
    LOG_CLASS(LLPanelEnvironmentInfo);

    static const std::string    RDG_ENVIRONMENT_SELECT;
    static const std::string    RDO_USEDEFAULT;
    static const std::string    RDO_USEINV;
    static const std::string    RDO_USECUSTOM;
    static const std::string    EDT_INVNAME;
    static const std::string    BTN_SELECTINV;
    static const std::string    BTN_EDIT;
    static const std::string    SLD_DAYLENGTH;
    static const std::string    SLD_DAYOFFSET;
    static const std::string    CHK_ALLOWOVERRIDE;
    static const std::string    BTN_APPLY;
    static const std::string    BTN_CANCEL;

    static const std::string    STR_LABEL_USEDEFAULT;
    static const std::string    STR_LABEL_USEREGION;
    static const std::string    STR_LABEL_UNKNOWNINV;

    static const U32            DIRTY_FLAG_DAYCYCLE;
    static const U32            DIRTY_FLAG_DAYLENGTH;
    static const U32            DIRTY_FLAG_DAYOFFSET;

    static const U32            DIRTY_FLAG_MASK;

    void                        setControlsEnabled(bool enabled);
    void                        setApplyProgress(bool started);
    void                        setDirtyFlag(U32 flag);
    void                        clearDirtyFlag(U32 flag);
    bool                        getIsDirty() const              { return (mDirtyFlag != 0); }
    bool                        getIsDirtyFlag(U32 flag) const  { return ((mDirtyFlag & flag) != 0); }
    U32                         getDirtyFlag() const            { return mDirtyFlag; }

    void                        onSwitchDefaultSelection();
    void                        onSldDayLengthChanged(F32 value);
    void                        onSldDayOffsetChanged(F32 value);
    void                        onBtnApply();
    void                        onBtnReset();
    void                        onBtnEdit();
    void                        onBtnSelect();

    virtual void                doApply();

    void                        onPickerCommited(LLUUID asset_id);
    void                        onEditCommited(LLSettingsDay::ptr_t newday);
    void                        onPickerAssetDownloaded(LLSettingsBase::ptr_t settings);
    void                        onEnvironmentReceived(S32 parcel_id, LLEnvironment::EnvironmentInfo::ptr_t envifo);

    void                        refreshFromSource();

    std::string                 getInventoryNameForAssetId(LLUUID asset_id);

    LLFloaterSettingsPicker *   getSettingsPicker();
    LLFloaterEditExtDayCycle *  getEditFloater();

    LLEnvironment::EnvironmentInfo::ptr_t   mCurrentEnvironment;
    S32                                     mCurrentParcelId;


private:
    LLHandle<LLFloater>                     mSettingsFloater;
    LLHandle<LLFloater>                     mEditFloater;
    S32                                     mDirtyFlag;
};
#endif // LL_LLPANELEXPERIENCES_H
