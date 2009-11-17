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

/*
static const S32 BORDER_MARGIN = 2;
static const S32 PARENT_BORDER_MARGIN = 0;

static const S32 HORIZONTAL_MULTIPLE = 8;
static const S32 VERTICAL_MULTIPLE = 16;
static const F32 MIN_AUTO_SCROLL_RATE = 120.f;
static const F32 MAX_AUTO_SCROLL_RATE = 500.f;
static const F32 AUTO_SCROLL_RATE_ACCEL = 120.f;

#define MAX_CHAT_HISTORY 100
*/

static const S32 msg_left_offset = 30;
static const S32 msg_right_offset = 10;
static const S32 msg_height_pad = 2;

//static LLDefaultChildRegistry::Register<LLChatItemsContainerCtrl>	t2("chat_items_container");

//*******************************************************************************************************************
//LLChatItemCtrl
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

	// *NOTE: we must check if child items exist because reshape is called from the 
	// LLView::initFromParams BEFORE postBuild is called and child controls are not exist yet
	LLPanel* caption = findChild<LLPanel>("msg_caption", false);
	LLChatMsgBox* msg_text = findChild<LLChatMsgBox>("msg_text" ,false);
	if(caption && msg_text)
	{
		LLRect caption_rect = caption->getRect();
		caption_rect.setLeftTopAndSize( 2, height, width - 4, caption_rect.getHeight());
		caption->reshape( width - 4, caption_rect.getHeight(), 1);
		caption->setRect(caption_rect);

		LLRect msg_text_rect = msg_text->getRect();
		msg_text_rect.setLeftTopAndSize( msg_left_offset, height - caption_rect.getHeight() , width - msg_left_offset - msg_right_offset, height - caption_rect.getHeight());
		msg_text->reshape( width - msg_left_offset - msg_right_offset, height - caption_rect.getHeight(), 1);
		msg_text->setRect(msg_text_rect);
	}
}

BOOL LLNearbyChatToastPanel::postBuild()
{
	return LLPanel::postBuild();
}


std::string LLNearbyChatToastPanel::appendTime()
{
	time_t utc_time;
	utc_time = time_corrected();
	std::string timeStr ="["+ LLTrans::getString("TimeHour")+"]:["
		+LLTrans::getString("TimeMin")+"] ";

	LLSD substitution;

	substitution["datetime"] = (S32) utc_time;
	LLStringUtil::format (timeStr, substitution);

	return timeStr;
}



void	LLNearbyChatToastPanel::addText	(const std::string& message , const LLStyle::Params& input_params)
{
	LLChatMsgBox* msg_text = getChild<LLChatMsgBox>("msg_text", false);
	msg_text->addText(message , input_params);
	mMessages.push_back(message);
}

void LLNearbyChatToastPanel::init(LLSD& notification)
{
	LLPanel* caption = getChild<LLPanel>("msg_caption", false);

	mText = notification["message"].asString();		// UTF-8 line of text
	mFromName = notification["from"].asString();	// agent or object name
	mFromID = notification["from_id"].asUUID();		// agent id or object id
	
	int sType = notification["source"].asInteger();
    mSourceType = (EChatSourceType)sType;
	
	std::string color_name = notification["text_color"].asString();
	
	mTextColor = LLUIColorTable::instance().getColor(color_name);
	mTextColor.mV[VALPHA] =notification["color_alpha"].asReal();
	
	S32 font_size = notification["font_size"].asInteger();
	switch(font_size)
	{
		case 0:
			mFont = LLFontGL::getFontSansSerifSmall();
			break;
		default:
		case 1:
			mFont = LLFontGL::getFontSansSerif();
			break;
		case 2:
			mFont = LLFontGL::getFontSansSerifBig();
			break;
	}
	
	LLStyle::Params style_params;
	style_params.color(mTextColor);
	style_params.font(mFont);
	
	std::string str_sender;

	if(gAgentID != mFromID)
		str_sender = mFromName;
	else
		str_sender = LLTrans::getString("You");;

	caption->getChild<LLTextBox>("sender_name", false)->setText(str_sender , style_params);
	
	LLChatMsgBox* msg_text = getChild<LLChatMsgBox>("msg_text", false);
	msg_text->setText(mText, style_params);

	LLUICtrl* msg_inspector = caption->getChild<LLUICtrl>("msg_inspector");
	if(mSourceType != CHAT_SOURCE_AGENT)
		msg_inspector->setVisible(false);

	mMessages.clear();

	snapToMessageHeight	();

	mIsDirty = true;//will set Avatar Icon in draw
}

void	LLNearbyChatToastPanel::setMessage	(const LLChat& chat_msg)
{
	LLSD notification;
	notification["message"] = chat_msg.mText;
	notification["from"] = chat_msg.mFromName;
	notification["from_id"] = chat_msg.mFromID;
	notification["time"] = chat_msg.mTime;
	notification["source"] = (S32)chat_msg.mSourceType;
	
	std::string r_color_name="White";
	F32 r_color_alpha = 1.0f; 
	LLViewerChat::getChatColor( chat_msg, r_color_name, r_color_alpha);
	
	notification["text_color"] = r_color_name;
	notification["color_alpha"] = r_color_alpha;
	
	notification["font_size"] = (S32)LLViewerChat::getChatFontSize() ;
	init(notification);

}

