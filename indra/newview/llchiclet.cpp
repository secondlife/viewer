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
static LLDefaultChildRegistry::Register<LLNotificationChiclet> t2("chiclet_notification");
static LLDefaultChildRegistry::Register<LLIMP2PChiclet> t3("chiclet_im_p2p");
static LLDefaultChildRegistry::Register<LLIMGroupChiclet> t4("chiclet_im_group");

static const LLRect CHICLET_RECT(0, 25, 25, 0);
static const LLRect CHICLET_ICON_RECT(0, 24, 24, 0);
static const LLRect VOICE_INDICATOR_RECT(25, 25, 45, 0);

// static
const S32 LLChicletPanel::s_scroll_ratio = 10;

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
, mShowSpeaker(false)
, mNewMessagesIcon(NULL)
, mSpeakerCtrl(NULL)
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
	overlay_icon_rect.translate(overlay_icon_rect.getWidth()/5, overlay_icon_rect.getHeight()/5);
	mNewMessagesIcon->setRect(overlay_icon_rect);
	addChild(mNewMessagesIcon);

	setShowCounter(false);
}

void LLIMChiclet::setShowSpeaker(bool show)
{
	bool needs_resize = getShowSpeaker() != show;
	if(needs_resize)
	{		
		mShowSpeaker = show;
		toggleSpeakerControl();
		onChicletSizeChanged();		
	}
}
void LLIMChiclet::initSpeakerControl()
{
	// virtual
}

void LLIMChiclet::toggleSpeakerControl()
{
	LLRect speaker_rect = mSpeakerCtrl->getRect();
	S32 required_width = getRect().getWidth();

	if(getShowSpeaker())
	{
		required_width = required_width + speaker_rect.getWidth();
		initSpeakerControl();		
	}
	else
	{
		required_width = required_width - speaker_rect.getWidth();
	}
	
	reshape(required_width, getRect().getHeight());
	mSpeakerCtrl->setVisible(getShowSpeaker());
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
	rect(CHICLET_RECT);

	avatar_icon.name("avatar_icon");
	avatar_icon.follows.flags(FOLLOWS_LEFT | FOLLOWS_TOP | FOLLOWS_BOTTOM);

	// *NOTE dzaporozhan
	// Changed icon height from 25 to 24 to fix ticket EXT-794.
	// In some cases(after changing UI scale) 25 pixel height icon was 
	// drawn incorrectly, i'm not sure why.
	avatar_icon.rect(CHICLET_ICON_RECT);
	avatar_icon.mouse_opaque(false);

	unread_notifications.name("unread");
	unread_notifications.font(LLFontGL::getFontSansSerif());
	unread_notifications.font_halign(LLFontGL::HCENTER);
	unread_notifications.v_pad(5);
	unread_notifications.text_color(LLColor4::white);
	unread_notifications.mouse_opaque(false);
	unread_notifications.visible(false);

	speaker.name("speaker");
	speaker.rect(VOICE_INDICATOR_RECT);
	speaker.auto_update(true);
	speaker.draw_border(false);

	show_speaker = false;
}

LLIMP2PChiclet::LLIMP2PChiclet(const Params& p)
: LLIMChiclet(p)
, mChicletIconCtrl(NULL)
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

void LLIMP2PChiclet::initSpeakerControl()
{
	mSpeakerCtrl->setSpeakerId(getOtherParticipantId());
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
	rect(CHICLET_RECT);

	avatar_icon.name("avatar_icon");
	avatar_icon.follows.flags(FOLLOWS_LEFT | FOLLOWS_TOP | FOLLOWS_BOTTOM);

	// *NOTE dzaporozhan
	// Changed icon height from 25 to 24 to fix ticket EXT-794.
	// In some cases(after changing UI scale) 25 pixel height icon was 
	// drawn incorrectly, i'm not sure why.
	avatar_icon.rect(CHICLET_ICON_RECT);
	avatar_icon.mouse_opaque(false);

	unread_notifications.name("unread");
	unread_notifications.font(LLFontGL::getFontSansSerif());
	unread_notifications.font_halign(LLFontGL::HCENTER);
	unread_notifications.v_pad(5);
	unread_notifications.text_color(LLColor4::white);
	unread_notifications.mouse_opaque(false);
	unread_notifications.visible(false);


	speaker.name("speaker");
	speaker.rect(VOICE_INDICATOR_RECT);
	speaker.auto_update(true);
	speaker.draw_border(false);

	show_speaker = false;
}

LLAdHocChiclet::LLAdHocChiclet(const Params& p)
: LLIMChiclet(p)
, mChicletIconCtrl(NULL)
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

void LLAdHocChiclet::draw()
{
	switchToCurrentSpeaker();
	LLIMChiclet::draw();
}

void LLAdHocChiclet::initSpeakerControl()
{
	switchToCurrentSpeaker();
}

void LLAdHocChiclet::switchToCurrentSpeaker()
{
	LLUUID speaker_id;
	LLSpeakerMgr::speaker_list_t speaker_list;

	LLIMModel::getInstance()->findIMSession(getSessionId())->mSpeakers->getSpeakerList(&speaker_list, FALSE);
	for (LLSpeakerMgr::speaker_list_t::iterator i = speaker_list.begin(); i != speaker_list.end(); ++i)
	{
		LLPointer<LLSpeaker> s = *i;
		if (s->mSpeechVolume > 0 || s->mStatus == LLSpeaker::STATUS_SPEAKING)
		{
			speaker_id = s->mID;
			break;
		}
	}

	mSpeakerCtrl->setSpeakerId(speaker_id);
}

