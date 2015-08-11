/** 
 * @file llhttpclient.cpp
 * @brief Implementation of classes for making HTTP requests.
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

#include "linden_common.h"
#include <openssl/x509_vfy.h>
#include "llhttpclient.h"

#include "llassetstorage.h"
#include "lliopipe.h"
#include "llurlrequest.h"
#include "llbufferstream.h"
#include "llsdserialize.h"
#include "llvfile.h"
#include "llvfs.h"
#include "lluri.h"

#include "message.h"
#include <curl/curl.h>


const F32 HTTP_REQUEST_EXPIRY_SECS = 60.0f;
LLURLRequest::SSLCertVerifyCallback LLHTTPClient::mCertVerifyCallback = NULL;

////////////////////////////////////////////////////////////////////////////

// Responder class moved to LLCurl

namespace
{
	class LLHTTPClientURLAdaptor : public LLURLRequestComplete
	{
	public:
		LLHTTPClientURLAdaptor(LLCurl::ResponderPtr responder)
			: LLURLRequestComplete(), mResponder(responder), mStatus(HTTP_INTERNAL_ERROR),
			  mReason("LLURLRequest complete w/no status")
		{
		}
		
		~LLHTTPClientURLAdaptor()
		{
		}

		virtual void httpStatus(S32 status, const std::string& reason)
		{
			LLURLRequestComplete::httpStatus(status,reason);

			mStatus = status;
			mReason = reason;
		}

		virtual void complete(const LLChannelDescriptors& channels,
							  const buffer_ptr_t& buffer)
		{
			// *TODO: Re-interpret mRequestStatus codes?
			//        Would like to detect curl errors, such as
			//        connection errors, write erros, etc.
			if (mResponder.get())
			{
				mResponder->setResult(mStatus, mReason);
				mResponder->completedRaw(channels, buffer);
			}
		}
		virtual void header(const std::string& header, const std::string& value)
		{
			if (mResponder.get())
			{
				mResponder->setResponseHeader(header, value);
			}
		}

	private:
		LLCurl::ResponderPtr mResponder;
		S32 mStatus;
		std::string mReason;
	};
	
	class Injector : public LLIOPipe
	{
	public:
		virtual const std::string& contentType() = 0;
	};

	class LLSDInjector : public Injector
	{
	public:
		LLSDInjector(const LLSD& sd) : mSD(sd) {}
		virtual ~LLSDInjector() {}

		const std::string& contentType() { return HTTP_CONTENT_LLSD_XML; }

		virtual EStatus process_impl(const LLChannelDescriptors& channels,
			buffer_ptr_t& buffer, bool& eos, LLSD& context, LLPumpIO* pump)
		{
			LLBufferStream ostream(channels, buffer.get());
			LLSDSerialize::toXML(mSD, ostream);
			eos = true;
			return STATUS_DONE;
		}

		const LLSD mSD;
	};

	class RawInjector : public Injector
	{
	public:
		RawInjector(const U8* data, S32 size) : mData(data), mSize(size) {}
		virtual ~RawInjector() {delete [] mData;}

		const std::string& contentType() { return HTTP_CONTENT_OCTET_STREAM; }

		virtual EStatus process_impl(const LLChannelDescriptors& channels,
			buffer_ptr_t& buffer, bool& eos, LLSD& context, LLPumpIO* pump)
		{
			LLBufferStream ostream(channels, buffer.get());
			ostream.write((const char *)mData, mSize);  // hopefully chars are always U8s
			eos = true;
			return STATUS_DONE;
		}

		const U8* mData;
		S32 mSize;
	};
	
	class FileInjector : public Injector
	{
	public:
		FileInjector(const std::string& filename) : mFilename(filename) {}
		virtual ~FileInjector() {}

		const std::string& contentType() { return HTTP_CONTENT_OCTET_STREAM; }

		virtual EStatus process_impl(const LLChannelDescriptors& channels,
			buffer_ptr_t& buffer, bool& eos, LLSD& context, LLPumpIO* pump)
		{
			LLBufferStream ostream(channels, buffer.get());

			llifstream fstream(mFilename.c_str(), std::iostream::binary | std::iostream::out);
			if(fstream.is_open())
			{
				fstream.seekg(0, std::ios::end);
				U32 fileSize = (U32)fstream.tellg();
				fstream.seekg(0, std::ios::beg);
				std::vector<char> fileBuffer(fileSize);
				fstream.read(&fileBuffer[0], fileSize);
				ostream.write(&fileBuffer[0], fileSize);
				fstream.close();
				eos = true;
				return STATUS_DONE;
			}
			
			return STATUS_ERROR;
		}

		const std::string mFilename;
	};
	
	class VFileInjector : public Injector
	{
	public:
		VFileInjector(const LLUUID& uuid, LLAssetType::EType asset_type) : mUUID(uuid), mAssetType(asset_type) {}
		virtual ~VFileInjector() {}

		const std::string& contentType() { return HTTP_CONTENT_OCTET_STREAM; }

		virtual EStatus process_impl(const LLChannelDescriptors& channels,
			buffer_ptr_t& buffer, bool& eos, LLSD& context, LLPumpIO* pump)
		{
			LLBufferStream ostream(channels, buffer.get());
			
			LLVFile vfile(gVFS, mUUID, mAssetType, LLVFile::READ);
			S32 fileSize = vfile.getSize();
			U8* fileBuffer;
			fileBuffer = new U8 [fileSize];
            vfile.read(fileBuffer, fileSize);
            ostream.write((char*)fileBuffer, fileSize);
			delete [] fileBuffer;
			eos = true;
			return STATUS_DONE;
		}

		const LLUUID mUUID;
		LLAssetType::EType mAssetType;
	};

	
	LLPumpIO* theClientPump = NULL;
}

void LLHTTPClient::setCertVerifyCallback(LLURLRequest::SSLCertVerifyCallback callback)
{
	LLHTTPClient::mCertVerifyCallback = callback;
}

static void request(
	const std::string& url,
	EHTTPMethod method,
	Injector* body_injector,
	LLCurl::ResponderPtr responder,
	const F32 timeout = HTTP_REQUEST_EXPIRY_SECS,
	const LLSD& headers = LLSD(),
	bool follow_redirects = true
    )
{
	if (!LLHTTPClient::hasPump())
	{
		if (responder)
		{
			responder->completeResult(HTTP_INTERNAL_ERROR, "No pump");
		}
		delete body_injector;
		return;
	}
	LLPumpIO::chain_t chain;

	LLURLRequest* req = new LLURLRequest(method, url, follow_redirects);
	if(!req->isValid())//failed
	{
		if (responder)
		{
			responder->completeResult(HTTP_INTERNAL_CURL_ERROR, "Internal Error - curl failure");
		}
		delete req;
		delete body_injector;
		return;
	}

	req->setSSLVerifyCallback(LLHTTPClient::getCertVerifyCallback(), (void *)req);

	LL_DEBUGS("LLHTTPClient") << httpMethodAsVerb(method) << " " << url << " " << headers << LL_ENDL;

	// Insert custom headers if the caller sent any
	if (headers.isMap())
	{
		if (headers.has(HTTP_OUT_HEADER_COOKIE))
		{
			req->allowCookies();
		}

		LLSD::map_const_iterator iter = headers.beginMap();
		LLSD::map_const_iterator end  = headers.endMap();

		for (; iter != end; ++iter)
		{
			//if the header is "Pragma" with no value
			//the caller intends to force libcurl to drop
			//the Pragma header it so gratuitously inserts
			//Before inserting the header, force libcurl
			//to not use the proxy (read: llurlrequest.cpp)
			if ((iter->first == HTTP_OUT_HEADER_PRAGMA) && (iter->second.asString().empty()))
			{
				req->useProxy(false);
			}
			LL_DEBUGS("LLHTTPClient") << "header = " << iter->first 
				<< ": " << iter->second.asString() << LL_ENDL;
			req->addHeader(iter->first, iter->second.asString());
		}
	}

	// Check to see if we have already set Accept or not. If no one
	// set it, set it to application/llsd+xml since that's what we
	// almost always want.
	if( method != HTTP_PUT && method != HTTP_POST )
	{
		if(!headers.has(HTTP_OUT_HEADER_ACCEPT))
		{
			req->addHeader(HTTP_OUT_HEADER_ACCEPT, HTTP_CONTENT_LLSD_XML);
		}
	}

	if (responder)
	{
		responder->setURL(url);
		responder->setHTTPMethod(method);
	}

	req->setCallback(new LLHTTPClientURLAdaptor(responder));

	if (method == HTTP_POST  &&  gMessageSystem)
	{
		req->addHeader("X-SecondLife-UDP-Listen-Port", llformat("%d",
					gMessageSystem->mPort));
	}

	if (method == HTTP_PUT || method == HTTP_POST || method == HTTP_PATCH)
	{
		if(!headers.has(HTTP_OUT_HEADER_CONTENT_TYPE))
		{
			// If the Content-Type header was passed in, it has
			// already been added as a header through req->addHeader
			// in the loop above. We defer to the caller's wisdom, but
			// if they did not specify a Content-Type, then ask the
			// injector.
			req->addHeader(HTTP_OUT_HEADER_CONTENT_TYPE, body_injector->contentType());
		}
		chain.push_back(LLIOPipe::ptr_t(body_injector));
	}

	chain.push_back(LLIOPipe::ptr_t(req));

	theClientPump->addChain(chain, timeout);
}


void LLHTTPClient::getByteRange(
	const std::string& url,
	S32 offset,
	S32 bytes,
	ResponderPtr responder,
	const LLSD& hdrs,
	const F32 timeout,
	bool follow_redirects /* = true */)
{
	LLSD headers = hdrs;
	if(offset > 0 || bytes > 0)
	{
		std::string range = llformat("bytes=%d-%d", offset, offset+bytes-1);
		headers[HTTP_OUT_HEADER_RANGE] = range;
	}
    request(url,HTTP_GET, NULL, responder, timeout, headers, follow_redirects);
}

