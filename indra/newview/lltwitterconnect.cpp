/** 
 * @file lltwitterconnect.h
 * @author Merov, Cho
 * @brief Connection to Twitter Service
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

boost::scoped_ptr<LLEventPump> LLTwitterConnect::sStateWatcher(new LLEventStream("TwitterConnectState"));
boost::scoped_ptr<LLEventPump> LLTwitterConnect::sInfoWatcher(new LLEventStream("TwitterConnectInfo"));
boost::scoped_ptr<LLEventPump> LLTwitterConnect::sContentWatcher(new LLEventStream("TwitterConnectContent"));

// Local functions
void log_twitter_connect_error(const std::string& request, U32 status, const std::string& reason, const std::string& code, const std::string& description)
{
    // Note: 302 (redirect) is *not* an error that warrants logging
    if (status != 302)
    {
		LL_WARNS("TwitterConnect") << request << " request failed with a " << status << " " << reason << ". Reason: " << code << " (" << description << ")" << LL_ENDL;
    }
}

void toast_user_for_twitter_success()
{
	LLSD args;
    args["MESSAGE"] = LLTrans::getString("twitter_post_success");
    LLNotificationsUtil::add("TwitterConnect", args);
}

///////////////////////////////////////////////////////////////////////////////
//
class LLTwitterConnectResponder : public LLHTTPClient::Responder
{
	LOG_CLASS(LLTwitterConnectResponder);
public:
	
    LLTwitterConnectResponder()
    {
        LLTwitterConnect::instance().setConnectionState(LLTwitterConnect::TWITTER_CONNECTION_IN_PROGRESS);
    }
    
	virtual void completed(U32 status, const std::string& reason, const LLSD& content)
	{
		if (isGoodStatus(status))
		{
			LL_DEBUGS("TwitterConnect") << "Connect successful. content: " << content << LL_ENDL;
			
            LLTwitterConnect::instance().setConnectionState(LLTwitterConnect::TWITTER_CONNECTED);
		}
		else if (status != 302)
		{
            LLTwitterConnect::instance().setConnectionState(LLTwitterConnect::TWITTER_CONNECTION_FAILED);
            log_twitter_connect_error("Connect", status, reason, content.get("error_code"), content.get("error_description"));
		}
	}
    
    void completedHeader(U32 status, const std::string& reason, const LLSD& content)
    {
        if (status == 302)
        {
            LLTwitterConnect::instance().openTwitterWeb(content["location"]);
        }
    }
};

///////////////////////////////////////////////////////////////////////////////
//
class LLTwitterShareResponder : public LLHTTPClient::Responder
{
	LOG_CLASS(LLTwitterShareResponder);
public:
    
	LLTwitterShareResponder()
	{
		LLTwitterConnect::instance().setConnectionState(LLTwitterConnect::TWITTER_POSTING);
	}
	
	virtual void completed(U32 status, const std::string& reason, const LLSD& content)
	{
		if (isGoodStatus(status))
		{
            toast_user_for_twitter_success();
			LL_DEBUGS("TwitterConnect") << "Post successful. content: " << content << LL_ENDL;
			
			LLTwitterConnect::instance().setConnectionState(LLTwitterConnect::TWITTER_POSTED);
		}
		else if (status == 404)
		{
			LLTwitterConnect::instance().connectToTwitter();
		}
		else
		{
            LLTwitterConnect::instance().setConnectionState(LLTwitterConnect::TWITTER_POST_FAILED);
            log_twitter_connect_error("Share", status, reason, content.get("error_code"), content.get("error_description"));
		}
	}
    
    void completedHeader(U32 status, const std::string& reason, const LLSD& content)
    {
        if (status == 302)
        {
            LLTwitterConnect::instance().openTwitterWeb(content["location"]);
        }
    }
};

///////////////////////////////////////////////////////////////////////////////
//
class LLTwitterDisconnectResponder : public LLHTTPClient::Responder
{
	LOG_CLASS(LLTwitterDisconnectResponder);
public:
 
	LLTwitterDisconnectResponder()
	{
		LLTwitterConnect::instance().setConnectionState(LLTwitterConnect::TWITTER_DISCONNECTING);
	}

	void setUserDisconnected()
	{
		// Clear data
		LLTwitterConnect::instance().clearInfo();

		//Notify state change
		LLTwitterConnect::instance().setConnectionState(LLTwitterConnect::TWITTER_NOT_CONNECTED);
	}

	virtual void completed(U32 status, const std::string& reason, const LLSD& content)
	{
		if (isGoodStatus(status)) 
		{
			LL_DEBUGS("TwitterConnect") << "Disconnect successful. content: " << content << LL_ENDL;
			setUserDisconnected();

		}
		//User not found so already disconnected
		else if(status == 404)
		{
			LL_DEBUGS("TwitterConnect") << "Already disconnected. content: " << content << LL_ENDL;
			setUserDisconnected();
		}
		else
		{
			LLTwitterConnect::instance().setConnectionState(LLTwitterConnect::TWITTER_DISCONNECT_FAILED);
            log_twitter_connect_error("Disconnect", status, reason, content.get("error_code"), content.get("error_description"));
		}
	}
};

///////////////////////////////////////////////////////////////////////////////
//
class LLTwitterConnectedResponder : public LLHTTPClient::Responder
{
	LOG_CLASS(LLTwitterConnectedResponder);
public:
    
	LLTwitterConnectedResponder(bool auto_connect) : mAutoConnect(auto_connect)
    {
		LLTwitterConnect::instance().setConnectionState(LLTwitterConnect::TWITTER_CONNECTION_IN_PROGRESS);
    }
    
	virtual void completed(U32 status, const std::string& reason, const LLSD& content)
	{
		if (isGoodStatus(status))
		{
			LL_DEBUGS("TwitterConnect") << "Connect successful. content: " << content << LL_ENDL;
            
            LLTwitterConnect::instance().setConnectionState(LLTwitterConnect::TWITTER_CONNECTED);
		}
		else
		{
			// show the twitter login page if not connected yet
			if (status == 404)
			{
				if (mAutoConnect)
				{
					LLTwitterConnect::instance().connectToTwitter();
				}
				else
				{
					LLTwitterConnect::instance().setConnectionState(LLTwitterConnect::TWITTER_NOT_CONNECTED);
				}
			}
            else
            {
                LLTwitterConnect::instance().setConnectionState(LLTwitterConnect::TWITTER_CONNECTION_FAILED);
				log_twitter_connect_error("Connected", status, reason, content.get("error_code"), content.get("error_description"));
            }
		}
	}
    
private:
	bool mAutoConnect;
};

///////////////////////////////////////////////////////////////////////////////
//
class LLTwitterInfoResponder : public LLHTTPClient::Responder
{
	LOG_CLASS(LLTwitterInfoResponder);
public:

	virtual void completed(U32 status, const std::string& reason, const LLSD& info)
	{
		if (isGoodStatus(status))
		{
			llinfos << "Twitter: Info received" << llendl;
			LL_DEBUGS("TwitterConnect") << "Getting Twitter info successful. info: " << info << LL_ENDL;
			LLTwitterConnect::instance().storeInfo(info);
		}
		else
		{
			log_twitter_connect_error("Info", status, reason, info.get("error_code"), info.get("error_description"));
		}
	}

	void completedHeader(U32 status, const std::string& reason, const LLSD& content)
	{
		if (status == 302)
		{
			LLTwitterConnect::instance().openTwitterWeb(content["location"]);
		}
	}
};

///////////////////////////////////////////////////////////////////////////////
//
LLTwitterConnect::LLTwitterConnect()
:	mConnectionState(TWITTER_NOT_CONNECTED),
	mConnected(false),
	mInfo(),
	mRefreshInfo(false),
	mReadFromMaster(false)
{
}

void LLTwitterConnect::openTwitterWeb(std::string url)
{
	// Open the URL in an internal browser window without navigation UI
	LLFloaterWebContent::Params p;
    p.url(url);
    p.show_chrome(true);
    p.allow_address_entry(false);
    p.allow_back_forward_navigation(false);
    p.trusted_content(true);
    p.clean_browser(true);
	LLFloater *floater = LLFloaterReg::showInstance("twitter_web", p);
	//the internal web browser has a bug that prevents it from gaining focus unless a mouse event occurs first (it seems).
	//So when showing the internal web browser, set focus to it's containing floater "twitter_web". When a mouse event 
	//occurs on the "webbrowser" panel part of the floater, a mouse cursor will properly show and the "webbrowser" will gain focus.
	//twitter_web floater contains the "webbrowser" panel.    JIRA: ACME-744
	gFocusMgr.setKeyboardFocus( floater );

	//LLUrlAction::openURLExternal(url);
}

std::string LLTwitterConnect::getTwitterConnectURL(const std::string& route, bool include_read_from_master)
{
    std::string url("");
    LLViewerRegion *regionp = gAgent.getRegion();
    if (regionp)
    {
		//url = "http://pdp15.lindenlab.com/twitter/agent/" + gAgentID.asString(); // TEMPORARY FOR TESTING - CHO
        url = regionp->getCapability("TwitterConnect");
        url += route;
    
        if (include_read_from_master && mReadFromMaster)
        {
            url += "?read_from_master=true";
        }
    }
	return url;
}

void LLTwitterConnect::connectToTwitter(const std::string& request_token, const std::string& oauth_verifier)
{
	LLSD body;
	if (!request_token.empty())
		body["request_token"] = request_token;
	if (!oauth_verifier.empty())
		body["oauth_verifier"] = oauth_verifier;
    
	LLHTTPClient::put(getTwitterConnectURL("/connection"), body, new LLTwitterConnectResponder());
}

void LLTwitterConnect::disconnectFromTwitter()
{
	LLHTTPClient::del(getTwitterConnectURL("/connection"), new LLTwitterDisconnectResponder());
}

void LLTwitterConnect::checkConnectionToTwitter(bool auto_connect)
{
	const bool follow_redirects = false;
	const F32 timeout = HTTP_REQUEST_EXPIRY_SECS;
	LLHTTPClient::get(getTwitterConnectURL("/connection", true), new LLTwitterConnectedResponder(auto_connect),
						LLSD(), timeout, follow_redirects);
}

void LLTwitterConnect::loadTwitterInfo()
{
	if(mRefreshInfo)
	{
		const bool follow_redirects = false;
		const F32 timeout = HTTP_REQUEST_EXPIRY_SECS;
		LLHTTPClient::get(getTwitterConnectURL("/info", true), new LLTwitterInfoResponder(),
			LLSD(), timeout, follow_redirects);
	}
}

void LLTwitterConnect::uploadPhoto(const std::string& image_url, const std::string& status)
{
	LLSD body;
	body["image"] = image_url;
	body["status"] = status;
	
    // Note: we can use that route for different publish action. We should be able to use the same responder.
	LLHTTPClient::post(getTwitterConnectURL("/share/photo", true), body, new LLTwitterShareResponder());
}

void LLTwitterConnect::uploadPhoto(LLPointer<LLImageFormatted> image, const std::string& status)
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
			<< "Content-Disposition: form-data; name=\"status\"\r\n\r\n"
			<< status << "\r\n";

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
	LLHTTPClient::postRaw(getTwitterConnectURL("/share/photo", true), data, size, new LLTwitterShareResponder(), headers);
}

void LLTwitterConnect::updateStatus(const std::string& status)
{
	LLSD body;
	body["status"] = status;
	
    // Note: we can use that route for different publish action. We should be able to use the same responder.
	LLHTTPClient::post(getTwitterConnectURL("/share/status", true), body, new LLTwitterShareResponder());
}

void LLTwitterConnect::storeInfo(const LLSD& info)
{
	mInfo = info;
	mRefreshInfo = false;

	sInfoWatcher->post(info);
}

const LLSD& LLTwitterConnect::getInfo() const
{
	return mInfo;
}

void LLTwitterConnect::clearInfo()
{
	mInfo = LLSD();
}

void LLTwitterConnect::setDataDirty()
{
	mRefreshInfo = true;
}

void LLTwitterConnect::setConnectionState(LLTwitterConnect::EConnectionState connection_state)
{
	if(connection_state == TWITTER_CONNECTED)
	{
		mReadFromMaster = true;
		setConnected(true);
		setDataDirty();
	}
	else if(connection_state == TWITTER_NOT_CONNECTED)
	{
		setConnected(false);
	}
	else if(connection_state == TWITTER_POSTED)
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

void LLTwitterConnect::setConnected(bool connected)
{
	mConnected = connected;
}
