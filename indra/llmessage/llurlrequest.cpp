/** 
 * @file llurlrequest.cpp
 * @author Phoenix
 * @date 2005-04-28
 * @brief Implementation of the URLRequest class and related classes.
 *
 * $LicenseInfo:firstyear=2005&license=viewergpl$
 * 
 * Copyright (c) 2005-2007, Linden Research, Inc.
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
#include "llurlrequest.h"

#include <algorithm>

#include "llcurl.h"
#include "llioutil.h"
#include "llmemtype.h"
#include "llpumpio.h"
#include "llsd.h"
#include "llstring.h"
#include "apr_env.h"

static const U32 HTTP_STATUS_PIPE_ERROR = 499;

/**
 * String constants
 */
const std::string CONTEXT_DEST_URI_SD_LABEL("dest_uri");


static size_t headerCallback(void* data, size_t size, size_t nmemb, void* user);

/**
 * class LLURLRequestDetail
 */
class LLURLRequestDetail
{
public:
	LLURLRequestDetail();
	~LLURLRequestDetail();
	std::string mURL;
	LLCurlEasyRequest* mCurlRequest;
	LLBufferArray* mResponseBuffer;
	LLChannelDescriptors mChannels;
	U8* mLastRead;
	U32 mBodyLimit;
	bool mIsBodyLimitSet;
};

LLURLRequestDetail::LLURLRequestDetail() :
	mCurlRequest(NULL),
	mResponseBuffer(NULL),
	mLastRead(NULL),
	mBodyLimit(0),
	mIsBodyLimitSet(false)
	
{
	LLMemType m1(LLMemType::MTYPE_IO_URL_REQUEST);
	mCurlRequest = new LLCurlEasyRequest();
}

LLURLRequestDetail::~LLURLRequestDetail()
{
	LLMemType m1(LLMemType::MTYPE_IO_URL_REQUEST);
	delete mCurlRequest;
	mResponseBuffer = NULL;
	mLastRead = NULL;
}


/**
 * class LLURLRequest
 */

LLURLRequest::LLURLRequest(LLURLRequest::ERequestAction action) :
	mAction(action)
{
	LLMemType m1(LLMemType::MTYPE_IO_URL_REQUEST);
	initialize();
}

LLURLRequest::LLURLRequest(
	LLURLRequest::ERequestAction action,
	const std::string& url) :
	mAction(action)
{
	LLMemType m1(LLMemType::MTYPE_IO_URL_REQUEST);
	initialize();
	setURL(url);
}

LLURLRequest::~LLURLRequest()
{
	LLMemType m1(LLMemType::MTYPE_IO_URL_REQUEST);
	delete mDetail;
}

void LLURLRequest::setURL(const std::string& url)
{
	LLMemType m1(LLMemType::MTYPE_IO_URL_REQUEST);
	mDetail->mURL = url;
}

void LLURLRequest::addHeader(const char* header)
{
	LLMemType m1(LLMemType::MTYPE_IO_URL_REQUEST);
	mDetail->mCurlRequest->slist_append(header);
}

void LLURLRequest::setBodyLimit(U32 size)
{
	mDetail->mBodyLimit = size;
	mDetail->mIsBodyLimitSet = true;
}

void LLURLRequest::checkRootCertificate(bool check)
{
	mDetail->mCurlRequest->setopt(CURLOPT_SSL_VERIFYPEER, (check? TRUE : FALSE));
	mDetail->mCurlRequest->setoptString(CURLOPT_ENCODING, "");
}

void LLURLRequest::setCallback(LLURLRequestComplete* callback)
{
	LLMemType m1(LLMemType::MTYPE_IO_URL_REQUEST);
	mCompletionCallback = callback;
	mDetail->mCurlRequest->setHeaderCallback(&headerCallback, (void*)callback);
}

// Added to mitigate the effect of libcurl looking
// for the ALL_PROXY and http_proxy env variables
// and deciding to insert a Pragma: no-cache
// header! The only usage of this method at the
// time of this writing is in llhttpclient.cpp
// in the request() method, where this method
// is called with use_proxy = FALSE
void LLURLRequest::useProxy(bool use_proxy)
{
    static char *env_proxy;

    if (use_proxy && (env_proxy == NULL))
    {
        apr_status_t status;
        apr_pool_t* pool;
        apr_pool_create(&pool, NULL);
        status = apr_env_get(&env_proxy, "ALL_PROXY", pool);
        if (status != APR_SUCCESS)
        {
            status = apr_env_get(&env_proxy, "http_proxy", pool);
        }
        if (status != APR_SUCCESS)
        {
           use_proxy = FALSE;
        }
        apr_pool_destroy(pool);
    }


    lldebugs << "use_proxy = " << (use_proxy?'Y':'N') << ", env_proxy = " << (env_proxy ? env_proxy : "(null)") << llendl;

    if (env_proxy && use_proxy)
    {
		mDetail->mCurlRequest->setoptString(CURLOPT_PROXY, env_proxy);
    }
    else
    {
        mDetail->mCurlRequest->setoptString(CURLOPT_PROXY, "");
    }
}

