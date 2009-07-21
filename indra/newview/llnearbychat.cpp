/** 
 * @file LLNearbyChat.cpp
 * @brief Nearby chat history scrolling panel implementation
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * 
 * Copyright (c) 2009, Linden Research, Inc.
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

#include "llviewerprecompiledheaders.h"

#include "llnearbychat.h"
#include "llviewercontrol.h"
#include "llviewerwindow.h"
#include "llrootview.h"
#include "llchatitemscontainerctrl.h"
#include "lliconctrl.h"
#include "llsidetray.h"
#include "llfocusmgr.h"
#include "llresizebar.h"
#include "llresizehandle.h"
#include "llmenugl.h"
#include "llviewermenu.h"//for gMenuHolder

static const S32 RESIZE_BAR_THICKNESS = 3;

LLNearbyChat::LLNearbyChat(const LLSD& key) :
	LLFloater(key),
	mEChatTearofState(CHAT_PINNED)
{
}

LLNearbyChat::~LLNearbyChat()
{
}

BOOL LLNearbyChat::postBuild()
{
	//resize bars
	setCanResize(true);

	mResizeBar[LLResizeBar::BOTTOM]->setVisible(false);
	mResizeBar[LLResizeBar::LEFT]->setVisible(false);
	mResizeBar[LLResizeBar::RIGHT]->setVisible(false);

	mResizeHandle[0]->setVisible(false);
	mResizeHandle[1]->setVisible(false);
	mResizeHandle[2]->setVisible(false);
	mResizeHandle[3]->setVisible(false);

	//menu
	LLUICtrl::CommitCallbackRegistry::ScopedRegistrar registrar;
	LLUICtrl::EnableCallbackRegistry::ScopedRegistrar enable_registrar;

	enable_registrar.add("NearbyChat.Check", boost::bind(&LLNearbyChat::onNearbyChatCheckContextMenuItem, this, _2));
	registrar.add("NearbyChat.Action", boost::bind(&LLNearbyChat::onNearbyChatContextMenuItemClicked, this, _2));

	
	LLMenuGL* menu = LLUICtrlFactory::getInstance()->createFromFile<LLMenuGL>("menu_nearby_chat.xml", gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());

	if(menu)
		mPopupMenuHandle = menu->getHandle();

	gSavedSettings.declareS32("nearbychat_showicons_and_names",2,"NearByChat header settings",true);

	LLChatItemsContainerCtrl* panel = getChild<LLChatItemsContainerCtrl>("chat_history",false,false);
	if(panel)
	{
		panel->setHeaderVisibility((EShowItemHeader)gSavedSettings.getS32("nearbychat_showicons_and_names"));
	}
	
	reshape(getRect().getWidth(), getRect().getHeight(), FALSE);
	
	return LLFloater::postBuild();
}

void	LLNearbyChat::addMessage(const LLChat& message)
{
	LLChatItemsContainerCtrl* panel = getChild<LLChatItemsContainerCtrl>("chat_history",false,false);
	if(!panel)
		return;
	panel->addMessage(message);
	
}

void LLNearbyChat::onNearbySpeakers()
{
	LLSD param = "nearby_panel";
	LLSideTray::getInstance()->showPanel("panel_people",param);
}

void LLNearbyChat::onTearOff()
{
	if(mEChatTearofState == CHAT_PINNED)
		float_panel();
	else
		pinn_panel();
}

void LLNearbyChat::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	
	LLFloater::reshape(width, height, called_from_parent);

	LLPanel* caption = getChild<LLPanel>("chat_caption",false,false);

	LLRect resize_rect;
	resize_rect.setLeftTopAndSize( 0, height, width, RESIZE_BAR_THICKNESS);
	if (mResizeBar[LLResizeBar::TOP])
	{
		mResizeBar[LLResizeBar::TOP]->reshape(width,RESIZE_BAR_THICKNESS);
		mResizeBar[LLResizeBar::TOP]->setRect(resize_rect);
	}
	
	resize_rect.setLeftTopAndSize( 0, RESIZE_BAR_THICKNESS, width, RESIZE_BAR_THICKNESS);
	if (mResizeBar[LLResizeBar::BOTTOM])
	{
		mResizeBar[LLResizeBar::BOTTOM]->reshape(width,RESIZE_BAR_THICKNESS);
		mResizeBar[LLResizeBar::BOTTOM]->setRect(resize_rect);
	}

	resize_rect.setLeftTopAndSize( 0, height, RESIZE_BAR_THICKNESS, height);
	if (mResizeBar[LLResizeBar::LEFT])
	{
		mResizeBar[LLResizeBar::LEFT]->reshape(RESIZE_BAR_THICKNESS,height);
		mResizeBar[LLResizeBar::LEFT]->setRect(resize_rect);
	}

	resize_rect.setLeftTopAndSize( width - RESIZE_BAR_THICKNESS, height, RESIZE_BAR_THICKNESS, height);
	if (mResizeBar[LLResizeBar::RIGHT])
	{
		mResizeBar[LLResizeBar::RIGHT]->reshape(RESIZE_BAR_THICKNESS,height);
		mResizeBar[LLResizeBar::RIGHT]->setRect(resize_rect);
	}


	LLRect caption_rect;
	if (caption)
	{
		caption_rect = caption->getRect();
		caption_rect.setLeftTopAndSize( 2, height - RESIZE_BAR_THICKNESS, width - 4, caption_rect.getHeight());
		caption->reshape( width - 4, caption_rect.getHeight(), 1);
		caption->setRect(caption_rect);
	}
	
	LLPanel* scroll_panel = getChild<LLPanel>("chat_history",false,false);
	if (scroll_panel)
	{
		LLRect scroll_rect = scroll_panel->getRect();
		scroll_rect.setLeftTopAndSize( 2, height - caption_rect.getHeight() - RESIZE_BAR_THICKNESS, width - 4, height - caption_rect.getHeight() - RESIZE_BAR_THICKNESS*2);
		scroll_panel->reshape( width - 4, height - caption_rect.getHeight() - RESIZE_BAR_THICKNESS*2, 1);
		scroll_panel->setRect(scroll_rect);
	}
	
	//
	if(mEChatTearofState == CHAT_PINNED)
	{
		const LLRect& parent_rect = gViewerWindow->getRootView()->getRect();
		
		LLRect 	panel_rect;
		panel_rect.setLeftTopAndSize( parent_rect.mLeft+2, parent_rect.mBottom+height+4, width, height);
		setRect(panel_rect);
	}
	else
	{
		LLRect 	panel_rect;
		panel_rect.setLeftTopAndSize( getRect().mLeft, getRect().mTop, width, height);
		setRect(panel_rect);
	}
	
}

BOOL	LLNearbyChat::handleMouseDown	(S32 x, S32 y, MASK mask)
{
	LLPanel* caption = getChild<LLPanel>("chat_caption",false,false);
	if(caption)
	{
		LLUICtrl* nearby_speakers_btn = caption->getChild<LLUICtrl>("nearby_speakers_btn");
		LLUICtrl* tearoff_btn = caption->getChild<LLUICtrl>("tearoff_btn");
		LLUICtrl* close_btn = caption->getChild<LLUICtrl>("close_btn");
		
		S32 caption_local_x = x - caption->getRect().mLeft;
		S32 caption_local_y = y - caption->getRect().mBottom;
		
		if(nearby_speakers_btn && tearoff_btn)
		{
			S32 local_x = caption_local_x - nearby_speakers_btn->getRect().mLeft;
			S32 local_y = caption_local_y - nearby_speakers_btn->getRect().mBottom;
			if(nearby_speakers_btn->pointInView(local_x, local_y))
			{
				onNearbySpeakers();
				bringToFront( x, y );
				return true;
			}
			local_x = caption_local_x - tearoff_btn->getRect().mLeft;
			local_y = caption_local_y- tearoff_btn->getRect().mBottom;
			if(tearoff_btn->pointInView(local_x, local_y))
			{
				onTearOff();
				bringToFront( x, y );
				return true;
			}

			if(close_btn)
			{
				local_x = caption_local_x - close_btn->getRect().mLeft;
				local_y = caption_local_y - close_btn->getRect().mBottom;
				if(close_btn->pointInView(local_x, local_y))
				{
					setVisible(false);
					bringToFront( x, y );
					return true;
				}
			}
		}

		if(mEChatTearofState == CHAT_UNPINNED && caption->pointInView(caption_local_x, caption_local_y) )
		{
			//start draggind
			gFocusMgr.setMouseCapture(this);
			mStart_Y = y;
			mStart_X = x;
			bringToFront( x, y );
			return true;
		}
	}
	
	return LLFloater::handleMouseDown(x,y,mask);
}

BOOL	LLNearbyChat::handleMouseUp(S32 x, S32 y, MASK mask)
{
	if( hasMouseCapture() )
	{
		// Release the mouse
		gFocusMgr.setMouseCapture( NULL );
		mStart_X = 0;
		mStart_Y = 0;
		return true; 
	}

	return LLFloater::handleMouseUp(x,y,mask);
}

BOOL	LLNearbyChat::handleHover(S32 x, S32 y, MASK mask)
{
	if( hasMouseCapture() )
	{
		translate(x-mStart_X,y-mStart_Y);
		return true;
	}
	return LLFloater::handleHover(x,y,mask);
}

void	LLNearbyChat::pinn_panel()
{
	mEChatTearofState = CHAT_PINNED;
	LLPanel* caption = getChild<LLPanel>("chat_caption",false,false);
	LLIconCtrl* tearoff_btn = caption->getChild<LLIconCtrl>("tearoff_btn",false,false);
	
	tearoff_btn->setValue("inv_item_landmark_visited.tga");

	const LLRect& parent_rect = gViewerWindow->getRootView()->getRect();
	
	LLRect 	panel_rect;
	panel_rect.setLeftTopAndSize( parent_rect.mLeft+2, parent_rect.mBottom+getRect().getHeight()+4, getRect().getWidth(), getRect().getHeight());
	setRect(panel_rect);

	mResizeBar[LLResizeBar::BOTTOM]->setVisible(false);
	mResizeBar[LLResizeBar::LEFT]->setVisible(false);
	mResizeBar[LLResizeBar::RIGHT]->setVisible(false);

}

void	LLNearbyChat::float_panel()
{
	mEChatTearofState = CHAT_UNPINNED;
	LLPanel* caption = getChild<LLPanel>("chat_caption",false,false);
	LLIconCtrl* tearoff_btn = caption->getChild<LLIconCtrl>("tearoff_btn",false,false);
	
	tearoff_btn->setValue("inv_item_landmark.tga");
	mResizeBar[LLResizeBar::BOTTOM]->setVisible(true);
	mResizeBar[LLResizeBar::LEFT]->setVisible(true);
	mResizeBar[LLResizeBar::RIGHT]->setVisible(true);
}

void LLNearbyChat::onNearbyChatContextMenuItemClicked(const LLSD& userdata)
{
	LLChatItemsContainerCtrl* panel = getChild<LLChatItemsContainerCtrl>("chat_history",false,false);
	if(!panel)
		return;

	std::string str = userdata.asString();
	if(str == "show_buddy_icons")
		panel->setHeaderVisibility(CHATITEMHEADER_SHOW_ONLY_ICON);
	else if(str == "show_names")
		panel->setHeaderVisibility(CHATITEMHEADER_SHOW_ONLY_NAME);
	else if(str == "show_icons_and_names")
		panel->setHeaderVisibility(CHATITEMHEADER_SHOW_BOTH);

	gSavedSettings.setS32("nearbychat_showicons_and_names", (S32)panel->getHeaderVisibility());


}
bool	LLNearbyChat::onNearbyChatCheckContextMenuItem(const LLSD& userdata)
{
	LLChatItemsContainerCtrl* panel = getChild<LLChatItemsContainerCtrl>("chat_history",false,false);
	if(!panel)
		return false;
	std::string str = userdata.asString();
	if(str == "show_buddy_icons")
		return panel->getHeaderVisibility() == CHATITEMHEADER_SHOW_ONLY_ICON;
	else if(str == "show_names")
		return panel->getHeaderVisibility() == CHATITEMHEADER_SHOW_ONLY_NAME;
	else if(str == "show_icons_and_names")
		return panel->getHeaderVisibility() == CHATITEMHEADER_SHOW_BOTH;
	else if(str == "nearby_people")
		onNearbySpeakers();
	return false;
}

BOOL LLNearbyChat::handleRightMouseDown(S32 x, S32 y, MASK mask)
{
	LLPanel* caption = getChild<LLPanel>("chat_caption",false,false);
	if(caption && caption->pointInView(x - caption->getRect().mLeft, y - caption->getRect().mBottom) )
	{
		LLMenuGL* menu = (LLMenuGL*)mPopupMenuHandle.get();

		if(menu)
		{
			menu->buildDrawLabels();
			menu->updateParent(LLMenuGL::sMenuContainer);
			LLMenuGL::showPopup(this, menu, x, y);
		}
		return true;
	}
	return LLFloater::handleRightMouseDown(x, y, mask);
}

