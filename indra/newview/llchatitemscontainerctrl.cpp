/** 
 * @file llchatitemscontainer.cpp
 * @brief chat history scrolling panel implementation
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

#include "llchatitemscontainerctrl.h"
#include "lltextbox.h"

#include "llavatariconctrl.h"
#include "llcommandhandler.h"
#include "llfloaterreg.h"
#include "lllocalcliprect.h"
#include "lltrans.h"
#include "llfloaterimnearbychat.h"

#include "llviewercontrol.h"
#include "llagentdata.h"

#include "llslurl.h"

static const S32 msg_left_offset = 10;
static const S32 msg_right_offset = 10;
static const S32 msg_height_pad = 5;

//*******************************************************************************************************************
// LLObjectHandler
//*******************************************************************************************************************

// handle secondlife:///app/object/<ID>/inspect SLURLs
class LLObjectHandler : public LLCommandHandler
{
public:
	LLObjectHandler() : LLCommandHandler("object", UNTRUSTED_BLOCK) { }

	bool handle(const LLSD& params, const LLSD& query_map, LLMediaCtrl* web)
	{
		if (params.size() < 2) return false;

		LLUUID object_id;
		if (!object_id.set(params[0], FALSE))
		{
			return false;
		}

		const std::string verb = params[1].asString();

		if (verb == "inspect")
		{
			LLFloaterReg::showInstance("inspect_object", LLSD().with("object_id", object_id));
			return true;
		}

		return false;
	}
};

LLObjectHandler gObjectHandler;

//*******************************************************************************************************************
//LLFloaterIMNearbyChatToastPanel
//*******************************************************************************************************************

LLFloaterIMNearbyChatToastPanel* LLFloaterIMNearbyChatToastPanel::createInstance()
{
	LLFloaterIMNearbyChatToastPanel* item = new LLFloaterIMNearbyChatToastPanel();
	item->buildFromFile("panel_chat_item.xml");
	item->setFollows(FOLLOWS_NONE);
	return item;
}

void	LLFloaterIMNearbyChatToastPanel::reshape		(S32 width, S32 height, BOOL called_from_parent )
{
	LLPanel::reshape(width, height,called_from_parent);

	// reshape() may be called from LLView::initFromParams() before the children are created.
	// We call findChild() instead of getChild() here to avoid creating dummy controls.
	LLUICtrl* msg_text = findChild<LLUICtrl>("msg_text", false);
	LLUICtrl* icon = findChild<LLUICtrl>("avatar_icon", false);

	if (!msg_text || !icon)
	{
		return;
	}

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

BOOL LLFloaterIMNearbyChatToastPanel::postBuild()
{
	return LLPanel::postBuild();
}

void LLFloaterIMNearbyChatToastPanel::addMessage(LLSD& notification)
{
	std::string		messageText = notification["message"].asString();		// UTF-8 line of text


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
		mMsgText->appendText(messageText, TRUE, style_params);
	}

	snapToMessageHeight();

}

void LLFloaterIMNearbyChatToastPanel::init(LLSD& notification)
{
	std::string		messageText = notification["message"].asString();		// UTF-8 line of text
	std::string		fromName = notification["from"].asString();	// agent or object name
	mFromID = notification["from_id"].asUUID();		// agent id or object id
	mFromName = fromName;
	
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
	
	mMsgText = getChild<LLChatMsgBox>("msg_text", false);
	mMsgText->setContentTrusted(false);

	mMsgText->setText(std::string(""));

	if ( notification["chat_style"].asInteger() != CHAT_STYLE_IRC )
	{
		std::string str_sender;

		str_sender = fromName;

		str_sender+=" ";

		//append sender name
		if (mSourceType == CHAT_SOURCE_AGENT || mSourceType == CHAT_SOURCE_OBJECT)
		{
			LLStyle::Params style_params_name;

			LLColor4 user_name_color = LLUIColorTable::instance().getColor("HTMLLinkColor");
			style_params_name.color(user_name_color);

			std::string font_name = LLFontGL::nameFromFont(messageFont);
			std::string font_style_size = LLFontGL::sizeFromFont(messageFont);
			style_params_name.font.name(font_name);
			style_params_name.font.size(font_style_size);

			style_params_name.link_href = notification["sender_slurl"].asString();
			style_params_name.is_link = true;

			mMsgText->appendText(str_sender, FALSE, style_params_name);

		}
		else
		{
			mMsgText->appendText(str_sender, false);
		}
	}

	S32 chars_in_line = mMsgText->getRect().getWidth() / messageFont->getWidth("c");
	S32 max_lines = notification["available_height"].asInteger() / (mMsgText->getTextPixelHeight() + 4);
	S32 new_line_chars = std::count(messageText.begin(), messageText.end(), '\n');
	S32 lines_count = (messageText.size() - new_line_chars) / chars_in_line + new_line_chars + 1;

	//Remove excessive chars if message is not fit in available height. MAINT-6891
	if(lines_count > max_lines)
	{
		while(lines_count > max_lines)
		{
			std::size_t nl_pos = messageText.rfind('\n');
			if (nl_pos != std::string::npos)
			{
				nl_pos = nl_pos > messageText.length() - chars_in_line? nl_pos : messageText.length() - chars_in_line;
				messageText.erase(messageText.begin() + nl_pos, messageText.end());
			}
			else
			{
				messageText.erase(messageText.end() - chars_in_line, messageText.end());
			}
			new_line_chars = std::count(messageText.begin(), messageText.end(), '\n');
			lines_count = (messageText.size() - new_line_chars) / chars_in_line + new_line_chars;
		}
		messageText += " ...";
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
		mMsgText->appendText(messageText, FALSE, style_params);
	}


	snapToMessageHeight();

	mIsDirty = true;//will set Avatar Icon in draw
}

void	LLFloaterIMNearbyChatToastPanel::snapToMessageHeight	()
{
	S32 new_height = llmax (mMsgText->getTextPixelHeight() + 2*mMsgText->getVPad() + 2*msg_height_pad, 25);
	
	LLRect panel_rect = getRect();

	panel_rect.setLeftTopAndSize( panel_rect.mLeft, panel_rect.mTop, panel_rect.getWidth(), new_height);
	
	reshape( getRect().getWidth(), getRect().getHeight(), 1);
	
	setRect(panel_rect);

}

void LLFloaterIMNearbyChatToastPanel::onMouseLeave			(S32 x, S32 y, MASK mask)
{
	
}
void LLFloaterIMNearbyChatToastPanel::onMouseEnter				(S32 x, S32 y, MASK mask)
{
	if(mSourceType != CHAT_SOURCE_AGENT)
		return;
}

BOOL	LLFloaterIMNearbyChatToastPanel::handleMouseDown	(S32 x, S32 y, MASK mask)
{
	return LLPanel::handleMouseDown(x,y,mask);
}

BOOL	LLFloaterIMNearbyChatToastPanel::handleMouseUp	(S32 x, S32 y, MASK mask)
{
	/*
	fix for request  EXT-4780
	leaving this commented since I don't remember why ew block those messages...
	if(mSourceType != CHAT_SOURCE_AGENT)
		return LLPanel::handleMouseUp(x,y,mask);
    */

	S32 local_x = x - mMsgText->getRect().mLeft;
	S32 local_y = y - mMsgText->getRect().mBottom;
	
	//if text_box process mouse up (ussually this is click on url) - we didn't show nearby_chat.
	if (mMsgText->pointInView(local_x, local_y) )
	{
		if (mMsgText->handleMouseUp(local_x,local_y,mask) == TRUE)
			return TRUE;
		else
		{
			LLFloaterReg::getTypedInstance<LLFloaterIMNearbyChat>("nearby_chat")->showHistory();
			return FALSE;
		}
	}
	LLFloaterReg::getTypedInstance<LLFloaterIMNearbyChat>("nearby_chat")->showHistory();
	return LLPanel::handleMouseUp(x,y,mask);
}

