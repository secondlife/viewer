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
#include "llvfile.h"
#include "llxfermanager.h"
#include "indra_constants.h"
#include "message.h"

#include "llagent.h"
#include "llappviewer.h"
#include "llavatarname.h"
#include "llfloateravatarpicker.h"
#include "llbutton.h" 
#include "llcheckboxctrl.h"
#include "llcombobox.h"
#include "llfilepicker.h"
#include "llfloatergodtools.h"	// for send_sim_wide_deletes()
#include "llfloatertopobjects.h" // added to fix SL-32336
#include "llfloatergroups.h"
#include "llfloaterreg.h"
#include "llfloaterregiondebugconsole.h"
#include "llfloatertelehub.h"
#include "llfloaterwindlight.h"
#include "llinventorymodel.h"
#include "lllineeditor.h"
#include "llnamelistctrl.h"
#include "llnotifications.h"
#include "llnotificationsutil.h"
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

const S32 TERRAIN_TEXTURE_COUNT = 4;
const S32 CORNER_COUNT = 4;

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



bool estate_dispatch_initialized = false;


///----------------------------------------------------------------------------
/// LLFloaterRegionInfo
///----------------------------------------------------------------------------

//S32 LLFloaterRegionInfo::sRequestSerial = 0;
LLUUID LLFloaterRegionInfo::sRequestInvoice;


void LLFloaterRegionInfo::onConsoleReplyReceived(const std::string& output)
{
	llwarns << "here is what they're giving us:  " << output << llendl;

	if (output.find("FALSE") != std::string::npos)
	{
		getChild<LLUICtrl>("mesh_rez_enabled_check")->setValue(FALSE);
	}
	else
	{
		getChild<LLUICtrl>("mesh_rez_enabled_check")->setValue(TRUE);
	}
}


LLFloaterRegionInfo::LLFloaterRegionInfo(const LLSD& seed)
	: LLFloater(seed)
{
	mConsoleReplySignalConnection = LLFloaterRegionDebugConsole::setConsoleReplyCallback(
	boost::bind(
		&LLFloaterRegionInfo::onConsoleReplyReceived,
		this,
		_1));
}

BOOL LLFloaterRegionInfo::postBuild()
{
	mTab = getChild<LLTabContainer>("region_panels");

	// contruct the panels
	LLPanelRegionInfo* panel;
	panel = new LLPanelRegionGeneralInfo;
	mInfoPanels.push_back(panel);
	panel->getCommitCallbackRegistrar().add("RegionInfo.ManageTelehub",	boost::bind(&LLPanelRegionInfo::onClickManageTelehub, panel));
	
	panel->buildFromFile("panel_region_general.xml");
	mTab->addTabPanel(LLTabContainer::TabPanelParams().panel(panel).select_tab(true));

	panel = new LLPanelRegionDebugInfo;
	mInfoPanels.push_back(panel);
	panel->buildFromFile("panel_region_debug.xml");
	mTab->addTabPanel(panel);

	panel = new LLPanelRegionTextureInfo;
	mInfoPanels.push_back(panel);
	panel->buildFromFile("panel_region_texture.xml");
	mTab->addTabPanel(panel);

	panel = new LLPanelRegionTerrainInfo;
	mInfoPanels.push_back(panel);
	panel->buildFromFile("panel_region_terrain.xml");
	mTab->addTabPanel(panel);

	panel = new LLPanelEstateInfo;
	mInfoPanels.push_back(panel);
	panel->buildFromFile("panel_region_estate.xml");
	mTab->addTabPanel(panel);

	panel = new LLPanelEstateCovenant;
	mInfoPanels.push_back(panel);
	panel->buildFromFile("panel_region_covenant.xml");
	mTab->addTabPanel(panel);

	gMessageSystem->setHandlerFunc(
		"EstateOwnerMessage", 
		&processEstateOwnerRequest);

	return TRUE;
}

LLFloaterRegionInfo::~LLFloaterRegionInfo()
{
	mConsoleReplySignalConnection.disconnect();
}

void LLFloaterRegionInfo::onOpen(const LLSD& key)
{
	refreshFromRegion(gAgent.getRegion());
	requestRegionInfo();
	requestMeshRezInfo();
}

// static
void LLFloaterRegionInfo::requestRegionInfo()
{
	LLTabContainer* tab = getChild<LLTabContainer>("region_panels");

	tab->getChild<LLPanel>("General")->setCtrlsEnabled(FALSE);
	tab->getChild<LLPanel>("Debug")->setCtrlsEnabled(FALSE);
	tab->getChild<LLPanel>("Terrain")->setCtrlsEnabled(FALSE);
	tab->getChild<LLPanel>("Estate")->setCtrlsEnabled(FALSE);

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

	LLTabContainer* tab = floater->getChild<LLTabContainer>("region_panels");
	LLPanelEstateInfo* panel = (LLPanelEstateInfo*)tab->getChild<LLPanel>("Estate");

	// unpack the message
	std::string request;
	LLUUID invoice;
	LLDispatcher::sparam_t strings;
	LLDispatcher::unpackMessage(msg, request, invoice, strings);
	if(invoice != getLastInvoice())
	{
		llwarns << "Mismatched Estate message: " << request << llendl;
		return;
	}

	//dispatch the message
	dispatch.dispatch(request, invoice, strings);

	LLViewerRegion* region = gAgent.getRegion();
	panel->updateControls(region);
}


// static
void LLFloaterRegionInfo::processRegionInfo(LLMessageSystem* msg)
{
	LLPanel* panel;
	LLFloaterRegionInfo* floater = LLFloaterReg::getTypedInstance<LLFloaterRegionInfo>("region_info");
	llinfos << "LLFloaterRegionInfo::processRegionInfo" << llendl;
	if(!floater)
	{
		return;
	}
	
	LLTabContainer* tab = floater->getChild<LLTabContainer>("region_panels");

	LLViewerRegion* region = gAgent.getRegion();
	BOOL allow_modify = gAgent.isGodlike() || (region && region->canManageEstate());

	// extract message
	std::string sim_name;
	std::string sim_type = LLTrans::getString("land_type_unknown");
	U32 region_flags;
	U8 agent_limit;
	F32 object_bonus_factor;
	U8 sim_access;
	F32 water_height;
	F32 terrain_raise_limit;
	F32 terrain_lower_limit;
	BOOL use_estate_sun;
	F32 sun_hour;
	msg->getString("RegionInfo", "SimName", sim_name);
	msg->getU32("RegionInfo", "RegionFlags", region_flags);
	msg->getU8("RegionInfo", "MaxAgents", agent_limit);
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
	}

	// GENERAL PANEL
	panel = tab->getChild<LLPanel>("General");
	panel->getChild<LLUICtrl>("region_text")->setValue(LLSD(sim_name));
	panel->getChild<LLUICtrl>("region_type")->setValue(LLSD(sim_type));
	panel->getChild<LLUICtrl>("version_channel_text")->setValue(gLastVersionChannel);

	panel->getChild<LLUICtrl>("block_terraform_check")->setValue((region_flags & REGION_FLAGS_BLOCK_TERRAFORM) ? TRUE : FALSE );
	panel->getChild<LLUICtrl>("block_fly_check")->setValue((region_flags & REGION_FLAGS_BLOCK_FLY) ? TRUE : FALSE );
	panel->getChild<LLUICtrl>("allow_damage_check")->setValue((region_flags & REGION_FLAGS_ALLOW_DAMAGE) ? TRUE : FALSE );
	panel->getChild<LLUICtrl>("restrict_pushobject")->setValue((region_flags & REGION_FLAGS_RESTRICT_PUSHOBJECT) ? TRUE : FALSE );
	panel->getChild<LLUICtrl>("allow_land_resell_check")->setValue((region_flags & REGION_FLAGS_BLOCK_LAND_RESELL) ? FALSE : TRUE );
	panel->getChild<LLUICtrl>("allow_parcel_changes_check")->setValue((region_flags & REGION_FLAGS_ALLOW_PARCEL_CHANGES) ? TRUE : FALSE );
	panel->getChild<LLUICtrl>("block_parcel_search_check")->setValue((region_flags & REGION_FLAGS_BLOCK_PARCEL_SEARCH) ? TRUE : FALSE );
	panel->getChild<LLUICtrl>("agent_limit_spin")->setValue(LLSD((F32)agent_limit) );
	panel->getChild<LLUICtrl>("object_bonus_spin")->setValue(LLSD(object_bonus_factor) );
	panel->getChild<LLUICtrl>("access_combo")->setValue(LLSD(sim_access) );


 	// detect teen grid for maturity

	U32 parent_estate_id;
	msg->getU32("RegionInfo", "ParentEstateID", parent_estate_id);
	BOOL teen_grid = (parent_estate_id == 5);  // *TODO add field to estate table and test that
	panel->getChildView("access_combo")->setEnabled(gAgent.isGodlike() || (region && region->canManageEstate() && !teen_grid));
	panel->setCtrlsEnabled(allow_modify);
	

	// DEBUG PANEL
	panel = tab->getChild<LLPanel>("Debug");

	panel->getChild<LLUICtrl>("region_text")->setValue(LLSD(sim_name) );
	panel->getChild<LLUICtrl>("disable_scripts_check")->setValue(LLSD((BOOL)(region_flags & REGION_FLAGS_SKIP_SCRIPTS)) );
	panel->getChild<LLUICtrl>("disable_collisions_check")->setValue(LLSD((BOOL)(region_flags & REGION_FLAGS_SKIP_COLLISIONS)) );
	panel->getChild<LLUICtrl>("disable_physics_check")->setValue(LLSD((BOOL)(region_flags & REGION_FLAGS_SKIP_PHYSICS)) );
	panel->setCtrlsEnabled(allow_modify);

	// TERRAIN PANEL
	panel = tab->getChild<LLPanel>("Terrain");

	panel->getChild<LLUICtrl>("region_text")->setValue(LLSD(sim_name));
	panel->getChild<LLUICtrl>("water_height_spin")->setValue(LLSD(water_height));
	panel->getChild<LLUICtrl>("terrain_raise_spin")->setValue(LLSD(terrain_raise_limit));
	panel->getChild<LLUICtrl>("terrain_lower_spin")->setValue(LLSD(terrain_lower_limit));
	panel->getChild<LLUICtrl>("use_estate_sun_check")->setValue(LLSD(use_estate_sun));

	panel->getChild<LLUICtrl>("fixed_sun_check")->setValue(LLSD((BOOL)(region_flags & REGION_FLAGS_SUN_FIXED)));
	panel->getChildView("fixed_sun_check")->setEnabled(allow_modify && !use_estate_sun);
	panel->getChild<LLUICtrl>("sun_hour_slider")->setValue(LLSD(sun_hour));
	panel->getChildView("sun_hour_slider")->setEnabled(allow_modify && !use_estate_sun);
	panel->setCtrlsEnabled(allow_modify);

	floater->refreshFromRegion( gAgent.getRegion() );
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
LLPanelEstateCovenant* LLFloaterRegionInfo::getPanelCovenant()
{
	LLFloaterRegionInfo* floater = LLFloaterReg::getTypedInstance<LLFloaterRegionInfo>("region_info");
	if (!floater) return NULL;
	LLTabContainer* tab = floater->getChild<LLTabContainer>("region_panels");
	LLPanelEstateCovenant* panel = (LLPanelEstateCovenant*)tab->getChild<LLPanel>("Covenant");
	return panel;
}

