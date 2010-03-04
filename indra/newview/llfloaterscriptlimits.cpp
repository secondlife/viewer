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

// debug switches, won't work in release
#ifndef LL_RELEASE_FOR_DOWNLOAD

// dump responder replies to llinfos for debugging
//#define DUMP_REPLIES_TO_LLINFOS

#ifdef DUMP_REPLIES_TO_LLINFOS
#include "llsdserialize.h"
#include "llwindow.h"
#endif

// use fake LLSD responses to check the viewer side is working correctly
// I'm syncing this with the server side efforts so hopfully we can keep
// the to-ing and fro-ing between the two teams to a minimum
//#define USE_FAKE_RESPONSES

#ifdef USE_FAKE_RESPONSES
const S32 FAKE_NUMBER_OF_URLS = 329;
const S32 FAKE_AVAILABLE_URLS = 731;
const S32 FAKE_AMOUNT_OF_MEMORY = 66741;
const S32 FAKE_AVAILABLE_MEMORY = 895577;
#endif

#endif

const S32 SIZE_OF_ONE_KB = 1024;

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
	
	if(!mTab)
	{
		llinfos << "Error! couldn't get scriptlimits_panels, aborting Script Information setup" << llendl;
		return FALSE;
	}

	// contruct the panels
	std::string land_url = gAgent.getRegion()->getCapability("LandResources");
	if (!land_url.empty())
	{
		LLPanelScriptLimitsRegionMemory* panel_memory;
		panel_memory = new LLPanelScriptLimitsRegionMemory;
		mInfoPanels.push_back(panel_memory);
		LLUICtrlFactory::getInstance()->buildPanel(panel_memory, "panel_script_limits_region_memory.xml");
		mTab->addTabPanel(panel_memory);
	}
	
	std::string attachment_url = gAgent.getRegion()->getCapability("AttachmentResources");
	if (!attachment_url.empty())
	{
		LLPanelScriptLimitsAttachment* panel_attachments = new LLPanelScriptLimitsAttachment;
		mInfoPanels.push_back(panel_attachments);
		LLUICtrlFactory::getInstance()->buildPanel(panel_attachments, "panel_script_limits_my_avatar.xml");
		mTab->addTabPanel(panel_attachments);
	}
	
	if(mInfoPanels.size() > 0)
	{
		mTab->selectTab(0);
	}

	if(!selectParcelPanel && (mInfoPanels.size() > 1))
	{
		mTab->selectTab(1);
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
	//we don't need to test with a fake respose here (shouldn't anyway)

#ifdef DUMP_REPLIES_TO_LLINFOS

	LLSDNotationStreamer notation_streamer(content);
	std::ostringstream nice_llsd;
	nice_llsd << notation_streamer;

	OSMessageBox(nice_llsd.str(), "main cap response:", 0);

	llinfos << "main cap response:" << content << llendl;

#endif

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

void fetchScriptLimitsRegionSummaryResponder::result(const LLSD& content_ref)
{
#ifdef USE_FAKE_RESPONSES

	LLSD fake_content;
	LLSD summary = LLSD::emptyMap();
	LLSD available = LLSD::emptyArray();
	LLSD available_urls = LLSD::emptyMap();
	LLSD available_memory = LLSD::emptyMap();
	LLSD used = LLSD::emptyArray();
	LLSD used_urls = LLSD::emptyMap();
	LLSD used_memory = LLSD::emptyMap();

	used_urls["type"] = "urls";
	used_urls["amount"] = FAKE_NUMBER_OF_URLS;
	available_urls["type"] = "urls";
	available_urls["amount"] = FAKE_AVAILABLE_URLS;
	used_memory["type"] = "memory";
	used_memory["amount"] = FAKE_AMOUNT_OF_MEMORY;
	available_memory["type"] = "memory";
	available_memory["amount"] = FAKE_AVAILABLE_MEMORY;

//summary response:{'summary':{'available':[{'amount':i731,'type':'urls'},{'amount':i895577,'type':'memory'},{'amount':i731,'type':'urls'},{'amount':i895577,'type':'memory'}],'used':[{'amount':i329,'type':'urls'},{'amount':i66741,'type':'memory'}]}}

	used.append(used_urls);
	used.append(used_memory);
	available.append(available_urls);
	available.append(available_memory);

	summary["available"] = available;
	summary["used"] = used;
	
	fake_content["summary"] = summary;

	const LLSD& content = fake_content;

#else

	const LLSD& content = content_ref;

#endif


#ifdef DUMP_REPLIES_TO_LLINFOS

	LLSDNotationStreamer notation_streamer(content);
	std::ostringstream nice_llsd;
	nice_llsd << notation_streamer;

	OSMessageBox(nice_llsd.str(), "summary response:", 0);

	llinfos << "summary response:" << *content << llendl;

#endif

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
	}
}

