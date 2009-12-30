/** 
 * @file llsyswellwindow.cpp
 * @brief                                    // TODO
 * $LicenseInfo:firstyear=2000&license=viewergpl$
 * 
 * Copyright (c) 2000-2009, Linden Research, Inc.
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

#include "llagent.h"

#include "llflatlistview.h"
#include "llfloaterreg.h"
#include "llnotifications.h"

#include "llsyswellwindow.h"

#include "llbottomtray.h"
#include "llscriptfloater.h"
#include "llviewercontrol.h"
#include "llviewerwindow.h"

#include "llchiclet.h"
#include "lltoastpanel.h"
#include "llnotificationmanager.h"
#include "llnotificationsutil.h"
#include "llspeakers.h"

//---------------------------------------------------------------------------------
LLSysWellWindow::LLSysWellWindow(const LLSD& key) : LLTransientDockableFloater(NULL, true,  key),
													mChannel(NULL),
													mMessageList(NULL),
													mSysWellChiclet(NULL),
													mSeparator(NULL),
													NOTIFICATION_WELL_ANCHOR_NAME("notification_well_panel"),
													IM_WELL_ANCHOR_NAME("im_well_panel")

{
	mTypedItemsCount[IT_NOTIFICATION] = 0;
	mTypedItemsCount[IT_INSTANT_MESSAGE] = 0;
	setOverlapsScreenChannel(true);
}

//---------------------------------------------------------------------------------
BOOL LLSysWellWindow::postBuild()
{
	mMessageList = getChild<LLFlatListView>("notification_list");

	// get a corresponding channel
	initChannel();

	LLPanel::Params params;
	mSeparator = LLUICtrlFactory::create<LLPanel>(params);
	LLUICtrlFactory::instance().buildPanel(mSeparator, "panel_separator.xml");

	LLRect rc = mSeparator->getRect();
	rc.setOriginAndSize(0, 0, mMessageList->getItemsRect().getWidth(), rc.getHeight());
	mSeparator->setRect(rc);
	mSeparator->setFollows(FOLLOWS_LEFT | FOLLOWS_RIGHT | FOLLOWS_TOP);
	mSeparator->setVisible(FALSE);

	mMessageList->addItem(mSeparator);

	// click on SysWell Window should clear "new message" state (and 'Lit' status). EXT-3147.
	// mouse up callback is not called in this case.
	setMouseDownCallback(boost::bind(&LLSysWellWindow::releaseNewMessagesState, this));

	return LLTransientDockableFloater::postBuild();
}

//---------------------------------------------------------------------------------
void LLSysWellWindow::setMinimized(BOOL minimize)
{
	LLTransientDockableFloater::setMinimized(minimize);
}

//---------------------------------------------------------------------------------
void LLSysWellWindow::onStartUpToastClick(S32 x, S32 y, MASK mask)
{
	// just set floater visible. Screen channels will be cleared.
	setVisible(TRUE);
}

void LLSysWellWindow::setSysWellChiclet(LLSysWellChiclet* chiclet) 
{ 
	mSysWellChiclet = chiclet;
	if(mSysWellChiclet)
		mSysWellChiclet->updateWidget(isWindowEmpty()); 
}
//---------------------------------------------------------------------------------
LLSysWellWindow::~LLSysWellWindow()
{
}

//---------------------------------------------------------------------------------
void LLSysWellWindow::removeItemByID(const LLUUID& id)
{
	if(mMessageList->removeItemByValue(id))
	{
		handleItemRemoved(IT_NOTIFICATION);
		reshapeWindow();
	}
	else
	{
		llwarns << "Unable to remove notification from the list, ID: " << id
			<< llendl;
	}

	// hide chiclet window if there are no items left
	if(isWindowEmpty())
	{
		setVisible(FALSE);
	}
}

//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------
void LLSysWellWindow::initChannel() 
{
	LLNotificationsUI::LLScreenChannelBase* channel = LLNotificationsUI::LLChannelManager::getInstance()->findChannelByID(
																LLUUID(gSavedSettings.getString("NotificationChannelUUID")));
	mChannel = dynamic_cast<LLNotificationsUI::LLScreenChannel*>(channel);
	if(NULL == mChannel)
	{
		llwarns << "LLSysWellWindow::initChannel() - could not get a requested screen channel" << llendl;
	}
}

//---------------------------------------------------------------------------------
void LLSysWellWindow::getAllowedRect(LLRect& rect)
{
	rect = gViewerWindow->getWorldViewRectScaled();
}

//---------------------------------------------------------------------------------


//---------------------------------------------------------------------------------
void LLSysWellWindow::setVisible(BOOL visible)
{
	if (visible)
	{
		if (NULL == getDockControl() && getDockTongue().notNull())
		{
			setDockControl(new LLDockControl(
				LLBottomTray::getInstance()->getChild<LLView>(getAnchorViewName()), this,
				getDockTongue(), LLDockControl::TOP, boost::bind(&LLSysWellWindow::getAllowedRect, this, _1)));
		}
	}

	// do not show empty window
	if (NULL == mMessageList || isWindowEmpty()) visible = FALSE;

	LLTransientDockableFloater::setVisible(visible);

	// update notification channel state	
	if(mChannel)
	{
		mChannel->updateShowToastsState();
		mChannel->redrawToasts();
	}

	if (visible)
	{
		releaseNewMessagesState();
	}
}

//---------------------------------------------------------------------------------
void LLSysWellWindow::setDocked(bool docked, bool pop_on_undock)
{
	LLTransientDockableFloater::setDocked(docked, pop_on_undock);

	// update notification channel state
	if(mChannel)
	{
		mChannel->updateShowToastsState();
		mChannel->redrawToasts();
	}
}

//---------------------------------------------------------------------------------
void LLSysWellWindow::reshapeWindow()
{
	// save difference between floater height and the list height to take it into account while calculating new window height
	// it includes height from floater top to list top and from floater bottom and list bottom
	static S32 parent_list_delta_height = getRect().getHeight() - mMessageList->getRect().getHeight();

	S32 notif_list_height = mMessageList->getItemsRect().getHeight() + 2 * mMessageList->getBorderWidth();

	LLRect curRect = getRect();

	S32 new_window_height = notif_list_height + parent_list_delta_height;

	if (new_window_height > MAX_WINDOW_HEIGHT)
	{
		new_window_height = MAX_WINDOW_HEIGHT;
	}
	S32 newY = curRect.mTop + new_window_height - curRect.getHeight();
	S32 newWidth = curRect.getWidth() < MIN_WINDOW_WIDTH ? MIN_WINDOW_WIDTH
			: curRect.getWidth();
	curRect.setLeftTopAndSize(curRect.mLeft, newY, newWidth, new_window_height);
	reshape(curRect.getWidth(), curRect.getHeight(), TRUE);
	setRect(curRect);

	// update notification channel state
	// update on a window reshape is important only when a window is visible and docked
	if(mChannel && getVisible() && isDocked())
	{
		mChannel->updateShowToastsState();
	}
}

void LLSysWellWindow::releaseNewMessagesState()
{
	if (NULL != mSysWellChiclet)
	{
		mSysWellChiclet->setNewMessagesState(false);
	}
}

//---------------------------------------------------------------------------------
bool LLSysWellWindow::isWindowEmpty()
{
	// keep in mind, mSeparator is always in the list
	return mMessageList->size() == 1;
}

// *TODO: mantipov: probably is deprecated
void LLSysWellWindow::handleItemAdded(EItemType added_item_type)
{
	bool should_be_shown = ++mTypedItemsCount[added_item_type] == 1 && anotherTypeExists(added_item_type);

	if (should_be_shown && !mSeparator->getVisible())
	{
		mSeparator->setVisible(TRUE);

		// refresh list to recalculate mSeparator position
		mMessageList->reshape(mMessageList->getRect().getWidth(), mMessageList->getRect().getHeight());
	}

	//fix for EXT-3254
	//set limits for min_height. 
	S32 parent_list_delta_height = getRect().getHeight() - mMessageList->getRect().getHeight();

	std::vector<LLPanel*> items;
	mMessageList->getItems(items);

	if(items.size()>1)//first item is separator
	{
		S32 min_height;
		S32 min_width;
		getResizeLimits(&min_width,&min_height);

		min_height = items[1]->getRect().getHeight() + 2 * mMessageList->getBorderWidth() + parent_list_delta_height;

		setResizeLimits(min_width,min_height);
	}
	mSysWellChiclet->updateWidget(isWindowEmpty());
}

void LLSysWellWindow::handleItemRemoved(EItemType removed_item_type)
{
	bool should_be_hidden = --mTypedItemsCount[removed_item_type] == 0;

	if (should_be_hidden && mSeparator->getVisible())
	{
		mSeparator->setVisible(FALSE);

		// refresh list to recalculate mSeparator position
		mMessageList->reshape(mMessageList->getRect().getWidth(), mMessageList->getRect().getHeight());
	}
	mSysWellChiclet->updateWidget(isWindowEmpty());
}

bool LLSysWellWindow::anotherTypeExists(EItemType item_type)
{
	bool exists = false;
	switch(item_type)
	{
	case IT_INSTANT_MESSAGE:
		if (mTypedItemsCount[IT_NOTIFICATION] > 0)
		{
			exists = true;
		}
		break;
	case IT_NOTIFICATION:
		if (mTypedItemsCount[IT_INSTANT_MESSAGE] > 0)
		{
			exists = true;
		}
		break;
	}
	return exists;
}

/************************************************************************/
/*         RowPanel implementation                                      */
/************************************************************************/