void	LLNearbyChatToastPanel::snapToMessageHeight	()
{
	LLChatMsgBox* text_box = getChild<LLChatMsgBox>("msg_text", false);
	S32 new_height = text_box->getTextPixelHeight() + msg_height_pad;
	LLRect panel_rect = getRect();

	S32 caption_height = 0;
	LLPanel* caption = getChild<LLPanel>("msg_caption", false);
	caption_height = caption->getRect().getHeight();

	panel_rect.setLeftTopAndSize( panel_rect.mLeft, panel_rect.mTop, panel_rect.getWidth(), caption_height + new_height);
	
	reshape( getRect().getWidth(), caption_height + new_height, 1);
	
	setRect(panel_rect);

}


void	LLNearbyChatToastPanel::setWidth(S32 width)
{
	LLChatMsgBox* text_box = getChild<LLChatMsgBox>("msg_text", false);
	text_box->reshape(width - msg_left_offset - msg_right_offset,100/*its not magic number, we just need any number*/);

	LLChatMsgBox* msg_text = getChild<LLChatMsgBox>("msg_text", false);
	
	LLStyle::Params style_params;
	style_params.color(mTextColor);
	style_params.font(mFont);
	
	
	if(mText.length())
		msg_text->setText(mText, style_params);
	
	for(size_t i=0;i<mMessages.size();++i)
		msg_text->addText(mMessages[i] , style_params);

	setRect(LLRect(getRect().mLeft, getRect().mTop, getRect().mLeft + width	, getRect().mBottom));
	snapToMessageHeight	();
}

void LLNearbyChatToastPanel::onMouseLeave			(S32 x, S32 y, MASK mask)
{
	LLPanel* caption = getChild<LLPanel>("msg_caption", false);
	LLUICtrl* msg_inspector = caption->getChild<LLUICtrl>("msg_inspector");
	msg_inspector->setVisible(false);
	
}
void LLNearbyChatToastPanel::onMouseEnter				(S32 x, S32 y, MASK mask)
{
	if(mSourceType != CHAT_SOURCE_AGENT)
		return;
	LLPanel* caption = getChild<LLPanel>("msg_caption", false);
	LLUICtrl* msg_inspector = caption->getChild<LLUICtrl>("msg_inspector");
	msg_inspector->setVisible(true);
}

BOOL	LLNearbyChatToastPanel::handleMouseDown	(S32 x, S32 y, MASK mask)
{
	if(mSourceType != CHAT_SOURCE_AGENT)
		return LLPanel::handleMouseDown(x,y,mask);
	LLPanel* caption = getChild<LLPanel>("msg_caption", false);
	LLUICtrl* msg_inspector = caption->getChild<LLUICtrl>("msg_inspector");
	S32 local_x = x - msg_inspector->getRect().mLeft - caption->getRect().mLeft;
	S32 local_y = y - msg_inspector->getRect().mBottom - caption->getRect().mBottom;
	if(msg_inspector->pointInView(local_x, local_y))
	{
		LLFloaterReg::showInstance("inspect_avatar", LLSD().insert("avatar_id", mFromID));
	}
	else
	{
		LLFloaterReg::showInstance("nearby_chat",LLSD());
	}
	return LLPanel::handleMouseDown(x,y,mask);
}

void	LLNearbyChatToastPanel::setHeaderVisibility(EShowItemHeader e)
{
	LLPanel* caption = getChild<LLPanel>("msg_caption", false);

	LLUICtrl* icon = caption->getChild<LLUICtrl>("avatar_icon", false);
	LLUICtrl* name = caption->getChild<LLUICtrl>("sender_name", false);

	icon->setVisible(e == CHATITEMHEADER_SHOW_ONLY_ICON || e==CHATITEMHEADER_SHOW_BOTH);
	name->setVisible(e == CHATITEMHEADER_SHOW_ONLY_NAME || e==CHATITEMHEADER_SHOW_BOTH);

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
	LLPanel* caption = getChild<LLPanel>("msg_caption", false);
	LLUICtrl* avatar_icon = caption->getChild<LLUICtrl>("avatar_icon", false);

	S32 local_x = x - avatar_icon->getRect().mLeft - caption->getRect().mLeft;
	S32 local_y = y - avatar_icon->getRect().mBottom - caption->getRect().mBottom;

	//eat message for avatar icon if msg was from object
	if(avatar_icon->pointInView(local_x, local_y) && mSourceType != CHAT_SOURCE_AGENT)
		return TRUE;
	return LLPanel::handleRightMouseDown(x,y,mask);
}
void LLNearbyChatToastPanel::draw()
{
	if(mIsDirty)
	{
		LLPanel* caption = findChild<LLPanel>("msg_caption", false);
		if(caption)
			caption->getChild<LLAvatarIconCtrl>("avatar_icon", false)->setValue(mFromID);
		mIsDirty = false;
	}
	LLToastPanelBase::draw();
}


