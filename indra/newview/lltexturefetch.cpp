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

#include "lltexturefetch.h"

#include "lldir.h"
#include "llhttpconstants.h"
#include "llimage.h"
#include "llimagej2c.h"
#include "llimageworker.h"
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

LLTrace::CountStatHandle<F64> LLTextureFetch::sCacheHit("texture_cache_hit");
LLTrace::CountStatHandle<F64> LLTextureFetch::sCacheAttempt("texture_cache_attempt");
LLTrace::EventStatHandle<LLUnit<F32, LLUnits::Percent> > LLTextureFetch::sCacheHitRate("texture_cache_hits");

LLTrace::SampleStatHandle<F32Seconds> LLTextureFetch::sCacheReadLatency("texture_cache_read_latency");
LLTrace::SampleStatHandle<F32Seconds> LLTextureFetch::sTexDecodeLatency("texture_decode_latency");
LLTrace::SampleStatHandle<F32Seconds> LLTextureFetch::sCacheWriteLatency("texture_write_latency");
LLTrace::SampleStatHandle<F32Seconds> LLTextureFetch::sTexFetchLatency("texture_fetch_latency");

LLTextureFetchTester* LLTextureFetch::sTesterp = NULL ;
const std::string sTesterName("TextureFetchTester");

//////////////////////////////////////////////////////////////////////////////
//
// Introduction
//
// This is an attempt to document what's going on in here after-the-fact.
// It's a sincere attempt to be accurate but there will be mistakes.
//
//
// Purpose
//
// What is this module trying to do?  It accepts requests to load textures
// at a given priority and discard level and notifies the caller when done
// (successfully or not).  Additional constraints are:
//
// * Support a local texture cache.  Don't hit network when possible
//   to avoid it.
// * Use UDP or HTTP as directed or as fallback.  HTTP is tried when
//   not disabled and a URL is available.  UDP when a URL isn't
//   available or HTTP attempts fail.
// * Asynchronous (using threads).  Main thread is not to be blocked or
//   burdened.
// * High concurrency.  Many requests need to be in-flight and at various
//   stages of completion.
// * Tolerate frequent re-prioritizations of requests.  Priority is
//   a reflection of a camera's viewpoint and as that viewpoint changes,
//   objects and textures become more and less relevant and that is
//   expressed at this level by priority changes and request cancelations.
//
// The caller interfaces that fall out of the above and shape the
// implementation are:
// * createRequest - Load j2c image via UDP or HTTP at given discard level and priority
// * deleteRequest - Request removal of prior request
// * getRequestFinished - Test if request is finished returning data to caller
// * updateRequestPriority - Change priority of existing request
// * getFetchState - Retrieve progress on existing request
//
// Everything else in here is mostly plumbing, metrics and debug.
//
//
// The Work Queue
//
// The two central classes are LLTextureFetch and LLTextureFetchWorker.
// LLTextureFetch combines threading with a priority queue of work
// requests.  The priority queue is sorted by a U32 priority derived
// from the F32 priority in the APIs.  The *only* work request that
// receives service time by this thread is the highest priority
// request.  All others wait until it is complete or a dynamic priority
// change has re-ordered work.
//
// LLTextureFetchWorker implements the work request and is 1:1 with
// texture fetch requests.  Embedded in each is a state machine that
// walks it through the cache, HTTP, UDP, image decode and retry
// steps of texture acquisition.
//
//
// Threads
//
// Several threads are actively invoking code in this module.  They
// include:
//
// 1.  Tmain    Main thread of execution
// 2.  Ttf      LLTextureFetch's worker thread provided by LLQueuedThread
// 3.  Tcurl    LLCurl's worker thread (should disappear over time)
// 4.  Ttc      LLTextureCache's worker thread
// 5.  Tid      Image decoder's worker thread
// 6.  Thl      HTTP library's worker thread
//
//
// Mutexes/Condition Variables
//
// 1.  Mt       Mutex defined for LLThread's condition variable (base class of
//              LLTextureFetch)
// 2.  Ct       Condition variable for LLThread and used by lock/unlockData().
// 3.  Mwtd     Special LLWorkerThread mutex used for request deletion
//              operations (base class of LLTextureFetch)
// 4.  Mfq      LLTextureFetch's mutex covering request and command queue
//              data.
// 5.  Mfnq     LLTextureFetch's mutex covering udp and http request
//              queue data.
// 6.  Mwc      Mutex covering LLWorkerClass's members (base class of
//              LLTextureFetchWorker).  One per request.
// 7.  Mw       LLTextureFetchWorker's mutex.  One per request.
//
//
// Lock Ordering Rules
//
// Not an exhaustive list but shows the order of lock acquisition
// needed to prevent deadlocks.  'A < B' means acquire 'A' before
// acquiring 'B'.
//
// 1.    Mw < Mfnq
// (there are many more...)
//
//
// Method and Member Definitions
//
// With the above, we'll try to document what threads can call what
// methods (using T* for any), what locks must be held on entry and
// are taken out during execution and what data is covered by which
// lock (if any).  This latter category will be especially prone to
// error so be skeptical.
//
// A line like:  "// Locks:  M<xxx>" indicates a method that must
// be invoked by a caller holding the 'M<xxx>' lock.  Similarly,
// "// Threads:  T<xxx>" means that a caller should be running in
// the indicated thread.
//
// For data members, a trailing comment like "// M<xxx>" means that
// the data member is covered by the specified lock.  Absence of a
// comment can mean the member is unlocked or that I didn't bother
// to do the archaeology.  In the case of LLTextureFetchWorker,
// most data members added by the leaf class are actually covered
// by the Mw lock.  You may also see "// T<xxx>" which means that
// the member's usage is restricted to one thread (except for
// perhaps construction and destruction) and so explicit locking
// isn't used.
//
// In code, a trailing comment like "// [-+]M<xxx>" indicates a
// lock acquision or release point.
//
//
// Worker Lifecycle
//
// The threading and responder model makes it very likely that
// other components are holding on to a pointer to a worker request.
// So, uncoordinated deletions of requests is a guarantee of memory
// corruption in a short time.  So destroying a request involves
// invocations's of LLQueuedThread/LLWorkerThread's abort/stop
// logic that removes workers and puts them ona delete queue for
// 2-phase destruction.  That second phase is deferrable by calls
// to deleteOK() which only allow final destruction (via dtor)
// once deleteOK has determined that the request is in a safe
// state.
//
//
// Worker State Machine
//
// "doWork" will be executed for a given worker on its respective
// LLQueuedThread.  If doWork returns true, the worker is treated
// as completed.  If doWork returns false, the worker will be
// put on the back of the work queue at the start of the next iteration
// of the mainloop.  If a worker is waiting on a resource, it should
// return false as soon as possible and not block to avoid starving
// other workers of cpu cycles.
//



//////////////////////////////////////////////////////////////////////////////

// Tuning/Parameterization Constants

static const S32 HTTP_PIPE_REQUESTS_HIGH_WATER = 100;       // Maximum requests to have active in HTTP (pipelined)
static const S32 HTTP_PIPE_REQUESTS_LOW_WATER = 50;         // Active level at which to refill
static const S32 HTTP_NONPIPE_REQUESTS_HIGH_WATER = 40;
static const S32 HTTP_NONPIPE_REQUESTS_LOW_WATER = 20;

// BUG-3323/SH-4375
// *NOTE:  This is a heuristic value.  Texture fetches have a habit of using a
// value of 32MB to indicate 'get the rest of the image'.  Certain ISPs and
// network equipment get confused when they see this in a Range: header.  So,
// if the request end is beyond this value, we issue an open-ended Range:
// request (e.g. 'Range: <start>-') which seems to fix the problem.
static const S32 HTTP_REQUESTS_RANGE_END_MAX = 20000000;

// stop after 720 seconds, might be overkill, but cap request can keep going forever.
static const S32 MAX_CAP_MISSING_RETRIES = 720;
static const S32 CAP_MISSING_EXPIRATION_DELAY = 1; // seconds

//////////////////////////////////////////////////////////////////////////////
namespace
{
    // The NoOpDeletor is used when passing certain objects (the LLTextureFetchWorker)
    // in a smart pointer below for passage into
    // the LLCore::Http libararies. When the smart pointer is destroyed,  no
    // action will be taken since we do not in these cases want the object to
    // be destroyed at the end of the call.
    //
    // *NOTE$: Yes! It is "Deletor"
    // http://english.stackexchange.com/questions/4733/what-s-the-rule-for-adding-er-vs-or-when-nouning-a-verb
    // "delete" derives from Latin "deletus"
    void NoOpDeletor(LLCore::HttpHandler *)
    { /*NoOp*/ }
}

static const char* e_state_name[] =
{
    "INVALID",
    "INIT",
    "LOAD_FROM_TEXTURE_CACHE",
    "CACHE_POST",
    "LOAD_FROM_NETWORK",
    "WAIT_HTTP_RESOURCE",
    "WAIT_HTTP_RESOURCE2",
    "SEND_HTTP_REQ",
    "WAIT_HTTP_REQ",
    "DECODE_IMAGE",
    "DECODE_IMAGE_UPDATE",
    "WRITE_TO_CACHE",
    "WAIT_ON_WRITE",
    "DONE"
};

// Log scope
static const char * const LOG_TXT = "Texture";

class LLTextureFetchWorker : public LLWorkerClass, public LLCore::HttpHandler

{
    friend class LLTextureFetch;

private:
    class CacheReadResponder : public LLTextureCache::ReadResponder
    {
    public:

        // Threads:  Ttf
        CacheReadResponder(LLTextureFetch* fetcher, const LLUUID& id, LLImageFormatted* image)
            : mFetcher(fetcher), mID(id)
        {
            setImage(image);
        }

        // Threads:  Ttc
        virtual void completed(bool success)
        {
            LL_PROFILE_ZONE_SCOPED;
            LLTextureFetchWorker* worker = mFetcher->getWorker(mID);
            if (worker)
            {
                worker->callbackCacheRead(success, mFormattedImage, mImageSize, mImageLocal);
            }
        }
    private:
        LLTextureFetch* mFetcher;
        LLUUID mID;
    };

    class CacheWriteResponder : public LLTextureCache::WriteResponder
    {
    public:

        // Threads:  Ttf
        CacheWriteResponder(LLTextureFetch* fetcher, const LLUUID& id)
            : mFetcher(fetcher), mID(id)
        {
        }

        // Threads:  Ttc
        virtual void completed(bool success)
        {
            LL_PROFILE_ZONE_SCOPED;
            LLTextureFetchWorker* worker = mFetcher->getWorker(mID);
            if (worker)
            {
                worker->callbackCacheWrite(success);
            }
        }
    private:
        LLTextureFetch* mFetcher;
        LLUUID mID;
    };

    class DecodeResponder : public LLImageDecodeThread::Responder
    {
    public:

        // Threads:  Ttf
        DecodeResponder(LLTextureFetch* fetcher, const LLUUID& id, LLTextureFetchWorker* worker)
            : mFetcher(fetcher), mID(id)
        {
        }

        // Threads:  Tid
        virtual void completed(bool success, const std::string& error_message, LLImageRaw* raw, LLImageRaw* aux, U32 request_id)
        {
            LL_PROFILE_ZONE_SCOPED;
            LLTextureFetchWorker* worker = mFetcher->getWorker(mID);
            if (worker)
            {
                worker->callbackDecoded(success, error_message, raw, aux, request_id);
            }
        }
    private:
        LLTextureFetch* mFetcher;
        LLUUID mID;
    };

    struct Compare
    {
        // lhs < rhs
        bool operator()(const LLTextureFetchWorker* lhs, const LLTextureFetchWorker* rhs) const
        {
            // greater priority is "less"
            return lhs->mImagePriority > rhs->mImagePriority;
        }
    };

public:

    // Threads:  Ttf
    /*virtual*/ bool doWork(S32 param); // Called from LLWorkerThread::processRequest()

    // Threads:  Ttf
    /*virtual*/ void finishWork(S32 param, bool completed); // called from finishRequest() (WORK THREAD)

    // Threads:  Tmain
    /*virtual*/ bool deleteOK(); // called from update()

    ~LLTextureFetchWorker();

    // Threads:  Ttf
    // Locks:  Mw
    S32 callbackHttpGet(LLCore::HttpResponse * response,
                        bool partial, bool success);

    // Threads:  Ttc
    void callbackCacheRead(bool success, LLImageFormatted* image,
                           S32 imagesize, bool islocal);

    // Threads:  Ttc
    void callbackCacheWrite(bool success);

    // Threads:  Tid
    void callbackDecoded(bool success, const std::string& error_message, LLImageRaw* raw, LLImageRaw* aux, S32 decode_id);

    // Threads:  T*
    void setGetStatus(LLCore::HttpStatus status, const std::string& reason)
    {
        LLMutexLock lock(&mWorkMutex);

        mGetStatus = status;
        mGetReason = reason;
    }

    void setCanUseHTTP(bool can_use_http) { mCanUseHTTP = can_use_http; }
    bool getCanUseHTTP() const { return mCanUseHTTP; }

    void setUrl(const std::string& url) { mUrl = url; }

    LLTextureFetch & getFetcher() { return *mFetcher; }

    // Inherited from LLCore::HttpHandler
    // Threads:  Ttf
    virtual void onCompleted(LLCore::HttpHandle handle, LLCore::HttpResponse * response);

    enum e_state // mState
    {
        // *NOTE:  Do not change the order/value of state variables, some code
        // depends upon specific ordering/adjacency.

        // NOTE: Affects LLTextureBar::draw in lltextureview.cpp (debug hack)
        INVALID = 0,
        INIT,
        LOAD_FROM_TEXTURE_CACHE,
        CACHE_POST,
        LOAD_FROM_NETWORK,
        WAIT_HTTP_RESOURCE,             // Waiting for HTTP resources
        WAIT_HTTP_RESOURCE2,            // Waiting for HTTP resources
        SEND_HTTP_REQ,                  // Commit to sending as HTTP
        WAIT_HTTP_REQ,                  // Request sent, wait for completion
        DECODE_IMAGE,
        DECODE_IMAGE_UPDATE,
        WRITE_TO_CACHE,
        WAIT_ON_WRITE,
        DONE
    };

protected:
    LLTextureFetchWorker(LLTextureFetch* fetcher, FTType f_type,
                         const std::string& url, const LLUUID& id, const LLHost& host,
                         F32 priority, S32 discard, S32 size);

private:

    // Threads:  Tmain
    /*virtual*/ void startWork(S32 param); // called from addWork() (MAIN THREAD)

    // Threads:  Tmain
    /*virtual*/ void endWork(S32 param, bool aborted); // called from doWork() (MAIN THREAD)

    // Locks:  Mw
    void resetFormattedData();

    // get the relative priority of this worker (should map to max virtual size)
    F32 getImagePriority() const;

    // Locks:  Mw
    void setImagePriority(F32 priority);

    // Locks:  Mw (ctor invokes without lock)
    void setDesiredDiscard(S32 discard, S32 size);

    // Locks:  Mw
    void removeFromCache();

    // Threads:  Ttf
    bool writeToCacheComplete();

    // Threads:  Ttf
    void recordTextureStart(bool is_http);

    // Threads:  Ttf
    void recordTextureDone(bool is_http, F64 byte_count);

    void lockWorkMutex() { mWorkMutex.lock(); }
    void unlockWorkMutex() { mWorkMutex.unlock(); }

