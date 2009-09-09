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

#include "llsyswellwindow.h"

#include "llbottomtray.h"
#include "llviewercontrol.h"
#include "llviewerwindow.h"

#include "llchiclet.h"
//---------------------------------------------------------------------------------
LLSysWellWindow::LLSysWellWindow(const LLSD& key) : LLDockableFloater(NULL, key),
													mChannel(NULL),
													mScrollContainer(NULL),
													mNotificationList(NULL)
{
	LLIMMgr::getInstance()->addSessionObserver(this);
	LLIMChiclet::sFindChicletsSignal.connect(boost::bind(&LLSysWellWindow::findIMChiclet, this, _1));
}

//---------------------------------------------------------------------------------
BOOL LLSysWellWindow::postBuild()
{
	mScrollContainer = getChild<LLScrollContainer>("notification_list_container");
	mTwinListPanel = getChild<LLPanel>("twin_list_panel");
	mNotificationList = getChild<LLScrollingPanelList>("notification_list");
	mIMRowList = getChild<LLScrollingPanelList>("im_row_panel_list");

	mScrollContainer->setBorderVisible(FALSE);

	return LLDockableFloater::postBuild();
}

//---------------------------------------------------------------------------------
LLSysWellWindow::~LLSysWellWindow()
{
	LLIMMgr::getInstance()->removeSessionObserver(this);
}

//---------------------------------------------------------------------------------
void LLSysWellWindow::addItem(LLSysWellItem::Params p)
{
	// do not add clones
	if( findItemByID(p.notification_id) >= 0 )
		return;

	LLSysWellItem* new_item = new LLSysWellItem(p);
	mNotificationList->addPanel(dynamic_cast<LLScrollingPanel*>(new_item));
	reshapeWindow();

	new_item->setOnItemCloseCallback(boost::bind(&LLSysWellWindow::onItemClose, this, _1));
	new_item->setOnItemClickCallback(boost::bind(&LLSysWellWindow::onItemClick, this, _1));
}

//---------------------------------------------------------------------------------
void LLSysWellWindow::clear()
{
 // *TODO: fill later
}

//---------------------------------------------------------------------------------
S32 LLSysWellWindow::findItemByID(const LLUUID& id)
{
	const LLScrollingPanelList::panel_list_t list = mNotificationList->getPanelList();
	if(list.size() == 0)
		return -1;

	LLScrollingPanelList::panel_list_t::const_iterator it; 
	S32 index = 0;
	for(it = list.begin(); it != list.end(); ++it, ++index)
	{
		if( dynamic_cast<LLSysWellItem*>(*it)->getID() == id )
			break;
	}

	if(it == list.end())
		return -1;
	else
		return index;

}

//---------------------------------------------------------------------------------
void LLSysWellWindow::removeItemByID(const LLUUID& id)
{
	S32 index = findItemByID(id);

	if(index >= 0)
		mNotificationList->removePanel(index);
	else
		return;

	reshapeWindow();
}

//---------------------------------------------------------------------------------
void LLSysWellWindow::onItemClick(LLSysWellItem* item)
{
	LLUUID id = item->getID();
	if(mChannel)
		mChannel->loadStoredToastByIDToChannel(id);
}

//---------------------------------------------------------------------------------
void LLSysWellWindow::onItemClose(LLSysWellItem* item)
{
	LLUUID id = item->getID();
	removeItemByID(id);
	if(mChannel)
		mChannel->killToastByNotificationID(id);

	// hide chiclet window if there are no items left
	setVisible(!isWindowEmpty());
}

//---------------------------------------------------------------------------------
void LLSysWellWindow::toggleWindow()
{
	if (getDockControl() == NULL)
	{
		setDockControl(new LLDockControl(
				LLBottomTray::getInstance()->getSysWell(), this,
				getDockTongue(), LLDockControl::TOP, isDocked()));
	}
	setVisible(!getVisible());
	//set window in foreground
	setFocus(getVisible());
}

