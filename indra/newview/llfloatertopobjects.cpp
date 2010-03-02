/** 
 * @file llfloatertopobjects.cpp
 * @brief Shows top colliders, top scripts, etc.
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

#include "llfloatertopobjects.h"

#include "message.h"
#include "llfontgl.h"

#include "llagent.h"
#include "llbutton.h"
#include "llfloatergodtools.h"
#include "llfloaterreg.h"
#include "llnotificationsutil.h"
#include "llparcel.h"
#include "llscrolllistctrl.h"
#include "llscrolllistitem.h"
#include "llscrolllistcell.h"
#include "lllineeditor.h"
#include "lltextbox.h"
#include "lltracker.h"
#include "llviewermessage.h"
#include "llviewerparcelmgr.h"
#include "llviewerregion.h"
#include "lluictrlfactory.h"
#include "llviewerwindow.h"

//LLFloaterTopObjects* LLFloaterTopObjects::sInstance = NULL;

// Globals
// const U32 TIME_STR_LENGTH = 30;
/*
// static
void LLFloaterTopObjects::show()
{
	if (sInstance)
	{
		sInstance->setVisibleAndFrontmost();
		return;
	}

	sInstance = new LLFloaterTopObjects();
//	LLUICtrlFactory::getInstance()->buildFloater(sInstance, "floater_top_objects.xml");
	sInstance->center();
}
*/
LLFloaterTopObjects::LLFloaterTopObjects(const LLSD& key)
:	LLFloater(key),
	mInitialized(FALSE),
	mtotalScore(0.f)
{
	mCommitCallbackRegistrar.add("TopObjects.ShowBeacon",		boost::bind(&LLFloaterTopObjects::onClickShowBeacon, this));
	mCommitCallbackRegistrar.add("TopObjects.ReturnSelected",	boost::bind(&LLFloaterTopObjects::onReturnSelected, this));
	mCommitCallbackRegistrar.add("TopObjects.ReturnAll",		boost::bind(&LLFloaterTopObjects::onReturnAll, this));
	mCommitCallbackRegistrar.add("TopObjects.DisableSelected",	boost::bind(&LLFloaterTopObjects::onDisableSelected, this));
	mCommitCallbackRegistrar.add("TopObjects.DisableAll",		boost::bind(&LLFloaterTopObjects::onDisableAll, this));
	mCommitCallbackRegistrar.add("TopObjects.Refresh",			boost::bind(&LLFloaterTopObjects::onRefresh, this));
	mCommitCallbackRegistrar.add("TopObjects.GetByObjectName",	boost::bind(&LLFloaterTopObjects::onGetByObjectName, this));
	mCommitCallbackRegistrar.add("TopObjects.GetByOwnerName",	boost::bind(&LLFloaterTopObjects::onGetByOwnerName, this));
	mCommitCallbackRegistrar.add("TopObjects.CommitObjectsList",boost::bind(&LLFloaterTopObjects::onCommitObjectsList, this));
}

LLFloaterTopObjects::~LLFloaterTopObjects()
{
}

// virtual
BOOL LLFloaterTopObjects::postBuild()
{
	LLScrollListCtrl *objects_list = getChild<LLScrollListCtrl>("objects_list");
	childSetFocus("objects_list");
	objects_list->setDoubleClickCallback(onDoubleClickObjectsList, this);
	objects_list->setCommitOnSelectionChange(TRUE);

	setDefaultBtn("show_beacon_btn");

	/*
	LLLineEditor* line_editor = getChild<LLLineEditor>("owner_name_editor");
	if (line_editor)
	{
		line_editor->setCommitOnFocusLost(FALSE);
		line_editor->setCommitCallback(onGetByOwnerName, this);
	}

	line_editor = getChild<LLLineEditor>("object_name_editor");
	if (line_editor)
	{
		line_editor->setCommitOnFocusLost(FALSE);
		line_editor->setCommitCallback(onGetByObjectName, this);
	}*/

	mCurrentMode = STAT_REPORT_TOP_SCRIPTS;
	mFlags = 0;
	mFilter.clear();

	return TRUE;
}
// static
void LLFloaterTopObjects::setMode(U32 mode)
{
	LLFloaterTopObjects* instance = LLFloaterReg::getTypedInstance<LLFloaterTopObjects>("top_objects");
	if(!instance) return;
	instance->mCurrentMode = mode; 
}

// static 
void LLFloaterTopObjects::handle_land_reply(LLMessageSystem* msg, void** data)
{
	LLFloaterTopObjects* instance = LLFloaterReg::getTypedInstance<LLFloaterTopObjects>("top_objects");
	if(!instance) return;
	// Make sure dialog is on screen
	LLFloaterReg::showInstance("top_objects");
	instance->handleReply(msg, data);

	//HACK: for some reason sometimes top scripts originally comes back
	//with no results even though they're there
	if (!instance->mObjectListIDs.size() && !instance->mInitialized)
	{
		instance->onRefresh();
		instance->mInitialized = TRUE;
	}

}

