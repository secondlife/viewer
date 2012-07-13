/** 
 * @file llfloatertopobjects.cpp
 * @brief Shows top colliders, top scripts, etc.
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

#include "llfloatertopobjects.h"

// library includes
#include "message.h"
#include "llavatarnamecache.h"
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
	mCommitCallbackRegistrar.add("TopObjects.GetByParcelName",	boost::bind(&LLFloaterTopObjects::onGetByParcelName, this));
	mCommitCallbackRegistrar.add("TopObjects.CommitObjectsList",boost::bind(&LLFloaterTopObjects::onCommitObjectsList, this));
}

LLFloaterTopObjects::~LLFloaterTopObjects()
{
}

// virtual
BOOL LLFloaterTopObjects::postBuild()
{
	LLScrollListCtrl *objects_list = getChild<LLScrollListCtrl>("objects_list");
	getChild<LLUICtrl>("objects_list")->setFocus(TRUE);
	objects_list->setDoubleClickCallback(onDoubleClickObjectsList, this);
	objects_list->setCommitOnSelectionChange(TRUE);

	setDefaultBtn("show_beacon_btn");

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
		std::string parcel_buf("unknown");
		F32 mono_score = 0.f;
		bool have_extended_data = false;
		S32 public_urls = 0;
		F32 script_memory = 0.f;

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
			msg->getS32("DataExtended", "PublicURLs", public_urls, block);
			if (msg->getSize("DataExtended", "ParcelName") > 0)
			{
				msg->getString("DataExtended", "ParcelName", parcel_buf, block);
				msg->getF32("DataExtended", "Size", script_memory, block);
			}
		}

		LLSD element;

		element["id"] = task_id;

		LLSD columns;
		S32 column_num = 0;
		columns[column_num]["column"] = "score";
		columns[column_num]["value"] = llformat("%0.3f", score);
		columns[column_num++]["font"] = "SANSSERIF";
		
		columns[column_num]["column"] = "name";
		columns[column_num]["value"] = name_buf;
		columns[column_num++]["font"] = "SANSSERIF";
		
		// Owner names can have trailing spaces sent from server
		LLStringUtil::trim(owner_buf);
		
		if (LLAvatarNameCache::useDisplayNames())
		{
			// ...convert hard-coded name from server to a username
			// *TODO: Send owner_id from server and look up display name
			owner_buf = LLCacheName::buildUsername(owner_buf);
		}
		else
		{
			// ...just strip out legacy "Resident" name
			owner_buf = LLCacheName::cleanFullName(owner_buf);
		}
		columns[column_num]["column"] = "owner";
		columns[column_num]["value"] = owner_buf;
		columns[column_num++]["font"] = "SANSSERIF";

		columns[column_num]["column"] = "location";
		columns[column_num]["value"] = llformat("<%0.1f,%0.1f,%0.1f>", location_x, location_y, location_z);
		columns[column_num++]["font"] = "SANSSERIF";

		columns[column_num]["column"] = "parcel";
		columns[column_num]["value"] = parcel_buf;
		columns[column_num++]["font"] = "SANSSERIF";

		columns[column_num]["column"] = "time";
		columns[column_num]["type"] = "date";
		columns[column_num]["value"] = LLDate((time_t)time_stamp);
		columns[column_num++]["font"] = "SANSSERIF";

		if (mCurrentMode == STAT_REPORT_TOP_SCRIPTS
			&& have_extended_data)
		{
			columns[column_num]["column"] = "memory";
			columns[column_num]["value"] = llformat("%0.0f", (script_memory / 1000.f));
			columns[column_num++]["font"] = "SANSSERIF";

			columns[column_num]["column"] = "URLs";
			columns[column_num]["value"] = llformat("%d", public_urls);
			columns[column_num++]["font"] = "SANSSERIF";
		}
		element["columns"] = columns;
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
		
		LLUIString format = getString("top_scripts_text");
		format.setArg("[COUNT]", llformat("%d", total_count));
		format.setArg("[TIME]", llformat("%0.3f", mtotalScore));
		getChild<LLUICtrl>("title_text")->setValue(LLSD(format));
	}
	else
	{
		setTitle(getString("top_colliders_title"));
		list->setColumnLabel("score", getString("colliders_score_label"));
		list->setColumnLabel("URLs", "");
		list->setColumnLabel("memory", "");
		LLUIString format = getString("top_colliders_text");
		format.setArg("[COUNT]", llformat("%d", total_count));
		getChild<LLUICtrl>("title_text")->setValue(LLSD(format));
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

	getChild<LLUICtrl>("id_editor")->setValue(LLSD(object_id_string));
	LLScrollListItem* sli = list->getFirstSelected();
	llassert(sli);
	if (sli)
	{
		getChild<LLUICtrl>("object_name_editor")->setValue(sli->getColumn(1)->getValue().asString());
		getChild<LLUICtrl>("owner_name_editor")->setValue(sli->getColumn(2)->getValue().asString());
		getChild<LLUICtrl>("parcel_name_editor")->setValue(sli->getColumn(4)->getValue().asString());
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

	LLCtrlListInterface *list = getChild<LLUICtrl>("objects_list")->getListInterface();
	if (!list || list->getItemCount() == 0) return;

	uuid_vec_t::iterator id_itor;

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
	mFilter = getChild<LLUICtrl>("object_name_editor")->getValue().asString();
	onRefresh();
}

void LLFloaterTopObjects::onGetByOwnerName()
{
	mFlags  = STAT_FILTER_BY_OWNER;
	mFilter = getChild<LLUICtrl>("owner_name_editor")->getValue().asString();
	onRefresh();
}


void LLFloaterTopObjects::onGetByParcelName()
{
	mFlags  = STAT_FILTER_BY_PARCEL_NAME;
	mFilter = getChild<LLUICtrl>("parcel_name_editor")->getValue().asString();
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