void LLURLRequest::useProxy(const std::string &proxy)
{
    mDetail->mCurlRequest->setoptString(CURLOPT_PROXY, proxy);
}

// virtual
LLIOPipe::EStatus LLURLRequest::handleError(
	LLIOPipe::EStatus status,
	LLPumpIO* pump)
{
	LLMemType m1(LLMemType::MTYPE_IO_URL_REQUEST);
	if(mCompletionCallback && pump)
	{
		LLURLRequestComplete* complete = NULL;
		complete = (LLURLRequestComplete*)mCompletionCallback.get();
		complete->httpStatus(
			HTTP_STATUS_PIPE_ERROR,
			LLIOPipe::lookupStatusString(status));
		complete->responseStatus(status);
		pump->respond(complete);
		mCompletionCallback = NULL;
	}
	return status;
}

// virtual
LLIOPipe::EStatus LLURLRequest::process_impl(
	const LLChannelDescriptors& channels,
	buffer_ptr_t& buffer,
	bool& eos,
	LLSD& context,
	LLPumpIO* pump)
{
	PUMP_DEBUG;
	LLMemType m1(LLMemType::MTYPE_IO_URL_REQUEST);
	//llinfos << "LLURLRequest::process_impl()" << llendl;
	if(!buffer) return STATUS_ERROR;
	switch(mState)
	{
	case STATE_INITIALIZED:
	{
		PUMP_DEBUG;
		// We only need to wait for input if we are uploading
		// something.
		if(((HTTP_PUT == mAction) || (HTTP_POST == mAction)) && !eos)
		{
			// we're waiting to get all of the information
			return STATUS_BREAK;
		}

		// *FIX: bit of a hack, but it should work. The configure and
		// callback method expect this information to be ready.
		mDetail->mResponseBuffer = buffer.get();
		mDetail->mChannels = channels;
		if(!configure())
		{
			return STATUS_ERROR;
		}
		mState = STATE_WAITING_FOR_RESPONSE;

		// *FIX: Maybe we should just go to the next state now...
		return STATUS_BREAK;
	}
	case STATE_WAITING_FOR_RESPONSE:
	case STATE_PROCESSING_RESPONSE:
	{
		PUMP_DEBUG;
		LLIOPipe::EStatus status = STATUS_BREAK;
		mDetail->mCurlRequest->perform();
		while(1)
		{
			CURLcode result;
			bool newmsg = mDetail->mCurlRequest->getResult(&result);
			if (!newmsg)
			{
				break;
			}

			mState = STATE_HAVE_RESPONSE;
			switch(result)
			{
				case CURLE_OK:
				case CURLE_WRITE_ERROR:
					// NB: The error indication means that we stopped the
					// writing due the body limit being reached
					if(mCompletionCallback && pump)
					{
						LLURLRequestComplete* complete = NULL;
						complete = (LLURLRequestComplete*)
							mCompletionCallback.get();
						complete->responseStatus(
								result == CURLE_OK
									? STATUS_OK : STATUS_STOP);
						LLPumpIO::links_t chain;
						LLPumpIO::LLLinkInfo link;
						link.mPipe = mCompletionCallback;
						link.mChannels = LLBufferArray::makeChannelConsumer(
							channels);
						chain.push_back(link);
						pump->respond(chain, buffer, context);
						mCompletionCallback = NULL;
					}
					break;
				case CURLE_FAILED_INIT:
				case CURLE_COULDNT_CONNECT:
					status = STATUS_NO_CONNECTION;
					break;
				default:
					llwarns << "URLRequest Error: " << result
							<< ", "
							<< LLCurl::strerror(result)
							<< ", "
							<< (mDetail->mURL.empty() ? "<EMPTY URL>" : mDetail->mURL)
							<< llendl;
					status = STATUS_ERROR;
					break;
			}
		}
		return status;
	}
	case STATE_HAVE_RESPONSE:
		PUMP_DEBUG;
		// we already stuffed everything into channel in in the curl
		// callback, so we are done.
		eos = true;
		return STATUS_DONE;

	default:
		PUMP_DEBUG;
		return STATUS_ERROR;
	}
}

