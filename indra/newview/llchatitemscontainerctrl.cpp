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
#include "lltrans.h"

#include "llviewercontrol.h"

static const S32 BORDER_MARGIN = 2;
static const S32 PARENT_BORDER_MARGIN = 0;

static const S32 HORIZONTAL_MULTIPLE = 8;
static const S32 VERTICAL_MULTIPLE = 16;
static const F32 MIN_AUTO_SCROLL_RATE = 120.f;
static const F32 MAX_AUTO_SCROLL_RATE = 500.f;
static const F32 AUTO_SCROLL_RATE_ACCEL = 120.f;

#define MAX_CHAT_HISTORY 100


static LLDefaultChildRegistry::Register<LLChatItemsContainerCtrl>	t2("chat_items_container");



//*******************************************************************************************************************
//LLChatItemCtrl
//*******************************************************************************************************************

LLChatItemCtrl* LLChatItemCtrl::createInstance()
{
	LLChatItemCtrl* item = new LLChatItemCtrl();
	LLUICtrlFactory::getInstance()->buildPanel(item, "panel_chat_item.xml");
	return item;
}

void	LLChatItemCtrl::draw()
{
	LLPanel::draw();
}

void	LLChatItemCtrl::reshape		(S32 width, S32 height, BOOL called_from_parent )
{
	LLPanel::reshape(width, height,called_from_parent);

	LLPanel* caption = getChild<LLPanel>("msg_caption",false,false);
	LLChatMsgBox* msg_text = getChild<LLChatMsgBox>("msg_text",false,false);
	if(caption && msg_text)
	{
		LLRect caption_rect = caption->getRect();
		caption_rect.setLeftTopAndSize( 2, height, width - 4, caption_rect.getHeight());
		caption->reshape( width - 4, caption_rect.getHeight(), 1);
		caption->setRect(caption_rect);

		
		LLRect msg_text_rect = msg_text->getRect();
		msg_text_rect.setLeftTopAndSize( 10, height - caption_rect.getHeight() , width - 20, height - caption_rect.getHeight());
		msg_text->reshape( width - 20, height - caption_rect.getHeight(), 1);
		msg_text->setRect(msg_text_rect);
	}
		
}

BOOL LLChatItemCtrl::postBuild()
{
	return LLPanel::postBuild();
}


std::string LLChatItemCtrl::appendTime()
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



void	LLChatItemCtrl::addText		(const std::string& message)
{
	LLChatMsgBox* msg_text = getChild<LLChatMsgBox>("msg_text",false,false);
	if(msg_text)
		msg_text->addText(message);
	mMessages.push_back(message);
}

void	LLChatItemCtrl::setMessage	(const LLChat& msg)
{
	LLPanel* caption = getChild<LLPanel>("msg_caption",false,false);
	if(!caption)
		return;
	caption->getChild<LLTextBox>("sender_name",false,false)->setText(msg.mFromName);
	std::string tt = appendTime();
	
	caption->getChild<LLTextBox>("msg_time",false,false)->setText(tt);

	caption->getChild<LLAvatarIconCtrl>("avatar_icon",false,false)->setValue(msg.mFromID);

	mOriginalMessage = msg;

	LLChatMsgBox* msg_text = getChild<LLChatMsgBox>("msg_text",false,false);
	if(msg_text)
		msg_text->setText(msg.mText);

	LLUICtrl* msg_inspector = caption->getChild<LLUICtrl>("msg_inspector");
	if(mOriginalMessage.mSourceType != CHAT_SOURCE_AGENT)
		msg_inspector->setVisible(false);

	mMessages.clear();

}


void	LLChatItemCtrl::setWidth(S32 width)
{
	LLChatMsgBox* text_box = getChild<LLChatMsgBox>("msg_text",false,false);
	if(!text_box)
		return;///actually assert fits better

	text_box->reshape(width - 20,100/*its not magic number, we just need any number*/);

	LLChatMsgBox* msg_text = getChild<LLChatMsgBox>("msg_text",false,false);
	if(msg_text && mOriginalMessage.mText.length())
		msg_text->setText(mOriginalMessage.mText);
	
	for(size_t i=0;i<mMessages.size();++i)
		msg_text->addText(mMessages[i]);

	S32 new_height = text_box->getTextPixelHeight();
	LLRect panel_rect = getRect();
	panel_rect.setLeftTopAndSize( panel_rect.mLeft, panel_rect.mTop, width	, 35 + new_height);
	
	reshape( width, panel_rect.getHeight(), 1);
	
	setRect(panel_rect);
}

