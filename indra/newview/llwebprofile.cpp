/**
 * @file llwebprofile.cpp
 * @brief Web profile access.
 *
 * $LicenseInfo:firstyear=2011&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2011, Linden Research, Inc.
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

#include "llwebprofile.h"

// libs
#include "llbufferstream.h"
#include "llimagepng.h"

#include "llsdserialize.h"
#include "llstring.h"

// newview
#include "llavataractions.h" // for getProfileURL()
#include "llviewermedia.h" // FIXME: don't use LLViewerMedia internals
#include "llnotificationsutil.h"

#include "llcorehttputil.h"

// third-party


/*
 * Workflow:
 * 1. LLViewerMedia::setOpenIDCookie()
 *    -> GET https://my-demo.secondlife.com/ via LLViewerMediaWebProfileResponder
 *    -> LLWebProfile::setAuthCookie()
 * 2. LLWebProfile::uploadImage()
 *    -> GET "https://my-demo.secondlife.com/snapshots/s3_upload_config" via ConfigResponder
 * 3. LLWebProfile::post()
 *    -> POST <config_url> via PostImageResponder
 *    -> redirect
 *    -> GET <redirect_url> via PostImageRedirectResponder
 */

///////////////////////////////////////////////////////////////////////////////
// LLWebProfile

std::string LLWebProfile::sAuthCookie;
LLWebProfile::status_callback_t LLWebProfile::mStatusCallback;

// static
void LLWebProfile::uploadImage(LLPointer<LLImageFormatted> image, const std::string& caption, bool add_location)
{
    LLCoros::instance().launch("LLWebProfile::uploadImageCoro",
        boost::bind(&LLWebProfile::uploadImageCoro, image, caption, add_location));

}

// static
void LLWebProfile::setAuthCookie(const std::string& cookie)
{
    LL_DEBUGS("Snapshots") << "Setting auth cookie: " << cookie << LL_ENDL;
    sAuthCookie = cookie;
}


/*static*/
LLCore::HttpHeaders::ptr_t LLWebProfile::buildDefaultHeaders()
{
    LLCore::HttpHeaders::ptr_t httpHeaders(new LLCore::HttpHeaders);
    LLSD headers = LLViewerMedia::getInstance()->getHeaders();

    for (LLSD::map_iterator it = headers.beginMap(); it != headers.endMap(); ++it)
    {
        httpHeaders->append((*it).first, (*it).second.asStringRef());
    }

    return httpHeaders;
}


/*static*/
void LLWebProfile::uploadImageCoro(LLPointer<LLImageFormatted> image, std::string caption, bool addLocation)
{
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("genericPostCoro", httpPolicy));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);
    LLCore::HttpOptions::ptr_t httpOpts(new LLCore::HttpOptions);
    LLCore::HttpHeaders::ptr_t httpHeaders;

    if (dynamic_cast<LLImagePNG*>(image.get()) == 0)
    {
        LL_WARNS() << "Image to upload is not a PNG" << LL_ENDL;
        llassert(dynamic_cast<LLImagePNG*>(image.get()) != 0);
        return;
    }

    httpOpts->setWantHeaders(true);
    httpOpts->setFollowRedirects(false);
    httpOpts->setSSLVerifyPeer(false); ; // viewer's cert bundle doesn't appear to agree with web certs from "https://my.secondlife.com/"

    // Get upload configuration data.
    std::string configUrl(getProfileURL(std::string()) + "snapshots/s3_upload_config");
    configUrl += "?caption=" + LLURI::escape(caption);
    configUrl += "&add_loc=" + std::string(addLocation ? "1" : "0");

    LL_DEBUGS("Snapshots") << "Requesting " << configUrl << LL_ENDL;

    httpHeaders = buildDefaultHeaders();
    httpHeaders->append(HTTP_OUT_HEADER_COOKIE, getAuthCookie());

    LLSD result = httpAdapter->getJsonAndSuspend(httpRequest, configUrl, httpOpts, httpHeaders);

    LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    if (!status)
    {
        LL_WARNS("Snapshots") << "Failed to get image upload config" << LL_ENDL;
        LLWebProfile::reportImageUploadStatus(false);
        if (image->getDataSize() > MAX_WEB_DATASIZE)
        {
            LLNotificationsUtil::add("CannotUploadSnapshotWebTooBig");
        }
        return;
    }

    // Ready to build our image post body.

    const LLSD &data = result["data"];
    const std::string &uploadUrl = result["url"].asStringRef();
    const std::string boundary = "----------------------------0123abcdefab";

    // a new set of headers.
    httpHeaders = LLWebProfile::buildDefaultHeaders();
    httpHeaders->append(HTTP_OUT_HEADER_COOKIE, getAuthCookie());
    httpHeaders->remove(HTTP_OUT_HEADER_CONTENT_TYPE);
    httpHeaders->append(HTTP_OUT_HEADER_CONTENT_TYPE, "multipart/form-data; boundary=" + boundary);

    LLCore::BufferArray::ptr_t body = LLWebProfile::buildPostData(data, image, boundary);

    result = httpAdapter->postAndSuspend(httpRequest, uploadUrl, body, httpOpts, httpHeaders);

    body.reset();
    httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    if (!status && (status != LLCore::HttpStatus(HTTP_SEE_OTHER)))
    {
        LL_WARNS("Snapshots") << "Failed to upload image data." << LL_ENDL;
        LLWebProfile::reportImageUploadStatus(false);
        if (image->getDataSize() > MAX_WEB_DATASIZE)
        {
            LLNotificationsUtil::add("CannotUploadSnapshotWebTooBig");
        }
        return;
    }

    LLSD resultHeaders = httpResults[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS_HEADERS];

    httpHeaders = LLWebProfile::buildDefaultHeaders();
    httpHeaders->append(HTTP_OUT_HEADER_COOKIE, getAuthCookie());

    const std::string& redirUrl = resultHeaders[HTTP_IN_HEADER_LOCATION].asStringRef();

    if (redirUrl.empty())
    {
        LL_WARNS("Snapshots") << "Received empty redirection URL in post image." << LL_ENDL;
        LLWebProfile::reportImageUploadStatus(false);
    }

    LL_DEBUGS("Snapshots") << "Got redirection URL: " << redirUrl << LL_ENDL;

    result = httpAdapter->getRawAndSuspend(httpRequest, redirUrl, httpOpts, httpHeaders);

    httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    if (status != LLCore::HttpStatus(HTTP_OK))
    {
        LL_WARNS("Snapshots") << "Failed to upload image." << LL_ENDL;
        LLWebProfile::reportImageUploadStatus(false);
        if (image->getDataSize() > MAX_WEB_DATASIZE)
        {
            LLNotificationsUtil::add("CannotUploadSnapshotWebTooBig");
        }
        return;
    }

    //LLSD raw = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS_RAW];

    LL_INFOS("Snapshots") << "Image uploaded." << LL_ENDL;
    //LL_DEBUGS("Snapshots") << "Uploading image succeeded. Response: [" << raw.asString() << "]" << LL_ENDL;
    LLWebProfile::reportImageUploadStatus(true);
}