//---------------------------------------------------------------------------------
void LLSysWellWindow::setVisible(BOOL visible)
{
	// on Show adjust position of SysWell chiclet's window
	if(visible)
	{
		if (LLBottomTray::instanceExists())
		{
			LLBottomTray::getInstance()->getSysWell()->setToggleState(TRUE);
		}
		if(mChannel)
			mChannel->removeAndStoreAllVisibleToasts();
	}
	else
	{
		if (LLBottomTray::instanceExists())
		{
			LLBottomTray::getInstance()->getSysWell()->setToggleState(FALSE);
		}
	}
	if(mChannel)
		mChannel->setShowToasts(!visible);

	LLFloater::setVisible(visible);
}

//---------------------------------------------------------------------------------
void LLSysWellWindow::reshapeWindow()
{
	// Get size for scrollbar and floater's header
	const LLUICachedControl<S32> SCROLLBAR_SIZE("UIScrollbarSize", 0);
	const LLUICachedControl<S32> HEADER_SIZE("UIFloaterHeaderSize", 0);

	LLRect notif_list_rect = mNotificationList->getRect();
	LLRect im_list_rect = mIMRowList->getRect();
	LLRect panel_rect = mTwinListPanel->getRect();

	S32 notif_list_height = notif_list_rect.getHeight();
	S32 im_list_height = im_list_rect.getHeight();

	S32 new_panel_height = notif_list_height + LLScrollingPanelList::GAP_BETWEEN_PANELS + im_list_height;
	S32 new_window_height = new_panel_height + LLScrollingPanelList::GAP_BETWEEN_PANELS + HEADER_SIZE;

	U32 twinListWidth = 0;

	if (new_window_height > MAX_WINDOW_HEIGHT)
	{
		twinListWidth = MIN_PANELLIST_WIDTH - SCROLLBAR_SIZE;
		new_window_height = MAX_WINDOW_HEIGHT;
	}
	else
	{
		twinListWidth = MIN_PANELLIST_WIDTH;
	}

	reshape(MIN_WINDOW_WIDTH, new_window_height, FALSE);
	mTwinListPanel->reshape(twinListWidth, new_panel_height, TRUE);
	mNotificationList->reshape(twinListWidth, notif_list_height, TRUE);
	mIMRowList->reshape(twinListWidth, im_list_height, TRUE);

	// arrange panel and lists
	// move panel
	panel_rect.setLeftTopAndSize(1, new_panel_height, twinListWidth, new_panel_height);
	mTwinListPanel->setRect(panel_rect);
	// move notif list panel
	notif_list_rect.setLeftTopAndSize(notif_list_rect.mLeft, new_panel_height, twinListWidth, notif_list_height);
	mNotificationList->setRect(notif_list_rect);
	// move IM list panel
	im_list_rect.setLeftTopAndSize(im_list_rect.mLeft, notif_list_rect.mBottom - LLScrollingPanelList::GAP_BETWEEN_PANELS, twinListWidth, im_list_height);
	mIMRowList->setRect(im_list_rect);

	mNotificationList->updatePanels(TRUE);
	mIMRowList->updatePanels(TRUE);
}

//---------------------------------------------------------------------------------
LLSysWellWindow::RowPanel * LLSysWellWindow::findIMRow(const LLUUID& sessionId)
{
	RowPanel * res = NULL;
	const LLScrollingPanelList::panel_list_t &list = mIMRowList->getPanelList();
	if (!list.empty())
	{
		for (LLScrollingPanelList::panel_list_t::const_iterator iter = list.begin(); iter != list.end(); ++iter)
		{
			RowPanel *panel = static_cast<RowPanel*> (*iter);
			if (panel->mChiclet->getSessionId() == sessionId)
			{
				res = panel;
				break;
			}
		}
	}
	return res;
}

//---------------------------------------------------------------------------------
LLChiclet* LLSysWellWindow::findIMChiclet(const LLUUID& sessionId)
{
	LLChiclet* res = NULL;
	RowPanel* panel = findIMRow(sessionId);
	if (panel != NULL)
	{
		res = panel->mChiclet;
	}

	return res;
}

//---------------------------------------------------------------------------------
void LLSysWellWindow::addIMRow(const LLUUID& sessionId, S32 chicletCounter,
		const std::string& name, const LLUUID& otherParticipantId)
{

	mIMRowList->addPanel(new RowPanel(this, sessionId, chicletCounter, name, otherParticipantId));
}

