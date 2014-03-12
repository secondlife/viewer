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


LLPanelExperiences::LLPanelExperiences(  )
	: mExperiencesList(NULL)
{
    buildFromFile("panel_experiences.xml");
}

BOOL LLPanelExperiences::postBuild( void )
{
	mExperiencesList = getChild<LLFlatListView>("experiences_list");
	if(hasString("no_experiences"))
	{
		mExperiencesList->setNoItemsCommentText(getString("no_experiences"));
	}

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
    mExperiencesList->clear();

    LLSD::array_const_iterator it = experiences.beginArray();
    for( /**/ ; it != experiences.endArray(); ++it)
    {
        LLUUID public_key = it->asUUID();
        LLExperienceItem* item = new LLExperienceItem();

        item->init(public_key);
        mExperiencesList->addItem(item, public_key);
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
{
	buildFromFile("panel_experience_list_item.xml");
}

void LLExperienceItem::init( const LLUUID& id)
{
    getChild<LLUICtrl>("experience_name")->setValue(LLSLURL("experience", id, "profile").getSLURLString());
}


LLExperienceItem::~LLExperienceItem()
{

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
