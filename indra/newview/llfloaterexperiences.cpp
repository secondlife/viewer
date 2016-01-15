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
#include "llhttpclient.h"
#include "llnotificationsutil.h"
#include "llpanelexperiencelog.h"
#include "llpanelexperiencepicker.h"
#include "llpanelexperiences.h"
#include "lltabcontainer.h"
#include "lltrans.h"
#include "llviewerregion.h"


#define SHOW_RECENT_TAB (0)

class LLExperienceListResponder : public LLHTTPClient::Responder
{
public:
    typedef std::map<std::string, std::string> NameMap;
	typedef boost::function<void(LLPanelExperiences*, const LLSD&)> Callback;
	LLExperienceListResponder(const LLHandle<LLFloaterExperiences>& parent, NameMap& nameMap, const std::string& errorMessage="ErrorMessage"):mParent(parent),mErrorMessage(errorMessage)
	{
		mNameMap.swap(nameMap);
	}

	Callback mCallback;
    LLHandle<LLFloaterExperiences> mParent;
    NameMap mNameMap;
	const std::string mErrorMessage;
    /*virtual*/ void httpSuccess()
    {
        if(mParent.isDead())
            return;

        LLFloaterExperiences* parent=mParent.get();
        LLTabContainer* tabs = parent->getChild<LLTabContainer>("xp_tabs");
 
        NameMap::iterator it = mNameMap.begin();
        while(it != mNameMap.end())
        {
            if(getContent().has(it->first))
            {
                LLPanelExperiences* tab = (LLPanelExperiences*)tabs->getPanelByName(it->second);
                if(tab)
                {
                    const LLSD& ids = getContent()[it->first];
                    tab->setExperienceList(ids);
					if(!mCallback.empty())
					{
						mCallback(tab, getContent());
					}
                }
            }
            ++it;
        }
    }

	/*virtual*/ void httpFailure()
	{
		LLSD subs;
		subs["ERROR_MESSAGE"] = getReason();
		LLNotificationsUtil::add(mErrorMessage, subs);
	}
};



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

   
    LLEventPumps::instance().obtain("experience_permission").listen("LLFloaterExperiences", 
        boost::bind(&LLFloaterExperiences::updatePermissions, this, _1));
     
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
        LLExperienceListResponder::NameMap nameMap;
        std::string lookup_url=region->getCapability("GetExperiences"); 
        if(!lookup_url.empty())
        {
            nameMap["experiences"]="Allowed_Experiences_Tab";
            nameMap["blocked"]="Blocked_Experiences_Tab";
            LLHTTPClient::get(lookup_url, new LLExperienceListResponder(getDerivedHandle<LLFloaterExperiences>(), nameMap));
        }

        updateInfo("GetAdminExperiences","Admin_Experiences_Tab");
        updateInfo("GetCreatorExperiences","Contrib_Experiences_Tab");

		lookup_url = region->getCapability("AgentExperiences"); 
		if(!lookup_url.empty())
		{
			nameMap["experience_ids"]="Owned_Experiences_Tab";
			LLExperienceListResponder* responder = new LLExperienceListResponder(getDerivedHandle<LLFloaterExperiences>(), nameMap, "ExperienceAcquireFailed");
			responder->mCallback = boost::bind(&LLFloaterExperiences::checkPurchaseInfo, this, _1, _2);
			LLHTTPClient::get(lookup_url, responder);
		}
    }
}

void LLFloaterExperiences::onOpen( const LLSD& key )
{
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

void LLFloaterExperiences::updateInfo(std::string experiences, std::string tab)
{
	LLViewerRegion* region = gAgent.getRegion();
	if (region)
	{
		LLExperienceListResponder::NameMap nameMap;
		std::string lookup_url = region->getCapability(experiences);
		if(!lookup_url.empty())
		{
			nameMap["experience_ids"]=tab;
			LLHTTPClient::get(lookup_url, new LLExperienceListResponder(getDerivedHandle<LLFloaterExperiences>(), nameMap));
		}
	}
}

void LLFloaterExperiences::sendPurchaseRequest()
{
	LLViewerRegion* region = gAgent.getRegion();
	std::string url = region->getCapability("AgentExperiences");
	if(!url.empty())
	{
		LLSD content;
        const std::string tab_owned_name = "Owned_Experiences_Tab";

        // extract ids for experiences that we already have
        LLTabContainer* tabs = getChild<LLTabContainer>("xp_tabs");
        LLPanelExperiences* tab_owned = (LLPanelExperiences*)tabs->getPanelByName(tab_owned_name);
        mPrepurchaseIds.clear();
        if (tab_owned)
        {
            tab_owned->getExperienceIdsList(mPrepurchaseIds);
        }

		LLExperienceListResponder::NameMap nameMap;
        nameMap["experience_ids"] = tab_owned_name;
		LLExperienceListResponder* responder = new LLExperienceListResponder(getDerivedHandle<LLFloaterExperiences>(), nameMap, "ExperienceAcquireFailed");
        responder->mCallback = boost::bind(&LLFloaterExperiences::checkAndOpen, this, _1, _2);
		LLHTTPClient::post(url, content, responder);
	}
}

LLFloaterExperiences* LLFloaterExperiences::findInstance()
{
	return LLFloaterReg::findTypedInstance<LLFloaterExperiences>("experiences");
}
