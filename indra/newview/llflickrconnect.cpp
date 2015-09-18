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
void LLFlickrConnect::flickrConnectCoro(std::string requestToken, std::string oauthVerifier)
{
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("FlickrConnect", httpPolicy));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);
    LLCore::HttpOptions::ptr_t httpOpts(new LLCore::HttpOptions);

    httpOpts->setWantHeaders(true);
    httpOpts->setFollowRedirects(false);

    LLSD body;
    if (!requestToken.empty())
        body["request_token"] = requestToken;
    if (!oauthVerifier.empty())
        body["oauth_verifier"] = oauthVerifier;

    setConnectionState(LLFlickrConnect::FLICKR_CONNECTION_IN_PROGRESS);

    LLSD result = httpAdapter->putAndSuspend(httpRequest, getFlickrConnectURL("/connection"), body, httpOpts);

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
                openFlickrWeb(location);
            }
        }
        else
        {
            LL_WARNS("FlickrConnect") << "Connection failed " << status.toString() << LL_ENDL;
            setConnectionState(LLFlickrConnect::FLICKR_CONNECTION_FAILED);
            log_flickr_connect_error("Connect", status.getStatus(), status.toString(),
                result.get("error_code"), result.get("error_description"));
        }
    }
    else
    {
        LL_DEBUGS("FlickrConnect") << "Connect successful. " << LL_ENDL;
        setConnectionState(LLFlickrConnect::FLICKR_CONNECTED);
    }
}

///////////////////////////////////////////////////////////////////////////////
//
bool LLFlickrConnect::testShareStatus(LLSD &result)
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
            LL_WARNS("FlickrConnect") << "Missing Location header " << LL_ENDL;
        }
        else
        {
            openFlickrWeb(location);
        }
    }
    if (status == LLCore::HttpStatus(HTTP_NOT_FOUND))
    {
        LL_DEBUGS("FlickrConnect") << "Not connected. " << LL_ENDL;
        connectToFlickr();
    }
    else
    {
        LL_WARNS("FlickrConnect") << "HTTP Status error " << status.toString() << LL_ENDL;
        setConnectionState(LLFlickrConnect::FLICKR_POST_FAILED);
        log_flickr_connect_error("Share", status.getStatus(), status.toString(),
            result.get("error_code"), result.get("error_description"));
    }
    return false;
}

void LLFlickrConnect::flickrShareCoro(LLSD share)
{
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("FlickrConnect", httpPolicy));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);
    LLCore::HttpOptions::ptr_t httpOpts(new LLCore::HttpOptions);

    httpOpts->setWantHeaders(true);
    httpOpts->setFollowRedirects(false);

    LLSD result = httpAdapter->postAndSuspend(httpRequest, getFlickrConnectURL("/share/photo", true), share, httpOpts);

    if (testShareStatus(result))
    {
        toast_user_for_flickr_success();
        LL_DEBUGS("FlickrConnect") << "Post successful. " << LL_ENDL;
        setConnectionState(LLFlickrConnect::FLICKR_POSTED);
    }

}

void LLFlickrConnect::flickrShareImageCoro(LLPointer<LLImageFormatted> image, std::string title, std::string description, std::string tags, int safetyLevel)
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
        << "Content-Disposition: form-data; name=\"title\"\r\n\r\n"
        << title << "\r\n";

    body << "--" << boundary << "\r\n"
        << "Content-Disposition: form-data; name=\"description\"\r\n\r\n"
        << description << "\r\n";

    body << "--" << boundary << "\r\n"
        << "Content-Disposition: form-data; name=\"tags\"\r\n\r\n"
        << tags << "\r\n";

    body << "--" << boundary << "\r\n"
        << "Content-Disposition: form-data; name=\"safety_level\"\r\n\r\n"
        << safetyLevel << "\r\n";

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

    LLSD result = httpAdapter->postAndSuspend(httpRequest, getFlickrConnectURL("/share/photo", true), raw, httpOpts, httpHeaders);

    if (testShareStatus(result))
    {
        toast_user_for_flickr_success();
        LL_DEBUGS("FlickrConnect") << "Post successful. " << LL_ENDL;
        setConnectionState(LLFlickrConnect::FLICKR_POSTED);
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void LLFlickrConnect::flickrDisconnectCoro()
{
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("FlickrConnect", httpPolicy));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);
    LLCore::HttpOptions::ptr_t httpOpts(new LLCore::HttpOptions);

    setConnectionState(LLFlickrConnect::FLICKR_DISCONNECTING);
    httpOpts->setFollowRedirects(false);

    LLSD result = httpAdapter->deleteAndSuspend(httpRequest, getFlickrConnectURL("/connection"), httpOpts);

    LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    if (!status && (status != LLCore::HttpStatus(HTTP_NOT_FOUND)))
    {
        LL_WARNS("FlickrConnect") << "Disconnect failed!" << LL_ENDL;
        setConnectionState(LLFlickrConnect::FLICKR_DISCONNECT_FAILED);

        log_flickr_connect_error("Disconnect", status.getStatus(), status.toString(),
            result.get("error_code"), result.get("error_description"));
    }
    else
    {
        LL_DEBUGS("FlickrConnect") << "Disconnect successful. " << LL_ENDL;
        clearInfo();
        setConnectionState(LLFlickrConnect::FLICKR_NOT_CONNECTED);
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void LLFlickrConnect::flickrConnectedCoro(bool autoConnect)
{
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("FlickrConnect", httpPolicy));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);
    LLCore::HttpOptions::ptr_t httpOpts(new LLCore::HttpOptions);

    setConnectionState(LLFlickrConnect::FLICKR_CONNECTION_IN_PROGRESS);

    httpOpts->setFollowRedirects(false);

    LLSD result = httpAdapter->getAndSuspend(httpRequest, getFlickrConnectURL("/connection", true), httpOpts);

    LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    if (!status)
    {
        if (status == LLCore::HttpStatus(HTTP_NOT_FOUND))
        {
            LL_DEBUGS("FlickrConnect") << "Not connected. " << LL_ENDL;
            if (autoConnect)
            {
                connectToFlickr();
            }
            else
            {
                setConnectionState(LLFlickrConnect::FLICKR_NOT_CONNECTED);
            }
        }
        else
        {
            LL_WARNS("FlickrConnect") << "Failed to test connection:" << status.toTerseString() << LL_ENDL;

            setConnectionState(LLFlickrConnect::FLICKR_CONNECTION_FAILED);
            log_flickr_connect_error("Connected", status.getStatus(), status.toString(),
                result.get("error_code"), result.get("error_description"));
        }
    }
    else
    {
        LL_DEBUGS("FlickrConnect") << "Connect successful. " << LL_ENDL;
        setConnectionState(LLFlickrConnect::FLICKR_CONNECTED);
    }

}

