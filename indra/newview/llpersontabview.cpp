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

#include "llavataractions.h"
#include "llfloaterreg.h"
#include "llpersonmodelcommon.h"

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
		gFocusMgr.setKeyboardFocus( this );
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

bool LLPersonView::sChildrenWidthsInitialized = false;
ChildWidthVec LLPersonView::mChildWidthVec;

LLPersonView::Params::Params() :
facebook_icon("facebook_icon"),
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
mFacebookIcon(NULL),
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
}

S32 LLPersonView::getLabelXPos()
{
	S32 label_x_pos;

	if(mAvatarIcon->getVisible())
	{
		label_x_pos = getIndentation() + mAvatarIcon->getRect().getWidth() + mIconPad;
	}
	else
	{
		label_x_pos = getIndentation() + mFacebookIcon->getRect().getWidth() + mIconPad;
	}


	return label_x_pos;
}

void LLPersonView::addToFolder(LLFolderViewFolder * person_folder_view)
{
	const LLFontGL * font = LLFontGL::getFontSansSerifSmall();

	LLFolderViewItem::addToFolder(person_folder_view);
	//Added item to folder could change folder's mHasVisibleChildren flag so call arrange
	person_folder_view->requestArrange();

	mPersonTabModel = static_cast<LLPersonTabModel *>(getParentFolder()->getViewModelItem());

	if(mPersonTabModel->mTabType == LLPersonTabModel::FB_SL_NON_SL_FRIEND)
	{
		mAvatarIcon->setVisible(TRUE);
		mFacebookIcon->setVisible(TRUE); 

		S32 label_width = font->getWidth(mLabel);
		F32 text_left = (F32)getLabelXPos();

		LLRect mFacebookIconRect = mFacebookIcon->getRect();
		S32 new_left = text_left + label_width + 7;
		mFacebookIconRect.set(new_left, 
			mFacebookIconRect.mTop, 
			new_left + mFacebookIconRect.getWidth(),
			mFacebookIconRect.mBottom);
		mFacebookIcon->setRect(mFacebookIconRect);
	}
	else if(mPersonTabModel->mTabType == LLPersonTabModel::FB_ONLY_FRIEND)
	{
		mFacebookIcon->setVisible(TRUE); 
	}

}

LLPersonView::~LLPersonView()
{

}

BOOL LLPersonView::postBuild()
{
	if(!sChildrenWidthsInitialized)
	{
		initChildrenWidthVec(this);
		sChildrenWidthsInitialized = true;
	}
	
	initChildVec();
	updateChildren();

	LLPersonModel * person_model = static_cast<LLPersonModel *>(getViewModelItem());

	mAvatarIcon->setValue(person_model->getAgentID());
	mInfoBtn->setClickedCallback(boost::bind(&LLFloaterReg::showInstance, "inspect_avatar", LLSD().with("avatar_id", person_model->getAgentID()), FALSE));
	mProfileBtn->setClickedCallback(boost::bind(&LLAvatarActions::showProfile, person_model->getAgentID()));

	return LLFolderViewItem::postBuild();
}

void LLPersonView::onMouseEnter(S32 x, S32 y, MASK mask)
{
	if(mPersonTabModel->mTabType == LLPersonTabModel::FB_SL_NON_SL_FRIEND)
	{
		mInfoBtn->setVisible(TRUE);
		mProfileBtn->setVisible(TRUE);
	}

	updateChildren();
	LLFolderViewItem::onMouseEnter(x, y, mask);
}

void LLPersonView::onMouseLeave(S32 x, S32 y, MASK mask)
{
	if(mPersonTabModel->mTabType == LLPersonTabModel::FB_SL_NON_SL_FRIEND)
	{
		mInfoBtn->setVisible(FALSE);
		mProfileBtn->setVisible(FALSE);
	}

	updateChildren();
	LLFolderViewItem::onMouseLeave(x, y, mask);
}

