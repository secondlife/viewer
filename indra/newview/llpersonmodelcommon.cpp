/** 
* @file llavatarfolder.cpp
* @brief Implementation of llavatarfolder
* @author Gilbert@lindenlab.com
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

#include "llpersonmodelcommon.h"

#include "llevents.h"
#include "llsdutil.h"
#include "llstring.h"

//
// LLPersonModelCommon
// 

LLPersonModelCommon::LLPersonModelCommon(std::string display_name, LLFolderViewModelInterface& root_view_model) :
    LLFolderViewModelItemCommon(root_view_model),
	mID(LLUUID().generateNewID())
{
    renameItem(display_name);
}

LLPersonModelCommon::LLPersonModelCommon(LLFolderViewModelInterface& root_view_model) :
    LLFolderViewModelItemCommon(root_view_model),
	mName(""),
    mSearchableName(""),
	mID(LLUUID().generateNewID())
{
}

LLPersonModelCommon::~LLPersonModelCommon()
{

}

BOOL LLPersonModelCommon::renameItem(const std::string& new_name)
{
    mName = new_name;
    mSearchableName = new_name;
    LLStringUtil::toUpper(mSearchableName);
    return TRUE;
}

void LLPersonModelCommon::postEvent(const std::string& event_type, LLPersonTabModel* folder, LLPersonModel* person)
{
	LLUUID folder_id = folder->getID();
	LLUUID person_id = person->getID();
	LLSD event(LLSDMap("type", event_type)("folder_id", folder_id)("person_id", person_id));
	LLEventPumps::instance().obtain("ConversationsEventsTwo").post(event);
}

// Virtual action callbacks
void LLPersonModelCommon::performAction(LLInventoryModel* model, std::string action)
{
}

void LLPersonModelCommon::openItem( void )
{
}

void LLPersonModelCommon::closeItem( void )
{
}

void LLPersonModelCommon::previewItem( void )
{
}

void LLPersonModelCommon::showProperties(void)
{
}

bool LLPersonModelCommon::filter( LLFolderViewFilter& filter)
{
    // See LLFolderViewModelItemInventory::filter()
/*
    if (!filter.isModified())
    {
        llinfos << "Merov : LLPersonModelCommon::filter, exit, no modif" << llendl;
        return true;
    }
*/        
    if (!mChildren.empty())
    {
        //llinfos << "Merov : LLPersonModelCommon::filter, filtering folder = " << getDisplayName() << llendl;
        setPassedFilter(1, -1, filter.getStringMatchOffset(this), filter.getFilterStringSize());
        for (child_list_t::iterator iter = mChildren.begin(), end_iter = mChildren.end();
            iter != end_iter;
            ++iter)
        {
            // LLFolderViewModelItem
            LLPersonModelCommon* item = dynamic_cast<LLPersonModelCommon*>(*iter);
            item->filter(filter);
        }
    }
    else
    {
        const bool passed_filter = filter.check(this);
        setPassedFilter(passed_filter, -1, filter.getStringMatchOffset(this), filter.getFilterStringSize());
    }
    
    filter.clearModified();
    return true;
}

//
// LLPersonTabModel
// 

LLPersonTabModel::LLPersonTabModel(std::string display_name, LLFolderViewModelInterface& root_view_model) :
LLPersonModelCommon(display_name,root_view_model)
{

}

LLPersonTabModel::LLPersonTabModel(LLFolderViewModelInterface& root_view_model) :
LLPersonModelCommon(root_view_model)
{

}

void LLPersonTabModel::addParticipant(LLPersonModel* participant)
{
	addChild(participant);
	postEvent("add_participant", this, participant);
}

void LLPersonTabModel::removeParticipant(LLPersonModel* participant)
{
	removeChild(participant);
	postEvent("remove_participant", this, participant);
}

void LLPersonTabModel::removeParticipant(const LLUUID& participant_id)
{
	LLPersonModel* participant = findParticipant(participant_id);
	if (participant)
	{
		removeParticipant(participant);
	}
}

void LLPersonTabModel::clearParticipants()
{
	clearChildren();
}

LLPersonModel* LLPersonTabModel::findParticipant(const LLUUID& person_id)
{
	LLPersonModel * person_model = NULL;
	child_list_t::iterator iter;

	for(iter = mChildren.begin(); iter != mChildren.end(); ++iter)
	{
		person_model = static_cast<LLPersonModel *>(*iter);

		if(person_model->getID() == person_id)
		{
			break;
		}
	}

	return iter == mChildren.end() ? NULL : person_model;
}

//
// LLPersonModel
// 

LLPersonModel::LLPersonModel(const LLUUID& agent_id, const std::string display_name, LLFolderViewModelInterface& root_view_model) :
LLPersonModelCommon(display_name,root_view_model),
mAgentID(agent_id)
{
}

LLPersonModel::LLPersonModel(LLFolderViewModelInterface& root_view_model) :
LLPersonModelCommon(root_view_model),
mAgentID(LLUUID(NULL))
{
}

LLUUID LLPersonModel::getAgentID()
{
	return mAgentID;
}

//
// LLPersonViewFilter
//

LLPersonViewFilter::LLPersonViewFilter() :
    mEmptyLookupMessage(""),
    mFilterSubString(""),
    mName(""),
    mFilterModified(FILTER_NONE)
{
}

void LLPersonViewFilter::setFilterSubString(const std::string& string)
{
	std::string filter_sub_string_new = string;
	LLStringUtil::trimHead(filter_sub_string_new);
	LLStringUtil::toUpper(filter_sub_string_new);
    
	if (mFilterSubString != filter_sub_string_new)
	{
        // *TODO : Add logic to support more and less restrictive filtering
        setModified(FILTER_RESTART);
		mFilterSubString = filter_sub_string_new;
	}
}

bool LLPersonViewFilter::showAllResults() const
{
	return mFilterSubString.size() > 0;
}

bool LLPersonViewFilter::check(const LLFolderViewModelItem* item)
{
	return (mFilterSubString.size() ? (item->getSearchableName().find(mFilterSubString) != std::string::npos) : true);
}

std::string::size_type LLPersonViewFilter::getStringMatchOffset(LLFolderViewModelItem* item) const
{
	return mFilterSubString.size() ? item->getSearchableName().find(mFilterSubString) : std::string::npos;
}

std::string::size_type LLPersonViewFilter::getFilterStringSize() const
{
	return mFilterSubString.size();
}

bool LLPersonViewFilter::isActive() const
{
	return mFilterSubString.size();
}

bool LLPersonViewFilter::isModified() const
{
	return mFilterModified != FILTER_NONE;
}

void LLPersonViewFilter::clearModified()
{
    mFilterModified = FILTER_NONE;
}

void LLPersonViewFilter::setEmptyLookupMessage(const std::string& message)
{
	mEmptyLookupMessage = message;
}

std::string LLPersonViewFilter::getEmptyLookupMessage() const
{
	return mEmptyLookupMessage;
}