//---------------------------------------------------------------------------------
LLIMWellWindow::RowPanel::RowPanel(const LLSysWellWindow* parent, const LLUUID& sessionId,
		S32 chicletCounter, const std::string& name, const LLUUID& otherParticipantId) :
		LLPanel(LLPanel::Params()), mChiclet(NULL), mParent(parent)
{
	LLUICtrlFactory::getInstance()->buildPanel(this, "panel_activeim_row.xml", NULL);

	// Choose which of the pre-created chiclets (IM/group) to use.
	// The other one gets hidden.

	LLIMChiclet::EType im_chiclet_type = LLIMChiclet::getIMSessionType(sessionId);
	switch (im_chiclet_type)
	{
	case LLIMChiclet::TYPE_GROUP:
		mChiclet = getChild<LLIMGroupChiclet>("group_chiclet");
		break;
	case LLIMChiclet::TYPE_AD_HOC:
		mChiclet = getChild<LLAdHocChiclet>("adhoc_chiclet");		
		break;
	case LLIMChiclet::TYPE_UNKNOWN: // assign mChiclet a non-null value anyway
	case LLIMChiclet::TYPE_IM:
		mChiclet = getChild<LLIMP2PChiclet>("p2p_chiclet");
		break;
	}

	// Initialize chiclet.
	mChiclet->setRect(LLRect(5, 28, 30, 3)); // *HACK: workaround for (EXT-3599)
	mChiclet->setChicletSizeChangedCallback(boost::bind(&LLIMWellWindow::RowPanel::onChicletSizeChanged, this, mChiclet, _2));
	mChiclet->enableCounterControl(true);
	mChiclet->setCounter(chicletCounter);
	mChiclet->setSessionId(sessionId);
	mChiclet->setIMSessionName(name);
	mChiclet->setOtherParticipantId(otherParticipantId);
	mChiclet->setVisible(true);

	LLTextBox* contactName = getChild<LLTextBox>("contact_name");
	contactName->setValue(name);

	mCloseBtn = getChild<LLButton>("hide_btn");
	mCloseBtn->setCommitCallback(boost::bind(&LLIMWellWindow::RowPanel::onClosePanel, this));
}