BOOL LLPersonView::handleMouseDown( S32 x, S32 y, MASK mask)
{
	if(!LLView::childrenHandleMouseDown(x, y, mask))
	{
		gFocusMgr.setMouseCapture( this );
	}

	if (!mIsSelected)
	{
		if(mask & MASK_CONTROL)
		{
			getRoot()->changeSelection(this, !mIsSelected);
		}
		else if (mask & MASK_SHIFT)
		{
			getParentFolder()->extendSelectionTo(this);
		}
		else
		{
			getRoot()->setSelection(this, FALSE);
		}
		make_ui_sound("UISndClick");
	}
	else
	{
		// If selected, we reserve the decision of deselecting/reselecting to the mouse up moment.
		// This is necessary so we maintain selection consistent when starting a drag.
		mSelectPending = TRUE;
	}

	mDragStartX = x;
	mDragStartY = y;
	return TRUE;
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
	drawLabel(mLabel, font, text_left, y, color, right_x);
	drawLabel(mLabelSuffix, font, mFacebookIcon->getRect().mRight + 7, y, color, right_x);

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

void LLPersonView::drawLabel(const std::string text, const LLFontGL * font, const F32 x, const F32 y, const LLColor4& color, F32 &right_x)
{
	font->renderUTF8(text, 0, x, y, color,
		LLFontGL::LEFT, LLFontGL::BOTTOM, LLFontGL::NORMAL, LLFontGL::NO_SHADOW,
		S32_MAX, getRect().getWidth() - (S32) x - mLabelPaddingRight, &right_x, TRUE);
}

void LLPersonView::initFromParams(const LLPersonView::Params & params)
{
	LLIconCtrl::Params facebook_icon_params(params.facebook_icon());
	applyXUILayout(facebook_icon_params, this);
	mFacebookIcon = LLUICtrlFactory::create<LLIconCtrl>(facebook_icon_params);
	addChild(mFacebookIcon);

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

	LLIconCtrl::Params permission_edit_mine_icon(params.permission_edit_mine_icon());
	applyXUILayout(permission_edit_mine_icon, this);
	mPermissionEditMineIcon = LLUICtrlFactory::create<LLIconCtrl>(permission_edit_mine_icon);
	addChild(mPermissionEditMineIcon);

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

void LLPersonView::initChildrenWidthVec(LLPersonView* self)
{
	S32 output_monitor_width = self->getRect().getWidth() - self->mOutputMonitorCtrl->getRect().mLeft;
	S32 profile_btn_width = self->mOutputMonitorCtrl->getRect().mLeft - self->mProfileBtn->getRect().mLeft;
	S32 info_btn_width = self->mProfileBtn->getRect().mLeft - self->mInfoBtn->getRect().mLeft;
	S32 permission_online_icon_width = self->mInfoBtn->getRect().mLeft - self->mPermissionOnlineIcon->getRect().mLeft;
	S32 permissions_map_icon_width = self->mPermissionOnlineIcon->getRect().mLeft - self->mPermissionMapIcon->getRect().mLeft;
	S32 permission_edit_mine_icon_width = self->mPermissionMapIcon->getRect().mLeft - self->mPermissionEditMineIcon->getRect().mLeft;
	S32 permission_edit_theirs_icon_width = self->mPermissionEditMineIcon->getRect().mLeft - self->mPermissionEditTheirsIcon->getRect().mLeft;
	S32 last_interaction_time_textbox_width = self->mPermissionEditTheirsIcon->getRect().mLeft - self->mLastInteractionTimeTextbox->getRect().mLeft;

	self->mChildWidthVec.push_back(output_monitor_width);
	self->mChildWidthVec.push_back(profile_btn_width);
	self->mChildWidthVec.push_back(info_btn_width);
	self->mChildWidthVec.push_back(permission_online_icon_width);
	self->mChildWidthVec.push_back(permissions_map_icon_width);
	self->mChildWidthVec.push_back(permission_edit_mine_icon_width);
	self->mChildWidthVec.push_back(permission_edit_theirs_icon_width);
	self->mChildWidthVec.push_back(last_interaction_time_textbox_width);
}

void LLPersonView::initChildVec()
{
	mChildVec.push_back(mOutputMonitorCtrl);
	mChildVec.push_back(mProfileBtn);
	mChildVec.push_back(mInfoBtn);
	mChildVec.push_back(mPermissionOnlineIcon);
	mChildVec.push_back(mPermissionMapIcon);
	mChildVec.push_back(mPermissionEditMineIcon);
	mChildVec.push_back(mPermissionEditTheirsIcon);
	mChildVec.push_back(mLastInteractionTimeTextbox);
}

void LLPersonView::updateChildren()
{
	mLabelPaddingRight = 0;
	LLView * control;
	S32 control_width;
	LLRect control_rect;

	llassert(mChildWidthVec.size() == mChildVec.size());

	for(S32 i = 0; i < mChildWidthVec.size(); ++i)
	{
		control = mChildVec[i];

		if(!control->getVisible())
		{
			continue;
		}

		control_width = mChildWidthVec[i];
		mLabelPaddingRight += control_width;

		control_rect = control->getRect();
		control_rect.setLeftTopAndSize(
			getLocalRect().getWidth() - mLabelPaddingRight,
			control_rect.mTop,
			control_rect.getWidth(),
			control_rect.getHeight());

		control->setShape(control_rect);

	}
}
