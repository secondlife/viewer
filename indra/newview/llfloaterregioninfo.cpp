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
#include "llfloatergodtools.h"	// for send_sim_wide_deletes()
#include "llfloatertopobjects.h" // added to fix SL-32336
#include "llfloatergroups.h"
#include "llfloaterreg.h"
#include "llfloaterregiondebugconsole.h"
#include "llfloatertelehub.h"
#include "llinventorymodel.h"
#include "lllineeditor.h"
#include "llnamelistctrl.h"
#include "llnotifications.h"
#include "llnotificationsutil.h"
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
#include "llvlcomposition.h"
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

const S32 TERRAIN_TEXTURE_COUNT = 4;
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

	LLSD getIDs( sparam_t::const_iterator it, sparam_t::const_iterator end, S32 count );
};


/*
void unpack_request_params(
	LLMessageSystem* msg,
	LLDispatcher::sparam_t& strings,
	LLDispatcher::iparam_t& integers)
{
	char str_buf[MAX_STRING];
	S32 str_count = msg->getNumberOfBlocksFast(_PREHASH_StringData);
	S32 i;
	for (i = 0; i < str_count; ++i)
	{
		// we treat the SParam as binary data (since it might be an 
		// LLUUID in compressed form which may have embedded \0's,)
		str_buf[0] = '\0';
		S32 data_size = msg->getSizeFast(_PREHASH_StringData, i, _PREHASH_SParam);
		if (data_size >= 0)
		{
			msg->getBinaryDataFast(_PREHASH_StringData, _PREHASH_SParam,
								   str_buf, data_size, i, MAX_STRING - 1);
			strings.push_back(std::string(str_buf, data_size));
		}
	}

	U32 int_buf;
	S32 int_count = msg->getNumberOfBlocksFast(_PREHASH_IntegerData);
	for (i = 0; i < int_count; ++i)
	{
		msg->getU32("IntegerData", "IParam", int_buf, i);
		integers.push_back(int_buf);
	}
}
*/

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

    virtual BOOL        postBuild() override;
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

BOOL LLFloaterRegionInfo::postBuild()
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
	panel->getCommitCallbackRegistrar().add("RegionInfo.ManageTelehub",	boost::bind(&LLPanelRegionInfo::onClickManageTelehub, panel));
	panel->buildFromFile("panel_region_general.xml");
	mTab->addTabPanel(panel);

	panel = new LLPanelRegionTerrainInfo;
	mInfoPanels.push_back(panel);
	panel->buildFromFile("panel_region_terrain.xml");
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
		return TRUE;
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

	return TRUE;
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
	refreshFromRegion(gAgent.getRegion());
	requestRegionInfo();

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
        requestRegionInfo();
    }
}

