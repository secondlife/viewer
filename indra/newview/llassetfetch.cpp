/** 
 * @file lltexturefetch.cpp
 * @brief Object which fetches textures from the cache and/or network
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2012-2014, Linden Research, Inc.
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

#include <iostream>
#include <map>
#include <algorithm>

#include "llassetfetch.h"

#include "llapr.h"
#include "lldir.h"
#include "llhttpconstants.h"
#include "llimage.h"
#include "llimagej2c.h"
#include "llworkerthread.h"
#include "message.h"

#include "llagent.h"
#include "lltexturecache.h"
#include "llviewercontrol.h"
#include "llviewertexturelist.h"
#include "llviewertexture.h"
#include "llviewerregion.h"
#include "llviewerstats.h"
#include "llviewerstatsrecorder.h"
#include "llviewerassetstats.h"
#include "llworld.h"
#include "llsdparam.h"
#include "llsdutil.h"
#include "llstartup.h"

#include "httprequest.h"
#include "httphandler.h"
#include "httpresponse.h"
#include "bufferarray.h"
#include "bufferstream.h"
#include "llcorehttputil.h"
#include "llhttpretrypolicy.h"

#include "llfile.h"
#include "llcoros.h"
#include "lleventcoro.h"

#include "llvfs.h"
#include "llvfile.h"

//========================================================================
namespace
{
    class TextureRequest : public LLAssetFetch::AssetRequest
    {
        LOG_CLASS(TextureRequest);
    public:
        typedef std::shared_ptr<TextureRequest> ptr_t;

                                    TextureRequest(LLUUID id, std::string url, FTType ftype, S32 width, S32 height, S32 components, S32 discard, bool needs_aux);
        virtual                     ~TextureRequest() {};

        static void                 setJPEGDecoder(S32 type);

        S32                         getJpegDecoderType() { return sJpegDecoderType; }

        LLPointer<LLImageRaw>       getRawImage() const { return mRawImage; }
        LLPointer<LLImageRaw>       getAuxImage() const { return mAuxImage; }
        LLPointer<LLImageFormatted> getFormattedImage() const { return mFormattedImage; }
        S32                         getDiscardLever() const { return mDiscard; }
        S32                         getFullWidth() const { return mWidth; }
        S32                         getFullHeight() const { return mHeight; }

    protected:
        std::string                 mURL;

        FTType                      mFTType;
        S32                         mWidth;
        S32                         mHeight;
        S32                         mComponents;
        S32                         mDiscard;
        bool                        mNeedsAux;

        LLPointer<LLImageRaw>       mRawImage;
        LLPointer<LLImageRaw>       mAuxImage;
        LLPointer<LLImageFormatted> mFormattedImage;

        bool                        buildFormattedImage();
        bool                        decodeTexture();
        virtual U8 *                getFilledDataBuffer() = 0;
        virtual S8                  getImageCodec() = 0;

        static S32                  sJpegDecoderType;

    private:
        void                        initialize();
    };

    class TextureDownloadRequest : public TextureRequest
    {
        LOG_CLASS(TextureDownloadRequest);
    public:
                                    TextureDownloadRequest(LLUUID id, FTType ftype, S32 width, S32 height, S32 components, S32 discard, bool needs_aux);
                                    TextureDownloadRequest(std::string url, FTType ftype, S32 width, S32 height, S32 components, S32 discard, bool needs_aux);
                                    TextureDownloadRequest(LLUUID id, std::string url, FTType ftype, S32 width, S32 height, S32 components, S32 discard, bool needs_aux);

        virtual                     ~TextureDownloadRequest() {}

        virtual std::string         getURL() const override;

        virtual bool                postfetch(const LLCore::HttpResponse::ptr_t &response) override;
        virtual bool                execute(U32 priority) override;

    protected:
        virtual U8 *                getFilledDataBuffer() override;
        virtual S8                  getImageCodec() override;

    private:
        bool                        cacheTexture();

    };

    class TextureCacheReadRequest : public TextureRequest
    {
        LOG_CLASS(TextureCacheReadRequest);
    public:
                                    TextureCacheReadRequest(LLUUID id, FTType ftype, S32 width, S32 height, S32 components, S32 discard, bool needs_aux);

        virtual bool                execute(U32 priority) override;

    protected:
        virtual U8 *                getFilledDataBuffer() override;
        virtual S8                  getImageCodec() override;

    };

    class TextureFileRequest : public TextureRequest
    {
        LOG_CLASS(TextureFileRequest);
    public:
        TextureFileRequest(LLUUID id, std::string url, FTType ftype, S32 width, S32 height, S32 components, S32 discard, bool needs_aux);

        virtual bool                execute(U32 priority) override;

    protected:
        virtual U8 *                getFilledDataBuffer() override;
        virtual S8                  getImageCodec() override;

    };

    S32 TextureRequest::sJpegDecoderType(0);

    const char *LOG_KEY_ASSETFETCH("ASSETFETCH");
    const std::string FILE_PROTOCOL("file://");
}

//========================================================================

const std::string LLAssetFetch::SETTING_JPEGDECODER("JpegDecoderType");
const std::string LLAssetFetch::FETCHER_NAME("AssetFetcher");
const S32 LLAssetFetch::HTTP_REQUESTS_RANGE_END_MAX(20000000);
const std::string LLAssetFetch::REQUEST_EVENT_PUMP("LLAssetFetch-event-pump");

namespace
{
    const S32 POOL_SIZE(2);
}

//========================================================================
LLAssetFetch::LLAssetFetch():
    LLThreadPool(FETCHER_NAME),
    mAssetRequests(),
    mHttpQueue(),
    mHttpInFlight(),
    mThreadInFlight(),
    mThreadDone(),
    mAllRequestMtx(),
    mThreadInFlightMtx(),
    mThreadDoneMtx(),
    mHttpRequest(),
    mHttpOptions(),
    mHttpOptionsWithHeaders(),
    mHttpHeaders()
{
}

void LLAssetFetch::initSingleton()
{
    TextureRequest::setJPEGDecoder(gSavedSettings.getS32(SETTING_JPEGDECODER));
    setPoolSize(POOL_SIZE);    /*TODO*/// get this from a config

