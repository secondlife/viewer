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
#include "llmultisliderctrl.h"
#include "llsettingsvo.h"

#include "llappviewer.h"
#include "llcallbacklist.h"
#include "llviewerparcelmgr.h"

#include "llinventorymodel.h"

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
const std::string LLPanelEnvironmentInfo::SLD_ALTITUDES("sld_altitudes");
const std::string LLPanelEnvironmentInfo::ICN_GROUND("icon_ground");
const std::string LLPanelEnvironmentInfo::CHK_ALLOWOVERRIDE("chk_allow_override");
const std::string LLPanelEnvironmentInfo::BTN_APPLY("btn_apply");
const std::string LLPanelEnvironmentInfo::BTN_CANCEL("btn_cancel");
const std::string LLPanelEnvironmentInfo::LBL_TIMEOFDAY("lbl_apparent_time");
const std::string LLPanelEnvironmentInfo::PNL_SETTINGS("pnl_environment_config");
const std::string LLPanelEnvironmentInfo::PNL_ENVIRONMENT_ALTITUDES("pnl_environment_altitudes");
const std::string LLPanelEnvironmentInfo::PNL_BUTTONS("pnl_environment_buttons");
const std::string LLPanelEnvironmentInfo::PNL_DISABLED("pnl_environment_disabled");
const std::string LLPanelEnvironmentInfo::TXT_DISABLED("txt_environment_disabled");
const std::string LLPanelEnvironmentInfo::SDT_DROP_TARGET("sdt_drop_target");

const std::string LLPanelEnvironmentInfo::STR_LABEL_USEDEFAULT("str_label_use_default");
const std::string LLPanelEnvironmentInfo::STR_LABEL_USEREGION("str_label_use_region");
const std::string LLPanelEnvironmentInfo::STR_LABEL_UNKNOWNINV("str_unknow_inventory");
const std::string LLPanelEnvironmentInfo::STR_ALTITUDE_DESCRIPTION("str_altitude_desription");
const std::string LLPanelEnvironmentInfo::STR_NO_PARCEL("str_no_parcel");
const std::string LLPanelEnvironmentInfo::STR_CROSS_REGION("str_cross_region");
const std::string LLPanelEnvironmentInfo::STR_LEGACY("str_legacy");

const U32 LLPanelEnvironmentInfo::DIRTY_FLAG_DAYCYCLE(0x01 << 0);
const U32 LLPanelEnvironmentInfo::DIRTY_FLAG_DAYLENGTH(0x01 << 1);
const U32 LLPanelEnvironmentInfo::DIRTY_FLAG_DAYOFFSET(0x01 << 2);
const U32 LLPanelEnvironmentInfo::DIRTY_FLAG_ALTITUDES(0x01 << 3);

const U32 LLPanelEnvironmentInfo::DIRTY_FLAG_MASK(
        LLPanelEnvironmentInfo::DIRTY_FLAG_DAYCYCLE | 
        LLPanelEnvironmentInfo::DIRTY_FLAG_DAYLENGTH | 
        LLPanelEnvironmentInfo::DIRTY_FLAG_DAYOFFSET |
        LLPanelEnvironmentInfo::DIRTY_FLAG_ALTITUDES);

const U32 ALTITUDE_SLIDER_COUNT = 3;

const std::string alt_sliders[] = {
    "sld1",
    "sld2",
    "sld3",
};

const std::string alt_labels[] = {
    "alt1",
    "alt2",
    "alt3",
    "ground",
};


static LLDefaultChildRegistry::Register<LLSettingsDropTarget> r("settings_drop_target");

//=========================================================================
LLPanelEnvironmentInfo::LLPanelEnvironmentInfo(): 
    mCurrentEnvironment(),
    mDirtyFlag(0),
    mEditorLastParcelId(INVALID_PARCEL_ID),
    mCrossRegion(false),
    mNoSelection(false),
    mNoEnvironment(false),
    mCurEnvVersion(INVALID_PARCEL_ENVIRONMENT_VERSION),
    mSettingsFloater(),
    mEditFloater()
{
}

