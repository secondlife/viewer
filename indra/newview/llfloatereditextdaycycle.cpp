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
#include "lltabcontainer.h"
#include "llfilepicker.h"

#include "llsettingsvo.h"
#include "llinventorymodel.h"
#include "llviewerparcelmgr.h"

// newview
#include "llagent.h"
#include "llparcel.h"
#include "llflyoutcombobtn.h" //Todo: make a proper UI element/button/panel instead
#include "llregioninfomodel.h"
#include "llviewerregion.h"
#include "llpaneleditwater.h"
#include "llpaneleditsky.h"

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
const std::string XML_FLYOUTMENU_FILE("menu_save_settings.xml");
// From menu_save_settings.xml, consider moving into flyout since it should be supported by flyout either way
const std::string ACTION_SAVE("save_settings");
const std::string ACTION_SAVEAS("save_as_new_settings");
const std::string ACTION_APPLY_LOCAL("apply_local");
const std::string ACTION_APPLY_PARCEL("apply_parcel");
const std::string ACTION_APPLY_REGION("apply_region");


//=========================================================================
// **RIDER**

const std::string LLFloaterEditExtDayCycle::KEY_INVENTORY_ID("inventory_id");
const std::string LLFloaterEditExtDayCycle::KEY_LIVE_ENVIRONMENT("live_environment");
const std::string LLFloaterEditExtDayCycle::KEY_DAY_LENGTH("day_length");

// **RIDER**

LLFloaterEditExtDayCycle::LLFloaterEditExtDayCycle(const LLSD &key):
    LLFloater(key),
    mFlyoutControl(NULL),
    mCancelButton(NULL),
    mDayLength(0),
    mCurrentTrack(4),
    mTimeSlider(NULL),
    mFramesSlider(NULL),
    mCurrentTimeLabel(NULL),
    // **RIDER**
    mImportButton(nullptr),
    mInventoryId(),
    mInventoryItem(nullptr),
    mSkyBlender(),
    mWaterBlender(),
    mScratchSky(),
    mScratchWater()
    // **RIDER**
{
    mCommitCallbackRegistrar.add("DayCycle.Track", boost::bind(&LLFloaterEditExtDayCycle::onTrackSelectionCallback, this, _2));

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
    getChild<LLLineEditor>("day_cycle_name")->setKeystrokeCallback(boost::bind(&LLFloaterEditExtDayCycle::onCommitName, this, _1, _2), NULL);

    mCancelButton = getChild<LLButton>("cancel_btn", true);
    mAddFrameButton = getChild<LLButton>("add_frame", true);
    mDeleteFrameButton = getChild<LLButton>("delete_frame", true);
    mTimeSlider = getChild<LLMultiSliderCtrl>("WLTimeSlider");
    mFramesSlider = getChild<LLMultiSliderCtrl>("WLDayCycleFrames");
    mSkyTabLayoutContainer = getChild<LLView>("frame_settings_sky", true);
    mWaterTabLayoutContainer = getChild<LLView>("frame_settings_water", true);
    mCurrentTimeLabel = getChild<LLTextBox>("current_time", true);
    mImportButton = getChild<LLButton>("btn_import", true);

    mFlyoutControl = new LLFlyoutComboBtnCtrl(this, "save_btn", "btn_flyout", XML_FLYOUTMENU_FILE);
    mFlyoutControl->setAction([this](LLUICtrl *ctrl, const LLSD &data) { onButtonApply(ctrl, data); });

    mCancelButton->setCommitCallback(boost::bind(&LLFloaterEditExtDayCycle::onBtnCancel, this));
    mTimeSlider->setCommitCallback(boost::bind(&LLFloaterEditExtDayCycle::onTimeSliderMoved, this));
    mFramesSlider->setCommitCallback(boost::bind(&LLFloaterEditExtDayCycle::onFrameSliderCallback, this));
    mAddFrameButton->setCommitCallback(boost::bind(&LLFloaterEditExtDayCycle::onAddTrack, this));
    mDeleteFrameButton->setCommitCallback(boost::bind(&LLFloaterEditExtDayCycle::onRemoveTrack, this));
    mImportButton->setCommitCallback([this](LLUICtrl *, const LLSD &){ onButtonImport(); });

    mTimeSlider->addSlider(0);


    getChild<LLButton>("sky4_track", true)->setToggleState(true);

	return TRUE;
}

