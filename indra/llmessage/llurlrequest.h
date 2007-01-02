/** 
 * @file llurlrequest.h
 * @author Phoenix
 * @date 2005-04-21
 * @brief Declaration of url based requests on pipes.
 *
 * Copyright (c) 2005-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLURLREQUEST_H
#define LL_LLURLREQUEST_H

/** 
 * This file holds the declaration of useful classes for dealing with
 * url based client requests. 
 */

#include <string>
#include "lliopipe.h"
#include "llchainio.h"

class LLURLRequestDetail;

class LLURLRequestComplete;

/** 
 * @class LLURLRequest
 * @brief Class to handle url based requests.
 * @see LLIOPipe
 *
 * Currently, this class is implemented on top of curl. From the
 * vantage of a programmer using this class, you do not care so much,
 * but it's useful to know since in order to accomplish 'non-blocking'
 * behavior, we have to use a more expensive curl interface which can
 * still block if the server enters a half-accepted state. It would be
 * worth the time and effort to eventually port this to a raw client
 * socket.
 */
class LLURLRequest : public LLIOPipe
{
public:
	/** 
	 * @brief This enumeration is for specifying the type of request.
	 */
	enum ERequestAction
	{
		INVALID,
		HTTP_GET,
		HTTP_PUT,
		HTTP_POST,
		HTTP_DELETE,
		REQUEST_ACTION_COUNT
	};

	/** 
	 * @brief Constructor.
	 *
	 * @param action One of the ERequestAction enumerations.
	 */
	LLURLRequest(ERequestAction action);

	/** 
	 * @brief Constructor.
	 *
	 * @param action One of the ERequestAction enumerations.
	 * @param url The url of the request. It should already be encoded.
	 */
	LLURLRequest(ERequestAction action, const std::string& url);

	/** 
	 * @brief Destructor.
	 */
	virtual ~LLURLRequest();

	/* @name Instance methods
	 */
	//@{
	/** 
	 * @brief Set the url for the request
	 *
	 * This method assumes the url is encoded appropriately for the
	 * request. 
	 * The url must be set somehow before the first call to process(),
	 * or the url will not be set correctly.
	 * 
	 */
	void setURL(const std::string& url);

	/** 
	 * @brief Add a header to the http post.
	 *
	 * The header must be correctly formatted for HTTP requests. This
	 * provides a raw interface if you know what kind of request you
	 * will be making during construction of this instance. All
	 * required headers will be automatically constructed, so this is
	 * usually useful for encoding parameters.
	 */
	void addHeader(const char* header);

	/**
	 * @brief Check remote server certificate signed by a known root CA.
	 *
	 * Set whether request will check that remote server
	 * certificates are signed by a known root CA when using HTTPS.
	 * Use the supplied root certificate bundle if supplied, else use
	 * the standard bundle as found by libcurl and openssl.
	 */
	void checkRootCertificate(bool check, const char* caBundle = NULL);

	/** 
	 * @brief Request a particular response encoding if available.
	 *
	 * This call is a shortcut for requesting a particular encoding
	 * from the server, eg, 'gzip'. 
	 */
	void requestEncoding(const char* encoding);

	/**
	 * @brief Return at most size bytes of body.
	 *
	 * If the body had more bytes than this limit, they will not be
	 * returned and the connection closed.  In this case, STATUS_STOP
	 * will be passed to responseStatus();
	 */
	void setBodyLimit(U32 size);

	/** 
	 * @brief Set a completion callback for this URLRequest.
	 *
	 * The callback is added to this URLRequet's pump when either the
	 * entire buffer is known or an error like timeout or connection
	 * refused has happened. In the case of a complete transfer, this
	 * object builds a response chain such that the callback and the
	 * next process consumer get to read the output.
	 *
	 * This setup is a little fragile since the url request consumer
	 * might not just read the data - it may do a channel change,
	 * which invalidates the input to the callback, but it works well
	 * in practice.
	 */
	void setCallback(LLURLRequestComplete* callback);
	//@}

	/**
	 * @ brief Set certificate authority file used to verify HTTPS certs.
	 */
	static void setCertificateAuthorityFile(const std::string& file_name);

	/**
	 * @ brief Set certificate authority path used to verify HTTPS certs.
	 */
	static void setCertificateAuthorityPath(const std::string& path);

	/* @name LLIOPipe virtual implementations
	 */
public:
	/** 
	 * @brief Give this pipe a chance to handle a generated error
	 */
	virtual EStatus handleError(EStatus status, LLPumpIO* pump);
	
protected:
	/** 
	 * @brief Process the data in buffer
	 */
	virtual EStatus process_impl(
		const LLChannelDescriptors& channels,
		buffer_ptr_t& buffer,
		bool& eos,
		LLSD& context,
		LLPumpIO* pump);
	//@}

protected:
	enum EState
	{
		STATE_INITIALIZED,
		STATE_WAITING_FOR_RESPONSE,
		STATE_PROCESSING_RESPONSE,
		STATE_HAVE_RESPONSE,
	};
	EState mState;
	ERequestAction mAction;
	LLURLRequestDetail* mDetail;
	LLIOPipe::ptr_t mCompletionCallback;

private:
	/** 
	 * @brief Initialize the object. Called during construction.
	 */
	void initialize();

