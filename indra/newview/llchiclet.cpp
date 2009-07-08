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
#include "llvoiceclient.h"
#include "llagent.h"
#include "lltextbox.h"
#include "lliconctrl.h"
#include "llvoicecontrolpanel.h"
#include "lloutputmonitorctrl.h"
#include "llimview.h"
#include "llbottomtray.h"
#include "llimpanel.h"

static const S32 CHICLET_HEIGHT = 25;
static const S32 CHICLET_SPACING = 0;
static const S32 CHICLET_PADDING = 3;
static const S32 AVATAR_WIDTH = 25;
static const S32 SPEAKER_WIDTH = 20;
static const S32 COUNTER_WIDTH = 20;
static const S32 SCROLL_BUTTON_WIDTH = 19;
static const S32 SCROLL_BUTTON_HEIGHT = 20;
static const S32 NOTIFICATION_TEXT_TOP_PAD = 5;

static LLDefaultChildRegistry::Register<LLChicletPanel> t1("chiclet_panel");
static LLDefaultChildRegistry::Register<LLTalkButton> t2("chiclet_talk");
static LLDefaultChildRegistry::Register<LLNotificationChiclet> t3("chiclet_notification");
static LLDefaultChildRegistry::Register<LLChicletPanel> t4("chiclet_panel");

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

LLNotificationChiclet::Params::Params()
: image_unselected("image_unselected")
, image_selected("image_selected")
, image_overlay("image_overlay")
{
}

LLNotificationChiclet::LLNotificationChiclet(const Params& p)
: LLChiclet(p)
, mButton(NULL)
, mCounterText(NULL)
{
	LLRect rc(p.rect);

	LLButton::Params button_params;
	button_params.name("btn");
	button_params.label(LLStringUtil::null);
	button_params.rect(LLRect(0,rc.getHeight(),rc.getWidth(),0));
	button_params.image_overlay(p.image_overlay);
	button_params.image_unselected(p.image_unselected);
	button_params.image_selected(p.image_selected);
	button_params.tab_stop(false);
	mButton = LLUICtrlFactory::create<LLButton>(button_params);
	addChild(mButton);

	LLTextBox::Params textbox_params;
	textbox_params.name("txt");
	textbox_params.rect(LLRect(p.label_left,rc.getHeight(),
		rc.getWidth()-p.label_left,0));
	textbox_params.mouse_opaque(false);
	textbox_params.v_pad(NOTIFICATION_TEXT_TOP_PAD);
	textbox_params.font.style("SansSerif");
	textbox_params.font_halign(LLFontGL::HCENTER);
	mCounterText = LLUICtrlFactory::create<LLTextBox>(textbox_params);
	addChild(mCounterText);
	mCounterText->setColor(LLColor4::white);
	mCounterText->setText(LLStringUtil::null);
}

LLNotificationChiclet::~LLNotificationChiclet()
{

}

LLChiclet* LLNotificationChiclet::create(const Params& p)
{
	LLChiclet* chiclet = new LLNotificationChiclet(p);
	return chiclet;
}

void LLNotificationChiclet::setCounter(S32 counter)
{
	std::stringstream stream;
	mCounter = counter;
	stream << mCounter;
	mCounterText->setText(stream.str());
}

