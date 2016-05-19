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
#include "llnotificationsutil.h"
#include "llurlaction.h"
#include "llimagepng.h"
#include "llimagejpeg.h"
#include "lltrans.h"
#include "llevents.h"
#include "llviewerregion.h"
#include "llviewercontrol.h"

#include "llfloaterwebcontent.h"
#include "llfloaterreg.h"
#include "llcorehttputil.h"

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

LLCore::HttpHeaders::ptr_t get_headers()
{
    LLCore::HttpHeaders::ptr_t httpHeaders(new LLCore::HttpHeaders);
    // The DebugSlshareLogTag mechanism is intended to trigger slshare-service
    // debug logging. slshare-service is coded to respond to an X-debug-tag
    // header by engaging debug logging for that request only. This way a
    // developer need not muck with the slshare-service image to engage debug
    // logging. Moreover, the value of X-debug-tag is embedded in each such
    // log line so the developer can quickly find the log lines pertinent to
    // THIS session.
    std::string logtag(gSavedSettings.getString("DebugSlshareLogTag"));
    if (! logtag.empty())
    {
        httpHeaders->append("X-debug-tag", logtag);
    }
    return httpHeaders;
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
void LLFacebookConnect::facebookConnectCoro(std::string authCode, std::string authState)
{
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("FacebookConnect", httpPolicy));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);
    LLCore::HttpOptions::ptr_t httpOpts(new LLCore::HttpOptions);

    LLSD putData;
    if (!authCode.empty())
    {
        putData["code"] = authCode;
    }
    if (!authState.empty())
    {
        putData["state"] = authState;
    }

    httpOpts->setWantHeaders(true);
    httpOpts->setFollowRedirects(false);

    LLSD result = httpAdapter->putAndSuspend(httpRequest, getFacebookConnectURL("/connection"), putData, httpOpts, get_headers());

    LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);
    if (!status)
    {
        if (status == LLCore::HttpStatus(HTTP_FOUND))
        {
            std::string location = httpResults[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS_HEADERS][HTTP_IN_HEADER_LOCATION];
            if (location.empty())
            {
                LL_WARNS("FacebookConnect") << "Missing Location header " << LL_ENDL;
            }
            else
            {
                openFacebookWeb(location);
            }
        }
    }
    else
    {
        LL_INFOS("FacebookConnect") << "Connect successful. " << LL_ENDL;
        setConnectionState(LLFacebookConnect::FB_CONNECTED);
    }

}

///////////////////////////////////////////////////////////////////////////////
//
bool LLFacebookConnect::testShareStatus(LLSD &result)
{
    LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    if (status)
        return true;

    if (status == LLCore::HttpStatus(HTTP_FOUND))
    {
        std::string location = httpResults[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS_HEADERS][HTTP_IN_HEADER_LOCATION];
        if (location.empty())
        {
            LL_WARNS("FacebookConnect") << "Missing Location header " << LL_ENDL;
        }
        else
        {
            openFacebookWeb(location);
        }
    }
    if (status == LLCore::HttpStatus(HTTP_NOT_FOUND))
    {
        LL_DEBUGS("FacebookConnect") << "Not connected. " << LL_ENDL;
        connectToFacebook();
    }
    else
    {
        LL_WARNS("FacebookConnect") << "HTTP Status error " << status.toString() << LL_ENDL;
        setConnectionState(LLFacebookConnect::FB_POST_FAILED);
        log_facebook_connect_error("Share", status.getStatus(), status.toString(),
            result.get("error_code"), result.get("error_description"));
    }
    return false;
}

void LLFacebookConnect::facebookShareCoro(std::string route, LLSD share)
{
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("FacebookConnect", httpPolicy));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);
    LLCore::HttpOptions::ptr_t httpOpts(new LLCore::HttpOptions);

    httpOpts->setWantHeaders(true);
    httpOpts->setFollowRedirects(false);

    LLSD result = httpAdapter->postAndSuspend(httpRequest, getFacebookConnectURL(route, true), share, httpOpts, get_headers());

    if (testShareStatus(result))
    {
        toast_user_for_facebook_success();
        LL_DEBUGS("FacebookConnect") << "Post successful. " << LL_ENDL;
        setConnectionState(LLFacebookConnect::FB_POSTED);
    }
}

