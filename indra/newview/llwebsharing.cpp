/** 
 * @file llwebsharing.cpp
 * @author Aimee
 * @brief Web Snapshot Sharing
 *
 * $LicenseInfo:firstyear=2010&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
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

#include "llwebsharing.h"

#include "llagentui.h"
#include "llbufferstream.h"
#include "llhttpclient.h"
#include "llhttpstatuscodes.h"
#include "llsdserialize.h"
#include "llsdutil.h"
#include "llurl.h"
#include "llviewercontrol.h"

#include <boost/regex.hpp>
#include <boost/algorithm/string/replace.hpp>



///////////////////////////////////////////////////////////////////////////////
//
class LLWebSharingConfigResponder : public LLHTTPClient::Responder
{
	LOG_CLASS(LLWebSharingConfigResponder);
public:
	/// Overrides the default LLSD parsing behaviour, to allow parsing a JSON response.
	virtual void completedRaw(U32 status, const std::string& reason,
							  const LLChannelDescriptors& channels,
							  const LLIOPipe::buffer_ptr_t& buffer)
	{
		LLSD content;
		LLBufferStream istr(channels, buffer.get());
		LLPointer<LLSDParser> parser = new LLSDNotationParser();

		if (parser->parse(istr, content, LLSDSerialize::SIZE_UNLIMITED) == LLSDParser::PARSE_FAILURE)
		{
			LL_WARNS("WebSharing") << "Failed to deserialize LLSD from JSON response. " << " [" << status << "]: " << reason << LL_ENDL;
		}
		else
		{
			completed(status, reason, content);
		}
	}

	virtual void errorWithContent(U32 status, const std::string& reason, const LLSD& content)
	{
		LL_WARNS("WebSharing") << "Error [status:" << status << "]: " << content << LL_ENDL;
	}

	virtual void result(const LLSD& content)
	{
		LLWebSharing::instance().receiveConfig(content);
	}
};



///////////////////////////////////////////////////////////////////////////////
//
class LLWebSharingOpenIDAuthResponder : public LLHTTPClient::Responder
{
	LOG_CLASS(LLWebSharingOpenIDAuthResponder);
public:
	/* virtual */ void completedHeader(U32 status, const std::string& reason, const LLSD& content)
	{
		completed(status, reason, content);
	}

	/* virtual */ void completedRaw(U32 status, const std::string& reason,
									const LLChannelDescriptors& channels,
									const LLIOPipe::buffer_ptr_t& buffer)
	{
		/// Left empty to override the default LLSD parsing behaviour.
	}

	virtual void errorWithContent(U32 status, const std::string& reason, const LLSD& content)
	{
		if (HTTP_UNAUTHORIZED == status)
		{
			LL_WARNS("WebSharing") << "AU account not authenticated." << LL_ENDL;
			// *TODO: No account found on AU, so start the account creation process here.
		}
		else
		{
			LL_WARNS("WebSharing") << "Error [status:" << status << "]: " << content << LL_ENDL;
			LLWebSharing::instance().retryOpenIDAuth();
		}

	}

	virtual void result(const LLSD& content)
	{
		if (content.has("set-cookie"))
		{
			// OpenID request succeeded and returned a session cookie.
			LLWebSharing::instance().receiveSessionCookie(content["set-cookie"].asString());
		}
	}
};



///////////////////////////////////////////////////////////////////////////////
//
class LLWebSharingSecurityTokenResponder : public LLHTTPClient::Responder
{
	LOG_CLASS(LLWebSharingSecurityTokenResponder);
public:
	/// Overrides the default LLSD parsing behaviour, to allow parsing a JSON response.
	virtual void completedRaw(U32 status, const std::string& reason,
							  const LLChannelDescriptors& channels,
							  const LLIOPipe::buffer_ptr_t& buffer)
	{
		LLSD content;
		LLBufferStream istr(channels, buffer.get());
		LLPointer<LLSDParser> parser = new LLSDNotationParser();

		if (parser->parse(istr, content, LLSDSerialize::SIZE_UNLIMITED) == LLSDParser::PARSE_FAILURE)
		{
			LL_WARNS("WebSharing") << "Failed to deserialize LLSD from JSON response. " << " [" << status << "]: " << reason << LL_ENDL;
			LLWebSharing::instance().retryOpenIDAuth();
		}
		else
		{
			completed(status, reason, content);
		}
	}

	virtual void errorWithContent(U32 status, const std::string& reason, const LLSD& content)
	{
		LL_WARNS("WebSharing") << "Error [status:" << status << "]: " << content << LL_ENDL;
		LLWebSharing::instance().retryOpenIDAuth();
	}