void fetchScriptLimitsRegionSummaryResponder::error(U32 status, const std::string& reason)
{
	llinfos << "Error from responder " << reason << llendl;
}

void fetchScriptLimitsRegionDetailsResponder::result(const LLSD& content_ref)
{
#ifdef USE_FAKE_RESPONSES
/*
Updated detail service, ** denotes field added:

result (map)
+-parcels (array of maps)
  +-id (uuid)
  +-local_id (S32)**
  +-name (string)
  +-owner_id (uuid) (in ERS as owner, but owner_id in code)
  +-objects (array of maps)
    +-id (uuid)
    +-name (string)
	+-owner_id (uuid) (in ERS as owner, in code as owner_id)
	+-owner_name (sting)**
	+-location (map)**
	  +-x (float)
	  +-y (float)
	  +-z (float)
    +-resources (map) (this is wrong in the ERS but right in code)
      +-type (string)
      +-amount (int)
*/
	LLSD fake_content;
	LLSD resource = LLSD::emptyMap();
	LLSD location = LLSD::emptyMap();
	LLSD object = LLSD::emptyMap();
	LLSD objects = LLSD::emptyArray();
	LLSD parcel = LLSD::emptyMap();
	LLSD parcels = LLSD::emptyArray();

	resource["urls"] = FAKE_NUMBER_OF_URLS;
	resource["memory"] = FAKE_AMOUNT_OF_MEMORY;
	
	location["x"] = 128.0f;
	location["y"] = 128.0f;
	location["z"] = 0.0f;
	
	object["id"] = LLUUID("d574a375-0c6c-fe3d-5733-da669465afc7");
	object["name"] = "Gabs fake Object!";
	object["owner_id"] = LLUUID("8dbf2d41-69a0-4e5e-9787-0c9d297bc570");
	object["owner_name"] = "Gabs Linden";
	object["location"] = location;
	object["resources"] = resource;

	objects.append(object);

	parcel["id"] = LLUUID("da05fb28-0d20-e593-2728-bddb42dd0160");
	parcel["local_id"] = 42;
	parcel["name"] = "Gabriel Linden\'s Sub Plot";
	parcel["objects"] = objects;
	parcels.append(parcel);

	fake_content["parcels"] = parcels;
	const LLSD& content = fake_content;

#else

	const LLSD& content = content_ref;

#endif

#ifdef DUMP_REPLIES_TO_LLINFOS

	LLSDNotationStreamer notation_streamer(content);
	std::ostringstream nice_llsd;
	nice_llsd << notation_streamer;

	OSMessageBox(nice_llsd.str(), "details response:", 0);

	llinfos << "details response:" << content << llendl;

#endif

	LLFloaterScriptLimits* instance = LLFloaterReg::getTypedInstance<LLFloaterScriptLimits>("script_limits");

	if(!instance)
	{
		llinfos << "Failed to get llfloaterscriptlimits instance" << llendl;
	}
	else
	{
		LLTabContainer* tab = instance->getChild<LLTabContainer>("scriptlimits_panels");
		if(tab)
		{
			LLPanelScriptLimitsRegionMemory* panel_memory = (LLPanelScriptLimitsRegionMemory*)tab->getChild<LLPanel>("script_limits_region_memory_panel");
			if(panel_memory)
			{
				panel_memory->setRegionDetails(content);
			}
			else
			{
				llinfos << "Failed to get scriptlimits memory panel" << llendl;
			}
		}
		else
		{
			llinfos << "Failed to get scriptlimits_panels" << llendl;
		}
	}
}

