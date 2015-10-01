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
#include "llnotificationsutil.h"
#include "llurlaction.h"
#include "llimagepng.h"
#include "llimagejpeg.h"
#include "lltrans.h"
#include "llevents.h"
#include "llviewerregion.h"

#include "llfloaterwebcontent.h"
#include "llfloaterreg.h"
#include "llcorehttputil.h"

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
void LLTwitterConnect::twitterConnectCoro(std::string requestToken, std::string oauthVerifier)
{
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("TwitterConnect", httpPolicy));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);
    LLCore::HttpOptions::ptr_t httpOpts(new LLCore::HttpOptions);

    httpOpts->setWantHeaders(true);
    httpOpts->setFollowRedirects(false);

    LLSD body;
    if (!requestToken.empty())
        body["request_token"] = requestToken;
    if (!oauthVerifier.empty())
        body["oauth_verifier"] = oauthVerifier;

    LLSD result = httpAdapter->putAndSuspend(httpRequest, getTwitterConnectURL("/connection"), body, httpOpts);

    LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    if (!status)
    {
        if ( status == LLCore::HttpStatus(HTTP_FOUND) )
        {
            std::string location = httpResults[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS_HEADERS][HTTP_IN_HEADER_LOCATION];
            if (location.empty())
            {
                LL_WARNS("FlickrConnect") << "Missing Location header " << LL_ENDL;
            }
            else
            {
                openTwitterWeb(location);
            }
        }
        else
        {
            LL_WARNS("TwitterConnect") << "Connection failed " << status.toString() << LL_ENDL;
            setConnectionState(LLTwitterConnect::TWITTER_CONNECTION_FAILED);
            log_twitter_connect_error("Connect", status.getStatus(), status.toString(),
                result.get("error_code"), result.get("error_description"));
        }
    }
    else
    {
        LL_DEBUGS("TwitterConnect") << "Connect successful. " << LL_ENDL;
        setConnectionState(LLTwitterConnect::TWITTER_CONNECTED);
    }
}

///////////////////////////////////////////////////////////////////////////////
//
bool LLTwitterConnect::testShareStatus(LLSD &result)
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
            LL_WARNS("TwitterConnect") << "Missing Location header " << LL_ENDL;
        }
        else
        {
            openTwitterWeb(location);
        }
    }
    if (status == LLCore::HttpStatus(HTTP_NOT_FOUND))
    {
        LL_DEBUGS("TwitterConnect") << "Not connected. " << LL_ENDL;
        connectToTwitter();
    }
    else
    {
        LL_WARNS("TwitterConnect") << "HTTP Status error " << status.toString() << LL_ENDL;
        setConnectionState(LLTwitterConnect::TWITTER_POST_FAILED);
        log_twitter_connect_error("Share", status.getStatus(), status.toString(),
            result.get("error_code"), result.get("error_description"));
    }
    return false;
}

void LLTwitterConnect::twitterShareCoro(std::string route, LLSD share)
{
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("TwitterConnect", httpPolicy));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);
    LLCore::HttpOptions::ptr_t httpOpts(new LLCore::HttpOptions);

    httpOpts->setWantHeaders(true);
    httpOpts->setFollowRedirects(false);

    LLSD result = httpAdapter->postAndSuspend(httpRequest, getTwitterConnectURL(route, true), share, httpOpts);

    if (testShareStatus(result))
    {
        toast_user_for_twitter_success();
        LL_DEBUGS("TwitterConnect") << "Post successful. " << LL_ENDL;
        setConnectionState(LLTwitterConnect::TWITTER_POSTED);
    }
}

void LLTwitterConnect::twitterShareImageCoro(LLPointer<LLImageFormatted> image, std::string status)
{
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("FlickrConnect", httpPolicy));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);
    LLCore::HttpHeaders::ptr_t httpHeaders(new LLCore::HttpHeaders);
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
    const std::string boundary = "----------------------------0123abcdefab";

    std::string contentType = "multipart/form-data; boundary=" + boundary;
    httpHeaders->append("Content-Type", contentType.c_str());

    LLCore::BufferArray::ptr_t raw = LLCore::BufferArray::ptr_t(new LLCore::BufferArray()); // 
    LLCore::BufferArrayStream body(raw.get());

    // *NOTE: The order seems to matter.
    body << "--" << boundary << "\r\n"
        << "Content-Disposition: form-data; name=\"status\"\r\n\r\n"
        << status << "\r\n";

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

    LLSD result = httpAdapter->postAndSuspend(httpRequest, getTwitterConnectURL("/share/photo", true), raw, httpOpts, httpHeaders);

    if (testShareStatus(result))
    {
        toast_user_for_twitter_success();
        LL_DEBUGS("TwitterConnect") << "Post successful. " << LL_ENDL;
        setConnectionState(LLTwitterConnect::TWITTER_POSTED);
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void LLTwitterConnect::twitterDisconnectCoro()
{
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("TwitterConnect", httpPolicy));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);
    LLCore::HttpOptions::ptr_t httpOpts(new LLCore::HttpOptions);

    httpOpts->setFollowRedirects(false);

    LLSD result = httpAdapter->deleteAndSuspend(httpRequest, getTwitterConnectURL("/connection"), httpOpts);

    LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    if (!status && (status != LLCore::HttpStatus(HTTP_NOT_FOUND)))
    {
        LL_WARNS("TwitterConnect") << "Disconnect failed!" << LL_ENDL;
        setConnectionState(LLTwitterConnect::TWITTER_DISCONNECT_FAILED);

        log_twitter_connect_error("Disconnect", status.getStatus(), status.toString(),
            result.get("error_code"), result.get("error_description"));
    }
    else
    {
        LL_DEBUGS("TwitterConnect") << "Disconnect successful. " << LL_ENDL;
        clearInfo();
        setConnectionState(LLTwitterConnect::TWITTER_NOT_CONNECTED);
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void LLTwitterConnect::twitterConnectedCoro(bool autoConnect)
{
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("TwitterConnect", httpPolicy));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);
    LLCore::HttpOptions::ptr_t httpOpts(new LLCore::HttpOptions);

    httpOpts->setFollowRedirects(false);
    setConnectionState(LLTwitterConnect::TWITTER_CONNECTION_IN_PROGRESS);

    LLSD result = httpAdapter->getAndSuspend(httpRequest, getTwitterConnectURL("/connection", true), httpOpts);

    LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    if (!status)
    {
        if (status == LLCore::HttpStatus(HTTP_NOT_FOUND))
        {
            LL_DEBUGS("TwitterConnect") << "Not connected. " << LL_ENDL;
            if (autoConnect)
            {
                connectToTwitter();
            }
            else
            {
                setConnectionState(LLTwitterConnect::TWITTER_NOT_CONNECTED);
            }
        }
        else
        {
            LL_WARNS("TwitterConnect") << "Failed to test connection:" << status.toTerseString() << LL_ENDL;

            setConnectionState(LLTwitterConnect::TWITTER_CONNECTION_FAILED);
            log_twitter_connect_error("Connected", status.getStatus(), status.toString(),
                result.get("error_code"), result.get("error_description"));
        }
    }
    else
    {
        LL_DEBUGS("TwitterConnect") << "Connect successful. " << LL_ENDL;
        setConnectionState(LLTwitterConnect::TWITTER_CONNECTED);
    }

}

