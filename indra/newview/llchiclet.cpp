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
#include "llimfloater.h"
#include "llimview.h"
#include "llfloaterreg.h"
#include "lllocalcliprect.h"
#include "llmenugl.h"
#include "lloutputmonitorctrl.h"
#include "lltextbox.h"
#include "llvoiceclient.h"
#include "llvoicecontrolpanel.h"
#include "llgroupmgr.h"
#include "llnotificationmanager.h"
#include "lltransientfloatermgr.h"

static LLDefaultChildRegistry::Register<LLChicletPanel> t1("chiclet_panel");
static LLDefaultChildRegistry::Register<LLTalkButton> t2("chiclet_talk");
static LLDefaultChildRegistry::Register<LLNotificationChiclet> t3("chiclet_notification");
static LLDefaultChildRegistry::Register<LLIMP2PChiclet> t4("chiclet_im_p2p");
static LLDefaultChildRegistry::Register<LLIMGroupChiclet> t5("chiclet_im_group");

S32 LLNotificationChiclet::mUreadSystemNotifications = 0;

boost::signals2::signal<LLChiclet* (const LLUUID&),
		LLIMChiclet::CollectChicletCombiner<std::list<LLChiclet*> > >
		LLIMChiclet::sFindChicletsSignal;
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

	// connect counter handlers to the signals
	connectCounterUpdatersToSignal("notify");
	connectCounterUpdatersToSignal("groupnotify");
}

LLNotificationChiclet::~LLNotificationChiclet()
{

}

