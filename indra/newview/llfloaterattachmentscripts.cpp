/**
 * @file llfloaterconversationlog.cpp
 * @brief Functionality of the "conversation log" floater
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2012, Linden Research, Inc.
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

#include "llfloaterattachmentscripts.h"
#include "llfloaterreg.h"
#include "llpanel.h"
#include "llaccordionctrltab.h"
#include "llsdutil.h"
#include "llscrollcontainer.h"
#include "llcorehttputil.h"
#include "llviewerregion.h"
#include "boost\smart_ptr\make_shared_object.hpp"
#include "llagent.h"

namespace
{
    const std::string  CAP_AGENTSCRIPTDETAILS("AgentScriptDetails");

//     struct attch_scripts_accordion_tab_params : public LLInitParam::Block<attch_scripts_accordion_tab_params, LLAccordionCtrlTab::Params>
//     {
//         Mandatory<LLPanel::Params> script_panel;
// 
//         attch_scripts_accordion_tab_params():
//             script_panel("attachment_scripts_panel")
//         {}
//     };
// 
//     const attch_scripts_accordion_tab_params& get_accordion_tab_params()
//     {
//         static attch_scripts_accordion_tab_params tab_params;
//         static bool initialized = false;
//         if (!initialized)
//         {
//             initialized = true;
// 
//             LLXMLNodePtr xmlNode;
//             if (LLUICtrlFactory::getLayeredXMLNode("attachment_scripts_acc_tab.xml", xmlNode))
//             {
//                 LLXUIParser parser;
//                 parser.readXUI(xmlNode, tab_params, "attachment_scripts_acc_tab.xml");
//             }
//             else
//             {
//                 LL_WARNS() << "Failed to read xml of Attachments Accordion Tab from attachment_scripts_acc_tab.xml" << LL_ENDL;
//             }
//         }
// 
//         return tab_params;
//     }
// 
//     class LLPanelAttachmentScript : public LLPanel
//     {
//     public:
//                             LLPanelAttachmentScript() : LLPanel() {}
//         virtual             ~LLPanelAttachmentScript() {}
// 
// 
//     protected:
// 
//     };
// 
//     LLPanelInjector<LLPanelAttachmentScript> t_attachment_script_details("atch_scripts_details");

    typedef std::function<void(const LLSD &result, U32 status)>  results_fn;

    void get_attachment_scripts_coro(LLUUID agent_id, results_fn cb);



}

LLFloaterAttachmentScripts::LLFloaterAttachmentScripts(const LLSD& key) :
    LLFloater(key)
//    mAttachments(nullptr)
{
}

BOOL LLFloaterAttachmentScripts::postBuild()
{
    if (!LLFloater::postBuild())
    	return FALSE;

    mScrollList = getChild<LLScrollListCtrl>("scripts_scroll_list");

    return TRUE;
}


void LLFloaterAttachmentScripts::onOpen(const LLSD& key)
{
    refresh();
}


void LLFloaterAttachmentScripts::refresh()
{
    LLCheckedHandle<LLFloaterAttachmentScripts> handle(getDerivedHandle<LLFloaterAttachmentScripts>());

    LLCoros::instance().launch("LLFloaterAttachmentScripts", boost::bind(&get_attachment_scripts_coro, LLUUID(),
        [handle](const LLSD &result, U32 status) 
        {
            try
            {
                handle->handleScriptData(result, status);
            }
            catch (LLCheckedHandleBase::Stale &)
            {   // floater no longer relevant... catch and release.        	    
            }
        }));

//     addAttachmentTab(LLSDMap("name", "My Attachment 1"));
//     addAttachmentTab(LLSDMap("name", "My Attachment 2"));
//     addAttachmentTab(LLSDMap("name", "My Attachment 3"));
}


void LLFloaterAttachmentScripts::handleScriptData(const LLSD &results, U32 status)
{
    if (!mScrollList)
        return;

    mScrollList->deleteAllItems();

    LLUUID agent_id = results["agent_id"].asUUID();
    S32    scripts_running = results["scripts_running"].asInteger();
    S32    scripts_remaining = results["scripts_remaining"].asInteger();
    S32    scripts_total = results["scripts_total"].asInteger();

    LL_WARNS("MILOTIC") << "results returned: agent_id=" << agent_id << " running=" << scripts_running << " remaining=" << scripts_remaining << " total=" << scripts_total << LL_ENDL;

    LLSD    attachments = results["attachments"];
    for (LLSD::array_const_iterator it_atch = attachments.beginArray(); it_atch != attachments.endArray(); ++it_atch)
    {
        LLUUID      item_id = (*it_atch)["item_id"].asUUID();
        std::string item_name = (*it_atch)["name"].asString();
        S32         item_location = (*it_atch)["location"].asInteger();
        bool        item_istemp = (*it_atch)["is_temp"].asBoolean();
        LLUUID      item_exp = (*it_atch)["experience"].asUUID();

        LL_WARNS("MILOTIC") << "results returned: *** id=" << item_id << " name='" << item_name << "' location=" << item_location << " is_temp=" << item_istemp << " experience=" << item_exp << LL_ENDL;

        LLSD scripts = (*it_atch)["scripts"];
        for (LLSD::array_const_iterator it_scpt = scripts.beginArray(); it_scpt != scripts.endArray(); ++it_scpt)
        {
            LLUUID      script_id = (*it_scpt)["script_id"].asUUID();
            std::string script_name = (*it_scpt)["name"].asString();
            bool        script_running = (*it_scpt)["running"].asBoolean();
            bool        script_canrun = (*it_scpt)["can_run"].asBoolean();
            F32         script_exectime = (*it_scpt)["execution_time"].asReal();
            LLUUID      script_experience = (*it_scpt)["experience"].asUUID();
            U32         script_permissions = (*it_scpt)["permissions"].asInteger();
            S32         script_memory = (*it_scpt)["resources"]["memory"].asInteger();
            S32         script_urls = (*it_scpt)["resources"]["urls"].asInteger();
            S32         script_listens = (*it_scpt)["resources"]["listens"].asInteger();

            LL_WARNS("MILOTIC") << "results returned: ******* id=" << script_id << " name='" << script_name <<
                "' running=" << script_running << " canrun=" << script_canrun <<
                " time=" << script_exectime << " exp=" << script_experience <<
                " permissions=" << script_permissions << " memory=" << script_memory << " urls=" << script_urls << " listens=" << script_listens << LL_ENDL;

            LLSD element;
            element["id"] = script_id;
            element["columns"] = LLSDArray(
                LLSDMap("column", "attachment")("value", item_name))
                (LLSDMap("column", "name")("value", script_name))
                (LLSDMap("column", "running")("value", script_running)("type", "checkbox"))
                (LLSDMap("column", "runtime")("value", script_exectime))
                (LLSDMap("column", "experience")("value", script_experience))
                (LLSDMap("column", "permissions")("value", LLSD::Integer(script_permissions)))
                (LLSDMap("column", "memory")("value", script_memory))
                (LLSDMap("column", "urls")("value", script_urls));

            mScrollList->addElement(element);
        }
        
    }

}

void LLFloaterAttachmentScripts::addAttachmentTab(const LLSD &params)
{
//     attch_scripts_accordion_tab_params tab_params(get_accordion_tab_params());
//     LLAccordionCtrlTab* tab = LLUICtrlFactory::create<LLAccordionCtrlTab>(tab_params);
//     if (!tab) 
//         return;
//     LLPanel * script_panel = LLUICtrlFactory::create<LLPanel>(tab_params.script_panel);
//     //script_panel->setShape(tab->getLocalRect());
//     //script_panel->setShape(LLRect(0, 0, 400, 200));
//     tab->addChild(script_panel);
// 
//     tab->setName(params["name"].asString());
//     tab->setTitle(params["name"].asString());
// 
//     // *TODO: LLUICtrlFactory::defaultBuilder does not use "display_children" from xml. Should be investigated.
//     //tab->setDisplayChildren(false);
//     mAttachments->addCollapsibleCtrl(tab);
}

namespace
{
    void get_attachment_scripts_coro(LLUUID agent_id, results_fn cb)
    {
        LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
        LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
            httpAdapter(boost::make_shared<LLCoreHttpUtil::HttpCoroutineAdapter>(CAP_AGENTSCRIPTDETAILS, httpPolicy));
        LLCore::HttpRequest::ptr_t httpRequest(boost::make_shared<LLCore::HttpRequest>());

        
        std::string url(gAgent.getRegionCapability(CAP_AGENTSCRIPTDETAILS));

        if (url.empty())
        {
            cb(LLSD(), 1);
            return;
        }
        if (!agent_id.isNull())
        {
            url += "?agent_id=" + agent_id.asString();
        }

        LLSD result = httpAdapter->getAndSuspend(httpRequest, url);

        LLSD httpResults = result["http_result"];
        LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);
        if (!status)
        {
            cb(result, status.getStatus());
            return;
        }

        // remove the http_result from the llsd
        result.erase("http_result");
        cb(result, 0);

    }

}