void LLFacebookConnect::facebookShareImageCoro(std::string route, LLPointer<LLImageFormatted> image, std::string caption)
{
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("FacebookConnect", httpPolicy));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);
    LLCore::HttpHeaders::ptr_t httpHeaders(get_headers());
    LLCore::HttpOptions::ptr_t httpOpts(new LLCore::HttpOptions);

    httpOpts->setWantHeaders(true);
    httpOpts->setFollowRedirects(false);

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
    static const std::string boundary = "----------------------------0123abcdefab";

    std::string contentType = "multipart/form-data; boundary=" + boundary;
    httpHeaders->append("Content-Type", contentType.c_str());

    LLCore::BufferArray::ptr_t raw = LLCore::BufferArray::ptr_t(new LLCore::BufferArray()); // 
    LLCore::BufferArrayStream body(raw.get());

    // *NOTE: The order seems to matter.
    body << "--" << boundary << "\r\n"
        << "Content-Disposition: form-data; name=\"caption\"\r\n\r\n"
        << caption << "\r\n";

    body << "--" << boundary << "\r\n"
        << "Content-Disposition: form-data; name=\"image\"; filename=\"Untitled." << imageFormat << "\"\r\n"
        << "Content-Type: image/" << imageFormat << "\r\n\r\n";

    // Insert the image data.
    // *FIX: Treating this as a string will probably screw it up ...
    U8* image_data = image->getData();
    for (S32 i = 0; i < image->getDataSize(); ++i)
    {
        body << image_data[i];
    }

    body << "\r\n--" << boundary << "--\r\n";

    setConnectionState(LLFacebookConnect::FB_POSTING);

    LLSD result = httpAdapter->postAndSuspend(httpRequest, getFacebookConnectURL(route, true), raw, httpOpts, httpHeaders);

    if (testShareStatus(result))
    {
        toast_user_for_facebook_success();
        LL_DEBUGS("FacebookConnect") << "Post successful. " << LL_ENDL;
        setConnectionState(LLFacebookConnect::FB_POSTED);
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void LLFacebookConnect::facebookDisconnectCoro()
{
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("FacebookConnect", httpPolicy));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);
    LLCore::HttpOptions::ptr_t httpOpts(new LLCore::HttpOptions);

    httpOpts->setFollowRedirects(false);

    LLSD result = httpAdapter->deleteAndSuspend(httpRequest, getFacebookConnectURL("/connection"), httpOpts, get_headers());

    LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);
    if (!status && (status != LLCore::HttpStatus(HTTP_FOUND)))
    {
        LL_WARNS("FacebookConnect") << "Failed to disconnect:" << status.toTerseString() << LL_ENDL;
        setConnectionState(LLFacebookConnect::FB_DISCONNECT_FAILED);
        log_facebook_connect_error("Disconnect", status.getStatus(), status.toString(),
            result.get("error_code"), result.get("error_description"));
    }
    else
    {
        LL_DEBUGS("FacebookConnect") << "Facebook Disconnect successful. " << LL_ENDL;
        clearInfo();
        clearContent();
        //Notify state change
        setConnectionState(LLFacebookConnect::FB_NOT_CONNECTED);
    }

}

///////////////////////////////////////////////////////////////////////////////
//
void LLFacebookConnect::facebookConnectedCheckCoro(bool autoConnect)
{
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("FacebookConnect", httpPolicy));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);
    LLCore::HttpOptions::ptr_t httpOpts(new LLCore::HttpOptions);

    setConnectionState(LLFacebookConnect::FB_CONNECTION_IN_PROGRESS);

    httpOpts->setFollowRedirects(false);

    LLSD result = httpAdapter->getAndSuspend(httpRequest, getFacebookConnectURL("/connection", true), httpOpts, get_headers());

    LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    if (!status)
    {
        if ( status == LLCore::HttpStatus(HTTP_NOT_FOUND) )
        {
            LL_DEBUGS("FacebookConnect") << "Not connected. " << LL_ENDL;
            if (autoConnect)
            {
                connectToFacebook();
            }
            else
            {
                setConnectionState(LLFacebookConnect::FB_NOT_CONNECTED);
            }
        }
        else
        {
            LL_WARNS("FacebookConnect") << "Failed to test connection:" << status.toTerseString() << LL_ENDL;

            setConnectionState(LLFacebookConnect::FB_DISCONNECT_FAILED);
            log_facebook_connect_error("Connected", status.getStatus(), status.toString(),
                result.get("error_code"), result.get("error_description"));
        }
    }
    else
    {
        LL_DEBUGS("FacebookConnect") << "Connect successful. " << LL_ENDL;
        setConnectionState(LLFacebookConnect::FB_CONNECTED);
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void LLFacebookConnect::facebookConnectInfoCoro()
{
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("FacebookConnect", httpPolicy));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);
    LLCore::HttpOptions::ptr_t httpOpts(new LLCore::HttpOptions);

    httpOpts->setWantHeaders(true);
    httpOpts->setFollowRedirects(false);

    LLSD result = httpAdapter->getAndSuspend(httpRequest, getFacebookConnectURL("/info", true), httpOpts, get_headers());

    LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    if (status == LLCore::HttpStatus(HTTP_FOUND))
    {
        std::string location = httpResults[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS_HEADERS][HTTP_IN_HEADER_LOCATION];
        if (location.empty())
        {
            LL_WARNS("FacebookConnect") << "Missing Location header " << LL_ENDL;
        }
        else
        {
            openFacebookWeb(location);
        }
    }
    else if (!status)
    {
        LL_WARNS("FacebookConnect") << "Facebook Info failed: " << status.toString() << LL_ENDL;
        log_facebook_connect_error("Info", status.getStatus(), status.toString(),
            result.get("error_code"), result.get("error_description"));
    }
    else
    {
        LL_INFOS("FacebookConnect") << "Facebook: Info received" << LL_ENDL;
        result.erase(LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS);
        storeInfo(result);
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void LLFacebookConnect::facebookConnectFriendsCoro()
{
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("FacebookConnect", httpPolicy));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);
    LLCore::HttpOptions::ptr_t httpOpts(new LLCore::HttpOptions);

    httpOpts->setFollowRedirects(false);

    LLSD result = httpAdapter->getAndSuspend(httpRequest, getFacebookConnectURL("/friends", true), httpOpts, get_headers());

    LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    if (status == LLCore::HttpStatus(HTTP_FOUND))
    {
        std::string location = httpResults[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS_HEADERS][HTTP_IN_HEADER_LOCATION];
        if (location.empty())
        {
            LL_WARNS("FacebookConnect") << "Missing Location header " << LL_ENDL;
        }
        else
        {
            openFacebookWeb(location);
        }
    }
    else if (!status)
    {
        LL_WARNS("FacebookConnect") << "Facebook Friends failed: " << status.toString() << LL_ENDL;
        log_facebook_connect_error("Info", status.getStatus(), status.toString(),
            result.get("error_code"), result.get("error_description"));
    }
    else
    {
        LL_INFOS("FacebookConnect") << "Facebook: Friends received" << LL_ENDL;
        result.erase(LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS);
        LLSD content = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS_CONTENT];
        storeContent(content);
    }
}

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
    setConnectionState(LLFacebookConnect::FB_CONNECTION_IN_PROGRESS);

    LLCoros::instance().launch("LLFacebookConnect::facebookConnectCoro",
        boost::bind(&LLFacebookConnect::facebookConnectCoro, this, auth_code, auth_state));
}

