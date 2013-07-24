/** 
 * @file llwebsharing.h
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

#ifndef LL_LLWEBSHARING_H
#define LL_LLWEBSHARING_H

#include "llimagejpeg.h"
#include "llsingleton.h"



/**
 * @class LLWebSharing
 *
 * Manages authentication to, and interaction with, a web service allowing the
 * upload of snapshot images taken within the viewer, using OpenID and the
 * OpenSocial APIs.
 * http://www.opensocial.org/Technical-Resources/opensocial-spec-v09/RPC-Protocol.html
 */
class LLWebSharing : public LLSingleton<LLWebSharing>
{
	LOG_CLASS(LLWebSharing);
public:
	/*
	 * Performs initial setup, by requesting config data from the web service if
	 * it has not already been received.
	 */
	void init();

	/*
	 * @return true if both the OpenID cookie and config data have been received.
	 */
	bool enabled() const { return mEnabled; };

	/*
	 * Sets the OpenID cookie to use for login to the web service.
	 *
	 * @param cookie a string containing the OpenID cookie.
	 *
	 * @return true if both the OpenID cookie and config data have been received.
	 */
	bool setOpenIDCookie(const std::string& cookie);

	/*
	 * Receive config data used to connect to the web service.
	 *
	 * @param config an LLSD map of URL templates for the web service end-points.
	 *
	 * @return true if both the OpenID cookie and config data have been received.
	 *
	 * @see sendConfigRequest()
	 */
	bool receiveConfig(const LLSD& config);

	/*
	 * Receive the session cookie from the web service, which is the result of
	 * the OpenID login process.
	 *
	 * @see sendOpenIDAuthRequest()
	 */
	bool receiveSessionCookie(const std::string& cookie);

	/*
	 * Receive a security token for the upload service.
	 *
	 * @see sendSecurityTokenRequest()
	 */
	bool receiveSecurityToken(const std::string& token, const std::string& expires);

	/*
	 * Restarts the authentication process if the maximum number of retries has
	 * not been exceeded.
	 *
	 * @return true if retrying, false if LLWebSharing::MAX_AUTH_RETRIES has been exceeded.
	 */
	bool retryOpenIDAuth();

	/*
	 * Post a snapshot to the upload service.
	 *
	 * @return true if accepted for upload, false if already uploading another image.
	 */
	bool shareSnapshot(LLImageJPEG* snapshot, LLSD& metadata);

private:
	static const S32 MAX_AUTH_RETRIES = 4;

	friend class LLSingleton<LLWebSharing>;

	LLWebSharing();
	~LLWebSharing() {};

	/*
	 * Request a map of URLs and URL templates to the web service end-points.
	 *
	 * @see receiveConfig()
	 */
	void sendConfigRequest();

	/*
	 * Initiate the OpenID login process.
	 *
	 * @see receiveSessionCookie()
	 */
	void sendOpenIDAuthRequest();

	/*
	 * Request a security token for the upload service.
	 *
	 * @see receiveSecurityToken()
	 */
	void sendSecurityTokenRequest();

	/*
	 * Request a security token for the upload service.
	 *
	 * @see receiveSecurityToken()
	 */
	void sendUploadRequest();

	/*
	 * Checks all necessary config information has been received, and sets mEnabled.
	 *
	 * @return true if both the OpenID cookie and config data have been received.
	 */
	bool validateConfig();

	/*
	 * Checks the security token is present and has not expired.
	 *
	 * @param token an LLSD map containing the token string and the time it expires.
	 *
	 * @return true if the token is not empty and has not expired.
	 */
	static bool securityTokenIsValid(LLSD& token);

	std::string mOpenIDCookie;
	std::string mSessionCookie;
	LLSD mSecurityToken;

	LLSD mConfig;
	bool mEnabled;

	LLPointer<LLImageJPEG> mImage;
	LLSD mMetadata;

	S32 mRetries;
};

/**
 * @class LLUriTemplate
 *
 * @brief Builds complete URIs, given URI template and a map of keys and values
 *        to use for substition.
 *        Note: This is only a partial implementation of a draft standard required
 *        by the web API used by LLWebSharing.
 *        See: http://tools.ietf.org/html/draft-gregorio-uritemplate-03
 *
 * @see LLWebSharing
 */
class LLUriTemplate
{
	LOG_CLASS(LLUriTemplate);
public:
	LLUriTemplate(const std::string& uri_template);
	~LLUriTemplate() {};

	/*
	 * Builds a complete URI from the template.
	 *
	 * @param vars an LLSD map of keys and values for substitution.
	 *
	 * @return a string containing the complete URI.
	 */
	std::string buildURI(const LLSD& vars);

private:
	/*
	 * Builds a URL query string.
	 *
	 * @param delim    a string containing the separator to use between name=value pairs.
	 * @param var_list a string containing a comma separated list of variable names.
	 * @param vars     an LLSD map of keys and values for substitution.
	 *
	 * @return a URL query string.
	 */
	std::string expandJoin(const std::string& delim, const std::string& var_list, const LLSD& vars);

	/*
	 * URL escape the given string.
	 * LLWeb::escapeURL() only does a partial escape, so this uses curl_escape() instead.
	 */
	static std::string escapeURL(const std::string& unescaped);

	std::string mTemplate;
};



#endif // LL_LLWEBSHARING_H
