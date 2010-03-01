/** 
 * @file llurlrequest.cpp
 * @author Phoenix
 * @date 2005-04-28
 * @brief Implementation of the URLRequest class and related classes.
 *
 * $LicenseInfo:firstyear=2005&license=viewergpl$
 * 
 * Copyright (c) 2005-2009, Linden Research, Inc.
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
#include "llurlrequest.h"

#include <algorithm>
#include <openssl/x509_vfy.h>
#include <openssl/ssl.h>
#include "llcurl.h"
#include "llioutil.h"
#include "llmemtype.h"
#include "llpumpio.h"
#include "llsd.h"
#include "llstring.h"
#include "apr_env.h"
#include "llapr.h"
static const U32 HTTP_STATUS_PIPE_ERROR = 499;

/**
 * String constants
 */
const std::string CONTEXT_DEST_URI_SD_LABEL("dest_uri");
const std::string CONTEXT_TRANSFERED_BYTES("transfered_bytes");


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
	S32 mByteAccumulator;
	bool mIsBodyLimitSet;
	LLURLRequest::SSLCertVerifyCallback mSSLVerifyCallback;
};

LLURLRequestDetail::LLURLRequestDetail() :
	mCurlRequest(NULL),
	mResponseBuffer(NULL),
	mLastRead(NULL),
	mBodyLimit(0),
	mByteAccumulator(0),
	mIsBodyLimitSet(false),
    mSSLVerifyCallback(NULL)
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

void LLURLRequest::setSSLVerifyCallback(SSLCertVerifyCallback callback, void *param)
{
	mDetail->mSSLVerifyCallback = callback;
	mDetail->mCurlRequest->setSSLCtxCallback(LLURLRequest::_sslCtxCallback, (void *)this);
	mDetail->mCurlRequest->setopt(CURLOPT_SSL_VERIFYPEER, true);
	mDetail->mCurlRequest->setopt(CURLOPT_SSL_VERIFYHOST, 2);	
}


// _sslCtxFunction
// Callback function called when an SSL Context is created via CURL
// used to configure the context for custom cert validation

CURLcode LLURLRequest::_sslCtxCallback(CURL * curl, void *sslctx, void *param)
{	
	LLURLRequest *req = (LLURLRequest *)param;
	if(req == NULL || req->mDetail->mSSLVerifyCallback == NULL)
	{
		SSL_CTX_set_cert_verify_callback((SSL_CTX *)sslctx, NULL, NULL);
		return CURLE_OK;
	}
	SSL_CTX * ctx = (SSL_CTX *) sslctx;
	// disable any default verification for server certs
	SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, NULL);
	// set the verification callback.
	SSL_CTX_set_cert_verify_callback(ctx, req->mDetail->mSSLVerifyCallback, (void *)req);
	// the calls are void
	return CURLE_OK;
	
}

/**
 * class LLURLRequest
 */

// static
std::string LLURLRequest::actionAsVerb(LLURLRequest::ERequestAction action)
{
	static const std::string VERBS[] =
	{
		"(invalid)",
		"HEAD",
		"GET",
		"PUT",
		"POST",
		"DELETE",
		"MOVE"
	};
	if(((S32)action <=0) || ((S32)action >= REQUEST_ACTION_COUNT))
	{
		return VERBS[0];
	}
	return VERBS[action];
}

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

