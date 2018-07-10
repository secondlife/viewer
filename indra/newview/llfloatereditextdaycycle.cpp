/** 
 * @file llfloatereditextdaycycle.cpp
 * @brief Floater to create or edit a day cycle
 *
 * $LicenseInfo:firstyear=2011&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2011, Linden Research, Inc.
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

#include "llfloatereditextdaycycle.h"

// libs
#include "llbutton.h"
#include "llcallbacklist.h"
#include "llcheckboxctrl.h"
#include "llcombobox.h"
#include "llloadingindicator.h"
#include "llmultisliderctrl.h"
#include "llnotifications.h"
#include "llnotificationsutil.h"
#include "llspinctrl.h"
#include "lltimectrl.h"
#include "lltabcontainer.h"
#include "llfilepicker.h"

#include "llsettingsvo.h"
#include "llinventorymodel.h"
#include "llviewerparcelmgr.h"

#include "llsettingspicker.h"

// newview
#include "llagent.h"
#include "llparcel.h"
#include "llflyoutcombobtn.h" //Todo: make a proper UI element/button/panel instead
#include "llregioninfomodel.h"
#include "llviewerregion.h"
#include "llpaneleditwater.h"
#include "llpaneleditsky.h"

#include "llui.h"

#include "llenvironment.h"
#include "lltrans.h"

//=========================================================================
namespace {
    const std::string track_tabs[] = {
        "water_track",
        "sky1_track",
        "sky2_track",
        "sky3_track",
        "sky4_track",
    };

    const std::string ICN_LOCK_EDIT("icn_lock_edit");
    const std::string BTN_SAVE("save_btn");
    const std::string BTN_FLYOUT("btn_flyout");
    const std::string BTN_CANCEL("cancel_btn");
    const std::string BTN_ADDFRAME("add_frame");
    const std::string BTN_DELFRAME("delete_frame");
    const std::string BTN_IMPORT("btn_import");
    const std::string BTN_LOADFRAME("btn_load_frame");
    const std::string SLDR_TIME("WLTimeSlider");
    const std::string SLDR_KEYFRAMES("WLDayCycleFrames");
    const std::string VIEW_SKY_SETTINGS("frame_settings_sky");
    const std::string VIEW_WATER_SETTINGS("frame_settings_water");
    const std::string LBL_CURRENT_TIME("current_time");
    const std::string TXT_DAY_NAME("day_cycle_name");
    const std::string TABS_SKYS("sky_tabs");
    const std::string TABS_WATER("water_tabs");

    const std::string EVNT_DAYTRACK("DayCycle.Track");
    const std::string EVNT_PLAY("DayCycle.PlayActions");

    const std::string ACTION_PLAY("play");
    const std::string ACTION_PAUSE("pause");
    const std::string ACTION_FORWARD("forward");
    const std::string ACTION_BACK("back");

    // For flyout
    const std::string XML_FLYOUTMENU_FILE("menu_save_settings.xml");
    // From menu_save_settings.xml, consider moving into flyout since it should be supported by flyout either way
    const std::string ACTION_SAVE("save_settings");
    const std::string ACTION_SAVEAS("save_as_new_settings");
    const std::string ACTION_APPLY_LOCAL("apply_local");
    const std::string ACTION_APPLY_PARCEL("apply_parcel");
    const std::string ACTION_APPLY_REGION("apply_region");

    const F32 DAY_CYCLE_PLAY_TIME_SECONDS = 60;

    const F32 FRAME_SLOP_FACTOR = 0.025f;
}

//=========================================================================
const std::string LLFloaterEditExtDayCycle::KEY_INVENTORY_ID("inventory_id");
const std::string LLFloaterEditExtDayCycle::KEY_LIVE_ENVIRONMENT("live_environment");
const std::string LLFloaterEditExtDayCycle::KEY_DAY_LENGTH("day_length");

//=========================================================================
LLFloaterEditExtDayCycle::LLFloaterEditExtDayCycle(const LLSD &key) :
    LLFloater(key),
    mFlyoutControl(nullptr),
    mDayLength(0),
    mCurrentTrack(1),
    mTimeSlider(nullptr),
    mFramesSlider(nullptr),
    mCurrentTimeLabel(nullptr),
    mImportButton(nullptr),
    mInventoryId(),
    mInventoryItem(nullptr),
    mLoadFrame(nullptr),
    mSkyBlender(),
    mWaterBlender(),
    mScratchSky(),
    mScratchWater(),
    mIsPlaying(false)
{

    mCommitCallbackRegistrar.add(EVNT_DAYTRACK, [this](LLUICtrl *ctrl, const LLSD &data) { onTrackSelectionCallback(data); });
    mCommitCallbackRegistrar.add(EVNT_PLAY, [this](LLUICtrl *ctrl, const LLSD &data) { onPlayActionCallback(data); });

    mScratchSky = LLSettingsVOSky::buildDefaultSky();
    mScratchWater = LLSettingsVOWater::buildDefaultWater();
}

LLFloaterEditExtDayCycle::~LLFloaterEditExtDayCycle()
{
    // Todo: consider remaking mFlyoutControl into full view class that initializes intself with floater,
    // complete with postbuild, e t c...
    delete mFlyoutControl;
}

// virtual
BOOL LLFloaterEditExtDayCycle::postBuild()
{
    getChild<LLLineEditor>(TXT_DAY_NAME)->setKeystrokeCallback(boost::bind(&LLFloaterEditExtDayCycle::onCommitName, this, _1, _2), NULL);

    mAddFrameButton = getChild<LLButton>(BTN_ADDFRAME, true);
    mDeleteFrameButton = getChild<LLButton>(BTN_DELFRAME, true);
    mTimeSlider = getChild<LLMultiSliderCtrl>(SLDR_TIME);
    mFramesSlider = getChild<LLMultiSliderCtrl>(SLDR_KEYFRAMES);
    mSkyTabLayoutContainer = getChild<LLView>(VIEW_SKY_SETTINGS, true);
    mWaterTabLayoutContainer = getChild<LLView>(VIEW_WATER_SETTINGS, true);
    mCurrentTimeLabel = getChild<LLTextBox>(LBL_CURRENT_TIME, true);
    mImportButton = getChild<LLButton>(BTN_IMPORT, true);
    mLoadFrame = getChild<LLButton>(BTN_LOADFRAME, true);

    mFlyoutControl = new LLFlyoutComboBtnCtrl(this, BTN_SAVE, BTN_FLYOUT, XML_FLYOUTMENU_FILE);
    mFlyoutControl->setAction([this](LLUICtrl *ctrl, const LLSD &data) { onButtonApply(ctrl, data); });

    getChild<LLButton>(BTN_CANCEL, true)->setCommitCallback([this](LLUICtrl *ctrl, const LLSD &data) { onBtnCancel(); });
    mTimeSlider->setCommitCallback([this](LLUICtrl *ctrl, const LLSD &data) { onTimeSliderMoved(); });
    mAddFrameButton->setCommitCallback([this](LLUICtrl *ctrl, const LLSD &data) { onAddTrack(); });
    mDeleteFrameButton->setCommitCallback([this](LLUICtrl *ctrl, const LLSD &data) { onRemoveTrack(); });
    mImportButton->setCommitCallback([this](LLUICtrl *, const LLSD &){ onButtonImport(); });
    mLoadFrame->setCommitCallback([this](LLUICtrl *, const LLSD &){ onButtonLoadFrame(); });

    mFramesSlider->setCommitCallback([this](LLUICtrl *, const LLSD &data) { onFrameSliderCallback(data); });
    mFramesSlider->setDoubleClickCallback([this](LLUICtrl*, S32 x, S32 y, MASK mask){ onFrameSliderDoubleClick(x, y, mask); });
    mFramesSlider->setMouseDownCallback([this](LLUICtrl*, S32 x, S32 y, MASK mask){ onFrameSliderMouseDown(x, y, mask); });
    mFramesSlider->setMouseUpCallback([this](LLUICtrl*, S32 x, S32 y, MASK mask){ onFrameSliderMouseUp(x, y, mask); });

    mTimeSlider->addSlider(0);

    //getChild<LLButton>("sky1_track", true)->setToggleState(true);

	return TRUE;
}

void LLFloaterEditExtDayCycle::onOpen(const LLSD& key)
{
    mEditDay.reset();
    if (key.has(KEY_INVENTORY_ID))
    {
        loadInventoryItem(key[KEY_INVENTORY_ID].asUUID());
    }
    else if (key.has(KEY_LIVE_ENVIRONMENT))
    {
        LLEnvironment::EnvSelection_t env = static_cast<LLEnvironment::EnvSelection_t>(key[KEY_LIVE_ENVIRONMENT].asInteger());

        loadLiveEnvironment(env);
    }
    else
    {
        loadLiveEnvironment(LLEnvironment::ENV_DEFAULT);
    }

    mDayLength.value(0);
    if (key.has(KEY_DAY_LENGTH))
    {
        mDayLength.value(key[KEY_DAY_LENGTH].asReal());
    }

    // time labels
    mCurrentTimeLabel->setTextArg("[PRCNT]", std::string("0"));
    const S32 max_elm = 5;
    if (mDayLength.value() != 0)
    {
        S32Hours hrs;
        S32Minutes minutes;
        LLSettingsDay::Seconds total;
        LLUIString formatted_label = getString("time_label");
        for (int i = 0; i < max_elm; i++)
        {
            total = ((mDayLength / (max_elm - 1)) * i); 
            hrs = total;
            minutes = total - hrs;

            formatted_label.setArg("[HH]", llformat("%d", hrs.value()));
            formatted_label.setArg("[MM]", llformat("%d", abs(minutes.value())));
            getChild<LLTextBox>("p" + llformat("%d", i), true)->setTextArg("[DSC]", formatted_label.getString());
        }
        hrs = mDayLength;
        minutes = mDayLength - hrs;
        formatted_label.setArg("[HH]", llformat("%d", hrs.value()));
        formatted_label.setArg("[MM]", llformat("%d", abs(minutes.value())));
        mCurrentTimeLabel->setTextArg("[DSC]", formatted_label.getString());
    }
    else
    {
        for (int i = 0; i < max_elm; i++)
        {
            getChild<LLTextBox>("p" + llformat("%d", i), true)->setTextArg("[DSC]", std::string());
        }
        mCurrentTimeLabel->setTextArg("[DSC]", std::string());
    }

    const LLEnvironment::altitude_list_t &altitudes = LLEnvironment::instance().getRegionAltitudes();

    for (S32 idx = 1; idx < 4; ++idx)
    {
        std::stringstream label;
        label << altitudes[idx] << "m";
        getChild<LLButton>(track_tabs[idx + 1], true)->setTextArg("[DSC]", label.str());
    }

}

void LLFloaterEditExtDayCycle::onClose(bool app_quitting)
{
    doCloseInventoryFloater(app_quitting);
    // there's no point to change environment if we're quitting
    // or if we already restored environment
    if (!app_quitting && LLEnvironment::instance().getSelectedEnvironment() == LLEnvironment::ENV_EDIT)
    {
        LLEnvironment::instance().setSelectedEnvironment(LLEnvironment::ENV_LOCAL);
        LLEnvironment::instance().clearEnvironment(LLEnvironment::ENV_EDIT);
    }
    stopPlay();
}

void LLFloaterEditExtDayCycle::onFocusReceived()
{
    updateEditEnvironment();
    LLEnvironment::instance().setSelectedEnvironment(LLEnvironment::ENV_EDIT, LLEnvironment::TRANSITION_FAST);
}

void LLFloaterEditExtDayCycle::onFocusLost()
{
}


void LLFloaterEditExtDayCycle::onVisibilityChange(BOOL new_visibility)
{
}

void LLFloaterEditExtDayCycle::refresh()
{
    if (mEditDay)
    {
        LLLineEditor* name_field = getChild<LLLineEditor>(TXT_DAY_NAME);
        name_field->setText(mEditDay->getName());
    }

    bool is_inventory_avail = canUseInventory();

    mFlyoutControl->setMenuItemEnabled(ACTION_SAVE, is_inventory_avail);
    mFlyoutControl->setMenuItemEnabled(ACTION_SAVEAS, is_inventory_avail);
    mFlyoutControl->setMenuItemEnabled(ACTION_APPLY_PARCEL, canApplyParcel());
    mFlyoutControl->setMenuItemEnabled(ACTION_APPLY_REGION, canApplyRegion());

    LLFloater::refresh();
}


void LLFloaterEditExtDayCycle::onButtonApply(LLUICtrl *ctrl, const LLSD &data)
{
    std::string ctrl_action = ctrl->getName();

    if (ctrl_action == ACTION_SAVE)
    {
        doApplyUpdateInventory();
    }
    else if (ctrl_action == ACTION_SAVEAS)
    {
        doApplyCreateNewInventory();
    }
    else if ((ctrl_action == ACTION_APPLY_LOCAL) ||
        (ctrl_action == ACTION_APPLY_PARCEL) ||
        (ctrl_action == ACTION_APPLY_REGION))
    {
        doApplyEnvironment(ctrl_action);
    }
    else
    {
        LL_WARNS("ENVIRONMENT") << "Unknown settings action '" << ctrl_action << "'" << LL_ENDL;
    }
}

void LLFloaterEditExtDayCycle::onBtnCancel()
{
    closeFloater(); // will restore env
}

void LLFloaterEditExtDayCycle::onButtonImport()
{
    doImportFromDisk();
}

void LLFloaterEditExtDayCycle::onButtonLoadFrame()
{
    LLUUID curassetId;

    if (mCurrentEdit)
    { 
        curassetId = mCurrentEdit->getAssetId();
    }
    
    doOpenInventoryFloater((mCurrentTrack == LLSettingsDay::TRACK_WATER) ? LLSettingsType::ST_WATER : LLSettingsType::ST_SKY, curassetId);
}

void LLFloaterEditExtDayCycle::onAddTrack()
{
    // todo: 2.5% safety zone
    std::string sldr_key = mFramesSlider->getCurSlider();
    LLSettingsBase::Seconds frame(mTimeSlider->getCurSliderValue());
    LLSettingsBase::ptr_t setting;
    if ((mEditDay->getSettingsNearKeyframe(frame, mCurrentTrack, FRAME_SLOP_FACTOR)).second)
    {
        LL_WARNS("ENVIRONMENT") << "Attempt to add new frame too close to existing frame." << LL_ENDL;
        return;
    }

    if (mCurrentTrack == LLSettingsDay::TRACK_WATER)
    {
        // scratch water should always have the current water settings.
        LLSettingsWater::ptr_t water(mScratchWater->buildClone());
        setting = water;
        mEditDay->setWaterAtKeyframe( std::static_pointer_cast<LLSettingsWater>(setting), frame);
    }
    else
    {
        // scratch sky should always have the current sky settings.
        LLSettingsSky::ptr_t sky(mScratchSky->buildClone());
        setting = sky;
        mEditDay->setSkyAtKeyframe(sky, frame, mCurrentTrack);
    }

    addSliderFrame(frame, setting);
    reblendSettings();
    updateTabs();
}

void LLFloaterEditExtDayCycle::onRemoveTrack()
{
    std::string sldr_key = mFramesSlider->getCurSlider();
    if (!sldr_key.empty())
    {
        return;
    }
    removeCurrentSliderFrame();
    updateButtons();
}

void LLFloaterEditExtDayCycle::onCommitName(class LLLineEditor* caller, void* user_data)
{
    mEditDay->setName(caller->getText());
}

void LLFloaterEditExtDayCycle::onTrackSelectionCallback(const LLSD& user_data)
{
    U32 track_index = user_data.asInteger(); // 1-5
    selectTrack(track_index);
}

void LLFloaterEditExtDayCycle::onPlayActionCallback(const LLSD& user_data)
{
    std::string action = user_data.asString();

    F32 frame = mTimeSlider->getCurSliderValue();

    if (action == ACTION_PLAY)
    {
        startPlay();
    }
    else if (action == ACTION_PAUSE)
    {
        stopPlay();
    }
    else if (mSliderKeyMap.size() != 0)
    {
        F32 new_frame = 0;
        if (action == ACTION_FORWARD)
        {
            new_frame = mEditDay->getUpperBoundFrame(mCurrentTrack, frame + (mTimeSlider->getIncrement() / 2));
        }
        else if (action == ACTION_BACK)
        {
            new_frame = mEditDay->getLowerBoundFrame(mCurrentTrack, frame - (mTimeSlider->getIncrement() / 2));
        }
        selectFrame(new_frame, 0.0f);
        stopPlay();
    }
}

void LLFloaterEditExtDayCycle::onFrameSliderCallback(const LLSD &data)
{
    //LL_WARNS("LAPRAS") << "LLFloaterEditExtDayCycle::onFrameSliderCallback(" << data << ")" << LL_ENDL;

    std::string curslider = mFramesSlider->getCurSlider();

    LL_WARNS("LAPRAS") << "Current slider set to \"" << curslider << "\"" << LL_ENDL;
    F32 sliderpos(0.0);


    if (curslider.empty())
    {
        S32 x(0), y(0);
        LLUI::getMousePositionLocal(mFramesSlider, &x, &y);

        sliderpos = mFramesSlider->getSliderValueFromX(x);
    }
    else
    {
        sliderpos = mFramesSlider->getCurSliderValue();

        keymap_t::iterator it = mSliderKeyMap.find(curslider);
        if (it != mSliderKeyMap.end())
        {
            //         if (gKeyboard->currentMask(TRUE) == MASK_SHIFT)
            //         {
            //             LL_DEBUGS() << "Copying frame from " << iter->second.mFrame << " to " << new_frame << LL_ENDL;
            //             LLSettingsBase::ptr_t new_settings;
            // 
            //             // mEditDay still remembers old position, add copy at new position
            //             if (mCurrentTrack == LLSettingsDay::TRACK_WATER)
            //             {
            //                 LLSettingsWaterPtr_t water_ptr = std::dynamic_pointer_cast<LLSettingsWater>(iter->second.pSettings)->buildClone();
            //                 mEditDay->setWaterAtKeyframe(water_ptr, new_frame);
            //                 new_settings = water_ptr;
            //             }
            //             else
            //             {
            //                 LLSettingsSkyPtr_t sky_ptr = std::dynamic_pointer_cast<LLSettingsSky>(iter->second.pSettings)->buildClone();
            //                 mEditDay->setSkyAtKeyframe(sky_ptr, new_frame, mCurrentTrack);
            //                 new_settings = sky_ptr;
            //             }
            // 
            //             // mSliderKeyMap still remembers old position, for simplicity, just move it to be identical to slider
            //             F32 old_frame = iter->second.mFrame;
            //             iter->second.mFrame = new_frame;
            //             // slider already moved old frame, create new one in old place
            //             addSliderFrame(old_frame, new_settings, false /*because we are going to reselect new one*/);
            //             // reselect new frame
            //             mFramesSlider->setCurSlider(iter->first);
            //         }
            //         else
            //        {
            LL_WARNS("LAPRAS") << "Moving frame from " << (*it).second.mFrame << " to " << sliderpos << LL_ENDL;
            if (mEditDay->moveTrackKeyframe(mCurrentTrack, (*it).second.mFrame, sliderpos))
            {
                (*it).second.mFrame = sliderpos;
            }
            else 
            {
                mFramesSlider->setCurSliderValue((*it).second.mFrame);
            }
        }

    }

    mTimeSlider->setCurSliderValue(sliderpos);

    updateTabs();
    LLEnvironment::instance().updateEnvironment();
}

