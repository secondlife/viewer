/** 
 * @file llfloatertopobjects.cpp
 * @brief Shows top colliders, top scripts, etc.
 *
 * $LicenseInfo:firstyear=2005&license=viewergpl$
 * 
 * Copyright (c) 2005-2007, Linden Research, Inc.
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

#include "llfloatertopobjects.h"

#include "message.h"
#include "llfontgl.h"

#include "llagent.h"
#include "llbutton.h"
#include "llfloatergodtools.h"
#include "llparcel.h"
#include "llscrolllistctrl.h"
#include "lllineeditor.h"
#include "lltextbox.h"
#include "lltracker.h"
#include "llviewerparcelmgr.h"
#include "llviewerregion.h"
#include "llvieweruictrlfactory.h"
#include "llviewerwindow.h"

LLFloaterTopObjects* LLFloaterTopObjects::sInstance = NULL;


// static
void LLFloaterTopObjects::show()
{
	if (sInstance)
	{
		sInstance->setVisibleAndFrontmost();
		return;
	}

	sInstance = new LLFloaterTopObjects();
	gUICtrlFactory->buildFloater(sInstance, "floater_top_objects.xml");
	sInstance->center();
}

LLFloaterTopObjects::LLFloaterTopObjects()
:	LLFloater("top_objects"),
	mInitialized(FALSE),
	mtotalScore(0.f)
{
	sInstance = this;
}

LLFloaterTopObjects::~LLFloaterTopObjects()
{
	sInstance = NULL;
}

// virtual
BOOL LLFloaterTopObjects::postBuild()
{
	childSetCommitCallback("objects_list", onCommitObjectsList, this);
	childSetDoubleClickCallback("objects_list", onDoubleClickObjectsList);
	childSetFocus("objects_list");
	LLScrollListCtrl *objects_list = LLUICtrlFactory::getScrollListByName(this, "objects_list");
	if (objects_list)
	{
		objects_list->setCommitOnSelectionChange(TRUE);
	}

	childSetAction("show_beacon_btn", onClickShowBeacon, this);
	setDefaultBtn("show_beacon_btn");

	childSetAction("return_selected_btn", onReturnSelected, this);
	childSetAction("return_all_btn", onReturnAll, this);
	childSetAction("disable_selected_btn", onDisableSelected, this);
	childSetAction("disable_all_btn", onDisableAll, this);
	childSetAction("refresh_btn", onRefresh, this);	


	childSetAction("filter_object_btn", onGetByObjectNameClicked, this);
	childSetAction("filter_owner_btn", onGetByOwnerNameClicked, this);	


	/*
	LLLineEditor* line_editor = LLUICtrlFactory::getLineEditorByName(this, "owner_name_editor");
	if (line_editor)
	{
		line_editor->setCommitOnFocusLost(FALSE);
		line_editor->setCommitCallback(onGetByOwnerName);
		line_editor->setCallbackUserData(this);
	}

	line_editor = LLUICtrlFactory::getLineEditorByName(this, "object_name_editor");
	if (line_editor)
	{
		line_editor->setCommitOnFocusLost(FALSE);
		line_editor->setCommitCallback(onGetByObjectName);
		line_editor->setCallbackUserData(this);
	}*/

	mCurrentMode = STAT_REPORT_TOP_SCRIPTS;
	mFlags = 0;
	mFilter = "";

	return TRUE;
}

void LLFloaterTopObjects::handle_land_reply(LLMessageSystem* msg, void** data)
{
	// Make sure dialog is on screen
	show();
	sInstance->handleReply(msg, data);

	//HACK: for some reason sometimes top scripts originally comes back
	//with no results even though they're there
	if (!sInstance->mObjectListIDs.size() && !sInstance->mInitialized)
	{
		sInstance->onRefresh(NULL);
		sInstance->mInitialized = TRUE;
	}

}

