/** 
* @file llchiclet.cpp
* @brief LLChiclet class implementation
*
* $LicenseInfo:firstyear=2002&license=viewergpl$
* 
* Copyright (c) 2002-2009, Linden Research, Inc.
* 
* Second Life Viewer Source Code
* The source code in this file ("Source Code") is provided by Linden Lab
* to you under the terms of the GNU General Public License, version 2.0
* ("GPL"), unless you have obtained a separate licensing agreement
* ("Other License"), formally executed by you and Linden Lab.  Terms of
* the GPL can be found in doc/GPL-license.txt in this distribution, or
* online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
* 
* There are special exceptions to the terms and conditions of the GPL as
* it is applied to this Source Code. View the full text of the exception
* in the file doc/FLOSS-exception.txt in this software distribution, or
* online at
* http://secondlifegrid.net/programs/open_source/licensing/flossexception
* 
* By copying, modifying or distributing this software, you acknowledge
* that you have read and understood your obligations described above,
* and agree to abide by those obligations.
* 
* ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
* WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
* COMPLETENESS OR PERFORMANCE.
* $/LicenseInfo$
*/

#include "llviewerprecompiledheaders.h" // must be first include
#include "llchiclet.h"
#include "llagent.h"
#include "llavataractions.h"
#include "llbottomtray.h"
#include "llgroupactions.h"
#include "lliconctrl.h"
#include "llimpanel.h"
#include "llimview.h"
#include "llfloatergroupinfo.h"
#include "llmenugl.h"
#include "lloutputmonitorctrl.h"
#include "lltextbox.h"
#include "llvoiceclient.h"
#include "llvoicecontrolpanel.h"

static const std::string P2P_MENU_NAME = "IMChiclet P2P Menu";
static const std::string GROUP_MENU_NAME = "IMChiclet Group Menu";

static LLDefaultChildRegistry::Register<LLChicletPanel> t1("chiclet_panel");
static LLDefaultChildRegistry::Register<LLTalkButton> t2("chiclet_talk");
static LLDefaultChildRegistry::Register<LLNotificationChiclet> t3("chiclet_notification");
static LLDefaultChildRegistry::Register<LLIMChiclet> t4("chiclet_im");

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

LLNotificationChiclet::Params::Params()
: button("button")
, unread_notifications("unread_notifications")
{
	button.name("button");
	button.tab_stop(FALSE);
	button.label(LLStringUtil::null);

	unread_notifications.name("unread");
	unread_notifications.font(LLFontGL::getFontSansSerif());
	unread_notifications.text_color=(LLColor4::white);
	unread_notifications.font_halign(LLFontGL::HCENTER);
	unread_notifications.mouse_opaque(FALSE);
}

LLNotificationChiclet::LLNotificationChiclet(const Params& p)
: LLChiclet(p)
, mButton(NULL)
, mCounterCtrl(NULL)
{
	LLButton::Params button_params = p.button;
	button_params.rect(p.rect());
	mButton = LLUICtrlFactory::create<LLButton>(button_params);
	addChild(mButton);

 	LLChicletNotificationCounterCtrl::Params unread_params = p.unread_notifications;
	mCounterCtrl = LLUICtrlFactory::create<LLChicletNotificationCounterCtrl>(unread_params);
	addChild(mCounterCtrl);
}

LLNotificationChiclet::~LLNotificationChiclet()
{

}

void LLNotificationChiclet::setCounter(S32 counter)
{
	mCounterCtrl->setCounter(counter);
}

void LLNotificationChiclet::setShowCounter(bool show)
{
	LLChiclet::setShowCounter(show);
	mCounterCtrl->setVisible(getShowCounter());
}

