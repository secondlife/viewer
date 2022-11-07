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
#include "lllocalbitmaps.h"
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
#include "lltrackpicker.h"

// newview
#include "llagent.h"
#include "llappviewer.h" //gDisconected
#include "llparcel.h"
#include "llflyoutcombobtn.h" //Todo: make a proper UI element/button/panel instead
#include "llregioninfomodel.h"
#include "llviewermenufile.h" // LLFilePickerReplyThread
#include "llviewerregion.h"
#include "llpaneleditwater.h"
#include "llpaneleditsky.h"

#include "llui.h"

#include "llenvironment.h"
#include "lltrans.h"

extern LLControlGroup gSavedSettings;

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
    const std::string BTN_CLONETRACK("copy_track");
    const std::string BTN_LOADTRACK("load_track");
    const std::string BTN_CLEARTRACK("clear_track");
    const std::string SLDR_TIME("WLTimeSlider");
    const std::string SLDR_KEYFRAMES("WLDayCycleFrames");
    const std::string VIEW_SKY_SETTINGS("frame_settings_sky");
    const std::string VIEW_WATER_SETTINGS("frame_settings_water");
    const std::string LBL_CURRENT_TIME("current_time");
    const std::string TXT_DAY_NAME("day_cycle_name");
    const std::string TABS_SKYS("sky_tabs");
    const std::string TABS_WATER("water_tabs");

    // 'Play' buttons
    const std::string BTN_PLAY("play_btn");
    const std::string BTN_SKIP_BACK("skip_back_btn");
    const std::string BTN_SKIP_FORWARD("skip_forward_btn");

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
    const std::string ACTION_COMMIT("commit_changes");
    const std::string ACTION_APPLY_LOCAL("apply_local");
    const std::string ACTION_APPLY_PARCEL("apply_parcel");
    const std::string ACTION_APPLY_REGION("apply_region");

    const F32 DAY_CYCLE_PLAY_TIME_SECONDS = 60;

    const std::string STR_COMMIT_PARCEL("commit_parcel");
    const std::string STR_COMMIT_REGION("commit_region");
    //---------------------------------------------------------------------

}

//=========================================================================
const std::string LLFloaterEditExtDayCycle::KEY_EDIT_CONTEXT("edit_context");
const std::string LLFloaterEditExtDayCycle::KEY_DAY_LENGTH("day_length");
const std::string LLFloaterEditExtDayCycle::KEY_CANMOD("canmod");

const std::string LLFloaterEditExtDayCycle::VALUE_CONTEXT_INVENTORY("inventory");
const std::string LLFloaterEditExtDayCycle::VALUE_CONTEXT_PARCEL("parcel");
const std::string LLFloaterEditExtDayCycle::VALUE_CONTEXT_REGION("region");
/*
//=========================================================================

class LLDaySettingCopiedCallback : public LLInventoryCallback
{
public:
    LLDaySettingCopiedCallback(LLHandle<LLFloater> handle) : mHandle(handle) {}

    virtual void fire(const LLUUID& inv_item_id)
    {
        if (!mHandle.isDead())
        {
            LLViewerInventoryItem* item = gInventory.getItem(inv_item_id);
            if (item)
            {
                LLFloaterEditExtDayCycle* floater = (LLFloaterEditExtDayCycle*)mHandle.get();
                floater->onInventoryCreated(item->getAssetUUID(), inv_item_id);
            }
        }
    }

private:
    LLHandle<LLFloater> mHandle;
};*/

//=========================================================================

LLFloaterEditExtDayCycle::LLFloaterEditExtDayCycle(const LLSD &key) :
    LLFloaterEditEnvironmentBase(key),
    mFlyoutControl(nullptr),
    mDayLength(0),
    mCurrentTrack(1),
    mShiftCopyEnabled(false),
    mTimeSlider(nullptr),
    mFramesSlider(nullptr),
    mCurrentTimeLabel(nullptr),
    mImportButton(nullptr),
    mLoadFrame(nullptr),
    mSkyBlender(),
    mWaterBlender(),
    mScratchSky(),
    mScratchWater(),
    mIsPlaying(false),
    mCloneTrack(nullptr),
    mLoadTrack(nullptr),
    mClearTrack(nullptr)
{

    mCommitCallbackRegistrar.add(EVNT_DAYTRACK, [this](LLUICtrl *ctrl, const LLSD &data) { onTrackSelectionCallback(data); });
    mCommitCallbackRegistrar.add(EVNT_PLAY, [this](LLUICtrl *ctrl, const LLSD &data) { onPlayActionCallback(data); });

    mScratchSky = LLSettingsVOSky::buildDefaultSky();
    mScratchWater = LLSettingsVOWater::buildDefaultWater();

    mEditSky = mScratchSky;
    mEditWater = mScratchWater;
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
    mCloneTrack = getChild<LLButton>(BTN_CLONETRACK, true);
    mLoadTrack = getChild<LLButton>(BTN_LOADTRACK, true);
    mClearTrack = getChild<LLButton>(BTN_CLEARTRACK, true);

    mFlyoutControl = new LLFlyoutComboBtnCtrl(this, BTN_SAVE, BTN_FLYOUT, XML_FLYOUTMENU_FILE, false);
    mFlyoutControl->setAction([this](LLUICtrl *ctrl, const LLSD &data) { onButtonApply(ctrl, data); });

    getChild<LLButton>(BTN_CANCEL, true)->setCommitCallback([this](LLUICtrl *ctrl, const LLSD &data) { onClickCloseBtn(); });
    mTimeSlider->setCommitCallback([this](LLUICtrl *ctrl, const LLSD &data) { onTimeSliderCallback(); });
    mAddFrameButton->setCommitCallback([this](LLUICtrl *ctrl, const LLSD &data) { onAddFrame(); });
    mDeleteFrameButton->setCommitCallback([this](LLUICtrl *ctrl, const LLSD &data) { onRemoveFrame(); });
    mImportButton->setCommitCallback([this](LLUICtrl *, const LLSD &){ onButtonImport(); });
    mLoadFrame->setCommitCallback([this](LLUICtrl *, const LLSD &){ onButtonLoadFrame(); });

    mCloneTrack->setCommitCallback([this](LLUICtrl *, const LLSD&){ onCloneTrack(); });
    mLoadTrack->setCommitCallback([this](LLUICtrl *, const LLSD&){  onLoadTrack();});
    mClearTrack->setCommitCallback([this](LLUICtrl *, const LLSD&){ onClearTrack(); });


    mFramesSlider->setCommitCallback([this](LLUICtrl *, const LLSD &data) { onFrameSliderCallback(data); });
    mFramesSlider->setDoubleClickCallback([this](LLUICtrl*, S32 x, S32 y, MASK mask){ onFrameSliderDoubleClick(x, y, mask); });
    mFramesSlider->setMouseDownCallback([this](LLUICtrl*, S32 x, S32 y, MASK mask){ onFrameSliderMouseDown(x, y, mask); });
    mFramesSlider->setMouseUpCallback([this](LLUICtrl*, S32 x, S32 y, MASK mask){ onFrameSliderMouseUp(x, y, mask); });

    mTimeSlider->addSlider(0);

    LLTabContainer* tab_container = mSkyTabLayoutContainer->getChild<LLTabContainer>("sky_tabs"); 
    S32 tab_count = tab_container->getTabCount();

    LLSettingsEditPanel *panel = nullptr;
    
    for (S32 idx = 0; idx < tab_count; ++idx)
    {
        panel = static_cast<LLSettingsEditPanel *>(tab_container->getPanelByIndex(idx));
        if (panel)
            panel->setOnDirtyFlagChanged([this](LLPanel *, bool val) { onPanelDirtyFlagChanged(val); });
    }

    tab_container = mWaterTabLayoutContainer->getChild<LLTabContainer>("water_tabs");
    tab_count = tab_container->getTabCount();

    for (S32 idx = 0; idx < tab_count; ++idx)
    {
        LLSettingsEditPanel *panel = static_cast<LLSettingsEditPanel *>(tab_container->getPanelByIndex(idx));
        if (panel)
            panel->setOnDirtyFlagChanged([this](LLPanel *, bool val) { onPanelDirtyFlagChanged(val); });
    }

    return TRUE;
}