void LLFloaterTopObjects::handleReply(LLMessageSystem *msg, void** data)
{
	U32 request_flags;
	U32 total_count;

	msg->getU32Fast(_PREHASH_RequestData, _PREHASH_RequestFlags, request_flags);
	msg->getU32Fast(_PREHASH_RequestData, _PREHASH_TotalObjectCount, total_count);
	msg->getU32Fast(_PREHASH_RequestData, _PREHASH_ReportType, mCurrentMode);

	LLCtrlListInterface *list = childGetListInterface("objects_list");
	if (!list) return;
	
	S32 block_count = msg->getNumberOfBlocks("ReportData");
	for (S32 block = 0; block < block_count; ++block)
	{
		U32 task_local_id;
		LLUUID task_id;
		F32 location_x, location_y, location_z;
		F32 score;
		char name_buf[MAX_STRING];		/* Flawfinder: ignore */
		char owner_buf[MAX_STRING];		/* Flawfinder: ignore */

		msg->getU32Fast(_PREHASH_ReportData, _PREHASH_TaskLocalID, task_local_id, block);
		msg->getUUIDFast(_PREHASH_ReportData, _PREHASH_TaskID, task_id, block);
		msg->getF32Fast(_PREHASH_ReportData, _PREHASH_LocationX, location_x, block);
		msg->getF32Fast(_PREHASH_ReportData, _PREHASH_LocationY, location_y, block);
		msg->getF32Fast(_PREHASH_ReportData, _PREHASH_LocationZ, location_z, block);
		msg->getF32Fast(_PREHASH_ReportData, _PREHASH_Score, score, block);
		msg->getStringFast(_PREHASH_ReportData, _PREHASH_TaskName, MAX_STRING, name_buf, block);
		msg->getStringFast(_PREHASH_ReportData, _PREHASH_OwnerName, MAX_STRING, owner_buf, block);
		
		LLSD element;

		element["id"] = task_id;
		element["object_name"] = LLString(name_buf);
		element["owner_name"] = LLString(owner_buf);
		element["columns"][0]["column"] = "score";
		element["columns"][0]["value"] = llformat("%0.3f", score);
		element["columns"][0]["font"] = "SANSSERIF";
		element["columns"][1]["column"] = "name";
		element["columns"][1]["value"] = name_buf;
		element["columns"][1]["font"] = "SANSSERIF";
		element["columns"][2]["column"] = "owner";
		element["columns"][2]["value"] = owner_buf;
		element["columns"][2]["font"] = "SANSSERIF";
		element["columns"][3]["column"] = "location";
		element["columns"][3]["value"] = llformat("<%0.1f,%0.1f,%0.1f>", location_x, location_y, location_z);
		element["columns"][3]["font"] = "SANSSERIF";

		list->addElement(element);
		
		mObjectListData.append(element);
		mObjectListIDs.push_back(task_id);

		mtotalScore += score;
	}

	if (total_count == 0 && list->getItemCount() == 0)
	{
		LLSD element;
		element["id"] = LLUUID::null;
		element["columns"][0]["column"] = "name";
		element["columns"][0]["value"] = getString("none_descriptor");
		element["columns"][0]["font"] = "SANSSERIF";

		list->addElement(element);
	}
	else
	{
		list->selectFirstItem();
	}

	if (mCurrentMode == STAT_REPORT_TOP_SCRIPTS)
	{
		setTitle(getString("top_scripts_title"));
		list->setColumnLabel("score", getString("scripts_score_label"));
		
		LLUIString format = getString("top_scripts_text");
		format.setArg("[COUNT]", llformat("%d", total_count));
		format.setArg("[TIME]", llformat("%0.1f", mtotalScore));
		childSetValue("title_text", LLSD(format));
	}
	else
	{
		setTitle(getString("top_colliders_title"));
		list->setColumnLabel("score", getString("colliders_score_label"));
		LLUIString format = getString("top_colliders_text");
		format.setArg("[COUNT]", llformat("%d", total_count));
		childSetValue("title_text", LLSD(format));
	}
}

// static
void LLFloaterTopObjects::onCommitObjectsList(LLUICtrl* ctrl, void* data)
{
	LLFloaterTopObjects* self = (LLFloaterTopObjects*)data;

	self->updateSelectionInfo();
}

void LLFloaterTopObjects::updateSelectionInfo()
{
	LLScrollListCtrl* list = LLUICtrlFactory::getScrollListByName(this, "objects_list");

	if (!list) return;

	LLUUID object_id = list->getCurrentID();
	if (object_id.isNull()) return;

	std::string object_id_string = object_id.asString();

	childSetValue("id_editor", LLSD(object_id_string));
	childSetValue("object_name_editor", list->getFirstSelected()->getColumn(1)->getValue().asString());
	childSetValue("owner_name_editor", list->getFirstSelected()->getColumn(2)->getValue().asString());
}

// static
void LLFloaterTopObjects::onDoubleClickObjectsList(void* data)
{
	LLFloaterTopObjects* self = (LLFloaterTopObjects*)data;
	self->showBeacon();
}

// static
void LLFloaterTopObjects::onClickShowBeacon(void* data)
{
	LLFloaterTopObjects* self = (LLFloaterTopObjects*)data;
	if (!self) return;
	self->showBeacon();
}

