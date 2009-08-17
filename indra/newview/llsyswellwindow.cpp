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

//---------------------------------------------------------------------------------
LLSysWellWindow::LLSysWellWindow(const LLSD& key) : LLFloater(LLSD()),
													mChannel(NULL),
													mScrollContainer(NULL),
													mNotificationList(NULL)
{
}

//---------------------------------------------------------------------------------
BOOL LLSysWellWindow::postBuild()
{
	mScrollContainer = getChild<LLScrollContainer>("notification_list_container");
	mNotificationList = getChild<LLScrollingPanelList>("notification_list");

	gViewerWindow->setOnBottomTrayWidthChanged(boost::bind(&LLSysWellWindow::adjustWindowPosition, this)); // *TODO: won't be necessary after docking is realized
	mScrollContainer->setBorderVisible(FALSE);

	mDockTongue = LLUI::getUIImage("windows/Flyout_Pointer.png");

	return TRUE;
}

//---------------------------------------------------------------------------------
LLSysWellWindow::~LLSysWellWindow()
{
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
	adjustWindowPosition();	// *TODO: won't be necessary after docking is realized

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
	adjustWindowPosition();	// *TODO: won't be necessary after docking is realized
	// hide chiclet window if there are no items left
	S32 items_left = mNotificationList->getPanelList().size();
	if(items_left == 0)
		setVisible(FALSE);
}

//---------------------------------------------------------------------------------
void LLSysWellWindow::onItemClick(LLSysWellItem* item)
{
	LLUUID id = item->getID();
	mChannel->loadStoredToastByIDToChannel(id);
}

//---------------------------------------------------------------------------------
void LLSysWellWindow::onItemClose(LLSysWellItem* item)
{
	LLUUID id = item->getID();
	removeItemByID(id);
	mChannel->killToastByNotificationID(id);
}

//---------------------------------------------------------------------------------
void LLSysWellWindow::toggleWindow()
{
	setVisible(!getVisible());
}

//---------------------------------------------------------------------------------
void LLSysWellWindow::setVisible(BOOL visible)
{
	// on Show adjust position of SysWell chiclet's window
	if(visible)
	{
		mChannel->removeAndStoreAllVisibleToasts();
		adjustWindowPosition();	// *TODO: won't be necessary after docking is realized
	}

	LLFloater::setVisible(visible);
}

//---------------------------------------------------------------------------------
void LLSysWellWindow::adjustWindowPosition()	// *TODO: won't be necessary after docking is realized
{
	const S32 WINDOW_MARGIN	= 5;

	LLRect btm_rect = LLBottomTray::getInstance()->getRect();
	LLRect this_rect = getRect();
	setOrigin(btm_rect.mRight - this_rect.getWidth() - WINDOW_MARGIN, WINDOW_MARGIN); 
}
//---------------------------------------------------------------------------------
void LLSysWellWindow::reshapeWindow()
{
	// Get size for scrollbar and floater's header
	const LLUICachedControl<S32> SCROLLBAR_SIZE("UIScrollbarSize", 0);
	const LLUICachedControl<S32> HEADER_SIZE("UIFloaterHeaderSize", 0);

	// Get item list	
	const LLScrollingPanelList::panel_list_t list = mNotificationList->getPanelList();

	// Get height for a scrolling panel list
	S32 list_height	= mNotificationList->getRect().getHeight();

	// Check that the floater doesn't exceed its parent view limits after reshape
	S32 new_height = list_height + LLScrollingPanelList::GAP_BETWEEN_PANELS + HEADER_SIZE;

	if(new_height > MAX_WINDOW_HEIGHT)
	{
		reshape(MIN_WINDOW_WIDTH, MAX_WINDOW_HEIGHT, FALSE);
		mNotificationList->reshape(MIN_PANELLIST_WIDTH - SCROLLBAR_SIZE, list_height, TRUE);
	}
	else
	{
		reshape(MIN_WINDOW_WIDTH, new_height, FALSE);
		mNotificationList->reshape(MIN_PANELLIST_WIDTH, list_height, TRUE);
	}
	
	mNotificationList->updatePanels(TRUE);
}

//---------------------------------------------------------------------------------



