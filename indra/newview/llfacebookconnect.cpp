/** 
 * @file llfacebookconnect.h
 * @author Merov, Cho, Gil
 * @brief Connection to Facebook Service
 *
 * $LicenseInfo:firstyear=2013&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2013, Linden Research, Inc.
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

#include "llfacebookconnect.h"

#include "llagent.h"
#include "llcallingcard.h"			// for LLAvatarTracker
//#include "llcommandhandler.h"
#include "llhttpclient.h"

///////////////////////////////////////////////////////////////////////////////
//
/*
class LLFacebookConnectHandler : public LLCommandHandler
{
public:
	LLFacebookConnectHandler() : LLCommandHandler("fbc", UNTRUSTED_THROTTLE) { }
    
	bool handle(const LLSD& tokens, const LLSD& query_map, LLMediaCtrl* web)
	{
		if (tokens.size() > 0)
		{
			if (tokens[0].asString() == "connect")
			{
				if (query_map.has("code"))
				{
                    LLFacebookConnect::instance().connectToFacebook(query_map["code"]);
				}
				return true;
			}
		}
		return false;
	}
};
LLFacebookConnectHandler gFacebookConnectHandler;
*/

///////////////////////////////////////////////////////////////////////////////
//
class LLFacebookConnectResponder : public LLHTTPClient::Responder
{
	LOG_CLASS(LLFacebookConnectResponder);
public:
    
	virtual void completed(U32 status, const std::string& reason, const LLSD& content)
	{
		if (isGoodStatus(status))
		{
			LL_DEBUGS("FacebookConnect") << "Connect successful. content: " << content << LL_ENDL;
			
			// Grab some graph data now that we are connected
            LLFacebookConnect::instance().setConnected(true);
			LLFacebookConnect::instance().loadFacebookFriends();
		}
		else
		{
			LL_WARNS("FacebookConnect") << "Failed to get a response. reason: " << reason << " status: " << status << LL_ENDL;
		}
	}
};

///////////////////////////////////////////////////////////////////////////////
//
class LLFacebookDisconnectResponder : public LLHTTPClient::Responder
{
	LOG_CLASS(LLFacebookDisconnectResponder);
public:
    
	virtual void completed(U32 status, const std::string& reason, const LLSD& content)
	{
		if (isGoodStatus(status))
		{
			LL_DEBUGS("FacebookConnect") << "Disconnect successful. content: " << content << LL_ENDL;
			
			// Hide all the facebook stuff
            LLFacebookConnect::instance().setConnected(false);
			LLFacebookConnect::instance().hideFacebookFriends();
		}
		else
		{
			LL_WARNS("FacebookConnect") << "Failed to get a response. reason: " << reason << " status: " << status << LL_ENDL;
		}
	}
};

///////////////////////////////////////////////////////////////////////////////
//
class LLFacebookConnectedResponder : public LLHTTPClient::Responder
{
	LOG_CLASS(LLFacebookConnectedResponder);
public:
    
	LLFacebookConnectedResponder(bool show_login_if_not_connected) : mShowLoginIfNotConnected(show_login_if_not_connected) {}
    
	virtual void completed(U32 status, const std::string& reason, const LLSD& content)
	{
		if (isGoodStatus(status))
		{
			LL_DEBUGS("FacebookConnect") << "Connect successful. content: " << content << LL_ENDL;
            
			// Grab some graph data if already connected
            LLFacebookConnect::instance().setConnected(true);
			LLFacebookConnect::instance().loadFacebookFriends();
		}
		else
		{
			LL_WARNS("FacebookConnect") << "Failed to get a response. reason: " << reason << " status: " << status << LL_ENDL;
            
			// show the facebook login page if not connected yet
			if ((status == 404) && mShowLoginIfNotConnected)
			{
				LLFacebookConnect::instance().connectToFacebook();
			}
		}
	}
    
private:
	bool mShowLoginIfNotConnected;
};

///////////////////////////////////////////////////////////////////////////////
//
class LLFacebookFriendsResponder : public LLHTTPClient::Responder
{
	LOG_CLASS(LLFacebookFriendsResponder);
public:

	virtual void completed(U32 status, const std::string& reason, const LLSD& content)
	{
		if (isGoodStatus(status))
		{
			LL_DEBUGS("FacebookConnect") << "Getting Facebook friends successful. content: " << content << LL_ENDL;
            
			// Display the list of friends
			LLFacebookConnect::instance().showFacebookFriends(content);
		}
		else
		{
			LL_WARNS("FacebookConnect") << "Failed to get a response. reason: " << reason << " status: " << status << LL_ENDL;
		}
	}
};


///////////////////////////////////////////////////////////////////////////////
//
LLFacebookConnect::LLFacebookConnect()
:	mConnectedToFbc(false)
{
    llinfos << "Merov : LLFacebookConnect::LLFacebookConnect" << llendl;
}

