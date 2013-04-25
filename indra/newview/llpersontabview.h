/** 
* @file   llpersontabview.h
* @brief  Header file for llpersontabview
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
#ifndef LL_LLPERSONTABVIEW_H
#define LL_LLPERSONTABVIEW_H

#include "llavatariconctrl.h"
#include "llfolderviewitem.h"

class LLPersonTabView : public LLFolderViewFolder
{

public:

	struct Params : public LLInitParam::Block<Params, LLFolderViewFolder::Params>
	{
		Params();
	};

	LLPersonTabView(const LLPersonTabView::Params& p);
	virtual ~LLPersonTabView();

	S32 getLabelXPos();
	bool highlight;

	BOOL handleMouseDown( S32 x, S32 y, MASK mask );

protected:	
	 void draw();

private:
};

class LLPersonView : public LLFolderViewItem
{

public:

	struct Params : public LLInitParam::Block<Params, LLFolderViewItem::Params>
	{
		Params();
	};

	LLPersonView(const LLPersonView::Params& p);
	virtual ~LLPersonView();

	 S32 getLabelXPos();
	 void addToFolder(LLFolderViewFolder * person_folder_view);

protected:	
	void draw();

private:

	LLAvatarIconCtrl* mAvatarIcon;
	LLButton * mInfoBtn;

	typedef enum e_avatar_item_child {
		ALIC_SPEAKER_INDICATOR,
		ALIC_INFO_BUTTON,
		ALIC_COUNT,
	} EAvatarListItemChildIndex;

	static bool	sStaticInitialized; // this variable is introduced to improve code readability
	static S32 sChildrenWidths[ALIC_COUNT];
	//static void initChildrenWidths(LLConversationViewParticipant* self);
	//void updateChildren();
	//LLView* getItemChildView(EAvatarListItemChildIndex child_view_index);
};

#endif // LL_LLPERSONTABVIEW_H

