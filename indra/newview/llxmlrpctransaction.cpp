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
#include <openssl/x509_vfy.h>
#include <openssl/ssl.h>
#include "llsecapi.h"

#include "llxmlrpctransaction.h"
#include "llxmlrpclistener.h"

#include "llcurl.h"
#include "llviewercontrol.h"

// Have to include these last to avoid queue redefinition!
#include <xmlrpc-epi/xmlrpc.h>

#include "llappviewer.h"
#include "lltrans.h"

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


class LLXMLRPCTransaction::Impl
{
public:
	typedef LLXMLRPCTransaction::EStatus	EStatus;

	LLCurlEasyRequest* mCurlRequest;

	EStatus		mStatus;
	CURLcode	mCurlCode;
	std::string	mStatusMessage;
	std::string	mStatusURI;
	LLCurl::TransferInfo mTransferInfo;
	
	std::string			mURI;
	char*				mRequestText;
	int					mRequestTextSize;
	
	std::string			mProxyAddress;

	std::string			mResponseText;
	XMLRPC_REQUEST		mResponse;
	std::string         mCertStore;
	LLPointer<LLCertificate> mErrorCert;
	
	Impl(const std::string& uri, XMLRPC_REQUEST request, bool useGzip);
	Impl(const std::string& uri,
		 const std::string& method, LLXMLRPCValue params, bool useGzip);
	~Impl();
	
	bool process();
	
	void setStatus(EStatus code,
				   const std::string& message = "", const std::string& uri = "");
	void setCurlStatus(CURLcode);

private:
	void init(XMLRPC_REQUEST request, bool useGzip);
	static int _sslCertVerifyCallback(X509_STORE_CTX *ctx, void *param);
	static CURLcode _sslCtxFunction(CURL * curl, void *sslctx, void *param);
	static size_t curlDownloadCallback(
		char* data, size_t size, size_t nmemb, void* user_data);
};

LLXMLRPCTransaction::Impl::Impl(const std::string& uri,
		XMLRPC_REQUEST request, bool useGzip)
	: mCurlRequest(0),
	  mStatus(LLXMLRPCTransaction::StatusNotStarted),
	  mURI(uri),
	  mRequestText(0), 
	  mResponse(0)
{
	init(request, useGzip);
}


LLXMLRPCTransaction::Impl::Impl(const std::string& uri,
		const std::string& method, LLXMLRPCValue params, bool useGzip)
	: mCurlRequest(0),
	  mStatus(LLXMLRPCTransaction::StatusNotStarted),
	  mURI(uri),
	  mRequestText(0), 
	  mResponse(0)
{
	XMLRPC_REQUEST request = XMLRPC_RequestNew();
	XMLRPC_RequestSetMethodName(request, method.c_str());
	XMLRPC_RequestSetRequestType(request, xmlrpc_request_call);
	XMLRPC_RequestSetData(request, params.getValue());
	
	init(request, useGzip);
    // DEV-28398: without this XMLRPC_RequestFree() call, it looks as though
    // the 'request' object is simply leaked. It's less clear to me whether we
    // should also ask to free request value data (second param 1), since the
    // data come from 'params'.
    XMLRPC_RequestFree(request, 1);
}

