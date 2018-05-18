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
#include "llinventorymodel.h"

// newview
#include "llagent.h"
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


//=========================================================================
// **RIDER**

const std::string LLFloaterFixedEnvironment::KEY_INVENTORY_ID("inventory_id");
const std::string LLFloaterFixedEnvironment::KEY_LIVE_ENVIRONMENT("live_environment");
const std::string LLFloaterFixedEnvironment::KEY_DAY_LENGTH("day_length");
const std::string LLFloaterFixedEnvironment::KEY_DAY_OFFSET("day_offset");

// **RIDER**

LLFloaterEditExtDayCycle::LLFloaterEditExtDayCycle(const LLSD &key):	
    LLFloater(key),
    mSaveButton(NULL),
    mCancelButton(NULL),
    mDayLength(0),
    mDayOffset(0),
    mCurrentTrack(4),
    mTimeSlider(NULL),
    mFramesSlider(NULL),
    mCurrentTimeLabel(NULL),
    // **RIDER**
    mInventoryId(),
    mInventoryItem(nullptr)
    // **RIDER**
// ,	mTimeCtrl(NULL)
// ,	mMakeDefaultCheckBox(NULL)
// ,	
{
    mCommitCallbackRegistrar.add("DayCycle.Track", boost::bind(&LLFloaterEditExtDayCycle::onTrackSelectionCallback, this, _2));
}

