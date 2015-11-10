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
#include "llflickrconnect.h"
#include "lltwitterconnect.h"

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
#include "llviewerregion.h"

#include "llfloaterwebcontent.h"
#include "llfloaterreg.h"

boost::scoped_ptr<LLEventPump> LLFacebookConnect::sStateWatcher(new LLEventStream("FacebookConnectState"));
boost::scoped_ptr<LLEventPump> LLFacebookConnect::sInfoWatcher(new LLEventStream("FacebookConnectInfo"));
boost::scoped_ptr<LLEventPump> LLFacebookConnect::sContentWatcher(new LLEventStream("FacebookConnectContent"));

// Local functions
void log_facebook_connect_error(const std::string& request, U32 status, const std::string& reason, const std::string& code, const std::string& description)
{
    // Note: 302 (redirect) is *not* an error that warrants logging
    if (status != 302)
    {
		LL_WARNS("FacebookConnect") << request << " request failed with a " << status << " " << reason << ". Reason: " << code << " (" << description << ")" << LL_ENDL;
    }
}

void toast_user_for_facebook_success()
{
	LLSD args;
    args["MESSAGE"] = LLTrans::getString("facebook_post_success");
    LLNotificationsUtil::add("FacebookConnect", args);
}

///////////////////////////////////////////////////////////////////////////////
//
class LLFacebookConnectHandler : public LLCommandHandler
{
public:
	LLFacebookConnectHandler() : LLCommandHandler("fbc", UNTRUSTED_THROTTLE) { }
    
