/** 
* @file   llpersonfolderview.h
* @brief  Header file for llpersonfolderview
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
#ifndef LL_LLPERSONFOLDERVIEW_H
#define LL_LLPERSONFOLDERVIEW_H

#include "llevents.h"
#include "llfolderview.h"
#include "llpersonmodelcommon.h"

class LLPersonTabView;
class LLPersonView;
class LLPersonModel;

typedef std::map<LLUUID, LLPersonTabModel *> person_folder_model_map;
typedef std::map<LLUUID, LLPersonTabView *> person_folder_view_map;

class LLPersonFolderView : public LLFolderView
{
public:
	struct Params : public LLInitParam::Block<Params, LLFolderView::Params>
	{
		Params()
		{}
	};

	LLPersonFolderView(const Params &p);
	~LLPersonFolderView();

	BOOL handleMouseDown( S32 x, S32 y, MASK mask );
	
	void createPersonTabs();
	void createPersonTab(LLPersonTabModel::tab_type tab_type, const std::string& tab_name);
	bool onConversationModelEvent(const LLSD &event);
	LLPersonView * createConversationViewParticipant(LLPersonModel * item);

	LLPersonTabModel * getPersonTabModelByIndex(LLPersonTabModel::tab_type tab_type);
	LLPersonTabView * getPersonTabViewByIndex(LLPersonTabModel::tab_type tab_type);

	person_folder_model_map mPersonFolderModelMap;
	person_folder_view_map mPersonFolderViewMap;
	std::map<LLPersonTabModel::tab_type, LLUUID> mIndexToFolderMap;
	LLEventStream mConversationsEventStream;
};

#endif // LL_LLPERSONFOLDERVIEW_H

