/**
 * @file llfloaterregioninfo.cpp
 * @author Aaron Brashears
 * @brief Implementation of the region info and controls floater and panels.
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
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
#include "llfloaterregioninfo.h"

#include <algorithm>
#include <functional>

#include "lldir.h"
#include "lldispatcher.h"
#include "llglheaders.h"
#include "llregionflags.h"
#include "llstl.h"
#include "llfilesystem.h"
#include "llxfermanager.h"
#include "indra_constants.h"
#include "message.h"
#include "llloadingindicator.h"
#include "llradiogroup.h"
#include "llsd.h"
#include "llsdserialize.h"

#include "llagent.h"
#include "llappviewer.h"
#include "llavataractions.h"
#include "llavatarname.h"
#include "llfloateravatarpicker.h"
#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "llclipboard.h"
#include "llcombobox.h"
#include "llestateinfomodel.h"
#include "llfilepicker.h"
#include "llfloatergodtools.h"  // for send_sim_wide_deletes()
#include "llfloatertopobjects.h" // added to fix SL-32336
#include "llfloatergroups.h"
#include "llfloaterreg.h"
#include "llfloaterregiondebugconsole.h"
#include "llfloaterregionrestartschedule.h"
#include "llfloatertelehub.h"
#include "llgltfmateriallist.h"
#include "llinventorymodel.h"
#include "lllineeditor.h"
#include "llnamelistctrl.h"
#include "llnotifications.h"
#include "llnotificationsutil.h"
#include "llpbrterrainfeatures.h"
#include "llregioninfomodel.h"
#include "llscrolllistitem.h"
#include "llsliderctrl.h"
#include "llslurl.h"
#include "llspinctrl.h"
#include "lltabcontainer.h"
#include "lltextbox.h"
#include "llinventory.h"
#include "lltexturectrl.h"
#include "lltrans.h"
#include "llviewercontrol.h"
#include "lluictrlfactory.h"
#include "llviewerinventory.h"
#include "llviewertexture.h"
#include "llviewertexturelist.h"
#include "llviewerregion.h"
#include "llviewerstats.h"
#include "llviewertexteditor.h"
#include "llviewerwindow.h"
#include "lltrans.h"
#include "llagentui.h"
#include "llmeshrepository.h"
#include "llfloaterregionrestarting.h"
#include "llpanelexperiencelisteditor.h"
#include <boost/function.hpp>
#include "llpanelexperiencepicker.h"
#include "llexperiencecache.h"
#include "llpanelexperiences.h"
#include "llcorehttputil.h"
#include "llavatarnamecache.h"
#include "llenvironment.h"

const S32 CORNER_COUNT = 4;

const U32 MAX_LISTED_NAMES = 100;

#define TMP_DISABLE_WLES // STORM-1180

///----------------------------------------------------------------------------
/// Local class declaration
///----------------------------------------------------------------------------

class LLDispatchEstateUpdateInfo : public LLDispatchHandler
{
public:
    LLDispatchEstateUpdateInfo() {}
    virtual ~LLDispatchEstateUpdateInfo() {}
    virtual bool operator()(
        const LLDispatcher* dispatcher,
        const std::string& key,
        const LLUUID& invoice,
        const sparam_t& strings);
};

class LLDispatchSetEstateAccess : public LLDispatchHandler
{
public:
    LLDispatchSetEstateAccess() {}
    virtual ~LLDispatchSetEstateAccess() {}
    virtual bool operator()(
        const LLDispatcher* dispatcher,
        const std::string& key,
        const LLUUID& invoice,
        const sparam_t& strings);
};

class LLDispatchSetEstateExperience : public LLDispatchHandler
{
public:
    virtual bool operator()(
        const LLDispatcher* dispatcher,
        const std::string& key,
        const LLUUID& invoice,
        const sparam_t& strings);

    static LLSD getIDs( sparam_t::const_iterator it, sparam_t::const_iterator end, S32 count );
};

class LLPanelRegionEnvironment : public LLPanelEnvironmentInfo
{
public:
                        LLPanelRegionEnvironment();
    virtual             ~LLPanelRegionEnvironment();

    virtual void        refresh() override;

    virtual bool        isRegion() const override { return true; }
    virtual LLParcel *  getParcel() override { return nullptr; }
    virtual bool        canEdit() override { return LLEnvironment::instance().canAgentUpdateRegionEnvironment(); }
    virtual bool        isLargeEnough() override { return true; }   // regions are always large enough.

    bool                refreshFromRegion(LLViewerRegion* region);

    virtual bool        postBuild() override;
    virtual void        onOpen(const LLSD& key) override {};

    virtual S32         getParcelId() override { return INVALID_PARCEL_ID; }

protected:
    static const U32    DIRTY_FLAG_OVERRIDE;

    virtual void        refreshFromSource() override;

    bool                confirmUpdateEstateEnvironment(const LLSD& notification, const LLSD& response);

    void                onChkAllowOverride(bool value);

private:
    bool                mAllowOverrideRestore;
    connection_t        mCommitConnect;
};



bool estate_dispatch_initialized = false;


///----------------------------------------------------------------------------
/// LLFloaterRegionInfo
///----------------------------------------------------------------------------

//S32 LLFloaterRegionInfo::sRequestSerial = 0;
LLUUID LLFloaterRegionInfo::sRequestInvoice;


LLFloaterRegionInfo::LLFloaterRegionInfo(const LLSD& seed)
    : LLFloater(seed),
    mEnvironmentPanel(NULL),
    mRegionChangedCallback()
{}

bool LLFloaterRegionInfo::postBuild()
{
    mTab = getChild<LLTabContainer>("region_panels");
    mTab->setCommitCallback(boost::bind(&LLFloaterRegionInfo::onTabSelected, this, _2));

    // contruct the panels
    LLPanelRegionInfo* panel;
    panel = new LLPanelEstateInfo;
    mInfoPanels.push_back(panel);
    panel->buildFromFile("panel_region_estate.xml");
    mTab->addTabPanel(LLTabContainer::TabPanelParams().panel(panel).select_tab(true));

    panel = new LLPanelEstateAccess;
    mInfoPanels.push_back(panel);
    panel->buildFromFile("panel_region_access.xml");
    mTab->addTabPanel(panel);

    panel = new LLPanelEstateCovenant;
    mInfoPanels.push_back(panel);
    panel->buildFromFile("panel_region_covenant.xml");
    mTab->addTabPanel(panel);

    panel = new LLPanelRegionGeneralInfo;
    mInfoPanels.push_back(panel);
    panel->getCommitCallbackRegistrar().add("RegionInfo.ManageTelehub", { boost::bind(&LLPanelRegionInfo::onClickManageTelehub, panel) });
    panel->getCommitCallbackRegistrar().add("RegionInfo.ManageRestart", { boost::bind(&LLPanelRegionInfo::onClickManageRestartSchedule, panel) });
    panel->buildFromFile("panel_region_general.xml");
    mTab->addTabPanel(panel);

    panel = new LLPanelRegionTerrainInfo;
    mInfoPanels.push_back(panel);
    static LLCachedControl<bool> feature_pbr_terrain_enabled(gSavedSettings, "RenderTerrainPBREnabled", false);
    static LLCachedControl<bool> feature_pbr_terrain_transforms_enabled(gSavedSettings, "RenderTerrainPBRTransformsEnabled", false);
    if (!feature_pbr_terrain_transforms_enabled() || !feature_pbr_terrain_enabled())
    {
        panel->buildFromFile("panel_region_terrain.xml");
    }
    else
    {
        panel->buildFromFile("panel_region_terrain_texture_transform.xml");
    }
    mTab->addTabPanel(panel);

    mEnvironmentPanel = new LLPanelRegionEnvironment;
    mEnvironmentPanel->buildFromFile("panel_region_environment.xml");
//  mEnvironmentPanel->configureForRegion();
    mTab->addTabPanel(mEnvironmentPanel);

    panel = new LLPanelRegionDebugInfo;
    mInfoPanels.push_back(panel);
    panel->buildFromFile("panel_region_debug.xml");
    mTab->addTabPanel(panel);

    if(gDisconnected)
    {
        return true;
    }

    if(!gAgent.getRegionCapability("RegionExperiences").empty())
    {
        panel = new LLPanelRegionExperiences;
        mInfoPanels.push_back(panel);
        panel->buildFromFile("panel_region_experiences.xml");
        mTab->addTabPanel(panel);
    }

    gMessageSystem->setHandlerFunc(
        "EstateOwnerMessage",
        &processEstateOwnerRequest);

    // Request region info when agent region changes.
    mRegionChangedCallback = gAgent.addRegionChangedCallback(boost::bind(&LLFloaterRegionInfo::onRegionChanged, this));

    return true;
}

LLFloaterRegionInfo::~LLFloaterRegionInfo()
{
    if (mRegionChangedCallback.connected())
    {
        mRegionChangedCallback.disconnect();
    }
}

void LLFloaterRegionInfo::onOpen(const LLSD& key)
{
    if(gDisconnected)
    {
        disableTabCtrls();
        return;
    }
    refreshFromRegion(gAgent.getRegion(), ERefreshFromRegionPhase::BeforeRequestRegionInfo);
    requestRegionInfo(true);

    if (!mGodLevelChangeSlot.connected())
    {
        mGodLevelChangeSlot = gAgent.registerGodLevelChanageListener(boost::bind(&LLFloaterRegionInfo::onGodLevelChange, this, _1));
    }
}

void LLFloaterRegionInfo::onClose(bool app_quitting)
{
    if (mGodLevelChangeSlot.connected())
    {
        mGodLevelChangeSlot.disconnect();
    }
}

void LLFloaterRegionInfo::onRegionChanged()
{
    if (getVisible()) //otherwise onOpen will do request
    {
        requestRegionInfo(false);
    }
}

void LLFloaterRegionInfo::requestRegionInfo(bool is_opening)
{
    mIsRegionInfoRequestedFromOpening = is_opening;

    LLTabContainer* tab = findChild<LLTabContainer>("region_panels");
    if (tab)
    {
        tab->getChild<LLPanel>("General")->setCtrlsEnabled(false);
        tab->getChild<LLPanel>("Debug")->setCtrlsEnabled(false);
        tab->getChild<LLPanel>("Terrain")->setAllChildrenEnabled(false, true);
        tab->getChild<LLPanel>("Estate")->setCtrlsEnabled(false);
        tab->getChild<LLPanel>("Access")->setCtrlsEnabled(false);
    }

    // Must allow anyone to request the RegionInfo data
    // so non-owners/non-gods can see the values.
    // Therefore can't use an EstateOwnerMessage JC
    LLMessageSystem* msg = gMessageSystem;
    msg->newMessage("RequestRegionInfo");
    msg->nextBlock("AgentData");
    msg->addUUID("AgentID", gAgent.getID());
    msg->addUUID("SessionID", gAgent.getSessionID());
    gAgent.sendReliableMessage();
}

// static
void LLFloaterRegionInfo::processEstateOwnerRequest(LLMessageSystem* msg,void**)
{
    static LLDispatcher dispatch;
    LLFloaterRegionInfo* floater = LLFloaterReg::getTypedInstance<LLFloaterRegionInfo>("region_info");
    if(!floater)
    {
        return;
    }

    if (!estate_dispatch_initialized)
    {
        LLPanelEstateInfo::initDispatch(dispatch);
    }

    LLPanelEstateInfo* panel = LLFloaterRegionInfo::getPanelEstate();

    // unpack the message
    std::string request;
    LLUUID invoice;
    LLDispatcher::sparam_t strings;
    LLDispatcher::unpackMessage(msg, request, invoice, strings);
    if(invoice != getLastInvoice())
    {
        LL_WARNS() << "Mismatched Estate message: " << request << LL_ENDL;
        return;
    }

    //dispatch the message
    dispatch.dispatch(request, invoice, strings);

    if (panel)
    {
        panel->updateControls(gAgent.getRegion());
    }
}


// static
void LLFloaterRegionInfo::processRegionInfo(LLMessageSystem* msg)
{
    LLPanel* panel;
    LLFloaterRegionInfo* floater = LLFloaterReg::getTypedInstance<LLFloaterRegionInfo>("region_info");
    if(!floater)
    {
        return;
    }
#if 0
    // We need to re-request environment setting here,
    // otherwise after we apply (send) updated region settings we won't get them back,
    // so our environment won't be updated.
    // This is also the way to know about externally changed region environment.
    LLEnvManagerNew::instance().requestRegionSettings();
#endif
    LLTabContainer* tab = floater->getChild<LLTabContainer>("region_panels");

    LLViewerRegion* region = gAgent.getRegion();
    bool allow_modify = gAgent.isGodlike() || (region && region->canManageEstate());

    // *TODO: Replace parsing msg with accessing the region info model.
    LLRegionInfoModel& region_info = LLRegionInfoModel::instance();

    // extract message
    std::string sim_name;
    std::string sim_type = LLTrans::getString("land_type_unknown");
    U64 region_flags;
    U8 agent_limit;
    S32 hard_agent_limit;
    F32 object_bonus_factor;
    U8 sim_access;
    F32 water_height;
    F32 terrain_raise_limit;
    F32 terrain_lower_limit;
    bool use_estate_sun;
    F32 sun_hour;
    msg->getString("RegionInfo", "SimName", sim_name);
    msg->getU8("RegionInfo", "MaxAgents", agent_limit);
    msg->getS32("RegionInfo2", "HardMaxAgents", hard_agent_limit);
    msg->getF32("RegionInfo", "ObjectBonusFactor", object_bonus_factor);
    msg->getU8("RegionInfo", "SimAccess", sim_access);
    msg->getF32Fast(_PREHASH_RegionInfo, _PREHASH_WaterHeight, water_height);
    msg->getF32Fast(_PREHASH_RegionInfo, _PREHASH_TerrainRaiseLimit, terrain_raise_limit);
    msg->getF32Fast(_PREHASH_RegionInfo, _PREHASH_TerrainLowerLimit, terrain_lower_limit);
    msg->getBOOL("RegionInfo", "UseEstateSun", use_estate_sun);
    // actually the "last set" sun hour, not the current sun hour. JC
    msg->getF32("RegionInfo", "SunHour", sun_hour);
    // the only reasonable way to decide if we actually have any data is to
    // check to see if any of these fields have nonzero sizes
    if (msg->getSize("RegionInfo2", "ProductSKU") > 0 ||
        msg->getSize("RegionInfo2", "ProductName") > 0)
    {
        msg->getString("RegionInfo2", "ProductName", sim_type);
        LLTrans::findString(sim_type, sim_type); // try localizing sim product name
    }

    if (msg->has(_PREHASH_RegionInfo3))
    {
        msg->getU64("RegionInfo3", "RegionFlagsExtended", region_flags);
    }
    else
    {
        U32 flags = 0;
        msg->getU32("RegionInfo", "RegionFlags", flags);
        region_flags = flags;
    }

    if (msg->has(_PREHASH_RegionInfo5))
    {
        F32 chat_whisper_range;
        F32 chat_normal_range;
        F32 chat_shout_range;
        F32 chat_whisper_offset;
        F32 chat_normal_offset;
        F32 chat_shout_offset;
        U32 chat_flags;

        msg->getF32Fast(_PREHASH_RegionInfo5, _PREHASH_ChatWhisperRange, chat_whisper_range);
        msg->getF32Fast(_PREHASH_RegionInfo5, _PREHASH_ChatNormalRange, chat_normal_range);
        msg->getF32Fast(_PREHASH_RegionInfo5, _PREHASH_ChatShoutRange, chat_shout_range);
        msg->getF32Fast(_PREHASH_RegionInfo5, _PREHASH_ChatWhisperOffset, chat_whisper_offset);
        msg->getF32Fast(_PREHASH_RegionInfo5, _PREHASH_ChatNormalOffset, chat_normal_offset);
        msg->getF32Fast(_PREHASH_RegionInfo5, _PREHASH_ChatShoutOffset, chat_shout_offset);
        msg->getU32Fast(_PREHASH_RegionInfo5, _PREHASH_ChatFlags, chat_flags);

        LL_INFOS() << "Whisper range: " << chat_whisper_range << " normal range: " << chat_normal_range << " shout range: " << chat_shout_range
            << " whisper offset: " << chat_whisper_offset << " normal offset: " << chat_normal_offset << " shout offset: " << chat_shout_offset
            << " chat flags: " << chat_flags << LL_ENDL;
    }

    // GENERAL PANEL
    panel = tab->getChild<LLPanel>("General");
    panel->getChild<LLUICtrl>("region_text")->setValue(LLSD(sim_name));
    panel->getChild<LLUICtrl>("region_type")->setValue(LLSD(sim_type));
    panel->getChild<LLUICtrl>("version_channel_text")->setValue(gLastVersionChannel);

    panel->getChild<LLUICtrl>("block_terraform_check")->setValue(is_flag_set(region_flags, REGION_FLAGS_BLOCK_TERRAFORM));
    panel->getChild<LLUICtrl>("block_fly_check")->setValue(is_flag_set(region_flags, REGION_FLAGS_BLOCK_FLY));
    panel->getChild<LLUICtrl>("block_fly_over_check")->setValue(is_flag_set(region_flags, REGION_FLAGS_BLOCK_FLYOVER));
    panel->getChild<LLUICtrl>("allow_damage_check")->setValue(is_flag_set(region_flags, REGION_FLAGS_ALLOW_DAMAGE));
    panel->getChild<LLUICtrl>("restrict_pushobject")->setValue(is_flag_set(region_flags, REGION_FLAGS_RESTRICT_PUSHOBJECT));
    panel->getChild<LLUICtrl>("allow_land_resell_check")->setValue(!is_flag_set(region_flags, REGION_FLAGS_BLOCK_LAND_RESELL));
    panel->getChild<LLUICtrl>("allow_parcel_changes_check")->setValue(is_flag_set(region_flags, REGION_FLAGS_ALLOW_PARCEL_CHANGES));
    panel->getChild<LLUICtrl>("block_parcel_search_check")->setValue(is_flag_set(region_flags, REGION_FLAGS_BLOCK_PARCEL_SEARCH));
    panel->getChild<LLUICtrl>("agent_limit_spin")->setValue(LLSD((F32)agent_limit));
    panel->getChild<LLUICtrl>("object_bonus_spin")->setValue(LLSD(object_bonus_factor));
    panel->getChild<LLUICtrl>("access_combo")->setValue(LLSD(sim_access));

    panel->getChild<LLSpinCtrl>("agent_limit_spin")->setMaxValue((F32)hard_agent_limit);

    if (LLPanelRegionGeneralInfo* panel_general = LLFloaterRegionInfo::getPanelGeneral())
    {
        panel_general->setObjBonusFactor(object_bonus_factor);
    }

    // detect teen grid for maturity

    U32 parent_estate_id;
    msg->getU32("RegionInfo", "ParentEstateID", parent_estate_id);
    bool teen_grid = (parent_estate_id == 5);  // *TODO add field to estate table and test that
    panel->getChildView("access_combo")->setEnabled(gAgent.isGodlike() || (region && region->canManageEstate() && !teen_grid));
    panel->setCtrlsEnabled(allow_modify);

    panel->getChild<LLLineEditor>("estate_id")->setValue((S32)region_info.mEstateID);

    if (region)
    {
        panel->getChild<LLLineEditor>("grid_position_x")->setValue((S32)(region->getOriginGlobal()[VX] / 256));
        panel->getChild<LLLineEditor>("grid_position_y")->setValue((S32)(region->getOriginGlobal()[VY] / 256));
    }
    else
    {
        panel->getChild<LLLineEditor>("grid_position_x")->setDefaultText();
        panel->getChild<LLLineEditor>("grid_position_y")->setDefaultText();
    }

    // DEBUG PANEL
    panel = tab->getChild<LLPanel>("Debug");

    panel->getChild<LLUICtrl>("region_text")->setValue(LLSD(sim_name) );
    panel->getChild<LLUICtrl>("disable_scripts_check")->setValue(LLSD((bool)(region_flags & REGION_FLAGS_SKIP_SCRIPTS)));
    panel->getChild<LLUICtrl>("disable_collisions_check")->setValue(LLSD((bool)(region_flags & REGION_FLAGS_SKIP_COLLISIONS)));
    panel->getChild<LLUICtrl>("disable_physics_check")->setValue(LLSD((bool)(region_flags & REGION_FLAGS_SKIP_PHYSICS)));
    panel->setCtrlsEnabled(allow_modify);

    // TERRAIN PANEL
    panel = tab->getChild<LLPanel>("Terrain");

    panel->getChild<LLUICtrl>("region_text")->setValue(LLSD(sim_name));
    panel->getChild<LLUICtrl>("water_height_spin")->setValue(region_info.mWaterHeight);
    panel->getChild<LLUICtrl>("terrain_raise_spin")->setValue(region_info.mTerrainRaiseLimit);
    panel->getChild<LLUICtrl>("terrain_lower_spin")->setValue(region_info.mTerrainLowerLimit);

    panel->setAllChildrenEnabled(allow_modify, true);

    if (floater->getVisible())
    {
        // Note: region info also causes LLRegionInfoModel::instance().update(msg); -> requestRegion(); -> changed message
        // we need to know env version here and in update(msg) to know when to request and when not to, when to filter 'changed'
        ERefreshFromRegionPhase phase = floater->mIsRegionInfoRequestedFromOpening ?
            ERefreshFromRegionPhase::AfterRequestRegionInfo :
            ERefreshFromRegionPhase::NotFromFloaterOpening;
        floater->refreshFromRegion(gAgent.getRegion(), phase);
    } // else will rerequest on onOpen either way
}

// static
void LLFloaterRegionInfo::refreshFromRegion(LLViewerRegion* region)
{
    if (region != gAgent.getRegion())
        return;

    if (LLFloaterRegionInfo* floater = LLFloaterReg::getTypedInstance<LLFloaterRegionInfo>("region_info"))
    {
        if (floater->getVisible() && region == gAgent.getRegion())
        {
            floater->refreshFromRegion(region, ERefreshFromRegionPhase::NotFromFloaterOpening);
        }
    }
}

// static
LLPanelEstateInfo* LLFloaterRegionInfo::getPanelEstate()
{
    LLFloaterRegionInfo* floater = LLFloaterReg::getTypedInstance<LLFloaterRegionInfo>("region_info");
    if (!floater) return NULL;
    LLTabContainer* tab = floater->getChild<LLTabContainer>("region_panels");
    LLPanelEstateInfo* panel = (LLPanelEstateInfo*)tab->getChild<LLPanel>("Estate");
    return panel;
}

// static
LLPanelEstateAccess* LLFloaterRegionInfo::getPanelAccess()
{
    LLFloaterRegionInfo* floater = LLFloaterReg::getTypedInstance<LLFloaterRegionInfo>("region_info");
    if (!floater) return NULL;
    LLTabContainer* tab = floater->getChild<LLTabContainer>("region_panels");
    LLPanelEstateAccess* panel = (LLPanelEstateAccess*)tab->getChild<LLPanel>("Access");
    return panel;
}

// static
LLPanelEstateCovenant* LLFloaterRegionInfo::getPanelCovenant()
{
    LLFloaterRegionInfo* floater = LLFloaterReg::getTypedInstance<LLFloaterRegionInfo>("region_info");
    if (!floater) return NULL;
    LLTabContainer* tab = floater->getChild<LLTabContainer>("region_panels");
    LLPanelEstateCovenant* panel = (LLPanelEstateCovenant*)tab->getChild<LLPanel>("Covenant");
    return panel;
}

// static
LLPanelRegionGeneralInfo* LLFloaterRegionInfo::getPanelGeneral()
{
    LLFloaterRegionInfo* floater = LLFloaterReg::getTypedInstance<LLFloaterRegionInfo>("region_info");
    if (!floater) return NULL;
    LLTabContainer* tab = floater->getChild<LLTabContainer>("region_panels");
    LLPanelRegionGeneralInfo* panel = (LLPanelRegionGeneralInfo*)tab->getChild<LLPanel>("General");
    return panel;
}

// static
LLPanelRegionEnvironment* LLFloaterRegionInfo::getPanelEnvironment()
{
    LLFloaterRegionInfo* floater = LLFloaterReg::getTypedInstance<LLFloaterRegionInfo>("region_info");
    if (!floater) return NULL;
    LLTabContainer* tab = floater->getChild<LLTabContainer>("region_panels");
    LLPanelRegionEnvironment* panel = (LLPanelRegionEnvironment*)tab->getChild<LLPanel>("panel_env_info");
    return panel;
}

LLTerrainMaterials::Type material_type_from_ctrl(LLCheckBoxCtrl* ctrl)
{
    return ctrl->get() ? LLTerrainMaterials::Type::PBR : LLTerrainMaterials::Type::TEXTURE;
}

void material_type_to_ctrl(LLCheckBoxCtrl* ctrl, LLTerrainMaterials::Type new_type)
{
    ctrl->set(new_type == LLTerrainMaterials::Type::PBR);
}

// static
LLPanelRegionTerrainInfo* LLFloaterRegionInfo::getPanelRegionTerrain()
{
    LLFloaterRegionInfo* floater = LLFloaterReg::getTypedInstance<LLFloaterRegionInfo>("region_info");
    if (!floater)
    {
        llassert(floater);
        return NULL;
    }

    LLTabContainer* tab_container = floater->getChild<LLTabContainer>("region_panels");
    LLPanelRegionTerrainInfo* panel =
        dynamic_cast<LLPanelRegionTerrainInfo*>(tab_container->getChild<LLPanel>("Terrain"));
    llassert(panel);
    return panel;
}

LLPanelRegionExperiences* LLFloaterRegionInfo::getPanelExperiences()
{
    LLFloaterRegionInfo* floater = LLFloaterReg::getTypedInstance<LLFloaterRegionInfo>("region_info");
    if (!floater) return NULL;
    LLTabContainer* tab = floater->getChild<LLTabContainer>("region_panels");
    return (LLPanelRegionExperiences*)tab->getChild<LLPanel>("Experiences");
}

void LLFloaterRegionInfo::disableTabCtrls()
{
    LLTabContainer* tab = getChild<LLTabContainer>("region_panels");

    tab->getChild<LLPanel>("General")->setCtrlsEnabled(false);
    tab->getChild<LLPanel>("Debug")->setCtrlsEnabled(false);
    tab->getChild<LLPanel>("Terrain")->setAllChildrenEnabled(false, true);
    tab->getChild<LLPanel>("panel_env_info")->setCtrlsEnabled(false);
    tab->getChild<LLPanel>("Estate")->setCtrlsEnabled(false);
    tab->getChild<LLPanel>("Access")->setCtrlsEnabled(false);
}

void LLFloaterRegionInfo::onTabSelected(const LLSD& param)
{
    LLPanel* active_panel = getChild<LLPanel>(param.asString());
    active_panel->onOpen(LLSD());
}

void LLFloaterRegionInfo::refreshFromRegion(LLViewerRegion* region, ERefreshFromRegionPhase phase)
{
    if (!region)
    {
        return;
    }

    // call refresh from region on all panels
    for (const auto& infoPanel : mInfoPanels)
    {
        infoPanel->refreshFromRegion(region, phase);
    }
    mEnvironmentPanel->refreshFromRegion(region);
}

// public
void LLFloaterRegionInfo::refresh()
{
    for(info_panels_t::iterator iter = mInfoPanels.begin();
        iter != mInfoPanels.end(); ++iter)
    {
        (*iter)->refresh();
    }
    mEnvironmentPanel->refresh();
}

void LLFloaterRegionInfo::enableTopButtons()
{
    getChildView("top_colliders_btn")->setEnabled(true);
    getChildView("top_scripts_btn")->setEnabled(true);
}

void LLFloaterRegionInfo::disableTopButtons()
{
    getChildView("top_colliders_btn")->setEnabled(false);
    getChildView("top_scripts_btn")->setEnabled(false);
}

void LLFloaterRegionInfo::onGodLevelChange(U8 god_level)
{
    LLFloaterRegionInfo* floater = LLFloaterReg::getTypedInstance<LLFloaterRegionInfo>("region_info");
    if (floater && floater->getVisible())
    {
        refreshFromRegion(gAgent.getRegion(), ERefreshFromRegionPhase::NotFromFloaterOpening);
    }
}

///----------------------------------------------------------------------------
/// Local class implementation
///----------------------------------------------------------------------------

//
// LLPanelRegionInfo
//

LLPanelRegionInfo::LLPanelRegionInfo()
    : LLPanel()
{
}

void LLPanelRegionInfo::onBtnSet()
{
    if (sendUpdate())
    {
        disableButton("apply_btn");
    }
}

void LLPanelRegionInfo::onChangeChildCtrl(LLUICtrl* ctrl)
{
    updateChild(ctrl); // virtual function
}

// Enables the "set" button if it is not already enabled
void LLPanelRegionInfo::onChangeAnything()
{
    enableButton("apply_btn");
    refresh();
}

// static
// Enables set button on change to line editor
void LLPanelRegionInfo::onChangeText(LLLineEditor* caller, void* user_data)
{
    LLPanelRegionInfo* panel = dynamic_cast<LLPanelRegionInfo*>(caller->getParent());
    if(panel)
    {
        panel->enableButton("apply_btn");
        panel->refresh();
    }
}


// virtual
bool LLPanelRegionInfo::postBuild()
{
    // If the panel has an Apply button, set a callback for it.
    LLUICtrl* apply_btn = findChild<LLUICtrl>("apply_btn");
    if (apply_btn)
    {
        apply_btn->setCommitCallback(boost::bind(&LLPanelRegionInfo::onBtnSet, this));
    }

    refresh();
    return true;
}

// virtual
void LLPanelRegionInfo::updateChild(LLUICtrl* child_ctr)
{
}

// virtual
bool LLPanelRegionInfo::refreshFromRegion(LLViewerRegion* region, ERefreshFromRegionPhase phase)
{
    if (region) mHost = region->getHost();
    return true;
}

void LLPanelRegionInfo::sendEstateOwnerMessage(
    LLMessageSystem* msg,
    const std::string& request,
    const LLUUID& invoice,
    const strings_t& strings)
{
    LL_INFOS() << "Sending estate request '" << request << "'" << LL_ENDL;
    msg->newMessage("EstateOwnerMessage");
    msg->nextBlockFast(_PREHASH_AgentData);
    msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
    msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
    msg->addUUIDFast(_PREHASH_TransactionID, LLUUID::null); //not used
    msg->nextBlock("MethodData");
    msg->addString("Method", request);
    msg->addUUID("Invoice", invoice);
    if(strings.empty())
    {
        msg->nextBlock("ParamList");
        msg->addString("Parameter", NULL);
    }
    else
    {
        for (const std::string& string : strings)
        {
            msg->nextBlock("ParamList");
            msg->addString("Parameter", string);
        }
    }
    msg->sendReliable(mHost);
}

void LLPanelRegionInfo::enableButton(const std::string& btn_name, bool enable)
{
    LLView* button = findChildView(btn_name);
    if (button) button->setEnabled(enable);
}

void LLPanelRegionInfo::disableButton(const std::string& btn_name)
{
    LLView* button = findChildView(btn_name);
    if (button) button->setEnabled(false);
}

void LLPanelRegionInfo::initCtrl(const std::string& name)
{
    getChild<LLUICtrl>(name)->setCommitCallback(boost::bind(&LLPanelRegionInfo::onChangeAnything, this));
}

void LLPanelRegionInfo::initAndSetTexCtrl(LLTextureCtrl*& ctrl, const std::string& name)
{
    ctrl = findChild<LLTextureCtrl>(name);
    if (ctrl)
        ctrl->setOnSelectCallback([this](LLUICtrl* ctrl, const LLSD& param){ onChangeAnything(); });
}

template<typename CTRL>
void LLPanelRegionInfo::initAndSetCtrl(CTRL*& ctrl, const std::string& name)
{
    ctrl = findChild<CTRL>(name);
    if (ctrl)
        ctrl->setCommitCallback(boost::bind(&LLPanelRegionInfo::onChangeAnything, this));
}

void LLPanelRegionInfo::onClickManageTelehub()
{
    LLFloaterReg::hideInstance("region_info");
    LLFloaterReg::showInstance("telehubs");
}

void LLPanelRegionInfo::onClickManageRestartSchedule()
{
    LLFloater* floaterp = mFloaterRestartScheduleHandle.get();
    // Show the dialog
    if (!floaterp)
    {
        floaterp = new LLFloaterRegionRestartSchedule(this);
    }

    if (floaterp->getVisible())
    {
        floaterp->closeFloater();
    }
    else
    {
        floaterp->openFloater();
    }
}

/////////////////////////////////////////////////////////////////////////////
// LLPanelRegionGeneralInfo
//
bool LLPanelRegionGeneralInfo::refreshFromRegion(LLViewerRegion* region, ERefreshFromRegionPhase phase)
{
    bool allow_modify = gAgent.isGodlike() || (region && region->canManageEstate());
    setCtrlsEnabled(allow_modify);
    getChildView("apply_btn")->setEnabled(false);
    getChildView("access_text")->setEnabled(allow_modify);
    // getChildView("access_combo")->setEnabled(allow_modify);
    getChildView("estate_id")->setEnabled(false);
    getChildView("grid_position_x")->setEnabled(false);
    getChildView("grid_position_y")->setEnabled(false);
    // now set in processRegionInfo for teen grid detection
    getChildView("kick_btn")->setEnabled(allow_modify);
    getChildView("kick_all_btn")->setEnabled(allow_modify);
    getChildView("im_btn")->setEnabled(allow_modify);
    getChildView("manage_telehub_btn")->setEnabled(allow_modify);
    getChildView("manage_restart_btn")->setEnabled(allow_modify);
    getChildView("manage_restart_btn")->setVisible(LLFloaterRegionRestartSchedule::canUse());

    // Data gets filled in by processRegionInfo

    return LLPanelRegionInfo::refreshFromRegion(region, phase);
}

bool LLPanelRegionGeneralInfo::postBuild()
{
    // Enable the "Apply" button if something is changed. JC
    initCtrl("block_terraform_check");
    initCtrl("block_fly_check");
    initCtrl("block_fly_over_check");
    initCtrl("allow_damage_check");
    initCtrl("allow_land_resell_check");
    initCtrl("allow_parcel_changes_check");
    initCtrl("agent_limit_spin");
    initCtrl("object_bonus_spin");
    initCtrl("access_combo");
    initCtrl("restrict_pushobject");
    initCtrl("block_parcel_search_check");

    childSetAction("kick_btn", boost::bind(&LLPanelRegionGeneralInfo::onClickKick, this));
    childSetAction("kick_all_btn", onClickKickAll, this);
    childSetAction("im_btn", onClickMessage, this);
//  childSetAction("manage_telehub_btn", onClickManageTelehub, this);

    LLUICtrl* apply_btn = findChild<LLUICtrl>("apply_btn");
    if (apply_btn)
    {
        apply_btn->setCommitCallback(boost::bind(&LLPanelRegionGeneralInfo::onBtnSet, this));
    }

    refresh();
    return true;
}

void LLPanelRegionGeneralInfo::onBtnSet()
{
    if(mObjBonusFactor == getChild<LLUICtrl>("object_bonus_spin")->getValue().asReal())
    {
        if (sendUpdate())
        {
            disableButton("apply_btn");
        }
    }
    else
    {
        LLNotificationsUtil::add("ChangeObjectBonusFactor", LLSD(), LLSD(), boost::bind(&LLPanelRegionGeneralInfo::onChangeObjectBonus, this, _1, _2));
    }
}

bool LLPanelRegionGeneralInfo::onChangeObjectBonus(const LLSD& notification, const LLSD& response)
{
    S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
    if (option == 0)
    {
        if (sendUpdate())
        {
            disableButton("apply_btn");
        }
    }
    return false;
}

void LLPanelRegionGeneralInfo::onClickKick()
{
    LL_INFOS() << "LLPanelRegionGeneralInfo::onClickKick" << LL_ENDL;

    // this depends on the grandparent view being a floater
    // in order to set up floater dependency
    LLView * button = findChild<LLButton>("kick_btn");
    LLFloater* parent_floater = gFloaterView->getParentFloater(this);
    LLFloater* child_floater = LLFloaterAvatarPicker::show(boost::bind(&LLPanelRegionGeneralInfo::onKickCommit, this, _1),
                                                                                false, true, false, parent_floater->getName(), button);
    if (child_floater)
    {
        parent_floater->addDependentFloater(child_floater);
    }
}

void LLPanelRegionGeneralInfo::onKickCommit(const uuid_vec_t& ids)
{
    if (ids.empty()) return;
    if(ids[0].notNull())
    {
        strings_t strings;
        // [0] = our agent id
        // [1] = target agent id
        std::string buffer;
        gAgent.getID().toString(buffer);
        strings.push_back(buffer);

        ids[0].toString(buffer);
        strings.push_back(strings_t::value_type(buffer));

        LLUUID invoice(LLFloaterRegionInfo::getLastInvoice());
        sendEstateOwnerMessage(gMessageSystem, "teleporthomeuser", invoice, strings);
    }
}

// static
void LLPanelRegionGeneralInfo::onClickKickAll(void* userdata)
{
    LL_INFOS() << "LLPanelRegionGeneralInfo::onClickKickAll" << LL_ENDL;
    LLNotificationsUtil::add("KickUsersFromRegion",
                                    LLSD(),
                                    LLSD(),
                                    boost::bind(&LLPanelRegionGeneralInfo::onKickAllCommit, (LLPanelRegionGeneralInfo*)userdata, _1, _2));
}

bool LLPanelRegionGeneralInfo::onKickAllCommit(const LLSD& notification, const LLSD& response)
{
    S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
    if (option == 0)
    {
        strings_t strings;
        // [0] = our agent id
        std::string buffer;
        gAgent.getID().toString(buffer);
        strings.push_back(buffer);

        LLUUID invoice(LLFloaterRegionInfo::getLastInvoice());
        // historical message name
        sendEstateOwnerMessage(gMessageSystem, "teleporthomeallusers", invoice, strings);
    }
    return false;
}

// static
void LLPanelRegionGeneralInfo::onClickMessage(void* userdata)
{
    LL_INFOS() << "LLPanelRegionGeneralInfo::onClickMessage" << LL_ENDL;
    LLNotificationsUtil::add("MessageRegion",
        LLSD(),
        LLSD(),
        boost::bind(&LLPanelRegionGeneralInfo::onMessageCommit, (LLPanelRegionGeneralInfo*)userdata, _1, _2));
}

// static
bool LLPanelRegionGeneralInfo::onMessageCommit(const LLSD& notification, const LLSD& response)
{
    if(LLNotificationsUtil::getSelectedOption(notification, response) != 0) return false;

    std::string text = response["message"].asString();
    if (text.empty()) return false;

    LL_INFOS() << "Message to everyone: " << text << LL_ENDL;
    strings_t strings;
    // [0] grid_x, unused here
    // [1] grid_y, unused here
    // [2] agent_id of sender
    // [3] sender name
    // [4] message
    strings.push_back("-1");
    strings.push_back("-1");
    std::string buffer;
    gAgent.getID().toString(buffer);
    strings.push_back(buffer);
    std::string name;
    LLAgentUI::buildFullname(name);
    strings.push_back(strings_t::value_type(name));
    strings.push_back(strings_t::value_type(text));
    LLUUID invoice(LLFloaterRegionInfo::getLastInvoice());
    sendEstateOwnerMessage(gMessageSystem, "simulatormessage", invoice, strings);
    return false;
}

// setregioninfo
// strings[0] = 'Y' - block terraform, 'N' - not
// strings[1] = 'Y' - block fly, 'N' - not
// strings[2] = 'Y' - allow damage, 'N' - not
// strings[3] = 'Y' - allow land sale, 'N' - not
// strings[4] = agent limit
// strings[5] = object bonus
// strings[6] = sim access (0 = unknown, 13 = PG, 21 = Mature, 42 = Adult)
// strings[7] = restrict pushobject
// strings[8] = 'Y' - allow parcel subdivide, 'N' - not
// strings[9] = 'Y' - block parcel search, 'N' - allow
bool LLPanelRegionGeneralInfo::sendUpdate()
{
    LL_INFOS() << "LLPanelRegionGeneralInfo::sendUpdate()" << LL_ENDL;

    // First try using a Cap.  If that fails use the old method.
    LLSD body;
    std::string url = gAgent.getRegionCapability("DispatchRegionInfo");
    if (!url.empty())
    {
        body["block_terraform"] = getChild<LLUICtrl>("block_terraform_check")->getValue();
        body["block_fly"] = getChild<LLUICtrl>("block_fly_check")->getValue();
        body["block_fly_over"] = getChild<LLUICtrl>("block_fly_over_check")->getValue();
        body["allow_damage"] = getChild<LLUICtrl>("allow_damage_check")->getValue();
        body["allow_land_resell"] = getChild<LLUICtrl>("allow_land_resell_check")->getValue();
        body["agent_limit"] = getChild<LLUICtrl>("agent_limit_spin")->getValue();
        body["prim_bonus"] = getChild<LLUICtrl>("object_bonus_spin")->getValue();
        body["sim_access"] = getChild<LLUICtrl>("access_combo")->getValue();
        body["restrict_pushobject"] = getChild<LLUICtrl>("restrict_pushobject")->getValue();
        body["allow_parcel_changes"] = getChild<LLUICtrl>("allow_parcel_changes_check")->getValue();
        body["block_parcel_search"] = getChild<LLUICtrl>("block_parcel_search_check")->getValue();

        LLCoreHttpUtil::HttpCoroutineAdapter::messageHttpPost(url, body,
            "Region info update posted.", "Region info update not posted.");
    }
    else
    {
        strings_t strings;
        std::string buffer;

        buffer = llformat("%s", (getChild<LLUICtrl>("block_terraform_check")->getValue().asBoolean() ? "Y" : "N"));
        strings.push_back(strings_t::value_type(buffer));

        buffer = llformat("%s", (getChild<LLUICtrl>("block_fly_check")->getValue().asBoolean() ? "Y" : "N"));
        strings.push_back(strings_t::value_type(buffer));

        buffer = llformat("%s", (getChild<LLUICtrl>("allow_damage_check")->getValue().asBoolean() ? "Y" : "N"));
        strings.push_back(strings_t::value_type(buffer));

        buffer = llformat("%s", (getChild<LLUICtrl>("allow_land_resell_check")->getValue().asBoolean() ? "Y" : "N"));
        strings.push_back(strings_t::value_type(buffer));

        F32 value = (F32)getChild<LLUICtrl>("agent_limit_spin")->getValue().asReal();
        buffer = llformat("%f", value);
        strings.push_back(strings_t::value_type(buffer));

        value = (F32)getChild<LLUICtrl>("object_bonus_spin")->getValue().asReal();
        buffer = llformat("%f", value);
        strings.push_back(strings_t::value_type(buffer));

        buffer = llformat("%d", getChild<LLUICtrl>("access_combo")->getValue().asInteger());
        strings.push_back(strings_t::value_type(buffer));

        buffer = llformat("%s", (getChild<LLUICtrl>("restrict_pushobject")->getValue().asBoolean() ? "Y" : "N"));
        strings.push_back(strings_t::value_type(buffer));

        buffer = llformat("%s", (getChild<LLUICtrl>("allow_parcel_changes_check")->getValue().asBoolean() ? "Y" : "N"));
        strings.push_back(strings_t::value_type(buffer));

        LLUUID invoice(LLFloaterRegionInfo::getLastInvoice());
        sendEstateOwnerMessage(gMessageSystem, "setregioninfo", invoice, strings);
    }

    // if we changed access levels, tell user about it
    LLViewerRegion* region = gAgent.getRegion();
    if (region && (getChild<LLUICtrl>("access_combo")->getValue().asInteger() != region->getSimAccess()) )
    {
        LLNotificationsUtil::add("RegionMaturityChange");
    }

    return true;
}

/////////////////////////////////////////////////////////////////////////////
// LLPanelRegionDebugInfo
/////////////////////////////////////////////////////////////////////////////
bool LLPanelRegionDebugInfo::postBuild()
{
    LLPanelRegionInfo::postBuild();
    initCtrl("disable_scripts_check");
    initCtrl("disable_collisions_check");
    initCtrl("disable_physics_check");

    childSetAction("choose_avatar_btn", boost::bind(&LLPanelRegionDebugInfo::onClickChooseAvatar, this));
    childSetAction("return_btn", onClickReturn, this);
    childSetAction("top_colliders_btn", onClickTopColliders, this);
    childSetAction("top_scripts_btn", onClickTopScripts, this);
    childSetAction("restart_btn", onClickRestart, this);
    childSetAction("cancel_restart_btn", onClickCancelRestart, this);
    childSetAction("region_debug_console_btn", onClickDebugConsole, this);

    return true;
}

// virtual
bool LLPanelRegionDebugInfo::refreshFromRegion(LLViewerRegion* region, ERefreshFromRegionPhase phase)
{
    bool allow_modify = gAgent.isGodlike() || (region && region->canManageEstate());
    setCtrlsEnabled(allow_modify);
    getChildView("apply_btn")->setEnabled(false);
    getChildView("target_avatar_name")->setEnabled(false);

    getChildView("choose_avatar_btn")->setEnabled(allow_modify);
    getChildView("return_scripts")->setEnabled(allow_modify && !mTargetAvatar.isNull());
    getChildView("return_other_land")->setEnabled(allow_modify && !mTargetAvatar.isNull());
    getChildView("return_estate_wide")->setEnabled(allow_modify && !mTargetAvatar.isNull());
    getChildView("return_btn")->setEnabled(allow_modify && !mTargetAvatar.isNull());
    getChildView("top_colliders_btn")->setEnabled(allow_modify);
    getChildView("top_scripts_btn")->setEnabled(allow_modify);
    getChildView("restart_btn")->setEnabled(allow_modify);
    getChildView("cancel_restart_btn")->setEnabled(allow_modify);
    getChildView("region_debug_console_btn")->setEnabled(allow_modify);

    return LLPanelRegionInfo::refreshFromRegion(region, phase);
}

// virtual
bool LLPanelRegionDebugInfo::sendUpdate()
{
    LL_INFOS() << "LLPanelRegionDebugInfo::sendUpdate" << LL_ENDL;
    strings_t strings;
    std::string buffer;

    buffer = llformat("%s", (getChild<LLUICtrl>("disable_scripts_check")->getValue().asBoolean() ? "Y" : "N"));
    strings.push_back(buffer);

    buffer = llformat("%s", (getChild<LLUICtrl>("disable_collisions_check")->getValue().asBoolean() ? "Y" : "N"));
    strings.push_back(buffer);

    buffer = llformat("%s", (getChild<LLUICtrl>("disable_physics_check")->getValue().asBoolean() ? "Y" : "N"));
    strings.push_back(buffer);

    LLUUID invoice(LLFloaterRegionInfo::getLastInvoice());
    sendEstateOwnerMessage(gMessageSystem, "setregiondebug", invoice, strings);
    return true;
}

void LLPanelRegionDebugInfo::onClickChooseAvatar()
{
    LLView * button = findChild<LLButton>("choose_avatar_btn");
    LLFloater* parent_floater = gFloaterView->getParentFloater(this);
    LLFloater * child_floater = LLFloaterAvatarPicker::show(boost::bind(&LLPanelRegionDebugInfo::callbackAvatarID, this, _1, _2),
                                                                                    false, true, false, parent_floater->getName(), button);
    if (child_floater)
    {
        parent_floater->addDependentFloater(child_floater);
    }
}


void LLPanelRegionDebugInfo::callbackAvatarID(const uuid_vec_t& ids, const std::vector<LLAvatarName> names)
{
    if (ids.empty() || names.empty()) return;
    mTargetAvatar = ids[0];
    getChild<LLUICtrl>("target_avatar_name")->setValue(LLSD(names[0].getCompleteName()));
    refreshFromRegion(gAgent.getRegion(), ERefreshFromRegionPhase::NotFromFloaterOpening);
}

// static
void LLPanelRegionDebugInfo::onClickReturn(void* data)
{
    LLPanelRegionDebugInfo* panelp = (LLPanelRegionDebugInfo*) data;
    if (panelp->mTargetAvatar.isNull()) return;

    LLSD args;
    args["USER_NAME"] = panelp->getChild<LLUICtrl>("target_avatar_name")->getValue().asString();
    LLSD payload;
    payload["avatar_id"] = panelp->mTargetAvatar;

    U32 flags = SWD_ALWAYS_RETURN_OBJECTS;

    if (panelp->getChild<LLUICtrl>("return_scripts")->getValue().asBoolean())
    {
        flags |= SWD_SCRIPTED_ONLY;
    }

    if (panelp->getChild<LLUICtrl>("return_other_land")->getValue().asBoolean())
    {
        flags |= SWD_OTHERS_LAND_ONLY;
    }
    payload["flags"] = int(flags);
    payload["return_estate_wide"] = panelp->getChild<LLUICtrl>("return_estate_wide")->getValue().asBoolean();
    LLNotificationsUtil::add("EstateObjectReturn", args, payload,
                                    boost::bind(&LLPanelRegionDebugInfo::callbackReturn, panelp, _1, _2));
}

bool LLPanelRegionDebugInfo::callbackReturn(const LLSD& notification, const LLSD& response)
{
    S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
    if (option != 0) return false;

    LLUUID target_avatar = notification["payload"]["avatar_id"].asUUID();
    if (!target_avatar.isNull())
    {
        U32 flags = notification["payload"]["flags"].asInteger();
        bool return_estate_wide = notification["payload"]["return_estate_wide"];
        if (return_estate_wide)
        {
            // send as estate message - routed by spaceserver to all regions in estate
            strings_t strings;
            strings.push_back(llformat("%d", flags));
            strings.push_back(target_avatar.asString());

            LLUUID invoice(LLFloaterRegionInfo::getLastInvoice());

            sendEstateOwnerMessage(gMessageSystem, "estateobjectreturn", invoice, strings);
        }
        else
        {
            // send to this simulator only
            send_sim_wide_deletes(target_avatar, flags);
        }
    }
    return false;
}


// static
void LLPanelRegionDebugInfo::onClickTopColliders(void* data)
{
    LLPanelRegionDebugInfo* self = (LLPanelRegionDebugInfo*)data;
    strings_t strings;
    strings.push_back("1"); // one physics step
    LLUUID invoice(LLFloaterRegionInfo::getLastInvoice());
    LLFloaterTopObjects* instance = LLFloaterReg::getTypedInstance<LLFloaterTopObjects>("top_objects");
    if(!instance) return;
    LLFloaterReg::showInstance("top_objects");
    instance->clearList();
    instance->disableRefreshBtn();

    self->getChildView("top_colliders_btn")->setEnabled(false);
    self->getChildView("top_scripts_btn")->setEnabled(false);

    self->sendEstateOwnerMessage(gMessageSystem, "colliders", invoice, strings);
}

// static
void LLPanelRegionDebugInfo::onClickTopScripts(void* data)
{
    LLPanelRegionDebugInfo* self = (LLPanelRegionDebugInfo*)data;
    strings_t strings;
    strings.push_back("6"); // top 5 scripts
    LLUUID invoice(LLFloaterRegionInfo::getLastInvoice());
    LLFloaterTopObjects* instance = LLFloaterReg::getTypedInstance<LLFloaterTopObjects>("top_objects");
    if(!instance) return;
    LLFloaterReg::showInstance("top_objects");
    instance->clearList();
    instance->disableRefreshBtn();

    self->getChildView("top_colliders_btn")->setEnabled(false);
    self->getChildView("top_scripts_btn")->setEnabled(false);

    self->sendEstateOwnerMessage(gMessageSystem, "scripts", invoice, strings);
}

// static
void LLPanelRegionDebugInfo::onClickRestart(void* data)
{
    LLNotificationsUtil::add("ConfirmRestart", LLSD(), LLSD(),
        boost::bind(&LLPanelRegionDebugInfo::callbackRestart, (LLPanelRegionDebugInfo*)data, _1, _2));
}

bool LLPanelRegionDebugInfo::callbackRestart(const LLSD& notification, const LLSD& response)
{
    S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
    if (option != 0) return false;

    strings_t strings;
    strings.push_back("120");
    LLUUID invoice(LLFloaterRegionInfo::getLastInvoice());
    sendEstateOwnerMessage(gMessageSystem, "restart", invoice, strings);
    return false;
}

// static
void LLPanelRegionDebugInfo::onClickCancelRestart(void* data)
{
    LLPanelRegionDebugInfo* self = (LLPanelRegionDebugInfo*)data;
    strings_t strings;
    strings.push_back("-1");
    LLUUID invoice(LLFloaterRegionInfo::getLastInvoice());
    self->sendEstateOwnerMessage(gMessageSystem, "restart", invoice, strings);
}

// static
void LLPanelRegionDebugInfo::onClickDebugConsole(void* data)
{
    LLFloaterReg::showInstance("region_debug_console");
}

bool LLPanelRegionTerrainInfo::validateTextureSizes()
{
    if (mMaterialTypeCtrl)
    {
        const LLTerrainMaterials::Type material_type = material_type_from_ctrl(mMaterialTypeCtrl);
        const bool is_material_selected = material_type == LLTerrainMaterials::Type::PBR;
        if (is_material_selected) { return true; }
    }

    bool valid = true;
    static LLCachedControl<U32> max_texture_resolution(gSavedSettings, "RenderMaxTextureResolution", 2048);
    const S32 max_terrain_texture_size = (S32)max_texture_resolution;
    for(S32 i = 0; i < LLTerrainMaterials::ASSET_COUNT; ++i)
    {
        LLTextureCtrl* texture_ctrl = mTextureDetailCtrl[i];
        if (!texture_ctrl) continue;

        LLUUID image_asset_id = texture_ctrl->getImageAssetID();
        LLViewerFetchedTexture* img = LLViewerTextureManager::getFetchedTexture(image_asset_id);
        S32 components = img->getComponents();
        // Must ask for highest resolution version's width. JC
        S32 width = img->getFullWidth();
        S32 height = img->getFullHeight();

        //LL_INFOS() << "texture detail " << i << " is " << width << "x" << height << "x" << components << LL_ENDL;

        if (components != 3 && components != 4)
        {
            LLSD args;
            args["TEXTURE_NUM"] = i+1;
            args["TEXTURE_BIT_DEPTH"] = llformat("%d",components * 8);
            args["MAX_SIZE"] = max_terrain_texture_size;
            LLNotificationsUtil::add("InvalidTerrainBitDepth", args);
            valid = false;
            continue;
        }

        if (components == 4)
        {
            if (!img->hasSavedRawImage())
            {
                // Raw image isn't loaded yet
                // Assume it's invalid due to presence of alpha channel
                LLSD args;
                args["TEXTURE_NUM"] = i+1;
                args["TEXTURE_BIT_DEPTH"] = llformat("%d",components * 8);
                LLNotificationsUtil::add("InvalidTerrainAlphaNotFullyLoaded", args);
                valid = false;
            }
            else
            {
                // Slower path: Calculate alpha from raw image pixels (not needed
                // for GLTF materials, which use alphaMode to determine
                // transparency)
                // Raw image is pretty much guaranteed to be saved due to the texture swatches
                LLImageRaw* raw = img->getSavedRawImage();
                if (raw->checkHasTransparentPixels())
                {
                    LLSD args;
                    args["TEXTURE_NUM"] = i+1;
                    LLNotificationsUtil::add("InvalidTerrainAlpha", args);
                    valid = false;
                }
                LL_WARNS() << "Terrain texture image in slot " << i << " with ID " << image_asset_id << " has alpha channel, but pixels are opaque. Is alpha being optimized away in the texture uploader?" << LL_ENDL;
            }
        }

        if (width > max_terrain_texture_size || height > max_terrain_texture_size)
        {

            LLSD args;
            args["TEXTURE_NUM"] = i+1;
            args["TEXTURE_SIZE_X"] = width;
            args["TEXTURE_SIZE_Y"] = height;
            args["MAX_SIZE"] = max_terrain_texture_size;
            LLNotificationsUtil::add("InvalidTerrainSize", args);
            valid = false;
        }
    }

    return valid;
}

bool LLPanelRegionTerrainInfo::validateMaterials()
{
    if (mMaterialTypeCtrl)
    {
        const LLTerrainMaterials::Type material_type = material_type_from_ctrl(mMaterialTypeCtrl);
        const bool is_texture_selected = material_type == LLTerrainMaterials::Type::TEXTURE;
        if (is_texture_selected) { return true; }
    }

    // *TODO: If/when we implement additional GLTF extensions, they may not be
    // compatible with our GLTF terrain implementation. We may want to disallow
    // materials with some features from being set on terrain, if their
    // implementation on terrain is not compliant with the spec:
    //     - KHR_materials_transmission: Probably OK?
    //     - KHR_materials_ior: Probably OK?
    //     - KHR_materials_volume: Likely incompatible, as our terrain
    //       heightmaps cannot currently be described as finite enclosed
    //       volumes.
    // See also LLGLTFMaterial
#ifdef LL_WINDOWS
    llassert(sizeof(LLGLTFMaterial) == 232);
#endif

    bool valid = true;
    for (S32 i = 0; i < LLTerrainMaterials::ASSET_COUNT; ++i)
    {
        LLTextureCtrl* material_ctrl = mMaterialDetailCtrl[i];
        if (!material_ctrl) { continue; }

        const LLUUID& material_asset_id = material_ctrl->getImageAssetID();
        llassert(material_asset_id.notNull());
        if (material_asset_id.isNull()) { return false; }
        const LLFetchedGLTFMaterial* material = gGLTFMaterialList.getMaterial(material_asset_id);
        if (!material->isLoaded())
        {
            if (material->isFetching())
            {
                LLSD args;
                args["MATERIAL_NUM"] = i + 1;
                LLNotificationsUtil::add("InvalidTerrainMaterialNotLoaded", args);
            }
            else // Loading failed
            {
                LLSD args;
                args["MATERIAL_NUM"] = i + 1;
                LLNotificationsUtil::add("InvalidTerrainMaterialLoadFailed", args);
            }
            valid = false;
            continue;
        }

        if (material->mDoubleSided)
        {
            LLSD args;
            args["MATERIAL_NUM"] = i + 1;
            LLNotificationsUtil::add("InvalidTerrainMaterialDoubleSided", args);
            valid = false;
        }
        if (material->mAlphaMode != LLGLTFMaterial::ALPHA_MODE_OPAQUE && material->mAlphaMode != LLGLTFMaterial::ALPHA_MODE_MASK)
        {
            LLSD args;
            args["MATERIAL_NUM"] = i + 1;
            const char* alpha_mode = material->getAlphaMode();
            args["MATERIAL_ALPHA_MODE"] = alpha_mode;
            LLNotificationsUtil::add("InvalidTerrainMaterialAlphaMode", args);
            valid = false;
        }
    }

    return valid;
}

bool LLPanelRegionTerrainInfo::validateTextureHeights()
{
    for (S32 i = 0; i < CORNER_COUNT; ++i)
    {
        std::string low = llformat("height_start_spin_%d", i);
        std::string high = llformat("height_range_spin_%d", i);

        if (getChild<LLUICtrl>(low)->getValue().asReal() > getChild<LLUICtrl>(high)->getValue().asReal())
        {
            return false;
        }
    }

    return true;
}

/////////////////////////////////////////////////////////////////////////////
// LLPanelRegionTerrainInfo
/////////////////////////////////////////////////////////////////////////////

LLPanelRegionTerrainInfo::LLPanelRegionTerrainInfo()
: LLPanelRegionInfo()
{
    const LLUUID (&default_textures)[LLVLComposition::ASSET_COUNT] = LLVLComposition::getDefaultTextures();
    for (S32 i = 0; i < LLTerrainMaterials::ASSET_COUNT; ++i)
    {
        mTextureDetailCtrl[i] = nullptr;
        mMaterialDetailCtrl[i] = nullptr;

        mLastSetTextures[i] = default_textures[i];
        mLastSetMaterials[i] = BLANK_MATERIAL_ASSET_ID;

        mMaterialScaleUCtrl[i] = nullptr;
        mMaterialScaleVCtrl[i] = nullptr;
        mMaterialRotationCtrl[i] = nullptr;
        mMaterialOffsetUCtrl[i] = nullptr;
        mMaterialOffsetVCtrl[i] = nullptr;
    }
}

// Initialize statics

bool LLPanelRegionTerrainInfo::postBuild()
{
    LLPanelRegionInfo::postBuild();

    initCtrl("water_height_spin");
    initCtrl("terrain_raise_spin");
    initCtrl("terrain_lower_spin");

    mMaterialTypeCtrl = findChild<LLCheckBoxCtrl>("terrain_material_type");
    if (mMaterialTypeCtrl) { mMaterialTypeCtrl->setCommitCallback(boost::bind(&LLPanelRegionTerrainInfo::onSelectMaterialType, this)); }

    std::string buffer;

    for(S32 i = 0; i < LLTerrainMaterials::ASSET_COUNT; ++i)
    {
        initAndSetTexCtrl(mTextureDetailCtrl[i], llformat("texture_detail_%d", i));
        if (mTextureDetailCtrl[i])
        {
            mTextureDetailCtrl[i]->setBakeTextureEnabled(false);
        }
        initMaterialCtrl(mMaterialDetailCtrl[i], llformat("material_detail_%d", i), i);

        initAndSetCtrl(mMaterialScaleUCtrl[i], llformat("terrain%dScaleU", i));
        initAndSetCtrl(mMaterialScaleVCtrl[i], llformat("terrain%dScaleV", i));
        initAndSetCtrl(mMaterialRotationCtrl[i], llformat("terrain%dRotation", i));
        initAndSetCtrl(mMaterialOffsetUCtrl[i], llformat("terrain%dOffsetU", i));
        initAndSetCtrl(mMaterialOffsetVCtrl[i], llformat("terrain%dOffsetV", i));
    }

    for(S32 i = 0; i < CORNER_COUNT; ++i)
    {
        buffer = llformat("height_start_spin_%d", i);
        initCtrl(buffer);
        buffer = llformat("height_range_spin_%d", i);
        initCtrl(buffer);
    }

    childSetAction("download_raw_btn", onClickDownloadRaw, this);
    childSetAction("upload_raw_btn", onClickUploadRaw, this);
    childSetAction("bake_terrain_btn", onClickBakeTerrain, this);

    mAskedTextureHeights = false;
    mConfirmedTextureHeights = false;

    return LLPanelRegionInfo::postBuild();
}

void LLPanelRegionTerrainInfo::onSelectMaterialType()
{
    updateForMaterialType();
    onChangeAnything();
}

void LLPanelRegionTerrainInfo::updateForMaterialType()
{
    if (!mMaterialTypeCtrl) { return; }
    const LLTerrainMaterials::Type material_type = material_type_from_ctrl(mMaterialTypeCtrl);
    const bool show_texture_controls = material_type == LLTerrainMaterials::Type::TEXTURE;
    const bool show_material_controls = material_type == LLTerrainMaterials::Type::PBR;

    // Toggle visibility of correct swatches
    for(S32 i = 0; i < LLTerrainMaterials::ASSET_COUNT; ++i)
    {
        LLTextureCtrl* texture_ctrl = mTextureDetailCtrl[i];
        if (texture_ctrl)
        {
            texture_ctrl->setVisible(show_texture_controls);
        }
    }
    for(S32 i = 0; i < LLTerrainMaterials::ASSET_COUNT; ++i)
    {
        LLTextureCtrl* material_ctrl = mMaterialDetailCtrl[i];
        if (material_ctrl)
        {
            material_ctrl->setVisible(show_material_controls);
        }
    }

    // Toggle visibility of terrain tabs
    LLTabContainer* terrain_tabs = findChild<LLTabContainer>("terrain_tabs");
    if (terrain_tabs)
    {
        LLPanel* pbr_terrain_repeats_tab = findChild<LLPanel>("terrain_transform_panel");
        if (pbr_terrain_repeats_tab)
        {
            terrain_tabs->setTabVisibility(pbr_terrain_repeats_tab, show_material_controls);
        }
    }

    // Toggle visibility of labels
    LLUICtrl* texture_label = findChild<LLUICtrl>("detail_texture_text");
    if (texture_label) { texture_label->setVisible(show_texture_controls); }
    LLUICtrl* material_label = findChild<LLUICtrl>("detail_material_text");
    if (material_label) { material_label->setVisible(show_material_controls); }

    // Toggle visibility of documentation labels for terrain blending ranges
    const std::vector<std::string> doc_suffixes { "5", "10", "11" };
    std::string buffer;
    for (const std::string& suffix : doc_suffixes)
    {
        buffer = "height_text_lbl" + suffix;
        LLUICtrl* texture_doc_label = findChild<LLUICtrl>(buffer);
        if (texture_doc_label) { texture_doc_label->setVisible(show_texture_controls); }
        buffer += "_material";
        LLUICtrl* material_doc_label = findChild<LLUICtrl>(buffer);
        if (material_doc_label) { material_doc_label->setVisible(show_material_controls); }
    }
}

// virtual
bool LLPanelRegionTerrainInfo::refreshFromRegion(LLViewerRegion* region, ERefreshFromRegionPhase phase)
{
    bool owner_or_god = gAgent.isGodlike()
                        || (region && (region->getOwner() == gAgent.getID()));
    bool owner_or_god_or_manager = owner_or_god
                        || (region && region->isEstateManager());
    setAllChildrenEnabled(owner_or_god_or_manager, true);

    getChildView("apply_btn")->setEnabled(false);

    if (region)
    {
        getChild<LLUICtrl>("region_text")->setValue(LLSD(region->getName()));

        LLVLComposition* compp = region->getComposition();

        static LLCachedControl<bool> feature_pbr_terrain_enabled(gSavedSettings, "RenderTerrainPBREnabled", false);

        const bool textures_ready = compp->makeTexturesReady(false, false);
        const bool materials_ready = feature_pbr_terrain_enabled() && compp->makeMaterialsReady(false, false);

        bool set_texture_swatches;
        bool set_material_swatches;
        bool reset_texture_swatches;
        bool reset_material_swatches;
        LLTerrainMaterials::Type material_type;
        if (!textures_ready && !materials_ready)
        {
            // Are these 4 texture IDs or 4 material IDs? Who knows! Let's set
            // the IDs on both pickers for now.
            material_type = LLTerrainMaterials::Type::TEXTURE;
            set_texture_swatches = true;
            set_material_swatches = true;
            reset_texture_swatches = false;
            reset_material_swatches = false;
        }
        else
        {
            material_type = compp->getMaterialType();
            set_texture_swatches = material_type == LLTerrainMaterials::Type::TEXTURE;
            set_material_swatches = !set_texture_swatches;
            reset_texture_swatches = !set_texture_swatches;
            reset_material_swatches = !set_material_swatches;
        }

        if (mMaterialTypeCtrl)
        {
            material_type_to_ctrl(mMaterialTypeCtrl, material_type);
            updateForMaterialType();
            mMaterialTypeCtrl->setVisible(feature_pbr_terrain_enabled());
        }

        if (set_texture_swatches)
        {
            for(S32 i = 0; i < LLTerrainMaterials::ASSET_COUNT; ++i)
            {
                LLTextureCtrl* asset_ctrl = mTextureDetailCtrl[i];
                if(asset_ctrl)
                {
                    LL_DEBUGS("Terrain", "Texture") << "Detail Texture " << i << ": "
                             << compp->getDetailAssetID(i) << LL_ENDL;
                    LLUUID tmp_id(compp->getDetailAssetID(i));
                    asset_ctrl->setImageAssetID(tmp_id);
                }
            }
        }
        if (set_material_swatches)
        {
            for(S32 i = 0; i < LLTerrainMaterials::ASSET_COUNT; ++i)
            {
                LLTextureCtrl* asset_ctrl = mMaterialDetailCtrl[i];
                if(asset_ctrl)
                {
                    LL_DEBUGS("Terrain", "Material") << "Detail Material " << i << ": "
                             << compp->getDetailAssetID(i) << LL_ENDL;
                    LLUUID tmp_id(compp->getDetailAssetID(i));
                    asset_ctrl->setImageAssetID(tmp_id);
                }
            }
        }
        if (reset_texture_swatches)
        {
            for(S32 i = 0; i < LLTerrainMaterials::ASSET_COUNT; ++i)
            {
                LL_DEBUGS("Terrain", "Texture") << "Reset Texture swatch " << i
                         << LL_ENDL;
                LLTextureCtrl* asset_ctrl = mTextureDetailCtrl[i];
                if(asset_ctrl)
                {
                    asset_ctrl->setImageAssetID(mLastSetTextures[i]);
                }
            }
        }
        if (reset_material_swatches)
        {
            for(S32 i = 0; i < LLTerrainMaterials::ASSET_COUNT; ++i)
            {
                LL_DEBUGS("Terrain", "Material") << "Reset Material swatch " << i
                         << LL_ENDL;
                LLTextureCtrl* asset_ctrl = mMaterialDetailCtrl[i];
                if(asset_ctrl)
                {
                    asset_ctrl->setImageAssetID(mLastSetMaterials[i]);
                }
            }
        }

        for(S32 i = 0; i < LLTerrainMaterials::ASSET_COUNT; ++i)
        {
            if (!mMaterialScaleUCtrl[i] || !mMaterialScaleVCtrl[i] || !mMaterialRotationCtrl[i] || !mMaterialOffsetUCtrl[i] || !mMaterialOffsetVCtrl[i]) { continue; }
            const LLGLTFMaterial* mat_override = compp->getMaterialOverride(i);
            if (!mat_override) { mat_override = &LLGLTFMaterial::sDefault; }

            // Assume all texture transforms have the same value
            const LLGLTFMaterial::TextureTransform& transform = mat_override->mTextureTransform[LLGLTFMaterial::GLTF_TEXTURE_INFO_BASE_COLOR];
            mMaterialScaleUCtrl[i]->setValue(transform.mScale.mV[VX]);
            mMaterialScaleVCtrl[i]->setValue(transform.mScale.mV[VY]);
            mMaterialRotationCtrl[i]->setValue(transform.mRotation * RAD_TO_DEG);
            mMaterialOffsetUCtrl[i]->setValue(transform.mOffset.mV[VX]);
            mMaterialOffsetVCtrl[i]->setValue(transform.mOffset.mV[VY]);
        }

        std::string buffer;
        for(S32 i = 0; i < CORNER_COUNT; ++i)
        {
            buffer = llformat("height_start_spin_%d", i);
            getChild<LLUICtrl>(buffer)->setValue(LLSD(compp->getStartHeight(i)));
            buffer = llformat("height_range_spin_%d", i);
            getChild<LLUICtrl>(buffer)->setValue(LLSD(compp->getHeightRange(i)));
        }
    }
    else
    {
        LL_DEBUGS() << "no region set" << LL_ENDL;
        getChild<LLUICtrl>("region_text")->setValue(LLSD(""));
    }

    // Update visibility of terrain swatches, etc
    refresh();

    getChildView("download_raw_btn")->setEnabled(owner_or_god);
    getChildView("upload_raw_btn")->setEnabled(owner_or_god);
    getChildView("bake_terrain_btn")->setEnabled(owner_or_god);

    return LLPanelRegionInfo::refreshFromRegion(region, phase);
}


// virtual
bool LLPanelRegionTerrainInfo::sendUpdate()
{
    LL_INFOS() << __FUNCTION__ << LL_ENDL;

    LLUICtrl* apply_btn = getChild<LLUICtrl>("apply_btn");
    if (apply_btn && !apply_btn->getEnabled())
    {
        LL_WARNS() << "Duplicate update, ignored" << LL_ENDL;
        return false;
    }

    // Make sure user hasn't chosen wacky textures.
    if (!validateTextureSizes())
    {
        return false;
    }

    // Prevent applying unsupported alpha blend/double-sided materials
    if (!validateMaterials())
    {
        return false;
    }

    // Check if terrain Elevation Ranges are correct
    if (gSavedSettings.getBOOL("RegionCheckTextureHeights") && !validateTextureHeights())
    {
        if (!mAskedTextureHeights)
        {
            LLNotificationsUtil::add("ConfirmTextureHeights", LLSD(), LLSD(), boost::bind(&LLPanelRegionTerrainInfo::callbackTextureHeights, this, _1, _2));
            mAskedTextureHeights = true;
            return false;
        }
        else if (!mConfirmedTextureHeights)
        {
            return false;
        }
    }

    std::string buffer;
    strings_t strings;
    LLUUID invoice(LLFloaterRegionInfo::getLastInvoice());

    // update the model
    LLRegionInfoModel& region_info = LLRegionInfoModel::instance();
    region_info.mWaterHeight = (F32) getChild<LLUICtrl>("water_height_spin")->getValue().asReal();
    region_info.mTerrainRaiseLimit = (F32) getChild<LLUICtrl>("terrain_raise_spin")->getValue().asReal();
    region_info.mTerrainLowerLimit = (F32) getChild<LLUICtrl>("terrain_lower_spin")->getValue().asReal();

    // and sync the region with it
    region_info.sendRegionTerrain(invoice);

    // =======================================
    // Assemble and send texturedetail message

    std::string id_str;
    LLMessageSystem* msg = gMessageSystem;

    // Send either material IDs instead of texture IDs depending on
    // material_type - they both occupy the same slot.
    const LLTerrainMaterials::Type material_type = mMaterialTypeCtrl ? material_type_from_ctrl(mMaterialTypeCtrl) : LLTerrainMaterials::Type::TEXTURE;
    for(S32 i = 0; i < LLTerrainMaterials::ASSET_COUNT; ++i)
    {
        LLTextureCtrl* asset_ctrl;
        if (material_type == LLTerrainMaterials::Type::PBR)
        {
            asset_ctrl = mMaterialDetailCtrl[i];
        }
        else
        {
            asset_ctrl = mTextureDetailCtrl[i];
        }

        if (!asset_ctrl) { continue; }

        LLUUID tmp_id(asset_ctrl->getImageAssetID());
        tmp_id.toString(id_str);
        buffer = llformat("%d %s", i, id_str.c_str());
        strings.push_back(buffer);

        // Store asset for later terrain editing
        if (material_type == LLTerrainMaterials::Type::PBR)
        {
            mLastSetMaterials[i] = tmp_id;
        }
        else
        {
            mLastSetTextures[i] = tmp_id;
        }
    }
    sendEstateOwnerMessage(msg, "texturedetail", invoice, strings);
    strings.clear();

    // ========================================
    // Assemble and send textureheights message

    for(S32 i = 0; i < CORNER_COUNT; ++i)
    {
        buffer = llformat("height_start_spin_%d", i);
        std::string buffer2 = llformat("height_range_spin_%d", i);
        std::string buffer3 = llformat("%d %f %f", i, (F32)getChild<LLUICtrl>(buffer)->getValue().asReal(), (F32)getChild<LLUICtrl>(buffer2)->getValue().asReal());
        strings.push_back(buffer3);
    }
    sendEstateOwnerMessage(msg, "textureheights", invoice, strings);
    strings.clear();

    // ========================================
    // Send texturecommit message

    sendEstateOwnerMessage(msg, "texturecommit", invoice, strings);

    // ========================================
    // POST to ModifyRegion endpoint, if enabled

    static LLCachedControl<bool> feature_pbr_terrain_transforms_enabled(gSavedSettings, "RenderTerrainPBRTransformsEnabled", false);
    if (material_type == LLTerrainMaterials::Type::PBR && feature_pbr_terrain_transforms_enabled())
    {
        LLTerrainMaterials composition;
        for (S32 i = 0; i < LLTerrainMaterials::ASSET_COUNT; ++i)
        {
            LLPointer<LLGLTFMaterial> mat_override = new LLGLTFMaterial();

            const bool transform_controls_valid = mMaterialScaleUCtrl[i] && mMaterialScaleVCtrl[i] && mMaterialRotationCtrl[i] && mMaterialOffsetUCtrl[i] && mMaterialOffsetVCtrl[i];
            if (transform_controls_valid)
            {
                // Set texture transforms for all texture infos to the same value,
                // because the PBR terrain shader doesn't currently support
                // different transforms per texture info. See also
                // LLDrawPoolTerrain::renderFullShaderPBR .
                for (U32 tt = 0; tt < LLGLTFMaterial::GLTF_TEXTURE_INFO_COUNT; ++tt)
                {
                    LLGLTFMaterial::TextureTransform& transform = mat_override->mTextureTransform[tt];
                    transform.mScale.mV[VX] = (F32)mMaterialScaleUCtrl[i]->getValue().asReal();
                    transform.mScale.mV[VY] = (F32)mMaterialScaleVCtrl[i]->getValue().asReal();
                    transform.mRotation = (F32)mMaterialRotationCtrl[i]->getValue().asReal() * DEG_TO_RAD;
                    transform.mOffset.mV[VX] = (F32)mMaterialOffsetUCtrl[i]->getValue().asReal();
                    transform.mOffset.mV[VY] = (F32)mMaterialOffsetVCtrl[i]->getValue().asReal();
                }
            }

            if (*mat_override == LLGLTFMaterial::sDefault) { mat_override = nullptr; }
            composition.setMaterialOverride(i, mat_override.get());
        }

        // queueModify leads to a few messages being sent back and forth:
        //   viewer: POST ModifyRegion
        //   simulator: RegionHandshake
        //   viewer: GET ModifyRegion
        LLViewerRegion* region = gAgent.getRegion();
        llassert(region);
        if (region)
        {
            LLPBRTerrainFeatures::queueModify(*region, composition);
        }
    }

    return true;
}

void LLPanelRegionTerrainInfo::initMaterialCtrl(LLTextureCtrl*& ctrl, const std::string& name, S32 index)
{
    ctrl = findChild<LLTextureCtrl>(name, true);
    if (!ctrl) return;

    // consume cancel events, otherwise they will trigger commit callbacks
    ctrl->setOnCancelCallback([](LLUICtrl* ctrl, const LLSD& param) {});
    ctrl->setCommitCallback(
        [this, index](LLUICtrl* ctrl, const LLSD& param)
    {
        if (!mMaterialScaleUCtrl[index]
            || !mMaterialScaleVCtrl[index]
            || !mMaterialRotationCtrl[index]
            || !mMaterialOffsetUCtrl[index]
            || !mMaterialOffsetVCtrl[index]) return;

        mMaterialScaleUCtrl[index]->setValue(1.f);
        mMaterialScaleVCtrl[index]->setValue(1.f);
        mMaterialRotationCtrl[index]->setValue(0.f);
        mMaterialOffsetUCtrl[index]->setValue(0.f);
        mMaterialOffsetVCtrl[index]->setValue(0.f);
        onChangeAnything();
    });
}

bool LLPanelRegionTerrainInfo::callbackTextureHeights(const LLSD& notification, const LLSD& response)
{
    S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
    if (option == 0) // ok
    {
        mConfirmedTextureHeights = true;
    }
    else if (option == 1) // cancel
    {
        mConfirmedTextureHeights = false;
    }
    else if (option == 2) // don't ask
    {
        gSavedSettings.setBOOL("RegionCheckTextureHeights", false);
        mConfirmedTextureHeights = true;
    }

    onBtnSet();

    mAskedTextureHeights = false;
    return false;
}

// static
void LLPanelRegionTerrainInfo::onClickDownloadRaw(void* data)
{
    LLFilePicker& picker = LLFilePicker::instance();
    if (!picker.getSaveFile(LLFilePicker::FFSAVE_RAW, "terrain.raw"))
    {
        LL_WARNS() << "No file" << LL_ENDL;
        return;
    }
    std::string filepath = picker.getFirstFile();
    gXferManager->expectFileForRequest(filepath);

    LLPanelRegionTerrainInfo* self = (LLPanelRegionTerrainInfo*)data;
    strings_t strings;
    strings.push_back("download filename");
    strings.push_back(filepath);
    LLUUID invoice(LLFloaterRegionInfo::getLastInvoice());
    self->sendEstateOwnerMessage(gMessageSystem, "terrain", invoice, strings);
}

// static
void LLPanelRegionTerrainInfo::onClickUploadRaw(void* data)
{
    LLFilePicker& picker = LLFilePicker::instance();
    if (!picker.getOpenFile(LLFilePicker::FFLOAD_RAW))
    {
        LL_WARNS() << "No file" << LL_ENDL;
        return;
    }
    std::string filepath = picker.getFirstFile();
    gXferManager->expectFileForTransfer(filepath);

    LLPanelRegionTerrainInfo* self = (LLPanelRegionTerrainInfo*)data;
    strings_t strings;
    strings.push_back("upload filename");
    strings.push_back(filepath);
    LLUUID invoice(LLFloaterRegionInfo::getLastInvoice());
    self->sendEstateOwnerMessage(gMessageSystem, "terrain", invoice, strings);

    LLNotificationsUtil::add("RawUploadStarted");
}

// static
void LLPanelRegionTerrainInfo::onClickBakeTerrain(void* data)
{
    LLNotificationsUtil::add("ConfirmBakeTerrain", LLSD(), LLSD(), boost::bind(&LLPanelRegionTerrainInfo::callbackBakeTerrain, (LLPanelRegionTerrainInfo*)data, _1, _2));
}

bool LLPanelRegionTerrainInfo::callbackBakeTerrain(const LLSD& notification, const LLSD& response)
{
    S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
    if (option != 0) return false;

    strings_t strings;
    strings.push_back("bake");
    LLUUID invoice(LLFloaterRegionInfo::getLastInvoice());
    sendEstateOwnerMessage(gMessageSystem, "terrain", invoice, strings);

    return false;
}

/////////////////////////////////////////////////////////////////////////////
// LLPanelEstateInfo
//

LLPanelEstateInfo::LLPanelEstateInfo()
:   LLPanelRegionInfo(),
    mEstateID(0)    // invalid
{
    LLEstateInfoModel& estate_info = LLEstateInfoModel::instance();
    estate_info.setCommitCallback(boost::bind(&LLPanelEstateInfo::refreshFromEstate, this));
    estate_info.setUpdateCallback(boost::bind(&LLPanelEstateInfo::refreshFromEstate, this));
}

// static
void LLPanelEstateInfo::initDispatch(LLDispatcher& dispatch)
{
    std::string name;

    name.assign("estateupdateinfo");
    static LLDispatchEstateUpdateInfo estate_update_info;
    dispatch.addHandler(name, &estate_update_info);

    name.assign("setaccess");
    static LLDispatchSetEstateAccess set_access;
    dispatch.addHandler(name, &set_access);

    name.assign("setexperience");
    static LLDispatchSetEstateExperience set_experience;
    dispatch.addHandler(name, &set_experience);

    estate_dispatch_initialized = true;
}

//---------------------------------------------------------------------------
// Kick from estate methods
//---------------------------------------------------------------------------

void LLPanelEstateInfo::onClickKickUser()
{
    // this depends on the grandparent view being a floater
    // in order to set up floater dependency
    LLView * button = findChild<LLButton>("kick_user_from_estate_btn");
    LLFloater* parent_floater = gFloaterView->getParentFloater(this);
    LLFloater* child_floater = LLFloaterAvatarPicker::show(boost::bind(&LLPanelEstateInfo::onKickUserCommit, this, _1),
                                                                        false, true, false, parent_floater->getName(), button);
    if (child_floater)
    {
        parent_floater->addDependentFloater(child_floater);
    }
}

void LLPanelEstateInfo::onKickUserCommit(const uuid_vec_t& ids)
{
    if (ids.empty()) return;

    //Bring up a confirmation dialog
    LLSD args;
    args["EVIL_USER"] = LLSLURL("agent", ids[0], "completename").getSLURLString();
    LLSD payload;
    payload["agent_id"] = ids[0];
    LLNotificationsUtil::add("EstateKickUser", args, payload, boost::bind(&LLPanelEstateInfo::kickUserConfirm, this, _1, _2));

}

bool LLPanelEstateInfo::kickUserConfirm(const LLSD& notification, const LLSD& response)
{
    S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
    switch(option)
    {
    case 0:
        {
            //Kick User
            strings_t strings;
            strings.push_back(notification["payload"]["agent_id"].asString());

            sendEstateOwnerMessage(gMessageSystem, "kickestate", LLFloaterRegionInfo::getLastInvoice(), strings);
            break;
        }
    default:
        break;
    }
    return false;
}

//---------------------------------------------------------------------------
// Core Add/Remove estate access methods
// TODO: INTERNATIONAL: don't build message text here;
// instead, create multiple translatable messages and choose
// one based on the status.
//---------------------------------------------------------------------------
std::string all_estates_text()
{
    LLPanelEstateInfo* panel = LLFloaterRegionInfo::getPanelEstate();
    if (!panel) return "(" + LLTrans::getString("RegionInfoError") + ")";

    LLStringUtil::format_map_t args;
    std::string owner = panel->getOwnerName();

    LLViewerRegion* region = gAgent.getRegion();
    if (gAgent.isGodlike())
    {
        args["[OWNER]"] = owner.c_str();
        return LLTrans::getString("RegionInfoAllEstatesOwnedBy", args);
    }
    else if (region && region->getOwner() == gAgent.getID())
    {
        return LLTrans::getString("RegionInfoAllEstatesYouOwn");
    }
    else if (region && region->isEstateManager())
    {
        args["[OWNER]"] = owner.c_str();
        return LLTrans::getString("RegionInfoAllEstatesYouManage", args);
    }
    else
    {
        return "(" + LLTrans::getString("RegionInfoError") + ")";
    }
}

// static
bool LLPanelEstateInfo::isLindenEstate()
{
    U32 estate_id = LLEstateInfoModel::instance().getID();
    return (estate_id <= ESTATE_LAST_LINDEN);
}

struct LLEstateAccessChangeInfo
{
    LLEstateAccessChangeInfo(const LLSD& sd)
    {
        mDialogName = sd["dialog_name"].asString();
        mOperationFlag = (U32)sd["operation"].asInteger();
        LLSD::array_const_iterator end_it = sd["allowed_ids"].endArray();
        for (LLSD::array_const_iterator id_it = sd["allowed_ids"].beginArray();
            id_it != end_it;
            ++id_it)
        {
            mAgentOrGroupIDs.push_back(id_it->asUUID());
        }
    }

    const LLSD asLLSD() const
    {
        LLSD sd;
        sd["name"] = mDialogName;
        sd["operation"] = (S32)mOperationFlag;
        for (U32 i = 0; i < mAgentOrGroupIDs.size(); ++i)
        {
            sd["allowed_ids"].append(mAgentOrGroupIDs[i]);
            if (mAgentNames.size() > i)
            {
                sd["allowed_names"].append(mAgentNames[i].asLLSD());
            }
        }
        return sd;
    }

    U32 mOperationFlag; // ESTATE_ACCESS_BANNED_AGENT_ADD, _REMOVE, etc.
    std::string mDialogName;
    uuid_vec_t mAgentOrGroupIDs; // List of agent IDs to apply to this change
    std::vector<LLAvatarName> mAgentNames; // Optional list of the agent names for notifications
};

// static
void LLPanelEstateInfo::updateEstateOwnerName(const std::string& name)
{
    LLPanelEstateInfo* panelp = LLFloaterRegionInfo::getPanelEstate();
    if (panelp)
    {
        panelp->setOwnerName(name);
    }
}

// static
void LLPanelEstateInfo::updateEstateName(const std::string& name)
{
    LLPanelEstateInfo* panelp = LLFloaterRegionInfo::getPanelEstate();
    if (panelp)
    {
        panelp->getChildRef<LLTextBox>("estate_name").setText(name);
    }
}

void LLPanelEstateInfo::updateControls(LLViewerRegion* region)
{
    bool god = gAgent.isGodlike();
    bool owner = (region && (region->getOwner() == gAgent.getID()));
    bool manager = (region && region->isEstateManager());
    setCtrlsEnabled(god || owner || manager);

    getChildView("apply_btn")->setEnabled(false);
    getChildView("estate_owner")->setEnabled(true);
    getChildView("message_estate_btn")->setEnabled(god || owner || manager);
    getChildView("kick_user_from_estate_btn")->setEnabled(god || owner || manager);

    refresh();
}

bool LLPanelEstateInfo::refreshFromRegion(LLViewerRegion* region, ERefreshFromRegionPhase phase)
{
    updateControls(region);

    // let the parent class handle the general data collection.
    bool rv = LLPanelRegionInfo::refreshFromRegion(region, phase);

    if (phase != ERefreshFromRegionPhase::BeforeRequestRegionInfo)
    {
        // We want estate info. To make sure it works across region
        // boundaries and multiple packets, we add a serial number to the
        // integers and track against that on update.
        strings_t strings;
        //integers_t integers;
        //LLFloaterRegionInfo::incrementSerial();
        LLFloaterRegionInfo::nextInvoice();
        LLUUID invoice(LLFloaterRegionInfo::getLastInvoice());
        //integers.push_back(LLFloaterRegionInfo::());::getPanelEstate();

        sendEstateOwnerMessage(gMessageSystem, "getinfo", invoice, strings);
    }

    refresh();

    return rv;
}

void LLPanelEstateInfo::updateChild(LLUICtrl* child_ctrl)
{
    // Ensure appropriate state of the management ui.
    updateControls(gAgent.getRegion());
}

bool LLPanelEstateInfo::estateUpdate(LLMessageSystem* msg)
{
    LL_INFOS() << "LLPanelEstateInfo::estateUpdate()" << LL_ENDL;
    return false;
}


bool LLPanelEstateInfo::postBuild()
{
    // set up the callbacks for the generic controls
    initCtrl("externally_visible_radio");
    initCtrl("allow_direct_teleport");
    initCtrl("limit_payment");
    initCtrl("limit_age_verified");
    initCtrl("limit_bots");
    initCtrl("voice_chat_check");
    initCtrl("parcel_access_override");

    childSetAction("message_estate_btn", boost::bind(&LLPanelEstateInfo::onClickMessageEstate, this));
    childSetAction("kick_user_from_estate_btn", boost::bind(&LLPanelEstateInfo::onClickKickUser, this));

    getChild<LLUICtrl>("parcel_access_override")->setCommitCallback(boost::bind(&LLPanelEstateInfo::onChangeAccessOverride, this));

    getChild<LLUICtrl>("externally_visible_radio")->setFocus(true);

    getChild<LLTextBox>("estate_owner")->setIsFriendCallback(LLAvatarActions::isFriend);

    return LLPanelRegionInfo::postBuild();
}

void LLPanelEstateInfo::refresh()
{
    // Disable access restriction controls if they make no sense.
    bool public_access = ("estate_public_access" == getChild<LLUICtrl>("externally_visible_radio")->getValue().asString());

    getChildView("limit_payment")->setEnabled(public_access);
    getChildView("limit_age_verified")->setEnabled(public_access);
    getChildView("limit_bots")->setEnabled(public_access);

    // if this is set to false, then the limit fields are meaningless and should be turned off
    if (!public_access)
    {
        getChild<LLUICtrl>("limit_payment")->setValue(false);
        getChild<LLUICtrl>("limit_age_verified")->setValue(false);
        getChild<LLUICtrl>("limit_bots")->setValue(false);
    }
}

void LLPanelEstateInfo::refreshFromEstate()
{
    const LLEstateInfoModel& estate_info = LLEstateInfoModel::instance();

    getChild<LLUICtrl>("estate_name")->setValue(estate_info.getName());
    setOwnerName(LLSLURL("agent", estate_info.getOwnerID(), "inspect").getSLURLString());

    getChild<LLUICtrl>("externally_visible_radio")->setValue(estate_info.getIsExternallyVisible() ? "estate_public_access" : "estate_restricted_access");
    getChild<LLUICtrl>("voice_chat_check")->setValue(estate_info.getAllowVoiceChat());
    getChild<LLUICtrl>("allow_direct_teleport")->setValue(estate_info.getAllowDirectTeleport());
    getChild<LLUICtrl>("limit_payment")->setValue(estate_info.getDenyAnonymous());
    getChild<LLUICtrl>("limit_age_verified")->setValue(estate_info.getDenyAgeUnverified());
    getChild<LLUICtrl>("parcel_access_override")->setValue(estate_info.getAllowAccessOverride());
    getChild<LLUICtrl>("limit_bots")->setValue(estate_info.getDenyScriptedAgents());

    // Ensure appriopriate state of the management UI
    updateControls(gAgent.getRegion());
    refresh();
}

bool LLPanelEstateInfo::sendUpdate()
{
    LL_INFOS() << "LLPanelEsateInfo::sendUpdate()" << LL_ENDL;

    LLNotification::Params params("ChangeLindenEstate");
    params.functor.function(boost::bind(&LLPanelEstateInfo::callbackChangeLindenEstate, this, _1, _2));

    if (isLindenEstate())
    {
        // trying to change reserved estate, warn
        LLNotifications::instance().add(params);
    }
    else
    {
        // for normal estates, just make the change
        LLNotifications::instance().forceResponse(params, 0);
    }
    return true;
}

bool LLPanelEstateInfo::callbackChangeLindenEstate(const LLSD& notification, const LLSD& response)
{
    S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
    switch(option)
    {
    case 0:
        {
            LLEstateInfoModel& estate_info = LLEstateInfoModel::instance();

            // update model
            estate_info.setUseFixedSun(false); // we don't support fixed sun estates anymore
            estate_info.setIsExternallyVisible("estate_public_access" == getChild<LLUICtrl>("externally_visible_radio")->getValue().asString());
            estate_info.setAllowDirectTeleport(getChild<LLUICtrl>("allow_direct_teleport")->getValue().asBoolean());
            estate_info.setDenyAnonymous(getChild<LLUICtrl>("limit_payment")->getValue().asBoolean());
            estate_info.setDenyAgeUnverified(getChild<LLUICtrl>("limit_age_verified")->getValue().asBoolean());
            estate_info.setAllowVoiceChat(getChild<LLUICtrl>("voice_chat_check")->getValue().asBoolean());
            estate_info.setAllowAccessOverride(getChild<LLUICtrl>("parcel_access_override")->getValue().asBoolean());
            estate_info.setDenyScriptedAgents(getChild<LLUICtrl>("limit_bots")->getValue().asBoolean());
            // JIGGLYPUFF
            //estate_info.setAllowAccessOverride(getChild<LLUICtrl>("")->getValue().asBoolean());
            // send the update to sim
            estate_info.sendEstateInfo();
        }

        // we don't want to do this because we'll get it automatically from the sim
        // after the spaceserver processes it
//      else
//      {
//          // caps method does not automatically send this info
//          LLFloaterRegionInfo::requestRegionInfo();
//      }
        break;
    case 1:
    default:
        // do nothing
        break;
    }
    return false;
}


/*
// Request = "getowner"
// SParam[0] = "" (empty string)
// IParam[0] = serial
void LLPanelEstateInfo::getEstateOwner()
{
    // TODO -- disable the panel
    // and call this function whenever we cross a region boundary
    // re-enable when owner matches, and get new estate info
    LLMessageSystem* msg = gMessageSystem;
    msg->newMessageFast(_PREHASH_EstateOwnerRequest);
    msg->nextBlockFast(_PREHASH_AgentData);
    msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());

    msg->nextBlockFast(_PREHASH_RequestData);
    msg->addStringFast(_PREHASH_Request, "getowner");

    // we send an empty string so that the variable block is not empty
    msg->nextBlockFast(_PREHASH_StringData);
    msg->addStringFast(_PREHASH_SParam, "");

    msg->nextBlockFast(_PREHASH_IntegerData);
    msg->addS32Fast(_PREHASH_IParam, LLFloaterRegionInfo::getSerial());

    gAgent.sendMessage();
}
*/

