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
#include "llhttpclient.h"
#include "llimagepng.h"
#include "llplugincookiestore.h"

// newview
#include "llpanelprofile.h" // for getProfileURL(). FIXME: move the method to LLAvatarActions
#include "llviewermedia.h" // FIXME: don't use LLViewerMedia internals

// third-party
#include "reader.h" // JSON

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
// LLWebProfileResponders::ConfigResponder

class LLWebProfileResponders::ConfigResponder : public LLHTTPClient::Responder
{
	LOG_CLASS(LLWebProfileResponders::ConfigResponder);

public:
	ConfigResponder(LLPointer<LLImageFormatted> imagep)
	:	mImagep(imagep)
	{
	}

	/*virtual*/ void completedRaw(
		U32 status,
		const std::string& reason,
		const LLChannelDescriptors& channels,
		const LLIOPipe::buffer_ptr_t& buffer)
	{
		LLBufferStream istr(channels, buffer.get());
		std::stringstream strstrm;
		strstrm << istr.rdbuf();
		const std::string body = strstrm.str();

		if (status != 200)
		{
			llwarns << "Failed to get upload config (" << status << ")" << llendl;
			LLWebProfile::reportImageUploadStatus(false);
			return;
		}

		Json::Value root;
		Json::Reader reader;
		if (!reader.parse(body, root))
		{
			llwarns << "Failed to parse upload config: " << reader.getFormatedErrorMessages() << llendl;
			LLWebProfile::reportImageUploadStatus(false);
			return;
		}

		// *TODO: 404 = not supported by the grid
		// *TODO: increase timeout or handle 499 Expired

		// Convert config to LLSD.
		const Json::Value data = root["data"];
		const std::string upload_url = root["url"].asString();
		LLSD config;
		config["acl"]						= data["acl"].asString();
		config["AWSAccessKeyId"]			= data["AWSAccessKeyId"].asString();
		config["Content-Type"]				= data["Content-Type"].asString();
		config["key"]						= data["key"].asString();
		config["policy"]					= data["policy"].asString();
		config["success_action_redirect"]	= data["success_action_redirect"].asString();
		config["signature"]					= data["signature"].asString();
		config["add_loc"]					= data.get("add_loc", "0").asString();
		config["caption"]					= data.get("caption", "").asString();

		// Do the actual image upload using the configuration.
		LL_DEBUGS("Snapshots") << "Got upload config, POSTing image to " << upload_url << ", config=[" << config << "]" << llendl;
		LLWebProfile::post(mImagep, config, upload_url);
	}

private:
	LLPointer<LLImageFormatted> mImagep;
};

///////////////////////////////////////////////////////////////////////////////
// LLWebProfilePostImageRedirectResponder
class LLWebProfileResponders::PostImageRedirectResponder : public LLHTTPClient::Responder
{
	LOG_CLASS(LLWebProfileResponders::PostImageRedirectResponder);

public:
	/*virtual*/ void completedRaw(
		U32 status,
		const std::string& reason,
		const LLChannelDescriptors& channels,
		const LLIOPipe::buffer_ptr_t& buffer)
	{
		if (status != 200)
		{
			llwarns << "Failed to upload image: " << status << " " << reason << llendl;
			LLWebProfile::reportImageUploadStatus(false);
			return;
		}

		LLBufferStream istr(channels, buffer.get());
		std::stringstream strstrm;
		strstrm << istr.rdbuf();
		const std::string body = strstrm.str();
		llinfos << "Image uploaded." << llendl;
		LL_DEBUGS("Snapshots") << "Uploading image succeeded. Response: [" << body << "]" << llendl;
		LLWebProfile::reportImageUploadStatus(true);
	}

private:
	LLPointer<LLImageFormatted> mImagep;
};


///////////////////////////////////////////////////////////////////////////////
// LLWebProfileResponders::PostImageResponder
class LLWebProfileResponders::PostImageResponder : public LLHTTPClient::Responder
{
	LOG_CLASS(LLWebProfileResponders::PostImageResponder);

public:
	/*virtual*/ void completedHeader(U32 status, const std::string& reason, const LLSD& content)
	{
		// Viewer seems to fail to follow a 303 redirect on POST request
		// (URLRequest Error: 65, Send failed since rewinding of the data stream failed).
		// Handle it manually.
		if (status == 303)
		{
			LLSD headers = LLViewerMedia::getHeaders();
			headers["Cookie"] = LLWebProfile::getAuthCookie();
			const std::string& redir_url = content["location"];
			LL_DEBUGS("Snapshots") << "Got redirection URL: " << redir_url << llendl;
			LLHTTPClient::get(redir_url, new LLWebProfileResponders::PostImageRedirectResponder, headers);
		}
		else
		{
			llwarns << "Unexpected POST status: " << status << " " << reason << llendl;
			LL_DEBUGS("Snapshots") << "headers: [" << content << "]" << llendl;
			LLWebProfile::reportImageUploadStatus(false);
		}
	}