void LLFloaterEditExtDayCycle::onFrameSliderDoubleClick(S32 x, S32 y, MASK mask)
{
    onAddTrack();
}

void LLFloaterEditExtDayCycle::onFrameSliderMouseDown(S32 x, S32 y, MASK mask)
{
    stopPlay();
    F32 sliderpos = mFramesSlider->getSliderValueFromX(x);

    std::string slidername = mFramesSlider->getCurSlider();

    if (!slidername.empty())
    {
        F32 sliderval = mFramesSlider->getSliderValue(slidername);

        LL_WARNS("LAPRAS") << "Selected vs mouse delta = " << (sliderval - sliderpos) << LL_ENDL;

        if (fabs(sliderval - sliderpos) > FRAME_SLOP_FACTOR)
        {
            mFramesSlider->resetCurSlider();
        }
    }
    LL_WARNS("LAPRAS") << "DOWN: X=" << x << "  Y=" << y << " MASK=" << mask << " Position=" << sliderpos << LL_ENDL;
}

void LLFloaterEditExtDayCycle::onFrameSliderMouseUp(S32 x, S32 y, MASK mask)
{
    F32 sliderpos = mFramesSlider->getSliderValueFromX(x);

    LL_WARNS("LAPRAS") << "  UP: X=" << x << "  Y=" << y << " MASK=" << mask << " Position=" << sliderpos << LL_ENDL;
    mTimeSlider->setCurSliderValue(sliderpos);
    selectFrame(sliderpos, FRAME_SLOP_FACTOR);
}