void LLHTTPClient::head(
	const std::string& url,
	ResponderPtr responder,
	const LLSD& headers,
	const F32 timeout,
	bool follow_redirects /* = true */)
{
	request(url, HTTP_HEAD, NULL, responder, timeout, headers, follow_redirects);
}

void LLHTTPClient::get(const std::string& url, ResponderPtr responder, const LLSD& headers, const F32 timeout,
					   bool follow_redirects /* = true */)
{
	request(url, HTTP_GET, NULL, responder, timeout, headers, follow_redirects);
}
void LLHTTPClient::getHeaderOnly(const std::string& url, ResponderPtr responder, const LLSD& headers,
								 const F32 timeout, bool follow_redirects /* = true */)
{
	request(url, HTTP_HEAD, NULL, responder, timeout, headers, follow_redirects);
}
void LLHTTPClient::getHeaderOnly(const std::string& url, ResponderPtr responder, const F32 timeout,
								 bool follow_redirects /* = true */)
{
	getHeaderOnly(url, responder, LLSD(), timeout, follow_redirects);
}

void LLHTTPClient::get(const std::string& url, const LLSD& query, ResponderPtr responder, const LLSD& headers,
					   const F32 timeout, bool follow_redirects /* = true */)
{
	LLURI uri;
	
	uri = LLURI::buildHTTP(url, LLSD::emptyArray(), query);
	get(uri.asString(), responder, headers, timeout, follow_redirects);
}

