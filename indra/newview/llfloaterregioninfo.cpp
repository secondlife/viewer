/** 
 * @file llfloaterregioninfo.cpp
 * @author Aaron Brashears
 * @brief Implementation of the region info and controls floater and panels.
 *
 * $LicenseInfo:firstyear=2004&license=viewergpl$
 * 
 * Copyright (c) 2004-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"
#include "llfloaterregioninfo.h"

#include <algorithm>
#include <functional>

#include "llcachename.h"
#include "lldir.h"
#include "lldispatcher.h"
#include "llglheaders.h"
#include "llregionflags.h"
#include "llstl.h"
#include "indra_constants.h"
#include "message.h"

#include "llagent.h"
#include "llalertdialog.h"
#include "llfloateravatarpicker.h"
#include "llbutton.h" 
#include "llcheckboxctrl.h"
#include "llcombobox.h"
#include "llfilepicker.h"
#include "llfloatergodtools.h"	// for send_sim_wide_deletes()
#include "llfloatertopobjects.h" // added to fix SL-32336
#include "llfloatergroups.h"
#include "llfloatertelehub.h"
#include "lllineeditor.h"
#include "llalertdialog.h"
#include "llnamelistctrl.h"
#include "llsliderctrl.h"
#include "llspinctrl.h"
#include "lltabcontainer.h"
#include "lltextbox.h"
#include "llinventory.h"
#include "lltexturectrl.h"
#include "llviewercontrol.h"
#include "llvieweruictrlfactory.h"
#include "llviewerimage.h"
#include "llviewerimagelist.h"
#include "llviewerregion.h"
#include "llviewerstats.h"
#include "llviewertexteditor.h"
#include "llviewerwindow.h"
#include "llvlcomposition.h"

const S32 TERRAIN_TEXTURE_COUNT = 4;
const S32 CORNER_COUNT = 4;


///----------------------------------------------------------------------------
/// Local class declaration
///----------------------------------------------------------------------------

/*
class LLDispatchSetEstateOwner : public LLDispatchHandler
{
public:
	LLDispatchSetEstateOwner() {}
	virtual ~LLDispatchSetEstateOwner() {}
	virtual bool operator()(
		const LLDispatcher* dispatcher,
		const std::string& key,
		const sparam_t& strings,
		const iparam_t& integers);
};
*/

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
		msg->getBinaryDataFast(_PREHASH_StringData, _PREHASH_SParam,
							   str_buf, data_size, i, MAX_STRING - 1);
		strings.push_back(std::string(str_buf, data_size));
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

LLFloaterRegionInfo::LLFloaterRegionInfo(const LLSD& seed)
{
	gUICtrlFactory->buildFloater(this, "floater_region_info.xml", NULL, FALSE);
}

BOOL LLFloaterRegionInfo::postBuild()
{
	mTab = gUICtrlFactory->getTabContainerByName(this, "region_panels");

	// contruct the panels
	LLPanelRegionInfo* panel;
	panel = new LLPanelRegionGeneralInfo;
	mInfoPanels.push_back(panel);
	gUICtrlFactory->buildPanel(panel, "panel_region_general.xml");
	mTab->addTabPanel(panel, panel->getLabel(), TRUE);

	panel = new LLPanelRegionDebugInfo;
	mInfoPanels.push_back(panel);
	gUICtrlFactory->buildPanel(panel, "panel_region_debug.xml");
	mTab->addTabPanel(panel, panel->getLabel(), FALSE);

	panel = new LLPanelRegionTextureInfo;
	mInfoPanels.push_back(panel);
	gUICtrlFactory->buildPanel(panel, "panel_region_texture.xml");
	mTab->addTabPanel(panel, panel->getLabel(), FALSE);

	panel = new LLPanelRegionTerrainInfo;
	mInfoPanels.push_back(panel);
	gUICtrlFactory->buildPanel(panel, "panel_region_terrain.xml");
	mTab->addTabPanel(panel, panel->getLabel(), FALSE);

	panel = new LLPanelEstateInfo;
	mInfoPanels.push_back(panel);
	gUICtrlFactory->buildPanel(panel, "panel_region_estate.xml");
	mTab->addTabPanel(panel, panel->getLabel(), FALSE);

	panel = new LLPanelEstateCovenant;
	mInfoPanels.push_back(panel);
	gUICtrlFactory->buildPanel(panel, "panel_region_covenant.xml");
	mTab->addTabPanel(panel, panel->getLabel(), FALSE);

	gMessageSystem->setHandlerFunc(
		"EstateOwnerMessage", 
		&processEstateOwnerRequest);

	return TRUE;
}

LLFloaterRegionInfo::~LLFloaterRegionInfo()
{
	sInstance = NULL;
}

void LLFloaterRegionInfo::onOpen()
{
	LLRect rect = gSavedSettings.getRect("FloaterRegionInfo");
	S32 left, top;
	gFloaterView->getNewFloaterPosition(&left, &top);
	rect.translate(left,top);

	requestRegionInfo();
	refreshFromRegion(gAgent.getRegion());
	LLFloater::onOpen();
}

void LLFloaterRegionInfo::requestRegionInfo()
{
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
	if(!sInstance) return;

	if (!estate_dispatch_initialized)
	{
		LLPanelEstateInfo::initDispatch(dispatch);
	}

	LLTabContainerCommon* tab = LLUICtrlFactory::getTabContainerByName(sInstance, "region_panels");
	if (!tab) return;

	LLPanelEstateInfo* panel = (LLPanelEstateInfo*)LLUICtrlFactory::getPanelByName(tab, "Estate");
	if (!panel) return;

	// unpack the message
	std::string request;
	LLUUID invoice;
	LLDispatcher::sparam_t strings;
	LLDispatcher::unpackMessage(msg, request, invoice, strings);
	if(invoice != getLastInvoice())
	{
		llwarns << "Mismatched Estate message: " << request.c_str() << llendl;
		return;
	}

	//dispatch the message
	dispatch.dispatch(request, invoice, strings);
}


// static
void LLFloaterRegionInfo::processRegionInfo(LLMessageSystem* msg)
{
	LLPanel* panel;

	llinfos << "LLFloaterRegionInfo::processRegionInfo" << llendl;
	if(!sInstance) return;
	LLTabContainerCommon* tab = LLUICtrlFactory::getTabContainerByName(sInstance, "region_panels");
	if(!tab) return;

	// extract message
	char sim_name[MAX_STRING];		/* Flawfinder: ignore*/
	U32 region_flags;
	U8 agent_limit;
	F32 object_bonus_factor;
	U8 sim_access;
	F32 water_height;
	F32 terrain_raise_limit;
	F32 terrain_lower_limit;
	BOOL use_estate_sun;
	F32 sun_hour;
	msg->getString("RegionInfo", "SimName", MAX_STRING, sim_name);
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

	// GENERAL PANEL
	panel = LLUICtrlFactory::getPanelByName(tab, "General");
	if(!panel) return;
	panel->childSetValue("region_text", LLSD(sim_name));

	panel->childSetValue("block_terraform_check", (region_flags & REGION_FLAGS_BLOCK_TERRAFORM) ? TRUE : FALSE );
	panel->childSetValue("block_fly_check", (region_flags & REGION_FLAGS_BLOCK_FLY) ? TRUE : FALSE );
	panel->childSetValue("allow_damage_check", (region_flags & REGION_FLAGS_ALLOW_DAMAGE) ? TRUE : FALSE );
	panel->childSetValue("restrict_pushobject", (region_flags & REGION_FLAGS_RESTRICT_PUSHOBJECT) ? TRUE : FALSE );
	panel->childSetValue("allow_land_resell_check", (region_flags & REGION_FLAGS_BLOCK_LAND_RESELL) ? FALSE : TRUE );
	panel->childSetValue("allow_parcel_changes_check", (region_flags & REGION_FLAGS_ALLOW_PARCEL_CHANGES) ? TRUE : FALSE );
	panel->childSetValue("block_parcel_search_check", (region_flags & REGION_FLAGS_BLOCK_PARCEL_SEARCH) ? TRUE : FALSE );
	panel->childSetValue("agent_limit_spin", LLSD((F32)agent_limit) );
	panel->childSetValue("object_bonus_spin", LLSD(object_bonus_factor) );
	panel->childSetValue("access_combo", LLSD(LLViewerRegion::accessToString(sim_access)) );


	// detect teen grid for maturity
	LLViewerRegion* region = gAgent.getRegion();

	U32 parent_estate_id;
	msg->getU32("RegionInfo", "ParentEstateID", parent_estate_id);
	BOOL teen_grid = (parent_estate_id == 5);  // *TODO add field to estate table and test that
	panel->childSetEnabled("access_combo", gAgent.isGodlike() || (region && region->canManageEstate() && !teen_grid));
	

	// DEBUG PANEL
	panel = LLUICtrlFactory::getPanelByName(tab, "Debug");
	if(!panel) return;

	panel->childSetValue("region_text", LLSD(sim_name) );
	panel->childSetValue("disable_scripts_check", LLSD((BOOL)(region_flags & REGION_FLAGS_SKIP_SCRIPTS)) );
	panel->childSetValue("disable_collisions_check", LLSD((BOOL)(region_flags & REGION_FLAGS_SKIP_COLLISIONS)) );
	panel->childSetValue("disable_physics_check", LLSD((BOOL)(region_flags & REGION_FLAGS_SKIP_PHYSICS)) );

	// TERRAIN PANEL
	panel = LLUICtrlFactory::getPanelByName(tab, "Terrain");
	if(!panel) return;

	panel->childSetValue("region_text", LLSD(sim_name));
	panel->childSetValue("water_height_spin", LLSD(water_height));
	panel->childSetValue("terrain_raise_spin", LLSD(terrain_raise_limit));
	panel->childSetValue("terrain_lower_spin", LLSD(terrain_lower_limit));
	panel->childSetValue("use_estate_sun_check", LLSD(use_estate_sun));

	BOOL allow_modify = gAgent.isGodlike() || (region && region->canManageEstate());
	panel->childSetValue("fixed_sun_check", LLSD((BOOL)(region_flags & REGION_FLAGS_SUN_FIXED)));
	panel->childSetEnabled("fixed_sun_check", allow_modify && !use_estate_sun);
	panel->childSetValue("sun_hour_slider", LLSD(sun_hour));
	panel->childSetEnabled("sun_hour_slider", allow_modify && !use_estate_sun);
}

