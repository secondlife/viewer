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
#include "llcheckboxctrl.h"
#include "llcombobox.h"
#include "llloadingindicator.h"
#include "llmultisliderctrl.h"
#include "llnotifications.h"
#include "llnotificationsutil.h"
#include "llspinctrl.h"
#include "lltimectrl.h"

#include "llsettingsvo.h"

// newview
#include "llagent.h"
//#include "llflyoutcombobtnctrl.h" //Todo: get rid of this and LLSaveOutfitComboBtn, make a proper UI element/button/pannel instead
#include "llregioninfomodel.h"
#include "llviewerregion.h"
#include "llpaneleditwater.h"
#include "llpaneleditsky.h"
//#include "llsettingsvo.h"
//#include "llinventorymodel.h"

#include "llenvironment.h"
#include "lltrans.h"

static const std::string track_tabs[] = {
    "water_track",
    "sky1_track",
    "sky2_track",
    "sky3_track",
    "sky4_track",
};

// For flyout
/*const std::string ACTION_SAVE("save_settings");
const std::string ACTION_SAVEAS("save_as_new_settings");
const std::string ACTION_APPLY_LOCAL("apply_local");
const std::string ACTION_APPLY_PARCEL("apply_parcel");
const std::string ACTION_APPLY_REGION("apply_region");

const std::string XML_FLYOUTMENU_FILE("menu_save_settings.xml");*/


LLFloaterEditExtDayCycle::LLFloaterEditExtDayCycle(const LLSD &key):
    LLFloater(key),
    mSaveButton(NULL),
    mCancelButton(NULL),
    mUploadButton(NULL),
    mDayLength(0),
    mDayOffset(0),
    mCurrentTrack(4),
    mTimeSlider(NULL),
    mFramesSlider(NULL),
    //mFlyoutControl(NULL),
    mCurrentTimeLabel(NULL)
{
    mCommitCallbackRegistrar.add("DayCycle.Track", boost::bind(&LLFloaterEditExtDayCycle::onTrackSelectionCallback, this, _2));
}

LLFloaterEditExtDayCycle::~LLFloaterEditExtDayCycle()
{
    // Todo: consider remaking mFlyoutControl into class that initializes intself with floater,
    // completes at postbuild, e t c...
    // (make it into actual button?, In such case XML_FLYOUTMENU_FILE will be specified in xml)
    //delete mFlyoutControl;
}

void LLFloaterEditExtDayCycle::openFloater(LLSettingsDay::ptr_t settings, S64Seconds daylength, S64Seconds dayoffset)
{
        mSavedDay = settings;
        mEditDay = settings->buildClone();
        mDayLength = daylength;
        mDayOffset = dayoffset;
        LLFloater::openFloater();
}