    // Threads:  Ttf
    // Locks:  Mw
    bool acquireHttpSemaphore()
        {
            llassert(! mHttpHasResource);
            if (mFetcher->mHttpSemaphore >= mFetcher->mHttpHighWater)
            {
                return false;
            }
            mHttpHasResource = true;
            mFetcher->mHttpSemaphore++;
            return true;
        }

    // Threads:  Ttf
    // Locks:  Mw
    void releaseHttpSemaphore()
        {
            llassert(mHttpHasResource);
            mHttpHasResource = false;
            mFetcher->mHttpSemaphore--;
            llassert_always(mFetcher->mHttpSemaphore >= 0);
        }

private:
    enum e_request_state // mSentRequest
    {
        UNSENT = 0,
        QUEUED = 1,
        SENT_SIM = 2
    };
    enum e_write_to_cache_state //mWriteToCacheState
    {
        NOT_WRITE = 0,
        CAN_WRITE = 1,
        SHOULD_WRITE = 2
    };

    e_state mState;
    void setState(e_state new_state);
    LLViewerRegion* getRegion();

    e_write_to_cache_state mWriteToCacheState;
    LLTextureFetch* mFetcher;
    LLPointer<LLImageFormatted> mFormattedImage;
    LLPointer<LLImageRaw>       mRawImage,
                                mAuxImage;
    FTType mFTType;
    LLUUID mID;
    LLHost mHost;
    std::string mUrl;
    U8 mType;
    F32 mImagePriority; // should map to max virtual size
    F32 mRequestedPriority;
    S32 mDesiredDiscard;
    S32 mSimRequestedDiscard;
    S32 mRequestedDiscard;
    S32 mLoadedDiscard;
    S32 mDecodedDiscard;
    LLFrameTimer mRequestedDeltaTimer;
    LLFrameTimer mFetchDeltaTimer;
    LLTimer mCacheReadTimer;
    LLTimer mDecodeTimer;
    LLTimer mCacheWriteTimer;
    LLTimer mFetchTimer;
    LLTimer mStateTimer;
    F32 mCacheReadTime; // time for cache read only
    F32 mDecodeTime;    // time for decode only
    F32 mCacheWriteTime;
    F32 mFetchTime;     // total time from req to finished fetch
    std::map<S32, F32> mStateTimersMap;
    F32 mSkippedStatesTime;
    LLTextureCache::handle_t    mCacheReadHandle,
                                mCacheWriteHandle;
    S32                         mRequestedSize,
                                mRequestedOffset,
                                mDesiredSize,
                                mFileSize,
                                mCachedSize;
    e_request_state mSentRequest;
    handle_t mDecodeHandle;
    bool mLoaded;
    bool mDecoded;
    bool mWritten;
    bool mNeedsAux;
    bool mHaveAllData;
    bool mInLocalCache;
    bool mInCache;
    bool                        mCanUseHTTP;
    S32 mRetryAttempt;
    S32 mActiveCount;
    LLCore::HttpStatus mGetStatus;
    std::string mGetReason;
    LLAdaptiveRetryPolicy mFetchRetryPolicy;
    bool mCanUseCapability;
    LLTimer mRegionRetryTimer;
    S32 mRegionRetryAttempt;
    LLUUID mLastRegionId;


    // Work Data
    LLMutex mWorkMutex;
    U8 mImageCodec;

    LLViewerAssetStats::duration_t mMetricsStartTime;

    LLCore::HttpHandle      mHttpHandle;                // Handle of any active request
    LLCore::BufferArray *   mHttpBufferArray;           // Refcounted pointer to response data
    S32                     mHttpPolicyClass;
    bool                    mHttpActive;                // Active request to http library
    U32                     mHttpReplySize,             // Actual received data size
                            mHttpReplyOffset;           // Actual received data offset
    bool                    mHttpHasResource;           // Counts against Fetcher's mHttpSemaphore

    // State history
    U32                     mCacheReadCount,
                            mCacheWriteCount,
                            mResourceWaitCount;         // Requests entering WAIT_HTTP_RESOURCE2
};

//////////////////////////////////////////////////////////////////////////////

// Cross-thread messaging for asset metrics.

/**
 * @brief Base class for cross-thread requests made of the fetcher
 *
 * I believe the intent of the LLQueuedThread class was to
 * have these operations derived from LLQueuedThread::QueuedRequest
 * but the texture fetcher has elected to manage the queue
 * in its own manner.  So these are free-standing objects which are
 * managed in simple FIFO order on the mCommands queue of the
 * LLTextureFetch object.
 *
 * What each represents is a simple command sent from an
 * outside thread into the TextureFetch thread to be processed
 * in order and in a timely fashion (though not an absolute
 * higher priority than other operations of the thread).
 * Each operation derives a new class from the base customizing
 * members, constructors and the doWork() method to effect
 * the command.
 *
 * The flow is one-directional.  There are two global instances
 * of the LLViewerAssetStats collector, one for the main program's
 * thread pointed to by gViewerAssetStatsMain and one for the
 * TextureFetch thread pointed to by gViewerAssetStatsThread1.
 * Common operations has each thread recording metrics events
 * into the respective collector unconcerned with locking and
 * the state of any other thread.  But when the agent moves into
 * a different region or the metrics timer expires and a report
 * needs to be sent back to the grid, messaging across threads
 * is required to distribute data and perform global actions.
 * In pseudo-UML, it looks like:
 *
 * @verbatim
 *                       Main                 Thread1
 *                        .                      .
 *                        .                      .
 *                     +-----+                   .
 *                     | AM  |                   .
 *                     +--+--+                   .
 *      +-------+         |                      .
 *      | Main  |      +--+--+                   .
 *      |       |      | SRE |---.               .
 *      | Stats |      +-----+    \              .
 *      |       |         |        \  (uuid)  +-----+
 *      | Coll. |      +--+--+      `-------->| SR  |
 *      +-------+      | MSC |                +--+--+
 *         | ^         +-----+                   |
 *         | |  (uuid)  / .                   +-----+ (uuid)
 *         |  `--------'  .                   | MSC |---------.
 *         |              .                   +-----+         |
 *         |           +-----+                   .            v
 *         |           | TE  |                   .        +-------+
 *         |           +--+--+                   .        | Thd1  |
 *         |              |                      .        |       |
 *         |           +-----+                   .        | Stats |
 *          `--------->| RSC |                   .        |       |
 *                     +--+--+                   .        | Coll. |
 *                        |                      .        +-------+
 *                     +--+--+                   .            |
 *                     | SME |---.               .            |
 *                     +-----+    \              .            |
 *                        .        \ (clone)  +-----+         |
 *                        .         `-------->| SM  |         |
 *                        .                   +--+--+         |
 *                        .                      |            |
 *                        .                   +-----+         |
 *                        .                   | RSC |<--------'
 *                        .                   +-----+
 *                        .                      |
 *                        .                   +-----+
 *                        .                   | CP  |--> HTTP POST
 *                        .                   +-----+
 *                        .                      .
 *                        .                      .
 *
 * Key:
 *
 * SRE - Set Region Enqueued.  Enqueue a 'Set Region' command in
 *       the other thread providing the new UUID of the region.
 *       TFReqSetRegion carries the data.
 * SR  - Set Region.  New region UUID is sent to the thread-local
 *       collector.
 * SME - Send Metrics Enqueued.  Enqueue a 'Send Metrics' command
 *       including an ownership transfer of a cloned LLViewerAssetStats.
 *       TFReqSendMetrics carries the data.
 * SM  - Send Metrics.  Global metrics reporting operation.  Takes
 *       the cloned stats from the command, merges it with the
 *       thread's local stats, converts to LLSD and sends it on
 *       to the grid.
 * AM  - Agent Moved.  Agent has completed some sort of move to a
 *       new region.
 * TE  - Timer Expired.  Metrics timer has expired (on the order
 *       of 10 minutes).
 * CP  - CURL Post
 * MSC - Modify Stats Collector.  State change in the thread-local
 *       collector.  Typically a region change which affects the
 *       global pointers used to find the 'current stats'.
 * RSC - Read Stats Collector.  Extract collector data cloning it
 *       (i.e. deep copy) when necessary.
 * @endverbatim
 *
 */
class LLTextureFetch::TFRequest // : public LLQueuedThread::QueuedRequest
{
public:
    // Default ctors and assignment operator are correct.

    virtual ~TFRequest()
        {}

    // Patterned after QueuedRequest's method but expected behavior
    // is different.  Always expected to complete on the first call
    // and work dispatcher will assume the same and delete the
    // request after invocation.
    virtual bool doWork(LLTextureFetch * fetcher) = 0;
};

namespace
{

/**
 * @brief Implements a 'Set Region' cross-thread command.
 *
 * When an agent moves to a new region, subsequent metrics need
 * to be binned into a new or existing stats collection in 1:1
 * relationship with the region.  We communicate this region
 * change across the threads involved in the communication with
 * this message.
 *
 * Corresponds to LLTextureFetch::commandSetRegion()
 */
class TFReqSetRegion : public LLTextureFetch::TFRequest
{
public:
    TFReqSetRegion(U64 region_handle)
        : LLTextureFetch::TFRequest(),
          mRegionHandle(region_handle)
        {}
    TFReqSetRegion & operator=(const TFReqSetRegion &); // Not defined

    virtual ~TFReqSetRegion()
        {}

    virtual bool doWork(LLTextureFetch * fetcher);

public:
    const U64 mRegionHandle;
};


/**
 * @brief Implements a 'Send Metrics' cross-thread command.
 *
 * This is the big operation.  The main thread gathers metrics
 * for a period of minutes into LLViewerAssetStats and other
 * objects then makes a snapshot of the data by cloning the
 * collector.  This command transfers the clone, along with a few
 * additional arguments (UUIDs), handing ownership to the
 * TextureFetch thread.  It then merges its own data into the
 * cloned copy, converts to LLSD and kicks off an HTTP POST of
 * the resulting data to the currently active metrics collector.
 *
 * Corresponds to LLTextureFetch::commandSendMetrics()
 */
class TFReqSendMetrics : public LLTextureFetch::TFRequest
{
public:
    /**
     * Construct the 'Send Metrics' command to have the TextureFetch
     * thread add and log metrics data.
     *
     * @param   caps_url        URL of a "ViewerMetrics" Caps target
     *                          to receive the data.  Does not have to
     *                          be associated with a particular region.
     *
     * @param   session_id      UUID of the agent's session.
     *
     * @param   agent_id        UUID of the agent.  (Being pure here...)
     *
     * @param   main_stats      Pointer to a clone of the main thread's
     *                          LLViewerAssetStats data.  Thread1 takes
     *                          ownership of the copy and disposes of it
     *                          when done.
     */
    TFReqSendMetrics(const std::string & caps_url,
        const LLUUID & session_id,
        const LLUUID & agent_id,
        LLSD& stats_sd);
    TFReqSendMetrics & operator=(const TFReqSendMetrics &); // Not defined

    virtual ~TFReqSendMetrics();

    virtual bool doWork(LLTextureFetch * fetcher);

public:
    const std::string mCapsURL;
    const LLUUID mSessionID;
    const LLUUID mAgentID;
    LLSD mStatsSD;

private:
    LLCore::HttpHandler::ptr_t  mHandler;
};

/*
 * Examines the merged viewer metrics report and if found to be too long,
 * will attempt to truncate it in some reasonable fashion.
 *
 * @param       max_regions     Limit of regions allowed in report.
 *
 * @param       metrics         Full, merged viewer metrics report.
 *
 * @returns     If data was truncated, returns true.
 */
bool truncate_viewer_metrics(int max_regions, LLSD & metrics);

} // end of anonymous namespace


//////////////////////////////////////////////////////////////////////////////

const char* sStateDescs[] = {
    "INVALID",
    "INIT",
    "LOAD_FROM_TEXTURE_CACHE",
    "CACHE_POST",
    "LOAD_FROM_NETWORK",
    "WAIT_HTTP_RESOURCE",
    "WAIT_HTTP_RESOURCE2",
    "SEND_HTTP_REQ",
    "WAIT_HTTP_REQ",
    "DECODE_IMAGE",
    "DECODE_IMAGE_UPDATE",
    "WRITE_TO_CACHE",
    "WAIT_ON_WRITE",
    "DONE"
};

const std::set<S32> LOGGED_STATES = { LLTextureFetchWorker::LOAD_FROM_TEXTURE_CACHE, LLTextureFetchWorker::LOAD_FROM_NETWORK,
                                        LLTextureFetchWorker::WAIT_HTTP_REQ, LLTextureFetchWorker::DECODE_IMAGE_UPDATE, LLTextureFetchWorker::WAIT_ON_WRITE };

// static
volatile bool LLTextureFetch::svMetricsDataBreak(true); // Start with a data break

// called from MAIN THREAD

LLTextureFetchWorker::LLTextureFetchWorker(LLTextureFetch* fetcher,
                                           FTType f_type, // Fetched image type
                                           const std::string& url, // Optional URL
                                           const LLUUID& id,    // Image UUID
                                           const LLHost& host,  // Simulator host
                                           F32 priority,        // Priority
                                           S32 discard,         // Desired discard
                                           S32 size)            // Desired size
    : LLWorkerClass(fetcher, "TextureFetch"),
      LLCore::HttpHandler(),
      mState(INIT),
      mWriteToCacheState(NOT_WRITE),
      mFetcher(fetcher),
      mFTType(f_type),
      mID(id),
      mHost(host),
      mUrl(url),
      mImagePriority(priority),
      mRequestedPriority(0.f),
      mDesiredDiscard(-1),
      mSimRequestedDiscard(-1),
      mRequestedDiscard(-1),
      mLoadedDiscard(-1),
      mDecodedDiscard(-1),
      mCacheReadTime(0.f),
      mCacheWriteTime(0.f),
      mDecodeTime(0.f),
      mFetchTime(0.f),
      mCacheReadHandle(LLTextureCache::nullHandle()),
      mCacheWriteHandle(LLTextureCache::nullHandle()),
      mRequestedSize(0),
      mRequestedOffset(0),
      mDesiredSize(TEXTURE_CACHE_ENTRY_SIZE),
      mFileSize(0),
      mSkippedStatesTime(0),
      mCachedSize(0),
      mLoaded(false),
      mSentRequest(UNSENT),
      mDecodeHandle(0),
      mDecoded(false),
      mWritten(false),
      mNeedsAux(false),
      mHaveAllData(false),
      mInLocalCache(false),
      mInCache(false),
      mCanUseHTTP(true),
      mRetryAttempt(0),
      mActiveCount(0),
      mWorkMutex(),
      mImageCodec(IMG_CODEC_INVALID),
      mMetricsStartTime(0),
      mHttpHandle(LLCORE_HTTP_HANDLE_INVALID),
      mHttpBufferArray(NULL),
      mHttpPolicyClass(mFetcher->mHttpPolicyClass),
      mHttpActive(false),
      mHttpReplySize(0U),
      mHttpReplyOffset(0U),
      mHttpHasResource(false),
      mCacheReadCount(0U),
      mCacheWriteCount(0U),
      mResourceWaitCount(0U),
      mFetchRetryPolicy(10.f,3600.f,2.f,10),
      mCanUseCapability(true),
      mRegionRetryAttempt(0)
{
    mType = host.isOk() ? LLImageBase::TYPE_AVATAR_BAKE : LLImageBase::TYPE_NORMAL;
//  LL_INFOS(LOG_TXT) << "Create: " << mID << " mHost:" << host << " Discard=" << discard << LL_ENDL;
    if (!mFetcher->mDebugPause)
    {
        addWork(0);
    }
    setDesiredDiscard(discard, size);
}