void	LLFloaterIMNearbyChatToastPanel::setHeaderVisibility(EShowItemHeader e)
{
	LLUICtrl* icon = getChild<LLUICtrl>("avatar_icon", false);
	if(icon)
		icon->setVisible(e == CHATITEMHEADER_SHOW_ONLY_ICON || e==CHATITEMHEADER_SHOW_BOTH);

}

bool	LLFloaterIMNearbyChatToastPanel::canAddText	()
{
	LLChatMsgBox* msg_text = findChild<LLChatMsgBox>("msg_text");
	if(!msg_text)
		return false;
	return msg_text->getLineCount()<10;
}

BOOL	LLFloaterIMNearbyChatToastPanel::handleRightMouseDown(S32 x, S32 y, MASK mask)
{
	LLUICtrl* avatar_icon = getChild<LLUICtrl>("avatar_icon", false);

	S32 local_x = x - avatar_icon->getRect().mLeft;
	S32 local_y = y - avatar_icon->getRect().mBottom;

	//eat message for avatar icon if msg was from object
	if(avatar_icon->pointInView(local_x, local_y) && mSourceType != CHAT_SOURCE_AGENT)
		return TRUE;
	return LLPanel::handleRightMouseDown(x,y,mask);
}
void LLFloaterIMNearbyChatToastPanel::draw()
{
	LLPanel::draw();

	if(mIsDirty)
	{
		LLAvatarIconCtrl* icon = getChild<LLAvatarIconCtrl>("avatar_icon", false);
		if(icon)
		{
			icon->setDrawTooltip(mSourceType == CHAT_SOURCE_AGENT);
			if(mSourceType == CHAT_SOURCE_OBJECT)
				icon->setValue(LLSD("OBJECT_Icon"));
			else if(mSourceType == CHAT_SOURCE_SYSTEM)
				icon->setValue(LLSD("SL_Logo"));
			else if(mSourceType == CHAT_SOURCE_AGENT)
				icon->setValue(mFromID);
			else if(!mFromID.isNull())
				icon->setValue(mFromID);
		}
		mIsDirty = false;
	}
}


