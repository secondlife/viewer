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
//#include "llchatitemscontainerctrl.h"
#include "lliconctrl.h"
#include "llsidetray.h"
#include "llfocusmgr.h"
#include "llresizebar.h"
#include "llresizehandle.h"
#include "llmenugl.h"
#include "llviewermenu.h"//for gMenuHolder

#include "llnearbychathandler.h"
#include "llchannelmanager.h"

//for LLViewerTextEditor support
#include "llagent.h" 			// gAgent
#include "llfloaterscriptdebug.h"
#include "llviewertexteditor.h"
#include "llstylemap.h"

#include "lldraghandle.h"


static const S32 RESIZE_BAR_THICKNESS = 3;

LLNearbyChat::LLNearbyChat(const LLSD& key) :
	LLFloater(key),
	mEChatTearofState(CHAT_PINNED),
	mChatCaptionPanel(NULL),
	mChatHistoryEditor(NULL)
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

	mResizeBar[LLResizeBar::BOTTOM]->setResizeLimits(120,500);
	mResizeBar[LLResizeBar::TOP]->setResizeLimits(120,500);
	mResizeBar[LLResizeBar::LEFT]->setResizeLimits(220,600);
	mResizeBar[LLResizeBar::RIGHT]->setResizeLimits(220,600);

	mResizeHandle[0]->setVisible(false);
	mResizeHandle[1]->setVisible(false);
	mResizeHandle[2]->setVisible(false);
	mResizeHandle[3]->setVisible(false);

	getDragHandle()->setVisible(false);

	//menu
	LLUICtrl::CommitCallbackRegistry::ScopedRegistrar registrar;
	LLUICtrl::EnableCallbackRegistry::ScopedRegistrar enable_registrar;

	enable_registrar.add("NearbyChat.Check", boost::bind(&LLNearbyChat::onNearbyChatCheckContextMenuItem, this, _2));
	registrar.add("NearbyChat.Action", boost::bind(&LLNearbyChat::onNearbyChatContextMenuItemClicked, this, _2));

	
	LLMenuGL* menu = LLUICtrlFactory::getInstance()->createFromFile<LLMenuGL>("menu_nearby_chat.xml", gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());

	if(menu)
		mPopupMenuHandle = menu->getHandle();

	gSavedSettings.declareS32("nearbychat_showicons_and_names",2,"NearByChat header settings",true);

	mChatCaptionPanel = getChild<LLPanel>("chat_caption", false);
	mChatHistoryEditor = getChild<LLViewerTextEditor>("Chat History Editor");

	reshape(getRect().getWidth(), getRect().getHeight(), FALSE);
	
	return LLFloater::postBuild();
}


LLColor4 nearbychat_get_text_color(const LLChat& chat)
{
	LLColor4 text_color;

	if(chat.mMuted)
	{
		text_color.setVec(0.8f, 0.8f, 0.8f, 1.f);
	}
	else
	{
		switch(chat.mSourceType)
		{
		case CHAT_SOURCE_SYSTEM:
			text_color = LLUIColorTable::instance().getColor("SystemChatColor"); 
			break;
		case CHAT_SOURCE_AGENT:
		    if (chat.mFromID.isNull())
			{
				text_color = LLUIColorTable::instance().getColor("SystemChatColor");
			}
			else
			{
				if(gAgentID == chat.mFromID)
				{
					text_color = LLUIColorTable::instance().getColor("UserChatColor");
				}
				else
				{
					text_color = LLUIColorTable::instance().getColor("AgentChatColor");
				}
			}
			break;
		case CHAT_SOURCE_OBJECT:
			if (chat.mChatType == CHAT_TYPE_DEBUG_MSG)
			{
				text_color = LLUIColorTable::instance().getColor("ScriptErrorColor");
			}
			else if ( chat.mChatType == CHAT_TYPE_OWNER )
			{
				text_color = LLUIColorTable::instance().getColor("llOwnerSayChatColor");
			}
			else
			{
				text_color = LLUIColorTable::instance().getColor("ObjectChatColor");
			}
			break;
		default:
			text_color.setToWhite();
		}

		if (!chat.mPosAgent.isExactlyZero())
		{
			LLVector3 pos_agent = gAgent.getPositionAgent();
			F32 distance = dist_vec(pos_agent, chat.mPosAgent);
			if (distance > gAgent.getNearChatRadius())
			{
				// diminish far-off chat
				text_color.mV[VALPHA] = 0.8f;
			}
		}
	}

	return text_color;
}

