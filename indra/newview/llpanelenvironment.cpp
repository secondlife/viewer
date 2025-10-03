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
#include "lliconctrl.h"
#include "llmultisliderctrl.h"
#include "llnotificationsutil.h"
#include "llsettingsvo.h"

#include "llappviewer.h"
#include "llcallbacklist.h"
#include "llviewercontrol.h"
#include "llviewerparcelmgr.h"

#include "llinventorymodel.h"

//=========================================================================
namespace
{
    const std::string FLOATER_DAY_CYCLE_EDIT("env_edit_extdaycycle");
    const std::string STRING_REGION_ENV("str_region_env");
    const std::string STRING_EMPTY_NAME("str_empty");
}

//=========================================================================
const std::string LLPanelEnvironmentInfo::BTN_SELECTINV("btn_select_inventory");
const std::string LLPanelEnvironmentInfo::BTN_EDIT("btn_edit");
const std::string LLPanelEnvironmentInfo::BTN_USEDEFAULT("btn_usedefault");
const std::string LLPanelEnvironmentInfo::BTN_RST_ALTITUDES("btn_rst_altitudes");
const std::string LLPanelEnvironmentInfo::SLD_DAYLENGTH("sld_day_length");
const std::string LLPanelEnvironmentInfo::SLD_DAYOFFSET("sld_day_offset");
const std::string LLPanelEnvironmentInfo::SLD_ALTITUDES("sld_altitudes");
const std::string LLPanelEnvironmentInfo::ICN_GROUND("icon_ground");
const std::string LLPanelEnvironmentInfo::ICN_WATER("icon_water");
const std::string LLPanelEnvironmentInfo::CHK_ALLOWOVERRIDE("chk_allow_override");
const std::string LLPanelEnvironmentInfo::LBL_TIMEOFDAY("lbl_apparent_time");
const std::string LLPanelEnvironmentInfo::PNL_SETTINGS("pnl_environment_config");
const std::string LLPanelEnvironmentInfo::PNL_ENVIRONMENT_ALTITUDES("pnl_environment_altitudes");
const std::string LLPanelEnvironmentInfo::PNL_BUTTONS("pnl_environment_buttons");
const std::string LLPanelEnvironmentInfo::PNL_DISABLED("pnl_environment_disabled");
const std::string LLPanelEnvironmentInfo::TXT_DISABLED("txt_environment_disabled");
const std::string LLPanelEnvironmentInfo::PNL_REGION_MSG("pnl_environment_region_msg");
const std::string LLPanelEnvironmentInfo::SDT_DROP_TARGET("sdt_drop_target");

const std::string LLPanelEnvironmentInfo::STR_LABEL_USEDEFAULT("str_label_use_default");
const std::string LLPanelEnvironmentInfo::STR_LABEL_USEREGION("str_label_use_region");
const std::string LLPanelEnvironmentInfo::STR_ALTITUDE_DESCRIPTION("str_altitude_desription");
const std::string LLPanelEnvironmentInfo::STR_NO_PARCEL("str_no_parcel");
const std::string LLPanelEnvironmentInfo::STR_CROSS_REGION("str_cross_region");
const std::string LLPanelEnvironmentInfo::STR_LEGACY("str_legacy");
const std::string LLPanelEnvironmentInfo::STR_DISALLOWED("str_disallowed");
const std::string LLPanelEnvironmentInfo::STR_TOO_SMALL("str_too_small");

const S32 LLPanelEnvironmentInfo::MINIMUM_PARCEL_SIZE(128);

const U32 LLPanelEnvironmentInfo::DIRTY_FLAG_DAYCYCLE(0x01 << 0);
const U32 LLPanelEnvironmentInfo::DIRTY_FLAG_DAYLENGTH(0x01 << 1);
const U32 LLPanelEnvironmentInfo::DIRTY_FLAG_DAYOFFSET(0x01 << 2);
const U32 LLPanelEnvironmentInfo::DIRTY_FLAG_ALTITUDES(0x01 << 3);

const U32 LLPanelEnvironmentInfo::DIRTY_FLAG_MASK(
        LLPanelEnvironmentInfo::DIRTY_FLAG_DAYCYCLE |
        LLPanelEnvironmentInfo::DIRTY_FLAG_DAYLENGTH |
        LLPanelEnvironmentInfo::DIRTY_FLAG_DAYOFFSET |
        LLPanelEnvironmentInfo::DIRTY_FLAG_ALTITUDES);

const F32 ALTITUDE_DEFAULT_HEIGHT_STEP = 1000;

const std::string slider_marker_base = "mark";

const std::string alt_sliders[] = {
    "sld1",
    "sld2",
    "sld3",
};

const std::string alt_prefixes[] = {
    "alt1",
    "alt2",
    "alt3",
    "ground",
    "water",
};

