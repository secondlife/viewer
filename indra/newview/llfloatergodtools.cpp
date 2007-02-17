/** 
 * @file llfloatergodtools.cpp
 * @brief The on-screen rectangle with tool options.
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "llfloatergodtools.h"

#include "llcoord.h"
#include "llfontgl.h"
#include "llframetimer.h"
#include "llgl.h"
#include "llhost.h"
#include "llregionflags.h"
#include "llstring.h"
#include "message.h"

#include "llagent.h"
#include "llalertdialog.h"
#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "llcombobox.h"
#include "lldraghandle.h"
#include "llfloater.h"
#include "llfocusmgr.h"
#include "llfloatertopobjects.h"
#include "lllineeditor.h"
#include "llmenugl.h"
#include "llresmgr.h"
#include "llselectmgr.h"
#include "llsky.h"
#include "llspinctrl.h"
#include "llstatusbar.h"
#include "lltabcontainer.h"
#include "lltextbox.h"
#include "lluictrl.h"
#include "llviewerparcelmgr.h"
#include "llviewerregion.h"
#include "llviewerwindow.h"
#include "llworld.h"
#include "llfloateravatarpicker.h"
#include "llnotify.h"
#include "llxfermanager.h"
#include "llvlcomposition.h"
#include "llsurface.h"
#include "llviewercontrol.h"
#include "llvieweruictrlfactory.h"

#include "lltransfertargetfile.h"
#include "lltransfersourcefile.h"

const F32 SECONDS_BETWEEN_UPDATE_REQUESTS = 5.0f;

static LLFloaterGodTools* sGodTools = NULL;

//*****************************************************************************
// LLFloaterGodTools
//*****************************************************************************

// static
LLFloaterGodTools* LLFloaterGodTools::instance()
{
	if (!sGodTools)
	{
		sGodTools = new LLFloaterGodTools();
		sGodTools->open();	/*Flawfinder: ignore*/
		sGodTools->center();
		sGodTools->setFocus(TRUE);
	}
	return sGodTools;
}
 

// static
void LLFloaterGodTools::refreshAll()
{
	LLFloaterGodTools* god_tools = instance();
	if (god_tools)
	{
		if (gAgent.getRegionHost() != god_tools->mCurrentHost)
		{
			// we're in a new region
			god_tools->sendRegionInfoRequest();
		}
	}
}



LLFloaterGodTools::LLFloaterGodTools()
:	LLFloater("godtools floater"),
	mCurrentHost(LLHost::invalid),
	mUpdateTimer()
{
	LLCallbackMap::map_t factory_map;
	factory_map["grid"] = LLCallbackMap(createPanelGrid, this);
	factory_map["region"] = LLCallbackMap(createPanelRegion, this);
	factory_map["objects"] = LLCallbackMap(createPanelObjects, this);
	factory_map["request"] = LLCallbackMap(createPanelRequest, this);
	gUICtrlFactory->buildFloater(this, "floater_god_tools.xml", &factory_map);

	childSetTabChangeCallback("GodTools Tabs", "grid", onTabChanged, this);
	childSetTabChangeCallback("GodTools Tabs", "region", onTabChanged, this);
	childSetTabChangeCallback("GodTools Tabs", "objects", onTabChanged, this);
	childSetTabChangeCallback("GodTools Tabs", "request", onTabChanged, this);

	sendRegionInfoRequest();

	childShowTab("GodTools Tabs", "region");
}

// static
void* LLFloaterGodTools::createPanelGrid(void *userdata)
{
	return new LLPanelGridTools("grid");
}

// static
void* LLFloaterGodTools::createPanelRegion(void *userdata)
{
	LLFloaterGodTools* self = (LLFloaterGodTools*)userdata;
	self->mPanelRegionTools = new LLPanelRegionTools("region");
	return self->mPanelRegionTools;
}

// static
void* LLFloaterGodTools::createPanelObjects(void *userdata)
{
	LLFloaterGodTools* self = (LLFloaterGodTools*)userdata;
	self->mPanelObjectTools = new LLPanelObjectTools("objects");
	return self->mPanelObjectTools;
}

// static
void* LLFloaterGodTools::createPanelRequest(void *userdata)
{
	return new LLPanelRequestTools("region");
}

LLFloaterGodTools::~LLFloaterGodTools()
{
	// children automatically deleted
}

U32 LLFloaterGodTools::computeRegionFlags() const
{
	U32 flags = gAgent.getRegion()->getRegionFlags();
	if (mPanelRegionTools) flags = mPanelRegionTools->computeRegionFlags(flags);
	if (mPanelObjectTools) flags = mPanelObjectTools->computeRegionFlags(flags);
	return flags;
}


void LLFloaterGodTools::updatePopup(LLCoordGL center, MASK mask)
{
}

// virtual
void LLFloaterGodTools::onClose(bool app_quitting)
{
	if (sGodTools)
	{
		sGodTools->setVisible(FALSE);
	}
}

// virtual
void LLFloaterGodTools::draw()
{
	if (mCurrentHost == LLHost::invalid)
	{
		if (mUpdateTimer.getElapsedTimeF32() > SECONDS_BETWEEN_UPDATE_REQUESTS)
		{
			sendRegionInfoRequest();
		}
	}
	else if (gAgent.getRegionHost() != mCurrentHost)
	{
		sendRegionInfoRequest();
	}
	LLFloater::draw();
}

// static
void LLFloaterGodTools::show(void *)
{
	LLFloaterGodTools* god_tools = instance();
	god_tools->open();
	LLPanel *panel = god_tools->childGetVisibleTab("GodTools Tabs");
	if (panel) panel->setFocus(TRUE);
	if (god_tools->mPanelObjectTools) god_tools->mPanelObjectTools->setTargetAvatar(LLUUID::null);

	if (gAgent.getRegionHost() != god_tools->mCurrentHost)
	{
		// we're in a new region
		god_tools->sendRegionInfoRequest();
	}
}