//---------------------------------------------------------------------------------
void LLIMWellWindow::RowPanel::onChicletSizeChanged(LLChiclet* ctrl, const LLSD& param)
{
	LLTextBox* text = getChild<LLTextBox>("contact_name");
	S32 new_text_left = mChiclet->getRect().mRight + CHICLET_HPAD;
	LLRect text_rect = text->getRect(); 
	text_rect.mLeft = new_text_left;
	text->setRect(text_rect);
}

//---------------------------------------------------------------------------------
LLIMWellWindow::RowPanel::~RowPanel()
{
}

//---------------------------------------------------------------------------------
void LLIMWellWindow::RowPanel::onClosePanel()
{
	gIMMgr->leaveSession(mChiclet->getSessionId());
	// This row panel will be removed from the list in LLSysWellWindow::sessionRemoved().
}

//---------------------------------------------------------------------------------
void LLIMWellWindow::RowPanel::onMouseEnter(S32 x, S32 y, MASK mask)
{
	setTransparentColor(LLUIColorTable::instance().getColor("SysWellItemSelected"));
}

//---------------------------------------------------------------------------------
void LLIMWellWindow::RowPanel::onMouseLeave(S32 x, S32 y, MASK mask)
{
	setTransparentColor(LLUIColorTable::instance().getColor("SysWellItemUnselected"));
}