void LLFloaterEditExtDayCycle::onTimeSliderMoved()
{
    selectFrame(mTimeSlider->getCurSliderValue(), FRAME_SLOP_FACTOR);
}

void LLFloaterEditExtDayCycle::selectTrack(U32 track_index, bool force )
{
    if (track_index < LLSettingsDay::TRACK_MAX)
        mCurrentTrack = track_index;

    LLButton* button = getChild<LLButton>(track_tabs[mCurrentTrack], true);
    if (button->getToggleState() && !force)
    {
        return;
    }

    for (int i = 0; i < LLSettingsDay::TRACK_MAX; i++) // use max value
    {
        getChild<LLButton>(track_tabs[i], true)->setToggleState(i == mCurrentTrack);
    }

    bool show_water = (mCurrentTrack == LLSettingsDay::TRACK_WATER);
    mSkyTabLayoutContainer->setVisible(!show_water);
    mWaterTabLayoutContainer->setVisible(show_water);
    updateSlider();
}

void LLFloaterEditExtDayCycle::selectFrame(F32 frame, F32 slop_factor)
{
    mFramesSlider->resetCurSlider();

    keymap_t::iterator iter = mSliderKeyMap.begin();
    keymap_t::iterator end_iter = mSliderKeyMap.end();
    while (iter != end_iter)
    {
        F32 keyframe = iter->second.mFrame;
        if (fabs(keyframe - frame) <= slop_factor)
        {
            mFramesSlider->setCurSlider(iter->first);
            frame = iter->second.mFrame;  
            break;
        }
        iter++;
    }

    mTimeSlider->setCurSliderValue(frame);
    // block or update tabs according to new selection
    updateTabs();
//  LLEnvironment::instance().updateEnvironment();
}