	virtual void result(const LLSD& content)
	{
		if (content[0].has("st") && content[0].has("expires"))
		{
			const std::string& token   = content[0]["st"].asString();
			const std::string& expires = content[0]["expires"].asString();
			if (LLWebSharing::instance().receiveSecurityToken(token, expires))
			{
				// Sucessfully received a valid security token.
				return;
			}
		}
		else
		{
			LL_WARNS("WebSharing") << "No security token received." << LL_ENDL;
		}

		LLWebSharing::instance().retryOpenIDAuth();
	}
};



///////////////////////////////////////////////////////////////////////////////
//
class LLWebSharingUploadResponder : public LLHTTPClient::Responder
{
	LOG_CLASS(LLWebSharingUploadResponder);
public:
	/// Overrides the default LLSD parsing behaviour, to allow parsing a JSON response.
	virtual void completedRaw(U32 status, const std::string& reason,
							  const LLChannelDescriptors& channels,
							  const LLIOPipe::buffer_ptr_t& buffer)
	{
/*
		 // Dump the body, for debugging.

		 LLBufferStream istr1(channels, buffer.get());
		 std::ostringstream ostr;
		 std::string body;

		 while (istr1.good())
		 {
			char buf[1024];
			istr1.read(buf, sizeof(buf));
			body.append(buf, istr1.gcount());
		 }
		 LL_DEBUGS("WebSharing") << body << LL_ENDL;
*/
		LLSD content;
		LLBufferStream istr(channels, buffer.get());
		LLPointer<LLSDParser> parser = new LLSDNotationParser();

		if (parser->parse(istr, content, LLSDSerialize::SIZE_UNLIMITED) == LLSDParser::PARSE_FAILURE)
		{
			LL_WARNS("WebSharing") << "Failed to deserialize LLSD from JSON response. " << " [" << status << "]: " << reason << LL_ENDL;
		}
		else
		{
			completed(status, reason, content);
		}
	}

	virtual void errorWithContent(U32 status, const std::string& reason, const LLSD& content)
	{
		LL_WARNS("WebSharing") << "Error [status:" << status << "]: " << content << LL_ENDL;
	}

	virtual void result(const LLSD& content)
	{
		if (content[0].has("result") && content[0].has("id") &&
			content[0]["id"].asString() == "newMediaItem")
		{
			// *TODO: Upload successful, continue from here to post metadata and create AU activity.
		}
		else
		{
			LL_WARNS("WebSharing") << "Error [" << content[0]["code"].asString()
								   << "]: " << content[0]["message"].asString() << LL_ENDL;
		}
	}
};



///////////////////////////////////////////////////////////////////////////////
//
LLWebSharing::LLWebSharing()
:	mConfig(),
	mSecurityToken(LLSD::emptyMap()),
	mEnabled(false),
	mRetries(0),
	mImage(NULL),
	mMetadata(LLSD::emptyMap())
{
}

void LLWebSharing::init()
{
	if (!mEnabled)
	{
		sendConfigRequest();
	}
}

bool LLWebSharing::shareSnapshot(LLImageJPEG* snapshot, LLSD& metadata)
{
	LL_INFOS("WebSharing") << metadata << LL_ENDL;

	if (mImage)
	{
		// *TODO: Handle this possibility properly, queue them up?
		LL_WARNS("WebSharing") << "Snapshot upload already in progress." << LL_ENDL;
		return false;
	}

	mImage = snapshot;
	mMetadata = metadata;

	// *TODO: Check whether we have a valid security token already and re-use it.
	sendOpenIDAuthRequest();
	return true;
}

bool LLWebSharing::setOpenIDCookie(const std::string& cookie)
{
	LL_DEBUGS("WebSharing") << "Setting OpenID cookie " << cookie << LL_ENDL;
	mOpenIDCookie = cookie;
	return validateConfig();
}

bool LLWebSharing::receiveConfig(const LLSD& config)
{
	LL_DEBUGS("WebSharing") << "Received config data: " << config << LL_ENDL;
	mConfig = config;
	return validateConfig();
}

bool LLWebSharing::receiveSessionCookie(const std::string& cookie)
{
	LL_DEBUGS("WebSharing") << "Received AU session cookie: " << cookie << LL_ENDL;
	mSessionCookie = cookie;

	// Fetch a security token using the new session cookie.
	LLWebSharing::instance().sendSecurityTokenRequest();

	return (!mSessionCookie.empty());
}

bool LLWebSharing::receiveSecurityToken(const std::string& token, const std::string& expires)
{
	mSecurityToken["st"] = token;
	mSecurityToken["expires"] = LLDate(expires);

	if (!securityTokenIsValid(mSecurityToken))
	{
		LL_WARNS("WebSharing") << "Invalid security token received: \"" << token << "\" Expires: " << expires << LL_ENDL;
		return false;
	}

	LL_DEBUGS("WebSharing") << "Received security token: \"" << token << "\" Expires: " << expires << LL_ENDL;
	mRetries = 0;

	// Continue the upload process now that we have a security token.
	sendUploadRequest();

	return true;
}