void LLFloaterGodTools::showPanel(const LLString& panel_name)
{
	childShowTab("GodTools Tabs", panel_name);
	open();	/*Flawfinder: ignore*/
	LLPanel *panel = childGetVisibleTab("GodTools Tabs");
	if (panel) panel->setFocus(TRUE);
}


// static
void LLFloaterGodTools::onTabChanged(void* data, bool from_click)
{
	LLPanel* panel = (LLPanel*)data;
	if (panel)
	{
		panel->setFocus(TRUE);
	}
}


// static
void LLFloaterGodTools::processRegionInfo(LLMessageSystem* msg)
{
	LLHost host = msg->getSender();
	if (host != gAgent.getRegionHost())
	{
		// update is for a different region than the one we're in
		return;
	}

	//const S32 SIM_NAME_BUF = 256;
	U32 region_flags;
	U8 sim_access;
	U8 agent_limit;
	char sim_name[MAX_STRING];		/*Flawfinder: ignore*/
	U32 estate_id;
	U32 parent_estate_id;
	F32 water_height;
	F32 billable_factor;
	F32 object_bonus_factor;
	F32 terrain_raise_limit;
	F32 terrain_lower_limit;
	S32 price_per_meter;
	S32 redirect_grid_x;
	S32 redirect_grid_y;
	LLUUID cache_id;

	msg->getStringFast(_PREHASH_RegionInfo, _PREHASH_SimName, MAX_STRING, sim_name);
	msg->getU32Fast(_PREHASH_RegionInfo, _PREHASH_EstateID, estate_id);
	msg->getU32Fast(_PREHASH_RegionInfo, _PREHASH_ParentEstateID, parent_estate_id);
	msg->getU32Fast(_PREHASH_RegionInfo, _PREHASH_RegionFlags, region_flags);
	msg->getU8Fast(_PREHASH_RegionInfo, _PREHASH_SimAccess, sim_access);
	msg->getU8Fast(_PREHASH_RegionInfo, _PREHASH_MaxAgents, agent_limit);
	msg->getF32Fast(_PREHASH_RegionInfo, _PREHASH_ObjectBonusFactor, object_bonus_factor);
	msg->getF32Fast(_PREHASH_RegionInfo, _PREHASH_BillableFactor, billable_factor);
	msg->getF32Fast(_PREHASH_RegionInfo, _PREHASH_WaterHeight, water_height);
	msg->getF32Fast(_PREHASH_RegionInfo, _PREHASH_TerrainRaiseLimit, terrain_raise_limit);
	msg->getF32Fast(_PREHASH_RegionInfo, _PREHASH_TerrainLowerLimit, terrain_lower_limit);
	msg->getS32Fast(_PREHASH_RegionInfo, _PREHASH_PricePerMeter, price_per_meter);
	msg->getS32Fast(_PREHASH_RegionInfo, _PREHASH_RedirectGridX, redirect_grid_x);
	msg->getS32Fast(_PREHASH_RegionInfo, _PREHASH_RedirectGridY, redirect_grid_y);

	// push values to the current LLViewerRegion
	LLViewerRegion *regionp = gAgent.getRegion();
	if (regionp)
	{
		regionp->setRegionNameAndZone(sim_name);
		regionp->setRegionFlags(region_flags);
		regionp->setSimAccess(sim_access);
		regionp->setWaterHeight(water_height);
		regionp->setBillableFactor(billable_factor);
	}

	// push values to god tools, if available
	if (sGodTools 
		&& sGodTools->mPanelRegionTools
		&& sGodTools->mPanelObjectTools
		&& msg
		&& gAgent.isGodlike())
	{
		LLPanelRegionTools* rtool = sGodTools->mPanelRegionTools;
		sGodTools->mCurrentHost = host;

		// store locally
		rtool->setSimName(sim_name);
		rtool->setEstateID(estate_id);
		rtool->setParentEstateID(parent_estate_id);
		rtool->setCheckFlags(region_flags);
		rtool->setBillableFactor(billable_factor);
		rtool->setPricePerMeter(price_per_meter);
		rtool->setRedirectGridX(redirect_grid_x);
		rtool->setRedirectGridY(redirect_grid_y);
		rtool->enableAllWidgets();

		LLPanelObjectTools *otool = sGodTools->mPanelObjectTools;
		otool->setCheckFlags(region_flags);
		otool->enableAllWidgets();

		LLViewerRegion *regionp = gAgent.getRegion();
		if ( !regionp )
		{
			// -1 implies non-existent
			rtool->setGridPosX(-1);
			rtool->setGridPosY(-1);
		}
		else
		{
			//compute the grid position of the region
			LLVector3d global_pos = regionp->getPosGlobalFromRegion(LLVector3::zero);
			S32 grid_pos_x = (S32) (global_pos.mdV[VX] / 256.0f);
			S32 grid_pos_y = (S32) (global_pos.mdV[VY] / 256.0f);

			rtool->setGridPosX(grid_pos_x);
			rtool->setGridPosY(grid_pos_y);
		}
	}
}


void LLFloaterGodTools::sendRegionInfoRequest()
{
	if (mPanelRegionTools) mPanelRegionTools->clearAllWidgets();
	if (mPanelObjectTools) mPanelObjectTools->clearAllWidgets();
	mCurrentHost = LLHost::invalid;
	mUpdateTimer.reset();

	LLMessageSystem* msg = gMessageSystem;
	msg->newMessage("RequestRegionInfo");
	msg->nextBlock("AgentData");
	msg->addUUID("AgentID", gAgent.getID());
	msg->addUUID("SessionID", gAgent.getSessionID());
	gAgent.sendReliableMessage();
}