void nearbychat_add_timestamped_line(LLViewerTextEditor* edit, LLChat chat, const LLColor4& color)
{
	std::string line = chat.mText;

	//chat.mText starts with Avatar Name if entered message was "/me <action>". 
	// In this case output chat message should be "<Avatar Name> <action>". See EXT-656
	// See also process_chat_from_simulator() in the llviewermessage.cpp where ircstyle = TRUE;
	if (CHAT_STYLE_IRC != chat.mChatStyle)
		line = chat.mFromName + ": " + line;

	bool prepend_newline = true;
	if (gSavedSettings.getBOOL("ChatShowTimestamps"))
	{
		edit->appendTime(prepend_newline);
		prepend_newline = false;
	}

	// If the msg is from an agent (not yourself though),
	// extract out the sender name and replace it with the hotlinked name.
	if (chat.mSourceType == CHAT_SOURCE_AGENT &&
		chat.mFromID != LLUUID::null)
	{
		chat.mURL = llformat("secondlife:///app/agent/%s/about",chat.mFromID.asString().c_str());
	}

	// If the chat line has an associated url, link it up to the name.
	if (!chat.mURL.empty()
		&& (line.length() > chat.mFromName.length() && line.find(chat.mFromName,0) == 0))
	{
		std::string start_line = line.substr(0, chat.mFromName.length() + 1);
		line = line.substr(chat.mFromName.length() + 1);
		edit->appendStyledText(start_line, false, prepend_newline, LLStyleMap::instance().lookup(chat.mFromID,chat.mURL));
		prepend_newline = false;
	}

	S32 font_size = gSavedSettings.getS32("ChatFontSize");

	std::string font_name = "";

	if (0 == font_size)
		font_name = "small";
	else if (2 == font_size)
		font_name = "sansserifbig";

	edit->appendColoredText(line, false, prepend_newline, color, font_name);
}



void	LLNearbyChat::addMessage(const LLChat& chat)
{
	LLColor4 color = nearbychat_get_text_color(chat);
	

	if (chat.mChatType == CHAT_TYPE_DEBUG_MSG)
	{
		LLFloaterScriptDebug::addScriptLine(chat.mText,
											chat.mFromName, 
											color, 
											chat.mFromID);
		if (!gSavedSettings.getBOOL("ScriptErrorsAsChat"))
		{
			return;
		}
	}
	
	// could flash the chat button in the status bar here. JC


	mChatHistoryEditor->setParseHTML(TRUE);
	mChatHistoryEditor->setParseHighlights(TRUE);
	
	if (!chat.mMuted)
		nearbychat_add_timestamped_line(mChatHistoryEditor, chat, color);
}

