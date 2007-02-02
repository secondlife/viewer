/** 
 * @file llxmlrpctransaction.cpp
 * @brief LLXMLRPCTransaction and related class implementations 
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "llxmlrpctransaction.h"

#include "llviewercontrol.h"

// Have to include these last to avoid queue redefinition!
#include <curl/curl.h>
#include <xmlrpc-epi/xmlrpc.h>

#include "viewer.h"

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

void LLXMLRPCValue::free()
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
	typedef LLXMLRPCTransaction::Status	Status;
	
	CURL*	mCurl;
	CURLM*	mCurlMulti;

	Status		mStatus;
	CURLcode	mCurlCode;
	std::string	mStatusMessage;
	std::string	mStatusURI;
	
	char				mCurlErrorBuffer[CURL_ERROR_SIZE];		/* Flawfinder: ignore */

	std::string			mURI;
	char*				mRequestText;
	int					mRequestTextSize;
	
	std::string			mProxyAddress;
	struct curl_slist*	mHeaders;

	std::string			mResponseText;
	XMLRPC_REQUEST		mResponse;
	
	Impl(const std::string& uri, XMLRPC_REQUEST request, bool useGzip);
	Impl(const std::string& uri,
		const std::string& method, LLXMLRPCValue params, bool useGzip);
	~Impl();
	
	bool process();
	
	void setStatus(Status code,
		const std::string& message = "", const std::string& uri = "");
	void setCurlStatus(CURLcode);

private:
	void init(XMLRPC_REQUEST request, bool useGzip);

	static size_t curlDownloadCallback(
		void* data, size_t size, size_t nmemb, void* user_data);
};

LLXMLRPCTransaction::Impl::Impl(const std::string& uri,
		XMLRPC_REQUEST request, bool useGzip)
	: mCurl(0), mCurlMulti(0),
	  mStatus(LLXMLRPCTransaction::StatusNotStarted),
	  mURI(uri),
	  mRequestText(0), mHeaders(0),
	  mResponse(0)
{
	init(request, useGzip);
}


LLXMLRPCTransaction::Impl::Impl(const std::string& uri,
		const std::string& method, LLXMLRPCValue params, bool useGzip)
	: mCurl(0), mCurlMulti(0),
	  mStatus(LLXMLRPCTransaction::StatusNotStarted),
	  mURI(uri),
	  mRequestText(0), mHeaders(0),
	  mResponse(0)
{
	XMLRPC_REQUEST request = XMLRPC_RequestNew();
	XMLRPC_RequestSetMethodName(request, method.c_str());
	XMLRPC_RequestSetRequestType(request, xmlrpc_request_call);
	XMLRPC_RequestSetData(request, params.getValue());
	
	init(request, useGzip);
}




void LLXMLRPCTransaction::Impl::init(XMLRPC_REQUEST request, bool useGzip)
{
	mCurl = curl_easy_init();

	if (gSavedSettings.getBOOL("BrowserProxyEnabled"))
	{
		mProxyAddress = gSavedSettings.getString("BrowserProxyAddress");
		S32 port = gSavedSettings.getS32 ( "BrowserProxyPort" );

		// tell curl about the settings
		curl_easy_setopt(mCurl, CURLOPT_PROXY, mProxyAddress.c_str());
		curl_easy_setopt(mCurl, CURLOPT_PROXYPORT, (long)port);
		curl_easy_setopt(mCurl, CURLOPT_PROXYTYPE, CURLPROXY_HTTP);
	};

//	curl_easy_setopt(mCurl, CURLOPT_VERBOSE, 1); // usefull for debugging
	curl_easy_setopt(mCurl, CURLOPT_NOSIGNAL, 1);
	curl_easy_setopt(mCurl, CURLOPT_WRITEFUNCTION, &curlDownloadCallback);
	curl_easy_setopt(mCurl, CURLOPT_WRITEDATA, this);
	curl_easy_setopt(mCurl, CURLOPT_ERRORBUFFER, &mCurlErrorBuffer);
	curl_easy_setopt(mCurl, CURLOPT_CAINFO, gDirUtilp->getCAFile().c_str());
	curl_easy_setopt(mCurl, CURLOPT_SSL_VERIFYPEER, gVerifySSLCert);

	/* Setting the DNS cache timeout to -1 disables it completely.
	   This might help with bug #503 */
	curl_easy_setopt(mCurl, CURLOPT_DNS_CACHE_TIMEOUT, -1);

    mHeaders = curl_slist_append(mHeaders, "Content-Type: text/xml");
	curl_easy_setopt(mCurl, CURLOPT_URL, mURI.c_str());
	curl_easy_setopt(mCurl, CURLOPT_HTTPHEADER, mHeaders);
	if (useGzip)
	{
		curl_easy_setopt(mCurl, CURLOPT_ENCODING, "");
	}
	
	mRequestText = XMLRPC_REQUEST_ToXML(request, &mRequestTextSize);
	if (mRequestText)
	{
		curl_easy_setopt(mCurl, CURLOPT_POSTFIELDS, mRequestText);
		curl_easy_setopt(mCurl, CURLOPT_POSTFIELDSIZE, mRequestTextSize);
	}
	else
	{
		setStatus(StatusOtherError);
	}
	
	mCurlMulti = curl_multi_init();
	curl_multi_add_handle(mCurlMulti, mCurl);
}


