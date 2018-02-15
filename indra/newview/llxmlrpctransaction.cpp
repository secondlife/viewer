/** 
 * @file llxmlrpctransaction.cpp
 * @brief LLXMLRPCTransaction and related class implementations 
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

#include "llviewerprecompiledheaders.h"
// include this to get winsock2 because openssl attempts to include winsock1
#include "llwin32headerslean.h"
#include <openssl/x509_vfy.h>
#include <openssl/ssl.h>
#include "llsecapi.h"

#include "llxmlrpctransaction.h"
#include "llxmlrpclistener.h"

#include "httpcommon.h"
#include "llhttpconstants.h"
#include "httprequest.h"
#include "httpoptions.h"
#include "httpheaders.h"
#include "bufferarray.h"
#include "llviewercontrol.h"

// Have to include these last to avoid queue redefinition!
#include <xmlrpc-epi/xmlrpc.h>

#include "llappviewer.h"
#include "lltrans.h"

#include "boost/move/unique_ptr.hpp"

namespace boost
{
	using ::boost::movelib::unique_ptr; // move unique_ptr into the boost namespace.
}

// Static instance of LLXMLRPCListener declared here so that every time we
// bring in this code, we instantiate a listener. If we put the static
// instance of LLXMLRPCListener into llxmlrpclistener.cpp, the linker would
// simply omit llxmlrpclistener.o, and shouting on the LLEventPump would do
// nothing.
static LLXMLRPCListener listener("LLXMLRPCTransaction");

LLXMLRPCValue LLXMLRPCValue::operator[](const char* id) const
{
	return LLXMLRPCValue(XMLRPC_VectorGetValueWithID(mV, id));
}

std::string LLXMLRPCValue::asString() const
{
	const char* s = XMLRPC_GetValueString(mV);
	return s ? s : "";
}

int		LLXMLRPCValue::asInt() const	{ return XMLRPC_GetValueInt(mV); }
bool	LLXMLRPCValue::asBool() const	{ return XMLRPC_GetValueBoolean(mV) != 0; }
double	LLXMLRPCValue::asDouble() const	{ return XMLRPC_GetValueDouble(mV); }

LLXMLRPCValue LLXMLRPCValue::rewind()
{
	return LLXMLRPCValue(XMLRPC_VectorRewind(mV));
}

LLXMLRPCValue LLXMLRPCValue::next()
{
	return LLXMLRPCValue(XMLRPC_VectorNext(mV));
}

bool LLXMLRPCValue::isValid() const
{
	return mV != NULL;
}

LLXMLRPCValue LLXMLRPCValue::createArray()
{
	return LLXMLRPCValue(XMLRPC_CreateVector(NULL, xmlrpc_vector_array));
}

LLXMLRPCValue LLXMLRPCValue::createStruct()
{
	return LLXMLRPCValue(XMLRPC_CreateVector(NULL, xmlrpc_vector_struct));
}


void LLXMLRPCValue::append(LLXMLRPCValue& v)
{
	XMLRPC_AddValueToVector(mV, v.mV);
}

void LLXMLRPCValue::appendString(const std::string& v)
{
	XMLRPC_AddValueToVector(mV, XMLRPC_CreateValueString(NULL, v.c_str(), 0));
}

void LLXMLRPCValue::appendInt(int v)
{
	XMLRPC_AddValueToVector(mV, XMLRPC_CreateValueInt(NULL, v));
}

void LLXMLRPCValue::appendBool(bool v)
{
	XMLRPC_AddValueToVector(mV, XMLRPC_CreateValueBoolean(NULL, v));
}

void LLXMLRPCValue::appendDouble(double v)
{
	XMLRPC_AddValueToVector(mV, XMLRPC_CreateValueDouble(NULL, v));
}


void LLXMLRPCValue::append(const char* id, LLXMLRPCValue& v)
{
	XMLRPC_SetValueID(v.mV, id, 0);
	XMLRPC_AddValueToVector(mV, v.mV);
}

void LLXMLRPCValue::appendString(const char* id, const std::string& v)
{
	XMLRPC_AddValueToVector(mV, XMLRPC_CreateValueString(id, v.c_str(), 0));
}

void LLXMLRPCValue::appendInt(const char* id, int v)
{
	XMLRPC_AddValueToVector(mV, XMLRPC_CreateValueInt(id, v));
}

void LLXMLRPCValue::appendBool(const char* id, bool v)
{
	XMLRPC_AddValueToVector(mV, XMLRPC_CreateValueBoolean(id, v));
}

void LLXMLRPCValue::appendDouble(const char* id, double v)
{
	XMLRPC_AddValueToVector(mV, XMLRPC_CreateValueDouble(id, v));
}

void LLXMLRPCValue::cleanup()
{
	XMLRPC_CleanupValue(mV);
	mV = NULL;
}

XMLRPC_VALUE LLXMLRPCValue::getValue() const
{
	return mV;
}


class LLXMLRPCTransaction::Handler : public LLCore::HttpHandler
{
public: 
	Handler(LLCore::HttpRequest::ptr_t &request, LLXMLRPCTransaction::Impl *impl);
	virtual ~Handler();

	virtual void onCompleted(LLCore::HttpHandle handle, LLCore::HttpResponse * response);

	typedef boost::shared_ptr<LLXMLRPCTransaction::Handler> ptr_t;

private:

	LLXMLRPCTransaction::Impl *mImpl;
	LLCore::HttpRequest::ptr_t mRequest;
};

class LLXMLRPCTransaction::Impl
{
public:
	typedef LLXMLRPCTransaction::EStatus	EStatus;

	LLCore::HttpRequest::ptr_t	mHttpRequest;


	EStatus				mStatus;
	CURLcode			mCurlCode;
	std::string			mStatusMessage;
	std::string			mStatusURI;
	LLCore::HttpResponse::TransferStats::ptr_t	mTransferStats;
	Handler::ptr_t		mHandler;
	LLCore::HttpHandle	mPostH;

	std::string			mURI;

	std::string			mProxyAddress;

	std::string			mResponseText;
	XMLRPC_REQUEST		mResponse;
	std::string         mCertStore;
	LLSD mErrorCertData;

	Impl(const std::string& uri, XMLRPC_REQUEST request, bool useGzip, const LLSD& httpParams);
	Impl(const std::string& uri,
		const std::string& method, LLXMLRPCValue params, bool useGzip);
	~Impl();

	bool process();

	void setStatus(EStatus code, const std::string& message = "", const std::string& uri = "");
	void setHttpStatus(const LLCore::HttpStatus &status);

private:
	void init(XMLRPC_REQUEST request, bool useGzip, const LLSD& httpParams);
};

LLXMLRPCTransaction::Handler::Handler(LLCore::HttpRequest::ptr_t &request, 
		LLXMLRPCTransaction::Impl *impl) :
	mImpl(impl),
	mRequest(request)
{
}

LLXMLRPCTransaction::Handler::~Handler()
{
}

void LLXMLRPCTransaction::Handler::onCompleted(LLCore::HttpHandle handle, 
	LLCore::HttpResponse * response)
{
	LLCore::HttpStatus status = response->getStatus();

	if (!status)
	{
		if ((status.toULong() != CURLE_SSL_PEER_CERTIFICATE) &&
			(status.toULong() != CURLE_SSL_CACERT))
		{
			// if we have a curl error that's not already been handled
			// (a non cert error), then generate the error message as
			// appropriate
			mImpl->setHttpStatus(status);
			LLSD errordata = status.getErrorData();
            mImpl->mErrorCertData = errordata;

			LL_WARNS() << "LLXMLRPCTransaction error "
				<< status.toHex() << ": " << status.toString() << LL_ENDL;
			LL_WARNS() << "LLXMLRPCTransaction request URI: "
				<< mImpl->mURI << LL_ENDL;
		}

		return;
	}

	mImpl->setStatus(LLXMLRPCTransaction::StatusComplete);
	mImpl->mTransferStats = response->getTransferStats();

	// the contents of a buffer array are potentially noncontiguous, so we
	// will need to copy them into an contiguous block of memory for XMLRPC.
	LLCore::BufferArray *body = response->getBody();
	char * bodydata = new char[body->size()];

	body->read(0, bodydata, body->size());

	mImpl->mResponse = XMLRPC_REQUEST_FromXML(bodydata, body->size(), 0);

	delete[] bodydata;

	bool		hasError = false;
	bool		hasFault = false;
	int			faultCode = 0;
	std::string	faultString;

	LLXMLRPCValue error(XMLRPC_RequestGetError(mImpl->mResponse));
	if (error.isValid())
	{
		hasError = true;
		faultCode = error["faultCode"].asInt();
		faultString = error["faultString"].asString();
	}
	else if (XMLRPC_ResponseIsFault(mImpl->mResponse))
	{
		hasFault = true;
		faultCode = XMLRPC_GetResponseFaultCode(mImpl->mResponse);
		faultString = XMLRPC_GetResponseFaultString(mImpl->mResponse);
	}

	if (hasError || hasFault)
	{
		mImpl->setStatus(LLXMLRPCTransaction::StatusXMLRPCError);

		LL_WARNS() << "LLXMLRPCTransaction XMLRPC "
			<< (hasError ? "error " : "fault ")
			<< faultCode << ": "
			<< faultString << LL_ENDL;
		LL_WARNS() << "LLXMLRPCTransaction request URI: "
			<< mImpl->mURI << LL_ENDL;
	}

}

//=========================================================================

LLXMLRPCTransaction::Impl::Impl(const std::string& uri,
		XMLRPC_REQUEST request, bool useGzip, const LLSD& httpParams)
	: mHttpRequest(),
	  mStatus(LLXMLRPCTransaction::StatusNotStarted),
	  mURI(uri),
	  mResponse(0)
{
	init(request, useGzip, httpParams);
}


LLXMLRPCTransaction::Impl::Impl(const std::string& uri,
		const std::string& method, LLXMLRPCValue params, bool useGzip)
	: mHttpRequest(),
	  mStatus(LLXMLRPCTransaction::StatusNotStarted),
	  mURI(uri),
	  mResponse(0)
{
	XMLRPC_REQUEST request = XMLRPC_RequestNew();
	XMLRPC_RequestSetMethodName(request, method.c_str());
	XMLRPC_RequestSetRequestType(request, xmlrpc_request_call);
	XMLRPC_RequestSetData(request, params.getValue());
	
	init(request, useGzip, LLSD());
    // DEV-28398: without this XMLRPC_RequestFree() call, it looks as though
    // the 'request' object is simply leaked. It's less clear to me whether we
    // should also ask to free request value data (second param 1), since the
    // data come from 'params'.
    XMLRPC_RequestFree(request, 1);
}

void LLXMLRPCTransaction::Impl::init(XMLRPC_REQUEST request, bool useGzip, const LLSD& httpParams)
{
	LLCore::HttpOptions::ptr_t httpOpts;
	LLCore::HttpHeaders::ptr_t httpHeaders;


	if (!mHttpRequest)
	{
		mHttpRequest = LLCore::HttpRequest::ptr_t(new LLCore::HttpRequest);
	}

	// LLRefCounted starts with a 1 ref, so don't add a ref in the smart pointer
	httpOpts = LLCore::HttpOptions::ptr_t(new LLCore::HttpOptions()); 

	// delay between repeats will start from 5 sec and grow to 20 sec with each repeat
	httpOpts->setMinBackoff(5E6L);
	httpOpts->setMaxBackoff(20E6L);

	httpOpts->setTimeout(httpParams.has("timeout") ? httpParams["timeout"].asInteger() : 40L);
	if (httpParams.has("retries"))
	{
		httpOpts->setRetries(httpParams["retries"].asInteger());
	}

	bool vefifySSLCert = !gSavedSettings.getBOOL("NoVerifySSLCert");
	mCertStore = gSavedSettings.getString("CertStore");

	httpOpts->setSSLVerifyPeer( vefifySSLCert );
	httpOpts->setSSLVerifyHost( vefifySSLCert ? 2 : 0);

	// LLRefCounted starts with a 1 ref, so don't add a ref in the smart pointer
	httpHeaders = LLCore::HttpHeaders::ptr_t(new LLCore::HttpHeaders());

	httpHeaders->append(HTTP_OUT_HEADER_CONTENT_TYPE, HTTP_CONTENT_TEXT_XML);

	///* Setting the DNS cache timeout to -1 disables it completely.
	//This might help with bug #503 */
	//httpOpts->setDNSCacheTimeout(-1);

	LLCore::BufferArray::ptr_t body = LLCore::BufferArray::ptr_t(new LLCore::BufferArray());

	// TODO: See if there is a way to serialize to a preallocated buffer I'm 
	// not fond of the copy here.
	int	requestSize(0);
	char * requestText = XMLRPC_REQUEST_ToXML(request, &requestSize);

	body->append(requestText, requestSize);
	
	XMLRPC_Free(requestText);

	mHandler = LLXMLRPCTransaction::Handler::ptr_t(new Handler( mHttpRequest, this ));

	mPostH = mHttpRequest->requestPost(LLCore::HttpRequest::DEFAULT_POLICY_ID, 0, 
		mURI, body.get(), httpOpts, httpHeaders, mHandler);

}