void LLFloaterRegionInfo::refreshFromRegion(LLViewerRegion* region)
{
	if (!region)
	{
		return; 
	}

	// call refresh from region on all panels
	std::for_each(
		mInfoPanels.begin(),
		mInfoPanels.end(),
		llbind2nd(
#if LL_WINDOWS
			std::mem_fun1(&LLPanelRegionInfo::refreshFromRegion),
#else
			std::mem_fun(&LLPanelRegionInfo::refreshFromRegion),
#endif
			region));
}

// public
void LLFloaterRegionInfo::refresh()
{
	for(info_panels_t::iterator iter = mInfoPanels.begin();
		iter != mInfoPanels.end(); ++iter)
	{
		(*iter)->refresh();
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
	getChild<LLUICtrl>("apply_btn")->setCommitCallback(boost::bind(&LLPanelRegionInfo::onBtnSet, this));
	getChildView("apply_btn")->setEnabled(FALSE);
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
	llinfos << "Sending estate request '" << request << "'" << llendl;
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
	getChildView(btn_name)->setEnabled(enable);
}

void LLPanelRegionInfo::disableButton(const std::string& btn_name)
{
	getChildView(btn_name)->setEnabled(FALSE);
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

	const bool enable_mesh = gSavedSettings.getBOOL("MeshEnabled") && 
		gAgent.getRegion() &&
		!gAgent.getRegion()->getCapability("GetMesh").empty();
	getChildView("mesh_rez_enabled_check")->setVisible(enable_mesh);
	getChildView("mesh_rez_enabled_check")->setEnabled(getChildView("mesh_rez_enabled_check")->getEnabled() && enable_mesh);
	// Data gets filled in by processRegionInfo

	return LLPanelRegionInfo::refreshFromRegion(region);
}

BOOL LLPanelRegionGeneralInfo::postBuild()
{
	// Enable the "Apply" button if something is changed. JC
	initCtrl("block_terraform_check");
	initCtrl("block_fly_check");
	initCtrl("allow_damage_check");
	initCtrl("allow_land_resell_check");
	initCtrl("allow_parcel_changes_check");
	initCtrl("agent_limit_spin");
	initCtrl("object_bonus_spin");
	initCtrl("access_combo");
	initCtrl("restrict_pushobject");
	initCtrl("block_parcel_search_check");
	initCtrl("mesh_rez_enabled_check");

	childSetAction("kick_btn", boost::bind(&LLPanelRegionGeneralInfo::onClickKick, this));
	childSetAction("kick_all_btn", onClickKickAll, this);
	childSetAction("im_btn", onClickMessage, this);
//	childSetAction("manage_telehub_btn", onClickManageTelehub, this);

	return LLPanelRegionInfo::postBuild();
}

void LLPanelRegionGeneralInfo::onClickKick()
{
	llinfos << "LLPanelRegionGeneralInfo::onClickKick" << llendl;

	// this depends on the grandparent view being a floater
	// in order to set up floater dependency
	LLFloater* parent_floater = gFloaterView->getParentFloater(this);
	LLFloater* child_floater = LLFloaterAvatarPicker::show(boost::bind(&LLPanelRegionGeneralInfo::onKickCommit, this, _1), FALSE, TRUE);
	parent_floater->addDependentFloater(child_floater);
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
	llinfos << "LLPanelRegionGeneralInfo::onClickKickAll" << llendl;
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
	llinfos << "LLPanelRegionGeneralInfo::onClickMessage" << llendl;
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

	llinfos << "Message to everyone: " << text << llendl;
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

class ConsoleRequestResponder : public LLHTTPClient::Responder
{
public:
	/*virtual*/
	void error(U32 status, const std::string& reason)
	{
		llwarns << "requesting mesh_rez_enabled failed" << llendl;
	}
};


// called if this request times out.
class ConsoleUpdateResponder : public LLHTTPClient::Responder
{
public:
	/* virtual */
	void error(U32 status, const std::string& reason)
	{
		llwarns << "Updating mesh enabled region setting failed" << llendl;
	}
};

void LLFloaterRegionInfo::requestMeshRezInfo()
{
	std::string sim_console_url = gAgent.getRegion()->getCapability("SimConsoleAsync");

	if (!sim_console_url.empty())
	{
		std::string request_str = "get mesh_rez_enabled";
		
		LLHTTPClient::post(
			sim_console_url,
			LLSD(request_str),
			new ConsoleRequestResponder);
	}
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
	llinfos << "LLPanelRegionGeneralInfo::sendUpdate()" << llendl;

	// First try using a Cap.  If that fails use the old method.
	LLSD body;
	std::string url = gAgent.getRegion()->getCapability("DispatchRegionInfo");
	if (!url.empty())
	{
		body["block_terraform"] = getChild<LLUICtrl>("block_terraform_check")->getValue();
		body["block_fly"] = getChild<LLUICtrl>("block_fly_check")->getValue();
		body["allow_damage"] = getChild<LLUICtrl>("allow_damage_check")->getValue();
		body["allow_land_resell"] = getChild<LLUICtrl>("allow_land_resell_check")->getValue();
		body["agent_limit"] = getChild<LLUICtrl>("agent_limit_spin")->getValue();
		body["prim_bonus"] = getChild<LLUICtrl>("object_bonus_spin")->getValue();
		body["sim_access"] = getChild<LLUICtrl>("access_combo")->getValue();
		body["restrict_pushobject"] = getChild<LLUICtrl>("restrict_pushobject")->getValue();
		body["allow_parcel_changes"] = getChild<LLUICtrl>("allow_parcel_changes_check")->getValue();
		body["block_parcel_search"] = getChild<LLUICtrl>("block_parcel_search_check")->getValue();

		LLHTTPClient::post(url, body, new LLHTTPClient::Responder());
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

	std::string sim_console_url = gAgent.getRegion()->getCapability("SimConsoleAsync");

	if (!sim_console_url.empty())
	{
		std::string update_str = "set mesh_rez_enabled ";
		if (getChild<LLUICtrl>("mesh_rez_enabled_check")->getValue().asBoolean())
		{
			update_str += "true";
		}
		else
		{
			update_str += "false";
		}

		LLHTTPClient::post(
			sim_console_url,
			LLSD(update_str),
			new ConsoleUpdateResponder);
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

	return LLPanelRegionInfo::refreshFromRegion(region);
}

// virtual
BOOL LLPanelRegionDebugInfo::sendUpdate()
{
	llinfos << "LLPanelRegionDebugInfo::sendUpdate" << llendl;
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
	LLFloaterAvatarPicker::show(boost::bind(&LLPanelRegionDebugInfo::callbackAvatarID, this, _1, _2), FALSE, TRUE);
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


/////////////////////////////////////////////////////////////////////////////
// LLPanelRegionTextureInfo
//
LLPanelRegionTextureInfo::LLPanelRegionTextureInfo() : LLPanelRegionInfo()
{
	// nothing.
}

bool LLPanelRegionTextureInfo::refreshFromRegion(LLViewerRegion* region)
{
	BOOL allow_modify = gAgent.isGodlike() || (region && region->canManageEstate());
	setCtrlsEnabled(allow_modify);
	getChildView("apply_btn")->setEnabled(FALSE);

	if (region)
	{
		getChild<LLUICtrl>("region_text")->setValue(LLSD(region->getName()));
	}
	else
	{
		getChild<LLUICtrl>("region_text")->setValue(LLSD(""));
	}

	if (!region) return LLPanelRegionInfo::refreshFromRegion(region);

	LLVLComposition* compp = region->getComposition();
	LLTextureCtrl* texture_ctrl;
	std::string buffer;
	for(S32 i = 0; i < TERRAIN_TEXTURE_COUNT; ++i)
	{
		buffer = llformat("texture_detail_%d", i);
		texture_ctrl = getChild<LLTextureCtrl>(buffer);
		if(texture_ctrl)
		{
			lldebugs << "Detail Texture " << i << ": "
					 << compp->getDetailTextureID(i) << llendl;
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

	// Call the parent for common book-keeping
	return LLPanelRegionInfo::refreshFromRegion(region);
}


BOOL LLPanelRegionTextureInfo::postBuild()
{
	LLPanelRegionInfo::postBuild();
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

//	LLButton* btn = ("dump", LLRect(0, 20, 100, 0), "", onClickDump, this);
//	btn->setFollows(FOLLOWS_TOP|FOLLOWS_LEFT);
//	addChild(btn);

	return LLPanelRegionInfo::postBuild();
}

BOOL LLPanelRegionTextureInfo::sendUpdate()
{
	llinfos << "LLPanelRegionTextureInfo::sendUpdate()" << llendl;

	// Make sure user hasn't chosen wacky textures.
	if (!validateTextureSizes())
	{
		return FALSE;
	}

	LLTextureCtrl* texture_ctrl;
	std::string buffer;
	std::string id_str;
	LLMessageSystem* msg = gMessageSystem;
	strings_t strings;

	LLUUID invoice(LLFloaterRegionInfo::getLastInvoice());
	
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
	for(S32 i = 0; i < CORNER_COUNT; ++i)
	{
		buffer = llformat("height_start_spin_%d", i);
		std::string buffer2 = llformat("height_range_spin_%d", i);
		std::string buffer3 = llformat("%d %f %f", i, (F32)getChild<LLUICtrl>(buffer)->getValue().asReal(), (F32)getChild<LLUICtrl>(buffer2)->getValue().asReal());
		strings.push_back(buffer3);
	}
	sendEstateOwnerMessage(msg, "textureheights", invoice, strings);
	strings.clear();
	sendEstateOwnerMessage(msg, "texturecommit", invoice, strings);
	return TRUE;
}

BOOL LLPanelRegionTextureInfo::validateTextureSizes()
{
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

		//llinfos << "texture detail " << i << " is " << width << "x" << height << "x" << components << llendl;

		if (components != 3)
		{
			LLSD args;
			args["TEXTURE_NUM"] = i+1;
			args["TEXTURE_BIT_DEPTH"] = llformat("%d",components * 8);
			LLNotificationsUtil::add("InvalidTerrainBitDepth", args);
			return FALSE;
		}

		if (width > 512 || height > 512)
		{

			LLSD args;
			args["TEXTURE_NUM"] = i+1;
			args["TEXTURE_SIZE_X"] = width;
			args["TEXTURE_SIZE_Y"] = height;
			LLNotificationsUtil::add("InvalidTerrainSize", args);
			return FALSE;
			
		}
	}

	return TRUE;
}


// static
void LLPanelRegionTextureInfo::onClickDump(void* data)
{
	llinfos << "LLPanelRegionTextureInfo::onClickDump()" << llendl;
}


/////////////////////////////////////////////////////////////////////////////
// LLPanelRegionTerrainInfo
/////////////////////////////////////////////////////////////////////////////
BOOL LLPanelRegionTerrainInfo::postBuild()
{
	LLPanelRegionInfo::postBuild();

	initCtrl("water_height_spin");
	initCtrl("terrain_raise_spin");
	initCtrl("terrain_lower_spin");

	initCtrl("fixed_sun_check");
	getChild<LLUICtrl>("fixed_sun_check")->setCommitCallback(boost::bind(&LLPanelRegionTerrainInfo::onChangeFixedSun, this));
	getChild<LLUICtrl>("use_estate_sun_check")->setCommitCallback(boost::bind(&LLPanelRegionTerrainInfo::onChangeUseEstateTime, this));
	getChild<LLUICtrl>("sun_hour_slider")->setCommitCallback(boost::bind(&LLPanelRegionTerrainInfo::onChangeSunHour, this));

	childSetAction("download_raw_btn", onClickDownloadRaw, this);
	childSetAction("upload_raw_btn", onClickUploadRaw, this);
	childSetAction("bake_terrain_btn", onClickBakeTerrain, this);

	return TRUE;
}

// virtual
bool LLPanelRegionTerrainInfo::refreshFromRegion(LLViewerRegion* region)
{
	llinfos << "LLPanelRegionTerrainInfo::refreshFromRegion" << llendl;

	BOOL owner_or_god = gAgent.isGodlike() 
						|| (region && (region->getOwner() == gAgent.getID()));
	BOOL owner_or_god_or_manager = owner_or_god
						|| (region && region->isEstateManager());
	setCtrlsEnabled(owner_or_god_or_manager);
	getChildView("apply_btn")->setEnabled(FALSE);

	getChildView("download_raw_btn")->setEnabled(owner_or_god);
	getChildView("upload_raw_btn")->setEnabled(owner_or_god);
	getChildView("bake_terrain_btn")->setEnabled(owner_or_god);

	return LLPanelRegionInfo::refreshFromRegion(region);
}

// virtual
BOOL LLPanelRegionTerrainInfo::sendUpdate()
{
	llinfos << "LLPanelRegionTerrainInfo::sendUpdate" << llendl;
	std::string buffer;
	strings_t strings;
	LLUUID invoice(LLFloaterRegionInfo::getLastInvoice());

	buffer = llformat("%f", (F32)getChild<LLUICtrl>("water_height_spin")->getValue().asReal());
	strings.push_back(buffer);
	buffer = llformat("%f", (F32)getChild<LLUICtrl>("terrain_raise_spin")->getValue().asReal());
	strings.push_back(buffer);
	buffer = llformat("%f", (F32)getChild<LLUICtrl>("terrain_lower_spin")->getValue().asReal());
	strings.push_back(buffer);
	buffer = llformat("%s", (getChild<LLUICtrl>("use_estate_sun_check")->getValue().asBoolean() ? "Y" : "N"));
	strings.push_back(buffer);
	buffer = llformat("%s", (getChild<LLUICtrl>("fixed_sun_check")->getValue().asBoolean() ? "Y" : "N"));
	strings.push_back(buffer);
	buffer = llformat("%f", (F32)getChild<LLUICtrl>("sun_hour_slider")->getValue().asReal() );
	strings.push_back(buffer);

	// Grab estate information in case the user decided to set the
	// region back to estate time. JC
	LLFloaterRegionInfo* floater = LLFloaterReg::getTypedInstance<LLFloaterRegionInfo>("region_info");
	if (!floater) return true;

	LLTabContainer* tab = floater->getChild<LLTabContainer>("region_panels");
	if (!tab) return true;

	LLPanelEstateInfo* panel = (LLPanelEstateInfo*)tab->getChild<LLPanel>("Estate");
	if (!panel) return true;

	BOOL estate_global_time = panel->getGlobalTime();
	BOOL estate_fixed_sun = panel->getFixedSun();
	F32 estate_sun_hour;
	if (estate_global_time)
	{
		estate_sun_hour = 0.f;
	}
	else
	{
		estate_sun_hour = panel->getSunHour();
	}

	buffer = llformat("%s", (estate_global_time ? "Y" : "N") );
	strings.push_back(buffer);
	buffer = llformat("%s", (estate_fixed_sun ? "Y" : "N") );
	strings.push_back(buffer);
	buffer = llformat("%f", estate_sun_hour);
	strings.push_back(buffer);

	sendEstateOwnerMessage(gMessageSystem, "setregionterrain", invoice, strings);
	return TRUE;
}

void LLPanelRegionTerrainInfo::onChangeUseEstateTime()
{
	BOOL use_estate_sun = getChild<LLUICtrl>("use_estate_sun_check")->getValue().asBoolean();
	getChildView("fixed_sun_check")->setEnabled(!use_estate_sun);
	getChildView("sun_hour_slider")->setEnabled(!use_estate_sun);
	if (use_estate_sun)
	{
		getChild<LLUICtrl>("fixed_sun_check")->setValue(LLSD(FALSE));
		getChild<LLUICtrl>("sun_hour_slider")->setValue(LLSD(0.f));
	}
	getChildView("apply_btn")->setEnabled(TRUE);
}

void LLPanelRegionTerrainInfo::onChangeFixedSun()
{
	// Just enable the apply button.  We let the sun-hour slider be enabled
	// for both fixed-sun and non-fixed-sun. JC
	getChildView("apply_btn")->setEnabled(TRUE);
}

void LLPanelRegionTerrainInfo::onChangeSunHour()
{
	getChildView("apply_btn")->setEnabled(TRUE);
}

// static
void LLPanelRegionTerrainInfo::onClickDownloadRaw(void* data)
{
	LLFilePicker& picker = LLFilePicker::instance();
	if (!picker.getSaveFile(LLFilePicker::FFSAVE_RAW, "terrain.raw"))
	{
		llwarns << "No file" << llendl;
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
		llwarns << "No file" << llendl;
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
}

// static
void LLPanelEstateInfo::initDispatch(LLDispatcher& dispatch)
{
	std::string name;

//	name.assign("setowner");
//	static LLDispatchSetEstateOwner set_owner;
//	dispatch.addHandler(name, &set_owner);

	name.assign("estateupdateinfo");
	static LLDispatchEstateUpdateInfo estate_update_info;
	dispatch.addHandler(name, &estate_update_info);

	name.assign("setaccess");
	static LLDispatchSetEstateAccess set_access;
	dispatch.addHandler(name, &set_access);

	estate_dispatch_initialized = true;
}

// Disables the sun-hour slider and the use fixed time check if the use global time is check
void LLPanelEstateInfo::onChangeUseGlobalTime()
{
	bool enabled = !getChild<LLUICtrl>("use_global_time_check")->getValue().asBoolean();
	getChildView("sun_hour_slider")->setEnabled(enabled);
	getChildView("fixed_sun_check")->setEnabled(enabled);
	getChild<LLUICtrl>("fixed_sun_check")->setValue(LLSD(FALSE));
	enableButton("apply_btn");
}

// Enables the sun-hour slider if the fixed-sun checkbox is set
void LLPanelEstateInfo::onChangeFixedSun()
{
	bool enabled = !getChild<LLUICtrl>("fixed_sun_check")->getValue().asBoolean();
	getChildView("use_global_time_check")->setEnabled(enabled);
	getChild<LLUICtrl>("use_global_time_check")->setValue(LLSD(FALSE));
	enableButton("apply_btn");
}




//---------------------------------------------------------------------------
// Add/Remove estate access button callbacks
//---------------------------------------------------------------------------
void LLPanelEstateInfo::onClickEditSky()
{
	LLFloaterReg::showInstance("env_windlight");
}

void LLPanelEstateInfo::onClickEditDayCycle()
{
	LLFloaterReg::showInstance("env_day_cycle");
}

void LLPanelEstateInfo::onClickAddAllowedAgent()
{
	LLCtrlListInterface *list = childGetListInterface("allowed_avatar_name_list");
	if (!list) return;
	if (list->getItemCount() >= ESTATE_MAX_ACCESS_IDS)
	{
		//args

		LLSD args;
		args["MAX_AGENTS"] = llformat("%d",ESTATE_MAX_ACCESS_IDS);
		LLNotificationsUtil::add("MaxAllowedAgentOnRegion", args);
		return;
	}
	accessAddCore(ESTATE_ACCESS_ALLOWED_AGENT_ADD, "EstateAllowedAgentAdd");
}

void LLPanelEstateInfo::onClickRemoveAllowedAgent()
{
	accessRemoveCore(ESTATE_ACCESS_ALLOWED_AGENT_REMOVE, "EstateAllowedAgentRemove", "allowed_avatar_name_list");
}

void LLPanelEstateInfo::onClickAddAllowedGroup()
{
	LLCtrlListInterface *list = childGetListInterface("allowed_group_name_list");
	if (!list) return;
	if (list->getItemCount() >= ESTATE_MAX_ACCESS_IDS)
	{
		LLSD args;
		args["MAX_GROUPS"] = llformat("%d",ESTATE_MAX_ACCESS_IDS);
		LLNotificationsUtil::add("MaxAllowedGroupsOnRegion", args);
		return;
	}

	LLNotification::Params params("ChangeLindenAccess");
	params.functor.function(boost::bind(&LLPanelEstateInfo::addAllowedGroup, this, _1, _2));
	if (isLindenEstate())
	{
		LLNotifications::instance().add(params);
	}
	else
	{
		LLNotifications::instance().forceResponse(params, 0);
	}
}

bool LLPanelEstateInfo::addAllowedGroup(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if (option != 0) return false;

	LLFloater* parent_floater = gFloaterView->getParentFloater(this);

	LLFloaterGroupPicker* widget = LLFloaterReg::showTypedInstance<LLFloaterGroupPicker>("group_picker", LLSD(gAgent.getID()));
	if (widget)
	{
		widget->removeNoneOption();
		widget->setSelectGroupCallback(boost::bind(&LLPanelEstateInfo::addAllowedGroup2, this, _1));
		if (parent_floater)
		{
			LLRect new_rect = gFloaterView->findNeighboringPosition(parent_floater, widget);
			widget->setOrigin(new_rect.mLeft, new_rect.mBottom);
			parent_floater->addDependentFloater(widget);
		}
	}

	return false;
}

void LLPanelEstateInfo::onClickRemoveAllowedGroup()
{
	accessRemoveCore(ESTATE_ACCESS_ALLOWED_GROUP_REMOVE, "EstateAllowedGroupRemove", "allowed_group_name_list");
}

void LLPanelEstateInfo::onClickAddBannedAgent()
{
	LLCtrlListInterface *list = childGetListInterface("banned_avatar_name_list");
	if (!list) return;
	if (list->getItemCount() >= ESTATE_MAX_ACCESS_IDS)
	{
		LLSD args;
		args["MAX_BANNED"] = llformat("%d",ESTATE_MAX_ACCESS_IDS);
		LLNotificationsUtil::add("MaxBannedAgentsOnRegion", args);
		return;
	}
	accessAddCore(ESTATE_ACCESS_BANNED_AGENT_ADD, "EstateBannedAgentAdd");
}

void LLPanelEstateInfo::onClickRemoveBannedAgent()
{
	accessRemoveCore(ESTATE_ACCESS_BANNED_AGENT_REMOVE, "EstateBannedAgentRemove", "banned_avatar_name_list");
}

// static
void LLPanelEstateInfo::onClickAddEstateManager()
{
	LLCtrlListInterface *list = childGetListInterface("estate_manager_name_list");
	if (!list) return;
	if (list->getItemCount() >= ESTATE_MAX_MANAGERS)
	{	// Tell user they can't add more managers
		LLSD args;
		args["MAX_MANAGER"] = llformat("%d",ESTATE_MAX_MANAGERS);
		LLNotificationsUtil::add("MaxManagersOnRegion", args);
	}
	else
	{	// Go pick managers to add
		accessAddCore(ESTATE_ACCESS_MANAGER_ADD, "EstateManagerAdd");
	}
}

// static
void LLPanelEstateInfo::onClickRemoveEstateManager()
{
	accessRemoveCore(ESTATE_ACCESS_MANAGER_REMOVE, "EstateManagerRemove", "estate_manager_name_list");
}

//---------------------------------------------------------------------------
// Kick from estate methods
//---------------------------------------------------------------------------

void LLPanelEstateInfo::onClickKickUser()
{
	// this depends on the grandparent view being a floater
	// in order to set up floater dependency
	LLFloater* parent_floater = gFloaterView->getParentFloater(this);
	LLFloater* child_floater = LLFloaterAvatarPicker::show(boost::bind(&LLPanelEstateInfo::onKickUserCommit, this, _1), FALSE, TRUE);
	parent_floater->addDependentFloater(child_floater);
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
	LLPanelEstateInfo* panel = LLFloaterRegionInfo::getPanelEstate();
	if (!panel) return false;

	U32 estate_id = panel->getEstateID();
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
		for (uuid_vec_t::const_iterator it = mAgentOrGroupIDs.begin();
			it != mAgentOrGroupIDs.end();
			++it)
		{
			sd["allowed_ids"].append(*it);
		}
		return sd;
	}

	U32 mOperationFlag;	// ESTATE_ACCESS_BANNED_AGENT_ADD, _REMOVE, etc.
	std::string mDialogName;
	uuid_vec_t mAgentOrGroupIDs; // List of agent IDs to apply to this change
};

// Special case callback for groups, since it has different callback format than names
void LLPanelEstateInfo::addAllowedGroup2(LLUUID id)
{
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
	if (isLindenEstate())
	{
		LLNotifications::instance().forceResponse(params, 0);
	}
	else
	{
		LLNotifications::instance().add(params);
	}
}

// static
void LLPanelEstateInfo::accessAddCore(U32 operation_flag, const std::string& dialog_name)
{
	LLSD payload;
	payload["operation"] = (S32)operation_flag;
	payload["dialog_name"] = dialog_name;
	// agent id filled in after avatar picker

	LLNotification::Params params("ChangeLindenAccess");
	params.payload(payload)
		.functor.function(accessAddCore2);

	if (isLindenEstate())
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
bool LLPanelEstateInfo::accessAddCore2(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if (option != 0)
	{
		// abort change
		return false;
	}

	LLEstateAccessChangeInfo* change_info = new LLEstateAccessChangeInfo(notification["payload"]);
	// avatar picker yes multi-select, yes close-on-select
	LLFloaterAvatarPicker::show(boost::bind(&LLPanelEstateInfo::accessAddCore3, _1, (void*)change_info), TRUE, TRUE);
	return false;
}

// static
void LLPanelEstateInfo::accessAddCore3(const uuid_vec_t& ids, void* data)
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
	LLPanelEstateInfo* panel = LLFloaterRegionInfo::getPanelEstate();
	if (!panel) return;
	LLViewerRegion* region = gAgent.getRegion();
	if (!region) return;
	
	if (change_info->mOperationFlag & ESTATE_ACCESS_ALLOWED_AGENT_ADD)
	{
		LLCtrlListInterface *list = panel->childGetListInterface("allowed_avatar_name_list");
		int currentCount = (list ? list->getItemCount() : 0);
		if (ids.size() + currentCount > ESTATE_MAX_ACCESS_IDS)
		{
			LLSD args;
			args["NUM_ADDED"] = llformat("%d",ids.size());
			args["MAX_AGENTS"] = llformat("%d",ESTATE_MAX_ACCESS_IDS);
			args["LIST_TYPE"] = "Allowed Residents";
			args["NUM_EXCESS"] = llformat("%d",(ids.size()+currentCount)-ESTATE_MAX_ACCESS_IDS);
			LLNotificationsUtil::add("MaxAgentOnRegionBatch", args);
			delete change_info;
			return;
		}
	}
	if (change_info->mOperationFlag & ESTATE_ACCESS_BANNED_AGENT_ADD)
	{
		LLCtrlListInterface *list = panel->childGetListInterface("banned_avatar_name_list");
		int currentCount = (list ? list->getItemCount() : 0);
		if (ids.size() + currentCount > ESTATE_MAX_ACCESS_IDS)
		{
			LLSD args;
			args["NUM_ADDED"] = llformat("%d",ids.size());
			args["MAX_AGENTS"] = llformat("%d",ESTATE_MAX_ACCESS_IDS);
			args["LIST_TYPE"] = "Banned Residents";
			args["NUM_EXCESS"] = llformat("%d",(ids.size()+currentCount)-ESTATE_MAX_ACCESS_IDS);
			LLNotificationsUtil::add("MaxAgentOnRegionBatch", args);
			delete change_info;
			return;
		}
	}

	LLSD args;
	args["ALL_ESTATES"] = all_estates_text();

	LLNotification::Params params(change_info->mDialogName);
	params.substitutions(args)
		.payload(change_info->asLLSD())
		.functor.function(accessCoreConfirm);

	if (isLindenEstate())
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
void LLPanelEstateInfo::accessRemoveCore(U32 operation_flag, const std::string& dialog_name, const std::string& list_ctrl_name)
{
	LLPanelEstateInfo* panel = LLFloaterRegionInfo::getPanelEstate();
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

	if (isLindenEstate())
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
bool LLPanelEstateInfo::accessRemoveCore2(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if (option != 0)
	{
		// abort
		return false;
	}

	// If Linden estate, can only apply to "this" estate, not all estates
	// owned by NULL.
	if (isLindenEstate())
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
bool LLPanelEstateInfo::accessCoreConfirm(const LLSD& notification, const LLSD& response)
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
		if (((U32)notification["payload"]["operation"].asInteger() & ESTATE_ACCESS_BANNED_AGENT_ADD)
		    && region && (region->getOwner() == id))
		{
			LLNotificationsUtil::add("OwnerCanNotBeDenied");
			break;
		}
		switch(option)
		{
			case 0:
			    // This estate
			    sendEstateAccessDelta(flags, id);
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
					sendEstateAccessDelta(flags, id);
				}
				else if (region->isEstateManager())
				{
					flags |= ESTATE_ACCESS_APPLY_TO_MANAGED_ESTATES;
					sendEstateAccessDelta(flags, id);
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

// key = "estateaccessdelta"
// str(estate_id) will be added to front of list by forward_EstateOwnerRequest_to_dataserver
// str[0] = str(agent_id) requesting the change
// str[1] = str(flags) (ESTATE_ACCESS_DELTA_*)
// str[2] = str(agent_id) to add or remove
// static
void LLPanelEstateInfo::sendEstateAccessDelta(U32 flags, const LLUUID& agent_or_group_id)
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


	LLPanelEstateInfo* panel = LLFloaterRegionInfo::getPanelEstate();

	if (flags & (ESTATE_ACCESS_ALLOWED_AGENT_ADD | ESTATE_ACCESS_ALLOWED_AGENT_REMOVE |
		         ESTATE_ACCESS_BANNED_AGENT_ADD | ESTATE_ACCESS_BANNED_AGENT_REMOVE))
	{
		
		panel->clearAccessLists();
	}

	gAgent.sendReliableMessage();
}

void LLPanelEstateInfo::updateControls(LLViewerRegion* region)
{
	BOOL god = gAgent.isGodlike();
	BOOL owner = (region && (region->getOwner() == gAgent.getID()));
	BOOL manager = (region && region->isEstateManager());
	setCtrlsEnabled(god || owner || manager);
	
	getChildView("apply_btn")->setEnabled(FALSE);
	getChildView("add_allowed_avatar_btn")->setEnabled(god || owner || manager);
	getChildView("remove_allowed_avatar_btn")->setEnabled(god || owner || manager);
	getChildView("add_allowed_group_btn")->setEnabled(god || owner || manager);
	getChildView("remove_allowed_group_btn")->setEnabled(god || owner || manager);

	// Can't ban people from mainland, orientation islands, etc. because this
	// creates much network traffic and server load.
	// Disable their accounts in CSR tool instead.
	bool linden_estate = (getEstateID() <= ESTATE_LAST_LINDEN);
	bool enable_ban = (god || owner || manager) && !linden_estate;
	getChildView("add_banned_avatar_btn")->setEnabled(enable_ban);
	getChildView("remove_banned_avatar_btn")->setEnabled(enable_ban);

	getChildView("message_estate_btn")->setEnabled(god || owner || manager);
	getChildView("kick_user_from_estate_btn")->setEnabled(god || owner || manager);

	// estate managers can't add estate managers
	getChildView("add_estate_manager_btn")->setEnabled(god || owner);
	getChildView("remove_estate_manager_btn")->setEnabled(god || owner);
	getChildView("estate_manager_name_list")->setEnabled(god || owner);
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

	
	LLPanelEstateInfo* panel = LLFloaterRegionInfo::getPanelEstate();
	panel->clearAccessLists();
	

	sendEstateOwnerMessage(gMessageSystem, "getinfo", invoice, strings);

	refresh();

	return rv;
}

void LLPanelEstateInfo::updateChild(LLUICtrl* child_ctrl)
{
	if (checkRemovalButton(child_ctrl->getName()))
	{
		// do nothing
	}
	else if (checkSunHourSlider(child_ctrl))
	{
		// do nothing
	}
}

bool LLPanelEstateInfo::estateUpdate(LLMessageSystem* msg)
{
	llinfos << "LLPanelEstateInfo::estateUpdate()" << llendl;
	return false;
}


BOOL LLPanelEstateInfo::postBuild()
{
	// set up the callbacks for the generic controls
	initCtrl("externally_visible_check");
	initCtrl("use_global_time_check");
	initCtrl("fixed_sun_check");
	initCtrl("allow_direct_teleport");
	initCtrl("limit_payment");
	initCtrl("limit_age_verified");
	initCtrl("voice_chat_check");

	// set up the use global time checkbox
	getChild<LLUICtrl>("use_global_time_check")->setCommitCallback(boost::bind(&LLPanelEstateInfo::onChangeUseGlobalTime, this));
	getChild<LLUICtrl>("fixed_sun_check")->setCommitCallback(boost::bind(&LLPanelEstateInfo::onChangeFixedSun, this));
	getChild<LLUICtrl>("sun_hour_slider")->setCommitCallback(boost::bind(&LLPanelEstateInfo::onChangeChildCtrl, this, _1));
	
	getChild<LLUICtrl>("allowed_avatar_name_list")->setCommitCallback(boost::bind(&LLPanelEstateInfo::onChangeChildCtrl, this, _1));	
	LLNameListCtrl *avatar_name_list = getChild<LLNameListCtrl>("allowed_avatar_name_list");
	if (avatar_name_list)
	{
		avatar_name_list->setCommitOnSelectionChange(TRUE);
		avatar_name_list->setMaxItemCount(ESTATE_MAX_ACCESS_IDS);
	}

	childSetAction("add_allowed_avatar_btn", boost::bind(&LLPanelEstateInfo::onClickAddAllowedAgent, this));
	childSetAction("remove_allowed_avatar_btn", boost::bind(&LLPanelEstateInfo::onClickRemoveAllowedAgent, this));

	getChild<LLUICtrl>("allowed_group_name_list")->setCommitCallback(boost::bind(&LLPanelEstateInfo::onChangeChildCtrl, this, _1));
	LLNameListCtrl* group_name_list = getChild<LLNameListCtrl>("allowed_group_name_list");
	if (group_name_list)
	{
		group_name_list->setCommitOnSelectionChange(TRUE);
		group_name_list->setMaxItemCount(ESTATE_MAX_ACCESS_IDS);
	}

	getChild<LLUICtrl>("add_allowed_group_btn")->setCommitCallback(boost::bind(&LLPanelEstateInfo::onClickAddAllowedGroup, this));
	childSetAction("remove_allowed_group_btn", boost::bind(&LLPanelEstateInfo::onClickRemoveAllowedGroup, this));

	getChild<LLUICtrl>("banned_avatar_name_list")->setCommitCallback(boost::bind(&LLPanelEstateInfo::onChangeChildCtrl, this, _1));
	LLNameListCtrl* banned_name_list = getChild<LLNameListCtrl>("banned_avatar_name_list");
	if (banned_name_list)
	{
		banned_name_list->setCommitOnSelectionChange(TRUE);
		banned_name_list->setMaxItemCount(ESTATE_MAX_ACCESS_IDS);
	}

	childSetAction("add_banned_avatar_btn", boost::bind(&LLPanelEstateInfo::onClickAddBannedAgent, this));
	childSetAction("remove_banned_avatar_btn", boost::bind(&LLPanelEstateInfo::onClickRemoveBannedAgent, this));

	getChild<LLUICtrl>("estate_manager_name_list")->setCommitCallback(boost::bind(&LLPanelEstateInfo::onChangeChildCtrl, this, _1));
	LLNameListCtrl* manager_name_list = getChild<LLNameListCtrl>("estate_manager_name_list");
	if (manager_name_list)
	{
		manager_name_list->setCommitOnSelectionChange(TRUE);
		manager_name_list->setMaxItemCount(ESTATE_MAX_MANAGERS * 4);	// Allow extras for dupe issue
	}

	childSetAction("add_estate_manager_btn", boost::bind(&LLPanelEstateInfo::onClickAddEstateManager, this));
	childSetAction("remove_estate_manager_btn", boost::bind(&LLPanelEstateInfo::onClickRemoveEstateManager, this));
	childSetAction("message_estate_btn", boost::bind(&LLPanelEstateInfo::onClickMessageEstate, this));
	childSetAction("kick_user_from_estate_btn", boost::bind(&LLPanelEstateInfo::onClickKickUser, this));

	childSetAction("WLEditSky", boost::bind(&LLPanelEstateInfo::onClickEditSky, this));
	childSetAction("WLEditDayCycle", boost::bind(&LLPanelEstateInfo::onClickEditDayCycle, this));

	return LLPanelRegionInfo::postBuild();
}

void LLPanelEstateInfo::refresh()
{
	bool public_access = getChild<LLUICtrl>("externally_visible_check")->getValue().asBoolean();
	getChildView("Only Allow")->setEnabled(public_access);
	getChildView("limit_payment")->setEnabled(public_access);
	getChildView("limit_age_verified")->setEnabled(public_access);
	// if this is set to false, then the limit fields are meaningless and should be turned off
	if (public_access == false)
	{
		getChild<LLUICtrl>("limit_payment")->setValue(false);
		getChild<LLUICtrl>("limit_age_verified")->setValue(false);
	}
}

BOOL LLPanelEstateInfo::sendUpdate()
{
	llinfos << "LLPanelEsateInfo::sendUpdate()" << llendl;

	LLNotification::Params params("ChangeLindenEstate");
	params.functor.function(boost::bind(&LLPanelEstateInfo::callbackChangeLindenEstate, this, _1, _2));

	if (getEstateID() <= ESTATE_LAST_LINDEN)
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
		// send the update
		if (!commitEstateInfoCaps())
		{
			// the caps method failed, try the old way
			LLFloaterRegionInfo::nextInvoice();
			commitEstateInfoDataserver();
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

class LLEstateChangeInfoResponder : public LLHTTPClient::Responder
{
public:
	LLEstateChangeInfoResponder(LLPanelEstateInfo* panel)
	{
		mpPanel = panel->getHandle();
	}
	
	// if we get a normal response, handle it here
	virtual void result(const LLSD& content)
	{
	    // refresh the panel from the database
		LLPanelEstateInfo* panel = dynamic_cast<LLPanelEstateInfo*>(mpPanel.get());
		if (panel)
			panel->refresh();
	}
	
	// if we get an error response
	virtual void error(U32 status, const std::string& reason)
	{
		llinfos << "LLEstateChangeInfoResponder::error "
			<< status << ": " << reason << llendl;
	}
private:
	LLHandle<LLPanel> mpPanel;
};

// tries to send estate info using a cap; returns true if it succeeded
bool LLPanelEstateInfo::commitEstateInfoCaps()
{
	std::string url = gAgent.getRegion()->getCapability("EstateChangeInfo");
	
	if (url.empty())
	{
		// whoops, couldn't find the cap, so bail out
		return false;
	}
	
	LLSD body;
	body["estate_name"] = getEstateName();

	body["is_externally_visible"] = getChild<LLUICtrl>("externally_visible_check")->getValue().asBoolean();
	body["allow_direct_teleport"] = getChild<LLUICtrl>("allow_direct_teleport")->getValue().asBoolean();
	body["is_sun_fixed"         ] = getChild<LLUICtrl>("fixed_sun_check")->getValue().asBoolean();
	body["deny_anonymous"       ] = getChild<LLUICtrl>("limit_payment")->getValue().asBoolean();
	body["deny_age_unverified"  ] = getChild<LLUICtrl>("limit_age_verified")->getValue().asBoolean();
	body["allow_voice_chat"     ] = getChild<LLUICtrl>("voice_chat_check")->getValue().asBoolean();
	body["invoice"              ] = LLFloaterRegionInfo::getLastInvoice();

	// block fly is in estate database but not in estate UI, so we're not supporting it
	//body["block_fly"            ] = getChild<LLUICtrl>("")->getValue().asBoolean();

	F32 sun_hour = getSunHour();
	if (getChild<LLUICtrl>("use_global_time_check")->getValue().asBoolean())
	{
		sun_hour = 0.f;			// 0 = global time
	}
	body["sun_hour"] = sun_hour;

	// we use a responder so that we can re-get the data after committing to the database
	LLHTTPClient::post(url, body, new LLEstateChangeInfoResponder(this));
    return true;
}

/* This is the old way of doing things, is deprecated, and should be 
   deleted when the dataserver model can be removed */
// key = "estatechangeinfo"
// strings[0] = str(estate_id) (added by simulator before relay - not here)
// strings[1] = estate_name
// strings[2] = str(estate_flags)
// strings[3] = str((S32)(sun_hour * 1024.f))
void LLPanelEstateInfo::commitEstateInfoDataserver()
{
	LLMessageSystem* msg = gMessageSystem;
	msg->newMessage("EstateOwnerMessage");
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->addUUIDFast(_PREHASH_TransactionID, LLUUID::null); //not used

	msg->nextBlock("MethodData");
	msg->addString("Method", "estatechangeinfo");
	msg->addUUID("Invoice", LLFloaterRegionInfo::getLastInvoice());

	msg->nextBlock("ParamList");
	msg->addString("Parameter", getEstateName());

	std::string buffer;
	buffer = llformat("%u", computeEstateFlags());
	msg->nextBlock("ParamList");
	msg->addString("Parameter", buffer);

	F32 sun_hour = getSunHour();
	if (getChild<LLUICtrl>("use_global_time_check")->getValue().asBoolean())
	{
		sun_hour = 0.f;	// 0 = global time
	}

	buffer = llformat("%d", (S32)(sun_hour*1024.0f));
	msg->nextBlock("ParamList");
	msg->addString("Parameter", buffer);

	gAgent.sendMessage();
}

void LLPanelEstateInfo::setEstateFlags(U32 flags)
{
	getChild<LLUICtrl>("externally_visible_check")->setValue(LLSD(flags & REGION_FLAGS_EXTERNALLY_VISIBLE ? TRUE : FALSE) );
	getChild<LLUICtrl>("fixed_sun_check")->setValue(LLSD(flags & REGION_FLAGS_SUN_FIXED ? TRUE : FALSE) );
	getChild<LLUICtrl>("voice_chat_check")->setValue(
		LLSD(flags & REGION_FLAGS_ALLOW_VOICE ? TRUE : FALSE));
	getChild<LLUICtrl>("allow_direct_teleport")->setValue(LLSD(flags & REGION_FLAGS_ALLOW_DIRECT_TELEPORT ? TRUE : FALSE) );
	getChild<LLUICtrl>("limit_payment")->setValue(LLSD(flags & REGION_FLAGS_DENY_ANONYMOUS ? TRUE : FALSE) );
	getChild<LLUICtrl>("limit_age_verified")->setValue(LLSD(flags & REGION_FLAGS_DENY_AGEUNVERIFIED ? TRUE : FALSE) );

	refresh();
}

U32 LLPanelEstateInfo::computeEstateFlags()
{
	U32 flags = 0;

	if (getChild<LLUICtrl>("externally_visible_check")->getValue().asBoolean())
	{
		flags |= REGION_FLAGS_EXTERNALLY_VISIBLE;
	}

	if ( getChild<LLUICtrl>("voice_chat_check")->getValue().asBoolean() )
	{
		flags |= REGION_FLAGS_ALLOW_VOICE;
	}
	
	if (getChild<LLUICtrl>("allow_direct_teleport")->getValue().asBoolean())
	{
		flags |= REGION_FLAGS_ALLOW_DIRECT_TELEPORT;
	}

	if (getChild<LLUICtrl>("fixed_sun_check")->getValue().asBoolean())
	{
		flags |= REGION_FLAGS_SUN_FIXED;
	}
	
	if (getChild<LLUICtrl>("limit_payment")->getValue().asBoolean())
	{
		flags |= REGION_FLAGS_DENY_ANONYMOUS;
	}
	
	if (getChild<LLUICtrl>("limit_age_verified")->getValue().asBoolean())
	{
		flags |= REGION_FLAGS_DENY_AGEUNVERIFIED;
	}

	
	return flags;
}

BOOL LLPanelEstateInfo::getGlobalTime()
{
	return getChild<LLUICtrl>("use_global_time_check")->getValue().asBoolean();
}

void LLPanelEstateInfo::setGlobalTime(bool b)
{
	getChild<LLUICtrl>("use_global_time_check")->setValue(LLSD(b));
	getChildView("fixed_sun_check")->setEnabled(LLSD(!b));
	getChildView("sun_hour_slider")->setEnabled(LLSD(!b));
	if (b)
	{
		getChild<LLUICtrl>("sun_hour_slider")->setValue(LLSD(0.f));
	}
}


BOOL LLPanelEstateInfo::getFixedSun()
{
	return getChild<LLUICtrl>("fixed_sun_check")->getValue().asBoolean();
}

void LLPanelEstateInfo::setSunHour(F32 sun_hour)
{
	if(sun_hour < 6.0f)
	{
		sun_hour = 24.0f + sun_hour;
	}
	getChild<LLUICtrl>("sun_hour_slider")->setValue(LLSD(sun_hour));
}

F32 LLPanelEstateInfo::getSunHour()
{
	if (getChildView("sun_hour_slider")->getEnabled())
	{
		return (F32)getChild<LLUICtrl>("sun_hour_slider")->getValue().asReal();
	}
	return 0.f;
}

const std::string LLPanelEstateInfo::getEstateName() const
{
	return getChild<LLUICtrl>("estate_name")->getValue().asString();
}

void LLPanelEstateInfo::setEstateName(const std::string& name)
{
	getChild<LLUICtrl>("estate_name")->setValue(LLSD(name));
}

const std::string LLPanelEstateInfo::getOwnerName() const
{
	return getChild<LLUICtrl>("estate_owner")->getValue().asString();
}

void LLPanelEstateInfo::setOwnerName(const std::string& name)
{
	getChild<LLUICtrl>("estate_owner")->setValue(LLSD(name));
}

void LLPanelEstateInfo::setAccessAllowedEnabled(bool enable_agent,
												bool enable_group,
												bool enable_ban)
{
	getChildView("allow_resident_label")->setEnabled(enable_agent);
	getChildView("allowed_avatar_name_list")->setEnabled(enable_agent);
	getChildView("allowed_avatar_name_list")->setVisible( enable_agent);
	getChildView("add_allowed_avatar_btn")->setEnabled(enable_agent);
	getChildView("remove_allowed_avatar_btn")->setEnabled(enable_agent);

	// Groups
	getChildView("allow_group_label")->setEnabled(enable_group);
	getChildView("allowed_group_name_list")->setEnabled(enable_group);
	getChildView("allowed_group_name_list")->setVisible( enable_group);
	getChildView("add_allowed_group_btn")->setEnabled(enable_group);
	getChildView("remove_allowed_group_btn")->setEnabled(enable_group);

	// Ban
	getChildView("ban_resident_label")->setEnabled(enable_ban);
	getChildView("banned_avatar_name_list")->setEnabled(enable_ban);
	getChildView("banned_avatar_name_list")->setVisible( enable_ban);
	getChildView("add_banned_avatar_btn")->setEnabled(enable_ban);
	getChildView("remove_banned_avatar_btn")->setEnabled(enable_ban);

	// Update removal buttons if needed
	if (enable_agent)
	{
		checkRemovalButton("allowed_avatar_name_list");
	}

	if (enable_group)
	{
		checkRemovalButton("allowed_group_name_list");
	}

	if (enable_ban)
	{
		checkRemovalButton("banned_avatar_name_list");
	}
}

void LLPanelEstateInfo::clearAccessLists() 
{
	LLNameListCtrl* name_list = getChild<LLNameListCtrl>("allowed_avatar_name_list");
	if (name_list)
	{
		name_list->deleteAllItems();
	}

	name_list = getChild<LLNameListCtrl>("banned_avatar_name_list");
	if (name_list)
	{
		name_list->deleteAllItems();
	}
}

// enables/disables the "remove" button for the various allow/ban lists
BOOL LLPanelEstateInfo::checkRemovalButton(std::string name)
{
	std::string btn_name = "";
	if (name == "allowed_avatar_name_list")
	{
		btn_name = "remove_allowed_avatar_btn";
	}
	else if (name == "allowed_group_name_list")
	{
		btn_name = "remove_allowed_group_btn";
	}
	else if (name == "banned_avatar_name_list")
	{
		btn_name = "remove_banned_avatar_btn";
	}
	else if (name == "estate_manager_name_list")
	{
		//ONLY OWNER CAN ADD /DELET ESTATE MANAGER
		LLViewerRegion* region = gAgent.getRegion();
		if (region && (region->getOwner() == gAgent.getID()))
		{
			btn_name = "remove_estate_manager_btn";
		}
	}

	// enable the remove button if something is selected
	LLNameListCtrl* name_list = getChild<LLNameListCtrl>(name);
	getChildView(btn_name)->setEnabled(name_list && name_list->getFirstSelected() ? TRUE : FALSE);

	return (btn_name != "");
}

BOOL LLPanelEstateInfo::checkSunHourSlider(LLUICtrl* child_ctrl)
{
	BOOL found_child_ctrl = FALSE;
	if (child_ctrl->getName() == "sun_hour_slider")
	{
		enableButton("apply_btn");
		found_child_ctrl = TRUE;
	}
	return found_child_ctrl;
}

// static
void LLPanelEstateInfo::onClickMessageEstate(void* userdata)
{
	llinfos << "LLPanelEstateInfo::onClickMessageEstate" << llendl;
	LLNotificationsUtil::add("MessageEstate", LLSD(), LLSD(), boost::bind(&LLPanelEstateInfo::onMessageCommit, (LLPanelEstateInfo*)userdata, _1, _2));
}

bool LLPanelEstateInfo::onMessageCommit(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	std::string text = response["message"].asString();
	if(option != 0) return false;
	if(text.empty()) return false;
	llinfos << "Message to everyone: " << text << llendl;
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
		if (region->getRegionFlags() & REGION_FLAGS_BLOCK_LAND_RESELL)
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
		if (region->getRegionFlags() & REGION_FLAGS_ALLOW_PARCEL_CHANGES)
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
	if (region_landtype)
	{
		region_landtype->setText(region->getSimProductName());
	}
	
	
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
	llinfos << "LLPanelEstateCovenant::estateUpdate()" << llendl;
	return true;
}
	
// virtual 
BOOL LLPanelEstateCovenant::postBuild()
{
	mEstateNameText = getChild<LLTextBox>("estate_name_text");
	mEstateOwnerText = getChild<LLTextBox>("estate_owner_text");
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
void LLPanelEstateCovenant::onLoadComplete(LLVFS *vfs,
									   const LLUUID& asset_uuid,
									   LLAssetType::EType type,
									   void* user_data, S32 status, LLExtStat ext_status)
{
	llinfos << "LLPanelEstateCovenant::onLoadComplete()" << llendl;
	LLPanelEstateCovenant* panelp = (LLPanelEstateCovenant*)user_data;
	if( panelp )
	{
		if(0 == status)
		{
			LLVFile file(vfs, asset_uuid, type, LLVFile::READ);

			S32 file_length = file.getSize();

			std::vector<char> buffer(file_length+1);
			file.read((U8*)&buffer[0], file_length);
			// put a EOS at the end
			buffer[file_length] = 0;

			if( (file_length > 19) && !strncmp( &buffer[0], "Linden text version", 19 ) )
			{
				if( !panelp->mEditor->importBuffer( &buffer[0], file_length+1 ) )
				{
					llwarns << "Problem importing estate covenant." << llendl;
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
			LLViewerStats::getInstance()->incStat( LLViewerStats::ST_DOWNLOAD_FAILED );

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

			llwarns << "Problem loading notecard: " << status << llendl;
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
	LLPanelEstateInfo* panel = LLFloaterRegionInfo::getPanelEstate();
	if (!panel) return true;

	// NOTE: LLDispatcher extracts strings with an extra \0 at the
	// end.  If we pass the std::string direct to the UI/renderer
	// it draws with a weird character at the end of the string.
	std::string estate_name = strings[0].c_str(); // preserve c_str() call!
	panel->setEstateName(estate_name);
	
	LLViewerRegion* regionp = gAgent.getRegion();

	LLUUID owner_id(strings[1]);
	regionp->setOwner(owner_id);
	// Update estate owner name in UI
	std::string owner_name = LLSLURL("agent", owner_id, "inspect").getSLURLString();
	panel->setOwnerName(owner_name);

	U32 estate_id = strtoul(strings[2].c_str(), NULL, 10);
	panel->setEstateID(estate_id);

	U32 flags = strtoul(strings[3].c_str(), NULL, 10);
	panel->setEstateFlags(flags);

	F32 sun_hour = ((F32)(strtod(strings[4].c_str(), NULL)))/1024.0f;
	if(sun_hour == 0 && (flags & REGION_FLAGS_SUN_FIXED ? FALSE : TRUE))
	{
		panel->setGlobalTime(TRUE);
	} 
	else
	{
		panel->setGlobalTime(FALSE);
		panel->setSunHour(sun_hour);
	}

	bool visible_from_mainland = (bool)(flags & REGION_FLAGS_EXTERNALLY_VISIBLE);
	bool god = gAgent.isGodlike();
	bool linden_estate = (estate_id <= ESTATE_LAST_LINDEN);

	// If visible from mainland, disable the access allowed
	// UI, as anyone can teleport there.
	// However, gods need to be able to edit the access list for
	// linden estates, regardless of visibility, to allow object
	// and L$ transfers.
	bool enable_agent = (!visible_from_mainland || (god && linden_estate));
	bool enable_group = enable_agent;
	bool enable_ban = !linden_estate;
	panel->setAccessAllowedEnabled(enable_agent, enable_group, enable_ban);

	return true;
}


// key = "setaccess"
// strings[0] = str(estate_id)
// strings[1] = str(packed_access_lists)
// strings[2] = str(num allowed agent ids)
// strings[3] = str(num allowed group ids)
// strings[4] = str(num banned agent ids)
// strings[5] = str(num estate manager agent ids)
// strings[6] = bin(uuid)
// strings[7] = bin(uuid)
// strings[8] = bin(uuid)
// ...
bool LLDispatchSetEstateAccess::operator()(
		const LLDispatcher* dispatcher,
		const std::string& key,
		const LLUUID& invoice,
		const sparam_t& strings)
{
	LLPanelEstateInfo* panel = LLFloaterRegionInfo::getPanelEstate();
	if (!panel) return true;

	S32 index = 1;	// skip estate_id
	U32 access_flags = strtoul(strings[index++].c_str(), NULL,10);
	S32 num_allowed_agents = strtol(strings[index++].c_str(), NULL, 10);
	S32 num_allowed_groups = strtol(strings[index++].c_str(), NULL, 10);
	S32 num_banned_agents = strtol(strings[index++].c_str(), NULL, 10);
	S32 num_estate_managers = strtol(strings[index++].c_str(), NULL, 10);

	// sanity ckecks
	if (num_allowed_agents > 0
		&& !(access_flags & ESTATE_ACCESS_ALLOWED_AGENTS))
	{
		llwarns << "non-zero count for allowed agents, but no corresponding flag" << llendl;
	}
	if (num_allowed_groups > 0
		&& !(access_flags & ESTATE_ACCESS_ALLOWED_GROUPS))
	{
		llwarns << "non-zero count for allowed groups, but no corresponding flag" << llendl;
	}
	if (num_banned_agents > 0
		&& !(access_flags & ESTATE_ACCESS_BANNED_AGENTS))
	{
		llwarns << "non-zero count for banned agents, but no corresponding flag" << llendl;
	}
	if (num_estate_managers > 0
		&& !(access_flags & ESTATE_ACCESS_MANAGERS))
	{
		llwarns << "non-zero count for managers, but no corresponding flag" << llendl;
	}

	// grab the UUID's out of the string fields
	if (access_flags & ESTATE_ACCESS_ALLOWED_AGENTS)
	{
		LLNameListCtrl* allowed_agent_name_list;
		allowed_agent_name_list = panel->getChild<LLNameListCtrl>("allowed_avatar_name_list");

		int totalAllowedAgents = num_allowed_agents;
		
		if (allowed_agent_name_list) 
		{
			totalAllowedAgents += allowed_agent_name_list->getItemCount();
		}

		LLStringUtil::format_map_t args;
		args["[ALLOWEDAGENTS]"] = llformat ("%d", totalAllowedAgents);
		args["[MAXACCESS]"] = llformat ("%d", ESTATE_MAX_ACCESS_IDS);
		std::string msg = LLTrans::getString("RegionInfoAllowedResidents", args);
		panel->getChild<LLUICtrl>("allow_resident_label")->setValue(LLSD(msg));

		if (allowed_agent_name_list)
		{
			//allowed_agent_name_list->deleteAllItems();
			for (S32 i = 0; i < num_allowed_agents && i < ESTATE_MAX_ACCESS_IDS; i++)
			{
				LLUUID id;
				memcpy(id.mData, strings[index++].data(), UUID_BYTES);		/* Flawfinder: ignore */
				allowed_agent_name_list->addNameItem(id);
			}
			panel->getChildView("remove_allowed_avatar_btn")->setEnabled(allowed_agent_name_list->getFirstSelected() ? TRUE : FALSE);
			allowed_agent_name_list->sortByColumnIndex(0, TRUE);
		}
	}

	if (access_flags & ESTATE_ACCESS_ALLOWED_GROUPS)
	{
		LLNameListCtrl* allowed_group_name_list;
		allowed_group_name_list = panel->getChild<LLNameListCtrl>("allowed_group_name_list");

		LLStringUtil::format_map_t args;
		args["[ALLOWEDGROUPS]"] = llformat ("%d", num_allowed_groups);
		args["[MAXACCESS]"] = llformat ("%d", ESTATE_MAX_GROUP_IDS);
		std::string msg = LLTrans::getString("RegionInfoAllowedGroups", args);
		panel->getChild<LLUICtrl>("allow_group_label")->setValue(LLSD(msg));

		if (allowed_group_name_list)
		{
			allowed_group_name_list->deleteAllItems();
			for (S32 i = 0; i < num_allowed_groups && i < ESTATE_MAX_GROUP_IDS; i++)
			{
				LLUUID id;
				memcpy(id.mData, strings[index++].data(), UUID_BYTES);		/* Flawfinder: ignore */
				allowed_group_name_list->addGroupNameItem(id);
			}
			panel->getChildView("remove_allowed_group_btn")->setEnabled(allowed_group_name_list->getFirstSelected() ? TRUE : FALSE);
			allowed_group_name_list->sortByColumnIndex(0, TRUE);
		}
	}

	if (access_flags & ESTATE_ACCESS_BANNED_AGENTS)
	{
		LLNameListCtrl* banned_agent_name_list;
		banned_agent_name_list = panel->getChild<LLNameListCtrl>("banned_avatar_name_list");

		int totalBannedAgents = num_banned_agents;
		
		if (banned_agent_name_list) 
		{
			totalBannedAgents += banned_agent_name_list->getItemCount();
		}


		std::string msg = llformat("Banned residents: (%d, max %d)",
									totalBannedAgents,
									ESTATE_MAX_ACCESS_IDS);
		panel->getChild<LLUICtrl>("ban_resident_label")->setValue(LLSD(msg));

		if (banned_agent_name_list)
		{
			//banned_agent_name_list->deleteAllItems();
			for (S32 i = 0; i < num_banned_agents && i < ESTATE_MAX_ACCESS_IDS; i++)
			{
				LLUUID id;
				memcpy(id.mData, strings[index++].data(), UUID_BYTES);		/* Flawfinder: ignore */
				banned_agent_name_list->addNameItem(id);
			}
			panel->getChildView("remove_banned_avatar_btn")->setEnabled(banned_agent_name_list->getFirstSelected() ? TRUE : FALSE);
			banned_agent_name_list->sortByColumnIndex(0, TRUE);
		}
	}

	if (access_flags & ESTATE_ACCESS_MANAGERS)
	{
		std::string msg = llformat("Estate Managers: (%d, max %d)",
									num_estate_managers,
									ESTATE_MAX_MANAGERS);
		panel->getChild<LLUICtrl>("estate_manager_label")->setValue(LLSD(msg));

		LLNameListCtrl* estate_manager_name_list =
			panel->getChild<LLNameListCtrl>("estate_manager_name_list");
		if (estate_manager_name_list)
		{	
			estate_manager_name_list->deleteAllItems();		// Clear existing entries

			// There should be only ESTATE_MAX_MANAGERS people in the list, but if the database gets more (SL-46107) don't 
			// truncate the list unless it's really big.  Go ahead and show the extras so the user doesn't get confused, 
			// and they can still remove them.
			for (S32 i = 0; i < num_estate_managers && i < (ESTATE_MAX_MANAGERS * 4); i++)
			{
				LLUUID id;
				memcpy(id.mData, strings[index++].data(), UUID_BYTES);		/* Flawfinder: ignore */
				estate_manager_name_list->addNameItem(id);
			}
			panel->getChildView("remove_estate_manager_btn")->setEnabled(estate_manager_name_list->getFirstSelected() ? TRUE : FALSE);
			estate_manager_name_list->sortByColumnIndex(0, TRUE);
		}
	}

	return true;
}