LLPanelEnvironmentInfo::~LLPanelEnvironmentInfo()
{
    if (mChangeMonitor.connected())
        mChangeMonitor.disconnect();
    if (mCommitConnection.connected())
        mCommitConnection.disconnect();
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

    getChild<LLMultiSliderCtrl>(SLD_ALTITUDES)->setCommitCallback([this](LLUICtrl *cntrl, const LLSD &value) { onAltSliderCallback(cntrl, value); });

    mChangeMonitor = LLEnvironment::instance().setEnvironmentChanged([this](LLEnvironment::EnvSelection_t env) { onEnvironmentChanged(env); });

    getChild<LLSettingsDropTarget>(SDT_DROP_TARGET)->setPanel(this);

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
    if (new_visibility)
    {
        gIdleCallbacks.addFunction(onIdlePlay, this);
    }
    else
    {
        LLFloaterSettingsPicker *picker = getSettingsPicker(false);
        if (picker)
        {
            picker->closeFloater();
        }

        gIdleCallbacks.deleteFunction(onIdlePlay, this);
        LLFloaterEditExtDayCycle *dayeditor = getEditFloater(false);
        if (mCommitConnection.connected())
            mCommitConnection.disconnect();

        if (dayeditor)
        {
            if (dayeditor->isDirty())
                dayeditor->refresh();
            else
            {
                dayeditor->closeFloater();
                mEditFloater.markDead();
            }
        }
    }

}

void LLPanelEnvironmentInfo::refresh()
{
    if (gDisconnected)
        return;

    if (!setControlsEnabled(canEdit()))
        return;

    if (!mCurrentEnvironment)
    {
        return;
    }

    S32 rdo_selection = 0;
    if ((!mCurrentEnvironment->mDayCycle) ||
        ((mCurrentEnvironment->mParcelId == INVALID_PARCEL_ID) && (mCurrentEnvironment->mDayCycle->getAssetId() == LLSettingsDay::GetDefaultAssetId() )))
    {
        getChild<LLUICtrl>(EDT_INVNAME)->setValue("");
    }
    else if (!mCurrentEnvironment->mDayCycle->getAssetId().isNull())
    {
        rdo_selection = 1;

        LLUUID asset_id = mCurrentEnvironment->mDayCycle->getAssetId();

        std::string inventoryname = getInventoryNameForAssetId(asset_id);

        if (inventoryname.empty())
            inventoryname = "(" + mCurrentEnvironment->mDayCycle->getName() + ")";

        getChild<LLUICtrl>(EDT_INVNAME)->setValue(inventoryname);
    }
    else
    {   // asset id is null so this is a custom environment
        rdo_selection = 2;
        getChild<LLUICtrl>(EDT_INVNAME)->setValue("");
    }
    getChild<LLRadioGroup>(RDG_ENVIRONMENT_SELECT)->setSelectedIndex(rdo_selection);

    F32Hours daylength(mCurrentEnvironment->mDayLength);
    F32Hours dayoffset(mCurrentEnvironment->mDayOffset);

    if (dayoffset.value() > 12.0f)
        dayoffset -= F32Hours(24.0);

    getChild<LLSliderCtrl>(SLD_DAYLENGTH)->setValue(daylength.value());
    getChild<LLSliderCtrl>(SLD_DAYOFFSET)->setValue(dayoffset.value());
    getChild<LLSliderCtrl>(SLD_DAYLENGTH)->setEnabled(canEdit() && (rdo_selection != 0) && !mCurrentEnvironment->mIsLegacy);
    getChild<LLSliderCtrl>(SLD_DAYOFFSET)->setEnabled(canEdit() && (rdo_selection != 0) && !mCurrentEnvironment->mIsLegacy);
   
    udpateApparentTimeOfDay();

    updateEditFloater(mCurrentEnvironment, canEdit());

    LLEnvironment::altitude_list_t altitudes = LLEnvironment::instance().getRegionAltitudes();
    if (altitudes.size() > 0)
    {
        for (S32 idx = 0; idx < ALTITUDE_SLIDER_COUNT; ++idx)
        {
            LLMultiSliderCtrl *sld = getChild<LLMultiSliderCtrl>(SLD_ALTITUDES);
            sld->setSliderValue(alt_sliders[idx], altitudes[idx+1], FALSE);
            updateAltLabel(alt_labels[idx], idx + 2, altitudes[idx+1]);
            mAltitudes[alt_sliders[idx]] = AltitudeData(idx+1, idx, altitudes[idx+1]);
        }
        readjustAltLabels();
    }

}

