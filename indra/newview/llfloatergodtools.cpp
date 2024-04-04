/** 
 * @file llfloatergodtools.cpp
 * @brief The on-screen rectangle with tool options.
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#include "llfloatergodtools.h"

#include "llavatarnamecache.h"
#include "llcoord.h"
#include "llfontgl.h"
#include "llframetimer.h"
#include "llgl.h"
#include "llhost.h"
#include "llnotificationsutil.h"
#include "llregionflags.h"
#include "llstring.h"
#include "message.h"

#include "llagent.h"
#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "llcombobox.h"
#include "lldraghandle.h"
#include "llfloater.h"
#include "llfloaterreg.h"
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
#include "llxfermanager.h"
#include "llvlcomposition.h"
#include "llsurface.h"
#include "llviewercontrol.h"
#include "lluictrlfactory.h"
#include "lltrans.h"

#include "lltransfertargetfile.h"
#include "lltransfersourcefile.h"

const F32 SECONDS_BETWEEN_UPDATE_REQUESTS = 5.0f;

//*****************************************************************************
// LLFloaterGodTools
//*****************************************************************************

void LLFloaterGodTools::onOpen(const LLSD& key)
{
	center();
	setFocus(true);
// 	LLPanel *panel = getChild<LLTabContainer>("GodTools Tabs")->getCurrentPanel();
// 	if (panel)
// 		panel->setFocus(true);
	if (mPanelObjectTools)
		mPanelObjectTools->setTargetAvatar(LLUUID::null);

	if (gAgent.getRegionHost() != mCurrentHost)
	{
		// we're in a new region
		sendRegionInfoRequest();
	}
}
 

// static
void LLFloaterGodTools::refreshAll()
{
	LLFloaterGodTools* god_tools = LLFloaterReg::getTypedInstance<LLFloaterGodTools>("god_tools");
	if (god_tools)
	{
		if (gAgent.getRegionHost() != god_tools->mCurrentHost)
		{
			// we're in a new region
			god_tools->sendRegionInfoRequest();
		}
	}
}



LLFloaterGodTools::LLFloaterGodTools(const LLSD& key)
:	LLFloater(key),
	mCurrentHost(LLHost()),
	mUpdateTimer()
{
	mFactoryMap["grid"] = LLCallbackMap(createPanelGrid, this);
	mFactoryMap["region"] = LLCallbackMap(createPanelRegion, this);
	mFactoryMap["objects"] = LLCallbackMap(createPanelObjects, this);
	mFactoryMap["request"] = LLCallbackMap(createPanelRequest, this);
}

bool LLFloaterGodTools::postBuild()
{
	sendRegionInfoRequest();
	getChild<LLTabContainer>("GodTools Tabs")->selectTabByName("region");
	return true;
}
// static
void* LLFloaterGodTools::createPanelGrid(void *userdata)
{
	return new LLPanelGridTools();
}

// static
void* LLFloaterGodTools::createPanelRegion(void *userdata)
{
	LLFloaterGodTools* self = (LLFloaterGodTools*)userdata;
	self->mPanelRegionTools = new LLPanelRegionTools();
	return self->mPanelRegionTools;
}

// static
void* LLFloaterGodTools::createPanelObjects(void *userdata)
{
	LLFloaterGodTools* self = (LLFloaterGodTools*)userdata;
	self->mPanelObjectTools = new LLPanelObjectTools();
	return self->mPanelObjectTools;
}

// static
void* LLFloaterGodTools::createPanelRequest(void *userdata)
{
	return new LLPanelRequestTools();
}

LLFloaterGodTools::~LLFloaterGodTools()
{
	// children automatically deleted
}


U64 LLFloaterGodTools::computeRegionFlags() const
{
	U64 flags = gAgent.getRegion()->getRegionFlags();
	if (mPanelRegionTools) flags = mPanelRegionTools->computeRegionFlags(flags);
	if (mPanelObjectTools) flags = mPanelObjectTools->computeRegionFlags(flags);
	return flags;
}


void LLFloaterGodTools::updatePopup(LLCoordGL center, MASK mask)
{
}

// virtual
void LLFloaterGodTools::draw()
{
	if (mCurrentHost == LLHost())
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

void LLFloaterGodTools::showPanel(const std::string& panel_name)
{
	getChild<LLTabContainer>("GodTools Tabs")->selectTabByName(panel_name);
	openFloater();
	LLPanel *panel = getChild<LLTabContainer>("GodTools Tabs")->getCurrentPanel();
	if (panel)
		panel->setFocus(true);
}

// static
void LLFloaterGodTools::processRegionInfo(LLMessageSystem* msg)
{
	llassert(msg);
	if (!msg) return;

	//const S32 SIM_NAME_BUF = 256;
	U64 region_flags;
	U8 sim_access;
	U8 agent_limit;
	std::string sim_name;
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

	LLHost host = msg->getSender();

	msg->getStringFast(_PREHASH_RegionInfo, _PREHASH_SimName, sim_name);
	msg->getU32Fast(_PREHASH_RegionInfo, _PREHASH_EstateID, estate_id);
	msg->getU32Fast(_PREHASH_RegionInfo, _PREHASH_ParentEstateID, parent_estate_id);
	msg->getU8Fast(_PREHASH_RegionInfo, _PREHASH_SimAccess, sim_access);
	msg->getU8Fast(_PREHASH_RegionInfo, _PREHASH_MaxAgents, agent_limit);
	msg->getF32Fast(_PREHASH_RegionInfo, _PREHASH_ObjectBonusFactor, object_bonus_factor);
	msg->getF32Fast(_PREHASH_RegionInfo, _PREHASH_BillableFactor, billable_factor);
	msg->getF32Fast(_PREHASH_RegionInfo, _PREHASH_WaterHeight, water_height);

	if (msg->has(_PREHASH_RegionInfo3))
	{
		msg->getU64Fast(_PREHASH_RegionInfo3, _PREHASH_RegionFlagsExtended, region_flags);
	}
	else
	{
		U32 flags = 0;
		msg->getU32Fast(_PREHASH_RegionInfo, _PREHASH_RegionFlags, flags);
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

	if (host != gAgent.getRegionHost())
	{
		// Update is for a different region than the one we're in.
		// Just check for a waterheight change.
		LLWorld::getInstance()->waterHeightRegionInfo(sim_name, water_height);
		return;
	}

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
	
	LLFloaterGodTools* god_tools = LLFloaterReg::getTypedInstance<LLFloaterGodTools>("god_tools");
	if (!god_tools) return;

	// push values to god tools, if available
	if ( gAgent.isGodlike()
		&& LLFloaterReg::instanceVisible("god_tools")
		&& god_tools->mPanelRegionTools
		&& god_tools->mPanelObjectTools)
	{
		LLPanelRegionTools* rtool = god_tools->mPanelRegionTools;
		god_tools->mCurrentHost = host;

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

		LLPanelObjectTools *otool = god_tools->mPanelObjectTools;
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
	mCurrentHost = LLHost();
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
	LLFloaterGodTools* god_tools = LLFloaterReg::getTypedInstance<LLFloaterGodTools>("god_tools");
	if (!god_tools) return;

	LLViewerRegion *regionp = gAgent.getRegion();
	if (gAgent.isGodlike()
		&& god_tools->mPanelRegionTools
		&& regionp
		&& gAgent.getRegionHost() == mCurrentHost)
	{
		LLMessageSystem *msg = gMessageSystem;
		LLPanelRegionTools *rtool = god_tools->mPanelRegionTools;

		U64 region_flags = computeRegionFlags();
		msg->newMessage("GodUpdateRegionInfo");
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		msg->nextBlockFast(_PREHASH_RegionInfo);
		msg->addStringFast(_PREHASH_SimName, rtool->getSimName());
		msg->addU32Fast(_PREHASH_EstateID, rtool->getEstateID());
		msg->addU32Fast(_PREHASH_ParentEstateID, rtool->getParentEstateID());
		// Legacy flags
		msg->addU32Fast(_PREHASH_RegionFlags, U32(region_flags));
		msg->addF32Fast(_PREHASH_BillableFactor, rtool->getBillableFactor());
		msg->addS32Fast(_PREHASH_PricePerMeter, rtool->getPricePerMeter());
		msg->addS32Fast(_PREHASH_RedirectGridX, rtool->getRedirectGridX());
		msg->addS32Fast(_PREHASH_RedirectGridY, rtool->getRedirectGridY());
		msg->nextBlockFast(_PREHASH_RegionInfo2);
		msg->addU64Fast(_PREHASH_RegionFlagsExtended, region_flags);

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

// floats because spinners only understand floats. JC
const F32 PRICE_PER_METER_DEFAULT = 1.f;

LLPanelRegionTools::LLPanelRegionTools()
: 	LLPanel()
{
	mCommitCallbackRegistrar.add("RegionTools.ChangeAnything",	boost::bind(&LLPanelRegionTools::onChangeAnything, this));
	mCommitCallbackRegistrar.add("RegionTools.ChangePrelude",	boost::bind(&LLPanelRegionTools::onChangePrelude, this));
	mCommitCallbackRegistrar.add("RegionTools.BakeTerrain",		boost::bind(&LLPanelRegionTools::onBakeTerrain, this));
	mCommitCallbackRegistrar.add("RegionTools.RevertTerrain",	boost::bind(&LLPanelRegionTools::onRevertTerrain, this));	
	mCommitCallbackRegistrar.add("RegionTools.SwapTerrain",		boost::bind(&LLPanelRegionTools::onSwapTerrain, this));		
	mCommitCallbackRegistrar.add("RegionTools.Refresh",			boost::bind(&LLPanelRegionTools::onRefresh, this));		
	mCommitCallbackRegistrar.add("RegionTools.ApplyChanges",	boost::bind(&LLPanelRegionTools::onApplyChanges, this));	
	mCommitCallbackRegistrar.add("RegionTools.SelectRegion",	boost::bind(&LLPanelRegionTools::onSelectRegion, this));
	mCommitCallbackRegistrar.add("RegionTools.SaveState",		boost::bind(&LLPanelRegionTools::onSaveState, this));	
}

bool LLPanelRegionTools::postBuild()
{
	getChild<LLLineEditor>("region name")->setKeystrokeCallback(onChangeSimName, this);
	getChild<LLLineEditor>("region name")->setPrevalidate(&LLTextValidate::validateASCIIPrintableNoPipe);
	getChild<LLLineEditor>("estate")->setPrevalidate(&LLTextValidate::validatePositiveS32);
	getChild<LLLineEditor>("parentestate")->setPrevalidate(&LLTextValidate::validatePositiveS32);
	getChildView("parentestate")->setEnabled(false);
	getChild<LLLineEditor>("gridposx")->setPrevalidate(&LLTextValidate::validatePositiveS32);
	getChildView("gridposx")->setEnabled(false);
	getChild<LLLineEditor>("gridposy")->setPrevalidate(&LLTextValidate::validatePositiveS32);
	getChildView("gridposy")->setEnabled(false);
	
	getChild<LLLineEditor>("redirectx")->setPrevalidate(&LLTextValidate::validatePositiveS32);
	getChild<LLLineEditor>("redirecty")->setPrevalidate(&LLTextValidate::validatePositiveS32);
			 
	return true;
}

// Destroys the object
LLPanelRegionTools::~LLPanelRegionTools()
{
	// base class will take care of everything
}

U64 LLPanelRegionTools::computeRegionFlags(U64 flags) const
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
	getChild<LLUICtrl>("region name")->setValue("unknown");
	getChild<LLUICtrl>("region name")->setFocus( false);

	getChild<LLUICtrl>("check prelude")->setValue(false);
	getChildView("check prelude")->setEnabled(false);

	getChild<LLUICtrl>("check fixed sun")->setValue(false);
	getChildView("check fixed sun")->setEnabled(false);

	getChild<LLUICtrl>("check reset home")->setValue(false);
	getChildView("check reset home")->setEnabled(false);

	getChild<LLUICtrl>("check damage")->setValue(false);
	getChildView("check damage")->setEnabled(false);

	getChild<LLUICtrl>("check visible")->setValue(false);
	getChildView("check visible")->setEnabled(false);

	getChild<LLUICtrl>("block terraform")->setValue(false);
	getChildView("block terraform")->setEnabled(false);

	getChild<LLUICtrl>("block dwell")->setValue(false);
	getChildView("block dwell")->setEnabled(false);

	getChild<LLUICtrl>("is sandbox")->setValue(false);
	getChildView("is sandbox")->setEnabled(false);

	getChild<LLUICtrl>("billable factor")->setValue(BILLABLE_FACTOR_DEFAULT);
	getChildView("billable factor")->setEnabled(false);

	getChild<LLUICtrl>("land cost")->setValue(PRICE_PER_METER_DEFAULT);
	getChildView("land cost")->setEnabled(false);

	getChildView("Apply")->setEnabled(false);
	getChildView("Bake Terrain")->setEnabled(false);
	getChildView("Autosave now")->setEnabled(false);
}


void LLPanelRegionTools::enableAllWidgets()
{
	// enable all of the widgets
	
	getChildView("check prelude")->setEnabled(true);
	getChildView("check fixed sun")->setEnabled(true);
	getChildView("check reset home")->setEnabled(true);
	getChildView("check damage")->setEnabled(true);
	getChildView("check visible")->setEnabled(false); // use estates to update...
	getChildView("block terraform")->setEnabled(true);
	getChildView("block dwell")->setEnabled(true);
	getChildView("is sandbox")->setEnabled(true);
	
	getChildView("billable factor")->setEnabled(true);
	getChildView("land cost")->setEnabled(true);

	getChildView("Apply")->setEnabled(false);	// don't enable this one
	getChildView("Bake Terrain")->setEnabled(true);
	getChildView("Autosave now")->setEnabled(true);
}

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
	return getChild<LLUICtrl>("region name")->getValue();
}

U32 LLPanelRegionTools::getEstateID() const
{
	U32 id = (U32)getChild<LLUICtrl>("estate")->getValue().asInteger();
	return id;
}

U32 LLPanelRegionTools::getParentEstateID() const
{
	U32 id = (U32)getChild<LLUICtrl>("parentestate")->getValue().asInteger();
	return id;
}

S32 LLPanelRegionTools::getRedirectGridX() const
{
	return getChild<LLUICtrl>("redirectx")->getValue().asInteger();
}

S32 LLPanelRegionTools::getRedirectGridY() const
{
	return getChild<LLUICtrl>("redirecty")->getValue().asInteger();
}

S32 LLPanelRegionTools::getGridPosX() const
{
	return getChild<LLUICtrl>("gridposx")->getValue().asInteger();
}

S32 LLPanelRegionTools::getGridPosY() const
{
	return getChild<LLUICtrl>("gridposy")->getValue().asInteger();
}

U64 LLPanelRegionTools::getRegionFlags() const
{
	U64 flags = 0x0;
	flags = getChild<LLUICtrl>("check prelude")->getValue().asBoolean()  
					? set_prelude_flags(flags)
					: unset_prelude_flags(flags);

	// override prelude
	if (getChild<LLUICtrl>("check fixed sun")->getValue().asBoolean())
	{
		flags |= REGION_FLAGS_SUN_FIXED;
	}
	if (getChild<LLUICtrl>("check reset home")->getValue().asBoolean())
	{
		flags |= REGION_FLAGS_RESET_HOME_ON_TELEPORT;
	}
	if (getChild<LLUICtrl>("check visible")->getValue().asBoolean())
	{
		flags |= REGION_FLAGS_EXTERNALLY_VISIBLE;
	}
	if (getChild<LLUICtrl>("check damage")->getValue().asBoolean())
	{
		flags |= REGION_FLAGS_ALLOW_DAMAGE;
	}
	if (getChild<LLUICtrl>("block terraform")->getValue().asBoolean())
	{
		flags |= REGION_FLAGS_BLOCK_TERRAFORM;
	}
	if (getChild<LLUICtrl>("block dwell")->getValue().asBoolean())
	{
		flags |= REGION_FLAGS_BLOCK_DWELL;
	}
	if (getChild<LLUICtrl>("is sandbox")->getValue().asBoolean())
	{
		flags |= REGION_FLAGS_SANDBOX;
	}
	return flags;
}

U64 LLPanelRegionTools::getRegionFlagsMask() const
{
	U64 flags = 0xFFFFFFFFFFFFFFFFULL;
	flags = getChild<LLUICtrl>("check prelude")->getValue().asBoolean()
				? set_prelude_flags(flags)
				: unset_prelude_flags(flags);

	if (!getChild<LLUICtrl>("check fixed sun")->getValue().asBoolean())
	{
		flags &= ~REGION_FLAGS_SUN_FIXED;
	}
	if (!getChild<LLUICtrl>("check reset home")->getValue().asBoolean())
	{
		flags &= ~REGION_FLAGS_RESET_HOME_ON_TELEPORT;
	}
	if (!getChild<LLUICtrl>("check visible")->getValue().asBoolean())
	{
		flags &= ~REGION_FLAGS_EXTERNALLY_VISIBLE;
	}
	if (!getChild<LLUICtrl>("check damage")->getValue().asBoolean())
	{
		flags &= ~REGION_FLAGS_ALLOW_DAMAGE;
	}
	if (!getChild<LLUICtrl>("block terraform")->getValue().asBoolean())
	{
		flags &= ~REGION_FLAGS_BLOCK_TERRAFORM;
	}
	if (!getChild<LLUICtrl>("block dwell")->getValue().asBoolean())
	{
		flags &= ~REGION_FLAGS_BLOCK_DWELL;
	}
	if (!getChild<LLUICtrl>("is sandbox")->getValue().asBoolean())
	{
		flags &= ~REGION_FLAGS_SANDBOX;
	}
	return flags;
}

F32 LLPanelRegionTools::getBillableFactor() const
{
	return (F32)getChild<LLUICtrl>("billable factor")->getValue().asReal();
}

S32 LLPanelRegionTools::getPricePerMeter() const
{
	return getChild<LLUICtrl>("land cost")->getValue();
}

void LLPanelRegionTools::setSimName(const std::string& name)
{
	getChild<LLUICtrl>("region name")->setValue(name);
}

void LLPanelRegionTools::setEstateID(U32 id)
{
	getChild<LLUICtrl>("estate")->setValue((S32)id);
}

void LLPanelRegionTools::setGridPosX(S32 pos)
{
	getChild<LLUICtrl>("gridposx")->setValue(pos);
}

void LLPanelRegionTools::setGridPosY(S32 pos)
{
	getChild<LLUICtrl>("gridposy")->setValue(pos);
}

void LLPanelRegionTools::setRedirectGridX(S32 pos)
{
	getChild<LLUICtrl>("redirectx")->setValue(pos);
}

void LLPanelRegionTools::setRedirectGridY(S32 pos)
{
	getChild<LLUICtrl>("redirecty")->setValue(pos);
}

void LLPanelRegionTools::setParentEstateID(U32 id)
{
	getChild<LLUICtrl>("parentestate")->setValue((S32)id);
}

void LLPanelRegionTools::setCheckFlags(U64 flags)
{
	getChild<LLUICtrl>("check prelude")->setValue(is_prelude(flags) ? true : false);
	getChild<LLUICtrl>("check fixed sun")->setValue(flags & REGION_FLAGS_SUN_FIXED ? true : false);
	getChild<LLUICtrl>("check reset home")->setValue(flags & REGION_FLAGS_RESET_HOME_ON_TELEPORT ? true : false);
	getChild<LLUICtrl>("check damage")->setValue(flags & REGION_FLAGS_ALLOW_DAMAGE ? true : false);
	getChild<LLUICtrl>("check visible")->setValue(flags & REGION_FLAGS_EXTERNALLY_VISIBLE ? true : false);
	getChild<LLUICtrl>("block terraform")->setValue(flags & REGION_FLAGS_BLOCK_TERRAFORM ? true : false);
	getChild<LLUICtrl>("block dwell")->setValue(flags & REGION_FLAGS_BLOCK_DWELL ? true : false);
	getChild<LLUICtrl>("is sandbox")->setValue(flags & REGION_FLAGS_SANDBOX ? true : false );
}

void LLPanelRegionTools::setBillableFactor(F32 billable_factor)
{
	getChild<LLUICtrl>("billable factor")->setValue(billable_factor);
}

void LLPanelRegionTools::setPricePerMeter(S32 price)
{
	getChild<LLUICtrl>("land cost")->setValue(price);
}

void LLPanelRegionTools::onChangeAnything()
{
	if (gAgent.isGodlike())
	{
		getChildView("Apply")->setEnabled(true);
	}
}

void LLPanelRegionTools::onChangePrelude()
{
	// checking prelude auto-checks fixed sun
	if (getChild<LLUICtrl>("check prelude")->getValue().asBoolean())
	{
		getChild<LLUICtrl>("check fixed sun")->setValue(true);
		getChild<LLUICtrl>("check reset home")->setValue(true);
		onChangeAnything();
	}
	// pass on to default onChange handler

}

// static
void LLPanelRegionTools::onChangeSimName(LLLineEditor* caller, void* userdata )
{
	if (userdata && gAgent.isGodlike())
	{
		LLPanelRegionTools* region_tools = (LLPanelRegionTools*) userdata;
		region_tools->getChildView("Apply")->setEnabled(true);
	}
}


void LLPanelRegionTools::onRefresh()
{
	LLFloaterGodTools* god_tools = LLFloaterReg::getTypedInstance<LLFloaterGodTools>("god_tools");
	if(!god_tools) return;
	LLViewerRegion *region = gAgent.getRegion();
	if (region && gAgent.isGodlike())
	{
		god_tools->sendRegionInfoRequest();
		//LLFloaterGodTools::getInstance()->sendRegionInfoRequest();
		//LLFloaterReg::getTypedInstance<LLFloaterGodTools>("god_tools")->sendRegionInfoRequest();
	}
}

void LLPanelRegionTools::onApplyChanges()
{
	LLFloaterGodTools* god_tools = LLFloaterReg::getTypedInstance<LLFloaterGodTools>("god_tools");
	if(!god_tools) return;
	LLViewerRegion *region = gAgent.getRegion();
	if (region && gAgent.isGodlike())
	{
		getChildView("Apply")->setEnabled(false);
		god_tools->sendGodUpdateRegionInfo();
		//LLFloaterReg::getTypedInstance<LLFloaterGodTools>("god_tools")->sendGodUpdateRegionInfo();
	}
}

void LLPanelRegionTools::onBakeTerrain()
{
	LLPanelRequestTools::sendRequest("terrain", "bake", gAgent.getRegionHost());
}

void LLPanelRegionTools::onRevertTerrain()
{
	LLPanelRequestTools::sendRequest("terrain", "revert", gAgent.getRegionHost());
}


void LLPanelRegionTools::onSwapTerrain()
{
	LLPanelRequestTools::sendRequest("terrain", "swap", gAgent.getRegionHost());
}

void LLPanelRegionTools::onSelectRegion()
{
	LL_INFOS() << "LLPanelRegionTools::onSelectRegion" << LL_ENDL;

	LLViewerRegion *regionp = LLWorld::getInstance()->getRegionFromPosGlobal(gAgent.getPositionGlobal());
	if (!regionp)
	{
		return;
	}

	LLVector3d north_east(REGION_WIDTH_METERS, REGION_WIDTH_METERS, 0);
	LLViewerParcelMgr::getInstance()->selectLand(regionp->getOriginGlobal(), 
						   regionp->getOriginGlobal() + north_east, false);
	
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

LLPanelGridTools::LLPanelGridTools() :
	LLPanel()
{
	mCommitCallbackRegistrar.add("GridTools.FlushMapVisibilityCaches",		boost::bind(&LLPanelGridTools::onClickFlushMapVisibilityCaches, this));	
}

// Destroys the object
LLPanelGridTools::~LLPanelGridTools()
{
}

bool LLPanelGridTools::postBuild()
{
	return true;
}

void LLPanelGridTools::refresh()
{
}

void LLPanelGridTools::onClickFlushMapVisibilityCaches()
{
	LLNotificationsUtil::add("FlushMapVisibilityCaches", LLSD(), LLSD(), flushMapVisibilityCachesConfirm);
}

// static
bool LLPanelGridTools::flushMapVisibilityCachesConfirm(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if (option != 0) return false;

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
	return false;
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
LLPanelObjectTools::LLPanelObjectTools() 
	: 	LLPanel(),
		mTargetAvatar()
{
	mCommitCallbackRegistrar.add("ObjectTools.ChangeAnything",		boost::bind(&LLPanelObjectTools::onChangeAnything, this));
	mCommitCallbackRegistrar.add("ObjectTools.DeletePublicOwnedBy",	boost::bind(&LLPanelObjectTools::onClickDeletePublicOwnedBy, this));
	mCommitCallbackRegistrar.add("ObjectTools.DeleteAllScriptedOwnedBy",		boost::bind(&LLPanelObjectTools::onClickDeleteAllScriptedOwnedBy, this));
	mCommitCallbackRegistrar.add("ObjectTools.DeleteAllOwnedBy",		boost::bind(&LLPanelObjectTools::onClickDeleteAllOwnedBy, this));
	mCommitCallbackRegistrar.add("ObjectTools.ApplyChanges",		boost::bind(&LLPanelObjectTools::onApplyChanges, this));
	mCommitCallbackRegistrar.add("ObjectTools.Set",		boost::bind(&LLPanelObjectTools::onClickSet, this));
	mCommitCallbackRegistrar.add("ObjectTools.GetTopColliders",		boost::bind(&LLPanelObjectTools::onGetTopColliders, this));
	mCommitCallbackRegistrar.add("ObjectTools.GetTopScripts",		boost::bind(&LLPanelObjectTools::onGetTopScripts, this));
	mCommitCallbackRegistrar.add("ObjectTools.GetScriptDigest",		boost::bind(&LLPanelObjectTools::onGetScriptDigest, this));
}

// Destroys the object
LLPanelObjectTools::~LLPanelObjectTools()
{
	// base class will take care of everything
}

bool LLPanelObjectTools::postBuild()
{
	refresh();
	return true;
}

void LLPanelObjectTools::setTargetAvatar(const LLUUID &target_id)
{
	mTargetAvatar = target_id;
	if (target_id.isNull())
	{
		getChild<LLUICtrl>("target_avatar_name")->setValue(getString("no_target"));
	}
} 


void LLPanelObjectTools::refresh()
{
	LLViewerRegion *regionp = gAgent.getRegion();
	if (regionp)
	{
		getChild<LLUICtrl>("region name")->setValue(regionp->getName());
	}
}


U64 LLPanelObjectTools::computeRegionFlags(U64 flags) const
{
	if (getChild<LLUICtrl>("disable scripts")->getValue().asBoolean())
	{
		flags |= REGION_FLAGS_SKIP_SCRIPTS;
	}
	else
	{
		flags &= ~REGION_FLAGS_SKIP_SCRIPTS;
	}
	if (getChild<LLUICtrl>("disable collisions")->getValue().asBoolean())
	{
		flags |= REGION_FLAGS_SKIP_COLLISIONS;
	}
	else
	{
		flags &= ~REGION_FLAGS_SKIP_COLLISIONS;
	}
	if (getChild<LLUICtrl>("disable physics")->getValue().asBoolean())
	{
		flags |= REGION_FLAGS_SKIP_PHYSICS;
	}
	else
	{
		flags &= ~REGION_FLAGS_SKIP_PHYSICS;
	}
	return flags;
}


void LLPanelObjectTools::setCheckFlags(U64 flags)
{
	getChild<LLUICtrl>("disable scripts")->setValue(flags & REGION_FLAGS_SKIP_SCRIPTS ? true : false);
	getChild<LLUICtrl>("disable collisions")->setValue(flags & REGION_FLAGS_SKIP_COLLISIONS ? true : false);
	getChild<LLUICtrl>("disable physics")->setValue(flags & REGION_FLAGS_SKIP_PHYSICS ? true : false);
}


void LLPanelObjectTools::clearAllWidgets()
{
	getChild<LLUICtrl>("disable scripts")->setValue(false);
	getChildView("disable scripts")->setEnabled(false);

	getChildView("Apply")->setEnabled(false);
	getChildView("Set Target")->setEnabled(false);
	getChildView("Delete Target's Scripted Objects On Others Land")->setEnabled(false);
	getChildView("Delete Target's Scripted Objects On *Any* Land")->setEnabled(false);
	getChildView("Delete *ALL* Of Target's Objects")->setEnabled(false);
}


void LLPanelObjectTools::enableAllWidgets()
{
	getChildView("disable scripts")->setEnabled(true);

	getChildView("Apply")->setEnabled(false);	// don't enable this one
	getChildView("Set Target")->setEnabled(true);
	getChildView("Delete Target's Scripted Objects On Others Land")->setEnabled(true);
	getChildView("Delete Target's Scripted Objects On *Any* Land")->setEnabled(true);
	getChildView("Delete *ALL* Of Target's Objects")->setEnabled(true);
	getChildView("Get Top Colliders")->setEnabled(true);
	getChildView("Get Top Scripts")->setEnabled(true);
}


void LLPanelObjectTools::onGetTopColliders()
{
	LLFloaterTopObjects* instance = LLFloaterReg::getTypedInstance<LLFloaterTopObjects>("top_objects");
	if(!instance) return;
	
	if (gAgent.isGodlike())
	{
		LLFloaterReg::showInstance("top_objects");
		LLFloaterTopObjects::setMode(STAT_REPORT_TOP_COLLIDERS);
		instance->onRefresh();
	}
}

void LLPanelObjectTools::onGetTopScripts()
{
	LLFloaterTopObjects* instance = LLFloaterReg::getTypedInstance<LLFloaterTopObjects>("top_objects");
	if(!instance) return;
	
	if (gAgent.isGodlike()) 
	{
		LLFloaterReg::showInstance("top_objects");
		LLFloaterTopObjects::setMode(STAT_REPORT_TOP_SCRIPTS);
		instance->onRefresh();
	}
}

void LLPanelObjectTools::onGetScriptDigest()
{
	if (gAgent.isGodlike())
	{
		// get the list of scripts and number of occurences of each
		// (useful for finding self-replicating objects)
		LLPanelRequestTools::sendRequest("scriptdigest","0",gAgent.getRegionHost());
	}
}

void LLPanelObjectTools::onClickDeletePublicOwnedBy()
{
	// Bring up view-modal dialog

	if (!mTargetAvatar.isNull())
	{
		mSimWideDeletesFlags = 
			SWD_SCRIPTED_ONLY | SWD_OTHERS_LAND_ONLY;

		LLSD args;
		args["AVATAR_NAME"] = getChild<LLUICtrl>("target_avatar_name")->getValue().asString();
		LLSD payload;
		payload["avatar_id"] = mTargetAvatar;
		payload["flags"] = (S32)mSimWideDeletesFlags;

		LLNotificationsUtil::add( "GodDeleteAllScriptedPublicObjectsByUser",
								args,
								payload,
								callbackSimWideDeletes);
	}
}

void LLPanelObjectTools::onClickDeleteAllScriptedOwnedBy()
{
	// Bring up view-modal dialog
	if (!mTargetAvatar.isNull())
	{
		mSimWideDeletesFlags = SWD_SCRIPTED_ONLY;

		LLSD args;
		args["AVATAR_NAME"] = getChild<LLUICtrl>("target_avatar_name")->getValue().asString();
		LLSD payload;
		payload["avatar_id"] = mTargetAvatar;
		payload["flags"] = (S32)mSimWideDeletesFlags;

		LLNotificationsUtil::add( "GodDeleteAllScriptedObjectsByUser",
								args,
								payload,
								callbackSimWideDeletes);
	}
}

void LLPanelObjectTools::onClickDeleteAllOwnedBy()
{
	// Bring up view-modal dialog
	if (!mTargetAvatar.isNull())
	{
		mSimWideDeletesFlags = 0;

		LLSD args;
		args["AVATAR_NAME"] = getChild<LLUICtrl>("target_avatar_name")->getValue().asString();
		LLSD payload;
		payload["avatar_id"] = mTargetAvatar;
		payload["flags"] = (S32)mSimWideDeletesFlags;

		LLNotificationsUtil::add( "GodDeleteAllObjectsByUser",
								args,
								payload,
								callbackSimWideDeletes);
	}
}

// static
bool LLPanelObjectTools::callbackSimWideDeletes( const LLSD& notification, const LLSD& response )
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if (option == 0)
	{
		if (!notification["payload"]["avatar_id"].asUUID().isNull())
		{
			send_sim_wide_deletes(notification["payload"]["avatar_id"].asUUID(), 
								  notification["payload"]["flags"].asInteger());
		}
	}
	return false;
}

void LLPanelObjectTools::onClickSet()
{
    LLView * button = findChild<LLButton>("Set Target");
    LLFloater * root_floater = gFloaterView->getParentFloater(this);
	LLFloaterAvatarPicker* picker = LLFloaterAvatarPicker::show(boost::bind(&LLPanelObjectTools::callbackAvatarID, this, _1,_2), false, false, false, root_floater->getName(), button);
	// grandparent is a floater, which can have a dependent
	if (picker)
	{
		root_floater->addDependentFloater(picker);
	}
}

void LLPanelObjectTools::onClickSetBySelection(void* data)
{
	LLPanelObjectTools* panelp = (LLPanelObjectTools*) data;
	if (!panelp) return;

	const bool non_root_ok = true; 
	LLSelectNode* node = LLSelectMgr::getInstance()->getSelection()->getFirstRootNode(NULL, non_root_ok);
	if (!node) return;

	std::string owner_name;
	LLUUID owner_id;
	LLSelectMgr::getInstance()->selectGetOwner(owner_id, owner_name);

	panelp->mTargetAvatar = owner_id;
	LLStringUtil::format_map_t args;
	args["[OBJECT]"] = node->mName;
	args["[OWNER]"] = owner_name;
	std::string name = LLTrans::getString("GodToolsObjectOwnedBy", args);
	panelp->getChild<LLUICtrl>("target_avatar_name")->setValue(name);
}

void LLPanelObjectTools::callbackAvatarID(const uuid_vec_t& ids, const std::vector<LLAvatarName> names)
{
	if (ids.empty() || names.empty()) return;
	mTargetAvatar = ids[0];
	getChild<LLUICtrl>("target_avatar_name")->setValue(names[0].getCompleteName());
	refresh();
}

void LLPanelObjectTools::onChangeAnything()
{
	if (gAgent.isGodlike())
	{
		getChildView("Apply")->setEnabled(true);
	}
}

void LLPanelObjectTools::onApplyChanges()
{
	LLFloaterGodTools* god_tools = LLFloaterReg::getTypedInstance<LLFloaterGodTools>("god_tools");
	if(!god_tools) return;
	LLViewerRegion *region = gAgent.getRegion();
	if (region && gAgent.isGodlike())
	{
		// TODO -- implement this
		getChildView("Apply")->setEnabled(false);
		god_tools->sendGodUpdateRegionInfo();
		//LLFloaterReg::getTypedInstance<LLFloaterGodTools>("god_tools")->sendGodUpdateRegionInfo();
	}
}


// --------------------
// LLPanelRequestTools
// --------------------

const std::string SELECTION = "Selection";
const std::string AGENT_REGION = "Agent Region";

LLPanelRequestTools::LLPanelRequestTools():
	LLPanel()
{
	mCommitCallbackRegistrar.add("GodTools.Request",		boost::bind(&LLPanelRequestTools::onClickRequest, this));
}

LLPanelRequestTools::~LLPanelRequestTools()
{
}

bool LLPanelRequestTools::postBuild()
{
	refresh();

	return true;
}

void LLPanelRequestTools::refresh()
{
	std::string buffer = getChild<LLUICtrl>("destination")->getValue();
	LLCtrlListInterface *list = childGetListInterface("destination");
	if (!list) return;

	S32 last_item = list->getItemCount();

	if (last_item >=3)
	{
	list->selectItemRange(2,last_item);
	list->operateOnSelection(LLCtrlListInterface::OP_DELETE);
	}
	for (LLWorld::region_list_t::const_iterator iter = LLWorld::getInstance()->getRegionList().begin();
		 iter != LLWorld::getInstance()->getRegionList().end(); ++iter)
	{
		LLViewerRegion* regionp = *iter;
		std::string name = regionp->getName();
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
		list->operateOnSelection(LLCtrlListInterface::OP_DESELECT);
	}
}


// static
void LLPanelRequestTools::sendRequest(const std::string& request, 
									  const std::string& parameter, 
									  const LLHost& host)
{
	LL_INFOS() << "Sending request '" << request << "', '"
			<< parameter << "' to " << host << LL_ENDL;
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

void LLPanelRequestTools::onClickRequest()
{
	const std::string dest = getChild<LLUICtrl>("destination")->getValue().asString();
	if(dest == SELECTION)
	{
		std::string req =getChild<LLUICtrl>("request")->getValue();
		req = req.substr(0, req.find_first_of(" "));
		std::string param = getChild<LLUICtrl>("parameter")->getValue();
		LLSelectMgr::getInstance()->sendGodlikeRequest(req, param);
	}
	else if(dest == AGENT_REGION)
	{
		sendRequest(gAgent.getRegionHost());
	}
	else
	{
		// find region by name
		for (LLWorld::region_list_t::const_iterator iter = LLWorld::getInstance()->getRegionList().begin();
			 iter != LLWorld::getInstance()->getRegionList().end(); ++iter)
		{
			LLViewerRegion* regionp = *iter;
			if(dest == regionp->getName())
			{
				// found it
				sendRequest(regionp->getHost());
			}
		}
	}
}

void terrain_download_done(void** data, S32 status, LLExtStat ext_status)
{
	LLNotificationsUtil::add("TerrainDownloaded");
}


void test_callback(const LLTSCode status)
{
	LL_INFOS() << "Test transfer callback returned!" << LL_ENDL;
}


void LLPanelRequestTools::sendRequest(const LLHost& host)
{

	// intercept viewer local actions here
	std::string req = getChild<LLUICtrl>("request")->getValue();
	if (req == "terrain download")
	{
		gXferManager->requestFile(std::string("terrain.raw"), std::string("terrain.raw"), LL_PATH_NONE,
								  host,
								  false,
								  terrain_download_done,
								  NULL);
	}
	else
	{
		req = req.substr(0, req.find_first_of(" "));
		sendRequest(req, getChild<LLUICtrl>("parameter")->getValue().asString(), host);
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