	/** 
	 * @brief Handle action specific url request configuration.
	 *
	 * @return Returns true if this is configured.
	 */
	bool configure();

	/** 
	 * @brief Download callback method.
	 */
 	static size_t downCallback(
		void* data,
		size_t size,
		size_t nmemb,
		void* user);

	/** 
	 * @brief Upload callback method.
	 */
 	static size_t upCallback(
		void* data,
		size_t size,
		size_t nmemb,
		void* user);

	/** 
	 * @brief Declaration of unimplemented method to prevent copy
	 * construction.
	 */
	LLURLRequest(const LLURLRequest&);
};


/** 
 * @class LLContextURLExtractor
 * @brief This class unpacks the url out of a agent usher service so
 * it can be packed into a LLURLRequest object.
 * @see LLIOPipe
 *
 * This class assumes that the context is a map that contains an entry
 * named CONTEXT_DEST_URI_SD_LABEL.
 */
class LLContextURLExtractor : public LLIOPipe
{
public:
	LLContextURLExtractor(LLURLRequest* req) : mRequest(req) {}
	~LLContextURLExtractor() {}

protected:
	/* @name LLIOPipe virtual implementations
	 */
	//@{
	/** 
	 * @brief Process the data in buffer
	 */
	virtual EStatus process_impl(
		const LLChannelDescriptors& channels,
		buffer_ptr_t& buffer,
		bool& eos,
		LLSD& context,
		LLPumpIO* pump);
	//@}

protected:
	LLURLRequest* mRequest;
};


/** 
 * @class LLURLRequestComplete
 * @brief Class which can optionally be used with an LLURLRequest to
 * get notification when the url request is complete.
 */
class LLURLRequestComplete : public LLIOPipe
{
public:
	
	virtual void header(const std::string& header, const std::string& value);
	    ///< Called once for each header received, prior to httpStatus

	virtual void httpStatus(U32 status, const std::string& reason);
	    ///< Always called on request completion, prior to complete

	virtual void complete(
		const LLChannelDescriptors& channels,
		const buffer_ptr_t& buffer);

	/** 
	 * @brief This method is called when we got a valid response.
	 *
	 * It is up to class implementers to do something useful here.
	 */
	virtual void response(
		const LLChannelDescriptors& channels,
		const buffer_ptr_t& buffer);

	/** 
	 * @brief This method is called if there was no response.
	 *
	 * It is up to class implementers to do something useful here.
	 */
	virtual void noResponse();

	/** 
	 * @brief This method will be called by the LLURLRequest object.
	 *
	 * If this is set to STATUS_OK or STATUS_STOP, then the transfer
	 * is asssumed to have worked. This will lead to calling response()
	 * on the next call to process(). Otherwise, this object will call
	 * noResponse() on the next call to process.
	 * @param status The status of the URLRequest.
	 */
	void responseStatus(EStatus status);

	// constructor & destructor.
	LLURLRequestComplete();
	virtual ~LLURLRequestComplete();

protected:
	/* @name LLIOPipe virtual implementations
	 */
	//@{
	/** 
	 * @brief Process the data in buffer
	 */
	virtual EStatus process_impl(
		const LLChannelDescriptors& channels,
		buffer_ptr_t& buffer,
		bool& eos,
		LLSD& context,
		LLPumpIO* pump);
	//@}

	// value to note if we actually got the response. This value
	// depends on correct useage from the LLURLRequest instance.
	EStatus mRequestStatus;
};


/** 
 * @class LLURLRequestClientFactory
 * @brief Template class to build url request based client chains 
 *
 * This class eases construction of a basic sd rpc client. Here is an
 * example of it's use:
 * <code>
 *  class LLUsefulService : public LLService { ... }<br>
 *  LLService::registerCreator(<br>
 *    "useful",<br>
 *    LLService::creator_t(new LLURLRequestClientFactory<LLUsefulService>))<br>
 * </code>
 *
 * This class should work, but I never got around to using/testing it.
 *
 */
#if 0
template<class Client>
class LLURLRequestClientFactory : public LLChainIOFactory
{
public:
	LLURLRequestClientFactory(LLURLRequest::ERequestAction action) {}
	LLURLRequestClientFactory(
		LLURLRequest::ERequestAction action,
		const std::string& fixed_url) :
		mAction(action),
		mURL(fixed_url)
	{
	}
	virtual bool build(LLPumpIO::chain_t& chain, LLSD context) const
	{
		lldebugs << "LLURLRequestClientFactory::build" << llendl;
		LLIOPipe::ptr_t service(new Client);
		chain.push_back(service);
		LLURLRequest* http(new LLURLRequest(mAction));
		LLIOPipe::ptr_t http_pipe(http);
		// *FIX: how do we know the content type?
		//http->addHeader("Content-Type: text/llsd");
		if(mURL.empty())
		{
			chain.push_back(LLIOPipe::ptr_t(new LLContextURLExtractor(http)));
		}
		else
		{
			http->setURL(mURL);
		}
		chain.push_back(http_pipe);
		chain.push_back(service);
		return true;
	}

protected:
	LLURLRequest::ERequestAction mAction;
	std::string mURL;
};
#endif

/**
 * External constants
 */
extern const std::string CONTEXT_DEST_URI_SD_LABEL;

#endif // LL_LLURLREQUEST_H