// virtual
BOOL LLFloaterEditExtDayCycle::postBuild()
{
    getChild<LLLineEditor>("day_cycle_name")->setKeystrokeCallback(boost::bind(&LLFloaterEditExtDayCycle::onCommitName, this, _1, _2), NULL);

    mSaveButton = getChild<LLButton>("save_btn", true);
    mCancelButton = getChild<LLButton>("cancel_btn", true);
    mUploadButton = getChild<LLButton>("upload_btn", true);
    mAddFrameButton = getChild<LLButton>("add_frame", true);
    mDeleteFrameButton = getChild<LLButton>("delete_frame", true);
    mTimeSlider = getChild<LLMultiSliderCtrl>("WLTimeSlider");
    mFramesSlider = getChild<LLMultiSliderCtrl>("WLDayCycleFrames");
    mSkyTabLayoutContainer = getChild<LLView>("frame_settings_sky", true);
    mWaterTabLayoutContainer = getChild<LLView>("frame_settings_water", true);
    mCurrentTimeLabel = getChild<LLTextBox>("current_time", true);

    //mFlyoutControl = new LLFlyoutComboBtnCtrl(this, "save_btn", "btn_flyout", XML_FLYOUTMENU_FILE);
    //mFlyoutControl->setAction([this](LLUICtrl *ctrl, const LLSD &data) { onButtonApply(ctrl, data); });

    mUploadButton->setCommitCallback(boost::bind(&LLFloaterEditExtDayCycle::onBtnSave, this));
    mCancelButton->setCommitCallback(boost::bind(&LLFloaterEditExtDayCycle::onBtnCancel, this));
    mUploadButton->setCommitCallback(boost::bind(&LLFloaterEditExtDayCycle::onBtnUpload, this));
    mTimeSlider->setCommitCallback(boost::bind(&LLFloaterEditExtDayCycle::onTimeSliderMoved, this));
    mFramesSlider->setCommitCallback(boost::bind(&LLFloaterEditExtDayCycle::onFrameSliderCallback, this));
    mAddFrameButton->setCommitCallback(boost::bind(&LLFloaterEditExtDayCycle::onAddTrack, this));
    mDeleteFrameButton->setCommitCallback(boost::bind(&LLFloaterEditExtDayCycle::onRemoveTrack, this));

    mTimeSlider->addSlider(0);


    getChild<LLButton>("sky4_track", true)->setToggleState(true);

	return TRUE;
}

