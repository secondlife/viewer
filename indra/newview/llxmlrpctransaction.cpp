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
#include "llversioninfo.h"
#include "llviewercontrol.h"
#include "llxmlnode.h"
#include "stringize.h"

// Have to include these last to avoid queue redefinition!

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

class LLXMLRPCTransaction::Handler : public LLCore::HttpHandler
{
public:
    Handler(LLCore::HttpRequest::ptr_t &request, LLXMLRPCTransaction::Impl *impl);
    virtual ~Handler();

    virtual void onCompleted(LLCore::HttpHandle handle, LLCore::HttpResponse * response);

    typedef std::shared_ptr<LLXMLRPCTransaction::Handler> ptr_t;

private:

    bool parseResponse(LLXMLNodePtr root);
    bool parseValue(LLSD& target, LLXMLNodePtr source);

    LLXMLRPCTransaction::Impl *mImpl;
    LLCore::HttpRequest::ptr_t mRequest;
};

class LLXMLRPCTransaction::Impl
{
public:
    typedef LLXMLRPCTransaction::EStatus    EStatus;

    LLCore::HttpRequest::ptr_t  mHttpRequest;


    EStatus             mStatus;
    CURLcode            mCurlCode;
    std::string         mStatusMessage;
    std::string         mStatusURI;
    LLCore::HttpResponse::TransferStats::ptr_t  mTransferStats;
    Handler::ptr_t      mHandler;
    LLCore::HttpHandle  mPostH;

    std::string         mURI;
    std::string         mProxyAddress;

    std::string         mResponseText;
    LLSD                mResponseData;

    std::string         mCertStore;
    LLSD                mErrorCertData;

    Impl
    (
        const std::string& uri,
        const std::string& method,
        const LLSD& params,
        const LLSD& httpParams
    );

    bool process();

    void setStatus(EStatus code, const std::string& message = "", const std::string& uri = "");
    void setHttpStatus(const LLCore::HttpStatus &status);
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
        mImpl->setHttpStatus(status);
        LLSD errordata = status.getErrorData();
        mImpl->mErrorCertData = errordata;

        if ((status.toULong() != CURLE_SSL_PEER_CERTIFICATE) &&
            (status.toULong() != CURLE_SSL_CACERT))
        {
            // if we have a curl error that's not already been handled
            // (a non cert error), then generate the warning message as
            // appropriate
            LL_WARNS() << "LLXMLRPCTransaction error "
                << status.toHex() << ": " << status.toString() << LL_ENDL;
            LL_WARNS() << "LLXMLRPCTransaction request URI: "
                << mImpl->mURI << LL_ENDL;
        }

        return;
    }

    mImpl->setStatus(LLXMLRPCTransaction::StatusComplete);
    mImpl->mTransferStats = response->getTransferStats();

    // The contents of a buffer array are potentially noncontiguous, so we
    // will need to copy them into an contiguous block of memory for XMLRPC.
    LLCore::BufferArray *body = response->getBody();
    mImpl->mResponseText.resize(body->size());

    body->read(0, mImpl->mResponseText.data(), body->size());

    LLXMLNodePtr root;
    if (!LLXMLNode::parseBuffer(mImpl->mResponseText.data(), body->size(), root, nullptr))
    {
        LL_WARNS() << "Failed parsing XML response; request URI: " << mImpl->mURI << LL_ENDL;
        return;
    }

    if (!parseResponse(root))
        return;

    LL_INFOS() << "XML response parsed successfully; request URI: " << mImpl->mURI << LL_ENDL;
}

struct XMLTreeNode final : public LLSD::TreeNode
{
    XMLTreeNode(const LLXMLNodePtr impl)
        : mImpl(impl)
        , mFirstChild(impl ? create(impl->getFirstChild()) : nullptr)
        , mNextSibling(impl ? create(impl->getNextSibling()) : nullptr)
    {
    }

    static XMLTreeNode* create(LLXMLNodePtr node) { return node ? new XMLTreeNode(node) : nullptr; }

    virtual bool hasName(const LLSD::String& name) const override { return mImpl && mImpl->hasName(name); }
    virtual LLSD::String getTextContents() const override { return mImpl ? mImpl->getTextContents() : LLStringUtil::null; }
    virtual TreeNode* getFirstChild() const override { return mFirstChild.get(); }
    virtual TreeNode* getNextSibling() const override { return mNextSibling.get(); }

private:
    const LLXMLNodePtr mImpl;
    const std::shared_ptr<XMLTreeNode> mFirstChild;
    const std::shared_ptr<XMLTreeNode> mNextSibling;
};

bool LLXMLRPCTransaction::Handler::parseResponse(LLXMLNodePtr root)
{
    // We have alreasy checked in LLXMLNode::parseBuffer()
    // that root contains exactly one child
    if (!root->hasName("methodResponse"))
    {
        LL_WARNS() << "Invalid root element in XML response; request URI: " << mImpl->mURI << LL_ENDL;
        return false;
    }

    LLXMLNodePtr first = root->getFirstChild();
    LLXMLNodePtr second = first->getFirstChild();
    if (!first->getNextSibling() && second && !second->getNextSibling())
    {
        if (first->hasName("fault"))
        {
            LLSD fault;
            if (parseValue(fault, second) &&
                fault.isMap() && fault.has("faultCode") && fault.has("faultString"))
            {
                LL_WARNS() << "Request failed;"
                    << " faultCode: '" << fault.get("faultCode").asString() << "',"
                    << " faultString: '" << fault.get("faultString").asString() << "',"
                    << " request URI: " << mImpl->mURI << LL_ENDL;
                return false;
            }
        }
        else if (first->hasName("params") &&
            second->hasName("param") && !second->getNextSibling())
        {
            LLXMLNodePtr third = second->getFirstChild();
            if (third && !third->getNextSibling() && parseValue(mImpl->mResponseData, third))
            {
                return true;
            }
        }
    }

    LL_WARNS() << "Invalid response format; request URI: " << mImpl->mURI << LL_ENDL;

    return false;
}