// static
void LLFloaterRegionInfo::requestRegionInfo()
{
	LLTabContainer* tab = findChild<LLTabContainer>("region_panels");
	if (tab)
	{
		tab->getChild<LLPanel>("General")->setCtrlsEnabled(FALSE);
		tab->getChild<LLPanel>("Debug")->setCtrlsEnabled(FALSE);
		tab->getChild<LLPanel>("Terrain")->setCtrlsEnabled(FALSE);
		tab->getChild<LLPanel>("Estate")->setCtrlsEnabled(FALSE);
        tab->getChild<LLPanel>("Access")->setCtrlsEnabled(FALSE);
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
	BOOL allow_modify = gAgent.isGodlike() || (region && region->canManageEstate());

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
	BOOL use_estate_sun;
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

	panel->getChild<LLUICtrl>("block_terraform_check")->setValue((region_flags & REGION_FLAGS_BLOCK_TERRAFORM) ? TRUE : FALSE );
	panel->getChild<LLUICtrl>("block_fly_check")->setValue((region_flags & REGION_FLAGS_BLOCK_FLY) ? TRUE : FALSE );
	panel->getChild<LLUICtrl>("block_fly_over_check")->setValue((region_flags & REGION_FLAGS_BLOCK_FLYOVER) ? TRUE : FALSE );
	panel->getChild<LLUICtrl>("allow_damage_check")->setValue((region_flags & REGION_FLAGS_ALLOW_DAMAGE) ? TRUE : FALSE );
	panel->getChild<LLUICtrl>("restrict_pushobject")->setValue((region_flags & REGION_FLAGS_RESTRICT_PUSHOBJECT) ? TRUE : FALSE );
	panel->getChild<LLUICtrl>("allow_land_resell_check")->setValue((region_flags & REGION_FLAGS_BLOCK_LAND_RESELL) ? FALSE : TRUE );
	panel->getChild<LLUICtrl>("allow_parcel_changes_check")->setValue((region_flags & REGION_FLAGS_ALLOW_PARCEL_CHANGES) ? TRUE : FALSE );
	panel->getChild<LLUICtrl>("block_parcel_search_check")->setValue((region_flags & REGION_FLAGS_BLOCK_PARCEL_SEARCH) ? TRUE : FALSE );
	panel->getChild<LLUICtrl>("agent_limit_spin")->setValue(LLSD((F32)agent_limit) );
	panel->getChild<LLUICtrl>("object_bonus_spin")->setValue(LLSD(object_bonus_factor) );
	panel->getChild<LLUICtrl>("access_combo")->setValue(LLSD(sim_access) );

	panel->getChild<LLSpinCtrl>("agent_limit_spin")->setMaxValue(hard_agent_limit);

	LLPanelRegionGeneralInfo* panel_general = LLFloaterRegionInfo::getPanelGeneral();
	if (panel)
	{
		panel_general->setObjBonusFactor(object_bonus_factor);
	}

 	// detect teen grid for maturity

	U32 parent_estate_id;
	msg->getU32("RegionInfo", "ParentEstateID", parent_estate_id);
	BOOL teen_grid = (parent_estate_id == 5);  // *TODO add field to estate table and test that
	panel->getChildView("access_combo")->setEnabled(gAgent.isGodlike() || (region && region->canManageEstate() && !teen_grid));
	panel->setCtrlsEnabled(allow_modify);
	

	// DEBUG PANEL
	panel = tab->getChild<LLPanel>("Debug");

	panel->getChild<LLUICtrl>("region_text")->setValue(LLSD(sim_name) );
	panel->getChild<LLUICtrl>("disable_scripts_check")->setValue(LLSD((BOOL)((region_flags & REGION_FLAGS_SKIP_SCRIPTS) ? TRUE : FALSE )) );
	panel->getChild<LLUICtrl>("disable_collisions_check")->setValue(LLSD((BOOL)((region_flags & REGION_FLAGS_SKIP_COLLISIONS) ? TRUE : FALSE )) );
	panel->getChild<LLUICtrl>("disable_physics_check")->setValue(LLSD((BOOL)((region_flags & REGION_FLAGS_SKIP_PHYSICS) ? TRUE : FALSE )) );
	panel->setCtrlsEnabled(allow_modify);

	// TERRAIN PANEL
	panel = tab->getChild<LLPanel>("Terrain");

	panel->getChild<LLUICtrl>("region_text")->setValue(LLSD(sim_name));
	panel->getChild<LLUICtrl>("water_height_spin")->setValue(region_info.mWaterHeight);
	panel->getChild<LLUICtrl>("terrain_raise_spin")->setValue(region_info.mTerrainRaiseLimit);
	panel->getChild<LLUICtrl>("terrain_lower_spin")->setValue(region_info.mTerrainLowerLimit);

	panel->setCtrlsEnabled(allow_modify);

	if (floater->getVisible())
	{
		// Note: region info also causes LLRegionInfoModel::instance().update(msg); -> requestRegion(); -> changed message
		// we need to know env version here and in update(msg) to know when to request and when not to, when to filter 'changed'
		floater->refreshFromRegion(gAgent.getRegion());
	} // else will rerequest on onOpen either way
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

	tab->getChild<LLPanel>("General")->setCtrlsEnabled(FALSE);
	tab->getChild<LLPanel>("Debug")->setCtrlsEnabled(FALSE);
	tab->getChild<LLPanel>("Terrain")->setCtrlsEnabled(FALSE);
	tab->getChild<LLPanel>("panel_env_info")->setCtrlsEnabled(FALSE);
	tab->getChild<LLPanel>("Estate")->setCtrlsEnabled(FALSE);
	tab->getChild<LLPanel>("Access")->setCtrlsEnabled(FALSE);
}

void LLFloaterRegionInfo::onTabSelected(const LLSD& param)
{
	LLPanel* active_panel = getChild<LLPanel>(param.asString());
	active_panel->onOpen(LLSD());
}

void LLFloaterRegionInfo::refreshFromRegion(LLViewerRegion* region)
{
	if (!region)
	{
		return; 
	}

	// call refresh from region on all panels
	for (const auto& infoPanel : mInfoPanels)
	{
		infoPanel->refreshFromRegion(region);
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
		refreshFromRegion(gAgent.getRegion());
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
BOOL LLPanelRegionInfo::postBuild()
{
	// If the panel has an Apply button, set a callback for it.
	LLUICtrl* apply_btn = findChild<LLUICtrl>("apply_btn");
	if (apply_btn)
	{
		apply_btn->setCommitCallback(boost::bind(&LLPanelRegionInfo::onBtnSet, this));
	}

	refresh();
	return TRUE;
}

// virtual 
void LLPanelRegionInfo::updateChild(LLUICtrl* child_ctr)
{
}

// virtual
bool LLPanelRegionInfo::refreshFromRegion(LLViewerRegion* region)
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
		strings_t::const_iterator it = strings.begin();
		strings_t::const_iterator end = strings.end();
		for(; it != end; ++it)
		{
			msg->nextBlock("ParamList");
			msg->addString("Parameter", *it);
		}
	}
	msg->sendReliable(mHost);
}

void LLPanelRegionInfo::enableButton(const std::string& btn_name, BOOL enable)
{
	LLView* button = findChildView(btn_name);
	if (button) button->setEnabled(enable);
}

void LLPanelRegionInfo::disableButton(const std::string& btn_name)
{
	LLView* button = findChildView(btn_name);
	if (button) button->setEnabled(FALSE);
}

void LLPanelRegionInfo::initCtrl(const std::string& name)
{
	getChild<LLUICtrl>(name)->setCommitCallback(boost::bind(&LLPanelRegionInfo::onChangeAnything, this));
}

void LLPanelRegionInfo::onClickManageTelehub()
{
	LLFloaterReg::hideInstance("region_info");
	LLFloaterReg::showInstance("telehubs");
}

/////////////////////////////////////////////////////////////////////////////
// LLPanelRegionGeneralInfo
//
bool LLPanelRegionGeneralInfo::refreshFromRegion(LLViewerRegion* region)
{
	BOOL allow_modify = gAgent.isGodlike() || (region && region->canManageEstate());
	setCtrlsEnabled(allow_modify);
	getChildView("apply_btn")->setEnabled(FALSE);
	getChildView("access_text")->setEnabled(allow_modify);
	// getChildView("access_combo")->setEnabled(allow_modify);
	// now set in processRegionInfo for teen grid detection
	getChildView("kick_btn")->setEnabled(allow_modify);
	getChildView("kick_all_btn")->setEnabled(allow_modify);
	getChildView("im_btn")->setEnabled(allow_modify);
	getChildView("manage_telehub_btn")->setEnabled(allow_modify);

	// Data gets filled in by processRegionInfo

	return LLPanelRegionInfo::refreshFromRegion(region);
}

BOOL LLPanelRegionGeneralInfo::postBuild()
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
//	childSetAction("manage_telehub_btn", onClickManageTelehub, this);

	LLUICtrl* apply_btn = findChild<LLUICtrl>("apply_btn");
	if (apply_btn)
	{
		apply_btn->setCommitCallback(boost::bind(&LLPanelRegionGeneralInfo::onBtnSet, this));
	}

	refresh();
	return TRUE;
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
                                                                                FALSE, TRUE, FALSE, parent_floater->getName(), button);
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
BOOL LLPanelRegionGeneralInfo::sendUpdate()
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

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// LLPanelRegionDebugInfo
/////////////////////////////////////////////////////////////////////////////
BOOL LLPanelRegionDebugInfo::postBuild()
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

	return TRUE;
}

