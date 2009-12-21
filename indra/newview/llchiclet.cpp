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
#include "llnotifications.h"
#include "llnotificationsutil.h"
#include "lloutputmonitorctrl.h"
#include "llscriptfloater.h"
#include "lltextbox.h"
#include "llvoiceclient.h"
#include "llvoicecontrolpanel.h"
#include "llgroupmgr.h"
#include "llnotificationmanager.h"
#include "lltransientfloatermgr.h"
#include "llsyswellwindow.h"

static LLDefaultChildRegistry::Register<LLChicletPanel> t1("chiclet_panel");
static LLDefaultChildRegistry::Register<LLIMWellChiclet> t2_0("chiclet_im_well");
static LLDefaultChildRegistry::Register<LLNotificationChiclet> t2("chiclet_notification");
static LLDefaultChildRegistry::Register<LLIMP2PChiclet> t3("chiclet_im_p2p");
static LLDefaultChildRegistry::Register<LLIMGroupChiclet> t4("chiclet_im_group");
static LLDefaultChildRegistry::Register<LLAdHocChiclet> t5("chiclet_im_adhoc");
static LLDefaultChildRegistry::Register<LLScriptChiclet> t6("chiclet_script");
static LLDefaultChildRegistry::Register<LLInvOfferChiclet> t7("chiclet_offer");

static const LLRect CHICLET_RECT(0, 25, 25, 0);
static const LLRect CHICLET_ICON_RECT(0, 22, 22, 0);
static const LLRect VOICE_INDICATOR_RECT(50, 25, 70, 0);
static const LLRect COUNTER_RECT(25, 25, 50, 0);
static const S32 OVERLAY_ICON_SHIFT = 2;	// used for shifting of an overlay icon for new massages in a chiclet
static const S32 SCROLL_BUTTON_PAD = 5;

// static
const S32 LLChicletPanel::s_scroll_ratio = 10;
const S32 LLChicletNotificationCounterCtrl::MAX_DISPLAYED_COUNT = 99;


boost::signals2::signal<LLChiclet* (const LLUUID&),
		LLIMChiclet::CollectChicletCombiner<std::list<LLChiclet*> > >
		LLIMChiclet::sFindChicletsSignal;
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/**
 * Updates the Well's 'Lit' state to flash it when "new messages" are come.
 *
 * It gets callback which will be called 2*N times with passed period. See EXT-3147
 */
class LLSysWellChiclet::FlashToLitTimer : public LLEventTimer
{
public:
	typedef boost::function<void()> callback_t;

	/**
	 * Constructor.
	 *
	 * @param count - how many times callback should be called (twice to not change original state)
	 * @param period - how frequently callback should be called
	 * @param cb - callback to be called each tick
	 */
	FlashToLitTimer(S32 count, F32 period, callback_t cb)
		: LLEventTimer(period)
		, mCallback(cb)
		, mFlashCount(2 * count)
		, mCurrentFlashCount(0)
	{
		mEventTimer.stop();
	}

	BOOL tick()
	{
		mCallback();

		if (++mCurrentFlashCount == mFlashCount) mEventTimer.stop();
		return FALSE;
	}

	void flash()
	{
		mCurrentFlashCount = 0;
		mEventTimer.start();
	}

	void stopFlashing()
	{
		mEventTimer.stop();
	}

private:
	callback_t		mCallback;

	/**
	 * How many times Well will blink.
	 */
	S32 mFlashCount;
	S32 mCurrentFlashCount;
};

LLSysWellChiclet::Params::Params()
: button("button")
, unread_notifications("unread_notifications")
, max_displayed_count("max_displayed_count", 99)
, flash_to_lit_count("flash_to_lit_count", 3)
, flash_period("flash_period", 0.5F)
{
	button.name("button");
	button.tab_stop(FALSE);
	button.label(LLStringUtil::null);
}

LLSysWellChiclet::LLSysWellChiclet(const Params& p)
: LLChiclet(p)
, mButton(NULL)
, mCounter(0)
, mMaxDisplayedCount(p.max_displayed_count)
, mIsNewMessagesState(false)
, mFlashToLitTimer(NULL)
, mContextMenu(NULL)
{
	LLButton::Params button_params = p.button;
	mButton = LLUICtrlFactory::create<LLButton>(button_params);
	addChild(mButton);

	mFlashToLitTimer = new FlashToLitTimer(p.flash_to_lit_count, p.flash_period, boost::bind(&LLSysWellChiclet::changeLitState, this));
}

