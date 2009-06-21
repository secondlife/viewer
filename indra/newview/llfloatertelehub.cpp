/** 
 * @file llfloatertelehub.cpp
 * @author James Cook
 * @brief LLFloaterTelehub class implementation
 *
 * $LicenseInfo:firstyear=2005&license=viewergpl$
 * 
 * Copyright (c) 2005-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
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

#include "llfloatertelehub.h"

#include "message.h"
#include "llfontgl.h"

#include "llagent.h"
#include "llfloatertools.h"
#include "llscrolllistctrl.h"
#include "llselectmgr.h"
#include "lltoolcomp.h"
#include "lltoolmgr.h"
#include "llviewerobject.h"
#include "llviewerobjectlist.h"
#include "lluictrlfactory.h"

LLFloaterTelehub* LLFloaterTelehub::sInstance = NULL;


// static
void LLFloaterTelehub::show()
{
	if (sInstance)
	{
		sInstance->setVisibleAndFrontmost();
		return;
	}

	sInstance = new LLFloaterTelehub();

	// Show tools floater by selecting translate (select) tool
	LLToolMgr::getInstance()->setCurrentToolset(gBasicToolset);
	LLToolMgr::getInstance()->getCurrentToolset()->selectTool( LLToolCompTranslate::getInstance() );

	// Find tools floater, glue to bottom
	if (gFloaterTools)
	{
		LLRect tools_rect = gFloaterTools->getRect();
		S32 our_width = sInstance->getRect().getWidth();
		S32 our_height = sInstance->getRect().getHeight();
		LLRect our_rect;
		our_rect.setLeftTopAndSize(tools_rect.mLeft, tools_rect.mBottom, our_width, our_height);
		sInstance->setRect(our_rect);
	}

	sInstance->sendTelehubInfoRequest();
}

LLFloaterTelehub::LLFloaterTelehub()
:	LLFloater(),
	mTelehubObjectID(),
	mTelehubObjectName(),
	mTelehubPos(),
	mTelehubRot(),
	mNumSpawn(0)
{
	sInstance = this;

	gMessageSystem->setHandlerFunc("TelehubInfo", processTelehubInfo);

	LLUICtrlFactory::getInstance()->buildFloater(sInstance, "floater_telehub.xml");

	mObjectSelection = LLSelectMgr::getInstance()->getEditSelection();
}
BOOL LLFloaterTelehub::postBuild()
{

	childSetAction("connect_btn", onClickConnect, this);
	childSetAction("disconnect_btn", onClickDisconnect, this);
	childSetAction("add_spawn_point_btn", onClickAddSpawnPoint, this);
	childSetAction("remove_spawn_point_btn", onClickRemoveSpawnPoint, this);

	LLScrollListCtrl* list = getChild<LLScrollListCtrl>("spawn_points_list");
	if (list)
	{
		// otherwise you can't walk with arrow keys while floater is up
		list->setAllowKeyboardMovement(FALSE);
	}

	return TRUE;
}
LLFloaterTelehub::~LLFloaterTelehub()
{
	sInstance = NULL;

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
	childSetEnabled("connect_btn", have_selection && all_volume);

	BOOL have_telehub = mTelehubObjectID.notNull();
	childSetEnabled("disconnect_btn", have_telehub);

	BOOL space_avail = (mNumSpawn < MAX_SPAWNPOINTS_PER_TELEHUB);
	childSetEnabled("add_spawn_point_btn", have_selection && all_volume && space_avail);

	LLScrollListCtrl* list = getChild<LLScrollListCtrl>("spawn_points_list");
	if (list)
	{
		BOOL enable_remove = (list->getFirstSelected() != NULL);
		childSetEnabled("remove_spawn_point_btn", enable_remove);
	}
}

// static
BOOL LLFloaterTelehub::renderBeacons()
{
	// only render if we've got a telehub
	return sInstance && sInstance->mTelehubObjectID.notNull();
}

// static
void LLFloaterTelehub::addBeacons()
{
	if (!sInstance) return;

	// Find the telehub position, either our cached old position, or
	// an updated one based on the actual object position.
	LLVector3 hub_pos_region = sInstance->mTelehubPos;
	LLQuaternion hub_rot = sInstance->mTelehubRot;
	LLViewerObject* obj = gObjectList.findObject(sInstance->mTelehubObjectID);
	if (obj)
	{
		hub_pos_region = obj->getPositionRegion();
		hub_rot = obj->getRotationRegion();
	}
	// Draw nice thick 3-pixel lines.
	gObjectList.addDebugBeacon(hub_pos_region, "", LLColor4::yellow, LLColor4::white, 4);

	LLScrollListCtrl* list = sInstance->getChild<LLScrollListCtrl>("spawn_points_list");
	if (list)
	{
		S32 spawn_index = list->getFirstSelectedIndex();
		if (spawn_index >= 0)
		{
			LLVector3 spawn_pos = hub_pos_region  + (sInstance->mSpawnPointPos[spawn_index] * hub_rot);
			gObjectList.addDebugBeacon(spawn_pos, "", LLColor4::orange, LLColor4::white, 4);
		}
	}
}

void LLFloaterTelehub::sendTelehubInfoRequest()
{
	LLSelectMgr::getInstance()->sendGodlikeRequest("telehub", "info ui");
}

// static 
void LLFloaterTelehub::onClickConnect(void* data)
{
	LLSelectMgr::getInstance()->sendGodlikeRequest("telehub", "connect");
}

// static 
void LLFloaterTelehub::onClickDisconnect(void* data)
{
	LLSelectMgr::getInstance()->sendGodlikeRequest("telehub", "delete");
}

// static 
void LLFloaterTelehub::onClickAddSpawnPoint(void* data)
{
	LLSelectMgr::getInstance()->sendGodlikeRequest("telehub", "spawnpoint add");
	LLSelectMgr::getInstance()->deselectAll();
}

// static 
void LLFloaterTelehub::onClickRemoveSpawnPoint(void* data)
{
	if (!sInstance) return;

	LLScrollListCtrl* list = sInstance->getChild<LLScrollListCtrl>("spawn_points_list");
	if (!list) return;

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
	if (sInstance)
	{
		sInstance->unpackTelehubInfo(msg);
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
		childSetVisible("status_text_connected", false);
		childSetVisible("status_text_not_connected", true);
		childSetVisible("help_text_connected", false);
		childSetVisible("help_text_not_connected", true);
	}
	else
	{
		childSetTextArg("status_text_connected", "[OBJECT]", mTelehubObjectName);
		childSetVisible("status_text_connected", true);
		childSetVisible("status_text_not_connected", false);
		childSetVisible("help_text_connected", true);
		childSetVisible("help_text_not_connected", false);
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