// A simple class for managing data returned from a curl http request.
class LLHTTPBuffer
{
public:
	LLHTTPBuffer() { }

	static size_t curl_write( void *ptr, size_t size, size_t nmemb, void *user_data)
	{
		LLHTTPBuffer* self = (LLHTTPBuffer*)user_data;
		
		size_t bytes = (size * nmemb);
		self->mBuffer.append((char*)ptr,bytes);
		return nmemb;
	}

	LLSD asLLSD()
	{
		LLSD content;

		if (mBuffer.empty()) return content;
		
		std::istringstream istr(mBuffer);
		LLSDSerialize::fromXML(content, istr);
		return content;
	}

	const std::string& asString()
	{
		return mBuffer;
	}

private:
	std::string mBuffer;
};

// These calls are blocking! This is usually bad, unless you're a dataserver. Then it's awesome.

/**
	@brief does a blocking request on the url, returning the data or bad status.

	@param url URI to verb on.
	@param method the verb to hit the URI with.
	@param body the body of the call (if needed - for instance not used for GET and DELETE, but is for POST and PUT)
	@param headers HTTP headers to use for the request.
	@param timeout Curl timeout to use. Defaults to 5. Rationale:
	Without this timeout, blockingGet() calls have been observed to take
	up to 90 seconds to complete.  Users of blockingGet() already must 
	check the HTTP return code for validity, so this will not introduce
	new errors.  A 5 second timeout will succeed > 95% of the time (and 
	probably > 99% of the time) based on my statistics. JC

	@returns an LLSD map: {status: integer, body: map}
  */
