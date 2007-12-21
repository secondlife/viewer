/** 
 * @file llhttpclient.cpp
 * @brief Implementation of classes for making HTTP requests.
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

#include "linden_common.h"

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
static std::string gCABundle;


LLHTTPClient::Responder::Responder()
	: mReferenceCount(0)
{
}

LLHTTPClient::Responder::~Responder()
{
}

// virtual
void LLHTTPClient::Responder::error(U32 status, const std::string& reason)
{
	llinfos << "LLHTTPClient::Responder::error "
		<< status << ": " << reason << llendl;
}

// virtual
void LLHTTPClient::Responder::result(const LLSD& content)
{
}

// virtual 
void LLHTTPClient::Responder::completedRaw(U32 status, const std::string& reason, const LLChannelDescriptors& channels,
								const LLIOPipe::buffer_ptr_t& buffer)
{
	LLBufferStream istr(channels, buffer.get());
	LLSD content;

	if (isGoodStatus(status))
	{
		LLSDSerialize::fromXML(content, istr);
/*
		const S32 parseError = -1;
		if(LLSDSerialize::fromXML(content, istr) == parseError)
		{
			mStatus = 498;
			mReason = "Client Parse Error";
		}
*/
	}
	
	completed(status, reason, content);
}

// virtual
void LLHTTPClient::Responder::completed(U32 status, const std::string& reason, const LLSD& content)
{
	if(isGoodStatus(status))
	{
		result(content);
	}
	else
	{
		error(status, reason);
	}
}


namespace
{
	class LLHTTPClientURLAdaptor : public LLURLRequestComplete
	{
	public:
		LLHTTPClientURLAdaptor(LLHTTPClient::ResponderPtr responder)
			: mResponder(responder),
				mStatus(499), mReason("LLURLRequest complete w/no status")
		{
		}
		
		~LLHTTPClientURLAdaptor()
		{
		}

		virtual void httpStatus(U32 status, const std::string& reason)
		{
			mStatus = status;
			mReason = reason;
		}

		virtual void complete(const LLChannelDescriptors& channels,
								const buffer_ptr_t& buffer)
		{
			if (mResponder.get())
			{
				mResponder->completedRaw(mStatus, mReason, channels, buffer);
			}
		}

	private:
		LLHTTPClient::ResponderPtr mResponder;
		U32 mStatus;
		std::string mReason;
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

		const char* contentType() { return "application/xml"; }

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
		virtual ~RawInjector() {}

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

			llifstream fstream(mFilename.c_str(), std::iostream::binary | std::iostream::out);
            fstream.seekg(0, std::ios::end);
            U32 fileSize = fstream.tellg();
            fstream.seekg(0, std::ios::beg);
			char* fileBuffer;
			fileBuffer = new char [fileSize];
            fstream.read(fileBuffer, fileSize);
            ostream.write(fileBuffer, fileSize);
			fstream.close();
			eos = true;
			return STATUS_DONE;
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

static void request(
	const std::string& url,
	LLURLRequest::ERequestAction method,
	Injector* body_injector,
	LLHTTPClient::ResponderPtr responder,
    const LLSD& headers,
	const F32 timeout=HTTP_REQUEST_EXPIRY_SECS)
{
	if (!LLHTTPClient::hasPump())
	{
		responder->completed(U32_MAX, "No pump", LLSD());
		return;
	}
	LLPumpIO::chain_t chain;

	LLURLRequest *req = new LLURLRequest(method, url);
	req->requestEncoding("");

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
            if ((iter->first == "Pragma") && (iter->second.asString() == ""))
            {
                req->useProxy(FALSE);
            }
            header << iter->first << ": " << iter->second.asString() ;
            llinfos << "header = " << header.str() << llendl;
            req->addHeader(header.str().c_str());
        }
    }
	if (!gCABundle.empty())
	{
		req->checkRootCertificate(true, gCABundle.c_str());
	}
	req->setCallback(new LLHTTPClientURLAdaptor(responder));

	if (method == LLURLRequest::HTTP_POST  &&  gMessageSystem)
	{
		req->addHeader(llformat("X-SecondLife-UDP-Listen-Port: %d",
								gMessageSystem->mPort).c_str());
   	}
	
	if (method == LLURLRequest::HTTP_PUT || method == LLURLRequest::HTTP_POST)
	{
		req->addHeader(llformat("Content-Type: %s",
								body_injector->contentType()).c_str());

   		chain.push_back(LLIOPipe::ptr_t(body_injector));
	}
	chain.push_back(LLIOPipe::ptr_t(req));

	theClientPump->addChain(chain, timeout);
}

static void request(
	const std::string& url,
	LLURLRequest::ERequestAction method,
	Injector* body_injector,
	LLHTTPClient::ResponderPtr responder,
	const F32 timeout=HTTP_REQUEST_EXPIRY_SECS)
{
    request(url, method, body_injector, responder, LLSD(), timeout);
}

void LLHTTPClient::head(const std::string& url, ResponderPtr responder, const F32 timeout)
{
	request(url, LLURLRequest::HTTP_HEAD, NULL, responder, timeout);
}