LLXMLRPCTransaction::Impl::~Impl()
{
	if (mResponse)
	{
		XMLRPC_RequestFree(mResponse, 1);
	}
}

bool LLXMLRPCTransaction::Impl::process()
{
	if (!mPostH || !mHttpRequest)
	{
		LL_WARNS() << "transaction failed." << LL_ENDL;
		return true; //failed, quit.
	}

	switch (mStatus)
	{
		case LLXMLRPCTransaction::StatusComplete:
		case LLXMLRPCTransaction::StatusCURLError:
		case LLXMLRPCTransaction::StatusXMLRPCError:
		case LLXMLRPCTransaction::StatusOtherError:
		{
			return true;
		}

		case LLXMLRPCTransaction::StatusNotStarted:
		{
			setStatus(LLXMLRPCTransaction::StatusStarted);
			break;
		}

		default:
			break;
	}

	LLCore::HttpStatus status = mHttpRequest->update(0);

	status = mHttpRequest->getStatus();
	if (!status) 
	{
		return false;
	}

	return false;
}

void LLXMLRPCTransaction::Impl::setStatus(EStatus status,
	const std::string& message, const std::string& uri)
{
	mStatus = status;
	mStatusMessage = message;
	mStatusURI = uri;

	if (mStatusMessage.empty())
	{
		switch (mStatus)
		{
			case StatusNotStarted:
				mStatusMessage = "(not started)";
				break;
				
			case StatusStarted:
				mStatusMessage = "(waiting for server response)";
				break;
				
			case StatusDownloading:
				mStatusMessage = "(reading server response)";
				break;
				
			case StatusComplete:
				mStatusMessage = "(done)";
				break;
			default:
				// Usually this means that there's a problem with the login server,
				// not with the client.  Direct user to status page.
				mStatusMessage = LLTrans::getString("server_is_down");
				mStatusURI = "http://status.secondlifegrid.net/";
		}
	}
}