void LLFloaterGodTools::sendGodUpdateRegionInfo()
{
	LLViewerRegion *regionp = gAgent.getRegion();
	if (gAgent.isGodlike()
		&& sGodTools->mPanelRegionTools
		&& regionp
		&& gAgent.getRegionHost() == mCurrentHost)
	{
		LLMessageSystem *msg = gMessageSystem;
		LLPanelRegionTools *rtool = sGodTools->mPanelRegionTools;

		msg->newMessage("GodUpdateRegionInfo");
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		msg->nextBlockFast(_PREHASH_RegionInfo);
		msg->addStringFast(_PREHASH_SimName, rtool->getSimName().c_str());
		msg->addU32Fast(_PREHASH_EstateID, rtool->getEstateID());
		msg->addU32Fast(_PREHASH_ParentEstateID, rtool->getParentEstateID());
		msg->addU32Fast(_PREHASH_RegionFlags, computeRegionFlags());
		msg->addF32Fast(_PREHASH_BillableFactor, rtool->getBillableFactor());
		msg->addS32Fast(_PREHASH_PricePerMeter, rtool->getPricePerMeter());
		msg->addS32Fast(_PREHASH_RedirectGridX, rtool->getRedirectGridX());
		msg->addS32Fast(_PREHASH_RedirectGridY, rtool->getRedirectGridY());

		gAgent.sendReliableMessage();
	}
}

//*****************************************************************************
// LLPanelRegionTools
//*****************************************************************************


//   || Region |______________________________________
//   |                                                |
//   |  Sim Name: [________________________________]  |
//   |  ^         ^                                   |
//   |  LEFT      R1         Estate id:   [----]      |
//   |                       Parent id:   [----]      |
//   |  [X] Prelude          Grid Pos:     [--] [--]  |
//   |  [X] Visible          Redirect Pos: [--] [--]  |
//   |  [X] Damage           Bill Factor  [8_______]  |
//   |  [X] Block Terraform  PricePerMeter[8_______]  |
//   |                                    [Apply]     |
//   |                                                |
//   |  [Bake Terrain]            [Select Region]     |
//   |  [Revert Terrain]          [Autosave Now]      |
//   |  [Swap Terrain]                                | 
//   |				                                  | 
//   |________________________________________________|
//      ^                    ^                     ^
//      LEFT                 R2                   RIGHT


// Floats because spinners only support floats. JC
const F32 BILLABLE_FACTOR_DEFAULT = 1;
const F32 BILLABLE_FACTOR_MIN = 0.0f;
const F32 BILLABLE_FACTOR_MAX = 4.f;

// floats because spinners only understand floats. JC
const F32 PRICE_PER_METER_DEFAULT = 1.f;
const F32 PRICE_PER_METER_MIN = 0.f;
const F32 PRICE_PER_METER_MAX = 100.f;


LLPanelRegionTools::LLPanelRegionTools(const std::string& title)
: 	LLPanel(title)
{
}

BOOL LLPanelRegionTools::postBuild()
{
	childSetCommitCallback("region name", onChangeAnything, this);
	childSetKeystrokeCallback("region name", onChangeSimName, this);
	childSetPrevalidate("region name", &LLLineEditor::prevalidatePrintableNotPipe);

	childSetCommitCallback("check prelude", onChangePrelude, this);
	childSetCommitCallback("check fixed sun", onChangeAnything, this);
	childSetCommitCallback("check reset home", onChangeAnything, this);
	childSetCommitCallback("check visible", onChangeAnything, this);
	childSetCommitCallback("check damage", onChangeAnything, this);
	childSetCommitCallback("block dwell", onChangeAnything, this);
	childSetCommitCallback("block terraform", onChangeAnything, this);
	childSetCommitCallback("allow transfer", onChangeAnything, this);
	childSetCommitCallback("is sandbox", onChangeAnything, this);

	childSetAction("Bake Terrain", onBakeTerrain, this);
	childSetAction("Revert Terrain", onRevertTerrain, this);
	childSetAction("Swap Terrain", onSwapTerrain, this);

	childSetCommitCallback("estate", onChangeAnything, this);
	childSetPrevalidate("estate", &LLLineEditor::prevalidatePositiveS32);

	childSetCommitCallback("parentestate", onChangeAnything, this);
	childSetPrevalidate("parentestate", &LLLineEditor::prevalidatePositiveS32);
	childDisable("parentestate");

	childSetCommitCallback("gridposx", onChangeAnything, this);
	childSetPrevalidate("gridposx", &LLLineEditor::prevalidatePositiveS32);
	childDisable("gridposx");

	childSetCommitCallback("gridposy", onChangeAnything, this);
	childSetPrevalidate("gridposy", &LLLineEditor::prevalidatePositiveS32);
	childDisable("gridposy");

	childSetCommitCallback("redirectx", onChangeAnything, this);
	childSetPrevalidate("redirectx", &LLLineEditor::prevalidatePositiveS32);

	childSetCommitCallback("redirecty", onChangeAnything, this);
	childSetPrevalidate("redirecty", &LLLineEditor::prevalidatePositiveS32);

	childSetCommitCallback("billable factor", onChangeAnything, this);

	childSetCommitCallback("land cost", onChangeAnything, this);

	childSetAction("Refresh", onRefresh, this);
	childSetAction("Apply", onApplyChanges, this);

	childSetAction("Select Region", onSelectRegion, this);
	childSetAction("Autosave now", onSaveState, this);
			 
	return TRUE;
}

// Destroys the object
LLPanelRegionTools::~LLPanelRegionTools()
{
	// base class will take care of everything
}

U32 LLPanelRegionTools::computeRegionFlags(U32 flags) const
{
	flags &= getRegionFlagsMask();
	flags |= getRegionFlags();
	return flags;
}


void LLPanelRegionTools::refresh()
{
}