// virtual
BOOL LLFloaterEditExtDayCycle::postBuild()
{
    getChild<LLButton>("add_frame")->setCommitCallback(boost::bind(&LLFloaterEditExtDayCycle::onAddTrack, this));
    getChild<LLButton>("delete_frame")->setCommitCallback(boost::bind(&LLFloaterEditExtDayCycle::onRemoveTrack, this));
    getChild<LLLineEditor>("day_cycle_name")->setKeystrokeCallback(boost::bind(&LLFloaterEditExtDayCycle::onCommitName, this, _1, _2), NULL);

    mSaveButton = getChild<LLButton>("save_btn", true);
    mCancelButton = getChild<LLButton>("cancel_btn", true);
    mUploadButton = getChild<LLButton>("upload_btn", true);
    mTimeSlider = getChild<LLMultiSliderCtrl>("WLTimeSlider");
    mFramesSlider = getChild<LLMultiSliderCtrl>("WLDayCycleFrames");
    mSkyTabLayoutContainer = getChild<LLView>("frame_settings_sky", true);
    mWaterTabLayoutContainer = getChild<LLView>("frame_settings_water", true);
    mCurrentTimeLabel = getChild<LLTextBox>("current_time", true);

    //initCallbacks();

    mSaveButton->setCommitCallback(boost::bind(&LLFloaterEditExtDayCycle::onBtnSave, this));
    mCancelButton->setCommitCallback(boost::bind(&LLFloaterEditExtDayCycle::onBtnCancel, this));
    mUploadButton->setCommitCallback(boost::bind(&LLFloaterEditExtDayCycle::onUpload, this));
    mTimeSlider->setCommitCallback(boost::bind(&LLFloaterEditExtDayCycle::onTimeSliderMoved, this));
    mFramesSlider->setCommitCallback(boost::bind(&LLFloaterEditExtDayCycle::onFrameSliderCallback, this));

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

    mDayLength.value(0);
    mDayOffset.value(0);
    if (key.has(KEY_DAY_LENGTH))
    {
        mDayLength.value(key[KEY_DAY_LENGTH].asReal());
        mDayOffset.value(key[KEY_DAY_OFFSET].asReal());
    }

    // **RIDER**

    LLLineEditor* name_field = getChild<LLLineEditor>("day_cycle_name");
    name_field->setText(mEditDay->getName());

    selectTrack(mCurrentTrack);

    // time labels
    mCurrentTimeLabel->setTextArg("[PRCNT]", std::string("0"));
    const S32 max_elm = 5;
    if (mDayLength.value() != 0)
    {
        F32Hours hrs;
        LLUIString formatted_label = getString("time_label");
        for (int i = 0; i < max_elm; i++)
        {
            // Todo: if possible, find a better way, both to get string and child names
            hrs = ((mDayLength / (max_elm - 1)) * i) + mDayOffset;
            formatted_label.setArg("[TIME]", llformat("%.1f", hrs.value()));
            getChild<LLTextBox>("p" + llformat("%d", i), true)->setTextArg("[DSC]", formatted_label.getString());
        }
        hrs = mDayOffset;
        formatted_label.setArg("[TIME]", llformat("%.1f", hrs.value()));
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

	//todo: add time slider setting

    /*list_name_id_t              getSkyList() const;
    list_name_id_t              getWaterList() const;

    getChild<LLButton>("sky4_track", true)->setToggleState(true);*/
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


void LLFloaterEditExtDayCycle::onUpload()
{
    LLSettingsVOBase::createInventoryItem( mEditDay );

#if 0
    LLSettingsVOBase::storeAsAsset(mEditDay);

    LLTransactionID tid;
    tid.generate();
    LLAssetID new_asset_id = tid.makeAssetID(gAgent.getSecureSessionID());

    const std::string filename = asset_id_to_filename(mAssetID, LL_PATH_CACHE);
    if (!exportFile(filename))
    {
        std::string buffer = llformat("Unable to save '%s' to wearable file.", mName.c_str());
        LL_WARNS() << buffer << LL_ENDL;

        LLSD args;
        args["NAME"] = mName;
        LLNotificationsUtil::add("CannotSaveWearableOutOfSpace", args);
        return;
    }

    if (gSavedSettings.getBOOL("LogWearableAssetSave"))
    {
        const std::string log_filename = asset_id_to_filename(mAssetID, LL_PATH_LOGS);
        exportFile(log_filename);
    }

    // save it out to database
    if (gAssetStorage)
    {
        /*
        std::string url = gAgent.getRegion()->getCapability("NewAgentInventory");
        if (!url.empty())
        {
        LL_INFOS() << "Update Agent Inventory via capability" << LL_ENDL;
        LLSD body;
        body["folder_id"] = gInventory.findCategoryUUIDForType(LLFolderType::assetToFolderType(getAssetType()));
        body["asset_type"] = LLAssetType::lookup(getAssetType());
        body["inventory_type"] = LLInventoryType::lookup(LLInventoryType::IT_WEARABLE);
        body["name"] = getName();
        body["description"] = getDescription();
        LLHTTPClient::post(url, body, new LLNewAgentInventoryResponder(body, filename));
        }
        else
        {
        }
        */
        LLWearableSaveData* data = new LLWearableSaveData;
        data->mType = mType;
        gAssetStorage->storeAssetData(filename, mTransactionID, getAssetType(),
            &LLViewerWearable::onSaveNewAssetComplete,
            (void*)data);
    }
#endif
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

void LLFloaterEditExtDayCycle::onBtnSave()
{
    if (!mCommitSignal.empty())
        mCommitSignal(mEditDay);
    closeFloater();
}

void LLFloaterEditExtDayCycle::onBtnCancel()
{
	closeFloater();
}

void LLFloaterEditExtDayCycle::onAddTrack()
{
    F32 frame = mTimeSlider->getCurSliderValue();
    LLSettingsBase::ptr_t setting;
    // todo: expand to generate from panels instead of using defaults
    if (mEditDay->getSettingsAtKeyframe(frame, mCurrentTrack).get() != NULL) //temp check? todo: disable button in such cases
    {
        return;
    }
    if (mCurrentTrack == 0)
    {
        setting = LLSettingsVOWater::buildDefaultWater();
        mEditDay->setWaterAtKeyframe(std::dynamic_pointer_cast<LLSettingsWater>(setting), frame);
    }
    else
    {
        setting = LLSettingsVOSky::buildDefaultSky();
        mEditDay->setSkyAtKeyframe(std::dynamic_pointer_cast<LLSettingsSky>(setting), frame, mCurrentTrack);
    }
    addSliderFrame(frame, setting);
    updateTabs();
}

void LLFloaterEditExtDayCycle::onRemoveTrack()
{
    F32 frame = mTimeSlider->getCurSliderValue();
    mEditDay->removeTrackKeyframe(mCurrentTrack, frame);
    removeCurrentSliderFrame();
    //mFramesSlider->deleteCurSlider();
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
    if (mFramesSlider->getValue().size() == 0)
    {
        mLastFrameSlider.clear();
        return;
    }
    // make sure we have a slider
    const std::string& cur_sldr = mFramesSlider->getCurSlider();
    if (cur_sldr == "")
    {
        mLastFrameSlider.clear();
        return;
    }

    F32 new_frame = mFramesSlider->getCurSliderValue();
    // todo: add safety checks, user shouldn't be capable of moving one frame over another or move missing frame
    keymap_t::iterator iter = mSliderKeyMap.find(cur_sldr);
    if (iter != mSliderKeyMap.end() && /*temp? until disabling mechanics*/ mEditDay->getSettingsAtKeyframe(new_frame, mCurrentTrack).get() == NULL)
    {
        LL_DEBUGS() << "Moving frame from " << iter->second.first << " to " << new_frame << LL_ENDL;
        if (mEditDay->moveTrackKeyframe(mCurrentTrack, iter->second.first, new_frame))
        {
            iter->second.first = new_frame;
        }
    }

    if (mLastFrameSlider != cur_sldr)
    {
        // technically should not be possible for both frame and slider to change
        // but for safety, assume that they can change independently and both
        mLastFrameSlider = cur_sldr;
        updateTabs();
    }
    // 	updateTrack();
    // reset last known position
}

void LLFloaterEditExtDayCycle::onTimeSliderMoved()
{
    // Todo: safety checks
    // Update label
    F32 time = mTimeSlider->getCurSliderValue();
    mCurrentTimeLabel->setTextArg("[PRCNT]", llformat("%.0f", time * 100));
    if (mDayLength.value() != 0)
    {
        F32Hours hrs = (mDayLength  * time) + mDayOffset;
        LLUIString formatted_label = getString("time_label");
        formatted_label.setArg("[TIME]", llformat("%.1f", hrs.value()));
        mCurrentTimeLabel->setTextArg("[DSC]", formatted_label.getString());
    }
    else
    {
        mCurrentTimeLabel->setTextArg("[DSC]", std::string());
    }

    //Todo: update something related to time/play?
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

    for (int i = 0; i < 5; i++) // yse max value
    {
        getChild<LLButton>(track_tabs[i], true)->setToggleState(false);
    }

    button->setToggleState(true);

    bool show_water = mCurrentTrack == 0;
    mSkyTabLayoutContainer->setVisible(!show_water);
    mWaterTabLayoutContainer->setVisible(show_water);
    updateSlider();
}

void LLFloaterEditExtDayCycle::updateTabs()
{
    if (mCurrentTrack == 0)
    {
        updateWaterTabs();
    }
    else
    {
        updateSkyTabs();
    }
}

void LLFloaterEditExtDayCycle::updateWaterTabs()
{
    // todo: substitute with mSliderKeyMap?
    const LLSettingsWaterPtr_t p_water = mEditDay->getWaterAtKeyframe(mFramesSlider->getCurSliderValue());

    // Compiler warnings from getChild about LLPanelSettingsWaterMainTab not being complete/missing params constructor...
    // Todo: fix class to work with getChild()?
    LLView* tab_container = mWaterTabLayoutContainer->getChild<LLView>("water_tabs"); //can't extract panels directly, since they are in 'tuple'
    LLPanelSettingsWaterMainTab* panel = dynamic_cast<LLPanelSettingsWaterMainTab*>(tab_container->getChildView("water_panel"));
    if (panel)
    {
        panel->setWater(p_water); // todo: Null disables
    }
}

void LLFloaterEditExtDayCycle::updateSkyTabs()
{
    // todo: substitute with mSliderKeyMap?
    const LLSettingsSkyPtr_t p_sky = mEditDay->getSkyAtKeyframe(mFramesSlider->getCurSliderValue(), mCurrentTrack);

    LLView* tab_container = mSkyTabLayoutContainer->getChild<LLView>("sky_tabs"); //can't extract panels directly, since they are in 'tuple'

    // Compiler warnings from getChild about tabs...
    // Todo: fix class
    LLPanelSettingsSky* panel;
    panel = dynamic_cast<LLPanelSettingsSky*>(tab_container->getChildView("atmosphere_panel"));
    if (panel)
    {
        panel->setSky(p_sky); // todo: Null disables
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

void LLFloaterEditExtDayCycle::updateSlider()
{
    mFramesSlider->clear();
    mSliderKeyMap.clear();

    LLSettingsDay::CycleTrack_t track = mEditDay->getCycleTrack(mCurrentTrack);
    for (auto &frame : track)
    {
        addSliderFrame(frame.first, frame.second);
    }
}

void LLFloaterEditExtDayCycle::addSliderFrame(const F32 frame, LLSettingsBase::ptr_t setting)
{
    // multi slider distinguishes elements by key/name in string format
    // store names to map to be able to recal dependencies
    std::string new_slider = mFramesSlider->addSlider(frame);
    mSliderKeyMap[new_slider] = framedata_t(frame, setting);
    mLastFrameSlider = new_slider;

    updateTabs();
}

void LLFloaterEditExtDayCycle::removeCurrentSliderFrame()
{
    std::string sldr = mFramesSlider->getCurSlider();
    mFramesSlider->deleteCurSlider();
    keymap_t::iterator iter = mSliderKeyMap.find(sldr);
    if (iter != mSliderKeyMap.end())
    {
        LL_DEBUGS() << "Removing frame from " << iter->second.first << LL_ENDL;
        mSliderKeyMap.erase(iter);
    }

    updateTabs();
}

/*void LLFloaterEditExtDayCycle::updateTrack()
{
	LLMultiSliderCtrl* slider = getChild<LLMultiSliderCtrl>("WLDayCycleKeys");
	//mEditDay->getTrackKeyframes

	// todo make tracks named to allow movement 
}*/

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
    mEditDay = settings;
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
            mEditDay = day;
            break;
        }
    }

    updateEditEnvironment();
    syncronizeTabs();
    refresh();
}

void LLFloaterEditExtDayCycle::updateEditEnvironment(void)
{
    LLEnvironment::instance().setEnvironment(LLEnvironment::ENV_EDIT, mEditDay);
}

void LLFloaterFixedEnvironment::syncronizeTabs()
{
    LLView* tab_container = mWaterTabLayoutContainer->getChild<LLView>("water_tabs"); //can't extract panels directly, since they are in 'tuple'

    S32 count = mTab->getTabCount();

    for (S32 idx = 0; idx < count; ++idx)
    {
        LLSettingsEditPanel *panel = static_cast<LLSettingsEditPanel *>(mTab->getPanelByIndex(idx));
        if (panel)
            panel->setSettings(mSettings);
    }
}

// **RIDER**


// 
// virtual
// void LLFloaterEditExtDayCycle::draw()
// {
// 	syncTimeSlider();
// 	LLFloater::draw();
// }
// 
// void LLFloaterEditExtDayCycle::initCallbacks(void)
// {
// #if 0
// 	mDayCycleNameEditor->setKeystrokeCallback(boost::bind(&LLFloaterEditExtDayCycle::onDayCycleNameEdited, this), NULL);
// 	mDayCyclesCombo->setCommitCallback(boost::bind(&LLFloaterEditExtDayCycle::onDayCycleSelected, this));
// 	mDayCyclesCombo->setTextEntryCallback(boost::bind(&LLFloaterEditExtDayCycle::onDayCycleNameEdited, this));
// 	mTimeSlider->setCommitCallback(boost::bind(&LLFloaterEditExtDayCycle::onTimeSliderMoved, this));
// 	mKeysSlider->setCommitCallback(boost::bind(&LLFloaterEditExtDayCycle::onKeyTimeMoved, this));
// 	mTimeCtrl->setCommitCallback(boost::bind(&LLFloaterEditExtDayCycle::onKeyTimeChanged, this));
// 	mSkyPresetsCombo->setCommitCallback(boost::bind(&LLFloaterEditExtDayCycle::onKeyPresetChanged, this));
// 
// 	getChild<LLButton>("WLAddKey")->setClickedCallback(boost::bind(&LLFloaterEditExtDayCycle::onAddKey, this));
// 	getChild<LLButton>("WLDeleteKey")->setClickedCallback(boost::bind(&LLFloaterEditExtDayCycle::onDeleteKey, this));
// 
// 	mSaveButton->setCommitCallback(boost::bind(&LLFloaterEditExtDayCycle::onBtnSave, this));
// 	mSaveButton->setRightMouseDownCallback(boost::bind(&LLFloaterEditExtDayCycle::dumpTrack, this));
// 	getChild<LLButton>("cancel")->setCommitCallback(boost::bind(&LLFloaterEditExtDayCycle::onBtnCancel, this));
// 
// 	// Connect to env manager events.
// 	LLEnvManagerNew& env_mgr = LLEnvManagerNew::instance();
// 	env_mgr.setRegionSettingsChangeCallback(boost::bind(&LLFloaterEditExtDayCycle::onRegionSettingsChange, this));
// 	gAgent.addRegionChangedCallback(boost::bind(&LLFloaterEditExtDayCycle::onRegionChange, this));
// 	env_mgr.setRegionSettingsAppliedCallback(boost::bind(&LLFloaterEditExtDayCycle::onRegionSettingsApplied, this, _1));
// 	// Connect to day cycle manager events.
// 	LLDayCycleManager::instance().setModifyCallback(boost::bind(&LLFloaterEditExtDayCycle::onDayCycleListChange, this));
// 
// 	// Connect to sky preset list changes.
// 	LLWLParamManager::instance().setPresetListChangeCallback(boost::bind(&LLFloaterEditExtDayCycle::onSkyPresetListChange, this));
// 
// 
// 	// Connect to region info updates.
// 	LLRegionInfoModel::instance().setUpdateCallback(boost::bind(&LLFloaterEditExtDayCycle::onRegionInfoUpdate, this));
// #endif
// }
// 
// void LLFloaterEditExtDayCycle::syncTimeSlider()
// {
// #if 0
// 	// set time
// 	mTimeSlider->setCurSliderValue((F32)LLWLParamManager::getInstance()->mAnimator.getDayTime() * sHoursPerDay);
// #endif
// }
// 
// void LLFloaterEditExtDayCycle::loadTrack()
// {
// 	// clear the slider
// 	mKeysSlider->clear();
// 	mSliderToKey.clear();
// 
// 	// add sliders
// 
// 	LL_DEBUGS() << "Adding " << LLWLParamManager::getInstance()->mDay.mTimeMap.size() << " keys to slider" << LL_ENDL;
// 
// 	LLWLDayCycle& cur_dayp = LLWLParamManager::instance().mDay;
// 	for (std::map<F32, LLWLParamKey>::iterator it = cur_dayp.mTimeMap.begin(); it != cur_dayp.mTimeMap.end(); ++it)
// 	{
// 		addSliderKey(it->first * sHoursPerDay, it->second);
// 	}
// 
// 	// set drop-down menu to match preset of currently-selected keyframe (one is automatically selected initially)
// 	const std::string& cur_sldr = mKeysSlider->getCurSlider();
// 	if (strlen(cur_sldr.c_str()) > 0)	// only do this if there is a curSldr, otherwise we put an invalid entry into the map
// 	{
// 		mSkyPresetsCombo->selectByValue(mSliderToKey[cur_sldr].keyframe.toStringVal());
// 	}
// 
// 	syncTimeSlider();
// }
// 
// void LLFloaterEditExtDayCycle::applyTrack()
// {
// #if 0
// 	LL_DEBUGS() << "Applying track (" << mSliderToKey.size() << ")" << LL_ENDL;
// 
// 	// if no keys, do nothing
// 	if (mSliderToKey.size() == 0)
// 	{
// 		LL_DEBUGS() << "No keys, not syncing" << LL_ENDL;
// 		return;
// 	}
// 
// 	llassert_always(mSliderToKey.size() == mKeysSlider->getValue().size());
// 
// 	// create a new animation track
// 	LLWLParamManager::getInstance()->mDay.clearKeyframes();
// 
// 	// add the keys one by one
// 	for (std::map<std::string, SliderKey>::iterator it = mSliderToKey.begin();
// 		it != mSliderToKey.end(); ++it)
// 	{
// 		LLWLParamManager::getInstance()->mDay.addKeyframe(it->second.time / sHoursPerDay,
// 			it->second.keyframe);
// 	}
// 
// 	// set the param manager's track to the new one
// 	LLWLParamManager::getInstance()->resetAnimator(
// 		mTimeSlider->getCurSliderValue() / sHoursPerDay, false);
// 
// 	LLWLParamManager::getInstance()->mAnimator.update(
// 		LLWLParamManager::getInstance()->mCurParams);
// #endif
// }

// void LLFloaterEditExtDayCycle::refreshDayCyclesList()
// {
// #if 0
// 	llassert(isNewDay() == false);
// 
// 	mDayCyclesCombo->removeall();
// 
// #if 0 // Disable editing existing day cycle until the workflow is clear enough.
// 	const LLSD& region_day = LLEnvManagerNew::instance().getRegionSettings().getWLDayCycle();
// 	if (region_day.size() > 0)
// 	{
// 		LLWLParamKey key(getRegionName(), LLEnvKey::SCOPE_REGION);
// 		mDayCyclesCombo->add(key.name, key.toLLSD());
// 		mDayCyclesCombo->addSeparator();
// 	}
// #endif
// 
// 	LLDayCycleManager::preset_name_list_t user_days, sys_days;
// 	LLDayCycleManager::instance().getPresetNames(user_days, sys_days);
// 
// 	// Add user days.
// 	for (LLDayCycleManager::preset_name_list_t::const_iterator it = user_days.begin(); it != user_days.end(); ++it)
// 	{
// 		mDayCyclesCombo->add(*it, LLWLParamKey(*it, LLEnvKey::SCOPE_LOCAL).toLLSD());
// 	}
// 
// 	if (user_days.size() > 0)
// 	{
// 		mDayCyclesCombo->addSeparator();
// 	}
// 
// 	// Add system days.
// 	for (LLDayCycleManager::preset_name_list_t::const_iterator it = sys_days.begin(); it != sys_days.end(); ++it)
// 	{
// 		mDayCyclesCombo->add(*it, LLWLParamKey(*it, LLEnvKey::SCOPE_LOCAL).toLLSD());
// 	}
// 
// 	mDayCyclesCombo->setLabel(getString("combo_label"));
// #endif
// }
// 
// 
// 
// void LLFloaterEditExtDayCycle::onKeyTimeChanged()
// {
// #if 0
// 	// if no keys, skipped
// 	if (mSliderToKey.size() == 0)
// 	{
// 		return;
// 	}
// 
// 	F32 time24 = mTimeCtrl->getTime24();
// 
// 	const std::string& cur_sldr = mKeysSlider->getCurSlider();
// 	mKeysSlider->setCurSliderValue(time24, TRUE);
// 	F32 time = mKeysSlider->getCurSliderValue() / sHoursPerDay;
// 
// 	// now set the key's time in the sliderToKey map
// 	LL_DEBUGS() << "Setting key time: " << time << LL_ENDL;
// 	mSliderToKey[cur_sldr].time = time;
// 
// 	applyTrack();
// #endif
// }
// 
// 
// void LLFloaterEditExtDayCycle::onAddKey()
// {
// #if 0
// 	llassert_always(mSliderToKey.size() == mKeysSlider->getValue().size());
// 
// 	S32 max_sliders;
// 	LLEnvKey::EScope scope = LLEnvKey::SCOPE_LOCAL; // *TODO: editing region day cycle
// 	switch (scope)
// 	{
// 		case LLEnvKey::SCOPE_LOCAL:
// 			max_sliders = 20; // *HACK this should be LLWLPacketScrubber::MAX_LOCAL_KEY_FRAMES;
// 			break;
// 		case LLEnvKey::SCOPE_REGION:
// 			max_sliders = 12; // *HACK this should be LLWLPacketScrubber::MAX_REGION_KEY_FRAMES;
// 			break;
// 		default:
// 			max_sliders = (S32) mKeysSlider->getMaxValue();
// 			break;
// 	}
// 
// #if 0
// 	if ((S32)mSliderToKey.size() >= max_sliders)
// 	{
// 		LLSD args;
// 		args["SCOPE"] = LLEnvManagerNew::getScopeString(scope);
// 		args["MAX"] = max_sliders;
// 		LLNotificationsUtil::add("DayCycleTooManyKeyframes", args, LLSD(), LLNotificationFunctorRegistry::instance().DONOTHING);
// 		return;
// 	}
// #endif
// 
// 	// add the slider key
// 	std::string key_val = mSkyPresetsCombo->getSelectedValue().asString();
// 	LLWLParamKey sky_params(key_val);
// 	llassert(!sky_params.name.empty());
// 
// 	F32 time = mTimeSlider->getCurSliderValue();
// 	addSliderKey(time, sky_params);
// 
// 	// apply the change to current day cycles
// 	applyTrack();
// #endif
// }
// 

// #if 0
// LLWLParamKey LLFloaterEditExtDayCycle::getSelectedDayCycle()
// {
// 	LLWLParamKey dc_key;
// 
// 	if (mDayCycleNameEditor->getVisible())
// 	{
// 		dc_key.name = mDayCycleNameEditor->getText();
// 		dc_key.scope = LLEnvKey::SCOPE_LOCAL;
// 	}
// 	else
// 	{
// 		LLSD combo_val = mDayCyclesCombo->getValue();
// 
// 		if (!combo_val.isArray()) // manually typed text
// 		{
// 			dc_key.name = combo_val.asString();
// 			dc_key.scope = LLEnvKey::SCOPE_LOCAL;
// 		}
// 		else
// 		{
// 			dc_key.fromLLSD(combo_val);
// 		}
// 	}
// 
// 	return dc_key;
// }
// #endif
// 
// bool LLFloaterEditExtDayCycle::isNewDay() const
// {
// 	return mKey.asString() == "new";
// }
// 
// void LLFloaterEditExtDayCycle::dumpTrack()
// {
// #if 0
// 	LL_DEBUGS("Windlight") << "Dumping day cycle" << LL_ENDL;
// 
// 	LLWLDayCycle& cur_dayp = LLWLParamManager::instance().mDay;
// 	for (std::map<F32, LLWLParamKey>::iterator it = cur_dayp.mTimeMap.begin(); it != cur_dayp.mTimeMap.end(); ++it)
// 	{
// 		F32 time = it->first * 24.0f;
// 		S32 h = (S32) time;
// 		S32 m = (S32) ((time - h) * 60.0f);
// 		LL_DEBUGS("Windlight") << llformat("(%.3f) %02d:%02d", time, h, m) << " => " << it->second.name << LL_ENDL;
// 	}
// #endif
// }
// 
// void LLFloaterEditExtDayCycle::enableEditing(bool enable)
// {
// 	mSkyPresetsCombo->setEnabled(enable);
// 	mTimeCtrl->setEnabled(enable);
// 	getChild<LLPanel>("day_cycle_slider_panel")->setCtrlsEnabled(enable);
// 	mSaveButton->setEnabled(enable);
// 	mMakeDefaultCheckBox->setEnabled(enable);
// }
// 
// void LLFloaterEditExtDayCycle::reset()
// {
// #if 0
// 	// clear the slider
// 	mKeysSlider->clear();
// 	mSliderToKey.clear();
// 
// 	refreshSkyPresetsList();
// 
// 	if (isNewDay())
// 	{
// 		mDayCycleNameEditor->setValue(LLSD());
// 		F32 time = 0.5f * sHoursPerDay;
// 		mSaveButton->setEnabled(FALSE); // will be enabled as soon as users enters a name
// 		mTimeSlider->setCurSliderValue(time);
// 
// 		addSliderKey(time, LLWLParamKey("Default", LLEnvKey::SCOPE_LOCAL));
// 		onKeyTimeMoved(); // update the time control and sky sky combo
// 
// 		applyTrack();
// 	}
// 	else
// 	{
// 		refreshDayCyclesList();
// 
// 		// Disable controls until a day cycle  to edit is selected.
// 		enableEditing(false);
// 	}
// #endif
// }
// 
// void LLFloaterEditExtDayCycle::saveRegionDayCycle()
// {
// #if 0
// 	LLEnvManagerNew& env_mgr = LLEnvManagerNew::instance();
// 	LLWLDayCycle& cur_dayp = LLWLParamManager::instance().mDay; // the day cycle being edited
// 
// 	// Get current day cycle and the sky preset it references.
// 	LLSD day_cycle = cur_dayp.asLLSD();
// 	LLSD sky_map;
// 	cur_dayp.getSkyMap(sky_map);
// 
// 	// Apply it to the region.
// 	LLEnvironmentSettings new_region_settings;
// 	new_region_settings.saveParams(day_cycle, sky_map, env_mgr.getRegionSettings().getWaterParams(), 0.0f);
// 
// #if 1
// 	LLEnvManagerNew::instance().setRegionSettings(new_region_settings);
// #else // Temporary disabled ability to upload new region settings from the Day Cycle Editor.
// 	if (!LLEnvManagerNew::instance().sendRegionSettings(new_region_settings))
// 	{
// 		LL_WARNS() << "Error applying region environment settings" << LL_ENDL;
// 		return;
// 	}
// 
// 	setApplyProgress(true);
// #endif
// #endif
// }
// 
// void LLFloaterEditExtDayCycle::setApplyProgress(bool started)
// {
// 	LLLoadingIndicator* indicator = getChild<LLLoadingIndicator>("progress_indicator");
// 
// 	indicator->setVisible(started);
// 
// 	if (started)
// 	{
// 		indicator->start();
// 	}
// 	else
// 	{
// 		indicator->stop();
// 	}
// }
// 
// bool LLFloaterEditExtDayCycle::getApplyProgress() const
// {
// 	return getChild<LLLoadingIndicator>("progress_indicator")->getVisible();
// }
// 
// void LLFloaterEditExtDayCycle::onDeleteKey()
// {
// #if 0
// 	if (mSliderToKey.size() == 0)
// 	{
// 		return;
// 	}
// 	else if (mSliderToKey.size() == 1)
// 	{
// 		LLNotifications::instance().add("EnvCannotDeleteLastDayCycleKey", LLSD(), LLSD());
// 		return;
// 	}
// 
// 	// delete from map
// 	const std::string& sldr_name = mKeysSlider->getCurSlider();
// 	std::map<std::string, SliderKey>::iterator mIt = mSliderToKey.find(sldr_name);
// 	mSliderToKey.erase(mIt);
// 
// 	mKeysSlider->deleteCurSlider();
// 
// 	if (mSliderToKey.size() == 0)
// 	{
// 		return;
// 	}
// 
// 	const std::string& name = mKeysSlider->getCurSlider();
// 	mSkyPresetsCombo->selectByValue(mSliderToKey[name].keyframe.toStringVal());
// 	F32 time24 = mSliderToKey[name].time;
// 
// 	mTimeCtrl->setTime24(time24);
// 
// 	applyTrack();
// #endif
// }
// 
// void LLFloaterEditExtDayCycle::onRegionSettingsChange()
// {
// #if 0
// 	LL_DEBUGS("Windlight") << "Region settings changed" << LL_ENDL;
// 
// 	if (getApplyProgress()) // our region settings have being applied
// 	{
// 		setApplyProgress(false);
// 
// 		// Change preference if requested.
// 		if (mMakeDefaultCheckBox->getValue())
// 		{
// 			LL_DEBUGS("Windlight") << "Changed environment preference to region settings" << LL_ENDL;
// 			LLEnvManagerNew::instance().setUseRegionSettings(true);
// 		}
// 
// 		closeFloater();
// 	}
// #endif
// }
// 
// void LLFloaterEditExtDayCycle::onRegionChange()
// {
// #if 0
// 	LL_DEBUGS("Windlight") << "Region changed" << LL_ENDL;
// 
// 	// If we're editing the region day cycle
// 	if (getSelectedDayCycle().scope == LLEnvKey::SCOPE_REGION)
// 	{
// 		reset(); // undoes all unsaved changes
// 	}
// #endif
// }
// 
// void LLFloaterEditExtDayCycle::onRegionSettingsApplied(bool success)
// {
// 	LL_DEBUGS("Windlight") << "Region settings applied: " << success << LL_ENDL;
// 
// 	if (!success)
// 	{
// 		// stop progress indicator
// 		setApplyProgress(false);
// 	}
// }
// 
// void LLFloaterEditExtDayCycle::onRegionInfoUpdate()
// {
// #if 0
// 	LL_DEBUGS("Windlight") << "Region info updated" << LL_ENDL;
// 	bool can_edit = true;
// 
// 	// If we've selected the region day cycle for editing.
// 	if (getSelectedDayCycle().scope == LLEnvKey::SCOPE_REGION)
// 	{
// 		// check whether we have the access
// 		can_edit = LLEnvManagerNew::canEditRegionSettings();
// 	}
// 
// 	enableEditing(can_edit);
// #endif
// }
// 
// void LLFloaterEditExtDayCycle::onDayCycleNameEdited()
// {
// #if 0
// 	// Disable saving a day cycle having empty name.
// 	LLWLParamKey key = getSelectedDayCycle();
// 	mSaveButton->setEnabled(!key.name.empty());
// #endif
// }
// 
// void LLFloaterEditExtDayCycle::onDayCycleSelected()
// {
// #if 0
// 
// 	LLSD day_data;
// 	LLWLParamKey dc_key = getSelectedDayCycle();
// 	bool can_edit = true;
// 
// 	if (dc_key.scope == LLEnvKey::SCOPE_LOCAL)
// 	{
// 		if (!LLDayCycleManager::instance().getPreset(dc_key.name, day_data))
// 		{
// 			LL_WARNS() << "No day cycle named " << dc_key.name << LL_ENDL;
// 			return;
// 		}
// 	}
// 	else
// 	{
// 		day_data = LLEnvManagerNew::instance().getRegionSettings().getWLDayCycle();
// 		if (day_data.size() == 0)
// 		{
// 			LL_WARNS() << "Empty region day cycle" << LL_ENDL;
// 			llassert(day_data.size() > 0);
// 			return;
// 		}
// 
// 		can_edit = LLEnvManagerNew::canEditRegionSettings();
// 	}
// 
// 	// We may need to add or remove region skies from the list.
// 	refreshSkyPresetsList();
// 
// 	F32 slider_time = mTimeSlider->getCurSliderValue() / sHoursPerDay;
// 	LLWLParamManager::instance().applyDayCycleParams(day_data, dc_key.scope, slider_time);
// 	loadTrack();
// #endif
// 	enableEditing(false);
// }
// 
// bool LLFloaterEditExtDayCycle::onSaveAnswer(const LLSD& notification, const LLSD& response)
// {
// 	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
// 
// 	// If they choose save, do it.  Otherwise, don't do anything
// 	if (option == 0)
// 	{
// 		onSaveConfirmed();
// 	}
// 
// 	return false;
// }
// 
// void LLFloaterEditExtDayCycle::onSaveConfirmed()
// {
// #if 0
// 	std::string name = getSelectedDayCycle().name;
// 
// 	// Save preset.
// 	LLSD data = LLWLParamManager::instance().mDay.asLLSD();
// 	LL_DEBUGS("Windlight") << "Saving day cycle " << name << ": " << data << LL_ENDL;
// 	LLDayCycleManager::instance().savePreset(name, data);
// 
// 	// Change preference if requested.
// 	if (mMakeDefaultCheckBox->getValue())
// 	{
// 		LL_DEBUGS("Windlight") << name << " is now the new preferred day cycle" << LL_ENDL;
// 		LLEnvManagerNew::instance().setUseDayCycle(name);
// 	}
// #endif
// 	closeFloater();
// }
// 
// void LLFloaterEditExtDayCycle::onDayCycleListChange()
// {
// 	if (!isNewDay())
// 	{
// 		refreshDayCyclesList();
// 	}
// }
// 
// void LLFloaterEditExtDayCycle::onSkyPresetListChange()
// {
// 	refreshSkyPresetsList();
// 
// 	// Refresh sliders from the currently visible day cycle.
// 	loadTrack();
// }
// 
// static
// std::string LLFloaterEditExtDayCycle::getRegionName()
// {
// 	return gAgent.getRegion() ? gAgent.getRegion()->getName() : LLTrans::getString("Unknown");
// }
