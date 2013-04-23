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

//
// LLPersonModelCommon
// 

LLPersonModelCommon::LLPersonModelCommon(std::string display_name, LLFolderViewModelInterface& root_view_model) :
LLFolderViewModelItemCommon(root_view_model),
	mName(display_name)
{
}

LLPersonModelCommon::LLPersonModelCommon(LLFolderViewModelInterface& root_view_model) :
LLFolderViewModelItemCommon(root_view_model),
	mName(""),
	mID(LLUUID().generateNewID())
{
}

LLPersonModelCommon::~LLPersonModelCommon()
{

}

void LLPersonModelCommon::postEvent(const std::string& event_type, LLPersonFolderModel* folder, LLPersonModel* person)
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

//
// LLPersonFolderModel
// 

LLPersonFolderModel::LLPersonFolderModel(std::string display_name, LLFolderViewModelInterface& root_view_model) :
LLPersonModelCommon(display_name,root_view_model)
{

}

LLPersonFolderModel::LLPersonFolderModel(LLFolderViewModelInterface& root_view_model) :
LLPersonModelCommon(root_view_model)
{

}

void LLPersonFolderModel::addParticipant(LLPersonModel* participant)
{
	addChild(participant);
	postEvent("add_participant", this, participant);
}

void LLPersonFolderModel::removeParticipant(LLPersonModel* participant)
{
	removeChild(participant);
	postEvent("remove_participant", this, participant);
}

void LLPersonFolderModel::removeParticipant(const LLUUID& participant_id)
{
	LLPersonModel* participant = findParticipant(participant_id);
	if (participant)
	{
		removeParticipant(participant);
	}
}

void LLPersonFolderModel::clearParticipants()
{
	clearChildren();
}

LLPersonModel* LLPersonFolderModel::findParticipant(const LLUUID& person_id)
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
// LLConversationItemParticipant
// 

LLPersonModel::LLPersonModel(std::string display_name, LLFolderViewModelInterface& root_view_model) :
LLPersonModelCommon(display_name,root_view_model)
{
}

LLPersonModel::LLPersonModel(LLFolderViewModelInterface& root_view_model) :
LLPersonModelCommon(root_view_model)
{
}
