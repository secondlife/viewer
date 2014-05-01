/**
 * @file llpanelexperiencelisteditor.cpp
 * @brief Editor for building a list of experiences
 *
 * $LicenseInfo:firstyear=2014&license=viewerlgpl$
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

#include "llpanelexperiencelisteditor.h"

#include "llbutton.h"
#include "llexperiencecache.h"
#include "llfloaterexperiencepicker.h"
#include "llfloaterreg.h"
#include "llhandle.h"
#include "llscrolllistctrl.h"
#include "llviewerregion.h"
#include "llagent.h"
#include "lltextbox.h"
#include "lltrans.h"


static LLPanelInjector<LLPanelExperienceListEditor> t_panel_experience_list_editor("panel_experience_list_editor");


LLPanelExperienceListEditor::LLPanelExperienceListEditor()
	:mItems(NULL)
	,mProfile(NULL)
	,mRemove(NULL)
	,mReadonly(false)
{
}

BOOL LLPanelExperienceListEditor::postBuild()
{
	mItems = getChild<LLScrollListCtrl>("experience_list");
	mAdd = getChild<LLButton>("btn_add");
	mRemove = getChild<LLButton>("btn_remove");
	mProfile = getChild<LLButton>("btn_profile");

	childSetAction("btn_add", boost::bind(&LLPanelExperienceListEditor::onAdd, this));
	childSetAction("btn_remove", boost::bind(&LLPanelExperienceListEditor::onRemove, this));
	childSetAction("btn_profile", boost::bind(&LLPanelExperienceListEditor::onProfile, this));

	mItems->setCommitCallback(boost::bind(&LLPanelExperienceListEditor::checkButtonsEnabled, this));
	
	checkButtonsEnabled();
	return TRUE;
}

const uuid_list_t& LLPanelExperienceListEditor::getExperienceIds() const
{
	return mExperienceIds;
}

void LLPanelExperienceListEditor::addExperienceIds( const uuid_vec_t& experience_ids )
{
	mExperienceIds.insert(experience_ids.begin(), experience_ids.end());
	onItems();
	if(!mAddedCallback.empty())
	{
		for(uuid_vec_t::const_iterator it = experience_ids.begin(); it != experience_ids.end(); ++it)
		{
			mAddedCallback(*it);
		}
	}
}


void LLPanelExperienceListEditor::setExperienceIds( const LLSD& experience_ids )
{
	mExperienceIds.clear();
	mExperienceIds.insert(experience_ids.beginArray(), experience_ids.endArray());
	onItems();
}

void LLPanelExperienceListEditor::addExperience( const LLUUID& id )
{
	mExperienceIds.insert(id);
	onItems();
}
void LLPanelExperienceListEditor::onAdd()
{
	if(!mPicker.isDead())
	{
		mPicker.get()->setFrontmost(TRUE);
	}
	else
	{
		mKey.generateNewID();

		LLFloaterExperiencePicker* picker=LLFloaterExperiencePicker::show(boost::bind(&LLPanelExperienceListEditor::addExperienceIds, this, _1), mKey, FALSE, TRUE, mFilters, mAdd);
		mPicker = picker->getDerivedHandle<LLFloaterExperiencePicker>();
	}
}


void LLPanelExperienceListEditor::onRemove()
{
	std::vector<LLScrollListItem*> items= mItems->getAllSelected();
	std::vector<LLScrollListItem*>::iterator it = items.begin();
	for(/**/; it != items.end(); ++it)
	{
		if((*it) != NULL)
		{
			mExperienceIds.erase((*it)->getValue());
			mRemovedCallback((*it)->getValue());
		}
	}
	onItems();
}

void LLPanelExperienceListEditor::onProfile()
{
	LLScrollListItem* item = mItems->getFirstSelected();
	if(item)
	{
		LLFloaterReg::showInstance("experience_profile", item->getUUID(), true);
	}
}

void LLPanelExperienceListEditor::checkButtonsEnabled()
{
	mAdd->setEnabled(!mReadonly);
	int selected = mItems->getNumSelected();
	mRemove->setEnabled(!mReadonly && selected>0);
	mProfile->setEnabled(selected==1);
}

void LLPanelExperienceListEditor::onItems()
{
	mItems->deleteAllItems();

	LLSD item;
	uuid_list_t::iterator it = mExperienceIds.begin();
	for(/**/; it != mExperienceIds.end(); ++it)
	{
		const LLUUID& experience = *it;
		item["id"]=experience;
		LLSD& columns = item["columns"];
		columns[0]["column"] = "experience_name";
		columns[0]["value"] = getString("loading");
		mItems->addElement(item);

		LLExperienceCache::get(experience, boost::bind(&LLPanelExperienceListEditor::experienceDetailsCallback, 
			getDerivedHandle<LLPanelExperienceListEditor>(), _1));
	}


	if(mItems->getItemCount() == 0)
	{
		mItems->setCommentText(getString("no_results"));
	}


	checkButtonsEnabled();
}

void LLPanelExperienceListEditor::experienceDetailsCallback( LLHandle<LLPanelExperienceListEditor> panel, const LLSD& experience )
{
	if(!panel.isDead())
	{
		panel.get()->onExperienceDetails(experience);
	}
}

void LLPanelExperienceListEditor::onExperienceDetails( const LLSD& experience )
{
	LLScrollListItem* item = mItems->getItem(experience[LLExperienceCache::EXPERIENCE_ID]);
	if(!item)
		return;

	item->getColumn(0)->setValue(experience[LLExperienceCache::NAME]);
}

LLPanelExperienceListEditor::~LLPanelExperienceListEditor()
{
	if(!mPicker.isDead())
	{
		mPicker.get()->closeFloater();
	}
}

void LLPanelExperienceListEditor::loading()
{
	mItems->clear();
	mItems->setCommentText( getString("loading"));
}

void LLPanelExperienceListEditor::setReadonly( bool val )
{
	mReadonly = val;
	checkButtonsEnabled();
}

void LLPanelExperienceListEditor::refreshExperienceCounter(std::string string_name)
{
	LLStringUtil::format_map_t args;
	args["[EXPERIENCES]"] = llformat("%d", mItems->getItemCount());
	args["[MAXEXPERIENCES]"] = llformat("%d", ESTATE_MAX_EXPERIENCE_IDS);
	getChild<LLTextBox>("text_name")->setText(LLTrans::getString(string_name, args));
}

boost::signals2::connection LLPanelExperienceListEditor::setAddedCallback( list_changed_signal_t::slot_type cb )
{
	return mAddedCallback.connect(cb);
}

boost::signals2::connection LLPanelExperienceListEditor::setRemovedCallback( list_changed_signal_t::slot_type cb )
{
	return mRemovedCallback.connect(cb);
}