void LLXMLRPCTransaction::Impl::setHttpStatus(const LLCore::HttpStatus &status)
{
	CURLcode code = static_cast<CURLcode>(status.toULong());
	std::string message;
	std::string uri = "http://secondlife.com/community/support.php";
	LLURI failuri(mURI);


	switch (code)
	{
	case CURLE_COULDNT_RESOLVE_HOST:
		message =
			std::string("DNS could not resolve the host name(") + failuri.hostName() + ").\n"
			"Please verify that you can connect to the www.secondlife.com\n"
			"web site.  If you can, but continue to receive this error,\n"
			"please go to the support section and report this problem.";
		break;

	case CURLE_SSL_PEER_CERTIFICATE:
		message =
			"The login server couldn't verify itself via SSL.\n"
			"If you continue to receive this error, please go\n"
			"to the Support section of the SecondLife.com web site\n"
			"and report the problem.";
		break;

	case CURLE_SSL_CACERT:
	case CURLE_SSL_CONNECT_ERROR:
		message =
			"Often this means that your computer\'s clock is set incorrectly.\n"
			"Please go to Control Panels and make sure the time and date\n"
			"are set correctly.\n"
			"Also check that your network and firewall are set up correctly.\n"
			"If you continue to receive this error, please go\n"
			"to the Support section of the SecondLife.com web site\n"
			"and report the problem.";
		break;

	default:
		break;
	}

	mCurlCode = code;
	setStatus(StatusCURLError, message, uri);

}