void LLFloaterTopObjects::doToObjects(int action, bool all)
{
	LLMessageSystem *msg = gMessageSystem;

	LLViewerRegion* region = gAgent.getRegion();
	if (!region) return;

	LLCtrlListInterface *list = childGetListInterface("objects_list");
	if (!list || list->getItemCount() == 0) return;

	std::vector<LLUUID>::iterator id_itor;

	bool start_message = true;

	for (id_itor = mObjectListIDs.begin(); id_itor != mObjectListIDs.end(); ++id_itor)
	{
		LLUUID task_id = *id_itor;
		if (!all && !list->isSelected(task_id))
		{
			// Selected only
			continue;
		}
		if (start_message)
		{
			if (action == ACTION_RETURN)
			{
				msg->newMessageFast(_PREHASH_ParcelReturnObjects);
			}
			else
			{
				msg->newMessageFast(_PREHASH_ParcelDisableObjects);
			}
			msg->nextBlockFast(_PREHASH_AgentData);
			msg->addUUIDFast(_PREHASH_AgentID,	gAgent.getID());
			msg->addUUIDFast(_PREHASH_SessionID,gAgent.getSessionID());
			msg->nextBlockFast(_PREHASH_ParcelData);
			msg->addS32Fast(_PREHASH_LocalID, -1); // Whole region
			msg->addS32Fast(_PREHASH_ReturnType, RT_NONE);
			start_message = false;
		}

		msg->nextBlockFast(_PREHASH_TaskIDs);
		msg->addUUIDFast(_PREHASH_TaskID, task_id);

		if (msg->isSendFullFast(_PREHASH_TaskIDs))
		{
			msg->sendReliable(region->getHost());
			start_message = true;
		}
	}

	if (!start_message)
	{
		msg->sendReliable(region->getHost());
	}
}

//static
void LLFloaterTopObjects::callbackReturnAll(S32 option, void* userdata)
{
	if (option == 0)
	{
		sInstance->doToObjects(ACTION_RETURN, true);
	}
}

void LLFloaterTopObjects::onReturnAll(void* data)
{	
	gViewerWindow->alertXml("ReturnAllTopObjects", callbackReturnAll, NULL);
}


void LLFloaterTopObjects::onReturnSelected(void* data)
{
	sInstance->doToObjects(ACTION_RETURN, false);
}


//static
void LLFloaterTopObjects::callbackDisableAll(S32 option, void* userdata)
{
	if (option == 0)
	{
		sInstance->doToObjects(ACTION_DISABLE, true);
	}
}

void LLFloaterTopObjects::onDisableAll(void* data)
{
	gViewerWindow->alertXml("DisableAllTopObjects", callbackDisableAll, NULL);
}

void LLFloaterTopObjects::onDisableSelected(void* data)
{
	sInstance->doToObjects(ACTION_DISABLE, false);
}

//static
void LLFloaterTopObjects::clearList()
{
	LLCtrlListInterface *list = sInstance->childGetListInterface("objects_list");
	
	if (list) 
	{
		list->operateOnAll(LLCtrlListInterface::OP_DELETE);
	}

	sInstance->mObjectListData.clear();
	sInstance->mObjectListIDs.clear();
	sInstance->mtotalScore = 0.f;
}

//static
void LLFloaterTopObjects::onRefresh(void* data)
{
	U32 mode = STAT_REPORT_TOP_SCRIPTS;
	U32 flags = 0;
	LLString filter = "";

	if (sInstance)
	{
		mode = sInstance->mCurrentMode;
		flags = sInstance->mFlags;
		filter = sInstance->mFilter;
		sInstance->clearList();
	}

	LLMessageSystem *msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_LandStatRequest);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID() );
	msg->nextBlockFast(_PREHASH_RequestData);
	msg->addU32Fast(_PREHASH_ReportType, mode);
	msg->addU32Fast(_PREHASH_RequestFlags, flags);
	msg->addStringFast(_PREHASH_Filter, filter);
	msg->addS32Fast(_PREHASH_ParcelLocalID, 0);

	msg->sendReliable(gAgent.getRegionHost());

	if (sInstance)
	{
		sInstance->mFilter = "";
		sInstance->mFlags = 0;
	}
}

void LLFloaterTopObjects::onGetByObjectName(LLUICtrl* ctrl, void* data)
{
	if (sInstance)
	{
		sInstance->mFlags = STAT_FILTER_BY_OBJECT;
		sInstance->mFilter = sInstance->childGetText("object_name_editor");
		onRefresh(NULL);
	}
}

void LLFloaterTopObjects::onGetByOwnerName(LLUICtrl* ctrl, void* data)
{
	if (sInstance)
	{
		sInstance->mFlags = STAT_FILTER_BY_OWNER;
		sInstance->mFilter = sInstance->childGetText("owner_name_editor");
		onRefresh(NULL);
	}
}

void LLFloaterTopObjects::showBeacon()
{	
	LLScrollListCtrl* list = LLUICtrlFactory::getScrollListByName(this, "objects_list");
	if (!list) return;

	LLScrollListItem* first_selected = list->getFirstSelected();
	if (!first_selected) return;

	LLString name = first_selected->getColumn(1)->getValue().asString();
	LLString pos_string =  first_selected->getColumn(3)->getValue().asString();

	F32 x, y, z;
	S32 matched = sscanf(pos_string.c_str(), "<%g,%g,%g>", &x, &y, &z);
	if (matched != 3) return;

	LLVector3 pos_agent(x, y, z);
	LLVector3d pos_global = gAgent.getPosGlobalFromAgent(pos_agent);
	LLString tooltip("");
	LLTracker::trackLocation(pos_global, name, tooltip, LLTracker::LOCATION_ITEM);
}
