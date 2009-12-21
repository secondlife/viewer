/** 
 * @file llfloaterscriptlimits.cpp
 * @author Gabriel Lee
 * @brief Implementation of the region info and controls floater and panels.
 *
 * $LicenseInfo:firstyear=2004&license=viewergpl$
 * 
 * Copyright (c) 2004-2009, Linden Research, Inc.
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
#include "llfloaterscriptlimits.h"

#include "llsdutil.h"
#include "llsdutil_math.h"
#include "message.h"

#include "llagent.h"
#include "llfloateravatarpicker.h"
#include "llfloaterland.h"
#include "llfloaterreg.h"
#include "llregionhandle.h"
#include "llscrolllistctrl.h"
#include "llscrolllistitem.h"
#include "llparcel.h"
#include "lltabcontainer.h"
#include "lltracker.h"
#include "lltrans.h"
#include "llviewercontrol.h"
#include "lluictrlfactory.h"
#include "llviewerparcelmgr.h"
#include "llviewerregion.h"
#include "llviewerwindow.h"

///----------------------------------------------------------------------------
/// LLFloaterScriptLimits
///----------------------------------------------------------------------------

// due to server side bugs the full summary display is not possible
// until they are fixed this define creates a simple version of the
// summary which only shows available & correct information
#define USE_SIMPLE_SUMMARY

LLFloaterScriptLimits::LLFloaterScriptLimits(const LLSD& seed)
	: LLFloater(seed)
{
}

BOOL LLFloaterScriptLimits::postBuild()
{
	// a little cheap and cheerful - if there's an about land panel open default to showing parcel info,
	// otherwise default to showing attachments (avatar appearance)
	bool selectParcelPanel = false;
	
	LLFloaterLand* instance = LLFloaterReg::getTypedInstance<LLFloaterLand>("about_land");
	if(instance)
	{
		if(instance->isShown())
		{
			selectParcelPanel = true;
		}
	}

	mTab = getChild<LLTabContainer>("scriptlimits_panels");

	// contruct the panels
	LLPanelScriptLimitsRegionMemory* panel_memory;
	panel_memory = new LLPanelScriptLimitsRegionMemory;
	mInfoPanels.push_back(panel_memory);
	
	LLUICtrlFactory::getInstance()->buildPanel(panel_memory, "panel_script_limits_region_memory.xml");
	mTab->addTabPanel(panel_memory);

	LLPanelScriptLimitsRegionURLs* panel_urls = new LLPanelScriptLimitsRegionURLs;
	mInfoPanels.push_back(panel_urls);
	LLUICtrlFactory::getInstance()->buildPanel(panel_urls, "panel_script_limits_region_urls.xml");
	mTab->addTabPanel(panel_urls);

	LLPanelScriptLimitsAttachment* panel_attachments = new LLPanelScriptLimitsAttachment;
	mInfoPanels.push_back(panel_attachments);
	LLUICtrlFactory::getInstance()->buildPanel(panel_attachments, "panel_script_limits_my_avatar.xml");
	mTab->addTabPanel(panel_attachments);

	if(selectParcelPanel)
	{
		mTab->selectTab(0);
	}
	else
	{
		mTab->selectTab(2);
	}

	return TRUE;
}

LLFloaterScriptLimits::~LLFloaterScriptLimits()
{
}

// public
void LLFloaterScriptLimits::refresh()
{
	for(info_panels_t::iterator iter = mInfoPanels.begin();
		iter != mInfoPanels.end(); ++iter)
	{
		(*iter)->refresh();
	}
}


///----------------------------------------------------------------------------
// Base class for panels
///----------------------------------------------------------------------------

LLPanelScriptLimitsInfo::LLPanelScriptLimitsInfo()
	: LLPanel()
{
}


// virtual
BOOL LLPanelScriptLimitsInfo::postBuild()
{
	refresh();
	return TRUE;
}

// virtual 
void LLPanelScriptLimitsInfo::updateChild(LLUICtrl* child_ctr)
{
}

///----------------------------------------------------------------------------
// Responders
///----------------------------------------------------------------------------

void fetchScriptLimitsRegionInfoResponder::result(const LLSD& content)
{
	// at this point we have an llsd which should contain ether one or two urls to the services we want.
	// first we look for the details service:
	if(content.has("ScriptResourceDetails"))
	{
		LLHTTPClient::get(content["ScriptResourceDetails"], new fetchScriptLimitsRegionDetailsResponder(mInfo));
	}
	else
	{
		LLFloaterScriptLimits* instance = LLFloaterReg::getTypedInstance<LLFloaterScriptLimits>("script_limits");
		if(!instance)
		{
			llinfos << "Failed to get llfloaterscriptlimits instance" << llendl;
		}
		else
		{

// temp - only show info if we get details - there's nothing to show if not until the sim gets fixed
#ifdef USE_SIMPLE_SUMMARY

			LLTabContainer* tab = instance->getChild<LLTabContainer>("scriptlimits_panels");
			LLPanelScriptLimitsRegionMemory* panel_memory = (LLPanelScriptLimitsRegionMemory*)tab->getChild<LLPanel>("script_limits_region_memory_panel");
			std::string msg = LLTrans::getString("ScriptLimitsRequestDontOwnParcel");
			panel_memory->childSetValue("loading_text", LLSD(msg));
			LLPanelScriptLimitsRegionURLs* panel_urls = (LLPanelScriptLimitsRegionURLs*)tab->getChild<LLPanel>("script_limits_region_urls_panel");
			panel_urls->childSetValue("loading_text", LLSD(msg));
			
			// intentional early out as we dont want the resource summary if we are using the "simple summary"
			// and the details are missing
			return;
#endif
		}
	}

	// then the summary service:
	if(content.has("ScriptResourceSummary"))
	{
		LLHTTPClient::get(content["ScriptResourceSummary"], new fetchScriptLimitsRegionSummaryResponder(mInfo));
	}
}

void fetchScriptLimitsRegionInfoResponder::error(U32 status, const std::string& reason)
{
	llinfos << "Error from responder " << reason << llendl;
}

void fetchScriptLimitsRegionSummaryResponder::result(const LLSD& content)
{
	LLFloaterScriptLimits* instance = LLFloaterReg::getTypedInstance<LLFloaterScriptLimits>("script_limits");
	if(!instance)
	{
		llinfos << "Failed to get llfloaterscriptlimits instance" << llendl;
	}
	else
	{
		LLTabContainer* tab = instance->getChild<LLTabContainer>("scriptlimits_panels");
		LLPanelScriptLimitsRegionMemory* panel_memory = (LLPanelScriptLimitsRegionMemory*)tab->getChild<LLPanel>("script_limits_region_memory_panel");
		panel_memory->setRegionSummary(content);
		LLPanelScriptLimitsRegionURLs* panel_urls = (LLPanelScriptLimitsRegionURLs*)tab->getChild<LLPanel>("script_limits_region_urls_panel");
		panel_urls->setRegionSummary(content);
	}
}

void fetchScriptLimitsRegionSummaryResponder::error(U32 status, const std::string& reason)
{
	llinfos << "Error from responder " << reason << llendl;
}

void fetchScriptLimitsRegionDetailsResponder::result(const LLSD& content)
{
	LLFloaterScriptLimits* instance = LLFloaterReg::getTypedInstance<LLFloaterScriptLimits>("script_limits");

	if(!instance)
	{
		llinfos << "Failed to get llfloaterscriptlimits instance" << llendl;
	}
	else
	{
		LLTabContainer* tab = instance->getChild<LLTabContainer>("scriptlimits_panels");
		LLPanelScriptLimitsRegionMemory* panel_memory = (LLPanelScriptLimitsRegionMemory*)tab->getChild<LLPanel>("script_limits_region_memory_panel");
		panel_memory->setRegionDetails(content);
		
		LLPanelScriptLimitsRegionURLs* panel_urls = (LLPanelScriptLimitsRegionURLs*)tab->getChild<LLPanel>("script_limits_region_urls_panel");
		panel_urls->setRegionDetails(content);
	}
}

void fetchScriptLimitsRegionDetailsResponder::error(U32 status, const std::string& reason)
{
	llinfos << "Error from responder " << reason << llendl;
}

void fetchScriptLimitsAttachmentInfoResponder::result(const LLSD& content)
{
	LLFloaterScriptLimits* instance = LLFloaterReg::getTypedInstance<LLFloaterScriptLimits>("script_limits");

	if(!instance)
	{
		llinfos << "Failed to get llfloaterscriptlimits instance" << llendl;
	}
	else
	{
		LLTabContainer* tab = instance->getChild<LLTabContainer>("scriptlimits_panels");
		LLPanelScriptLimitsAttachment* panel = (LLPanelScriptLimitsAttachment*)tab->getChild<LLPanel>("script_limits_my_avatar_panel");
		panel->setAttachmentDetails(content);
	}
}

void fetchScriptLimitsAttachmentInfoResponder::error(U32 status, const std::string& reason)
{
	llinfos << "Error from responder " << reason << llendl;
}

///----------------------------------------------------------------------------
// Memory Panel
///----------------------------------------------------------------------------

BOOL LLPanelScriptLimitsRegionMemory::getLandScriptResources()
{
	LLSD body;
	std::string url = gAgent.getRegion()->getCapability("LandResources");
	if (!url.empty())
	{
		body["parcel_id"] = mParcelId;

		LLSD info;
		info["parcel_id"] = mParcelId;
		LLHTTPClient::post(url, body, new fetchScriptLimitsRegionInfoResponder(info));
				
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

void LLPanelScriptLimitsRegionMemory::processParcelInfo(const LLParcelData& parcel_data)
{
	mParcelId = parcel_data.parcel_id;

	if(!getLandScriptResources())
	{
		std::string msg_error = LLTrans::getString("ScriptLimitsRequestError");
		childSetValue("loading_text", LLSD(msg_error));
	}
	else
	{
		std::string msg_waiting = LLTrans::getString("ScriptLimitsRequestWaiting");
		childSetValue("loading_text", LLSD(msg_waiting));
	}	
}

void LLPanelScriptLimitsRegionMemory::setParcelID(const LLUUID& parcel_id)
{
	if (!parcel_id.isNull())
	{
		LLRemoteParcelInfoProcessor::getInstance()->addObserver(parcel_id, this);
		LLRemoteParcelInfoProcessor::getInstance()->sendParcelInfoRequest(parcel_id);
	}
	else
	{
		std::string msg_error = LLTrans::getString("ScriptLimitsRequestError");
		childSetValue("loading_text", LLSD(msg_error));
	}
}

// virtual
void LLPanelScriptLimitsRegionMemory::setErrorStatus(U32 status, const std::string& reason)
{
	llerrs << "Can't handle remote parcel request."<< " Http Status: "<< status << ". Reason : "<< reason<<llendl;
}

void LLPanelScriptLimitsRegionMemory::setRegionDetails(LLSD content)
{
	LLScrollListCtrl *list = getChild<LLScrollListCtrl>("scripts_list");

	S32 number_parcels = content["parcels"].size();

	LLStringUtil::format_map_t args_parcels;
	args_parcels["[PARCELS]"] = llformat ("%d", number_parcels);
	std::string msg_parcels = LLTrans::getString("ScriptLimitsParcelsOwned", args_parcels);
	childSetValue("parcels_listed", LLSD(msg_parcels));

	S32 total_objects = 0;
	S32 total_size = 0;

	for(S32 i = 0; i < number_parcels; i++)
	{
		std::string parcel_name = content["parcels"][i]["name"].asString();
		
		S32 number_objects = content["parcels"][i]["objects"].size();
		for(S32 j = 0; j < number_objects; j++)
		{
			S32 size = content["parcels"][i]["objects"][j]["resources"]["memory"].asInteger() / 1024;
			total_size += size;
			
			std::string name_buf = content["parcels"][i]["objects"][j]["name"].asString();
			LLUUID task_id = content["parcels"][i]["objects"][j]["id"].asUUID();

			LLSD element;

			element["id"] = task_id;
			element["columns"][0]["column"] = "size";
			element["columns"][0]["value"] = llformat("%d", size);
			element["columns"][0]["font"] = "SANSSERIF";
			element["columns"][1]["column"] = "name";
			element["columns"][1]["value"] = name_buf;
			element["columns"][1]["font"] = "SANSSERIF";
			element["columns"][2]["column"] = "owner";
			element["columns"][2]["value"] = "";
			element["columns"][2]["font"] = "SANSSERIF";
			element["columns"][3]["column"] = "location";
			element["columns"][3]["value"] = parcel_name;
			element["columns"][3]["font"] = "SANSSERIF";

			list->addElement(element);
			mObjectListIDs.push_back(task_id);
			total_objects++;
		}
	}
	
	mParcelMemoryUsed =total_size;
	mGotParcelMemoryUsed = TRUE;
	populateParcelMemoryText();
}

void LLPanelScriptLimitsRegionMemory::populateParcelMemoryText()
{
	if(mGotParcelMemoryUsed && mGotParcelMemoryMax)
	{
#ifdef USE_SIMPLE_SUMMARY
		LLStringUtil::format_map_t args_parcel_memory;
		args_parcel_memory["[COUNT]"] = llformat ("%d", mParcelMemoryUsed);
		std::string msg_parcel_memory = LLTrans::getString("ScriptLimitsMemoryUsedSimple", args_parcel_memory);
		childSetValue("memory_used", LLSD(msg_parcel_memory));
#else
		S32 parcel_memory_available = mParcelMemoryMax - mParcelMemoryUsed;
	
		LLStringUtil::format_map_t args_parcel_memory;
		args_parcel_memory["[COUNT]"] = llformat ("%d", mParcelMemoryUsed);
		args_parcel_memory["[MAX]"] = llformat ("%d", mParcelMemoryMax);
		args_parcel_memory["[AVAILABLE]"] = llformat ("%d", parcel_memory_available);
		std::string msg_parcel_memory = LLTrans::getString("ScriptLimitsMemoryUsed", args_parcel_memory);
		childSetValue("memory_used", LLSD(msg_parcel_memory));
#endif

		childSetValue("loading_text", LLSD(std::string("")));
	}
}

void LLPanelScriptLimitsRegionMemory::setRegionSummary(LLSD content)
{
	if(content["summary"]["available"][0]["type"].asString() == std::string("memory"))
	{
		mParcelMemoryMax = content["summary"]["available"][0]["amount"].asInteger();
		mGotParcelMemoryMax = TRUE;
	}
	else if(content["summary"]["available"][1]["type"].asString() == std::string("memory"))
	{
		mParcelMemoryMax = content["summary"]["available"][1]["amount"].asInteger();
		mGotParcelMemoryMax = TRUE;
	}
	else
	{
		llinfos << "summary doesn't contain memory info" << llendl;
		return;
	}
/*
	currently this is broken on the server, so we get this value from the details section
	and update via populateParcelMemoryText() when both sets of information have been returned

	when the sim is fixed this should be used instead:
	if(content["summary"]["used"][0]["type"].asString() == std::string("memory"))
	{
		mParcelMemoryUsed = content["summary"]["used"][0]["amount"].asInteger();
		mGotParcelMemoryUsed = TRUE;
	}
	else if(content["summary"]["used"][1]["type"].asString() == std::string("memory"))
	{
		mParcelMemoryUsed = content["summary"]["used"][1]["amount"].asInteger();
		mGotParcelMemoryUsed = TRUE;
	}
	else
	{
		//ERROR!!!
		return;
	}*/

	populateParcelMemoryText();
}