//     mMaxBandwidth = gSavedSettings.getF32("ThrottleBandwidthKBPS");
//     mTextureInfo.setLogging(true);

    mHttpRequest = std::make_shared<LLCore::HttpRequest>();

    mHttpOptions = std::make_shared<LLCore::HttpOptions>();
    mHttpOptionsWithHeaders = std::make_shared<LLCore::HttpOptions>();
    mHttpOptionsWithHeaders->setWantHeaders(true);

    mHttpHeaders = std::make_shared<LLCore::HttpHeaders>();
    mHttpHeaders->append(HTTP_OUT_HEADER_ACCEPT, HTTP_CONTENT_IMAGE_X_J2C);

//     mHttpMetricsHeaders = LLCore::HttpHeaders::ptr_t(new LLCore::HttpHeaders);
//     mHttpMetricsHeaders->append(HTTP_OUT_HEADER_CONTENT_TYPE, HTTP_CONTENT_LLSD_XML);
//     mHttpMetricsPolicyClass = app_core_http.getPolicy(LLAppCoreHttp::AP_REPORTING);
//     mHttpHighWater = HTTP_NONPIPE_REQUESTS_HIGH_WATER;
//     mHttpSemaphore = 0;

    LLThreadPool::initSingleton_();
    startPool();

//  gAgent.addRegionChangedCallback([this](){ onRegionChanged(); });
    gAgent.addRegionChangedCallback(boost::bind(&LLAssetFetch::onRegionChanged, this));

    LLCoros::instance().launch("AssetFetch", [this]() { assetHttpRequestCoro(); });
}

void LLAssetFetch::cleanupSingleton() 
{
    // Clear anything queued for HTTP
    mHttpQueue.clear();

    // tell our coro to exit cleanly.
    LLEventPumps::instance().post(REQUEST_EVENT_PUMP, "quit");

    // Clear anything in flight and cancel it.
    // The sequence here is organized so that it won't trigger the coro's pump and try to make new requests.
    std::vector<LLCore::HttpHandle> handle_list;
    handle_list.reserve(mHttpInFlight.size());

    {
        LLMutexLock lock(mAllRequestMtx);

        for (auto inflight_id : mHttpInFlight)
        {   // get a list of the in flight requests
            asset_fetch_map_t::iterator it(mAssetRequests.find(inflight_id));
            if (it != mAssetRequests.end())
                handle_list.push_back((*it).second->getHTTPHandle());
        }
    }
    mHttpInFlight.clear();  // clear the inflight list

    for (LLCore::HttpHandle handle : handle_list)
    {   // now cancel all the requests.
        mHttpRequest->requestCancel(handle, LLCore::HttpHandler::ptr_t() );
    }

    {   // Clear out anything in flight in the threads
        LLMutexLock lock(mThreadInFlightMtx);
        mThreadInFlight.clear();
    }

    clearThreadRequests();

    LLThreadPool::cleanupSingleton_();

    // Clear out anything in the done 
    // (don't need to mutex here since threads should no longer be running.
    mThreadDone.clear();

    mAssetRequests.clear();
}

//------------------------------------------------------------------------
void LLAssetFetch::update()
{
    // Deliver all completion notifications
    LLCore::HttpStatus status = mHttpRequest->update(0);
    if (!status)
    {
        LL_WARNS_ONCE(LOG_KEY_ASSETFETCH) << "Problem during HTTP servicing.  Reason:  " << status.toString() << LL_ENDL;
    }

    handleAllFinishedRequests();
}

LLUUID LLAssetFetch::requestTexture(FTType ftype, const LLUUID &id, S32 priority, S32 width, S32 height, S32 components, S32 discard, bool needs_aux, texture_signal_cb_t cb)
{
    return requestTexture(ftype, id, std::string(), priority, width, height, components, discard, needs_aux, cb);
}

LLUUID LLAssetFetch::requestTexture(FTType ftype, const std::string &url, S32 priority, S32 width, S32 height, S32 components, S32 discard, bool needs_aux, texture_signal_cb_t cb)
{
    return requestTexture(ftype, LLUUID::null, url, priority, width, height, components, discard, needs_aux, cb);
}

LLUUID LLAssetFetch::requestTexture(FTType ftype, const LLUUID &id, const std::string &url, S32 priority, S32 width, S32 height, S32 components, S32 discard, bool needs_aux, texture_signal_cb_t cb)
{
    if (id.isNull() && url.empty())
    {
        LL_WARNS(LOG_KEY_ASSETFETCH) << "Must have either UUID or url." << LL_ENDL;
        return LLUUID::null;
    }

    if (priority <= 0)
    {
        LL_WARNS(LOG_KEY_ASSETFETCH) << "Texture request with priority " << priority << " <= 0 (" << url << ")" << LL_ENDL;
        priority = 1;
    }

    LLUUID use_id = id;

    if (use_id.isNull())
    {
        use_id.generate(url);
    }

    TextureRequest::ptr_t request(std::dynamic_pointer_cast<TextureRequest>(getExistingRequest(use_id)));
    if (request)
    {   // we have an old request... we should probably just use that

        /*TODO*/ // see if this is a change to discard... what to do?

        // We already have a request for this texture.  Just bump its priority.

        adjustRequestPriority(request, priority);
    }
    else if (!url.empty() && isFileRequest(url))
    {   // We are actually requesting a file
        //LL_WARNS("ASSETFETCH") << "File request for " << url << " as " << use_id << LL_ENDL;
        request = std::make_shared<TextureFileRequest>(use_id, url, ftype, width, height, components, discard, needs_aux);
        request->setPriority(priority);
        recordThreadRequest(request);
    }
    else if (isInCache(use_id, LLAssetType::AT_TEXTURE))
    {
        //LL_WARNS("ASSETFETCH") << "Cache request for " << use_id << "(" << url << ")" << LL_ENDL;
        request = std::make_shared<TextureCacheReadRequest>(use_id, ftype, width, height, components, discard, needs_aux);
        request->setPriority(priority);
        recordThreadRequest(request);
    }
    else
    {
        //LL_WARNS("ASSETFETCH") << "Download request for " << use_id << "(" << url << ")" << LL_ENDL;
        // make a new request
        request = std::make_shared<TextureDownloadRequest>(use_id, url, ftype, width, height, components, discard, needs_aux);
        request->setPriority(priority);
        recordToHTTPRequest(request);
    }
    if (cb)
    {
        asset_signal_cb_t wrap = [cb](const AssetRequest::ptr_t &request) {
            TextureRequest::ptr_t textr = std::static_pointer_cast<TextureRequest>(request);

            LL_DEBUGS("ASSETFETCH") << "Signal done on texture. " << textr->getId() << LL_ENDL;
            TextureInfo texture_info;
            texture_info.mRawImage = textr->getRawImage();
            texture_info.mAuxImage = textr->getAuxImage();
            texture_info.mDiscardLevel = textr->getDiscardLever();
            texture_info.mFullWidth = textr->getFullWidth();
            texture_info.mFullHeight = textr->getFullHeight();

            cb(request, texture_info);
        };


        request->addSignal( wrap );
    }
    mAssetRequests[request->getId()] = request;
    return request->getId();
}