void fetchScriptLimitsRegionDetailsResponder::error(U32 status, const std::string& reason)
{
	llinfos << "Error from responder " << reason << llendl;
}

void fetchScriptLimitsAttachmentInfoResponder::result(const LLSD& content_ref)
{

#ifdef USE_FAKE_RESPONSES

	// just add the summary, as that's all I'm testing currently!
	LLSD fake_content = LLSD::emptyMap();
	LLSD summary = LLSD::emptyMap();
	LLSD available = LLSD::emptyArray();
	LLSD available_urls = LLSD::emptyMap();
	LLSD available_memory = LLSD::emptyMap();
	LLSD used = LLSD::emptyArray();
	LLSD used_urls = LLSD::emptyMap();
	LLSD used_memory = LLSD::emptyMap();

	used_urls["type"] = "urls";
	used_urls["amount"] = FAKE_NUMBER_OF_URLS;
	available_urls["type"] = "urls";
	available_urls["amount"] = FAKE_AVAILABLE_URLS;
	used_memory["type"] = "memory";
	used_memory["amount"] = FAKE_AMOUNT_OF_MEMORY;
	available_memory["type"] = "memory";
	available_memory["amount"] = FAKE_AVAILABLE_MEMORY;

	used.append(used_urls);
	used.append(used_memory);
	available.append(available_urls);
	available.append(available_memory);

	summary["available"] = available;
	summary["used"] = used;
	
	fake_content["summary"] = summary;
	fake_content["attachments"] = content_ref["attachments"];

	const LLSD& content = fake_content;

#else

	const LLSD& content = content_ref;

#endif

#ifdef DUMP_REPLIES_TO_LLINFOS

	LLSDNotationStreamer notation_streamer(content);
	std::ostringstream nice_llsd;
	nice_llsd << notation_streamer;

	OSMessageBox(nice_llsd.str(), "attachment response:", 0);
	
	llinfos << "attachment response:" << content << llendl;

#endif

	LLFloaterScriptLimits* instance = LLFloaterReg::getTypedInstance<LLFloaterScriptLimits>("script_limits");

	if(!instance)
	{
		llinfos << "Failed to get llfloaterscriptlimits instance" << llendl;
	}
	else
	{
		LLTabContainer* tab = instance->getChild<LLTabContainer>("scriptlimits_panels");
		if(tab)
		{
			LLPanelScriptLimitsAttachment* panel = (LLPanelScriptLimitsAttachment*)tab->getChild<LLPanel>("script_limits_my_avatar_panel");
			if(panel)
			{
				panel->setAttachmentDetails(content);
			}
			else
			{
				llinfos << "Failed to get script_limits_my_avatar_panel" << llendl;
			}
		}
		else
		{
			llinfos << "Failed to get scriptlimits_panels" << llendl;
		}
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
	llwarns << "Can't handle remote parcel request."<< " Http Status: "<< status << ". Reason : "<< reason<<llendl;
}

// callback from the name cache with an owner name to add to the list
void LLPanelScriptLimitsRegionMemory::onNameCache(
						 const LLUUID& id,
						 const std::string& first_name,
						 const std::string& last_name)
{
	std::string name = first_name + " " + last_name;

	LLScrollListCtrl *list = getChild<LLScrollListCtrl>("scripts_list");	
	if(!list)
	{
		return;
	}
	
	std::vector<LLSD>::iterator id_itor;
	for (id_itor = mObjectListItems.begin(); id_itor != mObjectListItems.end(); ++id_itor)
	{
		LLSD element = *id_itor;
		if(element["owner_id"].asUUID() == id)
		{
			LLScrollListItem* item = list->getItem(element["id"].asUUID());

			if(item)
			{
				item->getColumn(3)->setValue(LLSD(name));
				element["columns"][3]["value"] = name;
			}
		}
	}
}

void LLPanelScriptLimitsRegionMemory::setRegionDetails(LLSD content)
{
	LLScrollListCtrl *list = getChild<LLScrollListCtrl>("scripts_list");
	
	if(!list)
	{
		llinfos << "Error getting the scripts_list control" << llendl;
		return;
	}

	S32 number_parcels = content["parcels"].size();

	LLStringUtil::format_map_t args_parcels;
	args_parcels["[PARCELS]"] = llformat ("%d", number_parcels);
	std::string msg_parcels = LLTrans::getString("ScriptLimitsParcelsOwned", args_parcels);
	childSetValue("parcels_listed", LLSD(msg_parcels));

	std::vector<LLUUID> names_requested;

	// This makes the assumption that all objects will have the same set
	// of attributes, ie they will all have, or none will have locations
	// This is a pretty safe assumption as it's reliant on server version.
	bool has_locations = false;
	bool has_local_ids = false;

	for(S32 i = 0; i < number_parcels; i++)
	{
		std::string parcel_name = content["parcels"][i]["name"].asString();
		LLUUID parcel_id = content["parcels"][i]["id"].asUUID();
		S32 number_objects = content["parcels"][i]["objects"].size();

		S32 local_id = 0;
		if(content["parcels"][i].has("local_id"))
		{
			// if any locations are found flag that we can use them and turn on the highlight button
			has_local_ids = true;
			local_id = content["parcels"][i]["local_id"].asInteger();
		}

		for(S32 j = 0; j < number_objects; j++)
		{
			S32 size = content["parcels"][i]["objects"][j]["resources"]["memory"].asInteger() / SIZE_OF_ONE_KB;
			
			S32 urls = content["parcels"][i]["objects"][j]["resources"]["urls"].asInteger();
			
			std::string name_buf = content["parcels"][i]["objects"][j]["name"].asString();
			LLUUID task_id = content["parcels"][i]["objects"][j]["id"].asUUID();
			LLUUID owner_id = content["parcels"][i]["objects"][j]["owner_id"].asUUID();

			F32 location_x = 0.0f;
			F32 location_y = 0.0f;
			F32 location_z = 0.0f;

			if(content["parcels"][i]["objects"][j].has("location"))
			{
				// if any locations are found flag that we can use them and turn on the highlight button
				LLVector3 vec = ll_vector3_from_sd(content["parcels"][i]["objects"][j]["location"]);
				has_locations = true;
				location_x = vec.mV[0];
				location_y = vec.mV[1];
				location_z = vec.mV[2];
			}

			std::string owner_buf;

			// in the future the server will give us owner names, so see if we're there yet:
			if(content["parcels"][i]["objects"][j].has("owner_name"))
			{
				owner_buf = content["parcels"][i]["objects"][j]["owner_name"].asString();
			}
			// ...and if not use the slightly more painful method of disovery:
			else
			{
				BOOL name_is_cached = gCacheName->getFullName(owner_id, owner_buf);
				if(!name_is_cached)
				{
					if(std::find(names_requested.begin(), names_requested.end(), owner_id) == names_requested.end())
					{
						names_requested.push_back(owner_id);
						gCacheName->get(owner_id, TRUE,
						boost::bind(&LLPanelScriptLimitsRegionMemory::onNameCache,
							this, _1, _2, _3));
					}
				}
			}

			LLSD element;

			element["id"] = task_id;
			element["columns"][0]["column"] = "size";
			element["columns"][0]["value"] = llformat("%d", size);
			element["columns"][0]["font"] = "SANSSERIF";
			element["columns"][1]["column"] = "urls";
			element["columns"][1]["value"] = llformat("%d", urls);
			element["columns"][1]["font"] = "SANSSERIF";
			element["columns"][2]["column"] = "name";
			element["columns"][2]["value"] = name_buf;
			element["columns"][2]["font"] = "SANSSERIF";
			element["columns"][3]["column"] = "owner";
			element["columns"][3]["value"] = owner_buf;
			element["columns"][3]["font"] = "SANSSERIF";
			element["columns"][4]["column"] = "parcel";
			element["columns"][4]["value"] = parcel_name;
			element["columns"][4]["font"] = "SANSSERIF";
			element["columns"][5]["column"] = "location";
			if(has_locations)
			{
				element["columns"][5]["value"] = llformat("<%0.1f,%0.1f,%0.1f>", location_x, location_y, location_z);
			}
			else
			{
				element["columns"][5]["value"] = "";
			}
			element["columns"][5]["font"] = "SANSSERIF";

			list->addElement(element, ADD_SORTED);
			
			element["owner_id"] = owner_id;
			element["local_id"] = local_id;
			mObjectListItems.push_back(element);
		}
	}

	if (has_locations)
	{
		LLButton* btn = getChild<LLButton>("highlight_btn");
		if(btn)
		{
			btn->setVisible(true);
		}
	}

	if (has_local_ids)
	{
		LLButton* btn = getChild<LLButton>("return_btn");
		if(btn)
		{
			btn->setVisible(true);
		}
	}
	
	// save the structure to make object return easier
	mContent = content;

	childSetValue("loading_text", LLSD(std::string("")));
}

void LLPanelScriptLimitsRegionMemory::setRegionSummary(LLSD content)
{
	if(content["summary"]["used"][0]["type"].asString() == std::string("memory"))
	{
		mParcelMemoryUsed = content["summary"]["used"][0]["amount"].asInteger() / SIZE_OF_ONE_KB;
		mParcelMemoryMax = content["summary"]["available"][0]["amount"].asInteger() / SIZE_OF_ONE_KB;
		mGotParcelMemoryUsed = true;
	}
	else if(content["summary"]["used"][1]["type"].asString() == std::string("memory"))
	{
		mParcelMemoryUsed = content["summary"]["used"][1]["amount"].asInteger() / SIZE_OF_ONE_KB;
		mParcelMemoryMax = content["summary"]["available"][1]["amount"].asInteger() / SIZE_OF_ONE_KB;
		mGotParcelMemoryUsed = true;
	}
	else
	{
		llinfos << "summary doesn't contain memory info" << llendl;
		return;
	}
	
	if(content["summary"]["used"][0]["type"].asString() == std::string("urls"))
	{
		mParcelURLsUsed = content["summary"]["used"][0]["amount"].asInteger();
		mParcelURLsMax = content["summary"]["available"][0]["amount"].asInteger();
		mGotParcelURLsUsed = true;
	}
	else if(content["summary"]["used"][1]["type"].asString() == std::string("urls"))
	{
		mParcelURLsUsed = content["summary"]["used"][1]["amount"].asInteger();
		mParcelURLsMax = content["summary"]["available"][1]["amount"].asInteger();
		mGotParcelURLsUsed = true;
	}
	else
	{
		llinfos << "summary doesn't contain urls info" << llendl;
		return;
	}

	if((mParcelMemoryUsed >= 0) && (mParcelMemoryMax >= 0))
	{
		S32 parcel_memory_available = mParcelMemoryMax - mParcelMemoryUsed;

		LLStringUtil::format_map_t args_parcel_memory;
		args_parcel_memory["[COUNT]"] = llformat ("%d", mParcelMemoryUsed);
		args_parcel_memory["[MAX]"] = llformat ("%d", mParcelMemoryMax);
		args_parcel_memory["[AVAILABLE]"] = llformat ("%d", parcel_memory_available);
		std::string msg_parcel_memory = LLTrans::getString("ScriptLimitsMemoryUsed", args_parcel_memory);
		childSetValue("memory_used", LLSD(msg_parcel_memory));
	}

	if((mParcelURLsUsed >= 0) && (mParcelURLsMax >= 0))
	{
		S32 parcel_urls_available = mParcelURLsMax - mParcelURLsUsed;

		LLStringUtil::format_map_t args_parcel_urls;
		args_parcel_urls["[COUNT]"] = llformat ("%d", mParcelURLsUsed);
		args_parcel_urls["[MAX]"] = llformat ("%d", mParcelURLsMax);
		args_parcel_urls["[AVAILABLE]"] = llformat ("%d", parcel_urls_available);
		std::string msg_parcel_urls = LLTrans::getString("ScriptLimitsURLsUsed", args_parcel_urls);
		childSetValue("urls_used", LLSD(msg_parcel_urls));
	}
}

BOOL LLPanelScriptLimitsRegionMemory::postBuild()
{
	childSetAction("refresh_list_btn", onClickRefresh, this);
	childSetAction("highlight_btn", onClickHighlight, this);
	childSetAction("return_btn", onClickReturn, this);
		
	std::string msg_waiting = LLTrans::getString("ScriptLimitsRequestWaiting");
	childSetValue("loading_text", LLSD(msg_waiting));

	LLScrollListCtrl *list = getChild<LLScrollListCtrl>("scripts_list");
	if(!list)
	{
		return FALSE;
	}

	//set all columns to resizable mode even if some columns will be empty
	for(S32 column = 0; column < list->getNumColumns(); column++)
	{
		LLScrollListColumn* columnp = list->getColumn(column);
		columnp->mHeader->setHasResizableElement(TRUE);
	}

	return StartRequestChain();
}

BOOL LLPanelScriptLimitsRegionMemory::StartRequestChain()
{
	LLUUID region_id;
	
	LLFloaterLand* instance = LLFloaterReg::getTypedInstance<LLFloaterLand>("about_land");
	if(!instance)
	{
		childSetValue("loading_text", LLSD(std::string("")));
		//might have to do parent post build here
		//if not logic below could use early outs
		return FALSE;
	}
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
		}
	}
	else
	{
		std::string msg_waiting = LLTrans::getString("ScriptLimitsRequestNoParcelSelected");
		childSetValue("loading_text", LLSD(msg_waiting));
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

	mGotParcelMemoryUsed = false;
	mGotParcelMemoryMax = false;
	mGotParcelURLsUsed = false;
	mGotParcelURLsMax = false;
	
	LLStringUtil::format_map_t args_parcel_memory;
	std::string msg_empty_string("");
	childSetValue("memory_used", LLSD(msg_empty_string));
	childSetValue("urls_used", LLSD(msg_empty_string));
	childSetValue("parcels_listed", LLSD(msg_empty_string));

	mObjectListItems.clear();
}