LLTextureFetchWorker::~LLTextureFetchWorker()
{
//  LL_INFOS(LOG_TXT) << "Destroy: " << mID
//          << " Decoded=" << mDecodedDiscard
//          << " Requested=" << mRequestedDiscard
//          << " Desired=" << mDesiredDiscard << LL_ENDL;
    llassert_always(!haveWork());

    lockWorkMutex();                                                    // +Mw (should be useless)
    if (mHttpHasResource)
    {
        // Last-chance catchall to recover the resource.  Using an
        // atomic datatype solely because this can be running in
        // another thread.
        releaseHttpSemaphore();
    }
    if (mHttpActive)
    {
        // Issue a cancel on a live request...
        mFetcher->getHttpRequest().requestCancel(mHttpHandle, LLCore::HttpHandler::ptr_t());
    }
    if (mCacheReadHandle != LLTextureCache::nullHandle() && mFetcher->mTextureCache)
    {
        mFetcher->mTextureCache->readComplete(mCacheReadHandle, true);
    }
    if (mCacheWriteHandle != LLTextureCache::nullHandle() && mFetcher->mTextureCache)
    {
        mFetcher->mTextureCache->writeComplete(mCacheWriteHandle, true);
    }
    mFormattedImage = NULL;
    if (mHttpBufferArray)
    {
        mHttpBufferArray->release();
        mHttpBufferArray = NULL;
    }
    unlockWorkMutex();                                                  // -Mw
    mFetcher->removeFromHTTPQueue(mID, (S32Bytes)0);
    mFetcher->removeHttpWaiter(mID);
    mFetcher->updateStateStats(mCacheReadCount, mCacheWriteCount, mResourceWaitCount);
}

// Locks:  Mw (ctor invokes without lock)
void LLTextureFetchWorker::setDesiredDiscard(S32 discard, S32 size)
{
    bool prioritize = false;
    if (mDesiredDiscard != discard)
    {
        if (!haveWork())
        {
            if (!mFetcher->mDebugPause)
            {
                addWork(0);
            }
        }
        else if (mDesiredDiscard < discard)
        {
            prioritize = true;
        }
        mDesiredDiscard = discard;
        mDesiredSize = size;
    }
    else if (size > mDesiredSize)
    {
        mDesiredSize = size;
        prioritize = true;
    }
    mDesiredSize = llmax(mDesiredSize, TEXTURE_CACHE_ENTRY_SIZE);
    if ((prioritize && mState == INIT) || mState == DONE)
    {
        setState(INIT);
    }
}

// Locks:  Mw
void LLTextureFetchWorker::setImagePriority(F32 priority)
{
    mImagePriority = priority; //should map to max virtual size, abort if zero
}

// Locks:  Mw
void LLTextureFetchWorker::resetFormattedData()
{
    if (mHttpBufferArray)
    {
        mHttpBufferArray->release();
        mHttpBufferArray = NULL;
    }
    if (mFormattedImage.notNull())
    {
        mFormattedImage->deleteData();
    }
    mHttpReplySize = 0;
    mHttpReplyOffset = 0;
    mHaveAllData = false;
}

F32 LLTextureFetchWorker::getImagePriority() const
{
    return mImagePriority;
}

// Threads:  Tmain
void LLTextureFetchWorker::startWork(S32 param)
{
    llassert(mFormattedImage.isNull());
}

// Threads:  Ttf
bool LLTextureFetchWorker::doWork(S32 param)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD;
    if (gNonInteractive)
    {
        return true;
    }
    static const LLCore::HttpStatus http_not_found(HTTP_NOT_FOUND);                     // 404
    static const LLCore::HttpStatus http_service_unavail(HTTP_SERVICE_UNAVAILABLE);     // 503
    static const LLCore::HttpStatus http_not_sat(HTTP_REQUESTED_RANGE_NOT_SATISFIABLE); // 416;

    LLMutexLock lock(&mWorkMutex);                                      // +Mw

    if ((mFetcher->isQuitting() || getFlags(LLWorkerClass::WCF_DELETE_REQUESTED)))
    {
        if (mState < DECODE_IMAGE)
        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_THREAD("tfwdw - state < decode");
            return true; // abort
        }
    }

    if (mImagePriority < F_ALMOST_ZERO)
    {
        if (mState == INIT || mState == LOAD_FROM_NETWORK)
        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_THREAD("tfwdw - priority < 0");
            LL_DEBUGS(LOG_TXT) << mID << " abort: mImagePriority < F_ALMOST_ZERO" << LL_ENDL;
            return true; // abort
        }
    }
    if (mState > CACHE_POST && !mCanUseCapability && mCanUseHTTP)
    {
        if (mRegionRetryAttempt > MAX_CAP_MISSING_RETRIES)
        {
            mCanUseHTTP = false;
        }
        else if (!mRegionRetryTimer.hasExpired())
        {
            return false;
        }
        // else retry
    }
    if(mState > CACHE_POST && !mCanUseHTTP)
    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_THREAD("tfwdw - state > cache_post");
        //nowhere to get data, abort.
        LL_WARNS(LOG_TXT) << mID << " abort, nowhere to get data" << LL_ENDL;
        return true ;
    }

    if (mFetcher->mDebugPause)
    {
        return false; // debug: don't do any work
    }
    if (mID == mFetcher->mDebugID)
    {
        mFetcher->mDebugCount++; // for setting breakpoints
    }

    if (mState != DONE)
    {
        mFetchDeltaTimer.reset();
    }

    if (mState == INIT)
    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_THREAD("tfwdw - INIT");
        mStateTimer.reset();
        mFetchTimer.reset();
        for(auto i : LOGGED_STATES)
        {
            mStateTimersMap[i] = 0;
        }
        mSkippedStatesTime = 0;
        mRawImage = NULL ;
        mRequestedDiscard = -1;
        mLoadedDiscard = -1;
        mDecodedDiscard = -1;
        mRequestedSize = 0;
        mRequestedOffset = 0;
        mFileSize = 0;
        mCachedSize = 0;
        mLoaded = false;
        mSentRequest = UNSENT;
        mDecoded  = false;
        mWritten  = false;
        if (mHttpBufferArray)
        {
            mHttpBufferArray->release();
            mHttpBufferArray = NULL;
        }
        mHttpReplySize = 0;
        mHttpReplyOffset = 0;
        mHaveAllData = false;
        mCacheReadHandle = LLTextureCache::nullHandle();
        mCacheWriteHandle = LLTextureCache::nullHandle();
        setState(LOAD_FROM_TEXTURE_CACHE);
        mInCache = false;
        mDesiredSize = llmax(mDesiredSize, TEXTURE_CACHE_ENTRY_SIZE); // min desired size is TEXTURE_CACHE_ENTRY_SIZE
        LL_DEBUGS(LOG_TXT) << mID << ": Priority: " << llformat("%8.0f",mImagePriority)
                           << " Desired Discard: " << mDesiredDiscard << " Desired Size: " << mDesiredSize << LL_ENDL;

        // fall through
    }

    if (mState == LOAD_FROM_TEXTURE_CACHE)
    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_THREAD("tfwdw - LOAD_FROM_TEXTURE_CACHE");
        if (mCacheReadHandle == LLTextureCache::nullHandle())
        {
            S32 offset = mFormattedImage.notNull() ? mFormattedImage->getDataSize() : 0;
            S32 size = mDesiredSize - offset;
            if (size <= 0)
            {
                setState(CACHE_POST);
                return doWork(param);
                // return false;
            }
            mFileSize = 0;
            mLoaded = false;

            add(LLTextureFetch::sCacheAttempt, 1.0);

            if (mUrl.compare(0, 7, "file://") == 0)
            {
                // read file from local disk
                ++mCacheReadCount;
                std::string filename = mUrl.substr(7, std::string::npos);
                CacheReadResponder* responder = new CacheReadResponder(mFetcher, mID, mFormattedImage);
                mCacheReadTimer.reset();
                mCacheReadHandle = mFetcher->mTextureCache->readFromCache(filename, mID, offset, size, responder);

            }
            else if ((mUrl.empty() || mFTType==FTT_SERVER_BAKE) && mFetcher->canLoadFromCache())
            {
                ++mCacheReadCount;
                CacheReadResponder* responder = new CacheReadResponder(mFetcher, mID, mFormattedImage);
                mCacheReadTimer.reset();
                mCacheReadHandle = mFetcher->mTextureCache->readFromCache(mID,
                                                                          offset, size, responder);;
            }
            else if(!mUrl.empty() && mCanUseHTTP)
            {
                setState(WAIT_HTTP_RESOURCE);
            }
            else
            {
                setState(LOAD_FROM_NETWORK);
            }
        }

        if (mLoaded)
        {
            // Make sure request is complete. *TODO: make this auto-complete
            if (mFetcher->mTextureCache->readComplete(mCacheReadHandle, false))
            {
                mCacheReadHandle = LLTextureCache::nullHandle();
                setState(CACHE_POST);
                add(LLTextureFetch::sCacheHit, 1.0);
                mCacheReadTime = mCacheReadTimer.getElapsedTimeF32();
                // fall through
            }
            else
            {
                //
                //This should never happen
                //
                LL_DEBUGS(LOG_TXT) << mID << " this should never happen" << LL_ENDL;
                return false;
            }
        }
        else
        {
            return false;
        }
    }

    if (mState == CACHE_POST)
    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_THREAD("tfwdw - CACHE_POST");
        mCachedSize = mFormattedImage.notNull() ? mFormattedImage->getDataSize() : 0;
        // Successfully loaded
        if ((mCachedSize >= mDesiredSize) || mHaveAllData)
        {
            // we have enough data, decode it
            llassert_always(mFormattedImage->getDataSize() > 0);
            mLoadedDiscard = mDesiredDiscard;
            if (mLoadedDiscard < 0)
            {
                LL_WARNS(LOG_TXT) << mID << " mLoadedDiscard is " << mLoadedDiscard
                                  << ", should be >=0" << LL_ENDL;
            }
            setState(DECODE_IMAGE);
            mInCache = true;
            mWriteToCacheState = NOT_WRITE ;
            LL_DEBUGS(LOG_TXT) << mID << ": Cached. Bytes: " << mFormattedImage->getDataSize()
                               << " Size: " << llformat("%dx%d",mFormattedImage->getWidth(),mFormattedImage->getHeight())
                               << " Desired Discard: " << mDesiredDiscard << " Desired Size: " << mDesiredSize << LL_ENDL;
            record(LLTextureFetch::sCacheHitRate, LLUnits::Ratio::fromValue(1));
        }
        else
        {
            if (mUrl.compare(0, 7, "file://") == 0)
            {
                // failed to load local file, we're done.
                LL_WARNS(LOG_TXT) << mID << ": abort, failed to load local file " << mUrl << LL_ENDL;
                return true;
            }
            // need more data
            else
            {
                LL_DEBUGS(LOG_TXT) << mID << ": Not in Cache" << LL_ENDL;
                setState(LOAD_FROM_NETWORK);
            }
            record(LLTextureFetch::sCacheHitRate, LLUnits::Ratio::fromValue(0));
            // fall through
        }
    }

    if (mState == LOAD_FROM_NETWORK)
    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_THREAD("tfwdw - LOAD_FROM_NETWORK");
        // Check for retries to previous server failures.
        F32 wait_seconds;
        if (mFetchRetryPolicy.shouldRetry(wait_seconds))
        {
            if (wait_seconds <= 0.0)
            {
                LL_INFOS(LOG_TXT) << mID << " retrying now" << LL_ENDL;
            }
            else
            {
                //LL_INFOS(LOG_TXT) << mID << " waiting to retry for " << wait_seconds << " seconds" << LL_ENDL;
                return false;
            }
        }

        static LLCachedControl<bool> use_http(gSavedSettings, "ImagePipelineUseHTTP", true);

