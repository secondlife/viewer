/** 
 * @file llwebsharing.h
 * @author Aimee
 * @brief Web Snapshot Sharing
 *
 * $LicenseInfo:firstyear=2010&license=viewergpl$
 * 
 * Copyright (c) 2010, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
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