// virtual
bool LLPanelRegionDebugInfo::refreshFromRegion(LLViewerRegion* region)
{
	BOOL allow_modify = gAgent.isGodlike() || (region && region->canManageEstate());
	setCtrlsEnabled(allow_modify);
	getChildView("apply_btn")->setEnabled(FALSE);
	getChildView("target_avatar_name")->setEnabled(FALSE);
	
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

	return LLPanelRegionInfo::refreshFromRegion(region);
}

// virtual
BOOL LLPanelRegionDebugInfo::sendUpdate()
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
	return TRUE;
}

void LLPanelRegionDebugInfo::onClickChooseAvatar()
{
    LLView * button = findChild<LLButton>("choose_avatar_btn");
    LLFloater* parent_floater = gFloaterView->getParentFloater(this);
	LLFloater * child_floater = LLFloaterAvatarPicker::show(boost::bind(&LLPanelRegionDebugInfo::callbackAvatarID, this, _1, _2), 
                                                                                    FALSE, TRUE, FALSE, parent_floater->getName(), button);
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
	refreshFromRegion( gAgent.getRegion() );
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
	strings.push_back("1");	// one physics step
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
	strings.push_back("6");	// top 5 scripts
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

BOOL LLPanelRegionTerrainInfo::validateTextureSizes()
{
    static const S32 MAX_TERRAIN_TEXTURE_SIZE = 1024;
	for(S32 i = 0; i < TERRAIN_TEXTURE_COUNT; ++i)
	{
		std::string buffer;
		buffer = llformat("texture_detail_%d", i);
		LLTextureCtrl* texture_ctrl = getChild<LLTextureCtrl>(buffer);
		if (!texture_ctrl) continue;

		LLUUID image_asset_id = texture_ctrl->getImageAssetID();
		LLViewerTexture* img = LLViewerTextureManager::getFetchedTexture(image_asset_id);
		S32 components = img->getComponents();
		// Must ask for highest resolution version's width. JC
		S32 width = img->getFullWidth();
		S32 height = img->getFullHeight();

		//LL_INFOS() << "texture detail " << i << " is " << width << "x" << height << "x" << components << LL_ENDL;

		if (components != 3)
		{
			LLSD args;
			args["TEXTURE_NUM"] = i+1;
			args["TEXTURE_BIT_DEPTH"] = llformat("%d",components * 8);
            args["MAX_SIZE"] = MAX_TERRAIN_TEXTURE_SIZE;
			LLNotificationsUtil::add("InvalidTerrainBitDepth", args);
			return FALSE;
		}

		if (width > MAX_TERRAIN_TEXTURE_SIZE || height > MAX_TERRAIN_TEXTURE_SIZE)
		{

			LLSD args;
			args["TEXTURE_NUM"] = i+1;
			args["TEXTURE_SIZE_X"] = width;
			args["TEXTURE_SIZE_Y"] = height;
            args["MAX_SIZE"] = MAX_TERRAIN_TEXTURE_SIZE;
			LLNotificationsUtil::add("InvalidTerrainSize", args);
			return FALSE;
			
		}
	}

	return TRUE;
}

BOOL LLPanelRegionTerrainInfo::validateTextureHeights()
{
	for (S32 i = 0; i < CORNER_COUNT; ++i)
	{
		std::string low = llformat("height_start_spin_%d", i);
		std::string high = llformat("height_range_spin_%d", i);

		if (getChild<LLUICtrl>(low)->getValue().asReal() > getChild<LLUICtrl>(high)->getValue().asReal())
		{
			return FALSE;
		}
	}

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// LLPanelRegionTerrainInfo
/////////////////////////////////////////////////////////////////////////////
// Initialize statics

BOOL LLPanelRegionTerrainInfo::postBuild()
{
	LLPanelRegionInfo::postBuild();
	
	initCtrl("water_height_spin");
	initCtrl("terrain_raise_spin");
	initCtrl("terrain_lower_spin");

	std::string buffer;
	for(S32 i = 0; i < TERRAIN_TEXTURE_COUNT; ++i)
	{
		buffer = llformat("texture_detail_%d", i);
		initCtrl(buffer);
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

// virtual
bool LLPanelRegionTerrainInfo::refreshFromRegion(LLViewerRegion* region)
{
	BOOL owner_or_god = gAgent.isGodlike() 
						|| (region && (region->getOwner() == gAgent.getID()));
	BOOL owner_or_god_or_manager = owner_or_god
						|| (region && region->isEstateManager());
	setCtrlsEnabled(owner_or_god_or_manager);

	getChildView("apply_btn")->setEnabled(FALSE);

	if (region)
	{
		getChild<LLUICtrl>("region_text")->setValue(LLSD(region->getName()));

		LLVLComposition* compp = region->getComposition();
		LLTextureCtrl* texture_ctrl;
		std::string buffer;
		for(S32 i = 0; i < TERRAIN_TEXTURE_COUNT; ++i)
		{
			buffer = llformat("texture_detail_%d", i);
			texture_ctrl = getChild<LLTextureCtrl>(buffer);
			if(texture_ctrl)
			{
				LL_DEBUGS() << "Detail Texture " << i << ": "
						 << compp->getDetailTextureID(i) << LL_ENDL;
				LLUUID tmp_id(compp->getDetailTextureID(i));
				texture_ctrl->setImageAssetID(tmp_id);
			}
		}

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

	getChildView("download_raw_btn")->setEnabled(owner_or_god);
	getChildView("upload_raw_btn")->setEnabled(owner_or_god);
	getChildView("bake_terrain_btn")->setEnabled(owner_or_god);

	return LLPanelRegionInfo::refreshFromRegion(region);
}


// virtual
BOOL LLPanelRegionTerrainInfo::sendUpdate()
{
	LL_INFOS() << "LLPanelRegionTerrainInfo::sendUpdate" << LL_ENDL;
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

	// Make sure user hasn't chosen wacky textures.
	if (!validateTextureSizes())
	{
		return FALSE;
	}

	// Check if terrain Elevation Ranges are correct
	if (gSavedSettings.getBOOL("RegionCheckTextureHeights") && !validateTextureHeights())
	{
		if (!mAskedTextureHeights)
		{
			LLNotificationsUtil::add("ConfirmTextureHeights", LLSD(), LLSD(), boost::bind(&LLPanelRegionTerrainInfo::callbackTextureHeights, this, _1, _2));
			mAskedTextureHeights = true;
			return FALSE;
		}
		else if (!mConfirmedTextureHeights)
		{
			return FALSE;
		}
	}

	LLTextureCtrl* texture_ctrl;
	std::string id_str;
	LLMessageSystem* msg = gMessageSystem;

	for(S32 i = 0; i < TERRAIN_TEXTURE_COUNT; ++i)
	{
		buffer = llformat("texture_detail_%d", i);
		texture_ctrl = getChild<LLTextureCtrl>(buffer);
		if(texture_ctrl)
		{
			LLUUID tmp_id(texture_ctrl->getImageAssetID());
			tmp_id.toString(id_str);
			buffer = llformat("%d %s", i, id_str.c_str());
			strings.push_back(buffer);
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

	return TRUE;
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
		gSavedSettings.setBOOL("RegionCheckTextureHeights", FALSE);
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
:	LLPanelRegionInfo(),
	mEstateID(0)	// invalid
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
                                                                        FALSE, TRUE, FALSE, parent_floater->getName(), button);
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

	U32 mOperationFlag;	// ESTATE_ACCESS_BANNED_AGENT_ADD, _REMOVE, etc.
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
	BOOL god = gAgent.isGodlike();
	BOOL owner = (region && (region->getOwner() == gAgent.getID()));
	BOOL manager = (region && region->isEstateManager());
	setCtrlsEnabled(god || owner || manager);
	
	getChildView("apply_btn")->setEnabled(FALSE);
    getChildView("estate_owner")->setEnabled(TRUE);
	getChildView("message_estate_btn")->setEnabled(god || owner || manager);
	getChildView("kick_user_from_estate_btn")->setEnabled(god || owner || manager);

	refresh();
}

bool LLPanelEstateInfo::refreshFromRegion(LLViewerRegion* region)
{
	updateControls(region);
	
	// let the parent class handle the general data collection. 
	bool rv = LLPanelRegionInfo::refreshFromRegion(region);

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


BOOL LLPanelEstateInfo::postBuild()
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

	getChild<LLUICtrl>("externally_visible_radio")->setFocus(TRUE);

    getChild<LLTextBox>("estate_owner")->setIsFriendCallback(LLAvatarActions::isFriend);

	return LLPanelRegionInfo::postBuild();
}

void LLPanelEstateInfo::refresh()
{
	// Disable access restriction controls if they make no sense.
	bool public_access = ("estate_public_access" == getChild<LLUICtrl>("externally_visible_radio")->getValue().asString());

	getChildView("Only Allow")->setEnabled(public_access);
	getChildView("limit_payment")->setEnabled(public_access);
	getChildView("limit_age_verified")->setEnabled(public_access);
    getChildView("limit_bots")->setEnabled(public_access);

	// if this is set to false, then the limit fields are meaningless and should be turned off
	if (public_access == false)
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

BOOL LLPanelEstateInfo::sendUpdate()
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
	return TRUE;
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
//		else
//		{
//			// caps method does not automatically send this info
//			LLFloaterRegionInfo::requestRegionInfo();
//		}
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
bool LLPanelEstateCovenant::refreshFromRegion(LLViewerRegion* region)
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
	bool rv = LLPanelRegionInfo::refreshFromRegion(region);
	LLMessageSystem *msg = gMessageSystem;
	msg->newMessage("EstateCovenantRequest");
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID,	gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID,gAgent.getSessionID());
	msg->sendReliable(region->getHost());
	return rv;
}

// virtual 
bool LLPanelEstateCovenant::estateUpdate(LLMessageSystem* msg)
{
	LL_INFOS() << "LLPanelEstateCovenant::estateUpdate()" << LL_ENDL;
	return true;
}
	
// virtual 
BOOL LLPanelEstateCovenant::postBuild()
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
BOOL LLPanelEstateCovenant::handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
								  EDragAndDropType cargo_type,
								  void* cargo_data,
								  EAcceptance* accept,
								  std::string& tooltip_msg)
{
	LLInventoryItem* item = (LLInventoryItem*)cargo_data;

	if (!gAgent.canManageEstate())
	{
		*accept = ACCEPT_NO;
		return TRUE;
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

	return TRUE;
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
	const BOOL high_priority = TRUE;	
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
BOOL LLPanelEstateCovenant::sendUpdate()
{
	return TRUE;
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

LLSD LLDispatchSetEstateExperience::getIDs( sparam_t::const_iterator it, sparam_t::const_iterator end, S32 count )
{
	LLSD idList = LLSD::emptyArray();
	LLUUID id;
	while(count--> 0)
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
// strings[8] = bin(uuid) ...
// ...
bool LLDispatchSetEstateExperience::operator()(
	const LLDispatcher* dispatcher,
	const std::string& key,
	const LLUUID& invoice,
	const sparam_t& strings)
{
	LLPanelRegionExperiences* panel = LLFloaterRegionInfo::getPanelExperiences();
	if (!panel) return true;

	sparam_t::const_iterator it = strings.begin();
	++it; // U32 estate_id = strtol((*it).c_str(), NULL, 10);
	++it; // U32 send_to_agent_only = strtoul((*(++it)).c_str(), NULL, 10);

	LLUUID id;
	S32 num_blocked = strtol((*(it++)).c_str(), NULL, 10);
	S32 num_trusted = strtol((*(it++)).c_str(), NULL, 10);
	S32 num_allowed = strtol((*(it++)).c_str(), NULL, 10);

	LLSD ids = LLSD::emptyMap()
		.with("blocked", getIDs(it,								strings.end(), num_blocked))
		.with("trusted", getIDs(it + (num_blocked),				strings.end(), num_trusted))
		.with("allowed", getIDs(it + (num_blocked+num_trusted),	strings.end(), num_allowed));

	panel->processResponse(ids);			

	return true;
}

BOOL LLPanelRegionExperiences::postBuild()
{
	mAllowed = setupList("panel_allowed", ESTATE_EXPERIENCE_ALLOWED_ADD, ESTATE_EXPERIENCE_ALLOWED_REMOVE);
	mTrusted = setupList("panel_trusted", ESTATE_EXPERIENCE_TRUSTED_ADD, ESTATE_EXPERIENCE_TRUSTED_REMOVE);
	mBlocked = setupList("panel_blocked", ESTATE_EXPERIENCE_BLOCKED_ADD, ESTATE_EXPERIENCE_BLOCKED_REMOVE);

	getChild<LLLayoutPanel>("trusted_layout_panel")->setVisible(TRUE);
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

bool LLPanelRegionExperiences::refreshFromRegion(LLViewerRegion* region)
{
	BOOL allow_modify = gAgent.isGodlike() || (region && region->canManageEstate());

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

    LLExperienceCache::instance().getRegionExperiences(boost::bind(&LLPanelRegionExperiences::regionCapabilityQuery, region, _1),
        boost::bind(&LLPanelRegionExperiences::infoCallback, getDerivedHandle<LLPanelRegionExperiences>(), _1));

    return LLPanelRegionInfo::refreshFromRegion(region);
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


BOOL LLPanelRegionExperiences::sendUpdate()
{
	LLViewerRegion* region = gAgent.getRegion();

    LLSD content;

	content["allowed"]=addIds(mAllowed);
	content["blocked"]=addIds(mBlocked);
	content["trusted"]=addIds(mTrusted);

    LLExperienceCache::instance().setRegionExperiences(boost::bind(&LLPanelRegionExperiences::regionCapabilityQuery, region, _1),
        content, boost::bind(&LLPanelRegionExperiences::infoCallback, getDerivedHandle<LLPanelRegionExperiences>(), _1));

	return TRUE;
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

BOOL LLPanelEstateAccess::postBuild()
{
	getChild<LLUICtrl>("allowed_avatar_name_list")->setCommitCallback(boost::bind(&LLPanelEstateInfo::onChangeChildCtrl, this, _1));
	LLNameListCtrl *avatar_name_list = getChild<LLNameListCtrl>("allowed_avatar_name_list");
	if (avatar_name_list)
	{
		avatar_name_list->setCommitOnSelectionChange(TRUE); 
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
		group_name_list->setCommitOnSelectionChange(TRUE);
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
		banned_name_list->setCommitOnSelectionChange(TRUE);
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
		manager_name_list->setCommitOnSelectionChange(TRUE);
		manager_name_list->setMaxItemCount(ESTATE_MAX_MANAGERS * 4);	// Allow extras for dupe issue
	}

	childSetAction("add_estate_manager_btn", boost::bind(&LLPanelEstateAccess::onClickAddEstateManager, this));
	childSetAction("remove_estate_manager_btn", boost::bind(&LLPanelEstateAccess::onClickRemoveEstateManager, this));

	return TRUE;
}

void LLPanelEstateAccess::updateControls(LLViewerRegion* region)
{
	BOOL god = gAgent.isGodlike();
	BOOL owner = (region && (region->getOwner() == gAgent.getID()));
	BOOL manager = (region && region->isEstateManager());
	BOOL enable_cotrols = god || owner || manager;	
	setCtrlsEnabled(enable_cotrols);
	
	BOOL has_allowed_avatar = getChild<LLNameListCtrl>("allowed_avatar_name_list")->getFirstSelected() ? TRUE : FALSE;
	BOOL has_allowed_group = getChild<LLNameListCtrl>("allowed_group_name_list")->getFirstSelected() ? TRUE : FALSE;
	BOOL has_banned_agent = getChild<LLNameListCtrl>("banned_avatar_name_list")->getFirstSelected() ? TRUE : FALSE;
	BOOL has_estate_manager = getChild<LLNameListCtrl>("estate_manager_name_list")->getFirstSelected() ? TRUE : FALSE;

	getChildView("add_allowed_avatar_btn")->setEnabled(enable_cotrols);
	getChildView("remove_allowed_avatar_btn")->setEnabled(has_allowed_avatar && enable_cotrols);
	getChildView("allowed_avatar_name_list")->setEnabled(enable_cotrols);

	getChildView("add_allowed_group_btn")->setEnabled(enable_cotrols);
	getChildView("remove_allowed_group_btn")->setEnabled(has_allowed_group && enable_cotrols);
	getChildView("allowed_group_name_list")->setEnabled(enable_cotrols);

	// Can't ban people from mainland, orientation islands, etc. because this
	// creates much network traffic and server load.
	// Disable their accounts in CSR tool instead.
	bool linden_estate = LLPanelEstateInfo::isLindenEstate();
	bool enable_ban = enable_cotrols && !linden_estate;
	getChildView("add_banned_avatar_btn")->setEnabled(enable_ban);
	getChildView("remove_banned_avatar_btn")->setEnabled(has_banned_agent && enable_ban);
	getChildView("banned_avatar_name_list")->setEnabled(enable_cotrols);

	// estate managers can't add estate managers
	getChildView("add_estate_manager_btn")->setEnabled(god || owner);
	getChildView("remove_estate_manager_btn")->setEnabled(has_estate_manager && (god || owner));
	getChildView("estate_manager_name_list")->setEnabled(god || owner);

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
	{	// Tell user they can't add more managers
		LLSD args;
		args["MAX_MANAGER"] = llformat("%d", ESTATE_MAX_MANAGERS);
		LLNotificationsUtil::add("MaxManagersOnRegion", args);
	}
	else
	{	// Go pick managers to add
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
		TRUE, TRUE, FALSE, parent_floater_name, button);

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
	LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t	httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("requestEstateGetAccessoCoro", httpPolicy));
	LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);

	LLSD result = httpAdapter->getAndSuspend(httpRequest, url);

	LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
	LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

	LLPanelEstateAccess* panel = LLFloaterRegionInfo::getPanelAccess();
	if (!panel) return;
	
	LLNameListCtrl* allowed_agent_name_list	= panel->getChild<LLNameListCtrl>("allowed_avatar_name_list");
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
		allowed_agent_name_list->sortByName(TRUE);
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
		banned_agent_name_list->sortByName(TRUE);
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
		allowed_group_name_list->sortByName(TRUE);
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
		estate_manager_name_list->sortByName(TRUE);
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
		listCtrl->deselectAllItems(TRUE);
	}
}

void LLPanelEstateAccess::copyListToClipboard(std::string list_name)
{
	LLPanelEstateAccess* panel = LLFloaterRegionInfo::getPanelAccess();
	if (!panel) return;
	LLNameListCtrl* name_list = panel->getChild<LLNameListCtrl>(list_name);
	if (!name_list) return;

	std::vector<LLScrollListItem*> list_vector = name_list->getAllData();
	if (list_vector.size() == 0) return;

	LLSD::String list_to_copy;
	for (std::vector<LLScrollListItem*>::const_iterator iter = list_vector.begin();
		 iter != list_vector.end();
		 iter++)
	{
		LLScrollListItem *item = (*iter);
		if (item)
		{
			list_to_copy += item->getColumn(0)->getValue().asString();
		}
		if (std::next(iter) != list_vector.end())
		{
			list_to_copy += "\n";
		}
	}

	LLClipboard::instance().copyToClipboard(utf8str_to_wstring(list_to_copy), 0, list_to_copy.length());
}

bool LLPanelEstateAccess::refreshFromRegion(LLViewerRegion* region)
{
	updateLists();
	return LLPanelRegionInfo::refreshFromRegion(region);
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

BOOL LLPanelRegionEnvironment::postBuild()
{
    LLEstateInfoModel& estate_info = LLEstateInfoModel::instance();

    if (!LLPanelEnvironmentInfo::postBuild())
        return FALSE;

    getChild<LLUICtrl>(BTN_USEDEFAULT)->setLabelArg("[USEDEFAULT]", getString(STR_LABEL_USEDEFAULT));
    getChild<LLUICtrl>(CHK_ALLOWOVERRIDE)->setVisible(TRUE);
    getChild<LLUICtrl>(PNL_ENVIRONMENT_ALTITUDES)->setVisible(TRUE);

    getChild<LLUICtrl>(CHK_ALLOWOVERRIDE)->setCommitCallback([this](LLUICtrl *, const LLSD &value){ onChkAllowOverride(value.asBoolean()); });

    mCommitConnect = estate_info.setCommitCallback(boost::bind(&LLPanelRegionEnvironment::refreshFromEstate, this));
    return TRUE;
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

    getChild<LLUICtrl>(CHK_ALLOWOVERRIDE)->setValue(mAllowOverride);
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
        [that_h](S32 parcel_id, LLEnvironment::EnvironmentInfo::ptr_t envifo) { _onEnvironmentReceived(that_h, parcel_id, envifo); });

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
        getChild<LLUICtrl>(CHK_ALLOWOVERRIDE)->setValue(mAllowOverride);
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