const std::string LLPanelEstateInfo::getOwnerName() const
{
    return getChild<LLUICtrl>("estate_owner")->getValue().asString();
}

void LLPanelEstateInfo::setOwnerName(const std::string& name)
{
    getChild<LLUICtrl>("estate_owner")->setValue(LLSD(name));
}

// static
void LLPanelEstateInfo::onClickMessageEstate(void* userdata)
{
    LL_INFOS() << "LLPanelEstateInfo::onClickMessageEstate" << LL_ENDL;
    LLNotificationsUtil::add("MessageEstate", LLSD(), LLSD(), boost::bind(&LLPanelEstateInfo::onMessageCommit, (LLPanelEstateInfo*)userdata, _1, _2));
}

bool LLPanelEstateInfo::onMessageCommit(const LLSD& notification, const LLSD& response)
{
    S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
    std::string text = response["message"].asString();
    if(option != 0) return false;
    if(text.empty()) return false;
    LL_INFOS() << "Message to everyone: " << text << LL_ENDL;
    strings_t strings;
    //integers_t integers;
    std::string name;
    LLAgentUI::buildFullname(name);
    strings.push_back(strings_t::value_type(name));
    strings.push_back(strings_t::value_type(text));
    LLUUID invoice(LLFloaterRegionInfo::getLastInvoice());
    sendEstateOwnerMessage(gMessageSystem, "instantmessage", invoice, strings);
    return false;
}