void LLURLRequest::initialize()
{
	LLMemType m1(LLMemType::MTYPE_IO_URL_REQUEST);
	mState = STATE_INITIALIZED;
	mDetail = new LLURLRequestDetail;
	mDetail->mCurlRequest->setopt(CURLOPT_NOSIGNAL, 1);
	mDetail->mCurlRequest->setWriteCallback(&downCallback, (void*)this);
	mDetail->mCurlRequest->setReadCallback(&upCallback, (void*)this);
}

bool LLURLRequest::configure()
{
	LLMemType m1(LLMemType::MTYPE_IO_URL_REQUEST);
	bool rv = false;
	S32 bytes = mDetail->mResponseBuffer->countAfter(
   		mDetail->mChannels.in(),
		NULL);
	switch(mAction)
	{
	case HTTP_HEAD:
		mDetail->mCurlRequest->setopt(CURLOPT_HEADER, 1);
		mDetail->mCurlRequest->setopt(CURLOPT_NOBODY, 1);
		mDetail->mCurlRequest->setopt(CURLOPT_FOLLOWLOCATION, 1);
		rv = true;
		break;
	case HTTP_GET:
		mDetail->mCurlRequest->setopt(CURLOPT_HTTPGET, 1);
		mDetail->mCurlRequest->setopt(CURLOPT_FOLLOWLOCATION, 1);
		rv = true;
		break;

	case HTTP_PUT:
		// Disable the expect http 1.1 extension. POST and PUT default
		// to turning this on, and I am not too sure what it means.
		addHeader("Expect:");

		mDetail->mCurlRequest->setopt(CURLOPT_UPLOAD, 1);
		mDetail->mCurlRequest->setopt(CURLOPT_INFILESIZE, bytes);
		rv = true;
		break;

	case HTTP_POST:
		// Disable the expect http 1.1 extension. POST and PUT default
		// to turning this on, and I am not too sure what it means.
		addHeader("Expect:");

		// Disable the content type http header.
		// *FIX: what should it be?
		addHeader("Content-Type:");

		// Set the handle for an http post
		mDetail->mCurlRequest->setPost(NULL, bytes);
		rv = true;
		break;

	case HTTP_DELETE:
		// Set the handle for an http post
		mDetail->mCurlRequest->setoptString(CURLOPT_CUSTOMREQUEST, "DELETE");
		rv = true;
		break;

	case HTTP_MOVE:
		// Set the handle for an http post
		mDetail->mCurlRequest->setoptString(CURLOPT_CUSTOMREQUEST, "MOVE");
		// *NOTE: should we check for the Destination header?
		rv = true;
		break;

	default:
		llwarns << "Unhandled URLRequest action: " << mAction << llendl;
		break;
	}
	if(rv)
	{
		mDetail->mCurlRequest->sendRequest(mDetail->mURL);
	}
	return rv;
}

// static
size_t LLURLRequest::downCallback(
	char* data,
	size_t size,
	size_t nmemb,
	void* user)
{
	LLMemType m1(LLMemType::MTYPE_IO_URL_REQUEST);
	LLURLRequest* req = (LLURLRequest*)user;
	if(STATE_WAITING_FOR_RESPONSE == req->mState)
	{
		req->mState = STATE_PROCESSING_RESPONSE;
	}
	U32 bytes = size * nmemb;
	if (req->mDetail->mIsBodyLimitSet)
	{
		if (bytes > req->mDetail->mBodyLimit)
		{
			bytes = req->mDetail->mBodyLimit;
			req->mDetail->mBodyLimit = 0;
		}
		else
		{
			req->mDetail->mBodyLimit -= bytes;
		}
	}

	req->mDetail->mResponseBuffer->append(
		req->mDetail->mChannels.out(),
		(U8*)data,
		bytes);
	return bytes;
}

// static
size_t LLURLRequest::upCallback(
	char* data,
	size_t size,
	size_t nmemb,
	void* user)
{
	LLMemType m1(LLMemType::MTYPE_IO_URL_REQUEST);
	LLURLRequest* req = (LLURLRequest*)user;
	S32 bytes = llmin(
		(S32)(size * nmemb),
		req->mDetail->mResponseBuffer->countAfter(
			req->mDetail->mChannels.in(),
			req->mDetail->mLastRead));
	req->mDetail->mLastRead =  req->mDetail->mResponseBuffer->readAfter(
		req->mDetail->mChannels.in(),
		req->mDetail->mLastRead,
		(U8*)data,
		bytes);
	return bytes;
}