BOOL LLPanelScriptLimitsRegionMemory::postBuild()
{
	childSetAction("refresh_list_btn", onClickRefresh, this);
	childSetAction("highlight_btn", onClickHighlight, this);
	childSetAction("return_btn", onClickReturn, this);
		
	std::string msg_waiting = LLTrans::getString("ScriptLimitsRequestWaiting");
	childSetValue("loading_text", LLSD(msg_waiting));
	
	return StartRequestChain();
}

BOOL LLPanelScriptLimitsRegionMemory::StartRequestChain()
{
	LLUUID region_id;
	
	LLFloaterLand* instance = LLFloaterReg::getTypedInstance<LLFloaterLand>("about_land");
	if(!instance)
	{
		//this isnt really an error...
//		llinfos << "Failed to get about land instance" << llendl;
//		std::string msg_waiting = LLTrans::getString("ScriptLimitsRequestError");
		childSetValue("loading_text", LLSD(std::string("")));
		//might have to do parent post build here
		//if not logic below could use early outs
		return FALSE;
	}

	LLTabContainer* tab = instance->getChild<LLTabContainer>("scriptlimits_panels");
	LLPanelScriptLimitsRegionURLs* panel_urls = (LLPanelScriptLimitsRegionURLs*)tab->getChild<LLPanel>("script_limits_region_urls_panel");

	LLParcel* parcel = instance->getCurrentSelectedParcel();
	LLViewerRegion* region = LLViewerParcelMgr::getInstance()->getSelectionRegion();
	
	LLUUID current_region_id = gAgent.getRegion()->getRegionID();

	if ((region) && (parcel))
	{
		LLVector3 parcel_center = parcel->getCenterpoint();
		
		region_id = region->getRegionID();
		
		if(region_id != current_region_id)
		{
			std::string msg_wrong_region = LLTrans::getString("ScriptLimitsRequestWrongRegion");
			childSetValue("loading_text", LLSD(msg_wrong_region));
			panel_urls->childSetValue("loading_text", LLSD(msg_wrong_region));
			return FALSE;
		}
		
		LLVector3d pos_global = region->getCenterGlobal();
		
		LLSD body;
		std::string url = region->getCapability("RemoteParcelRequest");
		if (!url.empty())
		{
			body["location"] = ll_sd_from_vector3(parcel_center);
			if (!region_id.isNull())
			{
				body["region_id"] = region_id;
			}
			if (!pos_global.isExactlyZero())
			{
				U64 region_handle = to_region_handle(pos_global);
				body["region_handle"] = ll_sd_from_U64(region_handle);
			}
			LLHTTPClient::post(url, body, new LLRemoteParcelRequestResponder(getObserverHandle()));
		}
		else
		{
			llwarns << "Can't get parcel info for script information request" << region_id
					<< ". Region: "	<< region->getName()
					<< " does not support RemoteParcelRequest" << llendl;
					
			std::string msg_waiting = LLTrans::getString("ScriptLimitsRequestError");
			childSetValue("loading_text", LLSD(msg_waiting));
			panel_urls->childSetValue("loading_text", LLSD(msg_waiting));
		}
	}
	else
	{
		std::string msg_waiting = LLTrans::getString("ScriptLimitsRequestError");
		childSetValue("loading_text", LLSD(msg_waiting));
		panel_urls->childSetValue("loading_text", LLSD(msg_waiting));
	}

	return LLPanelScriptLimitsInfo::postBuild();
}