void LLNotificationChiclet::connectCounterUpdatersToSignal(std::string notification_type)
{
	LLNotificationsUI::LLNotificationManager* manager = LLNotificationsUI::LLNotificationManager::getInstance();
	LLNotificationsUI::LLEventHandler* n_handler = manager->getHandlerForNotification(notification_type);
	if(n_handler)
	{
		n_handler->setNewNotificationCallback(boost::bind(&LLNotificationChiclet::incUreadSystemNotifications, this));
		n_handler->setDelNotification(boost::bind(&LLNotificationChiclet::decUreadSystemNotifications, this));
	}
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

void LLNotificationChiclet::setToggleState(BOOL toggled) {
	mButton->setToggleState(toggled);
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

LLIMChiclet::LLIMChiclet(const LLIMChiclet::Params& p)
: LLChiclet(p)
, mNewMessagesIcon(NULL)
, mCounterCtrl(NULL)
{
	// initialize an overlay icon for new messages
	LLIconCtrl::Params icon_params;
	icon_params.visible = false;
	icon_params.image = LLUI::getUIImage(p.new_messages_icon_name);
	mNewMessagesIcon = LLUICtrlFactory::create<LLIconCtrl>(icon_params);
	// adjust size and position of an icon
	LLRect chiclet_rect = p.rect;
	LLRect overlay_icon_rect = LLRect(chiclet_rect.getWidth()/2, chiclet_rect.mTop, chiclet_rect.mRight, chiclet_rect.getHeight()/2); 
	// shift an icon a little bit to the right and up corner of a chiclet
	overlay_icon_rect.translate(overlay_icon_rect.getWidth()/4, overlay_icon_rect.getHeight()/4);
	mNewMessagesIcon->setRect(overlay_icon_rect);
	addChild(mNewMessagesIcon);

	setShowCounter(false);
}

void LLIMChiclet::setShowNewMessagesIcon(bool show)
{
	if(mNewMessagesIcon)
	{
		mNewMessagesIcon->setVisible(show);
	}
}

bool LLIMChiclet::getShowNewMessagesIcon()
{
	return mNewMessagesIcon->getVisible();
}

void LLIMChiclet::onMouseDown()
{
	LLIMFloater::toggle(getSessionId());
	setCounter(0);
}

BOOL LLIMChiclet::handleMouseDown(S32 x, S32 y, MASK mask)
{
	onMouseDown();
	return LLChiclet::handleMouseDown(x, y, mask);
}

void LLIMChiclet::draw()
{
	LLUICtrl::draw();

	gl_rect_2d(0, getRect().getHeight(), getRect().getWidth(), 0, LLColor4(0.0f,0.0f,0.0f,1.f), FALSE);
}

// static
LLIMChiclet::EType LLIMChiclet::getIMSessionType(const LLUUID& session_id)
{
	EType				type	= TYPE_UNKNOWN;

	if(session_id.isNull())
		return type;

	EInstantMessage im_type = LLIMModel::getInstance()->getType(session_id);
	if (IM_COUNT == im_type)
	{
		llassert_always(0 && "IM session not found"); // should never happen
		return type;
	}

	switch(im_type)
	{
	case IM_NOTHING_SPECIAL:
	case IM_SESSION_P2P_INVITE:
		type = TYPE_IM;
		break;
	case IM_SESSION_GROUP_START:
	case IM_SESSION_INVITE:
		if (gAgent.isInGroup(session_id))
		{
			type = TYPE_GROUP;
		}
		else
		{
			type = TYPE_AD_HOC;
		}
		break;
	case IM_SESSION_CONFERENCE_START:
		type = TYPE_AD_HOC;
	default:
		break;
	}

	return type;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

LLIMP2PChiclet::Params::Params()
: avatar_icon("avatar_icon")
, unread_notifications("unread_notifications")
, speaker("speaker")
, show_speaker("show_speaker")
{
	// *TODO Vadim: Get rid of hardcoded values.
	rect(LLRect(0, 25, 25, 0));

	avatar_icon.name("avatar_icon");
	avatar_icon.follows.flags(FOLLOWS_LEFT | FOLLOWS_TOP | FOLLOWS_BOTTOM);

	// *NOTE dzaporozhan
	// Changed icon height from 25 to 24 to fix ticket EXT-794.
	// In some cases(after changing UI scale) 25 pixel height icon was 
	// drawn incorrectly, i'm not sure why.
	avatar_icon.rect(LLRect(0, 24, 25, 0));
	avatar_icon.mouse_opaque(false);

	unread_notifications.name("unread");
	unread_notifications.rect(LLRect(25, 25, 45, 0));
	unread_notifications.font(LLFontGL::getFontSansSerif());
	unread_notifications.font_halign(LLFontGL::HCENTER);
	unread_notifications.v_pad(5);
	unread_notifications.text_color(LLColor4::white);
	unread_notifications.mouse_opaque(false);
	unread_notifications.visible(false);

	speaker.name("speaker");
	speaker.rect(LLRect(45, 25, 65, 0));

	show_speaker = false;
}

LLIMP2PChiclet::LLIMP2PChiclet(const Params& p)
: LLIMChiclet(p)
, mChicletIconCtrl(NULL)
, mSpeakerCtrl(NULL)
, mPopupMenu(NULL)
{
	LLChicletAvatarIconCtrl::Params avatar_params = p.avatar_icon;
	mChicletIconCtrl = LLUICtrlFactory::create<LLChicletAvatarIconCtrl>(avatar_params);
	addChild(mChicletIconCtrl);

	LLChicletNotificationCounterCtrl::Params unread_params = p.unread_notifications;
	mCounterCtrl = LLUICtrlFactory::create<LLChicletNotificationCounterCtrl>(unread_params);
	addChild(mCounterCtrl);

	setCounter(getCounter());
	setShowCounter(getShowCounter());

	LLChicletSpeakerCtrl::Params speaker_params = p.speaker;
	mSpeakerCtrl = LLUICtrlFactory::create<LLChicletSpeakerCtrl>(speaker_params);
	addChild(mSpeakerCtrl);

	sendChildToFront(mNewMessagesIcon);
	setShowSpeaker(p.show_speaker);
}

void LLIMP2PChiclet::setCounter(S32 counter)
{
	mCounterCtrl->setCounter(counter);
	setShowNewMessagesIcon(counter);
}

LLRect LLIMP2PChiclet::getRequiredRect()
{
	LLRect rect(0, 0, mChicletIconCtrl->getRect().getWidth(), 0);
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

void LLIMP2PChiclet::setOtherParticipantId(const LLUUID& other_participant_id)
{
	LLIMChiclet::setOtherParticipantId(other_participant_id);
	mChicletIconCtrl->setValue(getOtherParticipantId());
}

void LLIMP2PChiclet::updateMenuItems()
{
	if(!mPopupMenu)
		return;
	if(getSessionId().isNull())
		return;

	bool is_friend = LLAvatarActions::isFriend(getOtherParticipantId());

	mPopupMenu->getChild<LLUICtrl>("Add Friend")->setEnabled(!is_friend);
	mPopupMenu->getChild<LLUICtrl>("Remove Friend")->setEnabled(is_friend);
}

BOOL LLIMP2PChiclet::handleRightMouseDown(S32 x, S32 y, MASK mask)
{
	if(!mPopupMenu)
	{
		createPopupMenu();
	}

	if (mPopupMenu)
	{
		updateMenuItems();
		mPopupMenu->arrangeAndClear();
		LLMenuGL::showPopup(this, mPopupMenu, x, y);
	}

	return TRUE;
}

void LLIMP2PChiclet::createPopupMenu()
{
	if(mPopupMenu)
	{
		llwarns << "Menu already exists" << llendl;
		return;
	}
	if(getSessionId().isNull())
	{
		return;
	}

	LLUICtrl::CommitCallbackRegistry::ScopedRegistrar registrar;
	registrar.add("IMChicletMenu.Action", boost::bind(&LLIMP2PChiclet::onMenuItemClicked, this, _2));

	mPopupMenu = LLUICtrlFactory::getInstance()->createFromFile<LLMenuGL>
		("menu_imchiclet_p2p.xml", gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());
}

void LLIMP2PChiclet::onMenuItemClicked(const LLSD& user_data)
{
	std::string level = user_data.asString();
	LLUUID other_participant_id = getOtherParticipantId();

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
		LLAvatarActions::requestFriendshipDialog(other_participant_id);
	}
}

void LLIMP2PChiclet::setShowSpeaker(bool show)
{
	LLIMChiclet::setShowSpeaker(show);

	bool needs_resize = getShowSpeaker() != show;
	mSpeakerCtrl->setVisible(getShowSpeaker());
	if(needs_resize)
	{
		onChicletSizeChanged();
	}
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

LLAdHocChiclet::Params::Params()
: avatar_icon("avatar_icon")
, unread_notifications("unread_notifications")
, speaker("speaker")
, show_speaker("show_speaker")
, avatar_icon_color("avatar_icon_color", LLColor4::green)
{
	// *TODO Vadim: Get rid of hardcoded values.
	rect(LLRect(0, 25, 25, 0));

	avatar_icon.name("avatar_icon");
	avatar_icon.follows.flags(FOLLOWS_LEFT | FOLLOWS_TOP | FOLLOWS_BOTTOM);

	// *NOTE dzaporozhan
	// Changed icon height from 25 to 24 to fix ticket EXT-794.
	// In some cases(after changing UI scale) 25 pixel height icon was 
	// drawn incorrectly, i'm not sure why.
	avatar_icon.rect(LLRect(0, 24, 25, 0));
	avatar_icon.mouse_opaque(false);

	unread_notifications.name("unread");
	unread_notifications.rect(LLRect(25, 25, 45, 0));
	unread_notifications.font(LLFontGL::getFontSansSerif());
	unread_notifications.font_halign(LLFontGL::HCENTER);
	unread_notifications.v_pad(5);
	unread_notifications.text_color(LLColor4::white);
	unread_notifications.mouse_opaque(false);
	unread_notifications.visible(false);


	speaker.name("speaker");
	speaker.rect(LLRect(45, 25, 65, 0));

	show_speaker = false;
}

LLAdHocChiclet::LLAdHocChiclet(const Params& p)
: LLIMChiclet(p)
, mChicletIconCtrl(NULL)
, mSpeakerCtrl(NULL)
, mPopupMenu(NULL)
{
	LLChicletAvatarIconCtrl::Params avatar_params = p.avatar_icon;
	mChicletIconCtrl = LLUICtrlFactory::create<LLChicletAvatarIconCtrl>(avatar_params);
	//Make the avatar modified
	mChicletIconCtrl->setColor(p.avatar_icon_color);
	addChild(mChicletIconCtrl);

	LLChicletNotificationCounterCtrl::Params unread_params = p.unread_notifications;
	mCounterCtrl = LLUICtrlFactory::create<LLChicletNotificationCounterCtrl>(unread_params);
	addChild(mCounterCtrl);

	setCounter(getCounter());
	setShowCounter(getShowCounter());

	LLChicletSpeakerCtrl::Params speaker_params = p.speaker;
	mSpeakerCtrl = LLUICtrlFactory::create<LLChicletSpeakerCtrl>(speaker_params);
	addChild(mSpeakerCtrl);

	sendChildToFront(mNewMessagesIcon);
	setShowSpeaker(p.show_speaker);
}

void LLAdHocChiclet::setSessionId(const LLUUID& session_id)
{
	LLChiclet::setSessionId(session_id);
	LLIMModel::LLIMSession* im_session = LLIMModel::getInstance()->findIMSession(session_id);
	mChicletIconCtrl->setValue(im_session->mOtherParticipantID);
}

void LLAdHocChiclet::setCounter(S32 counter)
{
	mCounterCtrl->setCounter(counter);
	setShowNewMessagesIcon(counter);
}

LLRect LLAdHocChiclet::getRequiredRect()
{
	LLRect rect(0, 0, mChicletIconCtrl->getRect().getWidth(), 0);
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

BOOL LLAdHocChiclet::handleRightMouseDown(S32 x, S32 y, MASK mask)
{
	return TRUE;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

LLIMGroupChiclet::Params::Params()
: group_icon("group_icon")
{
	rect(LLRect(0, 25, 25, 0));

	group_icon.name("group_icon");
	
	// *NOTE dzaporozhan
	// Changed icon height from 25 to 24 to fix ticket EXT-794.
	// In some cases(after changing UI scale) 25 pixel height icon was 
	// drawn incorrectly, i'm not sure why.
	group_icon.rect(LLRect(0, 24, 25, 0));

	unread_notifications.name("unread");
	unread_notifications.rect(LLRect(25, 25, 45, 0));
	unread_notifications.font(LLFontGL::getFontSansSerif());
	unread_notifications.font_halign(LLFontGL::HCENTER);
	unread_notifications.v_pad(5);
	unread_notifications.text_color(LLColor4::white);
	unread_notifications.visible(false);

	speaker.name("speaker");
	speaker.rect(LLRect(45, 25, 65, 0));

	show_speaker = false;
}

LLIMGroupChiclet::LLIMGroupChiclet(const Params& p)
: LLIMChiclet(p)
, LLGroupMgrObserver(LLUUID::null)
, mChicletIconCtrl(NULL)
, mSpeakerCtrl(NULL)
, mPopupMenu(NULL)
{
	LLChicletGroupIconCtrl::Params avatar_params = p.group_icon;
	mChicletIconCtrl = LLUICtrlFactory::create<LLChicletGroupIconCtrl>(avatar_params);
	addChild(mChicletIconCtrl);

	LLChicletNotificationCounterCtrl::Params unread_params = p.unread_notifications;
	mCounterCtrl = LLUICtrlFactory::create<LLChicletNotificationCounterCtrl>(unread_params);
	addChild(mCounterCtrl);

	setCounter(getCounter());
	setShowCounter(getShowCounter());

	LLChicletSpeakerCtrl::Params speaker_params = p.speaker;
	mSpeakerCtrl = LLUICtrlFactory::create<LLChicletSpeakerCtrl>(speaker_params);
	addChild(mSpeakerCtrl);

	sendChildToFront(mNewMessagesIcon);
	setShowSpeaker(p.show_speaker);
}

LLIMGroupChiclet::~LLIMGroupChiclet()
{
	LLGroupMgr::getInstance()->removeObserver(this);
}

void LLIMGroupChiclet::setCounter(S32 counter)
{
	mCounterCtrl->setCounter(counter);
	setShowNewMessagesIcon(counter);
}

LLRect LLIMGroupChiclet::getRequiredRect()
{
	LLRect rect(0, 0, mChicletIconCtrl->getRect().getWidth(), 0);
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

void LLIMGroupChiclet::setSessionId(const LLUUID& session_id)
{
	LLChiclet::setSessionId(session_id);

	LLGroupMgr* grp_mgr = LLGroupMgr::getInstance();
	LLGroupMgrGroupData* group_data = grp_mgr->getGroupData(session_id);
	if (group_data && group_data->mInsigniaID.notNull())
	{
		mChicletIconCtrl->setValue(group_data->mInsigniaID);
	}
	else
	{
		if(getSessionId() != mID)
		{
			grp_mgr->removeObserver(this);
			mID = getSessionId();
			grp_mgr->addObserver(this);
		}
		grp_mgr->sendGroupPropertiesRequest(session_id);
	}
}

void LLIMGroupChiclet::changed(LLGroupChange gc)
{
	if (GC_PROPERTIES == gc)
	{
		LLGroupMgrGroupData* group_data = LLGroupMgr::getInstance()->getGroupData(getSessionId());
		if (group_data)
		{
			mChicletIconCtrl->setValue(group_data->mInsigniaID);
		}
	}
}

BOOL LLIMGroupChiclet::handleRightMouseDown(S32 x, S32 y, MASK mask)
{
	if(!mPopupMenu)
	{
		createPopupMenu();
	}

	if (mPopupMenu)
	{
		mPopupMenu->arrangeAndClear();
		LLMenuGL::showPopup(this, mPopupMenu, x, y);
	}

	return TRUE;
}

void LLIMGroupChiclet::createPopupMenu()
{
	if(mPopupMenu)
	{
		llwarns << "Menu already exists" << llendl;
		return;
	}
	if(getSessionId().isNull())
	{
		return;
	}

	LLUICtrl::CommitCallbackRegistry::ScopedRegistrar registrar;
	registrar.add("IMChicletMenu.Action", boost::bind(&LLIMGroupChiclet::onMenuItemClicked, this, _2));

	mPopupMenu = LLUICtrlFactory::getInstance()->createFromFile<LLMenuGL>
		("menu_imchiclet_group.xml", gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());
}

void LLIMGroupChiclet::onMenuItemClicked(const LLSD& user_data)
{
	std::string level = user_data.asString();
	LLUUID group_id = getSessionId();

	if("group chat" == level)
	{
		LLGroupActions::startChat(group_id);
	}
	else if("info" == level)
	{
		LLGroupActions::show(group_id);
	}
}

void LLIMGroupChiclet::setShowSpeaker(bool show)
{
	LLIMChiclet::setShowSpeaker(show);

	bool needs_resize = getShowSpeaker() != show;
	mSpeakerCtrl->setVisible(getShowSpeaker());
	if(needs_resize)
	{
		onChicletSizeChanged();
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
, min_width("min_width")
{
	chiclet_padding = 3;
	scrolling_offset = 40;

	if (!min_width.isProvided())
	{
		// min_width = 4 chiclets + 3 paddings
		min_width = 179 + 3*chiclet_padding;
	}

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
, mMinWidth(p.min_width)
, mShowControls(true)
{
	LLButton::Params scroll_button_params = p.left_scroll_button;

	mLeftScrollButton = LLUICtrlFactory::create<LLButton>(scroll_button_params);
	addChild(mLeftScrollButton);
	LLTransientFloaterMgr::getInstance()->addControlView(mLeftScrollButton);

	mLeftScrollButton->setClickedCallback(boost::bind(&LLChicletPanel::onLeftScrollClick,this));
	mLeftScrollButton->setEnabled(false);

	scroll_button_params = p.right_scroll_button;
	mRightScrollButton = LLUICtrlFactory::create<LLButton>(scroll_button_params);
	addChild(mRightScrollButton);
	LLTransientFloaterMgr::getInstance()->addControlView(mRightScrollButton);

	mRightScrollButton->setClickedCallback(boost::bind(&LLChicletPanel::onRightScrollClick,this));
	mRightScrollButton->setEnabled(false);

	LLPanel::Params panel_params;
	mScrollArea = LLUICtrlFactory::create<LLPanel>(panel_params,this);

	// important for Show/Hide Camera and Move controls menu in bottom tray to work properly
	mScrollArea->setMouseOpaque(false);

	addChild(mScrollArea);
}

LLChicletPanel::~LLChicletPanel()
{

}

void im_chiclet_callback(LLChicletPanel* panel, const LLSD& data){
	
	LLUUID session_id = data["session_id"].asUUID();
	S32 unread = data["num_unread"].asInteger();

	LLIMFloater* im_floater = LLIMFloater::findInstance(session_id);
	if (im_floater && im_floater->getVisible())
	{
		unread = 0;
	}

	std::list<LLChiclet*> chiclets = LLIMChiclet::sFindChicletsSignal(session_id);
	std::list<LLChiclet *>::iterator iter;
	for (iter = chiclets.begin(); iter != chiclets.end(); iter++) {
		LLChiclet* chiclet = *iter;
		if (chiclet != NULL)
		{
			chiclet->setCounter(unread);
		}
	    else
	    {
	    	llwarns << "Unable to set counter for chiclet " << session_id << llendl;
	    }
	}

}


BOOL LLChicletPanel::postBuild()
{
	LLPanel::postBuild();
	LLIMModel::instance().addNewMsgCallback(boost::bind(im_chiclet_callback, this, _1));
	LLIMModel::instance().addNoUnreadMsgsCallback(boost::bind(im_chiclet_callback, this, _1));
	LLIMChiclet::sFindChicletsSignal.connect(boost::bind(&LLChicletPanel::findChiclet<LLChiclet>, this, _1));

	return TRUE;
}

S32 LLChicletPanel::calcChickletPanleWidth()
{
	S32 res = 0;

	for (chiclet_list_t::iterator it = mChicletList.begin(); it
			!= mChicletList.end(); it++)
	{
		res = (*it)->getRect().getWidth() + getChicletPadding();
	}
	return res;
}

bool LLChicletPanel::addChiclet(LLChiclet* chiclet, S32 index)
{
	if(mScrollArea->addChild(chiclet))
	{
		// chicklets should be aligned to right edge of scroll panel
		S32 offset = 0;

		if (!canScrollLeft())
		{
			offset = mScrollArea->getRect().getWidth()
					- chiclet->getRect().getWidth() - calcChickletPanleWidth();
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

void LLChicletPanel::scrollToChiclet(const LLChiclet* chiclet)
{
	const LLRect& rect = chiclet->getRect();

	if (rect.mLeft < 0)
	{
		scroll(llabs(rect.mLeft));
		showScrollButtonsIfNeeded();
	}
	else
	{
		S32 scrollWidth = mScrollArea->getRect().getWidth();

		if (rect.mRight > scrollWidth)
		{
			scroll(-llabs(rect.mRight - scrollWidth));
			showScrollButtonsIfNeeded();
		}
	}
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
		height, width - scroll_button_rect.getWidth() - SCROLL_BUTTON_PAD, 0));

	mShowControls = width > mMinWidth;
	mScrollArea->setVisible(mShowControls);

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
	if(!mChicletList.empty())
	{
		S32 last_chiclet_right = (*mChicletList.rbegin())->getRect().mRight;
		S32 first_chiclet_left = getChiclet(0)->getRect().mLeft;
		S32 scroll_width = mScrollArea->getRect().getWidth();
		if(last_chiclet_right < scroll_width || first_chiclet_left > 0)
		{
			shiftChiclets(scroll_width - last_chiclet_right);
		}
	}
}

void LLChicletPanel::showScrollButtonsIfNeeded()
{
	bool can_scroll_left = canScrollLeft();
	bool can_scroll_right = canScrollRight();

	mLeftScrollButton->setEnabled(can_scroll_left);
	mRightScrollButton->setEnabled(can_scroll_right);

	bool show_scroll_buttons = (can_scroll_left || can_scroll_right) && mShowControls;

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

bool LLChicletPanel::isAnyIMFloaterDoked()
{
	bool res = false;
	for (chiclet_list_t::iterator it = mChicletList.begin(); it
			!= mChicletList.end(); it++)
	{
		LLIMFloater* im_floater = LLFloaterReg::findTypedInstance<LLIMFloater>(
				"impanel", (*it)->getSessionId());
		if (im_floater != NULL && im_floater->getVisible()
				&& !im_floater->isMinimized() && im_floater->isDocked())
		{
			res = true;
			break;
		}
	}

	return res;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// *TODO Vadim: Move this out of llchiclet.cpp.

LLTalkButton::Params::Params()
 : speak_button("speak_button")
 , show_button("show_button")
 , monitor("monitor")
{
	// *TODO Vadim: move hardcoded labels (!) and other params to XUI.
	speak_button.name("left");
	speak_button.label("Speak");
	speak_button.label_selected("Speak");
	speak_button.font(LLFontGL::getFontSansSerifSmall());
	speak_button.tab_stop(false);
	speak_button.is_toggle(true);
	speak_button.picture_style(true);
	// Use default button art. JC
	//speak_button.image_selected(LLUI::getUIImage("SegmentedBtn_Left_Selected"));
	//speak_button.image_unselected(LLUI::getUIImage("SegmentedBtn_Left_Off"));

	show_button.name("right");
	show_button.label(LLStringUtil::null);
	show_button.rect(LLRect(0, 0, 20, 0));
	show_button.tab_stop(false);
	show_button.is_toggle(true);
	show_button.picture_style(true);
	show_button.image_selected(LLUI::getUIImage("ComboButton_Selected"));
	show_button.image_unselected(LLUI::getUIImage("ComboButton_Off"));

	monitor.name("monitor");
	// *TODO: Make this data driven.
	monitor.rect(LLRect(0, 18, 18, 0));
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
	LLTransientFloaterMgr::getInstance()->addControlView(mSpeakBtn);

	mSpeakBtn->setClickedCallback(boost::bind(&LLTalkButton::onClick_SpeakBtn, this));
	mSpeakBtn->setToggleState(FALSE);

	LLButton::Params show_params = p.show_button;
	show_params.rect(show_rect);
	mShowBtn = LLUICtrlFactory::create<LLButton>(show_params);
	addChild(mShowBtn);
	LLTransientFloaterMgr::getInstance()->addControlView(mShowBtn);

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
	monitor_params.auto_update(true);
	monitor_params.speaker_id(gAgentID);
	mOutputMonitor = LLUICtrlFactory::create<LLOutputMonitorCtrl>(monitor_params);
	mSpeakBtn->addChild(mOutputMonitor);

	// never show "muted" because you can't mute yourself
	mOutputMonitor->setIsMuted(false);
	mOutputMonitor->setIsAgentControl(true);
}

LLTalkButton::~LLTalkButton()
{
}

void LLTalkButton::setSpeakBtnToggleState(bool state)
{
	mSpeakBtn->setToggleState(state);
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


	LLAvatarListItem* item = new LLAvatarListItem();
	item->showLastInteractionTime(false);
	item->showInfoBtn(true);
	item->showSpeakingIndicator(true);
	item->reshape(mPrivateCallPanel->getRect().getWidth(), item->getRect().getHeight(), FALSE);

	mPrivateCallPanel->addItem(item);
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
	if(mCounter != 0)
	{
		setText(stream.str());
	}
	else
	{
		setText(std::string(""));
	}
}

LLRect LLChicletNotificationCounterCtrl::getRequiredRect()
{
	LLRect rc;
	S32 text_width = getContentsRect().getWidth();

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

LLChicletGroupIconCtrl::LLChicletGroupIconCtrl(const Params& p)
: LLIconCtrl(p)
, mDefaultIcon(p.default_icon)
{
}

void LLChicletGroupIconCtrl::setValue(const LLSD& value )
{
	if(value.asUUID().isNull())
	{
		LLIconCtrl::setValue(mDefaultIcon);
	}
	else
	{
		LLIconCtrl::setValue(value);
	}
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

LLChicletSpeakerCtrl::LLChicletSpeakerCtrl(const Params&p)
 : LLIconCtrl(p)
{
}
