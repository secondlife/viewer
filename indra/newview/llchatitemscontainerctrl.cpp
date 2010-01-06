/** 
 * @file llchatitemscontainer.cpp
 * @brief chat history scrolling panel implementation
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

#include "llchatitemscontainerctrl.h"
#include "lltextbox.h"

#include "llchatmsgbox.h"
#include "llavatariconctrl.h"
#include "llfloaterreg.h"
#include "lllocalcliprect.h"
#include "lltrans.h"

#include "llviewercontrol.h"
#include "llagentdata.h"

static const S32 msg_left_offset = 10;
static const S32 msg_right_offset = 10;
static const S32 msg_height_pad = 5;

//*******************************************************************************************************************
//LLNearbyChatToastPanel
//*******************************************************************************************************************

LLNearbyChatToastPanel* LLNearbyChatToastPanel::createInstance()
{
	LLNearbyChatToastPanel* item = new LLNearbyChatToastPanel();
	LLUICtrlFactory::getInstance()->buildPanel(item, "panel_chat_item.xml");
	item->setFollows(FOLLOWS_NONE);
	return item;
}

void	LLNearbyChatToastPanel::reshape		(S32 width, S32 height, BOOL called_from_parent )
{
	LLPanel::reshape(width, height,called_from_parent);

	LLUICtrl* msg_text = getChild<LLUICtrl>("msg_text", false);
	LLUICtrl* icon = getChild<LLUICtrl>("avatar_icon", false);

	LLRect msg_text_rect = msg_text->getRect();
	LLRect avatar_rect = icon->getRect();
	
	avatar_rect.setLeftTopAndSize(2,height-2,avatar_rect.getWidth(),avatar_rect.getHeight());
	icon->setRect(avatar_rect);


	msg_text_rect.setLeftTopAndSize( avatar_rect.mRight + msg_left_offset, 
		height - msg_height_pad, 
		width - avatar_rect.mRight  - msg_left_offset - msg_right_offset, 
		height - 2*msg_height_pad);
	msg_text->reshape( msg_text_rect.getWidth(), msg_text_rect.getHeight(), 1);
	msg_text->setRect(msg_text_rect);
}

BOOL LLNearbyChatToastPanel::postBuild()
{
	return LLPanel::postBuild();
}

void LLNearbyChatToastPanel::addMessage(LLSD& notification)
{
	std::string		messageText = notification["message"].asString();		// UTF-8 line of text

	LLChatMsgBox* msg_text = getChild<LLChatMsgBox>("msg_text", false);

	std::string color_name = notification["text_color"].asString();
	
	LLColor4 textColor = LLUIColorTable::instance().getColor(color_name);
	textColor.mV[VALPHA] =notification["color_alpha"].asReal();
	
	S32 font_size = notification["font_size"].asInteger();

	LLFontGL*       messageFont;
	switch(font_size)
	{
		case 0:	messageFont = LLFontGL::getFontSansSerifSmall(); break;
		default:
		case 1: messageFont = LLFontGL::getFontSansSerif();	    break;
		case 2:	messageFont = LLFontGL::getFontSansSerifBig();	break;
	}

	//append text
	{
		LLStyle::Params style_params;
		style_params.color(textColor);
		std::string font_name = LLFontGL::nameFromFont(messageFont);
		std::string font_style_size = LLFontGL::sizeFromFont(messageFont);
		style_params.font.name(font_name);
		style_params.font.size(font_style_size);

		int chat_type = notification["chat_type"].asInteger();

		if(notification["chat_style"].asInteger()== CHAT_STYLE_IRC)
		{
			style_params.font.style = "ITALIC";
		}
		else if( chat_type == CHAT_TYPE_SHOUT)
		{
			style_params.font.style = "BOLD";
		}
		else if( chat_type == CHAT_TYPE_WHISPER)
		{
			style_params.font.style = "ITALIC";
		}
		msg_text->appendText(messageText, TRUE, style_params);
	}

	snapToMessageHeight();

}

void LLNearbyChatToastPanel::init(LLSD& notification)
{
	std::string		messageText = notification["message"].asString();		// UTF-8 line of text
	std::string		fromName = notification["from"].asString();	// agent or object name
	mFromID = notification["from_id"].asUUID();		// agent id or object id
	
	int sType = notification["source"].asInteger();
    mSourceType = (EChatSourceType)sType;
	
	std::string color_name = notification["text_color"].asString();
	
	LLColor4 textColor = LLUIColorTable::instance().getColor(color_name);
	textColor.mV[VALPHA] =notification["color_alpha"].asReal();
	
	S32 font_size = notification["font_size"].asInteger();

	LLFontGL*       messageFont;
	switch(font_size)
	{
		case 0:	messageFont = LLFontGL::getFontSansSerifSmall(); break;
		default:
		case 1: messageFont = LLFontGL::getFontSansSerif();	    break;
		case 2:	messageFont = LLFontGL::getFontSansSerifBig();	break;
	}
	
	LLChatMsgBox* msg_text = getChild<LLChatMsgBox>("msg_text", false);

	msg_text->setText(std::string(""));

	std::string str_sender;
	
	if(gAgentID != mFromID)
		str_sender = fromName;
	else
		str_sender = LLTrans::getString("You");

	str_sender+=" ";

	//append user name
	{
		LLStyle::Params style_params_name;

		LLColor4 userNameColor = LLUIColorTable::instance().getColor("ChatToastAgentNameColor");

		style_params_name.color(userNameColor);
		
		std::string font_name = LLFontGL::nameFromFont(messageFont);
		std::string font_style_size = LLFontGL::sizeFromFont(messageFont);
		style_params_name.font.name(font_name);
		style_params_name.font.size(font_style_size);
		
		msg_text->appendText(str_sender, FALSE, style_params_name);
		
	}

	//append text
	{
		LLStyle::Params style_params;
		style_params.color(textColor);
		std::string font_name = LLFontGL::nameFromFont(messageFont);
		std::string font_style_size = LLFontGL::sizeFromFont(messageFont);
		style_params.font.name(font_name);
		style_params.font.size(font_style_size);

		int chat_type = notification["chat_type"].asInteger();

		if(notification["chat_style"].asInteger()== CHAT_STYLE_IRC)
		{
			style_params.font.style = "ITALIC";
		}
		else if( chat_type == CHAT_TYPE_SHOUT)
		{
			style_params.font.style = "BOLD";
		}
		else if( chat_type == CHAT_TYPE_WHISPER)
		{
			style_params.font.style = "ITALIC";
		}
		msg_text->appendText(messageText, FALSE, style_params);
	}


	snapToMessageHeight();

	mIsDirty = true;//will set Avatar Icon in draw
}

void	LLNearbyChatToastPanel::snapToMessageHeight	()
{
	LLChatMsgBox* text_box = getChild<LLChatMsgBox>("msg_text", false);
	S32 new_height = llmax (text_box->getTextPixelHeight() + 2*text_box->getVPad() + 2*msg_height_pad, 25);
	
	LLRect panel_rect = getRect();

	panel_rect.setLeftTopAndSize( panel_rect.mLeft, panel_rect.mTop, panel_rect.getWidth(), new_height);
	
	reshape( getRect().getWidth(), getRect().getHeight(), 1);
	
	setRect(panel_rect);

}

void LLNearbyChatToastPanel::onMouseLeave			(S32 x, S32 y, MASK mask)
{
	
}
void LLNearbyChatToastPanel::onMouseEnter				(S32 x, S32 y, MASK mask)
{
	if(mSourceType != CHAT_SOURCE_AGENT)
		return;
}

BOOL	LLNearbyChatToastPanel::handleMouseDown	(S32 x, S32 y, MASK mask)
{
	return LLPanel::handleMouseDown(x,y,mask);
}

BOOL	LLNearbyChatToastPanel::handleMouseUp	(S32 x, S32 y, MASK mask)
{
	if(mSourceType != CHAT_SOURCE_AGENT)
		return LLPanel::handleMouseUp(x,y,mask);

	LLChatMsgBox* text_box = getChild<LLChatMsgBox>("msg_text", false);
	S32 local_x = x - text_box->getRect().mLeft;
	S32 local_y = y - text_box->getRect().mBottom;
	
	//if text_box process mouse up (ussually this is click on url) - we didn't show nearby_chat.
	if (text_box->pointInView(local_x, local_y) )
	{
		if (text_box->handleMouseUp(local_x,local_y,mask) == TRUE)
			return TRUE;
		else
		{
			LLFloaterReg::showInstance("nearby_chat",LLSD());
			return FALSE;
		}
	}

	LLFloaterReg::showInstance("nearby_chat",LLSD());
	return LLPanel::handleMouseUp(x,y,mask);
}

void	LLNearbyChatToastPanel::setHeaderVisibility(EShowItemHeader e)
{
	LLUICtrl* icon = getChild<LLUICtrl>("avatar_icon", false);
	if(icon)
		icon->setVisible(e == CHATITEMHEADER_SHOW_ONLY_ICON || e==CHATITEMHEADER_SHOW_BOTH);

}

bool	LLNearbyChatToastPanel::canAddText	()
{
	LLChatMsgBox* msg_text = findChild<LLChatMsgBox>("msg_text");
	if(!msg_text)
		return false;
	return msg_text->getLineCount()<10;
}

BOOL	LLNearbyChatToastPanel::handleRightMouseDown(S32 x, S32 y, MASK mask)
{
	LLUICtrl* avatar_icon = getChild<LLUICtrl>("avatar_icon", false);

	S32 local_x = x - avatar_icon->getRect().mLeft;
	S32 local_y = y - avatar_icon->getRect().mBottom;

	//eat message for avatar icon if msg was from object
	if(avatar_icon->pointInView(local_x, local_y) && mSourceType != CHAT_SOURCE_AGENT)
		return TRUE;
	return LLPanel::handleRightMouseDown(x,y,mask);
}
void LLNearbyChatToastPanel::draw()
{
	if(mIsDirty)
	{
		LLAvatarIconCtrl* icon = getChild<LLAvatarIconCtrl>("avatar_icon", false);
		if(icon)
		{
			icon->setDrawTooltip(mSourceType == CHAT_SOURCE_AGENT);
			icon->setValue(mFromID);
		}
		mIsDirty = false;
	}
	LLToastPanelBase::draw();
}