void LLPanelScriptLimitsRegionMemory::clearList()
{
	LLCtrlListInterface *list = childGetListInterface("scripts_list");
	
	if (list)
	{
		list->operateOnAll(LLCtrlListInterface::OP_DELETE);
	}

	mGotParcelMemoryUsed = FALSE;
	mGotParcelMemoryMax = FALSE;
	
	LLStringUtil::format_map_t args_parcel_memory;
	std::string msg_empty_string("");
	childSetValue("memory_used", LLSD(msg_empty_string));
	childSetValue("parcels_listed", LLSD(msg_empty_string));

	mObjectListIDs.clear();
}

// static
void LLPanelScriptLimitsRegionMemory::onClickRefresh(void* userdata)
{
	llinfos << "LLPanelRegionGeneralInfo::onClickRefresh" << llendl;
	
	LLFloaterScriptLimits* instance = LLFloaterReg::getTypedInstance<LLFloaterScriptLimits>("script_limits");
	if(instance)
	{
		LLTabContainer* tab = instance->getChild<LLTabContainer>("scriptlimits_panels");
		LLPanelScriptLimitsRegionMemory* panel_memory = (LLPanelScriptLimitsRegionMemory*)tab->getChild<LLPanel>("script_limits_region_memory_panel");
		panel_memory->clearList();

		LLPanelScriptLimitsRegionURLs* panel_urls = (LLPanelScriptLimitsRegionURLs*)tab->getChild<LLPanel>("script_limits_region_urls_panel");
		panel_urls->clearList();
		
		panel_memory->StartRequestChain();
		return;
	}
	else
	{
		llwarns << "could not find LLPanelScriptLimitsRegionMemory instance after refresh button clicked" << llendl;
		return;
	}
}