LLSysWellChiclet::~LLSysWellChiclet()
{
	delete mFlashToLitTimer;
}

void LLSysWellChiclet::setCounter(S32 counter)
{
	// note same code in LLChicletNotificationCounterCtrl::setCounter(S32 counter)
	std::string s_count;
	if(counter != 0)
	{
		static std::string more_messages_exist("+");
		std::string more_messages(counter > mMaxDisplayedCount ? more_messages_exist : "");
		s_count = llformat("%d%s"
			, llmin(counter, mMaxDisplayedCount)
			, more_messages.c_str()
			);
	}

	mButton->setLabel(s_count);

	setNewMessagesState(counter > 0);

	// we have to flash to 'Lit' state each time new unread message is comming.
	if (counter > mCounter)
	{
		mFlashToLitTimer->flash();
	}
	else if (counter == 0)
	{
		// if notification is resolved while well is flashing it can leave in the 'Lit' state
		// when flashing finishes itself. Let break flashing here.
		mFlashToLitTimer->stopFlashing();
	}

	mCounter = counter;
}

boost::signals2::connection LLSysWellChiclet::setClickCallback(
	const commit_callback_t& cb)
{
	return mButton->setClickedCallback(cb);
}

void LLSysWellChiclet::setToggleState(BOOL toggled) {
	mButton->setToggleState(toggled);
}

void LLSysWellChiclet::changeLitState()
{
	setNewMessagesState(!mIsNewMessagesState);
}

void LLSysWellChiclet::setNewMessagesState(bool new_messages)
{
	/*
	Emulate 4 states of button by background images, see detains in EXT-3147
	xml attribute           Description
	image_unselected        "Unlit" - there are no new messages
	image_selected          "Unlit" + "Selected" - there are no new messages and the Well is open
	image_pressed           "Lit" - there are new messages
	image_pressed_selected  "Lit" + "Selected" - there are new messages and the Well is open
	*/
	mButton->setForcePressedState(new_messages);

	mIsNewMessagesState = new_messages;
}

// virtual
BOOL LLSysWellChiclet::handleRightMouseDown(S32 x, S32 y, MASK mask)
{
	if(!mContextMenu)
	{
		createMenu();
	}
	if (mContextMenu)
	{
		mContextMenu->show(x, y);
		LLMenuGL::showPopup(this, mContextMenu, x, y);
	}
	return TRUE;
}

/************************************************************************/
/*               LLIMWellChiclet implementation                         */
/************************************************************************/
LLIMWellChiclet::LLIMWellChiclet(const Params& p)
: LLSysWellChiclet(p)
{
	LLIMModel::instance().addNewMsgCallback(boost::bind(&LLIMWellChiclet::messageCountChanged, this, _1));
	LLIMModel::instance().addNoUnreadMsgsCallback(boost::bind(&LLIMWellChiclet::messageCountChanged, this, _1));

	LLIMMgr::getInstance()->addSessionObserver(this);

	LLIMWellWindow::getInstance()->setSysWellChiclet(this);
}

LLIMWellChiclet::~LLIMWellChiclet()
{
	LLIMMgr::getInstance()->removeSessionObserver(this);
}

void LLIMWellChiclet::onMenuItemClicked(const LLSD& user_data)
{
	std::string action = user_data.asString();
	if("close all" == action)
	{
		LLIMWellWindow::getInstance()->closeAll();
	}
}

bool LLIMWellChiclet::enableMenuItem(const LLSD& user_data)
{
	std::string item = user_data.asString();
	if (item == "can close all")
	{
		return !LLIMWellWindow::getInstance()->isWindowEmpty();
	}
	return true;
}