void LLFloaterEditExtDayCycle::onOpen(const LLSD& key)
{
    LLEnvironment::instance().setSelectedEnvironment(LLEnvironment::ENV_EDIT);
    LLEnvironment::instance().updateEnvironment();

    // **RIDER**

    mEditingEnv = LLEnvironment::ENV_NONE;
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

    // **RIDER**

    selectTrack(mCurrentTrack);

    // time labels
    mCurrentTimeLabel->setTextArg("[PRCNT]", std::string("0"));
    const S32 max_elm = 5;
    if (mDayLength.value() != 0)
    {
        S32Hours hrs;
        S32Minutes minutes;
        S64Seconds total;
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
}

void LLFloaterEditExtDayCycle::onClose(bool app_quitting)
{
    // there's no point to change environment if we're quitting
    // or if we already restored environment
    if (!app_quitting && LLEnvironment::instance().getSelectedEnvironment() == LLEnvironment::ENV_EDIT)
    {
        LLEnvironment::instance().setSelectedEnvironment(LLEnvironment::ENV_LOCAL);
    }
}

void LLFloaterEditExtDayCycle::onVisibilityChange(BOOL new_visibility)
{
    if (new_visibility)
    {
        LLEnvironment::instance().setEnvironment(LLEnvironment::ENV_EDIT, mScratchSky, mScratchWater);
        LLEnvironment::instance().setSelectedEnvironment(LLEnvironment::ENV_EDIT);
    }
    else
    {
        LLEnvironment::instance().setSelectedEnvironment(LLEnvironment::ENV_LOCAL);
    }
}

void LLFloaterEditExtDayCycle::refresh()
{
    if (mEditDay)
    {
        LLLineEditor* name_field = getChild<LLLineEditor>("day_cycle_name");
        name_field->setText(mEditDay->getName());
    }

    bool is_inventory_avail = canUseInventory();

    mFlyoutControl->setMenuItemEnabled(ACTION_SAVE, is_inventory_avail);
    mFlyoutControl->setMenuItemEnabled(ACTION_SAVEAS, is_inventory_avail);


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
        // **RIDER**
        // scratch water should always have the current water settings.
        setting = mScratchWater->buildClone();
//         if (mSliderKeyMap.empty())
//         {
//             // No existing points, use defaults
//             setting = LLSettingsVOWater::buildDefaultWater();
//         }
//         else
//         {
//             // clone existing element, since we are intentionally dropping slider on time selection, copy from tab panels
//             LLView* tab_container = mWaterTabLayoutContainer->getChild<LLView>("water_tabs"); //can't extract panels directly, since it is in 'tuple'
//             LLPanelSettingsWaterMainTab* panel = dynamic_cast<LLPanelSettingsWaterMainTab*>(tab_container->getChildView("water_panel"));
//             if (panel)
//             {
//                 setting = panel->getWater()->buildClone();
//             }
//         }
        // **RIDER**
        mEditDay->setWaterAtKeyframe(std::dynamic_pointer_cast<LLSettingsWater>(setting), frame);
    }
    else
    {
        // **RIDER**
        // scratch sky should always have the current sky settings.
        setting = mScratchSky->buildClone();
//         if (mSliderKeyMap.empty())
//         {
//             // No existing points, use defaults
//             setting = LLSettingsVOSky::buildDefaultSky();
//         }
//         else
//         {
//             // clone existing element, since we are intentionally dropping slider on time selection, copy from tab panels
//             LLView* tab_container = mSkyTabLayoutContainer->getChild<LLView>("sky_tabs"); //can't extract panels directly, since they are in 'tuple'
// 
//             LLPanelSettingsSky* panel = dynamic_cast<LLPanelSettingsSky*>(tab_container->getChildView("atmosphere_panel"));
//             if (panel)
//             {
//                 setting = panel->getSky()->buildClone();
//             }
//         }
        // **RIDER**
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
    // todo: add safety 2.5% checks
    keymap_t::iterator iter = mSliderKeyMap.find(cur_sldr);
    if (iter != mSliderKeyMap.end() && mEditDay->getSettingsAtKeyframe(new_frame, mCurrentTrack).get() == NULL)
    {
        if (gKeyboard->currentMask(TRUE) == MASK_SHIFT)
        {
            LL_DEBUGS() << "Copying frame from " << iter->second.mFrame << " to " << new_frame << LL_ENDL;
            LLSettingsBase::ptr_t new_settings;

            // mEditDay still remembers old position, add copy at new position
            if (mCurrentTrack == LLSettingsDay::TRACK_WATER)
            {
                LLSettingsWaterPtr_t water_ptr = std::dynamic_pointer_cast<LLSettingsWater>(iter->second.pSettings)->buildClone();
                mEditDay->setWaterAtKeyframe(water_ptr, new_frame);
                new_settings = water_ptr;
            }
            else
            {
                LLSettingsSkyPtr_t sky_ptr = std::dynamic_pointer_cast<LLSettingsSky>(iter->second.pSettings)->buildClone();
                mEditDay->setSkyAtKeyframe(sky_ptr, new_frame, mCurrentTrack);
                new_settings = sky_ptr;
            }

            // mSliderKeyMap still remembers old position, for simplicity, just move it to be identical to slider
            F32 old_frame = iter->second.mFrame;
            iter->second.mFrame = new_frame;
            // slider already moved old frame, create new one in old place
            addSliderFrame(old_frame, new_settings, false /*because we are going to reselect new one*/);
            // reselect new frame
            mFramesSlider->setCurSlider(iter->first);
        }
        else
        {
            LL_DEBUGS() << "Moving frame from " << iter->second.mFrame << " to " << new_frame << LL_ENDL;
            if (mEditDay->moveTrackKeyframe(mCurrentTrack, iter->second.mFrame, new_frame))
            {
                iter->second.mFrame = new_frame;
            }
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
        if (iter->second.mFrame == frame)
        {
            mFramesSlider->setCurSlider(iter->first);
            break;
        }
        iter++;
    }

    // block or update tabs according to new selection
    updateTabs();

    // blending:
}

void LLFloaterEditExtDayCycle::selectTrack(U32 track_index)
{
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
//     std::string sldr = mFramesSlider->getCurSlider();
//     if (sldr.empty())
//     {
//        // keep old settings for duplicating if there are any
//        setWaterTabsEnabled(FALSE);
//        setSkyTabsEnabled(FALSE);
//     }
//     else if (mCurrentTrack == LLSettingsDay::TRACK_WATER)
//     {
//         const LLSettingsWaterPtr_t p_water = sldr.empty() ? LLSettingsWaterPtr_t(NULL) : mEditDay->getWaterAtKeyframe(mFramesSlider->getCurSliderValue());
//         updateWaterTabs(p_water);
//     }
//     else
//     {
//         const LLSettingsSkyPtr_t p_sky = sldr.empty() ? LLSettingsSkyPtr_t(NULL) : mEditDay->getSkyAtKeyframe(mFramesSlider->getCurSliderValue(), mCurrentTrack);
//         updateSkyTabs(p_sky);
//     }

    reblendSettings();
    syncronizeTabs();

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

void LLFloaterEditExtDayCycle::setWaterTabsEnabled(BOOL enable)
{
    LLView* tab_container = mWaterTabLayoutContainer->getChild<LLView>("water_tabs"); //can't extract panels directly, since it is in 'tuple'
    LLPanelSettingsWaterMainTab* panel = dynamic_cast<LLPanelSettingsWaterMainTab*>(tab_container->getChildView("water_panel"));
    if (panel)
    {
        panel->setEnabled(enable);
        panel->setAllChildrenEnabled(enable);
    }
}

void LLFloaterEditExtDayCycle::setSkyTabsEnabled(BOOL enable)
{
    LLView* tab_container = mSkyTabLayoutContainer->getChild<LLView>("sky_tabs"); //can't extract panels directly, since they are in 'tuple'

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
    F32 frame = mTimeSlider->getCurSliderValue();
    LLSettingsBase::ptr_t settings = mEditDay->getSettingsAtKeyframe(frame, mCurrentTrack);
    mAddFrameButton->setEnabled(settings.get() == NULL ? TRUE : FALSE);
    mDeleteFrameButton->setEnabled(mSliderKeyMap.size() > 0 ? TRUE : FALSE);
}

void LLFloaterEditExtDayCycle::updateSlider()
{
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
        mTimeSlider->setCurSliderValue(mFramesSlider->getCurSliderValue());
        updateTabs();
    }
    else
    {
        // disable panels
        clearTabs();
        mLastFrameSlider.clear();
    }
}

void LLFloaterEditExtDayCycle::updateTimeAndLabel()
{
    F32 time = mTimeSlider->getCurSliderValue();
    mCurrentTimeLabel->setTextArg("[PRCNT]", llformat("%.0f", time * 100));
    if (mDayLength.value() != 0)
    {
        LLUIString formatted_label = getString("time_label");

        S64Seconds total = (mDayLength  * time); 
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
        mEditDay->removeTrackKeyframe(mCurrentTrack, iter->second.mFrame);
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

// **RIDER**
void LLFloaterEditExtDayCycle::loadInventoryItem(const LLUUID  &inventoryId)
{
    if (inventoryId.isNull())
    {
        mInventoryItem = nullptr;
        mInventoryId.setNull();
        return;
    }

    mInventoryId = inventoryId;
    LL_INFOS("SETTINGS") << "Setting edit inventory item to " << mInventoryId << "." << LL_ENDL;
    mInventoryItem = gInventory.getItem(mInventoryId);

    if (!mInventoryItem)
    {
        LL_WARNS("SETTINGS") << "Could not find inventory item with Id = " << mInventoryId << LL_ENDL;
        mInventoryId.setNull();
        mInventoryItem = nullptr;
        return;
    }

    LLSettingsVOBase::getSettingsAsset(mInventoryItem->getAssetUUID(),
        [this](LLUUID asset_id, LLSettingsBase::ptr_t settins, S32 status, LLExtStat) { onAssetLoaded(asset_id, settins, status); });
}

void LLFloaterEditExtDayCycle::onAssetLoaded(LLUUID asset_id, LLSettingsBase::ptr_t settings, S32 status)
{
    mEditDay = std::dynamic_pointer_cast<LLSettingsDay>(settings);
    mOriginalDay = mEditDay->buildClone();
    updateEditEnvironment();
    syncronizeTabs();
    refresh();
}

void LLFloaterEditExtDayCycle::loadLiveEnvironment(LLEnvironment::EnvSelection_t env)
{
    mEditingEnv = env;
    for (S32 idx = static_cast<S32>(env); idx <= LLEnvironment::ENV_DEFAULT; ++idx)
    {
        LLSettingsDay::ptr_t day = LLEnvironment::instance().getEnvironmentDay(static_cast<LLEnvironment::EnvSelection_t>(idx));

        if (day)
        {
            mOriginalDay = day;
            mEditDay = day->buildClone();
            break;
        }
    }

    if (!mEditDay)
    {
        LL_WARNS("SETTINGS") << "Unable to load environment " << env << " building default." << LL_ENDL;
        mEditDay = LLSettingsVODay::buildDefaultDayCycle();
    }

    updateEditEnvironment();
    syncronizeTabs();
    refresh();
}

void LLFloaterEditExtDayCycle::updateEditEnvironment(void)
{
    S32 skytrack = (mCurrentTrack) ? mCurrentTrack : 1;
    mSkyBlender = std::make_shared<LLTrackBlenderLoopingManual>(mScratchSky, mEditDay, skytrack);
    mWaterBlender = std::make_shared<LLTrackBlenderLoopingManual>(mScratchWater, mEditDay, LLSettingsDay::TRACK_WATER);

    reblendSettings();

    LLEnvironment::instance().setEnvironment(LLEnvironment::ENV_EDIT, mScratchSky, mScratchWater);
}

void LLFloaterEditExtDayCycle::syncronizeTabs()
{
    // This should probably get moved into "updateTabs"

    F32 frame = mTimeSlider->getCurSliderValue();
    bool canedit(false);

    LLSettingsWater::ptr_t psettingWater;
    LLTabContainer * tabs = mWaterTabLayoutContainer->getChild<LLTabContainer>("water_tabs");
    if (mCurrentTrack == LLSettingsDay::TRACK_WATER)
    {
        canedit = true;
        psettingWater = std::static_pointer_cast<LLSettingsWater>(mEditDay->getSettingsAtKeyframe(frame, LLSettingsDay::TRACK_WATER));
        if (!psettingWater)
        {
            canedit = false;
            psettingWater = mScratchWater;
        }
    }
    else 
    {
        psettingWater = mScratchWater;
    }

    S32 count = tabs->getTabCount();
    for (S32 idx = 0; idx < count; ++idx)
    {
        LLSettingsEditPanel *panel = static_cast<LLSettingsEditPanel *>(tabs->getPanelByIndex(idx));
        if (panel)
        {
            panel->setAllChildrenEnabled(canedit);
            panel->setSettings(psettingWater);
            panel->refresh();
        }
    }

    LLSettingsSky::ptr_t psettingSky;

    canedit = false;
    tabs = mSkyTabLayoutContainer->getChild<LLTabContainer>("sky_tabs");
    if (mCurrentTrack != LLSettingsDay::TRACK_WATER)
    {
        canedit = true;
        psettingSky = std::static_pointer_cast<LLSettingsSky>(mEditDay->getSettingsAtKeyframe(frame, mCurrentTrack));
        if (!psettingSky)
        {
            canedit = false;
            psettingSky = mScratchSky;
        }
    }
    else
    {
        psettingSky = mScratchSky;
    }

    count = tabs->getTabCount();
    for (S32 idx = 0; idx < count; ++idx)
    {
        LLSettingsEditPanel *panel = static_cast<LLSettingsEditPanel *>(tabs->getPanelByIndex(idx));
        if (panel)
        {
            panel->setAllChildrenEnabled(canedit);
            panel->setSettings(psettingSky);
            panel->refresh();
        }
    }

    LLEnvironment::instance().setEnvironment(LLEnvironment::ENV_EDIT, psettingSky, psettingWater);
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
    LLSettingsVOBase::createInventoryItem(mEditDay, [this](LLUUID asset_id, LLUUID inventory_id, LLUUID, LLSD results) { onInventoryCreated(asset_id, inventory_id, results); });
}

void LLFloaterEditExtDayCycle::doApplyUpdateInventory()
{
    if (mInventoryId.isNull())
        LLSettingsVOBase::createInventoryItem(mEditDay, [this](LLUUID asset_id, LLUUID inventory_id, LLUUID, LLSD results) { onInventoryCreated(asset_id, inventory_id, results); });
    else
        LLSettingsVOBase::updateInventoryItem(mEditDay, mInventoryId, [this](LLUUID asset_id, LLUUID inventory_id, LLUUID, LLSD results) { onInventoryUpdated(asset_id, inventory_id, results); });
}

void LLFloaterEditExtDayCycle::doApplyEnvironment(const std::string &where)
{
    LLEnvironment::EnvSelection_t env(LLEnvironment::ENV_DEFAULT);
    bool updateSimulator(where != ACTION_APPLY_LOCAL);

    if (where == ACTION_APPLY_LOCAL)
        env = LLEnvironment::ENV_LOCAL;
    else if (where == ACTION_APPLY_PARCEL)
        env = LLEnvironment::ENV_PARCEL;
    else if (where == ACTION_APPLY_REGION)
        env = LLEnvironment::ENV_REGION;
    else
    {
        LL_WARNS("ENVIRONMENT") << "Unknown apply '" << where << "'" << LL_ENDL;
        return;
    }

    LLEnvironment::instance().setEnvironment(env, mEditDay);
    if (updateSimulator)
    {
        LL_WARNS("ENVIRONMENT") << "Attempting apply" << LL_ENDL;
    }
}

void LLFloaterEditExtDayCycle::onInventoryCreated(LLUUID asset_id, LLUUID inventory_id, LLSD results)
{
    LL_WARNS("ENVIRONMENT") << "Inventory item " << inventory_id << " has been created with asset " << asset_id << " results are:" << results << LL_ENDL;

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
        {   // *TODO* Put up error dialog here.  Could not create water from filename
            return;
        }

        mEditDay = legacyday;

        updateEditEnvironment();
        syncronizeTabs();
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

    return parcel->allowModifyBy(gAgent.getID(), gAgent.getGroupID()) &&
        LLEnvironment::instance().isExtendedEnvironmentEnabled();
}

// **RIDER**


