/** 
* @file llpersontabview.cpp
* @brief Implementation of llpersontabview
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

#include "llpersontabview.h"

static LLDefaultChildRegistry::Register<LLPersonTabView> r_person_tab_view("person_tab_view");

const LLColor4U DEFAULT_WHITE(255, 255, 255);

LLPersonTabView::Params::Params()
{}

LLPersonTabView::LLPersonTabView(const LLPersonTabView::Params& p) :
LLFolderViewFolder(p),
highlight(false),
mImageHeader(LLUI::getUIImage("Accordion_Off")),
mImageHeaderOver(LLUI::getUIImage("Accordion_Over")),
mImageHeaderFocused(LLUI::getUIImage("Accordion_Selected"))
{
}

S32 LLPersonTabView::getLabelXPos()
{ 
	return getIndentation() + mArrowSize + 15;//Should be a .xml variable but causes crash;
}

LLPersonTabView::~LLPersonTabView()
{

}

BOOL LLPersonTabView::handleMouseDown( S32 x, S32 y, MASK mask )
{
	bool selected_item = LLFolderViewFolder::handleMouseDown(x, y, mask);

	if(selected_item)
	{
		highlight = true;
	}

	return selected_item;
}

void LLPersonTabView::draw()
{
	static LLUIColor sFgColor = LLUIColorTable::instance().getColor("MenuItemEnabledColor", DEFAULT_WHITE);
	static const LLFolderViewItem::Params& default_params = LLUICtrlFactory::getDefaultParams<LLPersonTabView>();

	const LLFontGL * font = LLFontGL::getFontSansSerif();
	F32 text_left = (F32)getLabelXPos();
	F32 y = (F32)getRect().getHeight() - font->getLineHeight() - (F32)mTextPad;
	LLColor4 color = sFgColor;
	F32 right_x  = 0;

	drawHighlight();
	updateLabelRotation();
	drawOpenFolderArrow(default_params, sFgColor);

	drawLabel(font, text_left, y, color, right_x);

	LLView::draw();
}

void LLPersonTabView::drawHighlight()
{
	S32 width = getRect().getWidth();
	S32 height = mItemHeight;
	S32 x = 1;
	S32 y = getRect().getHeight() - mItemHeight;

	if(highlight)
	{
		mImageHeaderFocused->draw(x,y,width,height);
	}
	else
	{
		mImageHeader->draw(x,y,width,height);
	}

	if(mIsMouseOverTitle)
	{
		mImageHeaderOver->draw(x,y,width,height);
	}

}

//
// LLPersonView
// 

static LLDefaultChildRegistry::Register<LLPersonView> r_person_view("person_view");

LLPersonView::Params::Params() :
avatar_icon("avatar_icon"),
last_interaction_time_textbox("last_interaction_time_textbox"),
permission_edit_theirs_icon("permission_edit_theirs_icon"),
permission_edit_mine_icon("permission_edit_mine_icon"),
permission_map_icon("permission_map_icon"),
permission_online_icon("permission_online_icon"),
info_btn("info_btn"),
profile_btn("profile_btn"),
output_monitor("output_monitor")
{}

LLPersonView::LLPersonView(const LLPersonView::Params& p) :
LLFolderViewItem(p),
mImageOver(LLUI::getUIImage("ListItem_Over")),
mImageSelected(LLUI::getUIImage("ListItem_Select")),
mAvatarIcon(NULL),
mLastInteractionTimeTextbox(NULL),
mPermissionEditTheirsIcon(NULL),
mPermissionEditMineIcon(NULL),
mPermissionMapIcon(NULL),
mPermissionOnlineIcon(NULL),
mInfoBtn(NULL),
mProfileBtn(NULL),
mOutputMonitorCtrl(NULL)
{
	initChildrenWidths(this);
}

S32 LLPersonView::getLabelXPos()
{
	return getIndentation() + mAvatarIcon->getRect().getWidth() + mIconPad;
}

void LLPersonView::addToFolder(LLFolderViewFolder * person_folder_view)
{
	LLFolderViewItem::addToFolder(person_folder_view);
	//Added item to folder, could change folder's mHasVisibleChildren flag so call arrange
	person_folder_view->requestArrange();
}

LLPersonView::~LLPersonView()
{

}

void LLPersonView::draw()
{
	static LLUIColor sFgColor = LLUIColorTable::instance().getColor("MenuItemEnabledColor", DEFAULT_WHITE);
	static LLUIColor sHighlightFgColor = LLUIColorTable::instance().getColor("MenuItemHighlightFgColor", DEFAULT_WHITE);

	const LLFontGL * font = LLFontGL::getFontSansSerifSmall();
	F32 text_left = (F32)getLabelXPos();
	F32 y = (F32)getRect().getHeight() - font->getLineHeight() - (F32)mTextPad;
	LLColor4 color = mIsSelected ? sHighlightFgColor : sFgColor;
	F32 right_x  = 0;

	drawHighlight();
	drawLabel(font, text_left, y, color, right_x);

	LLView::draw();
}

void LLPersonView::drawHighlight()
{
	static LLUIColor outline_color = LLUIColorTable::instance().getColor("EmphasisColor", DEFAULT_WHITE);

	S32 width = getRect().getWidth();
	S32 height = mItemHeight;
	S32 x = 1;
	S32 y = 0;

	if(mIsSelected)
	{
		mImageSelected->draw(x, y, width, height);
		//Draw outline
		gl_rect_2d(x, 
			height, 
			width,
			y,
			outline_color, FALSE);
	}

	if(mIsMouseOverTitle)
	{
		mImageOver->draw(x, y, width, height);
	}
}

void LLPersonView::initFromParams(const LLPersonView::Params & params)
{
	LLAvatarIconCtrl::Params avatar_icon_params(params.avatar_icon());
	applyXUILayout(avatar_icon_params, this);
	mAvatarIcon = LLUICtrlFactory::create<LLAvatarIconCtrl>(avatar_icon_params);
	addChild(mAvatarIcon);
	
	LLTextBox::Params last_interaction_time_textbox(params.last_interaction_time_textbox());
	applyXUILayout(last_interaction_time_textbox, this);
	mLastInteractionTimeTextbox = LLUICtrlFactory::create<LLTextBox>(last_interaction_time_textbox);
	addChild(mLastInteractionTimeTextbox);

	LLIconCtrl::Params permission_edit_theirs_icon(params.permission_edit_theirs_icon());
	applyXUILayout(permission_edit_theirs_icon, this);
	mPermissionEditTheirsIcon = LLUICtrlFactory::create<LLIconCtrl>(permission_edit_theirs_icon);
	addChild(mPermissionEditTheirsIcon);

	LLIconCtrl::Params permission_map_icon(params.permission_map_icon());
	applyXUILayout(permission_map_icon, this);
	mPermissionMapIcon = LLUICtrlFactory::create<LLIconCtrl>(permission_map_icon);
	addChild(mPermissionMapIcon);

	LLIconCtrl::Params permission_online_icon(params.permission_online_icon());
	applyXUILayout(permission_online_icon, this);
	mPermissionOnlineIcon = LLUICtrlFactory::create<LLIconCtrl>(permission_online_icon);
	addChild(mPermissionOnlineIcon);

	LLButton::Params info_btn(params.info_btn());
	applyXUILayout(info_btn, this);
	mInfoBtn = LLUICtrlFactory::create<LLButton>(info_btn);
	addChild(mInfoBtn);

	LLButton::Params profile_btn(params.profile_btn());
	applyXUILayout(profile_btn, this);
	mProfileBtn = LLUICtrlFactory::create<LLButton>(profile_btn);
	addChild(mProfileBtn);
	
	LLOutputMonitorCtrl::Params output_monitor(params.output_monitor());
	applyXUILayout(output_monitor, this);
	mOutputMonitorCtrl = LLUICtrlFactory::create<LLOutputMonitorCtrl>(output_monitor);
	addChild(mOutputMonitorCtrl);
}

void LLPersonView::initChildrenWidths(LLPersonView* self)
{

}