void LLFacebookConnect::disconnectFromFacebook()
{
    LLCoros::instance().launch("LLFacebookConnect::facebookDisconnectCoro",
        boost::bind(&LLFacebookConnect::facebookDisconnectCoro, this));
}

void LLFacebookConnect::checkConnectionToFacebook(bool auto_connect)
{
    setConnectionState(LLFacebookConnect::FB_DISCONNECTING);

    LLCoros::instance().launch("LLFacebookConnect::facebookConnectedCheckCoro",
        boost::bind(&LLFacebookConnect::facebookConnectedCheckCoro, this, auto_connect));
}

void LLFacebookConnect::loadFacebookInfo()
{
	if(mRefreshInfo)
	{
        LLCoros::instance().launch("LLFacebookConnect::facebookConnectInfoCoro",
            boost::bind(&LLFacebookConnect::facebookConnectInfoCoro, this));
	}
}

void LLFacebookConnect::loadFacebookFriends()
{
	if(mRefreshContent)
	{
        LLCoros::instance().launch("LLFacebookConnect::facebookConnectFriendsCoro",
            boost::bind(&LLFacebookConnect::facebookConnectFriendsCoro, this));
	}
}

void LLFacebookConnect::postCheckin(const std::string& location, const std::string& name, 
    const std::string& description, const std::string& image, const std::string& message)
{
    setConnectionState(LLFacebookConnect::FB_POSTING);

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

    LLCoros::instance().launch("LLFacebookConnect::facebookShareCoro",
        boost::bind(&LLFacebookConnect::facebookShareCoro, this, "/share/checkin", body));
}

void LLFacebookConnect::sharePhoto(const std::string& image_url, const std::string& caption)
{
    setConnectionState(LLFacebookConnect::FB_POSTING);

	LLSD body;
	body["image"] = image_url;
	body["caption"] = caption;
	
    LLCoros::instance().launch("LLFacebookConnect::facebookShareCoro",
        boost::bind(&LLFacebookConnect::facebookShareCoro, this, "/share/photo", body));
}

void LLFacebookConnect::sharePhoto(LLPointer<LLImageFormatted> image, const std::string& caption)
{
    setConnectionState(LLFacebookConnect::FB_POSTING);

    LLCoros::instance().launch("LLFacebookConnect::facebookShareImageCoro",
        boost::bind(&LLFacebookConnect::facebookShareImageCoro, this, "/share/photo", image, caption));
}

void LLFacebookConnect::updateStatus(const std::string& message)
{
	LLSD body;
	body["message"] = message;

    setConnectionState(LLFacebookConnect::FB_POSTING);

    LLCoros::instance().launch("LLFacebookConnect::facebookShareCoro",
        boost::bind(&LLFacebookConnect::facebookShareCoro, this, "/share/wall", body));
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
