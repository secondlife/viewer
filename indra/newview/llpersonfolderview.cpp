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
#include "llpersonmodelcommon.h"


LLPersonFolderView::LLPersonFolderView(const Params &p) : 
LLFolderView(p),
	mConversationsEventStream("ConversationsEventsTwo")
{
	mConversationsEventStream.listen("ConversationsRefresh", boost::bind(&LLPersonFolderView::onConversationModelEvent, this, _1));
}

LLPersonFolderView::~LLPersonFolderView()
{
	mConversationsEventStream.stopListening("ConversationsRefresh");
}

BOOL LLPersonFolderView::handleMouseDown( S32 x, S32 y, MASK mask )
{
	LLFolderViewItem * prior_item = getCurSelectedItem();
	LLFolderViewItem * current_item;

	bool selected_item = LLFolderView::handleMouseDown(x, y, mask);

	current_item = getCurSelectedItem();
	
	LLPersonTabView * prior_folder = dynamic_cast<LLPersonTabView *>(prior_item);

	if(prior_folder && current_item != prior_folder)
	{
		prior_folder->highlight = false;
	}

	return selected_item;
}

bool LLPersonFolderView::onConversationModelEvent(const LLSD &event)
{
	std::string type = event.get("type").asString();
	LLUUID folder_id = event.get("folder_id").asUUID();
	LLUUID person_id = event.get("person_id").asUUID();

	if(type == "add_participant")
	{
		LLPersonTabModel * person_folder_model = dynamic_cast<LLPersonTabModel *>(mPersonFolderModelMap[folder_id]);
		LLPersonTabView * person_folder_view = dynamic_cast<LLPersonTabView *>(mPersonFolderViewMap[folder_id]);

		if(person_folder_model)
		{
			LLPersonModel * person_model = person_folder_model->findParticipant(person_id);

			if(person_model)
			{
				LLPersonView * participant_view = createConversationViewParticipant(person_model);
				participant_view->addToFolder(person_folder_view);
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
	params.folder_indentation = 2;

	return LLUICtrlFactory::create<LLPersonView>(params);
}