//========================================================================
LLAssetFetch::AssetRequest::ptr_t LLAssetFetch::getExistingRequest(const LLUUID &id) const
{
    {
	    LLMutexLock lock(mAllRequestMtx);
	    asset_fetch_map_t::const_iterator it( mAssetRequests.find(id) );
	
	    if (it != mAssetRequests.end())
	        return (*it).second;
    }

    // nothing in the list of active requests... check if we are just waiting to notify
    {
        LLMutexLock lock(mThreadDoneMtx);

        asset_fetch_set_t::iterator its = std::find_if(mThreadDone.begin(), mThreadDone.end(),
            [id](const AssetRequest::ptr_t &request) { return (id == request->getId()); });

        if (its != mThreadDone.end())
            return (*its);
    }

    return AssetRequest::ptr_t();
}

LLAssetFetch::AssetRequest::ptr_t LLAssetFetch::getExistingRequest(const std::string &url) const
{
    LLUUID url_id(LLUUID::generateNewID(url));
    return getExistingRequest(url_id);
}

LLAssetFetch::FetchState LLAssetFetch::getFetchState(const LLUUID &id) const
{
    AssetRequest::ptr_t request(getExistingRequest(id));

    if (request)
        return request->getFetchState();
    return RQST_UNKNOWN;
}

LLAssetFetch::FetchState LLAssetFetch::getFetchState(const std::string &url) const
{
    LLUUID url_id(LLUUID::generateNewID(url));
    return getFetchState(url_id);
}

// Used internally... move to private?


bool LLAssetFetch::isInCache(const LLUUID &id, LLAssetType::EType type) const
{
    return gVFS->getExists(id, type);
}

bool LLAssetFetch::isInCache(const std::string &url, LLAssetType::EType type) const
{
    LLUUID url_id(LLUUID::generateNewID(url));
    return isInCache(url_id, type);
}

void LLAssetFetch::adjustRequestPriority(const LLUUID &id, S32 adjustment)
{
    AssetRequest::ptr_t request(getExistingRequest(id));
    
    if (request)
        adjustRequestPriority(request, adjustment);
}

void LLAssetFetch::adjustRequestPriority(const AssetRequest::ptr_t &request, S32 adjustment)
{
    if (!request)
        return;

    FetchState state(request->getFetchState());

    switch (state)
    {
        break;
    case LLAssetFetch::HTTP_QUEUE:
        mHttpQueue.priorityAdjust(request->getId(), adjustment);
        break;
    case LLAssetFetch::THRD_QUEUE:
        adjustRequest(request->getId(), adjustment);
        break;
    default:
        break;
    }

    request->adjustPriority(adjustment);

    LL_DEBUGS("ASSETFETCH") << "Adjusted priority on " << request->getId() << " by " << adjustment << " priority is now " << request->getPriority() << LL_ENDL;

    if (request->getPriority() == 0)
    {   // priority has fallen to 0
        cancelRequest(request);
    }
}

U32 LLAssetFetch::getRequestPriority(const LLUUID &id) const
{
    AssetRequest::ptr_t request(getExistingRequest(id));

    if (!request)
        return 0;

    return request->getPriority();
}

void LLAssetFetch::setRequestPriority(const LLUUID &id, U32 priority)
{
    AssetRequest::ptr_t request(getExistingRequest(id));

    if (request)
        setRequestPriority(request, priority);
}

void LLAssetFetch::setRequestPriority(const AssetRequest::ptr_t &request, U32 priority)
{
    if (!request)
        return;
    if (priority == 0)
    {
        cancelRequest(request);
        return;
    }

    FetchState state(request->getFetchState());

    switch (state)
    {
        break;
    case LLAssetFetch::HTTP_QUEUE:
        mHttpQueue.prioritySet(request->getId(), priority);
        break;
    case LLAssetFetch::THRD_QUEUE:
        setRequest(request->getId(), priority);
        break;
    default:
        break;
    }

    request->setPriority(priority);
}

void LLAssetFetch::cancelRequest(const LLUUID &id)
{
    AssetRequest::ptr_t request(getExistingRequest(id));

    if (request)
        cancelRequest(request);
}

void LLAssetFetch::cancelRequests(const uuid_set_t &id_list)
{
    for (const LLUUID &id : id_list)
    {
        cancelRequest(id);
    }
}

void LLAssetFetch::cancelRequest(const AssetRequest::ptr_t &request)
{
    if (!request)
        return;
    LL_DEBUGS("ASSETFETCH") << "Canceling request " << request->getId() << LL_ENDL;

    LLMutexLock lock(mThreadDoneMtx);
    request->setFetchState(RQST_CANCELED);
    mThreadDone.insert(request);
}