static size_t headerCallback(void* data, size_t size, size_t nmemb, void* user)
{
	const char* headerLine = (const char*)data;
	size_t headerLen = size * nmemb;
	LLURLRequestComplete* complete = (LLURLRequestComplete*)user;

	// *TODO: This should be a utility in llstring.h: isascii()
	for (size_t i = 0; i < headerLen; ++i)
	{
		if (headerLine[i] < 0)
		{
			return headerLen;
		}
	}

	size_t sep;
	for (sep = 0; sep < headerLen  &&  headerLine[sep] != ':'; ++sep) { }

	if (sep < headerLen && complete)
	{
		std::string key(headerLine, sep);
		std::string value(headerLine + sep + 1, headerLen - sep - 1);

		key = utf8str_tolower(utf8str_trim(key));
		value = utf8str_trim(value);

		complete->header(key, value);
	}
	else
	{
		std::string s(headerLine, headerLen);

		std::string::iterator end = s.end();
		std::string::iterator pos1 = std::find(s.begin(), end, ' ');
		if (pos1 != end) ++pos1;
		std::string::iterator pos2 = std::find(pos1, end, ' ');
		if (pos2 != end) ++pos2;
		std::string::iterator pos3 = std::find(pos2, end, '\r');

		std::string version(s.begin(), pos1);
		std::string status(pos1, pos2);
		std::string reason(pos2, pos3);

		int statusCode = atoi(status.c_str());
		if (statusCode > 0)
		{
			if (complete)
			{
				complete->httpStatus((U32)statusCode, reason);
			}
		}
	}

	return headerLen;
}

/**
 * LLContextURLExtractor
 */
// virtual
LLIOPipe::EStatus LLContextURLExtractor::process_impl(
	const LLChannelDescriptors& channels,
	buffer_ptr_t& buffer,
	bool& eos,
	LLSD& context,
	LLPumpIO* pump)
{
	PUMP_DEBUG;
	LLMemType m1(LLMemType::MTYPE_IO_URL_REQUEST);
	// The destination host is in the context.
	if(context.isUndefined() || !mRequest)
	{
		return STATUS_PRECONDITION_NOT_MET;
	}

	// copy in to out, since this just extract the URL and does not
	// actually change the data.
	LLChangeChannel change(channels.in(), channels.out());
	std::for_each(buffer->beginSegment(), buffer->endSegment(), change);

	// find the context url
	if(context.has(CONTEXT_DEST_URI_SD_LABEL))
	{
		mRequest->setURL(context[CONTEXT_DEST_URI_SD_LABEL]);
		return STATUS_DONE;
	}
	return STATUS_ERROR;
}


/**
 * LLURLRequestComplete
 */
LLURLRequestComplete::LLURLRequestComplete() :
	mRequestStatus(LLIOPipe::STATUS_ERROR)
{
	LLMemType m1(LLMemType::MTYPE_IO_URL_REQUEST);
}

// virtual
LLURLRequestComplete::~LLURLRequestComplete()
{
	LLMemType m1(LLMemType::MTYPE_IO_URL_REQUEST);
}

//virtual 
void LLURLRequestComplete::header(const std::string& header, const std::string& value)
{
}

//virtual 
void LLURLRequestComplete::httpStatus(U32 status, const std::string& reason)
{
}

//virtual 
void LLURLRequestComplete::complete(const LLChannelDescriptors& channels,
		const buffer_ptr_t& buffer)
{
	if(STATUS_OK == mRequestStatus)
	{
		response(channels, buffer);
	}
	else
	{
		noResponse();
	}
}

//virtual 
void LLURLRequestComplete::response(const LLChannelDescriptors& channels,
		const buffer_ptr_t& buffer)
{
	llwarns << "LLURLRequestComplete::response default implementation called"
		<< llendl;
}

//virtual 
void LLURLRequestComplete::noResponse()
{
	llwarns << "LLURLRequestComplete::noResponse default implementation called"
		<< llendl;
}

void LLURLRequestComplete::responseStatus(LLIOPipe::EStatus status)
{
	LLMemType m1(LLMemType::MTYPE_IO_URL_REQUEST);
	mRequestStatus = status;
}

// virtual
LLIOPipe::EStatus LLURLRequestComplete::process_impl(
	const LLChannelDescriptors& channels,
	buffer_ptr_t& buffer,
	bool& eos,
	LLSD& context,
	LLPumpIO* pump)
{
	PUMP_DEBUG;
	complete(channels, buffer);
	return STATUS_OK;
}