void LLPanelEstateInfo::onChangeAccessOverride()
{
    if (!getChild<LLUICtrl>("parcel_access_override")->getValue().asBoolean())
    {
        LLNotificationsUtil::add("EstateParcelAccessOverride");
    }
}

LLPanelEstateCovenant::LLPanelEstateCovenant()
    :
    mCovenantID(LLUUID::null),
    mAssetStatus(ASSET_ERROR)
{
}

// virtual
bool LLPanelEstateCovenant::refreshFromRegion(LLViewerRegion* region, ERefreshFromRegionPhase phase)
{
    LLTextBox* region_name = getChild<LLTextBox>("region_name_text");
    if (region_name)
    {
        region_name->setText(region->getName());
    }

    LLTextBox* resellable_clause = getChild<LLTextBox>("resellable_clause");
    if (resellable_clause)
    {
        if (region->getRegionFlag(REGION_FLAGS_BLOCK_LAND_RESELL))
        {
            resellable_clause->setText(getString("can_not_resell"));
        }
        else
        {
            resellable_clause->setText(getString("can_resell"));
        }
    }

    LLTextBox* changeable_clause = getChild<LLTextBox>("changeable_clause");
    if (changeable_clause)
    {
        if (region->getRegionFlag(REGION_FLAGS_ALLOW_PARCEL_CHANGES))
        {
            changeable_clause->setText(getString("can_change"));
        }
        else
        {
            changeable_clause->setText(getString("can_not_change"));
        }
    }

    LLTextBox* region_maturity = getChild<LLTextBox>("region_maturity_text");
    if (region_maturity)
    {
        region_maturity->setText(region->getSimAccessString());
    }

    LLTextBox* region_landtype = getChild<LLTextBox>("region_landtype_text");
    region_landtype->setText(region->getLocalizedSimProductName());

    getChild<LLButton>("reset_covenant")->setEnabled(gAgent.isGodlike() || (region && region->canManageEstate()));

    // let the parent class handle the general data collection.
    bool rv = LLPanelRegionInfo::refreshFromRegion(region, phase);

    if (phase != ERefreshFromRegionPhase::AfterRequestRegionInfo)
    {
        gMessageSystem->newMessage("EstateCovenantRequest");
        gMessageSystem->nextBlockFast(_PREHASH_AgentData);
        gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
        gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
        gMessageSystem->sendReliable(region->getHost());
    }

    return rv;
}