void LLFloaterTopObjects::handleReply(LLMessageSystem *msg, void** data)
{
	U32 request_flags;
	U32 total_count;

	msg->getU32Fast(_PREHASH_RequestData, _PREHASH_RequestFlags, request_flags);
	msg->getU32Fast(_PREHASH_RequestData, _PREHASH_TotalObjectCount, total_count);
	msg->getU32Fast(_PREHASH_RequestData, _PREHASH_ReportType, mCurrentMode);

	LLScrollListCtrl *list = getChild<LLScrollListCtrl>("objects_list");

	S32 block_count = msg->getNumberOfBlocks("ReportData");
	for (S32 block = 0; block < block_count; ++block)
	{
		U32 task_local_id;
		U32 time_stamp = 0;
		LLUUID task_id;
		F32 location_x, location_y, location_z;
		F32 score;
		std::string name_buf;
		std::string owner_buf;
		F32 mono_score = 0.f;
		bool have_extended_data = false;
		S32 public_urls = 0;

		msg->getU32Fast(_PREHASH_ReportData, _PREHASH_TaskLocalID, task_local_id, block);
		msg->getUUIDFast(_PREHASH_ReportData, _PREHASH_TaskID, task_id, block);
		msg->getF32Fast(_PREHASH_ReportData, _PREHASH_LocationX, location_x, block);
		msg->getF32Fast(_PREHASH_ReportData, _PREHASH_LocationY, location_y, block);
		msg->getF32Fast(_PREHASH_ReportData, _PREHASH_LocationZ, location_z, block);
		msg->getF32Fast(_PREHASH_ReportData, _PREHASH_Score, score, block);
		msg->getStringFast(_PREHASH_ReportData, _PREHASH_TaskName, name_buf, block);
		msg->getStringFast(_PREHASH_ReportData, _PREHASH_OwnerName, owner_buf, block);
		if(msg->has("DataExtended"))
		{
			have_extended_data = true;
			msg->getU32("DataExtended", "TimeStamp", time_stamp, block);
			msg->getF32("DataExtended", "MonoScore", mono_score, block);
			msg->getS32(_PREHASH_ReportData,"PublicURLs",public_urls,block);
		}

		LLSD element;

		element["id"] = task_id;
		// These cause parse warnings. JC
		//element["object_name"] = name_buf;
		//element["owner_name"] = owner_buf;
		element["columns"][0]["column"] = "score";
		element["columns"][0]["value"] = llformat("%0.3f", score);
		element["columns"][0]["font"] = "SANSSERIF";
		
		element["columns"][1]["column"] = "name";
		element["columns"][1]["value"] = name_buf;
		element["columns"][1]["font"] = "SANSSERIF";
		element["columns"][2]["column"] = "owner";
		element["columns"][2]["value"] = LLCacheName::cleanFullName(owner_buf);
		element["columns"][2]["font"] = "SANSSERIF";
		element["columns"][3]["column"] = "location";
		element["columns"][3]["value"] = llformat("<%0.1f,%0.1f,%0.1f>", location_x, location_y, location_z);
		element["columns"][3]["font"] = "SANSSERIF";
		element["columns"][4]["column"] = "time";
		element["columns"][4]["value"] = formatted_time((time_t)time_stamp);
		element["columns"][4]["font"] = "SANSSERIF";

		if (mCurrentMode == STAT_REPORT_TOP_SCRIPTS
			&& have_extended_data)
		{
			element["columns"][5]["column"] = "mono_time";
			element["columns"][5]["value"] = llformat("%0.3f", mono_score);
			element["columns"][5]["font"] = "SANSSERIF";

			element["columns"][6]["column"] = "URLs";
			element["columns"][6]["value"] = llformat("%d", public_urls);
			element["columns"][6]["font"] = "SANSSERIF";
		}
		
		list->addElement(element);
		
		mObjectListData.append(element);
		mObjectListIDs.push_back(task_id);

		mtotalScore += score;
	}

	if (total_count == 0 && list->getItemCount() == 0)
	{
		list->setCommentText(getString("none_descriptor"));
	}
	else
	{
		list->selectFirstItem();
	}

	if (mCurrentMode == STAT_REPORT_TOP_SCRIPTS)
	{
		setTitle(getString("top_scripts_title"));
		list->setColumnLabel("score", getString("scripts_score_label"));
		list->setColumnLabel("mono_time", getString("scripts_mono_time_label"));
		
		LLUIString format = getString("top_scripts_text");
		format.setArg("[COUNT]", llformat("%d", total_count));
		format.setArg("[TIME]", llformat("%0.1f", mtotalScore));
		childSetValue("title_text", LLSD(format));
	}
	else
	{
		setTitle(getString("top_colliders_title"));
		list->setColumnLabel("score", getString("colliders_score_label"));
		list->setColumnLabel("mono_time", "");
		LLUIString format = getString("top_colliders_text");
		format.setArg("[COUNT]", llformat("%d", total_count));
		childSetValue("title_text", LLSD(format));
	}
}