void LLAdHocChiclet::setCounter(S32 counter)
{
	mCounterCtrl->setCounter(counter);
	setShowNewMessagesIcon(counter);
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
	rect(CHICLET_RECT);

	group_icon.name("group_icon");
	
	// *NOTE dzaporozhan
	// Changed icon height from 25 to 24 to fix ticket EXT-794.
	// In some cases(after changing UI scale) 25 pixel height icon was 
	// drawn incorrectly, i'm not sure why.
	group_icon.rect(CHICLET_ICON_RECT);

	unread_notifications.name("unread");
	unread_notifications.font(LLFontGL::getFontSansSerif());
	unread_notifications.font_halign(LLFontGL::HCENTER);
	unread_notifications.v_pad(5);
	unread_notifications.text_color(LLColor4::white);
	unread_notifications.visible(false);

	speaker.name("speaker");
	speaker.rect(VOICE_INDICATOR_RECT);
	speaker.auto_update(true);
	speaker.draw_border(false);

	show_speaker = false;
}

LLIMGroupChiclet::LLIMGroupChiclet(const Params& p)
: LLIMChiclet(p)
, LLGroupMgrObserver(LLUUID::null)
, mChicletIconCtrl(NULL)
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

void LLIMGroupChiclet::draw()
{
	switchToCurrentSpeaker();
	LLIMChiclet::draw();
}

void LLIMGroupChiclet::initSpeakerControl()
{
	switchToCurrentSpeaker();
}

void LLIMGroupChiclet::switchToCurrentSpeaker()
{
	LLUUID speaker_id;
	LLSpeakerMgr::speaker_list_t speaker_list;

	LLIMModel::getInstance()->findIMSession(getSessionId())->mSpeakers->getSpeakerList(&speaker_list, FALSE);
	for (LLSpeakerMgr::speaker_list_t::iterator i = speaker_list.begin(); i != speaker_list.end(); ++i)
	{
		LLPointer<LLSpeaker> s = *i;
		if (s->mSpeechVolume > 0 || s->mStatus == LLSpeaker::STATUS_SPEAKING)
		{
			speaker_id = s->mID;
			break;
		}
	}

	mSpeakerCtrl->setSpeakerId(speaker_id);
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


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

LLChicletPanel::Params::Params()
: chiclet_padding("chiclet_padding")
, scrolling_offset("scrolling_offset")
, min_width("min_width")
{
	chiclet_padding = 3;
	scrolling_offset = 40;

	if (!min_width.isProvided())
	{
		// min_width = 4 chiclets + 3 paddings
		min_width = 179 + 3*chiclet_padding;
	}
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
	LLVoiceChannel::setCurrentVoiceChannelChangedCallback(boost::bind(&LLChicletPanel::onCurrentVoiceChannelChanged, this, _1));

	mLeftScrollButton=getChild<LLButton>("chicklet_left_scroll_button");
	LLTransientFloaterMgr::getInstance()->addControlView(mLeftScrollButton);
	mLeftScrollButton->setMouseDownCallback(boost::bind(&LLChicletPanel::onLeftScrollClick,this));
	mLeftScrollButton->setHeldDownCallback(boost::bind(&LLChicletPanel::onLeftScrollHeldDown,this));
	mLeftScrollButton->setEnabled(false);

	mRightScrollButton=getChild<LLButton>("chicklet_right_scroll_button");
	LLTransientFloaterMgr::getInstance()->addControlView(mRightScrollButton);
	mRightScrollButton->setMouseDownCallback(boost::bind(&LLChicletPanel::onRightScrollClick,this));
	mRightScrollButton->setHeldDownCallback(boost::bind(&LLChicletPanel::onRightScrollHeldDown,this));
	mRightScrollButton->setEnabled(false);	

	return TRUE;
}

void LLChicletPanel::onCurrentVoiceChannelChanged(const LLUUID& session_id)
{
	for(chiclet_list_t::iterator it = mChicletList.begin(); it != mChicletList.end(); ++it)
	{
		LLIMChiclet* chiclet = dynamic_cast<LLIMChiclet*>(*it);
		if(chiclet)
		{
			if(chiclet->getSessionId() == session_id)
			{
				chiclet->setShowSpeaker(true);
				continue;
			}
			chiclet->setShowSpeaker(false);
		}
	}
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
	arrange();
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

	//Needed once- to avoid error at first call of reshape() before postBuild()
	if(!mLeftScrollButton||!mRightScrollButton)
		return;
	
	LLRect scroll_button_rect = mLeftScrollButton->getRect();
	mLeftScrollButton->setRect(LLRect(0,scroll_button_rect.mTop,scroll_button_rect.getWidth(),
		scroll_button_rect.mBottom));
	scroll_button_rect = mRightScrollButton->getRect();
	mRightScrollButton->setRect(LLRect(width - scroll_button_rect.getWidth(),scroll_button_rect.mTop,
		width, scroll_button_rect.mBottom));
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

void LLChicletPanel::onLeftScrollHeldDown()
{
	S32 offset = mScrollingOffset;
	mScrollingOffset = mScrollingOffset / s_scroll_ratio;
	scrollLeft();
	mScrollingOffset = offset;
}

void LLChicletPanel::onRightScrollHeldDown()
{
	S32 offset = mScrollingOffset;
	mScrollingOffset = mScrollingOffset / s_scroll_ratio;
	scrollRight();
	mScrollingOffset = offset;
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
 : LLOutputMonitorCtrl(p)
{
}