void LLFloaterEditExtDayCycle::onOpen(const LLSD& key)
{
    if (!mEditDay)
    {
        LLEnvironment::instance().saveBeaconsState();
    }
    mEditDay.reset();
    mEditContext = CONTEXT_UNKNOWN;
    if (key.has(KEY_EDIT_CONTEXT))
    {
        std::string context = key[KEY_EDIT_CONTEXT].asString();

        if (context == VALUE_CONTEXT_INVENTORY)
            mEditContext = CONTEXT_INVENTORY;
        else if (context == VALUE_CONTEXT_PARCEL)
            mEditContext = CONTEXT_PARCEL;
        else if (context == VALUE_CONTEXT_REGION)
            mEditContext = CONTEXT_REGION;
    }

    if (key.has(KEY_CANMOD))
    {
        mCanMod = key[KEY_CANMOD].asBoolean();
    }

    if (mEditContext == CONTEXT_UNKNOWN)
    {
        LL_WARNS("ENVDAYEDIT") << "Unknown editing context!" << LL_ENDL;
    }

    if (key.has(KEY_INVENTORY_ID))
    {
        loadInventoryItem(key[KEY_INVENTORY_ID].asUUID());
    }
    else
    {
        mCanSave = true;
        mCanCopy = true;
        mCanMod = true;
        mCanTrans = true;
        setEditDefaultDayCycle();
    }

    mDayLength.value(0);
    if (key.has(KEY_DAY_LENGTH))
    {
        mDayLength.value(key[KEY_DAY_LENGTH].asReal());
    }

    // Time&Percentage labels
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

    // Adjust Time&Percentage labels' location according to length
    LLRect label_rect = getChild<LLTextBox>("p0", true)->getRect();
    F32 slider_width = mFramesSlider->getRect().getWidth();
    for (int i = 1; i < max_elm; i++)
    {
        LLTextBox *pcnt_label = getChild<LLTextBox>("p" + llformat("%d", i), true);
        LLRect new_rect = pcnt_label->getRect();
        new_rect.mLeft = label_rect.mLeft + (S32)(slider_width * (F32)i / (F32)(max_elm - 1)) - (S32)(pcnt_label->getTextPixelWidth() / 2);
        pcnt_label->setRect(new_rect);
    }

    // Altitudes&Track labels
    LLUIString formatted_label = getString("sky_track_label");
    const LLEnvironment::altitude_list_t &altitudes = LLEnvironment::instance().getRegionAltitudes();
    bool extended_env = LLEnvironment::instance().isExtendedEnvironmentEnabled();
    bool use_altitudes = extended_env
                         && altitudes.size() > 0
                         && ((mEditContext == CONTEXT_PARCEL) || (mEditContext == CONTEXT_REGION));
    for (S32 idx = 1; idx < 4; ++idx)
    {
        std::ostringstream convert;
        if (use_altitudes)
        {
            convert << altitudes[idx] << "m";
        }
        else
        {
            convert << (idx + 1);
        }
        formatted_label.setArg("[ALT]", convert.str());
        getChild<LLButton>(track_tabs[idx + 1], true)->setLabel(formatted_label.getString());
    }

    for (U32 i = 2; i < LLSettingsDay::TRACK_MAX; i++) //skies #2 through #4
    {
        getChild<LLButton>(track_tabs[i])->setEnabled(extended_env);
    }

    if (mEditContext == CONTEXT_INVENTORY)
    {
        mFlyoutControl->setShownBtnEnabled(true);
        mFlyoutControl->setSelectedItem(ACTION_SAVE);
    }
    else if ((mEditContext == CONTEXT_REGION) || (mEditContext == CONTEXT_PARCEL))
    {
        std::string commit_str = (mEditContext == CONTEXT_PARCEL) ? STR_COMMIT_PARCEL : STR_COMMIT_REGION;
        mFlyoutControl->setMenuItemLabel(ACTION_COMMIT, getString(commit_str));
        mFlyoutControl->setShownBtnEnabled(true);
        mFlyoutControl->setSelectedItem(ACTION_COMMIT);
    }
    else
    {
        mFlyoutControl->setShownBtnEnabled(false);
    }
}

void LLFloaterEditExtDayCycle::onClose(bool app_quitting)
{
    doCloseInventoryFloater(app_quitting);
    doCloseTrackFloater(app_quitting);
    // there's no point to change environment if we're quitting
    // or if we already restored environment
    stopPlay();
    LLEnvironment::instance().revertBeaconsState();
    if (!app_quitting)
    {
        LLEnvironment::instance().setSelectedEnvironment(LLEnvironment::ENV_LOCAL, LLEnvironment::TRANSITION_FAST);
        LLEnvironment::instance().clearEnvironment(LLEnvironment::ENV_EDIT);
        mEditDay.reset();
    }
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
        name_field->setEnabled(mCanMod);
    }


    bool is_inventory_avail = canUseInventory();

    bool show_commit = ((mEditContext == CONTEXT_PARCEL) || (mEditContext == CONTEXT_REGION));
    bool show_apply = (mEditContext == CONTEXT_INVENTORY);

    if (show_commit)
    {
        std::string commit_text;
        if (mEditContext == CONTEXT_PARCEL)
            commit_text = getString(STR_COMMIT_PARCEL);
        else
            commit_text = getString(STR_COMMIT_REGION);

        mFlyoutControl->setMenuItemLabel(ACTION_COMMIT, commit_text);
    }

    mFlyoutControl->setMenuItemVisible(ACTION_COMMIT, show_commit);
    mFlyoutControl->setMenuItemVisible(ACTION_SAVE, is_inventory_avail);
    mFlyoutControl->setMenuItemVisible(ACTION_SAVEAS, is_inventory_avail);
    mFlyoutControl->setMenuItemVisible(ACTION_APPLY_LOCAL, true);
    mFlyoutControl->setMenuItemVisible(ACTION_APPLY_PARCEL, show_apply);
    mFlyoutControl->setMenuItemVisible(ACTION_APPLY_REGION, show_apply);

    mFlyoutControl->setMenuItemEnabled(ACTION_COMMIT, show_commit && !mCommitSignal.empty());
    mFlyoutControl->setMenuItemEnabled(ACTION_SAVE, is_inventory_avail && mCanMod && !mInventoryId.isNull() && mCanSave);
    mFlyoutControl->setMenuItemEnabled(ACTION_SAVEAS, is_inventory_avail && mCanCopy && mCanSave);
    mFlyoutControl->setMenuItemEnabled(ACTION_APPLY_LOCAL, true);
    mFlyoutControl->setMenuItemEnabled(ACTION_APPLY_PARCEL, canApplyParcel() && show_apply);
    mFlyoutControl->setMenuItemEnabled(ACTION_APPLY_REGION, canApplyRegion() && show_apply);

    mImportButton->setEnabled(mCanMod);

    LLFloater::refresh();
}