void LLPanelRegionTools::clearAllWidgets()
{
	// clear all widgets
	childSetValue("region name", "unknown");
	childSetFocus("region name", FALSE);

	childSetValue("check prelude", FALSE);
	childDisable("check prelude");

	childSetValue("check fixed sun", FALSE);
	childDisable("check fixed sun");

	childSetValue("check reset home", FALSE);
	childDisable("check reset home");

	childSetValue("check damage", FALSE);
	childDisable("check damage");

	childSetValue("check visible", FALSE);
	childDisable("check visible");

	childSetValue("block terraform", FALSE);
	childDisable("block terraform");

	childSetValue("block dwell", FALSE);
	childDisable("block dwell");

	childSetValue("is sandbox", FALSE);
	childDisable("is sandbox");

	childSetValue("billable factor", BILLABLE_FACTOR_DEFAULT);
	childDisable("billable factor");

	childSetValue("land cost", PRICE_PER_METER_DEFAULT);
	childDisable("land cost");

	childDisable("Apply");
	childDisable("Bake Terrain");
	childDisable("Autosave now");
}


void LLPanelRegionTools::enableAllWidgets()
{
	// enable all of the widgets
	
	childEnable("check prelude");
	childEnable("check fixed sun");
	childEnable("check reset home");
	childEnable("check damage");
	childDisable("check visible"); // use estates to update...
	childEnable("block terraform");
	childEnable("block dwell");
	childEnable("is sandbox");
	
	childEnable("billable factor");
	childEnable("land cost");

	childDisable("Apply");	// don't enable this one
	childEnable("Bake Terrain");
	childEnable("Autosave now");
}


// static
void LLPanelRegionTools::onSaveState(void* userdata)
{
	if (gAgent.isGodlike())
	{
		// Send message to save world state
		gMessageSystem->newMessageFast(_PREHASH_StateSave);
		gMessageSystem->nextBlockFast(_PREHASH_AgentData);
		gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		gMessageSystem->nextBlockFast(_PREHASH_DataBlock);
		gMessageSystem->addStringFast(_PREHASH_Filename, NULL);
		gAgent.sendReliableMessage();
	}
}

const std::string LLPanelRegionTools::getSimName() const
{
	return childGetValue("region name");
}

U32 LLPanelRegionTools::getEstateID() const
{
	U32 id = (U32)childGetValue("estate").asInteger();
	return id;
}

U32 LLPanelRegionTools::getParentEstateID() const
{
	U32 id = (U32)childGetValue("parentestate").asInteger();
	return id;
}

S32 LLPanelRegionTools::getRedirectGridX() const
{
	return childGetValue("redirectx").asInteger();
}

S32 LLPanelRegionTools::getRedirectGridY() const
{
	return childGetValue("redirecty").asInteger();
}

S32 LLPanelRegionTools::getGridPosX() const
{
	return childGetValue("gridposx").asInteger();
}

S32 LLPanelRegionTools::getGridPosY() const
{
	return childGetValue("gridposy").asInteger();
}

U32 LLPanelRegionTools::getRegionFlags() const
{
	U32 flags = 0x0;
	flags = childGetValue("check prelude").asBoolean()  
					? set_prelude_flags(flags)
					: unset_prelude_flags(flags);

	// override prelude
	if (childGetValue("check fixed sun").asBoolean())
	{
		flags |= REGION_FLAGS_SUN_FIXED;
	}
	if (childGetValue("check reset home").asBoolean())
	{
		flags |= REGION_FLAGS_RESET_HOME_ON_TELEPORT;
	}
	if (childGetValue("check visible").asBoolean())
	{
		flags |= REGION_FLAGS_EXTERNALLY_VISIBLE;
	}
	if (childGetValue("check damage").asBoolean())
	{
		flags |= REGION_FLAGS_ALLOW_DAMAGE;
	}
	if (childGetValue("block terraform").asBoolean())
	{
		flags |= REGION_FLAGS_BLOCK_TERRAFORM;
	}
	if (childGetValue("block dwell").asBoolean())
	{
		flags |= REGION_FLAGS_BLOCK_DWELL;
	}
	if (childGetValue("is sandbox").asBoolean())
	{
		flags |= REGION_FLAGS_SANDBOX;
	}
	return flags;
}

U32 LLPanelRegionTools::getRegionFlagsMask() const
{
	U32 flags = 0xffffffff;
	flags = childGetValue("check prelude").asBoolean()
				? set_prelude_flags(flags)
				: unset_prelude_flags(flags);

	if (!childGetValue("check fixed sun").asBoolean())
	{
		flags &= ~REGION_FLAGS_SUN_FIXED;
	}
	if (!childGetValue("check reset home").asBoolean())
	{
		flags &= ~REGION_FLAGS_RESET_HOME_ON_TELEPORT;
	}
	if (!childGetValue("check visible").asBoolean())
	{
		flags &= ~REGION_FLAGS_EXTERNALLY_VISIBLE;
	}
	if (!childGetValue("check damage").asBoolean())
	{
		flags &= ~REGION_FLAGS_ALLOW_DAMAGE;
	}
	if (!childGetValue("block terraform").asBoolean())
	{
		flags &= ~REGION_FLAGS_BLOCK_TERRAFORM;
	}
	if (!childGetValue("block dwell").asBoolean())
	{
		flags &= ~REGION_FLAGS_BLOCK_DWELL;
	}
	if (!childGetValue("is sandbox").asBoolean())
	{
		flags &= ~REGION_FLAGS_SANDBOX;
	}
	return flags;
}

F32 LLPanelRegionTools::getBillableFactor() const
{
	return (F32)childGetValue("billable factor").asReal();
}

S32 LLPanelRegionTools::getPricePerMeter() const
{
	return childGetValue("land cost");
}

void LLPanelRegionTools::setSimName(char *name)
{
	childSetValue("region name", name);
}

void LLPanelRegionTools::setEstateID(U32 id)
{
	childSetValue("estate", (S32)id);
}

void LLPanelRegionTools::setGridPosX(S32 pos)
{
	childSetValue("gridposx", pos);
}