LLXMLRPCTransaction::Impl::~Impl()
{
	if (mResponse)
	{
		XMLRPC_RequestFree(mResponse, 1);
	}
	
	if (mHeaders)
	{
		curl_slist_free_all(mHeaders);
	}
	
	if (mRequestText)
	{
		XMLRPC_Free(mRequestText);
	}
	
	if (mCurl)
	{
		if (mCurlMulti)
		{
			curl_multi_remove_handle(mCurlMulti, mCurl);
		}
		curl_easy_cleanup(mCurl);
	}
	
	if (mCurlMulti)
	{
		curl_multi_cleanup(mCurlMulti);
	}
	
}

bool LLXMLRPCTransaction::Impl::process()
{
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
	
	const F32 MAX_PROCESSING_TIME = 0.05f;
	LLTimer timer;
	int count;
	
	while (CURLM_CALL_MULTI_PERFORM == curl_multi_perform(mCurlMulti, &count))
	{
		if (timer.getElapsedTimeF32() >= MAX_PROCESSING_TIME)
		{
			return false;
		}
	}
			 
	while(CURLMsg* curl_msg = curl_multi_info_read(mCurlMulti, &count))
	{
		if (CURLMSG_DONE == curl_msg->msg)
		{
			if (curl_msg->data.result != CURLE_OK)
			{
				setCurlStatus(curl_msg->data.result);
				llalerts << "LLXMLRPCTransaction CURL error "
					<< mCurlCode << ": " << mCurlErrorBuffer << llendl;
				llalerts << "LLXMLRPCTransaction request URI: "
					<< mURI << llendl;
					
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
				
				llalerts << "LLXMLRPCTransaction XMLRPC "
					<< (hasError ? "error " : "fault ")
					<< faultCode << ": "
					<< faultString << llendl;
				llalerts << "LLXMLRPCTransaction request URI: "
					<< mURI << llendl;
			}
			
			return true;
		}
	}
	
	return false;
}

void LLXMLRPCTransaction::Impl::setStatus(Status status,
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
				// not with the client.  Direct user to status page. JC
				mStatusMessage =
					"Despite our best efforts, something unexpected has gone wrong. \n"
					" \n"
					"Please check www.secondlife.com/status and the Second Life \n"
					"Announcements forum to see if there is a known problem with \n"
					"the service.";
				mStatusURI = "http://secondlife.com/status/";
				/*
				mStatusMessage =
					"Despite our best efforts, something unexpected has gone wrong.\n"
					"Please go to the Support section of the SecondLife.com web site\n"
					"and report the problem.  If possible, include your SecondLife.log\n"
					"file from:\n"
#if LL_WINDOWS
					"C:\\Documents and Settings\\<name>\\Application Data\\SecondLife\\logs\n"
#elif LL_DARWIN
					"~/Library/Application Support/SecondLife/logs\n"
#elif LL_LINUX
					"~/.secondlife/logs\n"
#else
#error "Need platform here."
#endif
					"Thank you.";
				mStatusURI = "http://secondlife.com/community/support.php";
				*/
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
				"\n"
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
		void* data, size_t size, size_t nmemb, void* user_data)
{
	Impl& impl(*(Impl*)user_data);
	
	size_t n = size * nmemb;

	impl.mResponseText.append((const char*)data, n);
	
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

LLXMLRPCTransaction::Status LLXMLRPCTransaction::status(int* curlCode)
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
	if (!impl.mCurl  ||  impl.mStatus != StatusComplete)
	{
		return 0.0L;
	}
	
	double size_bytes = 0.0;
	double time_seconds = 0.0;
	double rate_bytes_per_sec = 0.0;

	curl_easy_getinfo(impl.mCurl, CURLINFO_SIZE_DOWNLOAD, &size_bytes);
	curl_easy_getinfo(impl.mCurl, CURLINFO_TOTAL_TIME, &time_seconds);
	curl_easy_getinfo(impl.mCurl, CURLINFO_SPEED_DOWNLOAD, &rate_bytes_per_sec);

	double rate_bits_per_sec = rate_bytes_per_sec * 8.0;
	
	llinfos << "Buffer size:   " << impl.mResponseText.size() << " B" << llendl;
	llinfos << "Transfer size: " << size_bytes << " B" << llendl;
	llinfos << "Transfer time: " << time_seconds << " s" << llendl;
	llinfos << "Transfer rate: " << rate_bits_per_sec/1000.0 << " Kb/s" << llendl;

	return rate_bits_per_sec;
}