// static
void LLPanelScriptLimitsRegionMemory::onClickRefresh(void* userdata)
{
	llinfos << "LLPanelRegionGeneralInfo::onClickRefresh" << llendl;
	
	LLFloaterScriptLimits* instance = LLFloaterReg::getTypedInstance<LLFloaterScriptLimits>("script_limits");
	if(instance)
	{
		LLTabContainer* tab = instance->getChild<LLTabContainer>("scriptlimits_panels");
		if(tab)
		{
			LLPanelScriptLimitsRegionMemory* panel_memory = (LLPanelScriptLimitsRegionMemory*)tab->getChild<LLPanel>("script_limits_region_memory_panel");
			if(panel_memory)
			{
				panel_memory->clearList();
		
				panel_memory->StartRequestChain();
			}
		}
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
	LLScrollListCtrl* list = getChild<LLScrollListCtrl>("scripts_list");
	if (!list) return;

	LLScrollListItem* first_selected = list->getFirstSelected();
	if (!first_selected) return;

	std::string name = first_selected->getColumn(2)->getValue().asString();
	std::string pos_string =  first_selected->getColumn(5)->getValue().asString();
	
	F32 x, y, z;
	S32 matched = sscanf(pos_string.c_str(), "<%g,%g,%g>", &x, &y, &z);
	if (matched != 3) return;

	LLVector3 pos_agent(x, y, z);
	LLVector3d pos_global = gAgent.getPosGlobalFromAgent(pos_agent);

	std::string tooltip("");
	LLTracker::trackLocation(pos_global, name, tooltip, LLTracker::LOCATION_ITEM);
}

// static
void LLPanelScriptLimitsRegionMemory::onClickHighlight(void* userdata)
{
	llinfos << "LLPanelRegionGeneralInfo::onClickHighlight" << llendl;
	LLFloaterScriptLimits* instance = LLFloaterReg::getTypedInstance<LLFloaterScriptLimits>("script_limits");
	if(instance)
	{
		LLTabContainer* tab = instance->getChild<LLTabContainer>("scriptlimits_panels");
		if(tab)
		{
			LLPanelScriptLimitsRegionMemory* panel = (LLPanelScriptLimitsRegionMemory*)tab->getChild<LLPanel>("script_limits_region_memory_panel");
			if(panel)
			{
				panel->showBeacon();
			}
		}
		return;
	}
	else
	{
		llwarns << "could not find LLPanelScriptLimitsRegionMemory instance after highlight button clicked" << llendl;
		return;
	}
}

void LLPanelScriptLimitsRegionMemory::returnObjectsFromParcel(S32 local_id)
{
	LLMessageSystem *msg = gMessageSystem;

	LLViewerRegion* region = gAgent.getRegion();
	if (!region) return;

	LLCtrlListInterface *list = childGetListInterface("scripts_list");
	if (!list || list->getItemCount() == 0) return;

	std::vector<LLSD>::iterator id_itor;

	bool start_message = true;

	for (id_itor = mObjectListItems.begin(); id_itor != mObjectListItems.end(); ++id_itor)
	{
		LLSD element = *id_itor;
		if (!list->isSelected(element["id"].asUUID()))
		{
			// Selected only
			continue;
		}
		
		if(element["local_id"].asInteger() != local_id)
		{
			// Not the parcel we are looking for
			continue;
		}

		if (start_message)
		{
			msg->newMessageFast(_PREHASH_ParcelReturnObjects);
			msg->nextBlockFast(_PREHASH_AgentData);
			msg->addUUIDFast(_PREHASH_AgentID,	gAgent.getID());
			msg->addUUIDFast(_PREHASH_SessionID,gAgent.getSessionID());
			msg->nextBlockFast(_PREHASH_ParcelData);
			msg->addS32Fast(_PREHASH_LocalID, element["local_id"].asInteger());
			msg->addU32Fast(_PREHASH_ReturnType, RT_LIST);
			start_message = false;
		}

		msg->nextBlockFast(_PREHASH_TaskIDs);
		msg->addUUIDFast(_PREHASH_TaskID, element["id"].asUUID());

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

void LLPanelScriptLimitsRegionMemory::returnObjects()
{
	if(!mContent.has("parcels"))
	{
		return;
	}
	
	S32 number_parcels = mContent["parcels"].size();

	// a message per parcel containing all objects to be returned from that parcel
	for(S32 i = 0; i < number_parcels; i++)
	{
		S32 local_id = 0;
		if(mContent["parcels"][i].has("local_id"))
		{
			local_id = mContent["parcels"][i]["local_id"].asInteger();
			returnObjectsFromParcel(local_id);
		}
	}

	onClickRefresh(NULL);
}


// static
void LLPanelScriptLimitsRegionMemory::onClickReturn(void* userdata)
{
	llinfos << "LLPanelRegionGeneralInfo::onClickReturn" << llendl;
	LLFloaterScriptLimits* instance = LLFloaterReg::getTypedInstance<LLFloaterScriptLimits>("script_limits");
	if(instance)
	{
		LLTabContainer* tab = instance->getChild<LLTabContainer>("scriptlimits_panels");
		if(tab)
		{
			LLPanelScriptLimitsRegionMemory* panel = (LLPanelScriptLimitsRegionMemory*)tab->getChild<LLPanel>("script_limits_region_memory_panel");
			if(panel)
			{
				panel->returnObjects();
			}
		}
		return;
	}
	else
	{
		llwarns << "could not find LLPanelScriptLimitsRegionMemory instance after highlight button clicked" << llendl;
		return;
	}
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
	
	if(!list)
	{
		return;
	}
	
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
				size = content["attachments"][i]["objects"][j]["resources"]["memory"].asInteger() / SIZE_OF_ONE_KB;
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
	
	setAttachmentSummary(content);

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

void LLPanelScriptLimitsAttachment::setAttachmentSummary(LLSD content)
{
	if(content["summary"]["used"][0]["type"].asString() == std::string("memory"))
	{
		mAttachmentMemoryUsed = content["summary"]["used"][0]["amount"].asInteger() / SIZE_OF_ONE_KB;
		mAttachmentMemoryMax = content["summary"]["available"][0]["amount"].asInteger() / SIZE_OF_ONE_KB;
		mGotAttachmentMemoryUsed = true;
	}
	else if(content["summary"]["used"][1]["type"].asString() == std::string("memory"))
	{
		mAttachmentMemoryUsed = content["summary"]["used"][1]["amount"].asInteger() / SIZE_OF_ONE_KB;
		mAttachmentMemoryMax = content["summary"]["available"][1]["amount"].asInteger() / SIZE_OF_ONE_KB;
		mGotAttachmentMemoryUsed = true;
	}
	else
	{
		llinfos << "attachment details don't contain memory summary info" << llendl;
		return;
	}
	
	if(content["summary"]["used"][0]["type"].asString() == std::string("urls"))
	{
		mAttachmentURLsUsed = content["summary"]["used"][0]["amount"].asInteger();
		mAttachmentURLsMax = content["summary"]["available"][0]["amount"].asInteger();
		mGotAttachmentURLsUsed = true;
	}
	else if(content["summary"]["used"][1]["type"].asString() == std::string("urls"))
	{
		mAttachmentURLsUsed = content["summary"]["used"][1]["amount"].asInteger();
		mAttachmentURLsMax = content["summary"]["available"][1]["amount"].asInteger();
		mGotAttachmentURLsUsed = true;
	}
	else
	{
		llinfos << "attachment details don't contain urls summary info" << llendl;
		return;
	}

	if((mAttachmentMemoryUsed >= 0) && (mAttachmentMemoryMax >= 0))
	{
		S32 attachment_memory_available = mAttachmentMemoryMax - mAttachmentMemoryUsed;

		LLStringUtil::format_map_t args_attachment_memory;
		args_attachment_memory["[COUNT]"] = llformat ("%d", mAttachmentMemoryUsed);
		args_attachment_memory["[MAX]"] = llformat ("%d", mAttachmentMemoryMax);
		args_attachment_memory["[AVAILABLE]"] = llformat ("%d", attachment_memory_available);
		std::string msg_attachment_memory = LLTrans::getString("ScriptLimitsMemoryUsed", args_attachment_memory);
		childSetValue("memory_used", LLSD(msg_attachment_memory));
	}

	if((mAttachmentURLsUsed >= 0) && (mAttachmentURLsMax >= 0))
	{
		S32 attachment_urls_available = mAttachmentURLsMax - mAttachmentURLsUsed;

		LLStringUtil::format_map_t args_attachment_urls;
		args_attachment_urls["[COUNT]"] = llformat ("%d", mAttachmentURLsUsed);
		args_attachment_urls["[MAX]"] = llformat ("%d", mAttachmentURLsMax);
		args_attachment_urls["[AVAILABLE]"] = llformat ("%d", attachment_urls_available);
		std::string msg_attachment_urls = LLTrans::getString("ScriptLimitsURLsUsed", args_attachment_urls);
		childSetValue("urls_used", LLSD(msg_attachment_urls));
	}
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