//      if (mHost.isInvalid()) get_url = false;
        if ( use_http && mCanUseHTTP && mUrl.empty())//get http url.
        {
            LLViewerRegion* region = getRegion();
            if (region)
            {
                std::string http_url = region->getViewerAssetUrl();
                if (!http_url.empty())
                {
                    if (mFTType != FTT_DEFAULT)
                    {
                        LL_WARNS(LOG_TXT) << "Trying to fetch a texture of non-default type by UUID. This probably won't work!" << LL_ENDL;
                    }
                    setUrl(http_url + "/?texture_id=" + mID.asString().c_str());
                    LL_DEBUGS(LOG_TXT) << "Texture URL: " << mUrl << LL_ENDL;
                    mWriteToCacheState = CAN_WRITE ; //because this texture has a fixed texture id.
                    mCanUseCapability = true;
                    mRegionRetryAttempt = 0;
                    mLastRegionId = region->getRegionID();
                }
                else
                {
                    mCanUseCapability = false;
                    mRegionRetryAttempt++;
                    mRegionRetryTimer.setTimerExpirySec(CAP_MISSING_EXPIRATION_DELAY);
                    // ex: waiting for caps
                    LL_INFOS_ONCE(LOG_TXT) << "Texture not available via HTTP: empty URL." << LL_ENDL;
                }
            }
            else
            {
                mCanUseCapability = false;
                mRegionRetryAttempt++;
                mRegionRetryTimer.setTimerExpirySec(CAP_MISSING_EXPIRATION_DELAY);
                // This will happen if not logged in or if a region deoes not have HTTP Texture enabled
                //LL_WARNS(LOG_TXT) << "Region not found for host: " << mHost << LL_ENDL;
                LL_INFOS_ONCE(LOG_TXT) << "Texture not available via HTTP: no region " << mUrl << LL_ENDL;
            }
        }
        else if (mFTType == FTT_SERVER_BAKE)
        {
            mWriteToCacheState = CAN_WRITE;
        }

        if (mCanUseCapability && mCanUseHTTP && !mUrl.empty())
        {
            setState(WAIT_HTTP_RESOURCE);
            if(mWriteToCacheState != NOT_WRITE)
            {
                mWriteToCacheState = CAN_WRITE ;
            }
            // don't return, fall through to next state
        }
        else
        {
            return false;
        }
    }

    if (mState == WAIT_HTTP_RESOURCE)
    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_THREAD("tfwdw - WAIT_HTTP_RESOURCE");
        // NOTE:
        // control the number of the http requests issued for:
        // 1, not openning too many file descriptors at the same time;
        // 2, control the traffic of http so udp gets bandwidth.
        //
        // If it looks like we're busy, keep this request here.
        // Otherwise, advance into the HTTP states.

        if (!mHttpHasResource && // sometimes we get into this state when we already have an http resource, go ahead and send the request in that case
            (mFetcher->getHttpWaitersCount() || ! acquireHttpSemaphore()))
        {
            setState(WAIT_HTTP_RESOURCE2);
            mFetcher->addHttpWaiter(this->mID);
            ++mResourceWaitCount;
            return false;
        }

        setState(SEND_HTTP_REQ);
        // *NOTE:  You must invoke releaseHttpSemaphore() if you transition
        // to a state other than SEND_HTTP_REQ or WAIT_HTTP_REQ or abort
        // the request.
    }

    if (mState == WAIT_HTTP_RESOURCE2)
    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_THREAD("tfwdw - WAIT_HTTP_RESOURCE2");
        // Just idle it if we make it to the head...
        return false;
    }

    if (mState == SEND_HTTP_REQ)
    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_THREAD("tfwdw - SEND_HTTP_REQ");
        // Also used in llmeshrepository
        static LLCachedControl<bool> disable_range_req(gSavedSettings, "HttpRangeRequestsDisable", false);

        if (! mCanUseHTTP)
        {
            releaseHttpSemaphore();
            LL_WARNS(LOG_TXT) << mID << " abort: SEND_HTTP_REQ but !mCanUseHTTP" << LL_ENDL;
            return true; // abort
        }

        S32 cur_size = 0;
        if (mFormattedImage.notNull())
        {
            cur_size = mFormattedImage->getDataSize(); // amount of data we already have
            if (mFormattedImage->getDiscardLevel() == 0)
            {
                if (cur_size > 0)
                {
                    // We already have all the data, just decode it
                    mLoadedDiscard = mFormattedImage->getDiscardLevel();
                    if (mLoadedDiscard < 0)
                    {
                        LL_WARNS(LOG_TXT) << mID << " mLoadedDiscard is " << mLoadedDiscard
                                          << ", should be >=0" << LL_ENDL;
                    }
                    setState(DECODE_IMAGE);
                    releaseHttpSemaphore();
                    //return false;
                    return doWork(param);
                }
                else
                {
                    releaseHttpSemaphore();
                    LL_WARNS(LOG_TXT) << mID << " SEND_HTTP_REQ abort: cur_size " << cur_size << " <=0" << LL_ENDL;
                    return true; // abort.
                }
            }
        }
        mRequestedSize = mDesiredSize;
        mRequestedDiscard = mDesiredDiscard;
        mRequestedSize -= cur_size;
        mRequestedOffset = cur_size;
        if (mRequestedOffset)
        {
            // Texture fetching often issues 'speculative' loads that
            // start beyond the end of the actual asset.  Some cache/web
            // systems, e.g. Varnish, will respond to this not with a
            // 416 but with a 200 and the entire asset in the response
            // body.  By ensuring that we always have a partially
            // satisfiable Range request, we avoid that hit to the network.
            // We just have to deal with the overlapping data which is made
            // somewhat harder by the fact that grid services don't necessarily
            // return the Content-Range header on 206 responses.  *Sigh*
            mRequestedOffset -= 1;
            mRequestedSize += 1;
        }
        mHttpHandle = LLCORE_HTTP_HANDLE_INVALID;

        if (mUrl.empty())
        {
            // *FIXME:  This should not be reachable except it has become
            // so after some recent 'work'.  Need to track this down
            // and illuminate the unenlightened.
            LL_WARNS(LOG_TXT) << "HTTP GET request failed for " << mID
                              << " on empty URL." << LL_ENDL;
            resetFormattedData();
            releaseHttpSemaphore();
            return true; // failed
        }

        mRequestedDeltaTimer.reset();
        mLoaded = false;
        mGetStatus = LLCore::HttpStatus();
        mGetReason.clear();
        LL_DEBUGS(LOG_TXT) << "HTTP GET: " << mID << " Offset: " << mRequestedOffset
                           << " Bytes: " << mRequestedSize
                           << " Bandwidth(kbps): " << mFetcher->getTextureBandwidth() << "/" << mFetcher->mMaxBandwidth
                           << LL_ENDL;

        // Will call callbackHttpGet when curl request completes
        // Only server bake images use the returned headers currently, for getting retry-after field.
        LLCore::HttpOptions::ptr_t options = (mFTType == FTT_SERVER_BAKE) ? mFetcher->mHttpOptionsWithHeaders: mFetcher->mHttpOptions;
        if (disable_range_req)
        {
            // 'Range:' requests may be disabled in which case all HTTP
            // texture fetches result in full fetches.  This can be used
            // by people with questionable ISPs or networking gear that
            // doesn't handle these well.
            mHttpHandle = mFetcher->mHttpRequest->requestGet(mHttpPolicyClass,
                                                             mUrl,
                                                             options,
                                                             mFetcher->mHttpHeaders,
                                                             LLCore::HttpHandler::ptr_t(this, &NoOpDeletor));
        }
        else
        {
            mHttpHandle = mFetcher->mHttpRequest->requestGetByteRange(mHttpPolicyClass,
                                                                      mUrl,
                                                                      mRequestedOffset,
                                                                      (mRequestedOffset + mRequestedSize) > HTTP_REQUESTS_RANGE_END_MAX
                                                                      ? 0
                                                                      : mRequestedSize,
                                                                      options,
                                                                      mFetcher->mHttpHeaders,
                                                                      LLCore::HttpHandler::ptr_t(this, &NoOpDeletor));
        }
        if (LLCORE_HTTP_HANDLE_INVALID == mHttpHandle)
        {
            LLCore::HttpStatus status(mFetcher->mHttpRequest->getStatus());
            LL_WARNS(LOG_TXT) << "HTTP GET request failed for " << mID
                              << ", Status: " << status.toTerseString()
                              << " Reason: '" << status.toString() << "'"
                              << LL_ENDL;
            resetFormattedData();
            releaseHttpSemaphore();
            return true; // failed
        }

        mHttpActive = true;
        mFetcher->addToHTTPQueue(mID);
        recordTextureStart(true);
        setState(WAIT_HTTP_REQ);

        // fall through
    }

    if (mState == WAIT_HTTP_REQ)
    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_THREAD("tfwdw - WAIT_HTTP_REQ");
        // *NOTE:  As stated above, all transitions out of this state should
        // call releaseHttpSemaphore().
        if (mLoaded)
        {
            S32 cur_size = mFormattedImage.notNull() ? mFormattedImage->getDataSize() : 0;
            if (mRequestedSize < 0)
            {
                if (http_not_found == mGetStatus)
                {
                    if (mFTType != FTT_MAP_TILE)
                    {
                        LL_WARNS(LOG_TXT) << "Texture missing from server (404): " << mUrl << LL_ENDL;
                    }

                    if(mWriteToCacheState == NOT_WRITE) //map tiles or server bakes
                    {
                        setState(DONE);
                        releaseHttpSemaphore();
                        if (mFTType != FTT_MAP_TILE)
                        {
                            LL_WARNS(LOG_TXT) << mID << " abort: WAIT_HTTP_REQ not found" << LL_ENDL;
                        }
                        return true;
                    }

                    if (mCanUseHTTP && !mUrl.empty() && cur_size <= 0)
                    {
                        LLViewerRegion* region = getRegion();
                        if (!region || mLastRegionId != region->getRegionID())
                        {
                            // cap failure? try on new region.
                            mUrl.clear();
                            ++mRetryAttempt;
                            mLastRegionId.setNull();
                            setState(INIT);
                            return false;
                        }
                    }
                }
                else if (http_service_unavail == mGetStatus)
                {
                    LL_INFOS_ONCE(LOG_TXT) << "Texture server busy (503): " << mUrl << LL_ENDL;
                    if (mCanUseHTTP && !mUrl.empty() && cur_size <= 0)
                    {
                        LLViewerRegion* region = getRegion();
                        if (!region || mLastRegionId != region->getRegionID())
                        {
                            // try on new region.
                            mUrl.clear();
                            ++mRetryAttempt;
                            mLastRegionId.setNull();
                            setState(INIT);
                            return false;
                        }
                    }
                }
                else if (http_not_sat == mGetStatus)
                {
                    // Allowed, we'll accept whatever data we have as complete.
                    mHaveAllData = true;
                }
                else
                {
                    LL_INFOS(LOG_TXT) << "HTTP GET failed for: " << mUrl
                                      << " Status: " << mGetStatus.toTerseString()
                                      << " Reason: '" << mGetReason << "'"
                                      << LL_ENDL;
                }

                if (mFTType != FTT_SERVER_BAKE && mFTType != FTT_MAP_TILE)
                {
                    mUrl.clear();
                }
                if (cur_size > 0)
                {
                    // Use available data
                    mLoadedDiscard = mFormattedImage->getDiscardLevel();
                    if (mLoadedDiscard < 0)
                    {
                        LL_WARNS(LOG_TXT) << mID << " mLoadedDiscard is " << mLoadedDiscard
                                          << ", should be >=0" << LL_ENDL;
                    }
                    setState(DECODE_IMAGE);
                    releaseHttpSemaphore();
                    //return false;
                    return doWork(param);
                }

                // Fail harder
                resetFormattedData();
                setState(DONE);
                releaseHttpSemaphore();
                LL_WARNS(LOG_TXT) << mID << " abort: fail harder" << LL_ENDL;
                return true; // failed
            }

            // Clear the url since we're done with the fetch
            // Note: mUrl is used to check is fetching is required so failure to clear it will force an http fetch
            // next time the texture is requested, even if the data have already been fetched.
            if(mWriteToCacheState != NOT_WRITE && mFTType != FTT_SERVER_BAKE)
            {
                // Why do we want to keep url if NOT_WRITE - is this a proxy for map tiles?
                mUrl.clear();
            }

            if (! mHttpBufferArray || ! mHttpBufferArray->size())
            {
                // no data received.
                if (mHttpBufferArray)
                {
                    mHttpBufferArray->release();
                    mHttpBufferArray = NULL;
                }

                // abort.
                setState(DONE);
                LL_WARNS(LOG_TXT) << mID << " abort: no data received" << LL_ENDL;
                releaseHttpSemaphore();
                return true;
            }

            S32 append_size(static_cast<S32>(mHttpBufferArray->size()));
            S32 total_size(cur_size + append_size);
            S32 src_offset(0);
            llassert_always(append_size == mRequestedSize);
            if (mHttpReplyOffset && mHttpReplyOffset != cur_size)
            {
                // In case of a partial response, our offset may
                // not be trivially contiguous with the data we have.
                // Get back into alignment.
                if ( ((S32)mHttpReplyOffset > cur_size) || (cur_size > (S32)mHttpReplyOffset + append_size))
                {
                    LL_WARNS(LOG_TXT) << "Partial HTTP response produces break in image data for texture "
                                      << mID << ".  Aborting load."  << LL_ENDL;
                    setState(DONE);
                    releaseHttpSemaphore();
                    return true;
                }
                src_offset = cur_size - mHttpReplyOffset;
                append_size -= src_offset;
                total_size -= src_offset;
                mRequestedSize -= src_offset;           // Make requested values reflect useful part
                mRequestedOffset += src_offset;
            }

            U8 * buffer = (U8 *)ll_aligned_malloc_16(total_size);
            if (!buffer)
            {
                // abort. If we have no space for packet, we have not enough space to decode image
                setState(DONE);
                LL_WARNS(LOG_TXT) << mID << " abort: out of memory" << LL_ENDL;
                releaseHttpSemaphore();
                return true;
            }

            if (mFormattedImage.isNull())
            {
                // For now, create formatted image based on extension
                std::string extension = gDirUtilp->getExtension(mUrl);
                mFormattedImage = LLImageFormatted::createFromType(LLImageBase::getCodecFromExtension(extension));
                if (mFormattedImage.isNull())
                {
                    mFormattedImage = new LLImageJ2C; // default
                }
            }

            LLImageDataLock lock(mFormattedImage);

            if (mHaveAllData) //the image file is fully loaded.
            {
                mFileSize = total_size;
            }
            else //the file size is unknown.
            {
                mFileSize = total_size + 1 ; //flag the file is not fully loaded.
            }

            if (cur_size > 0)
            {
                // Copy previously collected data into buffer
                memcpy(buffer, mFormattedImage->getData(), cur_size);
            }
            mHttpBufferArray->read(src_offset, (char *) buffer + cur_size, append_size);

            // NOTE: setData releases current data and owns new data (buffer)
            mFormattedImage->setData(buffer, total_size);

            // Done with buffer array
            mHttpBufferArray->release();
            mHttpBufferArray = NULL;
            mHttpReplySize = 0;
            mHttpReplyOffset = 0;

            mLoadedDiscard = mRequestedDiscard;
            if (mLoadedDiscard < 0)
            {
                LL_WARNS(LOG_TXT) << mID << " mLoadedDiscard is " << mLoadedDiscard
                                  << ", should be >=0" << LL_ENDL;
            }
            setState(DECODE_IMAGE);
            if (mWriteToCacheState != NOT_WRITE)
            {
                mWriteToCacheState = SHOULD_WRITE ;
            }
            releaseHttpSemaphore();
            //return false;
            return doWork(param);
        }
        else
        {
            // *HISTORY:  There was a texture timeout test here originally that
            // would cancel a request that was over 120 seconds old.  That's
            // probably not a good idea.  Particularly rich regions can take
            // an enormous amount of time to load textures.  We'll revisit the
            // various possible timeout components (total request time, connection
            // time, I/O time, with and without retries, etc.) in the future.

            return false;
        }
    }

    if (mState == DECODE_IMAGE)
    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_THREAD("tfwdw - DECODE_IMAGE");
        static LLCachedControl<bool> textures_decode_disabled(gSavedSettings, "TextureDecodeDisabled", false);

        if (textures_decode_disabled)
        {
            // for debug use, don't decode
            setState(DONE);
            return true;
        }

        if (mDesiredDiscard < 0)
        {
            // We aborted, don't decode
            setState(DONE);
            LL_DEBUGS(LOG_TXT) << mID << " DECODE_IMAGE abort: desired discard " << mDesiredDiscard << "<0" << LL_ENDL;
            return true;
        }

        if (mFormattedImage->getDataSize() <= 0)
        {
            LL_WARNS(LOG_TXT) << "Decode entered with invalid mFormattedImage. ID = " << mID << LL_ENDL;

            //abort, don't decode
            setState(DONE);
            LL_DEBUGS(LOG_TXT) << mID << " DECODE_IMAGE abort: (mFormattedImage->getDataSize() <= 0)" << LL_ENDL;
            return true;
        }
        if (mLoadedDiscard < 0)
        {
            LL_WARNS(LOG_TXT) << "Decode entered with invalid mLoadedDiscard. ID = " << mID << LL_ENDL;

            //abort, don't decode
            setState(DONE);
            LL_DEBUGS(LOG_TXT) << mID << " DECODE_IMAGE abort: mLoadedDiscard < 0" << LL_ENDL;
            return true;
        }
        mDecodeTimer.reset();
        mRawImage = NULL;
        mAuxImage = NULL;
        llassert_always(mFormattedImage.notNull());
        S32 discard = mHaveAllData ? 0 : mLoadedDiscard;
        mDecoded  = false;
        setState(DECODE_IMAGE_UPDATE);
        LL_DEBUGS(LOG_TXT) << mID << ": Decoding. Bytes: " << mFormattedImage->getDataSize() << " Discard: " << discard
                           << " All Data: " << mHaveAllData << LL_ENDL;

        // In case worked manages to request decode, be shut down,
        // then init and request decode again with first decode
        // still in progress, assign a sufficiently unique id
        mDecodeHandle = LLAppViewer::getImageDecodeThread()->decodeImage(mFormattedImage,
                                                                       discard,
                                                                       mNeedsAux,
                                                                       new DecodeResponder(mFetcher, mID, this));
        if (mDecodeHandle == 0)
        {
            // Abort, failed to put into queue.
            // Happens if viewer is shutting down
            setState(DONE);
            LL_DEBUGS(LOG_TXT) << mID << " DECODE_IMAGE abort: failed to post for decoding" << LL_ENDL;
            return true;
        }
        // fall though
    }

    if (mState == DECODE_IMAGE_UPDATE)
    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_THREAD("tfwdw - DECODE_IMAGE_UPDATE");
        if (mDecoded)
        {
            mDecodeTime = mDecodeTimer.getElapsedTimeF32();

            if (mDecodedDiscard < 0)
            {
                if (mCachedSize > 0 && !mInLocalCache && mRetryAttempt == 0)
                {
                    // Cache file should be deleted, try again
                    LL_DEBUGS(LOG_TXT) << mID << ": Decode of cached file failed (removed), retrying" << LL_ENDL;
                    llassert_always(mDecodeHandle == 0);
                    mFormattedImage = NULL;
                    ++mRetryAttempt;
                    setState(INIT);
                    //return false;
                    return doWork(param);
                }
                else
                {
                    LL_DEBUGS(LOG_TXT) << "Failed to Decode image " << mID << " after " << mRetryAttempt << " retries" << LL_ENDL;
                    setState(DONE); // failed
                }
            }
            else
            {
                llassert_always(mRawImage.notNull());
                LL_DEBUGS(LOG_TXT) << mID << ": Decoded. Discard: " << mDecodedDiscard
                                   << " Raw Image: " << llformat("%dx%d",mRawImage->getWidth(),mRawImage->getHeight()) << LL_ENDL;
                setState(WRITE_TO_CACHE);
            }
            // fall through
        }
        else
        {
            return false;
        }
    }

    if (mState == WRITE_TO_CACHE)
    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_THREAD("tfwdw - WRITE_TO_CACHE");
        if (mWriteToCacheState != SHOULD_WRITE || mFormattedImage.isNull())
        {
            // If we're in a local cache or we didn't actually receive any new data,
            // or we failed to load anything, skip
            setState(DONE);
            //return false;
            return doWork(param);
        }

        LLImageDataSharedLock lock(mFormattedImage);

        S32 datasize = mFormattedImage->getDataSize();
        if(mFileSize < datasize)//This could happen when http fetching and sim fetching mixed.
        {
            if(mHaveAllData)
            {
                mFileSize = datasize ;
            }
            else
            {
                mFileSize = datasize + 1 ; //flag not fully loaded.
            }
        }
        llassert_always(datasize);
        mWritten = false;
        setState(WAIT_ON_WRITE);
        ++mCacheWriteCount;
        CacheWriteResponder* responder = new CacheWriteResponder(mFetcher, mID);
        // This call might be under work mutex, but mRawImage is not nessesary safe here.
        // If something retrieves it via getRequestFinished() and modifies, image won't
        // be protected by work mutex and won't be safe to use here nor in cache worker.
        // So make sure users of getRequestFinished() does not attempt to modify image while
        // fetcher is working
        mCacheWriteTimer.reset();
        mCacheWriteHandle = mFetcher->mTextureCache->writeToCache(mID,
                                                                  mFormattedImage->getData(), datasize,
                                                                  mFileSize, mRawImage, mDecodedDiscard, responder);
        // fall through
    }

    if (mState == WAIT_ON_WRITE)
    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_THREAD("tfwdw - WAIT_ON_WRITE");
        if (writeToCacheComplete())
        {
            mCacheWriteTime = mCacheWriteTimer.getElapsedTimeF32();
            setState(DONE);
            // fall through
        }
        else
        {
            if (mDesiredDiscard < mDecodedDiscard)
            {
                // We're waiting for this write to complete before we can receive more data
                // (we can't touch mFormattedImage until the write completes)
                // Prioritize the write
                mFetcher->mTextureCache->prioritizeWrite(mCacheWriteHandle);
            }
            return false;
        }
    }

    if (mState == DONE)
    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_THREAD("tfwdw - DONE");
        if (mDecodedDiscard >= 0 && mDesiredDiscard < mDecodedDiscard)
        {
            // More data was requested, return to INIT
            setState(INIT);
            LL_DEBUGS(LOG_TXT) << mID << " more data requested, returning to INIT: "
                               << " mDecodedDiscard " << mDecodedDiscard << ">= 0 && mDesiredDiscard " << mDesiredDiscard
                               << "<" << " mDecodedDiscard " << mDecodedDiscard << LL_ENDL;
            // return false;
            return doWork(param);
        }
        else
        {
            mFetchTime = mFetchTimer.getElapsedTimeF32();
            return true;
        }
    }

    return false;
}                                                                       // -Mw