bool LLAssetFetch::isFileRequest(std::string url) const
{
    return (url.compare(0, FILE_PROTOCOL.size(), FILE_PROTOCOL) == 0);
}

//========================================================================
void LLAssetFetch::assetHttpRequestCoro()
{
    LL_INFOS("ASSETFETCH") << "Starting asset request monitor coro" << LL_ENDL;

    LLSD command_event;
    do 
    {
        // *NOTE*: During login we may not have a region, since we don't have a region, 
        // there will not be an asset download URL yet.  drop down to the bottom and wait
        // until we are told it is ok to go. (hence the getRegion() check below.)
        while ((mHttpInFlight.size() <= mMaxInFlight) && !mHttpQueue.empty() && gAgent.getRegion())
        {
            if (!gAgent.getRegion()->capabilitiesReceived())    
                break;  // we have a region but no capabilities. 

            AssetRequest::ptr_t request(mHttpQueue.top());
            if (!request)
                break;
            U32 priority(mHttpQueue.topPriority());

            mHttpQueue.pop();
            request->setPriority(priority); // make sure the priorities match.

            if (request->isRequestFinished())
            {   // request was errored or canceled while in the queue...
                request->advance();
                continue;
            }

            if (!request->prefetch())
            {
                continue;
            }

            if (!makeHTTPRequest(request))
            {
                continue;
            }
        }

        LL_DEBUGS("ASSETFETCH") << "Done requesting HTTP. " << mHttpInFlight.size() << " in flight, " << mHttpQueue.size() << " waiting in queue." << LL_ENDL;
        command_event = llcoro::suspendUntilEventOn(REQUEST_EVENT_PUMP);
    } while (!command_event.isString() || (command_event.asString() != "quit") );

    LL_INFOS("ASSETFETCH") << "Ending asset request monitor coro" << LL_ENDL;
}

void LLAssetFetch::onRegionChanged()
{
    // the region has changed (or more importantly we have a region for the first time.) 
    // if we have the caps give the coro a bump... if not wait for the signal.
    LLViewerRegion *regionp = gAgent.getRegion();
    if (mCapsSignal.connected())
        mCapsSignal.disconnect();
    if (regionp->capabilitiesReceived())
        onCapsReceived();
    else
    {   /*[this](){ onCapsReceived(); }*/// can't mix boost and lambda
	    mCapsSignal = regionp->setCapabilitiesReceivedCallback(boost::bind(&LLAssetFetch::onCapsReceived, this));
    }
}

void LLAssetFetch::onCapsReceived()
{   
    // we have caps... we can start downloading things.
    LLEventPumps::instance().post(REQUEST_EVENT_PUMP, "caps");
    if (mCapsSignal.connected())
        mCapsSignal.disconnect();
}

void LLAssetFetch::assetRequestError(const LLAssetFetch::AssetRequest::ptr_t &request)
{
    /*TODO*/ // better error reporting.
    request->setFetchState(RQST_ERROR);
}

void LLAssetFetch::recordToHTTPRequest(const LLAssetFetch::AssetRequest::ptr_t &request)
{
    request->setFetchState(HTTP_QUEUE);
    mHttpQueue.enqueue(request, request->getPriority());
    LLEventPumps::instance().post(REQUEST_EVENT_PUMP, "new");
}

void LLAssetFetch::recordHTTPInflight(const LLAssetFetch::AssetRequest::ptr_t &request)
{
    request->setFetchState(HTTP_DOWNLOAD);
    mHttpInFlight.insert(request->getId());
}

void LLAssetFetch::recordThreadRequest(const LLAssetFetch::AssetRequest::ptr_t &request)
{
    asset_id_set_t::iterator it(mHttpInFlight.find(request->getId()));

    if (it != mHttpInFlight.end())
        mHttpInFlight.erase(it);
    request->setFetchState(THRD_QUEUE);
    queueRequest(request, request->getPriority());
}

void LLAssetFetch::recordThreadInflight(const LLAssetFetch::AssetRequest::ptr_t &request)
{
    LLMutexLock lock(mThreadInFlightMtx);
    request->setFetchState(THRD_EXEC);
    mThreadInFlight.insert(request->getId());
}

void LLAssetFetch::recordRequestDone(const AssetRequest::ptr_t &request)
{
    //_WARNS("ASSETFETCH") << "Recording 'Done' for " << request->getId() << LL_ENDL;
    if ((request->getFetchState() != RQST_CANCELED) || (request->getFetchState() != RQST_ERROR))
    {
        request->setFetchState(RQST_DONE);
    }
    {
        LLMutexLock lock(mThreadInFlightMtx);
        asset_id_set_t::iterator it(mThreadInFlight.find(request->getId()));
        if (it != mThreadInFlight.end())
            mThreadInFlight.erase(it);
    }
    {   // put these somewhere so that we can clean them up on the main thread.
        LLMutexLock lock(mThreadDoneMtx);
        mThreadDone.insert(request);
    }
    {
        LLMutexLock lock(mAllRequestMtx);
        asset_fetch_map_t::iterator itm = mAssetRequests.find(request->getId());
        if (itm != mAssetRequests.end())
            mAssetRequests.erase(itm);
    }
}


