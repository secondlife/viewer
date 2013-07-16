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
#include "llimagepng.h"
#include "llimagejpeg.h"
#include "lltrans.h"
#include "llevents.h"

#include "llfloaterwebcontent.h"
#include "llfloaterreg.h"

boost::scoped_ptr<LLEventPump> LLFacebookConnect::sStateWatcher(new LLEventStream("FacebookConnectState"));
boost::scoped_ptr<LLEventPump> LLFacebookConnect::sContentWatcher(new LLEventStream("FacebookConnectContent"));

// Local functions
void log_facebook_connect_error(const std::string& request, U32 status, const std::string& reason, const std::string& code, const std::string& description)
{
    // Note: 302 (redirect) is *not* an error that warrants logging
    if (status != 302)
    {
		LL_WARNS("FacebookConnect") << request << " request failed with a " << status << " " << reason << ". Reason: " << code << "(" << description << ")" << LL_ENDL;
    }
}

void toast_user_for_success()
{
	LLSD args;
    args["MESSAGE"] = LLTrans::getString("facebook_post_success");
    LLNotificationsUtil::add("SystemMessage", args);
}

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
                    LLFacebookConnect::instance().connectToFacebook(query_map["code"], query_map.get("state"));
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
	
    LLFacebookConnectResponder()
    {
        LLFacebookConnect::instance().setConnectionState(LLFacebookConnect::FB_CONNECTION_IN_PROGRESS);
    }
    
	virtual void completed(U32 status, const std::string& reason, const LLSD& content)
	{
		if (isGoodStatus(status))
		{
			LL_DEBUGS("FacebookConnect") << "Connect successful. content: " << content << LL_ENDL;
			
			// Grab some graph data now that we are connected
            LLFacebookConnect::instance().setConnectionState(LLFacebookConnect::FB_CONNECTED);
		}
		else
		{
            LLFacebookConnect::instance().setConnectionState(LLFacebookConnect::FB_CONNECTION_FAILED);
            log_facebook_connect_error("Connect", status, reason, content.get("error_code"), content.get("error_description"));
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
class LLFacebookShareResponder : public LLHTTPClient::Responder
{
	LOG_CLASS(LLFacebookShareResponder);
public:
    
	LLFacebookShareResponder()
	{
		LLFacebookConnect::instance().setConnectionState(LLFacebookConnect::FB_POSTING);
	}
	
	virtual void completed(U32 status, const std::string& reason, const LLSD& content)
	{
		if (isGoodStatus(status))
		{
            toast_user_for_success();
			LL_DEBUGS("FacebookConnect") << "Post successful. content: " << content << LL_ENDL;
			
			LLFacebookConnect::instance().setConnectionState(LLFacebookConnect::FB_POSTED);
		}
		else if (status == 404)
		{
			LLFacebookConnect::instance().connectToFacebook();
		}
		else
		{
            LLFacebookConnect::instance().setConnectionState(LLFacebookConnect::FB_POST_FAILED);
            log_facebook_connect_error("Share", status, reason, content.get("error_code"), content.get("error_description"));
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
            LLFacebookConnect::instance().setConnectionState(LLFacebookConnect::FB_NOT_CONNECTED);
			LLFacebookConnect::instance().clearContent();
		}
		else
		{
            log_facebook_connect_error("Disconnect", status, reason, content.get("error_code"), content.get("error_description"));
		}
	}
};

///////////////////////////////////////////////////////////////////////////////
//
class LLFacebookConnectedResponder : public LLHTTPClient::Responder
{
	LOG_CLASS(LLFacebookConnectedResponder);
public:
    
	LLFacebookConnectedResponder(bool auto_connect) : mAutoConnect(auto_connect)
    {
		LLFacebookConnect::instance().setConnectionState(LLFacebookConnect::FB_CONNECTION_IN_PROGRESS);
    }
    
	virtual void completed(U32 status, const std::string& reason, const LLSD& content)
	{
		if (isGoodStatus(status))
		{
			LL_DEBUGS("FacebookConnect") << "Connect successful. content: " << content << LL_ENDL;
            
            LLFacebookConnect::instance().setConnectionState(LLFacebookConnect::FB_CONNECTED);
		}
		else
		{
			// show the facebook login page if not connected yet
			if (status == 404)
			{
				if (mAutoConnect)
				{
					LLFacebookConnect::instance().connectToFacebook();
				}
				else
				{
					LLFacebookConnect::instance().setConnectionState(LLFacebookConnect::FB_NOT_CONNECTED);
				}
			}
            else
            {
                LLFacebookConnect::instance().setConnectionState(LLFacebookConnect::FB_CONNECTION_FAILED);
				log_facebook_connect_error("Connected", status, reason, content.get("error_code"), content.get("error_description"));
            }
		}
	}
    
private:
	bool mAutoConnect;
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
			llinfos << "Facebook: Friends list received" << llendl;
			LL_DEBUGS("FacebookConnect") << "Getting Facebook friends successful. content: " << content << LL_ENDL;
			LLFacebookConnect::instance().storeContent(content);
		}
		else
		{
            log_facebook_connect_error("Friends", status, reason, content.get("error_code"), content.get("error_description"));
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
:	mConnectionState(FB_NOT_CONNECTED),
    mContent(),
    mGeneration(0)
{
}

void LLFacebookConnect::openFacebookWeb(std::string url)
{
	// Open the URL in an internal browser window without navigation UI
	LLFloaterWebContent::Params p;
	p.url(url).show_chrome(false);
	LLFloaterReg::showInstance("web_content", p);

	//LLUrlAction::openURLExternal(url);
}

std::string LLFacebookConnect::getFacebookConnectURL(const std::string& route)
{
	//static std::string sFacebookConnectUrl = gAgent.getRegion()->getCapability("FacebookConnect");
	static std::string sFacebookConnectUrl = "https://pdp15.lindenlab.com/fbc/agent/" + gAgentID.asString(); // TEMPORARY HACK FOR FB DEMO - Cho
	std::string url = sFacebookConnectUrl + route;
	llinfos << url << llendl;
	return url;
}

void LLFacebookConnect::connectToFacebook(const std::string& auth_code, const std::string& auth_state)
{
	LLSD body;
	if (!auth_code.empty())
		body["code"] = auth_code;
	if (!auth_state.empty())
		body["state"] = auth_state;
    
	LLHTTPClient::put(getFacebookConnectURL("/connection"), body, new LLFacebookConnectResponder());
}

void LLFacebookConnect::disconnectFromFacebook()
{
	LLHTTPClient::del(getFacebookConnectURL("/connection"), new LLFacebookDisconnectResponder());
}

void LLFacebookConnect::checkConnectionToFacebook(bool auto_connect)
{
    if ((mConnectionState == FB_NOT_CONNECTED) || (mConnectionState == FB_CONNECTION_FAILED) || (mConnectionState == FB_POST_FAILED))
    {
        const bool follow_redirects = false;
        const F32 timeout = HTTP_REQUEST_EXPIRY_SECS;
        LLHTTPClient::get(getFacebookConnectURL("/connection"), new LLFacebookConnectedResponder(auto_connect),
                          LLSD(), timeout, follow_redirects);
    }
}

void LLFacebookConnect::loadFacebookFriends()
{
	const bool follow_redirects = false;
	const F32 timeout = HTTP_REQUEST_EXPIRY_SECS;
	LLHTTPClient::get(getFacebookConnectURL("/friends"), new LLFacebookFriendsResponder(),
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
	LLHTTPClient::post(getFacebookConnectURL("/share/checkin"), body, new LLFacebookShareResponder());
}

void LLFacebookConnect::sharePhoto(const std::string& image_url, const std::string& caption)
{
	LLSD body;
	body["image"] = image_url;
	body["caption"] = caption;
	
    // Note: we can use that route for different publish action. We should be able to use the same responder.
	LLHTTPClient::post(getFacebookConnectURL("/share/photo"), body, new LLFacebookShareResponder());
}

void LLFacebookConnect::sharePhoto(LLPointer<LLImageFormatted> image, const std::string& caption)
{
	std::string imageFormat;
	if (dynamic_cast<LLImagePNG*>(image.get()))
	{
		imageFormat = "png";
	}
	else if (dynamic_cast<LLImageJPEG*>(image.get()))
	{
		imageFormat = "jpg";
	}
	else
	{
		llwarns << "Image to upload is not a PNG or JPEG" << llendl;
		return;
	}
	
	// All this code is mostly copied from LLWebProfile::post()
	const std::string boundary = "----------------------------0123abcdefab";

	LLSD headers;
	headers["Content-Type"] = "multipart/form-data; boundary=" + boundary;

	std::ostringstream body;

	// *NOTE: The order seems to matter.
	body	<< "--" << boundary << "\r\n"
			<< "Content-Disposition: form-data; name=\"caption\"\r\n\r\n"
			<< caption << "\r\n";

	body	<< "--" << boundary << "\r\n"
			<< "Content-Disposition: form-data; name=\"image\"; filename=\"snapshot." << imageFormat << "\"\r\n"
			<< "Content-Type: image/" << imageFormat << "\r\n\r\n";

	// Insert the image data.
	// *FIX: Treating this as a string will probably screw it up ...
	U8* image_data = image->getData();
	for (S32 i = 0; i < image->getDataSize(); ++i)
	{
		body << image_data[i];
	}

	body <<	"\r\n--" << boundary << "--\r\n";

	// postRaw() takes ownership of the buffer and releases it later.
	size_t size = body.str().size();
	U8 *data = new U8[size];
	memcpy(data, body.str().data(), size);
	
    // Note: we can use that route for different publish action. We should be able to use the same responder.
	LLHTTPClient::postRaw(getFacebookConnectURL("/share/photo"), data, size, new LLFacebookShareResponder(), headers);
}

void LLFacebookConnect::updateStatus(const std::string& message)
{
	LLSD body;
	body["message"] = message;
	
    // Note: we can use that route for different publish action. We should be able to use the same responder.
	LLHTTPClient::post(getFacebookConnectURL("/share/wall"), body, new LLFacebookShareResponder());
}

void LLFacebookConnect::storeContent(const LLSD& content)
{
    mGeneration++;
    mContent = content;

	sContentWatcher->post(content);
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

void LLFacebookConnect::setConnectionState(LLFacebookConnect::EConnectionState connection_state)
{
	if (mConnectionState != connection_state)
	{
		LLSD state_info;
		state_info["enum"] = connection_state;
		sStateWatcher->post(state_info);
	}
	
	mConnectionState = connection_state;
}