/*static*/
LLCore::BufferArray::ptr_t LLWebProfile::buildPostData(const LLSD &data, LLPointer<LLImageFormatted> &image, const std::string &boundary)
{
    LLCore::BufferArray::ptr_t body(new LLCore::BufferArray);
    LLCore::BufferArrayStream bas(body.get());

    // *NOTE: The order seems to matter.
    bas << "--" << boundary << "\r\n"
        << "Content-Disposition: form-data; name=\"key\"\r\n\r\n"
        << data["key"].asString() << "\r\n";

    bas << "--" << boundary << "\r\n"
        << "Content-Disposition: form-data; name=\"AWSAccessKeyId\"\r\n\r\n"
        << data["AWSAccessKeyId"].asString() << "\r\n";

    bas << "--" << boundary << "\r\n"
        << "Content-Disposition: form-data; name=\"acl\"\r\n\r\n"
        << data["acl"].asString() << "\r\n";

    bas << "--" << boundary << "\r\n"
        << "Content-Disposition: form-data; name=\"Content-Type\"\r\n\r\n"
        << data["Content-Type"].asString() << "\r\n";

    bas << "--" << boundary << "\r\n"
        << "Content-Disposition: form-data; name=\"policy\"\r\n\r\n"
        << data["policy"].asString() << "\r\n";

    bas << "--" << boundary << "\r\n"
        << "Content-Disposition: form-data; name=\"signature\"\r\n\r\n"
        << data["signature"].asString() << "\r\n";

    bas << "--" << boundary << "\r\n"
        << "Content-Disposition: form-data; name=\"success_action_redirect\"\r\n\r\n"
        << data["success_action_redirect"].asString() << "\r\n";

    bas << "--" << boundary << "\r\n"
        << "Content-Disposition: form-data; name=\"file\"; filename=\"snapshot.png\"\r\n"
        << "Content-Type: image/png\r\n\r\n";

    LLImageDataSharedLock lock(image);

    // Insert the image data.
    //char *datap = (char *)(image->getData());
    //bas.write(datap, image->getDataSize());
    const U8* image_data = image->getData();
    for (S32 i = 0; i < image->getDataSize(); ++i)
    {
        bas << image_data[i];
    }

    bas << "\r\n--" << boundary << "--\r\n";

    return body;
}

// static
void LLWebProfile::reportImageUploadStatus(bool ok)
{
    if (mStatusCallback)
    {
        mStatusCallback(ok);
    }
}

// static
std::string LLWebProfile::getAuthCookie()
{
    // This is needed to test image uploads on Linux viewer built with OpenSSL 1.0.0 (0.9.8 works fine).
    return LLStringUtil::getenv("LL_SNAPSHOT_COOKIE", sAuthCookie);
}
