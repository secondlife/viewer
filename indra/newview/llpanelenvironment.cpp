/** 
 * @file llpanelenvironment.cpp
 * @brief LLPanelExperiences class implementation
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

#include "llviewerprecompiledheaders.h"


#include "llpanelprofile.h"
#include "lluictrlfactory.h"
#include "llexperiencecache.h"
#include "llagent.h"
#include "llparcel.h"

#include "llviewerregion.h"
#include "llpanelenvironment.h"
#include "llslurl.h"
#include "lllayoutstack.h"

#include "llfloater.h"
#include "llfloaterreg.h"
#include "llfloatereditextdaycycle.h"
#include "llsettingsvo.h"

#include "llappviewer.h"

//=========================================================================
namespace 
{
    const std::string FLOATER_DAY_CYCLE_EDIT("env_edit_extdaycycle");
}

//=========================================================================
const std::string LLPanelEnvironmentInfo::RDG_ENVIRONMENT_SELECT("rdg_environment_select");
const std::string LLPanelEnvironmentInfo::RDO_USEDEFAULT("rdo_use_xxx_setting");
const std::string LLPanelEnvironmentInfo::RDO_USEINV("rdo_use_inv_setting");
const std::string LLPanelEnvironmentInfo::RDO_USECUSTOM("rdo_use_custom_setting");
const std::string LLPanelEnvironmentInfo::EDT_INVNAME("edt_inventory_name");
const std::string LLPanelEnvironmentInfo::BTN_SELECTINV("btn_select_inventory");
const std::string LLPanelEnvironmentInfo::BTN_EDIT("btn_edit");
const std::string LLPanelEnvironmentInfo::SLD_DAYLENGTH("sld_day_length");
const std::string LLPanelEnvironmentInfo::SLD_DAYOFFSET("sld_day_offset");
const std::string LLPanelEnvironmentInfo::CHK_ALLOWOVERRIDE("chk_allow_override");
const std::string LLPanelEnvironmentInfo::BTN_APPLY("btn_apply");
const std::string LLPanelEnvironmentInfo::BTN_CANCEL("btn_cancel");

const std::string LLPanelEnvironmentInfo::STR_LABEL_USEDEFAULT("str_label_use_default");
const std::string LLPanelEnvironmentInfo::STR_LABEL_USEREGION("str_label_use_region");
const std::string LLPanelEnvironmentInfo::STR_LABEL_UNKNOWNINV("str_unknow_inventory");

const S32 LLPanelEnvironmentInfo::DIRTY_FLAG_DAYCYCLE(0x01 << 0);
const S32 LLPanelEnvironmentInfo::DIRTY_FLAG_DAYLENGTH(0x01 << 1);
const S32 LLPanelEnvironmentInfo::DIRTY_FLAG_DAYOFFSET(0x01 << 2);

const S32 LLPanelEnvironmentInfo::DIRTY_FLAG_MASK(
        LLPanelEnvironmentInfo::DIRTY_FLAG_DAYCYCLE | 
        LLPanelEnvironmentInfo::DIRTY_FLAG_DAYLENGTH | 
        LLPanelEnvironmentInfo::DIRTY_FLAG_DAYOFFSET );

//=========================================================================
LLPanelEnvironmentInfo::LLPanelEnvironmentInfo(): 
    mCurrentEnvironment(),
    mCurrentParcelId(INVALID_PARCEL_ID),
    mDirtyFlag(0),
    mSettingsFloater(),
    mEditFloater()
{
}

BOOL LLPanelEnvironmentInfo::postBuild()
{
    getChild<LLUICtrl>(RDG_ENVIRONMENT_SELECT)->setCommitCallback([this](LLUICtrl *, const LLSD &){ onSwitchDefaultSelection(); });
    getChild<LLUICtrl>(BTN_SELECTINV)->setCommitCallback([this](LLUICtrl *, const LLSD &){ onBtnSelect(); });
    getChild<LLUICtrl>(BTN_EDIT)->setCommitCallback([this](LLUICtrl *, const LLSD &){ onBtnEdit(); });
    getChild<LLUICtrl>(BTN_APPLY)->setCommitCallback([this](LLUICtrl *, const LLSD &){ onBtnApply(); });
    getChild<LLUICtrl>(BTN_CANCEL)->setCommitCallback([this](LLUICtrl *, const LLSD &){ onBtnReset(); });

    getChild<LLUICtrl>(SLD_DAYLENGTH)->setCommitCallback([this](LLUICtrl *, const LLSD &value) { onSldDayLengthChanged(value.asReal()); });
    getChild<LLUICtrl>(SLD_DAYOFFSET)->setCommitCallback([this](LLUICtrl *, const LLSD &value) { onSldDayOffsetChanged(value.asReal()); });

    return TRUE;
}

// virtual
void LLPanelEnvironmentInfo::onOpen(const LLSD& key)
{
    refreshFromSource();
}

// virtual
void LLPanelEnvironmentInfo::onVisibilityChange(BOOL new_visibility)
{
    // If hiding (user switched to another tab or closed the floater),
    // display user's preferred environment.
    // switching back and forth between agent's environment and the one being edited. 
    // 
}

void LLPanelEnvironmentInfo::refresh()
{
    if (gDisconnected)
        return;

    if (((mCurrentEnvironment->mParcelId != INVALID_PARCEL_ID) && (!mCurrentEnvironment->mDayCycle)) ||
        ((mCurrentEnvironment->mParcelId == INVALID_PARCEL_ID) && (mCurrentEnvironment->mDayCycle->getAssetId() == LLSettingsDay::GetDefaultAssetId() )))
    {
        getChild<LLRadioGroup>(RDG_ENVIRONMENT_SELECT)->setSelectedIndex(0);
        getChild<LLUICtrl>(EDT_INVNAME)->setValue("");
    }
    else if (!mCurrentEnvironment->mDayCycle->getAssetId().isNull())
    {
        getChild<LLRadioGroup>(RDG_ENVIRONMENT_SELECT)->setSelectedIndex(1);

        LLUUID asset_id = mCurrentEnvironment->mDayCycle->getAssetId();

        std::string inventoryname = getInventoryNameForAssetId(asset_id);

        if (inventoryname.empty())
            inventoryname = "(" + mCurrentEnvironment->mDayCycle->getName() + ")";

        getChild<LLUICtrl>(EDT_INVNAME)->setValue(inventoryname);
    }
    else
    {   // asset id is null so this is a custom environment
        getChild<LLRadioGroup>(RDG_ENVIRONMENT_SELECT)->setSelectedIndex(2);
        getChild<LLUICtrl>(EDT_INVNAME)->setValue("");
    }

    F32Hours daylength(mCurrentEnvironment->mDayLength);
    F32Hours dayoffset(mCurrentEnvironment->mDayOffset);

    if (dayoffset.value() > 8.0f)
        dayoffset -= F32Hours(24.0);

    getChild<LLSliderCtrl>(SLD_DAYLENGTH)->setValue(daylength.value());
    getChild<LLSliderCtrl>(SLD_DAYOFFSET)->setValue(dayoffset.value());
   
    setControlsEnabled(canEdit());
}

void LLPanelEnvironmentInfo::refreshFromSource()
{
    LLEnvironment::instance().requestParcel(mCurrentParcelId, 
        [this](S32 parcel_id, LLEnvironment::EnvironmentInfo::ptr_t envifo) {handleEnvironmentReceived(parcel_id, envifo); });
}

std::string LLPanelEnvironmentInfo::getInventoryNameForAssetId(LLUUID asset_id) 
{
    LLFloaterSettingsPicker *picker = getSettingsPicker();

    if (!picker)
    {   
        LL_WARNS("ENVPANEL") << "Couldn't instantiate picker." << LL_ENDL;
        return std::string();
    }

    std::string name(picker->findItemName(asset_id, false, false));

    if (name.empty())
        return getString(STR_LABEL_UNKNOWNINV);
    return name;
}

LLFloaterSettingsPicker *LLPanelEnvironmentInfo::getSettingsPicker()
{
    LLFloaterSettingsPicker *picker = static_cast<LLFloaterSettingsPicker *>(mSettingsFloater.get());

    // Show the dialog
    if (!picker)
    {
        picker = new LLFloaterSettingsPicker(this,
            LLUUID::null, "SELECT SETTINGS");

        mSettingsFloater = picker->getHandle();

        picker->setCommitCallback([this](LLUICtrl *, const LLSD &data){ onPickerCommited(data.asUUID()); });
    }

    return picker;
}

LLFloaterEditExtDayCycle * LLPanelEnvironmentInfo::getEditFloater()
{
    LLFloaterEditExtDayCycle *editor = static_cast<LLFloaterEditExtDayCycle *>(mEditFloater.get());

    // Show the dialog
    if (!editor)
    {
        LLSD params(LLSDMap(LLFloaterEditExtDayCycle::KEY_EDIT_CONTEXT, "parcel")
            (LLFloaterEditExtDayCycle::KEY_DAY_LENGTH, static_cast<S32>(mCurrentEnvironment->mDayLength.value())));

        editor = (LLFloaterEditExtDayCycle *)LLFloaterReg::getInstance(FLOATER_DAY_CYCLE_EDIT, params);

        if (!editor)
            return nullptr;
    }

    return editor;
}

void LLPanelEnvironmentInfo::setControlsEnabled(bool enabled)
{
    S32 rdo_selection = getChild<LLRadioGroup>(RDG_ENVIRONMENT_SELECT)->getSelectedIndex();

    getChild<LLUICtrl>(RDG_ENVIRONMENT_SELECT)->setEnabled(enabled);
    getChild<LLUICtrl>(RDO_USEDEFAULT)->setEnabled(enabled);
    getChild<LLUICtrl>(RDO_USEINV)->setEnabled(enabled);
    getChild<LLUICtrl>(RDO_USECUSTOM)->setEnabled(enabled);
    getChild<LLUICtrl>(EDT_INVNAME)->setEnabled(FALSE);
    getChild<LLUICtrl>(BTN_SELECTINV)->setEnabled(enabled);
    getChild<LLUICtrl>(BTN_EDIT)->setEnabled(enabled);
    getChild<LLUICtrl>(SLD_DAYLENGTH)->setEnabled(enabled && (rdo_selection != 0));
    getChild<LLUICtrl>(SLD_DAYOFFSET)->setEnabled(enabled && (rdo_selection != 0));
    getChild<LLUICtrl>(CHK_ALLOWOVERRIDE)->setEnabled(enabled && (mCurrentParcelId == INVALID_PARCEL_ID));
    getChild<LLUICtrl>(BTN_APPLY)->setEnabled(enabled && (mDirtyFlag != 0));
    getChild<LLUICtrl>(BTN_CANCEL)->setEnabled(enabled && (mDirtyFlag != 0));
}

void LLPanelEnvironmentInfo::setApplyProgress(bool started)
{
//     LLLoadingIndicator* indicator = getChild<LLLoadingIndicator>("progress_indicator");
// 
//     indicator->setVisible(started);
// 
//     if (started)
//     {
//         indicator->start();
//     }
//     else
//     {
//         indicator->stop();
//     }
}

void LLPanelEnvironmentInfo::setDirtyFlag(S32 flag)
{
    bool can_edit = canEdit();
    mDirtyFlag |= flag;
    getChildView(BTN_APPLY)->setEnabled((mDirtyFlag != 0) && can_edit);
    getChildView(BTN_CANCEL)->setEnabled((mDirtyFlag != 0) && can_edit);
}

void LLPanelEnvironmentInfo::clearDirtyFlag(S32 flag)
{
    bool can_edit = canEdit();
    mDirtyFlag &= ~flag;
    getChildView(BTN_APPLY)->setEnabled((mDirtyFlag != 0) && can_edit);
    getChildView(BTN_CANCEL)->setEnabled((mDirtyFlag != 0) && can_edit);
}

void LLPanelEnvironmentInfo::onSwitchDefaultSelection()
{
    bool can_edit = canEdit();
    setDirtyFlag(DIRTY_FLAG_DAYCYCLE);

    S32 rdo_selection = getChild<LLRadioGroup>(RDG_ENVIRONMENT_SELECT)->getSelectedIndex();
    getChild<LLUICtrl>(SLD_DAYLENGTH)->setEnabled(can_edit && (rdo_selection != 0));
    getChild<LLUICtrl>(SLD_DAYOFFSET)->setEnabled(can_edit && (rdo_selection != 0));
}

void LLPanelEnvironmentInfo::onSldDayLengthChanged(F32 value)
{
    F32Hours daylength(value);

    mCurrentEnvironment->mDayLength = daylength;
    setDirtyFlag(DIRTY_FLAG_DAYLENGTH);
}

void LLPanelEnvironmentInfo::onSldDayOffsetChanged(F32 value)
{
    F32Hours dayoffset(value);

    if (dayoffset.value() < 0.0f)
        dayoffset += F32Hours(24.0);

    mCurrentEnvironment->mDayLength = dayoffset;
    setDirtyFlag(DIRTY_FLAG_DAYOFFSET);
}

void LLPanelEnvironmentInfo::onBtnApply()
{
    doApply();
}

void LLPanelEnvironmentInfo::onBtnReset()
{
    mCurrentEnvironment.reset();
    refreshFromSource();
}

void LLPanelEnvironmentInfo::onBtnEdit()
{
    LLFloaterEditExtDayCycle *dayeditor = getEditFloater();

    dayeditor->openFloater(dayeditor->getKey());
}

void LLPanelEnvironmentInfo::onBtnSelect()
{
    LLFloaterSettingsPicker *picker = getSettingsPicker();
    if (picker)
    {
        picker->setSettingsFilter(LLSettingsType::ST_NONE);
        picker->setSettingsAssetId((mCurrentEnvironment->mDayCycle) ? mCurrentEnvironment->mDayCycle->getAssetId() : LLUUID::null);
        picker->openFloater();
        picker->setFocus(TRUE);
    }
}


void LLPanelEnvironmentInfo::doApply()
{
    if (getIsDirtyFlag(DIRTY_FLAG_MASK))
    {
        S32 rdo_selection = getChild<LLRadioGroup>(RDG_ENVIRONMENT_SELECT)->getSelectedIndex();

        if (rdo_selection == 0)
        {
            LLEnvironment::instance().resetParcel(mCurrentParcelId,
                [this](S32 parcel_id, LLEnvironment::EnvironmentInfo::ptr_t envifo) {handleEnvironmentReceived(parcel_id, envifo); });
        }
        else if (rdo_selection == 1)
        {
            LLEnvironment::instance().updateParcel(mCurrentParcelId, 
                mCurrentEnvironment->mDayCycle->getAssetId(), mCurrentEnvironment->mDayLength.value(), mCurrentEnvironment->mDayOffset.value(),
                [this](S32 parcel_id, LLEnvironment::EnvironmentInfo::ptr_t envifo) {handleEnvironmentReceived(parcel_id, envifo); });
        }
        else
        {
            LLEnvironment::instance().updateParcel(mCurrentParcelId, 
                mCurrentEnvironment->mDayCycle, mCurrentEnvironment->mDayLength.value(), mCurrentEnvironment->mDayOffset.value(), 
                [this](S32 parcel_id, LLEnvironment::EnvironmentInfo::ptr_t envifo) {handleEnvironmentReceived(parcel_id, envifo); });
        }

    }
    setControlsEnabled(false);
}

void LLPanelEnvironmentInfo::onPickerCommited(LLUUID asset_id)
{
    LLSettingsVOBase::getSettingsAsset(asset_id, [this](LLUUID, LLSettingsBase::ptr_t settings, S32 status, LLExtStat) { 
        if (status)
            return;
        onPickerAssetDownloaded(settings);
    });
}

void LLPanelEnvironmentInfo::onEditiCommited(LLSettingsDay::ptr_t newday)
{
    doEditCommited(newday);
}

void LLPanelEnvironmentInfo::onPickerAssetDownloaded(LLSettingsBase::ptr_t settings)
{
    LLSettingsVODay::buildFromOtherSetting(settings, [this](LLSettingsDay::ptr_t pday)
        {
            if (pday)
            {
                mCurrentEnvironment->mDayCycle = pday;
                setDirtyFlag(DIRTY_FLAG_DAYCYCLE);
            }
            refresh();
        });
}

void LLPanelEnvironmentInfo::handleEnvironmentReceived(S32 parcel_id, LLEnvironment::EnvironmentInfo::ptr_t envifo)
{  
    if (parcel_id != mCurrentParcelId)
    {
        LL_WARNS("ENVPANEL") << "Have environment for parcel " << parcel_id << " expecting " << mCurrentParcelId << ". Discarding." << LL_ENDL;
        return;
    }
    mCurrentEnvironment = envifo;
    clearDirtyFlag(DIRTY_FLAG_MASK);
    refresh();
}

void LLPanelEnvironmentInfo::doEditCommited(LLSettingsDay::ptr_t &newday)
{
//     mEditingDayCycle = newday;
    /*TODO pure virtual*/
}