///////////////////////////////////////////////////////////////////////////////
//
void LLTwitterConnect::twitterInfoCoro()
{
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("TwitterConnect", httpPolicy));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);
    LLCore::HttpOptions::ptr_t httpOpts(new LLCore::HttpOptions);

    httpOpts->setWantHeaders(true);
    httpOpts->setFollowRedirects(false);

    LLSD result = httpAdapter->getAndSuspend(httpRequest, getTwitterConnectURL("/info", true), httpOpts);

    LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    if (status == LLCore::HttpStatus(HTTP_FOUND))
    {
        std::string location = httpResults[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS_HEADERS][HTTP_IN_HEADER_LOCATION];
        if (location.empty())
        {
            LL_WARNS("TwitterConnect") << "Missing Location header " << LL_ENDL;
        }
        else
        {
            openTwitterWeb(location);
        }
    }
    else if (!status)
    {
        LL_WARNS("TwitterConnect") << "Twitter Info failed: " << status.toString() << LL_ENDL;
        log_twitter_connect_error("Info", status.getStatus(), status.toString(),
            result.get("error_code"), result.get("error_description"));
    }
    else
    {
        LL_INFOS("TwitterConnect") << "Twitter: Info received" << LL_ENDL;
        result.erase(LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS);
        storeInfo(result);
    }
}

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
    setConnectionState(LLTwitterConnect::TWITTER_CONNECTION_IN_PROGRESS);

    LLCoros::instance().launch("LLTwitterConnect::twitterConnectCoro",
        boost::bind(&LLTwitterConnect::twitterConnectCoro, this, request_token, oauth_verifier));
}

void LLTwitterConnect::disconnectFromTwitter()
{
    setConnectionState(LLTwitterConnect::TWITTER_DISCONNECTING);

    LLCoros::instance().launch("LLTwitterConnect::twitterDisconnectCoro",
        boost::bind(&LLTwitterConnect::twitterDisconnectCoro, this));
}

void LLTwitterConnect::checkConnectionToTwitter(bool auto_connect)
{
    LLCoros::instance().launch("LLTwitterConnect::twitterConnectedCoro",
        boost::bind(&LLTwitterConnect::twitterConnectedCoro, this, auto_connect));
}

void LLTwitterConnect::loadTwitterInfo()
{
	if(mRefreshInfo)
	{
        LLCoros::instance().launch("LLTwitterConnect::twitterInfoCoro",
            boost::bind(&LLTwitterConnect::twitterInfoCoro, this));
	}
}

void LLTwitterConnect::uploadPhoto(const std::string& image_url, const std::string& status)
{
	LLSD body;
	body["image"] = image_url;
	body["status"] = status;

    setConnectionState(LLTwitterConnect::TWITTER_POSTING);

    LLCoros::instance().launch("LLTwitterConnect::twitterShareCoro",
        boost::bind(&LLTwitterConnect::twitterShareCoro, this, "/share/photo", body));
}

void LLTwitterConnect::uploadPhoto(LLPointer<LLImageFormatted> image, const std::string& status)
{
    setConnectionState(LLTwitterConnect::TWITTER_POSTING);

    LLCoros::instance().launch("LLTwitterConnect::twitterShareImageCoro",
        boost::bind(&LLTwitterConnect::twitterShareImageCoro, this, image, status));
}

void LLTwitterConnect::updateStatus(const std::string& status)
{
	LLSD body;
	body["status"] = status;
	
    setConnectionState(LLTwitterConnect::TWITTER_POSTING);

    LLCoros::instance().launch("LLTwitterConnect::twitterShareCoro",
        boost::bind(&LLTwitterConnect::twitterShareCoro, this, "/share/status", body));
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