void LLFloaterEditExtDayCycle::setEditSettingsAndUpdate(const LLSettingsBase::ptr_t &settings)
{
    setEditDayCycle(std::dynamic_pointer_cast<LLSettingsDay>(settings));
}

void LLFloaterEditExtDayCycle::setEditDayCycle(const LLSettingsDay::ptr_t &pday)
{
    mExpectingAssetId.setNull();
    mEditDay = pday->buildDeepCloneAndUncompress();

    if (mEditDay->isTrackEmpty(LLSettingsDay::TRACK_WATER))
    {
        LL_WARNS("ENVDAYEDIT") << "No water frames found, generating replacement" << LL_ENDL;
        mEditDay->setWaterAtKeyframe(LLSettingsVOWater::buildDefaultWater(), .5f);
    }

    if (mEditDay->isTrackEmpty(LLSettingsDay::TRACK_GROUND_LEVEL))
    {
        LL_WARNS("ENVDAYEDIT") << "No sky frames found, generating replacement" << LL_ENDL;
        mEditDay->setSkyAtKeyframe(LLSettingsVOSky::buildDefaultSky(), .5f, LLSettingsDay::TRACK_GROUND_LEVEL);
    }

    mCanSave = !pday->getFlag(LLSettingsBase::FLAG_NOSAVE);
    mCanCopy = !pday->getFlag(LLSettingsBase::FLAG_NOCOPY) && mCanSave;
    mCanMod = !pday->getFlag(LLSettingsBase::FLAG_NOMOD) && mCanSave;
    mCanTrans = !pday->getFlag(LLSettingsBase::FLAG_NOTRANS) && mCanSave;

    updateEditEnvironment();
    LLEnvironment::instance().setSelectedEnvironment(LLEnvironment::ENV_EDIT, LLEnvironment::TRANSITION_INSTANT);
    LLEnvironment::instance().updateEnvironment(LLEnvironment::TRANSITION_INSTANT);
    synchronizeTabs();
    updateTabs();
    refresh();
}


void LLFloaterEditExtDayCycle::setEditDefaultDayCycle()
{
    mInventoryItem = nullptr;
    mInventoryId.setNull();
    mExpectingAssetId = LLSettingsDay::GetDefaultAssetId();
    LLSettingsVOBase::getSettingsAsset(LLSettingsDay::GetDefaultAssetId(),
        [this](LLUUID asset_id, LLSettingsBase::ptr_t settings, S32 status, LLExtStat) { onAssetLoaded(asset_id, settings, status); });
}

std::string LLFloaterEditExtDayCycle::getEditName() const
{
    if (mEditDay)
        return mEditDay->getName();
    return "new";
}

void LLFloaterEditExtDayCycle::setEditName(const std::string &name)
{
    if (mEditDay)
        mEditDay->setName(name);
    getChild<LLLineEditor>(TXT_DAY_NAME)->setText(name);
}

/* virtual */
BOOL LLFloaterEditExtDayCycle::handleKeyUp(KEY key, MASK mask, BOOL called_from_parent)
{
    if (!mEditDay)
    {
        mShiftCopyEnabled = false;
    }
    else if (mask == MASK_SHIFT && mShiftCopyEnabled)
    {
        mShiftCopyEnabled = false;
        std::string curslider = mFramesSlider->getCurSlider();
        if (!curslider.empty())
        {
            F32 sliderpos = mFramesSlider->getCurSliderValue();

            keymap_t::iterator it = mSliderKeyMap.find(curslider);
            if (it != mSliderKeyMap.end())
            {
                if (mEditDay->moveTrackKeyframe(mCurrentTrack, (*it).second.mFrame, sliderpos))
                {
                    (*it).second.mFrame = sliderpos;
                }
                else
                {
                    mFramesSlider->setCurSliderValue((*it).second.mFrame);
                }
            }
            else
            {
                LL_WARNS("ENVDAYEDIT") << "Failed to find frame " << sliderpos << " for slider " << curslider << LL_ENDL;
            }
        }
    }
    return LLFloater::handleKeyUp(key, mask, called_from_parent);
}

void LLFloaterEditExtDayCycle::onButtonApply(LLUICtrl *ctrl, const LLSD &data)
{
    std::string ctrl_action = ctrl->getName();

    if (!mEditDay)
    {
        LL_WARNS("ENVDAYEDIT") << "mEditDay is null! This should never happen! Something is very very wrong" << LL_ENDL;
        LLNotificationsUtil::add("EnvironmentApplyFailed");
        closeFloater();
        return;
    }

    LLSettingsDay::ptr_t dayclone = mEditDay->buildClone(); // create a compressed copy

    if (!dayclone)
    {
        LL_WARNS("ENVDAYEDIT") << "Unable to clone daycylce from editor." << LL_ENDL;
        return;
    }

    // brute-force local texture scan
    for (U32 i = 0; i <= LLSettingsDay::TRACK_MAX; i++)
    {
        LLSettingsDay::CycleTrack_t &day_track = dayclone->getCycleTrack(i);

        LLSettingsDay::CycleTrack_t::iterator iter = day_track.begin();
        LLSettingsDay::CycleTrack_t::iterator end = day_track.end();
        S32 frame_num = 0;

        while (iter != end)
        {
            frame_num++;
            std::string desc;
            bool is_local = false; // because getString can be empty
            if (i == LLSettingsDay::TRACK_WATER)
            {
                LLSettingsWater::ptr_t water = std::static_pointer_cast<LLSettingsWater>(iter->second);
                if (water)
                {
                    // LLViewerFetchedTexture and check for FTT_LOCAL_FILE or check LLLocalBitmapMgr
                    if (LLLocalBitmapMgr::getInstance()->isLocal(water->getNormalMapID()))
                    {
                        desc = LLTrans::getString("EnvironmentNormalMap");
                        is_local = true;
                    }
                    else if (LLLocalBitmapMgr::getInstance()->isLocal(water->getTransparentTextureID()))
                    {
                        desc = LLTrans::getString("EnvironmentTransparent");
                        is_local = true;
                    }
                }
            }
            else
            {
                LLSettingsSky::ptr_t sky = std::static_pointer_cast<LLSettingsSky>(iter->second);
                if (sky)
                {
                    if (LLLocalBitmapMgr::getInstance()->isLocal(sky->getSunTextureId()))
                    {
                        desc = LLTrans::getString("EnvironmentSun");
                        is_local = true;
                    }
                    else if (LLLocalBitmapMgr::getInstance()->isLocal(sky->getMoonTextureId()))
                    {
                        desc = LLTrans::getString("EnvironmentMoon");
                        is_local = true;
                    }
                    else if (LLLocalBitmapMgr::getInstance()->isLocal(sky->getCloudNoiseTextureId()))
                    {
                        desc = LLTrans::getString("EnvironmentCloudNoise");
                        is_local = true;
                    }
                    else if (LLLocalBitmapMgr::getInstance()->isLocal(sky->getBloomTextureId()))
                    {
                        desc = LLTrans::getString("EnvironmentBloom");
                        is_local = true;
                    }
                }
            }

            if (is_local)
            {
                LLSD args;
                LLButton* button = getChild<LLButton>(track_tabs[i], true);
                args["TRACK"] = button->getCurrentLabel();
                args["FRAME"] = iter->first * 100; // %
                args["FIELD"] = desc;
                args["FRAMENO"] = frame_num;
                LLNotificationsUtil::add("WLLocalTextureDayBlock", args);
                return;
            }
            iter++;
        }
    }

    if (ctrl_action == ACTION_SAVE)
    {
        doApplyUpdateInventory(dayclone);
        clearDirtyFlag();
    }
    else if (ctrl_action == ACTION_SAVEAS)
    {
        LLSD args;
        args["DESC"] = dayclone->getName();
        LLNotificationsUtil::add("SaveSettingAs", args, LLSD(), boost::bind(&LLFloaterEditExtDayCycle::onSaveAsCommit, this, _1, _2, dayclone));
    }
    else if ((ctrl_action == ACTION_APPLY_LOCAL) ||
        (ctrl_action == ACTION_APPLY_PARCEL) ||
        (ctrl_action == ACTION_APPLY_REGION))
    {
        doApplyEnvironment(ctrl_action, dayclone);
    }
    else if (ctrl_action == ACTION_COMMIT)
    {
        doApplyCommit(dayclone);
    }
    else
    {
        LL_WARNS("ENVDAYEDIT") << "Unknown settings action '" << ctrl_action << "'" << LL_ENDL;
    }
}

