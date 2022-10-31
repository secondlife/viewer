/** 
 * @file llpanelexperiences.cpp
 * @brief LLPanelExperiences class implementation
 *
 * $LicenseInfo:firstyear=2013&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2013, Linden Research, Inc.
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


#include "llpanelprofile.h"
#include "lluictrlfactory.h"
#include "llexperiencecache.h"
#include "llagent.h"

#include "llpanelexperiences.h"
#include "llslurl.h"
#include "lllayoutstack.h"



static LLPanelInjector<LLPanelExperiences> register_experiences_panel("experiences_panel");


//comparators
static const LLExperienceItemComparator NAME_COMPARATOR;

LLPanelExperiences::LLPanelExperiences(  )
    : mExperiencesList(NULL)
{
    buildFromFile("panel_experiences.xml");
}

BOOL LLPanelExperiences::postBuild( void )
{
    mExperiencesList = getChild<LLFlatListView>("experiences_list");
    if (hasString("loading_experiences"))
    {
        mExperiencesList->setNoItemsCommentText(getString("loading_experiences"));
    }
    else if (hasString("no_experiences"))
    {
        mExperiencesList->setNoItemsCommentText(getString("no_experiences"));
    }
    mExperiencesList->setComparator(&NAME_COMPARATOR);

    return TRUE;
}



LLExperienceItem* LLPanelExperiences::getSelectedExperienceItem()
{
    LLPanel* selected_item = mExperiencesList->getSelectedItem();
    if (!selected_item) return NULL;

    return dynamic_cast<LLExperienceItem*>(selected_item);
}

void LLPanelExperiences::setExperienceList( const LLSD& experiences )
{
    if (hasString("no_experiences"))
    {
        mExperiencesList->setNoItemsCommentText(getString("no_experiences"));
    }
    mExperiencesList->clear();

    LLSD::array_const_iterator it = experiences.beginArray();
    for( /**/ ; it != experiences.endArray(); ++it)
    {
        LLUUID public_key = it->asUUID();
        LLExperienceItem* item = new LLExperienceItem();

        item->init(public_key);
        mExperiencesList->addItem(item, public_key);

        const LLSD& experience_details = LLExperienceCache::instance().get(public_key);
        if (experience_details.isUndefined())
        {
            LLExperienceCache::instance().get(public_key, boost::bind(&LLPanelExperiences::sortExperiencesList, this));
        }
    }

    sortExperiencesList();
}

void LLPanelExperiences::sortExperiencesList()
{
    mExperiencesList->sort();
}

void LLPanelExperiences::getExperienceIdsList(std::vector<LLUUID>& result)
{
    std::vector<LLSD> ids;
    mExperiencesList->getValues(ids);
    for (LLSD::array_const_iterator it = ids.begin(); it != ids.end(); ++it)
    {
        result.push_back(it->asUUID());
    }
}

LLPanelExperiences* LLPanelExperiences::create(const std::string& name)
{
    LLPanelExperiences* panel= new LLPanelExperiences();
    panel->setName(name);
    return panel;
}

void LLPanelExperiences::removeExperiences( const LLSD& ids )
{
    LLSD::array_const_iterator it = ids.beginArray();
    for( /**/ ; it != ids.endArray(); ++it)
    {
        removeExperience(it->asUUID());
    }
}

void LLPanelExperiences::removeExperience( const LLUUID& id )
{
    mExperiencesList->removeItemByUUID(id);
}

void LLPanelExperiences::addExperience( const LLUUID& id )
{
    if(!mExperiencesList->getItemByValue(id))
    {
        LLExperienceItem* item = new LLExperienceItem();

        item->init(id);
        mExperiencesList->addItem(item, id);
        mExperiencesList->sort();
    }
}

void LLPanelExperiences::setButtonAction(const std::string& label, const commit_signal_t::slot_type& cb )
{
    if(label.empty())
    {
        getChild<LLLayoutPanel>("button_panel")->setVisible(false);
    }
    else
    {
        getChild<LLLayoutPanel>("button_panel")->setVisible(true);
        LLButton* child = getChild<LLButton>("btn_action");
        child->setCommitCallback(cb);
        child->setLabel(getString(label));
    }
}

void LLPanelExperiences::enableButton( bool enable )
{
    getChild<LLButton>("btn_action")->setEnabled(enable);
}


LLExperienceItem::LLExperienceItem()
    : mName(NULL)
{
    buildFromFile("panel_experience_list_item.xml");
}

void LLExperienceItem::init( const LLUUID& id)
{
    mName = getChild<LLUICtrl>("experience_name");
    mName->setValue(LLSLURL("experience", id, "profile").getSLURLString());
}

LLExperienceItem::~LLExperienceItem()
{

}

std::string LLExperienceItem::getExperienceName() const
{
    if (mName)
    {
        return mName->getValue();
    }
    
    return "";
}

void LLPanelSearchExperiences::doSearch()
{

}

LLPanelSearchExperiences* LLPanelSearchExperiences::create( const std::string& name )
{
    LLPanelSearchExperiences* panel= new LLPanelSearchExperiences();
    panel->getChild<LLPanel>("results")->addChild(LLPanelExperiences::create(name));
    return panel;
}

BOOL LLPanelSearchExperiences::postBuild( void )
{
    childSetAction("search_button", boost::bind(&LLPanelSearchExperiences::doSearch, this));
    return TRUE;
}

bool LLExperienceItemComparator::compare(const LLPanel* item1, const LLPanel* item2) const
{
    const LLExperienceItem* experience_item1 = dynamic_cast<const LLExperienceItem*>(item1);
    const LLExperienceItem* experience_item2 = dynamic_cast<const LLExperienceItem*>(item2);
    
    if (!experience_item1 || !experience_item2)
    {
        LL_ERRS() << "item1 and item2 cannot be null" << LL_ENDL;
        return true;
    }

    std::string name1 = experience_item1->getExperienceName();
    std::string name2 = experience_item2->getExperienceName();

    LLStringUtil::toUpper(name1);
    LLStringUtil::toUpper(name2);

    return name1 < name2;
}