boost::signals2::connection LLNotificationChiclet::setClickCallback(
	const commit_callback_t& cb)
{
	return mButton->setClickedCallback(cb);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

LLChiclet::LLChiclet(const Params& p)
: LLUICtrl(p)
, mCounter(0)
, mShowCounter(true)
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

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

LLIMChiclet::LLIMChiclet(const LLChiclet::Params& p)
: LLChiclet(p)
, mAvatar(NULL)
, mCounterText(NULL)
, mSpeaker(NULL)
, mIMSessionId(LLUUID::null)
, mShowSpeaker(false)
, mSpeakerStatus(SPEAKER_IDLE)
{
	LLAvatarIconCtrl::Params avatar_params;
	avatar_params.control_name("avatar");
	avatar_params.draw_tooltip = FALSE;
	mAvatar = LLUICtrlFactory::create<LLAvatarIconCtrl>(avatar_params);

	addChild(mAvatar);

	LLTextBox::Params unread_params;
	unread_params.font.style("SansSerif");
	unread_params.font_halign(LLFontGL::HCENTER);
	unread_params.v_pad(5);
	mCounterText = LLUICtrlFactory::create<LLTextBox>(unread_params);
	addChild(mCounterText);
	mCounterText->setColor(LLColor4::white);
	setCounter(getCounter());

	LLIconCtrl::Params speaker_params;
	speaker_params.image( LLUI::getUIImage("icn_voice_ptt-on-lvl2.tga") );
	mSpeaker = LLUICtrlFactory::create<LLIconCtrl>(speaker_params);
	addChild(mSpeaker);
	mSpeaker->setVisible(getShowSpeaker());

	S32 left = 0;
	mAvatar->setRect(LLRect(left,CHICLET_HEIGHT,AVATAR_WIDTH,0));
	left += AVATAR_WIDTH + CHICLET_SPACING;
	mCounterText->setRect(LLRect(left,CHICLET_HEIGHT,left + COUNTER_WIDTH,0));
	left += COUNTER_WIDTH + CHICLET_SPACING;
	mSpeaker->setRect(LLRect(left,CHICLET_HEIGHT,left + SPEAKER_WIDTH,0));
}

LLIMChiclet::~LLIMChiclet()
{

}

LLChiclet* LLIMChiclet::create(const LLUUID& im_session_id /* = LLUUID::null */)
{
	LLIMChiclet* chiclet = new LLIMChiclet(LLChiclet::Params());
	chiclet->setIMSessionId(im_session_id);
	return chiclet;
}

void LLIMChiclet::setCounter(S32 counter)
{
	mCounter = counter;
	std::stringstream stream;
	stream << mCounter;
	mCounterText->setText(stream.str());

	LLRect rc = mCounterText->getRect();
	rc.mRight = rc.mLeft + calcCounterWidth();
	mCounterText->setRect(rc);
}

LLRect LLIMChiclet::getRequiredRect()
{
	LLRect rect(0,CHICLET_HEIGHT,AVATAR_WIDTH,0);
	if(getShowCounter())
	{
		rect.mRight += CHICLET_SPACING + calcCounterWidth();
	}
	if(getShowSpeaker())
	{
		rect.mRight += CHICLET_SPACING + SPEAKER_WIDTH;
	}
	return rect;
}

void LLIMChiclet::setShowCounter(bool show)
{
	LLChiclet::setShowCounter(show);
	mCounterText->setVisible(getShowCounter());
}

void LLIMChiclet::setIMSessionName(const std::string& name)
{
	setToolTip(name);
}

void LLIMChiclet::setOtherParticipantId(const LLUUID& other_participant_id)
{
	if (mAvatar)
	{
		mAvatar->setValue(other_participant_id);
	}
}

void LLIMChiclet::setShowSpeaker(bool show)
{
	mShowSpeaker = show;
	mSpeaker->setVisible(getShowSpeaker());
}

void LLIMChiclet::draw()
{
	LLUICtrl::draw();
	gl_rect_2d(0, getRect().getHeight(), getRect().getWidth(), 0, LLColor4(0.0f,0.0f,0.0f,1.f), FALSE);
}

S32 LLIMChiclet::calcCounterWidth()
{
	S32 font_width = mCounterText->getFont()->getWidth("0");
	S32 text_size = mCounterText->getText().size();

	return llmax(font_width * text_size, COUNTER_WIDTH);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

LLChicletPanel::LLChicletPanel(const Params&p)
: LLPanel(p)
, mScrollArea(NULL)
, mLeftScroll(NULL)
, mRightScroll(NULL)
{
	LLButton::Params params;

	params.name("scroll_left");
	params.label(LLStringUtil::null);
	params.tab_stop(false);
	params.image_selected(LLUI::getUIImage("bottom_tray_scroll_left.tga"));
	params.image_unselected(LLUI::getUIImage("bottom_tray_scroll_left.tga"));
	params.image_hover_selected(LLUI::getUIImage("bottom_tray_scroll_left.tga"));
	mLeftScroll = LLUICtrlFactory::create<LLButton>(params);
	addChild(mLeftScroll);
	mLeftScroll->setClickedCallback(boost::bind(&LLChicletPanel::onLeftScrollClick,this));
	mLeftScroll->setEnabled(false);

	params.name("scroll_right");
	params.image_selected(LLUI::getUIImage("bottom_tray_scroll_right.tga"));
	params.image_unselected(LLUI::getUIImage("bottom_tray_scroll_right.tga"));
	params.image_hover_selected(LLUI::getUIImage("bottom_tray_scroll_right.tga"));
	mRightScroll = LLUICtrlFactory::create<LLButton>(params);
	addChild(mRightScroll);
	mRightScroll->setClickedCallback(boost::bind(&LLChicletPanel::onRightScrollClick,this));
	mRightScroll->setEnabled(false);

	LLPanel::Params panel_params;
	mScrollArea = LLUICtrlFactory::create<LLPanel>(panel_params,this);
	addChild(mScrollArea);
}

LLChicletPanel::~LLChicletPanel()
{

}

void im_chiclet_callback(LLChicletPanel* panel, const LLSD& data){
	
	LLUUID session_id = data["session_id"].asUUID();
	LLChiclet* chiclet = panel->findIMChiclet(session_id);

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

LLChiclet* LLChicletPanel::createChiclet(const LLUUID& im_session_id /* = LLUUID::null */, S32 pos /* = 0 */)
{
	LLChiclet* chiclet = LLIMChiclet::create(im_session_id);
	if(!chiclet)
	{
		assert(false);
		return NULL;
	}

	if(!addChiclet(chiclet, pos))
	{
		assert(false);
		return NULL;
	}

	return chiclet;
}

bool LLChicletPanel::addChiclet(LLChiclet* chiclet, S32 pos)
{
	if(mScrollArea->addChild(chiclet))
	{
		// if first chiclet is scrolled left, the created one should be scrolled left too
		if(0 == pos && canScrollLeft())
		{
			LLRect first_chiclet_rect = getChiclet(0)->getRect();
			chiclet->setRect(first_chiclet_rect);
		}

		mChicletList.insert(mChicletList.begin() + pos, chiclet);

		chiclet->setLeftButtonClickCallback(boost::bind(&LLChicletPanel::onChicletClick, this, _1, _2));

		arrange();
		showScrollButtonsIfNeeded();

		return true;
	}

	return false;
}

void LLChicletPanel::onChicletClick(LLUICtrl*ctrl,const LLSD&param)
{
	LLIMChiclet* chiclet = dynamic_cast<LLIMChiclet*>(ctrl);
	if (chiclet)
	{
		S32 x, y;
		LLRect rect = getRect();
		localPointToScreen(rect.getCenterX(), 0, &x, &y);
		LLIMFloater::show(chiclet->getIMSessionId(), x);
	}
	mCommitSignal(ctrl,param);
}

LLChiclet* LLChicletPanel::findIMChiclet(const LLUUID& im_session_id)
{
	chiclet_list_t::const_iterator it = mChicletList.begin();
	for( ; mChicletList.end() != it; ++it)
	{
		// Only IM Chiclets have session id, skip non IM Chiclets
		LLIMChiclet* chiclet = dynamic_cast<LLIMChiclet*>(*it);
		if(!chiclet)
		{
			continue;
		}

		if(chiclet->getIMSessionId() == im_session_id)
		{
			return chiclet;
		}
	}
	return NULL;
}

LLChiclet* LLChicletPanel::getChiclet(S32 pos)
{
	return mChicletList[pos];
}

void LLChicletPanel::removeChiclet(chiclet_list_t::iterator it)
{
	// if possible, after deletion shift chiclets right
	if(canScrollLeft() && !canScrollRight())
	{
		LLChiclet* chiclet = *it;
		LLRect first_chiclet_rect = getChiclet(0)->getRect();
		S32 deleted_chiclet_width = chiclet->getRect().getWidth();
		deleted_chiclet_width += CHICLET_PADDING;
		
		first_chiclet_rect.mLeft += deleted_chiclet_width;
		first_chiclet_rect.mRight += deleted_chiclet_width;

		getChiclet(0)->setRect(first_chiclet_rect);
	}

	mScrollArea->removeChild(*it);
	mChicletList.erase(it);
	
	arrange();
	showScrollButtonsIfNeeded();
}

void LLChicletPanel::removeChiclet(S32 pos)
{
	if(0 > pos || getChicletCount() <= pos)
	{
		return;
	}
	removeChiclet(mChicletList.begin() + pos);
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

void LLChicletPanel::removeIMChiclet(const LLUUID& im_session_id)
{
	chiclet_list_t::iterator it = mChicletList.begin();
	for( ; mChicletList.end() != it; ++it)
	{
		// Only IM Chiclets have session id, skip non IM Chiclets
		LLIMChiclet* chiclet = dynamic_cast<LLIMChiclet*>(*it);
		if(!chiclet)
		{
			continue;
		}

		if(chiclet->getIMSessionId() == im_session_id)
		{
			removeChiclet(it);
			return;
		}
	}
}

void LLChicletPanel::removeAll()
{
	mScrollArea->deleteAllChildren();

	mChicletList.erase(mChicletList.begin(), mChicletList.end());

	showScrollButtonsIfNeeded();
}

void LLChicletPanel::reshape(S32 width, S32 height, BOOL called_from_parent )
{
	LLPanel::reshape(width,height,called_from_parent);

	mLeftScroll->setRect(LLRect(0,CHICLET_HEIGHT,SCROLL_BUTTON_WIDTH,
		CHICLET_HEIGHT - SCROLL_BUTTON_HEIGHT));
	mRightScroll->setRect(LLRect(width-SCROLL_BUTTON_WIDTH,CHICLET_HEIGHT,
		width,CHICLET_HEIGHT - SCROLL_BUTTON_HEIGHT));

	S32 old_scroll_width = mScrollArea->getRect().getWidth();

	mScrollArea->setRect(LLRect(SCROLL_BUTTON_WIDTH + 5,CHICLET_HEIGHT + 1,
		width - SCROLL_BUTTON_WIDTH - 5, 0));

	S32 current_scroll_width = mScrollArea->getRect().getWidth();
	reshapeScrollArea(current_scroll_width - old_scroll_width);

	showScrollButtonsIfNeeded();
}

void LLChicletPanel::reshapeScrollArea(S32 delta_width)
{
	if(mChicletList.empty())
		return;

	S32 last_chiclet_right = (*mChicletList.rbegin())->getRect().mRight;
	S32 scroll_width = mScrollArea->getRect().getWidth();

	// Align all chiclets to last chiclet
	// if there is a gap between last chiclet and scroll area right side
	// or last chiclet is at visible area right side
	if( last_chiclet_right < scroll_width 
		|| last_chiclet_right == scroll_width - delta_width)
	{
		LLRect first_chiclet_rect = getChiclet(0)->getRect();
		// if we can right shift all chiclets
		if(first_chiclet_rect.mLeft < 0)
		{
			first_chiclet_rect.mLeft += delta_width;
			first_chiclet_rect.mRight += delta_width;

			getChiclet(0)->setRect(first_chiclet_rect);

			arrange();
		}
	}
}

void LLChicletPanel::arrange()
{
	if(mChicletList.empty())
		return;

	LLRect first_chiclet_rect = getChiclet(0)->getRect();
	// don't allow gap between first chiclet and scroll area left side
	if(first_chiclet_rect.mLeft > 0)
	{
		first_chiclet_rect.mRight = first_chiclet_rect.getWidth();
		first_chiclet_rect.mLeft = 0;
	}

	S32 left = first_chiclet_rect.mLeft;

	S32 size = getChicletCount();
	for( int n = 0; n < size; ++n)
	{
		LLChiclet* chiclet = getChiclet(n);
		S32 chiclet_width = chiclet->getRequiredRect().getWidth();
		LLRect rc(left, CHICLET_HEIGHT, left + chiclet_width, 0);

		chiclet->setRect(rc);

		left += chiclet_width + CHICLET_PADDING;
	}
}

void LLChicletPanel::showScrollButtonsIfNeeded()
{
	bool can_scroll_left = canScrollLeft();
	bool can_scroll_right = canScrollRight();

	mLeftScroll->setEnabled(can_scroll_left);
	mRightScroll->setEnabled(can_scroll_right);

	bool show_scroll_buttons = can_scroll_left || can_scroll_right;

	mLeftScroll->setVisible(show_scroll_buttons);
	mRightScroll->setVisible(show_scroll_buttons);
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

void LLChicletPanel::scroll(ScrollDirection direction)
{
	S32 first_visible_chiclet = getFirstVisibleChiclet();
	if(-1 == first_visible_chiclet)
		return;

	S32 offset = 0;

	if(SCROLL_LEFT == direction)
	{
		if(0 == first_visible_chiclet)
		{
			// shift chiclets in case first chiclet is partially visible
			offset = llabs(getChiclet(first_visible_chiclet)->getRect().mLeft);
		}
		else
		{
			offset = getChiclet(first_visible_chiclet - 1)->getRect().getWidth() + CHICLET_PADDING;
		}
	}
	else if(SCROLL_RIGHT == direction)
	{
		S32 last_chiclet_right = (*mChicletList.rbegin())->getRect().mRight;
		S32 scroll_rect_width = mScrollArea->getRect().getWidth();

		offset = getChiclet(first_visible_chiclet)->getRect().getWidth() + CHICLET_PADDING;
		offset *= direction;
		// if after scrolling, the last chiclet will not be aligned to 
		// scroll area right side - align it.
		if( last_chiclet_right + offset < scroll_rect_width )
		{
			offset = scroll_rect_width - last_chiclet_right;
		}
	}

	chiclet_list_t::const_iterator it = mChicletList.begin();
	for(;mChicletList.end() != it; ++it)
	{
		LLChiclet* chiclet = *it;
		chiclet->translate(offset,0);
	}
}

S32 LLChicletPanel::getFirstVisibleChiclet()
{
	if(mChicletList.empty())
		return -1;

	for(int n = 0; n < getChicletCount(); ++n)
	{
		LLRect rc = getChiclet(n)->getRect();
		if(n > 0)
			rc.mLeft -= CHICLET_PADDING;
		// bottom left of scroll area is first visible point
		if(rc.pointInRect(0,0))
		{
			return n;
		}
	}

	return -1;
}

void LLChicletPanel::scrollLeft()
{
	if(canScrollLeft())
	{
		scroll(SCROLL_LEFT);
		
		showScrollButtonsIfNeeded();
	}
}

void LLChicletPanel::scrollRight()
{
	if(canScrollRight())
	{
		scroll(SCROLL_RIGHT);
		
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

boost::signals2::connection LLChicletPanel::setChicletClickCallback(
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

LLTalkButton::LLTalkButton(const LLUICtrl::Params& p)
: LLUICtrl(p)
{
	static S32 DROPDOWN_BTN_WIDTH = 20;

	LLRect rc(p.rect);

	LLButton::Params speak_params;
	speak_params.name("left");
	speak_params.rect(LLRect(0,rc.getHeight(),rc.getWidth()-DROPDOWN_BTN_WIDTH,0));
	speak_params.label("Speak");
	speak_params.label_selected("Speak");
	speak_params.font(LLFontGL::getFontSansSerifSmall());
	speak_params.tab_stop(false);
	speak_params.is_toggle(true);
	speak_params.picture_style(true);
	speak_params.image_selected(LLUI::getUIImage("SegmentedBtn_Left_Selected")); 
	speak_params.image_unselected(LLUI::getUIImage("SegmentedBtn_Left_Off"));
	mSpeakBtn = LLUICtrlFactory::create<LLButton>(speak_params);
	addChild(mSpeakBtn);

	mSpeakBtn->setClickedCallback(boost::bind(&LLTalkButton::onClick_SpeakBtn, this));
	mSpeakBtn->setToggleState(false);

	LLButton::Params show_params;
	show_params.name("right");
	show_params.rect(LLRect(rc.getWidth()-DROPDOWN_BTN_WIDTH,rc.getHeight(),rc.getWidth(),0));
	show_params.label("");
	show_params.tab_stop(false);
	show_params.is_toggle(true);
	show_params.picture_style(true);
	show_params.image_selected(LLUI::getUIImage("ComboButton_Selected"));
	show_params.image_unselected(LLUI::getUIImage("ComboButton_Off"));
	mShowBtn = LLUICtrlFactory::create<LLButton>(show_params);
	addChild(mShowBtn);

	mShowBtn->setClickedCallback(boost::bind(&LLTalkButton::onClick_ShowBtn, this));
	mShowBtn->setToggleState(false);

	mSpeakBtn->setToggleState(FALSE);
	mShowBtn->setToggleState(FALSE);

	rc = mSpeakBtn->getRect();

	LLOutputMonitorCtrl::Params monitor_param;
	monitor_param.name("monitor");
	monitor_param.draw_border(false);
	monitor_param.rect(LLRect(rc.getWidth()-20,18,rc.getWidth()-3,2));
	monitor_param.visible(true);
	mOutputMonitor = LLUICtrlFactory::create<LLOutputMonitorCtrl>(monitor_param);

	mSpeakBtn->addChild(mOutputMonitor);

	mPrivateCallPanel = NULL;
}

LLTalkButton::~LLTalkButton()
{
}

void LLTalkButton::draw()
{
	if(mSpeakBtn->getToggleState())
	{
		mOutputMonitor->setPower(gVoiceClient->getCurrentPower(gAgent.getID()));
	}

	LLUICtrl::draw();
}

void LLTalkButton::onClick_SpeakBtn()
{
	bool speaking = mSpeakBtn->getToggleState();
	gVoiceClient->setUserPTTState(speaking);
	mOutputMonitor->setIsMuted(!speaking);
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

 	if(gBottomTray)
 		y = gBottomTray->getRect().getHeight() + mPrivateCallPanel->getRect().getHeight();

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
