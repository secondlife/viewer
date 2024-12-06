/**
 * @file llpanelprofilepicks.cpp
 * @brief LLPanelProfilePicks and related class implementations
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
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

#include "llpanelprofilepicks.h"

#include "llagent.h"
#include "llagentpicksinfo.h"
#include "llavataractions.h"
#include "llavatarpropertiesprocessor.h"
#include "llcommandhandler.h"
#include "lldispatcher.h"
#include "llfloaterreg.h"
#include "llfloaterworldmap.h"
#include "lllandmarkactions.h"
#include "lllineeditor.h"
#include "llnotificationsutil.h"
#include "llpanelavatar.h"
#include "llpanelprofile.h"
#include "llparcel.h"
#include "llregionhandle.h"
#include "llstartup.h"
#include "lltabcontainer.h"
#include "lltextbox.h"
#include "lltexteditor.h"
#include "lltexturectrl.h"
#include "lltexturectrl.h"
#include "lltrans.h"
#include "llviewergenericmessage.h" // send_generic_message
#include "llviewerparcelmgr.h"
#include "llviewerregion.h"
#include "llworldmap.h"

static LLPanelInjector<LLPanelProfilePicks> t_panel_profile_picks("panel_profile_picks");
static LLPanelInjector<LLPanelProfilePick> t_panel_profile_pick("panel_profile_pick");


class LLPickHandler : public LLCommandHandler
{
public:

    // requires trusted browser to trigger
    LLPickHandler() : LLCommandHandler("pick", UNTRUSTED_THROTTLE) { }

    virtual bool canHandleUntrusted(
        const LLSD& params,
        const LLSD& query_map,
        LLMediaCtrl* web,
        const std::string& nav_type)
    {
        if (params.size() < 1)
        {
            return true; // don't block, will fail later
        }

        if (nav_type == NAV_TYPE_CLICKED
            || nav_type == NAV_TYPE_EXTERNAL)
        {
            return true;
        }

        const std::string verb = params[0].asString();
        if (verb == "create")
        {
            return false;
        }
        return true;
    }

    bool handle(const LLSD& params,
                const LLSD& query_map,
                const std::string& grid,
                LLMediaCtrl* web)
    {
        if (LLStartUp::getStartupState() < STATE_STARTED)
        {
            return true;
        }

        // handle app/pick/create urls first
        if (params.size() == 1 && params[0].asString() == "create")
        {
            LLAvatarActions::createPick();
            return true;
        }

        // then handle the general app/pick/{UUID}/{CMD} urls
        if (params.size() < 2)
        {
            return false;
        }

        // get the ID for the pick_id
        LLUUID pick_id;
        if (!pick_id.set(params[0], false))
        {
            return false;
        }

        // edit the pick in the side tray.
        // need to ask the server for more info first though...
        const std::string verb = params[1].asString();
        if (verb == "edit")
        {
            LLAvatarActions::showPick(gAgent.getID(), pick_id);
            return true;
        }
        else
        {
            LL_WARNS() << "unknown verb " << verb << LL_ENDL;
            return false;
        }
    }
};
LLPickHandler gPickHandler;


//-----------------------------------------------------------------------------
// LLPanelProfilePicks
//-----------------------------------------------------------------------------

LLPanelProfilePicks::LLPanelProfilePicks()
 : LLPanelProfilePropertiesProcessorTab()
 , mPickToSelectOnLoad(LLUUID::null)
{
}

LLPanelProfilePicks::~LLPanelProfilePicks()
{
}

void LLPanelProfilePicks::onOpen(const LLSD& key)
{
    LLPanelProfilePropertiesProcessorTab::onOpen(key);

    resetData();

    bool own_profile = getSelfProfile();
    if (own_profile)
    {
        mNewButton->setVisible(true);
        mNewButton->setEnabled(false);

        mDeleteButton->setVisible(true);
        mDeleteButton->setEnabled(false);
    }

    childSetVisible("buttons_header", own_profile);
}

void LLPanelProfilePicks::createPick(const LLPickData &data)
{
    if (getIsLoaded())
    {
        if (canAddNewPick())
        {
            mNoItemsLabel->setVisible(false);
            LLPanelProfilePick* pick_panel = LLPanelProfilePick::create();
            pick_panel->setAvatarId(getAvatarId());
            pick_panel->processProperties(&data);
            mTabContainer->addTabPanel(
                LLTabContainer::TabPanelParams().
                panel(pick_panel).
                select_tab(true).
                label(pick_panel->getPickName()));
            updateButtons();
        }
        else
        {
            // This means that something doesn't properly check limits
            // before creating a pick
            LL_WARNS() << "failed to add pick" << LL_ENDL;
        }
    }
    else
    {
        mSheduledPickCreation.push_back(data);
    }
}

void LLPanelProfilePicks::selectPick(const LLUUID& pick_id)
{
    if (getIsLoaded())
    {
        for (S32 tab_idx = 0; tab_idx < mTabContainer->getTabCount(); ++tab_idx)
        {
            LLPanelProfilePick* pick_panel = dynamic_cast<LLPanelProfilePick*>(mTabContainer->getPanelByIndex(tab_idx));
            if (pick_panel)
            {
                if (pick_panel->getPickId() == pick_id)
                {
                    mTabContainer->selectTabPanel(pick_panel);
                    break;
                }
            }
        }
    }
    else
    {
        mPickToSelectOnLoad = pick_id;
    }
}

bool LLPanelProfilePicks::postBuild()
{
    mTabContainer = getChild<LLTabContainer>("tab_picks");
    mNoItemsLabel = getChild<LLUICtrl>("picks_panel_text");
    mNewButton = getChild<LLButton>("new_btn");
    mDeleteButton = getChild<LLButton>("delete_btn");

    mNewButton->setCommitCallback(boost::bind(&LLPanelProfilePicks::onClickNewBtn, this));
    mDeleteButton->setCommitCallback(boost::bind(&LLPanelProfilePicks::onClickDelete, this));

    return true;
}

void LLPanelProfilePicks::onClickNewBtn()
{
    mNoItemsLabel->setVisible(false);
    LLPanelProfilePick* pick_panel = LLPanelProfilePick::create();
    pick_panel->setAvatarId(getAvatarId());
    mTabContainer->addTabPanel(
        LLTabContainer::TabPanelParams().
        panel(pick_panel).
        select_tab(true).
        label(pick_panel->getPickName()));
    updateButtons();

    pick_panel->addLocationChangedCallbacks();
}

void LLPanelProfilePicks::onClickDelete()
{
    LLPanelProfilePick* pick_panel = dynamic_cast<LLPanelProfilePick*>(mTabContainer->getCurrentPanel());
    if (pick_panel)
    {
        LLUUID pick_id = pick_panel->getPickId();
        LLSD args;
        args["PICK"] = pick_panel->getPickName();
        LLSD payload;
        payload["pick_id"] = pick_id;
        payload["tab_idx"] = mTabContainer->getCurrentPanelIndex();
        LLNotificationsUtil::add("ProfileDeletePick", args, payload,
            boost::bind(&LLPanelProfilePicks::callbackDeletePick, this, _1, _2));
    }
}

void LLPanelProfilePicks::callbackDeletePick(const LLSD& notification, const LLSD& response)
{
    S32 option = LLNotificationsUtil::getSelectedOption(notification, response);

    if (0 == option)
    {
        LLUUID pick_id = notification["payload"]["pick_id"].asUUID();
        S32 tab_idx = notification["payload"]["tab_idx"].asInteger();

        LLPanelProfilePick* pick_panel = dynamic_cast<LLPanelProfilePick*>(mTabContainer->getPanelByIndex(tab_idx));
        if (pick_panel && pick_panel->getPickId() == pick_id)
        {
            mTabContainer->removeTabPanel(pick_panel);
        }

        if (pick_id.notNull())
        {
            LLAvatarPropertiesProcessor::getInstance()->sendPickDelete(pick_id);
        }

        updateButtons();
    }
}

void LLPanelProfilePicks::processProperties(void* data, EAvatarProcessorType type)
{
    if (APT_PROPERTIES == type)
    {
        LLAvatarData* avatar_picks = static_cast<LLAvatarData*>(data);
        if (avatar_picks && getAvatarId() == avatar_picks->avatar_id)
        {
            if (getSelfProfile())
            {
                LLAgentPicksInfo::getInstance()->onServerRespond(avatar_picks);
            }
            processProperties(avatar_picks);
        }
    }
}

void LLPanelProfilePicks::processProperties(const LLAvatarData* avatar_picks)
{
    LLUUID selected_id = mPickToSelectOnLoad;
    bool has_selection = false;
    if (mPickToSelectOnLoad.isNull())
    {
        if (mTabContainer->getTabCount() > 0)
        {
            LLPanelProfilePick* active_pick_panel = dynamic_cast<LLPanelProfilePick*>(mTabContainer->getCurrentPanel());
            if (active_pick_panel)
            {
                selected_id = active_pick_panel->getPickId();
            }
        }
    }

    mTabContainer->deleteAllTabs();

    LLAvatarData::picks_list_t::const_iterator it = avatar_picks->picks_list.begin();
    for (; avatar_picks->picks_list.end() != it; ++it)
    {
        LLUUID pick_id = it->first;
        std::string pick_name = it->second;

        LLPanelProfilePick* pick_panel = LLPanelProfilePick::create();

        pick_panel->setPickId(pick_id);
        pick_panel->setPickName(pick_name);
        pick_panel->setAvatarId(getAvatarId());

        mTabContainer->addTabPanel(
            LLTabContainer::TabPanelParams().
            panel(pick_panel).
            select_tab(selected_id == pick_id).
            label(pick_name));

        if (selected_id == pick_id)
        {
            has_selection = true;
        }
    }

    while (!mSheduledPickCreation.empty() && canAddNewPick())
    {
        const LLPickData data =
            mSheduledPickCreation.back();

        LLPanelProfilePick* pick_panel = LLPanelProfilePick::create();
        pick_panel->setAvatarId(getAvatarId());
        pick_panel->processProperties(&data);
        mTabContainer->addTabPanel(
            LLTabContainer::TabPanelParams().
            panel(pick_panel).
            select_tab(!has_selection).
            label(pick_panel->getPickName()));

        mSheduledPickCreation.pop_back();
        has_selection = true;
    }

    // reset 'do on load' values
    mPickToSelectOnLoad = LLUUID::null;
    mSheduledPickCreation.clear();

    if (getSelfProfile())
    {
        mNoItemsLabel->setValue(LLTrans::getString("NoPicksText"));
    }
    else
    {
        mNoItemsLabel->setValue(LLTrans::getString("NoAvatarPicksText"));
    }

    bool has_data = mTabContainer->getTabCount() > 0;
    mNoItemsLabel->setVisible(!has_data);
    if (has_data && !has_selection)
    {
        mTabContainer->selectFirstTab();
    }

    setLoaded();
    updateButtons();
}

void LLPanelProfilePicks::resetData()
{
    resetLoading();
    mTabContainer->deleteAllTabs();
}

void LLPanelProfilePicks::updateButtons()
{
    if (getSelfProfile())
    {
        mNewButton->setEnabled(canAddNewPick());
        mDeleteButton->setEnabled(canDeletePick());
    }
}

void LLPanelProfilePicks::apply()
{
    if (getIsLoaded())
    {
        for (S32 tab_idx = 0; tab_idx < mTabContainer->getTabCount(); ++tab_idx)
        {
            LLPanelProfilePick* pick_panel = dynamic_cast<LLPanelProfilePick*>(mTabContainer->getPanelByIndex(tab_idx));
            if (pick_panel)
            {
                pick_panel->apply();
            }
        }
    }
}

void LLPanelProfilePicks::updateData()
{
    // Send picks request only once
    LLUUID avatar_id = getAvatarId();
    if (!getStarted() && avatar_id.notNull())
    {
        setIsLoading();

        LLAvatarPropertiesProcessor::getInstance()->sendAvatarPropertiesRequest(avatar_id);
    }
    if (!getIsLoaded())
    {
        mNoItemsLabel->setValue(LLTrans::getString("PicksClassifiedsLoadingText"));
        mNoItemsLabel->setVisible(true);
    }
}

bool LLPanelProfilePicks::hasUnsavedChanges()
{
    for (S32 tab_idx = 0; tab_idx < mTabContainer->getTabCount(); ++tab_idx)
    {
        LLPanelProfilePick* pick_panel = dynamic_cast<LLPanelProfilePick*>(mTabContainer->getPanelByIndex(tab_idx));
        if (pick_panel && (pick_panel->isDirty() || pick_panel->isDirty()))
        {
            return true;
        }
    }
    return false;
}

void LLPanelProfilePicks::commitUnsavedChanges()
{
    for (S32 tab_idx = 0; tab_idx < mTabContainer->getTabCount(); ++tab_idx)
    {
        LLPanelProfilePick* pick_panel = dynamic_cast<LLPanelProfilePick*>(mTabContainer->getPanelByIndex(tab_idx));
        if (pick_panel)
        {
            pick_panel->apply();
        }
    }
}

bool LLPanelProfilePicks::canAddNewPick()
{
    return (!LLAgentPicksInfo::getInstance()->isPickLimitReached() &&
        mTabContainer->getTabCount() < LLAgentPicksInfo::getInstance()->getMaxNumberOfPicks());
}

bool LLPanelProfilePicks::canDeletePick()
{
    return (mTabContainer->getTabCount() > 0);
}


//-----------------------------------------------------------------------------
// LLPanelProfilePick
//-----------------------------------------------------------------------------

LLPanelProfilePick::LLPanelProfilePick()
 : LLPanelProfilePropertiesProcessorTab()
 , LLRemoteParcelInfoObserver()
 , mSnapshotCtrl(NULL)
 , mPickId(LLUUID::null)
 , mParcelId(LLUUID::null)
 , mRequestedId(LLUUID::null)
 , mLocationChanged(false)
 , mNewPick(false)
 , mIsEditing(false)
 , mRegionCallbackConnection()
 , mParcelCallbackConnection()
{
}

//static
LLPanelProfilePick* LLPanelProfilePick::create()
{
    LLPanelProfilePick* panel = new LLPanelProfilePick();
    panel->buildFromFile("panel_profile_pick.xml");
    return panel;
}

LLPanelProfilePick::~LLPanelProfilePick()
{
    if (mParcelId.notNull())
    {
        LLRemoteParcelInfoProcessor::getInstance()->removeObserver(mParcelId, this);
    }

    if (mRegionCallbackConnection.connected())
    {
        mRegionCallbackConnection.disconnect();
    }
    if (mParcelCallbackConnection.connected())
    {
        mParcelCallbackConnection.disconnect();
    }
}

void LLPanelProfilePick::setAvatarId(const LLUUID& avatar_id)
{
    if (avatar_id.isNull())
    {
        return;
    }
    LLPanelProfilePropertiesProcessorTab::setAvatarId(avatar_id);

    // creating new Pick
    if (getPickId().isNull() && getSelfProfile())
    {
        mNewPick = true;

        setPosGlobal(gAgent.getPositionGlobal());

        LLUUID parcel_id = LLUUID::null, snapshot_id = LLUUID::null;
        std::string pick_name, pick_desc, region_name;

        LLParcel* parcel = LLViewerParcelMgr::getInstance()->getAgentParcel();
        if (parcel)
        {
            parcel_id = parcel->getID();
            pick_name = parcel->getName();
            pick_desc = parcel->getDesc();
            snapshot_id = parcel->getSnapshotID();
            mPickDescription->setParseHTML(false);
        }

        LLViewerRegion* region = gAgent.getRegion();
        if (region)
        {
            region_name = region->getName();
        }

        setParcelID(parcel_id);
        setPickName(pick_name.empty() ? region_name : pick_name);
        setPickDesc(pick_desc);
        setSnapshotId(snapshot_id);
        setPickLocation(createLocationText(getLocationNotice(), pick_name, region_name, getPosGlobal()));

        enableSaveButton(true);
    }
    else
    {
        LLAvatarPropertiesProcessor::getInstance()->sendPickInfoRequest(getAvatarId(), getPickId());

        enableSaveButton(false);
    }

    resetDirty();

    if (getSelfProfile())
    {
        mPickName->setEnabled(true);
        mPickDescription->setEnabled(true);
        mSetCurrentLocationButton->setVisible(true);
    }
    else
    {
        mSnapshotCtrl->setEnabled(false);
        mSetCurrentLocationButton->setVisible(false);
    }
}

bool LLPanelProfilePick::postBuild()
{
    mPickName = getChild<LLLineEditor>("pick_name");
    mPickDescription = getChild<LLTextEditor>("pick_desc");
    mSaveButton = getChild<LLButton>("save_changes_btn");
    mCreateButton = getChild<LLButton>("create_changes_btn");
    mCancelButton = getChild<LLButton>("cancel_changes_btn");
    mCreateLandmarkButton = getChild<LLButton>("create_landmark_btn");
    mSetCurrentLocationButton = getChild<LLButton>("set_to_curr_location_btn");

    mSnapshotCtrl = getChild<LLTextureCtrl>("pick_snapshot");
    mSnapshotCtrl->setCommitCallback([&](LLUICtrl*, const LLSD&) { onSnapshotChanged(); });
    mSnapshotCtrl->setAllowLocalTexture(false);
    mSnapshotCtrl->setBakeTextureEnabled(false);

    childSetAction("teleport_btn", [&](LLUICtrl*, const LLSD&) { onClickTeleport(); });
    childSetAction("show_on_map_btn", [&](LLUICtrl*, const LLSD&) { onClickMap(); });

    mSaveButton->setCommitCallback([&](LLUICtrl*, const LLSD&) { onClickSave(); });
    mCreateButton->setCommitCallback([&](LLUICtrl*, const LLSD&) { onClickSave(); });
    mCancelButton->setCommitCallback([&](LLUICtrl*, const LLSD&) { onClickCancel(); });
    mCreateLandmarkButton->setCommitCallback([&](LLUICtrl*, const LLSD&) { onClickCreateLandmark(); });
    mSetCurrentLocationButton->setCommitCallback([&](LLUICtrl*, const LLSD&) { onClickSetLocation(); });

    mPickName->setKeystrokeCallback([&](LLLineEditor* ctrl, void*) { onPickChanged(ctrl); }, NULL);
    mPickName->setEnabled(false);

    mPickDescription->setKeystrokeCallback([&](LLTextEditor* ctrl) { onPickChanged(ctrl); });
    mPickDescription->setFocusReceivedCallback([&](LLFocusableElement*) { onDescriptionFocusReceived(); });

    getChild<LLUICtrl>("pick_location")->setEnabled(false);

    return true;
}

void LLPanelProfilePick::onDescriptionFocusReceived()
{
    if (!mIsEditing && getSelfProfile())
    {
        mIsEditing = true;
        mPickDescription->setParseHTML(false);
    }
}

void LLPanelProfilePick::processProperties(void* data, EAvatarProcessorType type)
{
    if (APT_PICK_INFO != type)
    {
        return;
    }

    LLPickData* pick_info = static_cast<LLPickData*>(data);
    if (!pick_info
        || pick_info->creator_id != getAvatarId()
        || pick_info->pick_id != getPickId())
    {
        return;
    }

    processProperties(pick_info);
}

void LLPanelProfilePick::processProperties(const LLPickData* pick_info)
{
    mIsEditing = false;
    mPickDescription->setParseHTML(true);
    mParcelId = pick_info->parcel_id;
    setSnapshotId(pick_info->snapshot_id);
    if (!getSelfProfile())
    {
        mSnapshotCtrl->setEnabled(false);
    }
    setPickName(pick_info->name);
    setPickDesc(pick_info->desc);
    setPosGlobal(pick_info->pos_global);

    // Send remote parcel info request to get parcel name and sim (region) name.
    sendParcelInfoRequest();

    // *NOTE dzaporozhan
    // We want to keep listening to APT_PICK_INFO because user may
    // edit the Pick and we have to update Pick info panel.
    // revomeObserver is called from onClickBack

    setLoaded();
}

void LLPanelProfilePick::apply()
{
    if ((mNewPick || getIsLoaded()) && isDirty())
    {
        sendUpdate();
    }
}

void LLPanelProfilePick::setSnapshotId(const LLUUID& id)
{
    mSnapshotCtrl->setImageAssetID(id);
    mSnapshotCtrl->setValid(true);
}

void LLPanelProfilePick::setPickName(const std::string& name)
{
    mPickName->setValue(name);
    mPickNameStr = name;
}

const std::string LLPanelProfilePick::getPickName()
{
    return mPickName->getValue().asString();
}

void LLPanelProfilePick::setPickDesc(const std::string& desc)
{
    mPickDescription->setValue(desc);
}

void LLPanelProfilePick::setPickLocation(const std::string& location)
{
    getChild<LLUICtrl>("pick_location")->setValue(location);
}

void LLPanelProfilePick::onClickMap()
{
    LLFloaterWorldMap::getInstance()->trackLocation(getPosGlobal());
    LLFloaterReg::showInstance("world_map", "center");
}

void LLPanelProfilePick::onClickTeleport()
{
    if (!getPosGlobal().isExactlyZero())
    {
        gAgent.teleportViaLocation(getPosGlobal());
        LLFloaterWorldMap::getInstance()->trackLocation(getPosGlobal());
    }
}

void LLPanelProfilePick::enableSaveButton(bool enable)
{
    childSetVisible("save_changes_lp", enable);

    childSetVisible("save_btn_lp", enable && !mNewPick);
    childSetVisible("create_btn_lp", enable && mNewPick);
    childSetVisible("cancel_btn_lp", enable && !mNewPick);
}

void LLPanelProfilePick::onSnapshotChanged()
{
    enableSaveButton(true);
}

void LLPanelProfilePick::onPickChanged(LLUICtrl* ctrl)
{
    if (ctrl && ctrl == mPickName)
    {
        updateTabLabel(mPickName->getText());
    }

    enableSaveButton(isDirty());
}

void LLPanelProfilePick::resetDirty()
{
    LLPanel::resetDirty();

    mPickName->resetDirty();
    mPickDescription->resetDirty();
    mSnapshotCtrl->resetDirty();
    mLocationChanged = false;
}

bool LLPanelProfilePick::isDirty() const
{
    if (mNewPick
        || LLPanel::isDirty()
        || mLocationChanged
        || mSnapshotCtrl->isDirty()
        || mPickName->isDirty()
        || mPickDescription->isDirty())
    {
        return true;
    }
    return false;
}

void LLPanelProfilePick::onClickCreateLandmark()
{
    std::string title = getChild<LLUICtrl>("pick_location")->getValue().asString();
    LLLandmarkActions::showFloaterCreateLandmarkForPos(mPosGlobal, title);
}

void LLPanelProfilePick::onClickSetLocation()
{
    // Save location for later use.
    setPosGlobal(gAgent.getPositionGlobal());

    std::string parcel_name, region_name;

    LLParcel* parcel = LLViewerParcelMgr::getInstance()->getAgentParcel();
    if (parcel)
    {
        mParcelId = parcel->getID();
        parcel_name = parcel->getName();
    }

    LLViewerRegion* region = gAgent.getRegion();
    if (region)
    {
        region_name = region->getName();
    }

    setPickLocation(createLocationText(getLocationNotice(), parcel_name, region_name, getPosGlobal()));

    mLocationChanged = true;
    enableSaveButton(true);
}

void LLPanelProfilePick::onClickSave()
{
    if (mRegionCallbackConnection.connected())
    {
        mRegionCallbackConnection.disconnect();
    }
    if (mParcelCallbackConnection.connected())
    {
        mParcelCallbackConnection.disconnect();
    }
    if (mLocationChanged)
    {
        onClickSetLocation();
    }
    sendUpdate();

    mLocationChanged = false;
}

void LLPanelProfilePick::onClickCancel()
{
    updateTabLabel(mPickNameStr);
    LLAvatarPropertiesProcessor::getInstance()->sendPickInfoRequest(getAvatarId(), getPickId());
    mLocationChanged = false;
    enableSaveButton(false);
}

std::string LLPanelProfilePick::getLocationNotice()
{
    static const std::string notice = getString("location_notice");
    return notice;
}

void LLPanelProfilePick::sendParcelInfoRequest()
{
    if (mParcelId != mRequestedId)
    {
        if (mRequestedId.notNull())
        {
            LLRemoteParcelInfoProcessor::getInstance()->removeObserver(mRequestedId, this);
        }
        LLRemoteParcelInfoProcessor::getInstance()->addObserver(mParcelId, this);
        LLRemoteParcelInfoProcessor::getInstance()->sendParcelInfoRequest(mParcelId);

        mRequestedId = mParcelId;
    }
}

void LLPanelProfilePick::processParcelInfo(const LLParcelData& parcel_data)
{
    setPickLocation(createLocationText(LLStringUtil::null, parcel_data.name, parcel_data.sim_name, getPosGlobal()));

    // We have received parcel info for the requested ID so clear it now.
    mRequestedId.setNull();

    if (mParcelId.notNull())
    {
        LLRemoteParcelInfoProcessor::getInstance()->removeObserver(mParcelId, this);
    }
}

void LLPanelProfilePick::addLocationChangedCallbacks()
{
    mRegionCallbackConnection = gAgent.addRegionChangedCallback([this]() { onClickSetLocation(); });
    mParcelCallbackConnection = gAgent.addParcelChangedCallback([this]() { onClickSetLocation(); });
}

void LLPanelProfilePick::sendUpdate()
{
    LLPickData pick_data;

    // If we don't have a pick id yet, we'll need to generate one,
    // otherwise we'll keep overwriting pick_id 00000 in the database.
    if (getPickId().isNull())
    {
        getPickId().generate();
    }

    pick_data.agent_id = gAgentID;
    pick_data.session_id = gAgent.getSessionID();
    pick_data.pick_id = getPickId();
    pick_data.creator_id = gAgentID;;

    //legacy var  need to be deleted
    pick_data.top_pick = false;
    pick_data.parcel_id = mParcelId;
    pick_data.name = getPickName();
    pick_data.desc = mPickDescription->getValue().asString();
    pick_data.snapshot_id = mSnapshotCtrl->getImageAssetID();
    pick_data.pos_global = getPosGlobal();
    pick_data.sort_order = 0;
    pick_data.enabled = true;

    LLAvatarPropertiesProcessor::getInstance()->sendPickInfoUpdate(&pick_data);

    if(mNewPick)
    {
        // Assume a successful create pick operation, make new number of picks
        // available immediately. Actual number of picks will be requested in
        // LLAvatarPropertiesProcessor::sendPickInfoUpdate and updated upon server respond.
        LLAgentPicksInfo::getInstance()->incrementNumberOfPicks();
    }
}

// static
std::string LLPanelProfilePick::createLocationText(const std::string& owner_name, const std::string& original_name, const std::string& sim_name, const LLVector3d& pos_global)
{
    std::string location_text(owner_name);

    auto append = [&](const std::string& text, const std::string& delimiter)
        {
            if (!text.empty())
            {
                if (!location_text.empty())
                {
                    location_text.append(delimiter);
                }
                location_text.append(text);
            }
        };

    append(original_name, ", ");
    append(sim_name, ", ");

    if (!pos_global.isNull())
    {
        S32 region_x = ll_round((F32)pos_global.mdV[VX]) % REGION_WIDTH_UNITS;
        S32 region_y = ll_round((F32)pos_global.mdV[VY]) % REGION_WIDTH_UNITS;
        S32 region_z = ll_round((F32)pos_global.mdV[VZ]);
        append(llformat("(%d, %d, %d)", region_x, region_y, region_z), " ");
    }

    return location_text;
}

void LLPanelProfilePick::updateTabLabel(const std::string& title)
{
    setLabel(title);
    LLTabContainer* parent = dynamic_cast<LLTabContainer*>(getParent());
    if (parent)
    {
        parent->setCurrentTabName(title);
    }
}

