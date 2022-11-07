/** 
 * @file llfloatermyscripts.cpp
 * @brief LLFloaterMyScripts class implementation.
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2019, Linden Research, Inc.
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
#include "llfloatermyscripts.h"

#include "llagent.h"
#include "llcorehttputil.h"
#include "llcoros.h"
#include "lleventcoro.h"
#include "llfloaterreg.h"
#include "llscrolllistctrl.h"
#include "lltrans.h"
#include "llviewerregion.h"

const S32 SIZE_OF_ONE_KB = 1024;

LLFloaterMyScripts::LLFloaterMyScripts(const LLSD& seed)
    : LLFloater(seed), 
    mGotAttachmentMemoryUsed(false),
    mAttachmentDetailsRequested(false),
    mAttachmentMemoryMax(0),
    mAttachmentMemoryUsed(0),
    mGotAttachmentURLsUsed(false),
    mAttachmentURLsMax(0),
    mAttachmentURLsUsed(0)
{
}

BOOL LLFloaterMyScripts::postBuild()
{
    childSetAction("refresh_list_btn", onClickRefresh, this);

    std::string msg_waiting = LLTrans::getString("ScriptLimitsRequestWaiting");
    getChild<LLUICtrl>("loading_text")->setValue(LLSD(msg_waiting));
    mAttachmentDetailsRequested = requestAttachmentDetails();
    return TRUE;
}

// virtual
void LLFloaterMyScripts::onOpen(const LLSD& key)
{
    if (!mAttachmentDetailsRequested)
    {
        mAttachmentDetailsRequested = requestAttachmentDetails();
    }

    LLFloater::onOpen(key);
}

bool LLFloaterMyScripts::requestAttachmentDetails()
{
    if (!gAgent.getRegion()) return false;

    LLSD body;
    std::string url = gAgent.getRegion()->getCapability("AttachmentResources");
    if (!url.empty())
    {
        LLCoros::instance().launch("LLFloaterMyScripts::getAttachmentLimitsCoro",
            boost::bind(&LLFloaterMyScripts::getAttachmentLimitsCoro, this, url));
        return true;
    }
    else
    {
        return false;
    }
}

void LLFloaterMyScripts::getAttachmentLimitsCoro(std::string url)
{
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("getAttachmentLimitsCoro", httpPolicy));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);

    LLSD result = httpAdapter->getAndSuspend(httpRequest, url);

    LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    if (!status)
    {
        LL_WARNS() << "Unable to retrieve attachment limits." << LL_ENDL;
        return;
    }

    LLFloaterMyScripts* instance = LLFloaterReg::getTypedInstance<LLFloaterMyScripts>("my_scripts");

    if (!instance)
    {
        LL_WARNS() << "Failed to get LLFloaterMyScripts instance" << LL_ENDL;
        return;
    }

    instance->getChild<LLUICtrl>("loading_text")->setValue(LLSD(std::string("")));

    LLButton* btn = instance->getChild<LLButton>("refresh_list_btn");
    if (btn)
    {
        btn->setEnabled(true);
    }

    result.erase(LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS);
    instance->setAttachmentDetails(result);
}


void LLFloaterMyScripts::setAttachmentDetails(LLSD content)
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
            element["columns"][0]["halign"] = LLFontGL::RIGHT;

            element["columns"][1]["column"] = "urls";
            element["columns"][1]["value"] = llformat("%d", urls);
            element["columns"][1]["font"] = "SANSSERIF";
            element["columns"][1]["halign"] = LLFontGL::RIGHT;
            
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

    getChild<LLUICtrl>("loading_text")->setValue(LLSD(std::string("")));

    LLButton* btn = getChild<LLButton>("refresh_list_btn");
    if(btn)
    {
        btn->setEnabled(true);
    }
}

void LLFloaterMyScripts::clearList()
{
    LLCtrlListInterface *list = childGetListInterface("scripts_list");

    if (list)
    {
        list->operateOnAll(LLCtrlListInterface::OP_DELETE);
    }

    std::string msg_waiting = LLTrans::getString("ScriptLimitsRequestWaiting");
    getChild<LLUICtrl>("loading_text")->setValue(LLSD(msg_waiting));
}

void LLFloaterMyScripts::setAttachmentSummary(LLSD content)
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
        LL_WARNS() << "attachment details don't contain memory summary info" << LL_ENDL;
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
        LL_WARNS() << "attachment details don't contain urls summary info" << LL_ENDL;
        return;
    }

    if((mAttachmentMemoryUsed >= 0) && (mAttachmentMemoryMax >= 0))
    {
        LLStringUtil::format_map_t args_attachment_memory;
        args_attachment_memory["[COUNT]"] = llformat ("%d", mAttachmentMemoryUsed);
        std::string translate_message = "ScriptLimitsMemoryUsedSimple";

        if (0 < mAttachmentMemoryMax)
        {
            S32 attachment_memory_available = mAttachmentMemoryMax - mAttachmentMemoryUsed;

            args_attachment_memory["[MAX]"] = llformat ("%d", mAttachmentMemoryMax);
            args_attachment_memory["[AVAILABLE]"] = llformat ("%d", attachment_memory_available);
            translate_message = "ScriptLimitsMemoryUsed";
        }

        getChild<LLUICtrl>("memory_used")->setValue(LLTrans::getString(translate_message, args_attachment_memory));
    }

    if((mAttachmentURLsUsed >= 0) && (mAttachmentURLsMax >= 0))
    {
        S32 attachment_urls_available = mAttachmentURLsMax - mAttachmentURLsUsed;

        LLStringUtil::format_map_t args_attachment_urls;
        args_attachment_urls["[COUNT]"] = llformat ("%d", mAttachmentURLsUsed);
        args_attachment_urls["[MAX]"] = llformat ("%d", mAttachmentURLsMax);
        args_attachment_urls["[AVAILABLE]"] = llformat ("%d", attachment_urls_available);
        std::string msg_attachment_urls = LLTrans::getString("ScriptLimitsURLsUsed", args_attachment_urls);
        getChild<LLUICtrl>("urls_used")->setValue(LLSD(msg_attachment_urls));
    }
}

// static
void LLFloaterMyScripts::onClickRefresh(void* userdata)
{
    LLFloaterMyScripts* instance = LLFloaterReg::getTypedInstance<LLFloaterMyScripts>("my_scripts");
    if(instance)
    {
        LLButton* btn = instance->getChild<LLButton>("refresh_list_btn");
        
        //To stop people from hammering the refesh button and accidentally dosing themselves - enough requests can crash the viewer!
        //turn the button off, then turn it on when we get a response
        if(btn)
        {
            btn->setEnabled(false);
        }
        instance->clearList();
        instance->mAttachmentDetailsRequested = instance->requestAttachmentDetails();
    }
    else
    {
        LL_WARNS() << "could not find LLFloaterMyScripts instance after refresh button clicked" << LL_ENDL;
    }
}