bool LLAssetFetch::makeHTTPRequest(const AssetRequest::ptr_t &request)
{
    LLAppCoreHttp &app_core_http(LLAppViewer::instance()->getAppCoreHttp());

    LLCore::HttpHandle              http_handle(LLCORE_HTTP_HANDLE_INVALID);
    LLCore::HttpOptions::ptr_t      options = request->useHeaders() ? mHttpOptionsWithHeaders : mHttpOptions;
    LLCore::HttpRequest::policy_t   http_policy = app_core_http.getPolicy(request->getPolicyId());

    std::string  url(request->getURL());
    U32 priority(request->getPriority());

    //LL_WARNS("ASSETFETCH") << "Making HTTP Request for " << request->getURL() << LL_ENDL;
    // Only server bake images use the returned headers currently, for getting retry-after field.
/*TODO*/ // Is this really necessary?
//     if (request->useRangeRequest())
//     {
//         S32 request_offset(request->getRangeOffset());
//         S32 request_size(request->getRangeSize());
// 
//         http_handle = mHttpRequest->requestGetByteRange(http_policy, priority, url, 
//             request_offset, ((request_offset + request_size) > HTTP_REQUESTS_RANGE_END_MAX ? 0 : request_size),
//             options, mHttpHeaders, request);
//     }
//     else
    {   // 'Range:' requests may be disabled in which case all HTTP
        // texture fetches result in full fetches.  This can be used
        // by people with questionable ISPs or networking gear that
        // doesn't handle these well.
        http_handle = mHttpRequest->requestGet(http_policy, priority, url,
            options, mHttpHeaders, request);
    }

    if (http_handle == LLCORE_HTTP_HANDLE_INVALID)
    {
        LLCore::HttpStatus status(mHttpRequest->getStatus());
        LL_WARNS("ASSETFETCH") << "HTTP GET request failed for " << request->getId() << ", Status: " << status.toTerseString() <<
            " Reason: '" << status.toString() << "'" << LL_ENDL;

        request->reportError(ERROR_DOWNLOAD, 1, status.toTerseString());
        request->advance();

        return false;
    }

    request->setHTTPHandle(http_handle);
    recordHTTPInflight(request);

    return true;
}


void LLAssetFetch::handleHTTPRequest(const LLAssetFetch::AssetRequest::ptr_t &request, const LLCore::HttpResponse::ptr_t &response, LLCore::HttpStatus status)
{
    size_t inflight_count(mHttpInFlight.size());

    asset_id_set_t::iterator it(mHttpInFlight.find(request->getId()));
    if (it != mHttpInFlight.end())
    {   // Remove it from our inflight map.
        mHttpInFlight.erase(it);
    }

    //LL_WARNS("ASSETFETCH") << "HTTP GET DONE: id=" << request->getId() << " status=" << status.getStatus() << "/" << status.getType() << "(" << status.getMessage() << ")" << LL_ENDL;

    if (!status)
    {
        request->reportError(ERROR_DOWNLOAD, status.getType(), status.getMessage());
        LL_WARNS("ASSETFETCH") << "HTTP GET request failed for " << request->getId() << ", Status: " << status.toTerseString() <<
            " Reason: '" << status.toString() << "'" << LL_ENDL;
    }
    else
    {
        if (!request->postfetch(response))
        {
            if ((request->getFetchState() != RQST_ERROR) && (request->getFetchState() != RQST_CANCELED))
            {   // postfetch reported false, but did not record an error itself.
                request->reportError(ERROR_DOWNLOAD, status.getType(), "Error in post fetch processing");
            }
        }
    }

    request->advance();
    if (inflight_count != mHttpInFlight.size())
    {
        LLEventPumps::instance().post(REQUEST_EVENT_PUMP, "inflight_decr");
    }

    LL_DEBUGS("ASSETFETCH") << "Asset HTTP request finished, inflight count now: " << mHttpInFlight.size() << LL_ENDL;
}

void LLAssetFetch::handleAllFinishedRequests()
{
    LLMutexLock lock(mThreadDoneMtx);

    for (auto request : mThreadDone)
    {
        request->signalDone();
    }

    mThreadDone.clear();
}

//========================================================================
LLAssetFetch::AssetRequest::AssetRequest(LLUUID id, LLAssetType::EType asset_type) :
    LLCore::HttpHandler(),
    LLThreadPool::ThreadRequest(id),
    mAssetId(id),
    mAssetType(asset_type),
    mPriority(0),
    mState(RQST_UNKNOWN),
    mTotalTime(),
    mRequestQueue(),
    mInflight(),
    mProccessQueue(),
    mPostprocess(),
    mDownloadSize(0),
    mHttpHandle(LLCORE_HTTP_HANDLE_INVALID),
    mErrorCode(LLAssetFetch::ERROR_NONE),
    mErrorSubcode(0),
    mErrorMessage()
{
    mTotalTime.stop();
    mRequestQueue.stop();
    mInflight.stop();
    mProccessQueue.stop();
    mPostprocess.stop();
}

void LLAssetFetch::AssetRequest::setId(LLUUID id)
{
    mAssetId = id;
    mRequestId = id;
}

void LLAssetFetch::AssetRequest::adjustPriority(S32 adjustment)
{
    if (adjustment < 0)
    {
        adjustment *= -1;
        mPriority = ((U32)adjustment > mPriority) ? 0 : (mPriority - adjustment);
    }
    else
        mPriority += (U32)adjustment;
}

/*TODO*/ // Add an error reporting function so the request can be correctly cleaned up.

void LLAssetFetch::AssetRequest::setFetchState(LLAssetFetch::FetchState state)
{ 
    if ((state == mState) || (mState == RQST_DONE) || (mState == RQST_ERROR))
        return;

    switch (mState)
    {
    case RQST_UNKNOWN:   // we are leaving the unknown state, so starting the overall timer
        mTotalTime.start(); 
        break;
    case HTTP_QUEUE:
        mRequestQueue.stop();
        break;
    case HTTP_DOWNLOAD:
        mInflight.stop();
        break;
    case THRD_QUEUE:
        mProccessQueue.stop();
        break;
    case THRD_EXEC:
        mPostprocess.stop();
        break;
    case RQST_DONE:  // noop... should never leave this state...
    case RQST_ERROR:
    case RQST_CANCELED:
        break;
    };
    mState = state; 

    switch (mState)
    {
    case RQST_UNKNOWN:   // should not enter the unknown state
        break;
    case HTTP_QUEUE:
        mRequestQueue.start();
        break;
    case HTTP_DOWNLOAD:
        mInflight.start();
        break;
    case THRD_QUEUE:
        mProccessQueue.start();
        break;
    case THRD_EXEC:
        mPostprocess.start();
        break;
    case RQST_CANCELED:
        mRequestQueue.stop();
        mInflight.stop();
        mProccessQueue.stop();
        mPostprocess.stop();
        mTotalTime.stop();
        break;
    case RQST_ERROR:
        mTotalTime.stop();
        /*TODO*/    // trigger done here?
        break;
    default:
        break;
    };
}