void LLWebSharing::sendConfigRequest()
{
	std::string config_url = gSavedSettings.getString("SnapshotConfigURL");
	LL_DEBUGS("WebSharing") << "Requesting Snapshot Sharing config data from: " << config_url << LL_ENDL;

	LLSD headers = LLSD::emptyMap();
	headers["Accept"] = "application/json";

	LLHTTPClient::get(config_url, new LLWebSharingConfigResponder(), headers);
}

void LLWebSharing::sendOpenIDAuthRequest()
{
	std::string auth_url = mConfig["openIdAuthUrl"];
	LL_DEBUGS("WebSharing") << "Starting OpenID Auth: " << auth_url << LL_ENDL;

	LLSD headers = LLSD::emptyMap();
	headers["Cookie"] = mOpenIDCookie;
	headers["Accept"] = "*/*";

	// Send request, successful login will trigger fetching a security token.
	LLHTTPClient::get(auth_url, new LLWebSharingOpenIDAuthResponder(), headers);
}

bool LLWebSharing::retryOpenIDAuth()
{
	if (mRetries++ >= MAX_AUTH_RETRIES)
	{
		LL_WARNS("WebSharing") << "Exceeded maximum number of authorization attempts, aborting." << LL_ENDL;
		mRetries = 0;
		return false;
	}

	LL_WARNS("WebSharing") << "Authorization failed, retrying (" << mRetries << "/" << MAX_AUTH_RETRIES << ")" << LL_ENDL;
	sendOpenIDAuthRequest();
	return true;
}

void LLWebSharing::sendSecurityTokenRequest()
{
	std::string token_url = mConfig["securityTokenUrl"];
	LL_DEBUGS("WebSharing") << "Fetching security token from: " << token_url << LL_ENDL;

	LLSD headers = LLSD::emptyMap();
	headers["Cookie"] = mSessionCookie;

	headers["Accept"] = "application/json";
	headers["Content-Type"] = "application/json";

	std::ostringstream body;
	body << "{ \"gadgets\": [{ \"url\":\""
		 << mConfig["gadgetSpecUrl"].asString()
		 << "\" }] }";

	// postRaw() takes ownership of the buffer and releases it later.
	size_t size = body.str().size();
	U8 *data = new U8[size];
	memcpy(data, body.str().data(), size);

	// Send request, receiving a valid token will trigger snapshot upload.
	LLHTTPClient::postRaw(token_url, data, size, new LLWebSharingSecurityTokenResponder(), headers);
}

void LLWebSharing::sendUploadRequest()
{
	LLUriTemplate upload_template(mConfig["openSocialRpcUrlTemplate"].asString());
	std::string upload_url(upload_template.buildURI(mSecurityToken));

	LL_DEBUGS("WebSharing") << "Posting upload to: " << upload_url << LL_ENDL;

	static const std::string BOUNDARY("------------abcdef012345xyZ");

	LLSD headers = LLSD::emptyMap();
	headers["Cookie"] = mSessionCookie;

	headers["Accept"] = "application/json";
	headers["Content-Type"] = "multipart/form-data; boundary=" + BOUNDARY;

	std::ostringstream body;
	body << "--" << BOUNDARY << "\r\n"
		 << "Content-Disposition: form-data; name=\"request\"\r\n\r\n"
		 << "[{"
		 <<	  "\"method\":\"mediaItems.create\","
		 <<	  "\"params\": {"
		 <<	    "\"userId\":[\"@me\"],"
		 <<	    "\"groupId\":\"@self\","
		 <<	    "\"mediaItem\": {"
		 <<	      "\"mimeType\":\"image/jpeg\","
		 <<	      "\"type\":\"image\","
		 <<       "\"url\":\"@field:image1\""
		 <<	    "}"
		 <<	  "},"
		 <<	  "\"id\":\"newMediaItem\""
		 <<	"}]"
		 <<	"--" << BOUNDARY << "\r\n"
		 <<	"Content-Disposition: form-data; name=\"image1\"\r\n\r\n";

	// Insert the image data.
	// *FIX: Treating this as a string will probably screw it up ...
	U8* image_data = mImage->getData();
	for (S32 i = 0; i < mImage->getDataSize(); ++i)
	{
		body << image_data[i];
	}

	body <<	"\r\n--" << BOUNDARY << "--\r\n";

	// postRaw() takes ownership of the buffer and releases it later.
	size_t size = body.str().size();
	U8 *data = new U8[size];
	memcpy(data, body.str().data(), size);

	// Send request, successful upload will trigger posting metadata.
	LLHTTPClient::postRaw(upload_url, data, size, new LLWebSharingUploadResponder(), headers);
}