bool LLXMLRPCTransaction::Handler::parseValue(LLSD& target, LLXMLNodePtr source)
{
    XMLTreeNode tn(source);
    return target.fromXMLRPCValue(&tn);
}

//=========================================================================

LLXMLRPCTransaction::Impl::Impl
(
    const std::string& uri,
    const std::string& method,
    const LLSD& params,
    const LLSD& http_params
)
    : mHttpRequest()
    , mStatus(LLXMLRPCTransaction::StatusNotStarted)
    , mURI(uri)
{
    LLCore::HttpOptions::ptr_t httpOpts;
    LLCore::HttpHeaders::ptr_t httpHeaders;

    if (!mHttpRequest)
    {
        mHttpRequest = LLCore::HttpRequest::ptr_t(new LLCore::HttpRequest);
    }

    // LLRefCounted starts with a 1 ref, so don't add a ref in the smart pointer
    httpOpts = LLCore::HttpOptions::ptr_t(new LLCore::HttpOptions());

    // Delay between repeats will start from 5 sec and grow to 20 sec with each repeat
    httpOpts->setMinBackoff((LLCore::HttpTime)5E6L);
    httpOpts->setMaxBackoff((LLCore::HttpTime)20E6L);

    httpOpts->setTimeout(http_params.has("timeout") ? http_params["timeout"].asInteger() : 40L);
    if (http_params.has("retries"))
    {
        httpOpts->setRetries(http_params["retries"].asInteger());
    }
    if (http_params.has("DNSCacheTimeout"))
    {
        httpOpts->setDNSCacheTimeout(http_params["DNSCacheTimeout"].asInteger());
    }

    bool vefifySSLCert = !gSavedSettings.getBOOL("NoVerifySSLCert");
    mCertStore = gSavedSettings.getString("CertStore");

    httpOpts->setSSLVerifyPeer(vefifySSLCert);
    httpOpts->setSSLVerifyHost(vefifySSLCert ? 2 : 0);

    // LLRefCounted starts with a 1 ref, so don't add a ref in the smart pointer
    httpHeaders = LLCore::HttpHeaders::ptr_t(new LLCore::HttpHeaders());

    httpHeaders->append(HTTP_OUT_HEADER_CONTENT_TYPE, HTTP_CONTENT_TEXT_XML);

    const LLVersionInfo& vi(LLVersionInfo::instance());
    std::string user_agent = vi.getChannel() + llformat(" %d.%d.%d (%llu)",
        vi.getMajor(), vi.getMinor(), vi.getPatch(), vi.getBuild());

    httpHeaders->append(HTTP_OUT_HEADER_USER_AGENT, user_agent);

    ///* Setting the DNS cache timeout to -1 disables it completely.
    //This might help with bug #503 */
    //httpOpts->setDNSCacheTimeout(-1);

    std::string request =
        "<?xml version=\"1.0\"?><methodCall><methodName>" + method +
        "</methodName><params><param>" + params.asXMLRPCValue() +
        "</param></params></methodCall>";

    LLCore::BufferArray::ptr_t body = LLCore::BufferArray::ptr_t(new LLCore::BufferArray());

    body->append(request.c_str(), request.size());

    mHandler = LLXMLRPCTransaction::Handler::ptr_t(new Handler(mHttpRequest, this));

    mPostH = mHttpRequest->requestPost(LLCore::HttpRequest::DEFAULT_POLICY_ID,
        mURI, body.get(), httpOpts, httpHeaders, mHandler);
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
    std::string uri = "http://support.secondlife.com";
    LLURI failuri(mURI);
    LLStringUtil::format_map_t args;

    switch (code)
    {
    case CURLE_COULDNT_RESOLVE_HOST:
        args["[HOSTNAME]"] = failuri.hostName();
        message = LLTrans::getString("couldnt_resolve_host", args);
        break;

#if CURLE_SSL_PEER_CERTIFICATE != CURLE_SSL_CACERT
    case CURLE_SSL_PEER_CERTIFICATE:
        message = LLTrans::getString("ssl_peer_certificate");
        break;
#endif
    case CURLE_SSL_CACERT:
    case CURLE_SSL_CONNECT_ERROR:
        message = LLTrans::getString("ssl_connect_error");
        break;

    default:
        break;
    }

    mCurlCode = code;
    setStatus(StatusCURLError, message, uri);

}

LLXMLRPCTransaction::LLXMLRPCTransaction
(
    const std::string& uri,
    const std::string& method,
    const LLSD& params,
    const LLSD& http_params
)
: impl(*new Impl(uri, method, params, http_params))
{
}

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

const LLSD& LLXMLRPCTransaction::response()
{
    return impl.mResponseData;
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