std::string LLPanelEnvironmentInfo::getInventoryNameForAssetId(LLUUID asset_id) 
{
    std::string name(LLFloaterSettingsPicker::findItemName(asset_id, false, false));

    if (name.empty())
        return getString(STR_LABEL_UNKNOWNINV);
    return name;
}

LLFloaterSettingsPicker * LLPanelEnvironmentInfo::getSettingsPicker(bool create)
{
    LLFloaterSettingsPicker *picker = static_cast<LLFloaterSettingsPicker *>(mSettingsFloater.get());

    // Show the dialog
    if (!picker && create)
    {
        picker = new LLFloaterSettingsPicker(this,
            LLUUID::null, "SELECT SETTINGS");

        mSettingsFloater = picker->getHandle();

        picker->setCommitCallback([this](LLUICtrl *, const LLSD &data){ onPickerCommitted(data.asUUID()); });
    }

    return picker;
}

LLFloaterEditExtDayCycle * LLPanelEnvironmentInfo::getEditFloater(bool create)
{
    static const S32 FOURHOURS(4 * 60 * 60);
    LLFloaterEditExtDayCycle *editor = static_cast<LLFloaterEditExtDayCycle *>(mEditFloater.get());

    // Show the dialog
    if (!editor && create)
    {
        LLSD params(LLSDMap(LLFloaterEditExtDayCycle::KEY_EDIT_CONTEXT, isRegion() ? LLFloaterEditExtDayCycle::CONTEXT_REGION : LLFloaterEditExtDayCycle::CONTEXT_PARCEL)
            (LLFloaterEditExtDayCycle::KEY_DAY_LENGTH, mCurrentEnvironment ? (S32)(mCurrentEnvironment->mDayLength.value()) : FOURHOURS));

        editor = (LLFloaterEditExtDayCycle *)LLFloaterReg::getInstance(FLOATER_DAY_CYCLE_EDIT, params);

        if (!editor)
            return nullptr;
        mEditFloater = editor->getHandle();
    }

    if (editor && !mCommitConnection.connected())
        mCommitConnection = editor->setEditCommitSignal([this](LLSettingsDay::ptr_t pday) { onEditCommitted(pday); });

    return editor;
}


void LLPanelEnvironmentInfo::updateEditFloater(const LLEnvironment::EnvironmentInfo::ptr_t &nextenv, bool enable)
{
    LLFloaterEditExtDayCycle *dayeditor(getEditFloater(false));

    if (!dayeditor)
        return;

    if (!nextenv || !nextenv->mDayCycle || !enable)
    {
        if (mCommitConnection.connected())
            mCommitConnection.disconnect();

        if (dayeditor->isDirty())
            dayeditor->refresh();
        else
            dayeditor->closeFloater();
    }
    else if (dayeditor->getEditingAssetId() != nextenv->mDayCycle->getAssetId()
            || mEditorLastParcelId != nextenv->mParcelId
            || mEditorLastRegionId != nextenv->mRegionId)
    {
        // Ignore dirty
        // If parcel selection changed whatever we do except saving to inventory with
        // old settings will be invalid.
        mEditorLastParcelId = nextenv->mParcelId;
        mEditorLastRegionId = nextenv->mRegionId;
        dayeditor->setEditDayCycle(nextenv->mDayCycle);
    }
}