// _sslCertVerifyCallback
// callback called when a cert verification is requested.
// calls SECAPI to validate the context
int LLXMLRPCTransaction::Impl::_sslCertVerifyCallback(X509_STORE_CTX *ctx, void *param)
{
	LLXMLRPCTransaction::Impl *transaction = (LLXMLRPCTransaction::Impl *)param;
	LLPointer<LLCertificateStore> store = gSecAPIHandler->getCertificateStore(transaction->mCertStore);
	LLPointer<LLCertificateChain> chain = gSecAPIHandler->getCertificateChain(ctx);
	LLSD validation_params = LLSD::emptyMap();
	LLURI uri(transaction->mURI);
	validation_params[CERT_HOSTNAME] = uri.hostName();
	try
	{
		// don't validate hostname.  Let libcurl do it instead.  That way, it'll handle redirects
		store->validate(VALIDATION_POLICY_SSL & (~VALIDATION_POLICY_HOSTNAME), chain, validation_params);
	}
	catch (LLCertValidationTrustException& cert_exception)
	{
		// this exception is is handled differently than the general cert
		// exceptions, as we allow the user to actually add the certificate
		// for trust.
		// therefore we pass back a different error code
		// NOTE: We're currently 'wired' to pass around CURL error codes.  This is
		// somewhat clumsy, as we may run into errors that do not map directly to curl
		// error codes.  Should be refactored with login refactoring, perhaps.
		transaction->mCurlCode = CURLE_SSL_CACERT;
		// set the status directly.  set curl status generates error messages and we want
		// to use the fixed ones from the exceptions
		transaction->setStatus(StatusCURLError, cert_exception.getMessage(), std::string());
		// We should probably have a more generic way of passing information
		// back to the error handlers.
		transaction->mErrorCert = cert_exception.getCert();
		return 0;		
	}
	catch (LLCertException& cert_exception)
	{
		transaction->mCurlCode = CURLE_SSL_PEER_CERTIFICATE;
		// set the status directly.  set curl status generates error messages and we want
		// to use the fixed ones from the exceptions
		transaction->setStatus(StatusCURLError, cert_exception.getMessage(), std::string());
		transaction->mErrorCert = cert_exception.getCert();
		return 0;
	}
	catch (...)
	{
		// any other odd error, we just handle as a connect error.
		transaction->mCurlCode = CURLE_SSL_CONNECT_ERROR;
		transaction->setCurlStatus(CURLE_SSL_CONNECT_ERROR);
		return 0;
	}
	return 1;
}

// _sslCtxFunction
// Callback function called when an SSL Context is created via CURL
// used to configure the context for custom cert validate(<, <#const & xs#>, <#T * #>, <#long #>)tion
// based on SECAPI

CURLcode LLXMLRPCTransaction::Impl::_sslCtxFunction(CURL * curl, void *sslctx, void *param)
{
	SSL_CTX * ctx = (SSL_CTX *) sslctx;
	// disable any default verification for server certs
	SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, NULL);
	// set the verification callback.
	SSL_CTX_set_cert_verify_callback(ctx, _sslCertVerifyCallback, param);
	// the calls are void
	return CURLE_OK;
	
}

void LLXMLRPCTransaction::Impl::init(XMLRPC_REQUEST request, bool useGzip)
{
	if (!mCurlRequest)
	{
		mCurlRequest = new LLCurlEasyRequest();
	}
	if(!mCurlRequest->isValid())
	{
		llwarns << "mCurlRequest is invalid." << llendl ;

		delete mCurlRequest ;
		mCurlRequest = NULL ;
		return ;
	}

	mErrorCert = NULL;

//	mCurlRequest->setopt(CURLOPT_VERBOSE, 1); // useful for debugging
	mCurlRequest->setopt(CURLOPT_NOSIGNAL, 1);
	mCurlRequest->setWriteCallback(&curlDownloadCallback, (void*)this);
	BOOL vefifySSLCert = !gSavedSettings.getBOOL("NoVerifySSLCert");
	mCertStore = gSavedSettings.getString("CertStore");
	mCurlRequest->setopt(CURLOPT_SSL_VERIFYPEER, vefifySSLCert);
	mCurlRequest->setopt(CURLOPT_SSL_VERIFYHOST, vefifySSLCert ? 2 : 0);
	// Be a little impatient about establishing connections.
	mCurlRequest->setopt(CURLOPT_CONNECTTIMEOUT, 40L);
	mCurlRequest->setSSLCtxCallback(_sslCtxFunction, (void *)this);

	/* Setting the DNS cache timeout to -1 disables it completely.
	   This might help with bug #503 */
	mCurlRequest->setopt(CURLOPT_DNS_CACHE_TIMEOUT, -1);

    mCurlRequest->slist_append("Content-Type: text/xml");

	if (useGzip)
	{
		mCurlRequest->setoptString(CURLOPT_ENCODING, "");
	}
	
	mRequestText = XMLRPC_REQUEST_ToXML(request, &mRequestTextSize);
	if (mRequestText)
	{
		mCurlRequest->setoptString(CURLOPT_POSTFIELDS, mRequestText);
		mCurlRequest->setopt(CURLOPT_POSTFIELDSIZE, mRequestTextSize);
	}
	else
	{
		setStatus(StatusOtherError);
	}

	mCurlRequest->sendRequest(mURI);
}


LLXMLRPCTransaction::Impl::~Impl()
{
	if (mResponse)
	{
		XMLRPC_RequestFree(mResponse, 1);
	}
	
	if (mRequestText)
	{
		XMLRPC_Free(mRequestText);
	}
	
	delete mCurlRequest;
	mCurlRequest = NULL ;
}