std::string LLURLRequest::getURL() const
{
	return mDetail->mURL;
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
        LLAPRPool pool;
		status = apr_env_get(&env_proxy, "ALL_PROXY", pool.getAPRPool());
        if (status != APR_SUCCESS)
        {
			status = apr_env_get(&env_proxy, "http_proxy", pool.getAPRPool());
        }
        if (status != APR_SUCCESS)
        {
           use_proxy = FALSE;
        }
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
	if (!buffer) return STATUS_ERROR;
	
	// we're still waiting or prcessing, check how many
	// bytes we have accumulated.
	const S32 MIN_ACCUMULATION = 100000;
	if(pump && (mDetail->mByteAccumulator > MIN_ACCUMULATION))
	{
		 // This is a pretty sloppy calculation, but this
		 // tries to make the gross assumption that if data
		 // is coming in at 56kb/s, then this transfer will
		 // probably succeed. So, if we're accumlated
		 // 100,000 bytes (MIN_ACCUMULATION) then let's
		 // give this client another 2s to complete.
		 const F32 TIMEOUT_ADJUSTMENT = 2.0f;
		 mDetail->mByteAccumulator = 0;
		 pump->adjustTimeoutSeconds(TIMEOUT_ADJUSTMENT);
		 lldebugs << "LLURLRequest adjustTimeoutSeconds for request: " << mDetail->mURL << llendl;
		 if (mState == STATE_INITIALIZED)
		 {
			  llinfos << "LLURLRequest adjustTimeoutSeconds called during upload" << llendl;
		 }
	}

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
			if(!newmsg)
			{
				// keep processing
				break;
			}

			mState = STATE_HAVE_RESPONSE;
			context[CONTEXT_REQUEST][CONTEXT_TRANSFERED_BYTES] = mRequestTransferedBytes;
			context[CONTEXT_RESPONSE][CONTEXT_TRANSFERED_BYTES] = mResponseTransferedBytes;
			lldebugs << this << "Setting context to " << context << llendl;
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
		context[CONTEXT_REQUEST][CONTEXT_TRANSFERED_BYTES] = mRequestTransferedBytes;
		context[CONTEXT_RESPONSE][CONTEXT_TRANSFERED_BYTES] = mResponseTransferedBytes;
		lldebugs << this << "Setting context to " << context << llendl;
		return STATUS_DONE;

	default:
		PUMP_DEBUG;
		context[CONTEXT_REQUEST][CONTEXT_TRANSFERED_BYTES] = mRequestTransferedBytes;
		context[CONTEXT_RESPONSE][CONTEXT_TRANSFERED_BYTES] = mResponseTransferedBytes;
		lldebugs << this << "Setting context to " << context << llendl;
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
	mRequestTransferedBytes = 0;
	mResponseTransferedBytes = 0;
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
	req->mResponseTransferedBytes += bytes;
	req->mDetail->mByteAccumulator += bytes;
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
	req->mRequestTransferedBytes += bytes;
	return bytes;
}

static size_t headerCallback(void* data, size_t size, size_t nmemb, void* user)
{
	const char* header_line = (const char*)data;
	size_t header_len = size * nmemb;
	LLURLRequestComplete* complete = (LLURLRequestComplete*)user;

	if (!complete || !header_line)
	{
		return header_len;
	}

	// *TODO: This should be a utility in llstring.h: isascii()
	for (size_t i = 0; i < header_len; ++i)
	{
		if (header_line[i] < 0)
		{
			return header_len;
		}
	}

	std::string header(header_line, header_len);

	// Per HTTP spec the first header line must be the status line.
	if (header.substr(0,5) == "HTTP/")
	{
		std::string::iterator end = header.end();
		std::string::iterator pos1 = std::find(header.begin(), end, ' ');
		if (pos1 != end) ++pos1;
		std::string::iterator pos2 = std::find(pos1, end, ' ');
		if (pos2 != end) ++pos2;
		std::string::iterator pos3 = std::find(pos2, end, '\r');

		std::string version(header.begin(), pos1);
		std::string status(pos1, pos2);
		std::string reason(pos2, pos3);

		S32 status_code = atoi(status.c_str());
		if (status_code > 0)
		{
			complete->httpStatus((U32)status_code, reason);
			return header_len;
		}
	}

	std::string::iterator sep = std::find(header.begin(),header.end(),':');

	if (sep != header.end())
	{
		std::string key(header.begin(), sep);
		std::string value(sep + 1, header.end());

		key = utf8str_tolower(utf8str_trim(key));
		value = utf8str_trim(value);

		complete->header(key, value);
	}
	else
	{
		LLStringUtil::trim(header);
		if (!header.empty())
		{
			llwarns << "Unable to parse header: " << header << llendl;
		}
	}

	return header_len;
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
		mRequest->setURL(context[CONTEXT_DEST_URI_SD_LABEL].asString());
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