bool LLAssetFetch::AssetRequest::needsHttp() const
{
    return true;
}

bool LLAssetFetch::AssetRequest::needsPostProcess() const
{
    return true;
}

std::string LLAssetFetch::AssetRequest::getBaseURL() const
{
    LLViewerRegion *regionp = gAgent.getRegion();

    LL_WARNS_IF(!regionp, "ASSETFETCH") << "Request for asset but no region yet!" << LL_ENDL;

    if (!regionp)
    {
        return std::string();
    }

    return regionp->getViewerAssetUrl();
}


std::string LLAssetFetch::AssetRequest::getURL() const
{
    return getBaseURL();
}

LLAppCoreHttp::EAppPolicy LLAssetFetch::AssetRequest::getPolicyId() const
{
    return LLAppCoreHttp::AP_TEXTURE;
}

bool LLAssetFetch::AssetRequest::useRangeRequest() const
{
    return false;
}

bool LLAssetFetch::AssetRequest::useHeaders() const
{
    return false;
}

S32 LLAssetFetch::AssetRequest::getRangeOffset() const
{
    return 0;
}

S32 LLAssetFetch::AssetRequest::getRangeSize() const
{
    return 0;
}

bool LLAssetFetch::AssetRequest::prefetch()
{
    return true;
}

bool LLAssetFetch::AssetRequest::postfetch(const LLCore::HttpResponse::ptr_t &response)
{
    advance();
    return true;
}

void LLAssetFetch::AssetRequest::onCompleted(LLCore::HttpHandle handle, const LLCore::HttpResponse::ptr_t &response)
{   // Handle response from HTTP
    LLCore::HttpStatus status(response->getStatus());
    AssetRequest::ptr_t self(shared_from_this());

    LL_WARNS_IF(!status, "ASSETFETCH") << "HTTP GET request failed for " << self->getId() << ", Status: " << status.toTerseString() <<
        " Reason: '" << status.toString() << "'" << LL_ENDL;

    mHTTPResponse = response;
    mDownloadSize = response->getBodySize();

    LLAssetFetch::instance().handleHTTPRequest(self, response, status);
}

bool LLAssetFetch::AssetRequest::execute(U32 priority)
{
    return true;
}

bool LLAssetFetch::AssetRequest::preexecute(U32 priority) 
{   
    setPriority(priority);  // in case priority has changed.
    LLAssetFetch::instance().recordThreadInflight(shared_from_this());
    return true;
}

void LLAssetFetch::AssetRequest::postexecute(U32 priority)
{
    advance();
}

void LLAssetFetch::AssetRequest::advance()
{
    ptr_t self(shared_from_this());

    if (mState == RQST_UNKNOWN)
    {
        if (needsHttp())
            LLAssetFetch::instance().recordToHTTPRequest(self);
        else if (needsPostProcess())
            LLAssetFetch::instance().recordThreadRequest(self);
    }
    else if (mState == HTTP_DOWNLOAD)
    {
        if (needsPostProcess())
            LLAssetFetch::instance().recordThreadRequest(self);
        else
            LLAssetFetch::instance().recordRequestDone(self);
    }
    else if (mState == THRD_EXEC)
    {
        LLAssetFetch::instance().recordRequestDone(self);
    }
    else if ((mState == RQST_ERROR) || (mState == RQST_CANCELED))
    {
        LL_WARNS_IF((mState == RQST_ERROR), "ASSETFETCH") << "Advancing error for request " << getId() << " code=" << mErrorCode << "(" << mErrorSubcode << ":" << mErrorMessage << ")" << LL_ENDL;
        LLAssetFetch::instance().recordRequestDone(self);
    }
    else
    {
        LL_WARNS("ASSETFETCH") << "Unknown state advance for " << getId() << " can not auto advance from " << mState << LL_ENDL;
    }
}

void LLAssetFetch::AssetRequest::reportError(ErrorCodes code, U32 subcode, std::string message)
{
    mErrorCode = code;
    mErrorSubcode = subcode;
    mErrorMessage = message;

    setFetchState(RQST_ERROR);
}

LLAssetFetch::AssetRequest::connection_t LLAssetFetch::AssetRequest::addSignal(LLAssetFetch::AssetRequest::signal_cb_t cb)
{
    return mAssetDoneSignal.connect(cb);
}

void LLAssetFetch::AssetRequest::dropSignal(LLAssetFetch::AssetRequest::connection_t connection)
{
    mAssetDoneSignal.disconnect(connection);
}

void LLAssetFetch::AssetRequest::signalDone()
{
    mAssetDoneSignal(shared_from_this());
}

//========================================================================
namespace
{
    void TextureRequest::setJPEGDecoder(S32 type)
    {
        sJpegDecoderType = type;
    }

    //--------------------------------------------------------------------
    TextureRequest::TextureRequest(LLUUID id, std::string url, FTType ftype, S32 width, S32 height, S32 components, S32 discard, bool needs_aux) :
        LLAssetFetch::AssetRequest(id, LLAssetType::AT_TEXTURE),
        mURL(url),
        mFTType(ftype),
        mWidth(width),
        mHeight(height),
        mComponents(components),
        mDiscard(discard),
        mNeedsAux(needs_aux),
        mRawImage(nullptr),
        mAuxImage(nullptr),
        mFormattedImage(nullptr)
    {
        initialize();
    }