void LLPanelRegionTools::setGridPosY(S32 pos)
{
	childSetValue("gridposy", pos);
}

void LLPanelRegionTools::setRedirectGridX(S32 pos)
{
	childSetValue("redirectx", pos);
}

void LLPanelRegionTools::setRedirectGridY(S32 pos)
{
	childSetValue("redirecty", pos);
}

void LLPanelRegionTools::setParentEstateID(U32 id)
{
	childSetValue("parentestate", (S32)id);
}

void LLPanelRegionTools::setCheckFlags(U32 flags)
{
	childSetValue("check prelude", is_prelude(flags) ? TRUE : FALSE);
	childSetValue("check fixed sun", flags & REGION_FLAGS_SUN_FIXED ? TRUE : FALSE);
	childSetValue("check reset home", flags & REGION_FLAGS_RESET_HOME_ON_TELEPORT ? TRUE : FALSE);
	childSetValue("check damage", flags & REGION_FLAGS_ALLOW_DAMAGE ? TRUE : FALSE);
	childSetValue("check visible", flags & REGION_FLAGS_EXTERNALLY_VISIBLE ? TRUE : FALSE);
	childSetValue("block terraform", flags & REGION_FLAGS_BLOCK_TERRAFORM ? TRUE : FALSE);
	childSetValue("block dwell", flags & REGION_FLAGS_BLOCK_DWELL ? TRUE : FALSE);
	childSetValue("is sandbox", flags & REGION_FLAGS_SANDBOX ? TRUE : FALSE );
}

void LLPanelRegionTools::setBillableFactor(F32 billable_factor)
{
	childSetValue("billable factor", billable_factor);
}

void LLPanelRegionTools::setPricePerMeter(S32 price)
{
	childSetValue("land cost", price);
}

// static
void LLPanelRegionTools::onChangeAnything(LLUICtrl* ctrl, void* userdata)
{
	if (sGodTools 
		&& userdata
		&& gAgent.isGodlike())
	{
		LLPanelRegionTools* region_tools = (LLPanelRegionTools*) userdata;
		region_tools->childEnable("Apply");
	}
}

// static
void LLPanelRegionTools::onChangePrelude(LLUICtrl* ctrl, void* data)
{
	// checking prelude auto-checks fixed sun
	LLPanelRegionTools* self = (LLPanelRegionTools*)data;
	if (self->childGetValue("check prelude").asBoolean())
	{
		self->childSetValue("check fixed sun", TRUE);
		self->childSetValue("check reset home", TRUE);
	}
	// pass on to default onChange handler
	onChangeAnything(ctrl, data);
}

// static
void LLPanelRegionTools::onChangeSimName(LLLineEditor* caller, void* userdata )
{
	if (sGodTools 
		&& userdata
		&& gAgent.isGodlike())
	{
		LLPanelRegionTools* region_tools = (LLPanelRegionTools*) userdata;
		region_tools->childEnable("Apply");
	}
}

//static
void LLPanelRegionTools::onRefresh(void* userdata)
{
	LLViewerRegion *region = gAgent.getRegion();
	if (region 
		&& sGodTools 
		&& gAgent.isGodlike())
	{
		sGodTools->sendRegionInfoRequest();
	}
}

// static
void LLPanelRegionTools::onApplyChanges(void* userdata)
{
	LLViewerRegion *region = gAgent.getRegion();
	if (region 
		&& sGodTools 
		&& userdata
		&& gAgent.isGodlike())
	{
		LLPanelRegionTools* region_tools = (LLPanelRegionTools*) userdata;

		region_tools->childDisable("Apply");
		sGodTools->sendGodUpdateRegionInfo();
	}
}

// static 
void LLPanelRegionTools::onBakeTerrain(void *userdata)
{
	LLPanelRequestTools::sendRequest("terrain", "bake", gAgent.getRegionHost());
}

// static 
void LLPanelRegionTools::onRevertTerrain(void *userdata)
{
	LLPanelRequestTools::sendRequest("terrain", "revert", gAgent.getRegionHost());
}

// static 
void LLPanelRegionTools::onSwapTerrain(void *userdata)
{
	LLPanelRequestTools::sendRequest("terrain", "swap", gAgent.getRegionHost());
}

// static
void LLPanelRegionTools::onSelectRegion(void* userdata)
{
	llinfos << "LLPanelRegionTools::onSelectRegion" << llendl;

	if (!gWorldp)
	{
		return;
	}
	LLViewerRegion *regionp = gWorldp->getRegionFromPosGlobal(gAgent.getPositionGlobal());
	if (!regionp)
	{
		return;
	}

	LLVector3d north_east(REGION_WIDTH_METERS, REGION_WIDTH_METERS, 0);
	gParcelMgr->selectLand(regionp->getOriginGlobal(), 
						   regionp->getOriginGlobal() + north_east, FALSE);
	
}


//*****************************************************************************
// Class LLPanelGridTools
//*****************************************************************************

//   || Grid   |_____________________________________
//   |                                               |
//   |                                               |
//   |  Sun Phase: >--------[]---------< [________]  |
//   |                                               |
//   |  ^         ^                                  |
//   |  LEFT      R1                                 |
//   |                                               |
//   |  [Kick all users]                             | 
//   |                                               |
//   |                                               |
//   |                                               |
//   |                                               |
//   |                                               |
//   |_______________________________________________|
//      ^                                ^        ^
//      LEFT                             R2       RIGHT

const F32 HOURS_TO_RADIANS = (2.f*F_PI)/24.f;
const char FLOATER_GRID_ADMIN_TITLE[] = "Grid Administration";


LLPanelGridTools::LLPanelGridTools(const std::string& name) :
	LLPanel(name)
{
}

// Destroys the object
LLPanelGridTools::~LLPanelGridTools()
{
}

