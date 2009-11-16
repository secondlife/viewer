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

#include "llagent.h" 			// gAgent
#include "llfloaterscriptdebug.h"
#include "llchathistory.h"
#include "llstylemap.h"

#include "lldraghandle.h"
#include "lltrans.h"
#include "llbottomtray.h"
#include "llnearbychatbar.h"

static const S32 RESIZE_BAR_THICKNESS = 3;

LLNearbyChat::LLNearbyChat(const LLSD& key) 
	: LLDockableFloater(NULL, false, key)
	,mChatHistory(NULL)
{
	
}

LLNearbyChat::~LLNearbyChat()
{
}

BOOL LLNearbyChat::postBuild()
{
	//menu
	LLUICtrl::CommitCallbackRegistry::ScopedRegistrar registrar;
	LLUICtrl::EnableCallbackRegistry::ScopedRegistrar enable_registrar;

	enable_registrar.add("NearbyChat.Check", boost::bind(&LLNearbyChat::onNearbyChatCheckContextMenuItem, this, _2));
	registrar.add("NearbyChat.Action", boost::bind(&LLNearbyChat::onNearbyChatContextMenuItemClicked, this, _2));

	
	LLMenuGL* menu = LLUICtrlFactory::getInstance()->createFromFile<LLMenuGL>("menu_nearby_chat.xml", gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());
	if(menu)
		mPopupMenuHandle = menu->getHandle();

	gSavedSettings.declareS32("nearbychat_showicons_and_names",2,"NearByChat header settings",true);

	mChatHistory = getChild<LLChatHistory>("chat_history");

	setCanResize(true);

	if(!LLDockableFloater::postBuild())
		return false;

	if (getDockControl() == NULL)
	{
		setDockControl(new LLDockControl(
			LLBottomTray::getInstance()->getNearbyChatBar(), this,
			getDockTongue(), LLDockControl::LEFT, boost::bind(&LLNearbyChat::getAllowedRect, this, _1)));
	}

	return true;
}


void    LLNearbyChat::applySavedVariables()
{
	if (mRectControl.size() > 1)
	{
		const LLRect& rect = LLUI::sSettingGroups["floater"]->getRect(mRectControl);
		if(!rect.isEmpty() && rect.isValid())
		{
			reshape(rect.getWidth(), rect.getHeight());
			setRect(rect);
		}
	}


	if(!LLUI::sSettingGroups["floater"]->controlExists(mDocStateControl))
	{
		setDocked(true);
	}
	else
	{
		if (mDocStateControl.size() > 1)
		{
			bool dockState = LLUI::sSettingGroups["floater"]->getBOOL(mDocStateControl);
			setDocked(dockState);
		}
	}
}

void	LLNearbyChat::addMessage(const LLChat& chat)
{
	if (chat.mChatType == CHAT_TYPE_DEBUG_MSG)
	{
		if(gSavedSettings.getBOOL("ShowScriptErrors") == FALSE)
			return;
		if (gSavedSettings.getS32("ShowScriptErrorsLocation")== 1)// show error in window //("ScriptErrorsAsChat"))
		{

			LLColor4 txt_color;

			LLViewerChat::getChatColor(chat,txt_color);
			
			LLFloaterScriptDebug::addScriptLine(chat.mText,
												chat.mFromName, 
												txt_color, 
												chat.mFromID);
			return;
		}
	}
	
	if (!chat.mMuted)
	{
		std::string message = chat.mText;
		std::string prefix = message.substr(0, 4);
		if (chat.mChatStyle == CHAT_STYLE_IRC)
		{
			LLStyle::Params append_style_params;
			if (chat.mFromName.size() > 0)
			{
				append_style_params.italic= true;
				LLChat add_chat=chat;
				add_chat.mText = chat.mFromName + " ";
				mChatHistory->appendWidgetMessage(add_chat, append_style_params);
			}
			message = message.substr(3);
			
			LLColor4 txt_color = LLUIColorTable::instance().getColor("White");
			LLViewerChat::getChatColor(chat,txt_color);
			LLFontGL* fontp = LLViewerChat::getChatFont();
			append_style_params.color(txt_color);
			append_style_params.readonly_color(txt_color);
			append_style_params.font(fontp);
			append_style_params.underline = true;
			mChatHistory->appendText(message, FALSE, append_style_params);
		}
		else
		{
			mChatHistory->appendWidgetMessage(chat);
		}
	}
}

void LLNearbyChat::onNearbySpeakers()
{
	LLSD param;
	param["people_panel_tab_name"] = "nearby_panel";
	LLSideTray::getInstance()->showPanel("panel_people",param);
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

void	LLNearbyChat::onOpen(const LLSD& key )
{
	LLNotificationsUI::LLScreenChannelBase* chat_channel = LLNotificationsUI::LLChannelManager::getInstance()->findChannelByID(LLUUID(gSavedSettings.getString("NearByChatChannelUUID")));
	if(chat_channel)
	{
		chat_channel->removeToastsFromChannel();
	}
}

void	LLNearbyChat::setDocked			(bool docked, bool pop_on_undock)
{
	LLDockableFloater::setDocked(docked, pop_on_undock);

	setCanResize(!docked);
}

void LLNearbyChat::setRect	(const LLRect &rect)
{
	LLDockableFloater::setRect(rect);
}

void LLNearbyChat::getAllowedRect(LLRect& rect)
{
	rect = gViewerWindow->getWorldViewRectRaw();
}