// virtual
bool LLPanelEstateCovenant::estateUpdate(LLMessageSystem* msg)
{
    LL_INFOS() << "LLPanelEstateCovenant::estateUpdate()" << LL_ENDL;
    return true;
}

// virtual
bool LLPanelEstateCovenant::postBuild()
{
    mEstateNameText = getChild<LLTextBox>("estate_name_text");
    mEstateOwnerText = getChild<LLTextBox>("estate_owner_text");
    mEstateOwnerText->setIsFriendCallback(LLAvatarActions::isFriend);
    mLastModifiedText = getChild<LLTextBox>("covenant_timestamp_text");
    mEditor = getChild<LLViewerTextEditor>("covenant_editor");
    LLButton* reset_button = getChild<LLButton>("reset_covenant");
    reset_button->setEnabled(gAgent.canManageEstate());
    reset_button->setClickedCallback(LLPanelEstateCovenant::resetCovenantID, NULL);

    return LLPanelRegionInfo::postBuild();
}

// virtual
void LLPanelEstateCovenant::updateChild(LLUICtrl* child_ctrl)
{
}

// virtual
bool LLPanelEstateCovenant::handleDragAndDrop(S32 x, S32 y, MASK mask, bool drop,
                                  EDragAndDropType cargo_type,
                                  void* cargo_data,
                                  EAcceptance* accept,
                                  std::string& tooltip_msg)
{
    LLInventoryItem* item = (LLInventoryItem*)cargo_data;

    if (!gAgent.canManageEstate())
    {
        *accept = ACCEPT_NO;
        return true;
    }

    switch(cargo_type)
    {
    case DAD_NOTECARD:
        *accept = ACCEPT_YES_COPY_SINGLE;
        if (item && drop)
        {
            LLSD payload;
            payload["item_id"] = item->getUUID();
            LLNotificationsUtil::add("EstateChangeCovenant", LLSD(), payload,
                                    LLPanelEstateCovenant::confirmChangeCovenantCallback);
        }
        break;
    default:
        *accept = ACCEPT_NO;
        break;
    }

    return true;
}

