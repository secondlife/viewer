/** 
 * @file llhttpclient.h
 * @brief Declaration of classes for making HTTP client requests.
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

	// non-blocking
	static void head(const std::string& url, ResponderPtr, const F32 timeout=HTTP_REQUEST_EXPIRY_SECS);
	static void getByteRange(const std::string& url, S32 offset, S32 bytes, ResponderPtr, const LLSD& headers=LLSD(), const F32 timeout=HTTP_REQUEST_EXPIRY_SECS);
	static void get(const std::string& url, ResponderPtr, const LLSD& headers = LLSD(), const F32 timeout=HTTP_REQUEST_EXPIRY_SECS);
	static void get(const std::string& url, const LLSD& query, ResponderPtr, const LLSD& headers = LLSD(), const F32 timeout=HTTP_REQUEST_EXPIRY_SECS);

	static void put(const std::string& url, const LLSD& body, ResponderPtr, const F32 timeout=HTTP_REQUEST_EXPIRY_SECS);
	static void getHeaderOnly(const std::string& url, ResponderPtr, const F32 timeout=HTTP_REQUEST_EXPIRY_SECS);
	static void getHeaderOnly(const std::string& url, ResponderPtr, const LLSD& headers, const F32 timeout=HTTP_REQUEST_EXPIRY_SECS);

	///< non-blocking
	static void post(const std::string& url, const LLSD& body, ResponderPtr, const F32 timeout=HTTP_REQUEST_EXPIRY_SECS);
	static void post(const std::string& url, const U8* data, S32 size, ResponderPtr responder, const F32 timeout=HTTP_REQUEST_EXPIRY_SECS);
	static void postFile(const std::string& url, const std::string& filename, ResponderPtr, const F32 timeout=HTTP_REQUEST_EXPIRY_SECS);
	static void postFile(const std::string& url, const LLUUID& uuid,
		LLAssetType::EType asset_type, ResponderPtr responder, const F32 timeout=HTTP_REQUEST_EXPIRY_SECS);

	// Blocking HTTP get that returns an LLSD map of status and body.
	static LLSD blockingGet(const std::string& url);

	static void del(const std::string& url, ResponderPtr, const F32 timeout=HTTP_REQUEST_EXPIRY_SECS);
		///< sends a DELETE method, but we can't call it delete in c++
	
	
	static void setPump(LLPumpIO& pump);
		///< must be called before any of the above calls are made
	static bool hasPump();
		///< for testing
};

#endif // LL_LLHTTPCLIENT_H