bool LLXMLRPCTransaction::Impl::process()
{
	if(!mCurlRequest || !mCurlRequest->isValid())
	{
		llwarns << "transaction failed." << llendl ;

		delete mCurlRequest ;
		mCurlRequest = NULL ;
		return true ; //failed, quit.
	}

	switch(mStatus)
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
		{
			// continue onward
		}
	}
		
	if(!mCurlRequest->wait())
	{
		return false ;
	}

	while(1)
	{
		CURLcode result;
		bool newmsg = mCurlRequest->getResult(&result, &mTransferInfo);
		if (newmsg)
		{
			if (result != CURLE_OK)
			{
				if ((result != CURLE_SSL_PEER_CERTIFICATE) &&
					(result != CURLE_SSL_CACERT))
				{
					// if we have a curl error that's not already been handled
					// (a non cert error), then generate the error message as
					// appropriate
					setCurlStatus(result);
				
					llwarns << "LLXMLRPCTransaction CURL error "
					<< mCurlCode << ": " << mCurlRequest->getErrorString() << llendl;
					llwarns << "LLXMLRPCTransaction request URI: "
					<< mURI << llendl;
				}
					
				return true;
			}
			
			setStatus(LLXMLRPCTransaction::StatusComplete);

			mResponse = XMLRPC_REQUEST_FromXML(
					mResponseText.data(), mResponseText.size(), NULL);

			bool		hasError = false;
			bool		hasFault = false;
			int			faultCode = 0;
			std::string	faultString;

			LLXMLRPCValue error(XMLRPC_RequestGetError(mResponse));
			if (error.isValid())
			{
				hasError = true;
				faultCode = error["faultCode"].asInt();
				faultString = error["faultString"].asString();
			}
			else if (XMLRPC_ResponseIsFault(mResponse))
			{
				hasFault = true;
				faultCode = XMLRPC_GetResponseFaultCode(mResponse);
				faultString = XMLRPC_GetResponseFaultString(mResponse);
			}

			if (hasError || hasFault)
			{
				setStatus(LLXMLRPCTransaction::StatusXMLRPCError);
				
				llwarns << "LLXMLRPCTransaction XMLRPC "
						<< (hasError ? "error " : "fault ")
						<< faultCode << ": "
						<< faultString << llendl;
				llwarns << "LLXMLRPCTransaction request URI: "
						<< mURI << llendl;
			}
			
			return true;
		}
		else
		{
			break; // done
		}
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

void LLXMLRPCTransaction::Impl::setCurlStatus(CURLcode code)
{
	std::string message;
	std::string uri = "http://secondlife.com/community/support.php";
	
	switch (code)
	{
		case CURLE_COULDNT_RESOLVE_HOST:
			message =
				"DNS could not resolve the host name.\n"
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

size_t LLXMLRPCTransaction::Impl::curlDownloadCallback(
		char* data, size_t size, size_t nmemb, void* user_data)
{
	Impl& impl(*(Impl*)user_data);
	
	size_t n = size * nmemb;

	impl.mResponseText.append(data, n);
	
	if (impl.mStatus == LLXMLRPCTransaction::StatusStarted)
	{
		impl.setStatus(LLXMLRPCTransaction::StatusDownloading);
	}
	
	return n;
}


LLXMLRPCTransaction::LLXMLRPCTransaction(
	const std::string& uri, XMLRPC_REQUEST request, bool useGzip)
: impl(* new Impl(uri, request, useGzip))
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

LLPointer<LLCertificate> LLXMLRPCTransaction::getErrorCert()
{
	return impl.mErrorCert;
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
	
	double rate_bits_per_sec = impl.mTransferInfo.mSpeedDownload * 8.0;
	
	LL_INFOS("AppInit") << "Buffer size:   " << impl.mResponseText.size() << " B" << LL_ENDL;
	LL_DEBUGS("AppInit") << "Transfer size: " << impl.mTransferInfo.mSizeDownload << " B" << LL_ENDL;
	LL_DEBUGS("AppInit") << "Transfer time: " << impl.mTransferInfo.mTotalTime << " s" << LL_ENDL;
	LL_INFOS("AppInit") << "Transfer rate: " << rate_bits_per_sec / 1000.0 << " Kb/s" << LL_ENDL;

	return rate_bits_per_sec;
}