// static
bool LLPanelEstateCovenant::confirmChangeCovenantCallback(const LLSD& notification, const LLSD& response)
{
    S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
    LLInventoryItem* item = gInventory.getItem(notification["payload"]["item_id"].asUUID());
    LLPanelEstateCovenant* self = LLFloaterRegionInfo::getPanelCovenant();

    if (!item || !self) return false;

    switch(option)
    {
    case 0:
        self->loadInvItem(item);
        break;
    default:
        break;
    }
    return false;
}

// static
void LLPanelEstateCovenant::resetCovenantID(void* userdata)
{
    LLNotificationsUtil::add("EstateChangeCovenant", LLSD(), LLSD(), confirmResetCovenantCallback);
}

// static
bool LLPanelEstateCovenant::confirmResetCovenantCallback(const LLSD& notification, const LLSD& response)
{
    LLPanelEstateCovenant* self = LLFloaterRegionInfo::getPanelCovenant();
    if (!self) return false;

    S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
    switch(option)
    {
    case 0:
        self->loadInvItem(NULL);
        break;
    default:
        break;
    }
    return false;
}

void LLPanelEstateCovenant::loadInvItem(LLInventoryItem *itemp)
{
    const bool high_priority = true;
    if (itemp)
    {
        gAssetStorage->getInvItemAsset(gAgent.getRegionHost(),
                                    gAgent.getID(),
                                    gAgent.getSessionID(),
                                    itemp->getPermissions().getOwner(),
                                    LLUUID::null,
                                    itemp->getUUID(),
                                    itemp->getAssetUUID(),
                                    itemp->getType(),
                                    onLoadComplete,
                                    (void*)this,
                                    high_priority);
        mAssetStatus = ASSET_LOADING;
    }
    else
    {
        mAssetStatus = ASSET_LOADED;
        setCovenantTextEditor(LLTrans::getString("RegionNoCovenant"));
        sendChangeCovenantID(LLUUID::null);
    }
}

