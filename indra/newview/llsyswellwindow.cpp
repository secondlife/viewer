/** 
 * @file llsyswellwindow.cpp
 * @brief                                    // TODO
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
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

#include "llviewerprecompiledheaders.h" // must be first include
#include "llsyswellwindow.h"

#include "llchiclet.h"
#include "llchicletbar.h"
#include "llflatlistview.h"
#include "llfloaterreg.h"
#include "llnotificationmanager.h"
#include "llnotificationsutil.h"
#include "llscriptfloater.h"
#include "llspeakers.h"
#include "lltoastpanel.h"

//---------------------------------------------------------------------------------
LLSysWellWindow::LLSysWellWindow(const LLSD& key) : LLTransientDockableFloater(NULL, true,  key),
													mChannel(NULL),
													mMessageList(NULL),
													mSysWellChiclet(NULL),
													NOTIFICATION_WELL_ANCHOR_NAME("notification_well_panel"),
													IM_WELL_ANCHOR_NAME("im_well_panel"),
													mIsReshapedByUser(false)

{
	setOverlapsScreenChannel(true);
}

//---------------------------------------------------------------------------------
BOOL LLSysWellWindow::postBuild()
{
	mMessageList = getChild<LLFlatListView>("notification_list");

	// get a corresponding channel
	initChannel();

	return LLTransientDockableFloater::postBuild();
}

//---------------------------------------------------------------------------------
void LLSysWellWindow::setMinimized(BOOL minimize)
{
	LLTransientDockableFloater::setMinimized(minimize);
}

//---------------------------------------------------------------------------------
void LLSysWellWindow::handleReshape(const LLRect& rect, bool by_user)
{
	mIsReshapedByUser |= by_user; // mark floater that it is reshaped by user
	LLTransientDockableFloater::handleReshape(rect, by_user);
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
	if(NULL != mSysWellChiclet)
	{
		mSysWellChiclet->updateWidget(isWindowEmpty());
	}
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
		if (NULL != mSysWellChiclet)
		{
			mSysWellChiclet->updateWidget(isWindowEmpty());
		}
		reshapeWindow();
	}
	else
	{
		LL_WARNS() << "Unable to remove notification from the list, ID: " << id
			<< LL_ENDL;
	}

	// hide chiclet window if there are no items left
	if(isWindowEmpty())
	{
		setVisible(FALSE);
	}
}

 LLPanel * LLSysWellWindow::findItemByID(const LLUUID& id)
{
       return mMessageList->getItemByValue(id);
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
		LL_WARNS() << "LLSysWellWindow::initChannel() - could not get a requested screen channel" << LL_ENDL;
	}
}

//---------------------------------------------------------------------------------
void LLSysWellWindow::setVisible(BOOL visible)
{
	if (visible)
	{
		if (NULL == getDockControl() && getDockTongue().notNull())
		{
			setDockControl(new LLDockControl(
				LLChicletBar::getInstance()->getChild<LLView>(getAnchorViewName()), this,
				getDockTongue(), LLDockControl::BOTTOM));
		}
	}

	// do not show empty window
	if (NULL == mMessageList || isWindowEmpty()) visible = FALSE;

	LLTransientDockableFloater::setVisible(visible);

	// update notification channel state	
	initChannel(); // make sure the channel still exists
	if(mChannel)
	{
		mChannel->updateShowToastsState();
		mChannel->redrawToasts();
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

	if (!mIsReshapedByUser) // Don't reshape Well window, if it ever was reshaped by user. See EXT-5715.
	{
		S32 notif_list_height = mMessageList->getItemsRect().getHeight() + 2 * mMessageList->getBorderWidth();

		LLRect curRect = getRect();

		S32 new_window_height = notif_list_height + parent_list_delta_height;

		if (new_window_height > MAX_WINDOW_HEIGHT)
		{
			new_window_height = MAX_WINDOW_HEIGHT;
		}
		S32 newWidth = curRect.getWidth() < MIN_WINDOW_WIDTH ? MIN_WINDOW_WIDTH	: curRect.getWidth();

		curRect.setLeftTopAndSize(curRect.mLeft, curRect.mTop, newWidth, new_window_height);
		reshape(curRect.getWidth(), curRect.getHeight(), TRUE);
		setRect(curRect);
	}

	// update notification channel state
	// update on a window reshape is important only when a window is visible and docked
	if(mChannel && getVisible() && isDocked())
	{
		mChannel->updateShowToastsState();
	}
}

//---------------------------------------------------------------------------------
bool LLSysWellWindow::isWindowEmpty()
{
	return mMessageList->size() == 0;
}

/************************************************************************/
/*         ObjectRowPanel implementation                                */
/************************************************************************/

