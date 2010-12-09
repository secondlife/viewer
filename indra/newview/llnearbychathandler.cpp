/** 
 * @file LLNearbyChatHandler.cpp
 * @brief Nearby chat notification managment
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

#include "llagentdata.h" // for gAgentID
#include "llnearbychathandler.h"

#include "llbottomtray.h"
#include "llchatitemscontainerctrl.h"
#include "llfirstuse.h"
#include "llfloaterscriptdebug.h"
#include "llhints.h"
#include "llnearbychat.h"
#include "llrecentpeople.h"

#include "llviewercontrol.h"

#include "llfloaterreg.h"//for LLFloaterReg::getTypedInstance
#include "llviewerwindow.h"//for screen channel position

//add LLNearbyChatHandler to LLNotificationsUI namespace
using namespace LLNotificationsUI;

//-----------------------------------------------------------------------------------------------
//LLNearbyChatScreenChannel
//-----------------------------------------------------------------------------------------------	
LLToastPanelBase* createToastPanel()
{
	LLNearbyChatToastPanel* item = LLNearbyChatToastPanel::createInstance();
	return item;
}

class LLNearbyChatScreenChannel: public LLScreenChannelBase
{
	LOG_CLASS(LLNearbyChatScreenChannel);
public:
	typedef std::vector<LLHandle<LLToast> > toast_vec_t;
	typedef std::list<LLHandle<LLToast> > toast_list_t;

	LLNearbyChatScreenChannel(const LLUUID& id):LLScreenChannelBase(id) 
	{
		mStopProcessing = false;

		LLControlVariable* ctrl = gSavedSettings.getControl("NearbyToastLifeTime").get();
		if (ctrl)
		{
			ctrl->getSignal()->connect(boost::bind(&LLNearbyChatScreenChannel::updateToastsLifetime, this));
		}

		ctrl = gSavedSettings.getControl("NearbyToastFadingTime").get();
		if (ctrl)
		{
			ctrl->getSignal()->connect(boost::bind(&LLNearbyChatScreenChannel::updateToastFadingTime, this));
		}
	}

	void addNotification	(LLSD& notification);
	void arrangeToasts		();
	void showToastsBottom	();
	
	typedef boost::function<LLToastPanelBase* (void )> create_toast_panel_callback_t;
	void setCreatePanelCallback(create_toast_panel_callback_t value) { m_create_toast_panel_callback_t = value;}

	void onToastDestroyed	(LLToast* toast, bool app_quitting);
	void onToastFade		(LLToast* toast);

	void reshape			(S32 width, S32 height, BOOL called_from_parent);

	void redrawToasts()
	{
		arrangeToasts();
	}

	// hide all toasts from screen, but not remove them from a channel
	// removes all toasts from a channel
	virtual void		removeToastsFromChannel() 
	{
		for(toast_vec_t::iterator it = m_active_toasts.begin(); it != m_active_toasts.end(); ++it)
		{
			addToToastPool(it->get());
		}
		m_active_toasts.clear();
	};

	virtual void deleteAllChildren()
	{
		LL_DEBUGS("NearbyChat") << "Clearing toast pool" << llendl;
		m_toast_pool.clear();
		m_active_toasts.clear();
		LLScreenChannelBase::deleteAllChildren();
	}

protected:
	void	deactivateToast(LLToast* toast);
	void	addToToastPool(LLToast* toast)
	{
		if (!toast) return;
		LL_DEBUGS("NearbyChat") << "Pooling toast" << llendl;
		toast->setVisible(FALSE);
		toast->stopTimer();
		toast->setIsHidden(true);

		// Nearby chat toasts that are hidden, not destroyed. They are collected to the toast pool, so that
		// they can be used next time, this is done for performance. But if the toast lifetime was changed
		// (from preferences floater (STORY-36)) while it was shown (at this moment toast isn't in the pool yet)
		// changes don't take affect.
		// So toast's lifetime should be updated each time it's added to the pool. Otherwise viewer would have
		// to be restarted so that changes take effect.
		toast->setLifetime(gSavedSettings.getS32("NearbyToastLifeTime"));
		toast->setFadingTime(gSavedSettings.getS32("NearbyToastFadingTime"));
		m_toast_pool.push_back(toast->getHandle());
	}

	void	createOverflowToast(S32 bottom, F32 timer);

	void 	updateToastsLifetime();

	void	updateToastFadingTime();

	create_toast_panel_callback_t m_create_toast_panel_callback_t;

	bool	createPoolToast();
	
	toast_vec_t m_active_toasts;
	toast_list_t m_toast_pool;

	bool	mStopProcessing;
};

//-----------------------------------------------------------------------------------------------
// LLNearbyChatToast
//-----------------------------------------------------------------------------------------------

// We're deriving from LLToast to be able to override onClose()
// in order to handle closing nearby chat toasts properly.
class LLNearbyChatToast : public LLToast
{
	LOG_CLASS(LLNearbyChatToast);
public:
	LLNearbyChatToast(const LLToast::Params& p, LLNearbyChatScreenChannel* nc_channelp)
	:	LLToast(p),
	 	mNearbyChatScreenChannelp(nc_channelp)
	{
	}

	/*virtual*/ void onClose(bool app_quitting);