//---------------------------------------------------------------------------------
// virtual
BOOL LLIMWellWindow::RowPanel::handleMouseDown(S32 x, S32 y, MASK mask)
{
	// Pass the mouse down event to the chiclet (EXT-596).
	if (!mChiclet->pointInView(x, y) && !mCloseBtn->getRect().pointInRect(x, y)) // prevent double call of LLIMChiclet::onMouseDown()
		mChiclet->onMouseDown();

	return LLPanel::handleMouseDown(x, y, mask);
}

/************************************************************************/
/*         ObjectRowPanel implementation                                */
/************************************************************************/

LLIMWellWindow::ObjectRowPanel::ObjectRowPanel(const LLUUID& object_id, bool new_message/* = false*/)
 : LLPanel()
 , mChiclet(NULL)
{
	LLUICtrlFactory::getInstance()->buildPanel(this, "panel_active_object_row.xml", NULL);

	initChiclet(object_id);

	LLTextBox* obj_name = getChild<LLTextBox>("object_name");
	obj_name->setValue(getObjectName(object_id));

	mCloseBtn = getChild<LLButton>("hide_btn");
	mCloseBtn->setCommitCallback(boost::bind(&LLIMWellWindow::ObjectRowPanel::onClosePanel, this));
}

//---------------------------------------------------------------------------------
LLIMWellWindow::ObjectRowPanel::~ObjectRowPanel()
{
}

std::string LLIMWellWindow::ObjectRowPanel::getObjectName(const LLUUID& object_id)
{
	using namespace LLNotificationsUI;
	LLUUID notification_id = LLScriptFloaterManager::getInstance()->findNotificationId(object_id);
	LLNotificationPtr notification = LLNotifications::getInstance()->find(notification_id);
	if(!notification)
	{
		llwarns << "Invalid notification" << llendl;
		return LLStringUtil::null;
	}

	std::string text;

	switch(getObjectType(notification))
	{
	case OBJ_SCRIPT:
		text = notification->getSubstitutions()["TITLE"].asString();
		break;
	case OBJ_LOAD_URL:
		text = notification->getSubstitutions()["OBJECTNAME"].asString();
		break;
	case OBJ_GIVE_INVENTORY:
		text = notification->getSubstitutions()["NAME"].asString();
		break;
	default:
		text = getString("unknown_obj");
		break;
	}

	return text;
}

//---------------------------------------------------------------------------------
void LLIMWellWindow::ObjectRowPanel::onClosePanel()
{
	LLScriptFloaterManager::getInstance()->removeNotificationByObjectId(mChiclet->getSessionId());
}

