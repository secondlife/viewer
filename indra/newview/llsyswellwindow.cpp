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


//---------------------------------------------------------------------------------
LLSysWellWindow::LLSysWellWindow(const LLSD& key) : LLFloater(LLSD()),
													mChannel(NULL),
													mScrollContainer(NULL),
													mNotificationList(NULL)
{
	// Ho to use:
	// LLFloaterReg::showInstance("syswell_window");
	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_sys_well.xml", NULL);
}

//---------------------------------------------------------------------------------
BOOL LLSysWellWindow::postBuild()
{
	mCloseBtn = getChild<LLButton>("close_btn");
	mScrollContainer = getChild<LLScrollContainer>("notification_list_container");
	mNotificationList = getChild<LLScrollingPanelList>("notification_list");

	mCloseBtn->setClickedCallback(boost::bind(&LLSysWellWindow::onClickCloseBtn,this));

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
	adjustWindowPosition();

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

	LLScrollingPanelList::panel_list_t::const_iterator it = list.begin(); 
	S32 index = 0;
	while(it != list.end())
	{
		if( dynamic_cast<LLSysWellItem*>(*it)->getID() == id )
			break;
		++it;
		++index;
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
	adjustWindowPosition();
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
void LLSysWellWindow::onClickCloseBtn()
{
	setVisible(false);
}

//---------------------------------------------------------------------------------
void LLSysWellWindow::setVisible(BOOL visible)
{
	// on Show adjust position of SysWell chiclet's window
	if(visible)
	{
		mChannel->removeAndStoreAllVisibleToasts();
		adjustWindowPosition();
	}

	LLFloater::setVisible(visible);
}

//---------------------------------------------------------------------------------
void LLSysWellWindow::adjustWindowPosition()
{
	const S32 WINDOW_MARGIN	= 5;

	LLRect btm_rect = LLBottomTray::getInstance()->getRect();
	LLRect this_rect = getRect();
	setOrigin(btm_rect.mRight - this_rect.getWidth() - WINDOW_MARGIN, WINDOW_MARGIN); 
}
//---------------------------------------------------------------------------------
void LLSysWellWindow::reshapeWindow()
{
	// Get scrollbar size
	const LLUICachedControl<S32> SCROLLBAR_SIZE("UIScrollbarSize", 0);

	// Get item list	
	const LLScrollingPanelList::panel_list_t list = mNotificationList->getPanelList();

	// window's size constants
	const S32 WINDOW_HEADER_HEIGHT	= 30;
	const S32 MAX_WINDOW_HEIGHT		= 200;
	const S32 MIN_WINDOW_WIDTH		= 320;

	// Get height and border's width for a scrolling panel list
	S32 list_height			= mNotificationList->getRect().getHeight();
	S32 list_border_width	= mScrollContainer->getBorderWidth() * 2;

	// Check that the floater doesn't exceed its parent view limits after reshape
	S32 new_height = list_height + WINDOW_HEADER_HEIGHT + list_border_width;

	if(new_height > MAX_WINDOW_HEIGHT)
	{
		reshape(MIN_WINDOW_WIDTH + SCROLLBAR_SIZE, MAX_WINDOW_HEIGHT, FALSE);
	}
	else
	{
		reshape(MIN_WINDOW_WIDTH, new_height, FALSE);
	}
}

//---------------------------------------------------------------------------------



