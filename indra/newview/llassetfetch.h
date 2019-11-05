/** 
 * @file llassetfetch.h
 * @brief Object for managing texture fetches.
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2012-2013, Linden Research, Inc.
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

#ifndef LL_ASSETFETCH_H
#define LL_ASSETFETCH_H

#include <vector>
#include <map>

#include <boost/signals2.hpp>

#include "lldir.h"
#include "llimage.h"
#include "lluuid.h"
#include "llworkerthread.h"
#include "lltextureinfo.h"
#include "httprequest.h"
#include "httpoptions.h"
#include "httpheaders.h"
#include "httphandler.h"
#include "lltrace.h"
#include "llsingleton.h"

#include "llthreadpool.h"
#include "lldynamicpqueue.h"
#include "llassettype.h"
#include "httpcommon.h"

#include "llfttype.h"
#include "llappcorehttp.h"
//========================================================================
// forwards
class LLImageRaw;


//========================================================================
class LLAssetFetch :
    public LLThreadPool,
    public LLSingleton<LLAssetFetch>
{
    LLSINGLETON(LLAssetFetch);
    LOG_CLASS(LLAssetFetch);

public:
    static const std::string FETCHER_NAME;
    enum FetchState
    {
        RQST_UNKNOWN,   // No request has been made, or the request has not yet been queued
        HTTP_QUEUE,     // Request is waiting for HTTP services
        HTTP_DOWNLOAD,  // Request is downloading via HTTP
        THRD_QUEUE,     // Request is waiting to be processed by the thread pool
        THRD_EXEC,      // Request is actively being processed in the thread pool
        RQST_DONE,      // Request has completed but not yet notified any listeners
        RQST_CANCELED,  // Request was canceled (priority fell to 0)
        RQST_ERROR,     // An error was encountered
    };

    enum ErrorCodes
    {
        ERROR_NONE,
        ERROR_QUEUEING,
        ERROR_DOWNLOAD,
        ERROR_PROCESSING,
    };

    // Track an asset request throughout the request.
    class AssetRequest :
        public std::enable_shared_from_this<AssetRequest>,
        public LLCore::HttpHandler,
        public LLThreadPool::ThreadRequest
    {
        LOG_CLASS(AssetRequest);
    public:
        typedef boost::signals2::connection         connection_t;
        typedef std::shared_ptr<AssetRequest>  ptr_t;
        typedef std::function<void(const ptr_t &)>  signal_cb_t;

        virtual             ~AssetRequest() {}

        LLUUID              getId() const   { return mAssetId; }
        LLAssetType::EType  getType() const { return mAssetType; }
        U32                 getPriority() const { return mPriority; }
        void                setPriority(U32 priority) { mPriority = priority; }
        void                adjustPriority(S32 adjustment);
        FetchState          getFetchState() const { return mState; }
        void                setFetchState(FetchState state);
        void                setHTTPHandle(LLCore::HttpHandle handle) { mHttpHandle = handle; }
        LLCore::HttpHandle  getHTTPHandle() const { return mHttpHandle; }
        void                clearHTTPHandle() { mHttpHandle = LLCORE_HTTP_HANDLE_INVALID; }

        // For HTTP handling
        virtual bool        needsHttp() const;
        virtual std::string getURL() const;
        virtual LLAppCoreHttp::EAppPolicy   getPolicyId() const;
        virtual bool        useRangeRequest() const;
        virtual bool        useHeaders() const;
        virtual S32         getRangeOffset() const;
        virtual S32         getRangeSize() const;
        virtual bool        prefetch();
        virtual bool        postfetch(const LLCore::HttpResponse::ptr_t &response);

        virtual bool        needsPostProcess() const;

        // From LLThreadPool::ThreadRequest
        virtual bool        execute(U32 priority) override;
        virtual bool        preexecute(U32 priority) override;
        virtual void        postexecute(U32 priority) override;

        virtual void        advance();
        void                reportError(ErrorCodes code, U32 subcode, std::string message);

        connection_t        addSignal(signal_cb_t cb);
        void                dropSignal(connection_t connection);
        void                signalDone();

        bool                isRequestFinished() const { return ((mState == RQST_DONE) || (mState == RQST_CANCELED) || (mState == RQST_ERROR)); }

        ErrorCodes          getErrorCode() const { return mErrorCode; }
        U32                 getErrorSubcode() const { return mErrorSubcode; }
        std::string         getErrorMessage() const { return mErrorMessage; }

    protected:
        LLUUID              mAssetId;
        LLAssetType::EType  mAssetType;
        U32                 mPriority;
        FetchState          mState;

        LLCore::HttpResponse::ptr_t mHTTPResponse;
        U64BytesImplicit    mDownloadSize;

        LLTimer             mTotalTime;
        LLTimer             mRequestQueue;
        LLTimer             mInflight;
        LLTimer             mProccessQueue;
        LLTimer             mPostprocess;

        LLCore::HttpHandle  mHttpHandle;

        ErrorCodes          mErrorCode;
        U32                 mErrorSubcode;
        std::string         mErrorMessage;

        typedef boost::signals2::signal<void(const ptr_t &)> asset_done_signal_t;
        asset_done_signal_t mAssetDoneSignal;

        AssetRequest(LLUUID id, LLAssetType::EType asset_type);

        void                setId(LLUUID id);
        std::string         getBaseURL() const;

        // Override for LLCore::HttpHandler
        virtual void        onCompleted(LLCore::HttpHandle handle, const LLCore::HttpResponse::ptr_t &response) override;

    };

    typedef std::function<void(const AssetRequest::ptr_t &)>  asset_signal_cb_t;

    // Asset specific information on completion.
    // --- Texture info
    struct TextureInfo
    {
        TextureInfo():
            mRawImage(nullptr),
            mAuxImage(nullptr),
            mDiscardLevel(-1),
            mFullWidth(-1),
            mFullHeight(-1)
        {}

        LLPointer<LLImageRaw>   mRawImage;
        LLPointer<LLImageRaw>   mAuxImage;
        S32                     mDiscardLevel;
        S32                     mFullWidth;
        S32                     mFullHeight;
    };
    typedef std::function<void(const AssetRequest::ptr_t &, const TextureInfo &)>    texture_signal_cb_t;
    // ---

    // texture_signal_cb_t is a special case callback issued in response to textures

    //--------------------------------------------------------------------
    virtual ~LLAssetFetch() {}
    void    update();

    LLUUID                      requestTexture(FTType ftype, const LLUUID &id, S32 priority, S32 width, S32 height, S32 components, S32 discard, bool needs_aux, texture_signal_cb_t cb);
    LLUUID                      requestTexture(FTType ftype, const std::string &url, S32 priority, S32 width, S32 height, S32 components, S32 discard, bool needs_aux, texture_signal_cb_t cb);
    LLUUID                      requestTexture(FTType ftype, const LLUUID &id, const std::string &url, S32 priority, S32 width, S32 height, S32 components, S32 discard, bool needs_aux, texture_signal_cb_t cb);

    void                        cancelRequest(const LLUUID &id);
    void                        cancelRequests(const uuid_set_t &id_list);
    void                        setRequestPriority(const LLUUID &id, U32 priority);
    void                        adjustRequestPriority(const LLUUID &id, S32 adjustment);
    U32                         getRequestPriority(const LLUUID &id) const;

    // Note... these are the old interfaces... Free to update them.
//    bool    createRequest(FTType f_type, const std::string& url, const LLUUID& id, const LLHost& host, F32 priority, S32 w, S32 h, S32 c, S32 desired_discard, bool needs_aux, bool can_use_http);
//    void    deleteRequest(const LLUUID& id, bool cancel);
//     bool    getRequestFinished(const LLUUID& id, S32& discard_level, S32& full_w, S32& full_h, LLPointer<LLImageRaw>& raw, LLPointer<LLImageRaw>& aux, LLCore::HttpStatus& last_http_get_status);
//     bool    updateRequestPriority(const LLUUID& id, F32 priority);
//     S32     getFetchState(const LLUUID& id, F32& data_progress_p, F32& requested_priority_p, U32& fetch_priority_p, F32& fetch_dtime_p, F32& request_dtime_p, bool& can_use_http);
    bool                        isAssetInCache(const LLUUID &id);

    FetchState                  getFetchState(const LLUUID &id) const;
    FetchState                  getFetchState(const std::string &url) const;

// Used internally... move to private?
    bool                        isInCache(const LLUUID &id, LLAssetType::EType type) const;
    bool                        isInCache(const std::string &url, LLAssetType::EType type) const;

protected:
    virtual void                initSingleton() override;
    virtual void                cleanupSingleton() override;

private:
    static const std::string    SETTING_JPEGDECODER;
    static const S32            HTTP_REQUESTS_RANGE_END_MAX;
    static const std::string    REQUEST_EVENT_PUMP;

    typedef boost::signals2::connection connection_t;
    struct get_asset_request_id
    {
        LLUUID operator()(const AssetRequest::ptr_t &request)  { return request->getId(); }
    };
    typedef LLDynamicPriorityQueue<AssetRequest::ptr_t, get_asset_request_id> asset_http_queue_t;
    typedef std::set<LLUUID>                            asset_id_set_t;
    typedef std::map<LLUUID, AssetRequest::ptr_t>  asset_fetch_map_t;
    typedef std::set<AssetRequest::ptr_t>          asset_fetch_set_t;

    void                        adjustRequestPriority(const AssetRequest::ptr_t &request, S32 adjustment);
    void                        setRequestPriority(const AssetRequest::ptr_t &request, U32 priority);
    void                        cancelRequest(const AssetRequest::ptr_t &request);

    AssetRequest::ptr_t         getExistingRequest(const LLUUID &id) const;
    AssetRequest::ptr_t         getExistingRequest(const std::string &url) const;

    bool                        isFileRequest(std::string url) const;

    void                        recordToHTTPRequest(const AssetRequest::ptr_t &request);
    void                        recordHTTPInflight(const AssetRequest::ptr_t &request);
    void                        recordThreadRequest(const AssetRequest::ptr_t &request);
    void                        recordThreadInflight(const AssetRequest::ptr_t &request);
    void                        recordRequestDone(const AssetRequest::ptr_t &request);

    void                        assetHttpRequestCoro();
    void                        onRegionChanged();
    void                        onCapsReceived();
    void                        assetRequestError(const AssetRequest::ptr_t &request);
    bool                        makeHTTPRequest(const AssetRequest::ptr_t &request);
    void                        handleHTTPRequest(const AssetRequest::ptr_t &request, const LLCore::HttpResponse::ptr_t &response, LLCore::HttpStatus status);
    void                        handleAllFinishedRequests();

    asset_fetch_map_t           mAssetRequests;
    asset_http_queue_t          mHttpQueue;
    asset_id_set_t              mHttpInFlight;
    asset_id_set_t              mThreadInFlight;
    asset_fetch_set_t           mThreadDone;

    mutable LLMutex             mAllRequestMtx;
    mutable LLMutex             mThreadInFlightMtx;
    mutable LLMutex             mThreadDoneMtx;

    connection_t                mCapsSignal;

    LLCore::HttpRequest::ptr_t  mHttpRequest;
    LLCore::HttpOptions::ptr_t  mHttpOptions;
    LLCore::HttpOptions::ptr_t  mHttpOptionsWithHeaders;
    LLCore::HttpHeaders::ptr_t  mHttpHeaders;

    U32                         mMaxInFlight;

};


#endif // LL_ASSETFETCH_H