void LLPanelScriptLimitsRegionMemory::showBeacon()
{	
/*	LLScrollListCtrl* list = getChild<LLScrollListCtrl>("scripts_list");
	if (!list) return;

	LLScrollListItem* first_selected = list->getFirstSelected();
	if (!first_selected) return;

	std::string name = first_selected->getColumn(1)->getValue().asString();
	std::string pos_string =  first_selected->getColumn(3)->getValue().asString();
	
	llinfos << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>" <<llendl;
	llinfos << "name = " << name << " pos = " << pos_string << llendl;

	F32 x, y, z;
	S32 matched = sscanf(pos_string.c_str(), "<%g,%g,%g>", &x, &y, &z);
	if (matched != 3) return;

	LLVector3 pos_agent(x, y, z);
	LLVector3d pos_global = gAgent.getPosGlobalFromAgent(pos_agent);
	llinfos << "name = " << name << " pos = " << pos_string << llendl;
	std::string tooltip("");
	LLTracker::trackLocation(pos_global, name, tooltip, LLTracker::LOCATION_ITEM);*/
}

// static
void LLPanelScriptLimitsRegionMemory::onClickHighlight(void* userdata)
{
/*	llinfos << "LLPanelRegionGeneralInfo::onClickHighlight" << llendl;
	LLFloaterScriptLimits* instance = LLFloaterReg::getTypedInstance<LLFloaterScriptLimits>("script_limits");
	if(instance)
	{
		LLTabContainer* tab = instance->getChild<LLTabContainer>("scriptlimits_panels");
		LLPanelScriptLimitsRegionMemory* panel = (LLPanelScriptLimitsRegionMemory*)tab->getChild<LLPanel>("script_limits_region_memory_panel");
		panel->showBeacon();
		return;
	}
	else
	{
		llwarns << "could not find LLPanelScriptLimitsRegionMemory instance after highlight button clicked" << llendl;
//		std::string msg_waiting = LLTrans::getString("ScriptLimitsRequestError");
//		panel->childSetValue("loading_text", LLSD(msg_waiting));
		return;
	}*/
}

