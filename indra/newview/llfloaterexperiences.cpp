/** 
 * @file llfloaterexperiences.cpp
 * @brief LLFloaterExperiences class implementation
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
#include "llfloaterexperiences.h"
#include "llfloaterreg.h"

#include "llagent.h"
#include "llevents.h"
#include "llexperiencecache.h"
#include "llfloaterregioninfo.h"
#include "llnotificationsutil.h"
#include "llpanelexperiencelog.h"
#include "llpanelexperiencepicker.h"
#include "llpanelexperiences.h"
#include "lltabcontainer.h"
#include "lltrans.h"
#include "llviewerregion.h"


#define SHOW_RECENT_TAB (0)
LLFloaterExperiences::LLFloaterExperiences(const LLSD& data)
    :LLFloater(data)
{
}

LLPanelExperiences* LLFloaterExperiences::addTab(const std::string& name, bool select)
{
    LLPanelExperiences* newPanel = LLPanelExperiences::create(name);
    getChild<LLTabContainer>("xp_tabs")->addTabPanel(LLTabContainer::TabPanelParams().
        panel(newPanel).
        label(LLTrans::getString(name)).
        select_tab(select));

    return newPanel;
}

BOOL LLFloaterExperiences::postBuild()
{
    getChild<LLTabContainer>("xp_tabs")->addTabPanel(new LLPanelExperiencePicker());
    addTab("Allowed_Experiences_Tab", true);
    addTab("Blocked_Experiences_Tab", false);
    addTab("Admin_Experiences_Tab", false);
    addTab("Contrib_Experiences_Tab", false);
    LLPanelExperiences* owned = addTab("Owned_Experiences_Tab", false);
    owned->setButtonAction("acquire", boost::bind(&LLFloaterExperiences::sendPurchaseRequest, this));
    owned->enableButton(false);
#if SHOW_RECENT_TAB
    addTab("Recent_Experiences_Tab", false);
#endif //SHOW_RECENT_TAB
    getChild<LLTabContainer>("xp_tabs")->addTabPanel(new LLPanelExperienceLog());
    resizeToTabs();

    return TRUE;
}


void LLFloaterExperiences::clearFromRecent(const LLSD& ids)
{
#if SHOW_RECENT_TAB
    LLTabContainer* tabs = getChild<LLTabContainer>("xp_tabs");

    LLPanelExperiences* tab = (LLPanelExperiences*)tabs->getPanelByName("Recent_Experiences_Tab");
    if(!tab)
        return;

    tab->removeExperiences(ids);
#endif // SHOW_RECENT_TAB
}

void LLFloaterExperiences::setupRecentTabs()
{
#if SHOW_RECENT_TAB
    LLTabContainer* tabs = getChild<LLTabContainer>("xp_tabs");

    LLPanelExperiences* tab = (LLPanelExperiences*)tabs->getPanelByName("Recent_Experiences_Tab");
    if(!tab)
        return;

    LLSD recent;

    const LLExperienceCache::cache_t& experiences = LLExperienceCache::getCached();

    LLExperienceCache::cache_t::const_iterator it = experiences.begin();
    while( it != experiences.end() )
    {
        if(!it->second.has(LLExperienceCache::MISSING))
        {
            recent.append(it->first);
        }
        ++it;
    }

    tab->setExperienceList(recent);
#endif // SHOW_RECENT_TAB
}


void LLFloaterExperiences::resizeToTabs()
{
    const S32 TAB_WIDTH_PADDING = 16;

    LLTabContainer* tabs = getChild<LLTabContainer>("xp_tabs");
    LLRect rect = getRect();
    if(rect.getWidth() < tabs->getTotalTabWidth() + TAB_WIDTH_PADDING)
    {
        rect.mRight = rect.mLeft + tabs->getTotalTabWidth() + TAB_WIDTH_PADDING;
    }
    reshape(rect.getWidth(), rect.getHeight(), FALSE);
}

void LLFloaterExperiences::refreshContents()
{
    setupRecentTabs();

    LLViewerRegion* region = gAgent.getRegion();

    if (region)
    {
        NameMap_t tabMap;
        LLHandle<LLFloaterExperiences> handle = getDerivedHandle<LLFloaterExperiences>();

        tabMap["experiences"]="Allowed_Experiences_Tab";
        tabMap["blocked"]="Blocked_Experiences_Tab";
        tabMap["experience_ids"]="Owned_Experiences_Tab";

        retrieveExperienceList(region->getCapability("GetExperiences"), handle, tabMap);

        updateInfo("GetAdminExperiences","Admin_Experiences_Tab");
        updateInfo("GetCreatorExperiences","Contrib_Experiences_Tab");

        retrieveExperienceList(region->getCapability("AgentExperiences"), handle, tabMap, 
            "ExperienceAcquireFailed", boost::bind(&LLFloaterExperiences::checkPurchaseInfo, this, _1, _2));
    }
}

void LLFloaterExperiences::onOpen( const LLSD& key )
{
    LLEventPumps::instance().obtain("experience_permission").stopListening("LLFloaterExperiences");
    LLEventPumps::instance().obtain("experience_permission").listen("LLFloaterExperiences",
        boost::bind(&LLFloaterExperiences::updatePermissions, this, _1));

    LLViewerRegion* region = gAgent.getRegion();
    if(region)
    {
        if(region->capabilitiesReceived())
        {
            refreshContents();
            return;
        }
        region->setCapabilitiesReceivedCallback(boost::bind(&LLFloaterExperiences::refreshContents, this));
        return;
    }
}

bool LLFloaterExperiences::updatePermissions( const LLSD& permission )
{
    LLTabContainer* tabs = getChild<LLTabContainer>("xp_tabs");
    LLUUID experience;
    std::string permission_string;
    if(permission.has("experience"))
    {
        experience = permission["experience"].asUUID();
        permission_string = permission[experience.asString()]["permission"].asString();

    }
    LLPanelExperiences* tab = (LLPanelExperiences*)tabs->getPanelByName("Allowed_Experiences_Tab");
    if(tab)
    {
        if(permission.has("experiences"))
        {
            tab->setExperienceList(permission["experiences"]);
        }
        else if(experience.notNull())
        {
            if(permission_string != "Allow")
            {
                tab->removeExperience(experience);
            }
            else
            {
                tab->addExperience(experience);
            }
        }
    }
    
    tab = (LLPanelExperiences*)tabs->getPanelByName("Blocked_Experiences_Tab");
    if(tab)
    {
        if(permission.has("blocked"))
        {
            tab->setExperienceList(permission["blocked"]);
        }
        else if(experience.notNull())
        {
            if(permission_string != "Block")
            {
                tab->removeExperience(experience);
            }
            else
            {
                tab->addExperience(experience);
            }
        }
    }
    return false;
}

void LLFloaterExperiences::onClose( bool app_quitting )
{
    LLEventPumps::instance().obtain("experience_permission").stopListening("LLFloaterExperiences");
    LLFloater::onClose(app_quitting);
}

void LLFloaterExperiences::checkPurchaseInfo(LLPanelExperiences* panel, const LLSD& content) const
{
    panel->enableButton(content.has("purchase"));

    LLFloaterExperiences::findInstance()->updateInfo("GetAdminExperiences","Admin_Experiences_Tab");
    LLFloaterExperiences::findInstance()->updateInfo("GetCreatorExperiences","Contrib_Experiences_Tab");
}

void LLFloaterExperiences::checkAndOpen(LLPanelExperiences* panel, const LLSD& content) const
{
    checkPurchaseInfo(panel, content);

    // determine new item
    const LLSD& response_ids = content["experience_ids"];

    if (mPrepurchaseIds.size() + 1 == response_ids.size())
    {
        // we have a new element
        for (LLSD::array_const_iterator it = response_ids.beginArray(); it != response_ids.endArray(); ++it)
        {
            LLUUID experience_id = it->asUUID();
            if (std::find(mPrepurchaseIds.begin(), mPrepurchaseIds.end(), experience_id) == mPrepurchaseIds.end())
            {
                // new element found, open it
                LLSD args;
                args["experience_id"] = experience_id;
                args["edit_experience"] = true;
                LLFloaterReg::showInstance("experience_profile", args, true);
                break;
            }
        }
    }
}

void LLFloaterExperiences::updateInfo(std::string experienceCap, std::string tab)
{
    LLViewerRegion* region = gAgent.getRegion();
    if (region)
    {
        NameMap_t tabMap;
        LLHandle<LLFloaterExperiences> handle = getDerivedHandle<LLFloaterExperiences>();

        tabMap["experience_ids"] = tab;

        retrieveExperienceList(region->getCapability(experienceCap), handle, tabMap);
    }
}

void LLFloaterExperiences::sendPurchaseRequest() 
{
    LLViewerRegion* region = gAgent.getRegion();

    if (region)
    {
        NameMap_t tabMap;
        const std::string tab_owned_name = "Owned_Experiences_Tab";
        LLHandle<LLFloaterExperiences> handle = getDerivedHandle<LLFloaterExperiences>();

        tabMap["experience_ids"] = tab_owned_name;

        // extract ids for experiences that we already have
        LLTabContainer* tabs = getChild<LLTabContainer>("xp_tabs");
        LLPanelExperiences* tab_owned = (LLPanelExperiences*)tabs->getPanelByName(tab_owned_name);
        mPrepurchaseIds.clear();
        if (tab_owned)
        {
            tab_owned->getExperienceIdsList(mPrepurchaseIds);
        }

        requestNewExperience(region->getCapability("AgentExperiences"), handle, tabMap, "ExperienceAcquireFailed",
            boost::bind(&LLFloaterExperiences::checkAndOpen, this, _1, _2));
    }
}

LLFloaterExperiences* LLFloaterExperiences::findInstance()
{
    return LLFloaterReg::findTypedInstance<LLFloaterExperiences>("experiences");
}


void LLFloaterExperiences::retrieveExperienceList(const std::string &url,
    const LLHandle<LLFloaterExperiences> &hparent, const NameMap_t &tabMapping,
    const std::string &errorNotify, Callback_t cback)

{
    invokationFn_t getFn = boost::bind(
        // Humans ignore next line.  It is just a cast to specify which LLCoreHttpUtil::HttpCoroutineAdapter routine overload.
        static_cast<LLSD(LLCoreHttpUtil::HttpCoroutineAdapter::*)(LLCore::HttpRequest::ptr_t, const std::string &, LLCore::HttpOptions::ptr_t, LLCore::HttpHeaders::ptr_t)>
        //----
        // _1 -> httpAdapter
        // _2 -> httpRequest
        // _3 -> url
        // _4 -> httpOptions
        // _5 -> httpHeaders
        (&LLCoreHttpUtil::HttpCoroutineAdapter::getAndSuspend), _1, _2, _3, _4, _5);

    LLCoros::instance().launch("LLFloaterExperiences::retrieveExperienceList",
        boost::bind(&LLFloaterExperiences::retrieveExperienceListCoro,
        url, hparent, tabMapping, errorNotify, cback, getFn));

}

void LLFloaterExperiences::requestNewExperience(const std::string &url,
    const LLHandle<LLFloaterExperiences> &hparent, const NameMap_t &tabMapping,
    const std::string &errorNotify, Callback_t cback)
{
    invokationFn_t postFn = boost::bind(
        // Humans ignore next line.  It is just a cast to specify which LLCoreHttpUtil::HttpCoroutineAdapter routine overload.
        static_cast<LLSD(LLCoreHttpUtil::HttpCoroutineAdapter::*)(LLCore::HttpRequest::ptr_t, const std::string &, const LLSD &, LLCore::HttpOptions::ptr_t, LLCore::HttpHeaders::ptr_t)>
        //----
        // _1 -> httpAdapter
        // _2 -> httpRequest
        // _3 -> url
        // _4 -> httpOptions
        // _5 -> httpHeaders
        (&LLCoreHttpUtil::HttpCoroutineAdapter::postAndSuspend), _1, _2, _3, LLSD(), _4, _5);

    LLCoros::instance().launch("LLFloaterExperiences::requestNewExperience",
        boost::bind(&LLFloaterExperiences::retrieveExperienceListCoro,
        url, hparent, tabMapping, errorNotify, cback, postFn));

}


void LLFloaterExperiences::retrieveExperienceListCoro(std::string url, 
    LLHandle<LLFloaterExperiences> hparent, NameMap_t tabMapping, 
    std::string errorNotify, Callback_t cback, invokationFn_t invoker)
{
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("retrieveExperienceListCoro", httpPolicy));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);
    LLCore::HttpOptions::ptr_t httpOptions(new LLCore::HttpOptions);
    LLCore::HttpHeaders::ptr_t httpHeaders(new LLCore::HttpHeaders);


    if (url.empty())
    {
        LL_WARNS() << "retrieveExperienceListCoro called with empty capability!" << LL_ENDL;
        return;
    }

    LLSD result = invoker(httpAdapter, httpRequest, url, httpOptions, httpHeaders);

    LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    if (!status)
    {
        LLSD subs;
        subs["ERROR_MESSAGE"] = status.getType();
        LLNotificationsUtil::add(errorNotify, subs);

        return;
    }

    if (hparent.isDead())
        return;

    LLFloaterExperiences* parent = hparent.get();
    LLTabContainer* tabs = parent->getChild<LLTabContainer>("xp_tabs");

    for (NameMap_t::iterator it = tabMapping.begin(); it != tabMapping.end(); ++it)
    {
        if (result.has(it->first))
        {
            LLPanelExperiences* tab = (LLPanelExperiences*)tabs->getPanelByName(it->second);
            if (tab)
            {
                const LLSD& ids = result[it->first];
                tab->setExperienceList(ids);
                if (!cback.empty())
                {
                    cback(tab, result);
                }
            }
        }
    }

}