void LLFacebookConnect::init()
{
}

std::string LLFacebookConnect::getFacebookConnectURL(const std::string& route)
{
    llinfos << "Merov : LLFacebookConnect::getFacebookConnectURL. route = " << route << llendl;
	//static std::string sFacebookConnectUrl = gAgent.getRegion()->getCapability("FacebookConnect");
	static std::string sFacebookConnectUrl = "https://pdp15.lindenlab.com/fbc/agent/" + gAgentID.asString(); // TEMPORARY HACK FOR FB DEMO - Cho
	std::string url = sFacebookConnectUrl + route;
	llinfos << url << llendl;
	return url;
}

void LLFacebookConnect::loadFacebookFriends()
{
    llinfos << "Merov : LLFacebookConnect::loadFacebookFriends" << llendl;
	const bool follow_redirects=false;
	const F32 timeout=HTTP_REQUEST_EXPIRY_SECS;
	LLHTTPClient::get(getFacebookConnectURL("/friend"), new LLFacebookFriendsResponder(),
					  LLSD(), timeout, follow_redirects);
}

void LLFacebookConnect::hideFacebookFriends()
{
    llinfos << "Merov : LLFacebookConnect::hideFacebookFriends" << llendl;
    // That needs to be done in llpanelpeople...
	//mFacebookFriends->clear();
}

void LLFacebookConnect::connectToFacebook(const std::string& auth_code)
{
    llinfos << "Merov : LLFacebookConnect::connectToFacebook" << llendl;
	LLSD body;
	if (!auth_code.empty())
		body["code"] = auth_code;
    
	LLHTTPClient::put(getFacebookConnectURL("/connection"), body, new LLFacebookConnectResponder());
}

void LLFacebookConnect::disconnectFromFacebook()
{
    llinfos << "Merov : LLFacebookConnect::disconnectFromFacebook" << llendl;
	LLHTTPClient::del(getFacebookConnectURL("/connection"), new LLFacebookDisconnectResponder());
}

void LLFacebookConnect::tryToReconnectToFacebook()
{
    llinfos << "Merov : LLFacebookConnect::tryToReconnectToFacebook" << llendl;
	if (!mConnectedToFbc)
	{
		const bool follow_redirects=false;
		const F32 timeout=HTTP_REQUEST_EXPIRY_SECS;
		LLHTTPClient::get(getFacebookConnectURL("/connection"), new LLFacebookConnectedResponder(false),
						  LLSD(), timeout, follow_redirects);
	}
}

void LLFacebookConnect::getConnectionToFacebook()
{
    llinfos << "Merov : LLFacebookConnect::getConnectionToFacebook" << llendl;
    const bool follow_redirects=false;
    const F32 timeout=HTTP_REQUEST_EXPIRY_SECS;
    LLHTTPClient::get(getFacebookConnectURL("/connection"), new LLFacebookConnectedResponder(true),
                  LLSD(), timeout, follow_redirects);
}

void LLFacebookConnect::showFacebookFriends(const LLSD& friends)
{
    /* All that needs to be rewritten a different way */
    // FOR TESTING ONLY!! Print out the data in the log
	//mFacebookFriends->clear();
	//LLPersonTabModel::tab_type tab_type;
	LLAvatarTracker& avatar_tracker = LLAvatarTracker::instance();
    llinfos << "Merov : LLFacebookConnect::showFacebookFriends" << llendl;
    
	for (LLSD::map_const_iterator i = friends.beginMap(); i != friends.endMap(); ++i)
	{
		std::string name = i->second["name"].asString();
		LLUUID agent_id = i->second.has("agent_id") ? i->second["agent_id"].asUUID() : LLUUID(NULL);
		
		//add to avatar list
		//mFacebookFriends->addNewItem(agent_id, name, false);
        
		//FB+SL but not SL friend
        bool is_SL_friend = false;
		if(agent_id.notNull() && !avatar_tracker.isBuddy(agent_id))
		{
			//tab_type = LLPersonTabModel::FB_SL_NON_SL_FRIEND;
            is_SL_friend = true;
		}
		//FB only friend
		else
		{
			//tab_type = LLPersonTabModel::FB_ONLY_FRIEND;
            is_SL_friend = false;
		}
        llinfos << "Merov : LLFacebookConnect : agent_id = " << agent_id << ", name = " << name << ", SL friend = " << is_SL_friend << llendl;
        
		//Add to person tab model
        /*
		LLPersonTabModel * person_tab_model = dynamic_cast<LLPersonTabModel *>(mPersonFolderView->getPersonTabModelByIndex(tab_type));
		if(person_tab_model)
		{
			addParticipantToModel(person_tab_model, agent_id, name);
		}
         */
	}
}








