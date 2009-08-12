/** 
 * @file LLNearbyChatHandler.cpp
 * @brief Nearby chat notification managment
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

#include "llnearbychathandler.h"

#include "llchatitemscontainerctrl.h"
#include "llnearbychat.h"
#include "llrecentpeople.h"

#include "llviewercontrol.h"

#include "llfloaterreg.h"//for LLFloaterReg::getTypedInstance

//add LLNearbyChatHandler to LLNotificationsUI namespace
namespace LLNotificationsUI{


LLNearbyChatHandler::LLNearbyChatHandler(e_notification_type type, const LLSD& id)
{
	mType = type;
	LLNearbyChat* nearby_chat = LLFloaterReg::getTypedInstance<LLNearbyChat>("nearby_chat", LLSD());
	/////////////////////////////////////////////////////
	LLChannelManager::Params p;														  //TODO: check and correct
	p.id = LLUUID(NEARBY_CHAT_ID);
	p.channel_right_bound = nearby_chat->getRect().mRight;
	p.channel_width = nearby_chat->getRect().mRight - 16;   //HACK: 16 - ?
	/////////////////////////////////////////////////////


	// Getting a Channel for our notifications
	mChannel = LLChannelManager::getInstance()->createChannel(p);
	mChannel->setFollows(FOLLOWS_LEFT | FOLLOWS_BOTTOM | FOLLOWS_TOP); 
	mChannel->setOverflowFormatString("You have %d unread nearby chat messages");
	mChannel->setCanStoreToasts(false);
}
LLNearbyChatHandler::~LLNearbyChatHandler()
{
}
void LLNearbyChatHandler::processChat(const LLChat& chat_msg)
{
	if(chat_msg.mSourceType == CHAT_SOURCE_AGENT && chat_msg.mFromID.notNull())
         LLRecentPeople::instance().add(chat_msg.mFromID);

	if(chat_msg.mText.empty())
		return;//don't process empty messages

	LLNearbyChat* nearby_chat = LLFloaterReg::getTypedInstance<LLNearbyChat>("nearby_chat", LLSD());
	nearby_chat->addMessage(chat_msg);
	if(nearby_chat->getVisible())
		return;//no need in toast if chat is visible
	
	LLUUID id;
	id.generate();

	LLChatItemCtrl* item = LLChatItemCtrl::createInstance();
	
	item->setMessage(chat_msg);
	//static S32 chat_item_width = nearby_chat->getRect().getWidth() - 16;
	static S32 chat_item_width = 304;
	item->setWidth(chat_item_width);
	item->setHeaderVisibility((EShowItemHeader)gSavedSettings.getS32("nearbychat_showicons_and_names"));
	
	item->setVisible(true);

	LLToast::Params p;
	p.id = id;
	p.panel = item;
	p.on_mouse_enter = boost::bind(&LLNearbyChatHandler::onToastDestroy, this, _1);
	mChannel->addToast(p);	
}

void LLNearbyChatHandler::onToastDestroy(LLToast* toast)
{
	//TODO: what should be done to toasts here? may be htey are to be destroyed?
	//toast->hide();
	if(mChannel)
		mChannel->removeToastsFromChannel();
	else if(toast)
		toast->hide();

	LLFloaterReg::showTypedInstance<LLNearbyChat>("nearby_chat", LLSD());
}

void LLNearbyChatHandler::onChicletClick(void)
{
}
void LLNearbyChatHandler::onChicletClose(void)
{

}

}