// Threads:  Ttf
// virtual
void LLTextureFetchWorker::onCompleted(LLCore::HttpHandle handle, LLCore::HttpResponse * response)
{
    LL_PROFILE_ZONE_SCOPED;
    static LLCachedControl<bool> log_to_viewer_log(gSavedSettings, "LogTextureDownloadsToViewerLog", false);
    static LLCachedControl<bool> log_to_sim(gSavedSettings, "LogTextureDownloadsToSimulator", false);
    static LLCachedControl<bool> log_texture_traffic(gSavedSettings, "LogTextureNetworkTraffic", false) ;

    LLMutexLock lock(&mWorkMutex);                                      // +Mw

    mHttpActive = false;

    if (log_to_viewer_log || log_to_sim)
    {
        mFetcher->mTextureInfo.setRequestStartTime(mID, mMetricsStartTime.value());
        mFetcher->mTextureInfo.setRequestType(mID, LLTextureInfoDetails::REQUEST_TYPE_HTTP);
        mFetcher->mTextureInfo.setRequestSize(mID, mRequestedSize);
        mFetcher->mTextureInfo.setRequestOffset(mID, mRequestedOffset);
        mFetcher->mTextureInfo.setRequestCompleteTimeAndLog(mID, LLTimer::getTotalTime());
    }

    static LLCachedControl<F32> fake_failure_rate(gSavedSettings, "TextureFetchFakeFailureRate", 0.0f);
    F32 rand_val = ll_frand();
    F32 rate = fake_failure_rate;
    if (mFTType == FTT_SERVER_BAKE && (fake_failure_rate > 0.0) && (rand_val < fake_failure_rate))
    {
        LL_WARNS(LOG_TXT) << mID << " for debugging, setting fake failure status for texture " << mID
                          << " (rand was " << rand_val << "/" << rate << ")" << LL_ENDL;
        response->setStatus(LLCore::HttpStatus(503));
    }
    bool success = true;
    bool partial = false;
    LLCore::HttpStatus status(response->getStatus());
    if (!status && (mFTType == FTT_SERVER_BAKE))
    {
        LL_INFOS(LOG_TXT) << mID << " state " << e_state_name[mState] << LL_ENDL;
        mFetchRetryPolicy.onFailure(response);
        F32 retry_after;
        if (mFetchRetryPolicy.shouldRetry(retry_after))
        {
            LL_INFOS(LOG_TXT) << mID << " will retry after " << retry_after << " seconds, resetting state to LOAD_FROM_NETWORK" << LL_ENDL;
            mFetcher->removeFromHTTPQueue(mID, S32Bytes(0));
            std::string reason(status.toString());
            setGetStatus(status, reason);
            releaseHttpSemaphore();
            setState(LOAD_FROM_NETWORK);
            return;
        }
        else
        {
            LL_INFOS(LOG_TXT) << mID << " will not retry" << LL_ENDL;
        }
    }
    else
    {
        mFetchRetryPolicy.onSuccess();
    }

    std::string reason(status.toString());
    setGetStatus(status, reason);
    LL_DEBUGS(LOG_TXT) << "HTTP COMPLETE: " << mID
                       << " status: " << status.toTerseString()
                       << " '" << reason << "'"
                       << LL_ENDL;

    if (! status)
    {
        success = false;
        if (mFTType != FTT_MAP_TILE) // missing map tiles are normal, don't complain about them.
        {
            LL_WARNS(LOG_TXT) << "CURL GET FAILED, status: " << status.toTerseString()
                              << " reason: " << reason << LL_ENDL;
        }
    }
    else
    {
        // A warning about partial (HTTP 206) data.  Some grid services
        // do *not* return a 'Content-Range' header in the response to
        // Range requests with a 206 status.  We're forced to assume
        // we get what we asked for in these cases until we can fix
        // the services.
        static const LLCore::HttpStatus par_status(HTTP_PARTIAL_CONTENT);

        partial = (par_status == status);
    }

    S32BytesImplicit data_size = callbackHttpGet(response, partial, success);

    if (log_texture_traffic && data_size > 0)
    {
        // one worker per multiple textures
        std::vector<LLViewerTexture*> textures;
        LLViewerTextureManager::findTextures(mID, textures);
        std::vector<LLViewerTexture*>::iterator iter = textures.begin();
        while (iter != textures.end())
        {
            LLViewerTexture* tex = *iter++;
            if (tex)
            {
                gTotalTextureBytesPerBoostLevel[tex->getBoostLevel()] += data_size;
            }
        }
    }

    mFetcher->removeFromHTTPQueue(mID, data_size);

    recordTextureDone(true, data_size);
}                                                                       // -Mw


// Threads:  Tmain
void LLTextureFetchWorker::endWork(S32 param, bool aborted)
{
    LL_PROFILE_ZONE_SCOPED;
    if (mDecodeHandle != 0)
    {
        // LL::ThreadPool has no operation to cancel a particular work item
        mDecodeHandle = 0;
    }
    mFormattedImage = NULL;
}

//////////////////////////////////////////////////////////////////////////////

// Threads:  Ttf

// virtual
void LLTextureFetchWorker::finishWork(S32 param, bool completed)
{
    LL_PROFILE_ZONE_SCOPED;
    // The following are required in case the work was aborted
    if (mCacheReadHandle != LLTextureCache::nullHandle())
    {
        mFetcher->mTextureCache->readComplete(mCacheReadHandle, true);
        mCacheReadHandle = LLTextureCache::nullHandle();
    }
    if (mCacheWriteHandle != LLTextureCache::nullHandle())
    {
        mFetcher->mTextureCache->writeComplete(mCacheWriteHandle, true);
        mCacheWriteHandle = LLTextureCache::nullHandle();
    }
}

// LLQueuedThread's update() method is asking if it's okay to
// delete this worker.  You'll notice we're not locking in here
// which is a slight concern.  Caller is expected to have made
// this request 'quiet' by whatever means...
//
// Threads:  Tmain

// virtual
bool LLTextureFetchWorker::deleteOK()
{
    bool delete_ok = true;

    if (mHttpActive)
    {
        // HTTP library has a pointer to this worker
        // and will dereference it to do notification.
        delete_ok = false;
    }

    if (WAIT_HTTP_RESOURCE2 == mState)
    {
        if (mFetcher->isHttpWaiter(mID))
        {
            // Don't delete the worker out from under the releaseHttpWaiters()
            // method.  Keep the pointers valid, clean up after that method
            // has recognized the cancelation and removed the UUID from the
            // waiter list.
            delete_ok = false;
        }
    }

    // Allow any pending reads or writes to complete
    if (mCacheReadHandle != LLTextureCache::nullHandle())
    {
        if (!mFetcher->mTextureCache || mFetcher->mTextureCache->readComplete(mCacheReadHandle, true))
        {
            mCacheReadHandle = LLTextureCache::nullHandle();
        }
        else
        {
            delete_ok = false;
        }
    }
    if (mCacheWriteHandle != LLTextureCache::nullHandle())
    {
        if (!mFetcher->mTextureCache || mFetcher->mTextureCache->writeComplete(mCacheWriteHandle))
        {
            mCacheWriteHandle = LLTextureCache::nullHandle();
        }
        else
        {
            delete_ok = false;
        }
    }

    if ((haveWork() &&
         // not ok to delete from these states
         ((mState >= WRITE_TO_CACHE && mState <= WAIT_ON_WRITE))))
    {
        delete_ok = false;
    }

    return delete_ok;
}

// Threads:  Ttf
void LLTextureFetchWorker::removeFromCache()
{
    if (!mInLocalCache)
    {
        mFetcher->mTextureCache->removeFromCache(mID);
    }
}


//////////////////////////////////////////////////////////////////////////////

// Threads:  Ttf
// Locks:  Mw
S32 LLTextureFetchWorker::callbackHttpGet(LLCore::HttpResponse * response,
                                          bool partial, bool success)
{
    S32 data_size = 0 ;

    if (mState != WAIT_HTTP_REQ)
    {
        LL_WARNS(LOG_TXT) << "callbackHttpGet for unrequested fetch worker: " << mID
                          << " req=" << mSentRequest << " state= " << mState << LL_ENDL;
        return data_size;
    }
    if (mLoaded)
    {
        LL_WARNS(LOG_TXT) << "Duplicate callback for " << mID.asString() << LL_ENDL;
        return data_size ; // ignore duplicate callback
    }
    if (success)
    {
        // get length of stream:
        LLCore::BufferArray * body(response->getBody());
        data_size = body ? static_cast<S32>(body->size()) : 0;

        LL_DEBUGS(LOG_TXT) << "HTTP RECEIVED: " << mID.asString() << " Bytes: " << data_size << LL_ENDL;
        if (data_size > 0)
        {
            // *TODO: set the formatted image data here directly to avoid the copy

            // Hold on to body for later copy
            llassert_always(NULL == mHttpBufferArray);
            body->addRef();
            mHttpBufferArray = body;

            if (partial)
            {
                unsigned int offset(0), length(0), full_length(0);
                response->getRange(&offset, &length, &full_length);
                if (! offset && ! length)
                {
                    // This is the case where we receive a 206 status but
                    // there wasn't a useful Content-Range header in the response.
                    // This could be because it was badly formatted but is more
                    // likely due to capabilities services which scrub headers
                    // from responses.  Assume we got what we asked for...
                    mHttpReplySize = data_size;
                    mHttpReplyOffset = mRequestedOffset;
                }
                else
                {
                    mHttpReplySize = length;
                    mHttpReplyOffset = offset;
                }
            }

            if (! partial)
            {
                // Response indicates this is the entire asset regardless
                // of our asking for a byte range.  Mark it so and drop
                // any partial data we might have so that the current
                // response body becomes the entire dataset.
                if (data_size <= mRequestedOffset)
                {
                    LL_WARNS(LOG_TXT) << "Fetched entire texture " << mID
                                      << " when it was expected to be marked complete.  mImageSize:  "
                                      << mFileSize << " datasize:  " << mFormattedImage->getDataSize()
                                      << LL_ENDL;
                }
                mHaveAllData = true;
                llassert_always(mDecodeHandle == 0);
                mFormattedImage = NULL; // discard any previous data we had
            }
            else if (data_size < mRequestedSize)
            {
                mHaveAllData = true;
            }
            else if (data_size > mRequestedSize)
            {
                // *TODO: This shouldn't be happening any more  (REALLY don't expect this anymore)
                LL_WARNS(LOG_TXT) << "data_size = " << data_size << " > requested: " << mRequestedSize << LL_ENDL;
                mHaveAllData = true;
                llassert_always(mDecodeHandle == 0);
                mFormattedImage = NULL; // discard any previous data we had
            }
        }
        else
        {
            // We requested data but received none (and no error),
            // so presumably we have all of it
            mHaveAllData = true;
        }
        mRequestedSize = data_size;

        if (mHaveAllData)
        {
            LLViewerStatsRecorder::instance().textureFetch();
        }

        // *TODO: set the formatted image data here directly to avoid the copy
    }
    else
    {
        mRequestedSize = -1; // error
    }

    mLoaded = true;

    return data_size ;
}

//////////////////////////////////////////////////////////////////////////////

// Threads:  Ttc
void LLTextureFetchWorker::callbackCacheRead(bool success, LLImageFormatted* image,
                                             S32 imagesize, bool islocal)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    LLMutexLock lock(&mWorkMutex);                                      // +Mw
    if (mState != LOAD_FROM_TEXTURE_CACHE)
    {
//      LL_WARNS(LOG_TXT) << "Read callback for " << mID << " with state = " << mState << LL_ENDL;
        return;
    }
    if (success)
    {
        llassert_always(imagesize >= 0);
        mFileSize = imagesize;
        mFormattedImage = image;
        mImageCodec = image->getCodec();
        mInLocalCache = islocal;
        if (mFileSize != 0 && mFormattedImage->getDataSize() >= mFileSize)
        {
            mHaveAllData = true;
        }
    }
    mLoaded = true;
}                                                                       // -Mw