void LLPanelScriptLimitsRegionMemory::returnObjects()
{
/*	llinfos << "started" << llendl;
	LLMessageSystem *msg = gMessageSystem;

	LLViewerRegion* region = gAgent.getRegion();
	if (!region) return;

	llinfos << "got region" << llendl;
	LLCtrlListInterface *list = childGetListInterface("scripts_list");
	if (!list || list->getItemCount() == 0) return;

	llinfos << "got list" << llendl;
	std::vector<LLUUID>::iterator id_itor;

	bool start_message = true;

	for (id_itor = mObjectListIDs.begin(); id_itor != mObjectListIDs.end(); ++id_itor)
	{
		LLUUID task_id = *id_itor;
		llinfos << task_id << llendl;
		if (!list->isSelected(task_id))
		{
			llinfos << "not selected" << llendl;
			// Selected only
			continue;
		}
		llinfos << "selected" << llendl;
		if (start_message)
		{
			msg->newMessageFast(_PREHASH_ParcelReturnObjects);
			msg->nextBlockFast(_PREHASH_AgentData);
			msg->addUUIDFast(_PREHASH_AgentID,	gAgent.getID());
			msg->addUUIDFast(_PREHASH_SessionID,gAgent.getSessionID());
			msg->nextBlockFast(_PREHASH_ParcelData);
			msg->addS32Fast(_PREHASH_LocalID, -1); // Whole region
			msg->addS32Fast(_PREHASH_ReturnType, RT_LIST);
			start_message = false;
			llinfos << "start message" << llendl;
		}

		msg->nextBlockFast(_PREHASH_TaskIDs);
		msg->addUUIDFast(_PREHASH_TaskID, task_id);
		llinfos << "added id" << llendl;

		if (msg->isSendFullFast(_PREHASH_TaskIDs))
		{
			msg->sendReliable(region->getHost());
			start_message = true;
			llinfos << "sent 1" << llendl;
		}
	}

	if (!start_message)
	{
		msg->sendReliable(region->getHost());
		llinfos << "sent 2" << llendl;
	}*/
}