//static
LLIMWellWindow::ObjectRowPanel::object_type_map LLIMWellWindow::ObjectRowPanel::initObjectTypeMap()
{
	object_type_map type_map;
	type_map["ScriptDialog"] = OBJ_SCRIPT;
	type_map["LoadWebPage"] = OBJ_LOAD_URL;
	type_map["ObjectGiveItem"] = OBJ_GIVE_INVENTORY;
	return type_map;
}

// static
LLIMWellWindow::ObjectRowPanel::EObjectType LLIMWellWindow::ObjectRowPanel::getObjectType(const LLNotificationPtr& notification)
{
	if(!notification)
	{
		llwarns << "Invalid notification" << llendl;
		return OBJ_UNKNOWN;
	}

	static object_type_map type_map = initObjectTypeMap();
	std::string name = notification->getName();
	object_type_map::const_iterator it = type_map.find(name);
	if(it != type_map.end())
	{
		return it->second;
	}

	llwarns << "Unknown object type" << llendl;
	return OBJ_UNKNOWN;
}

void LLIMWellWindow::ObjectRowPanel::initChiclet(const LLUUID& object_id, bool new_message/* = false*/)
{
	using namespace LLNotificationsUI;
	LLUUID notification_id = LLScriptFloaterManager::getInstance()->findNotificationId(object_id);
	LLNotificationPtr notification = LLNotifications::getInstance()->find(notification_id);
	if(!notification)
	{
		llwarns << "Invalid notification" << llendl;
		return;
	}

	// Choose which of the pre-created chiclets to use.
	switch(getObjectType(notification))
	{
	case OBJ_GIVE_INVENTORY:
		mChiclet = getChild<LLInvOfferChiclet>("inv_offer_chiclet");
		break;
	default:
		mChiclet = getChild<LLScriptChiclet>("object_chiclet");
		break;
	}

	mChiclet->setVisible(true);
	mChiclet->setSessionId(object_id);
//	mChiclet->setShowNewMessagesIcon(new_message);
}

//---------------------------------------------------------------------------------
void LLIMWellWindow::ObjectRowPanel::onMouseEnter(S32 x, S32 y, MASK mask)
{
	setTransparentColor(LLUIColorTable::instance().getColor("SysWellItemSelected"));
}

//---------------------------------------------------------------------------------
void LLIMWellWindow::ObjectRowPanel::onMouseLeave(S32 x, S32 y, MASK mask)
{
	setTransparentColor(LLUIColorTable::instance().getColor("SysWellItemUnselected"));
}

//---------------------------------------------------------------------------------
// virtual
BOOL LLIMWellWindow::ObjectRowPanel::handleMouseDown(S32 x, S32 y, MASK mask)
{
	// Pass the mouse down event to the chiclet (EXT-596).
	if (!mChiclet->pointInView(x, y) && !mCloseBtn->getRect().pointInRect(x, y)) // prevent double call of LLIMChiclet::onMouseDown()
		mChiclet->onMouseDown();

	return LLPanel::handleMouseDown(x, y, mask);
}

/************************************************************************/
/*         LLNotificationWellWindow implementation                      */
/************************************************************************/

//////////////////////////////////////////////////////////////////////////
// PUBLIC METHODS
LLNotificationWellWindow::LLNotificationWellWindow(const LLSD& key)
: LLSysWellWindow(key)
{
	// init connections to the list's update events
	connectListUpdaterToSignal("notify");
	connectListUpdaterToSignal("groupnotify");
	connectListUpdaterToSignal("offer");
}

// static
LLNotificationWellWindow* LLNotificationWellWindow::getInstance(const LLSD& key /*= LLSD()*/)
{
	return LLFloaterReg::getTypedInstance<LLNotificationWellWindow>("notification_well_window", key);
}

// virtual
BOOL LLNotificationWellWindow::postBuild()
{
	BOOL rv = LLSysWellWindow::postBuild();
	setTitle(getString("title_notification_well_window"));
	return rv;
}

// virtual
void LLNotificationWellWindow::setVisible(BOOL visible)
{
	if (visible)
	{
		// when Notification channel is cleared, storable toasts will be added into the list.
		clearScreenChannels();
	}

	LLSysWellWindow::setVisible(visible);
}