// static
void LLPanelEstateCovenant::onLoadComplete(const LLUUID& asset_uuid,
                                           LLAssetType::EType type,
                                           void* user_data, S32 status, LLExtStat ext_status)
{
    LL_INFOS() << "LLPanelEstateCovenant::onLoadComplete()" << LL_ENDL;
    LLPanelEstateCovenant* panelp = (LLPanelEstateCovenant*)user_data;
    if( panelp )
    {
        if(0 == status)
        {
            LLFileSystem file(asset_uuid, type, LLFileSystem::READ);

            S32 file_length = file.getSize();

            std::vector<char> buffer(file_length+1);
            file.read((U8*)&buffer[0], file_length);
            // put a EOS at the end
            buffer[file_length] = 0;

            if( (file_length > 19) && !strncmp( &buffer[0], "Linden text version", 19 ) )
            {
                if( !panelp->mEditor->importBuffer( &buffer[0], file_length+1 ) )
                {
                    LL_WARNS() << "Problem importing estate covenant." << LL_ENDL;
                    LLNotificationsUtil::add("ProblemImportingEstateCovenant");
                }
                else
                {
                    panelp->sendChangeCovenantID(asset_uuid);
                }
            }
            else
            {
                // Version 0 (just text, doesn't include version number)
                panelp->sendChangeCovenantID(asset_uuid);
            }
        }
        else
        {
            if( LL_ERR_ASSET_REQUEST_NOT_IN_DATABASE == status ||
                LL_ERR_FILE_EMPTY == status)
            {
                LLNotificationsUtil::add("MissingNotecardAssetID");
            }
            else if (LL_ERR_INSUFFICIENT_PERMISSIONS == status)
            {
                LLNotificationsUtil::add("NotAllowedToViewNotecard");
            }
            else
            {
                LLNotificationsUtil::add("UnableToLoadNotecardAsset");
            }

            LL_WARNS() << "Problem loading notecard: " << status << LL_ENDL;
        }
        panelp->mAssetStatus = ASSET_LOADED;
        panelp->setCovenantID(asset_uuid);
    }
}

// key = "estatechangecovenantid"
// strings[0] = str(estate_id) (added by simulator before relay - not here)
// strings[1] = str(covenant_id)
void LLPanelEstateCovenant::sendChangeCovenantID(const LLUUID &asset_id)
{
    if (asset_id != getCovenantID())
    {
        setCovenantID(asset_id);

        LLMessageSystem* msg = gMessageSystem;
        msg->newMessage("EstateOwnerMessage");
        msg->nextBlockFast(_PREHASH_AgentData);
        msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
        msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
        msg->addUUIDFast(_PREHASH_TransactionID, LLUUID::null); //not used

        msg->nextBlock("MethodData");
        msg->addString("Method", "estatechangecovenantid");
        msg->addUUID("Invoice", LLFloaterRegionInfo::getLastInvoice());

        msg->nextBlock("ParamList");
        msg->addString("Parameter", getCovenantID().asString());
        gAgent.sendReliableMessage();
    }
}

// virtual
bool LLPanelEstateCovenant::sendUpdate()
{
    return true;
}

std::string LLPanelEstateCovenant::getEstateName() const
{
    return mEstateNameText->getText();
}

void LLPanelEstateCovenant::setEstateName(const std::string& name)
{
    mEstateNameText->setText(name);
}

// static
void LLPanelEstateCovenant::updateCovenant(const LLTextBase* source, const LLUUID& asset_id)
{
    if (LLPanelEstateCovenant* panelp = LLFloaterRegionInfo::getPanelCovenant())
    {
        panelp->mEditor->copyContents(source);
        panelp->setCovenantID(asset_id);
    }
}

// static
void LLPanelEstateCovenant::updateCovenantText(const std::string& string, const LLUUID& asset_id)
{
    LLPanelEstateCovenant* panelp = LLFloaterRegionInfo::getPanelCovenant();
    if( panelp )
    {
        panelp->mEditor->setText(string);
        panelp->setCovenantID(asset_id);
    }
}

// static
void LLPanelEstateCovenant::updateEstateName(const std::string& name)
{
    LLPanelEstateCovenant* panelp = LLFloaterRegionInfo::getPanelCovenant();
    if( panelp )
    {
        panelp->mEstateNameText->setText(name);
    }
}

// static
void LLPanelEstateCovenant::updateLastModified(const std::string& text)
{
    LLPanelEstateCovenant* panelp = LLFloaterRegionInfo::getPanelCovenant();
    if( panelp )
    {
        panelp->mLastModifiedText->setText(text);
    }
}

// static
void LLPanelEstateCovenant::updateEstateOwnerName(const std::string& name)
{
    LLPanelEstateCovenant* panelp = LLFloaterRegionInfo::getPanelCovenant();
    if( panelp )
    {
        panelp->mEstateOwnerText->setText(name);
    }
}

std::string LLPanelEstateCovenant::getOwnerName() const
{
    return mEstateOwnerText->getText();
}

void LLPanelEstateCovenant::setOwnerName(const std::string& name)
{
    mEstateOwnerText->setText(name);
}

void LLPanelEstateCovenant::setCovenantTextEditor(const std::string& text)
{
    mEditor->setText(text);
}

// key = "estateupdateinfo"
// strings[0] = estate name
// strings[1] = str(owner_id)
// strings[2] = str(estate_id)
// strings[3] = str(estate_flags)
// strings[4] = str((S32)(sun_hour * 1024))
// strings[5] = str(parent_estate_id)
// strings[6] = str(covenant_id)
// strings[7] = str(covenant_timestamp)
// strings[8] = str(send_to_agent_only)
// strings[9] = str(abuse_email_addr)
bool LLDispatchEstateUpdateInfo::operator()(
        const LLDispatcher* dispatcher,
        const std::string& key,
        const LLUUID& invoice,
        const sparam_t& strings)
{
    LL_DEBUGS() << "Received estate update" << LL_ENDL;

    // Update estate info model.
    // This will call LLPanelEstateInfo::refreshFromEstate().
    // *TODO: Move estate message handling stuff to llestateinfomodel.cpp.
    LLEstateInfoModel::instance().update(strings);

    return true;
}

bool LLDispatchSetEstateAccess::operator()(
    const LLDispatcher* dispatcher,
    const std::string& key,
    const LLUUID& invoice,
    const sparam_t& strings)
{
    LLPanelEstateAccess* panel = LLFloaterRegionInfo::getPanelAccess();
    if (panel && panel->getPendingUpdate())
    {
        panel->setPendingUpdate(false);
        panel->updateLists();
    }
    return true;
}

// static
LLSD LLDispatchSetEstateExperience::getIDs( sparam_t::const_iterator it, sparam_t::const_iterator end, S32 count )
{
    LLSD idList = LLSD::emptyArray();
    LLUUID id;
    while (count-- > 0 && it < end)
    {
        memcpy(id.mData, (*(it++)).data(), UUID_BYTES);
        idList.append(id);
    }
    return idList;
}

// key = "setexperience"
// strings[0] = str(estate_id)
// strings[1] = str(send_to_agent_only)
// strings[2] = str(num blocked)
// strings[3] = str(num trusted)
// strings[4] = str(num allowed)
// strings[5] = bin(uuid) ...
// ...
bool LLDispatchSetEstateExperience::operator()(
    const LLDispatcher* dispatcher,
    const std::string& key,
    const LLUUID& invoice,
    const sparam_t& strings)
{
    LLPanelRegionExperiences* panel = LLFloaterRegionInfo::getPanelExperiences();
    if (!panel)
        return true;

    const sparam_t::size_type MIN_SIZE = 5;
    if (strings.size() < MIN_SIZE)
        return true;

    // Skip 2 parameters
    sparam_t::const_iterator it = strings.begin();
    ++it; // U32 estate_id = strtol((*it).c_str(), NULL, 10);
    ++it; // U32 send_to_agent_only = strtoul((*(++it)).c_str(), NULL, 10);

    // Read 3 parameters
    LLUUID id;
    S32 num_blocked = strtol((*(it++)).c_str(), NULL, 10);
    S32 num_trusted = strtol((*(it++)).c_str(), NULL, 10);
    S32 num_allowed = strtol((*(it++)).c_str(), NULL, 10);

    LLSD ids = LLSD::emptyMap()
        .with("blocked", getIDs(it, strings.end(), num_blocked))
        .with("trusted", getIDs(it + num_blocked, strings.end(), num_trusted))
        .with("allowed", getIDs(it + num_blocked + num_trusted, strings.end(), num_allowed));

    panel->processResponse(ids);

    return true;
}

bool LLPanelRegionExperiences::postBuild()
{
    mAllowed = setupList("panel_allowed", ESTATE_EXPERIENCE_ALLOWED_ADD, ESTATE_EXPERIENCE_ALLOWED_REMOVE);
    mTrusted = setupList("panel_trusted", ESTATE_EXPERIENCE_TRUSTED_ADD, ESTATE_EXPERIENCE_TRUSTED_REMOVE);
    mBlocked = setupList("panel_blocked", ESTATE_EXPERIENCE_BLOCKED_ADD, ESTATE_EXPERIENCE_BLOCKED_REMOVE);

    getChild<LLLayoutPanel>("trusted_layout_panel")->setVisible(true);
    getChild<LLTextBox>("experiences_help_text")->setText(getString("estate_caption"));
    getChild<LLTextBox>("trusted_text_help")->setText(getString("trusted_estate_text"));
    getChild<LLTextBox>("allowed_text_help")->setText(getString("allowed_estate_text"));
    getChild<LLTextBox>("blocked_text_help")->setText(getString("blocked_estate_text"));

    return LLPanelRegionInfo::postBuild();
}

LLPanelExperienceListEditor* LLPanelRegionExperiences::setupList( const char* control_name, U32 add_id, U32 remove_id )
{
    LLPanelExperienceListEditor* child = findChild<LLPanelExperienceListEditor>(control_name);
    if(child)
    {
        child->getChild<LLTextBox>("text_name")->setText(child->getString(control_name));
        child->setMaxExperienceIDs(ESTATE_MAX_EXPERIENCE_IDS);
        child->setAddedCallback(  boost::bind(&LLPanelRegionExperiences::itemChanged, this, add_id, _1));
        child->setRemovedCallback(boost::bind(&LLPanelRegionExperiences::itemChanged, this, remove_id, _1));
    }

    return child;
}


void LLPanelRegionExperiences::processResponse( const LLSD& content )
{
    if(content.has("default"))
    {
        mDefaultExperience = content["default"].asUUID();
    }

    mAllowed->setExperienceIds(content["allowed"]);
    mBlocked->setExperienceIds(content["blocked"]);

    LLSD trusted = content["trusted"];
    if(mDefaultExperience.notNull())
    {
        mTrusted->setStickyFunction(boost::bind(LLPanelExperiencePicker::FilterMatching, _1, mDefaultExperience));
        trusted.append(mDefaultExperience);
    }

    mTrusted->setExperienceIds(trusted);

    mAllowed->refreshExperienceCounter();
    mBlocked->refreshExperienceCounter();
    mTrusted->refreshExperienceCounter();

}

// Used for both access add and remove operations, depending on the flag
// passed in (ESTATE_EXPERIENCE_ALLOWED_ADD, ESTATE_EXPERIENCE_ALLOWED_REMOVE, etc.)
// static
bool LLPanelRegionExperiences::experienceCoreConfirm(const LLSD& notification, const LLSD& response)
{
    S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
    const U32 originalFlags = (U32)notification["payload"]["operation"].asInteger();

    LLViewerRegion* region = gAgent.getRegion();

    LLSD::array_const_iterator end_it = notification["payload"]["allowed_ids"].endArray();

    for (LLSD::array_const_iterator iter = notification["payload"]["allowed_ids"].beginArray();
        iter != end_it;
         iter++)
    {
        U32 flags = originalFlags;
        if (iter + 1 != end_it)
            flags |= ESTATE_ACCESS_NO_REPLY;

        const LLUUID id = iter->asUUID();
        switch(option)
        {
            case 0:
                // This estate
                sendEstateExperienceDelta(flags, id);
                break;
            case 1:
            {
                // All estates, either than I own or manage for this owner.
                // This will be verified on simulator. JC
                if (!region) break;
                if (region->getOwner() == gAgent.getID()
                    || gAgent.isGodlike())
                {
                    flags |= ESTATE_ACCESS_APPLY_TO_ALL_ESTATES;
                    sendEstateExperienceDelta(flags, id);
                }
                else if (region->isEstateManager())
                {
                    flags |= ESTATE_ACCESS_APPLY_TO_MANAGED_ESTATES;
                    sendEstateExperienceDelta(flags, id);
                }
                break;
            }
            case 2:
            default:
                break;
        }
    }
    return false;
}


// Send the actual "estateexperiencedelta" message
void LLPanelRegionExperiences::sendEstateExperienceDelta(U32 flags, const LLUUID& experience_id)
{
    strings_t str(3, std::string());
    gAgent.getID().toString(str[0]);
    str[1] = llformat("%u", flags);
    experience_id.toString(str[2]);

    LLPanelRegionExperiences* panel = LLFloaterRegionInfo::getPanelExperiences();
    if (panel)
    {
        panel->sendEstateOwnerMessage(gMessageSystem, "estateexperiencedelta", LLFloaterRegionInfo::getLastInvoice(), str);
    }
}


void LLPanelRegionExperiences::infoCallback(LLHandle<LLPanelRegionExperiences> handle, const LLSD& content)
{
    if(handle.isDead())
        return;

    LLPanelRegionExperiences* floater = handle.get();
    if (floater)
    {
        floater->processResponse(content);
    }
}

/*static*/
std::string LLPanelRegionExperiences::regionCapabilityQuery(LLViewerRegion* region, const std::string &cap)
{
    // region->getHandle()  How to get a region * from a handle?

    return region->getCapability(cap);
}

bool LLPanelRegionExperiences::refreshFromRegion(LLViewerRegion* region, ERefreshFromRegionPhase phase)
{
    bool allow_modify = gAgent.isGodlike() || (region && region->canManageEstate());

    mAllowed->loading();
    mAllowed->setReadonly(!allow_modify);
    // remove grid-wide experiences
    mAllowed->addFilter(boost::bind(LLPanelExperiencePicker::FilterWithProperty, _1, LLExperienceCache::PROPERTY_GRID));
    // remove default experience
    mAllowed->addFilter(boost::bind(LLPanelExperiencePicker::FilterMatching, _1, mDefaultExperience));

    mBlocked->loading();
    mBlocked->setReadonly(!allow_modify);
    // only grid-wide experiences
    mBlocked->addFilter(boost::bind(LLPanelExperiencePicker::FilterWithoutProperty, _1, LLExperienceCache::PROPERTY_GRID));
    // but not privileged ones
    mBlocked->addFilter(boost::bind(LLPanelExperiencePicker::FilterWithProperty, _1, LLExperienceCache::PROPERTY_PRIVILEGED));
    // remove default experience
    mBlocked->addFilter(boost::bind(LLPanelExperiencePicker::FilterMatching, _1, mDefaultExperience));

    mTrusted->loading();
    mTrusted->setReadonly(!allow_modify);

    if (phase != ERefreshFromRegionPhase::AfterRequestRegionInfo)
    {
        LLExperienceCache::instance().getRegionExperiences(boost::bind(&LLPanelRegionExperiences::regionCapabilityQuery, region, _1),
            boost::bind(&LLPanelRegionExperiences::infoCallback, getDerivedHandle<LLPanelRegionExperiences>(), _1));
    }

    return LLPanelRegionInfo::refreshFromRegion(region, phase);
}

LLSD LLPanelRegionExperiences::addIds(LLPanelExperienceListEditor* panel)
{
    LLSD ids;
    const uuid_list_t& id_list = panel->getExperienceIds();
    for(uuid_list_t::const_iterator it = id_list.begin(); it != id_list.end(); ++it)
    {
        ids.append(*it);
    }
    return ids;
}


bool LLPanelRegionExperiences::sendUpdate()
{
    LLViewerRegion* region = gAgent.getRegion();

    LLSD content;

    content["allowed"]=addIds(mAllowed);
    content["blocked"]=addIds(mBlocked);
    content["trusted"]=addIds(mTrusted);

    LLExperienceCache::instance().setRegionExperiences(boost::bind(&LLPanelRegionExperiences::regionCapabilityQuery, region, _1),
        content, boost::bind(&LLPanelRegionExperiences::infoCallback, getDerivedHandle<LLPanelRegionExperiences>(), _1));

    return true;
}

void LLPanelRegionExperiences::itemChanged( U32 event_type, const LLUUID& id )
{
    std::string dialog_name;
    switch (event_type)
    {
        case ESTATE_EXPERIENCE_ALLOWED_ADD:
            dialog_name = "EstateAllowedExperienceAdd";
            break;

        case ESTATE_EXPERIENCE_ALLOWED_REMOVE:
            dialog_name = "EstateAllowedExperienceRemove";
            break;

        case ESTATE_EXPERIENCE_TRUSTED_ADD:
            dialog_name = "EstateTrustedExperienceAdd";
            break;

        case ESTATE_EXPERIENCE_TRUSTED_REMOVE:
            dialog_name = "EstateTrustedExperienceRemove";
            break;

        case ESTATE_EXPERIENCE_BLOCKED_ADD:
            dialog_name = "EstateBlockedExperienceAdd";
            break;

        case ESTATE_EXPERIENCE_BLOCKED_REMOVE:
            dialog_name = "EstateBlockedExperienceRemove";
            break;

        default:
            return;
    }

    LLSD payload;
    payload["operation"] = (S32)event_type;
    payload["dialog_name"] = dialog_name;
    payload["allowed_ids"].append(id);

    LLSD args;
    args["ALL_ESTATES"] = all_estates_text();

    LLNotification::Params params(dialog_name);
    params.payload(payload)
        .substitutions(args)
        .functor.function(LLPanelRegionExperiences::experienceCoreConfirm);
    if (LLPanelEstateInfo::isLindenEstate())
    {
        LLNotifications::instance().forceResponse(params, 0);
    }
    else
    {
        LLNotifications::instance().add(params);
    }

    onChangeAnything();
}


LLPanelEstateAccess::LLPanelEstateAccess()
: LLPanelRegionInfo(), mPendingUpdate(false)
{}