void LLChatItemCtrl::onMouseLeave			(S32 x, S32 y, MASK mask)
{
	LLPanel* caption = getChild<LLPanel>("msg_caption",false,false);
	if(!caption)
		return;
	LLUICtrl* msg_inspector = caption->getChild<LLUICtrl>("msg_inspector");
	if(msg_inspector)
		msg_inspector->setVisible(false);
	
}
void LLChatItemCtrl::onMouseEnter				(S32 x, S32 y, MASK mask)
{
	LLPanel* caption = getChild<LLPanel>("msg_caption",false,false);
	if(!caption)
		return;
	LLUICtrl* msg_inspector = caption->getChild<LLUICtrl>("msg_inspector");
	if(msg_inspector)
		msg_inspector->setVisible(true);
}

BOOL	LLChatItemCtrl::handleMouseDown	(S32 x, S32 y, MASK mask)
{
	LLPanel* caption = getChild<LLPanel>("msg_caption",false,false);
	if(mOriginalMessage.mSourceType == CHAT_SOURCE_AGENT && caption)
	{
		LLUICtrl* msg_inspector = caption->getChild<LLUICtrl>("msg_inspector");
		if(msg_inspector)
		{
			S32 local_x = x - msg_inspector->getRect().mLeft - caption->getRect().mLeft;
			S32 local_y = y - msg_inspector->getRect().mBottom - caption->getRect().mBottom;
			if(msg_inspector->pointInView(local_x, local_y))
			{
				LLFloaterReg::showInstance("mini_inspector", mOriginalMessage.mFromID);
			}
		}
	}
	return LLPanel::handleMouseDown(x,y,mask);
}

void	LLChatItemCtrl::setHeaderVisibility(EShowItemHeader e)
{
	LLPanel* caption = getChild<LLPanel>("msg_caption",false,false);
	if(!caption)
		return;

	LLUICtrl* icon = caption->getChild<LLUICtrl>("avatar_icon",false,false);
	LLUICtrl* name = caption->getChild<LLUICtrl>("sender_name",false,false);

	if(icon == 0 || name == 0)
		return;

	icon->setVisible(e == CHATITEMHEADER_SHOW_ONLY_ICON || e==CHATITEMHEADER_SHOW_BOTH);
	name->setVisible(e == CHATITEMHEADER_SHOW_ONLY_NAME || e==CHATITEMHEADER_SHOW_BOTH);

}

bool	LLChatItemCtrl::canAddText	()
{
	LLChatMsgBox* msg_text = getChild<LLChatMsgBox>("msg_text",false,false);
	if(!msg_text )
		return false;
	return msg_text->getTextLinesNum()<10;
}


//*******************************************************************************************************************
//LLChatItemsContainerCtrl
//*******************************************************************************************************************

LLChatItemsContainerCtrl::LLChatItemsContainerCtrl(const Params& params):LLPanel(params)
{
	mEShowItemHeader = CHATITEMHEADER_SHOW_BOTH;
}


void	LLChatItemsContainerCtrl::addMessage(const LLChat& msg)
{
	/*
	if(msg.mChatType == CHAT_TYPE_DEBUG_MSG)
		return;
	*/
	if(mItems.size() >= MAX_CHAT_HISTORY)
	{
		LLChatItemCtrl* item = mItems[0];
		removeChild(item);
		delete item;
	}

	
	if(mItems.size() > 0 && msg.mFromID == mItems[mItems.size()-1]->getMessage().mFromID && mItems[mItems.size()-1]->canAddText())
	{
		mItems[mItems.size()-1]->addText(msg.mText);
		mItems[mItems.size()-1]->setWidth(getRect().getWidth() - 16);
	}
	else
	{
		LLChatItemCtrl* item = LLChatItemCtrl::createInstance();
		mItems.push_back(item);
		addChild(item,0);
		item->setWidth(getRect().getWidth() - 16);
		item->setMessage(msg);
		item->setHeaderVisibility((EShowItemHeader)gSavedSettings.getS32("nearbychat_showicons_and_names"));

		item->setVisible(true);
	}

	arrange(getRect().getWidth(),getRect().getHeight());
	updateLayout(getRect().getWidth(),getRect().getHeight());
	scrollToBottom();
}

