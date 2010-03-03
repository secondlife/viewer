/** 
 * @file llviewerchat.cpp
 * @brief Builds menus out of items.
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
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
#include "llviewerchat.h" 

// newview includes
#include "llagent.h" 	// gAgent		
#include "lluicolortable.h"
#include "llviewercontrol.h" // gSavedSettings
#include "llinstantmessage.h" //SYSTEM_FROM

// LLViewerChat

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

