/** 
 * @file llfloaterchatterbox.cpp
 * @author Richard
 * @date 2007-05-08
 * @brief Implementation of the chatterbox integrated conversation ui
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007-2009, Linden Research, Inc.
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

#include "llfloaterreg.h"
#include "llfloaterchatterbox.h"
#include "lluictrlfactory.h"
#include "llfloaterchat.h"
#include "llfloaterfriends.h"
#include "llfloatergroups.h"
#include "llviewercontrol.h"
#include "llvoicechannel.h"
#include "llimpanel.h"
#include "llimview.h"

//
// LLFloaterMyFriends
//

LLFloaterMyFriends::LLFloaterMyFriends(const LLSD& seed)
	: LLFloater(seed)
{
	mFactoryMap["friends_panel"] = LLCallbackMap(LLFloaterMyFriends::createFriendsPanel, NULL);
	mFactoryMap["groups_panel"] = LLCallbackMap(LLFloaterMyFriends::createGroupsPanel, NULL);
	//Called from floater reg: LLUICtrlFactory::getInstance()->buildFloater(this, "floater_my_friends.xml");
}

LLFloaterMyFriends::~LLFloaterMyFriends()
{
}

BOOL LLFloaterMyFriends::postBuild()
{
	return TRUE;
}

void LLFloaterMyFriends::onOpen(const LLSD& key)
{
	if (key.asString() == "friends")
	{
		childShowTab("friends_and_groups", "friends_panel");
	}
	else if (key.asString() == "groups")
	{
		childShowTab("friends_and_groups", "groups_panel");
	}
}

//static
void* LLFloaterMyFriends::createFriendsPanel(void* data)
{
	return new LLPanelFriends();
}

//static
void* LLFloaterMyFriends::createGroupsPanel(void* data)
{
	return new LLPanelGroups();
}

//static
LLFloaterMyFriends* LLFloaterMyFriends::getInstance()
{
	return LLFloaterReg::getTypedInstance<LLFloaterMyFriends>("contacts", "friends") ;
}

//
// LLFloaterChatterBox
//
LLFloaterChatterBox::LLFloaterChatterBox(const LLSD& seed)
:	LLMultiFloater(seed),
	mActiveVoiceFloater(NULL)
{
	mAutoResize = FALSE;

	//Called from floater reg: LLUICtrlFactory::getInstance()->buildFloater(this, "floater_chatterbox.xml", FALSE);
}

LLFloaterChatterBox::~LLFloaterChatterBox()
{
}

BOOL LLFloaterChatterBox::postBuild()
{
	setVisibleCallback(boost::bind(&LLFloaterChatterBox::onVisibilityChange, this, _2));
	
	if (gSavedSettings.getBOOL("ContactsTornOff"))
	{
		LLFloaterMyFriends* floater_contacts = LLFloaterMyFriends::getInstance();
		if(floater_contacts)
		{
			// add then remove to set up relationship for re-attach
			addFloater(floater_contacts, FALSE);
			removeFloater(floater_contacts);
			// reparent to floater view
			gFloaterView->addChild(floater_contacts);
		}
	}
	else
	{
		addFloater(LLFloaterMyFriends::getInstance(), TRUE);
	}

	if (gSavedSettings.getBOOL("ChatHistoryTornOff"))
	{
		LLFloaterChat* floater_chat = LLFloaterChat::getInstance();
		if(floater_chat)
		{
			// add then remove to set up relationship for re-attach
			addFloater(floater_chat, FALSE);
			removeFloater(floater_chat);
			// reparent to floater view
			gFloaterView->addChild(floater_chat);
		}
	}
	else
	{
		addFloater(LLFloaterChat::getInstance(), FALSE);
	}
	mTabContainer->lockTabs();
	return TRUE;
}

BOOL LLFloaterChatterBox::handleKeyHere(KEY key, MASK mask)
{
	if (key == 'W' && mask == MASK_CONTROL)
	{
		LLFloater* floater = getActiveFloater();
		// is user closeable and is system closeable
		if (floater && floater->canClose())
		{
			if (floater->isCloseable())
			{
				floater->closeFloater();
			}
			else
			{
				// close chatterbox window if frontmost tab is reserved, non-closeable tab
				// such as contacts or near me
				closeFloater();
			}
		}
		return TRUE;
	}

	return LLMultiFloater::handleKeyHere(key, mask);
}

void LLFloaterChatterBox::draw()
{
	// clear new im notifications when chatterbox is visible
	if (!isMinimized()) 
	{
		gIMMgr->clearNewIMNotification();
	}
	LLFloater* current_active_floater = getCurrentVoiceFloater();
	// set icon on tab for floater currently associated with active voice channel
	if(mActiveVoiceFloater != current_active_floater)
	{
		// remove image from old floater's tab
		if (mActiveVoiceFloater)
		{
			mTabContainer->setTabImage(mActiveVoiceFloater, "");
		}
	}

	// update image on current active tab
	if (current_active_floater)
	{
		LLColor4 icon_color = LLColor4::white;
		LLVoiceChannel* channelp = LLVoiceChannel::getCurrentVoiceChannel();
		if (channelp)
		{
			if (channelp->isActive())
			{
				icon_color = LLColor4::green;
			}
			else if (channelp->getState() == LLVoiceChannel::STATE_ERROR)
			{
				icon_color = LLColor4::red;
			}
			else // active, but not connected
			{
				icon_color = LLColor4::yellow;
			}
		}
		mTabContainer->setTabImage(current_active_floater, "active_voice_tab.tga", icon_color);
	}

	mActiveVoiceFloater = current_active_floater;

	LLMultiFloater::draw();
}

void LLFloaterChatterBox::onOpen(const LLSD& key)
{
	//*TODO:Skinning show the session id associated with key
	if (key.asString() == "local")
	{
		LLFloaterChat* chat = LLFloaterReg::findTypedInstance<LLFloaterChat>("chat");
		chat->openFloater();
	}
	else if (key.isDefined())
	{
		LLFloaterIMPanel* impanel = gIMMgr->findFloaterBySession(key.asUUID());
		if (impanel)
		{
			impanel->openFloater();
		}
	}
}

void LLFloaterChatterBox::onVisibilityChange ( const LLSD& new_visibility )
{
	// HACK: potentially need to toggle console
	LLFloaterChat* instance = LLFloaterChat::getInstance();
	if(instance)
	{
		instance->updateConsoleVisibility();
	}
}

void LLFloaterChatterBox::removeFloater(LLFloater* floaterp)
{
	if(!floaterp) return;
		
	if (floaterp->getName() == "chat floater")
	{
		// only my friends floater now locked
		mTabContainer->lockTabs(mTabContainer->getNumLockedTabs() - 1);
		gSavedSettings.setBOOL("ChatHistoryTornOff", TRUE);
		floaterp->setCanClose(TRUE);
	}
	else if (floaterp->getName() == "floater_my_friends")
	{
		// only chat floater now locked
		mTabContainer->lockTabs(mTabContainer->getNumLockedTabs() - 1);
		gSavedSettings.setBOOL("ContactsTornOff", TRUE);
		floaterp->setCanClose(TRUE);
	}
	LLMultiFloater::removeFloater(floaterp);
}

void LLFloaterChatterBox::addFloater(LLFloater* floaterp, 
									BOOL select_added_floater, 
									LLTabContainer::eInsertionPoint insertion_point)
{
	if(!floaterp) return;
	
	S32 num_locked_tabs = mTabContainer->getNumLockedTabs();

	// already here
	if (floaterp->getHost() == this)
	{
		openFloater(floaterp->getKey());
		return;
	}

	// make sure my friends and chat history both locked when re-attaching chat history
	if (floaterp->getName() == "chat floater")
	{
		mTabContainer->unlockTabs();
		// add chat history as second tab if contact window is present, first tab otherwise
		if (getChildView("floater_my_friends"))
		{
			// assuming contacts window is first tab, select it
			mTabContainer->selectFirstTab();
			// and add ourselves after
			LLMultiFloater::addFloater(floaterp, select_added_floater, LLTabContainer::RIGHT_OF_CURRENT);
		}
		else
		{
			LLMultiFloater::addFloater(floaterp, select_added_floater, LLTabContainer::START);
		}
		
		// make sure first two tabs are now locked
		mTabContainer->lockTabs(num_locked_tabs + 1);
		gSavedSettings.setBOOL("ChatHistoryTornOff", FALSE);
		floaterp->setCanClose(FALSE);
	}
	else if (floaterp->getName() == "floater_my_friends")
	{
		mTabContainer->unlockTabs();
		// add contacts window as first tab
		LLMultiFloater::addFloater(floaterp, select_added_floater, LLTabContainer::START);
		// make sure first two tabs are now locked
		mTabContainer->lockTabs(num_locked_tabs + 1);
		gSavedSettings.setBOOL("ContactsTornOff", FALSE);
		floaterp->setCanClose(FALSE);
	}
	else
	{
		LLMultiFloater::addFloater(floaterp, select_added_floater, insertion_point);
		// openFloater(floaterp->getKey());
	}

	// make sure active voice icon shows up for new tab
	if (floaterp == mActiveVoiceFloater)
	{
		mTabContainer->setTabImage(floaterp, "active_voice_tab.tga");	
	}
}

//static
LLFloaterChatterBox* LLFloaterChatterBox::getInstance()
{
	return LLFloaterReg::getTypedInstance<LLFloaterChatterBox>("communicate", LLSD()) ; 
}

//static 
LLFloater* LLFloaterChatterBox::getCurrentVoiceFloater()
{
	if (!LLVoiceClient::voiceEnabled())
	{
		return NULL;
	}
	if (LLVoiceChannelProximal::getInstance() == LLVoiceChannel::getCurrentVoiceChannel())
	{
		// show near me tab if in proximal channel
		return LLFloaterChat::getInstance();
	}
	else
	{
		LLFloaterChatterBox* floater = LLFloaterChatterBox::getInstance();
		if(!floater) return NULL;
		// iterator over all IM tabs (skip friends and near me)
		for (S32 i = 0; i < floater->getFloaterCount(); i++)
		{
			LLPanel* panelp = floater->mTabContainer->getPanelByIndex(i);
			if (panelp->getName() == "im_floater")
			{
				// only LLFloaterIMPanels are called "im_floater"
				LLFloaterIMPanel* im_floaterp = (LLFloaterIMPanel*)panelp;
				LLVoiceChannel* voice_channel = LLIMModel::getInstance()->getVoiceChannel(im_floaterp->getSessionID());
				if (voice_channel  == LLVoiceChannel::getCurrentVoiceChannel())
				{
					return im_floaterp;
				}
			}
		}
	}
	return NULL;
}
