/** 
 * @file llhttpclient.h
 * @brief Declaration of classes for making HTTP client requests.
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
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

#ifndef LL_LLHTTPCLIENT_H
#define LL_LLHTTPCLIENT_H

/**
 * These classes represent the HTTP client framework.
 */

#include <string>

#include <boost/intrusive_ptr.hpp>
#include <openssl/x509_vfy.h>
#include "llurlrequest.h"
#include "llassettype.h"
#include "llcurl.h"
#include "lliopipe.h"

extern const F32 HTTP_REQUEST_EXPIRY_SECS;

class LLUUID;
class LLPumpIO;
class LLSD;


class LLHTTPClient
{
public:
	// class Responder moved to LLCurl

	// For convenience
	typedef LLCurl::Responder Responder;
	typedef LLCurl::ResponderPtr ResponderPtr;

	
	/** @name non-blocking API */
	//@{
	static void head(
		const std::string& url,
		ResponderPtr,
		const LLSD& headers = LLSD(),
		const F32 timeout=HTTP_REQUEST_EXPIRY_SECS);
	static void getByteRange(const std::string& url, S32 offset, S32 bytes, ResponderPtr, const LLSD& headers=LLSD(), const F32 timeout=HTTP_REQUEST_EXPIRY_SECS);
	static void get(const std::string& url, ResponderPtr, const LLSD& headers = LLSD(), const F32 timeout=HTTP_REQUEST_EXPIRY_SECS);
	static void get(const std::string& url, const LLSD& query, ResponderPtr, const LLSD& headers = LLSD(), const F32 timeout=HTTP_REQUEST_EXPIRY_SECS);

	static void put(
		const std::string& url,
		const LLSD& body,
		ResponderPtr,
		const LLSD& headers = LLSD(),
		const F32 timeout=HTTP_REQUEST_EXPIRY_SECS);
	static void getHeaderOnly(const std::string& url, ResponderPtr, const F32 timeout=HTTP_REQUEST_EXPIRY_SECS);
	static void getHeaderOnly(const std::string& url, ResponderPtr, const LLSD& headers, const F32 timeout=HTTP_REQUEST_EXPIRY_SECS);

	static void post(
		const std::string& url,
		const LLSD& body,
		ResponderPtr,
		const LLSD& headers = LLSD(),
		const F32 timeout=HTTP_REQUEST_EXPIRY_SECS);
	/** Takes ownership of data and deletes it when sent */
	static void postRaw(
		const std::string& url,
		const U8* data,
		S32 size,
		ResponderPtr responder,
		const LLSD& headers = LLSD(),
		const F32 timeout=HTTP_REQUEST_EXPIRY_SECS);
	static void postFile(
		const std::string& url,
		const std::string& filename,
		ResponderPtr,
		const LLSD& headers = LLSD(),
		const F32 timeout=HTTP_REQUEST_EXPIRY_SECS);
	static void postFile(
		const std::string& url,
		const LLUUID& uuid,
		LLAssetType::EType asset_type,
		ResponderPtr responder,
		const LLSD& headers = LLSD(),
		const F32 timeout=HTTP_REQUEST_EXPIRY_SECS);

	static void del(
		const std::string& url,
		ResponderPtr responder,
		const LLSD& headers = LLSD(),
		const F32 timeout=HTTP_REQUEST_EXPIRY_SECS);
		///< sends a DELETE method, but we can't call it delete in c++
	
	/**
	 * @brief Send a MOVE webdav method
	 *
	 * @param url The complete serialized (and escaped) url to get.
	 * @param destination The complete serialized destination url.
	 * @param responder The responder that will handle the result.
	 * @param headers A map of key:value headers to pass to the request
	 * @param timeout The number of seconds to give the server to respond.
	 */
	static void move(
		const std::string& url,
		const std::string& destination,
		ResponderPtr responder,
		const LLSD& headers = LLSD(),
		const F32 timeout=HTTP_REQUEST_EXPIRY_SECS);

	//@}

	/**
	 * @brief Blocking HTTP get that returns an LLSD map of status and body.
	 *
	 * @param url the complete serialized (and escaped) url to get
	 * @return An LLSD of { 'status':status, 'body':payload }
	 */
	static LLSD blockingGet(const std::string& url);

	/**
	 * @brief Blocking HTTP POST that returns an LLSD map of status and body.
	 *
	 * @param url the complete serialized (and escaped) url to get
	 * @param body the LLSD post body
	 * @return An LLSD of { 'status':status (an int), 'body':payload (an LLSD) }
	 */
	static LLSD blockingPost(const std::string& url, const LLSD& body);

	
	static void setPump(LLPumpIO& pump);
		///< must be called before any of the above calls are made
	static bool hasPump();

	static void setCertVerifyCallback(LLURLRequest::SSLCertVerifyCallback callback);
	static  LLURLRequest::SSLCertVerifyCallback getCertVerifyCallback() { return mCertVerifyCallback; }

protected:
	static LLURLRequest::SSLCertVerifyCallback mCertVerifyCallback;
};

#endif // LL_LLHTTPCLIENT_H