//---------------------------------------------------------------------------------
void LLSysWellWindow::delIMRow(const LLUUID& sessionId)
{
	RowPanel *panel = findIMRow(sessionId);
	if (panel != NULL)
	{
		mIMRowList->removePanel(panel);
	}

	// hide chiclet window if there are no items left
	setVisible(!isWindowEmpty());
}

//---------------------------------------------------------------------------------
bool LLSysWellWindow::isWindowEmpty()
{
	if(mIMRowList->getPanelList().size() == 0 && LLBottomTray::getInstance()->getSysWell()->getCounter() == 0)
	{
		return true;
	}
	else
	{
		return false;
	}
}

//---------------------------------------------------------------------------------
//virtual
void LLSysWellWindow::sessionAdded(const LLUUID& sessionId,
		const std::string& name, const LLUUID& otherParticipantId)
{
	if (findIMRow(sessionId) == NULL)
	{
		S32 chicletCounter = 0;
		LLIMModel::LLIMSession* session = get_if_there(LLIMModel::sSessionsMap,
				sessionId, (LLIMModel::LLIMSession*) NULL);
		if (session != NULL)
		{
			chicletCounter = session->mNumUnread;
		}
		addIMRow(sessionId, chicletCounter, name, otherParticipantId);
		reshapeWindow();
	}
}

//---------------------------------------------------------------------------------
//virtual
void LLSysWellWindow::sessionRemoved(const LLUUID& sessionId)
{
	delIMRow(sessionId);
	reshapeWindow();
	LLBottomTray::getInstance()->getSysWell()->updateUreadIMNotifications();
}

//---------------------------------------------------------------------------------
LLSysWellWindow::RowPanel::RowPanel(const LLSysWellWindow* parent, const LLUUID& sessionId,
		S32 chicletCounter, const std::string& name, const LLUUID& otherParticipantId) :
		LLScrollingPanel(LLPanel::Params()), mParent(parent)
{
	LLUICtrlFactory::getInstance()->buildPanel(this, "panel_activeim_row.xml", NULL);

	mChiclet = getChild<LLIMChiclet>("chiclet");
	mChiclet->setCounter(chicletCounter);
	mChiclet->setSessionId(sessionId);
	mChiclet->setIMSessionName(name);
	mChiclet->setOtherParticipantId(otherParticipantId);

	LLTextBox* contactName = getChild<LLTextBox>("contact_name");
	contactName->setValue(name);

	mCloseBtn = getChild<LLButton>("hide_btn");
	mCloseBtn->setCommitCallback(boost::bind(&LLSysWellWindow::RowPanel::onClose, this));
}

//---------------------------------------------------------------------------------
LLSysWellWindow::RowPanel::~RowPanel()
{
}

//---------------------------------------------------------------------------------
void LLSysWellWindow::RowPanel::onClose()
{
	mParent->mIMRowList->removePanel(this);
	gIMMgr->removeSession(mChiclet->getSessionId());
}

//---------------------------------------------------------------------------------
void LLSysWellWindow::RowPanel::onMouseEnter(S32 x, S32 y, MASK mask)
{
	setTransparentColor(LLUIColorTable::instance().getColor("SysWellItemSelected"));
}

//---------------------------------------------------------------------------------
void LLSysWellWindow::RowPanel::onMouseLeave(S32 x, S32 y, MASK mask)
{
	setTransparentColor(LLUIColorTable::instance().getColor("SysWellItemUnselected"));
}

//---------------------------------------------------------------------------------
// virtual
BOOL LLSysWellWindow::RowPanel::handleMouseDown(S32 x, S32 y, MASK mask)
{
	// Pass the mouse down event to the chiclet (EXT-596).
	if (!mChiclet->pointInView(x, y) && !mCloseBtn->getRect().pointInRect(x, y)) // prevent double call of LLIMChiclet::onMouseDown()
		mChiclet->onMouseDown();

	return LLPanel::handleMouseDown(x, y, mask);
}

//---------------------------------------------------------------------------------
void LLSysWellWindow::RowPanel::updatePanel(BOOL allow_modify)
{
	S32 parent_width = getParent()->getRect().getWidth();
	S32 panel_height = getRect().getHeight();

	reshape(parent_width, panel_height, TRUE);
}

//---------------------------------------------------------------------------------