void LLFloaterEditExtDayCycle::clearTabs()
{
    // Note: If this doesn't look good, init panels with default settings. It might be better looking
    if (mCurrentTrack == LLSettingsDay::TRACK_WATER)
    {
        const LLSettingsWaterPtr_t p_water = LLSettingsWaterPtr_t(NULL);
        updateWaterTabs(p_water);
    }
    else
    {
        const LLSettingsSkyPtr_t p_sky = LLSettingsSkyPtr_t(NULL);
        updateSkyTabs(p_sky);
    }
    updateButtons();
    updateTimeAndLabel();
}

void LLFloaterEditExtDayCycle::updateTabs()
{
    reblendSettings();
    synchronizeTabs();

    updateButtons();
    updateTimeAndLabel();
}

void LLFloaterEditExtDayCycle::updateWaterTabs(const LLSettingsWaterPtr_t &p_water)
{
    LLView* tab_container = mWaterTabLayoutContainer->getChild<LLView>(TABS_WATER); //can't extract panels directly, since it is in 'tuple'
    LLPanelSettingsWaterMainTab* panel = dynamic_cast<LLPanelSettingsWaterMainTab*>(tab_container->getChildView("water_panel"));
    if (panel)
    {
        panel->setWater(p_water);
    }
}

void LLFloaterEditExtDayCycle::updateSkyTabs(const LLSettingsSkyPtr_t &p_sky)
{
    LLView* tab_container = mSkyTabLayoutContainer->getChild<LLView>(TABS_SKYS); //can't extract panels directly, since they are in 'tuple'

    LLPanelSettingsSky* panel;
    panel = dynamic_cast<LLPanelSettingsSky*>(tab_container->getChildView("atmosphere_panel"));
    if (panel)
    {
        panel->setSky(p_sky);
    }
    panel = dynamic_cast<LLPanelSettingsSky*>(tab_container->getChildView("clouds_panel"));
    if (panel)
    {
        panel->setSky(p_sky);
    }
    panel = dynamic_cast<LLPanelSettingsSky*>(tab_container->getChildView("moon_panel"));
    if (panel)
    {
        panel->setSky(p_sky);
    }
}