bool LLPanelEstateAccess::postBuild()
{
    getChild<LLUICtrl>("allowed_avatar_name_list")->setCommitCallback(boost::bind(&LLPanelEstateInfo::onChangeChildCtrl, this, _1));
    LLNameListCtrl *avatar_name_list = getChild<LLNameListCtrl>("allowed_avatar_name_list");
    if (avatar_name_list)
    {
        avatar_name_list->setCommitOnSelectionChange(true);
        avatar_name_list->setMaxItemCount(ESTATE_MAX_ACCESS_IDS);
    }

    getChild<LLUICtrl>("allowed_search_input")->setCommitCallback(boost::bind(&LLPanelEstateAccess::onAllowedSearchEdit, this, _2));
    childSetAction("add_allowed_avatar_btn", boost::bind(&LLPanelEstateAccess::onClickAddAllowedAgent, this));
    childSetAction("remove_allowed_avatar_btn", boost::bind(&LLPanelEstateAccess::onClickRemoveAllowedAgent, this));
    childSetAction("copy_allowed_list_btn", boost::bind(&LLPanelEstateAccess::onClickCopyAllowedList, this));

    getChild<LLUICtrl>("allowed_group_name_list")->setCommitCallback(boost::bind(&LLPanelEstateInfo::onChangeChildCtrl, this, _1));
    LLNameListCtrl* group_name_list = getChild<LLNameListCtrl>("allowed_group_name_list");
    if (group_name_list)
    {
        group_name_list->setCommitOnSelectionChange(true);
        group_name_list->setMaxItemCount(ESTATE_MAX_ACCESS_IDS);
    }

    getChild<LLUICtrl>("allowed_group_search_input")->setCommitCallback(boost::bind(&LLPanelEstateAccess::onAllowedGroupsSearchEdit, this, _2));
    getChild<LLUICtrl>("add_allowed_group_btn")->setCommitCallback(boost::bind(&LLPanelEstateAccess::onClickAddAllowedGroup, this));
    childSetAction("remove_allowed_group_btn", boost::bind(&LLPanelEstateAccess::onClickRemoveAllowedGroup, this));
    childSetAction("copy_allowed_group_list_btn", boost::bind(&LLPanelEstateAccess::onClickCopyAllowedGroupList, this));

    getChild<LLUICtrl>("banned_avatar_name_list")->setCommitCallback(boost::bind(&LLPanelEstateInfo::onChangeChildCtrl, this, _1));
    LLNameListCtrl* banned_name_list = getChild<LLNameListCtrl>("banned_avatar_name_list");
    if (banned_name_list)
    {
        banned_name_list->setCommitOnSelectionChange(true);
        banned_name_list->setMaxItemCount(ESTATE_MAX_BANNED_IDS);
    }

    getChild<LLUICtrl>("banned_search_input")->setCommitCallback(boost::bind(&LLPanelEstateAccess::onBannedSearchEdit, this, _2));
    childSetAction("add_banned_avatar_btn", boost::bind(&LLPanelEstateAccess::onClickAddBannedAgent, this));
    childSetAction("remove_banned_avatar_btn", boost::bind(&LLPanelEstateAccess::onClickRemoveBannedAgent, this));
    childSetAction("copy_banned_list_btn", boost::bind(&LLPanelEstateAccess::onClickCopyBannedList, this));

    getChild<LLUICtrl>("estate_manager_name_list")->setCommitCallback(boost::bind(&LLPanelEstateInfo::onChangeChildCtrl, this, _1));
    LLNameListCtrl* manager_name_list = getChild<LLNameListCtrl>("estate_manager_name_list");
    if (manager_name_list)
    {
        manager_name_list->setCommitOnSelectionChange(true);
        manager_name_list->setMaxItemCount(ESTATE_MAX_MANAGERS * 4);    // Allow extras for dupe issue
    }

    childSetAction("add_estate_manager_btn", boost::bind(&LLPanelEstateAccess::onClickAddEstateManager, this));
    childSetAction("remove_estate_manager_btn", boost::bind(&LLPanelEstateAccess::onClickRemoveEstateManager, this));

    return true;
}

void LLPanelEstateAccess::updateControls(LLViewerRegion* region)
{
    bool god = gAgent.isGodlike();
    bool owner = (region && (region->getOwner() == gAgent.getID()));
    bool manager = (region && region->isEstateManager());
    bool enable_cotrols = god || owner || manager;
    setCtrlsEnabled(enable_cotrols);

    LLNameListCtrl* allowedAvatars = getChild<LLNameListCtrl>("allowed_avatar_name_list");
    LLNameListCtrl* allowedGroups = getChild<LLNameListCtrl>("allowed_group_name_list");
    LLNameListCtrl* bannedAvatars = getChild<LLNameListCtrl>("banned_avatar_name_list");
    LLNameListCtrl* estateManagers = getChild<LLNameListCtrl>("estate_manager_name_list");

    bool has_allowed_avatar = allowedAvatars->getFirstSelected();
    bool has_allowed_group = allowedGroups->getFirstSelected();
    bool has_banned_agent = bannedAvatars->getFirstSelected();
    bool has_estate_manager = estateManagers->getFirstSelected();

    getChildView("add_allowed_avatar_btn")->setEnabled(enable_cotrols);
    getChildView("remove_allowed_avatar_btn")->setEnabled(has_allowed_avatar && enable_cotrols);
    allowedAvatars->setEnabled(enable_cotrols);

    getChildView("add_allowed_group_btn")->setEnabled(enable_cotrols);
    getChildView("remove_allowed_group_btn")->setEnabled(has_allowed_group && enable_cotrols);
    allowedGroups->setEnabled(enable_cotrols);

    // Can't ban people from mainland, orientation islands, etc. because this
    // creates much network traffic and server load.
    // Disable their accounts in CSR tool instead.
    bool linden_estate = LLPanelEstateInfo::isLindenEstate();
    bool enable_ban = enable_cotrols && !linden_estate;
    getChildView("add_banned_avatar_btn")->setEnabled(enable_ban);
    getChildView("remove_banned_avatar_btn")->setEnabled(has_banned_agent && enable_ban);
    bannedAvatars->setEnabled(enable_cotrols);

    // estate managers can't add estate managers
    getChildView("add_estate_manager_btn")->setEnabled(god || owner);
    getChildView("remove_estate_manager_btn")->setEnabled(has_estate_manager && (god || owner));
    estateManagers->setEnabled(god || owner);

    if (enable_cotrols != mCtrlsEnabled)
    {
        mCtrlsEnabled = enable_cotrols;
        updateLists(); // update the lists on the agent's access level change
    }
}

//---------------------------------------------------------------------------
// Add/Remove estate access button callbacks
//---------------------------------------------------------------------------
void LLPanelEstateAccess::onClickAddAllowedAgent()
{
    LLCtrlListInterface *list = childGetListInterface("allowed_avatar_name_list");
    if (!list) return;
    if (list->getItemCount() >= ESTATE_MAX_ACCESS_IDS)
    {
        //args

        LLSD args;
        args["MAX_AGENTS"] = llformat("%d", ESTATE_MAX_ACCESS_IDS);
        LLNotificationsUtil::add("MaxAllowedAgentOnRegion", args);
        return;
    }
    accessAddCore(ESTATE_ACCESS_ALLOWED_AGENT_ADD, "EstateAllowedAgentAdd");
}

void LLPanelEstateAccess::onClickRemoveAllowedAgent()
{
    accessRemoveCore(ESTATE_ACCESS_ALLOWED_AGENT_REMOVE, "EstateAllowedAgentRemove", "allowed_avatar_name_list");
}

void LLPanelEstateAccess::onClickAddAllowedGroup()
{
    LLCtrlListInterface *list = childGetListInterface("allowed_group_name_list");
    if (!list) return;
    if (list->getItemCount() >= ESTATE_MAX_ACCESS_IDS)
    {
        LLSD args;
        args["MAX_GROUPS"] = llformat("%d", ESTATE_MAX_ACCESS_IDS);
        LLNotificationsUtil::add("MaxAllowedGroupsOnRegion", args);
        return;
    }

    LLNotification::Params params("ChangeLindenAccess");
    params.functor.function(boost::bind(&LLPanelEstateAccess::addAllowedGroup, this, _1, _2));
    if (LLPanelEstateInfo::isLindenEstate())
    {
        LLNotifications::instance().add(params);
    }
    else
    {
        LLNotifications::instance().forceResponse(params, 0);
    }
}

bool LLPanelEstateAccess::addAllowedGroup(const LLSD& notification, const LLSD& response)
{
    S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
    if (option != 0) return false;

    LLFloater* parent_floater = gFloaterView->getParentFloater(this);

    LLFloaterGroupPicker* widget = LLFloaterReg::showTypedInstance<LLFloaterGroupPicker>("group_picker", LLSD(gAgent.getID()));
    if (widget)
    {
        widget->removeNoneOption();
        widget->setSelectGroupCallback(boost::bind(&LLPanelEstateAccess::addAllowedGroup2, this, _1));
        if (parent_floater)
        {
            LLRect new_rect = gFloaterView->findNeighboringPosition(parent_floater, widget);
            widget->setOrigin(new_rect.mLeft, new_rect.mBottom);
            parent_floater->addDependentFloater(widget);
        }
    }

    return false;
}

void LLPanelEstateAccess::onClickRemoveAllowedGroup()
{
    accessRemoveCore(ESTATE_ACCESS_ALLOWED_GROUP_REMOVE, "EstateAllowedGroupRemove", "allowed_group_name_list");
}

void LLPanelEstateAccess::onClickAddBannedAgent()
{
    LLCtrlListInterface *list = childGetListInterface("banned_avatar_name_list");
    if (!list) return;
    if (list->getItemCount() >= ESTATE_MAX_BANNED_IDS)
    {
        LLSD args;
        args["MAX_BANNED"] = llformat("%d", ESTATE_MAX_BANNED_IDS);
        LLNotificationsUtil::add("MaxBannedAgentsOnRegion", args);
        return;
    }
    accessAddCore(ESTATE_ACCESS_BANNED_AGENT_ADD, "EstateBannedAgentAdd");
}

void LLPanelEstateAccess::onClickRemoveBannedAgent()
{
    accessRemoveCore(ESTATE_ACCESS_BANNED_AGENT_REMOVE, "EstateBannedAgentRemove", "banned_avatar_name_list");
}

void LLPanelEstateAccess::onClickCopyAllowedList()
{
    copyListToClipboard("allowed_avatar_name_list");
}

void LLPanelEstateAccess::onClickCopyAllowedGroupList()
{
    copyListToClipboard("allowed_group_name_list");
}

void LLPanelEstateAccess::onClickCopyBannedList()
{
    copyListToClipboard("banned_avatar_name_list");
}

// static
void LLPanelEstateAccess::onClickAddEstateManager()
{
    LLCtrlListInterface *list = childGetListInterface("estate_manager_name_list");
    if (!list) return;
    if (list->getItemCount() >= ESTATE_MAX_MANAGERS)
    {   // Tell user they can't add more managers
        LLSD args;
        args["MAX_MANAGER"] = llformat("%d", ESTATE_MAX_MANAGERS);
        LLNotificationsUtil::add("MaxManagersOnRegion", args);
    }
    else
    {   // Go pick managers to add
        accessAddCore(ESTATE_ACCESS_MANAGER_ADD, "EstateManagerAdd");
    }
}

// static
void LLPanelEstateAccess::onClickRemoveEstateManager()
{
    accessRemoveCore(ESTATE_ACCESS_MANAGER_REMOVE, "EstateManagerRemove", "estate_manager_name_list");
}


// Special case callback for groups, since it has different callback format than names
void LLPanelEstateAccess::addAllowedGroup2(LLUUID id)
{
    LLPanelEstateAccess* panel = LLFloaterRegionInfo::getPanelAccess();
    if (panel)
    {
        LLNameListCtrl* group_list = panel->getChild<LLNameListCtrl>("allowed_group_name_list");
        LLScrollListItem* item = group_list->getNameItemByAgentId(id);
        if (item)
        {
            LLSD args;
            args["GROUP"] = item->getColumn(0)->getValue().asString();
            LLNotificationsUtil::add("GroupIsAlreadyInList", args);
            return;
        }
    }

    LLSD payload;
    payload["operation"] = (S32)ESTATE_ACCESS_ALLOWED_GROUP_ADD;
    payload["dialog_name"] = "EstateAllowedGroupAdd";
    payload["allowed_ids"].append(id);

    LLSD args;
    args["ALL_ESTATES"] = all_estates_text();

    LLNotification::Params params("EstateAllowedGroupAdd");
    params.payload(payload)
        .substitutions(args)
        .functor.function(accessCoreConfirm);
    if (LLPanelEstateInfo::isLindenEstate())
    {
        LLNotifications::instance().forceResponse(params, 0);
    }
    else
    {
        LLNotifications::instance().add(params);
    }
}

// static
void LLPanelEstateAccess::accessAddCore(U32 operation_flag, const std::string& dialog_name)
{
    LLSD payload;
    payload["operation"] = (S32)operation_flag;
    payload["dialog_name"] = dialog_name;
    // agent id filled in after avatar picker

    LLNotification::Params params("ChangeLindenAccess");
    params.payload(payload)
        .functor.function(accessAddCore2);

    if (LLPanelEstateInfo::isLindenEstate())
    {
        LLNotifications::instance().add(params);
    }
    else
    {
        // same as clicking "OK"
        LLNotifications::instance().forceResponse(params, 0);
    }
}

// static
bool LLPanelEstateAccess::accessAddCore2(const LLSD& notification, const LLSD& response)
{
    S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
    if (option != 0)
    {
        // abort change
        return false;
    }

    LLEstateAccessChangeInfo* change_info = new LLEstateAccessChangeInfo(notification["payload"]);
    //Get parent floater name
    LLPanelEstateAccess* panel = LLFloaterRegionInfo::getPanelAccess();
    LLFloater* parent_floater = panel ? gFloaterView->getParentFloater(panel) : NULL;
    const std::string& parent_floater_name = parent_floater ? parent_floater->getName() : "";

    //Determine the button that triggered opening of the avatar picker
    //(so that a shadow frustum from the button to the avatar picker can be created)
    LLView * button = NULL;
    switch (change_info->mOperationFlag)
    {
    case ESTATE_ACCESS_ALLOWED_AGENT_ADD:
        button = panel->findChild<LLButton>("add_allowed_avatar_btn");
        break;

    case ESTATE_ACCESS_BANNED_AGENT_ADD:
        button = panel->findChild<LLButton>("add_banned_avatar_btn");
        break;

    case ESTATE_ACCESS_MANAGER_ADD:
        button = panel->findChild<LLButton>("add_estate_manager_btn");
        break;
    }

    // avatar picker yes multi-select, yes close-on-select
    LLFloater* child_floater = LLFloaterAvatarPicker::show(boost::bind(&LLPanelEstateAccess::accessAddCore3, _1, _2, (void*)change_info),
        true, true, false, parent_floater_name, button);

    //Allows the closed parent floater to close the child floater (avatar picker)
    if (child_floater)
    {
        parent_floater->addDependentFloater(child_floater);
    }

    return false;
}

// static
void LLPanelEstateAccess::accessAddCore3(const uuid_vec_t& ids, std::vector<LLAvatarName> names, void* data)
{
    LLEstateAccessChangeInfo* change_info = (LLEstateAccessChangeInfo*)data;
    if (!change_info) return;
    if (ids.empty())
    {
        // User didn't select a name.
        delete change_info;
        change_info = NULL;
        return;
    }
    // User did select a name.
    change_info->mAgentOrGroupIDs = ids;
    // Can't put estate owner on ban list
    LLPanelEstateAccess* panel = LLFloaterRegionInfo::getPanelAccess();
    if (!panel) return;
    LLViewerRegion* region = gAgent.getRegion();
    if (!region) return;

    if (change_info->mOperationFlag & ESTATE_ACCESS_ALLOWED_AGENT_ADD)
    {
        LLNameListCtrl* name_list = panel->getChild<LLNameListCtrl>("allowed_avatar_name_list");
        int currentCount = (name_list ? name_list->getItemCount() : 0);
        if (ids.size() + currentCount > ESTATE_MAX_ACCESS_IDS)
        {
            LLSD args;
            args["NUM_ADDED"] = llformat("%d", ids.size());
            args["MAX_AGENTS"] = llformat("%d", ESTATE_MAX_ACCESS_IDS);
            args["LIST_TYPE"] = LLTrans::getString("RegionInfoListTypeAllowedAgents");
            args["NUM_EXCESS"] = llformat("%d", (ids.size() + currentCount) - ESTATE_MAX_ACCESS_IDS);
            LLNotificationsUtil::add("MaxAgentOnRegionBatch", args);
            delete change_info;
            return;
        }

        uuid_vec_t ids_allowed;
        std::vector<LLAvatarName> names_allowed;
        std::string already_allowed;
        bool single = true;
        for (U32 i = 0; i < ids.size(); ++i)
        {
            LLScrollListItem* item = name_list->getNameItemByAgentId(ids[i]);
            if (item)
            {
                if (!already_allowed.empty())
                {
                    already_allowed += ", ";
                    single = false;
                }
                already_allowed += item->getColumn(0)->getValue().asString();
            }
            else
            {
                ids_allowed.push_back(ids[i]);
                names_allowed.push_back(names[i]);
            }
        }
        if (!already_allowed.empty())
        {
            LLSD args;
            args["AGENT"] = already_allowed;
            args["LIST_TYPE"] = LLTrans::getString("RegionInfoListTypeAllowedAgents");
            LLNotificationsUtil::add(single ? "AgentIsAlreadyInList" : "AgentsAreAlreadyInList", args);
            if (ids_allowed.empty())
            {
                delete change_info;
                return;
            }
        }
        change_info->mAgentOrGroupIDs = ids_allowed;
        change_info->mAgentNames = names_allowed;
    }
    if (change_info->mOperationFlag & ESTATE_ACCESS_BANNED_AGENT_ADD)
    {
        LLNameListCtrl* name_list = panel->getChild<LLNameListCtrl>("banned_avatar_name_list");
        LLNameListCtrl* em_list = panel->getChild<LLNameListCtrl>("estate_manager_name_list");
        int currentCount = (name_list ? name_list->getItemCount() : 0);
        if (ids.size() + currentCount > ESTATE_MAX_BANNED_IDS)
        {
            LLSD args;
            args["NUM_ADDED"] = llformat("%d", ids.size());
            args["MAX_AGENTS"] = llformat("%d", ESTATE_MAX_BANNED_IDS);
            args["LIST_TYPE"] = LLTrans::getString("RegionInfoListTypeBannedAgents");
            args["NUM_EXCESS"] = llformat("%d", (ids.size() + currentCount) - ESTATE_MAX_BANNED_IDS);
            LLNotificationsUtil::add("MaxAgentOnRegionBatch", args);
            delete change_info;
            return;
        }

        uuid_vec_t ids_allowed;
        std::vector<LLAvatarName> names_allowed;
        std::string already_banned;
        std::string em_ban;
        bool single = true;
        for (U32 i = 0; i < ids.size(); ++i)
        {
            bool is_allowed = true;
            LLScrollListItem* em_item = em_list->getNameItemByAgentId(ids[i]);
            if (em_item)
            {
                if (!em_ban.empty())
                {
                    em_ban += ", ";
                }
                em_ban += em_item->getColumn(0)->getValue().asString();
                is_allowed = false;
            }

            LLScrollListItem* item = name_list->getNameItemByAgentId(ids[i]);
            if (item)
            {
                if (!already_banned.empty())
                {
                    already_banned += ", ";
                    single = false;
                }
                already_banned += item->getColumn(0)->getValue().asString();
                is_allowed = false;
            }

            if (is_allowed)
            {
                ids_allowed.push_back(ids[i]);
                names_allowed.push_back(names[i]);
            }
        }
        if (!em_ban.empty())
        {
            LLSD args;
            args["AGENT"] = em_ban;
            LLNotificationsUtil::add("ProblemBanningEstateManager", args);
            if (ids_allowed.empty())
            {
                delete change_info;
                return;
            }
        }
        if (!already_banned.empty())
        {
            LLSD args;
            args["AGENT"] = already_banned;
            args["LIST_TYPE"] = LLTrans::getString("RegionInfoListTypeBannedAgents");
            LLNotificationsUtil::add(single ? "AgentIsAlreadyInList" : "AgentsAreAlreadyInList", args);
            if (ids_allowed.empty())
            {
                delete change_info;
                return;
            }
        }
        change_info->mAgentOrGroupIDs = ids_allowed;
        change_info->mAgentNames = names_allowed;
    }

    LLSD args;
    args["ALL_ESTATES"] = all_estates_text();
    LLNotification::Params params(change_info->mDialogName);
    params.substitutions(args)
        .payload(change_info->asLLSD())
        .functor.function(accessCoreConfirm);

    if (LLPanelEstateInfo::isLindenEstate())
    {
        // just apply to this estate
        LLNotifications::instance().forceResponse(params, 0);
    }
    else
    {
        // ask if this estate or all estates with this owner
        LLNotifications::instance().add(params);
    }
}

// static
void LLPanelEstateAccess::accessRemoveCore(U32 operation_flag, const std::string& dialog_name, const std::string& list_ctrl_name)
{
    LLPanelEstateAccess* panel = LLFloaterRegionInfo::getPanelAccess();
    if (!panel) return;
    LLNameListCtrl* name_list = panel->getChild<LLNameListCtrl>(list_ctrl_name);
    if (!name_list) return;

    std::vector<LLScrollListItem*> list_vector = name_list->getAllSelected();
    if (list_vector.size() == 0)
        return;

    LLSD payload;
    payload["operation"] = (S32)operation_flag;
    payload["dialog_name"] = dialog_name;

    for (std::vector<LLScrollListItem*>::const_iterator iter = list_vector.begin();
        iter != list_vector.end();
        iter++)
    {
        LLScrollListItem *item = (*iter);
        payload["allowed_ids"].append(item->getUUID());
    }

    LLNotification::Params params("ChangeLindenAccess");
    params.payload(payload)
        .functor.function(accessRemoveCore2);

    if (LLPanelEstateInfo::isLindenEstate())
    {
        // warn on change linden estate
        LLNotifications::instance().add(params);
    }
    else
    {
        // just proceed, as if clicking OK
        LLNotifications::instance().forceResponse(params, 0);
    }
}