boost::signals2::connection LLNotificationChiclet::setClickCallback(
	const commit_callback_t& cb)
{
	return mButton->setClickedCallback(cb);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

LLChiclet::Params::Params()
 : show_counter("show_counter")
{
	show_counter = true;
}

LLChiclet::LLChiclet(const Params& p)
: LLUICtrl(p)
, mSessionId(LLUUID::null)
, mShowCounter(p.show_counter)
{

}

LLChiclet::~LLChiclet()
{

}

boost::signals2::connection LLChiclet::setLeftButtonClickCallback(
	const commit_callback_t& cb)
{
	return mCommitSignal.connect(cb);
}

BOOL LLChiclet::handleMouseDown(S32 x, S32 y, MASK mask)
{
	onCommit();
	childrenHandleMouseDown(x,y,mask);
	return TRUE;
}

boost::signals2::connection LLChiclet::setChicletSizeChangedCallback(
	const chiclet_size_changed_callback_t& cb)
{
	return mChicletSizeChangedSignal.connect(cb);
}

void LLChiclet::onChicletSizeChanged()
{
	mChicletSizeChangedSignal(this, getValue());
}

LLSD LLChiclet::getValue() const
{
	return LLSD(getSessionId());
}

void LLChiclet::setValue(const LLSD& value)
{
	if(value.isUUID())
		setSessionId(value.asUUID());
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

LLIMChiclet::Params::Params()
: avatar_icon("avatar_icon")
, unread_notifications("unread_notifications")
, speaker("speaker")
, show_speaker("show_speaker")
{
	rect(LLRect(0, 25, 45, 0));

	avatar_icon.name("avatar_icon");
	avatar_icon.rect(LLRect(0, 25, 25, 0));

	unread_notifications.name("unread");
	unread_notifications.rect(LLRect(25, 25, 45, 0));
	unread_notifications.font(LLFontGL::getFontSansSerif());
	unread_notifications.font_halign(LLFontGL::HCENTER);
	unread_notifications.v_pad(5);
	unread_notifications.text_color(LLColor4::white);

	speaker.name("speaker");
	speaker.rect(LLRect(45, 25, 65, 0));

	show_speaker = false;
}

LLIMChiclet::LLIMChiclet(const Params& p)
: LLChiclet(p)
, mAvatarCtrl(NULL)
, mCounterCtrl(NULL)
, mSpeakerCtrl(NULL)
, mShowSpeaker(p.show_speaker)
, mPopupMenu(NULL)
{
	LLChicletAvatarIconCtrl::Params avatar_params = p.avatar_icon;
	mAvatarCtrl = LLUICtrlFactory::create<LLChicletAvatarIconCtrl>(avatar_params);
	addChild(mAvatarCtrl);

	LLChicletNotificationCounterCtrl::Params unread_params = p.unread_notifications;
	mCounterCtrl = LLUICtrlFactory::create<LLChicletNotificationCounterCtrl>(unread_params);
	addChild(mCounterCtrl);

	setCounter(getCounter());
	setShowCounter(getShowCounter());

	LLChicletSpeakerCtrl::Params speaker_params = p.speaker;
	mSpeakerCtrl = LLUICtrlFactory::create<LLChicletSpeakerCtrl>(speaker_params);
	addChild(mSpeakerCtrl);

	setShowSpeaker(getShowSpeaker());
}

LLIMChiclet::~LLIMChiclet()
{

}

void LLIMChiclet::setCounter(S32 counter)
{
	mCounterCtrl->setCounter(counter);

	if(getShowCounter())
	{
		LLRect counter_rect = mCounterCtrl->getRect();
		LLRect required_rect = mCounterCtrl->getRequiredRect();
		bool needs_resize = required_rect.getWidth() != counter_rect.getWidth();

		if(needs_resize)
		{
			counter_rect.mRight = counter_rect.mLeft + required_rect.getWidth();
			mCounterCtrl->reshape(counter_rect.getWidth(), counter_rect.getHeight());
			mCounterCtrl->setRect(counter_rect);

			onChicletSizeChanged();
		}
	}
}

LLRect LLIMChiclet::getRequiredRect()
{
	LLRect rect(0, 0, mAvatarCtrl->getRect().getWidth(), 0);
	if(getShowCounter())
	{
		rect.mRight += mCounterCtrl->getRequiredRect().getWidth();
	}
	if(getShowSpeaker())
	{
		rect.mRight += mSpeakerCtrl->getRect().getWidth();
	}
	return rect;
}

void LLIMChiclet::setShowCounter(bool show)
{
	bool needs_resize = getShowCounter() != show;

	LLChiclet::setShowCounter(show);
	mCounterCtrl->setVisible(getShowCounter());

	if(needs_resize)
	{
		onChicletSizeChanged();
	}
}

void LLIMChiclet::setIMSessionName(const std::string& name)
{
	setToolTip(name);
}

void LLIMChiclet::setOtherParticipantId(const LLUUID& other_participant_id)
{
	if (mAvatarCtrl)
	{
		mAvatarCtrl->setValue(other_participant_id);
	}
}

void LLIMChiclet::updateMenuItems()
{
	if(!mPopupMenu)
		return;
	if(getSessionId().isNull())
		return;

	if(P2P_MENU_NAME == mPopupMenu->getName())
	{
		bool is_friend = LLAvatarActions::isFriend(mAvatarCtrl->getAvatarId());

		mPopupMenu->getChild<LLUICtrl>("Add Friend")->setEnabled(!is_friend);
		mPopupMenu->getChild<LLUICtrl>("Remove Friend")->setEnabled(is_friend);
	}
}

void LLIMChiclet::setShowSpeaker(bool show)
{
	bool needs_resize = getShowSpeaker() != show;

	mShowSpeaker = show;
	mSpeakerCtrl->setVisible(getShowSpeaker());

	if(needs_resize)
	{
		onChicletSizeChanged();
	}
}

void LLIMChiclet::draw()
{
	LLUICtrl::draw();
	gl_rect_2d(0, getRect().getHeight(), getRect().getWidth(), 0, LLColor4(0.0f,0.0f,0.0f,1.f), FALSE);
}

BOOL LLIMChiclet::handleRightMouseDown(S32 x, S32 y, MASK mask)
{
	if(!mPopupMenu)
		createPopupMenu();

	updateMenuItems();

	if (mPopupMenu)
	{
		mPopupMenu->arrangeAndClear();
	}
	
	LLMenuGL::showPopup(this, mPopupMenu, x, y);

	return TRUE;
}

void LLIMChiclet::createPopupMenu()
{
	if(mPopupMenu)
	{
		llwarns << "Menu already exists" << llendl;
		return;
	}
	if(getSessionId().isNull())
		return;

	LLFloaterIMPanel*floater = gIMMgr->findFloaterBySession(getSessionId());
	if(!floater)
		return;

	LLUICtrl::CommitCallbackRegistry::ScopedRegistrar registrar;
	registrar.add("IMChicletMenu.Action", boost::bind(&LLIMChiclet::onMenuItemClicked, this, _2));

	switch(floater->getDialogType())
	{
	case IM_SESSION_GROUP_START:
		mPopupMenu = LLUICtrlFactory::getInstance()->createFromFile<LLMenuGL>
			("menu_imchiclet_group.xml", gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());
		break;
	case IM_NOTHING_SPECIAL:
		mPopupMenu = LLUICtrlFactory::getInstance()->createFromFile<LLMenuGL>
			("menu_imchiclet_p2p.xml", gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());
		break;
	default:
		llwarns << "Unexpected dialog type" << llendl;
		break;
	}
}

void LLIMChiclet::onMenuItemClicked(const LLSD& user_data)
{
	std::string level = user_data.asString();
	LLUUID other_participant_id = mAvatarCtrl->getAvatarId();

	if("profile" == level)
	{
		LLAvatarActions::showProfile(other_participant_id);
	}
	else if("im" == level)
	{
		LLAvatarActions::startIM(other_participant_id);
	}
	else if("add" == level)
	{
		std::string name;
		gCacheName->getFullName(other_participant_id,name);
		LLAvatarActions::requestFriendshipDialog(other_participant_id,name);
	}
	else if("remove" == level)
	{
		LLAvatarActions::removeFriendDialog(other_participant_id);
	}
	else if("group chat" == level)
	{
		LLGroupActions::startChat(other_participant_id);
	}
	else if("info" == level)
	{
		LLFloaterGroupInfo::showFromUUID(other_participant_id);
	}
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

LLChicletPanel::Params::Params()
: chiclet_padding("chiclet_padding")
, scrolling_offset("scrolling_offset")
, left_scroll_button("left_scroll_button")
, right_scroll_button("right_scroll_button")
{
	chiclet_padding = 3;
	scrolling_offset = 40;

	LLRect scroll_button_rect(0, 25, 19, 5);

	left_scroll_button.name("left_scroll");
	left_scroll_button.label(LLStringUtil::null);
	left_scroll_button.rect(scroll_button_rect);
	left_scroll_button.tab_stop(false);
	left_scroll_button.image_selected(LLUI::getUIImage("bottom_tray_scroll_left.tga"));
	left_scroll_button.image_unselected(LLUI::getUIImage("bottom_tray_scroll_left.tga"));
	left_scroll_button.image_hover_selected(LLUI::getUIImage("bottom_tray_scroll_left.tga"));

	right_scroll_button.name("right_scroll");
	right_scroll_button.label(LLStringUtil::null);
	right_scroll_button.rect(scroll_button_rect);
	right_scroll_button.tab_stop(false);
	right_scroll_button.image_selected(LLUI::getUIImage("bottom_tray_scroll_right.tga"));
	right_scroll_button.image_unselected(LLUI::getUIImage("bottom_tray_scroll_right.tga"));
	right_scroll_button.image_hover_selected(LLUI::getUIImage("bottom_tray_scroll_right.tga"));
};

LLChicletPanel::LLChicletPanel(const Params&p)
: LLPanel(p)
, mScrollArea(NULL)
, mLeftScrollButton(NULL)
, mRightScrollButton(NULL)
, mChicletPadding(p.chiclet_padding)
, mScrollingOffset(p.scrolling_offset)
{
	LLButton::Params scroll_button_params = p.left_scroll_button;

	mLeftScrollButton = LLUICtrlFactory::create<LLButton>(scroll_button_params);
	addChild(mLeftScrollButton);

	mLeftScrollButton->setClickedCallback(boost::bind(&LLChicletPanel::onLeftScrollClick,this));
	mLeftScrollButton->setEnabled(false);

	scroll_button_params = p.right_scroll_button;
	mRightScrollButton = LLUICtrlFactory::create<LLButton>(scroll_button_params);
	addChild(mRightScrollButton);

	mRightScrollButton->setClickedCallback(boost::bind(&LLChicletPanel::onRightScrollClick,this));
	mRightScrollButton->setEnabled(false);

	LLPanel::Params panel_params;
	mScrollArea = LLUICtrlFactory::create<LLPanel>(panel_params,this);
	addChild(mScrollArea);
}

LLChicletPanel::~LLChicletPanel()
{

}

void im_chiclet_callback(LLChicletPanel* panel, const LLSD& data){
	
	LLUUID session_id = data["session_id"].asUUID();
	LLChiclet* chiclet = panel->findChiclet<LLChiclet>(session_id);

	if (chiclet)
	{
		chiclet->setCounter(data["num_unread"].asInteger());
	}
    else
    {
    	llwarns << "Unable to set counter for chiclet " << session_id << llendl;
    }
}


BOOL LLChicletPanel::postBuild()
{
	LLPanel::postBuild();
	LLIMModel::instance().addChangedCallback(boost::bind(im_chiclet_callback, this, _1));

	return TRUE;
}

bool LLChicletPanel::addChiclet(LLChiclet* chiclet, S32 index)
{
	if(mScrollArea->addChild(chiclet))
	{
		S32 offset = 0;
		// Do not scroll chiclets if chiclets are scrolled right and new
		// chiclet is added to the beginning of the list
		if(canScrollLeft())
		{
			offset = - (chiclet->getRequiredRect().getWidth() + getChicletPadding());
			if(0 == index)
			{
				offset += getChiclet(0)->getRect().mLeft;
			}
		}

		mChicletList.insert(mChicletList.begin() + index, chiclet);

		getChiclet(0)->translate(offset, 0);

		chiclet->setLeftButtonClickCallback(boost::bind(&LLChicletPanel::onChicletClick, this, _1, _2));
		chiclet->setChicletSizeChangedCallback(boost::bind(&LLChicletPanel::onChicletSizeChanged, this, _1, index));

		arrange();
		showScrollButtonsIfNeeded();

		return true;
	}

	return false;
}

void LLChicletPanel::onChicletSizeChanged(LLChiclet* ctrl, const LLSD& param)
{
	S32 chiclet_width = ctrl->getRect().getWidth();
	S32 chiclet_new_width = ctrl->getRequiredRect().getWidth();

	if(chiclet_new_width == chiclet_width)
	{
		return;
	}

	LLRect chiclet_rect = ctrl->getRect();
	chiclet_rect.mRight = chiclet_rect.mLeft + chiclet_new_width;	

	ctrl->setRect(chiclet_rect);

	S32 offset = chiclet_new_width - chiclet_width;
	S32 index = getChicletIndex(ctrl);

	shiftChiclets(offset, index + 1);
	trimChiclets();
	showScrollButtonsIfNeeded();
}

void LLChicletPanel::onChicletClick(LLUICtrl*ctrl,const LLSD&param)
{
	LLIMChiclet* chiclet = dynamic_cast<LLIMChiclet*>(ctrl);
	if (chiclet)
	{
		S32 x, y;
		LLRect rect = getRect();
		localPointToScreen(rect.getCenterX(), 0, &x, &y);
		LLIMFloater::show(chiclet->getSessionId(), x);
	}
	mCommitSignal(ctrl,param);
}

void LLChicletPanel::removeChiclet(chiclet_list_t::iterator it)
{
	mScrollArea->removeChild(*it);
	mChicletList.erase(it);
	
	arrange();
	trimChiclets();
	showScrollButtonsIfNeeded();
}

void LLChicletPanel::removeChiclet(S32 index)
{
	if(index >= 0 && index < getChicletCount())
	{
		removeChiclet(mChicletList.begin() + index);
	}
}

S32 LLChicletPanel::getChicletIndex(const LLChiclet* chiclet)
{
	if(mChicletList.empty())
		return -1;

	S32 size = getChicletCount();
	for(int n = 0; n < size; ++n)
	{
		if(chiclet == mChicletList[n])
			return n;
	}

	return -1;
}

void LLChicletPanel::removeChiclet(LLChiclet*chiclet)
{
	chiclet_list_t::iterator it = mChicletList.begin();
	for( ; mChicletList.end() != it; ++it)
	{
		LLChiclet* temp = *it;
		if(temp == chiclet)
		{
			removeChiclet(it);
			return;
		}
	}
}

void LLChicletPanel::removeChiclet(const LLUUID& im_session_id)
{
	chiclet_list_t::iterator it = mChicletList.begin();
	for( ; mChicletList.end() != it; ++it)
	{
		LLIMChiclet* chiclet = dynamic_cast<LLIMChiclet*>(*it);

		if(chiclet->getSessionId() == im_session_id)
		{
			removeChiclet(it);
			return;
		}
	}
}

void LLChicletPanel::removeAll()
{
	S32 size = getChicletCount();
	for(S32 n = 0; n < size; ++n)
	{
		mScrollArea->removeChild(mChicletList[n]);
	}

	mChicletList.erase(mChicletList.begin(), mChicletList.end());

	showScrollButtonsIfNeeded();
}

void LLChicletPanel::reshape(S32 width, S32 height, BOOL called_from_parent )
{
	LLPanel::reshape(width,height,called_from_parent);

	static const S32 SCROLL_BUTTON_PAD = 5;

	LLRect scroll_button_rect = mLeftScrollButton->getRect();
	mLeftScrollButton->setRect(LLRect(0,height,scroll_button_rect.getWidth(),
		height - scroll_button_rect.getHeight()));

	scroll_button_rect = mRightScrollButton->getRect();
	mRightScrollButton->setRect(LLRect(width - scroll_button_rect.getWidth(),height,
		width, height - scroll_button_rect.getHeight()));

	mScrollArea->setRect(LLRect(scroll_button_rect.getWidth() + SCROLL_BUTTON_PAD,
		height + 1, width - scroll_button_rect.getWidth() - SCROLL_BUTTON_PAD, 0));

	trimChiclets();

	showScrollButtonsIfNeeded();
}

void LLChicletPanel::arrange()
{
	if(mChicletList.empty())
		return;

	S32 chiclet_left = getChiclet(0)->getRect().mLeft;

	S32 size = getChicletCount();
	for( int n = 0; n < size; ++n)
	{
		LLChiclet* chiclet = getChiclet(n);

		S32 chiclet_width = chiclet->getRequiredRect().getWidth();
		LLRect rect = chiclet->getRect();
		rect.set(chiclet_left, rect.mTop, chiclet_left + chiclet_width, rect.mBottom);

		chiclet->setRect(rect);

		chiclet_left += chiclet_width + getChicletPadding();
	}
}

void LLChicletPanel::trimChiclets()
{
	// trim right
	if(canScrollLeft() && !canScrollRight())
	{
		S32 last_chiclet_right = (*mChicletList.rbegin())->getRect().mRight;
		S32 scroll_width = mScrollArea->getRect().getWidth();
		if(last_chiclet_right < scroll_width)
		{
			shiftChiclets(scroll_width - last_chiclet_right);
		}
	}

	// trim left
	if(!mChicletList.empty())
	{
		LLRect first_chiclet_rect = getChiclet(0)->getRect();
		if(first_chiclet_rect.mLeft > 0)
		{
			shiftChiclets( - first_chiclet_rect.mLeft);
		}
	}
}

void LLChicletPanel::showScrollButtonsIfNeeded()
{
	bool can_scroll_left = canScrollLeft();
	bool can_scroll_right = canScrollRight();

	mLeftScrollButton->setEnabled(can_scroll_left);
	mRightScrollButton->setEnabled(can_scroll_right);

	bool show_scroll_buttons = can_scroll_left || can_scroll_right;

	mLeftScrollButton->setVisible(show_scroll_buttons);
	mRightScrollButton->setVisible(show_scroll_buttons);
}

void LLChicletPanel::draw()
{
	child_list_const_iter_t it = getChildList()->begin();
	for( ; getChildList()->end() != it; ++it)
	{
		LLView* child = *it;
		if(child == dynamic_cast<LLView*>(mScrollArea))
		{
			LLLocalClipRect clip(mScrollArea->getRect());
			drawChild(mScrollArea);
		}
		else
		{
			drawChild(child);
		}
	}
}

bool LLChicletPanel::canScrollRight()
{
	if(mChicletList.empty())
		return false;

	S32 scroll_width = mScrollArea->getRect().getWidth();
	S32 last_chiclet_right = (*mChicletList.rbegin())->getRect().mRight;

	if(last_chiclet_right > scroll_width)
		return true;

	return false;
}

bool LLChicletPanel::canScrollLeft()
{
	if(mChicletList.empty())
		return false;

	return getChiclet(0)->getRect().mLeft < 0;
}

void LLChicletPanel::scroll(S32 offset)
{
	shiftChiclets(offset);
}

void LLChicletPanel::shiftChiclets(S32 offset, S32 start_index /* = 0 */)
{
	if(start_index < 0 || start_index >= getChicletCount())
	{
		return;
	}

	chiclet_list_t::const_iterator it = mChicletList.begin() + start_index;
	for(;mChicletList.end() != it; ++it)
	{
		LLChiclet* chiclet = *it;
		chiclet->translate(offset,0);
	}
}

void LLChicletPanel::scrollLeft()
{
	if(canScrollLeft())
	{
		S32 offset = getScrollingOffset();
		LLRect first_chiclet_rect = getChiclet(0)->getRect();

		// shift chiclets in case first chiclet is partially visible
		if(first_chiclet_rect.mLeft < 0 && first_chiclet_rect.mRight > 0)
		{
			offset = llabs(first_chiclet_rect.mLeft);
		}

		scroll(offset);
		
		showScrollButtonsIfNeeded();
	}
}

void LLChicletPanel::scrollRight()
{
	if(canScrollRight())
	{
		S32 offset = - getScrollingOffset();

		S32 last_chiclet_right = (*mChicletList.rbegin())->getRect().mRight;
		S32 scroll_rect_width = mScrollArea->getRect().getWidth();
		// if after scrolling, the last chiclet will not be aligned to 
		// scroll area right side - align it.
		if( last_chiclet_right + offset < scroll_rect_width )
		{
			offset = scroll_rect_width - last_chiclet_right;
		}

		scroll(offset);
		
		showScrollButtonsIfNeeded();
	}
}

void LLChicletPanel::onLeftScrollClick()
{
	scrollLeft();
}

void LLChicletPanel::onRightScrollClick()
{
	scrollRight();
}

boost::signals2::connection LLChicletPanel::setChicletClickedCallback(
	const commit_callback_t& cb)
{
	return mCommitSignal.connect(cb);
}

BOOL LLChicletPanel::handleScrollWheel(S32 x, S32 y, S32 clicks)
{
	if(clicks > 0)
	{
		scrollRight();
	}
	else
	{
		scrollLeft();
	}
	return TRUE;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

LLTalkButton::Params::Params()
 : speak_button("speak_button")
 , show_button("show_button")
 , monitor("monitor")
{
	speak_button.name("left");
	speak_button.label("Speak");
	speak_button.label_selected("Speak");
	speak_button.font(LLFontGL::getFontSansSerifSmall());
	speak_button.tab_stop(false);
	speak_button.is_toggle(true);
	speak_button.picture_style(true);
	speak_button.image_selected(LLUI::getUIImage("SegmentedBtn_Left_Selected"));
	speak_button.image_unselected(LLUI::getUIImage("SegmentedBtn_Left_Off"));

	show_button.name("right");
	show_button.label(LLStringUtil::null);
	show_button.rect(LLRect(0, 0, 20, 0));
	show_button.tab_stop(false);
	show_button.is_toggle(true);
	show_button.picture_style(true);
	show_button.image_selected(LLUI::getUIImage("ComboButton_Selected"));
	show_button.image_unselected(LLUI::getUIImage("ComboButton_Off"));

	monitor.name("monitor");
	monitor.rect(LLRect(0, 10, 16, 0));
}

LLTalkButton::LLTalkButton(const Params& p)
: LLUICtrl(p)
, mPrivateCallPanel(NULL)
, mOutputMonitor(NULL)
, mSpeakBtn(NULL)
, mShowBtn(NULL)
{
	LLRect rect = p.rect();
	LLRect speak_rect(0, rect.getHeight(), rect.getWidth(), 0);
	LLRect show_rect = p.show_button.rect();
	show_rect.set(0, rect.getHeight(), show_rect.getWidth(), 0);

	speak_rect.mRight -= show_rect.getWidth();
	show_rect.mLeft = speak_rect.getWidth();
	show_rect.mRight = rect.getWidth();

	LLButton::Params speak_params = p.speak_button;
	speak_params.rect(speak_rect);
	mSpeakBtn = LLUICtrlFactory::create<LLButton>(speak_params);
	addChild(mSpeakBtn);

	mSpeakBtn->setClickedCallback(boost::bind(&LLTalkButton::onClick_SpeakBtn, this));
	mSpeakBtn->setToggleState(FALSE);

	LLButton::Params show_params = p.show_button;
	show_params.rect(show_rect);
	mShowBtn = LLUICtrlFactory::create<LLButton>(show_params);
	addChild(mShowBtn);

	mShowBtn->setClickedCallback(boost::bind(&LLTalkButton::onClick_ShowBtn, this));
	mShowBtn->setToggleState(FALSE);

	static const S32 MONITOR_RIGHT_PAD = 2;

	LLRect monitor_rect = p.monitor.rect();
	S32 monitor_height = monitor_rect.getHeight();
	monitor_rect.mLeft = speak_rect.getWidth() - monitor_rect.getWidth() - MONITOR_RIGHT_PAD;
	monitor_rect.mRight = speak_rect.getWidth() - MONITOR_RIGHT_PAD;
	monitor_rect.mBottom = (rect.getHeight() / 2) - (monitor_height / 2);
	monitor_rect.mTop = monitor_rect.mBottom + monitor_height;

	LLOutputMonitorCtrl::Params monitor_params = p.monitor;
	monitor_params.draw_border(false);
	monitor_params.rect(monitor_rect);
	mOutputMonitor = LLUICtrlFactory::create<LLOutputMonitorCtrl>(monitor_params);
	mSpeakBtn->addChild(mOutputMonitor);

	// never show "muted" because you can't mute yourself
	mOutputMonitor->setIsMuted(false);
}

LLTalkButton::~LLTalkButton()
{
}

void LLTalkButton::draw()
{
	// Always provide speaking feedback.  User can trigger speaking
	// with keyboard or middle-mouse shortcut.
	mOutputMonitor->setPower(gVoiceClient->getCurrentPower(gAgent.getID()));

	LLUICtrl::draw();
}

void LLTalkButton::onClick_SpeakBtn()
{
	bool speaking = mSpeakBtn->getToggleState();
	gVoiceClient->setUserPTTState(speaking);
}

void LLTalkButton::onClick_ShowBtn()
{
	if(!mShowBtn->getToggleState())
	{
		mPrivateCallPanel->onClickClose(mPrivateCallPanel);
		delete mPrivateCallPanel;
		mPrivateCallPanel = NULL;
		mShowBtn->setToggleState(FALSE);
		return;
	}

	S32 x = mSpeakBtn->getRect().mLeft;
	S32 y = 0;

	localPointToScreen(x, y, &x, &y);

	mPrivateCallPanel = new LLVoiceControlPanel;
	getRootView()->addChild(mPrivateCallPanel);

	y = LLBottomTray::getInstance()->getRect().getHeight() + mPrivateCallPanel->getRect().getHeight();

	LLRect rect;
	rect.setLeftTopAndSize(x, y, mPrivateCallPanel->getRect().getWidth(), mPrivateCallPanel->getRect().getHeight());
	mPrivateCallPanel->setRect(rect);

	LLAvatarListItem::Params p;
	p.buttons.status = true;
	p.buttons.info = true;
	p.buttons.profile = false;
	p.buttons.locator = true;

	mPrivateCallPanel->addItem(new LLAvatarListItem(p));
	mPrivateCallPanel->setVisible(TRUE);
	mPrivateCallPanel->setFrontmost(TRUE);

	mShowBtn->setToggleState(TRUE);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

LLChicletNotificationCounterCtrl::LLChicletNotificationCounterCtrl(const Params& p)
 : LLTextBox(p)
 , mCounter(0)
 , mInitialWidth(0)
{
	mInitialWidth = getRect().getWidth();
}

void LLChicletNotificationCounterCtrl::setCounter(S32 counter)
{
	mCounter = counter;

	std::stringstream stream;
	stream << getCounter();
	setText(stream.str());
}

LLRect LLChicletNotificationCounterCtrl::getRequiredRect()
{
	LLRect rc;
	S32 text_width = getFont()->getWidth(getText());

	rc.mRight = rc.mLeft + llmax(text_width, mInitialWidth);
	
	return rc;
}

void LLChicletNotificationCounterCtrl::setValue(const LLSD& value)
{
	if(value.isInteger())
		setCounter(value.asInteger());
}

LLSD LLChicletNotificationCounterCtrl::getValue() const
{
	return LLSD(getCounter());
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

LLChicletAvatarIconCtrl::LLChicletAvatarIconCtrl(const Params& p)
 : LLAvatarIconCtrl(p)
{
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

LLChicletSpeakerCtrl::LLChicletSpeakerCtrl(const Params&p)
 : LLIconCtrl(p)
{
}
