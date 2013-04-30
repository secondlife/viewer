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
#include "llbutton.h"
#include "llfolderviewitem.h"
#include "lloutputmonitorctrl.h"
#include "lltextbox.h"

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
	 void drawHighlight();

private:

	// Background images
	LLPointer<LLUIImage> mImageHeader;
	LLPointer<LLUIImage> mImageHeaderOver;
	LLPointer<LLUIImage> mImageHeaderFocused;

};

class LLPersonView : public LLFolderViewItem
{

public:

	struct Params : public LLInitParam::Block<Params, LLFolderViewItem::Params>
	{
		Params();
		Optional<LLAvatarIconCtrl::Params> avatar_icon;
		Optional<LLTextBox::Params> last_interaction_time_textbox;
		Optional<LLIconCtrl::Params> permission_edit_theirs_icon;
		Optional<LLIconCtrl::Params> permission_edit_mine_icon;
		Optional<LLIconCtrl::Params> permission_map_icon;
		Optional<LLIconCtrl::Params> permission_online_icon;
		Optional<LLButton::Params> info_btn;
		Optional<LLButton::Params> profile_btn;
		Optional<LLOutputMonitorCtrl::Params> output_monitor;
	};

	LLPersonView(const LLPersonView::Params& p);
	virtual ~LLPersonView();

	 S32 getLabelXPos();
	 void addToFolder(LLFolderViewFolder * person_folder_view);
	 void initFromParams(const LLPersonView::Params & params);

protected:	
	void draw();
	void drawHighlight();

private:

	LLPointer<LLUIImage> mImageOver;
	LLPointer<LLUIImage> mImageSelected;

	LLAvatarIconCtrl* mAvatarIcon;
	LLTextBox * mLastInteractionTimeTextbox;
	LLIconCtrl * mPermissionEditTheirsIcon;
	LLIconCtrl * mPermissionEditMineIcon;
	LLIconCtrl * mPermissionMapIcon;
	LLIconCtrl * mPermissionOnlineIcon;
	LLButton * mInfoBtn;
	LLButton * mProfileBtn;
	LLOutputMonitorCtrl * mOutputMonitorCtrl;



	typedef enum e_avatar_item_child {
		ALIC_SPEAKER_INDICATOR,
		ALIC_PROFILE_BUTTON,
		ALIC_INFO_BUTTON,
		ALIC_PERMISSION_ONLINE,
		ALIC_PERMISSION_MAP,
		ALIC_PERMISSION_EDIT_MINE,
		ALIC_PERMISSION_EDIT_THEIRS,
		ALIC_INTERACTION_TIME,
		ALIC_COUNT,
	} EAvatarListItemChildIndex;

	static bool	sStaticInitialized;
	static S32 sMouseOverChildrenWidths[ALIC_COUNT];
	static S32 sMouseOverChildren[ALIC_COUNT];
	static void initChildrenWidths(LLPersonView* self);
	void updateChildren();
	//LLView* getItemChildView(EAvatarListItemChildIndex child_view_index);
};

#endif // LL_LLPERSONTABVIEW_H