void LLHTTPClient::get(const std::string& url, ResponderPtr responder, const LLSD& headers, const F32 timeout)
{
	request(url, LLURLRequest::HTTP_GET, NULL, responder, headers, timeout);
}

void LLHTTPClient::get(const std::string& url, ResponderPtr responder, const F32 timeout)
{
	get(url, responder, LLSD(), timeout);
}

void LLHTTPClient::get(const std::string& url, const LLSD& query, ResponderPtr responder, const LLSD& headers, const F32 timeout)
{
	LLURI uri;
	
	uri = LLURI::buildHTTP(url, LLSD::emptyArray(), query);
	get(uri.asString(), responder, headers, timeout);
}

void LLHTTPClient::get(const std::string& url, const LLSD& query, ResponderPtr responder, const F32 timeout)
{
	get(url, query, responder, LLSD(), timeout);
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

// This call is blocking! This is probably usually bad. :(
LLSD LLHTTPClient::blockingGet(const std::string& url)
{
	llinfos << "blockingGet of " << url << llendl;

	// Returns an LLSD map: {status: integer, body: map}
	char curl_error_buffer[CURL_ERROR_SIZE];
	CURL* curlp = curl_easy_init();

	LLHTTPBuffer http_buffer;

	// Without this timeout, blockingGet() calls have been observed to take
	// up to 90 seconds to complete.  Users of blockingGet() already must 
	// check the HTTP return code for validity, so this will not introduce
	// new errors.  A 5 second timeout will succeed > 95% of the time (and 
	// probably > 99% of the time) based on my statistics. JC
	curl_easy_setopt(curlp, CURLOPT_NOSIGNAL, 1);	// don't use SIGALRM for timeouts
	curl_easy_setopt(curlp, CURLOPT_TIMEOUT, 5);	// seconds

	curl_easy_setopt(curlp, CURLOPT_WRITEFUNCTION, LLHTTPBuffer::curl_write);
	curl_easy_setopt(curlp, CURLOPT_WRITEDATA, &http_buffer);
	curl_easy_setopt(curlp, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curlp, CURLOPT_ERRORBUFFER, curl_error_buffer);
	curl_easy_setopt(curlp, CURLOPT_FAILONERROR, 1);

	LLSD response = LLSD::emptyMap();

	S32 curl_success = curl_easy_perform(curlp);

	S32 http_status = 499;
	curl_easy_getinfo(curlp,CURLINFO_RESPONSE_CODE, &http_status);

	response["status"] = http_status;

	if (curl_success != 0 
		&& http_status != 404)  // We expect 404s, don't spam for them.
	{
		llwarns << "CURL ERROR: " << curl_error_buffer << llendl;
		
		response["body"] = http_buffer.asString();
	}
	else
	{
		response["body"] = http_buffer.asLLSD();
	}
	
	curl_easy_cleanup(curlp);

	return response;
}

void LLHTTPClient::put(const std::string& url, const LLSD& body, ResponderPtr responder, const F32 timeout)
{
	request(url, LLURLRequest::HTTP_PUT, new LLSDInjector(body), responder, timeout);
}

void LLHTTPClient::post(const std::string& url, const LLSD& body, ResponderPtr responder, const F32 timeout)
{
	request(url, LLURLRequest::HTTP_POST, new LLSDInjector(body), responder, timeout);
}

void LLHTTPClient::post(const std::string& url, const U8* data, S32 size, ResponderPtr responder, const F32 timeout)
{
	request(url, LLURLRequest::HTTP_POST, new RawInjector(data, size), responder, timeout);
}

void LLHTTPClient::del(const std::string& url, ResponderPtr responder, const F32 timeout)
{
	request(url, LLURLRequest::HTTP_DELETE, NULL, responder, timeout);
}

#if 1
void LLHTTPClient::postFile(const std::string& url, const std::string& filename, ResponderPtr responder, const F32 timeout)
{
	request(url, LLURLRequest::HTTP_POST, new FileInjector(filename), responder, timeout);
}

void LLHTTPClient::postFile(const std::string& url, const LLUUID& uuid,
							LLAssetType::EType asset_type, ResponderPtr responder, const F32 timeout)
{
	request(url, LLURLRequest::HTTP_POST, new VFileInjector(uuid, asset_type), responder, timeout);
}
#endif

void LLHTTPClient::setPump(LLPumpIO& pump)
{
	theClientPump = &pump;
}

bool LLHTTPClient::hasPump()
{
	return theClientPump != NULL;
}

void LLHTTPClient::setCABundle(const std::string& caBundle)
{
	gCABundle = caBundle;
}

namespace boost
{
	void intrusive_ptr_add_ref(LLHTTPClient::Responder* p)
	{
		++p->mReferenceCount;
	}
	
	void intrusive_ptr_release(LLHTTPClient::Responder* p)
	{
		if(p && 0 == --p->mReferenceCount)
		{
			delete p;
		}
	}
};