//---------------------------------------------------------------------------------
void LLNotificationWellWindow::addItem(LLSysWellItem::Params p)
{
	LLSD value = p.notification_id;
	// do not add clones
	if( mMessageList->getItemByValue(value))
		return;

	LLSysWellItem* new_item = new LLSysWellItem(p);
	if (mMessageList->addItem(new_item, value, ADD_TOP))
	{
		handleItemAdded(IT_NOTIFICATION);

		reshapeWindow();

		new_item->setOnItemCloseCallback(boost::bind(&LLNotificationWellWindow::onItemClose, this, _1));
		new_item->setOnItemClickCallback(boost::bind(&LLNotificationWellWindow::onItemClick, this, _1));
	}
	else
	{
		llwarns << "Unable to add Notification into the list, notification ID: " << p.notification_id
			<< ", title: " << p.title
			<< llendl;

		new_item->die();
	}
}

void LLNotificationWellWindow::closeAll()
{
	// Need to clear notification channel, to add storable toasts into the list.
	clearScreenChannels();
	std::vector<LLPanel*> items;
	mMessageList->getItems(items);
	for (std::vector<LLPanel*>::iterator
			 iter = items.begin(),
			 iter_end = items.end();
		 iter != iter_end; ++iter)
	{
		LLSysWellItem* sys_well_item = dynamic_cast<LLSysWellItem*>(*iter);
		if (sys_well_item)
			onItemClose(sys_well_item);
	}
}

//////////////////////////////////////////////////////////////////////////
// PRIVATE METHODS
void LLNotificationWellWindow::initChannel() 
{
	LLSysWellWindow::initChannel();
	if(mChannel)
	{
		mChannel->setOnStoreToastCallback(boost::bind(&LLNotificationWellWindow::onStoreToast, this, _1, _2));
	}
}

void LLNotificationWellWindow::clearScreenChannels()
{
	// 1 - remove StartUp toast and channel if present
	if(!LLNotificationsUI::LLScreenChannel::getStartUpToastShown())
	{
		LLNotificationsUI::LLChannelManager::getInstance()->onStartUpToastClose();
	}

	// 2 - remove toasts in Notification channel
	if(mChannel)
	{
		mChannel->removeAndStoreAllStorableToasts();
	}
}

void LLNotificationWellWindow::onStoreToast(LLPanel* info_panel, LLUUID id)
{
	LLSysWellItem::Params p;	
	p.notification_id = id;
	p.title = static_cast<LLToastPanel*>(info_panel)->getTitle();
	addItem(p);
}

void LLNotificationWellWindow::connectListUpdaterToSignal(std::string notification_type)
{
	LLNotificationsUI::LLNotificationManager* manager = LLNotificationsUI::LLNotificationManager::getInstance();
	LLNotificationsUI::LLEventHandler* n_handler = manager->getHandlerForNotification(notification_type);
	if(n_handler)
	{
		n_handler->setNotificationIDCallback(boost::bind(&LLNotificationWellWindow::removeItemByID, this, _1));
	}
	else
	{
		llwarns << "LLSysWellWindow::connectListUpdaterToSignal() - could not get a handler for '" << notification_type <<"' type of notifications" << llendl;
	}
}

void LLNotificationWellWindow::onItemClick(LLSysWellItem* item)
{
	LLUUID id = item->getID();
	if(mChannel)
		mChannel->loadStoredToastByNotificationIDToChannel(id);
}

void LLNotificationWellWindow::onItemClose(LLSysWellItem* item)
{
	LLUUID id = item->getID();
	removeItemByID(id);
	if(mChannel)
		mChannel->killToastByNotificationID(id);
}



/************************************************************************/
/*         LLIMWellWindow  implementation                               */
/************************************************************************/