// Threads:  Ttc
void LLTextureFetchWorker::callbackCacheWrite(bool success)
{
    LLMutexLock lock(&mWorkMutex);                                      // +Mw
    if (mState != WAIT_ON_WRITE)
    {
//      LL_WARNS(LOG_TXT) << "Write callback for " << mID << " with state = " << mState << LL_ENDL;
        return;
    }
    mWritten = true;
}                                                                       // -Mw

//////////////////////////////////////////////////////////////////////////////

// Threads:  Tid
void LLTextureFetchWorker::callbackDecoded(bool success, const std::string &error_message, LLImageRaw* raw, LLImageRaw* aux, S32 decode_id)
{
    LLMutexLock lock(&mWorkMutex);                                      // +Mw
    if (mDecodeHandle == 0)
    {
        return; // aborted, ignore
    }
    if (mDecodeHandle != decode_id)
    {
        // Queue doesn't support canceling old requests.
        // This shouldn't normally happen, but in case it's possible that a worked
        // will request decode, be aborted, reinited then start a new decode
        LL_DEBUGS(LOG_TXT) << mID << " received obsolete decode's callback" << LL_ENDL;
        return; // ignore
    }
    if (mState != DECODE_IMAGE_UPDATE)
    {
        LL_DEBUGS(LOG_TXT) << "Decode callback for " << mID << " with state = " << mState << LL_ENDL;
        mDecodeHandle = 0;
        return;
    }
    llassert_always(mFormattedImage.notNull());

    mDecodeHandle = 0;
    if (success)
    {
        llassert_always(raw);
        mRawImage = raw;
        mAuxImage = aux;
        mDecodedDiscard = mFormattedImage->getDiscardLevel();
        LL_DEBUGS(LOG_TXT) << mID << ": Decode Finished. Discard: " << mDecodedDiscard
                           << " Raw Image: " << llformat("%dx%d",mRawImage->getWidth(),mRawImage->getHeight()) << LL_ENDL;
    }
    else
    {
        LL_WARNS(LOG_TXT) << "DECODE FAILED: " << mID << " Discard: " << (S32)mFormattedImage->getDiscardLevel() << ", reason: " << error_message << LL_ENDL;
        removeFromCache();
        mDecodedDiscard = -1; // Redundant, here for clarity and paranoia
    }
    mDecoded = true;
//  LL_INFOS(LOG_TXT) << mID << " : DECODE COMPLETE " << LL_ENDL;
}                                                                       // -Mw

//////////////////////////////////////////////////////////////////////////////

// Threads:  Ttf
bool LLTextureFetchWorker::writeToCacheComplete()
{
    // Complete write to cache
    if (mCacheWriteHandle != LLTextureCache::nullHandle())
    {
        if (!mWritten)
        {
            return false;
        }
        if (mFetcher->mTextureCache->writeComplete(mCacheWriteHandle))
        {
            mCacheWriteHandle = LLTextureCache::nullHandle();
        }
        else
        {
            return false;
        }
    }
    return true;
}


// Threads:  Ttf
void LLTextureFetchWorker::recordTextureStart(bool is_http)
{
    if (! mMetricsStartTime.value())
    {
        mMetricsStartTime = LLViewerAssetStatsFF::get_timestamp();
    }
    LLViewerAssetStatsFF::record_enqueue(LLViewerAssetType::AT_TEXTURE,
                                                 is_http,
                                                 LLImageBase::TYPE_AVATAR_BAKE == mType);
}


// Threads:  Ttf
void LLTextureFetchWorker::recordTextureDone(bool is_http, F64 byte_count)
{
    if (mMetricsStartTime.value())
    {
        LLViewerAssetStatsFF::record_response(LLViewerAssetType::AT_TEXTURE,
                                              is_http,
                                              LLImageBase::TYPE_AVATAR_BAKE == mType,
                                              LLViewerAssetStatsFF::get_timestamp() - mMetricsStartTime,
                                              byte_count);
        mMetricsStartTime = (U32Seconds)0;
    }
    LLViewerAssetStatsFF::record_dequeue(LLViewerAssetType::AT_TEXTURE,
                                                 is_http,
                                                 LLImageBase::TYPE_AVATAR_BAKE == mType);
}


//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
// public

std::string LLTextureFetch::getStateString(S32 state)
{
    if (state < 0 || state > sizeof(e_state_name) / sizeof(char*))
    {
        return llformat("%d", state);
    }

    return e_state_name[state];
}

LLTextureFetch::LLTextureFetch(LLTextureCache* cache, bool threaded, bool qa_mode)
    : LLWorkerThread("TextureFetch", threaded, true),
      mDebugCount(0),
      mDebugPause(false),
      mQueueMutex(),
      mNetworkQueueMutex(),
      mTextureCache(cache),
      mTextureBandwidth(0),
      mHTTPTextureBits(0),
      mTotalHTTPRequests(0),
      mQAMode(qa_mode),
      mHttpRequest(NULL),
      mHttpOptions(),
      mHttpOptionsWithHeaders(),
      mHttpHeaders(),
      mHttpPolicyClass(LLCore::HttpRequest::DEFAULT_POLICY_ID),
      mHttpMetricsHeaders(),
      mHttpMetricsPolicyClass(LLCore::HttpRequest::DEFAULT_POLICY_ID),
      mTotalCacheReadCount(0U),
      mTotalCacheWriteCount(0U),
      mTotalResourceWaitCount(0U),
      mFetchSource(LLTextureFetch::FROM_ALL),
      mOriginFetchSource(LLTextureFetch::FROM_ALL),
      mTextureInfoMainThread(false)
{
    mMaxBandwidth = gSavedSettings.getF32("ThrottleBandwidthKBPS");
    mTextureInfo.setLogging(true);

    LLAppCoreHttp & app_core_http(LLAppViewer::instance()->getAppCoreHttp());
    mHttpRequest = new LLCore::HttpRequest;
    mHttpOptions = LLCore::HttpOptions::ptr_t(new LLCore::HttpOptions);
    mHttpOptionsWithHeaders = LLCore::HttpOptions::ptr_t(new LLCore::HttpOptions);
    mHttpOptionsWithHeaders->setWantHeaders(true);
    mHttpHeaders = LLCore::HttpHeaders::ptr_t(new LLCore::HttpHeaders);
    mHttpHeaders->append(HTTP_OUT_HEADER_ACCEPT, HTTP_CONTENT_IMAGE_X_J2C);
    mHttpPolicyClass = app_core_http.getPolicy(LLAppCoreHttp::AP_TEXTURE);
    mHttpMetricsHeaders = LLCore::HttpHeaders::ptr_t(new LLCore::HttpHeaders);
    mHttpMetricsHeaders->append(HTTP_OUT_HEADER_CONTENT_TYPE, HTTP_CONTENT_LLSD_XML);
    mHttpMetricsPolicyClass = app_core_http.getPolicy(LLAppCoreHttp::AP_REPORTING);
    mHttpHighWater = HTTP_NONPIPE_REQUESTS_HIGH_WATER;
    mHttpLowWater = HTTP_NONPIPE_REQUESTS_LOW_WATER;
    mHttpSemaphore = 0;

    // If that test log has ben requested but not yet created, create it
    if (LLMetricPerformanceTesterBasic::isMetricLogRequested(sTesterName) && !LLMetricPerformanceTesterBasic::getTester(sTesterName))
    {
        sTesterp = new LLTextureFetchTester() ;
        if (!sTesterp->isValid())
        {
            delete sTesterp;
            sTesterp = NULL;
        }
    }
}

LLTextureFetch::~LLTextureFetch()
{
    clearDeleteList();

    while (! mCommands.empty())
    {
        TFRequest * req(mCommands.front());
        mCommands.erase(mCommands.begin());
        delete req;
    }

    mHttpWaitResource.clear();

    delete mHttpRequest;
    mHttpRequest = NULL;

    // ~LLQueuedThread() called here
}

S32 LLTextureFetch::createRequest(FTType f_type, const std::string& url, const LLUUID& id, const LLHost& host, F32 priority,
                                   S32 w, S32 h, S32 c, S32 desired_discard, bool needs_aux, bool can_use_http)
{
    LL_PROFILE_ZONE_SCOPED;
    if (mDebugPause)
    {
        return -1;
    }

    if (f_type == FTT_SERVER_BAKE)
    {
        LL_DEBUGS("Avatar") << " requesting " << id << " " << w << "x" << h << " discard " << desired_discard << " type " << f_type << LL_ENDL;
    }
    LLTextureFetchWorker* worker = getWorker(id) ;
    if (worker)
    {
        if (worker->mHost != host)
        {
            LL_WARNS(LOG_TXT) << "LLTextureFetch::createRequest " << id << " called with multiple hosts: "
                              << host << " != " << worker->mHost << LL_ENDL;
            removeRequest(worker, true);
            worker = NULL;
            return -1;
        }
    }

    S32 desired_size;
    std::string exten = gDirUtilp->getExtension(url);
    //if (f_type == FTT_SERVER_BAKE)
    if ((f_type == FTT_SERVER_BAKE) && !url.empty() && !exten.empty() && (LLImageBase::getCodecFromExtension(exten) != IMG_CODEC_J2C))
    {
        // SH-4030: This case should be redundant with the following one, just
        // breaking it out here to clarify that it's intended behavior.
        llassert(!url.empty() && (!exten.empty() && LLImageBase::getCodecFromExtension(exten) != IMG_CODEC_J2C));

        // Do full requests for baked textures to reduce interim blurring.
        LL_DEBUGS(LOG_TXT) << "full request for " << id << " texture is FTT_SERVER_BAKE" << LL_ENDL;
        desired_size = MAX_IMAGE_DATA_SIZE;
        desired_discard = 0;
    }
    else if (!url.empty() && (!exten.empty() && LLImageBase::getCodecFromExtension(exten) != IMG_CODEC_J2C))
    {
        LL_DEBUGS(LOG_TXT) << "full request for " << id << " exten is not J2C: " << exten << LL_ENDL;
        // Only do partial requests for J2C at the moment
        desired_size = MAX_IMAGE_DATA_SIZE;
        desired_discard = 0;
    }
    else if (desired_discard == 0)
    {
        // if we want the entire image, and we know its size, then get it all
        // (calcDataSizeJ2C() below makes assumptions about how the image
        // was compressed - this code ensures that when we request the entire image,
        // we really do get it.)
        desired_size = MAX_IMAGE_DATA_SIZE;
    }
    else if (w*h*c > 0)
    {
        // If the requester knows the dimensions of the image,
        // this will calculate how much data we need without having to parse the header

        desired_size = LLImageJ2C::calcDataSizeJ2C(w, h, c, desired_discard);
    }
    else
    {
        // If the requester knows nothing about the file, we fetch the smallest
        // amount of data at the lowest resolution (highest discard level) possible.
        desired_size = TEXTURE_CACHE_ENTRY_SIZE;
        desired_discard = MAX_DISCARD_LEVEL;
    }


    if (worker)
    {
        if (worker->wasAborted())
        {
            return -1; // need to wait for previous aborted request to complete
        }
        worker->lockWorkMutex();                                        // +Mw
        if (worker->mState == LLTextureFetchWorker::DONE && worker->mDesiredSize == llmax(desired_size, TEXTURE_CACHE_ENTRY_SIZE) && worker->mDesiredDiscard == desired_discard) {
            worker->unlockWorkMutex();                                  // -Mw

            return -1; // similar request has failed or is in a transitional state
        }
        worker->mActiveCount++;
        worker->mNeedsAux = needs_aux;
        worker->setImagePriority(priority);
        worker->setDesiredDiscard(desired_discard, desired_size);
        worker->setCanUseHTTP(can_use_http);

        //MAINT-4184 url is always empty.  Do not set with it.

        if (!worker->haveWork())
        {
            worker->setState(LLTextureFetchWorker::INIT);
            worker->unlockWorkMutex();                                  // -Mw

            worker->addWork(0);
        }
        else
        {
            worker->unlockWorkMutex();                                  // -Mw
        }
    }
    else
    {
        worker = new LLTextureFetchWorker(this, f_type, url, id, host, priority, desired_discard, desired_size);
        lockQueue();                                                    // +Mfq
        mRequestMap[id] = worker;
        unlockQueue();                                                  // -Mfq

        worker->lockWorkMutex();                                        // +Mw
        worker->mActiveCount++;
        worker->mNeedsAux = needs_aux;
        worker->setCanUseHTTP(can_use_http) ;
        worker->unlockWorkMutex();                                      // -Mw
    }

    LL_DEBUGS(LOG_TXT) << "REQUESTED: " << id << " f_type " << fttype_to_string(f_type)
                       << " Discard: " << desired_discard << " size " << desired_size << LL_ENDL;
    return desired_discard;
}
// Threads:  T*
//
// protected
void LLTextureFetch::addToHTTPQueue(const LLUUID& id)
{
    LL_PROFILE_ZONE_SCOPED;
    LLMutexLock lock(&mNetworkQueueMutex);                              // +Mfnq
    mHTTPTextureQueue.insert(id);
    mTotalHTTPRequests++;
}                                                                       // -Mfnq

// Threads:  T*
void LLTextureFetch::removeFromHTTPQueue(const LLUUID& id, S32Bytes received_size)
{
    LL_PROFILE_ZONE_SCOPED;
    LLMutexLock lock(&mNetworkQueueMutex);                              // +Mfnq
    mHTTPTextureQueue.erase(id);
    mHTTPTextureBits += received_size; // Approximate - does not include header bits
}                                                                       // -Mfnq

// NB:  If you change deleteRequest() you should probably make
// parallel changes in removeRequest().  They're functionally
// identical with only argument variations.
//
// Threads:  T*
void LLTextureFetch::deleteRequest(const LLUUID& id, bool cancel)
{
    LL_PROFILE_ZONE_SCOPED;
    lockQueue();                                                        // +Mfq
    LLTextureFetchWorker* worker = getWorkerAfterLock(id);
    if (worker)
    {
        size_t erased_1 = mRequestMap.erase(worker->mID);
        unlockQueue();                                                  // -Mfq

        llassert_always(erased_1 > 0) ;
        llassert_always(!(worker->getFlags(LLWorkerClass::WCF_DELETE_REQUESTED))) ;

        worker->scheduleDelete();
    }
    else
    {
        unlockQueue();                                                  // -Mfq
    }
}

// NB:  If you change removeRequest() you should probably make
// parallel changes in deleteRequest().  They're functionally
// identical with only argument variations.
//
// Threads:  T*
void LLTextureFetch::removeRequest(LLTextureFetchWorker* worker, bool cancel)
{
    LL_PROFILE_ZONE_SCOPED;
    if(!worker)
    {
        return;
    }

    lockQueue();                                                        // +Mfq
    size_t erased_1 = mRequestMap.erase(worker->mID);
    unlockQueue();                                                      // -Mfq

    llassert_always(erased_1 > 0) ;
    llassert_always(!(worker->getFlags(LLWorkerClass::WCF_DELETE_REQUESTED))) ;

    worker->scheduleDelete();
}

void LLTextureFetch::deleteAllRequests()
{
    while(1)
    {
        lockQueue();
        if(mRequestMap.empty())
        {
            unlockQueue() ;
            break;
        }

        LLTextureFetchWorker* worker = mRequestMap.begin()->second;
        unlockQueue() ;

        removeRequest(worker, true);
    }
}