LLXMLRPCTransaction::LLXMLRPCTransaction(
	const std::string& uri, XMLRPC_REQUEST request, bool useGzip, const LLSD& httpParams)
: impl(* new Impl(uri, request, useGzip, httpParams))
{ }


LLXMLRPCTransaction::LLXMLRPCTransaction(
	const std::string& uri,
	const std::string& method, LLXMLRPCValue params, bool useGzip)
: impl(* new Impl(uri, method, params, useGzip))
{ }

LLXMLRPCTransaction::~LLXMLRPCTransaction()
{
	delete &impl;
}

bool LLXMLRPCTransaction::process()
{
	return impl.process();
}

LLXMLRPCTransaction::EStatus LLXMLRPCTransaction::status(int* curlCode)
{
	if (curlCode)
	{
		*curlCode = 
			(impl.mStatus == StatusCURLError)
				? impl.mCurlCode
				: CURLE_OK;
	}
		
	return impl.mStatus;
}

std::string LLXMLRPCTransaction::statusMessage()
{
	return impl.mStatusMessage;
}

LLSD LLXMLRPCTransaction::getErrorCertData()
{
	return impl.mErrorCertData;
}

std::string LLXMLRPCTransaction::statusURI()
{
	return impl.mStatusURI;
}

XMLRPC_REQUEST LLXMLRPCTransaction::response()
{
	return impl.mResponse;
}

LLXMLRPCValue LLXMLRPCTransaction::responseValue()
{
	return LLXMLRPCValue(XMLRPC_RequestGetData(impl.mResponse));
}


F64 LLXMLRPCTransaction::transferRate()
{
	if (impl.mStatus != StatusComplete)
	{
		return 0.0L;
	}
	
	double rate_bits_per_sec = impl.mTransferStats->mSpeedDownload * 8.0;
	
	LL_INFOS("AppInit") << "Buffer size:   " << impl.mResponseText.size() << " B" << LL_ENDL;
	LL_DEBUGS("AppInit") << "Transfer size: " << impl.mTransferStats->mSizeDownload << " B" << LL_ENDL;
	LL_DEBUGS("AppInit") << "Transfer time: " << impl.mTransferStats->mTotalTime << " s" << LL_ENDL;
	LL_INFOS("AppInit") << "Transfer rate: " << rate_bits_per_sec / 1000.0 << " Kb/s" << LL_ENDL;

	return rate_bits_per_sec;
}