static LLSD blocking_request(
	const std::string& url,
	EHTTPMethod method,
	const LLSD& body,
	const LLSD& headers = LLSD(),
	const F32 timeout = 5
)
{
	LL_DEBUGS() << "blockingRequest of " << url << LL_ENDL;
	char curl_error_buffer[CURL_ERROR_SIZE] = "\0";
	CURL* curlp = LLCurl::newEasyHandle();
	llassert_always(curlp != NULL) ;

	LLHTTPBuffer http_buffer;
	std::string body_str;
	
	// other request method checks root cert first, we skip?

	// Apply configured proxy settings
	LLProxy::getInstance()->applyProxySettings(curlp);
	
	// * Set curl handle options
	curl_easy_setopt(curlp, CURLOPT_NOSIGNAL, 1);	// don't use SIGALRM for timeouts
	curl_easy_setopt(curlp, CURLOPT_TIMEOUT, timeout);	// seconds, see warning at top of function.
	curl_easy_setopt(curlp, CURLOPT_WRITEFUNCTION, LLHTTPBuffer::curl_write);
	curl_easy_setopt(curlp, CURLOPT_WRITEDATA, &http_buffer);
	curl_easy_setopt(curlp, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curlp, CURLOPT_ERRORBUFFER, curl_error_buffer);

	// * Setup headers (don't forget to free them after the call!)
	curl_slist* headers_list = NULL;
	if (headers.isMap())
	{
		LLSD::map_const_iterator iter = headers.beginMap();
		LLSD::map_const_iterator end  = headers.endMap();
		for (; iter != end; ++iter)
		{
			std::ostringstream header;
			header << iter->first << ": " << iter->second.asString() ;
			LL_DEBUGS() << "header = " << header.str() << LL_ENDL;
			headers_list = curl_slist_append(headers_list, header.str().c_str());
		}
	}
	
	// * Setup specific method / "verb" for the URI (currently only GET and POST supported + poppy)
	if (method == HTTP_GET)
	{
		curl_easy_setopt(curlp, CURLOPT_HTTPGET, 1);
	}
	else if (method == HTTP_POST)
	{
		curl_easy_setopt(curlp, CURLOPT_POST, 1);
		//serialize to ostr then copy to str - need to because ostr ptr is unstable :(
		std::ostringstream ostr;
		LLSDSerialize::toXML(body, ostr);
		body_str = ostr.str();
		curl_easy_setopt(curlp, CURLOPT_POSTFIELDS, body_str.c_str());
		//copied from PHP libs, correct?
		headers_list = curl_slist_append(headers_list, 
				llformat("%s: %s", HTTP_OUT_HEADER_CONTENT_TYPE.c_str(), HTTP_CONTENT_LLSD_XML.c_str()).c_str());

		// copied from llurlrequest.cpp
		// it appears that apache2.2.3 or django in etch is busted. If
		// we do not clear the expect header, we get a 500. May be
		// limited to django/mod_wsgi.
		headers_list = curl_slist_append(headers_list, llformat("%s:", HTTP_OUT_HEADER_EXPECT.c_str()).c_str());
	}
	
	// * Do the action using curl, handle results
	LL_DEBUGS() << "HTTP body: " << body_str << LL_ENDL;
	headers_list = curl_slist_append(headers_list,
				llformat("%s: %s", HTTP_OUT_HEADER_ACCEPT.c_str(), HTTP_CONTENT_LLSD_XML.c_str()).c_str());
	CURLcode curl_result = curl_easy_setopt(curlp, CURLOPT_HTTPHEADER, headers_list);
	if ( curl_result != CURLE_OK )
	{
		LL_INFOS() << "Curl is hosed - can't add headers" << LL_ENDL;
	}

	LLSD response = LLSD::emptyMap();
	S32 curl_success = curl_easy_perform(curlp);
	S32 http_status = HTTP_INTERNAL_ERROR;
	curl_easy_getinfo(curlp, CURLINFO_RESPONSE_CODE, &http_status);
	response["status"] = http_status;
	// if we get a non-404 and it's not a 200 OR maybe it is but you have error bits,
	if ( http_status != HTTP_NOT_FOUND && (http_status != HTTP_OK || curl_success != 0) )
	{
		// We expect 404s, don't spam for them.
		LL_WARNS() << "CURL REQ URL: " << url << LL_ENDL;
		LL_WARNS() << "CURL REQ METHOD TYPE: " << method << LL_ENDL;
		LL_WARNS() << "CURL REQ HEADERS: " << headers.asString() << LL_ENDL;
		LL_WARNS() << "CURL REQ BODY: " << body_str << LL_ENDL;
		LL_WARNS() << "CURL HTTP_STATUS: " << http_status << LL_ENDL;
		LL_WARNS() << "CURL ERROR: " << curl_error_buffer << LL_ENDL;
		LL_WARNS() << "CURL ERROR BODY: " << http_buffer.asString() << LL_ENDL;
		response["body"] = http_buffer.asString();
	}
	else
	{
		response["body"] = http_buffer.asLLSD();
		LL_DEBUGS() << "CURL response: " << http_buffer.asString() << LL_ENDL;
	}
	
	if(headers_list)
	{	// free the header list  
		curl_slist_free_all(headers_list); 
	}

	// * Cleanup
	LLCurl::deleteEasyHandle(curlp);
	return response;
}