LLIMWellWindow::ObjectRowPanel::ObjectRowPanel(const LLUUID& notification_id, bool new_message/* = false*/)
 : LLPanel()
 , mChiclet(NULL)
{
	buildFromFile( "panel_active_object_row.xml");

	initChiclet(notification_id);

	LLTextBox* obj_name = getChild<LLTextBox>("object_name");
	obj_name->setValue(LLScriptFloaterManager::getObjectName(notification_id));

	mCloseBtn = getChild<LLButton>("hide_btn");
	mCloseBtn->setCommitCallback(boost::bind(&LLIMWellWindow::ObjectRowPanel::onClosePanel, this));
}

//---------------------------------------------------------------------------------
LLIMWellWindow::ObjectRowPanel::~ObjectRowPanel()
{
}

//---------------------------------------------------------------------------------
void LLIMWellWindow::ObjectRowPanel::onClosePanel()
{
	LLScriptFloaterManager::getInstance()->removeNotification(mChiclet->getSessionId());
}

void LLIMWellWindow::ObjectRowPanel::initChiclet(const LLUUID& notification_id, bool new_message/* = false*/)
{
	// Choose which of the pre-created chiclets to use.
	switch(LLScriptFloaterManager::getObjectType(notification_id))
	{
	case LLScriptFloaterManager::OBJ_GIVE_INVENTORY:
		mChiclet = getChild<LLInvOfferChiclet>("inv_offer_chiclet");
		break;
	default:
		mChiclet = getChild<LLScriptChiclet>("object_chiclet");
		break;
	}

	mChiclet->setVisible(true);
	mChiclet->setSessionId(notification_id);
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
	{
		mChiclet->onMouseDown();
		return TRUE;
	}

	return LLPanel::handleMouseDown(x, y, mask);
}

// virtual
BOOL LLIMWellWindow::ObjectRowPanel::handleRightMouseDown(S32 x, S32 y, MASK mask)
{
	return mChiclet->handleRightMouseDown(x, y, mask);
}

/************************************************************************/
/*         LLIMWellWindow  implementation                               */
/************************************************************************/

//////////////////////////////////////////////////////////////////////////
// PUBLIC METHODS
LLIMWellWindow::LLIMWellWindow(const LLSD& key)
: LLSysWellWindow(key)
{
}

LLIMWellWindow::~LLIMWellWindow()
{
}

// static
LLIMWellWindow* LLIMWellWindow::getInstance(const LLSD& key /*= LLSD()*/)
{
	return LLFloaterReg::getTypedInstance<LLIMWellWindow>("im_well_window", key);
}


// static
LLIMWellWindow* LLIMWellWindow::findInstance(const LLSD& key /*= LLSD()*/)
{
	return LLFloaterReg::findTypedInstance<LLIMWellWindow>("im_well_window", key);
}

BOOL LLIMWellWindow::postBuild()
{
	BOOL rv = LLSysWellWindow::postBuild();
	setTitle(getString("title_im_well_window"));

	LLIMChiclet::sFindChicletsSignal.connect(boost::bind(&LLIMWellWindow::findObjectChiclet, this, _1));

	return rv;
}

LLChiclet* LLIMWellWindow::findObjectChiclet(const LLUUID& notification_id)
{
	if (!mMessageList) return NULL;

	LLChiclet* res = NULL;
	ObjectRowPanel* panel = mMessageList->getTypedItemByValue<ObjectRowPanel>(notification_id);
	if (panel != NULL)
	{
		res = panel->mChiclet;
	}

	return res;
}

//////////////////////////////////////////////////////////////////////////
// PRIVATE METHODS

void LLIMWellWindow::addObjectRow(const LLUUID& notification_id, bool new_message/* = false*/)
{
	if (mMessageList->getItemByValue(notification_id) == NULL)
	{
		ObjectRowPanel* item = new ObjectRowPanel(notification_id, new_message);
		if (!mMessageList->addItem(item, notification_id))
		{
			LL_WARNS() << "Unable to add Object Row into the list, notificationID: " << notification_id << LL_ENDL;
			item->die();
		}
		reshapeWindow();
	}
}

void LLIMWellWindow::removeObjectRow(const LLUUID& notification_id)
{
	if (!mMessageList->removeItemByValue(notification_id))
	{
		LL_WARNS() << "Unable to remove Object Row from the list, notificationID: " << notification_id << LL_ENDL;
	}

	reshapeWindow();
	// hide chiclet window if there are no items left
	if(isWindowEmpty())
	{
		setVisible(FALSE);
	}
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

		ObjectRowPanel* obj_panel = dynamic_cast <ObjectRowPanel*> (panel);
		if (obj_panel)
		{
			LLScriptFloaterManager::instance().removeNotification(*iter);
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