private:
	LLNearbyChatScreenChannel*	mNearbyChatScreenChannelp;
};

//-----------------------------------------------------------------------------------------------
// LLNearbyChatScreenChannel
//-----------------------------------------------------------------------------------------------

void LLNearbyChatScreenChannel::deactivateToast(LLToast* toast)
{
	toast_vec_t::iterator pos = std::find(m_active_toasts.begin(), m_active_toasts.end(), toast->getHandle());

	if (pos == m_active_toasts.end())
	{
		llassert(pos == m_active_toasts.end());
		return;
	}

	LL_DEBUGS("NearbyChat") << "Deactivating toast" << llendl;
	m_active_toasts.erase(pos);
}

void	LLNearbyChatScreenChannel::createOverflowToast(S32 bottom, F32 timer)
{
	//we don't need overflow toast in nearby chat
}

void LLNearbyChatScreenChannel::onToastDestroyed(LLToast* toast, bool app_quitting)
{	
	LL_DEBUGS("NearbyChat") << "Toast destroyed (app_quitting=" << app_quitting << ")" << llendl;

	if (app_quitting)
	{
		// Viewer is quitting.
		// Immediately stop processing chat messages (EXT-1419).
	mStopProcessing = true;
}
	else
	{
		// The toast is being closed by user (STORM-192).
		// Remove it from the list of active toasts to prevent
		// further references to the invalid pointer.
		deactivateToast(toast);
	}
}

void LLNearbyChatScreenChannel::onToastFade(LLToast* toast)
{	
	LL_DEBUGS("NearbyChat") << "Toast fading" << llendl;

	//fade mean we put toast to toast pool
	if(!toast)
		return;

	deactivateToast(toast);

	addToToastPool(toast);
	
	arrangeToasts();
}

void LLNearbyChatScreenChannel::updateToastsLifetime()
{
	S32 seconds = gSavedSettings.getS32("NearbyToastLifeTime");
	toast_list_t::iterator it;

	for(it = m_toast_pool.begin(); it != m_toast_pool.end(); ++it)
	{
		(*it).get()->setLifetime(seconds);
	}
}

void LLNearbyChatScreenChannel::updateToastFadingTime()
{
	S32 seconds = gSavedSettings.getS32("NearbyToastFadingTime");
	toast_list_t::iterator it;

	for(it = m_toast_pool.begin(); it != m_toast_pool.end(); ++it)
	{
		(*it).get()->setFadingTime(seconds);
	}
}

bool	LLNearbyChatScreenChannel::createPoolToast()
{
	LLToastPanelBase* panel= m_create_toast_panel_callback_t();
	if(!panel)
		return false;
	
	LLToast::Params p;
	p.panel = panel;
	p.lifetime_secs = gSavedSettings.getS32("NearbyToastLifeTime");
	p.fading_time_secs = gSavedSettings.getS32("NearbyToastFadingTime");

	LLToast* toast = new LLNearbyChatToast(p, this);
	
	
	toast->setOnFadeCallback(boost::bind(&LLNearbyChatScreenChannel::onToastFade, this, _1));

	LL_DEBUGS("NearbyChat") << "Creating and pooling toast" << llendl;	
	m_toast_pool.push_back(toast->getHandle());
	return true;
}