void LLFloaterEditExtDayCycle::onButtonLoadFrame()
{
    doOpenInventoryFloater((mCurrentTrack == LLSettingsDay::TRACK_WATER) ? LLSettingsType::ST_WATER : LLSettingsType::ST_SKY, LLUUID::null);
}

void LLFloaterEditExtDayCycle::onAddFrame()
{
    LLSettingsBase::Seconds frame(mTimeSlider->getCurSliderValue());
    LLSettingsBase::ptr_t setting;
    if (!mEditDay)
    {
        LL_WARNS("ENVDAYEDIT") << "Attempt to add new frame while waiting for day(asset) to load." << LL_ENDL;
        return;
    }
    if ((mEditDay->getSettingsNearKeyframe(frame, mCurrentTrack, LLSettingsDay::DEFAULT_FRAME_SLOP_FACTOR)).second)
    {
        LL_WARNS("ENVDAYEDIT") << "Attempt to add new frame too close to existing frame." << LL_ENDL;
        return;
    }
    if (!mFramesSlider->canAddSliders())
    {
        // Shouldn't happen, button should be disabled
        LL_WARNS("ENVDAYEDIT") << "Attempt to add new frame when slider is full." << LL_ENDL;
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
    setDirtyFlag();
    addSliderFrame(frame, setting);
    updateTabs();
}

void LLFloaterEditExtDayCycle::onRemoveFrame()
{
    std::string sldr_key = mFramesSlider->getCurSlider();
    if (sldr_key.empty())
    {
        return;
    }
    setDirtyFlag();
    removeCurrentSliderFrame();
    updateTabs();
}


void LLFloaterEditExtDayCycle::onCloneTrack()
{
    if (!mEditDay)
    {
        LL_WARNS("ENVDAYEDIT") << "Attempt to copy track while waiting for day(asset) to load." << LL_ENDL;
        return;
    }
    const LLEnvironment::altitude_list_t &altitudes = LLEnvironment::instance().getRegionAltitudes();
    bool use_altitudes = altitudes.size() > 0 && ((mEditContext == CONTEXT_PARCEL) || (mEditContext == CONTEXT_REGION));

    LLSD args = LLSD::emptyArray();

    S32 populated_counter = 0;
    for (U32 i = 1; i < LLSettingsDay::TRACK_MAX; i++)
    {
        LLSD track;
        track["id"] = LLSD::Integer(i);
        bool populated = (!mEditDay->isTrackEmpty(i)) && (i != mCurrentTrack);
        track["enabled"] = populated;
        if (populated)
        {
            populated_counter++;
        }
        if (use_altitudes)
        {
            track["altitude"] = altitudes[i - 1];
        }
        args.append(track);
    }

    if (populated_counter > 0)
    {
        doOpenTrackFloater(args);
    }
    else
    {
        // Should not happen
        LL_WARNS("ENVDAYEDIT") << "Tried to copy tracks, but there are no available sources" << LL_ENDL;
    }
}


void LLFloaterEditExtDayCycle::onLoadTrack()
{
    LLUUID curitemId = mInventoryId;

    if (mCurrentEdit && curitemId.notNull())
    {
        curitemId = LLFloaterSettingsPicker::findItemID(mCurrentEdit->getAssetId(), false, false);
    }

    doOpenInventoryFloater(LLSettingsType::ST_DAYCYCLE, curitemId);
}


void LLFloaterEditExtDayCycle::onClearTrack()
{
    if (!mEditDay)
    {
        LL_WARNS("ENVDAYEDIT") << "Attempt to clear track while waiting for day(asset) to load." << LL_ENDL;
        return;
    }

    if (mCurrentTrack > 1)
        mEditDay->getCycleTrack(mCurrentTrack).clear();
    else
    {
        LLSettingsDay::CycleTrack_t &track(mEditDay->getCycleTrack(mCurrentTrack));

        auto first = track.begin();
        auto last = track.end();
        ++first;
        track.erase(first, last);
    }

    updateEditEnvironment();
    LLEnvironment::instance().setSelectedEnvironment(LLEnvironment::ENV_EDIT, LLEnvironment::TRANSITION_INSTANT);
    LLEnvironment::instance().updateEnvironment(LLEnvironment::TRANSITION_INSTANT);
    synchronizeTabs();
    updateTabs();
    refresh();
}

void LLFloaterEditExtDayCycle::onCommitName(class LLLineEditor* caller, void* user_data)
{
    if (!mEditDay)
    {
        LL_WARNS("ENVDAYEDIT") << "Attempt to rename day while waiting for day(asset) to load." << LL_ENDL;
        return;
    }

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
    std::string curslider = mFramesSlider->getCurSlider();

    if (!curslider.empty() && mEditDay)
    {
        F32 sliderpos = mFramesSlider->getCurSliderValue();

        keymap_t::iterator it = mSliderKeyMap.find(curslider);
        if (it != mSliderKeyMap.end())
        {
            if (gKeyboard->currentMask(TRUE) == MASK_SHIFT && mShiftCopyEnabled && mCanMod)
            {
                // don't move the point/frame as long as shift is pressed and user is attempting to copy
                // handleKeyUp will do the move if user releases key too early.
                if (!(mEditDay->getSettingsNearKeyframe(sliderpos, mCurrentTrack, LLSettingsDay::DEFAULT_FRAME_SLOP_FACTOR)).second)
                {
                    LL_DEBUGS("ENVDAYEDIT") << "Copying frame from " << it->second.mFrame << " to " << sliderpos << LL_ENDL;
                    LLSettingsBase::ptr_t new_settings;

                    // mEditDay still remembers old position, add copy at new position
                    if (mCurrentTrack == LLSettingsDay::TRACK_WATER)
                    {
                        LLSettingsWaterPtr_t water_ptr = std::dynamic_pointer_cast<LLSettingsWater>(it->second.pSettings)->buildClone();
                        mEditDay->setWaterAtKeyframe(water_ptr, sliderpos);
                        new_settings = water_ptr;
                    }
                    else
                    {
                        LLSettingsSkyPtr_t sky_ptr = std::dynamic_pointer_cast<LLSettingsSky>(it->second.pSettings)->buildClone();
                        mEditDay->setSkyAtKeyframe(sky_ptr, sliderpos, mCurrentTrack);
                        new_settings = sky_ptr;
                    }
                    // mSliderKeyMap still remembers old position, for simplicity, just move it to be identical to slider
                    F32 old_frame = it->second.mFrame;
                    it->second.mFrame = sliderpos;
                    // slider already moved old frame, create new one in old place
                    addSliderFrame(old_frame, new_settings, false /*because we are going to reselect new one*/);
                    // reselect new frame
                    mFramesSlider->setCurSlider(it->first);
                    mShiftCopyEnabled = false;
                    setDirtyFlag();
                }
            }
            else
            {
                // slider rounds values to nearest increments, changes can be substanntial (half increment)
                if (abs(mFramesSlider->getNearestIncrement((*it).second.mFrame) - sliderpos) < F_APPROXIMATELY_ZERO)
                {
                    // same value
                    mFramesSlider->setCurSliderValue((*it).second.mFrame);
                }
                else if (mEditDay->moveTrackKeyframe(mCurrentTrack, (*it).second.mFrame, sliderpos) && mCanMod)
                {
                    (*it).second.mFrame = sliderpos;
                    setDirtyFlag();
                }
                else
                {
                    // same value, wrong track, no such value, no mod
                    mFramesSlider->setCurSliderValue((*it).second.mFrame);
                }

                mShiftCopyEnabled = false;
            }
        }
    }
}

void LLFloaterEditExtDayCycle::onFrameSliderDoubleClick(S32 x, S32 y, MASK mask)
{
    stopPlay();
    onAddFrame();
}

void LLFloaterEditExtDayCycle::onFrameSliderMouseDown(S32 x, S32 y, MASK mask)
{
    stopPlay();
    F32 sliderpos = mFramesSlider->getSliderValueFromPos(x, y);

    std::string slidername = mFramesSlider->getCurSlider();

    mShiftCopyEnabled = !slidername.empty() && gKeyboard->currentMask(TRUE) == MASK_SHIFT;

    if (!slidername.empty())
    {
        LLRect thumb_rect = mFramesSlider->getSliderThumbRect(slidername);
        if ((x >= thumb_rect.mRight) || (x <= thumb_rect.mLeft))
        {
            mFramesSlider->resetCurSlider();
        }
    }

    mTimeSlider->setCurSliderValue(sliderpos);

    updateTabs();
    LLEnvironment::instance().updateEnvironment(LLEnvironment::TRANSITION_INSTANT);
}

void LLFloaterEditExtDayCycle::onFrameSliderMouseUp(S32 x, S32 y, MASK mask)
{
    // Only happens when clicking on empty space of frameslider, not on specific frame
    F32 sliderpos = mFramesSlider->getSliderValueFromPos(x, y);

    mTimeSlider->setCurSliderValue(sliderpos);
    selectFrame(sliderpos, LLSettingsDay::DEFAULT_FRAME_SLOP_FACTOR);
}

void LLFloaterEditExtDayCycle::onTimeSliderCallback()
{
    stopPlay();
    selectFrame(mTimeSlider->getCurSliderValue(), LLSettingsDay::DEFAULT_FRAME_SLOP_FACTOR);
}

void LLFloaterEditExtDayCycle::cloneTrack(U32 source_index, U32 dest_index)
{
    cloneTrack(mEditDay, source_index, dest_index);
}

void LLFloaterEditExtDayCycle::cloneTrack(const LLSettingsDay::ptr_t &source_day, U32 source_index, U32 dest_index)
{
    if ((source_index == LLSettingsDay::TRACK_WATER || dest_index == LLSettingsDay::TRACK_WATER) && (source_index != dest_index))
    {   // one of the tracks is a water track, the other is not
        LLSD args;

        LL_WARNS() << "Can not import water track into sky track or vice versa" << LL_ENDL;

        LLButton* button = getChild<LLButton>(track_tabs[source_index], true);
        args["TRACK1"] = button->getCurrentLabel();
        button = getChild<LLButton>(track_tabs[dest_index], true);
        args["TRACK2"] = button->getCurrentLabel();

        LLNotificationsUtil::add("TrackLoadMismatch", args);
        return;
    }

    // don't use replaceCycleTrack because we will end up with references, but we need to clone

    // hold on to a backup of the 
    LLSettingsDay::CycleTrack_t backup_track = mEditDay->getCycleTrack(dest_index);

    mEditDay->clearCycleTrack(dest_index); // because source can be empty
    LLSettingsDay::CycleTrack_t source_track = source_day->getCycleTrack(source_index);
    S32 addcount(0);
    for (auto &track_frame : source_track)
    {
        LLSettingsBase::ptr_t pframe = track_frame.second;
        LLSettingsBase::ptr_t pframeclone = pframe->buildDerivedClone();
        if (pframeclone)
        {
            ++addcount;
            mEditDay->setSettingsAtKeyframe(pframeclone, track_frame.first, dest_index);
        }
    }

    if (!addcount)
    {   // nothing was actually added.  Restore the old track and issue a warning.
        mEditDay->replaceCycleTrack(dest_index, backup_track);

        LLSD args;
        LLButton* button = getChild<LLButton>(track_tabs[dest_index], true);
        args["TRACK"] = button->getCurrentLabel();

        LLNotificationsUtil::add("TrackLoadFailed", args);
    }
    setDirtyFlag();

    updateSlider();
    updateTabs();
    updateButtons();
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

    for (U32 i = 0; i < LLSettingsDay::TRACK_MAX; i++) // use max value
    {
        getChild<LLButton>(track_tabs[i], true)->setToggleState(i == mCurrentTrack);
    }

    bool show_water = (mCurrentTrack == LLSettingsDay::TRACK_WATER);
    mSkyTabLayoutContainer->setVisible(!show_water);
    mWaterTabLayoutContainer->setVisible(show_water);

    updateSlider();
    updateLabels();
}

void LLFloaterEditExtDayCycle::selectFrame(F32 frame, F32 slop_factor)
{
    mFramesSlider->resetCurSlider();

    keymap_t::iterator iter = mSliderKeyMap.begin();
    keymap_t::iterator end_iter = mSliderKeyMap.end();
    while (iter != end_iter)
    {
        F32 keyframe = iter->second.mFrame;
        F32 frame_dif = fabs(keyframe - frame);
        if (frame_dif <= slop_factor)
        {
            keymap_t::iterator next_iter = std::next(iter);
            if ((frame_dif != 0) && (next_iter != end_iter))
            {
                if (fabs(next_iter->second.mFrame - frame) < frame_dif)
                {
                    mFramesSlider->setCurSlider(next_iter->first);
                    frame = next_iter->second.mFrame;
                    break;
                }
            }
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
    LLPanelSettingsWaterMainTab* panel = dynamic_cast<LLPanelSettingsWaterMainTab*>(tab_container->findChildView("water_panel"));
    if (panel)
    {
        panel->setWater(p_water);
    }
}

void LLFloaterEditExtDayCycle::updateSkyTabs(const LLSettingsSkyPtr_t &p_sky)
{
    LLTabContainer* tab_container = mSkyTabLayoutContainer->getChild<LLTabContainer>(TABS_SKYS); //can't extract panels directly, since they are in 'tuple'

    LLPanelSettingsSky* panel;
    panel = dynamic_cast<LLPanelSettingsSky*>(tab_container->findChildView("atmosphere_panel"));
    if (panel)
    {
        panel->setSky(p_sky);
    }
    panel = dynamic_cast<LLPanelSettingsSky*>(tab_container->findChildView("clouds_panel"));
    if (panel)
    {
        panel->setSky(p_sky);
    }
    panel = dynamic_cast<LLPanelSettingsSky*>(tab_container->findChildView("moon_panel"));
    if (panel)
    {
        panel->setSky(p_sky);
    }
}

void LLFloaterEditExtDayCycle::updateLabels()
{
    std::string label_arg = (mCurrentTrack == LLSettingsDay::TRACK_WATER) ? "water_label" : "sky_label";

    mAddFrameButton->setLabelArg("[FRAME]", getString(label_arg));
    mDeleteFrameButton->setLabelArg("[FRAME]", getString(label_arg));
    mLoadFrame->setLabelArg("[FRAME]", getString(label_arg));
}

void LLFloaterEditExtDayCycle::updateButtons()
{
    // This logic appears to work in reverse, the add frame button
    // is only enabled when you're on an existing frame and disabled
    // in all the interim positions where you'd want to add a frame...

    bool can_manipulate = mEditDay && !mIsPlaying && mCanMod;
    bool can_clone(false);
    bool can_clear(false);

    if (can_manipulate)
    {
        if (mCurrentTrack == 0)
        {
            can_clone = false;
        }
        else
        {
            for (S32 track = 1; track < LLSettingsDay::TRACK_MAX; ++track)
            {
                if (track == mCurrentTrack)
                    continue;
                can_clone |= !mEditDay->getCycleTrack(track).empty();
            }
        }

        can_clear = (mCurrentTrack > 1) ? (!mEditDay->getCycleTrack(mCurrentTrack).empty()) : (mEditDay->getCycleTrack(mCurrentTrack).size() > 1);
    }

    mCloneTrack->setEnabled(can_clone);
    mLoadTrack->setEnabled(can_manipulate);
    mClearTrack->setEnabled(can_clear);
    mAddFrameButton->setEnabled(can_manipulate && isAddingFrameAllowed());
    mDeleteFrameButton->setEnabled(can_manipulate && isRemovingFrameAllowed());
    mLoadFrame->setEnabled(can_manipulate);

    BOOL enable_play = mEditDay ? TRUE : FALSE;
    childSetEnabled(BTN_PLAY, enable_play);
    childSetEnabled(BTN_SKIP_BACK, enable_play);
    childSetEnabled(BTN_SKIP_FORWARD, enable_play);

    // update track buttons
    bool extended_env = LLEnvironment::instance().isExtendedEnvironmentEnabled();
    for (S32 track = 0; track < LLSettingsDay::TRACK_MAX; ++track)
    {
        LLButton* button = getChild<LLButton>(track_tabs[track], true);
        button->setEnabled(extended_env);
        button->setToggleState(track == mCurrentTrack);
    }
}

void LLFloaterEditExtDayCycle::updateSlider()
{
    F32 frame_position = mTimeSlider->getCurSliderValue();
    mFramesSlider->clear();
    mSliderKeyMap.clear();

    if (!mEditDay)
    {
        // floater is waiting for asset
        return;
    }

    LLSettingsDay::CycleTrack_t track = mEditDay->getCycleTrack(mCurrentTrack);
    for (auto &track_frame : track)
    {
        addSliderFrame(track_frame.first, track_frame.second, false);
    }

    if (mSliderKeyMap.size() > 0)
    {
        // update positions
        mLastFrameSlider = mFramesSlider->getCurSlider();
    }
    else
    {
        // disable panels
        clearTabs();
        mLastFrameSlider.clear();
    }

    selectFrame(frame_position, LLSettingsDay::DEFAULT_FRAME_SLOP_FACTOR);
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

void LLFloaterEditExtDayCycle::addSliderFrame(F32 frame, const LLSettingsBase::ptr_t &setting, bool update_ui)
{
    // multi slider distinguishes elements by key/name in string format
    // store names to map to be able to recall dependencies
    std::string new_slider = mFramesSlider->addSlider(frame);
    if (!new_slider.empty())
    {
        mSliderKeyMap[new_slider] = FrameData(frame, setting);

        if (update_ui)
        {
            mLastFrameSlider = new_slider;
            mTimeSlider->setCurSliderValue(frame);
            updateTabs();
        }
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
        LL_DEBUGS("ENVDAYEDIT") << "Removing frame from " << iter->second.mFrame << LL_ENDL;
        LLSettingsBase::Seconds seconds(iter->second.mFrame);
        mEditDay->removeTrackKeyframe(mCurrentTrack, seconds);
        mSliderKeyMap.erase(iter);
    }

    mLastFrameSlider = mFramesSlider->getCurSlider();
    mTimeSlider->setCurSliderValue(mFramesSlider->getCurSliderValue());
    updateTabs();
}

void LLFloaterEditExtDayCycle::removeSliderFrame(F32 frame)
{
    keymap_t::iterator it = std::find_if(mSliderKeyMap.begin(), mSliderKeyMap.end(), 
        [frame](const keymap_t::value_type &value) { return fabs(value.second.mFrame - frame) < LLSettingsDay::DEFAULT_FRAME_SLOP_FACTOR; });

    if (it != mSliderKeyMap.end())
    {
        mFramesSlider->deleteSlider((*it).first);
        mSliderKeyMap.erase(it);
    }

}

//-------------------------------------------------------------------------

LLFloaterEditExtDayCycle::connection_t LLFloaterEditExtDayCycle::setEditCommitSignal(LLFloaterEditExtDayCycle::edit_commit_signal_t::slot_type cb)
{
    return mCommitSignal.connect(cb);
}

void LLFloaterEditExtDayCycle::updateEditEnvironment(void)
{
    if (!mEditDay)
        return;
    S32 skytrack = (mCurrentTrack) ? mCurrentTrack : 1;
    mSkyBlender = std::make_shared<LLTrackBlenderLoopingManual>(mScratchSky, mEditDay, skytrack);
    mWaterBlender = std::make_shared<LLTrackBlenderLoopingManual>(mScratchWater, mEditDay, LLSettingsDay::TRACK_WATER);

    if (LLEnvironment::instance().isExtendedEnvironmentEnabled())
    {
        selectTrack(LLSettingsDay::TRACK_MAX, true);
    }
    else
    {
        selectTrack(1, true);
    }

    reblendSettings();

    LLEnvironment::instance().setEnvironment(LLEnvironment::ENV_EDIT, mEditSky, mEditWater);
    LLEnvironment::instance().updateEnvironment(LLEnvironment::TRANSITION_INSTANT);
}

void LLFloaterEditExtDayCycle::synchronizeTabs()
{
    // This should probably get moved into "updateTabs"
    std::string curslider = mFramesSlider->getCurSlider();
    bool canedit(false);

    LLSettingsWater::ptr_t psettingW;
    LLTabContainer * tabs = mWaterTabLayoutContainer->getChild<LLTabContainer>(TABS_WATER);
    if (mCurrentTrack == LLSettingsDay::TRACK_WATER)
    {
        if (!mEditDay)
        {
            canedit = false;
        }
        else if (!curslider.empty())
        {
            canedit = !mIsPlaying;
            // either search mEditDay or retrieve from mSliderKeyMap
            keymap_t::iterator slider_it = mSliderKeyMap.find(curslider);
            if (slider_it != mSliderKeyMap.end())
            {
                psettingW = std::static_pointer_cast<LLSettingsWater>(slider_it->second.pSettings);
            }
        }
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
    mEditWater = psettingW;

    setTabsData(tabs, psettingW, canedit);

    LLSettingsSky::ptr_t psettingS;
    canedit = false;
    tabs = mSkyTabLayoutContainer->getChild<LLTabContainer>(TABS_SKYS);
    if (mCurrentTrack != LLSettingsDay::TRACK_WATER)
    {
        if (!mEditDay)
        {
            canedit = false;
        }
        else if (!curslider.empty())
        {
            canedit = !mIsPlaying;
            // either search mEditDay or retrieve from mSliderKeyMap
            keymap_t::iterator slider_it = mSliderKeyMap.find(curslider);
            if (slider_it != mSliderKeyMap.end())
            {
                psettingS = std::static_pointer_cast<LLSettingsSky>(slider_it->second.pSettings);
            }
        }
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
    mEditSky = psettingS;

    doCloseInventoryFloater();
    doCloseTrackFloater();

    setTabsData(tabs, psettingS, canedit);
    LLEnvironment::instance().setEnvironment(LLEnvironment::ENV_EDIT, mEditSky, mEditWater);
    LLEnvironment::instance().updateEnvironment(LLEnvironment::TRANSITION_INSTANT);
}

void LLFloaterEditExtDayCycle::setTabsData(LLTabContainer * tabcontainer, const LLSettingsBase::ptr_t &settings, bool editable)
{
    S32 count = tabcontainer->getTabCount();
    for (S32 idx = 0; idx < count; ++idx)
    {
        LLSettingsEditPanel *panel = static_cast<LLSettingsEditPanel *>(tabcontainer->getPanelByIndex(idx));
        if (panel)
        {
            panel->setCanChangeSettings(editable & mCanMod);
            panel->setSettings(settings);
        }
    }
}


void LLFloaterEditExtDayCycle::reblendSettings()
{
    F64 position = mTimeSlider->getCurSliderValue();

    if (mSkyBlender)
    {
        if ((mSkyBlender->getTrack() != mCurrentTrack) && (mCurrentTrack != LLSettingsDay::TRACK_WATER))
        {
            mSkyBlender->switchTrack(mCurrentTrack, position);
        }
        else
        {
            mSkyBlender->setPosition(position);
        }
    }

    if (mWaterBlender)
    {
        mWaterBlender->setPosition(position);
    }
}

void LLFloaterEditExtDayCycle::doApplyCommit(LLSettingsDay::ptr_t day)
{
    if (!mCommitSignal.empty())
    {
        mCommitSignal(day);

        closeFloater();
    }
}

bool LLFloaterEditExtDayCycle::isRemovingFrameAllowed()
{
    if (mFramesSlider->getCurSlider().empty()) return false;

    if (mCurrentTrack <= LLSettingsDay::TRACK_GROUND_LEVEL)
    {
        return (mSliderKeyMap.size() > 1);
    }
    else
    {
        return (mSliderKeyMap.size() > 0);
    }
}

bool LLFloaterEditExtDayCycle::isAddingFrameAllowed()
{
    if (!mFramesSlider->getCurSlider().empty() || !mEditDay) return false;

    LLSettingsBase::Seconds frame(mTimeSlider->getCurSliderValue());
    if ((mEditDay->getSettingsNearKeyframe(frame, mCurrentTrack, LLSettingsDay::DEFAULT_FRAME_SLOP_FACTOR)).second)
    {
        return false;
    }
    return mFramesSlider->canAddSliders();
}

void LLFloaterEditExtDayCycle::doImportFromDisk()
{   // Load a a legacy Windlight XML from disk.
    (new LLFilePickerReplyThread(boost::bind(&LLFloaterEditExtDayCycle::loadSettingFromFile, this, _1), LLFilePicker::FFLOAD_XML, false))->getFile();
}

void LLFloaterEditExtDayCycle::loadSettingFromFile(const std::vector<std::string>& filenames)
{
    LLSD messages;
    if (filenames.size() < 1) return;
    std::string filename = filenames[0];
    LL_DEBUGS("ENVDAYEDIT") << "Selected file: " << filename << LL_ENDL;
    LLSettingsDay::ptr_t legacyday = LLEnvironment::createDayCycleFromLegacyPreset(filename, messages);

    if (!legacyday)
    {   
        LLNotificationsUtil::add("WLImportFail", messages);
        return;
    }

    loadInventoryItem(LLUUID::null);

    mCurrentTrack = 1;
    setDirtyFlag();
    setEditDayCycle(legacyday);
}

void LLFloaterEditExtDayCycle::startPlay()
{
    doCloseInventoryFloater();
    doCloseTrackFloater();

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
    selectFrame(frame, LLSettingsDay::DEFAULT_FRAME_SLOP_FACTOR);

    getChild<LLView>("play_layout", true)->setVisible(TRUE);
    getChild<LLView>("pause_layout", true)->setVisible(FALSE);
}

//static
void LLFloaterEditExtDayCycle::onIdlePlay(void* user_data)
{
    if (!gDisconnected)
    {
        LLFloaterEditExtDayCycle* self = (LLFloaterEditExtDayCycle*)user_data;

        if (self->mSkyBlender == nullptr || self->mWaterBlender == nullptr)
        {
            self->stopPlay();
        }
        else
        {

            F32 prcnt_played = self->mPlayTimer.getElapsedTimeF32() / DAY_CYCLE_PLAY_TIME_SECONDS;
            F32 new_frame = fmod(self->mPlayStartFrame + prcnt_played, 1.f);

            self->mTimeSlider->setCurSliderValue(new_frame); // will do the rounding
            self->mSkyBlender->setPosition(new_frame);
            self->mWaterBlender->setPosition(new_frame);
            self->synchronizeTabs();
            self->updateTimeAndLabel();
            self->updateButtons();
        }
    }
}


void LLFloaterEditExtDayCycle::clearDirtyFlag()
{
    mIsDirty = false;

    LLTabContainer* tab_container = mSkyTabLayoutContainer->getChild<LLTabContainer>("sky_tabs");
    S32 tab_count = tab_container->getTabCount();

    for (S32 idx = 0; idx < tab_count; ++idx)
    {
        LLSettingsEditPanel *panel = static_cast<LLSettingsEditPanel *>(tab_container->getPanelByIndex(idx));
        if (panel)
            panel->clearIsDirty();
    }

    tab_container = mWaterTabLayoutContainer->getChild<LLTabContainer>("water_tabs");
    tab_count = tab_container->getTabCount();

    for (S32 idx = 0; idx < tab_count; ++idx)
    {
        LLSettingsEditPanel *panel = static_cast<LLSettingsEditPanel *>(tab_container->getPanelByIndex(idx));
        if (panel)
            panel->clearIsDirty();
    }

}

void LLFloaterEditExtDayCycle::doOpenTrackFloater(const LLSD &args)
{
    LLFloaterTrackPicker *picker = static_cast<LLFloaterTrackPicker *>(mTrackFloater.get());

    // Show the dialog
    if (!picker)
    {
        picker = new LLFloaterTrackPicker(this);

        mTrackFloater = picker->getHandle();

        picker->setCommitCallback([this](LLUICtrl *, const LLSD &data){ onPickerCommitTrackId(data.asInteger()); });
    }

    picker->showPicker(args);
}

void LLFloaterEditExtDayCycle::doCloseTrackFloater(bool quitting)
{
    LLFloater* floaterp = mTrackFloater.get();

    if (floaterp)
    {
        floaterp->closeFloater(quitting);
    }
}

LLFloaterSettingsPicker * LLFloaterEditExtDayCycle::getSettingsPicker()
{
    LLFloaterSettingsPicker *picker = static_cast<LLFloaterSettingsPicker *>(mInventoryFloater.get());

    // Show the dialog
    if (!picker)
    {
        picker = new LLFloaterSettingsPicker(this,
            LLUUID::null);

        mInventoryFloater = picker->getHandle();

        picker->setCommitCallback([this](LLUICtrl *, const LLSD &data){ onPickerCommitSetting(data["ItemId"].asUUID(), data["Track"].asInteger()); });
    }
    return picker;
}

void LLFloaterEditExtDayCycle::onPickerCommitTrackId(U32 track_id)
{
    cloneTrack(track_id, mCurrentTrack);
}

void LLFloaterEditExtDayCycle::doOpenInventoryFloater(LLSettingsType::type_e type, LLUUID curritem)
{
    LLFloaterSettingsPicker *picker = getSettingsPicker();
    picker->setSettingsFilter(type);
    picker->setSettingsItemId(curritem);
    if (type == LLSettingsType::ST_DAYCYCLE)
    {
        picker->setTrackMode((mCurrentTrack == LLSettingsDay::TRACK_WATER) ? LLFloaterSettingsPicker::TRACK_WATER : LLFloaterSettingsPicker::TRACK_SKY);
    }
    else
    {
        picker->setTrackMode(LLFloaterSettingsPicker::TRACK_NONE);
    }
    picker->openFloater();
    picker->setFocus(TRUE);
}

void LLFloaterEditExtDayCycle::onPickerCommitSetting(LLUUID item_id, S32 track)
{
    LLSettingsBase::TrackPosition frame(mTimeSlider->getCurSliderValue());
    LLViewerInventoryItem *itemp = gInventory.getItem(item_id);
    if (itemp)
    {
        LLSettingsVOBase::getSettingsAsset(itemp->getAssetUUID(),
            [this, track, frame, item_id](LLUUID asset_id, LLSettingsBase::ptr_t settings, S32 status, LLExtStat) { onAssetLoadedForInsertion(item_id, asset_id, settings, status, track, mCurrentTrack, frame); });
    }
}

void LLFloaterEditExtDayCycle::onAssetLoadedForInsertion(LLUUID item_id, LLUUID asset_id, LLSettingsBase::ptr_t settings, S32 status, S32 source_track, S32 dest_track, LLSettingsBase::TrackPosition frame)
{
    std::function<void()> cb = [this, settings, frame, source_track, dest_track]()
    {
        if (settings->getSettingsType() == "daycycle")
        {
            // Load full track
            LLSettingsDay::ptr_t pday = std::dynamic_pointer_cast<LLSettingsDay>(settings);
            if (dest_track == LLSettingsDay::TRACK_WATER)
            {
                cloneTrack(pday, LLSettingsDay::TRACK_WATER, LLSettingsDay::TRACK_WATER);
            }
            else
            {
                cloneTrack(pday, source_track, dest_track);
            }
        }
        else
        {
            if (!mFramesSlider->canAddSliders())
            {
                LL_WARNS("ENVDAYEDIT") << "Attempt to add new frame when slider is full." << LL_ENDL;
                return;
            }

            // load or replace single frame
            LLSettingsDay::CycleTrack_t::value_type nearest = mEditDay->getSettingsNearKeyframe(frame, dest_track, LLSettingsDay::DEFAULT_FRAME_SLOP_FACTOR);
            if (nearest.first != LLSettingsDay::INVALID_TRACKPOS)
            {   // There is already a frame near the target location. Remove it so we can put the new one in its place.
                mEditDay->removeTrackKeyframe(dest_track, nearest.first);
                removeSliderFrame(nearest.first);
            }

            // Don't forget to clone (we might reuse/load it couple times)
            if (settings->getSettingsType() == "sky")
            {
                // Load sky to frame
                if (dest_track != LLSettingsDay::TRACK_WATER)
                {
                    mEditDay->setSettingsAtKeyframe(settings->buildDerivedClone(), frame, dest_track);
                    addSliderFrame(frame, settings, false);
                }
                else
                {
                    LL_WARNS("ENVDAYEDIT") << "Trying to load day settings as sky" << LL_ENDL;
                }
            }
            else if (settings->getSettingsType() == "water")
            {
                // Load water to frame
                if (dest_track == LLSettingsDay::TRACK_WATER)
                {
                    mEditDay->setSettingsAtKeyframe(settings->buildDerivedClone(), frame, dest_track);
                    addSliderFrame(frame, settings, false);
                }
                else
                {
                    LL_WARNS("ENVDAYEDIT") << "Trying to load water settings as sky" << LL_ENDL;
                }
            }
        }
        reblendSettings();
        synchronizeTabs();
    };

    if (!settings || status)
    {
        LL_WARNS("ENVDAYEDIT") << "Could not load asset " << asset_id << " into frame. status=" << status << LL_ENDL;
        return;
    }

    if (!mEditDay)
    {
        // day got reset while we were waiting for response
        return;
    }

    LLInventoryItem *inv_item = gInventory.getItem(item_id);

    if (inv_item
        && (!inv_item->getPermissions().allowOperationBy(PERM_TRANSFER, gAgent.getID())
            || !inv_item->getPermissions().allowOperationBy(PERM_COPY, gAgent.getID())))
    {
        // Need to check if item is already no-transfer, otherwise make it no-transfer
        bool no_transfer = false;
        if (mInventoryItem)
        {
            no_transfer = !mInventoryItem->getPermissions().allowOperationBy(PERM_TRANSFER, gAgent.getID());
        }
        else
        {
            no_transfer = mEditDay->getFlag(LLSettingsBase::FLAG_NOTRANS);
        }

        if (!no_transfer)
        {
            LLSD args;

            // create and show confirmation textbox
            LLNotificationsUtil::add("SettingsMakeNoTrans", args, LLSD(),
                [this, cb](const LLSD&notif, const LLSD&resp)
            {
                S32 opt = LLNotificationsUtil::getSelectedOption(notif, resp);
                if (opt == 0)
                {
                    mCanTrans = false;
                    mEditDay->setFlag(LLSettingsBase::FLAG_NOTRANS);
                    cb();
                }
            });
            return;
        }
    }
    
    cb();
}