///////////////////////////////////////////////////////////////////////////////
//
void LLFlickrConnect::flickrInfoCoro()
{
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("FlickrConnect", httpPolicy));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);
    LLCore::HttpOptions::ptr_t httpOpts(new LLCore::HttpOptions);

    httpOpts->setWantHeaders(true);
    httpOpts->setFollowRedirects(false);

    LLSD result = httpAdapter->getAndSuspend(httpRequest, getFlickrConnectURL("/info", true), httpOpts);

    LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    if (status == LLCore::HttpStatus(HTTP_FOUND))
    {
        std::string location = httpResults[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS_HEADERS][HTTP_IN_HEADER_LOCATION];
        if (location.empty())
        {
            LL_WARNS("FlickrConnect") << "Missing Location header " << LL_ENDL;
        }
        else
        {
            openFlickrWeb(location);
        }
    }
    else if (!status)
    {
        LL_WARNS("FlickrConnect") << "Flickr Info failed: " << status.toString() << LL_ENDL;
        log_flickr_connect_error("Info", status.getStatus(), status.toString(),
            result.get("error_code"), result.get("error_description"));
    }
    else
    {
        LL_INFOS("FlickrConnect") << "Flickr: Info received" << LL_ENDL;
        result.erase(LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS);
        storeInfo(result);
    }
}

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
    LLCoros::instance().launch("LLFlickrConnect::flickrConnectCoro",
        boost::bind(&LLFlickrConnect::flickrConnectCoro, this, request_token, oauth_verifier));
}

void LLFlickrConnect::disconnectFromFlickr()
{
    LLCoros::instance().launch("LLFlickrConnect::flickrDisconnectCoro",
        boost::bind(&LLFlickrConnect::flickrDisconnectCoro, this));
}

void LLFlickrConnect::checkConnectionToFlickr(bool auto_connect)
{
    LLCoros::instance().launch("LLFlickrConnect::flickrConnectedCoro",
        boost::bind(&LLFlickrConnect::flickrConnectedCoro, this, auto_connect));
}

void LLFlickrConnect::loadFlickrInfo()
{
	if(mRefreshInfo)
	{
        LLCoros::instance().launch("LLFlickrConnect::flickrInfoCoro",
            boost::bind(&LLFlickrConnect::flickrInfoCoro, this));
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

    setConnectionState(LLFlickrConnect::FLICKR_POSTING);

    LLCoros::instance().launch("LLFlickrConnect::flickrShareCoro",
        boost::bind(&LLFlickrConnect::flickrShareCoro, this, body));
}

void LLFlickrConnect::uploadPhoto(LLPointer<LLImageFormatted> image, const std::string& title, const std::string& description, const std::string& tags, int safety_level)
{
    setConnectionState(LLFlickrConnect::FLICKR_POSTING);

    LLCoros::instance().launch("LLFlickrConnect::flickrShareImageCoro",
        boost::bind(&LLFlickrConnect::flickrShareImageCoro, this, image, 
        title, description, tags, safety_level));
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
		// set the connection state before notifying watchers
		mConnectionState = connection_state;

		LLSD state_info;
		state_info["enum"] = connection_state;
		sStateWatcher->post(state_info);
	}
}

void LLFlickrConnect::setConnected(bool connected)
{
	mConnected = connected;
}