	bool handle(const LLSD& tokens, const LLSD& query_map, LLMediaCtrl* web)
	{
		if (tokens.size() >= 1)
		{
			if (tokens[0].asString() == "connect")
			{
				if (tokens.size() >= 2 && tokens[1].asString() == "flickr")
				{
					// this command probably came from the flickr_web browser, so close it
					LLFloaterReg::hideInstance("flickr_web");

					// connect to flickr
					if (query_map.has("oauth_token"))
					{
						LLFlickrConnect::instance().connectToFlickr(query_map["oauth_token"], query_map.get("oauth_verifier"));
					}
					return true;
				}
				else if (tokens.size() >= 2 && tokens[1].asString() == "twitter")
				{
					// this command probably came from the twitter_web browser, so close it
					LLFloaterReg::hideInstance("twitter_web");

					// connect to twitter
					if (query_map.has("oauth_token"))
					{
						LLTwitterConnect::instance().connectToTwitter(query_map["oauth_token"], query_map.get("oauth_verifier"));
					}
					return true;
				}
				else //if (tokens.size() >= 2 && tokens[1].asString() == "facebook")
				{
					// this command probably came from the fbc_web browser, so close it
					LLFloaterReg::hideInstance("fbc_web");

					// connect to facebook
					if (query_map.has("code"))
					{
						LLFacebookConnect::instance().connectToFacebook(query_map["code"], query_map.get("state"));
					}
					return true;
				}
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
    
	/* virtual */ void httpSuccess()
	{
		LL_DEBUGS("FacebookConnect") << "Connect successful. " << dumpResponse() << LL_ENDL;
		LLFacebookConnect::instance().setConnectionState(LLFacebookConnect::FB_CONNECTED);
	}

	/* virtual */ void httpFailure()
	{
		if ( HTTP_FOUND == getStatus() )
		{
			const std::string& location = getResponseHeader(HTTP_IN_HEADER_LOCATION);
			if (location.empty())
			{
				LL_WARNS("FacebookConnect") << "Missing Location header " << dumpResponse()
							 << "[headers:" << getResponseHeaders() << "]" << LL_ENDL;
			}
			else
			{
				LLFacebookConnect::instance().openFacebookWeb(location);
			}
		}
		else
		{
			LL_WARNS("FacebookConnect") << dumpResponse() << LL_ENDL;
			LLFacebookConnect::instance().setConnectionState(LLFacebookConnect::FB_CONNECTION_FAILED);
			const LLSD& content = getContent();
			log_facebook_connect_error("Connect", getStatus(), getReason(),
									   content.get("error_code"), content.get("error_description"));
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

	/* virtual */ void httpSuccess()
	{
		toast_user_for_facebook_success();
		LL_DEBUGS("FacebookConnect") << "Post successful. " << dumpResponse() << LL_ENDL;
		LLFacebookConnect::instance().setConnectionState(LLFacebookConnect::FB_POSTED);
	}

	/* virtual */ void httpFailure()
	{
		if ( HTTP_FOUND == getStatus() )
		{
			const std::string& location = getResponseHeader(HTTP_IN_HEADER_LOCATION);
			if (location.empty())
			{
				LL_WARNS("FacebookConnect") << "Missing Location header " << dumpResponse()
							 << "[headers:" << getResponseHeaders() << "]" << LL_ENDL;
			}
			else
			{
				LLFacebookConnect::instance().openFacebookWeb(location);
			}
		}
		else if ( HTTP_NOT_FOUND == getStatus() )
		{
			LLFacebookConnect::instance().connectToFacebook();
		}
		else
		{
			LL_WARNS("FacebookConnect") << dumpResponse() << LL_ENDL;
			LLFacebookConnect::instance().setConnectionState(LLFacebookConnect::FB_POST_FAILED);
			const LLSD& content = getContent();
			log_facebook_connect_error("Share", getStatus(), getReason(),
									   content.get("error_code"), content.get("error_description"));
		}
	}
};

///////////////////////////////////////////////////////////////////////////////
//
class LLFacebookDisconnectResponder : public LLHTTPClient::Responder
{
	LOG_CLASS(LLFacebookDisconnectResponder);
public:
 
	LLFacebookDisconnectResponder()
	{
		LLFacebookConnect::instance().setConnectionState(LLFacebookConnect::FB_DISCONNECTING);
	}

	void setUserDisconnected()
	{
		// Clear data
		LLFacebookConnect::instance().clearInfo();
		LLFacebookConnect::instance().clearContent();
		//Notify state change
		LLFacebookConnect::instance().setConnectionState(LLFacebookConnect::FB_NOT_CONNECTED);
	}

	/* virtual */ void httpSuccess()
	{
		LL_DEBUGS("FacebookConnect") << "Disconnect successful. " << dumpResponse() << LL_ENDL;
		setUserDisconnected();
	}

	/* virtual */ void httpFailure()
	{
		//User not found so already disconnected
		if ( HTTP_NOT_FOUND == getStatus() )
		{
			LL_DEBUGS("FacebookConnect") << "Already disconnected. " << dumpResponse() << LL_ENDL;
			setUserDisconnected();
		}
		else
		{
			LL_WARNS("FacebookConnect") << dumpResponse() << LL_ENDL;
			LLFacebookConnect::instance().setConnectionState(LLFacebookConnect::FB_DISCONNECT_FAILED);
			const LLSD& content = getContent();
			log_facebook_connect_error("Disconnect", getStatus(), getReason(),
									   content.get("error_code"), content.get("error_description"));
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

	/* virtual */ void httpSuccess()
	{
		LL_DEBUGS("FacebookConnect") << "Connect successful. " << dumpResponse() << LL_ENDL;
		LLFacebookConnect::instance().setConnectionState(LLFacebookConnect::FB_CONNECTED);
	}

	/* virtual */ void httpFailure()
	{
		// show the facebook login page if not connected yet
		if ( HTTP_NOT_FOUND == getStatus() )
		{
			LL_DEBUGS("FacebookConnect") << "Not connected. " << dumpResponse() << LL_ENDL;
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
			LL_WARNS("FacebookConnect") << dumpResponse() << LL_ENDL;
			LLFacebookConnect::instance().setConnectionState(LLFacebookConnect::FB_DISCONNECT_FAILED);
			const LLSD& content = getContent();
			log_facebook_connect_error("Connected", getStatus(), getReason(),
									   content.get("error_code"), content.get("error_description"));
		}
	}
    
private:
	bool mAutoConnect;
};

///////////////////////////////////////////////////////////////////////////////
//
class LLFacebookInfoResponder : public LLHTTPClient::Responder
{
	LOG_CLASS(LLFacebookInfoResponder);
public:

	/* virtual */ void httpSuccess()
	{
		LL_INFOS("FacebookConnect") << "Facebook: Info received" << LL_ENDL;
		LL_DEBUGS("FacebookConnect") << "Getting Facebook info successful. " << dumpResponse() << LL_ENDL;
		LLFacebookConnect::instance().storeInfo(getContent());
	}

	/* virtual */ void httpFailure()
	{
		if ( HTTP_FOUND == getStatus() )
		{
			const std::string& location = getResponseHeader(HTTP_IN_HEADER_LOCATION);
			if (location.empty())
			{
				LL_WARNS("FacebookConnect") << "Missing Location header " << dumpResponse()
                << "[headers:" << getResponseHeaders() << "]" << LL_ENDL;
			}
			else
			{
				LLFacebookConnect::instance().openFacebookWeb(location);
			}
		}
		else
		{
			LL_WARNS("FacebookConnect") << dumpResponse() << LL_ENDL;
			const LLSD& content = getContent();
			log_facebook_connect_error("Info", getStatus(), getReason(),
									   content.get("error_code"), content.get("error_description"));
		}
	}
};

///////////////////////////////////////////////////////////////////////////////
//
class LLFacebookFriendsResponder : public LLHTTPClient::Responder
{
	LOG_CLASS(LLFacebookFriendsResponder);
public:
    
	/* virtual */ void httpSuccess()
	{
		LL_DEBUGS("FacebookConnect") << "Getting Facebook friends successful. " << dumpResponse() << LL_ENDL;
		LLFacebookConnect::instance().storeContent(getContent());
	}

	/* virtual */ void httpFailure()
	{
		if ( HTTP_FOUND == getStatus() )
		{
			const std::string& location = getResponseHeader(HTTP_IN_HEADER_LOCATION);
			if (location.empty())
			{
				LL_WARNS("FacebookConnect") << "Missing Location header " << dumpResponse()
							 << "[headers:" << getResponseHeaders() << "]" << LL_ENDL;
			}
			else
			{
				LLFacebookConnect::instance().openFacebookWeb(location);
			}
		}
		else
		{
			LL_WARNS("FacebookConnect") << dumpResponse() << LL_ENDL;
			const LLSD& content = getContent();
			log_facebook_connect_error("Friends", getStatus(), getReason(),
									   content.get("error_code"), content.get("error_description"));
		}
	}
};

///////////////////////////////////////////////////////////////////////////////
//
LLFacebookConnect::LLFacebookConnect()
:	mConnectionState(FB_NOT_CONNECTED),
	mConnected(false),
	mInfo(),
    mContent(),
	mRefreshInfo(false),
	mRefreshContent(false),
	mReadFromMaster(false)
{
}

void LLFacebookConnect::openFacebookWeb(std::string url)
{
	// Open the URL in an internal browser window without navigation UI
	LLFloaterWebContent::Params p;
    p.url(url);
    p.show_chrome(true);
    p.allow_address_entry(false);
    p.allow_back_forward_navigation(false);
    p.trusted_content(true);
    p.clean_browser(true);
	LLFloater *floater = LLFloaterReg::showInstance("fbc_web", p);
	//the internal web browser has a bug that prevents it from gaining focus unless a mouse event occurs first (it seems).
	//So when showing the internal web browser, set focus to it's containing floater "fbc_web". When a mouse event 
	//occurs on the "webbrowser" panel part of the floater, a mouse cursor will properly show and the "webbrowser" will gain focus.
	//fbc_web floater contains the "webbrowser" panel.    JIRA: ACME-744
	gFocusMgr.setKeyboardFocus( floater );

	//LLUrlAction::openURLExternal(url);
}

std::string LLFacebookConnect::getFacebookConnectURL(const std::string& route, bool include_read_from_master)
{
    std::string url("");
    LLViewerRegion *regionp = gAgent.getRegion();
    if (regionp)
    {
		//url = "http://pdp15.lindenlab.com/fbc/agent/" + gAgentID.asString(); // TEMPORARY FOR TESTING - CHO
		url = regionp->getCapability("FacebookConnect");
        url += route;
    
        if (include_read_from_master && mReadFromMaster)
        {
            url += "?read_from_master=true";
        }
    }
	return url;
}

void LLFacebookConnect::connectToFacebook(const std::string& auth_code, const std::string& auth_state)
{
	LLSD body;
	if (!auth_code.empty())
    {
		body["code"] = auth_code;
    }
	if (!auth_state.empty())
    {
		body["state"] = auth_state;
    }
    
	LLHTTPClient::put(getFacebookConnectURL("/connection"), body, new LLFacebookConnectResponder());
}

void LLFacebookConnect::disconnectFromFacebook()
{
	LLHTTPClient::del(getFacebookConnectURL("/connection"), new LLFacebookDisconnectResponder());
}

void LLFacebookConnect::checkConnectionToFacebook(bool auto_connect)
{
	const bool follow_redirects = false;
	const F32 timeout = HTTP_REQUEST_EXPIRY_SECS;
	LLHTTPClient::get(getFacebookConnectURL("/connection", true), new LLFacebookConnectedResponder(auto_connect),
						LLSD(), timeout, follow_redirects);
}

void LLFacebookConnect::loadFacebookInfo()
{
	if(mRefreshInfo)
	{
		const bool follow_redirects = false;
		const F32 timeout = HTTP_REQUEST_EXPIRY_SECS;
		LLHTTPClient::get(getFacebookConnectURL("/info", true), new LLFacebookInfoResponder(),
			LLSD(), timeout, follow_redirects);
	}
}

void LLFacebookConnect::loadFacebookFriends()
{
	if(mRefreshContent)
	{
		const bool follow_redirects = false;
		const F32 timeout = HTTP_REQUEST_EXPIRY_SECS;
		LLHTTPClient::get(getFacebookConnectURL("/friends", true), new LLFacebookFriendsResponder(),
			LLSD(), timeout, follow_redirects);
	}
}

void LLFacebookConnect::postCheckin(const std::string& location, const std::string& name, const std::string& description, const std::string& image, const std::string& message)
{
	LLSD body;
	if (!location.empty())
    {
		body["location"] = location;
    }
	if (!name.empty())
    {
		body["name"] = name;
    }
	if (!description.empty())
    {
		body["description"] = description;
    }
	if (!image.empty())
    {
		body["image"] = image;
    }
	if (!message.empty())
    {
		body["message"] = message;
    }

	// Note: we can use that route for different publish action. We should be able to use the same responder.
	LLHTTPClient::post(getFacebookConnectURL("/share/checkin", true), body, new LLFacebookShareResponder());
}

void LLFacebookConnect::sharePhoto(const std::string& image_url, const std::string& caption)
{
	LLSD body;
	body["image"] = image_url;
	body["caption"] = caption;
	
    // Note: we can use that route for different publish action. We should be able to use the same responder.
	LLHTTPClient::post(getFacebookConnectURL("/share/photo", true), body, new LLFacebookShareResponder());
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
		LL_WARNS() << "Image to upload is not a PNG or JPEG" << LL_ENDL;
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
			<< "Content-Disposition: form-data; name=\"image\"; filename=\"Untitled." << imageFormat << "\"\r\n"
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
	LLHTTPClient::postRaw(getFacebookConnectURL("/share/photo", true), data, size, new LLFacebookShareResponder(), headers);
}

void LLFacebookConnect::updateStatus(const std::string& message)
{
	LLSD body;
	body["message"] = message;
	
    // Note: we can use that route for different publish action. We should be able to use the same responder.
	LLHTTPClient::post(getFacebookConnectURL("/share/wall", true), body, new LLFacebookShareResponder());
}

void LLFacebookConnect::storeInfo(const LLSD& info)
{
	mInfo = info;
	mRefreshInfo = false;

	sInfoWatcher->post(info);
}

const LLSD& LLFacebookConnect::getInfo() const
{
	return mInfo;
}

void LLFacebookConnect::clearInfo()
{
	mInfo = LLSD();
}

void LLFacebookConnect::storeContent(const LLSD& content)
{
    mContent = content;
	mRefreshContent = false;

	sContentWatcher->post(content);
}

const LLSD& LLFacebookConnect::getContent() const
{
    return mContent;
}

void LLFacebookConnect::clearContent()
{
    mContent = LLSD();
}

void LLFacebookConnect::setDataDirty()
{
	mRefreshInfo = true;
	mRefreshContent = true;
}

void LLFacebookConnect::setConnectionState(LLFacebookConnect::EConnectionState connection_state)
{
	if(connection_state == FB_CONNECTED)
	{
		mReadFromMaster = true;
		setConnected(true);
		setDataDirty();
	}
	else if(connection_state == FB_NOT_CONNECTED)
	{
		setConnected(false);
	}
	else if(connection_state == FB_POSTED)
	{
		mReadFromMaster = false;
	}

	if (mConnectionState != connection_state)
	{
		// set the connection state before notifying watchers
		mConnectionState = connection_state;

		LLSD state_info;
		state_info["enum"] = connection_state;
		sStateWatcher->post(state_info);
	}
}

void LLFacebookConnect::setConnected(bool connected)
{
	mConnected = connected;
}