//////////////////////////////////////////////////////////////////////////
// PUBLIC METHODS
LLIMWellWindow::LLIMWellWindow(const LLSD& key)
: LLSysWellWindow(key)
{
	LLIMMgr::getInstance()->addSessionObserver(this);
	LLIMChiclet::sFindChicletsSignal.connect(boost::bind(&LLIMWellWindow::findIMChiclet, this, _1));
	LLIMChiclet::sFindChicletsSignal.connect(boost::bind(&LLIMWellWindow::findObjectChiclet, this, _1));
}

LLIMWellWindow::~LLIMWellWindow()
{
	LLIMMgr::getInstance()->removeSessionObserver(this);
}

// static
LLIMWellWindow* LLIMWellWindow::getInstance(const LLSD& key /*= LLSD()*/)
{
	return LLFloaterReg::getTypedInstance<LLIMWellWindow>("im_well_window", key);
}

BOOL LLIMWellWindow::postBuild()
{
	BOOL rv = LLSysWellWindow::postBuild();
	setTitle(getString("title_im_well_window"));
	return rv;
}

//virtual
void LLIMWellWindow::sessionAdded(const LLUUID& session_id,
								   const std::string& name, const LLUUID& other_participant_id)
{
	LLIMModel::LLIMSession* session = LLIMModel::getInstance()->findIMSession(session_id);
	if (!session) return;

	// no need to spawn chiclets for participants in P2P calls called through Avaline
	if (session->isP2P() && session->isOtherParticipantAvaline()) return;

	if (mMessageList->getItemByValue(session_id)) return;

	addIMRow(session_id, 0, name, other_participant_id);	
	reshapeWindow();
}

//virtual
void LLIMWellWindow::sessionRemoved(const LLUUID& sessionId)
{
	delIMRow(sessionId);
	reshapeWindow();
}

//virtual
void LLIMWellWindow::sessionIDUpdated(const LLUUID& old_session_id, const LLUUID& new_session_id)
{
	//for outgoing ad-hoc and group im sessions only
	LLChiclet* chiclet = findIMChiclet(old_session_id);
	if (chiclet)
	{
		chiclet->setSessionId(new_session_id);
		mMessageList->updateValue(old_session_id, new_session_id);
	}
}

LLChiclet* LLIMWellWindow::findObjectChiclet(const LLUUID& object_id)
{
	LLChiclet* res = NULL;
	ObjectRowPanel* panel = mMessageList->getTypedItemByValue<ObjectRowPanel>(object_id);
	if (panel != NULL)
	{
		res = panel->mChiclet;
	}

	return res;
}

//////////////////////////////////////////////////////////////////////////
// PRIVATE METHODS
LLChiclet* LLIMWellWindow::findIMChiclet(const LLUUID& sessionId)
{
	LLChiclet* res = NULL;
	RowPanel* panel = mMessageList->getTypedItemByValue<RowPanel>(sessionId);
	if (panel != NULL)
	{
		res = panel->mChiclet;
	}

	return res;
}

//---------------------------------------------------------------------------------
void LLIMWellWindow::addIMRow(const LLUUID& sessionId, S32 chicletCounter,
							   const std::string& name, const LLUUID& otherParticipantId)
{
	RowPanel* item = new RowPanel(this, sessionId, chicletCounter, name, otherParticipantId);
	if (mMessageList->insertItemAfter(mSeparator, item, sessionId))
	{
		handleItemAdded(IT_INSTANT_MESSAGE);
	}
	else
	{
		llwarns << "Unable to add IM Row into the list, sessionID: " << sessionId
			<< ", name: " << name
			<< ", other participant ID: " << otherParticipantId
			<< llendl;

		item->die();
	}
}

//---------------------------------------------------------------------------------
void LLIMWellWindow::delIMRow(const LLUUID& sessionId)
{
	//fix for EXT-3252
	//without this line LLIMWellWindow receive onFocusLost
	//and hide itself. It was becaue somehow LLIMChicklet was in focus group for
	//LLIMWellWindow...
	//But I didn't find why this happen..
	gFocusMgr.clearLastFocusForGroup(this);

	if (mMessageList->removeItemByValue(sessionId))
	{
		handleItemRemoved(IT_INSTANT_MESSAGE);
	}
	else
	{
		llwarns << "Unable to remove IM Row from the list, sessionID: " << sessionId
			<< llendl;
	}

	// remove all toasts that belong to this session from a screen
	if(mChannel)
		mChannel->removeToastsBySessionID(sessionId);

	// hide chiclet window if there are no items left
	if(isWindowEmpty())
	{
		setVisible(FALSE);
	}
	else
	{
		setFocus(true);
	}
}