// static
LLPanelEstateInfo* LLFloaterRegionInfo::getPanelEstate()
{
	LLFloaterRegionInfo* floater = LLFloaterRegionInfo::getInstance();
	if (!floater) return NULL;
	LLTabContainerCommon* tab = LLUICtrlFactory::getTabContainerByName(floater, "region_panels");
	if (!tab) return NULL;
	LLPanelEstateInfo* panel = (LLPanelEstateInfo*)LLUICtrlFactory::getPanelByName(tab,"Estate");
	return panel;
}

// static
LLPanelEstateCovenant* LLFloaterRegionInfo::getPanelCovenant()
{
	LLFloaterRegionInfo* floater = LLFloaterRegionInfo::getInstance();
	if (!floater) return NULL;
	LLTabContainerCommon* tab = LLUICtrlFactory::getTabContainerByName(floater, "region_panels");
	if (!tab) return NULL;
	LLPanelEstateCovenant* panel = (LLPanelEstateCovenant*)LLUICtrlFactory::getPanelByName(tab, "Covenant");
	return panel;
}

void LLFloaterRegionInfo::refreshFromRegion(LLViewerRegion* region)
{
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

// static
void LLPanelRegionInfo::onBtnSet(void* user_data)
{
	LLPanelRegionInfo* panel = (LLPanelRegionInfo*)user_data;
	if(!panel) return;
	if (panel->sendUpdate())
	{
		panel->disableButton("apply_btn");
	}
}

//static 
void LLPanelRegionInfo::onChangeChildCtrl(LLUICtrl* ctrl, void* user_data)
{
	if (ctrl)
	{
		LLPanelRegionInfo* panel = (LLPanelRegionInfo*) ctrl->getParent();
		panel->updateChild(ctrl);
	}
}

// static
// Enables the "set" button if it is not already enabled
void LLPanelRegionInfo::onChangeAnything(LLUICtrl* ctrl, void* user_data)
{
	LLPanelRegionInfo* panel = (LLPanelRegionInfo*)user_data;
	if(panel)
	{
		panel->enableButton("apply_btn");
		panel->refresh();
	}
}

// virtual
BOOL LLPanelRegionInfo::postBuild()
{
	childSetAction("apply_btn", onBtnSet, this);
	childDisable("apply_btn");
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
	const char* request,
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
			msg->addString("Parameter", (*it).c_str());
		}
	}
	msg->sendReliable(mHost);
}

void LLPanelRegionInfo::enableButton(const char* btn_name, BOOL enable)
{
	childSetEnabled(btn_name, enable);
}

void LLPanelRegionInfo::disableButton(const char* btn_name)
{
	childDisable(btn_name);
}

void LLPanelRegionInfo::initCtrl(const char* name)
{
	childSetCommitCallback(name, onChangeAnything, this);
}

void LLPanelRegionInfo::initHelpBtn(const char* name, const char* xml_alert)
{
	childSetAction(name, onClickHelp, (void*)xml_alert);
}

// static
void LLPanelRegionInfo::onClickHelp(void* data)
{
	const char* xml_alert = (const char*)data;
	gViewerWindow->alertXml(xml_alert);
}

/////////////////////////////////////////////////////////////////////////////
// LLPanelRegionGeneralInfo
//
bool LLPanelRegionGeneralInfo::refreshFromRegion(LLViewerRegion* region)
{
	BOOL allow_modify = gAgent.isGodlike() || (region && region->canManageEstate());
	setCtrlsEnabled(allow_modify);
	childDisable("apply_btn");
	childSetEnabled("access_text", allow_modify);
	// childSetEnabled("access_combo", allow_modify);
	// now set in processRegionInfo for teen grid detection
	childSetEnabled("kick_btn", allow_modify);
	childSetEnabled("kick_all_btn", allow_modify);
	childSetEnabled("im_btn", allow_modify);
	childSetEnabled("manage_telehub_btn", allow_modify);

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

	initHelpBtn("terraform_help",		"HelpRegionBlockTerraform");
	initHelpBtn("fly_help",				"HelpRegionBlockFly");
	initHelpBtn("damage_help",			"HelpRegionAllowDamage");
	initHelpBtn("agent_limit_help",		"HelpRegionAgentLimit");
	initHelpBtn("object_bonus_help",	"HelpRegionObjectBonus");
	initHelpBtn("access_help",			"HelpRegionMaturity");
	initHelpBtn("restrict_pushobject_help",		"HelpRegionRestrictPushObject");
	initHelpBtn("land_resell_help",	"HelpRegionLandResell");
	initHelpBtn("parcel_changes_help", "HelpParcelChanges");
	initHelpBtn("parcel_search_help", "HelpRegionSearch");

	childSetAction("kick_btn", onClickKick, this);
	childSetAction("kick_all_btn", onClickKickAll, this);
	childSetAction("im_btn", onClickMessage, this);
	childSetAction("manage_telehub_btn", onClickManageTelehub, this);

	return LLPanelRegionInfo::postBuild();
}

// static
void LLPanelRegionGeneralInfo::onClickKick(void* userdata)
{
	llinfos << "LLPanelRegionGeneralInfo::onClickKick" << llendl;
	LLPanelRegionGeneralInfo* panelp = (LLPanelRegionGeneralInfo*)userdata;

	// this depends on the grandparent view being a floater
	// in order to set up floater dependency
	LLFloater* parent_floater = gFloaterView->getParentFloater(panelp);
	LLFloater* child_floater = LLFloaterAvatarPicker::show(onKickCommit, userdata, FALSE, TRUE);
	parent_floater->addDependentFloater(child_floater);
}

// static
void LLPanelRegionGeneralInfo::onKickCommit(const std::vector<std::string>& names, const std::vector<LLUUID>& ids, void* userdata)
{
	if (names.empty() || ids.empty()) return;
	if(ids[0].notNull())
	{
		LLPanelRegionGeneralInfo* self = (LLPanelRegionGeneralInfo*)userdata;
		if(!self) return;
		strings_t strings;
		// [0] = our agent id
		// [1] = target agent id
		char buffer[MAX_STRING];		/* Flawfinder: ignore*/
		gAgent.getID().toString(buffer);
		strings.push_back(buffer);

		ids[0].toString(buffer);
		strings.push_back(strings_t::value_type(buffer));

		LLUUID invoice(LLFloaterRegionInfo::getLastInvoice());
		self->sendEstateOwnerMessage(gMessageSystem, "teleporthomeuser", invoice, strings);
	}
}

// static
void LLPanelRegionGeneralInfo::onClickKickAll(void* userdata)
{
	llinfos << "LLPanelRegionGeneralInfo::onClickKickAll" << llendl;
	gViewerWindow->alertXml("KickUsersFromRegion", onKickAllCommit, userdata);
}

// static
void LLPanelRegionGeneralInfo::onKickAllCommit(S32 option, void* userdata)
{
	if (option == 0)
	{
		LLPanelRegionGeneralInfo* self = (LLPanelRegionGeneralInfo*)userdata;
		if(!self) return;
		strings_t strings;
		// [0] = our agent id
		char buffer[MAX_STRING];		/* Flawfinder: ignore*/
		gAgent.getID().toString(buffer);
		strings.push_back(buffer);

		LLUUID invoice(LLFloaterRegionInfo::getLastInvoice());
		// historical message name
		self->sendEstateOwnerMessage(gMessageSystem, "teleporthomeallusers", invoice, strings);
	}
}

// static
void LLPanelRegionGeneralInfo::onClickMessage(void* userdata)
{
	llinfos << "LLPanelRegionGeneralInfo::onClickMessage" << llendl;
	gViewerWindow->alertXmlEditText("MessageRegion", LLString::format_map_t(),
									NULL, NULL,
									onMessageCommit, userdata);
}

// static
void LLPanelRegionGeneralInfo::onMessageCommit(S32 option, const LLString& text, void* userdata)
{
	if(option != 0) return;
	if(text.empty()) return;
	LLPanelRegionGeneralInfo* self = (LLPanelRegionGeneralInfo*)userdata;
	if(!self) return;
	llinfos << "Message to everyone: " << text << llendl;
	strings_t strings;
	// [0] grid_x, unused here
	// [1] grid_y, unused here
	// [2] agent_id of sender
	// [3] sender name
	// [4] message
	strings.push_back("-1");
	strings.push_back("-1");
	char buffer[MAX_STRING];		/* Flawfinder: ignore*/
	gAgent.getID().toString(buffer);
	strings.push_back(buffer);
	std::string name;
	gAgent.buildFullname(name);
	strings.push_back(strings_t::value_type(name));
	strings.push_back(strings_t::value_type(text));
	LLUUID invoice(LLFloaterRegionInfo::getLastInvoice());
	self->sendEstateOwnerMessage(gMessageSystem, "simulatormessage", invoice, strings);
}

// static
void LLPanelRegionGeneralInfo::onClickManageTelehub(void* data)
{
	LLFloaterRegionInfo::getInstance()->close();

	LLFloaterTelehub::show();
}