// static
void LLPanelScriptLimitsRegionMemory::onClickReturn(void* userdata)
{
/*	llinfos << "LLPanelRegionGeneralInfo::onClickReturn" << llendl;
	LLFloaterScriptLimits* instance = LLFloaterReg::getTypedInstance<LLFloaterScriptLimits>("script_limits");
	if(instance)
	{
		LLTabContainer* tab = instance->getChild<LLTabContainer>("scriptlimits_panels");
		LLPanelScriptLimitsRegionMemory* panel = (LLPanelScriptLimitsRegionMemory*)tab->getChild<LLPanel>("script_limits_region_memory_panel");
		panel->returnObjects();
		return;
	}
	else
	{
		llwarns << "could not find LLPanelScriptLimitsRegionMemory instance after highlight button clicked" << llendl;
//		std::string msg_waiting = LLTrans::getString("ScriptLimitsRequestError");
//		panel->childSetValue("loading_text", LLSD(msg_waiting));
		return;
	}*/
}

///----------------------------------------------------------------------------
// URLs Panel
///----------------------------------------------------------------------------

void LLPanelScriptLimitsRegionURLs::setRegionDetails(LLSD content)
{
	LLScrollListCtrl *list = getChild<LLScrollListCtrl>("scripts_list");

	S32 number_parcels = content["parcels"].size();

	LLStringUtil::format_map_t args_parcels;
	args_parcels["[PARCELS]"] = llformat ("%d", number_parcels);
	std::string msg_parcels = LLTrans::getString("ScriptLimitsParcelsOwned", args_parcels);
	childSetValue("parcels_listed", LLSD(msg_parcels));

	S32 total_objects = 0;
	S32 total_size = 0;

	for(S32 i = 0; i < number_parcels; i++)
	{
		std::string parcel_name = content["parcels"][i]["name"].asString();
		llinfos << parcel_name << llendl;

		S32 number_objects = content["parcels"][i]["objects"].size();
		for(S32 j = 0; j < number_objects; j++)
		{
			if(content["parcels"][i]["objects"][j]["resources"].has("urls"))
			{
				S32 size = content["parcels"][i]["objects"][j]["resources"]["urls"].asInteger();
				total_size += size;
				
				std::string name_buf = content["parcels"][i]["objects"][j]["name"].asString();
				LLUUID task_id = content["parcels"][i]["objects"][j]["id"].asUUID();

				LLSD element;

				element["id"] = task_id;
				element["columns"][0]["column"] = "urls";
				element["columns"][0]["value"] = llformat("%d", size);
				element["columns"][0]["font"] = "SANSSERIF";
				element["columns"][1]["column"] = "name";
				element["columns"][1]["value"] = name_buf;
				element["columns"][1]["font"] = "SANSSERIF";
				element["columns"][2]["column"] = "owner";
				element["columns"][2]["value"] = "";
				element["columns"][2]["font"] = "SANSSERIF";
				element["columns"][3]["column"] = "location";
				element["columns"][3]["value"] = parcel_name;
				element["columns"][3]["font"] = "SANSSERIF";

				list->addElement(element);
				mObjectListIDs.push_back(task_id);
				total_objects++;
			}
		}
	}
	
	mParcelURLsUsed =total_size;
	mGotParcelURLsUsed = TRUE;
	populateParcelURLsText();
}

void LLPanelScriptLimitsRegionURLs::populateParcelURLsText()
{
	if(mGotParcelURLsUsed && mGotParcelURLsMax)
	{

#ifdef USE_SIMPLE_SUMMARY
		LLStringUtil::format_map_t args_parcel_urls;
		args_parcel_urls["[COUNT]"] = llformat ("%d", mParcelURLsUsed);
		std::string msg_parcel_urls = LLTrans::getString("ScriptLimitsURLsUsedSimple", args_parcel_urls);
		childSetValue("urls_used", LLSD(msg_parcel_urls));
#else
		S32 parcel_urls_available = mParcelURLsMax - mParcelURLsUsed;

		LLStringUtil::format_map_t args_parcel_urls;
		args_parcel_urls["[COUNT]"] = llformat ("%d", mParcelURLsUsed);
		args_parcel_urls["[MAX]"] = llformat ("%d", mParcelURLsMax);
		args_parcel_urls["[AVAILABLE]"] = llformat ("%d", parcel_urls_available);
		std::string msg_parcel_urls = LLTrans::getString("ScriptLimitsURLsUsed", args_parcel_urls);
		childSetValue("urls_used", LLSD(msg_parcel_urls));
#endif

		childSetValue("loading_text", LLSD(std::string("")));

	}
}

