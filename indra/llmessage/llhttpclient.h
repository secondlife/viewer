/** 
 * @file llhttpclient.h
 * @brief Declaration of classes for making HTTP client requests.
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2009, Linden Research, Inc.
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