void LLNearbyChat::onNearbySpeakers()
{
	LLSD param;
	param["people_panel_tab_name"] = "nearby_panel";
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

	// *NOTE: we must check mChatCaptionPanel and mChatHistoryEditor against NULL because reshape is called from the 
	// LLView::initFromParams BEFORE postBuild is called and child controls are not exist yet
	LLRect caption_rect;
	if (NULL != mChatCaptionPanel)
	{
		caption_rect = mChatCaptionPanel->getRect();
		caption_rect.setLeftTopAndSize( 2, height - RESIZE_BAR_THICKNESS, width - 4, caption_rect.getHeight());
		mChatCaptionPanel->reshape( width - 4, caption_rect.getHeight(), 1);
		mChatCaptionPanel->setRect(caption_rect);
	}
	
	if (NULL != mChatHistoryEditor)
	{
		LLRect scroll_rect = mChatHistoryEditor->getRect();
		scroll_rect.setLeftTopAndSize( 2, height - caption_rect.getHeight() - RESIZE_BAR_THICKNESS, width - 4, height - caption_rect.getHeight() - RESIZE_BAR_THICKNESS*2);
		mChatHistoryEditor->reshape( width - 4, height - caption_rect.getHeight() - RESIZE_BAR_THICKNESS*2, 1);
		mChatHistoryEditor->setRect(scroll_rect);
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
	LLUICtrl* nearby_speakers_btn = mChatCaptionPanel->getChild<LLUICtrl>("nearby_speakers_btn");
	LLUICtrl* tearoff_btn = mChatCaptionPanel->getChild<LLUICtrl>("tearoff_btn");
	LLUICtrl* close_btn = mChatCaptionPanel->getChild<LLUICtrl>("close_btn");
	
	S32 caption_local_x = x - mChatCaptionPanel->getRect().mLeft;
	S32 caption_local_y = y - mChatCaptionPanel->getRect().mBottom;
	
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

	local_x = caption_local_x - close_btn->getRect().mLeft;
	local_y = caption_local_y - close_btn->getRect().mBottom;
	if(close_btn->pointInView(local_x, local_y))
	{
		setVisible(false);
		bringToFront( x, y );
		return true;
	}

	if(mEChatTearofState == CHAT_UNPINNED && mChatCaptionPanel->pointInView(caption_local_x, caption_local_y) )
	{
		//start draggind
		gFocusMgr.setMouseCapture(this);
		mStart_Y = y;
		mStart_X = x;
		bringToFront( x, y );
		return true;
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
	LLIconCtrl* tearoff_btn = mChatCaptionPanel->getChild<LLIconCtrl>("tearoff_btn",false);
	
	tearoff_btn->setValue("inv_item_landmark_visited.tga");

	const LLRect& parent_rect = gViewerWindow->getRootView()->getRect();
	
	LLRect 	panel_rect;
	panel_rect.setLeftTopAndSize( parent_rect.mLeft+2, parent_rect.mBottom+getRect().getHeight()+4, getRect().getWidth(), getRect().getHeight());
	setRect(panel_rect);

	mResizeBar[LLResizeBar::BOTTOM]->setVisible(false);
	mResizeBar[LLResizeBar::LEFT]->setVisible(false);
	mResizeBar[LLResizeBar::RIGHT]->setVisible(false);

	getDragHandle()->setVisible(false);

}

void	LLNearbyChat::float_panel()
{
	mEChatTearofState = CHAT_UNPINNED;
	LLIconCtrl* tearoff_btn = mChatCaptionPanel->getChild<LLIconCtrl>("tearoff_btn", false);
	
	tearoff_btn->setValue("inv_item_landmark.tga");
	mResizeBar[LLResizeBar::BOTTOM]->setVisible(true);
	mResizeBar[LLResizeBar::LEFT]->setVisible(true);
	mResizeBar[LLResizeBar::RIGHT]->setVisible(true);

	getDragHandle()->setVisible(true);

	translate(4,4);
}

void	LLNearbyChat::onNearbyChatContextMenuItemClicked(const LLSD& userdata)
{
}
bool	LLNearbyChat::onNearbyChatCheckContextMenuItem(const LLSD& userdata)
{
	std::string str = userdata.asString();
	if(str == "nearby_people")
		onNearbySpeakers();	
	return false;
}

BOOL LLNearbyChat::handleRightMouseDown(S32 x, S32 y, MASK mask)
{
	if(mChatCaptionPanel->pointInView(x - mChatCaptionPanel->getRect().mLeft, y - mChatCaptionPanel->getRect().mBottom) )
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

void	LLNearbyChat::onOpen(const LLSD& key )
{
	LLNotificationsUI::LLScreenChannel* chat_channel = LLNotificationsUI::LLChannelManager::getInstance()->findChannelByID(LLUUID(gSavedSettings.getString("NearByChatChannelUUID")));
	if(chat_channel)
	{
		chat_channel->removeToastsFromChannel();
	}
}