// setregioninfo
// strings[0] = 'Y' - block terraform, 'N' - not
// strings[1] = 'Y' - block fly, 'N' - not
// strings[2] = 'Y' - allow damage, 'N' - not
// strings[3] = 'Y' - allow land sale, 'N' - not
// strings[4] = agent limit
// strings[5] = object bonus
// strings[6] = sim access (0 = unknown, 13 = PG, 21 = Mature)
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
		body["block_terraform"] = childGetValue("block_terraform_check");
		body["block_fly"] = childGetValue("block_fly_check");
		body["allow_damage"] = childGetValue("allow_damage_check");
		body["allow_land_resell"] = childGetValue("allow_land_resell_check");
		body["agent_limit"] = childGetValue("agent_limit_spin");
		body["prim_bonus"] = childGetValue("object_bonus_spin");
		// the combo box stores strings "Mature" and "PG", but we have to convert back to a number, 
		// because the sim doesn't know from strings for this stuff
		body["sim_access"] = LLViewerRegion::stringToAccess(childGetValue("access_combo").asString().c_str());
		body["restrict_pushobject"] = childGetValue("restrict_pushobject");
		body["allow_parcel_changes"] = childGetValue("allow_parcel_changes_check");
		body["block_parcel_search"] = childGetValue("block_parcel_search_check");

		LLHTTPClient::post(url, body, new LLHTTPClient::Responder());
	}
	else
	{
		strings_t strings;
		char buffer[MAX_STRING];		/* Flawfinder: ignore*/

		snprintf(buffer, MAX_STRING, "%s", (childGetValue("block_terraform_check").asBoolean() ? "Y" : "N"));			/* Flawfinder: ignore */
		strings.push_back(strings_t::value_type(buffer));

		snprintf(buffer, MAX_STRING, "%s", (childGetValue("block_fly_check").asBoolean() ? "Y" : "N"));			/* Flawfinder: ignore */
		strings.push_back(strings_t::value_type(buffer));

		snprintf(buffer, MAX_STRING, "%s", (childGetValue("allow_damage_check").asBoolean() ? "Y" : "N"));			/* Flawfinder: ignore */
		strings.push_back(strings_t::value_type(buffer));

		snprintf(buffer, MAX_STRING, "%s", (childGetValue("allow_land_resell_check").asBoolean() ? "Y" : "N"));			/* Flawfinder: ignore */
		strings.push_back(strings_t::value_type(buffer));

		F32 value = (F32)childGetValue("agent_limit_spin").asReal();
		snprintf(buffer, MAX_STRING, "%f", value);			/* Flawfinder: ignore */
		strings.push_back(strings_t::value_type(buffer));

		value = (F32)childGetValue("object_bonus_spin").asReal();
		snprintf(buffer, MAX_STRING, "%f", value);			/* Flawfinder: ignore */
		strings.push_back(strings_t::value_type(buffer));

		U8 access = LLViewerRegion::stringToAccess(childGetValue("access_combo").asString().c_str());
		snprintf(buffer, MAX_STRING, "%d", (S32)access);			/* Flawfinder: ignore */
		strings.push_back(strings_t::value_type(buffer));

		snprintf(buffer, MAX_STRING, "%s", (childGetValue("restrict_pushobject").asBoolean() ? "Y" : "N"));			/* Flawfinder: ignore */
		strings.push_back(strings_t::value_type(buffer));

		snprintf(buffer, MAX_STRING, "%s", (childGetValue("allow_parcel_changes_check").asBoolean() ? "Y" : "N"));			/* Flawfinder: ignore */
		strings.push_back(strings_t::value_type(buffer));

		LLUUID invoice(LLFloaterRegionInfo::getLastInvoice());
		sendEstateOwnerMessage(gMessageSystem, "setregioninfo", invoice, strings);

		LLViewerRegion* region = gAgent.getRegion();
		if (region
			&& access != region->getSimAccess() )		/* Flawfinder: ignore */
		{
			gViewerWindow->alertXml("RegionMaturityChange");
		}
	}


	//integers_t integers;


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

	initHelpBtn("disable_scripts_help",		"HelpRegionDisableScripts");
	initHelpBtn("disable_collisions_help",	"HelpRegionDisableCollisions");
	initHelpBtn("disable_physics_help",		"HelpRegionDisablePhysics");
	initHelpBtn("top_colliders_help",		"HelpRegionTopColliders");
	initHelpBtn("top_scripts_help",			"HelpRegionTopScripts");
	initHelpBtn("restart_help",				"HelpRegionRestart");

	childSetAction("choose_avatar_btn", onClickChooseAvatar, this);
	childSetAction("return_scripted_other_land_btn", onClickReturnScriptedOtherLand, this);
	childSetAction("return_scripted_all_btn", onClickReturnScriptedAll, this);
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
	childDisable("apply_btn");

	childSetEnabled("choose_avatar_btn", allow_modify);
	childSetEnabled("return_scripted_other_land_btn", allow_modify && !mTargetAvatar.isNull());
	childSetEnabled("return_scripted_all_btn", allow_modify && !mTargetAvatar.isNull());
	childSetEnabled("top_colliders_btn", allow_modify);
	childSetEnabled("top_scripts_btn", allow_modify);
	childSetEnabled("restart_btn", allow_modify);
	childSetEnabled("cancel_restart_btn", allow_modify);

	return LLPanelRegionInfo::refreshFromRegion(region);
}

// virtual
BOOL LLPanelRegionDebugInfo::sendUpdate()
{
	llinfos << "LLPanelRegionDebugInfo::sendUpdate" << llendl;
	strings_t strings;
	char buffer[MAX_STRING];		/* Flawfinder: ignore */

	snprintf(buffer, MAX_STRING, "%s", (childGetValue("disable_scripts_check").asBoolean() ? "Y" : "N"));			/* Flawfinder: ignore */
	strings.push_back(buffer);

	snprintf(buffer, MAX_STRING, "%s", (childGetValue("disable_collisions_check").asBoolean() ? "Y" : "N"));			/* Flawfinder: ignore */
	strings.push_back(buffer);

	snprintf(buffer, MAX_STRING, "%s", (childGetValue("disable_physics_check").asBoolean() ? "Y" : "N"));			/* Flawfinder: ignore */
	strings.push_back(buffer);

	LLUUID invoice(LLFloaterRegionInfo::getLastInvoice());
	sendEstateOwnerMessage(gMessageSystem, "setregiondebug", invoice, strings);
	return TRUE;
}

void LLPanelRegionDebugInfo::onClickChooseAvatar(void* data)
{
	LLFloaterAvatarPicker::show(callbackAvatarID, data, FALSE, TRUE);
}

// static
void LLPanelRegionDebugInfo::callbackAvatarID(const std::vector<std::string>& names, const std::vector<LLUUID>& ids, void* data)
{
	LLPanelRegionDebugInfo* self = (LLPanelRegionDebugInfo*) data;
	if (ids.empty() || names.empty()) return;
	self->mTargetAvatar = ids[0];
	self->childSetValue("target_avatar_name", LLSD(names[0]));
	self->refreshFromRegion( gAgent.getRegion() );
}

// static
void LLPanelRegionDebugInfo::onClickReturnScriptedOtherLand(void* data)
{
	LLPanelRegionDebugInfo* panelp = (LLPanelRegionDebugInfo*) data;
	if (panelp->mTargetAvatar.isNull()) return;

	LLString::format_map_t args;
	args["[USER_NAME]"] = panelp->childGetValue("target_avatar_name").asString();
	gViewerWindow->alertXml("ReturnScriptedOnOthersLand", args, callbackReturnScriptedOtherLand, data);
}

// static
void LLPanelRegionDebugInfo::callbackReturnScriptedOtherLand( S32 option, void* userdata )
{
	if (option != 0) return;

	LLPanelRegionDebugInfo* self = (LLPanelRegionDebugInfo*) userdata;
	if (!self->mTargetAvatar.isNull())
	{
		U32 flags = 0;
		flags = flags | SWD_OTHERS_LAND_ONLY;
		flags = flags | SWD_ALWAYS_RETURN_OBJECTS;
		flags |= SWD_SCRIPTED_ONLY;

		send_sim_wide_deletes(self->mTargetAvatar, flags);
	}
}

// static
void LLPanelRegionDebugInfo::onClickReturnScriptedAll(void* data)
{
	LLPanelRegionDebugInfo* panelp = (LLPanelRegionDebugInfo*) data;
	if (panelp->mTargetAvatar.isNull()) return;
	
	
	LLString::format_map_t args;
	args["[USER_NAME]"] = panelp->childGetValue("target_avatar_name").asString();
	gViewerWindow->alertXml("ReturnScriptedOnAllLand", args, callbackReturnScriptedAll, data);
}

// static
void LLPanelRegionDebugInfo::callbackReturnScriptedAll( S32 option, void* userdata )
{
	if (option != 0) return;

	LLPanelRegionDebugInfo* self = (LLPanelRegionDebugInfo*) userdata;
	if (!self->mTargetAvatar.isNull())
	{
		U32 flags = 0;
		flags |= SWD_ALWAYS_RETURN_OBJECTS;
		flags |= SWD_SCRIPTED_ONLY;

		send_sim_wide_deletes(self->mTargetAvatar, flags);
	}
}

// static
void LLPanelRegionDebugInfo::onClickTopColliders(void* data)
{
	LLPanelRegionDebugInfo* self = (LLPanelRegionDebugInfo*)data;
	strings_t strings;
	strings.push_back("1");	// one physics step
	LLUUID invoice(LLFloaterRegionInfo::getLastInvoice());
	LLFloaterTopObjects::show();
	LLFloaterTopObjects::clearList();
	self->sendEstateOwnerMessage(gMessageSystem, "colliders", invoice, strings);
}

// static
void LLPanelRegionDebugInfo::onClickTopScripts(void* data)
{
	LLPanelRegionDebugInfo* self = (LLPanelRegionDebugInfo*)data;
	strings_t strings;
	strings.push_back("6");	// top 5 scripts
	LLUUID invoice(LLFloaterRegionInfo::getLastInvoice());
	LLFloaterTopObjects::show();
	LLFloaterTopObjects::clearList();
	self->sendEstateOwnerMessage(gMessageSystem, "scripts", invoice, strings);
}

// static
void LLPanelRegionDebugInfo::onClickRestart(void* data)
{
	gViewerWindow->alertXml("ConfirmRestart", callbackRestart, data);
}

// static
void LLPanelRegionDebugInfo::callbackRestart(S32 option, void* data)
{
	if (option != 0) return;

	LLPanelRegionDebugInfo* self = (LLPanelRegionDebugInfo*)data;
	strings_t strings;
	strings.push_back("120");
	LLUUID invoice(LLFloaterRegionInfo::getLastInvoice());
	self->sendEstateOwnerMessage(gMessageSystem, "restart", invoice, strings);
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
	childDisable("apply_btn");

	if (region)
	{
		childSetValue("region_text", LLSD(region->getName()));
	}
	else
	{
		childSetValue("region_text", LLSD(""));
	}

	if (!region) return LLPanelRegionInfo::refreshFromRegion(region);

	LLVLComposition* compp = region->getComposition();
	LLTextureCtrl* texture_ctrl;
	char buffer[MAX_STRING];		/* Flawfinder: ignore */
	for(S32 i = 0; i < TERRAIN_TEXTURE_COUNT; ++i)
	{
		snprintf(buffer, MAX_STRING, "texture_detail_%d", i);			/* Flawfinder: ignore */
		texture_ctrl = LLViewerUICtrlFactory::getTexturePickerByName(this, buffer);
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
		snprintf(buffer, MAX_STRING, "height_start_spin_%d", i);			/* Flawfinder: ignore */
		childSetValue(buffer, LLSD(compp->getStartHeight(i)));
		snprintf(buffer, MAX_STRING, "height_range_spin_%d", i);		/* Flawfinder: ignore */
		childSetValue(buffer, LLSD(compp->getHeightRange(i)));
	}

	// Call the parent for common book-keeping
	return LLPanelRegionInfo::refreshFromRegion(region);
}