// Threads:  T*
S32 LLTextureFetch::getNumRequests()
{
    lockQueue();                                                        // +Mfq
    S32 size = (S32)mRequestMap.size();
    unlockQueue();                                                      // -Mfq

    return size;
}

// Threads:  T*
S32 LLTextureFetch::getNumHTTPRequests()
{
    mNetworkQueueMutex.lock();                                          // +Mfq
    S32 size = (S32)mHTTPTextureQueue.size();
    mNetworkQueueMutex.unlock();                                        // -Mfq

    return size;
}

// Threads:  T*
U32 LLTextureFetch::getTotalNumHTTPRequests()
{
    mNetworkQueueMutex.lock();                                          // +Mfq
    U32 size = mTotalHTTPRequests;
    mNetworkQueueMutex.unlock();                                        // -Mfq

    return size;
}

// call lockQueue() first!
// Threads:  T*
// Locks:  Mfq
LLTextureFetchWorker* LLTextureFetch::getWorkerAfterLock(const LLUUID& id)
{
    LL_PROFILE_ZONE_SCOPED;
    LLTextureFetchWorker* res = NULL;
    map_t::iterator iter = mRequestMap.find(id);
    if (iter != mRequestMap.end())
    {
        res = iter->second;
    }
    return res;
}

// Threads:  T*
LLTextureFetchWorker* LLTextureFetch::getWorker(const LLUUID& id)
{
    LLMutexLock lock(&mQueueMutex);                                     // +Mfq

    return getWorkerAfterLock(id);
}                                                                       // -Mfq


// Threads:  T*
bool LLTextureFetch::getRequestFinished(const LLUUID& id, S32& discard_level,
                                        LLPointer<LLImageRaw>& raw, LLPointer<LLImageRaw>& aux,
                                        LLCore::HttpStatus& last_http_get_status)
{
    LL_PROFILE_ZONE_SCOPED;
    bool res = false;
    LLTextureFetchWorker* worker = getWorker(id);
    if (worker)
    {
        if (worker->wasAborted())
        {
            res = true;
        }
        else if (!worker->haveWork())
        {
            // Should only happen if we set mDebugPause...
            if (!mDebugPause)
            {
//              LL_WARNS(LOG_TXT) << "Adding work for inactive worker: " << id << LL_ENDL;
                worker->addWork(0);
            }
        }
        else if (worker->checkWork())
        {
            F32 decode_time;
            F32 fetch_time;
            F32 cache_read_time;
            F32 cache_write_time;
            S32 file_size;
            std::map<S32, F32> logged_state_timers;
            F32 skipped_states_time;
            worker->lockWorkMutex();                                    // +Mw
            last_http_get_status = worker->mGetStatus;
            discard_level = worker->mDecodedDiscard;
            raw = worker->mRawImage;
            aux = worker->mAuxImage;

            decode_time = worker->mDecodeTime;
            fetch_time = worker->mFetchTime;
            cache_read_time = worker->mCacheReadTime;
            cache_write_time = worker->mCacheWriteTime;
            file_size = worker->mFileSize;
            worker->mCacheReadTimer.reset();
            worker->mDecodeTimer.reset();
            worker->mCacheWriteTimer.reset();
            worker->mFetchTimer.reset();
            logged_state_timers = worker->mStateTimersMap;
            skipped_states_time = worker->mSkippedStatesTime;
            worker->mStateTimer.reset();
            res = true;
            LL_DEBUGS(LOG_TXT) << id << ": Request Finished. State: " << worker->mState << " Discard: " << discard_level << LL_ENDL;
            worker->unlockWorkMutex();                                  // -Mw

            sample(sTexDecodeLatency, decode_time);
            sample(sTexFetchLatency, fetch_time);
            sample(sCacheReadLatency, cache_read_time);
            sample(sCacheWriteLatency, cache_write_time);

            static LLCachedControl<F32> min_time_to_log(gSavedSettings, "TextureFetchMinTimeToLog", 2.f);
            if (fetch_time > min_time_to_log)
            {
                //LL_INFOS() << "fetch_time: " << fetch_time << " cache_read_time: " << cache_read_time << " decode_time: " << decode_time << " cache_write_time: " << cache_write_time << LL_ENDL;

                LLTextureFetchTester* tester = (LLTextureFetchTester*)LLMetricPerformanceTesterBasic::getTester(sTesterName);
                if (tester)
                {
                    tester->updateStats(logged_state_timers, fetch_time, skipped_states_time, file_size) ;
                }
            }
        }
        else
        {
            worker->lockWorkMutex();                                    // +Mw
            if ((worker->mDecodedDiscard >= 0) &&
                (worker->mDecodedDiscard < discard_level || discard_level < 0) &&
                (worker->mState >= LLTextureFetchWorker::WAIT_ON_WRITE))
            {
                // Not finished, but data is ready
                discard_level = worker->mDecodedDiscard;
                raw = worker->mRawImage;
                aux = worker->mAuxImage;
            }
            worker->unlockWorkMutex();                                  // -Mw
        }
    }
    else
    {
        res = true;
    }
    return res;
}

// Threads:  T*
bool LLTextureFetch::updateRequestPriority(const LLUUID& id, F32 priority)
{
    LL_PROFILE_ZONE_SCOPED;
    mRequestQueue.tryPost([=, this]()
        {
            LLTextureFetchWorker* worker = getWorker(id);
            if (worker)
            {
                worker->lockWorkMutex();                                        // +Mw
                worker->setImagePriority(priority);
                worker->unlockWorkMutex();                                      // -Mw
            }
        });

    return true;
}

// Replicates and expands upon the base class's
// getPending() implementation.  getPending() and
// runCondition() replicate one another's logic to
// an extent and are sometimes used for the same
// function (deciding whether or not to sleep/pause
// a thread).  So the implementations need to stay
// in step, at least until this can be refactored and
// the redundancy eliminated.
//
// Threads:  T*

//virtual
size_t LLTextureFetch::getPending()
{
    LL_PROFILE_ZONE_SCOPED;
    size_t res;
    lockData();                                                         // +Ct
    {
        LLMutexLock lock(&mQueueMutex);                                 // +Mfq

        res = mRequestQueue.size();
        res += mCommands.size();
    }                                                                   // -Mfq
    unlockData();                                                       // -Ct
    return res;
}

// Locks:  Ct
// virtual
bool LLTextureFetch::runCondition()
{
    // Caller is holding the lock on LLThread's condition variable.

    // LLQueuedThread, unlike its base class LLThread, makes this a
    // private method which is unfortunate.  I want to use it directly
    // but I'm going to have to re-implement the logic here (or change
    // declarations, which I don't want to do right now).
    //
    // Changes here may need to be reflected in getPending().

    bool have_no_commands(false);
    {
        LLMutexLock lock(&mQueueMutex);                                 // +Mfq

        have_no_commands = mCommands.empty();
    }                                                                   // -Mfq

    return ! (have_no_commands
              && (mRequestQueue.size() == 0 && mIdleThread));       // From base class
}

//////////////////////////////////////////////////////////////////////////////

// Threads:  Ttf
void LLTextureFetch::commonUpdate()
{
    LL_PROFILE_ZONE_SCOPED;
    // Update low/high water levels based on pipelining.  We pick
    // up setting eventually, so the semaphore/request level can
    // fall outside the [0..HIGH_WATER] range.  Expect that.
    if (LLAppViewer::instance()->getAppCoreHttp().isPipelined(LLAppCoreHttp::AP_TEXTURE))
    {
        mHttpHighWater = HTTP_PIPE_REQUESTS_HIGH_WATER;
        mHttpLowWater = HTTP_PIPE_REQUESTS_LOW_WATER;
    }
    else
    {
        mHttpHighWater = HTTP_NONPIPE_REQUESTS_HIGH_WATER;
        mHttpLowWater = HTTP_NONPIPE_REQUESTS_LOW_WATER;
    }

    // Release waiters
    releaseHttpWaiters();

    // Run a cross-thread command, if any.
    cmdDoWork();

    // Deliver all completion notifications
    LLCore::HttpStatus status = mHttpRequest->update(0);
    if (! status)
    {
        LL_INFOS_ONCE(LOG_TXT) << "Problem during HTTP servicing.  Reason:  "
                               << status.toString()
                               << LL_ENDL;
    }
}


// Threads:  Tmain

//virtual
size_t LLTextureFetch::update(F32 max_time_ms)
{
    LL_PROFILE_ZONE_SCOPED;
    static LLCachedControl<F32> band_width(gSavedSettings,"ThrottleBandwidthKBPS", 3000.0);

    {
        mNetworkQueueMutex.lock();                                      // +Mfnq
        mMaxBandwidth = band_width();

        add(LLStatViewer::TEXTURE_NETWORK_DATA_RECEIVED, mHTTPTextureBits);
        mHTTPTextureBits = (U32Bits)0;

        mNetworkQueueMutex.unlock();                                    // -Mfnq
    }

    size_t res = LLWorkerThread::update(max_time_ms);

    if (!mThreaded)
    {
        commonUpdate();
    }

    return res;
}

// called in the MAIN thread after the TextureCacheThread shuts down.
//
// Threads:  Tmain
void LLTextureFetch::shutDownTextureCacheThread()
{
    if(mTextureCache)
    {
        llassert_always(mTextureCache->isQuitting() || mTextureCache->isStopped()) ;
        mTextureCache = NULL ;
    }
}

// Threads:  Ttf
void LLTextureFetch::startThread()
{
    mTextureInfo.startRecording();
}

// Threads:  Ttf
void LLTextureFetch::endThread()
{
    LL_INFOS(LOG_TXT) << "CacheReads:  " << mTotalCacheReadCount
                      << ", CacheWrites:  " << mTotalCacheWriteCount
                      << ", ResWaits:  " << mTotalResourceWaitCount
                      << ", TotalHTTPReq:  " << getTotalNumHTTPRequests()
                      << LL_ENDL;

    mTextureInfo.stopRecording();
}

// Threads:  Ttf
void LLTextureFetch::threadedUpdate()
{
    LL_PROFILE_ZONE_SCOPED;
    llassert_always(mHttpRequest);

#if 0
    // Limit update frequency
    const F32 PROCESS_TIME = 0.05f;
    static LLFrameTimer process_timer;
    if (process_timer.getElapsedTimeF32() < PROCESS_TIME)
    {
        return;
    }
    process_timer.reset();
#endif

    commonUpdate();

#if 0
    const F32 INFO_TIME = 1.0f;
    static LLFrameTimer info_timer;
    if (info_timer.getElapsedTimeF32() >= INFO_TIME)
    {
        S32 q = mCurlGetRequest->getQueued();
        if (q > 0)
        {
            LL_INFOS(LOG_TXT) << "Queued gets: " << q << LL_ENDL;
            info_timer.reset();
        }
    }
#endif
}

void LLTextureFetchWorker::setState(e_state new_state)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    if (mFTType == FTT_SERVER_BAKE)
    {
    // NOTE: turning on these log statements is a reliable way to get
    // blurry images fairly frequently. Presumably this is an
    // indication of some subtle timing or locking issue.

//      LL_INFOS(LOG_TXT) << "id: " << mID << " FTType: " << mFTType << " disc: " << mDesiredDiscard << " sz: " << mDesiredSize << " state: " << e_state_name[mState] << " => " << e_state_name[new_state] << LL_ENDL;
    }

    F32 d_time = mStateTimer.getElapsedTimeF32();
    if (d_time >= 0.0001F)
    {
        if (LOGGED_STATES.count(mState))
        {
            mStateTimersMap[mState] = d_time;
        }
        else
        {
            mSkippedStatesTime += d_time;
        }
    }

    mStateTimer.reset();
    mState = new_state;
}

LLViewerRegion* LLTextureFetchWorker::getRegion()
{
    LLViewerRegion* region = NULL;
    if (mHost.isInvalid())
    {
        region = gAgent.getRegion();
    }
    else if (LLWorld::instanceExists())
    {
        region = LLWorld::getInstance()->getRegion(mHost);
    }
    return region;
}

//////////////////////////////////////////////////////////////////////////////

// Threads:  T*
bool LLTextureFetch::isFromLocalCache(const LLUUID& id)
{
    bool from_cache = false ;

    LLTextureFetchWorker* worker = getWorker(id);
    if (worker)
    {
        worker->lockWorkMutex();                                        // +Mw
        from_cache = worker->mInLocalCache;
        worker->unlockWorkMutex();                                      // -Mw
    }

    return from_cache ;
}

S32 LLTextureFetch::getFetchState(const LLUUID& id)
{
    S32 state = LLTextureFetchWorker::INVALID;
    LLTextureFetchWorker* worker = getWorker(id);
    if (worker && worker->haveWork())
    {
        state = worker->mState;
    }

    return state;
}

// Threads:  T*
S32 LLTextureFetch::getFetchState(const LLUUID& id, F32& data_progress_p, F32& requested_priority_p,
                                  U32& fetch_priority_p, F32& fetch_dtime_p, F32& request_dtime_p, bool& can_use_http)
{
    LL_PROFILE_ZONE_SCOPED;
    S32 state = LLTextureFetchWorker::INVALID;
    F32 data_progress = 0.0f;
    F32 requested_priority = 0.0f;
    F32 fetch_dtime = 999999.f;
    F32 request_dtime = 999999.f;
    U32 fetch_priority = 0;

    LLTextureFetchWorker* worker = getWorker(id);
    if (worker && worker->haveWork())
    {
        worker->lockWorkMutex();                                        // +Mw
        state = worker->mState;
        fetch_dtime = worker->mFetchDeltaTimer.getElapsedTimeF32();
        request_dtime = worker->mRequestedDeltaTimer.getElapsedTimeF32();
        if (worker->mFileSize > 0)
        {
            if (worker->mFormattedImage.notNull())
            {
                data_progress = (F32)worker->mFormattedImage->getDataSize() / (F32)worker->mFileSize;
            }
        }
        if (state >= LLTextureFetchWorker::LOAD_FROM_NETWORK && state <= LLTextureFetchWorker::WAIT_HTTP_REQ)
        {
            requested_priority = worker->mRequestedPriority;
        }
        else
        {
            requested_priority = worker->mImagePriority;
        }
        fetch_priority = (U32)worker->getImagePriority();
        can_use_http = worker->getCanUseHTTP() ;
        worker->unlockWorkMutex();                                      // -Mw
    }
    data_progress_p = data_progress;
    requested_priority_p = requested_priority;
    fetch_priority_p = fetch_priority;
    fetch_dtime_p = fetch_dtime;
    request_dtime_p = request_dtime;
    return state;
}

void LLTextureFetch::dump()
{
    LL_INFOS(LOG_TXT) << "LLTextureFetch ACTIVE_HTTP:" << LL_ENDL;
    for (queue_t::const_iterator iter(mHTTPTextureQueue.begin());
         mHTTPTextureQueue.end() != iter;
         ++iter)
    {
        LL_INFOS(LOG_TXT) << " ID: " << (*iter) << LL_ENDL;
    }

    LL_INFOS(LOG_TXT) << "LLTextureFetch WAIT_HTTP_RESOURCE:" << LL_ENDL;
    for (wait_http_res_queue_t::const_iterator iter(mHttpWaitResource.begin());
         mHttpWaitResource.end() != iter;
         ++iter)
    {
        LL_INFOS(LOG_TXT) << " ID: " << (*iter) << LL_ENDL;
    }
}

//////////////////////////////////////////////////////////////////////////////

// HTTP Resource Waiting Methods

// Threads:  Ttf
void LLTextureFetch::addHttpWaiter(const LLUUID & tid)
{
    mNetworkQueueMutex.lock();                                          // +Mfnq
    mHttpWaitResource.insert(tid);
    mNetworkQueueMutex.unlock();                                        // -Mfnq
}

