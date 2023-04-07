/** 
 * @file llfloaterscriptlimits.cpp
 * @author Gabriel Lee
 * @brief Implementation of the region info and controls floater and panels.
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
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
#include "llfloaterscriptlimits.h"

// library includes
#include "llavatarnamecache.h"
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
#include "llcorehttputil.h"

///----------------------------------------------------------------------------
/// LLFloaterScriptLimits
///----------------------------------------------------------------------------

// debug switches, won't work in release
#ifndef LL_RELEASE_FOR_DOWNLOAD

// dump responder replies to LL_INFOS() for debugging
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
	mTab = getChild<LLTabContainer>("scriptlimits_panels");
	
	if(!mTab)
	{
		LL_WARNS() << "Error! couldn't get scriptlimits_panels, aborting Script Information setup" << LL_ENDL;
		return FALSE;
	}

	// contruct the panel
	LLPanelScriptLimitsRegionMemory* panel_memory = new LLPanelScriptLimitsRegionMemory;
	mInfoPanels.push_back(panel_memory);
	panel_memory->buildFromFile( "panel_script_limits_region_memory.xml");
	mTab->addTabPanel(panel_memory);
	mTab->selectTab(0);
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
// Memory Panel
///----------------------------------------------------------------------------

LLPanelScriptLimitsRegionMemory::~LLPanelScriptLimitsRegionMemory()
{
	if(!mParcelId.isNull())
	{
		LLRemoteParcelInfoProcessor::getInstance()->removeObserver(mParcelId, this);
		mParcelId.setNull();
	}
};

BOOL LLPanelScriptLimitsRegionMemory::getLandScriptResources()
{
	if (!gAgent.getRegion()) return FALSE;

	LLSD body;
	std::string url = gAgent.getRegion()->getCapability("LandResources");
	if (!url.empty())
	{
        LLCoros::instance().launch("LLPanelScriptLimitsRegionMemory::getLandScriptResourcesCoro",
            boost::bind(&LLPanelScriptLimitsRegionMemory::getLandScriptResourcesCoro, this, url));
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

void LLPanelScriptLimitsRegionMemory::getLandScriptResourcesCoro(std::string url)
{
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("getLandScriptResourcesCoro", httpPolicy));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);

    LLSD postData;

    postData["parcel_id"] = mParcelId;

    LLSD result = httpAdapter->postAndSuspend(httpRequest, url, postData);

    LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    if (!status)
    {
        LL_WARNS() << "Failed to get script resource info" << LL_ENDL;
        return;
    }

    // We could retrieve these sequentially inline from this coroutine. But 
    // since the original code retrieved them in parallel I'll spawn two 
    // coroutines to do the retrieval. 

    // The summary service:
    if (result.has("ScriptResourceSummary"))
    {
        std::string urlResourceSummary = result["ScriptResourceSummary"].asString();
        LLCoros::instance().launch("LLPanelScriptLimitsRegionMemory::getLandScriptSummaryCoro",
            boost::bind(&LLPanelScriptLimitsRegionMemory::getLandScriptSummaryCoro, this, urlResourceSummary));
    }

    if (result.has("ScriptResourceDetails"))
    {
        std::string urlResourceDetails = result["ScriptResourceDetails"].asString();
        LLCoros::instance().launch("LLPanelScriptLimitsRegionMemory::getLandScriptDetailsCoro",
            boost::bind(&LLPanelScriptLimitsRegionMemory::getLandScriptDetailsCoro, this, urlResourceDetails));
    }

   
}

void LLPanelScriptLimitsRegionMemory::getLandScriptSummaryCoro(std::string url)
{
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("getLandScriptSummaryCoro", httpPolicy));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);

    LLSD result = httpAdapter->getAndSuspend(httpRequest, url);

    LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    if (!status)
    {
        LL_WARNS() << "Unable to retrieve script summary." << LL_ENDL;
        return;
    }

    LLFloaterScriptLimits* instance = LLFloaterReg::getTypedInstance<LLFloaterScriptLimits>("script_limits");
    if (!instance)
    {
        LL_WARNS() << "Failed to get llfloaterscriptlimits instance" << LL_ENDL;
        return;
    }

    LLTabContainer* tab = instance->getChild<LLTabContainer>("scriptlimits_panels");
    if (!tab)
    {
        LL_WARNS() << "Unable to access script limits tab" << LL_ENDL;
        return;
    }

    LLPanelScriptLimitsRegionMemory* panelMemory = (LLPanelScriptLimitsRegionMemory*)tab->getChild<LLPanel>("script_limits_region_memory_panel");
    if (!panelMemory)
    {
        LL_WARNS() << "Unable to get memory panel." << LL_ENDL;
        return;
    }

    panelMemory->getChild<LLUICtrl>("loading_text")->setValue(LLSD(std::string("")));

    LLButton* btn = panelMemory->getChild<LLButton>("refresh_list_btn");
    if (btn)
    {
        btn->setEnabled(true);
    }

    result.erase(LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS);
    panelMemory->setRegionSummary(result);

}

void LLPanelScriptLimitsRegionMemory::getLandScriptDetailsCoro(std::string url)
{
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("getLandScriptDetailsCoro", httpPolicy));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);

    LLSD result = httpAdapter->getAndSuspend(httpRequest, url);

    LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    if (!status)
    {
        LL_WARNS() << "Unable to retrieve script details." << LL_ENDL;
        return;
    }

    LLFloaterScriptLimits* instance = LLFloaterReg::getTypedInstance<LLFloaterScriptLimits>("script_limits");

    if (!instance)
    {
        LL_WARNS() << "Failed to get llfloaterscriptlimits instance" << LL_ENDL;
        return;
    }

    LLTabContainer* tab = instance->getChild<LLTabContainer>("scriptlimits_panels");
    if (!tab)
    {
        LL_WARNS() << "Unable to access script limits tab" << LL_ENDL;
        return;
    }

    LLPanelScriptLimitsRegionMemory* panelMemory = (LLPanelScriptLimitsRegionMemory*)tab->getChild<LLPanel>("script_limits_region_memory_panel");

    if (!panelMemory)
    {
        LL_WARNS() << "Unable to get memory panel." << LL_ENDL;
        return;
    }

    result.erase(LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS);
    panelMemory->setRegionDetails(result);
}

void LLPanelScriptLimitsRegionMemory::processParcelInfo(const LLParcelData& parcel_data)
{
	if(!getLandScriptResources())
	{
		std::string msg_error = LLTrans::getString("ScriptLimitsRequestError");
		getChild<LLUICtrl>("loading_text")->setValue(LLSD(msg_error));
	}
	else
	{
		std::string msg_waiting = LLTrans::getString("ScriptLimitsRequestWaiting");
		getChild<LLUICtrl>("loading_text")->setValue(LLSD(msg_waiting));
	}
}

void LLPanelScriptLimitsRegionMemory::setParcelID(const LLUUID& parcel_id)
{
	if (!parcel_id.isNull())
	{
		if(!mParcelId.isNull())
		{
			LLRemoteParcelInfoProcessor::getInstance()->removeObserver(mParcelId, this);
			mParcelId.setNull();
		}
		mParcelId = parcel_id;
		LLRemoteParcelInfoProcessor::getInstance()->addObserver(parcel_id, this);
		LLRemoteParcelInfoProcessor::getInstance()->sendParcelInfoRequest(parcel_id);
	}
	else
	{
		std::string msg_error = LLTrans::getString("ScriptLimitsRequestError");
		getChild<LLUICtrl>("loading_text")->setValue(LLSD(msg_error));
	}
}

// virtual
void LLPanelScriptLimitsRegionMemory::setErrorStatus(S32 status, const std::string& reason)
{
	LL_WARNS() << "Can't handle remote parcel request."<< " Http Status: "<< status << ". Reason : "<< reason<<LL_ENDL;
}

// callback from the name cache with an owner name to add to the list
void LLPanelScriptLimitsRegionMemory::onAvatarNameCache(
    const LLUUID& id,
    const LLAvatarName& av_name)
{
    onNameCache(id, av_name.getUserName());
}

// callback from the name cache with an owner name to add to the list
void LLPanelScriptLimitsRegionMemory::onNameCache(
						 const LLUUID& id,
						 const std::string& full_name)
{
	LLScrollListCtrl *list = getChild<LLScrollListCtrl>("scripts_list");	
	if(!list)
	{
		return;
	}
	
	std::string name = LLCacheName::buildUsername(full_name);

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
		LL_WARNS() << "Error getting the scripts_list control" << LL_ENDL;
		return;
	}

	S32 number_parcels = content["parcels"].size();

	LLStringUtil::format_map_t args_parcels;
	args_parcels["[PARCELS]"] = llformat ("%d", number_parcels);
	std::string msg_parcels = LLTrans::getString("ScriptLimitsParcelsOwned", args_parcels);
	getChild<LLUICtrl>("parcels_listed")->setValue(LLSD(msg_parcels));

	uuid_vec_t names_requested;

	// This makes the assumption that all objects will have the same set
	// of attributes, ie they will all have, or none will have locations
	// This is a pretty safe assumption as it's reliant on server version.
	bool has_locations = false;
	bool has_local_ids = false;

	for(S32 i = 0; i < number_parcels; i++)
	{
		std::string parcel_name = content["parcels"][i]["name"].asString();
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
			// This field may not be sent by all server versions, but it's OK if
			// it uses the LLSD default of false
			bool is_group_owned = content["parcels"][i]["objects"][j]["is_group_owned"].asBoolean();

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
				BOOL name_is_cached;
				if (is_group_owned)
				{
					name_is_cached = gCacheName->getGroupName(owner_id, owner_buf);
				}
				else
				{
					LLAvatarName av_name;
					name_is_cached = LLAvatarNameCache::get(owner_id, &av_name);
					owner_buf = av_name.getUserName();
					owner_buf = LLCacheName::buildUsername(owner_buf);
				}
				if(!name_is_cached)
				{
					if(std::find(names_requested.begin(), names_requested.end(), owner_id) == names_requested.end())
					{
						names_requested.push_back(owner_id);
						if (is_group_owned)
						{
							gCacheName->getGroup(owner_id,
								boost::bind(&LLPanelScriptLimitsRegionMemory::onNameCache,
								    this, _1, _2));
						}
						else
						{
							LLAvatarNameCache::get(owner_id,
								boost::bind(&LLPanelScriptLimitsRegionMemory::onAvatarNameCache,
								    this, _1, _2));
						}
					}
				}
			}

			LLScrollListItem::Params item_params;
			item_params.value = task_id;

			LLScrollListCell::Params cell_params;
			cell_params.font = LLFontGL::getFontSansSerif();
			// Start out right justifying numeric displays
			cell_params.font_halign = LLFontGL::RIGHT;

			cell_params.column = "size";
			cell_params.value = size;
			item_params.columns.add(cell_params);

			cell_params.column = "urls";
			cell_params.value = urls;
			item_params.columns.add(cell_params);

			cell_params.font_halign = LLFontGL::LEFT;
			// The rest of the columns are text to left justify them
			cell_params.column = "name";
			cell_params.value = name_buf;
			item_params.columns.add(cell_params);

			cell_params.column = "owner";
			cell_params.value = owner_buf;
			item_params.columns.add(cell_params);

			cell_params.column = "parcel";
			cell_params.value = parcel_name;
			item_params.columns.add(cell_params);

			cell_params.column = "location";
			cell_params.value = has_locations
				? llformat("<%0.0f, %0.0f, %0.0f>", location_x, location_y, location_z)
				: "";
			item_params.columns.add(cell_params);

			list->addRow(item_params);
			
			LLSD element;
			element["owner_id"] = owner_id;

			element["id"] = task_id;
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
		LL_WARNS() << "summary doesn't contain memory info" << LL_ENDL;
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
		LL_WARNS() << "summary doesn't contain urls info" << LL_ENDL;
		return;
	}

	if((mParcelMemoryUsed >= 0) && (mParcelMemoryMax >= 0))
	{
		LLStringUtil::format_map_t args_parcel_memory;
		args_parcel_memory["[COUNT]"] = llformat ("%d", mParcelMemoryUsed);
		std::string translate_message = "ScriptLimitsMemoryUsedSimple";

		if (0 < mParcelMemoryMax)
		{
			S32 parcel_memory_available = mParcelMemoryMax - mParcelMemoryUsed;

			args_parcel_memory["[MAX]"] = llformat ("%d", mParcelMemoryMax);
			args_parcel_memory["[AVAILABLE]"] = llformat ("%d", parcel_memory_available);
			translate_message = "ScriptLimitsMemoryUsed";
		}

		std::string msg_parcel_memory = LLTrans::getString(translate_message, args_parcel_memory);
		getChild<LLUICtrl>("memory_used")->setValue(LLSD(msg_parcel_memory));
	}

	if((mParcelURLsUsed >= 0) && (mParcelURLsMax >= 0))
	{
		S32 parcel_urls_available = mParcelURLsMax - mParcelURLsUsed;

		LLStringUtil::format_map_t args_parcel_urls;
		args_parcel_urls["[COUNT]"] = llformat ("%d", mParcelURLsUsed);
		args_parcel_urls["[MAX]"] = llformat ("%d", mParcelURLsMax);
		args_parcel_urls["[AVAILABLE]"] = llformat ("%d", parcel_urls_available);
		std::string msg_parcel_urls = LLTrans::getString("ScriptLimitsURLsUsed", args_parcel_urls);
		getChild<LLUICtrl>("urls_used")->setValue(LLSD(msg_parcel_urls));
	}
}

BOOL LLPanelScriptLimitsRegionMemory::postBuild()
{
	childSetAction("refresh_list_btn", onClickRefresh, this);
	childSetAction("highlight_btn", onClickHighlight, this);
	childSetAction("return_btn", onClickReturn, this);
		
	std::string msg_waiting = LLTrans::getString("ScriptLimitsRequestWaiting");
	getChild<LLUICtrl>("loading_text")->setValue(LLSD(msg_waiting));

	LLScrollListCtrl *list = getChild<LLScrollListCtrl>("scripts_list");
	if(!list)
	{
		return FALSE;
	}
	list->setCommitCallback(boost::bind(&LLPanelScriptLimitsRegionMemory::checkButtonsEnabled, this));
	checkButtonsEnabled();

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
		getChild<LLUICtrl>("loading_text")->setValue(LLSD(std::string("")));
		//might have to do parent post build here
		//if not logic below could use early outs
		return FALSE;
	}
	LLParcel* parcel = instance->getCurrentSelectedParcel();
	LLViewerRegion* region = LLViewerParcelMgr::getInstance()->getSelectionRegion();
	
	if ((region) && (parcel))
	{
		LLUUID current_region_id = gAgent.getRegion()->getRegionID();
		LLVector3 parcel_center = parcel->getCenterpoint();
		
		region_id = region->getRegionID();
		
		if(region_id != current_region_id)
		{
			std::string msg_wrong_region = LLTrans::getString("ScriptLimitsRequestWrongRegion");
			getChild<LLUICtrl>("loading_text")->setValue(LLSD(msg_wrong_region));
			return FALSE;
		}
		
		LLVector3d pos_global = region->getCenterGlobal();
		
		LLSD body;
		std::string url = region->getCapability("RemoteParcelRequest");
		if (!url.empty())
		{
            LLRemoteParcelInfoProcessor::getInstance()->requestRegionParcelInfo(url, 
                region_id, parcel_center, pos_global, getObserverHandle());
		}
		else
		{
			LL_WARNS() << "Can't get parcel info for script information request" << region_id
					<< ". Region: "	<< region->getName()
					<< " does not support RemoteParcelRequest" << LL_ENDL;
					
			std::string msg_waiting = LLTrans::getString("ScriptLimitsRequestError");
			getChild<LLUICtrl>("loading_text")->setValue(LLSD(msg_waiting));
		}
	}
	else
	{
		std::string msg_waiting = LLTrans::getString("ScriptLimitsRequestNoParcelSelected");
		getChild<LLUICtrl>("loading_text")->setValue(LLSD(msg_waiting));
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
	getChild<LLUICtrl>("memory_used")->setValue(LLSD(msg_empty_string));
	getChild<LLUICtrl>("urls_used")->setValue(LLSD(msg_empty_string));
	getChild<LLUICtrl>("parcels_listed")->setValue(LLSD(msg_empty_string));

	mObjectListItems.clear();
	checkButtonsEnabled();
}

void LLPanelScriptLimitsRegionMemory::checkButtonsEnabled()
{
	LLScrollListCtrl* list = getChild<LLScrollListCtrl>("scripts_list");
	getChild<LLButton>("highlight_btn")->setEnabled(list->getNumSelected() > 0);
	getChild<LLButton>("return_btn")->setEnabled(list->getNumSelected() > 0);
}

// static
void LLPanelScriptLimitsRegionMemory::onClickRefresh(void* userdata)
{
	LLFloaterScriptLimits* instance = LLFloaterReg::getTypedInstance<LLFloaterScriptLimits>("script_limits");
	if(instance)
	{
		LLTabContainer* tab = instance->getChild<LLTabContainer>("scriptlimits_panels");
		if(tab)
		{
			LLPanelScriptLimitsRegionMemory* panel_memory = (LLPanelScriptLimitsRegionMemory*)tab->getChild<LLPanel>("script_limits_region_memory_panel");
			if(panel_memory)
			{
				//To stop people from hammering the refesh button and accidentally dosing themselves - enough requests can crash the viewer!
				//turn the button off, then turn it on when we get a response
				LLButton* btn = panel_memory->getChild<LLButton>("refresh_list_btn");
				if(btn)
				{
					btn->setEnabled(false);
				}
				panel_memory->clearList();
		
				panel_memory->StartRequestChain();
			}
		}
		return;
	}
	else
	{
		LL_WARNS() << "could not find LLPanelScriptLimitsRegionMemory instance after refresh button clicked" << LL_ENDL;
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
		LL_WARNS() << "could not find LLPanelScriptLimitsRegionMemory instance after highlight button clicked" << LL_ENDL;
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
		LL_WARNS() << "could not find LLPanelScriptLimitsRegionMemory instance after highlight button clicked" << LL_ENDL;
		return;
	}
}

