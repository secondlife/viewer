/** 
 * @file llflickrconnect.h
 * @author Merov, Cho
 * @brief Connection to Flickr Service
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

#include "llflickrconnect.h"

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

boost::scoped_ptr<LLEventPump> LLFlickrConnect::sStateWatcher(new LLEventStream("FlickrConnectState"));
boost::scoped_ptr<LLEventPump> LLFlickrConnect::sInfoWatcher(new LLEventStream("FlickrConnectInfo"));
boost::scoped_ptr<LLEventPump> LLFlickrConnect::sContentWatcher(new LLEventStream("FlickrConnectContent"));

// Local functions
void log_flickr_connect_error(const std::string& request, U32 status, const std::string& reason, const std::string& code, const std::string& description)
{
    // Note: 302 (redirect) is *not* an error that warrants logging
    if (status != 302)
    {
		LL_WARNS("FlickrConnect") << request << " request failed with a " << status << " " << reason << ". Reason: " << code << " (" << description << ")" << LL_ENDL;
    }
}

void toast_user_for_flickr_success()
{
	LLSD args;
    args["MESSAGE"] = LLTrans::getString("flickr_post_success");
    LLNotificationsUtil::add("FlickrConnect", args);
}

///////////////////////////////////////////////////////////////////////////////
//
class LLFlickrConnectResponder : public LLHTTPClient::Responder
{
	LOG_CLASS(LLFlickrConnectResponder);
public:
	
    LLFlickrConnectResponder()
    {
        LLFlickrConnect::instance().setConnectionState(LLFlickrConnect::FLICKR_CONNECTION_IN_PROGRESS);
    }
    
	virtual void completed(U32 status, const std::string& reason, const LLSD& content)
	{
		if (isGoodStatus(status))
		{
			LL_DEBUGS("FlickrConnect") << "Connect successful. content: " << content << LL_ENDL;
			
            LLFlickrConnect::instance().setConnectionState(LLFlickrConnect::FLICKR_CONNECTED);
		}
		else if (status != 302)
		{
            LLFlickrConnect::instance().setConnectionState(LLFlickrConnect::FLICKR_CONNECTION_FAILED);
            log_flickr_connect_error("Connect", status, reason, content.get("error_code"), content.get("error_description"));
		}
	}
    
    void completedHeader(U32 status, const std::string& reason, const LLSD& content)
    {
        if (status == 302)
        {
            LLFlickrConnect::instance().openFlickrWeb(content["location"]);
        }
    }
};

///////////////////////////////////////////////////////////////////////////////
//
class LLFlickrShareResponder : public LLHTTPClient::Responder
{
	LOG_CLASS(LLFlickrShareResponder);
public:
    
	LLFlickrShareResponder()
	{
		LLFlickrConnect::instance().setConnectionState(LLFlickrConnect::FLICKR_POSTING);
	}
	
	virtual void completed(U32 status, const std::string& reason, const LLSD& content)
	{
		if (isGoodStatus(status))
		{
            toast_user_for_flickr_success();
			LL_DEBUGS("FlickrConnect") << "Post successful. content: " << content << LL_ENDL;
			
			LLFlickrConnect::instance().setConnectionState(LLFlickrConnect::FLICKR_POSTED);
		}
		else if (status == 404)
		{
			LLFlickrConnect::instance().connectToFlickr();
		}
		else
		{
            LLFlickrConnect::instance().setConnectionState(LLFlickrConnect::FLICKR_POST_FAILED);
            log_flickr_connect_error("Share", status, reason, content.get("error_code"), content.get("error_description"));
		}
	}
    
    void completedHeader(U32 status, const std::string& reason, const LLSD& content)
    {
        if (status == 302)
        {
            LLFlickrConnect::instance().openFlickrWeb(content["location"]);
        }
    }
};

///////////////////////////////////////////////////////////////////////////////
//
class LLFlickrDisconnectResponder : public LLHTTPClient::Responder
{
	LOG_CLASS(LLFlickrDisconnectResponder);
public:
 
	LLFlickrDisconnectResponder()
	{
		LLFlickrConnect::instance().setConnectionState(LLFlickrConnect::FLICKR_DISCONNECTING);
	}

	void setUserDisconnected()
	{
		// Clear data
		LLFlickrConnect::instance().clearInfo();

		//Notify state change
		LLFlickrConnect::instance().setConnectionState(LLFlickrConnect::FLICKR_NOT_CONNECTED);
	}

	virtual void completed(U32 status, const std::string& reason, const LLSD& content)
	{
		if (isGoodStatus(status)) 
		{
			LL_DEBUGS("FlickrConnect") << "Disconnect successful. content: " << content << LL_ENDL;
			setUserDisconnected();

		}
		//User not found so already disconnected
		else if(status == 404)
		{
			LL_DEBUGS("FlickrConnect") << "Already disconnected. content: " << content << LL_ENDL;
			setUserDisconnected();
		}
		else
		{
			LLFlickrConnect::instance().setConnectionState(LLFlickrConnect::FLICKR_DISCONNECT_FAILED);
            log_flickr_connect_error("Disconnect", status, reason, content.get("error_code"), content.get("error_description"));
		}
	}
};

///////////////////////////////////////////////////////////////////////////////
//
class LLFlickrConnectedResponder : public LLHTTPClient::Responder
{
	LOG_CLASS(LLFlickrConnectedResponder);
public:
    
	LLFlickrConnectedResponder(bool auto_connect) : mAutoConnect(auto_connect)
    {
		LLFlickrConnect::instance().setConnectionState(LLFlickrConnect::FLICKR_CONNECTION_IN_PROGRESS);
    }
    
	virtual void completed(U32 status, const std::string& reason, const LLSD& content)
	{
		if (isGoodStatus(status))
		{
			LL_DEBUGS("FlickrConnect") << "Connect successful. content: " << content << LL_ENDL;
            
            LLFlickrConnect::instance().setConnectionState(LLFlickrConnect::FLICKR_CONNECTED);
		}
		else
		{
			// show the flickr login page if not connected yet
			if (status == 404)
			{
				if (mAutoConnect)
				{
					LLFlickrConnect::instance().connectToFlickr();
				}
				else
				{
					LLFlickrConnect::instance().setConnectionState(LLFlickrConnect::FLICKR_NOT_CONNECTED);
				}
			}
            else
            {
                LLFlickrConnect::instance().setConnectionState(LLFlickrConnect::FLICKR_CONNECTION_FAILED);
				log_flickr_connect_error("Connected", status, reason, content.get("error_code"), content.get("error_description"));
            }
		}
	}
    
private:
	bool mAutoConnect;
};

///////////////////////////////////////////////////////////////////////////////
//
class LLFlickrInfoResponder : public LLHTTPClient::Responder
{
	LOG_CLASS(LLFlickrInfoResponder);
public:

	virtual void completed(U32 status, const std::string& reason, const LLSD& info)
	{
		if (isGoodStatus(status))
		{
			llinfos << "Flickr: Info received" << llendl;
			LL_DEBUGS("FlickrConnect") << "Getting Flickr info successful. info: " << info << LL_ENDL;
			LLFlickrConnect::instance().storeInfo(info);
		}
		else
		{
			log_flickr_connect_error("Info", status, reason, info.get("error_code"), info.get("error_description"));
		}
	}

	void completedHeader(U32 status, const std::string& reason, const LLSD& content)
	{
		if (status == 302)
		{
			LLFlickrConnect::instance().openFlickrWeb(content["location"]);
		}
	}
};

///////////////////////////////////////////////////////////////////////////////
//
LLFlickrConnect::LLFlickrConnect()
:	mConnectionState(FLICKR_NOT_CONNECTED),
	mConnected(false),
	mInfo(),
	mRefreshInfo(false),
	mReadFromMaster(false)
{
}

void LLFlickrConnect::openFlickrWeb(std::string url)
{
	// Open the URL in an internal browser window without navigation UI
	LLFloaterWebContent::Params p;
    p.url(url);
    p.show_chrome(true);
    p.allow_address_entry(false);
    p.allow_back_forward_navigation(false);
    p.trusted_content(true);
    p.clean_browser(true);
	LLFloater *floater = LLFloaterReg::showInstance("flickr_web", p);
	//the internal web browser has a bug that prevents it from gaining focus unless a mouse event occurs first (it seems).
	//So when showing the internal web browser, set focus to it's containing floater "flickr_web". When a mouse event 
	//occurs on the "webbrowser" panel part of the floater, a mouse cursor will properly show and the "webbrowser" will gain focus.
	//flickr_web floater contains the "webbrowser" panel.    JIRA: ACME-744
	gFocusMgr.setKeyboardFocus( floater );

	//LLUrlAction::openURLExternal(url);
}

std::string LLFlickrConnect::getFlickrConnectURL(const std::string& route, bool include_read_from_master)
{
    std::string url("");
    LLViewerRegion *regionp = gAgent.getRegion();
    if (regionp)
    {
		//url = "http://pdp15.lindenlab.com/flickr/agent/" + gAgentID.asString(); // TEMPORARY FOR TESTING - CHO
        url = regionp->getCapability("FlickrConnect");
        url += route;
    
        if (include_read_from_master && mReadFromMaster)
        {
            url += "?read_from_master=true";
        }
    }
	return url;
}

void LLFlickrConnect::connectToFlickr(const std::string& request_token, const std::string& oauth_verifier)
{
	LLSD body;
	if (!request_token.empty())
		body["request_token"] = request_token;
	if (!oauth_verifier.empty())
		body["oauth_verifier"] = oauth_verifier;
    
	LLHTTPClient::put(getFlickrConnectURL("/connection"), body, new LLFlickrConnectResponder());
}

void LLFlickrConnect::disconnectFromFlickr()
{
	LLHTTPClient::del(getFlickrConnectURL("/connection"), new LLFlickrDisconnectResponder());
}

void LLFlickrConnect::checkConnectionToFlickr(bool auto_connect)
{
	const bool follow_redirects = false;
	const F32 timeout = HTTP_REQUEST_EXPIRY_SECS;
	LLHTTPClient::get(getFlickrConnectURL("/connection", true), new LLFlickrConnectedResponder(auto_connect),
						LLSD(), timeout, follow_redirects);
}

void LLFlickrConnect::loadFlickrInfo()
{
	if(mRefreshInfo)
	{
		const bool follow_redirects = false;
		const F32 timeout = HTTP_REQUEST_EXPIRY_SECS;
		LLHTTPClient::get(getFlickrConnectURL("/info", true), new LLFlickrInfoResponder(),
			LLSD(), timeout, follow_redirects);
	}
}

void LLFlickrConnect::uploadPhoto(const std::string& image_url, const std::string& title, const std::string& description, const std::string& tags, int safety_level)
{
	LLSD body;
	body["image"] = image_url;
	body["title"] = title;
	body["description"] = description;
	body["tags"] = tags;
	body["safety_level"] = safety_level;
	
    // Note: we can use that route for different publish action. We should be able to use the same responder.
	LLHTTPClient::post(getFlickrConnectURL("/share/photo", true), body, new LLFlickrShareResponder());
}

void LLFlickrConnect::uploadPhoto(LLPointer<LLImageFormatted> image, const std::string& title, const std::string& description, const std::string& tags, int safety_level)
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
			<< "Content-Disposition: form-data; name=\"title\"\r\n\r\n"
			<< title << "\r\n";

	body	<< "--" << boundary << "\r\n"
			<< "Content-Disposition: form-data; name=\"description\"\r\n\r\n"
			<< description << "\r\n";

	body	<< "--" << boundary << "\r\n"
			<< "Content-Disposition: form-data; name=\"tags\"\r\n\r\n"
			<< tags << "\r\n";

	body	<< "--" << boundary << "\r\n"
			<< "Content-Disposition: form-data; name=\"safety_level\"\r\n\r\n"
			<< safety_level << "\r\n";

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
	LLHTTPClient::postRaw(getFlickrConnectURL("/share/photo", true), data, size, new LLFlickrShareResponder(), headers);
}

void LLFlickrConnect::storeInfo(const LLSD& info)
{
	mInfo = info;
	mRefreshInfo = false;

	sInfoWatcher->post(info);
}

const LLSD& LLFlickrConnect::getInfo() const
{
	return mInfo;
}

void LLFlickrConnect::clearInfo()
{
	mInfo = LLSD();
}

void LLFlickrConnect::setDataDirty()
{
	mRefreshInfo = true;
}

void LLFlickrConnect::setConnectionState(LLFlickrConnect::EConnectionState connection_state)
{
	if(connection_state == FLICKR_CONNECTED)
	{
		mReadFromMaster = true;
		setConnected(true);
		setDataDirty();
	}
	else if(connection_state == FLICKR_NOT_CONNECTED)
	{
		setConnected(false);
	}
	else if(connection_state == FLICKR_POSTED)
	{
		mReadFromMaster = false;
	}

	if (mConnectionState != connection_state)
	{
		LLSD state_info;
		state_info["enum"] = connection_state;
		sStateWatcher->post(state_info);
	}
	
	mConnectionState = connection_state;
}

void LLFlickrConnect::setConnected(bool connected)
{
	mConnected = connected;
}