void LLFloaterEditExtDayCycle::setWaterTabsEnabled(BOOL enable)
{
    LLView* tab_container = mWaterTabLayoutContainer->getChild<LLView>(TABS_WATER); //can't extract panels directly, since it is in 'tuple'
    LLPanelSettingsWaterMainTab* panel = dynamic_cast<LLPanelSettingsWaterMainTab*>(tab_container->getChildView("water_panel"));
    if (panel)
    {
        panel->setEnabled(enable);
        panel->setAllChildrenEnabled(enable);
    }
}

void LLFloaterEditExtDayCycle::setSkyTabsEnabled(BOOL enable)
{
    LLView* tab_container = mSkyTabLayoutContainer->getChild<LLView>(TABS_SKYS); //can't extract panels directly, since they are in 'tuple'

    LLPanelSettingsSky* panel;
    panel = dynamic_cast<LLPanelSettingsSky*>(tab_container->getChildView("atmosphere_panel"));
    if (panel)
    {
        panel->setEnabled(enable);
        panel->setAllChildrenEnabled(enable);
    }
    panel = dynamic_cast<LLPanelSettingsSky*>(tab_container->getChildView("clouds_panel"));
    if (panel)
    {
        panel->setEnabled(enable);
        panel->setAllChildrenEnabled(enable);
    }
    panel = dynamic_cast<LLPanelSettingsSky*>(tab_container->getChildView("moon_panel"));
    if (panel)
    {
        panel->setEnabled(enable);
        panel->setAllChildrenEnabled(enable);
    }
}

void LLFloaterEditExtDayCycle::updateButtons()
{
    // This logic appears to work in reverse, the add frame button
    // is only enabled when you're on an existing frame and disabled
    // in all the interim positions where you'd want to add a frame...
    //LLSettingsBase::Seconds frame(mTimeSlider->getCurSliderValue());
    //LLSettingsBase::ptr_t settings = mEditDay->getSettingsAtKeyframe(frame, mCurrentTrack);
    //bool can_add = static_cast<bool>(settings);
    //mAddFrameButton->setEnabled(can_add);
    //mDeleteFrameButton->setEnabled(!can_add);
    mAddFrameButton->setEnabled(true);
    mDeleteFrameButton->setEnabled(true);
}

void LLFloaterEditExtDayCycle::updateSlider()
{
    F32 frame_position = mTimeSlider->getCurSliderValue();
    mFramesSlider->clear();
    mSliderKeyMap.clear();

    LLSettingsDay::CycleTrack_t track = mEditDay->getCycleTrack(mCurrentTrack);
    for (auto &track_frame : track)
    {
        addSliderFrame(track_frame.first, track_frame.second, false);
    }

    if (mSliderKeyMap.size() > 0)
    {
        // update positions
        mLastFrameSlider = mFramesSlider->getCurSlider();
        updateTabs();
    }
    else
    {
        // disable panels
        clearTabs();
        mLastFrameSlider.clear();
    }

    selectFrame(frame_position, FRAME_SLOP_FACTOR);
}

void LLFloaterEditExtDayCycle::updateTimeAndLabel()
{
    F32 time = mTimeSlider->getCurSliderValue();
    mCurrentTimeLabel->setTextArg("[PRCNT]", llformat("%.0f", time * 100));
    if (mDayLength.value() != 0)
    {
        LLUIString formatted_label = getString("time_label");

        LLSettingsDay::Seconds total = (mDayLength  * time); 
        S32Hours hrs = total;
        S32Minutes minutes = total - hrs;

        formatted_label.setArg("[HH]", llformat("%d", hrs.value()));
        formatted_label.setArg("[MM]", llformat("%d", abs(minutes.value())));
        mCurrentTimeLabel->setTextArg("[DSC]", formatted_label.getString());
    }
    else
    {
        mCurrentTimeLabel->setTextArg("[DSC]", std::string());
    }

    // Update blender here:
}

void LLFloaterEditExtDayCycle::addSliderFrame(const F32 frame, LLSettingsBase::ptr_t &setting, bool update_ui)
{
    // multi slider distinguishes elements by key/name in string format
    // store names to map to be able to recall dependencies
    std::string new_slider = mFramesSlider->addSlider(frame);
    mSliderKeyMap[new_slider] = FrameData(frame, setting);

    if (update_ui)
    {
        mLastFrameSlider = new_slider;
        mTimeSlider->setCurSliderValue(frame);
        updateTabs();
    }
}