void LLPanelScriptLimitsRegionURLs::setRegionSummary(LLSD content)
{
	if(content["summary"]["available"][0]["type"].asString() == std::string("urls"))
	{
		mParcelURLsMax = content["summary"]["available"][0]["amount"].asInteger();
		mGotParcelURLsMax = TRUE;
	}
	else if(content["summary"]["available"][1]["type"].asString() == std::string("urls"))
	{
		mParcelURLsMax = content["summary"]["available"][1]["amount"].asInteger();
		mGotParcelURLsMax = TRUE;
	}
	else
	{
		llinfos << "summary contains no url info" << llendl;
		return;
	}
/*
	currently this is broken on the server, so we get this value from the details section
	and update via populateParcelMemoryText() when both sets of information have been returned

	when the sim is fixed this should be used instead:
	if(content["summary"]["used"][0]["type"].asString() == std::string("urls"))
	{
		mParcelURLsUsed = content["summary"]["used"][0]["amount"].asInteger();
		mGotParcelURLsUsed = TRUE;
	}
	else if(content["summary"]["used"][1]["type"].asString() == std::string("urls"))
	{
		mParcelURLsUsed = content["summary"]["used"][1]["amount"].asInteger();
		mGotParcelURLsUsed = TRUE;
	}
	else
	{
		//ERROR!!!
		return;
	}*/

	populateParcelURLsText();
}

BOOL LLPanelScriptLimitsRegionURLs::postBuild()
{
	childSetAction("refresh_list_btn", onClickRefresh, this);
	childSetAction("highlight_btn", onClickHighlight, this);
	childSetAction("return_btn", onClickReturn, this);
		
	std::string msg_waiting = LLTrans::getString("ScriptLimitsRequestWaiting");
	childSetValue("loading_text", LLSD(msg_waiting));
	return FALSE;
}

void LLPanelScriptLimitsRegionURLs::clearList()
{
	LLCtrlListInterface *list = childGetListInterface("scripts_list");

	if (list)
	{
		list->operateOnAll(LLCtrlListInterface::OP_DELETE);
	}

	mGotParcelURLsUsed = FALSE;
	mGotParcelURLsMax = FALSE;
	
	LLStringUtil::format_map_t args_parcel_urls;
	std::string msg_empty_string("");
	childSetValue("urls_used", LLSD(msg_empty_string));
	childSetValue("parcels_listed", LLSD(msg_empty_string));

	mObjectListIDs.clear();
}

// static
void LLPanelScriptLimitsRegionURLs::onClickRefresh(void* userdata)
{
	llinfos << "Refresh clicked" << llendl;
	
	LLFloaterScriptLimits* instance = LLFloaterReg::getTypedInstance<LLFloaterScriptLimits>("script_limits");
	if(instance)
	{
		LLTabContainer* tab = instance->getChild<LLTabContainer>("scriptlimits_panels");
		LLPanelScriptLimitsRegionMemory* panel_memory = (LLPanelScriptLimitsRegionMemory*)tab->getChild<LLPanel>("script_limits_region_memory_panel");
		// use the memory panel to re-request all the info
		panel_memory->clearList();

		LLPanelScriptLimitsRegionURLs* panel_urls = (LLPanelScriptLimitsRegionURLs*)tab->getChild<LLPanel>("script_limits_region_urls_panel");
		// but the urls panel to clear itself
		panel_urls->clearList();

		panel_memory->StartRequestChain();
		return;
	}
	else
	{
		llwarns << "could not find LLPanelScriptLimitsRegionMemory instance after refresh button clicked" << llendl;
		return;
	}
}

// static
void LLPanelScriptLimitsRegionURLs::onClickHighlight(void* userdata)
{
/*	llinfos << "Highlight clicked" << llendl;
	LLFloaterScriptLimits* instance = LLFloaterReg::getTypedInstance<LLFloaterScriptLimits>("script_limits");
	if(instance)
	{
		LLTabContainer* tab = instance->getChild<LLTabContainer>("scriptlimits_panels");
		LLPanelScriptLimitsRegionMemory* panel = (LLPanelScriptLimitsRegionMemory*)tab->getChild<LLPanel>("script_limits_region_memory_panel");
		// use the beacon function from the memory panel
		panel->showBeacon();
		return;
	}
	else
	{
		llwarns << "could not find LLPanelScriptLimitsRegionMemory instance after highlight button clicked" << llendl;
//		std::string msg_waiting = LLTrans::getString("ScriptLimitsRequestError");
//		panel->childSetValue("loading_text", LLSD(msg_waiting));
		return;
	}*/
}