void LLIMWellChiclet::createMenu()
{
	if(mContextMenu)
	{
		llwarns << "Menu already exists" << llendl;
		return;
	}

	LLUICtrl::CommitCallbackRegistry::ScopedRegistrar registrar;
	registrar.add("IMWellChicletMenu.Action",
		boost::bind(&LLIMWellChiclet::onMenuItemClicked, this, _2));

	LLUICtrl::EnableCallbackRegistry::ScopedRegistrar enable_registrar;
	enable_registrar.add("IMWellChicletMenu.EnableItem",
		boost::bind(&LLIMWellChiclet::enableMenuItem, this, _2));

	mContextMenu = LLUICtrlFactory::getInstance()->createFromFile<LLContextMenu>
		("menu_im_well_button.xml",
		 LLMenuGL::sMenuContainer,
		 LLViewerMenuHolderGL::child_registry_t::instance());
}

void LLIMWellChiclet::messageCountChanged(const LLSD& session_data)
{
	setCounter(LLBottomTray::getInstance()->getTotalUnreadIMCount());
}

/************************************************************************/
/*               LLNotificationChiclet implementation                   */
/************************************************************************/
LLNotificationChiclet::LLNotificationChiclet(const Params& p)
: LLSysWellChiclet(p)
, mUreadSystemNotifications(0)
{
	// connect counter handlers to the signals
	connectCounterUpdatersToSignal("notify");
	connectCounterUpdatersToSignal("groupnotify");
	connectCounterUpdatersToSignal("offer");

	// ensure that notification well window exists, to synchronously
	// handle toast add/delete events.
	LLNotificationWellWindow::getInstance()->setSysWellChiclet(this);
}

void LLNotificationChiclet::connectCounterUpdatersToSignal(const std::string& notification_type)
{
	LLNotificationsUI::LLNotificationManager* manager = LLNotificationsUI::LLNotificationManager::getInstance();
	LLNotificationsUI::LLEventHandler* n_handler = manager->getHandlerForNotification(notification_type);
	if(n_handler)
	{
		n_handler->setNewNotificationCallback(boost::bind(&LLNotificationChiclet::incUreadSystemNotifications, this));
		n_handler->setDelNotification(boost::bind(&LLNotificationChiclet::decUreadSystemNotifications, this));
	}
}

void LLNotificationChiclet::onMenuItemClicked(const LLSD& user_data)
{
	std::string action = user_data.asString();
	if("close all" == action)
	{
		LLNotificationWellWindow::getInstance()->closeAll();
	}
}

bool LLNotificationChiclet::enableMenuItem(const LLSD& user_data)
{
	std::string item = user_data.asString();
	if (item == "can close all")
	{
		return mUreadSystemNotifications != 0;
	}
	return true;
}