void LLFloaterTopObjects::onCommitObjectsList()
{
	updateSelectionInfo();
}

void LLFloaterTopObjects::updateSelectionInfo()
{
	LLScrollListCtrl* list = getChild<LLScrollListCtrl>("objects_list");

	if (!list) return;

	LLUUID object_id = list->getCurrentID();
	if (object_id.isNull()) return;

	std::string object_id_string = object_id.asString();

	childSetValue("id_editor", LLSD(object_id_string));
	LLScrollListItem* sli = list->getFirstSelected();
	llassert(sli);
	if (sli)
	{
		childSetValue("object_name_editor", sli->getColumn(1)->getValue().asString());
		childSetValue("owner_name_editor", sli->getColumn(2)->getValue().asString());
	}
}

// static
void LLFloaterTopObjects::onDoubleClickObjectsList(void* data)
{
	LLFloaterTopObjects* self = (LLFloaterTopObjects*)data;
	self->showBeacon();
}

// static
void LLFloaterTopObjects::onClickShowBeacon()
{
	showBeacon();
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
bool LLFloaterTopObjects::callbackReturnAll(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	LLFloaterTopObjects* instance = LLFloaterReg::getTypedInstance<LLFloaterTopObjects>("top_objects");
	if(!instance) return false;
	if (option == 0)
	{
		instance->doToObjects(ACTION_RETURN, true);
	}
	return false;
}

void LLFloaterTopObjects::onReturnAll()
{	
	LLNotificationsUtil::add("ReturnAllTopObjects", LLSD(), LLSD(), &callbackReturnAll);
}


void LLFloaterTopObjects::onReturnSelected()
{
	doToObjects(ACTION_RETURN, false);
}


//static
bool LLFloaterTopObjects::callbackDisableAll(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	LLFloaterTopObjects* instance = LLFloaterReg::getTypedInstance<LLFloaterTopObjects>("top_objects");
	if(!instance) return false;
	if (option == 0)
	{
		instance->doToObjects(ACTION_DISABLE, true);
	}
	return false;
}

void LLFloaterTopObjects::onDisableAll()
{
	LLNotificationsUtil::add("DisableAllTopObjects", LLSD(), LLSD(), callbackDisableAll);
}

void LLFloaterTopObjects::onDisableSelected()
{
	doToObjects(ACTION_DISABLE, false);
}


void LLFloaterTopObjects::clearList()
{
	LLCtrlListInterface *list = childGetListInterface("objects_list");
	
	if (list) 
	{
		list->operateOnAll(LLCtrlListInterface::OP_DELETE);
	}

	mObjectListData.clear();
	mObjectListIDs.clear();
	mtotalScore = 0.f;
}


void LLFloaterTopObjects::onRefresh()
{
	U32 mode = STAT_REPORT_TOP_SCRIPTS;
	U32 flags = 0;
	std::string filter = "";

	mode   = mCurrentMode;
	flags  = mFlags;
	filter = mFilter;
	clearList();

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

	mFilter.clear();
	mFlags = 0;
}

void LLFloaterTopObjects::onGetByObjectName()
{
	mFlags  = STAT_FILTER_BY_OBJECT;
	mFilter = childGetText("object_name_editor");
	onRefresh();
}

void LLFloaterTopObjects::onGetByOwnerName()
{
	mFlags  = STAT_FILTER_BY_OWNER;
	mFilter = childGetText("owner_name_editor");
	onRefresh();
}

void LLFloaterTopObjects::showBeacon()
{
	LLScrollListCtrl* list = getChild<LLScrollListCtrl>("objects_list");
	if (!list) return;

	LLScrollListItem* first_selected = list->getFirstSelected();
	if (!first_selected) return;

	std::string name = first_selected->getColumn(1)->getValue().asString();
	std::string pos_string =  first_selected->getColumn(3)->getValue().asString();

	F32 x, y, z;
	S32 matched = sscanf(pos_string.c_str(), "<%g,%g,%g>", &x, &y, &z);
	if (matched != 3) return;

	LLVector3 pos_agent(x, y, z);
	LLVector3d pos_global = gAgent.getPosGlobalFromAgent(pos_agent);
	std::string tooltip("");
	LLTracker::trackLocation(pos_global, name, tooltip, LLTracker::LOCATION_ITEM);
}