// static
void LLPanelScriptLimitsRegionURLs::onClickReturn(void* userdata)
{
/*	llinfos << "Return clicked" << llendl;
	LLFloaterScriptLimits* instance = LLFloaterReg::getTypedInstance<LLFloaterScriptLimits>("script_limits");
	if(instance)
	{
		LLTabContainer* tab = instance->getChild<LLTabContainer>("scriptlimits_panels");
		LLPanelScriptLimitsRegionMemory* panel = (LLPanelScriptLimitsRegionMemory*)tab->getChild<LLPanel>("script_limits_region_memory_panel");
		// use the return function from the memory panel
		panel->returnObjects();
		return;
	}
	else
	{
		llwarns << "could not find LLPanelScriptLimitsRegionMemory instance after highlight button clicked" << llendl;
//		std::string msg_waiting = LLTrans::getString("ScriptLimitsRequestError");
//		panel->childSetValue("loading_text", LLSD(msg_waiting));
		return;
	}*/
}

///----------------------------------------------------------------------------
// Attachment Panel
///----------------------------------------------------------------------------

BOOL LLPanelScriptLimitsAttachment::requestAttachmentDetails()
{
	LLSD body;
	std::string url = gAgent.getRegion()->getCapability("AttachmentResources");
	if (!url.empty())
	{
		LLHTTPClient::get(url, body, new fetchScriptLimitsAttachmentInfoResponder());
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

void LLPanelScriptLimitsAttachment::setAttachmentDetails(LLSD content)
{
	LLScrollListCtrl *list = getChild<LLScrollListCtrl>("scripts_list");
	S32 number_attachments = content["attachments"].size();

	for(int i = 0; i < number_attachments; i++)
	{
		std::string humanReadableLocation = "";
		if(content["attachments"][i].has("location"))
		{
			std::string actualLocation = content["attachments"][i]["location"];
			humanReadableLocation = LLTrans::getString(actualLocation.c_str());
		}
		
		S32 number_objects = content["attachments"][i]["objects"].size();
		for(int j = 0; j < number_objects; j++)
		{
			LLUUID task_id = content["attachments"][i]["objects"][j]["id"].asUUID();
			S32 size = 0;
			if(content["attachments"][i]["objects"][j]["resources"].has("memory"))
			{
				size = content["attachments"][i]["objects"][j]["resources"]["memory"].asInteger();
			}
			S32 urls = 0;
			if(content["attachments"][i]["objects"][j]["resources"].has("urls"))
			{
				urls = content["attachments"][i]["objects"][j]["resources"]["urls"].asInteger();
			}
			std::string name = content["attachments"][i]["objects"][j]["name"].asString();
			
			LLSD element;

			element["id"] = task_id;
			element["columns"][0]["column"] = "size";
			element["columns"][0]["value"] = llformat("%d", size);
			element["columns"][0]["font"] = "SANSSERIF";

			element["columns"][1]["column"] = "urls";
			element["columns"][1]["value"] = llformat("%d", urls);
			element["columns"][1]["font"] = "SANSSERIF";
			
			element["columns"][2]["column"] = "name";
			element["columns"][2]["value"] = name;
			element["columns"][2]["font"] = "SANSSERIF";
			
			element["columns"][3]["column"] = "location";
			element["columns"][3]["value"] = humanReadableLocation;
			element["columns"][3]["font"] = "SANSSERIF";

			list->addElement(element);
		}
	}

	childSetValue("loading_text", LLSD(std::string("")));
}

BOOL LLPanelScriptLimitsAttachment::postBuild()
{
	childSetAction("refresh_list_btn", onClickRefresh, this);
		
	std::string msg_waiting = LLTrans::getString("ScriptLimitsRequestWaiting");
	childSetValue("loading_text", LLSD(msg_waiting));
	return requestAttachmentDetails();
}

void LLPanelScriptLimitsAttachment::clearList()
{
	LLCtrlListInterface *list = childGetListInterface("scripts_list");

	if (list)
	{
		list->operateOnAll(LLCtrlListInterface::OP_DELETE);
	}

	std::string msg_waiting = LLTrans::getString("ScriptLimitsRequestWaiting");
	childSetValue("loading_text", LLSD(msg_waiting));
}

// static
void LLPanelScriptLimitsAttachment::onClickRefresh(void* userdata)
{
	llinfos << "Refresh clicked" << llendl;
	
	LLFloaterScriptLimits* instance = LLFloaterReg::getTypedInstance<LLFloaterScriptLimits>("script_limits");
	if(instance)
	{
		LLTabContainer* tab = instance->getChild<LLTabContainer>("scriptlimits_panels");
		LLPanelScriptLimitsAttachment* panel_attachments = (LLPanelScriptLimitsAttachment*)tab->getChild<LLPanel>("script_limits_my_avatar_panel");
		panel_attachments->clearList();
		panel_attachments->requestAttachmentDetails();
		return;
	}
	else
	{
		llwarns << "could not find LLPanelScriptLimitsRegionMemory instance after refresh button clicked" << llendl;
		return;
	}
}