void LLNotificationChiclet::createMenu()
{
	if(mContextMenu)
	{
		llwarns << "Menu already exists" << llendl;
		return;
	}

	LLUICtrl::CommitCallbackRegistry::ScopedRegistrar registrar;
	registrar.add("NotificationWellChicletMenu.Action",
		boost::bind(&LLNotificationChiclet::onMenuItemClicked, this, _2));

	LLUICtrl::EnableCallbackRegistry::ScopedRegistrar enable_registrar;
	enable_registrar.add("NotificationWellChicletMenu.EnableItem",
		boost::bind(&LLNotificationChiclet::enableMenuItem, this, _2));

	mContextMenu = LLUICtrlFactory::getInstance()->createFromFile<LLContextMenu>
		("menu_notification_well_button.xml",
		 LLMenuGL::sMenuContainer,
		 LLViewerMenuHolderGL::child_registry_t::instance());
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
	return setCommitCallback(cb);
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
	addChild(mNewMessagesIcon);

	// adjust size and position of an icon
	LLRect chiclet_rect = p.rect;
	LLRect overlay_icon_rect = LLRect(chiclet_rect.getWidth()/2, chiclet_rect.getHeight(), chiclet_rect.getWidth(), chiclet_rect.getHeight()/2); 
	mNewMessagesIcon->setRect(overlay_icon_rect);
	
	// shift an icon a little bit to the right and up corner of a chiclet
	overlay_icon_rect.translate(OVERLAY_ICON_SHIFT, OVERLAY_ICON_SHIFT);

	enableCounterControl(false);
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

void LLIMChiclet::enableCounterControl(bool enable) 
{
	mCounterEnabled = enable;
	if(!enable)
	{
		LLChiclet::setShowCounter(false);
	}
}

void LLIMChiclet::setShowCounter(bool show)
{
	if(!mCounterEnabled)
	{
		return;
	}

	bool needs_resize = getShowCounter() != show;
	if(needs_resize)
	{		
		LLChiclet::setShowCounter(show);
		toggleCounterControl();
		onChicletSizeChanged();		
	}
}

void LLIMChiclet::initSpeakerControl()
{
	// virtual
}

void LLIMChiclet::setRequiredWidth()
{
	bool show_speaker = getShowSpeaker();
	bool show_counter = getShowCounter();
	S32 required_width = CHICLET_RECT.getWidth();

	if (show_counter)
	{
		required_width += COUNTER_RECT.getWidth();
	}
	if (show_speaker)
	{
		required_width += VOICE_INDICATOR_RECT.getWidth();
	} 

	reshape(required_width, getRect().getHeight());
}

void LLIMChiclet::toggleSpeakerControl()
{
	if(getShowSpeaker())
	{
		if(getShowCounter())
		{
			mSpeakerCtrl->setRect(VOICE_INDICATOR_RECT);
		}
		else
		{
			mSpeakerCtrl->setRect(COUNTER_RECT);
		}
		initSpeakerControl();		
	}

	setRequiredWidth();
	mSpeakerCtrl->setVisible(getShowSpeaker());
}

void LLIMChiclet::setCounter(S32 counter)
{
	mCounterCtrl->setCounter(counter);
	setShowCounter(counter);
	setShowNewMessagesIcon(counter);
}

void LLIMChiclet::toggleCounterControl()
{
	setRequiredWidth();
	mCounterCtrl->setVisible(getShowCounter());
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
	unread_notifications.rect(COUNTER_RECT);
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

	//since mShowSpeaker initialized with false 
	//setShowSpeaker(false) will not hide mSpeakerCtrl
	mSpeakerCtrl->setVisible(getShowSpeaker());
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
	else if("end" == level)
	{
		LLAvatarActions::endIM(other_participant_id);
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
	unread_notifications.rect(COUNTER_RECT);
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

void LLAdHocChiclet::createPopupMenu()
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
	registrar.add("IMChicletMenu.Action", boost::bind(&LLAdHocChiclet::onMenuItemClicked, this, _2));

	mPopupMenu = LLUICtrlFactory::getInstance()->createFromFile<LLMenuGL>
		("menu_imchiclet_adhoc.xml", gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());
}

void LLAdHocChiclet::onMenuItemClicked(const LLSD& user_data)
{
	std::string level = user_data.asString();
	LLUUID group_id = getSessionId();

	if("end" == level)
	{
		LLGroupActions::endIM(group_id);
	}
}

BOOL LLAdHocChiclet::handleRightMouseDown(S32 x, S32 y, MASK mask)
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

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

LLIMGroupChiclet::Params::Params()
: group_icon("group_icon")
, unread_notifications("unread_notifications")
, speaker("speaker")
, show_speaker("show_speaker")
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
	unread_notifications.rect(COUNTER_RECT);
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
		LLGroupActions::startIM(group_id);
	}
	else if("info" == level)
	{
		LLGroupActions::show(group_id);
	}
	else if("end" == level)
	{
		LLGroupActions::endIM(group_id);
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
		min_width = 180 + 3*chiclet_padding;
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
	panel_params.follows.flags(FOLLOWS_LEFT | FOLLOWS_RIGHT);
	mScrollArea = LLUICtrlFactory::create<LLPanel>(panel_params,this);

	// important for Show/Hide Camera and Move controls menu in bottom tray to work properly
	mScrollArea->setMouseOpaque(false);

	addChild(mScrollArea);
}

LLChicletPanel::~LLChicletPanel()
{
	LLTransientFloaterMgr::getInstance()->removeControlView(mLeftScrollButton);
	LLTransientFloaterMgr::getInstance()->removeControlView(mRightScrollButton);

}

void im_chiclet_callback(LLChicletPanel* panel, const LLSD& data){
	
	LLUUID session_id = data["session_id"].asUUID();
	S32 unread = data["participant_unread"].asInteger();

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

void object_chiclet_callback(const LLSD& data)
{
	LLUUID object_id = data["object_id"];
	bool new_message = data["new_message"];

	std::list<LLChiclet*> chiclets = LLIMChiclet::sFindChicletsSignal(object_id);
	std::list<LLChiclet *>::iterator iter;
	for (iter = chiclets.begin(); iter != chiclets.end(); iter++)
	{
		LLIMChiclet* chiclet = dynamic_cast<LLIMChiclet*>(*iter);
		if (chiclet != NULL)
		{
			if(data.has("unread"))
			{
				chiclet->setCounter(data["unread"]);
			}
			chiclet->setShowNewMessagesIcon(new_message);
		}
	}
}

BOOL LLChicletPanel::postBuild()
{
	LLPanel::postBuild();
	LLIMModel::instance().addNewMsgCallback(boost::bind(im_chiclet_callback, this, _1));
	LLIMModel::instance().addNoUnreadMsgsCallback(boost::bind(im_chiclet_callback, this, _1));
	LLScriptFloaterManager::getInstance()->addNewObjectCallback(boost::bind(object_chiclet_callback, _1));
	LLScriptFloaterManager::getInstance()->addToggleObjectFloaterCallback(boost::bind(object_chiclet_callback, _1));
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
	static LLUUID s_previous_active_voice_session_id;

	std::list<LLChiclet*> chiclets = LLIMChiclet::sFindChicletsSignal(session_id);

	for(std::list<LLChiclet *>::iterator it = chiclets.begin(); it != chiclets.end(); ++it)
	{
		LLIMChiclet* chiclet = dynamic_cast<LLIMChiclet*>(*it);
		if(chiclet)
		{
			chiclet->setShowSpeaker(true);
		}
	}

	if(!s_previous_active_voice_session_id.isNull() && s_previous_active_voice_session_id != session_id)
	{
		chiclets = LLIMChiclet::sFindChicletsSignal(s_previous_active_voice_session_id);

		for(std::list<LLChiclet *>::iterator it = chiclets.begin(); it != chiclets.end(); ++it)
		{
			LLIMChiclet* chiclet = dynamic_cast<LLIMChiclet*>(*it);
			if(chiclet)
			{
				chiclet->setShowSpeaker(false);
			}
		}		
	}

	s_previous_active_voice_session_id = session_id;
}

bool LLChicletPanel::addChiclet(LLChiclet* chiclet, S32 index)
{
	if(mScrollArea->addChild(chiclet))
	{
		// chiclets should be aligned to right edge of scroll panel
		S32 left_shift = 0;

		if (!canScrollLeft())
		{
			// init left shift for the first chiclet in the list...
			if (mChicletList.empty())
			{
				// ...start from the right border of the scroll area for the first added chiclet 
				left_shift = mScrollArea->getRect().getWidth();
			}
			else
			{
				// ... start from the left border of the first chiclet minus padding
				left_shift = getChiclet(0)->getRect().mLeft - getChicletPadding();
			}

			// take into account width of the being added chiclet
			left_shift -= chiclet->getRequiredRect().getWidth();

			// if we overflow the scroll area we do not need to shift chiclets
			if (left_shift < 0)
			{
				left_shift = 0;
			}
		}

		mChicletList.insert(mChicletList.begin() + index, chiclet);

		// shift first chiclet to place it in correct position. 
		// rest ones will be placed in arrange()
		if (!canScrollLeft())
		{
			getChiclet(0)->translate(left_shift - getChiclet(0)->getRect().mLeft, 0);
		}

		chiclet->setLeftButtonClickCallback(boost::bind(&LLChicletPanel::onChicletClick, this, _1, _2));
		chiclet->setChicletSizeChangedCallback(boost::bind(&LLChicletPanel::onChicletSizeChanged, this, _1, index));

		arrange();

		return true;
	}

	return false;
}

void LLChicletPanel::onChicletSizeChanged(LLChiclet* ctrl, const LLSD& param)
{
	arrange();
}

void LLChicletPanel::onChicletClick(LLUICtrl*ctrl,const LLSD&param)
{
	if (mCommitSignal)
	{
		(*mCommitSignal)(ctrl,param);
	}
}

void LLChicletPanel::removeChiclet(chiclet_list_t::iterator it)
{
	mScrollArea->removeChild(*it);
	mChicletList.erase(it);
	
	arrange();
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

	//Needed once- to avoid error at first call of reshape() before postBuild()
	if(!mLeftScrollButton||!mRightScrollButton)
		return;
	
	LLRect scroll_button_rect = mLeftScrollButton->getRect();
	mLeftScrollButton->setRect(LLRect(0,scroll_button_rect.mTop,scroll_button_rect.getWidth(),
		scroll_button_rect.mBottom));
	scroll_button_rect = mRightScrollButton->getRect();
	mRightScrollButton->setRect(LLRect(width - scroll_button_rect.getWidth(),scroll_button_rect.mTop,
		width, scroll_button_rect.mBottom));
	

	bool need_show_scroll = needShowScroll();
	if(need_show_scroll)
	{
		mScrollArea->setRect(LLRect(scroll_button_rect.getWidth() + SCROLL_BUTTON_PAD,
			height, width - scroll_button_rect.getWidth() - SCROLL_BUTTON_PAD, 0));
	}
	else
	{
		mScrollArea->setRect(LLRect(0,height, width, 0));
	}
	
	mShowControls = width >= mMinWidth;
	
	mScrollArea->setVisible(mShowControls);

	trimChiclets();
	showScrollButtonsIfNeeded();

}

void LLChicletPanel::arrange()
{
	if(mChicletList.empty())
		return;

	//initial arrange of chicklets positions
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

	//reset size and pos on mScrollArea
	LLRect rect = getRect();
	LLRect scroll_button_rect = mLeftScrollButton->getRect();
	
	bool need_show_scroll = needShowScroll();
	if(need_show_scroll)
	{
		mScrollArea->setRect(LLRect(scroll_button_rect.getWidth() + SCROLL_BUTTON_PAD,
			rect.getHeight(), rect.getWidth() - scroll_button_rect.getWidth() - SCROLL_BUTTON_PAD, 0));
	}
	else
	{
		mScrollArea->setRect(LLRect(0,rect.getHeight(), rect.getWidth(), 0));
	}
	
	trimChiclets();
	showScrollButtonsIfNeeded();
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

bool LLChicletPanel::needShowScroll()
{
	if(mChicletList.empty())
		return false;
	
	S32 chicklet_width  = (*mChicletList.rbegin())->getRect().mRight - (*mChicletList.begin())->getRect().mLeft;

	return chicklet_width>getRect().getWidth();
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
	return setCommitCallback(cb);
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

S32 LLChicletPanel::getTotalUnreadIMCount()
{
	S32 count = 0;
	chiclet_list_t::const_iterator it = mChicletList.begin();
	for( ; mChicletList.end() != it; ++it)
	{
		LLIMChiclet* chiclet = dynamic_cast<LLIMChiclet*>(*it);
		if(chiclet)
		{
			count += chiclet->getCounter();
		}
	}
	return count;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
LLChicletNotificationCounterCtrl::Params::Params()
: max_displayed_count("max_displayed_count", MAX_DISPLAYED_COUNT)
{
}

LLChicletNotificationCounterCtrl::LLChicletNotificationCounterCtrl(const Params& p)
 : LLTextBox(p)
 , mCounter(0)
 , mInitialWidth(0)
 , mMaxDisplayedCount(p.max_displayed_count)
{
	mInitialWidth = getRect().getWidth();
}

void LLChicletNotificationCounterCtrl::setCounter(S32 counter)
{
	mCounter = counter;

	// note same code in LLSysWellChiclet::setCounter(S32 counter)
	std::string s_count;
	if(counter != 0)
	{
		static std::string more_messages_exist("+");
		std::string more_messages(counter > mMaxDisplayedCount ? more_messages_exist : "");
		s_count = llformat("%d%s"
			, llmin(counter, mMaxDisplayedCount)
			, more_messages.c_str()
			);
	}

	if(mCounter != 0)
	{
		setText(s_count);
	}
	else
	{
		setText(std::string(""));
	}
}

LLRect LLChicletNotificationCounterCtrl::getRequiredRect()
{
	LLRect rc;
	S32 text_width = getTextPixelWidth();

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

LLChicletInvOfferIconCtrl::LLChicletInvOfferIconCtrl(const Params& p)
: LLChicletAvatarIconCtrl(p)
 , mDefaultIcon(p.default_icon)
{
}

void LLChicletInvOfferIconCtrl::setValue(const LLSD& value )
{
	if(value.asUUID().isNull())
	{
		LLIconCtrl::setValue(mDefaultIcon);
	}
	else
	{
		LLChicletAvatarIconCtrl::setValue(value);
	}
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

LLChicletSpeakerCtrl::LLChicletSpeakerCtrl(const Params&p)
 : LLOutputMonitorCtrl(p)
{
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

LLScriptChiclet::Params::Params()
 : icon("icon")
{
	// *TODO Vadim: Get rid of hardcoded values.
 	rect(CHICLET_RECT);
	icon.rect(CHICLET_ICON_RECT);
}

LLScriptChiclet::LLScriptChiclet(const Params&p)
 : LLIMChiclet(p)
 , mChicletIconCtrl(NULL)
{
	LLIconCtrl::Params icon_params = p.icon;
	mChicletIconCtrl = LLUICtrlFactory::create<LLIconCtrl>(icon_params);
	// Let "new message" icon be on top, else it will be hidden behind chiclet icon.
	addChildInBack(mChicletIconCtrl);
}

void LLScriptChiclet::setSessionId(const LLUUID& session_id)
{
	setShowNewMessagesIcon( getSessionId() != session_id );

	LLIMChiclet::setSessionId(session_id);
	LLUUID notification_id = LLScriptFloaterManager::getInstance()->findNotificationId(session_id);
	LLNotificationPtr notification = LLNotifications::getInstance()->find(notification_id);
	if(notification)
	{
		setToolTip(notification->getSubstitutions()["TITLE"].asString());
	}
}

void LLScriptChiclet::setCounter(S32 counter)
{
	setShowNewMessagesIcon( counter > 0 );
}

void LLScriptChiclet::onMouseDown()
{
	LLScriptFloaterManager::getInstance()->toggleScriptFloater(getSessionId());
}

BOOL LLScriptChiclet::handleMouseDown(S32 x, S32 y, MASK mask)
{
	onMouseDown();
	return LLChiclet::handleMouseDown(x, y, mask);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

static const std::string INVENTORY_USER_OFFER	("UserGiveItem");

LLInvOfferChiclet::Params::Params()
{
	// *TODO Vadim: Get rid of hardcoded values.
	rect(CHICLET_RECT);
	icon.rect(CHICLET_ICON_RECT);
}

LLInvOfferChiclet::LLInvOfferChiclet(const Params&p)
 : LLIMChiclet(p)
 , mChicletIconCtrl(NULL)
{
	LLChicletInvOfferIconCtrl::Params icon_params = p.icon;
	mChicletIconCtrl = LLUICtrlFactory::create<LLChicletInvOfferIconCtrl>(icon_params);
	// Let "new message" icon be on top, else it will be hidden behind chiclet icon.
	addChildInBack(mChicletIconCtrl);
}

void LLInvOfferChiclet::setSessionId(const LLUUID& session_id)
{
	setShowNewMessagesIcon( getSessionId() != session_id );

	LLIMChiclet::setSessionId(session_id);
	LLUUID notification_id = LLScriptFloaterManager::getInstance()->findNotificationId(session_id);
	LLNotificationPtr notification = LLNotifications::getInstance()->find(notification_id);
	if(notification)
	{
		setToolTip(notification->getSubstitutions()["TITLE"].asString());
	}

	if ( notification && notification->getName() == INVENTORY_USER_OFFER )
	{
		mChicletIconCtrl->setValue(notification->getPayload()["from_id"]);
	}
	else
	{
		mChicletIconCtrl->setValue(LLUUID::null);
	}
}

void LLInvOfferChiclet::setCounter(S32 counter)
{
	setShowNewMessagesIcon( counter > 0 );
}

void LLInvOfferChiclet::onMouseDown()
{
	LLScriptFloaterManager::instance().toggleScriptFloater(getSessionId());
}

BOOL LLInvOfferChiclet::handleMouseDown(S32 x, S32 y, MASK mask)
{
	onMouseDown();
	return LLChiclet::handleMouseDown(x, y, mask);
}

// EOF