    void TextureRequest::initialize()
    {
        std::string  extension;

        // clamping the discard
        mDiscard = llclamp<S32>(mDiscard, 0, (MAX_DISCARD_LEVEL - 1));

        if (!mURL.empty())
        {
            extension = gDirUtilp->getExtension(mURL);
        }

        // Do full requests for baked textures to reduce interim blurring.
        if (mFTType == FTT_SERVER_BAKE)
        {
            llassert(!mURL.empty());
            LL_DEBUGS(LOG_KEY_ASSETFETCH) << "full request for " << getId() << " texture is FTT_SERVER_BAKE" << LL_ENDL;
            mDiscard = 0;
            //desired_size = LLImageJ2C::calcDataSizeJ2C(1024, 1024, 4, mDiscard);
        }
        else if (!mURL.empty() && !extension.empty() && (LLImageBase::getCodecFromExtension(extension) != IMG_CODEC_J2C))
        {
            LL_DEBUGS(LOG_KEY_ASSETFETCH) << "full request for " << getId() << " extension is not J2C: " << extension << LL_ENDL;
            // Only do partial requests for J2C at the moment
            mDiscard = 0;
            //desired_size = MAX_IMAGE_DATA_SIZE;
        }
        else if ((mWidth * mHeight * mComponents) > 0)
        {
            // If the requester knows the dimensions of the image,
            // this will calculate how much data we need without having to parse the header
            // desired_size = LLImageJ2C::calcDataSizeJ2C(mWidth, mHeight, mComponents, mDiscard);
        }
        else
        {
            // If the requester knows nothing about the file, fetch enough to parse the header
            // and determine how many discard levels are actually available
            //desired_size = LLImageJ2C::calcDataSizeJ2C(2048, 2048, 4, mDiscard);
        }

        // terrible hack to work around OpenJPEG issues with failing to decode in the face of incomplete data
        //         if (mJpegDecoderType > 0)
        //         {
        //             desired_size = MAX_IMAGE_DATA_SIZE;
        //         }

    }

    bool TextureRequest::buildFormattedImage()
    {
        // do this *before* any clearing of URLs so we don't just assume everything
        // fetched via HTTP is J2C
        S8 codec(getImageCodec());

        //             U32 reply_size(0);
        //             U32 reply_offset(0);

        // For now, create formatted image based on extension
        if (codec == IMG_CODEC_J2C)
        {
            mFormattedImage = LLImageFormatted::createFromTypeWithImpl(codec, getJpegDecoderType());
        }
        else
        {
            mFormattedImage = LLImageFormatted::createFromType(codec);
        }

        if (mFormattedImage.isNull())
        {
            mFormattedImage = new LLImageJ2C(LLImageJ2C::ImplType(getJpegDecoderType())); // default
        }


        if (mFormattedImage.isNull())
        {
            std::stringstream msg;

            msg << "Abort: Unable to allocate formated image with codec " << codec;

            reportError(LLAssetFetch::ERROR_PROCESSING, 3, msg.str());
            return false;
        }

        U8 * encoded_buffer = getFilledDataBuffer();
        if (!encoded_buffer)
        {
            // abort. If we have no space for packet, we have not enough space to decode image
            reportError(LLAssetFetch::ERROR_PROCESSING, 1, "Out of memory");
            LL_WARNS(LOG_KEY_ASSETFETCH) << getId() << " abort: out of memory" << LL_ENDL;
            return false;
        }

        // update image buffer and size to appended range   
        // takes ownership of encoded_buffer
        mFormattedImage->setData(encoded_buffer, mDownloadSize, true);

        // Parse headers to determine width/height/components
        if (!mFormattedImage->updateData())
        {
            reportError(LLAssetFetch::ERROR_PROCESSING, 2, "Could not parse image header.");
            LL_WARNS(LOG_KEY_ASSETFETCH) << getId() << " could not parse header data from HTTP result." << LL_ENDL;
            return false;
        }

        mFormattedImage->setDiscardLevel(mDiscard);

        return true;
    }


    bool TextureRequest::decodeTexture()
    {
//         static LLCachedControl<bool> textures_decode_disabled(gSavedSettings, "TextureDecodeDisabled", false);
//         if (textures_decode_disabled)
//         {
//             LL_WARNS(LOG_KEY_ASSETFETCH) << getId() << " Abort: Texture decode is disabled via TextureDecodeDisabled" << LL_ENDL;
//             return false;
//         }

#if 0
        if (mDesiredDiscard < 0)
        {
            LL_WARNS(LOG_KEY_ASSETFETCH) << getId() << " abort: decode abort desired discard " << mDesiredDiscard << " < 0" << LL_ENDL;
            return false;
        }
#endif

        if (mFormattedImage->getDataSize() <= 0)
        {
            reportError(LLAssetFetch::ERROR_PROCESSING, 3, "Decoding empty image!");
            LL_WARNS(LOG_KEY_ASSETFETCH) << getId() << " abort: decode abort (mFormattedImage->getDataSize() <= 0)" << LL_ENDL;
            return false;
        }
        if (mFormattedImage->getDiscardLevel() < 0)
        {
            reportError(LLAssetFetch::ERROR_PROCESSING, 4, "Invalid discard level.");
            LL_WARNS(LOG_KEY_ASSETFETCH) << getId() << " abort: Decode entered with invalid mLoadedDiscard. " << LL_ENDL;
            return false;
        }

        mRawImage = new LLImageRaw(mFormattedImage->getWidth(), mFormattedImage->getHeight(), mFormattedImage->getComponents());
        mAuxImage = mNeedsAux ? new LLImageRaw(mFormattedImage->getWidth(), mFormattedImage->getHeight(), 1) : NULL;

        //_WARNS("RIDER") << getId() << ": Decoding. Bytes: " << mFormattedImage->getDataSize() << " Discard: " << mFormattedImage->getDiscardLevel() << LL_ENDL;

        bool decoded = mFormattedImage->decode(mRawImage, 1.0f);

        if (!decoded)
        {
            reportError(LLAssetFetch::ERROR_PROCESSING, 4, "Failed to decode image data.");
            LL_DEBUGS(LOG_KEY_ASSETFETCH) << getId() << " DECODE_IMAGE failed to decode JPEG data " << LL_ENDL;
            return false;
        }

        if (!mRawImage->getFullWidth())
            mRawImage->setFullWidth(mRawImage->getWidth());
        if (!mRawImage->getFullHeight())
            mRawImage->setFullHeight(mRawImage->getHeight());
        mWidth = mRawImage->getFullWidth();
        mHeight = mRawImage->getFullHeight();

        if (mNeedsAux)
        {
            mFormattedImage->decodeChannels(mAuxImage, 1.0f, 4, 4);
        }

        //_WARNS("RIDER") << "Request " << getRequestId() << " decoded mRawImage=" << mRawImage.notNull() << " mAuxImage=" << mAuxImage.notNull() << " dims=" << mWidth << "x" << mHeight << LL_ENDL;

        return (mRawImage.notNull() && (!mNeedsAux || mAuxImage.notNull()));
    }