BOOL LLPanelGridTools::postBuild()
{
	childSetAction("Kick all users", onClickKickAll, this);
	childSetAction("Flush This Region's Map Visibility Caches", onClickFlushMapVisibilityCaches, this);

	return TRUE;
}

void LLPanelGridTools::refresh()
{
}


// static
void LLPanelGridTools::onClickKickAll(void* userdata)
{
	LLPanelGridTools* self = (LLPanelGridTools*) userdata;

	S32 left, top;
	gFloaterView->getNewFloaterPosition(&left, &top);
	LLRect rect(left, top, left+400, top-300);

	gViewerWindow->alertXmlEditText("KickAllUsers", LLString::format_map_t(),
									NULL, NULL,
									LLPanelGridTools::confirmKick, self);
}


void LLPanelGridTools::confirmKick(S32 option, const LLString& text, void* userdata)
{
	LLPanelGridTools* self = (LLPanelGridTools*) userdata;

	if (option == 0)
	{
		self->mKickMessage = text;
		gViewerWindow->alertXml("ConfirmKick",LLPanelGridTools::finishKick, self);
	}
}


// static
void LLPanelGridTools::finishKick(S32 option, void* userdata)
{
	LLPanelGridTools* self = (LLPanelGridTools*) userdata;

	if (option == 0)
	{
		LLMessageSystem* msg = gMessageSystem;

		msg->newMessageFast(_PREHASH_GodKickUser);
		msg->nextBlockFast(_PREHASH_UserInfo);
		msg->addUUIDFast(_PREHASH_GodID, gAgent.getID());
		msg->addUUIDFast(_PREHASH_GodSessionID, gAgent.getSessionID());
		msg->addUUIDFast(_PREHASH_AgentID,   LL_UUID_ALL_AGENTS );
		msg->addU32("KickFlags", KICK_FLAGS_DEFAULT );
		msg->addStringFast(_PREHASH_Reason,    self->mKickMessage.c_str() );
		gAgent.sendReliableMessage();
	}
}


// static
void LLPanelGridTools::onClickFlushMapVisibilityCaches(void* data)
{
	gViewerWindow->alertXml("FlushMapVisibilityCaches",
							flushMapVisibilityCachesConfirm, data);
}

// static
void LLPanelGridTools::flushMapVisibilityCachesConfirm(S32 option, void* data)
{
	if (option != 0) return;

	LLPanelGridTools* self = (LLPanelGridTools*)data;
	if (!self) return;

	// HACK: Send this as an EstateOwnerRequest so it gets routed
	// correctly by the spaceserver. JC
	LLMessageSystem* msg = gMessageSystem;
	msg->newMessage("EstateOwnerMessage");
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->addUUIDFast(_PREHASH_TransactionID, LLUUID::null); //not used
	msg->nextBlock("MethodData");
	msg->addString("Method", "refreshmapvisibility");
	msg->addUUID("Invoice", LLUUID::null);
	msg->nextBlock("ParamList");
	msg->addString("Parameter", gAgent.getID().asString());
	gAgent.sendReliableMessage();
}


//*****************************************************************************
// LLPanelObjectTools
//*****************************************************************************


//   || Object |_______________________________________________________
//   |                                                                 |
//   |  Sim Name: Foo                                                  |
//   |  ^         ^                                                    |
//   |  LEFT      R1                                                   |
//   |                                                                 |
//   |  [X] Disable Scripts [X] Disable Collisions [X] Disable Physics |
//   |                                                  [ Apply  ]     |
//   |                                                                 |
//   |  [Set Target Avatar]	Avatar Name                                |
//   |  [Delete Target's Objects on Public Land	   ]                   |
//   |  [Delete All Target's Objects			   ]                   |
//   |  [Delete All Scripted Objects on Public Land]                   |
//   |  [Get Top Colliders ]                                           |
//   |  [Get Top Scripts   ]                                           |
//   |_________________________________________________________________|
//      ^                                         ^
//      LEFT                                      RIGHT

// Default constructor
LLPanelObjectTools::LLPanelObjectTools(const std::string& title) 
: 	LLPanel(title), mTargetAvatar()
{
}

// Destroys the object
LLPanelObjectTools::~LLPanelObjectTools()
{
	// base class will take care of everything
}

BOOL LLPanelObjectTools::postBuild()
{
	childSetCommitCallback("disable scripts", onChangeAnything, this);
	childSetCommitCallback("disable collisions", onChangeAnything, this);
	childSetCommitCallback("disable physics", onChangeAnything, this);

	childSetAction("Apply", onApplyChanges, this);

	childSetAction("Set Target", onClickSet, this);

	childSetAction("Delete Target's Scripted Objects On Others Land", onClickDeletePublicOwnedBy, this);
	childSetAction("Delete Target's Scripted Objects On *Any* Land", onClickDeleteAllScriptedOwnedBy, this);
	childSetAction("Delete *ALL* Of Target's Objects", onClickDeleteAllOwnedBy, this);

	childSetAction("Get Top Colliders", onGetTopColliders, this);
	childSetAction("Get Top Scripts", onGetTopScripts, this);
	childSetAction("Scripts digest", onGetScriptDigest, this);

	return TRUE;
}

void LLPanelObjectTools::setTargetAvatar(const LLUUID &target_id)
{
	mTargetAvatar = target_id;
	if (target_id.isNull())
	{
		childSetValue("target_avatar_name", "(no target)");
	}
} 


void LLPanelObjectTools::refresh()
{
	LLViewerRegion *regionp = gAgent.getRegion();
	if (regionp)
	{
		childSetText("region name", regionp->getName());
	}
}


