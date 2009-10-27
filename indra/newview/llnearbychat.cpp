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
	: LLDockableFloater(NULL, key)
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

void LLNearbyChat::add_timestamped_line(const LLChat& chat, const LLColor4& color)
{
	S32 font_size = gSavedSettings.getS32("ChatFontSize");

	const LLFontGL* fontp = NULL;
	switch(font_size)
	{
	case 0:
		fontp = LLFontGL::getFontSansSerifSmall();
		break;
	default:
	case 1:
		fontp = LLFontGL::getFontSansSerif();
		break;
	case 2:
		fontp = LLFontGL::getFontSansSerifBig();
		break;
	}

	LLStyle::Params style_params;
	style_params.color(color);
	style_params.font(fontp);
	LLUUID uuid = chat.mFromID;
	std::string from = chat.mFromName;
	std::string message = chat.mText;
	mChatHistory->appendWidgetMessage(chat, style_params);
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
	if (!chat.mMuted)
		add_timestamped_line(chat, color);
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
	rect = gViewerWindow->getWorldViewRect();
}
void LLNearbyChat::setVisible	(BOOL visible)
{
	LLDockableFloater::setVisible(visible);
}
void LLNearbyChat::toggleWindow()
{
}