void LLIMWellWindow::addObjectRow(const LLUUID& object_id, bool new_message/* = false*/)
{
	if (mMessageList->getItemByValue(object_id) == NULL)
	{
		ObjectRowPanel* item = new ObjectRowPanel(object_id, new_message);
		if (mMessageList->insertItemAfter(mSeparator, item, object_id))
		{
			handleItemAdded(IT_INSTANT_MESSAGE);
		}
		else
		{
			llwarns << "Unable to add Object Row into the list, objectID: " << object_id << llendl;
			item->die();
		}
		reshapeWindow();
	}
}

void LLIMWellWindow::removeObjectRow(const LLUUID& object_id)
{
	if (mMessageList->removeItemByValue(object_id))
	{
		handleItemRemoved(IT_INSTANT_MESSAGE);
	}
	else
	{
		llwarns << "Unable to remove Object Row from the list, objectID: " << object_id << llendl;
	}

	reshapeWindow();
	// hide chiclet window if there are no items left
	if(isWindowEmpty())
	{
		setVisible(FALSE);
	}
}


void LLIMWellWindow::addIMRow(const LLUUID& session_id)
{
	if (hasIMRow(session_id)) return;

	LLIMModel* im_model = LLIMModel::getInstance();
	addIMRow(session_id, 0, im_model->getName(session_id), im_model->getOtherParticipantID(session_id));
	reshapeWindow();
}

bool LLIMWellWindow::hasIMRow(const LLUUID& session_id)
{
	return mMessageList->getItemByValue(session_id);
}

void LLIMWellWindow::closeAll()
{
	// Generate an ignorable alert dialog if there is an active voice IM sesion
	bool need_confirmation = false;
	const LLIMModel& im_model = LLIMModel::instance();
	std::vector<LLSD> values;
	mMessageList->getValues(values);
	for (std::vector<LLSD>::iterator
			 iter = values.begin(),
			 iter_end = values.end();
		 iter != iter_end; ++iter)
	{
		LLIMSpeakerMgr* speaker_mgr =  im_model.getSpeakerManager(*iter);
		if (speaker_mgr && speaker_mgr->isVoiceActive())
		{
			need_confirmation = true;
			break;
		}
	}
	if ( need_confirmation )
	{
		//Bring up a confirmation dialog
		LLNotificationsUtil::add
			("ConfirmCloseAll", LLSD(), LLSD(),
			 boost::bind(&LLIMWellWindow::confirmCloseAll, this, _1, _2));
	}
	else
	{
		closeAllImpl();
	}
}

void LLIMWellWindow::closeAllImpl()
{
	std::vector<LLSD> values;
	mMessageList->getValues(values);

	for (std::vector<LLSD>::iterator
			 iter = values.begin(),
			 iter_end = values.end();
		 iter != iter_end; ++iter)
	{
		LLPanel* panel = mMessageList->getItemByValue(*iter);

		RowPanel* im_panel = dynamic_cast <RowPanel*> (panel);
		if (im_panel)
		{
			gIMMgr->leaveSession(*iter);
			continue;
		}

		ObjectRowPanel* obj_panel = dynamic_cast <ObjectRowPanel*> (panel);
		if (obj_panel)
		{
			LLScriptFloaterManager::instance()
				.removeNotificationByObjectId(*iter);
		}
	}
}

bool LLIMWellWindow::confirmCloseAll(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	switch(option)
	{
	case 0:
		{
			closeAllImpl();
			return  true;
		}
	default:
		break;
	}
	return false;
}

// EOF