void LLNearbyChatScreenChannel::addNotification(LLSD& notification)
{
	//look in pool. if there is any message
	if(mStopProcessing)
		return;

	/*
    find last toast and check ID
	*/

	if(m_active_toasts.size())
	{
		LLUUID fromID = notification["from_id"].asUUID();		// agent id or object id
		std::string from = notification["from"].asString();
		LLToast* toast = m_active_toasts[0].get();
		if (toast)
		{
			LLNearbyChatToastPanel* panel = dynamic_cast<LLNearbyChatToastPanel*>(toast->getPanel());
  
			if(panel && panel->messageID() == fromID && panel->getFromName() == from && panel->canAddText())
			{
				panel->addMessage(notification);
				toast->reshapeToPanel();
				toast->startTimer();
	  
				arrangeToasts();
				return;
			}
		}
	}
	

	
	if(m_toast_pool.empty())
	{
		//"pool" is empty - create one more panel
		LL_DEBUGS("NearbyChat") << "Empty pool" << llendl;
		if(!createPoolToast())//created toast will go to pool. so next call will find it
			return;
		addNotification(notification);
		return;
	}

	int chat_type = notification["chat_type"].asInteger();
	
	if( ((EChatType)chat_type == CHAT_TYPE_DEBUG_MSG))
	{
		if(gSavedSettings.getBOOL("ShowScriptErrors") == FALSE) 
			return;
		if(gSavedSettings.getS32("ShowScriptErrorsLocation")== 1)
			return;
	}
		

	//take 1st element from pool, (re)initialize it, put it in active toasts

	LL_DEBUGS("NearbyChat") << "Getting toast from pool" << llendl;
	LLToast* toast = m_toast_pool.back().get();

	m_toast_pool.pop_back();


	LLToastPanelBase* panel = dynamic_cast<LLToastPanelBase*>(toast->getPanel());
	if(!panel)
		return;
	panel->init(notification);

	toast->reshapeToPanel();
	toast->startTimer();
	
	m_active_toasts.push_back(toast->getHandle());

	arrangeToasts();
}

void LLNearbyChatScreenChannel::arrangeToasts()
{
	if(!isHovering())
	{
		showToastsBottom();
	}

	if (m_active_toasts.empty())
	{
		LLHints::registerHintTarget("incoming_chat", LLHandle<LLView>());
	}
	else
	{
		LLToast* toast = m_active_toasts.front().get();
		if (toast)
		{
			LLHints::registerHintTarget("incoming_chat", m_active_toasts.front().get()->LLView::getHandle());
		}
	}
}

int sort_toasts_predicate(LLHandle<LLToast> first, LLHandle<LLToast> second)
{
	F32 v1 = first.get()->getTimeLeftToLive();
	F32 v2 = second.get()->getTimeLeftToLive();
	return v1 > v2;
}

void LLNearbyChatScreenChannel::showToastsBottom()
{
	if(mStopProcessing)
		return;

	LLRect	toast_rect;	
	S32		bottom = getRect().mBottom;
	S32		margin = gSavedSettings.getS32("ToastGap");

	//sort active toasts
	std::sort(m_active_toasts.begin(),m_active_toasts.end(),sort_toasts_predicate);

	//calc max visible item and hide other toasts.

	for(toast_vec_t::iterator it = m_active_toasts.begin(); it != m_active_toasts.end(); ++it)
	{
		LLToast* toast = it->get();
		if (!toast) continue;

		S32 toast_top = bottom + toast->getRect().getHeight() + margin;

		if(toast_top > gFloaterView->getRect().getHeight())
		{
			while(it!=m_active_toasts.end())
			{
				addToToastPool(it->get());
				it=m_active_toasts.erase(it);
			}
			break;
		}

		toast_rect = toast->getRect();
		toast_rect.setLeftTopAndSize(getRect().mLeft , bottom + toast_rect.getHeight(), toast_rect.getWidth() ,toast_rect.getHeight());

		toast->setRect(toast_rect);
		bottom += toast_rect.getHeight() - toast->getTopPad() + margin;
	}
	
	// use reverse order to provide correct z-order and avoid toast blinking
	
	for(toast_vec_t::reverse_iterator it = m_active_toasts.rbegin(); it != m_active_toasts.rend(); ++it)
	{
		LLToast* toast = it->get();
		if (toast)
	{
		toast->setIsHidden(false);
		toast->setVisible(TRUE);
		}
	}

	}

void LLNearbyChatScreenChannel::reshape			(S32 width, S32 height, BOOL called_from_parent)
{
	LLScreenChannelBase::reshape(width, height, called_from_parent);
	arrangeToasts();
}


//-----------------------------------------------------------------------------------------------
//LLNearbyChatHandler
//-----------------------------------------------------------------------------------------------
LLNearbyChatHandler::LLNearbyChatHandler(e_notification_type type, const LLSD& id)
{
	mType = type;

	// Getting a Channel for our notifications
	LLNearbyChatScreenChannel* channel = new LLNearbyChatScreenChannel(LLUUID(gSavedSettings.getString("NearByChatChannelUUID")));
	
	LLNearbyChatScreenChannel::create_toast_panel_callback_t callback = createToastPanel;

	channel->setCreatePanelCallback(callback);

	mChannel = LLChannelManager::getInstance()->addChannel(channel);
}

LLNearbyChatHandler::~LLNearbyChatHandler()
{
}


void LLNearbyChatHandler::initChannel()
{
	LLNearbyChat* nearby_chat = LLFloaterReg::getTypedInstance<LLNearbyChat>("nearby_chat", LLSD());
	LLView* chat_box = LLBottomTray::getInstance()->getChildView("chat_box");
	S32 channel_right_bound = nearby_chat->getRect().mRight;
	mChannel->init(chat_box->getRect().mLeft, channel_right_bound);
}