const std::string alt_panels[] = {
    "pnl_alt1",
    "pnl_alt2",
    "pnl_alt3",
    "pnl_ground",
    "pnl_water",
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
    mEditFloater(),
    mAllowOverride(true)
{
}

LLPanelEnvironmentInfo::~LLPanelEnvironmentInfo()
{
    if (mChangeMonitor.connected())
        mChangeMonitor.disconnect();
    if (mCommitConnection.connected())
        mCommitConnection.disconnect();
    if (mUpdateConnection.connected())
        mUpdateConnection.disconnect();
}

bool LLPanelEnvironmentInfo::postBuild()
{
    mIconGround = getChild<LLIconCtrl>(ICN_GROUND);
    mIconWater = getChild<LLIconCtrl>(ICN_WATER);

    mPanelEnvAltitudes = getChild<LLUICtrl>(PNL_ENVIRONMENT_ALTITUDES);
    mPanelEnvConfig = getChild<LLUICtrl>(PNL_SETTINGS);
    mPanelEnvButtons = getChild <LLUICtrl>(PNL_BUTTONS);
    mPanelEnvDisabled = getChild<LLUICtrl>(PNL_DISABLED);
    mPanelEnvRegionMsg = getChild<LLUICtrl>(PNL_REGION_MSG);

    mEnvironmentDisabledText = getChild<LLTextBox>(TXT_DISABLED);
    mLabelApparentTime = getChild<LLTextBox>(LBL_TIMEOFDAY);

    mBtnUseDefault = getChild<LLButton>(BTN_USEDEFAULT);
    mBtnUseDefault->setCommitCallback([this](LLUICtrl *, const LLSD &){ onBtnDefault(); });

    mBtnSelectInv = getChild<LLButton>(BTN_SELECTINV);
    mBtnSelectInv->setCommitCallback([this](LLUICtrl *, const LLSD &){ onBtnSelect(); });

    mBtnEdit = getChild<LLButton>(BTN_EDIT);
    mBtnEdit->setCommitCallback([this](LLUICtrl *, const LLSD &){ onBtnEdit(); });

    mBtnResetAltitudes = getChild<LLButton>(BTN_RST_ALTITUDES);
    mBtnResetAltitudes->setCommitCallback([this](LLUICtrl *, const LLSD &){ onBtnRstAltitudes(); });

    mCheckAllowOverride = getChild<LLCheckBoxCtrl>(CHK_ALLOWOVERRIDE);

    mSliderDayLength = getChild<LLSliderCtrl>(SLD_DAYLENGTH);
    mSliderDayLength->setCommitCallback([this](LLUICtrl *, const LLSD &value) { onSldDayLengthChanged((F32)value.asReal()); });
    mSliderDayLength->setSliderMouseUpCallback([this](LLUICtrl *, const LLSD &) { onDayLenOffsetMouseUp(); });
    mSliderDayLength->setSliderEditorCommitCallback([this](LLUICtrl *, const LLSD &) { onDayLenOffsetMouseUp(); });

    mSliderDayOffset = getChild<LLSliderCtrl>(SLD_DAYOFFSET);
    mSliderDayOffset->setCommitCallback([this](LLUICtrl *, const LLSD &value) { onSldDayOffsetChanged((F32)value.asReal()); });
    mSliderDayOffset->setSliderMouseUpCallback([this](LLUICtrl *, const LLSD &) { onDayLenOffsetMouseUp(); });
    mSliderDayOffset->setSliderEditorCommitCallback([this](LLUICtrl *, const LLSD &) { onDayLenOffsetMouseUp(); });

    mMultiSliderAltitudes = getChild<LLMultiSliderCtrl>(SLD_ALTITUDES);
    mMultiSliderAltitudes->setCommitCallback([this](LLUICtrl *cntrl, const LLSD &value) { onAltSliderCallback(cntrl, value); });
    mMultiSliderAltitudes->setSliderMouseUpCallback([this](LLUICtrl *, const LLSD &) { onAltSliderMouseUp(); });

    mChangeMonitor = LLEnvironment::instance().setEnvironmentChanged([this](LLEnvironment::EnvSelection_t env, S32 version) { onEnvironmentChanged(env, version); });

    for (U32 idx = 0; idx < ALTITUDE_MARKERS_COUNT; idx++)
    {
        mAltitudeMarkers[idx] = findChild<LLUICtrl>(slider_marker_base + llformat("%u", idx));
    }

    for (U32 idx = 0; idx < ALTITUDE_PREFIXERS_COUNT; idx++)
    {
        mAltitudeDropTarget[idx] = findChild<LLSettingsDropTarget>("sdt_" + alt_prefixes[idx]);
        mAltitudeLabels[idx] = findChild<LLTextBox>("txt_" + alt_prefixes[idx]);
        mAltitudeEditor[idx] = findChild<LLLineEditor>("edt_invname_" + alt_prefixes[idx]);
        mAltitudePanels[idx] = findChild<LLView>("pnl_" + alt_prefixes[idx]);
    }

    for (U32 idx = 0; idx < ALTITUDE_SLIDER_COUNT; idx++)
    {
        LLSettingsDropTarget* drop_target = findChild<LLSettingsDropTarget>("sdt_" + alt_prefixes[idx]);
        if (drop_target)
        {
            drop_target->setPanel(this, alt_sliders[idx]);
        }

        // set initial values to prevent [ALTITUDE] from displaying
        updateAltLabel(idx, idx + 2, (F32)(idx * 1000));
    }
    mAltitudeDropTarget[3]->setPanel(this, alt_prefixes[3]);
    mAltitudeDropTarget[4]->setPanel(this, alt_prefixes[4]);

    return true;
}

// virtual
void LLPanelEnvironmentInfo::onOpen(const LLSD& key)
{
    refreshFromSource();
}

// virtual
void LLPanelEnvironmentInfo::onVisibilityChange(bool new_visibility)
{
    if (new_visibility)
    {
        gIdleCallbacks.addFunction(onIdlePlay, this);
    }
    else
    {
        commitDayLenOffsetChanges(false); // arrow-key changes

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

    F32Hours daylength(mCurrentEnvironment->mDayLength);
    F32Hours dayoffset(mCurrentEnvironment->mDayOffset);

    if (dayoffset.value() > 12.0f)
        dayoffset -= F32Hours(24.0);

    mSliderDayLength->setValue(daylength.value());
    mSliderDayOffset->setValue(dayoffset.value());

    udpateApparentTimeOfDay();

    updateEditFloater(mCurrentEnvironment, canEdit());

    LLEnvironment::altitude_list_t altitudes = mCurrentEnvironment->mAltitudes;

    if (altitudes.size() > 0)
    {
        mMultiSliderAltitudes->clear();

        for (S32 idx = 0; idx < ALTITUDE_SLIDER_COUNT; ++idx)
        {
            // make sure values are in range, server is supposed to validate them,
            // but issues happen, try to fix values in such case
            F32 altitude = llclamp(altitudes[idx + 1], mMultiSliderAltitudes->getMinValue(), mMultiSliderAltitudes->getMaxValue());
            bool res = mMultiSliderAltitudes->addSlider(altitude, alt_sliders[idx]);
            if (!res)
            {
                LL_WARNS_ONCE("ENVPANEL") << "Failed to validate altitude from server for parcel id" << getParcelId() << LL_ENDL;
                // Find a spot to insert altitude.
                // Assuming everything alright with slider, we should find new place in 11 steps top (step 25m, no overlap 100m)
                F32 alt_step = (altitude > (mMultiSliderAltitudes->getMaxValue() / 2)) ? -mMultiSliderAltitudes->getIncrement() : mMultiSliderAltitudes->getIncrement();
                for (U32 i = 0; i < 30; i++)
                {
                    altitude += alt_step;
                    if (altitude > mMultiSliderAltitudes->getMaxValue())
                    {
                        altitude = mMultiSliderAltitudes->getMinValue();
                    }
                    else if (altitude < mMultiSliderAltitudes->getMinValue())
                    {
                        altitude = mMultiSliderAltitudes->getMaxValue();
                    }
                    res = mMultiSliderAltitudes->addSlider(altitude, alt_sliders[idx]);
                    if (res) break;
                }
            }
            if (res)
            {
                // slider has some auto correction that might have kicked in
                altitude = mMultiSliderAltitudes->getSliderValue(alt_sliders[idx]);
            }
            else
            {
                // Something is very very wrong
                LL_WARNS_ONCE("ENVPANEL") << "Failed to set up altitudes for parcel id " << getParcelId() << LL_ENDL;
            }
            updateAltLabel(idx, idx + 2, altitude);
            mAltitudes[alt_sliders[idx]] = AltitudeData(idx + 2, idx, altitude);
        }
        if (mMultiSliderAltitudes->getCurNumSliders() != ALTITUDE_SLIDER_COUNT)
        {
            LL_WARNS("ENVPANEL") << "Failed to add altitude sliders!" << LL_ENDL;
        }
        readjustAltLabels();
        mMultiSliderAltitudes->resetCurSlider();
    }

    updateAltLabel(3, 1, 0); // ground
    updateAltLabel(4, 0, 0); // water

}

void LLPanelEnvironmentInfo::refreshFromEstate()
{
    LLViewerRegion* pRegion = gAgent.getRegion();
    if (pRegion)
    {
        bool oldAO = mAllowOverride;
        mAllowOverride = (isRegion() && LLEstateInfoModel::instance().getAllowEnvironmentOverride()) || pRegion->getAllowEnvironmentOverride();
        if (oldAO != mAllowOverride)
            refresh();
    }
}

std::string LLPanelEnvironmentInfo::getNameForTrackIndex(U32 index)
{
    std::string invname;
    if (!mCurrentEnvironment || index < LLSettingsDay::TRACK_WATER || index >= LLSettingsDay::TRACK_MAX)
    {
        invname = getString(STRING_EMPTY_NAME);
    }
    else if (mCurrentEnvironment->mDayCycleName.empty())
    {
        invname = mCurrentEnvironment->mNameList[index];

        if (invname.empty())
        {
            if (index <= LLSettingsDay::TRACK_GROUND_LEVEL)
                invname = getString(isRegion() ? STRING_EMPTY_NAME : STRING_REGION_ENV);
        }
    }
    else if (!mCurrentEnvironment->mDayCycle->isTrackEmpty(index))
    {
        invname = mCurrentEnvironment->mDayCycleName;
    }


    if (invname.empty())
    {
        invname = getNameForTrackIndex(index - 1);
        if (!invname.empty() && invname.front() != '(')
        {
            invname = "(" + invname + ")";
        }
    }

    return invname;
}

LLFloaterSettingsPicker * LLPanelEnvironmentInfo::getSettingsPicker(bool create)
{
    LLFloaterSettingsPicker *picker = static_cast<LLFloaterSettingsPicker *>(mSettingsFloater.get());

    // Show the dialog
    if (!picker && create)
    {
        picker = new LLFloaterSettingsPicker(this,
            LLUUID::null);

        mSettingsFloater = picker->getHandle();

        picker->setCommitCallback([this](LLUICtrl *, const LLSD &data){ onPickerCommitted(data["ItemId"].asUUID()); });
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

    if (!dayeditor || !dayeditor->isInVisibleChain())
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
    bool is_bigenough = isLargeEnough();

    if (mNoEnvironment || (!LLEnvironment::instance().isExtendedEnvironmentEnabled() && !isRegion()))
    {
        is_unavailable = true;
        mEnvironmentDisabledText->setText(getString(STR_LEGACY));
    }
    else if (mNoSelection)
    {
        is_unavailable = true;
        mEnvironmentDisabledText->setText(getString(STR_NO_PARCEL));
    }
    else if (mCrossRegion)
    {
        is_unavailable = true;
        mEnvironmentDisabledText->setText(getString(STR_CROSS_REGION));
    }
    else if (!isRegion() && !mAllowOverride)
    {
        is_unavailable = true;
        mEnvironmentDisabledText->setText(getString(STR_DISALLOWED));
    }
    else if (!is_bigenough)
    {
        is_unavailable = true;
        mEnvironmentDisabledText->setText(getString(STR_TOO_SMALL));
    }

    if (is_unavailable)
    {
        mPanelEnvConfig->setVisible(false);
        mPanelEnvButtons->setVisible(false);
        mPanelEnvDisabled->setVisible(true);
        mPanelEnvAltitudes->setVisible(false);
        mPanelEnvRegionMsg->setVisible(false);
        updateEditFloater(mCurrentEnvironment, false);

        return false;
    }
    mPanelEnvConfig->setVisible(true);
    mPanelEnvButtons->setVisible(true);
    mPanelEnvDisabled->setVisible(false);
    mPanelEnvRegionMsg->setVisible(isRegion());

    mPanelEnvAltitudes->setVisible(LLEnvironment::instance().isExtendedEnvironmentEnabled());
    mBtnResetAltitudes->setVisible(isRegion());

    bool can_enable = enabled && !is_legacy && mCurrentEnvironment && (mCurEnvVersion != INVALID_PARCEL_ENVIRONMENT_VERSION);
    mBtnSelectInv->setEnabled(can_enable);
    mBtnUseDefault->setEnabled(can_enable);
    mBtnEdit->setEnabled(can_enable);
    mSliderDayLength->setEnabled(can_enable);
    mSliderDayOffset->setEnabled(can_enable);
    mMultiSliderAltitudes->setEnabled(can_enable && isRegion());
    mIconGround->setColor((can_enable && isRegion()) ? LLColor4::white : LLColor4::grey % 0.8f);
    mIconWater->setColor((can_enable && isRegion()) ? LLColor4::white : LLColor4::grey % 0.8f);
    mBtnResetAltitudes->setEnabled(can_enable && isRegion());
    mPanelEnvAltitudes->setEnabled(can_enable);
    mCheckAllowOverride->setEnabled(can_enable && isRegion());

    for (U32 idx = 0; idx < ALTITUDE_MARKERS_COUNT; idx++)
    {
        if (mAltitudeMarkers[idx])
        {
            static LLColor4 marker_color(0.75f, 0.75f, 0.75f, 1.f);
            mAltitudeMarkers[idx]->setColor((can_enable && isRegion()) ? marker_color : marker_color % 0.3f);
        }
    }

    for (U32 idx = 0; idx < ALTITUDE_PREFIXERS_COUNT; idx++)
    {
        if (mAltitudeDropTarget[idx])
        {
            mAltitudeDropTarget[idx]->setDndEnabled(can_enable);
        }
    }

    return true;
}

void LLPanelEnvironmentInfo::setDirtyFlag(U32 flag)
{
    mDirtyFlag |= flag;
}

void LLPanelEnvironmentInfo::clearDirtyFlag(U32 flag)
{
    mDirtyFlag &= ~flag;
}

void LLPanelEnvironmentInfo::updateAltLabel(U32 alt_index, U32 sky_index, F32 alt_value)
{
    LLRect sld_rect = mMultiSliderAltitudes->getRect();
    S32 sld_range = sld_rect.getHeight();
    S32 sld_bottom = sld_rect.mBottom;
    S32 sld_offset = sld_rect.getWidth(); // Roughly identical to thumb's width in slider.
    S32 pos = (S32)((sld_range - sld_offset) * ((alt_value - 100) / (4000 - 100)));

    // get related views
    LLTextBox* text = mAltitudeLabels[alt_index];
    LLLineEditor* field = mAltitudeEditor[alt_index];
    LLView* alt_panel = mAltitudePanels[alt_index];

    if (text && (sky_index > 1))
    {
        // update text
        std::ostringstream convert;
        convert << alt_value;
        text->setTextArg("[ALTITUDE]", convert.str());
        convert.str("");
        convert.clear();
        convert << sky_index;
        text->setTextArg("[INDEX]", convert.str());
    }

    if (field)
    {
        field->setText(getNameForTrackIndex(sky_index));
    }

    if (alt_panel && (sky_index > 1))
    {
        // move containing panel
        LLRect rect = alt_panel->getRect();
        S32 height = rect.getHeight();
        rect.mBottom = sld_bottom + (sld_offset / 2 + 1) + pos - (height / 2);
        rect.mTop = rect.mBottom + height;
        alt_panel->setRect(rect);
    }

}

void LLPanelEnvironmentInfo::readjustAltLabels()
{
    // Re-adjust all labels
    // Very simple "adjust after the fact" method
    // Note: labels can be in any order

    LLView* view_midle = NULL;
    U32 midle_ind = 0;
    S32 shift_up = 0;
    S32 shift_down = 0;
    LLRect sld_rect = mMultiSliderAltitudes->getRect();

    // Find the middle one
    for (U32 i = 0; i < ALTITUDE_SLIDER_COUNT; i++)
    {
        LLView* cmp_view = mAltitudePanels[i];
        if (!cmp_view) return;
        LLRect cmp_rect = cmp_view->getRect();
        S32 pos = 0;
        shift_up = 0;
        shift_down = 0;

        for (U32 j = 0; j < ALTITUDE_SLIDER_COUNT; j++)
        {
            if (i != j)
            {
                LLView* intr_view = mAltitudePanels[j];
                if (!intr_view) return;
                LLRect intr_rect = intr_view->getRect();
                if (cmp_rect.mBottom >= intr_rect.mBottom)
                {
                    pos++;
                }
                if (intr_rect.mBottom <= cmp_rect.mTop && intr_rect.mBottom >= cmp_rect.mBottom)
                {
                    shift_up = cmp_rect.mTop - intr_rect.mBottom;
                }
                else if (intr_rect.mTop >= cmp_rect.mBottom && intr_rect.mBottom <= cmp_rect.mBottom)
                {
                    shift_down = cmp_rect.mBottom - intr_rect.mTop;
                }
            }
        }
        if (pos == 1) //  middle
        {
            view_midle = cmp_view;
            midle_ind = i;
            break;
        }
    }

    // Account for edges
    LLRect midle_rect = view_midle->getRect();
    F32 factor = 0.5f;
    S32 edge_zone_height = (S32)(midle_rect.getHeight() * 1.5f);

    if (midle_rect.mBottom - sld_rect.mBottom < edge_zone_height)
    {
        factor = 1.f - (F32)((midle_rect.mBottom - sld_rect.mBottom) / (edge_zone_height * 2));
    }
    else if (sld_rect.mTop - midle_rect.mTop < edge_zone_height )
    {
        factor = (F32)((sld_rect.mTop - midle_rect.mTop) / (edge_zone_height * 2));
    }

    S32 shift_middle = (S32)(((F32)shift_down * factor) + ((F32)shift_up * (1.f - factor)));
    shift_down = shift_down - shift_middle;
    shift_up = shift_up - shift_middle;

    // fix crossings
    for (U32 i = 0; i < ALTITUDE_SLIDER_COUNT; i++)
    {
        if (i != midle_ind)
        {
            LLView* trn_view = mAltitudePanels[i];
            LLRect trn_rect = trn_view->getRect();

            if (trn_rect.mBottom <= midle_rect.mTop && trn_rect.mBottom >= midle_rect.mBottom)
            {
                // Approximate shift
                trn_rect.translate(0, shift_up);
                trn_view->setRect(trn_rect);
            }
            else if (trn_rect.mTop >= midle_rect.mBottom && trn_rect.mBottom <= midle_rect.mBottom)
            {
                // Approximate shift
                trn_rect.translate(0, shift_down);
                trn_view->setRect(trn_rect);
            }
        }
    }

    if (shift_middle != 0)
    {
        midle_rect.translate(0, -shift_middle); //reversed relative to others
        view_midle->setRect(midle_rect);
    }
}

void LLPanelEnvironmentInfo::onSldDayLengthChanged(F32 value)
{
    if (mCurrentEnvironment)
    {
        F32Hours daylength(value);

        mCurrentEnvironment->mDayLength = daylength;
        setDirtyFlag(DIRTY_FLAG_DAYLENGTH);

        udpateApparentTimeOfDay();
    }
}

void LLPanelEnvironmentInfo::onSldDayOffsetChanged(F32 value)
{
    if (mCurrentEnvironment)
    {
        F32Hours dayoffset(value);

        if (dayoffset.value() <= 0.0f)
            dayoffset += F32Hours(24.0);

        mCurrentEnvironment->mDayOffset = dayoffset;
        setDirtyFlag(DIRTY_FLAG_DAYOFFSET);

        udpateApparentTimeOfDay();
    }
}

void LLPanelEnvironmentInfo::onDayLenOffsetMouseUp()
{
    commitDayLenOffsetChanges(true);
}

void LLPanelEnvironmentInfo::commitDayLenOffsetChanges(bool need_callback)
{
    if (mCurrentEnvironment && (getDirtyFlag() & (DIRTY_FLAG_DAYLENGTH | DIRTY_FLAG_DAYOFFSET)))
    {
        clearDirtyFlag(DIRTY_FLAG_DAYOFFSET);
        clearDirtyFlag(DIRTY_FLAG_DAYLENGTH);

        LLHandle<LLPanel> that_h = getHandle();

        if (need_callback)
        {
            LLEnvironment::instance().updateParcel(getParcelId(),
                                                   LLSettingsDay::ptr_t(),
                                                   (S32)mCurrentEnvironment->mDayLength.value(),
                                                   (S32)mCurrentEnvironment->mDayOffset.value(),
                                                   LLEnvironment::altitudes_vect_t(),
                                                   [that_h](S32 parcel_id, LLEnvironment::EnvironmentInfo::ptr_t envifo) { onEnvironmentReceived(that_h, parcel_id, envifo); });
        }
        else
        {
            LLEnvironment::instance().updateParcel(getParcelId(),
                                                   LLSettingsDay::ptr_t(),
                                                   (S32)mCurrentEnvironment->mDayLength.value(),
                                                   (S32)mCurrentEnvironment->mDayOffset.value(),
                                                   LLEnvironment::altitudes_vect_t());
        }

    }
}

void LLPanelEnvironmentInfo::onAltSliderCallback(LLUICtrl *cntrl, const LLSD &data)
{
    LLMultiSliderCtrl *sld = (LLMultiSliderCtrl *)cntrl;
    std::string sld_name = sld->getCurSlider();

    if (sld_name.empty()) return;

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
        new_index = 2;
        while (iter2 != end)
        {
            if (iter->second.mAltitude > iter2->second.mAltitude)
            {
                new_index++;
            }
            iter2++;
        }
        iter->second.mTrackIndex = new_index;

        updateAltLabel(iter->second.mLabelIndex, iter->second.mTrackIndex, iter->second.mAltitude);
        iter++;
    }

    readjustAltLabels();
    setDirtyFlag(DIRTY_FLAG_ALTITUDES);
}

void LLPanelEnvironmentInfo::onAltSliderMouseUp()
{
    if (isRegion() && (getDirtyFlag() & DIRTY_FLAG_ALTITUDES))
    {
        clearDirtyFlag(DIRTY_FLAG_ALTITUDES);
        clearDirtyFlag(DIRTY_FLAG_DAYLENGTH);
        clearDirtyFlag(DIRTY_FLAG_DAYOFFSET);

        LLHandle<LLPanel> that_h = getHandle();
        LLEnvironment::altitudes_vect_t alts;

        for (auto alt : mAltitudes)
        {
            alts.push_back(alt.second.mAltitude);
        }
        setControlsEnabled(false);
        LLEnvironment::instance().updateParcel(getParcelId(),
                                               LLSettingsDay::ptr_t(),
                                               mCurrentEnvironment ? (S32)mCurrentEnvironment->mDayLength.value() : -1,
                                               mCurrentEnvironment ? (S32)mCurrentEnvironment->mDayOffset.value() : -1,
                                               alts);
    }
}

void LLPanelEnvironmentInfo::onBtnDefault()
{
    LLHandle<LLPanel> that_h = getHandle();
    S32 parcel_id = getParcelId();
    LLNotificationsUtil::add("SettingsConfirmReset", LLSD(), LLSD(),
        [that_h, parcel_id](const LLSD&notif, const LLSD&resp)
    {
        S32 opt = LLNotificationsUtil::getSelectedOption(notif, resp);
        if (opt == 0)
        {
            LLEnvironment::instance().resetParcel(parcel_id,
                [that_h](S32 parcel_id, LLEnvironment::EnvironmentInfo::ptr_t envifo) { onEnvironmentReceived(that_h, parcel_id, envifo); });
        }
    });
}

void LLPanelEnvironmentInfo::onBtnEdit()
{
    static const S32 FOURHOURS(4 * 60 * 60);

    LLFloaterEditExtDayCycle *dayeditor = getEditFloater();

    LLSD params(LLSDMap(LLFloaterEditExtDayCycle::KEY_EDIT_CONTEXT, isRegion() ? LLFloaterEditExtDayCycle::VALUE_CONTEXT_REGION : LLFloaterEditExtDayCycle::VALUE_CONTEXT_PARCEL)
            (LLFloaterEditExtDayCycle::KEY_DAY_LENGTH, mCurrentEnvironment ? (S32)(mCurrentEnvironment->mDayLength.value()) : FOURHOURS));

    dayeditor->openFloater(params);

    if (mCurrentEnvironment && mCurrentEnvironment->mDayCycle)
    {
        dayeditor->setEditDayCycle(mCurrentEnvironment->mDayCycle);
        dayeditor->setEditName(mCurrentEnvironment->mDayCycleName);
    }
    else
    {
        dayeditor->setEditDefaultDayCycle();
    }
}

void LLPanelEnvironmentInfo::onBtnSelect()
{
    if (LLFloaterSettingsPicker* picker = getSettingsPicker())
    {
        LLUUID item_id;
        if (mCurrentEnvironment && mCurrentEnvironment->mDayCycle)
        {
            LLUUID asset_id = mCurrentEnvironment->mDayCycle->getAssetId();
            item_id = LLFloaterSettingsPicker::findItemID(asset_id, false);
        }
        picker->setSettingsFilter(LLSettingsType::ST_NONE);
        picker->setSettingsItemId(item_id);
        picker->openFloater();
        picker->setFocus(true);
    }
}

void LLPanelEnvironmentInfo::onBtnRstAltitudes()
{
    if (isRegion())
    {
        LLHandle<LLPanel> that_h = getHandle();
        LLEnvironment::altitudes_vect_t alts;

        clearDirtyFlag(DIRTY_FLAG_ALTITUDES);
        clearDirtyFlag(DIRTY_FLAG_DAYLENGTH);
        clearDirtyFlag(DIRTY_FLAG_DAYOFFSET);

        for (S32 idx = 1; idx <= ALTITUDE_SLIDER_COUNT; ++idx)
        {
            F32 new_height = idx * ALTITUDE_DEFAULT_HEIGHT_STEP;
            alts.push_back(new_height);
        }

        LLEnvironment::instance().updateParcel(getParcelId(),
                                               LLSettingsDay::ptr_t(),
                                               mCurrentEnvironment ? (S32)mCurrentEnvironment->mDayLength.value() : -1,
                                               mCurrentEnvironment ? (S32)mCurrentEnvironment->mDayOffset.value() : -1,
                                               alts,
            [that_h](S32 parcel_id, LLEnvironment::EnvironmentInfo::ptr_t envifo) { onEnvironmentReceived(that_h, parcel_id, envifo); });
    }
}

void LLPanelEnvironmentInfo::udpateApparentTimeOfDay()
{
    static const F32 SECONDSINDAY(24.0 * 60.0 * 60.0);

    if ((!mCurrentEnvironment) || (mCurrentEnvironment->mDayLength.value() < 1.0) || (mCurrentEnvironment->mDayOffset.value() < 1.0))
    {
        mLabelApparentTime->setVisible(false);
        return;
    }
    mLabelApparentTime->setVisible(true);

    S32Seconds now((S32)LLDate::now().secondsSinceEpoch());

    now += mCurrentEnvironment->mDayOffset;

    F32 perc = (F32)(now.value() % mCurrentEnvironment->mDayLength.value()) / (F32)(mCurrentEnvironment->mDayLength.value());

    S32Seconds  secondofday((S32)(perc * SECONDSINDAY));
    S32Hours    hourofday(secondofday);
    S32Seconds  secondofhour(secondofday - hourofday);
    S32Minutes  minutesofhour(secondofhour);
    static bool use_24h = gSavedSettings.getBOOL("Use24HourClock");
    bool        am_pm(hourofday.value() >= 12);

    if (!use_24h)
    {
        if (hourofday.value() < 1)
            hourofday = S32Hours(12);
        if (hourofday.value() > 12)
            hourofday -= S32Hours(12);
    }

    std::string lblminute(((minutesofhour.value() < 10) ? "0" : "") + LLSD(minutesofhour.value()).asString());

    mLabelApparentTime->setTextArg("[HH]", LLSD(hourofday.value()).asString());
    mLabelApparentTime->setTextArg("[MM]", lblminute);
    if (use_24h)
    {
        mLabelApparentTime->setTextArg("[AP]", std::string());
    }
    else
    {
        mLabelApparentTime->setTextArg("[AP]", std::string(am_pm ? "PM" : "AM"));
    }
    mLabelApparentTime->setTextArg("[PRC]", LLSD((S32)(100 * perc)).asString());

}

void LLPanelEnvironmentInfo::onIdlePlay(void *data)
{
    ((LLPanelEnvironmentInfo *)data)->udpateApparentTimeOfDay();
}


void LLPanelEnvironmentInfo::onPickerCommitted(LLUUID item_id, std::string source)
{
    if (source == alt_prefixes[4])
    {
        onPickerCommitted(item_id, 0);
    }
    else if (source == alt_prefixes[3])
    {
        onPickerCommitted(item_id, 1);
    }
    else
    {
        onPickerCommitted(item_id, mAltitudes[source].mTrackIndex);
    }
}

void LLPanelEnvironmentInfo::onPickerCommitted(LLUUID item_id, S32 track_num)
{
    if (LLInventoryItem* itemp = gInventory.getItem(item_id))
    {
        LL_INFOS("ENVPANEL") << "item '" << item_id << "' : '" << itemp->getDescription() << "'" << LL_ENDL;

        LLHandle<LLPanel> that_h = getHandle();
        clearDirtyFlag(DIRTY_FLAG_DAYLENGTH);
        clearDirtyFlag(DIRTY_FLAG_DAYOFFSET);

        U32 flags(0);

        if (!itemp->getPermissions().allowOperationBy(PERM_MODIFY, gAgent.getID()))
        {
            flags |= LLSettingsBase::FLAG_NOMOD;
        }

        if (!itemp->getPermissions().allowOperationBy(PERM_TRANSFER, gAgent.getID()))
        {
            flags |= LLSettingsBase::FLAG_NOTRANS;
        }

        LLEnvironment::instance().updateParcel
        (
            getParcelId(),
            itemp->getAssetUUID(),
            itemp->getName(),
            track_num,
            mCurrentEnvironment ? (S32)mCurrentEnvironment->mDayLength.value() : -1,
            mCurrentEnvironment ? (S32)mCurrentEnvironment->mDayOffset.value() : -1,
            flags,
            LLEnvironment::altitudes_vect_t(),
            [that_h](S32 parcel_id, LLEnvironment::EnvironmentInfo::ptr_t envifo)
            {
                onEnvironmentReceived(that_h, parcel_id, envifo);
            }
        );
    }
}

void LLPanelEnvironmentInfo::onEditCommitted(LLSettingsDay::ptr_t newday)
{
    LLEnvironment::instance().clearEnvironment(LLEnvironment::ENV_EDIT);
    LLEnvironment::instance().updateEnvironment();

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
        LLHandle<LLPanel> that_h = getHandle();
        clearDirtyFlag(DIRTY_FLAG_DAYLENGTH);
        clearDirtyFlag(DIRTY_FLAG_DAYOFFSET);

        LLEnvironment::instance().updateParcel(getParcelId(),
                                               newday,
                                               mCurrentEnvironment ? (S32)mCurrentEnvironment->mDayLength.value() : -1,
                                               mCurrentEnvironment ? (S32)mCurrentEnvironment->mDayOffset.value() : -1,
                                               LLEnvironment::altitudes_vect_t(),
            [that_h](S32 parcel_id, LLEnvironment::EnvironmentInfo::ptr_t envifo) { onEnvironmentReceived(that_h, parcel_id, envifo); });
    }
}

void LLPanelEnvironmentInfo::onEnvironmentChanged(LLEnvironment::EnvSelection_t env, S32 new_version)
{
    if (new_version < INVALID_PARCEL_ENVIRONMENT_VERSION)
    {
        // cleanups and local changes, we are only interested in changes sent by server
        return;
    }

    LL_DEBUGS("ENVPANEL") << "Received environment update " << mCurEnvVersion << " " << new_version << LL_ENDL;

    // Environment comes from different sources, from environment update callbacks,
    // from hovers (causes callbacks on version change) and from personal requests
    // filter out duplicates and out of order packets by checking parcel environment version.

    if (isRegion())
    {
        // Note: region uses same init versions as parcel
        if (env == LLEnvironment::ENV_REGION
            // version should be always growing, UNSET_PARCEL_ENVIRONMENT_VERSION is backup case
            && (mCurEnvVersion < new_version || mCurEnvVersion <= UNSET_PARCEL_ENVIRONMENT_VERSION))
        {
            if (new_version >= UNSET_PARCEL_ENVIRONMENT_VERSION)
            {
                // 'pending state' to prevent re-request on following onEnvironmentChanged if there will be any
                mCurEnvVersion = new_version;
            }
            mCurrentEnvironment.reset();
            refreshFromSource();
        }
    }
    else if ((env == LLEnvironment::ENV_PARCEL)
             && (getParcelId() == LLViewerParcelMgr::instance().getAgentParcelId()))
    {
        if (LLParcel* parcel = getParcel())
        {
            // first for parcel own settings, second is for case when parcel uses region settings
            if (mCurEnvVersion < new_version
                || (mCurEnvVersion != new_version && new_version == UNSET_PARCEL_ENVIRONMENT_VERSION))
            {
                // 'pending state' to prevent re-request on following onEnvironmentChanged if there will be any
                mCurEnvVersion = new_version;
                mCurrentEnvironment.reset();

                refreshFromSource();
            }
            else if (mCurrentEnvironment)
            {
                // update controls
                refresh();
            }
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
    if (mCurrentEnvironment->mEnvVersion > INVALID_PARCEL_ENVIRONMENT_VERSION)
    {
        // Server provided version, use it
        mCurEnvVersion = mCurrentEnvironment->mEnvVersion;
        LL_DEBUGS("ENVPANEL") << " Setting environment version: " << mCurEnvVersion << " for parcel id: " << parcel_id << LL_ENDL;
    }
    // Backup: Version was not provided for some reason
    else
    {
        LL_WARNS("ENVPANEL") << " Environment version was not provided for " << parcel_id << ", old env version: " << mCurEnvVersion << LL_ENDL;
    }

    refreshFromEstate();
    refresh();

    // todo: we have envifo and parcel env version, should we just setEnvironment() and parcel's property to prevent dupplicate requests?
}

// static
void LLPanelEnvironmentInfo::onEnvironmentReceived(LLHandle<LLPanel> that_h, S32 parcel_id, LLEnvironment::EnvironmentInfo::ptr_t envifo)
{
    if (LLPanelEnvironmentInfo* that = (LLPanelEnvironmentInfo*)that_h.get())
    {
        that->onEnvironmentReceived(parcel_id, envifo);
    }
}

LLSettingsDropTarget::LLSettingsDropTarget(const LLSettingsDropTarget::Params& p)
    : LLView(p)
    , mEnvironmentInfoPanel(NULL)
    , mDndEnabled(false)
{
}

bool LLSettingsDropTarget::handleDragAndDrop(S32 x, S32 y, MASK mask, bool drop,
    EDragAndDropType cargo_type,
    void* cargo_data,
    EAcceptance* accept,
    std::string& tooltip_msg)
{
    bool handled = false;

    if (getParent() && mDndEnabled)
    {
        handled = true;

        switch (cargo_type)
        {
        case DAD_SETTINGS:
            if (cargo_data && mEnvironmentInfoPanel)
            {
                LLUUID item_id = ((LLViewerInventoryItem*)cargo_data)->getUUID();
                if (gInventory.getItem(item_id))
                {
                    *accept = ACCEPT_YES_COPY_SINGLE;
                    if (drop)
                    {
                        // might be better to use name of the element
                        mEnvironmentInfoPanel->onPickerCommitted(item_id, mTrack);
                    }
                }
            }
            else
            {
                *accept = ACCEPT_NO;
            }
            break;
        default:
            *accept = ACCEPT_NO;
            break;
        }
    }

    return handled;
}
