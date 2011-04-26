/** 
 * @file llviewerchat.cpp
 * @brief Builds menus out of items.
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#include "llviewerprecompiledheaders.h"
#include "llviewerchat.h" 

// newview includes
#include "llagent.h" 	// gAgent		
#include "lluicolortable.h"
#include "llviewercontrol.h" // gSavedSettings
#include "llviewerregion.h"
#include "llworld.h"
#include "llinstantmessage.h" //SYSTEM_FROM

// LLViewerChat
LLViewerChat::font_change_signal_t LLViewerChat::sChatFontChangedSignal;

//static 
void LLViewerChat::getChatColor(const LLChat& chat, LLColor4& r_color)
{
	if(chat.mMuted)
	{
		r_color= LLUIColorTable::instance().getColor("LtGray");
	}
	else
	{
		switch(chat.mSourceType)
		{
			case CHAT_SOURCE_SYSTEM:
				r_color = LLUIColorTable::instance().getColor("SystemChatColor"); 
				break;
			case CHAT_SOURCE_AGENT:
				if (chat.mFromID.isNull() || SYSTEM_FROM == chat.mFromName)
				{
					r_color = LLUIColorTable::instance().getColor("SystemChatColor");
				}
				else
				{
					if(gAgentID == chat.mFromID)
					{
						r_color = LLUIColorTable::instance().getColor("UserChatColor");
					}
					else
					{
						r_color = LLUIColorTable::instance().getColor("AgentChatColor");
					}
				}
				break;
			case CHAT_SOURCE_OBJECT:
				if (chat.mChatType == CHAT_TYPE_DEBUG_MSG)
				{
					r_color = LLUIColorTable::instance().getColor("ScriptErrorColor");
				}
				else if ( chat.mChatType == CHAT_TYPE_OWNER )
				{
					r_color = LLUIColorTable::instance().getColor("llOwnerSayChatColor");
				}
				else if ( chat.mChatType == CHAT_TYPE_DIRECT )
				{
					r_color = LLUIColorTable::instance().getColor("DirectChatColor");
				}
				else
				{
					r_color = LLUIColorTable::instance().getColor("ObjectChatColor");
				}
				break;
			default:
				r_color.setToWhite();
		}
		
		if (!chat.mPosAgent.isExactlyZero())
		{
			LLVector3 pos_agent = gAgent.getPositionAgent();
			F32 distance = dist_vec(pos_agent, chat.mPosAgent);
			if (distance > gAgent.getNearChatRadius())
			{
				// diminish far-off chat
				r_color.mV[VALPHA] = 0.8f;
			}
		}
	}
}


//static 
void LLViewerChat::getChatColor(const LLChat& chat, std::string& r_color_name, F32& r_color_alpha)
{
	if(chat.mMuted)
	{
		r_color_name = "LtGray";
	}
	else
	{
		switch(chat.mSourceType)
		{
			case CHAT_SOURCE_SYSTEM:
				r_color_name = "SystemChatColor";
				break;
				
			case CHAT_SOURCE_AGENT:
				if (chat.mFromID.isNull())
				{
					r_color_name = "SystemChatColor";
				}
				else
				{
					if(gAgentID == chat.mFromID)
					{
						r_color_name = "UserChatColor";
					}
					else
					{
						r_color_name = "AgentChatColor";
					}
				}
				break;
				
			case CHAT_SOURCE_OBJECT:
				if (chat.mChatType == CHAT_TYPE_DEBUG_MSG)
				{
					r_color_name = "ScriptErrorColor";
				}
				else if ( chat.mChatType == CHAT_TYPE_OWNER )
				{
					r_color_name = "llOwnerSayChatColor";
				}
				else if ( chat.mChatType == CHAT_TYPE_DIRECT )
				{
					r_color_name = "DirectChatColor";
				}
				else
				{
					r_color_name = "ObjectChatColor";
				}
				break;
			default:
				r_color_name = "White";
		}
		
		if (!chat.mPosAgent.isExactlyZero())
		{
			LLVector3 pos_agent = gAgent.getPositionAgent();
			F32 distance = dist_vec(pos_agent, chat.mPosAgent);
			if (distance > gAgent.getNearChatRadius())
			{
				// diminish far-off chat
				r_color_alpha = 0.8f; 
			}
			else
			{
				r_color_alpha = 1.0f;
			}
		}
	}
	
}


//static 
LLFontGL* LLViewerChat::getChatFont()
{
	S32 font_size = gSavedSettings.getS32("ChatFontSize");
	LLFontGL* fontp = NULL;
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
	
	return fontp;
	
}

//static
S32 LLViewerChat::getChatFontSize()
{
	return gSavedSettings.getS32("ChatFontSize");
}


//static
void LLViewerChat::formatChatMsg(const LLChat& chat, std::string& formated_msg)
{
	std::string tmpmsg = chat.mText;
	
	if(chat.mChatStyle == CHAT_STYLE_IRC)
	{
		formated_msg = chat.mFromName + tmpmsg.substr(3);
	}
	else 
	{
		formated_msg = tmpmsg;
	}

}

//static
std::string LLViewerChat::getSenderSLURL(const LLChat& chat, const LLSD& args)
{
	switch (chat.mSourceType)
	{
	case CHAT_SOURCE_AGENT:
		return LLSLURL("agent", chat.mFromID, "about").getSLURLString();

	case CHAT_SOURCE_OBJECT:
		return getObjectImSLURL(chat, args);

	default:
		llwarns << "Getting SLURL for an unsupported sender type: " << chat.mSourceType << llendl;
	}

	return LLStringUtil::null;
}

//static
std::string LLViewerChat::getObjectImSLURL(const LLChat& chat, const LLSD& args)
{
	std::string url = LLSLURL("objectim", chat.mFromID, "").getSLURLString();
	url += "?name=" + chat.mFromName;
	url += "&owner=" + chat.mOwnerID.asString();

	std::string slurl = args["slurl"].asString();
	if (slurl.empty())
	{
		LLViewerRegion *region = LLWorld::getInstance()->getRegionFromPosAgent(chat.mPosAgent);
		if(region)
		{
			LLSLURL region_slurl(region->getName(), chat.mPosAgent);
			slurl = region_slurl.getLocationString();
		}
	}

	url += "&slurl=" + LLURI::escape(slurl);

	return url;
}

//static 
boost::signals2::connection LLViewerChat::setFontChangedCallback(const font_change_signal_t::slot_type& cb)
{
	return sChatFontChangedSignal.connect(cb);
}

//static
void LLViewerChat::signalChatFontChanged()
{
	// Notify all observers that our font has changed
	sChatFontChangedSignal(getChatFont());
}