	// Override just to suppress warnings.
	/*virtual*/ void completedRaw(U32 status, const std::string& reason,
							  const LLChannelDescriptors& channels,
							  const LLIOPipe::buffer_ptr_t& buffer)
	{
	}
};

///////////////////////////////////////////////////////////////////////////////
// LLWebProfile

std::string LLWebProfile::sAuthCookie;
LLWebProfile::status_callback_t LLWebProfile::mStatusCallback;

// static
void LLWebProfile::uploadImage(LLPointer<LLImageFormatted> image, const std::string& caption, bool add_location)
{
	// Get upload configuration data.
	std::string config_url(getProfileURL(LLStringUtil::null) + "snapshots/s3_upload_config");
	config_url += "?caption=" + LLURI::escape(caption);
	config_url += "&add_loc=" + std::string(add_location ? "1" : "0");

	LL_DEBUGS("Snapshots") << "Requesting " << config_url << llendl;
	LLSD headers = LLViewerMedia::getHeaders();
	headers["Cookie"] = getAuthCookie();
	LLHTTPClient::get(config_url, new LLWebProfileResponders::ConfigResponder(image), headers);
}

// static
void LLWebProfile::setAuthCookie(const std::string& cookie)
{
	LL_DEBUGS("Snapshots") << "Setting auth cookie: " << cookie << llendl;
	sAuthCookie = cookie;
}

// static
void LLWebProfile::post(LLPointer<LLImageFormatted> image, const LLSD& config, const std::string& url)
{
	if (dynamic_cast<LLImagePNG*>(image.get()) == 0)
	{
		llwarns << "Image to upload is not a PNG" << llendl;
		llassert(dynamic_cast<LLImagePNG*>(image.get()) != 0);
		return;
	}

	const std::string boundary = "----------------------------0123abcdefab";

	LLSD headers = LLViewerMedia::getHeaders();
	headers["Cookie"] = getAuthCookie();
	headers["Content-Type"] = "multipart/form-data; boundary=" + boundary;

	std::ostringstream body;

	// *NOTE: The order seems to matter.
	body	<< "--" << boundary << "\r\n"
			<< "Content-Disposition: form-data; name=\"key\"\r\n\r\n"
			<< config["key"].asString() << "\r\n";

	body	<< "--" << boundary << "\r\n"
			<< "Content-Disposition: form-data; name=\"AWSAccessKeyId\"\r\n\r\n"
			<< config["AWSAccessKeyId"].asString() << "\r\n";

	body	<< "--" << boundary << "\r\n"
			<< "Content-Disposition: form-data; name=\"acl\"\r\n\r\n"
			<< config["acl"].asString() << "\r\n";

	body	<< "--" << boundary << "\r\n"
			<< "Content-Disposition: form-data; name=\"Content-Type\"\r\n\r\n"
			<< config["Content-Type"].asString() << "\r\n";

	body	<< "--" << boundary << "\r\n"
			<< "Content-Disposition: form-data; name=\"policy\"\r\n\r\n"
			<< config["policy"].asString() << "\r\n";

	body	<< "--" << boundary << "\r\n"
			<< "Content-Disposition: form-data; name=\"signature\"\r\n\r\n"
			<< config["signature"].asString() << "\r\n";

	body	<< "--" << boundary << "\r\n"
			<< "Content-Disposition: form-data; name=\"success_action_redirect\"\r\n\r\n"
			<< config["success_action_redirect"].asString() << "\r\n";

	body	<< "--" << boundary << "\r\n"
			<< "Content-Disposition: form-data; name=\"file\"; filename=\"snapshot.png\"\r\n"
			<< "Content-Type: image/png\r\n\r\n";

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

	// Send request, successful upload will trigger posting metadata.
	LLHTTPClient::postRaw(url, data, size, new LLWebProfileResponders::PostImageResponder(), headers);
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
	const char* debug_cookie = getenv("LL_SNAPSHOT_COOKIE");
	return debug_cookie ? debug_cookie : sAuthCookie;
}