void	LLChatItemsContainerCtrl::scrollToBottom	()
{
	if(mScrollbar->getVisible())
	{
		mScrollbar->setDocPos(mScrollbar->getDocPosMax());
		onScrollPosChangeCallback(0,0);
	}
}

void	LLChatItemsContainerCtrl::draw()
{
	LLLocalClipRect clip(getRect());
	LLPanel::draw();
}

void	LLChatItemsContainerCtrl::reshape					(S32 width, S32 height, BOOL called_from_parent )
{
	S32 delta_width = width - getRect().getWidth();
	S32 delta_height = height - getRect().getHeight();

	if (delta_width || delta_height || sForceReshape)
	{
		arrange(width, height);
	}

	updateBoundingRect();
}

void	LLChatItemsContainerCtrl::arrange					(S32 width, S32 height)
{
	S32 delta_width = width - getRect().getWidth();
	if(delta_width)//width changed...too bad. now we need to reformat all items
		reformatHistoryScrollItems(width);

	calcRecuiredHeight();

	show_hide_scrollbar(width,height);

	updateLayout(width,height);
}

void	LLChatItemsContainerCtrl::reformatHistoryScrollItems(S32 width)
{
	for(std::vector<LLChatItemCtrl*>::iterator it = mItems.begin(); it != mItems.end();++it)
	{
		(*it)->setWidth(width);
	}
}

S32		LLChatItemsContainerCtrl::calcRecuiredHeight	()
{
	S32 rec_height = 0;
	
	std::vector<LLChatItemCtrl*>::iterator it;
	for(it=mItems.begin(); it!=mItems.end(); ++it)
	{
		rec_height += (*it)->getRect().getHeight();
	}

	mInnerRect.setLeftTopAndSize(0,rec_height + BORDER_MARGIN*2,getRect().getWidth(),rec_height + BORDER_MARGIN);

	return mInnerRect.getHeight();
}


void	LLChatItemsContainerCtrl::updateLayout				(S32 width, S32 height)
{
	S32 panel_top = height - BORDER_MARGIN ;
	S32 panel_width = width;
	if(mScrollbar->getVisible())
	{
		static LLUICachedControl<S32> scrollbar_size ("UIScrollbarSize", 0);
		
		panel_top+=mScrollbar->getDocPos();
		panel_width-=scrollbar_size;
	}


	//set sizes for first panels and dragbars
	for(size_t i=0;i<mItems.size();++i)
	{
		LLRect panel_rect = mItems[i]->getRect();
		panelSetLeftTopAndSize(mItems[i],panel_rect.mLeft,panel_top,panel_width,panel_rect.getHeight());
		panel_top-=panel_rect.getHeight();
	}
}

void	LLChatItemsContainerCtrl::show_hide_scrollbar		(S32 width, S32 height)
{
	calcRecuiredHeight();
	if(getRecuiredHeight() > height )
		showScrollbar(width, height);
	else
		hideScrollbar(width, height);
}

void	LLChatItemsContainerCtrl::showScrollbar			(S32 width, S32 height)
{
	bool was_visible = mScrollbar->getVisible();

	mScrollbar->setVisible(true);

	static LLUICachedControl<S32> scrollbar_size ("UIScrollbarSize", 0);
	
	panelSetLeftTopAndSize(mScrollbar,width-scrollbar_size
		,height-PARENT_BORDER_MARGIN,scrollbar_size,height-2*PARENT_BORDER_MARGIN);
	
	mScrollbar->setPageSize(height);
	mScrollbar->setDocParams(mInnerRect.getHeight(),mScrollbar->getDocPos());

	if(was_visible)
	{
		S32 scroll_pos = llmin(mScrollbar->getDocPos(), getRecuiredHeight() - height - 1);
		mScrollbar->setDocPos(scroll_pos);
		updateLayout(width,height);
		return;
	}
}