void LLFloaterEditExtDayCycle::onOpen(const LLSD& key)
{
    if (mEditDay.get() == NULL)
    {
        LL_WARNS() << "Uninitialized day settings, closing floater" << LL_ENDL;
        closeFloater();
    }
    LLEnvironment::instance().setSelectedEnvironment(LLEnvironment::ENV_EDIT);
    LLEnvironment::instance().updateEnvironment();

    LLLineEditor* name_field = getChild<LLLineEditor>("day_cycle_name");
    name_field->setText(mEditDay->getName());

    selectTrack(mCurrentTrack);

    // time labels
    mCurrentTimeLabel->setTextArg("[PRCNT]", std::string("0"));
    const S32 max_elm = 5;
    if (mDayLength.value() != 0)
    {
        S32Hours hrs;
        S32Minutes minutes;
        S64Seconds total;
        //LLDate date;
        LLUIString formatted_label = getString("time_label");
        for (int i = 0; i < max_elm; i++)
        {
            total = ((mDayLength / (max_elm - 1)) * i) + mDayOffset;
            hrs = total;
            minutes = total - hrs;

            //date = LLDate(((mDayLength / (max_elm - 1)) * i) + mDayOffset);
            //formatted_label.setArg("[TIME]", date.toHTTPDateString(std::string("%H:%M")));
            //formatted_label.setArg("[TIME]", llformat("%.1f", hrs.value()));

            formatted_label.setArg("[HH]", llformat("%d", hrs.value()));
            formatted_label.setArg("[MM]", llformat("%d", abs(minutes.value())));
            getChild<LLTextBox>("p" + llformat("%d", i), true)->setTextArg("[DSC]", formatted_label.getString());
        }
        hrs = mDayOffset;
        minutes = mDayOffset - hrs;
        //formatted_label.setArg("[TIME]", llformat("%.1f", hrs.value()));
        //date = LLDate(mDayOffset);
        //formatted_label.setArg("[TIME]", date.toHTTPDateString(std::string("%H:%M")));
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
}

void LLFloaterEditExtDayCycle::onClose(bool app_quitting)
{
	if (!app_quitting) // there's no point to change environment if we're quitting
	{
        /* TODO: don't restore this environment.  We may have gotten here from land or region. */
        LLEnvironment::instance().clearEnvironment(LLEnvironment::ENV_EDIT);
        LLEnvironment::instance().updateEnvironment();
	}
}

void LLFloaterEditExtDayCycle::onVisibilityChange(BOOL new_visibility)
{
    if (new_visibility)
    {
        LLEnvironment::instance().setEnvironment(LLEnvironment::ENV_EDIT, mEditDay, LLSettingsDay::DEFAULT_DAYLENGTH, LLSettingsDay::DEFAULT_DAYOFFSET);
        LLEnvironment::instance().setSelectedEnvironment(LLEnvironment::ENV_EDIT);
    }
    else
    {
        /* TODO: don't restore this environment.  We may have gotten here from land or region. */
        LLEnvironment::instance().setSelectedEnvironment(LLEnvironment::ENV_LOCAL);
    }
}

/*void LLFloaterEditExtDayCycle::onButtonApply(LLUICtrl *ctrl, const LLSD &data)
{
    std::string ctrl_action = ctrl->getName();

    if (ctrl_action == ACTION_SAVE)
    {
        mSavedDay = mEditDay;
        //doApplyUpdateInventory();
    }
    else if (ctrl_action == ACTION_SAVEAS)
    {
        //doApplyCreateNewInventory();
        LLSettingsVOBase::createInventoryItem(mEditDay, NULL);
    }
    else if ((ctrl_action == ACTION_APPLY_LOCAL) ||
        (ctrl_action == ACTION_APPLY_PARCEL) ||
        (ctrl_action == ACTION_APPLY_REGION))
    {
        //doApplyEnvironment(ctrl_action);
        // Shouldn't be supported?
    }
    else
    {
        LL_WARNS("ENVIRONMENT") << "Unknown settings action '" << ctrl_action << "'" << LL_ENDL;
    }

    if (!mCommitSignal.empty())
        mCommitSignal(mEditDay);
    closeFloater();
    }*/

void LLFloaterEditExtDayCycle::onBtnSave()
{
    mSavedDay = mEditDay;

    //no longer needed?
    if (!mCommitSignal.empty())
        mCommitSignal(mEditDay);

    closeFloater();
}

void LLFloaterEditExtDayCycle::onBtnCancel()
{
    closeFloater();
}

void LLFloaterEditExtDayCycle::onBtnUpload()
{
    LLSettingsVOBase::createInventoryItem(mEditDay);
    //closeFloater();
}

void LLFloaterEditExtDayCycle::onAddTrack()
{
    // todo: 2.5% safety zone
    std::string sldr_key = mFramesSlider->getCurSlider();
    F32 frame = mTimeSlider->getCurSliderValue();
    LLSettingsBase::ptr_t setting;
    if (mEditDay->getSettingsAtKeyframe(frame, mCurrentTrack).get() != NULL)
    {
        return;
    }

    if (mCurrentTrack == LLSettingsDay::TRACK_WATER)
    {
        if (mSliderKeyMap.empty())
        {
            // No existing points, use defaults
            setting = LLSettingsVOWater::buildDefaultWater();
        }
        else
        {
            // clone existing element, since we are intentionally dropping slider on time selection, copy from tab panels
            LLView* tab_container = mWaterTabLayoutContainer->getChild<LLView>("water_tabs"); //can't extract panels directly, since it is in 'tuple'
            LLPanelSettingsWaterMainTab* panel = dynamic_cast<LLPanelSettingsWaterMainTab*>(tab_container->getChildView("water_panel"));
            if (panel)
            {
                setting = panel->getWater()->buildClone();
            }
        }
        mEditDay->setWaterAtKeyframe(std::dynamic_pointer_cast<LLSettingsWater>(setting), frame);
    }
    else
    {
        if (mSliderKeyMap.empty())
        {
            // No existing points, use defaults
            setting = LLSettingsVOSky::buildDefaultSky();
        }
        else
        {
            // clone existing element, since we are intentionally dropping slider on time selection, copy from tab panels
            LLView* tab_container = mSkyTabLayoutContainer->getChild<LLView>("sky_tabs"); //can't extract panels directly, since they are in 'tuple'

            LLPanelSettingsSky* panel = dynamic_cast<LLPanelSettingsSky*>(tab_container->getChildView("atmosphere_panel"));
            if (panel)
            {
                setting = panel->getSky()->buildClone();
            }
        }
        mEditDay->setSkyAtKeyframe(std::dynamic_pointer_cast<LLSettingsSky>(setting), frame, mCurrentTrack);
    }

    addSliderFrame(frame, setting);
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

void LLFloaterEditExtDayCycle::onFrameSliderCallback()
{
    if (mSliderKeyMap.size() == 0)
    {
        mLastFrameSlider.clear();
        return;
    }
    // make sure we have a slider
    const std::string& cur_sldr = mFramesSlider->getCurSlider();
    if (cur_sldr.empty())
    {
        mLastFrameSlider.clear();
        return;
    }

    F32 new_frame = mFramesSlider->getCurSliderValue();
    // todo: add safety checks, user shouldn't be capable of moving one frame over another or move missing frame
    keymap_t::iterator iter = mSliderKeyMap.find(cur_sldr);
    if (iter != mSliderKeyMap.end() && mEditDay->getSettingsAtKeyframe(new_frame, mCurrentTrack).get() == NULL)
    {
        LL_DEBUGS() << "Moving frame from " << iter->second.first << " to " << new_frame << LL_ENDL;
        if (mEditDay->moveTrackKeyframe(mCurrentTrack, iter->second.first, new_frame))
        {
            iter->second.first = new_frame;
        }
    }

    mTimeSlider->setCurSliderValue(new_frame);

    if (mLastFrameSlider != cur_sldr)
    {
        // technically should not be possible for both frame and slider to change
        // but for safety, assume that they can change independently and both
        mLastFrameSlider = cur_sldr;
        updateTabs();
    }
    else
    {
        updateButtons();
        updateTimeAndLabel();
    }
}

void LLFloaterEditExtDayCycle::onTimeSliderMoved()
{
    mFramesSlider->resetCurSlider();

    keymap_t::iterator iter = mSliderKeyMap.begin();
    keymap_t::iterator end_iter = mSliderKeyMap.end();
    F32 frame = mTimeSlider->getCurSliderValue();
    while (iter != end_iter)
    {
        if (iter->second.first == frame)
        {
            mFramesSlider->setCurSlider(iter->first);
            break;
        }
        iter++;
    }

    // Todo: safety checks
    updateTabs();
    //Todo: update something related to time/play/blending?
}

void LLFloaterEditExtDayCycle::selectTrack(U32 track_index)
{
    // todo: safety checks
    mCurrentTrack = track_index;
    LLButton* button = getChild<LLButton>(track_tabs[track_index], true);
    if (button->getToggleState())
    {
        return;
    }

    for (int i = 0; i < LLSettingsDay::TRACK_MAX; i++) // use max value
    {
        getChild<LLButton>(track_tabs[i], true)->setToggleState(false);
    }

    button->setToggleState(true);

    bool show_water = mCurrentTrack == LLSettingsDay::TRACK_WATER;
    mSkyTabLayoutContainer->setVisible(!show_water);
    mWaterTabLayoutContainer->setVisible(show_water);
    updateSlider();
}

void LLFloaterEditExtDayCycle::clearTabs()
{
    // todo: instead init with defaults?
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
    // TODO: either prevent user from editing existing settings or clone them to not affect saved frames
    std::string sldr = mFramesSlider->getCurSlider();
    if (sldr.empty())
    {
        // keep old settings for duplicating if there are any
        // TODO: disable tabs to prevent editing without nulling settings
    }
    else if (mCurrentTrack == LLSettingsDay::TRACK_WATER)
    {
        const LLSettingsWaterPtr_t p_water = sldr.empty() ? LLSettingsWaterPtr_t(NULL) : mEditDay->getWaterAtKeyframe(mFramesSlider->getCurSliderValue());
        updateWaterTabs(p_water);
    }
    else
    {
        const LLSettingsSkyPtr_t p_sky = sldr.empty() ? LLSettingsSkyPtr_t(NULL) : mEditDay->getSkyAtKeyframe(mFramesSlider->getCurSliderValue(), mCurrentTrack);
        updateSkyTabs(p_sky);
    }
    updateButtons();
    updateTimeAndLabel();
}

void LLFloaterEditExtDayCycle::updateWaterTabs(const LLSettingsWaterPtr_t &p_water)
{
    LLView* tab_container = mWaterTabLayoutContainer->getChild<LLView>("water_tabs"); //can't extract panels directly, since it is in 'tuple'
    LLPanelSettingsWaterMainTab* panel = dynamic_cast<LLPanelSettingsWaterMainTab*>(tab_container->getChildView("water_panel"));
    if (panel)
    {
        panel->setWater(p_water);
    }
}

void LLFloaterEditExtDayCycle::updateSkyTabs(const LLSettingsSkyPtr_t &p_sky)
{
    LLView* tab_container = mSkyTabLayoutContainer->getChild<LLView>("sky_tabs"); //can't extract panels directly, since they are in 'tuple'

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

void LLFloaterEditExtDayCycle::updateButtons()
{
    F32 frame = mTimeSlider->getCurSliderValue();
    LLSettingsBase::ptr_t settings = mEditDay->getSettingsAtKeyframe(frame, mCurrentTrack);
    mAddFrameButton->setEnabled(settings.get() == NULL ? TRUE : FALSE);
    mDeleteFrameButton->setEnabled(mSliderKeyMap.size() > 0 ? TRUE : FALSE);
}

void LLFloaterEditExtDayCycle::updateSlider()
{
    mFramesSlider->clear();
    mSliderKeyMap.clear();

    std::string new_slider;
    F32 frame = 0;
    LLSettingsDay::CycleTrack_t track = mEditDay->getCycleTrack(mCurrentTrack);
    for (auto &track_frame : track)
    {
        // multi slider distinguishes elements by key/name in string format
        // store names to map to be able to recall dependencies
        frame = track_frame.first;
        new_slider = mFramesSlider->addSlider(frame);
        mSliderKeyMap[new_slider] = framedata_t(frame, track_frame.second);
    }

    mLastFrameSlider = new_slider;

    if (mSliderKeyMap.size() > 0)
    {
        mTimeSlider->setCurSliderValue(frame);
        updateTabs();
    }
    else
    {
        // disable panels
        clearTabs();
    }
}

void LLFloaterEditExtDayCycle::updateTimeAndLabel()
{
    F32 time = mTimeSlider->getCurSliderValue();
    mCurrentTimeLabel->setTextArg("[PRCNT]", llformat("%.0f", time * 100));
    if (mDayLength.value() != 0)
    {
        LLUIString formatted_label = getString("time_label");

        //F32Hours hrs = (mDayLength  * time) + mDayOffset;
        //LLDate date((mDayLength  * time) + mDayOffset);
        //formatted_label.setArg("[TIME]", llformat("%.1f", hrs.value()));
        //formatted_label.setArg("[TIME]", date.toHTTPDateString(std::string("%H:%M")));

        S64Seconds total = (mDayLength  * time) + mDayOffset;
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

void LLFloaterEditExtDayCycle::addSliderFrame(const F32 frame, LLSettingsBase::ptr_t setting)
{
    // multi slider distinguishes elements by key/name in string format
    // store names to map to be able to recall dependencies
    std::string new_slider = mFramesSlider->addSlider(frame);
    mSliderKeyMap[new_slider] = framedata_t(frame, setting);
    mLastFrameSlider = new_slider;

    mTimeSlider->setCurSliderValue(frame);
    updateTabs();
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
        LL_DEBUGS() << "Removing frame from " << iter->second.first << LL_ENDL;
        mSliderKeyMap.erase(iter);
        mEditDay->removeTrackKeyframe(mCurrentTrack, iter->second.first);
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