U32 LLPanelObjectTools::computeRegionFlags(U32 flags) const
{
	if (childGetValue("disable scripts").asBoolean())
	{
		flags |= REGION_FLAGS_SKIP_SCRIPTS;
	}
	else
	{
		flags &= ~REGION_FLAGS_SKIP_SCRIPTS;
	}
	if (childGetValue("disable collisions").asBoolean())
	{
		flags |= REGION_FLAGS_SKIP_COLLISIONS;
	}
	else
	{
		flags &= ~REGION_FLAGS_SKIP_COLLISIONS;
	}
	if (childGetValue("disable physics").asBoolean())
	{
		flags |= REGION_FLAGS_SKIP_PHYSICS;
	}
	else
	{
		flags &= ~REGION_FLAGS_SKIP_PHYSICS;
	}
	return flags;
}


void LLPanelObjectTools::setCheckFlags(U32 flags)
{
	childSetValue("disable scripts", flags & REGION_FLAGS_SKIP_SCRIPTS ? TRUE : FALSE);
	childSetValue("disable collisions", flags & REGION_FLAGS_SKIP_COLLISIONS ? TRUE : FALSE);
	childSetValue("disable physics", flags & REGION_FLAGS_SKIP_PHYSICS ? TRUE : FALSE);
}


void LLPanelObjectTools::clearAllWidgets()
{
	childSetValue("disable scripts", FALSE);
	childDisable("disable scripts");

	childDisable("Apply");
	childDisable("Set Target");
	childDisable("Delete Target's Scripted Objects On Others Land");
	childDisable("Delete Target's Scripted Objects On *Any* Land");
	childDisable("Delete *ALL* Of Target's Objects");
}


void LLPanelObjectTools::enableAllWidgets()
{
	childEnable("disable scripts");

	childDisable("Apply");	// don't enable this one
	childEnable("Set Target");
	childEnable("Delete Target's Scripted Objects On Others Land");
	childEnable("Delete Target's Scripted Objects On *Any* Land");
	childEnable("Delete *ALL* Of Target's Objects");
	childEnable("Get Top Colliders");
	childEnable("Get Top Scripts");
}


// static
void LLPanelObjectTools::onGetTopColliders(void* userdata)
{
	if (sGodTools 
		&& gAgent.isGodlike())
	{
		LLFloaterTopObjects::show();
		LLFloaterTopObjects::setMode(STAT_REPORT_TOP_COLLIDERS);
		LLFloaterTopObjects::onRefresh(NULL);
	}
}

// static
void LLPanelObjectTools::onGetTopScripts(void* userdata)
{
	if (sGodTools 
		&& gAgent.isGodlike()) 
	{
		LLFloaterTopObjects::show();
		LLFloaterTopObjects::setMode(STAT_REPORT_TOP_SCRIPTS);
		LLFloaterTopObjects::onRefresh(NULL);
	}
}

// static
void LLPanelObjectTools::onGetScriptDigest(void* userdata)
{
	if (sGodTools 
		&& gAgent.isGodlike())
	{
		// get the list of scripts and number of occurences of each
		// (useful for finding self-replicating objects)
		LLPanelRequestTools::sendRequest("scriptdigest","0",gAgent.getRegionHost());
	}
}

void LLPanelObjectTools::onClickDeletePublicOwnedBy(void* userdata)
{
	// Bring up view-modal dialog
	LLPanelObjectTools* panelp = (LLPanelObjectTools*)userdata;
	if (!panelp->mTargetAvatar.isNull())
	{
		panelp->mSimWideDeletesFlags = 
			SWD_SCRIPTED_ONLY | SWD_OTHERS_LAND_ONLY;

		LLStringBase<char>::format_map_t args;
		args["[AVATAR_NAME]"] = panelp->childGetValue("target_avatar_name").asString();

		gViewerWindow->alertXml( "GodDeleteAllScriptedPublicObjectsByUser",
								args,
								callbackSimWideDeletes, 
								userdata);
	}
}

// static
void LLPanelObjectTools::onClickDeleteAllScriptedOwnedBy(void* userdata)
{
	// Bring up view-modal dialog
	LLPanelObjectTools* panelp = (LLPanelObjectTools*)userdata;
	if (!panelp->mTargetAvatar.isNull())
	{
		panelp->mSimWideDeletesFlags = SWD_SCRIPTED_ONLY;

		LLStringBase<char>::format_map_t args;
		args["[AVATAR_NAME]"] = panelp->childGetValue("target_avatar_name").asString();

		gViewerWindow->alertXml( "GodDeleteAllScriptedObjectsByUser",
								args,
								callbackSimWideDeletes, 
								userdata);
	}
}

// static
void LLPanelObjectTools::onClickDeleteAllOwnedBy(void* userdata)
{
	// Bring up view-modal dialog
	LLPanelObjectTools* panelp = (LLPanelObjectTools*)userdata;
	if (!panelp->mTargetAvatar.isNull())
	{
		panelp->mSimWideDeletesFlags = 0;

		LLStringBase<char>::format_map_t args;
		args["[AVATAR_NAME]"] = panelp->childGetValue("target_avatar_name").asString();

		gViewerWindow->alertXml( "GodDeleteAllObjectsByUser",
								args,
								callbackSimWideDeletes, 
								userdata);
	}
}

// static
void LLPanelObjectTools::callbackSimWideDeletes( S32 option, void* userdata )
{
	if (option == 0)
	{
		LLPanelObjectTools* object_tools = (LLPanelObjectTools*) userdata;
		if (!object_tools->mTargetAvatar.isNull())
		{
			send_sim_wide_deletes(object_tools->mTargetAvatar, 
								  object_tools->mSimWideDeletesFlags);
		}
	}
}

void LLPanelObjectTools::onClickSet(void* data)
{
	LLPanelObjectTools* panelp = (LLPanelObjectTools*) data;
	// grandparent is a floater, which can have a dependent
	gFloaterView->getParentFloater(panelp)->addDependentFloater(LLFloaterAvatarPicker::show(callbackAvatarID, data));
}