LLSD LLHTTPClient::blockingGet(const std::string& url)
{
	return blocking_request(url, HTTP_GET, LLSD());
}

LLSD LLHTTPClient::blockingPost(const std::string& url, const LLSD& body)
{
	return blocking_request(url, HTTP_POST, body);
}

void LLHTTPClient::put(
	const std::string& url,
	const LLSD& body,
	ResponderPtr responder,
	const LLSD& headers,
	const F32 timeout)
{
	request(url, HTTP_PUT, new LLSDInjector(body), responder, timeout, headers);
}

void LLHTTPClient::patch(
	const std::string& url,
	const LLSD& body,
	ResponderPtr responder,
	const LLSD& headers,
	const F32 timeout)
{
	request(url, HTTP_PATCH, new LLSDInjector(body), responder, timeout, headers);
}

void LLHTTPClient::putRaw(
    const std::string& url,
    const U8* data,
    S32 size,
    ResponderPtr responder,
    const LLSD& headers,
    const F32 timeout)
{
	request(url, HTTP_PUT, new RawInjector(data, size), responder, timeout, headers);
}

void LLHTTPClient::post(
	const std::string& url,
	const LLSD& body,
	ResponderPtr responder,
	const LLSD& headers,
	const F32 timeout)
{
	request(url, HTTP_POST, new LLSDInjector(body), responder, timeout, headers);
}

void LLHTTPClient::postRaw(
	const std::string& url,
	const U8* data,
	S32 size,
	ResponderPtr responder,
	const LLSD& headers,
	const F32 timeout)
{
	request(url, HTTP_POST, new RawInjector(data, size), responder, timeout, headers);
}

void LLHTTPClient::postFile(
	const std::string& url,
	const std::string& filename,
	ResponderPtr responder,
	const LLSD& headers,
	const F32 timeout)
{
	request(url, HTTP_POST, new FileInjector(filename), responder, timeout, headers);
}

void LLHTTPClient::postFile(
	const std::string& url,
	const LLUUID& uuid,
	LLAssetType::EType asset_type,
	ResponderPtr responder,
	const LLSD& headers,
	const F32 timeout)
{
	request(url, HTTP_POST, new VFileInjector(uuid, asset_type), responder, timeout, headers);
}

// static
void LLHTTPClient::del(
	const std::string& url,
	ResponderPtr responder,
	const LLSD& headers,
	const F32 timeout)
{
	request(url, HTTP_DELETE, NULL, responder, timeout, headers);
}

// static
void LLHTTPClient::move(
	const std::string& url,
	const std::string& destination,
	ResponderPtr responder,
	const LLSD& hdrs,
	const F32 timeout)
{
	LLSD headers = hdrs;
	headers[HTTP_OUT_HEADER_DESTINATION] = destination;
	request(url, HTTP_MOVE, NULL, responder, timeout, headers);
}

// static
void LLHTTPClient::copy(
	const std::string& url,
	const std::string& destination,
	ResponderPtr responder,
	const LLSD& hdrs,
	const F32 timeout)
{
	LLSD headers = hdrs;
	headers[HTTP_OUT_HEADER_DESTINATION] = destination;
	request(url, HTTP_COPY, NULL, responder, timeout, headers);
}


void LLHTTPClient::setPump(LLPumpIO& pump)
{
	theClientPump = &pump;
}

bool LLHTTPClient::hasPump()
{
	return theClientPump != NULL;
}