void LLFloaterEditExtDayCycle::removeCurrentSliderFrame()
{
    std::string sldr = mFramesSlider->getCurSlider();
    if (sldr.empty())
    {
        return;
    }
    mFramesSlider->deleteCurSlider();
    keymap_t::iterator iter = mSliderKeyMap.find(sldr);
    if (iter != mSliderKeyMap.end())
    {
        LL_DEBUGS() << "Removing frame from " << iter->second.mFrame << LL_ENDL;
        mSliderKeyMap.erase(iter);
        LLSettingsBase::Seconds seconds(iter->second.mFrame);
        mEditDay->removeTrackKeyframe(mCurrentTrack, seconds);
    }

    mLastFrameSlider = mFramesSlider->getCurSlider();
    mTimeSlider->setCurSliderValue(mFramesSlider->getCurSliderValue());
    updateTabs();
}

//-------------------------------------------------------------------------

LLFloaterEditExtDayCycle::connection_t LLFloaterEditExtDayCycle::setEditCommitSignal(LLFloaterEditExtDayCycle::edit_commit_signal_t::slot_type cb)
{
    return mCommitSignal.connect(cb);
}

void LLFloaterEditExtDayCycle::loadInventoryItem(const LLUUID  &inventoryId)
{
    if (inventoryId.isNull())
    {
        LL_WARNS("ENVIRONMENT") << "Attempt to load NULL inventory ID" << LL_ENDL;
        mInventoryItem = nullptr;
        mInventoryId.setNull();
        return;
    }

    mInventoryId = inventoryId;
    LL_INFOS("ENVIRONMENT") << "Setting edit inventory item to " << mInventoryId << "." << LL_ENDL;
    mInventoryItem = gInventory.getItem(mInventoryId);

    if (!mInventoryItem)
    {
        LL_WARNS("ENVIRONMENT") << "Could not find inventory item with Id = " << mInventoryId << LL_ENDL;

        LLNotificationsUtil::add("CantFindInvItem");
        closeFloater();
        mInventoryId.setNull();
        mInventoryItem = nullptr;
        return;
    }

    if (mInventoryItem->getAssetUUID().isNull())
    {
        LL_WARNS("ENVIRONMENT") << "Asset ID in inventory item is NULL (" << mInventoryId << ")" <<  LL_ENDL;

        LLNotificationsUtil::add("UnableEditItem");
        closeFloater();

        mInventoryId.setNull();
        mInventoryItem = nullptr;
        return;
    }

    LLSettingsVOBase::getSettingsAsset(mInventoryItem->getAssetUUID(),
        [this](LLUUID asset_id, LLSettingsBase::ptr_t settings, S32 status, LLExtStat) { onAssetLoaded(asset_id, settings, status); });
}

void LLFloaterEditExtDayCycle::onAssetLoaded(LLUUID asset_id, LLSettingsBase::ptr_t settings, S32 status)
{
    if (!settings || status)
    {
        LLSD args;
        args["NAME"] = (mInventoryItem) ? mInventoryItem->getName() : "Unknown";
        LLNotificationsUtil::add("FailedToFindSettings", args);
        closeFloater();
        return;
    }
    mEditDay = std::dynamic_pointer_cast<LLSettingsDay>(settings);
    updateEditEnvironment();
    LLEnvironment::instance().setSelectedEnvironment(LLEnvironment::ENV_EDIT, LLEnvironment::TRANSITION_INSTANT);
    LLEnvironment::instance().updateEnvironment();
    synchronizeTabs();
    updateTabs();
    refresh();
}

void LLFloaterEditExtDayCycle::loadLiveEnvironment(LLEnvironment::EnvSelection_t env)
{
    for (S32 idx = static_cast<S32>(env); idx <= LLEnvironment::ENV_DEFAULT; ++idx)
    {
        LLSettingsDay::ptr_t day = LLEnvironment::instance().getEnvironmentDay(static_cast<LLEnvironment::EnvSelection_t>(idx));

        if (day)
        {
            mEditDay = day->buildClone();
            break;
        }
    }

    if (!mEditDay)
    {
        LL_WARNS("ENVIRONMENT") << "Unable to load environment " << env << " building default." << LL_ENDL;
        mEditDay = LLSettingsVODay::buildDefaultDayCycle();
    }

    updateEditEnvironment();
    LLEnvironment::instance().setSelectedEnvironment(LLEnvironment::ENV_EDIT, LLEnvironment::TRANSITION_INSTANT);
    LLEnvironment::instance().updateEnvironment();
    synchronizeTabs();
    updateTabs();
    refresh();
}

void LLFloaterEditExtDayCycle::updateEditEnvironment(void)
{
    if (!mEditDay)
        return;
    S32 skytrack = (mCurrentTrack) ? mCurrentTrack : 1;
    mSkyBlender = std::make_shared<LLTrackBlenderLoopingManual>(mScratchSky, mEditDay, skytrack);
    mWaterBlender = std::make_shared<LLTrackBlenderLoopingManual>(mScratchWater, mEditDay, LLSettingsDay::TRACK_WATER);

    selectTrack(LLSettingsDay::TRACK_MAX, true);

    reblendSettings();

    LLEnvironment::instance().setEnvironment(LLEnvironment::ENV_EDIT, mScratchSky, mScratchWater);
}

