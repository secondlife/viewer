#include "llviewerprecompiledheaders.h"

#include "llpanelexperiences.h"
#include "llfloaterexperiences.h"
#include "llagent.h"
#include "llfloaterregioninfo.h"
#include "lltabcontainer.h"
#include "lltrans.h"
#include "llexperiencecache.h"


class LLExperienceListResponder : public LLHTTPClient::Responder
{
public:
    typedef std::map<std::string, std::string> NameMap;
    LLExperienceListResponder(const LLHandle<LLFloaterExperiences>& parent, NameMap& nameMap):mParent(parent)
    {
        mNameMap.swap(nameMap);
    }

    LLHandle<LLFloaterExperiences> mParent;
    NameMap mNameMap;

    virtual void result(const LLSD& content)
    {
        if(mParent.isDead())
            return;

        LLFloaterExperiences* parent=mParent.get();
        LLTabContainer* tabs = parent->getChild<LLTabContainer>("xp_tabs");
 
        NameMap::iterator it = mNameMap.begin();
        while(it != mNameMap.end())
        {
            if(content.has(it->first))
            {
                LLPanelExperiences* tab = (LLPanelExperiences*)tabs->getPanelByName(it->second);
                if(tab)
                {
                    const LLSD& ids = content[it->first];
                    tab->setExperienceList(ids);
                    //parent->clearFromRecent(ids);
                }
            }
            ++it;
        }
    }
};



LLFloaterExperiences::LLFloaterExperiences(const LLSD& data)
	:LLFloater(data)
{
}

void LLFloaterExperiences::addTab(const std::string& name, bool select)
{
    getChild<LLTabContainer>("xp_tabs")->addTabPanel(LLTabContainer::TabPanelParams().
        panel(LLPanelExperiences::create(name)).
        label(LLTrans::getString(name)).
        select_tab(select));
}

BOOL LLFloaterExperiences::postBuild()
{
    addTab("Allowed_Experiences_Tab", true);
    addTab("Blocked_Experiences_Tab", false);
    addTab("Admin_Experiences_Tab", false);
    addTab("Contrib_Experiences_Tab", false);
    addTab("Recent_Experiences_Tab", false);
    resizeToTabs();

    refreshContents();

	return TRUE;
}

void LLFloaterExperiences::clearFromRecent(const LLSD& ids)
{
    LLTabContainer* tabs = getChild<LLTabContainer>("xp_tabs");

    LLPanelExperiences* tab = (LLPanelExperiences*)tabs->getPanelByName("Recent_Experiences_Tab");
    if(!tab)
        return;

    tab->removeExperiences(ids);
}

void LLFloaterExperiences::setupRecentTabs()
{
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

        lookup_url = region->getCapability("GetAdminExperiences"); 
        if(!lookup_url.empty())
        {
            nameMap["experience_ids"]="Admin_Experiences_Tab";
            LLHTTPClient::get(lookup_url, new LLExperienceListResponder(getDerivedHandle<LLFloaterExperiences>(), nameMap));
        }

        lookup_url = region->getCapability("GetCreatorExperiences"); 
        if(!lookup_url.empty())
        {
            nameMap["experience_ids"]="Contrib_Experiences_Tab";
            LLHTTPClient::get(lookup_url, new LLExperienceListResponder(getDerivedHandle<LLFloaterExperiences>(), nameMap));
        }
    }
}
