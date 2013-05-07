/** 
* @file llpersonfolderview.cpp
* @brief Implementation of llpersonfolderview
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

#include "llpersonfolderview.h"

#include "llpersontabview.h"


LLPersonFolderView::LLPersonFolderView(const Params &p) : 
LLFolderView(p),
	mConversationsEventStream("ConversationsEventsTwo")
{
    rename("Persons");  // For tracking!
	mConversationsEventStream.listen("ConversationsRefresh", boost::bind(&LLPersonFolderView::onConversationModelEvent, this, _1));

	createPersonTabs();
}

LLPersonFolderView::~LLPersonFolderView()
{
	mConversationsEventStream.stopListening("ConversationsRefresh");
}

BOOL LLPersonFolderView::handleMouseDown( S32 x, S32 y, MASK mask )
{
	LLFolderViewItem * item = getCurSelectedItem();

	//Will disable highlight on tab
	if(item)
	{
		LLPersonTabView * person_tab= dynamic_cast<LLPersonTabView *>(item);
		if(person_tab)
		{
			person_tab->highlight = false;
		}
		else
		{
			person_tab = dynamic_cast<LLPersonTabView *>(item->getParent());
			person_tab->highlight = false;
		}
	}

	mKeyboardSelection = FALSE; 
	mSearchString.clear();

	LLEditMenuHandler::gEditMenuHandler = this;

	return LLView::handleMouseDown( x, y, mask );
}

void LLPersonFolderView::createPersonTabs()
{
	createPersonTab(LLPersonTabModel::FB_SL_NON_SL_FRIEND, "SL residents you may want to friend");
	createPersonTab(LLPersonTabModel::FB_ONLY_FRIEND, "Invite people you know to SL");
}

void LLPersonFolderView::createPersonTab(LLPersonTabModel::tab_type tab_type, const std::string& tab_name)
{
	//Create a person tab
	LLPersonTabModel* item = new LLPersonTabModel(tab_type, tab_name, *mViewModel);
	LLPersonTabView::Params params;
	params.name = item->getDisplayName();
	params.root = this;
	params.listener = item;
	params.tool_tip = params.name;
	LLPersonTabView * widget = LLUICtrlFactory::create<LLPersonTabView>(params);
	widget->addToFolder(this);

	mIndexToFolderMap[tab_type] = item->getID();
	mPersonFolderModelMap[item->getID()] = item;
	mPersonFolderViewMap[item->getID()] = widget;
}

bool LLPersonFolderView::onConversationModelEvent(const LLSD &event)
{
	std::string type = event.get("type").asString();
	LLUUID folder_id = event.get("folder_id").asUUID();
	LLUUID person_id = event.get("person_id").asUUID();

	if(type == "add_participant")
	{
		LLPersonTabModel * person_tab_model = dynamic_cast<LLPersonTabModel *>(mPersonFolderModelMap[folder_id]);
		LLPersonTabView * person_tab_view = dynamic_cast<LLPersonTabView *>(mPersonFolderViewMap[folder_id]);

		if(person_tab_model)
		{
			LLPersonModel * person_model = person_tab_model->findParticipant(person_id);

			if(person_model)
			{
				LLPersonView * person_view = createConversationViewParticipant(person_model);
				person_view->addToFolder(person_tab_view);
			}
		}
	}

	return false;
}

LLPersonView * LLPersonFolderView::createConversationViewParticipant(LLPersonModel * item)
{
	LLPersonView::Params params;

	params.name = item->getDisplayName();
	params.root = this;
	params.listener = item;

	//24 should be loaded from .xml somehow
	params.rect = LLRect (0, 24, getRect().getWidth(), 0);
	params.tool_tip = params.name;

	return LLUICtrlFactory::create<LLPersonView>(params);
}

LLPersonTabModel * LLPersonFolderView::getPersonTabModelByIndex(LLPersonTabModel::tab_type tab_type)
{
	return mPersonFolderModelMap[mIndexToFolderMap[tab_type]];
}

LLPersonTabView * LLPersonFolderView::getPersonTabViewByIndex(LLPersonTabModel::tab_type tab_type)
{
	return mPersonFolderViewMap[mIndexToFolderMap[tab_type]];
}