    //====================================================================
    TextureDownloadRequest::TextureDownloadRequest(LLUUID id, FTType ftype, S32 width, S32 height, S32 components, S32 discard, bool needs_aux):
        TextureRequest(id, std::string(), ftype, width, height, components, discard, needs_aux)
    {
    }

    TextureDownloadRequest::TextureDownloadRequest(std::string url, FTType ftype, S32 width, S32 height, S32 components, S32 discard, bool needs_aux):
        TextureRequest(LLUUID::generateNewID(url), url, ftype, width, height, components, discard, needs_aux)
    {
        /*TODO*/ // verify that generateNewID() above will generate the same UUID for the same URL.
    }

    TextureDownloadRequest::TextureDownloadRequest(LLUUID id, std::string url, FTType ftype, S32 width, S32 height, S32 components, S32 discard, bool needs_aux) :
        TextureRequest(id, url, ftype, width, height, components, discard, needs_aux)
    {
        /*TODO*/ // verify that generateNewID() above will generate the same UUID for the same URL.
    }


    std::string TextureDownloadRequest::getURL() const
    {
        if (!mURL.empty())
            return mURL;
        std::string base_url = AssetRequest::getBaseURL();

        return base_url + "/?texture_id=" + getId().asString();
    }

    bool TextureDownloadRequest::postfetch(const LLCore::HttpResponse::ptr_t &response) 
    {
        return true;
    }

    bool TextureDownloadRequest::execute(U32 priority) 
    {
        //_WARNS("ASSETFETCH") << "Post HTTP processing for " << getURL() << LL_ENDL;

        if (buildFormattedImage())
        {
            if (decodeTexture())
            {
                cacheTexture();
            }
        }
        // always returning true so we advance.
        return true;
    }

    U8 *TextureDownloadRequest::getFilledDataBuffer()
    {
        LLCore::BufferArray *body = mHTTPResponse->getBody();
        U8* encoded_buffer = (U8*)ll_aligned_malloc_16(mDownloadSize);

        if (!encoded_buffer)
        {
            return nullptr;
        }

        body->read(0, (char *)encoded_buffer, mDownloadSize);
        return encoded_buffer;
    }

    S8  TextureDownloadRequest::getImageCodec()
    {
        std::string extension(gDirUtilp->getExtension(mHTTPResponse->getRequestURL()));
        return LLImageFormatted::getCodecFromExtension(extension);
    }

    bool TextureDownloadRequest::cacheTexture()
    {
//         LLVFile file(gVFS, getId(), mAssetType, LLVFile::WRITE);
//         file.setMaxSize(mFormattedImage->getDataSize());
//         // write is synchronous.
//         file.write(mFormattedImage->getData(), mFormattedImage->getDataSize()); 

        return true;
    }

    //====================================================================
    TextureCacheReadRequest::TextureCacheReadRequest(LLUUID id, FTType ftype, S32 width, S32 height, S32 components, S32 discard, bool needs_aux):
        TextureRequest(id, std::string(), ftype, width, height, components, discard, needs_aux)
    {
    }

    bool TextureCacheReadRequest::execute(U32 priority)
    {
        //_WARNS("ASSETFETCH") << "Thread processing for " << getId() << LL_ENDL;

        if (buildFormattedImage())
        {
            decodeTexture();
        }
        // always return true so we advance.
        return true;
    }

    U8 *TextureCacheReadRequest::getFilledDataBuffer()
    {
        LLVFile file(gVFS, getId(), mAssetType, LLVFile::READ);

        mDownloadSize = file.getSize();

        U8* encoded_buffer = (U8*)ll_aligned_malloc_16(mDownloadSize);

        if (!encoded_buffer)
        {
            return nullptr;
        }

        file.read(encoded_buffer, mDownloadSize, false);
        return encoded_buffer;
    }

    S8 TextureCacheReadRequest::getImageCodec()
    {
        return IMG_CODEC_J2C;
    }

    //====================================================================
    TextureFileRequest::TextureFileRequest(LLUUID id, std::string url, FTType ftype, S32 width, S32 height, S32 components, S32 discard, bool needs_aux) :
        TextureRequest(id, url, ftype, width, height, components, discard, needs_aux)
    {
    }

    bool TextureFileRequest::execute(U32 priority)
    {
        //_WARNS("ASSETFETCH") << "Thread processing for " << getId() << LL_ENDL;
        if (buildFormattedImage())
            decodeTexture();

        // Always return true so we advance the request.
        return true;
    }

    U8 *TextureFileRequest::getFilledDataBuffer()
    {
        std::string filename = mURL.substr(FILE_PROTOCOL.length());
        llifstream file(filename, std::ios::binary);

        // llifstream_size is not particularly efficient... but we're off on our own thread.
        mDownloadSize = llifstream_size(file);  

        U8* encoded_buffer = (U8*)ll_aligned_malloc_16(mDownloadSize);

        if (!encoded_buffer)
        {
            return nullptr;
        }

        file.read((char *)encoded_buffer, mDownloadSize);
        return encoded_buffer;

    }

    S8 TextureFileRequest::getImageCodec()
    {
        std::string extension(gDirUtilp->getExtension(mURL));
        return LLImageFormatted::getCodecFromExtension(extension);
    }

}

//========================================================================