void LLPanelObjectTools::onClickSetBySelection(void* data)
{
	LLPanelObjectTools* panelp = (LLPanelObjectTools*) data;
	if (!panelp) return;

	LLSelectNode* node = gSelectMgr->getSelection()->getFirstRootNode();
	if (!node) node = gSelectMgr->getSelection()->getFirstNode();
	if (!node) return;

	LLString owner_name;
	LLUUID owner_id;
	gSelectMgr->selectGetOwner(owner_id, owner_name);

	panelp->mTargetAvatar = owner_id;
	LLString name = "Object " + node->mName + " owned by " + owner_name;
	panelp->childSetValue("target_avatar_name", name);
}

// static
void LLPanelObjectTools::callbackAvatarID(const std::vector<std::string>& names, const std::vector<LLUUID>& ids, void* data)
{
	LLPanelObjectTools* object_tools = (LLPanelObjectTools*) data;
	if (ids.empty() || names.empty()) return;
	object_tools->mTargetAvatar = ids[0];
	object_tools->childSetValue("target_avatar_name", names[0]);
	object_tools->refresh();
}


// static
void LLPanelObjectTools::onChangeAnything(LLUICtrl* ctrl, void* userdata)
{
	if (sGodTools 
		&& userdata
		&& gAgent.isGodlike())
	{
		LLPanelObjectTools* object_tools = (LLPanelObjectTools*) userdata;
		object_tools->childEnable("Apply");
	}
}

// static
void LLPanelObjectTools::onApplyChanges(void* userdata)
{
	LLViewerRegion *region = gAgent.getRegion();
	if (region 
		&& sGodTools 
		&& userdata
		&& gAgent.isGodlike())
	{
		LLPanelObjectTools* object_tools = (LLPanelObjectTools*) userdata;
		// TODO -- implement this

		object_tools->childDisable("Apply");
		sGodTools->sendGodUpdateRegionInfo();
	}
}


// --------------------
// LLPanelRequestTools
// --------------------

const char SELECTION[] = "Selection";
const char AGENT_REGION[] = "Agent Region";

LLPanelRequestTools::LLPanelRequestTools(const std::string& name):
	LLPanel(name)
{
}

LLPanelRequestTools::~LLPanelRequestTools()
{
}

BOOL LLPanelRequestTools::postBuild()
{
	childSetAction("Make Request", onClickRequest, this);

	refresh();

	return TRUE;
}

void LLPanelRequestTools::refresh()
{
	std::string buffer = childGetValue("destination");
	LLCtrlListInterface *list = childGetListInterface("destination");
	if (!list) return;

	list->operateOnAll(LLCtrlListInterface::OP_DELETE);
	list->addSimpleElement(SELECTION);
	list->addSimpleElement(AGENT_REGION);
	LLViewerRegion* regionp;
	for(regionp = gWorldp->mActiveRegionList.getFirstData();
		regionp != NULL;
		regionp = gWorldp->mActiveRegionList.getNextData())
	{
		LLString name = regionp->getName();
		if (!name.empty())
		{
			list->addSimpleElement(name);
		}
	}
	if(!buffer.empty())
	{
		list->selectByValue(buffer);
	}
	else
	{
		list->selectByValue(SELECTION);
	}
}


// static
void LLPanelRequestTools::sendRequest(const char *request, 
									  const char *parameter, 
									  const LLHost& host)
{
	llinfos << "Sending request '" << request << "', '"
			<< parameter << "' to " << host << llendl;
	LLMessageSystem* msg = gMessageSystem;
	msg->newMessage("GodlikeMessage");
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->addUUIDFast(_PREHASH_TransactionID, LLUUID::null); //not used
	msg->nextBlock("MethodData");
	msg->addString("Method", request);
	msg->addUUID("Invoice", LLUUID::null);
	msg->nextBlock("ParamList");
	msg->addString("Parameter", parameter);
	msg->sendReliable(host);
}

// static
void LLPanelRequestTools::onClickRequest(void* data)
{
	LLPanelRequestTools* self = (LLPanelRequestTools*)data;
	const std::string dest = self->childGetValue("destination").asString();
	if(dest == SELECTION)
	{
		std::string req = self->childGetValue("request");
		req = req.substr(0, req.find_first_of(" "));
		std::string param = self->childGetValue("parameter");
		gSelectMgr->sendGodlikeRequest(req, param);
	}
	else if(dest == AGENT_REGION)
	{
		self->sendRequest(gAgent.getRegionHost());
	}
	else
	{
		// find region by name
		LLViewerRegion* regionp;
		for(regionp = gWorldp->mActiveRegionList.getFirstData();
			regionp != NULL;
			regionp = gWorldp->mActiveRegionList.getNextData())
		{
			if(dest == regionp->getName())
			{
				// found it
				self->sendRequest(regionp->getHost());
			}
		}
	}
}

void terrain_download_done(void** data, S32 status)
{
	LLNotifyBox::showXml("TerrainDownloaded");
}


void test_callback(const LLTSCode status)
{
	llinfos << "Test transfer callback returned!" << llendl;
}


void LLPanelRequestTools::sendRequest(const LLHost& host)
{

	// intercept viewer local actions here
	std::string req = childGetValue("request");
	if (req == "terrain download")
	{
		gXferManager->requestFile("terrain.raw", "terrain.raw", LL_PATH_NONE,
								  host,
								  FALSE,
								  terrain_download_done,
								  NULL);
	}
	else
	{
		req = req.substr(0, req.find_first_of(" "));
		sendRequest(req.c_str(), childGetValue("parameter").asString().c_str(), host);
	}
}

// Flags are SWD_ flags.
void send_sim_wide_deletes(const LLUUID& owner_id, U32 flags)
{
	LLMessageSystem* msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_SimWideDeletes);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->nextBlockFast(_PREHASH_DataBlock);
	msg->addUUIDFast(_PREHASH_TargetID, owner_id);
	msg->addU32Fast(_PREHASH_Flags, flags);
	gAgent.sendReliableMessage();
}