void LLFloaterEditExtDayCycle::synchronizeTabs()
{
    // This should probably get moved into "updateTabs"
    LLSettingsBase::TrackPosition frame(mTimeSlider->getCurSliderValue());
    bool canedit(false);

    LLSettingsWater::ptr_t psettingW;
    LLTabContainer * tabs = mWaterTabLayoutContainer->getChild<LLTabContainer>(TABS_WATER);
    if (mCurrentTrack == LLSettingsDay::TRACK_WATER)
    {
        canedit = !mIsPlaying;
        LLSettingsDay::CycleTrack_t::value_type found = mEditDay->getSettingsNearKeyframe(frame, LLSettingsDay::TRACK_WATER, FRAME_SLOP_FACTOR);
        psettingW = std::static_pointer_cast<LLSettingsWater>(found.second);
        mCurrentEdit = psettingW;
        if (!psettingW)
        {
            canedit = false;
            psettingW = mScratchWater;
        }

        getChild<LLUICtrl>(ICN_LOCK_EDIT)->setVisible(!canedit);
    }
    else 
    {
        psettingW = mScratchWater;
    }

    setTabsData(tabs, psettingW, canedit);

    LLSettingsSky::ptr_t psettingS;
    canedit = false;
    tabs = mSkyTabLayoutContainer->getChild<LLTabContainer>(TABS_SKYS);
    if (mCurrentTrack != LLSettingsDay::TRACK_WATER)
    {
        canedit = !mIsPlaying;
        LLSettingsDay::CycleTrack_t::value_type found = mEditDay->getSettingsNearKeyframe(frame, mCurrentTrack, FRAME_SLOP_FACTOR);
        psettingS = std::static_pointer_cast<LLSettingsSky>(found.second);
        mCurrentEdit = psettingS;
        if (!psettingS)
        {
            canedit = false;
            psettingS = mScratchSky;
        }

        getChild<LLUICtrl>(ICN_LOCK_EDIT)->setVisible(!canedit);
    }
    else
    {
        psettingS = mScratchSky;
    }

    doCloseInventoryFloater();

    setTabsData(tabs, psettingS, canedit);
    LLEnvironment::instance().setEnvironment(LLEnvironment::ENV_EDIT, psettingS, psettingW);
}

void LLFloaterEditExtDayCycle::setTabsData(LLTabContainer * tabcontainer, const LLSettingsBase::ptr_t &settings, bool editable)
{
    S32 count = tabcontainer->getTabCount();
    for (S32 idx = 0; idx < count; ++idx)
    {
        LLSettingsEditPanel *panel = static_cast<LLSettingsEditPanel *>(tabcontainer->getPanelByIndex(idx));
        if (panel)
        {
            panel->setSettings(settings);
            panel->setEnabled(editable);
            panel->setAllChildrenEnabled(editable);
        }
    }
}


void LLFloaterEditExtDayCycle::reblendSettings()
{
    F64 position = mTimeSlider->getCurSliderValue();

    if ((mSkyBlender->getTrack() != mCurrentTrack) && (mCurrentTrack != LLSettingsDay::TRACK_WATER))
    {
        mSkyBlender->switchTrack(mCurrentTrack, position);
    }
    else
        mSkyBlender->setPosition(position);

    mWaterBlender->setPosition(position);    
}

void LLFloaterEditExtDayCycle::doApplyCreateNewInventory()
{
    // This method knows what sort of settings object to create.
    LLUUID parent_id = mInventoryItem ? mInventoryItem->getParentUUID() : gInventory.findCategoryUUIDForType(LLFolderType::FT_SETTINGS);

    LLSettingsVOBase::createInventoryItem(mEditDay, parent_id, 
            [this](LLUUID asset_id, LLUUID inventory_id, LLUUID, LLSD results) { onInventoryCreated(asset_id, inventory_id, results); });
}

void LLFloaterEditExtDayCycle::doApplyUpdateInventory()
{
    if (mInventoryId.isNull())
        LLSettingsVOBase::createInventoryItem(mEditDay, gInventory.findCategoryUUIDForType(LLFolderType::FT_SETTINGS), 
                [this](LLUUID asset_id, LLUUID inventory_id, LLUUID, LLSD results) { onInventoryCreated(asset_id, inventory_id, results); });
    else
        LLSettingsVOBase::updateInventoryItem(mEditDay, mInventoryId, 
                [this](LLUUID asset_id, LLUUID inventory_id, LLUUID, LLSD results) { onInventoryUpdated(asset_id, inventory_id, results); });
}

void LLFloaterEditExtDayCycle::doApplyEnvironment(const std::string &where)
{
    if (where == ACTION_APPLY_LOCAL)
    {
        LLEnvironment::instance().setEnvironment(LLEnvironment::ENV_LOCAL, mEditDay);
    }
    else if (where == ACTION_APPLY_PARCEL)
    {
        LLParcelSelectionHandle handle(LLViewerParcelMgr::instance().getParcelSelection());
        LLParcel *parcel(nullptr);

        if (handle)
            parcel = handle->getParcel();
        if (!parcel || (parcel->getLocalID() == INVALID_PARCEL_ID))
            parcel = LLViewerParcelMgr::instance().getAgentParcel();

        if ((!parcel) || (parcel->getLocalID() == INVALID_PARCEL_ID))
        {
            LL_WARNS("ENVIRONMENT") << "Can not identify parcel. Not applying." << LL_ENDL;
            LLNotificationsUtil::add("WLParcelApplyFail");
            return;
        }

        LLEnvironment::instance().updateParcel(parcel->getLocalID(), mEditDay, -1, -1);
    }
    else if (where == ACTION_APPLY_REGION)
    {
        LLEnvironment::instance().updateRegion(mEditDay, -1, -1);
    }
    else
    {
        LL_WARNS("ENVIRONMENT") << "Unknown apply '" << where << "'" << LL_ENDL;
        return;
    }

}

void LLFloaterEditExtDayCycle::onInventoryCreated(LLUUID asset_id, LLUUID inventory_id, LLSD results)
{
    LL_INFOS("ENVIRONMENT") << "Inventory item " << inventory_id << " has been created with asset " << asset_id << " results are:" << results << LL_ENDL;

    if (inventory_id.isNull() || !results["success"].asBoolean())
    {
        LLNotificationsUtil::add("CantCreateInventory");
        return;
    }

    setFocus(TRUE);                 // Call back the focus...
    loadInventoryItem(inventory_id);
}