bool LLWebSharing::validateConfig()
{
	// Check the OpenID Cookie has been set.
	if (mOpenIDCookie.empty())
	{
		mEnabled = false;
		return mEnabled;
	}

	if (!mConfig.isMap())
	{
		mEnabled = false;
		return mEnabled;
	}

	// Template to match the received config against.
	LLSD required(LLSD::emptyMap());
	required["gadgetSpecUrl"] = "";
	required["loginTokenUrl"] = "";
	required["openIdAuthUrl"] = "";
	required["photoPageUrlTemplate"] = "";
	required["openSocialRpcUrlTemplate"] = "";
	required["securityTokenUrl"] = "";
	required["tokenBasedLoginUrlTemplate"] = "";
	required["viewerIdUrl"] = "";

	std::string mismatch(llsd_matches(required, mConfig));
	if (!mismatch.empty())
	{
		LL_WARNS("WebSharing") << "Malformed config data response: " << mismatch << LL_ENDL;
		mEnabled = false;
		return mEnabled;
	}

	mEnabled = true;
	return mEnabled;
}

// static
bool LLWebSharing::securityTokenIsValid(LLSD& token)
{
	return (token.has("st") &&
			token.has("expires") &&
			(token["st"].asString() != "") &&
			(token["expires"].asDate() > LLDate::now()));
}



///////////////////////////////////////////////////////////////////////////////
//
LLUriTemplate::LLUriTemplate(const std::string& uri_template)
	:
	mTemplate(uri_template)
{
}

std::string LLUriTemplate::buildURI(const LLSD& vars)
{
	// *TODO: Separate parsing the template from building the URI.
	// Parsing only needs to happen on construction/assignnment.

	static const std::string VAR_NAME_REGEX("[[:alpha:]][[:alnum:]\\._-]*");
	// Capture var name with and without surrounding {}
	static const std::string VAR_REGEX("\\{(" + VAR_NAME_REGEX + ")\\}");
	// Capture delimiter and comma separated list of var names.
	static const std::string JOIN_REGEX("\\{-join\\|(&)\\|(" + VAR_NAME_REGEX + "(?:," + VAR_NAME_REGEX + ")*)\\}");

	std::string uri = mTemplate;
	boost::smatch results;

	// Validate and expand join operators : {-join|&|var1,var2,...}

	boost::regex join_regex(JOIN_REGEX);

	while (boost::regex_search(uri, results, join_regex))
	{
		// Extract the list of var names from the results.
		std::string delim = results[1].str();
		std::string var_list = results[2].str();

		// Expand the list of vars into a query string with their values
		std::string query = expandJoin(delim, var_list, vars);

		// Substitute the query string into the template.
		uri = boost::regex_replace(uri, join_regex, query, boost::format_first_only);
	}

	// Expand vars : {var1}

	boost::regex var_regex(VAR_REGEX);

	std::set<std::string> var_names;
	std::string::const_iterator start = uri.begin();
	std::string::const_iterator end = uri.end();

	// Extract the var names used.
	while (boost::regex_search(start, end, results, var_regex))
	{
		var_names.insert(results[1].str());
		start = results[0].second;
	}

	// Replace each var with its value.
	for (std::set<std::string>::const_iterator it = var_names.begin(); it != var_names.end(); ++it)
	{
		std::string var = *it;
		if (vars.has(var))
		{
			boost::replace_all(uri, "{" + var + "}", vars[var].asString());
		}
	}

	return uri;
}

std::string LLUriTemplate::expandJoin(const std::string& delim, const std::string& var_list, const LLSD& vars)
{
	std::ostringstream query;

	typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
	boost::char_separator<char> sep(",");
	tokenizer var_names(var_list, sep);
	tokenizer::const_iterator it = var_names.begin();

	// First var does not need a delimiter
	if (it != var_names.end())
	{
		const std::string& name = *it;
		if (vars.has(name))
		{
			// URL encode the value before appending the name=value pair.
			query << name << "=" << escapeURL(vars[name].asString());
		}
	}

	for (++it; it != var_names.end(); ++it)
	{
		const std::string& name = *it;
		if (vars.has(name))
		{
			// URL encode the value before appending the name=value pair.
			query << delim << name << "=" << escapeURL(vars[name].asString());
		}
	}

	return query.str();
}

// static
std::string LLUriTemplate::escapeURL(const std::string& unescaped)
{
	char* escaped = curl_escape(unescaped.c_str(), unescaped.size());
	std::string result = escaped;
	curl_free(escaped);
	return result;
}