// static
bool LLPanelEstateAccess::accessRemoveCore2(const LLSD& notification, const LLSD& response)
{
    S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
    if (option != 0)
    {
        // abort
        return false;
    }

    // If Linden estate, can only apply to "this" estate, not all estates
    // owned by NULL.
    if (LLPanelEstateInfo::isLindenEstate())
    {
        accessCoreConfirm(notification, response);
    }
    else
    {
        LLSD args;
        args["ALL_ESTATES"] = all_estates_text();
        LLNotificationsUtil::add(notification["payload"]["dialog_name"],
            args,
            notification["payload"],
            accessCoreConfirm);
    }
    return false;
}

// Used for both access add and remove operations, depending on the mOperationFlag
// passed in (ESTATE_ACCESS_BANNED_AGENT_ADD, ESTATE_ACCESS_ALLOWED_AGENT_REMOVE, etc.)
// static
bool LLPanelEstateAccess::accessCoreConfirm(const LLSD& notification, const LLSD& response)
{
    S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
    const U32 originalFlags = (U32)notification["payload"]["operation"].asInteger();
    U32 flags = originalFlags;

    LLViewerRegion* region = gAgent.getRegion();

    if (option == 2) // cancel
    {
        return false;
    }
    else if (option == 1)
    {
        // All estates, either than I own or manage for this owner.
        // This will be verified on simulator. JC
        if (!region) return false;
        if (region->getOwner() == gAgent.getID()
            || gAgent.isGodlike())
        {
            flags |= ESTATE_ACCESS_APPLY_TO_ALL_ESTATES;
        }
        else if (region->isEstateManager())
        {
            flags |= ESTATE_ACCESS_APPLY_TO_MANAGED_ESTATES;
        }
    }

    std::string names;
    U32 listed_names = 0;
    for (U32 i = 0; i < notification["payload"]["allowed_ids"].size(); ++i)
    {
        if (i + 1 != notification["payload"]["allowed_ids"].size())
        {
            flags |= ESTATE_ACCESS_NO_REPLY;
        }
        else
        {
            flags &= ~ESTATE_ACCESS_NO_REPLY;
        }

        const LLUUID id = notification["payload"]["allowed_ids"][i].asUUID();
        if (((U32)notification["payload"]["operation"].asInteger() & ESTATE_ACCESS_BANNED_AGENT_ADD)
            && region && (region->getOwner() == id))
        {
            LLNotificationsUtil::add("OwnerCanNotBeDenied");
            break;
        }

        sendEstateAccessDelta(flags, id);

        if ((flags & (ESTATE_ACCESS_ALLOWED_GROUP_ADD | ESTATE_ACCESS_ALLOWED_GROUP_REMOVE)) == 0)
        {
            // fill the name list for confirmation
            if (listed_names < MAX_LISTED_NAMES)
            {
                if (!names.empty())
                {
                    names += ", ";
                }
                if (!notification["payload"]["allowed_names"][i]["display_name"].asString().empty())
                {
                    names += notification["payload"]["allowed_names"][i]["display_name"].asString();
                }
                else
                { //try to get an agent name from cache
                    LLAvatarName av_name;
                    if (LLAvatarNameCache::get(id, &av_name))
                    {
                        names += av_name.getCompleteName();
                    }
                }

            }
            listed_names++;
        }
    }
    if (listed_names > MAX_LISTED_NAMES)
    {
        LLSD args;
        args["EXTRA_COUNT"] = llformat("%d", listed_names - MAX_LISTED_NAMES);
        names += " " + LLTrans::getString("AndNMore", args);
    }

    if (!names.empty()) // show the conirmation
    {
        LLSD args;
        args["AGENT"] = names;

        if (flags & (ESTATE_ACCESS_ALLOWED_AGENT_ADD | ESTATE_ACCESS_ALLOWED_AGENT_REMOVE))
        {
            args["LIST_TYPE"] = LLTrans::getString("RegionInfoListTypeAllowedAgents");
        }
        else if (flags & (ESTATE_ACCESS_BANNED_AGENT_ADD | ESTATE_ACCESS_BANNED_AGENT_REMOVE))
        {
            args["LIST_TYPE"] = LLTrans::getString("RegionInfoListTypeBannedAgents");
        }

        if (flags & ESTATE_ACCESS_APPLY_TO_ALL_ESTATES)
        {
            args["ESTATE"] = LLTrans::getString("RegionInfoAllEstates");
        }
        else if (flags & ESTATE_ACCESS_APPLY_TO_MANAGED_ESTATES)
        {
            args["ESTATE"] = LLTrans::getString("RegionInfoManagedEstates");
        }
        else
        {
            args["ESTATE"] = LLTrans::getString("RegionInfoThisEstate");
        }

        bool single = (listed_names == 1);
        if (flags & (ESTATE_ACCESS_ALLOWED_AGENT_ADD | ESTATE_ACCESS_BANNED_AGENT_ADD))
        {
            LLNotificationsUtil::add(single ? "AgentWasAddedToList" : "AgentsWereAddedToList", args);
        }
        else if (flags & (ESTATE_ACCESS_ALLOWED_AGENT_REMOVE | ESTATE_ACCESS_BANNED_AGENT_REMOVE))
        {
            LLNotificationsUtil::add(single ? "AgentWasRemovedFromList" : "AgentsWereRemovedFromList", args);
        }
    }

    LLPanelEstateAccess* panel = LLFloaterRegionInfo::getPanelAccess();
    if (panel)
    {
        panel->setPendingUpdate(true);
    }

    return false;
}

// key = "estateaccessdelta"
// str(estate_id) will be added to front of list by forward_EstateOwnerRequest_to_dataserver
// str[0] = str(agent_id) requesting the change
// str[1] = str(flags) (ESTATE_ACCESS_DELTA_*)
// str[2] = str(agent_id) to add or remove
// static
void LLPanelEstateAccess::sendEstateAccessDelta(U32 flags, const LLUUID& agent_or_group_id)
{
    LLMessageSystem* msg = gMessageSystem;
    msg->newMessage("EstateOwnerMessage");
    msg->nextBlockFast(_PREHASH_AgentData);
    msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
    msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
    msg->addUUIDFast(_PREHASH_TransactionID, LLUUID::null); //not used

    msg->nextBlock("MethodData");
    msg->addString("Method", "estateaccessdelta");
    msg->addUUID("Invoice", LLFloaterRegionInfo::getLastInvoice());

    std::string buf;
    gAgent.getID().toString(buf);
    msg->nextBlock("ParamList");
    msg->addString("Parameter", buf);

    buf = llformat("%u", flags);
    msg->nextBlock("ParamList");
    msg->addString("Parameter", buf);

    agent_or_group_id.toString(buf);
    msg->nextBlock("ParamList");
    msg->addString("Parameter", buf);

    gAgent.sendReliableMessage();
}

void LLPanelEstateAccess::updateChild(LLUICtrl* child_ctrl)
{
    // Ensure appropriate state of the management ui.
    updateControls(gAgent.getRegion());
}

void LLPanelEstateAccess::updateLists()
{
    std::string cap_url = gAgent.getRegionCapability("EstateAccess");
    if (!cap_url.empty())
    {
        LLCoros::instance().launch("LLFloaterRegionInfo::requestEstateGetAccessCoro", boost::bind(LLPanelEstateAccess::requestEstateGetAccessCoro, cap_url));
    }
}

void LLPanelEstateAccess::requestEstateGetAccessCoro(std::string url)
{
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("requestEstateGetAccessoCoro", httpPolicy));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);

    LLSD result = httpAdapter->getAndSuspend(httpRequest, url);

    LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    LLPanelEstateAccess* panel = LLFloaterRegionInfo::getPanelAccess();
    if (!panel) return;

    LLNameListCtrl* allowed_agent_name_list = panel->getChild<LLNameListCtrl>("allowed_avatar_name_list");
    if (allowed_agent_name_list && result.has("AllowedAgents"))
    {
        LLStringUtil::format_map_t args;
        args["[ALLOWEDAGENTS]"] = llformat("%d", result["AllowedAgents"].size());
        args["[MAXACCESS]"] = llformat("%d", ESTATE_MAX_ACCESS_IDS);
        std::string msg = LLTrans::getString("RegionInfoAllowedResidents", args);
        panel->getChild<LLUICtrl>("allow_resident_label")->setValue(LLSD(msg));

        allowed_agent_name_list->clearSortOrder();
        allowed_agent_name_list->deleteAllItems();
        for (LLSD::array_const_iterator it = result["AllowedAgents"].beginArray(); it != result["AllowedAgents"].endArray(); ++it)
        {
            LLUUID id = (*it)["id"].asUUID();
            allowed_agent_name_list->addNameItem(id);
        }
        allowed_agent_name_list->sortByName(true);
    }

    LLNameListCtrl* banned_agent_name_list = panel->getChild<LLNameListCtrl>("banned_avatar_name_list");
    if (banned_agent_name_list && result.has("BannedAgents"))
    {
        LLStringUtil::format_map_t args;
        args["[BANNEDAGENTS]"] = llformat("%d", result["BannedAgents"].size());
        args["[MAXBANNED]"] = llformat("%d", ESTATE_MAX_BANNED_IDS);
        std::string msg = LLTrans::getString("RegionInfoBannedResidents", args);
        panel->getChild<LLUICtrl>("ban_resident_label")->setValue(LLSD(msg));

        banned_agent_name_list->clearSortOrder();
        banned_agent_name_list->deleteAllItems();
        for (LLSD::array_const_iterator it = result["BannedAgents"].beginArray(); it != result["BannedAgents"].endArray(); ++it)
        {
            LLSD item;
            item["id"] = (*it)["id"].asUUID();
            LLSD& columns = item["columns"];

            columns[0]["column"] = "name"; // to be populated later

            columns[1]["column"] = "last_login_date";
            columns[1]["value"] = (*it)["last_login_date"].asString().substr(0, 16); // cut the seconds

            std::string ban_date = (*it)["ban_date"].asString();
            columns[2]["column"] = "ban_date";
            columns[2]["value"] = ban_date[0] != '0' ? ban_date.substr(0, 16) : LLTrans::getString("na"); // server returns the "0000-00-00 00:00:00" date in case it doesn't know it

            columns[3]["column"] = "bannedby";
            LLUUID banning_id = (*it)["banning_id"].asUUID();
            LLAvatarName av_name;
            if (banning_id.isNull())
            {
                columns[3]["value"] = LLTrans::getString("na");
            }
            else if (LLAvatarNameCache::get(banning_id, &av_name))
            {
                columns[3]["value"] = av_name.getCompleteName(); //TODO: fetch the name if it wasn't cached
            }

            banned_agent_name_list->addElement(item);
        }
        banned_agent_name_list->sortByName(true);
    }

    LLNameListCtrl* allowed_group_name_list = panel->getChild<LLNameListCtrl>("allowed_group_name_list");
    if (allowed_group_name_list && result.has("AllowedGroups"))
    {
        LLStringUtil::format_map_t args;
        args["[ALLOWEDGROUPS]"] = llformat("%d", result["AllowedGroups"].size());
        args["[MAXACCESS]"] = llformat("%d", ESTATE_MAX_GROUP_IDS);
        std::string msg = LLTrans::getString("RegionInfoAllowedGroups", args);
        panel->getChild<LLUICtrl>("allow_group_label")->setValue(LLSD(msg));

        allowed_group_name_list->clearSortOrder();
        allowed_group_name_list->deleteAllItems();
        for (LLSD::array_const_iterator it = result["AllowedGroups"].beginArray(); it != result["AllowedGroups"].endArray(); ++it)
        {
            LLUUID id = (*it)["id"].asUUID();
            allowed_group_name_list->addGroupNameItem(id);
        }
        allowed_group_name_list->sortByName(true);
    }

    LLNameListCtrl* estate_manager_name_list = panel->getChild<LLNameListCtrl>("estate_manager_name_list");
    if (estate_manager_name_list && result.has("Managers"))
    {
        LLStringUtil::format_map_t args;
        args["[ESTATEMANAGERS]"] = llformat("%d", result["Managers"].size());
        args["[MAXMANAGERS]"] = llformat("%d", ESTATE_MAX_MANAGERS);
        std::string msg = LLTrans::getString("RegionInfoEstateManagers", args);
        panel->getChild<LLUICtrl>("estate_manager_label")->setValue(LLSD(msg));

        estate_manager_name_list->clearSortOrder();
        estate_manager_name_list->deleteAllItems();
        for (LLSD::array_const_iterator it = result["Managers"].beginArray(); it != result["Managers"].endArray(); ++it)
        {
            LLUUID id = (*it)["agent_id"].asUUID();
            estate_manager_name_list->addNameItem(id);
        }
        estate_manager_name_list->sortByName(true);
    }


    panel->updateControls(gAgent.getRegion());
}

//---------------------------------------------------------------------------
// Access lists search
//---------------------------------------------------------------------------
void LLPanelEstateAccess::onAllowedSearchEdit(const std::string& search_string)
{
    LLPanelEstateAccess* panel = LLFloaterRegionInfo::getPanelAccess();
    if (!panel) return;
    LLNameListCtrl* allowed_agent_name_list = panel->getChild<LLNameListCtrl>("allowed_avatar_name_list");
    searchAgent(allowed_agent_name_list, search_string);
}

void LLPanelEstateAccess::onAllowedGroupsSearchEdit(const std::string& search_string)
{
    LLPanelEstateAccess* panel = LLFloaterRegionInfo::getPanelAccess();
    if (!panel) return;
    LLNameListCtrl* allowed_group_name_list = panel->getChild<LLNameListCtrl>("allowed_group_name_list");
    searchAgent(allowed_group_name_list, search_string);
}

void LLPanelEstateAccess::onBannedSearchEdit(const std::string& search_string)
{
    LLPanelEstateAccess* panel = LLFloaterRegionInfo::getPanelAccess();
    if (!panel) return;
    LLNameListCtrl* banned_agent_name_list = panel->getChild<LLNameListCtrl>("banned_avatar_name_list");
    searchAgent(banned_agent_name_list, search_string);
}

void LLPanelEstateAccess::searchAgent(LLNameListCtrl* listCtrl, const std::string& search_string)
{
    if (!listCtrl) return;

    if (!search_string.empty())
    {
        listCtrl->setSearchColumn(0); // name column
        listCtrl->searchItems(search_string, false, true);
    }
    else
    {
        listCtrl->deselectAllItems(true);
    }
}

void LLPanelEstateAccess::copyListToClipboard(std::string list_name)
{
    LLPanelEstateAccess* panel = LLFloaterRegionInfo::getPanelAccess();
    if (!panel) return;

    LLNameListCtrl* name_list = panel->getChild<LLNameListCtrl>(list_name);
    std::vector<LLScrollListItem*> list_vector = name_list->getAllData();
    if (list_vector.empty())
        return;

    LLSD::String list_to_copy;
    for (LLScrollListItem* item : list_vector)
    {
        if (item)
        {
            if (!list_to_copy.empty())
            {
                list_to_copy += "\n";
            }
            list_to_copy += item->getColumn(0)->getValue().asString();
        }
    }

    LLClipboard::instance().copyToClipboard(utf8str_to_wstring(list_to_copy), 0, static_cast<S32>(list_to_copy.length()));
}

bool LLPanelEstateAccess::refreshFromRegion(LLViewerRegion* region, ERefreshFromRegionPhase phase)
{
    updateLists();
    return LLPanelRegionInfo::refreshFromRegion(region, phase);
}

//=========================================================================
const U32 LLPanelRegionEnvironment::DIRTY_FLAG_OVERRIDE(0x01 << 4);

LLPanelRegionEnvironment::LLPanelRegionEnvironment():
    LLPanelEnvironmentInfo(),
    mAllowOverrideRestore(false)
{
}

LLPanelRegionEnvironment::~LLPanelRegionEnvironment()
{
    if (mCommitConnect.connected())
        mCommitConnect.disconnect();
}

bool LLPanelRegionEnvironment::postBuild()
{
    LLEstateInfoModel& estate_info = LLEstateInfoModel::instance();

    if (!LLPanelEnvironmentInfo::postBuild())
        return false;

    mBtnUseDefault->setLabelArg("[USEDEFAULT]", getString(STR_LABEL_USEDEFAULT));
    mCheckAllowOverride->setVisible(true);
    mPanelEnvAltitudes->setVisible(true);

    mCheckAllowOverride->setCommitCallback([this](LLUICtrl *, const LLSD &value){ onChkAllowOverride(value.asBoolean()); });

    mCommitConnect = estate_info.setCommitCallback(boost::bind(&LLPanelRegionEnvironment::refreshFromEstate, this));
    return true;
}


void LLPanelRegionEnvironment::refresh()
{
    commitDayLenOffsetChanges(false); // commit unsaved changes if any

    if (!mCurrentEnvironment)
    {
        if (mCurEnvVersion <= INVALID_PARCEL_ENVIRONMENT_VERSION)
        {
            refreshFromSource(); // will immediately set mCurEnvVersion
        } // else - already requesting
        return;
    }

    LLPanelEnvironmentInfo::refresh();

    mCheckAllowOverride->setValue(mAllowOverride);
}

bool LLPanelRegionEnvironment::refreshFromRegion(LLViewerRegion* region)
{
    if (!region)
    {
        setNoSelection(true);
        setControlsEnabled(false);
        mCurEnvVersion = INVALID_PARCEL_ENVIRONMENT_VERSION;
        getChild<LLUICtrl>("region_text")->setValue(LLSD(""));
    }
    else
    {
        getChild<LLUICtrl>("region_text")->setValue(LLSD(region->getName()));
    }
    setNoSelection(false);

    if (gAgent.getRegion()->getRegionID() != region->getRegionID())
    {
        setCrossRegion(true);
        mCurEnvVersion = INVALID_PARCEL_ENVIRONMENT_VERSION;
    }
    setCrossRegion(false);

    refreshFromSource();
    return true;
}

void LLPanelRegionEnvironment::refreshFromSource()
{
    LL_DEBUGS("ENVIRONMENT") << "Requesting environment for region, known version " << mCurEnvVersion << LL_ENDL;
    LLHandle<LLPanel> that_h = getHandle();

    if (mCurEnvVersion < UNSET_PARCEL_ENVIRONMENT_VERSION)
    {
        // to mark as requesting
        mCurEnvVersion = UNSET_PARCEL_ENVIRONMENT_VERSION;
    }

    LLEnvironment::instance().requestRegion(
        [that_h](S32 parcel_id, LLEnvironment::EnvironmentInfo::ptr_t envifo) { onEnvironmentReceived(that_h, parcel_id, envifo); });

    setControlsEnabled(false);
}

bool LLPanelRegionEnvironment::confirmUpdateEstateEnvironment(const LLSD& notification, const LLSD& response)
{
    S32 option = LLNotificationsUtil::getSelectedOption(notification, response);

    switch (option)
    {
    case 0:
    {
        LLEstateInfoModel& estate_info = LLEstateInfoModel::instance();

        // update model
        estate_info.setAllowEnvironmentOverride(mAllowOverride);
        // send the update to sim
        estate_info.sendEstateInfo();
        clearDirtyFlag(DIRTY_FLAG_OVERRIDE);
    }
    break;

    case 1:
        mAllowOverride = mAllowOverrideRestore;
        mCheckAllowOverride->setValue(mAllowOverride);
        break;
    default:
        break;
    }
    return false;
}

void LLPanelRegionEnvironment::onChkAllowOverride(bool value)
{
    setDirtyFlag(DIRTY_FLAG_OVERRIDE);
    mAllowOverrideRestore = mAllowOverride;
    mAllowOverride = value;

    std::string notification("EstateParcelEnvironmentOverride");
    if (LLPanelEstateInfo::isLindenEstate())
        notification = "ChangeLindenEstate";

    LLSD args;
    args["ESTATENAME"] = LLEstateInfoModel::instance().getName();
    LLNotification::Params params(notification);
    params.substitutions(args);
    params.functor.function([this](const LLSD& notification, const LLSD& response) { confirmUpdateEstateEnvironment(notification, response); });

    if (!value || LLPanelEstateInfo::isLindenEstate())
    {   // warn if turning off or a Linden Estate
        LLNotifications::instance().add(params);
    }
    else
    {
        LLNotifications::instance().forceResponse(params, 0);
    }

}