// Threads:  Ttf
void LLTextureFetch::removeHttpWaiter(const LLUUID & tid)
{
    mNetworkQueueMutex.lock();                                          // +Mfnq
    wait_http_res_queue_t::iterator iter(mHttpWaitResource.find(tid));
    if (mHttpWaitResource.end() != iter)
    {
        mHttpWaitResource.erase(iter);
    }
    mNetworkQueueMutex.unlock();                                        // -Mfnq
}

// Threads:  T*
bool LLTextureFetch::isHttpWaiter(const LLUUID & tid)
{
    mNetworkQueueMutex.lock();                                          // +Mfnq
    wait_http_res_queue_t::iterator iter(mHttpWaitResource.find(tid));
    const bool ret(mHttpWaitResource.end() != iter);
    mNetworkQueueMutex.unlock();                                        // -Mfnq
    return ret;
}

// Release as many requests as permitted from the WAIT_HTTP_RESOURCE2
// state to the SEND_HTTP_REQ state based on their current priority.
//
// This data structures and code associated with this looks a bit
// indirect and naive but it's done in the name of safety.  An
// ordered container may become invalid from time to time due to
// priority changes caused by actions in other threads.  State itself
// could also suffer the same fate with canceled operations.  Even
// done this way, I'm not fully trusting we're truly safe.  This
// module is due for a major refactoring and we'll deal with it then.
//
// Threads:  Ttf
// Locks:  -Mw (must not hold any worker when called)
void LLTextureFetch::releaseHttpWaiters()
{
    LL_PROFILE_ZONE_SCOPED;
    // Use mHttpSemaphore rather than mHTTPTextureQueue.size()
    // to avoid a lock.
    if (mHttpSemaphore >= mHttpLowWater)
        return;
    S32 needed(mHttpHighWater - mHttpSemaphore);
    if (needed <= 0)
    {
        // Would only happen if High/LowWater were changed behind
        // our back.  In that case, defer fill until usage falls within
        // limits.
        return;
    }

    // Quickly make a copy of all the LLUIDs.  Get off the
    // mutex as early as possible.
    typedef std::vector<LLUUID> uuid_vec_t;
    uuid_vec_t tids;

    {
        LLMutexLock lock(&mNetworkQueueMutex);                          // +Mfnq

        if (mHttpWaitResource.empty())
            return;
        tids.reserve(mHttpWaitResource.size());
        tids.assign(mHttpWaitResource.begin(), mHttpWaitResource.end());
    }                                                                   // -Mfnq

    // Now lookup the UUUIDs to find valid requests and sort
    // them in priority order, highest to lowest.  We're going
    // to modify priority later as a side-effect of releasing
    // these objects.  That, in turn, would violate the partial
    // ordering assumption of std::set, std::map, etc. so we
    // don't use those containers.  We use a vector and an explicit
    // sort to keep the containers valid later.
    typedef std::vector<LLTextureFetchWorker *> worker_list_t;
    worker_list_t tids2;

    tids2.reserve(tids.size());
    for (uuid_vec_t::iterator iter(tids.begin());
         tids.end() != iter;
         ++iter)
    {
        LLTextureFetchWorker * worker(getWorker(* iter));
        if (worker)
        {
            tids2.push_back(worker);
        }
        else
        {
            // If worker isn't found, this should be due to a request
            // for deletion.  We signal our recognition that this
            // uuid shouldn't be used for resource waiting anymore by
            // erasing it from the resource waiter list.  That allows
            // deleteOK to do final deletion on the worker.
            removeHttpWaiter(* iter);
        }
    }
    tids.clear();

    // Sort into priority order, if necessary and only as much as needed
    if (tids2.size() > needed)
    {
        LLTextureFetchWorker::Compare compare;
        std::partial_sort(tids2.begin(), tids2.begin() + needed, tids2.end(), compare);
    }

    // Release workers up to the high water mark.  Since we aren't
    // holding any locks at this point, we can be in competition
    // with other callers.  Do defensive things like getting
    // refreshed counts of requests and checking if someone else
    // has moved any worker state around....
    for (worker_list_t::iterator iter2(tids2.begin()); tids2.end() != iter2; ++iter2)
    {
        LLTextureFetchWorker * worker(* iter2);

        worker->lockWorkMutex();                                        // +Mw
        if (LLTextureFetchWorker::WAIT_HTTP_RESOURCE2 != worker->mState)
        {
            // Not in expected state, remove it, try the next one
            worker->unlockWorkMutex();                                  // -Mw
            LL_WARNS(LOG_TXT) << "Resource-waited texture " << worker->mID
                              << " in unexpected state:  " << worker->mState
                              << ".  Removing from wait list."
                              << LL_ENDL;
            removeHttpWaiter(worker->mID);
            continue;
        }

        if (! worker->acquireHttpSemaphore())
        {
            // Out of active slots, quit
            worker->unlockWorkMutex();                                  // -Mw
            break;
        }

        worker->setState(LLTextureFetchWorker::SEND_HTTP_REQ);
        worker->unlockWorkMutex();                                      // -Mw

        removeHttpWaiter(worker->mID);
    }
}

// Threads:  T*
void LLTextureFetch::cancelHttpWaiters()
{
    mNetworkQueueMutex.lock();                                          // +Mfnq
    mHttpWaitResource.clear();
    mNetworkQueueMutex.unlock();                                        // -Mfnq
}

// Threads:  T*
int LLTextureFetch::getHttpWaitersCount()
{
    mNetworkQueueMutex.lock();                                          // +Mfnq
    int ret(static_cast<int>(mHttpWaitResource.size()));
    mNetworkQueueMutex.unlock();                                        // -Mfnq
    return ret;
}


// Threads:  T*
void LLTextureFetch::updateStateStats(U32 cache_read, U32 cache_write, U32 res_wait)
{
    LLMutexLock lock(&mQueueMutex);                                     // +Mfq

    mTotalCacheReadCount += cache_read;
    mTotalCacheWriteCount += cache_write;
    mTotalResourceWaitCount += res_wait;
}                                                                       // -Mfq


// Threads:  T*
void LLTextureFetch::getStateStats(U32 * cache_read, U32 * cache_write, U32 * res_wait)
{
    U32 ret1(0U), ret2(0U), ret3(0U);

    {
        LLMutexLock lock(&mQueueMutex);                                 // +Mfq
        ret1 = mTotalCacheReadCount;
        ret2 = mTotalCacheWriteCount;
        ret3 = mTotalResourceWaitCount;
    }                                                                   // -Mfq

    *cache_read = ret1;
    *cache_write = ret2;
    *res_wait = ret3;
}

//////////////////////////////////////////////////////////////////////////////

// cross-thread command methods

// Threads:  T*
void LLTextureFetch::commandSetRegion(U64 region_handle)
{
    TFReqSetRegion * req = new TFReqSetRegion(region_handle);

    cmdEnqueue(req);
}

// Threads:  T*
void LLTextureFetch::commandSendMetrics(const std::string & caps_url,
                                        const LLUUID & session_id,
                                        const LLUUID & agent_id,
                                        LLSD& stats_sd)
{
    TFReqSendMetrics * req = new TFReqSendMetrics(caps_url, session_id, agent_id, stats_sd);

    cmdEnqueue(req);
}

// Threads:  T*
void LLTextureFetch::commandDataBreak()
{
    // The pedantically correct way to implement this is to create a command
    // request object in the above fashion and enqueue it.  However, this is
    // simple data of an advisorial not operational nature and this case
    // of shared-write access is tolerable.

    LLTextureFetch::svMetricsDataBreak = true;
}

// Threads:  T*
void LLTextureFetch::cmdEnqueue(TFRequest * req)
{
    LL_PROFILE_ZONE_SCOPED;
    lockQueue();                                                        // +Mfq
    mCommands.push_back(req);
    unlockQueue();                                                      // -Mfq

    unpause();
}

// Threads:  T*
LLTextureFetch::TFRequest * LLTextureFetch::cmdDequeue()
{
    LL_PROFILE_ZONE_SCOPED;
    TFRequest * ret = 0;

    lockQueue();                                                        // +Mfq
    if (! mCommands.empty())
    {
        ret = mCommands.front();
        mCommands.erase(mCommands.begin());
    }
    unlockQueue();                                                      // -Mfq

    return ret;
}

// Threads:  Ttf
void LLTextureFetch::cmdDoWork()
{
    LL_PROFILE_ZONE_SCOPED;
    if (mDebugPause)
    {
        return;  // debug: don't do any work
    }

    TFRequest * req = cmdDequeue();
    if (req)
    {
        // One request per pass should really be enough for this.
        req->doWork(this);
        delete req;
    }
}

//////////////////////////////////////////////////////////////////////////////

// Private (anonymous) class methods implementing the command scheme.

namespace
{


// Example of a simple notification handler for metrics
// delivery notification.  Earlier versions of the code used
// a Responder that tried harder to detect delivery breaks
// but it really isn't that important.  If someone wants to
// revisit that effort, here is a place to start.
class AssetReportHandler : public LLCore::HttpHandler
{
public:

    // Threads:  Ttf
    virtual void onCompleted(LLCore::HttpHandle handle, LLCore::HttpResponse * response)
    {
        LLCore::HttpStatus status(response->getStatus());

        if (status)
        {
            LL_DEBUGS(LOG_TXT) << "Successfully delivered asset metrics to grid."
                               << LL_ENDL;
        }
        else
        {
            LL_WARNS(LOG_TXT) << "Error delivering asset metrics to grid.  Status:  "
                              << status.toTerseString()
                              << ", Reason:  " << status.toString() << LL_ENDL;
        }
    }
}; // end class AssetReportHandler

/**
 * Implements the 'Set Region' command.
 *
 * Thread:  Thread1 (TextureFetch)
 */
bool
TFReqSetRegion::doWork(LLTextureFetch *)
{
    LLViewerAssetStatsFF::set_region(mRegionHandle);

    return true;
}

TFReqSendMetrics::TFReqSendMetrics(const std::string & caps_url,
                                   const LLUUID & session_id,
                                   const LLUUID & agent_id,
                                   LLSD& stats_sd):
    LLTextureFetch::TFRequest(),
    mCapsURL(caps_url),
    mSessionID(session_id),
    mAgentID(agent_id),
    mStatsSD(stats_sd),
    mHandler(new AssetReportHandler)
{}


TFReqSendMetrics::~TFReqSendMetrics()
{
}


/**
 * Implements the 'Send Metrics' command.  Takes over
 * ownership of the passed LLViewerAssetStats pointer.
 *
 * Thread:  Thread1 (TextureFetch)
 */
bool
TFReqSendMetrics::doWork(LLTextureFetch * fetcher)
{
    LL_PROFILE_ZONE_SCOPED;

    //if (! gViewerAssetStatsThread1)
    //  return true;

    static std::atomic<bool> reporting_started(false);
    static std::atomic<S32> report_sequence(0);

    // In mStatsSD, we have a copy we own of the LLSD representation
    // of the asset stats. Add some additional fields and ship it off.

    static const S32 metrics_data_version = 2;

    bool initial_report = !reporting_started;
    mStatsSD["session_id"] = mSessionID;
    mStatsSD["agent_id"] = mAgentID;
    mStatsSD["message"] = "ViewerAssetMetrics";
    mStatsSD["sequence"] = report_sequence;
    mStatsSD["initial"] = initial_report;
    mStatsSD["version"] = metrics_data_version;
    mStatsSD["break"] = static_cast<bool>(LLTextureFetch::svMetricsDataBreak);

    // Update sequence number
    if (S32_MAX == ++report_sequence)
    {
        report_sequence = 0;
    }
    reporting_started = true;

    // Limit the size of the stats report if necessary.

    mStatsSD["truncated"] = truncate_viewer_metrics(10, mStatsSD);

    if (gSavedSettings.getBOOL("QAModeMetrics"))
    {
        dump_sequential_xml("metric_asset_stats",mStatsSD);
    }

    if (! mCapsURL.empty())
    {
        // Don't care about handle, this is a fire-and-forget operation.
        LLCoreHttpUtil::requestPostWithLLSD(&fetcher->getHttpRequest(),
                                            fetcher->getMetricsPolicyClass(),
                                            mCapsURL,
                                            mStatsSD,
                                            LLCore::HttpOptions::ptr_t(),
                                            fetcher->getMetricsHeaders(),
                                            mHandler);
        LLTextureFetch::svMetricsDataBreak = false;
    }
    else
    {
        LLTextureFetch::svMetricsDataBreak = true;
    }

    // In QA mode, Metrics submode, log the result for ease of testing
    if (fetcher->isQAMode())
    {
        LL_INFOS(LOG_TXT) << "ViewerAssetMetrics as submitted\n" << ll_pretty_print_sd(mStatsSD) << LL_ENDL;
    }

    return true;
}


bool
truncate_viewer_metrics(int max_regions, LLSD & metrics)
{
    static const LLSD::String reg_tag("regions");
    static const LLSD::String duration_tag("duration");

    LLSD & reg_map(metrics[reg_tag]);
    if (reg_map.size() <= max_regions)
    {
        return false;
    }

    // Build map of region hashes ordered by duration
    typedef std::multimap<LLSD::Real, int> reg_ordered_list_t;
    reg_ordered_list_t regions_by_duration;

    int ind(0);
    LLSD::array_const_iterator it_end(reg_map.endArray());
    for (LLSD::array_const_iterator it(reg_map.beginArray()); it_end != it; ++it, ++ind)
    {
        LLSD::Real duration = (*it)[duration_tag].asReal();
        regions_by_duration.insert(reg_ordered_list_t::value_type(duration, ind));
    }

    // Build a replacement regions array with the longest-persistence regions
    LLSD new_region(LLSD::emptyArray());
    reg_ordered_list_t::const_reverse_iterator it2_end(regions_by_duration.rend());
    reg_ordered_list_t::const_reverse_iterator it2(regions_by_duration.rbegin());
    for (int i(0); i < max_regions && it2_end != it2; ++i, ++it2)
    {
        new_region.append(reg_map[it2->second]);
    }
    reg_map = new_region;

    return true;
}

} // end of anonymous namespace

LLTextureFetchTester::LLTextureFetchTester() : LLMetricPerformanceTesterBasic(sTesterName)
{
    mTextureFetchTime = 0;
    mSkippedStatesTime = 0;
    mFileSize = 0;
}

LLTextureFetchTester::~LLTextureFetchTester()
{
    outputTestResults();
    LLTextureFetch::sTesterp = NULL;
}

//virtual
void LLTextureFetchTester::outputTestRecord(LLSD *sd)
{
    std::string currentLabel = getCurrentLabelName();

    (*sd)[currentLabel]["Texture Fetch Time"]   = (LLSD::Real)mTextureFetchTime;
    (*sd)[currentLabel]["File Size"]            = (LLSD::Integer)mFileSize;
    (*sd)[currentLabel]["Skipped States Time"]  = (LLSD::String)llformat("%.6f", mSkippedStatesTime);

    for(auto i : LOGGED_STATES)
    {
        (*sd)[currentLabel][sStateDescs[i]] = mStateTimersMap[i];
    }
}

void LLTextureFetchTester::updateStats(const std::map<S32, F32> state_timers, const F32 fetch_time, const F32 skipped_states_time, const S32 file_size)
{
    mTextureFetchTime = fetch_time;
    mStateTimersMap = state_timers;
    mFileSize = file_size;
    mSkippedStatesTime = skipped_states_time;
    outputTestResults();
}