void LLNearbyChatHandler::processChat(const LLChat& chat_msg, const LLSD &args)
{
	if(chat_msg.mMuted == TRUE)
		return;

	if(chat_msg.mText.empty())
		return;//don't process empty messages

	LLChat& tmp_chat = const_cast<LLChat&>(chat_msg);

	LLNearbyChat* nearby_chat = LLFloaterReg::getTypedInstance<LLNearbyChat>("nearby_chat", LLSD());
	{
		//sometimes its usefull to have no name at all...
		//if(tmp_chat.mFromName.empty() && tmp_chat.mFromID!= LLUUID::null)
		//	tmp_chat.mFromName = tmp_chat.mFromID.asString();
	}

	// don't show toast and add message to chat history on receive debug message
	// with disabled setting showing script errors or enabled setting to show script
	// errors in separate window.
	if (chat_msg.mChatType == CHAT_TYPE_DEBUG_MSG)
	{
		if(gSavedSettings.getBOOL("ShowScriptErrors") == FALSE)
			return;

		// don't process debug messages from not owned objects, see EXT-7762
		if (gAgentID != chat_msg.mOwnerID)
		{
			return;
		}

		if (gSavedSettings.getS32("ShowScriptErrorsLocation")== 1)// show error in window //("ScriptErrorsAsChat"))
		{

			LLColor4 txt_color;

			LLViewerChat::getChatColor(chat_msg,txt_color);

			LLFloaterScriptDebug::addScriptLine(chat_msg.mText,
												chat_msg.mFromName,
												txt_color,
												chat_msg.mFromID);
			return;
		}
	}

	nearby_chat->addMessage(chat_msg, true, args);
	if( nearby_chat->getVisible()
		|| ( chat_msg.mSourceType == CHAT_SOURCE_AGENT
			&& gSavedSettings.getBOOL("UseChatBubbles") ) )
		return;//no need in toast if chat is visible or if bubble chat is enabled

	// Handle irc styled messages for toast panel
	if (tmp_chat.mChatStyle == CHAT_STYLE_IRC)
	{
		if(!tmp_chat.mFromName.empty())
			tmp_chat.mText = tmp_chat.mFromName + tmp_chat.mText.substr(3);
		else
			tmp_chat.mText = tmp_chat.mText.substr(3);
	}

	// arrange a channel on a screen
	if(!mChannel->getVisible())
	{
		initChannel();
	}

	/*
	//comment all this due to EXT-4432
	..may clean up after some time...

	//only messages from AGENTS
	if(CHAT_SOURCE_OBJECT == chat_msg.mSourceType)
	{
		if(chat_msg.mChatType == CHAT_TYPE_DEBUG_MSG)
			return;//ok for now we don't skip messeges from object, so skip only debug messages
	}
	*/

	LLUUID id;
	id.generate();

	LLNearbyChatScreenChannel* channel = dynamic_cast<LLNearbyChatScreenChannel*>(mChannel);
	

	if(channel)
	{
		LLSD notification;
		notification["id"] = id;
		notification["message"] = chat_msg.mText;
		notification["from"] = chat_msg.mFromName;
		notification["from_id"] = chat_msg.mFromID;
		notification["time"] = chat_msg.mTime;
		notification["source"] = (S32)chat_msg.mSourceType;
		notification["chat_type"] = (S32)chat_msg.mChatType;
		notification["chat_style"] = (S32)chat_msg.mChatStyle;
		
		std::string r_color_name = "White";
		F32 r_color_alpha = 1.0f; 
		LLViewerChat::getChatColor( chat_msg, r_color_name, r_color_alpha);
		
		notification["text_color"] = r_color_name;
		notification["color_alpha"] = r_color_alpha;
		notification["font_size"] = (S32)LLViewerChat::getChatFontSize() ;
		channel->addNotification(notification);	
	}
	
	if(chat_msg.mSourceType == CHAT_SOURCE_AGENT 
		&& chat_msg.mFromID.notNull() 
		&& chat_msg.mFromID != gAgentID)
	{
 		LLFirstUse::otherAvatarChatFirst();
	}
}

void LLNearbyChatHandler::onDeleteToast(LLToast* toast)
{
}


//-----------------------------------------------------------------------------------------------
// LLNearbyChatToast
//-----------------------------------------------------------------------------------------------

// virtual
void LLNearbyChatToast::onClose(bool app_quitting)
{
	mNearbyChatScreenChannelp->onToastDestroyed(this, app_quitting);
}

// EOF