void LLFloaterEditExtDayCycle::onInventoryUpdated(LLUUID asset_id, LLUUID inventory_id, LLSD results)
{
    LL_WARNS("ENVIRONMENT") << "Inventory item " << inventory_id << " has been updated with asset " << asset_id << " results are:" << results << LL_ENDL;

    if (inventory_id != mInventoryId)
    {
        loadInventoryItem(inventory_id);
    }
}

void LLFloaterEditExtDayCycle::doImportFromDisk()
{   // Load a a legacy Windlight XML from disk.

    LLFilePicker& picker = LLFilePicker::instance();
    if (picker.getOpenFile(LLFilePicker::FFLOAD_XML))
    {
        std::string filename = picker.getFirstFile();

        LL_WARNS("LAPRAS") << "Selected file: " << filename << LL_ENDL;
        LLSettingsDay::ptr_t legacyday = LLEnvironment::createDayCycleFromLegacyPreset(filename);

        if (!legacyday)
        {   
            LLSD args(LLSDMap("FILE", filename));
            LLNotificationsUtil::add("WLImportFail", args);
            return;
        }

        mEditDay = legacyday;
        mCurrentTrack = 1;
        updateSlider();
        updateEditEnvironment();
        synchronizeTabs();
        refresh();
    }
}

bool LLFloaterEditExtDayCycle::canUseInventory() const
{
    return LLEnvironment::instance().isInventoryEnabled();
}

bool LLFloaterEditExtDayCycle::canApplyRegion() const
{
    return gAgent.canManageEstate();
}

bool LLFloaterEditExtDayCycle::canApplyParcel() const
{
    LLParcelSelectionHandle handle(LLViewerParcelMgr::instance().getParcelSelection());
    LLParcel *parcel(nullptr);

    if (handle)
        parcel = handle->getParcel();
    if (!parcel)
        parcel = LLViewerParcelMgr::instance().getAgentParcel();

    if (!parcel)
        return false;

    return parcel->allowTerraformBy(gAgent.getID()) &&
        LLEnvironment::instance().isExtendedEnvironmentEnabled();
}

void LLFloaterEditExtDayCycle::startPlay()
{
    doCloseInventoryFloater();

    mIsPlaying = true;
    mFramesSlider->resetCurSlider();
    mPlayTimer.reset();
    mPlayTimer.start();
    gIdleCallbacks.addFunction(onIdlePlay, this);
    mPlayStartFrame = mTimeSlider->getCurSliderValue();

    getChild<LLView>("play_layout", true)->setVisible(FALSE);
    getChild<LLView>("pause_layout", true)->setVisible(TRUE);
}

void LLFloaterEditExtDayCycle::stopPlay()
{
    if (!mIsPlaying)
        return;

    mIsPlaying = false;
    gIdleCallbacks.deleteFunction(onIdlePlay, this);
    mPlayTimer.stop();
    F32 frame = mTimeSlider->getCurSliderValue();
    selectFrame(frame, FRAME_SLOP_FACTOR);

    getChild<LLView>("play_layout", true)->setVisible(TRUE);
    getChild<LLView>("pause_layout", true)->setVisible(FALSE);
}

//static
void LLFloaterEditExtDayCycle::onIdlePlay(void* user_data)
{
    LLFloaterEditExtDayCycle* self = (LLFloaterEditExtDayCycle*)user_data;

    F32 prcnt_played = self->mPlayTimer.getElapsedTimeF32() / DAY_CYCLE_PLAY_TIME_SECONDS;
    F32 new_frame = fmod(self->mPlayStartFrame + prcnt_played, 1.f);

    self->mTimeSlider->setCurSliderValue(new_frame); // will do the rounding
    self->mSkyBlender->setPosition(new_frame);
    self->mWaterBlender->setPosition(new_frame);
    self->synchronizeTabs();

}

void LLFloaterEditExtDayCycle::doOpenInventoryFloater(LLSettingsType::type_e type, LLUUID currasset)
{
//  LLUI::sWindow->setCursor(UI_CURSOR_WAIT);
    LLFloaterSettingsPicker *picker = static_cast<LLFloaterSettingsPicker *>(mInventoryFloater.get());

    // Show the dialog
    if (!picker)
    {
        picker = new LLFloaterSettingsPicker(this,
            LLUUID::null, "SELECT SETTINGS");

        mInventoryFloater = picker->getHandle();

        picker->setCommitCallback([this](LLUICtrl *, const LLSD &data){ onPickerCommitSetting(data.asUUID()); });
    }

    picker->setSettingsFilter(type);
    picker->openFloater();
    picker->setFocus(TRUE);
}

void LLFloaterEditExtDayCycle::doCloseInventoryFloater(bool quitting)
{
    LLFloater* floaterp = mInventoryFloater.get();

    if (floaterp)
    {
        floaterp->closeFloater(quitting);
    }
}

void LLFloaterEditExtDayCycle::onPickerCommitSetting(LLUUID asset_id)
{
    LLSettingsBase::TrackPosition frame(mTimeSlider->getCurSliderValue());
    S32 track = mCurrentTrack;

    LLSettingsVOBase::getSettingsAsset(asset_id,
        [this, track, frame](LLUUID asset_id, LLSettingsBase::ptr_t settings, S32 status, LLExtStat) { onAssetLoadedForFrame(asset_id, settings, status, track, frame); });
}

void LLFloaterEditExtDayCycle::onAssetLoadedForFrame(LLUUID asset_id, LLSettingsBase::ptr_t settings, S32 status, S32 track, LLSettingsBase::TrackPosition frame)
{
    if (!settings || status)
    {
        LL_WARNS("ENVIRONMENT") << "Could not load asset " << asset_id << " into frame. status=" << status << LL_ENDL;
        return;
    }

    mEditDay->setSettingsAtKeyframe(settings, frame, track);
    reblendSettings();
    synchronizeTabs();
}
