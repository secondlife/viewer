/** 
 * @file llhttpclient.cpp
 * @brief Implementation of classes for making HTTP requests.
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
			: LLURLRequestComplete(), mResponder(responder), mStatus(499),
			  mReason("LLURLRequest complete w/no status")
		{
		}
		
		~LLHTTPClientURLAdaptor()
		{
		}

		virtual void httpStatus(U32 status, const std::string& reason)
		{
			LLURLRequestComplete::httpStatus(status,reason);

			mStatus = status;
			mReason = reason;
		}

		virtual void complete(const LLChannelDescriptors& channels,
							  const buffer_ptr_t& buffer)
		{
			if (mResponder.get())
			{
				// Allow clients to parse headers before we attempt to parse
				// the body and provide completed/result/error calls.
				mResponder->completedHeader(mStatus, mReason, mHeaderOutput);
				mResponder->completedRaw(mStatus, mReason, channels, buffer);
			}
		}
		virtual void header(const std::string& header, const std::string& value)
		{
			mHeaderOutput[header] = value;
		}

	private:
		LLCurl::ResponderPtr mResponder;
		U32 mStatus;
		std::string mReason;
		LLSD mHeaderOutput;
	};
	
	class Injector : public LLIOPipe
	{
	public:
		virtual const char* contentType() = 0;
	};

	class LLSDInjector : public Injector
	{
	public:
		LLSDInjector(const LLSD& sd) : mSD(sd) {}
		virtual ~LLSDInjector() {}

		const char* contentType() { return "application/llsd+xml"; }

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
		virtual ~RawInjector() {delete mData;}

		const char* contentType() { return "application/octet-stream"; }

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

		const char* contentType() { return "application/octet-stream"; }

		virtual EStatus process_impl(const LLChannelDescriptors& channels,
			buffer_ptr_t& buffer, bool& eos, LLSD& context, LLPumpIO* pump)
		{
			LLBufferStream ostream(channels, buffer.get());

			llifstream fstream(mFilename, std::iostream::binary | std::iostream::out);
			if(fstream.is_open())
			{
				fstream.seekg(0, std::ios::end);
				U32 fileSize = fstream.tellg();
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

		const char* contentType() { return "application/octet-stream"; }

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
	LLURLRequest::ERequestAction method,
	Injector* body_injector,
	LLCurl::ResponderPtr responder,
	const F32 timeout = HTTP_REQUEST_EXPIRY_SECS,
	const LLSD& headers = LLSD()
    )
{
	if (!LLHTTPClient::hasPump())
	{
		responder->completed(U32_MAX, "No pump", LLSD());
		return;
	}
	LLPumpIO::chain_t chain;

	LLURLRequest* req = new LLURLRequest(method, url);
	req->setSSLVerifyCallback(LLHTTPClient::getCertVerifyCallback(), (void *)req);

	
	lldebugs << LLURLRequest::actionAsVerb(method) << " " << url << " "
		<< headers << llendl;

    // Insert custom headers is the caller sent any
    if (headers.isMap())
    {
        LLSD::map_const_iterator iter = headers.beginMap();
        LLSD::map_const_iterator end  = headers.endMap();

        for (; iter != end; ++iter)
        {
            std::ostringstream header;
            //if the header is "Pragma" with no value
            //the caller intends to force libcurl to drop
            //the Pragma header it so gratuitously inserts
            //Before inserting the header, force libcurl
            //to not use the proxy (read: llurlrequest.cpp)
			static const std::string PRAGMA("Pragma");
			if ((iter->first == PRAGMA) && (iter->second.asString().empty()))
            {
                req->useProxy(false);
            }
            header << iter->first << ": " << iter->second.asString() ;
            lldebugs << "header = " << header.str() << llendl;
            req->addHeader(header.str().c_str());
        }
    }

	// Check to see if we have already set Accept or not. If no one
	// set it, set it to application/llsd+xml since that's what we
	// almost always want.
	if( method != LLURLRequest::HTTP_PUT && method != LLURLRequest::HTTP_POST )
	{
		static const std::string ACCEPT("Accept");
		if(!headers.has(ACCEPT))
		{
			req->addHeader("Accept: application/llsd+xml");
		}
	}

	if (responder)
	{
		responder->setURL(url);
	}

	req->setCallback(new LLHTTPClientURLAdaptor(responder));

	if (method == LLURLRequest::HTTP_POST  &&  gMessageSystem)
	{
		req->addHeader(llformat("X-SecondLife-UDP-Listen-Port: %d",
								gMessageSystem->mPort).c_str());
   	}

	if (method == LLURLRequest::HTTP_PUT || method == LLURLRequest::HTTP_POST)
	{
		static const std::string CONTENT_TYPE("Content-Type");
		if(!headers.has(CONTENT_TYPE))
		{
			// If the Content-Type header was passed in, it has
			// already been added as a header through req->addHeader
			// in the loop above. We defer to the caller's wisdom, but
			// if they did not specify a Content-Type, then ask the
			// injector.
			req->addHeader(
				llformat(
					"Content-Type: %s",
					body_injector->contentType()).c_str());
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
	const F32 timeout)
{
	LLSD headers = hdrs;
	if(offset > 0 || bytes > 0)
	{
		std::string range = llformat("bytes=%d-%d", offset, offset+bytes-1);
		headers["Range"] = range;
	}
    request(url,LLURLRequest::HTTP_GET, NULL, responder, timeout, headers);
}

void LLHTTPClient::head(
	const std::string& url,
	ResponderPtr responder,
	const LLSD& headers,
	const F32 timeout)
{
	request(url, LLURLRequest::HTTP_HEAD, NULL, responder, timeout, headers);
}

void LLHTTPClient::get(const std::string& url, ResponderPtr responder, const LLSD& headers, const F32 timeout)
{
	request(url, LLURLRequest::HTTP_GET, NULL, responder, timeout, headers);
}
void LLHTTPClient::getHeaderOnly(const std::string& url, ResponderPtr responder, const LLSD& headers, const F32 timeout)
{
	request(url, LLURLRequest::HTTP_HEAD, NULL, responder, timeout, headers);
}
void LLHTTPClient::getHeaderOnly(const std::string& url, ResponderPtr responder, const F32 timeout)
{
	getHeaderOnly(url, responder, LLSD(), timeout);
}

void LLHTTPClient::get(const std::string& url, const LLSD& query, ResponderPtr responder, const LLSD& headers, const F32 timeout)
{
	LLURI uri;
	
	uri = LLURI::buildHTTP(url, LLSD::emptyArray(), query);
	get(uri.asString(), responder, headers, timeout);
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

	std::string asString()
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
	LLURLRequest::ERequestAction method,
	const LLSD& body,
	const LLSD& headers = LLSD(),
	const F32 timeout = 5
)
{
	lldebugs << "blockingRequest of " << url << llendl;
	char curl_error_buffer[CURL_ERROR_SIZE] = "\0";
	CURL* curlp = curl_easy_init();
	LLHTTPBuffer http_buffer;
	std::string body_str;
	
	// other request method checks root cert first, we skip?
	
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
			lldebugs << "header = " << header.str() << llendl;
			headers_list = curl_slist_append(headers_list, header.str().c_str());
		}
	}
	
	// * Setup specific method / "verb" for the URI (currently only GET and POST supported + poppy)
	if (method == LLURLRequest::HTTP_GET)
	{
		curl_easy_setopt(curlp, CURLOPT_HTTPGET, 1);
	}
	else if (method == LLURLRequest::HTTP_POST)
	{
		curl_easy_setopt(curlp, CURLOPT_POST, 1);
		//serialize to ostr then copy to str - need to because ostr ptr is unstable :(
		std::ostringstream ostr;
		LLSDSerialize::toXML(body, ostr);
		body_str = ostr.str();
		curl_easy_setopt(curlp, CURLOPT_POSTFIELDS, body_str.c_str());
		//copied from PHP libs, correct?
		headers_list = curl_slist_append(headers_list, "Content-Type: application/llsd+xml");

		// copied from llurlrequest.cpp
		// it appears that apache2.2.3 or django in etch is busted. If
		// we do not clear the expect header, we get a 500. May be
		// limited to django/mod_wsgi.
		headers_list = curl_slist_append(headers_list, "Expect:");
	}
	
	// * Do the action using curl, handle results
	lldebugs << "HTTP body: " << body_str << llendl;
	headers_list = curl_slist_append(headers_list, "Accept: application/llsd+xml");
	CURLcode curl_result = curl_easy_setopt(curlp, CURLOPT_HTTPHEADER, headers_list);
	if ( curl_result != CURLE_OK )
	{
		llinfos << "Curl is hosed - can't add headers" << llendl;
	}

	LLSD response = LLSD::emptyMap();
	S32 curl_success = curl_easy_perform(curlp);
	S32 http_status = 499;
	curl_easy_getinfo(curlp, CURLINFO_RESPONSE_CODE, &http_status);
	response["status"] = http_status;
	// if we get a non-404 and it's not a 200 OR maybe it is but you have error bits,
	if ( http_status != 404 && (http_status != 200 || curl_success != 0) )
	{
		// We expect 404s, don't spam for them.
		llwarns << "CURL REQ URL: " << url << llendl;
		llwarns << "CURL REQ METHOD TYPE: " << method << llendl;
		llwarns << "CURL REQ HEADERS: " << headers.asString() << llendl;
		llwarns << "CURL REQ BODY: " << body_str << llendl;
		llwarns << "CURL HTTP_STATUS: " << http_status << llendl;
		llwarns << "CURL ERROR: " << curl_error_buffer << llendl;
		llwarns << "CURL ERROR BODY: " << http_buffer.asString() << llendl;
		response["body"] = http_buffer.asString();
	}
	else
	{
		response["body"] = http_buffer.asLLSD();
		lldebugs << "CURL response: " << http_buffer.asString() << llendl;
	}
	
	if(headers_list)
	{	// free the header list  
		curl_slist_free_all(headers_list); 
	}

	// * Cleanup
	curl_easy_cleanup(curlp);
	return response;
}

LLSD LLHTTPClient::blockingGet(const std::string& url)
{
	return blocking_request(url, LLURLRequest::HTTP_GET, LLSD());
}

LLSD LLHTTPClient::blockingPost(const std::string& url, const LLSD& body)
{
	return blocking_request(url, LLURLRequest::HTTP_POST, body);
}

void LLHTTPClient::put(
	const std::string& url,
	const LLSD& body,
	ResponderPtr responder,
	const LLSD& headers,
	const F32 timeout)
{
	request(url, LLURLRequest::HTTP_PUT, new LLSDInjector(body), responder, timeout, headers);
}

void LLHTTPClient::post(
	const std::string& url,
	const LLSD& body,
	ResponderPtr responder,
	const LLSD& headers,
	const F32 timeout)
{
	request(url, LLURLRequest::HTTP_POST, new LLSDInjector(body), responder, timeout, headers);
}

void LLHTTPClient::postRaw(
	const std::string& url,
	const U8* data,
	S32 size,
	ResponderPtr responder,
	const LLSD& headers,
	const F32 timeout)
{
	request(url, LLURLRequest::HTTP_POST, new RawInjector(data, size), responder, timeout, headers);
}

void LLHTTPClient::postFile(
	const std::string& url,
	const std::string& filename,
	ResponderPtr responder,
	const LLSD& headers,
	const F32 timeout)
{
	request(url, LLURLRequest::HTTP_POST, new FileInjector(filename), responder, timeout, headers);
}

void LLHTTPClient::postFile(
	const std::string& url,
	const LLUUID& uuid,
	LLAssetType::EType asset_type,
	ResponderPtr responder,
	const LLSD& headers,
	const F32 timeout)
{
	request(url, LLURLRequest::HTTP_POST, new VFileInjector(uuid, asset_type), responder, timeout, headers);
}

// static
void LLHTTPClient::del(
	const std::string& url,
	ResponderPtr responder,
	const LLSD& headers,
	const F32 timeout)
{
	request(url, LLURLRequest::HTTP_DELETE, NULL, responder, timeout, headers);
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
	headers["Destination"] = destination;
	request(url, LLURLRequest::HTTP_MOVE, NULL, responder, timeout, headers);
}


void LLHTTPClient::setPump(LLPumpIO& pump)
{
	theClientPump = &pump;
}

bool LLHTTPClient::hasPump()
{
	return theClientPump != NULL;
}
