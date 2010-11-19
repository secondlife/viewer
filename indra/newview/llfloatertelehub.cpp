/** 
 * @file llfloatertelehub.cpp
 * @author James Cook
 * @brief LLFloaterTelehub class implementation
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
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

#include "llfloatertelehub.h"

#include "message.h"
#include "llfontgl.h"

#include "llagent.h"
#include "llfloaterreg.h"
#include "llfloatertools.h"
#include "llscrolllistctrl.h"
#include "llselectmgr.h"
#include "lltoolcomp.h"
#include "lltoolmgr.h"
#include "llviewerobject.h"
#include "llviewerobjectlist.h"
#include "lluictrlfactory.h"

LLFloaterTelehub::LLFloaterTelehub(const LLSD& key)
:	LLFloater(key),
	mTelehubObjectID(),
	mTelehubObjectName(),
	mTelehubPos(),
	mTelehubRot(),
	mNumSpawn(0)
{
}

BOOL LLFloaterTelehub::postBuild()
{
	gMessageSystem->setHandlerFunc("TelehubInfo", processTelehubInfo);

	getChild<LLUICtrl>("connect_btn")->setCommitCallback(boost::bind(&LLFloaterTelehub::onClickConnect, this));
	getChild<LLUICtrl>("disconnect_btn")->setCommitCallback(boost::bind(&LLFloaterTelehub::onClickDisconnect, this));
	getChild<LLUICtrl>("add_spawn_point_btn")->setCommitCallback(boost::bind(&LLFloaterTelehub::onClickAddSpawnPoint, this));
	getChild<LLUICtrl>("remove_spawn_point_btn")->setCommitCallback(boost::bind(&LLFloaterTelehub::onClickRemoveSpawnPoint, this));

	LLScrollListCtrl* list = getChild<LLScrollListCtrl>("spawn_points_list");
	if (list)
	{
		// otherwise you can't walk with arrow keys while floater is up
		list->setAllowKeyboardMovement(FALSE);
	}

	return TRUE;
}
void LLFloaterTelehub::onOpen(const LLSD& key)
{
	// Show tools floater by selecting translate (select) tool
	LLToolMgr::getInstance()->setCurrentToolset(gBasicToolset);
	LLToolMgr::getInstance()->getCurrentToolset()->selectTool( LLToolCompTranslate::getInstance() );

	sendTelehubInfoRequest();
	
	mObjectSelection = LLSelectMgr::getInstance()->getEditSelection();
}

LLFloaterTelehub::~LLFloaterTelehub()
{
	// no longer interested in this message
	gMessageSystem->setHandlerFunc("TelehubInfo", NULL);
}

void LLFloaterTelehub::draw()
{
	if (!isMinimized())
	{
		refresh();
	}
	LLFloater::draw();
}

// Per-frame updates, because we don't have a selection manager observer.
void LLFloaterTelehub::refresh()
{
	const BOOL children_ok = TRUE;
	LLViewerObject* object = mObjectSelection->getFirstRootObject(children_ok);
	
	BOOL have_selection = (object != NULL);
	BOOL all_volume = LLSelectMgr::getInstance()->selectionAllPCode( LL_PCODE_VOLUME );
	getChildView("connect_btn")->setEnabled(have_selection && all_volume);

	BOOL have_telehub = mTelehubObjectID.notNull();
	getChildView("disconnect_btn")->setEnabled(have_telehub);

	BOOL space_avail = (mNumSpawn < MAX_SPAWNPOINTS_PER_TELEHUB);
	getChildView("add_spawn_point_btn")->setEnabled(have_selection && all_volume && space_avail);

	LLScrollListCtrl* list = getChild<LLScrollListCtrl>("spawn_points_list");
	if (list)
	{
		BOOL enable_remove = (list->getFirstSelected() != NULL);
		getChildView("remove_spawn_point_btn")->setEnabled(enable_remove);
	}
}

// static
BOOL LLFloaterTelehub::renderBeacons()
{
	// only render if we've got a telehub
	LLFloaterTelehub* floater = LLFloaterReg::findTypedInstance<LLFloaterTelehub>("telehubs");
	return floater && floater->mTelehubObjectID.notNull();
}

// static
void LLFloaterTelehub::addBeacons()
{
	LLFloaterTelehub* floater = LLFloaterReg::findTypedInstance<LLFloaterTelehub>("telehubs");
	if (!floater)
		return;
	
	// Find the telehub position, either our cached old position, or
	// an updated one based on the actual object position.
	LLVector3 hub_pos_region = floater->mTelehubPos;
	LLQuaternion hub_rot = floater->mTelehubRot;
	LLViewerObject* obj = gObjectList.findObject(floater->mTelehubObjectID);
	if (obj)
	{
		hub_pos_region = obj->getPositionRegion();
		hub_rot = obj->getRotationRegion();
	}
	// Draw nice thick 3-pixel lines.
	gObjectList.addDebugBeacon(hub_pos_region, "", LLColor4::yellow, LLColor4::white, 4);

	LLScrollListCtrl* list = floater->getChild<LLScrollListCtrl>("spawn_points_list");
	if (list)
	{
		S32 spawn_index = list->getFirstSelectedIndex();
		if (spawn_index >= 0)
		{
			LLVector3 spawn_pos = hub_pos_region  + (floater->mSpawnPointPos[spawn_index] * hub_rot);
			gObjectList.addDebugBeacon(spawn_pos, "", LLColor4::orange, LLColor4::white, 4);
		}
	}
}

void LLFloaterTelehub::sendTelehubInfoRequest()
{
	LLSelectMgr::getInstance()->sendGodlikeRequest("telehub", "info ui");
}

void LLFloaterTelehub::onClickConnect()
{
	LLSelectMgr::getInstance()->sendGodlikeRequest("telehub", "connect");
}

void LLFloaterTelehub::onClickDisconnect()
{
	LLSelectMgr::getInstance()->sendGodlikeRequest("telehub", "delete");
}

void LLFloaterTelehub::onClickAddSpawnPoint()
{
	LLSelectMgr::getInstance()->sendGodlikeRequest("telehub", "spawnpoint add");
	LLSelectMgr::getInstance()->deselectAll();
}

void LLFloaterTelehub::onClickRemoveSpawnPoint()
{
	LLScrollListCtrl* list = getChild<LLScrollListCtrl>("spawn_points_list");
	if (!list)
		return;

	S32 spawn_index = list->getFirstSelectedIndex();
	if (spawn_index < 0) return;  // nothing selected

	LLMessageSystem* msg = gMessageSystem;

	// Could be god or estate owner.  If neither, server will reject message.
	if (gAgent.isGodlike())
	{
		msg->newMessage("GodlikeMessage");
	}
	else
	{
		msg->newMessage("EstateOwnerMessage");
	}
	msg->nextBlock("AgentData");
	msg->addUUID("AgentID", gAgent.getID());
	msg->addUUID("SessionID", gAgent.getSessionID());
	msg->addUUIDFast(_PREHASH_TransactionID, LLUUID::null); //not used
	msg->nextBlock("MethodData");
	msg->addString("Method", "telehub");
	msg->addUUID("Invoice", LLUUID::null);

	msg->nextBlock("ParamList");
	msg->addString("Parameter", "spawnpoint remove");

	std::string buffer;
	buffer = llformat("%d", spawn_index);
	msg->nextBlock("ParamList");
	msg->addString("Parameter", buffer);

	gAgent.sendReliableMessage();
}

// static 
void LLFloaterTelehub::processTelehubInfo(LLMessageSystem* msg, void**)
{
	LLFloaterTelehub* floater = LLFloaterReg::findTypedInstance<LLFloaterTelehub>("telehubs");
	if (floater)
	{
		floater->unpackTelehubInfo(msg);
	}
}

void LLFloaterTelehub::unpackTelehubInfo(LLMessageSystem* msg)
{
	msg->getUUID("TelehubBlock", "ObjectID", mTelehubObjectID);
	msg->getString("TelehubBlock", "ObjectName", mTelehubObjectName);
	msg->getVector3("TelehubBlock", "TelehubPos", mTelehubPos);
	msg->getQuat("TelehubBlock", "TelehubRot", mTelehubRot);

	mNumSpawn = msg->getNumberOfBlocks("SpawnPointBlock");
	for (S32 i = 0; i < mNumSpawn; i++)
	{
		msg->getVector3("SpawnPointBlock", "SpawnPointPos", mSpawnPointPos[i], i);
	}

	// Update parts of the UI that change only when message received.

	if (mTelehubObjectID.isNull())
	{
		getChildView("status_text_connected")->setVisible( false);
		getChildView("status_text_not_connected")->setVisible( true);
		getChildView("help_text_connected")->setVisible( false);
		getChildView("help_text_not_connected")->setVisible( true);
	}
	else
	{
		getChild<LLUICtrl>("status_text_connected")->setTextArg("[OBJECT]", mTelehubObjectName);
		getChildView("status_text_connected")->setVisible( true);
		getChildView("status_text_not_connected")->setVisible( false);
		getChildView("help_text_connected")->setVisible( true);
		getChildView("help_text_not_connected")->setVisible( false);
	}

	LLScrollListCtrl* list = getChild<LLScrollListCtrl>("spawn_points_list");
	if (list)
	{
		list->deleteAllItems();
		for (S32 i = 0; i < mNumSpawn; i++)
		{
			std::string pos = llformat("%.1f, %.1f, %.1f", 
									mSpawnPointPos[i].mV[VX],
									mSpawnPointPos[i].mV[VY],
									mSpawnPointPos[i].mV[VZ]);
			list->addSimpleElement(pos);
		}
		list->selectNthItem(mNumSpawn - 1);
	}
}
