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
#include "llcommandhandler.h"
#include "llhttpclient.h"
#include "llnotificationsutil.h"
#include "llurlaction.h"

///////////////////////////////////////////////////////////////////////////////
//

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
            LLSD args(LLSD::emptyMap());
            std::stringstream msg;
            msg << reason << " (Code " << status << ")";
            args["FAIL_REASON"] = msg.str();
			LLNotificationsUtil::add("FacebookCannotConnect",args);
			LL_WARNS("FacebookConnect") << "Failed to get a response. reason: " << reason << " status: " << status << LL_ENDL;
		}
	}
    
    void completedHeader(U32 status, const std::string& reason, const LLSD& content)
    {
        if (status == 302)
        {
            LLFacebookConnect::instance().openFacebookWeb(content["location"]);
        }
    }
    
};

///////////////////////////////////////////////////////////////////////////////
//
class LLFacebookPostResponder : public LLHTTPClient::Responder
{
	LOG_CLASS(LLFacebookPostResponder);
public:
    
	virtual void completed(U32 status, const std::string& reason, const LLSD& content)
	{
		if (isGoodStatus(status))
		{
			LL_DEBUGS("FacebookConnect") << "Post successful. content: " << content << LL_ENDL;
		}
		else
		{
            LLSD args(LLSD::emptyMap());
            std::stringstream msg;
            msg << reason << " (Code " << status << ")";
            args["FAIL_REASON"] = msg.str();
			LLNotificationsUtil::add("FacebookCannotConnect",args);
			LL_WARNS("FacebookConnect") << "Failed to get a post response. reason: " << reason << " status: " << status << LL_ENDL;
		}
	}
    
    void completedHeader(U32 status, const std::string& reason, const LLSD& content)
    {
        if (status == 302)
        {
            LLFacebookConnect::instance().openFacebookWeb(content["location"]);
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
			
			// Clear all facebook stuff
            LLFacebookConnect::instance().setConnected(false);
			LLFacebookConnect::instance().clearContent();
		}
		else
		{
            LLSD args(LLSD::emptyMap());
            std::stringstream msg;
            msg << reason << " (Code " << status << ")";
            args["FAIL_REASON"] = msg.str();
			LLNotificationsUtil::add("FacebookCannotConnect",args);
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
            else
            {
                LLSD args(LLSD::emptyMap());
                std::stringstream msg;
                msg << reason << " (Code " << status << ")";
                args["FAIL_REASON"] = msg.str();
                LLNotificationsUtil::add("FacebookCannotConnect",args);
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
			LLFacebookConnect::instance().storeContent(content);
		}
		else
		{
            LLSD args(LLSD::emptyMap());
            std::stringstream msg;
            msg << reason << " (Code " << status << ")";
            args["FAIL_REASON"] = msg.str();
			LLNotificationsUtil::add("FacebookCannotConnect",args);
			LL_WARNS("FacebookConnect") << "Failed to get a response. reason: " << reason << " status: " << status << LL_ENDL;
		}
	}

    void completedHeader(U32 status, const std::string& reason, const LLSD& content)
    {
        if (status == 302)
        {
            LLFacebookConnect::instance().openFacebookWeb(content["location"]);
        }
    }
};

///////////////////////////////////////////////////////////////////////////////
//
LLFacebookConnect::LLFacebookConnect()
:	mConnectedToFbc(false),
    mContent(),
    mGeneration(0)
{
}

void LLFacebookConnect::openFacebookWeb(std::string url)
{
	LLUrlAction::openURLExternal(url);
}

std::string LLFacebookConnect::getFacebookConnectURL(const std::string& route)
{
	//static std::string sFacebookConnectUrl = gAgent.getRegion()->getCapability("FacebookConnect");
	static std::string sFacebookConnectUrl = "https://pdp15.lindenlab.com/fbc/agent/" + gAgentID.asString(); // TEMPORARY HACK FOR FB DEMO - Cho
	std::string url = sFacebookConnectUrl + route;
	llinfos << url << llendl;
	return url;
}

void LLFacebookConnect::connectToFacebook(const std::string& auth_code)
{
	LLSD body;
	if (!auth_code.empty())
		body["code"] = auth_code;
    
	LLHTTPClient::put(getFacebookConnectURL("/connection"), body, new LLFacebookConnectResponder());
}

void LLFacebookConnect::disconnectFromFacebook()
{
	LLHTTPClient::del(getFacebookConnectURL("/connection"), new LLFacebookDisconnectResponder());
}

void LLFacebookConnect::tryToReconnectToFacebook()
{
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
    const bool follow_redirects=false;
    const F32 timeout=HTTP_REQUEST_EXPIRY_SECS;
    LLHTTPClient::get(getFacebookConnectURL("/connection"), new LLFacebookConnectedResponder(true),
                  LLSD(), timeout, follow_redirects);
}

void LLFacebookConnect::loadFacebookFriends()
{
	const bool follow_redirects=false;
	const F32 timeout=HTTP_REQUEST_EXPIRY_SECS;
	LLHTTPClient::get(getFacebookConnectURL("/friend"), new LLFacebookFriendsResponder(),
					  LLSD(), timeout, follow_redirects);
}

void LLFacebookConnect::postCheckin(const std::string& location, const std::string& name, const std::string& description, const std::string& image, const std::string& message)
{
	LLSD body;
	if (!location.empty())
		body["location"] = location;
	if (!name.empty())
		body["name"] = name;
	if (!description.empty())
		body["description"] = description;
	if (!image.empty())
		body["image"] = image;
	if (!message.empty())
		body["message"] = message;

	// Note: we can use that route for different publish action. We should be able to use the same responder.
	LLHTTPClient::post(getFacebookConnectURL("/share/checkin"), body, new LLFacebookPostResponder());
}

void LLFacebookConnect::postCheckinMessage(const std::string& message, const std::string& link, const std::string& name, const std::string& caption, const std::string& description, const std::string& picture)
{
    LLSD body;
	if (!message.empty())
		body["message"] = message;
	if (!link.empty())
		body["link"] = link;
	if (!name.empty())
		body["name"] = name;
	if (!caption.empty())
		body["caption"] = caption;
	if (!description.empty())
		body["description"] = description;
	if (!picture.empty())
		body["picture"] = picture;
    
    // Note: we can use that route for different publish action. We should be able to use the same responder.
	LLHTTPClient::post(getFacebookConnectURL("/share/wall"), body, new LLFacebookPostResponder());
}

void LLFacebookConnect::sharePhoto(const std::string& image_url, const std::string& caption)
{
	LLSD body;
	body["image"] = image_url;
	body["caption"] = caption;
	
    // Note: we can use that route for different publish action. We should be able to use the same responder.
	LLHTTPClient::post(getFacebookConnectURL("/share/photo"), body, new LLFacebookPostResponder());
}

void LLFacebookConnect::storeContent(const LLSD& content)
{
    mGeneration++;
    mContent = content;
}

const LLSD& LLFacebookConnect::getContent() const
{
    return mContent;
}

void LLFacebookConnect::clearContent()
{
    mGeneration++;
    mContent = LLSD();
}