BOOL LLPanelRegionTextureInfo::postBuild()
{
	LLPanelRegionInfo::postBuild();
	char buffer[MAX_STRING];		/* Flawfinder: ignore */
	for(S32 i = 0; i < TERRAIN_TEXTURE_COUNT; ++i)
	{
		snprintf(buffer, MAX_STRING, "texture_detail_%d", i);			/* Flawfinder: ignore */
		initCtrl(buffer);
	}

	for(S32 i = 0; i < CORNER_COUNT; ++i)
	{
		snprintf(buffer, MAX_STRING, "height_start_spin_%d", i);			/* Flawfinder: ignore */
		initCtrl(buffer);
		snprintf(buffer, MAX_STRING, "height_range_spin_%d", i);			/* Flawfinder: ignore */
		initCtrl(buffer);
	}

//	LLButton* btn = new LLButton("dump", LLRect(0, 20, 100, 0), "", onClickDump, this);
//	btn->setFollows(FOLLOWS_TOP|FOLLOWS_LEFT);
//	addChild(btn);

	return LLPanelRegionInfo::postBuild();
}

void LLPanelRegionTextureInfo::draw()
{
	if(getVisible())
	{
		LLPanel::draw();
	}
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
	char buffer[MAX_STRING];		/* Flawfinder: ignore */
	char buffer2[MAX_STRING];		/* Flawfinder: ignore */
	char id_str[UUID_STR_LENGTH];	/* Flawfinder: ignore */
	LLMessageSystem* msg = gMessageSystem;
	strings_t strings;

	LLUUID invoice(LLFloaterRegionInfo::getLastInvoice());
	
	for(S32 i = 0; i < TERRAIN_TEXTURE_COUNT; ++i)
	{
		snprintf(buffer, MAX_STRING, "texture_detail_%d", i);			/* Flawfinder: ignore */
		texture_ctrl = LLViewerUICtrlFactory::getTexturePickerByName(this, buffer);
		if(texture_ctrl)
		{
			LLUUID tmp_id(texture_ctrl->getImageAssetID());
			tmp_id.toString(id_str);
			snprintf(buffer, MAX_STRING, "%d %s", i, id_str);			/* Flawfinder: ignore */
			strings.push_back(strings_t::value_type(buffer));
		}
	}
	sendEstateOwnerMessage(msg, "texturedetail", invoice, strings);
	strings.clear();
	for(S32 i = 0; i < CORNER_COUNT; ++i)
	{
		snprintf(buffer, MAX_STRING, "height_start_spin_%d", i);			/* Flawfinder: ignore */
		snprintf(buffer2, MAX_STRING, "height_range_spin_%d", i);			/* Flawfinder: ignore */
		snprintf(buffer, MAX_STRING, "%d %f %f", i, (F32)childGetValue(buffer).asReal(), (F32)childGetValue(buffer2).asReal());			/* Flawfinder: ignore */
		strings.push_back(strings_t::value_type(buffer));
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
		char buffer[MAX_STRING];		/* Flawfinder: ignore */
		snprintf(buffer, MAX_STRING, "texture_detail_%d", i);			/* Flawfinder: ignore */
		LLTextureCtrl* texture_ctrl = LLViewerUICtrlFactory::getTexturePickerByName(this, buffer);
		if (!texture_ctrl) continue;

		LLUUID image_asset_id = texture_ctrl->getImageAssetID();
		LLViewerImage* img = gImageList.getImage(image_asset_id);
		S32 components = img->getComponents();
		// Must ask for highest resolution version's width. JC
		S32 width = img->getWidth(0);
		S32 height = img->getHeight(0);

		//llinfos << "texture detail " << i << " is " << width << "x" << height << "x" << components << llendl;

		if (components != 3)
		{
			LLString::format_map_t args;
			args["[TEXTURE_NUM]"] = llformat("%d",i+1);
			args["[TEXTURE_BIT_DEPTH]"] = llformat("%d",components * 8);
			gViewerWindow->alertXml("InvalidTerrainBitDepth", args);
			return FALSE;
		}

		if (width > 512 || height > 512)
		{

			LLString::format_map_t args;
			args["[TEXTURE_NUM]"] = i+1;
			args["[TEXTURE_SIZE_X]"] = llformat("%d",width);
			args["[TEXTURE_SIZE_Y]"] = llformat("%d",height);
			gViewerWindow->alertXml("InvalidTerrainSize", args);
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

	initHelpBtn("water_height_help",	"HelpRegionWaterHeight");
	initHelpBtn("terrain_raise_help",	"HelpRegionTerrainRaise");
	initHelpBtn("terrain_lower_help",	"HelpRegionTerrainLower");
	initHelpBtn("upload_raw_help",		"HelpRegionUploadRaw");
	initHelpBtn("download_raw_help",	"HelpRegionDownloadRaw");
	initHelpBtn("use_estate_sun_help",	"HelpRegionUseEstateSun");
	initHelpBtn("fixed_sun_help",		"HelpRegionFixedSun");
	initHelpBtn("bake_terrain_help",	"HelpRegionBakeTerrain");

	initCtrl("water_height_spin");
	initCtrl("terrain_raise_spin");
	initCtrl("terrain_lower_spin");

	initCtrl("fixed_sun_check");
	childSetCommitCallback("fixed_sun_check", onChangeFixedSun, this);
	childSetCommitCallback("use_estate_sun_check", onChangeUseEstateTime, this);
	childSetCommitCallback("sun_hour_slider", onChangeSunHour, this);

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
	childDisable("apply_btn");

	childSetEnabled("download_raw_btn", owner_or_god);
	childSetEnabled("upload_raw_btn", owner_or_god);
	childSetEnabled("bake_terrain_btn", owner_or_god);

	return LLPanelRegionInfo::refreshFromRegion(region);
}

// virtual
BOOL LLPanelRegionTerrainInfo::sendUpdate()
{
	llinfos << "LLPanelRegionTerrainInfo::sendUpdate" << llendl;
	char buffer[MAX_STRING];		/* Flawfinder: ignore */
	strings_t strings;
	LLUUID invoice(LLFloaterRegionInfo::getLastInvoice());

	snprintf(buffer, MAX_STRING, "%f", (F32)childGetValue("water_height_spin").asReal());			/* Flawfinder: ignore */
	strings.push_back(buffer);
	snprintf(buffer, MAX_STRING, "%f", (F32)childGetValue("terrain_raise_spin").asReal());			/* Flawfinder: ignore */
	strings.push_back(buffer);
	snprintf(buffer, MAX_STRING, "%f", (F32)childGetValue("terrain_lower_spin").asReal());			/* Flawfinder: ignore */
	strings.push_back(buffer);
	snprintf(buffer, MAX_STRING, "%s", (childGetValue("use_estate_sun_check").asBoolean() ? "Y" : "N"));			/* Flawfinder: ignore */
	strings.push_back(buffer);
	snprintf(buffer, MAX_STRING, "%s", (childGetValue("fixed_sun_check").asBoolean() ? "Y" : "N"));			/* Flawfinder: ignore */
	strings.push_back(buffer);
	snprintf(buffer, MAX_STRING, "%f", (F32)childGetValue("sun_hour_slider").asReal() );			/* Flawfinder: ignore */
	strings.push_back(buffer);

	// Grab estate information in case the user decided to set the
	// region back to estate time. JC
	LLFloaterRegionInfo* floater = LLFloaterRegionInfo::getInstance();
	if (!floater) return true;

	LLTabContainerCommon* tab = LLUICtrlFactory::getTabContainerByName(floater, "region_panels");
	if (!tab) return true;

	LLPanelEstateInfo* panel = (LLPanelEstateInfo*)LLUICtrlFactory::getPanelByName(tab, "Estate");
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

	snprintf(buffer, MAX_STRING, "%s", (estate_global_time ? "Y" : "N") );			/* Flawfinder: ignore */
	strings.push_back(buffer);
	snprintf(buffer, MAX_STRING, "%s", (estate_fixed_sun ? "Y" : "N") );			/* Flawfinder: ignore */
	strings.push_back(buffer);
	snprintf(buffer, MAX_STRING, "%f", estate_sun_hour);			/* Flawfinder: ignore */
	strings.push_back(buffer);

	sendEstateOwnerMessage(gMessageSystem, "setregionterrain", invoice, strings);
	return TRUE;
}

// static 
void LLPanelRegionTerrainInfo::onChangeUseEstateTime(LLUICtrl* ctrl, void* user_data)
{
	LLPanelRegionTerrainInfo* panel = (LLPanelRegionTerrainInfo*) user_data;
	if (!panel) return;
	BOOL use_estate_sun = panel->childGetValue("use_estate_sun_check").asBoolean();
	panel->childSetEnabled("fixed_sun_check", !use_estate_sun);
	panel->childSetEnabled("sun_hour_slider", !use_estate_sun);
	if (use_estate_sun)
	{
		panel->childSetValue("fixed_sun_check", LLSD(FALSE));
		panel->childSetValue("sun_hour_slider", LLSD(0.f));
	}
	panel->childEnable("apply_btn");
}

// static 
void LLPanelRegionTerrainInfo::onChangeFixedSun(LLUICtrl* ctrl, void* user_data)
{
	LLPanelRegionTerrainInfo* panel = (LLPanelRegionTerrainInfo*) user_data;
	if (!panel) return;
	// Just enable the apply button.  We let the sun-hour slider be enabled
	// for both fixed-sun and non-fixed-sun. JC
	panel->childEnable("apply_btn");
}

// static 
void LLPanelRegionTerrainInfo::onChangeSunHour(LLUICtrl* ctrl, void*)
{
	// can't use userdata to get panel, slider uses it internally
	LLPanelRegionTerrainInfo* panel = (LLPanelRegionTerrainInfo*) ctrl->getParent();
	if (!panel) return;
	panel->childEnable("apply_btn");
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
	LLString filepath = picker.getFirstFile();

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
	LLString filepath = picker.getFirstFile();

	LLPanelRegionTerrainInfo* self = (LLPanelRegionTerrainInfo*)data;
	strings_t strings;
	strings.push_back("upload filename");
	strings.push_back(filepath);
	LLUUID invoice(LLFloaterRegionInfo::getLastInvoice());
	self->sendEstateOwnerMessage(gMessageSystem, "terrain", invoice, strings);

	gViewerWindow->alertXml("RawUploadStarted");
}

// static
void LLPanelRegionTerrainInfo::onClickBakeTerrain(void* data)
{
	gViewerWindow->alertXml("ConfirmBakeTerrain", 
							 callbackBakeTerrain, data);
}

// static
void LLPanelRegionTerrainInfo::callbackBakeTerrain(S32 option, void* data)
{
	if (option != 0) return;

	LLPanelRegionTerrainInfo* self = (LLPanelRegionTerrainInfo*)data;
	strings_t strings;
	strings.push_back("bake");
	LLUUID invoice(LLFloaterRegionInfo::getLastInvoice());
	self->sendEstateOwnerMessage(gMessageSystem, "terrain", invoice, strings);
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

// static
// Disables the sun-hour slider and the use fixed time check if the use global time is check
void LLPanelEstateInfo::onChangeUseGlobalTime(LLUICtrl* ctrl, void* user_data)
{
	LLPanelEstateInfo* panel = (LLPanelEstateInfo*) user_data;
	if (panel)
	{
		bool enabled = !panel->childGetValue("use_global_time_check").asBoolean();
		panel->childSetEnabled("sun_hour_slider", enabled);
		panel->childSetEnabled("fixed_sun_check", enabled);
		panel->childSetValue("fixed_sun_check", LLSD(FALSE));
		panel->enableButton("apply_btn");
	}
}

// Enables the sun-hour slider if the fixed-sun checkbox is set
void LLPanelEstateInfo::onChangeFixedSun(LLUICtrl* ctrl, void* user_data)
{
	LLPanelEstateInfo* panel = (LLPanelEstateInfo*) user_data;
	if (panel)
	{
		bool enabled = !panel->childGetValue("fixed_sun_check").asBoolean();
		panel->childSetEnabled("use_global_time_check", enabled);
		panel->childSetValue("use_global_time_check", LLSD(FALSE));
		panel->enableButton("apply_btn");
	}
}

//---------------------------------------------------------------------------
// Add/Remove estate access button callbacks
//---------------------------------------------------------------------------

// static
void LLPanelEstateInfo::onClickAddAllowedAgent(void* user_data)
{
	LLPanelEstateInfo* self = (LLPanelEstateInfo*)user_data;
	LLCtrlListInterface *list = self->childGetListInterface("allowed_avatar_name_list");
	if (!list) return;
	if (list->getItemCount() >= ESTATE_MAX_ACCESS_IDS)
	{
		//args

		LLString::format_map_t args;
		args["[MAX_AGENTS]"] = llformat("%d",ESTATE_MAX_ACCESS_IDS);
		gViewerWindow->alertXml("MaxAllowedAgentOnRegion", args);
		return;
	}
	accessAddCore(ESTATE_ACCESS_ALLOWED_AGENT_ADD, "EstateAllowedAgentAdd");
}

// static
void LLPanelEstateInfo::onClickRemoveAllowedAgent(void* user_data)
{
	accessRemoveCore(ESTATE_ACCESS_ALLOWED_AGENT_REMOVE, "EstateAllowedAgentRemove", "allowed_avatar_name_list");
}

// static
void LLPanelEstateInfo::onClickAddAllowedGroup(void* user_data)
{
	LLPanelEstateInfo* self = (LLPanelEstateInfo*)user_data;
	LLCtrlListInterface *list = self->childGetListInterface("allowed_group_name_list");
	if (!list) return;
	if (list->getItemCount() >= ESTATE_MAX_ACCESS_IDS)
	{
		LLString::format_map_t args;
		args["[MAX_GROUPS]"] = llformat("%d",ESTATE_MAX_ACCESS_IDS);
		gViewerWindow->alertXml("MaxAllowedGroupsOnRegion", args);
		return;
	}
	if (isLindenEstate())
	{
		gViewerWindow->alertXml("ChangeLindenAccess", addAllowedGroup, user_data);
	}
	else
	{
		addAllowedGroup(0, user_data);
	}
}

// static
void LLPanelEstateInfo::addAllowedGroup(S32 option, void* user_data)
{
	if (option != 0) return;

	LLPanelEstateInfo* panelp = (LLPanelEstateInfo*)user_data;

	LLFloater* parent_floater = gFloaterView->getParentFloater(panelp);

	LLFloaterGroupPicker* widget;
	widget = LLFloaterGroupPicker::showInstance(LLSD(gAgent.getID()));
	if (widget)
	{
		widget->setSelectCallback(addAllowedGroup2, user_data);
		if (parent_floater)
		{
			LLRect new_rect = gFloaterView->findNeighboringPosition(parent_floater, widget);
			widget->setOrigin(new_rect.mLeft, new_rect.mBottom);
			parent_floater->addDependentFloater(widget);
		}
	}
}

// static
void LLPanelEstateInfo::onClickRemoveAllowedGroup(void* user_data)
{
	accessRemoveCore(ESTATE_ACCESS_ALLOWED_GROUP_REMOVE, "EstateAllowedGroupRemove", "allowed_group_name_list");
}

// static
void LLPanelEstateInfo::onClickAddBannedAgent(void* user_data)
{
	LLPanelEstateInfo* self = (LLPanelEstateInfo*)user_data;
	LLCtrlListInterface *list = self->childGetListInterface("banned_avatar_name_list");
	if (!list) return;
	if (list->getItemCount() >= ESTATE_MAX_ACCESS_IDS)
	{
		LLString::format_map_t args;
		args["[MAX_BANNED]"] = llformat("%d",ESTATE_MAX_ACCESS_IDS);
		gViewerWindow->alertXml("MaxBannedAgentsOnRegion", args);
		return;
	}
	accessAddCore(ESTATE_ACCESS_BANNED_AGENT_ADD, "EstateBannedAgentAdd");
}

// static
void LLPanelEstateInfo::onClickRemoveBannedAgent(void* user_data)
{
	accessRemoveCore(ESTATE_ACCESS_BANNED_AGENT_REMOVE, "EstateBannedAgentRemove", "banned_avatar_name_list");
}

// static
void LLPanelEstateInfo::onClickAddEstateManager(void* user_data)
{
	LLPanelEstateInfo* self = (LLPanelEstateInfo*)user_data;
	LLCtrlListInterface *list = self->childGetListInterface("estate_manager_name_list");
	if (!list) return;
	if (list->getItemCount() >= ESTATE_MAX_MANAGERS)
	{	// Tell user they can't add more managers
		LLString::format_map_t args;
		args["[MAX_MANAGER]"] = llformat("%d",ESTATE_MAX_MANAGERS);
		gViewerWindow->alertXml("MaxManagersOnRegion", args);
	}
	else
	{	// Go pick managers to add
		accessAddCore(ESTATE_ACCESS_MANAGER_ADD, "EstateManagerAdd");
	}
}

// static
void LLPanelEstateInfo::onClickRemoveEstateManager(void* user_data)
{
	accessRemoveCore(ESTATE_ACCESS_MANAGER_REMOVE, "EstateManagerRemove", "estate_manager_name_list");
}

//---------------------------------------------------------------------------
// Kick from estate methods
//---------------------------------------------------------------------------
struct LLKickFromEstateInfo
{
	LLPanelEstateInfo *mEstatePanelp;
	LLString    mDialogName;
	LLUUID      mAgentID;
};

void LLPanelEstateInfo::onClickKickUser(void *user_data)
{
	LLPanelEstateInfo* panelp = (LLPanelEstateInfo*)user_data;

	// this depends on the grandparent view being a floater
	// in order to set up floater dependency
	LLFloater* parent_floater = gFloaterView->getParentFloater(panelp);
	LLFloater* child_floater = LLFloaterAvatarPicker::show(LLPanelEstateInfo::onKickUserCommit, user_data, FALSE, TRUE);
	parent_floater->addDependentFloater(child_floater);
}

void LLPanelEstateInfo::onKickUserCommit(const std::vector<std::string>& names, const std::vector<LLUUID>& ids, void* userdata)
{
	if (names.empty() || ids.empty()) return;
	
	//check to make sure there is one valid user and id
	if( (ids[0].isNull()) ||
		(names[0].length() == 0) )
	{
		return;
	}

	LLPanelEstateInfo* self = (LLPanelEstateInfo*)userdata;
	if(!self) return;

	//keep track of what user they want to kick and other misc info
	LLKickFromEstateInfo *kick_info = new LLKickFromEstateInfo();
	kick_info->mEstatePanelp = self;
	kick_info->mDialogName  = "EstateKickUser";
	kick_info->mAgentID     = ids[0];

	//Bring up a confirmation dialog
	LLString::format_map_t args;
	args["[EVIL_USER]"] = names[0];
	gViewerWindow->alertXml(kick_info->mDialogName, args, LLPanelEstateInfo::kickUserConfirm, (void*)kick_info);

}

void LLPanelEstateInfo::kickUserConfirm(S32 option, void* userdata)
{
	//extract the callback parameter
	LLKickFromEstateInfo *kick_info = (LLKickFromEstateInfo*) userdata;
	if (!kick_info) return;

	LLUUID invoice(LLFloaterRegionInfo::getLastInvoice());
	strings_t strings;
	char buffer[MAX_STRING];		/* Flawfinder: ignore*/

	switch(option)
	{
	case 0:
		//Kick User
		kick_info->mAgentID.toString(buffer);
		strings.push_back(strings_t::value_type(buffer));

		kick_info->mEstatePanelp->sendEstateOwnerMessage(gMessageSystem, "kickestate", invoice, strings);
		break;
	default:
		break;
	}

	delete kick_info;
	kick_info = NULL;
}

//---------------------------------------------------------------------------
// Core Add/Remove estate access methods
//---------------------------------------------------------------------------
std::string all_estates_text()
{
	LLPanelEstateInfo* panel = LLFloaterRegionInfo::getPanelEstate();
	if (!panel) return "(error)";

	std::string owner = panel->getOwnerName();

	LLViewerRegion* region = gAgent.getRegion();
	if (gAgent.isGodlike())
	{
		return llformat("all estates\nowned by %s", owner.c_str());
	}
	else if (region && region->getOwner() == gAgent.getID())
	{
		return "all estates you own";
	}
	else if (region && region->isEstateManager())
	{
		return llformat("all estates that\nyou manage for %s", owner.c_str());
	}
	else
	{
		return "(error)";
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
	U32 mOperationFlag;	// ESTATE_ACCESS_BANNED_AGENT_ADD, _REMOVE, etc.
	LLString mDialogName;
	LLUUID mAgentOrGroupID;
};

// Special case callback for groups, since it has different callback format than names
// static
void LLPanelEstateInfo::addAllowedGroup2(LLUUID id, void* user_data)
{
	LLEstateAccessChangeInfo* change_info = new LLEstateAccessChangeInfo;
	change_info->mOperationFlag = ESTATE_ACCESS_ALLOWED_GROUP_ADD;
	change_info->mDialogName = "EstateAllowedGroupAdd";
	change_info->mAgentOrGroupID = id;

	if (isLindenEstate())
	{
		accessCoreConfirm(0, (void*)change_info);
	}
	else
	{
		LLString::format_map_t args;
		args["[ALL_ESTATES]"] = all_estates_text();
		gViewerWindow->alertXml(change_info->mDialogName, args, accessCoreConfirm, (void*)change_info);
	}
}

// static
void LLPanelEstateInfo::accessAddCore(U32 operation_flag, const char* dialog_name)
{
	LLEstateAccessChangeInfo* change_info = new LLEstateAccessChangeInfo;
	change_info->mOperationFlag = operation_flag;
	change_info->mDialogName = dialog_name;
	// agent id filled in after avatar picker

	if (isLindenEstate())
	{
		gViewerWindow->alertXml("ChangeLindenAccess", accessAddCore2, change_info);
	}
	else
	{
		// same as clicking "OK"
		accessAddCore2(0, change_info);
	}
}

// static
void LLPanelEstateInfo::accessAddCore2(S32 option, void* data)
{
	LLEstateAccessChangeInfo* change_info = (LLEstateAccessChangeInfo*)data;
	if (option != 0)
	{
		// abort change
		delete change_info;
		change_info = NULL;
		return;
	}

	// avatar picker no multi-select, yes close-on-select
	LLFloaterAvatarPicker::show(accessAddCore3, (void*)change_info, FALSE, TRUE);
}

// static
void LLPanelEstateInfo::accessAddCore3(const std::vector<std::string>& names, const std::vector<LLUUID>& ids, void* data)
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
	change_info->mAgentOrGroupID = ids[0];

	// Can't put estate owner on ban list
	LLPanelEstateInfo* panel = LLFloaterRegionInfo::getPanelEstate();
	if (!panel) return;
	LLViewerRegion* region = gAgent.getRegion();
	if (!region) return;

	if ((change_info->mOperationFlag & ESTATE_ACCESS_BANNED_AGENT_ADD)
		&& (region->getOwner() == change_info->mAgentOrGroupID))
	{
		gViewerWindow->alertXml("OwnerCanNotBeDenied");
		delete change_info;
		change_info = NULL;
		return;
	}

	if (isLindenEstate())
	{
		// just apply to this estate
		accessCoreConfirm(0, (void*)change_info);
	}
	else
	{
		// ask if this estate or all estates with this owner
		LLString::format_map_t args;
		args["[ALL_ESTATES]"] = all_estates_text();
		gViewerWindow->alertXml(change_info->mDialogName, args, accessCoreConfirm, (void*)change_info);
	}
}

// static
void LLPanelEstateInfo::accessRemoveCore(U32 operation_flag, const char* dialog_name, const char* list_ctrl_name)
{
	LLPanelEstateInfo* panel = LLFloaterRegionInfo::getPanelEstate();
	if (!panel) return;
	LLNameListCtrl* name_list = LLViewerUICtrlFactory::getNameListByName(panel, list_ctrl_name);
	if (!name_list) return;
	LLScrollListItem* item = name_list->getFirstSelected();
	if (!item) return;
	LLUUID agent_id = item->getUUID();

	LLEstateAccessChangeInfo* change_info = new LLEstateAccessChangeInfo;
	change_info->mAgentOrGroupID = agent_id;
	change_info->mOperationFlag = operation_flag;
	change_info->mDialogName = dialog_name;

	if (isLindenEstate())
	{
		// warn on change linden estate
		gViewerWindow->alertXml("ChangeLindenAccess", 
							   accessRemoveCore2,
							   (void*)change_info);
	}
	else
	{
		// just proceed, as if clicking OK
		accessRemoveCore2(0, (void*)change_info);
	}
}

// static
void LLPanelEstateInfo::accessRemoveCore2(S32 option, void* data)
{
	LLEstateAccessChangeInfo* change_info = (LLEstateAccessChangeInfo*)data;
	if (option != 0)
	{
		// abort
		delete change_info;
		change_info = NULL;
		return;
	}

	// If Linden estate, can only apply to "this" estate, not all estates
	// owned by NULL.
	if (isLindenEstate())
	{
		accessCoreConfirm(0, (void*)change_info);
	}
	else
	{
		LLString::format_map_t args;
		args["[ALL_ESTATES]"] = all_estates_text();
		gViewerWindow->alertXml(change_info->mDialogName, 
							   args, 
							   accessCoreConfirm, 
							   (void*)change_info);
	}
}

// Used for both access add and remove operations, depending on the mOperationFlag
// passed in (ESTATE_ACCESS_BANNED_AGENT_ADD, ESTATE_ACCESS_ALLOWED_AGENT_REMOVE, etc.)
// static
void LLPanelEstateInfo::accessCoreConfirm(S32 option, void* data)
{
	LLEstateAccessChangeInfo* change_info = (LLEstateAccessChangeInfo*)data;
	U32 flags = change_info->mOperationFlag;
	switch(option)
	{
	case 0:
		// This estate
		sendEstateAccessDelta(flags, change_info->mAgentOrGroupID);
		break;
	case 1:
		{
			// All estates, either than I own or manage for this owner.  
			// This will be verified on simulator. JC
			LLViewerRegion* region = gAgent.getRegion();
			if (!region) break;
			if (region->getOwner() == gAgent.getID()
				|| gAgent.isGodlike())
			{
				flags |= ESTATE_ACCESS_APPLY_TO_ALL_ESTATES;
				sendEstateAccessDelta(flags, change_info->mAgentOrGroupID);
			}
			else if (region->isEstateManager())
			{
				flags |= ESTATE_ACCESS_APPLY_TO_MANAGED_ESTATES;
				sendEstateAccessDelta(flags, change_info->mAgentOrGroupID);
			}
			break;
		}
	case 2:
	default:
		break;
	}
	delete change_info;
	change_info = NULL;
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

	char buf[MAX_STRING];		/* Flawfinder: ignore*/
	gAgent.getID().toString(buf);
	msg->nextBlock("ParamList");
	msg->addString("Parameter", buf);

	snprintf(buf, MAX_STRING, "%u", flags);			/* Flawfinder: ignore */
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


bool LLPanelEstateInfo::refreshFromRegion(LLViewerRegion* region)
{
	BOOL god = gAgent.isGodlike();
	BOOL owner = (region && (region->getOwner() == gAgent.getID()));
	BOOL manager = (region && region->isEstateManager());
	setCtrlsEnabled(god || owner || manager);
	
	childDisable("apply_btn");
	childSetEnabled("add_allowed_avatar_btn",		god || owner || manager);
	childSetEnabled("remove_allowed_avatar_btn",	god || owner || manager);
	childSetEnabled("add_allowed_group_btn",		god || owner || manager);
	childSetEnabled("remove_allowed_group_btn",		god || owner || manager);
	childSetEnabled("add_banned_avatar_btn",		god || owner || manager);
	childSetEnabled("remove_banned_avatar_btn",		god || owner || manager);
	childSetEnabled("message_estate_btn",			god || owner || manager);
	childSetEnabled("kick_user_from_estate_btn",	god || owner || manager);

	// estate managers can't add estate managers
	childSetEnabled("add_estate_manager_btn",		god || owner);
	childSetEnabled("remove_estate_manager_btn",	god || owner);
	childSetEnabled("estate_manager_name_list",		god || owner);

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

	initHelpBtn("estate_manager_help",			"HelpEstateEstateManager");
	initHelpBtn("use_global_time_help",			"HelpEstateUseGlobalTime");
	initHelpBtn("fixed_sun_help",				"HelpEstateFixedSun");
	initHelpBtn("externally_visible_help",		"HelpEstateExternallyVisible");
	initHelpBtn("allow_direct_teleport_help",	"HelpEstateAllowDirectTeleport");
	initHelpBtn("allow_resident_help",			"HelpEstateAllowResident");
	initHelpBtn("allow_group_help",				"HelpEstateAllowGroup");
	initHelpBtn("ban_resident_help",			"HelpEstateBanResident");
	initHelpBtn("voice_chat_help", "HelpEstateVoiceChat");

	// set up the use global time checkbox
	childSetCommitCallback("use_global_time_check", onChangeUseGlobalTime, this);
	childSetCommitCallback("fixed_sun_check", onChangeFixedSun, this);
	childSetCommitCallback("sun_hour_slider", onChangeChildCtrl, this);

	childSetCommitCallback("allowed_avatar_name_list", onChangeChildCtrl, this);
	LLNameListCtrl *avatar_name_list = LLViewerUICtrlFactory::getNameListByName(this, "allowed_avatar_name_list");
	if (avatar_name_list)
	{
		avatar_name_list->setCommitOnSelectionChange(TRUE);
		avatar_name_list->setMaxItemCount(ESTATE_MAX_ACCESS_IDS);
	}

	childSetAction("add_allowed_avatar_btn", onClickAddAllowedAgent, this);
	childSetAction("remove_allowed_avatar_btn", onClickRemoveAllowedAgent, this);

	childSetCommitCallback("allowed_group_name_list", onChangeChildCtrl, this);
	LLNameListCtrl* group_name_list = LLViewerUICtrlFactory::getNameListByName(this, "allowed_group_name_list");
	if (group_name_list)
	{
		group_name_list->setCommitOnSelectionChange(TRUE);
		group_name_list->setMaxItemCount(ESTATE_MAX_ACCESS_IDS);
	}

	childSetAction("add_allowed_group_btn", onClickAddAllowedGroup, this);
	childSetAction("remove_allowed_group_btn", onClickRemoveAllowedGroup, this);

	childSetCommitCallback("banned_avatar_name_list", onChangeChildCtrl, this);
	LLNameListCtrl* banned_name_list = LLViewerUICtrlFactory::getNameListByName(this, "banned_avatar_name_list");
	if (banned_name_list)
	{
		banned_name_list->setCommitOnSelectionChange(TRUE);
		banned_name_list->setMaxItemCount(ESTATE_MAX_ACCESS_IDS);
	}

	childSetAction("add_banned_avatar_btn", onClickAddBannedAgent, this);
	childSetAction("remove_banned_avatar_btn", onClickRemoveBannedAgent, this);

	childSetCommitCallback("estate_manager_name_list", onChangeChildCtrl, this);
	LLNameListCtrl* manager_name_list = LLViewerUICtrlFactory::getNameListByName(this, "estate_manager_name_list");
	if (manager_name_list)
	{
		manager_name_list->setCommitOnSelectionChange(TRUE);
		manager_name_list->setMaxItemCount(ESTATE_MAX_MANAGERS * 4);	// Allow extras for dupe issue
	}

	childSetAction("add_estate_manager_btn", onClickAddEstateManager, this);
	childSetAction("remove_estate_manager_btn", onClickRemoveEstateManager, this);
	childSetAction("message_estate_btn", onClickMessageEstate, this);
	childSetAction("kick_user_from_estate_btn", onClickKickUser, this);

	return LLPanelRegionInfo::postBuild();
}

void LLPanelEstateInfo::refresh()
{
	bool public_access = childGetValue("externally_visible_check").asBoolean();
	childSetEnabled("Only Allow", public_access);
	childSetEnabled("limit_payment", public_access);
	childSetEnabled("limit_age_verified", public_access);
	// if this is set to false, then the limit fields are meaningless and should be turned off
	if (public_access == false)
	{
		childSetValue("limit_payment", false);
		childSetValue("limit_age_verified", false);
	}
}

BOOL LLPanelEstateInfo::sendUpdate()
{
	llinfos << "LLPanelEsateInfo::sendUpdate()" << llendl;

	if (getEstateID() <= ESTATE_LAST_LINDEN)
	{
		// trying to change reserved estate, warn
		gViewerWindow->alertXml("ChangeLindenEstate",
							   callbackChangeLindenEstate,
							   this);
	}
	else
	{
		// for normal estates, just make the change
		callbackChangeLindenEstate(0, this);
	}
	return TRUE;
}

// static
void LLPanelEstateInfo::callbackChangeLindenEstate(S32 option, void* data)
{
	LLPanelEstateInfo* self = (LLPanelEstateInfo*)data;
	switch(option)
	{
	case 0:
		// send the update
		LLFloaterRegionInfo::nextInvoice();
		self->commitEstateInfo();
		break;
	case 1:
	default:
		// do nothing
		break;
	}
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

// key = "estatechangeinfo"
// strings[0] = str(estate_id) (added by simulator before relay - not here)
// strings[1] = estate_name
// strings[2] = str(estate_flags)
// strings[3] = str((S32)(sun_hour * 1024.f))
void LLPanelEstateInfo::commitEstateInfo()
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

	char buf[MAX_STRING];		/* Flawfinder: ignore*/
	snprintf(buf, MAX_STRING, "%u", computeEstateFlags());			/* Flawfinder: ignore */
	msg->nextBlock("ParamList");
	msg->addString("Parameter", buf);

	F32 sun_hour = getSunHour();
	if (childGetValue("use_global_time_check").asBoolean())
	{
		sun_hour = 0.f;	// 0 = global time
	}

	snprintf(buf, MAX_STRING, "%d", (S32)(sun_hour*1024.0f));	/* Flawfinder: ignore */
	msg->nextBlock("ParamList");
	msg->addString("Parameter", buf);

	gAgent.sendMessage();
}

void LLPanelEstateInfo::setEstateFlags(U32 flags)
{
	childSetValue("externally_visible_check", LLSD(flags & REGION_FLAGS_EXTERNALLY_VISIBLE ? TRUE : FALSE) );
	childSetValue("fixed_sun_check", LLSD(flags & REGION_FLAGS_SUN_FIXED ? TRUE : FALSE) );
	childSetValue(
		"voice_chat_check",
		LLSD(flags & REGION_FLAGS_ALLOW_VOICE ? TRUE : FALSE));
	childSetValue("allow_direct_teleport", LLSD(flags & REGION_FLAGS_ALLOW_DIRECT_TELEPORT ? TRUE : FALSE) );
	childSetValue("limit_payment", LLSD(flags & REGION_FLAGS_DENY_ANONYMOUS ? TRUE : FALSE) );
	childSetValue("limit_age_verified", LLSD(flags & REGION_FLAGS_DENY_AGEUNVERIFIED ? TRUE : FALSE) );
	childSetVisible("abuse_email_text", flags & REGION_FLAGS_ABUSE_EMAIL_TO_ESTATE_OWNER);

	refresh();
}

U32 LLPanelEstateInfo::computeEstateFlags()
{
	U32 flags = 0;

	if (childGetValue("externally_visible_check").asBoolean())
	{
		flags |= REGION_FLAGS_EXTERNALLY_VISIBLE;
	}

	if ( childGetValue("voice_chat_check").asBoolean() )
	{
		flags |= REGION_FLAGS_ALLOW_VOICE;
	}
	
	if (childGetValue("allow_direct_teleport").asBoolean())
	{
		flags |= REGION_FLAGS_ALLOW_DIRECT_TELEPORT;
	}

	if (childGetValue("fixed_sun_check").asBoolean())
	{
		flags |= REGION_FLAGS_SUN_FIXED;
	}
	
	if (childGetValue("limit_payment").asBoolean())
	{
		flags |= REGION_FLAGS_DENY_ANONYMOUS;
	}
	
	if (childGetValue("limit_age_verified").asBoolean())
	{
		flags |= REGION_FLAGS_DENY_AGEUNVERIFIED;
	}

	
	return flags;
}

BOOL LLPanelEstateInfo::getGlobalTime()
{
	return childGetValue("use_global_time_check").asBoolean();
}

void LLPanelEstateInfo::setGlobalTime(bool b)
{
	childSetValue("use_global_time_check", LLSD(b));
	childSetEnabled("fixed_sun_check", LLSD(!b));
	childSetEnabled("sun_hour_slider", LLSD(!b));
	if (b)
	{
		childSetValue("sun_hour_slider", LLSD(0.f));
	}
}


BOOL LLPanelEstateInfo::getFixedSun()
{
	return childGetValue("fixed_sun_check").asBoolean();
}

void LLPanelEstateInfo::setSunHour(F32 sun_hour)
{
	if(sun_hour < 6.0f)
	{
		sun_hour = 24.0f + sun_hour;
	}
	childSetValue("sun_hour_slider", LLSD(sun_hour));
}

F32 LLPanelEstateInfo::getSunHour()
{
	if (childIsEnabled("sun_hour_slider"))
	{
		return (F32)childGetValue("sun_hour_slider").asReal();
	}
	return 0.f;
}

const std::string LLPanelEstateInfo::getEstateName() const
{
	return childGetValue("estate_name").asString();
}

void LLPanelEstateInfo::setEstateName(const std::string& name)
{
	childSetValue("estate_name", LLSD(name));
}

const std::string LLPanelEstateInfo::getOwnerName() const
{
	return childGetValue("estate_owner").asString();
}

void LLPanelEstateInfo::setOwnerName(const std::string& name)
{
	childSetValue("estate_owner", LLSD(name));
}

void LLPanelEstateInfo::setAccessAllowedEnabled(bool enable_agent,
												bool enable_group,
												bool enable_ban)
{
	childSetEnabled("allow_resident_label", enable_agent);
	childSetEnabled("allowed_avatar_name_list", enable_agent);
	childSetVisible("allowed_avatar_name_list", enable_agent);
	childSetEnabled("add_allowed_avatar_btn", enable_agent);
	childSetEnabled("remove_allowed_avatar_btn", enable_agent);

	// Groups
	childSetEnabled("allow_group_label", enable_group);
	childSetEnabled("allowed_group_name_list", enable_group);
	childSetVisible("allowed_group_name_list", enable_group);
	childSetEnabled("add_allowed_group_btn", enable_group);
	childSetEnabled("remove_allowed_group_btn", enable_group);

	// Ban
	childSetEnabled("ban_resident_label", enable_ban);
	childSetEnabled("banned_avatar_name_list", enable_ban);
	childSetVisible("banned_avatar_name_list", enable_ban);
	childSetEnabled("add_banned_avatar_btn", enable_ban);
	childSetEnabled("remove_banned_avatar_btn", enable_ban);

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

// static
void LLPanelEstateInfo::callbackCacheName(
	const LLUUID& id,
	const char* first,
	const char* last,
	BOOL is_group,
	void*)
{
	LLPanelEstateInfo* self = LLFloaterRegionInfo::getPanelEstate();
	if (!self) return;

	std::string name;
	
	if (id.isNull())
	{
		name = "(none)";
	}
	else
	{
		name = first;
		name += " ";
		name += last;
	}

	self->setOwnerName(name);
}

void LLPanelEstateInfo::clearAccessLists() 
{
	LLNameListCtrl* name_list = LLViewerUICtrlFactory::getNameListByName(this, "allowed_avatar_name_list");
	if (name_list)
	{
		name_list->deleteAllItems();
	}

	name_list = LLViewerUICtrlFactory::getNameListByName(this, "banned_avatar_name_list");
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
		btn_name = "remove_estate_manager_btn";
	}

	// enable the remove button if something is selected
	LLNameListCtrl* name_list = LLViewerUICtrlFactory::getNameListByName(this, name);
	childSetEnabled(btn_name.c_str(), name_list && name_list->getFirstSelected() ? TRUE : FALSE);

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
	gViewerWindow->alertXmlEditText("MessageEstate", LLString::format_map_t(),
									NULL, NULL,
									onMessageCommit, userdata);
}

// static
void LLPanelEstateInfo::onMessageCommit(S32 option, const LLString& text, void* userdata)
{
	if(option != 0) return;
	if(text.empty()) return;
	LLPanelEstateInfo* self = (LLPanelEstateInfo*)userdata;
	if(!self) return;
	llinfos << "Message to everyone: " << text << llendl;
	strings_t strings;
	//integers_t integers;
	std::string name;
	gAgent.buildFullname(name);
	strings.push_back(strings_t::value_type(name));
	strings.push_back(strings_t::value_type(text));
	LLUUID invoice(LLFloaterRegionInfo::getLastInvoice());
	self->sendEstateOwnerMessage(gMessageSystem, "instantmessage", invoice, strings);
}

LLPanelEstateCovenant::LLPanelEstateCovenant()
: mCovenantID(LLUUID::null)
{
}

// virtual 
bool LLPanelEstateCovenant::refreshFromRegion(LLViewerRegion* region)
{
	LLTextBox* region_name = (LLTextBox*)getChildByName("region_name_text");
	if (region_name)
	{
		region_name->setText(region->getName());
	}

	LLTextBox* resellable_clause = (LLTextBox*)getChildByName("resellable_clause");
	if (resellable_clause)
	{
		if (region->getRegionFlags() & REGION_FLAGS_BLOCK_LAND_RESELL)
		{
			resellable_clause->setText(childGetText("can_not_resell"));
		}
		else
		{
			resellable_clause->setText(childGetText("can_resell"));
		}
	}
	
	LLTextBox* changeable_clause = (LLTextBox*)getChildByName("changeable_clause");
	if (changeable_clause)
	{
		if (region->getRegionFlags() & REGION_FLAGS_ALLOW_PARCEL_CHANGES)
		{
			changeable_clause->setText(childGetText("can_change"));
		}
		else
		{
			changeable_clause->setText(childGetText("can_not_change"));
		}
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
	initHelpBtn("covenant_help",		"HelpEstateCovenant");
	mEstateNameText = (LLTextBox*)getChildByName("estate_name_text");
	mEstateOwnerText = (LLTextBox*)getChildByName("estate_owner_text");
	mLastModifiedText = (LLTextBox*)getChildByName("covenant_timestamp_text");
	mEditor = (LLViewerTextEditor*)getChildByName("covenant_editor");
	if (mEditor) mEditor->setHandleEditKeysDirectly(TRUE);
	LLButton* reset_button = (LLButton*)getChildByName("reset_covenant");
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
								  LLString& tooltip_msg)
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
			gViewerWindow->alertXml("EstateChangeCovenant",
									LLPanelEstateCovenant::confirmChangeCovenantCallback,
									item);
		}
		break;
	default:
		*accept = ACCEPT_NO;
		break;
	}

	return TRUE;
} 

// static 
void LLPanelEstateCovenant::confirmChangeCovenantCallback(S32 option, void* userdata)
{
	LLInventoryItem* item = (LLInventoryItem*)userdata;
	LLPanelEstateCovenant* self = LLFloaterRegionInfo::getPanelCovenant();

	if (!item || !self) return;

	switch(option)
	{
	case 0:
		self->loadInvItem(item);
		break;
	default:
		break;
	}
}

// static
void LLPanelEstateCovenant::resetCovenantID(void* userdata)
{
	gViewerWindow->alertXml("EstateChangeCovenant",
							LLPanelEstateCovenant::confirmResetCovenantCallback,
							NULL);
}

// static
void LLPanelEstateCovenant::confirmResetCovenantCallback(S32 option, void* userdata)
{
	LLPanelEstateCovenant* self = LLFloaterRegionInfo::getPanelCovenant();
	if (!self) return;

	switch(option)
	{
	case 0:		
		self->loadInvItem(NULL);
		break;
	default:
		break;
	}
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
		setCovenantTextEditor("There is no Covenant provided for this Estate.");
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

			char* buffer = new char[file_length+1];
			if (buffer == NULL)
			{
				llerrs << "Memory Allocation Failed" << llendl;
				return;
			}

			file.read((U8*)buffer, file_length);		/* Flawfinder: ignore */
			// put a EOS at the end
			buffer[file_length] = 0;

			if( (file_length > 19) && !strncmp( buffer, "Linden text version", 19 ) )
			{
				if( !panelp->mEditor->importBuffer( buffer ) )
				{
					llwarns << "Problem importing estate covenant." << llendl;
					gViewerWindow->alertXml("ProblemImportingEstateCovenant");
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
			delete[] buffer;
		}
		else
		{
			if( gViewerStats )
			{
				gViewerStats->incStat( LLViewerStats::ST_DOWNLOAD_FAILED );
			}

			if( LL_ERR_ASSET_REQUEST_NOT_IN_DATABASE == status ||
				LL_ERR_FILE_EMPTY == status)
			{
				gViewerWindow->alertXml("MissingNotecardAssetID");
			}
			else if (LL_ERR_INSUFFICIENT_PERMISSIONS == status)
			{
				gViewerWindow->alertXml("NotAllowedToViewNotecard");
			}
			else
			{
				gViewerWindow->alertXml("UnableToLoadNotecard");
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

const std::string& LLPanelEstateCovenant::getEstateName() const
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

const std::string& LLPanelEstateCovenant::getOwnerName() const
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

/*
// AgentData = agent_id
// RequestData = "setowner"
// StringData[0] = compressed owner id
// IntegerData[0] = serial
bool LLDispatchSetEstateOwner::operator()(
		const LLDispatcher* dispatcher,
		const std::string& key,
		const sparam_t& strings,
		const iparam_t& integers)
{
	LLFloaterRegionInfo* floater = LLFloaterRegionInfo::getInstance();
	if (!floater) return true;

	LLTabContainer* tab = (LLTabContainer*)(floater->getChildByName("region_panels"));
	if (!tab) return true;

	LLPanelEstateInfo* panel = (LLPanelEstateInfo*)(tab->getChildByName("Estate"));
	if (!panel) return true;

	// TODO -- check owner, and disable floater if owner
	// does not match

	return true;
}
*/

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
	std::string estate_name = strings[0].c_str();
	panel->setEstateName(estate_name);


	LLViewerRegion* regionp = gAgent.getRegion();

	LLUUID owner_id(strings[1].c_str());
	regionp->setOwner(owner_id);
	// Update estate owner name in UI
	const BOOL is_group = FALSE;
	gCacheName->get(owner_id, is_group, LLPanelEstateInfo::callbackCacheName);

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
		allowed_agent_name_list = LLViewerUICtrlFactory::getNameListByName(panel, "allowed_avatar_name_list");

		int totalAllowedAgents = num_allowed_agents;
		
		if (allowed_agent_name_list) 
		{
			totalAllowedAgents += allowed_agent_name_list->getItemCount();
		}

		std::string msg = llformat("Allowed residents: (%d, max %d)",
									totalAllowedAgents,
									ESTATE_MAX_ACCESS_IDS);
		panel->childSetValue("allow_resident_label", LLSD(msg));

		if (allowed_agent_name_list)
		{
			//allowed_agent_name_list->deleteAllItems();
			for (S32 i = 0; i < num_allowed_agents && i < ESTATE_MAX_ACCESS_IDS; i++)
			{
				LLUUID id;
				memcpy(id.mData, strings[index++].data(), UUID_BYTES);		/* Flawfinder: ignore */
				allowed_agent_name_list->addNameItem(id);
			}
			panel->childSetEnabled("remove_allowed_avatar_btn", allowed_agent_name_list->getFirstSelected() ? TRUE : FALSE);
			allowed_agent_name_list->sortByColumn(0, TRUE);
		}
	}

	if (access_flags & ESTATE_ACCESS_ALLOWED_GROUPS)
	{
		LLNameListCtrl* allowed_group_name_list;
		allowed_group_name_list = LLViewerUICtrlFactory::getNameListByName(panel, "allowed_group_name_list");

		std::string msg = llformat("Allowed groups: (%d, max %d)",
									num_allowed_groups,
									(S32) ESTATE_MAX_GROUP_IDS);
		panel->childSetValue("allow_group_label", LLSD(msg));

		if (allowed_group_name_list)
		{
			allowed_group_name_list->deleteAllItems();
			for (S32 i = 0; i < num_allowed_groups && i < ESTATE_MAX_GROUP_IDS; i++)
			{
				LLUUID id;
				memcpy(id.mData, strings[index++].data(), UUID_BYTES);		/* Flawfinder: ignore */
				allowed_group_name_list->addGroupNameItem(id);
			}
			panel->childSetEnabled("remove_allowed_group_btn", allowed_group_name_list->getFirstSelected() ? TRUE : FALSE);
			allowed_group_name_list->sortByColumn(0, TRUE);
		}
	}

	if (access_flags & ESTATE_ACCESS_BANNED_AGENTS)
	{
		LLNameListCtrl* banned_agent_name_list;
		banned_agent_name_list = LLViewerUICtrlFactory::getNameListByName(panel, "banned_avatar_name_list");

		int totalBannedAgents = num_banned_agents;
		
		if (banned_agent_name_list) 
		{
			totalBannedAgents += banned_agent_name_list->getItemCount();
		}


		std::string msg = llformat("Banned residents: (%d, max %d)",
									totalBannedAgents,
									ESTATE_MAX_ACCESS_IDS);
		panel->childSetValue("ban_resident_label", LLSD(msg));

		if (banned_agent_name_list)
		{
			//banned_agent_name_list->deleteAllItems();
			for (S32 i = 0; i < num_banned_agents && i < ESTATE_MAX_ACCESS_IDS; i++)
			{
				LLUUID id;
				memcpy(id.mData, strings[index++].data(), UUID_BYTES);		/* Flawfinder: ignore */
				banned_agent_name_list->addNameItem(id);
			}
			panel->childSetEnabled("remove_banned_avatar_btn", banned_agent_name_list->getFirstSelected() ? TRUE : FALSE);
			banned_agent_name_list->sortByColumn(0, TRUE);
		}
	}

	if (access_flags & ESTATE_ACCESS_MANAGERS)
	{
		std::string msg = llformat("Estate Managers: (%d, max %d)",
									num_estate_managers,
									ESTATE_MAX_MANAGERS);
		panel->childSetValue("estate_manager_label", LLSD(msg));

		LLNameListCtrl* estate_manager_name_list =
			LLViewerUICtrlFactory::getNameListByName(panel, "estate_manager_name_list");
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
			panel->childSetEnabled("remove_estate_manager_btn", estate_manager_name_list->getFirstSelected() ? TRUE : FALSE);
			estate_manager_name_list->sortByColumn(0, TRUE);
		}
	}

	return true;
}