void	LLChatItemsContainerCtrl::hideScrollbar			(S32 width, S32 height)
{
	if(mScrollbar->getVisible() == false)
		return;
	mScrollbar->setVisible(false);

	mScrollbar->setDocPos(0);

	if(mItems.size()>0)
	{
		S32 panel_top = height - BORDER_MARGIN;		  // Top coordinate of the first panel
		S32 diff = panel_top - mItems[0]->getRect().mTop;
		shiftPanels(diff);
	}
}

//---------------------------------------------------------------------------------
void LLChatItemsContainerCtrl::panelSetLeftTopAndSize(LLView* panel, S32 left, S32 top, S32 width, S32 height)
{
	if(!panel)
		return;
	LLRect panel_rect = panel->getRect();
	panel_rect.setLeftTopAndSize( left, top, width, height);
	panel->reshape( width, height, 1);
	panel->setRect(panel_rect);
}

void LLChatItemsContainerCtrl::panelShiftVertical(LLView* panel,S32 delta)
{
	if(!panel)
		return;
	panel->translate(0,delta);
}

void LLChatItemsContainerCtrl::shiftPanels(S32 delta)
{
	//Arrange panels
	for(std::vector<LLChatItemCtrl*>::iterator it = mItems.begin(); it != mItems.end();++it)
	{
		panelShiftVertical((*it),delta);
	}	

}

//---------------------------------------------------------------------------------

void	LLChatItemsContainerCtrl::onScrollPosChangeCallback(S32, LLScrollbar*)
{
	updateLayout(getRect().getWidth(),getRect().getHeight());
}

BOOL LLChatItemsContainerCtrl::postBuild()
{
	static LLUICachedControl<S32> scrollbar_size ("UIScrollbarSize", 0);

	LLRect scroll_rect;
	scroll_rect.setOriginAndSize( 
		getRect().getWidth() - scrollbar_size,
		1,
		scrollbar_size,
		getRect().getHeight() - 1);
	

	LLScrollbar::Params sbparams;
	sbparams.name("scrollable vertical");
	sbparams.rect(scroll_rect);
	sbparams.orientation(LLScrollbar::VERTICAL);
	sbparams.doc_size(mInnerRect.getHeight());
	sbparams.doc_pos(0);
	sbparams.page_size(mInnerRect.getHeight());
	sbparams.step_size(VERTICAL_MULTIPLE);
	sbparams.follows.flags(FOLLOWS_RIGHT | FOLLOWS_TOP | FOLLOWS_BOTTOM);
	sbparams.change_callback(boost::bind(&LLChatItemsContainerCtrl::onScrollPosChangeCallback, this, _1, _2));
	
	mScrollbar = LLUICtrlFactory::create<LLScrollbar> (sbparams);
	LLView::addChild( mScrollbar );
	mScrollbar->setVisible( true );
	mScrollbar->setFollowsRight();
	mScrollbar->setFollowsTop();
	mScrollbar->setFollowsBottom();

	reformatHistoryScrollItems(getRect().getWidth());
	arrange(getRect().getWidth(),getRect().getHeight());

	return LLPanel::postBuild();
}
BOOL	LLChatItemsContainerCtrl::handleMouseDown	(S32 x, S32 y, MASK mask)
{
	return LLPanel::handleMouseDown(x,y,mask);
}
BOOL LLChatItemsContainerCtrl::handleKeyHere			(KEY key, MASK mask)
{
	if( mScrollbar->getVisible() && mScrollbar->handleKeyHere( key,mask ) )
		return TRUE;
	return LLPanel::handleKeyHere(key,mask);
}
BOOL LLChatItemsContainerCtrl::handleScrollWheel		( S32 x, S32 y, S32 clicks )
{
	if( mScrollbar->getVisible() && mScrollbar->handleScrollWheel( 0, 0, clicks ) )
		return TRUE;
	return false;
}

void	LLChatItemsContainerCtrl::setHeaderVisibility(EShowItemHeader e)
{
	if(e == mEShowItemHeader)
		return;
	mEShowItemHeader = e;
	for(std::vector<LLChatItemCtrl*>::iterator it = mItems.begin(); it != mItems.end();++it)
	{
		(*it)->setHeaderVisibility(e);
	}
}