bool LLPanelEnvironmentInfo::setControlsEnabled(bool enabled)
{
    bool is_unavailable(false);
    bool is_legacy = (mCurrentEnvironment) ? mCurrentEnvironment->mIsLegacy : true;

    if (mNoEnvironment || (!LLEnvironment::instance().isExtendedEnvironmentEnabled() && !isRegion()))
    {
        is_unavailable = true;
        getChild<LLTextBox>(TXT_DISABLED)->setText(getString(STR_LEGACY));
    }
    else if (mNoSelection)
    {
        is_unavailable = true;
        getChild<LLTextBox>(TXT_DISABLED)->setText(getString(STR_NO_PARCEL));
    }
    else if (mCrossRegion)
    {
        is_unavailable = true;
        getChild<LLTextBox>(TXT_DISABLED)->setText(getString(STR_CROSS_REGION));
    }

    if (is_unavailable)
    {
        getChild<LLUICtrl>(PNL_SETTINGS)->setVisible(false);
        getChild<LLUICtrl>(PNL_BUTTONS)->setVisible(false);
        getChild<LLUICtrl>(PNL_DISABLED)->setVisible(true);
        getChild<LLUICtrl>(PNL_ENVIRONMENT_ALTITUDES)->setVisible(FALSE);

        updateEditFloater(mCurrentEnvironment, false);

        return false;
    }
    getChild<LLUICtrl>(PNL_SETTINGS)->setVisible(true);
    getChild<LLUICtrl>(PNL_BUTTONS)->setVisible(true);
    getChild<LLUICtrl>(PNL_DISABLED)->setVisible(false);

    getChild<LLUICtrl>(PNL_ENVIRONMENT_ALTITUDES)->setVisible(isRegion() && LLEnvironment::instance().isExtendedEnvironmentEnabled());

    S32 rdo_selection = getChild<LLRadioGroup>(RDG_ENVIRONMENT_SELECT)->getSelectedIndex();

    bool can_enable = enabled && mCurEnvVersion != INVALID_PARCEL_ENVIRONMENT_VERSION;
    getChild<LLUICtrl>(RDG_ENVIRONMENT_SELECT)->setEnabled(can_enable);
    getChild<LLUICtrl>(RDO_USEDEFAULT)->setEnabled(can_enable && !is_legacy);
    getChild<LLUICtrl>(RDO_USEINV)->setEnabled(false);      // these two are selected automatically based on 
    getChild<LLUICtrl>(RDO_USECUSTOM)->setEnabled(false);
    getChild<LLUICtrl>(EDT_INVNAME)->setEnabled(FALSE);
    getChild<LLUICtrl>(BTN_SELECTINV)->setEnabled(can_enable && !is_legacy);
    getChild<LLUICtrl>(BTN_EDIT)->setEnabled(can_enable);
    getChild<LLUICtrl>(SLD_DAYLENGTH)->setEnabled(can_enable && (rdo_selection != 0) && !is_legacy);
    getChild<LLUICtrl>(SLD_DAYOFFSET)->setEnabled(can_enable && (rdo_selection != 0) && !is_legacy);
    getChild<LLUICtrl>(SLD_ALTITUDES)->setEnabled(can_enable && isRegion() && !is_legacy);
    getChild<LLUICtrl>(ICN_GROUND)->setColor((can_enable && isRegion() && !is_legacy) ? LLColor4::white : LLColor4::grey % 0.8f);
    getChild<LLUICtrl>(PNL_ENVIRONMENT_ALTITUDES)->setEnabled(can_enable && isRegion() && !is_legacy);
    getChild<LLUICtrl>(CHK_ALLOWOVERRIDE)->setEnabled(can_enable && isRegion() && !is_legacy);
    getChild<LLUICtrl>(BTN_APPLY)->setEnabled(can_enable && (mDirtyFlag != 0));
    getChild<LLUICtrl>(BTN_CANCEL)->setEnabled(enabled && (mDirtyFlag != 0));

    getChild<LLSettingsDropTarget>(SDT_DROP_TARGET)->setDndEnabled(enabled && !is_legacy);

    return true;
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

void LLPanelEnvironmentInfo::setDirtyFlag(U32 flag)
{
    bool can_edit = canEdit();
    mDirtyFlag |= flag;
    getChildView(BTN_APPLY)->setEnabled((mDirtyFlag != 0) && mCurEnvVersion != INVALID_PARCEL_ENVIRONMENT_VERSION && can_edit);
    getChildView(BTN_CANCEL)->setEnabled((mDirtyFlag != 0) && can_edit);
}

void LLPanelEnvironmentInfo::clearDirtyFlag(U32 flag)
{
    bool can_edit = canEdit();
    mDirtyFlag &= ~flag;
    getChildView(BTN_APPLY)->setEnabled((mDirtyFlag != 0) && mCurEnvVersion != INVALID_PARCEL_ENVIRONMENT_VERSION && can_edit);
    getChildView(BTN_CANCEL)->setEnabled((mDirtyFlag != 0) && can_edit);
}

void LLPanelEnvironmentInfo::updateAltLabel(const std::string &alt_name, U32 sky_index, F32 alt_value)
{
    LLMultiSliderCtrl *sld = getChild<LLMultiSliderCtrl>(SLD_ALTITUDES);
    LLRect sld_rect = sld->getRect();
    S32 sld_range = sld_rect.getHeight();
    S32 sld_bottom = sld_rect.mBottom;
    S32 sld_offset = sld_rect.getWidth(); // Roughly identical to thumb's width in slider.
    S32 pos = (sld_range - sld_offset) * ((alt_value - 100) / (4000 - 100));

    // get related text box
    LLTextBox* text = getChild<LLTextBox>(alt_name);
    if (text)
    {
        // move related text box
        LLRect rect = text->getRect();
        S32 height = rect.getHeight();
        rect.mBottom = sld_bottom + (sld_offset / 2 + 1) + pos - (height / 2);
        rect.mTop = rect.mBottom + height;
        text->setRect(rect);

        // update text
        std::ostringstream convert;
        convert << alt_value;
        text->setTextArg("[ALTITUDE]", convert.str());
        convert.str("");
        convert.clear();
        convert << sky_index;
        text->setTextArg("[INDEX]", convert.str());
    }
}

void LLPanelEnvironmentInfo::readjustAltLabels()
{
    // Restore ground label position
    LLView* icon = getChild<LLView>(ICN_GROUND);
    LLTextBox* text = getChild<LLTextBox>(alt_labels[ALTITUDE_SLIDER_COUNT]); // one more field then sliders
    LLRect ground_text_rect = text->getRect();
    LLRect icon_rect = icon->getRect();
    S32 height = ground_text_rect.getHeight();
    ground_text_rect.mBottom = icon_rect.mBottom + (icon_rect.getHeight()/2) - (height/2);
    ground_text_rect.mTop = ground_text_rect.mBottom + height;
    text->setRect(ground_text_rect);

    // Re-adjust all labels
    // Very simple "adjust after the fact" method
    // Note: labels are unordered, labels are 1 above sliders due to 'ground'

    for (U32 i = 0; i < ALTITUDE_SLIDER_COUNT; i++)
    {
        LLTextBox* text_cmp = getChild<LLTextBox>(alt_labels[i]);

        for (U32 j = i + 1; j <= ALTITUDE_SLIDER_COUNT; j++)
        {
            LLTextBox* text_intr = getChild<LLTextBox>(alt_labels[j]);
            if (text_cmp && text_intr)
            {
                LLRect cmp_rect = text_cmp->getRect();
                LLRect intr_rect = text_intr->getRect();
                S32 shift = 0;
                if (cmp_rect.mBottom <= intr_rect.mTop && cmp_rect.mBottom >= intr_rect.mBottom)
                {
                    // Aproximate shift
                    // We probably will need more cycle runs over all labels to get accurate one
                    // At the moment single cycle should do since we have too little elements to do something complicated
                    shift = (cmp_rect.mBottom - intr_rect.mTop) / 2;
                }
                else if (cmp_rect.mTop >= intr_rect.mBottom && cmp_rect.mTop <= intr_rect.mTop)
                {
                    // Aproximate shift
                    shift = (cmp_rect.mTop - intr_rect.mBottom) / 2;
                }
                if (shift != 0)
                {
                    cmp_rect.translate(0, -shift);
                    text_cmp->setRect(cmp_rect);

                    intr_rect.translate(0, shift);
                    text_intr->setRect(intr_rect);
                }
            }
        }
    }
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

    udpateApparentTimeOfDay();
}

void LLPanelEnvironmentInfo::onSldDayOffsetChanged(F32 value)
{
    F32Hours dayoffset(value);

    if (dayoffset.value() < 0.0f)
        dayoffset += F32Hours(24.0);

    mCurrentEnvironment->mDayOffset = dayoffset;
    setDirtyFlag(DIRTY_FLAG_DAYOFFSET);

    udpateApparentTimeOfDay();
}

void LLPanelEnvironmentInfo::onAltSliderCallback(LLUICtrl *cntrl, const LLSD &data)
{
    LLMultiSliderCtrl *sld = (LLMultiSliderCtrl *)cntrl;
    std::string sld_name = sld->getCurSlider();
    F32 sld_value = sld->getCurSliderValue();

    mAltitudes[sld_name].mAltitude = sld_value;

    // update all labels since we could have jumped multiple and we will need to readjust
    // (or sort by altitude, too little elements, so I didn't bother with efficiency)
    altitudes_data_t::iterator end = mAltitudes.end();
    altitudes_data_t::iterator iter = mAltitudes.begin();
    altitudes_data_t::iterator iter2;
    U32 new_index;
    while (iter != end)
    {
        iter2 = mAltitudes.begin();
        new_index = 1;
        while (iter2 != end)
        {
            if (iter->second.mAltitude > iter2->second.mAltitude)
            {
                new_index++;
            }
            iter2++;
        }
        iter->second.mAltitudeIndex = new_index;
        updateAltLabel(alt_labels[iter->second.mLabelIndex], iter->second.mAltitudeIndex + 1, iter->second.mAltitude);
        iter++;
    }

    readjustAltLabels();
    setDirtyFlag(DIRTY_FLAG_ALTITUDES);
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
    static const S32 FOURHOURS(4 * 60 * 60);

    LLFloaterEditExtDayCycle *dayeditor = getEditFloater();

    LLSD params(LLSDMap(LLFloaterEditExtDayCycle::KEY_EDIT_CONTEXT, isRegion() ? LLFloaterEditExtDayCycle::VALUE_CONTEXT_REGION : LLFloaterEditExtDayCycle::VALUE_CONTEXT_PARCEL)
            (LLFloaterEditExtDayCycle::KEY_DAY_LENGTH,  mCurrentEnvironment ? (S32)(mCurrentEnvironment->mDayLength.value()) : FOURHOURS)
            (LLFloaterEditExtDayCycle::KEY_CANMOD,      LLSD::Boolean(true)));

    dayeditor->openFloater(params);
    if (mCurrentEnvironment && mCurrentEnvironment->mDayCycle)
        dayeditor->setEditDayCycle(mCurrentEnvironment->mDayCycle);
    else
        dayeditor->setEditDefaultDayCycle();
}

void LLPanelEnvironmentInfo::onBtnSelect()
{
    LLFloaterSettingsPicker *picker = getSettingsPicker();
    if (picker)
    {
        LLUUID item_id;
        if (mCurrentEnvironment && mCurrentEnvironment->mDayCycle)
        {
            item_id = LLFloaterSettingsPicker::findItemID(mCurrentEnvironment->mDayCycle->getAssetId(), false, false);
        }
        picker->setSettingsFilter(LLSettingsType::ST_NONE);
        picker->setSettingsItemId(item_id);
        picker->openFloater();
        picker->setFocus(TRUE);
    }
}


void LLPanelEnvironmentInfo::doApply()
{
    S32 parcel_id = getParcelId();

    if (getIsDirtyFlag(DIRTY_FLAG_MASK))
    {
        LLHandle<LLPanel> that_h = getHandle();
        LLEnvironment::altitudes_vect_t alts;

        S32 rdo_selection = getChild<LLRadioGroup>(RDG_ENVIRONMENT_SELECT)->getSelectedIndex();

        if (isRegion() && getIsDirtyFlag(DIRTY_FLAG_ALTITUDES))
        {
            altitudes_data_t::iterator it;
            for (auto alt : mAltitudes)
            {
                alts.push_back(alt.second.mAltitude);
            }
        }

        if (rdo_selection == 0)
        {
            mCurEnvVersion = INVALID_PARCEL_ENVIRONMENT_VERSION;
            LLEnvironment::instance().resetParcel(parcel_id,
                [that_h](S32 parcel_id, LLEnvironment::EnvironmentInfo::ptr_t envifo) { _onEnvironmentReceived(that_h, parcel_id, envifo); });
        }
        else if (rdo_selection == 1)
        {
            if (!mCurrentEnvironment)
            {
                // Attempting to save mid update?
                LL_WARNS("ENVPANEL") << "Failed to apply changes from editor! Dirty state: " << mDirtyFlag << " update state: " << mCurEnvVersion << LL_ENDL;
                return;
            }
            mCurEnvVersion = INVALID_PARCEL_ENVIRONMENT_VERSION;
            LLEnvironment::instance().updateParcel(parcel_id,
                mCurrentEnvironment->mDayCycle->getAssetId(), std::string(), mCurrentEnvironment->mDayLength.value(), 
                mCurrentEnvironment->mDayOffset.value(), alts,
                [that_h](S32 parcel_id, LLEnvironment::EnvironmentInfo::ptr_t envifo) { _onEnvironmentReceived(that_h, parcel_id, envifo); });
        }
        else
        {
            if (!mCurrentEnvironment)
            {
                // Attempting to save mid update?
                LL_WARNS("ENVPANEL") << "Failed to apply changes from editor! Dirty state: " << mDirtyFlag << " update state: " << mCurEnvVersion << LL_ENDL;
                return;
            }
            mCurEnvVersion = INVALID_PARCEL_ENVIRONMENT_VERSION;
            LLEnvironment::instance().updateParcel(parcel_id,
                mCurrentEnvironment->mDayCycle, mCurrentEnvironment->mDayLength.value(), mCurrentEnvironment->mDayOffset.value(), alts,
                [that_h](S32 parcel_id, LLEnvironment::EnvironmentInfo::ptr_t envifo) { _onEnvironmentReceived(that_h, parcel_id, envifo); });
        }

        setControlsEnabled(false);
    }
}


void LLPanelEnvironmentInfo::udpateApparentTimeOfDay()
{
    static const F32 SECONDSINDAY(24.0 * 60.0 * 60.0);

    if ((!mCurrentEnvironment) || (mCurrentEnvironment->mDayLength.value() < 1.0) || (mCurrentEnvironment->mDayOffset.value() < 1.0))
    {
        getChild<LLUICtrl>(LBL_TIMEOFDAY)->setVisible(false);
        return;
    }
    getChild<LLUICtrl>(LBL_TIMEOFDAY)->setVisible(true);

    S32Seconds  now(LLDate::now().secondsSinceEpoch());

    now += mCurrentEnvironment->mDayOffset;

    F32 perc = (F32)(now.value() % mCurrentEnvironment->mDayLength.value()) / (F32)(mCurrentEnvironment->mDayLength.value());

    S32Seconds  secondofday((S32)(perc * SECONDSINDAY));
    S32Hours    hourofday(secondofday);
    S32Seconds  secondofhour(secondofday - hourofday);
    S32Minutes  minutesofhour(secondofhour);
    bool        am_pm(hourofday.value() >= 12);

    if (hourofday.value() < 1)
        hourofday = S32Hours(12);
    if (hourofday.value() > 12)
        hourofday -= S32Hours(12);

    std::string lblminute(((minutesofhour.value() < 10) ? "0" : "") + LLSD(minutesofhour.value()).asString());


    getChild<LLUICtrl>(LBL_TIMEOFDAY)->setTextArg("[HH]", LLSD(hourofday.value()).asString());
    getChild<LLUICtrl>(LBL_TIMEOFDAY)->setTextArg("[MM]", lblminute);
    getChild<LLUICtrl>(LBL_TIMEOFDAY)->setTextArg("[AP]", std::string(am_pm ? "PM" : "AM"));
    getChild<LLUICtrl>(LBL_TIMEOFDAY)->setTextArg("[PRC]", LLSD((S32)(100 * perc)).asString());

}

void LLPanelEnvironmentInfo::onIdlePlay(void *data)
{
    ((LLPanelEnvironmentInfo *)data)->udpateApparentTimeOfDay();
}

void LLPanelEnvironmentInfo::onPickerCommitted(LLUUID item_id)
{
    LLInventoryItem *itemp = gInventory.getItem(item_id);
    if (itemp)
    {
        LLSettingsVOBase::getSettingsAsset(itemp->getAssetUUID(), [this](LLUUID, LLSettingsBase::ptr_t settings, S32 status, LLExtStat) {
            if (status)
                return;
            onPickerAssetDownloaded(settings);
        });
    }
}

void LLPanelEnvironmentInfo::onEditCommitted(LLSettingsDay::ptr_t newday)
{
    if (!newday)
    {
        LL_WARNS("ENVPANEL") << "Editor committed an empty day. Do nothing." << LL_ENDL;
        return;
    }
    if (!mCurrentEnvironment)
    {
        // Attempting to save mid update?
        LL_WARNS("ENVPANEL") << "Failed to apply changes from editor! Dirty state: " << mDirtyFlag << " env version: " << mCurEnvVersion << LL_ENDL;
        return;
    }
    size_t newhash(newday->getHash());
    size_t oldhash((mCurrentEnvironment->mDayCycle) ? mCurrentEnvironment->mDayCycle->getHash() : 0);

    if (newhash != oldhash)
    {
        mCurrentEnvironment->mDayCycle = newday;
        setDirtyFlag(DIRTY_FLAG_DAYCYCLE);
        refresh();
    }
}

void LLPanelEnvironmentInfo::onEnvironmentChanged(LLEnvironment::EnvSelection_t env)
{
    if (isRegion())
    {
        // Note: at the moment mCurEnvVersion is only applyable to parcels, we might need separate version control for regions
        // but mCurEnvVersion still acts like indicator that update is pending
        if (env == LLEnvironment::ENV_REGION)
        {
            mCurrentEnvironment.reset();
            refreshFromSource();
        }
    }
    else if ((env == LLEnvironment::ENV_PARCEL) && (getParcelId() == LLViewerParcelMgr::instance().getAgentParcelId()))
    {
        // Panel receives environment from different sources, from environment update callbacks,
        // from hovers (causes callbacks on version change) and from personal requests
        // filter out dupplicates and out of order packets by checking parcel environment version.
        LL_DEBUGS("ENVPANEL") << "Received environment update " << mCurEnvVersion << " " << (getParcel() ? getParcel()->getParcelEnvironmentVersion() : (S32)-1) << LL_ENDL;
        LLParcel *parcel = getParcel();
        if (parcel && mCurEnvVersion < parcel->getParcelEnvironmentVersion())
        {
            mCurrentEnvironment.reset();
            refreshFromSource();
        }
        else
        {
            refresh();
        }
    }
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

void LLPanelEnvironmentInfo::onEnvironmentReceived(S32 parcel_id, LLEnvironment::EnvironmentInfo::ptr_t envifo)
{
    if (parcel_id != getParcelId())
    {
        LL_WARNS("ENVPANEL") << "Have environment for parcel " << parcel_id << " expecting " << getParcelId() << ". Discarding." << LL_ENDL;
        return;
    }
    mCurrentEnvironment = envifo;
    clearDirtyFlag(DIRTY_FLAG_MASK);
    if (parcel_id == INVALID_PARCEL_ID)
    {
        // region, no version
        mCurEnvVersion = 1;
    }
    else
    {
        LLParcel* parcel = getParcel();
        if (parcel)
        {
            // not always up to date, we will get onEnvironmentChanged() update in such case.
            mCurEnvVersion = parcel->getParcelEnvironmentVersion();
        }
        LL_DEBUGS("ENVPANEL") << " Setting environment version: " << mCurEnvVersion << LL_ENDL;
    }
    refresh();
}

void LLPanelEnvironmentInfo::_onEnvironmentReceived(LLHandle<LLPanel> that_h, S32 parcel_id, LLEnvironment::EnvironmentInfo::ptr_t envifo)
{
    LLPanelEnvironmentInfo *that = (LLPanelEnvironmentInfo *)that_h.get();
    if (!that)
        return;
    that->onEnvironmentReceived(parcel_id, envifo);
}

LLSettingsDropTarget::LLSettingsDropTarget(const LLSettingsDropTarget::Params& p)
    : LLView(p), mEnvironmentInfoPanel(NULL), mDndEnabled(false)
{}

BOOL LLSettingsDropTarget::handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
	EDragAndDropType cargo_type,
	void* cargo_data,
	EAcceptance* accept,
	std::string& tooltip_msg)
{
    BOOL handled = FALSE;

    if (getParent() && mDndEnabled)
    {
        handled = TRUE;

        switch (cargo_type)
        {
        case DAD_SETTINGS:
        {
            LLViewerInventoryItem* inv_item = (LLViewerInventoryItem*)cargo_data;
            if (inv_item && mEnvironmentInfoPanel)
            {
                LLUUID item_id = inv_item->getUUID();
                if (gInventory.getItem(item_id))
                {
                    *accept = ACCEPT_YES_COPY_SINGLE;
                    if (drop)
                    {
                        mEnvironmentInfoPanel->onPickerCommitted(item_id);
                    }
                }
            }
            else
            {
                *accept = ACCEPT_NO;
            }
            break;
        }
        default:
            *accept = ACCEPT_NO;
            break;
        }
    }
    return handled;
}
