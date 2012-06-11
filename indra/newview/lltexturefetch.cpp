/** 
 * @file lltexturefetch.cpp
 * @brief Object which fetches textures from the cache and/or network
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2012, Linden Research, Inc.
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

#include "llstl.h"

#include "lltexturefetch.h"

#include "lldir.h"
#include "llhttpclient.h"
#include "llhttpstatuscodes.h"
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
#include "llviewerassetstats.h"
#include "llworld.h"
#include "llsdserialize.h"
#include "llviewertexturelist.h" // debug

#include "httprequest.h"
#include "httphandler.h"
#include "httpresponse.h"
#include "bufferarray.h"


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
// (ASCII art needed)
//
//
// Priority Scheme
//
// [PRIORITY_LOW, PRIORITY_NORMAL)   - for WAIT_HTTP_RESOURCE state
// [PRIORITY_NORMAL, PRIORITY_HIGH)  - waiting for external event
// [PRIORITY_HIGH, PRIORITY_URGENT)  - External event delivered,
//                                     rapidly transitioning through states,
//                                     no waiting allowed
//
// By itself, the above work queue model would fail the concurrency
// and liveness requirements of the interface.  A high priority
// request could find itself on the head and stalled for external
// reasons (see VWR-28996).  So a few additional constraints are
// required to keep things running:
// * Anything that can make forward progress must be kept at a
//   higher priority than anything that can't.
// * On completion of external events, the associated request
//   needs to be elevated beyond the normal range to handle
//   any data delivery and release any external resource.
//
// This effort is made to keep higher-priority entities moving
// forward in their state machines at every possible step of
// processing.  It's not entirely proven that this produces the
// experiencial benefits promised.
//

//////////////////////////////////////////////////////////////////////////////

// Tuning/Parameterization Constants

static const S32 HTTP_REQUESTS_IN_QUEUE_HIGH_WATER = 40;		// Maximum requests to have active in HTTP
static const S32 HTTP_REQUESTS_IN_QUEUE_LOW_WATER = 20;			// Active level at which to refill


//////////////////////////////////////////////////////////////////////////////

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
			: mFetcher(fetcher), mID(id), mWorker(worker)
		{
		}

		// Threads:  Tid
		virtual void completed(bool success, LLImageRaw* raw, LLImageRaw* aux)
		{
			LLTextureFetchWorker* worker = mFetcher->getWorker(mID);
			if (worker)
			{
 				worker->callbackDecoded(success, raw, aux);
			}
		}
	private:
		LLTextureFetch* mFetcher;
		LLUUID mID;
		LLTextureFetchWorker* mWorker; // debug only (may get deleted from under us, use mFetcher/mID)
	};

	struct Compare
	{
		// lhs < rhs
		bool operator()(const LLTextureFetchWorker* lhs, const LLTextureFetchWorker* rhs) const
		{
			// greater priority is "less"
			const F32 lpriority = lhs->mImagePriority;
			const F32 rpriority = rhs->mImagePriority;
			if (lpriority > rpriority) // higher priority
				return true;
			else if (lpriority < rpriority)
				return false;
			else
				return lhs < rhs;
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
						   S32 imagesize, BOOL islocal);

	// Threads:  Ttc
	void callbackCacheWrite(bool success);

	// Threads:  Tid
	void callbackDecoded(bool success, LLImageRaw* raw, LLImageRaw* aux);
	
	// Threads:  T*
	void setGetStatus(LLCore::HttpStatus status, const std::string& reason)
	{
		LLMutexLock lock(&mWorkMutex);

		mGetStatus = status;
		mGetReason = reason;
	}

	void setCanUseHTTP(bool can_use_http) { mCanUseHTTP = can_use_http; }
	bool getCanUseHTTP() const { return mCanUseHTTP; }

	LLTextureFetch & getFetcher() { return *mFetcher; }

	// Inherited from LLCore::HttpHandler
	// Threads:  Ttf
	virtual void onCompleted(LLCore::HttpHandle handle, LLCore::HttpResponse * response);
	
protected:
	LLTextureFetchWorker(LLTextureFetch* fetcher, const std::string& url, const LLUUID& id, const LLHost& host,
						 F32 priority, S32 discard, S32 size);

private:

	// Threads:  Tmain
	/*virtual*/ void startWork(S32 param); // called from addWork() (MAIN THREAD)

	// Threads:  Tmain
	/*virtual*/ void endWork(S32 param, bool aborted); // called from doWork() (MAIN THREAD)

	// Locks:  Mw
	void resetFormattedData();
	
	// Locks:  Mw
	void setImagePriority(F32 priority);

	// Locks:  Mw (ctor invokes without lock)
	void setDesiredDiscard(S32 discard, S32 size);

    // Threads:  T*
	// Locks:  Mw
	bool insertPacket(S32 index, U8* data, S32 size);

	// Locks:  Mw
	void clearPackets();

	// Locks:  Mw
	void setupPacketData();

	// Locks:  Mw (ctor invokes without lock)
	U32 calcWorkPriority();
	
	// Locks:  Mw
	void removeFromCache();

	// Threads:  Ttf
	// Locks:  Mw
	bool processSimulatorPackets();

	// Threads:  Ttf
	bool writeToCacheComplete();
	
	// Threads:  Ttf
	void recordTextureStart(bool is_http);

	// Threads:  Ttf
	void recordTextureDone(bool is_http);

	void lockWorkMutex() { mWorkMutex.lock(); }
	void unlockWorkMutex() { mWorkMutex.unlock(); }

private:
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
		LOAD_FROM_SIMULATOR,
		SEND_HTTP_REQ,					// Commit to sending as HTTP
		WAIT_HTTP_RESOURCE,				// Waiting for HTTP resources
		WAIT_HTTP_REQ,					// Request sent, wait for completion
		DECODE_IMAGE,
		DECODE_IMAGE_UPDATE,
		WRITE_TO_CACHE,
		WAIT_ON_WRITE,
		DONE
	};
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
	static const char* sStateDescs[];
	e_state mState;
	e_write_to_cache_state mWriteToCacheState;
	LLTextureFetch* mFetcher;
	LLPointer<LLImageFormatted> mFormattedImage;
	LLPointer<LLImageRaw> mRawImage;
	LLPointer<LLImageRaw> mAuxImage;
	LLUUID mID;
	LLHost mHost;
	std::string mUrl;
	U8 mType;
	F32 mImagePriority;
	U32 mWorkPriority;
	F32 mRequestedPriority;
	S32 mDesiredDiscard;
	S32 mSimRequestedDiscard;
	S32 mRequestedDiscard;
	S32 mLoadedDiscard;
	S32 mDecodedDiscard;
	LLFrameTimer mRequestedTimer;
	LLFrameTimer mFetchTimer;
	LLTextureCache::handle_t mCacheReadHandle;
	LLTextureCache::handle_t mCacheWriteHandle;
	S32 mRequestedSize;
	S32 mRequestedOffset;
	S32 mDesiredSize;
	S32 mFileSize;
	S32 mCachedSize;	
	e_request_state mSentRequest;
	handle_t mDecodeHandle;
	BOOL mLoaded;
	BOOL mDecoded;
	BOOL mWritten;
	BOOL mNeedsAux;
	BOOL mHaveAllData;
	BOOL mInLocalCache;
	bool mCanUseHTTP ;
	bool mCanUseNET ; //can get from asset server.
	S32 mRetryAttempt;
	S32 mActiveCount;
	LLCore::HttpStatus mGetStatus;
	std::string mGetReason;
	
	// Work Data
	LLMutex mWorkMutex;
	struct PacketData
	{
		PacketData(U8* data, S32 size) { mData = data; mSize = size; }
		~PacketData() { clearData(); }
		void clearData() { delete[] mData; mData = NULL; }
		U8* mData;
		U32 mSize;
	};
	std::vector<PacketData*> mPackets;
	S32 mFirstPacket;
	S32 mLastPacket;
	U16 mTotalPackets;
	U8 mImageCodec;

	LLViewerAssetStats::duration_t mMetricsStartTime;

	LLCore::HttpHandle		mHttpHandle;				// Handle of any active request
	LLCore::BufferArray	*	mHttpBufferArray;			// Refcounted pointer to response data 
	int						mHttpPolicyClass;
	bool					mHttpActive;				// Active request to http library
	unsigned int			mHttpReplySize;
	unsigned int			mHttpReplyOffset;
	bool					mHttpReleased;				// Has been released from resource wait once
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
	TFReqSetRegion & operator=(const TFReqSetRegion &);	// Not defined

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
	 * @param	caps_url		URL of a "ViewerMetrics" Caps target
	 *							to receive the data.  Does not have to
	 *							be associated with a particular region.
	 *
	 * @param	session_id		UUID of the agent's session.
	 *
	 * @param	agent_id		UUID of the agent.  (Being pure here...)
	 *
	 * @param	main_stats		Pointer to a clone of the main thread's
	 *							LLViewerAssetStats data.  Thread1 takes
	 *							ownership of the copy and disposes of it
	 *							when done.
	 */
	TFReqSendMetrics(const std::string & caps_url,
					 const LLUUID & session_id,
					 const LLUUID & agent_id,
					 LLViewerAssetStats * main_stats)
		: LLTextureFetch::TFRequest(),
		  mCapsURL(caps_url),
		  mSessionID(session_id),
		  mAgentID(agent_id),
		  mMainStats(main_stats)
		{}
	TFReqSendMetrics & operator=(const TFReqSendMetrics &);	// Not defined

	virtual ~TFReqSendMetrics();

	virtual bool doWork(LLTextureFetch * fetcher);
		
public:
	const std::string mCapsURL;
	const LLUUID mSessionID;
	const LLUUID mAgentID;
	LLViewerAssetStats * mMainStats;
};

/*
 * Examines the merged viewer metrics report and if found to be too long,
 * will attempt to truncate it in some reasonable fashion.
 *
 * @param		max_regions		Limit of regions allowed in report.
 *
 * @param		metrics			Full, merged viewer metrics report.
 *
 * @returns		If data was truncated, returns true.
 */
bool truncate_viewer_metrics(int max_regions, LLSD & metrics);

} // end of anonymous namespace


//////////////////////////////////////////////////////////////////////////////

//static
const char* LLTextureFetchWorker::sStateDescs[] = {
	"INVALID",
	"INIT",
	"LOAD_FROM_TEXTURE_CACHE",
	"CACHE_POST",
	"LOAD_FROM_NETWORK",
	"LOAD_FROM_SIMULATOR",
	"SEND_HTTP_REQ",
	"WAIT_HTTP_RESOURCE",
	"WAIT_HTTP_REQ",
	"DECODE_IMAGE",
	"DECODE_IMAGE_UPDATE",
	"WRITE_TO_CACHE",
	"WAIT_ON_WRITE",
	"DONE"
};

// static
volatile bool LLTextureFetch::svMetricsDataBreak(true);	// Start with a data break

// called from MAIN THREAD

LLTextureFetchWorker::LLTextureFetchWorker(LLTextureFetch* fetcher,
										   const std::string& url, // Optional URL
										   const LLUUID& id,	// Image UUID
										   const LLHost& host,	// Simulator host
										   F32 priority,		// Priority
										   S32 discard,			// Desired discard
										   S32 size)			// Desired size
	: LLWorkerClass(fetcher, "TextureFetch"),
	  LLCore::HttpHandler(),
	  mState(INIT),
	  mWriteToCacheState(NOT_WRITE),
	  mFetcher(fetcher),
	  mID(id),
	  mHost(host),
	  mUrl(url),
	  mImagePriority(priority),
	  mWorkPriority(0),
	  mRequestedPriority(0.f),
	  mDesiredDiscard(-1),
	  mSimRequestedDiscard(-1),
	  mRequestedDiscard(-1),
	  mLoadedDiscard(-1),
	  mDecodedDiscard(-1),
	  mCacheReadHandle(LLTextureCache::nullHandle()),
	  mCacheWriteHandle(LLTextureCache::nullHandle()),
	  mRequestedSize(0),
	  mRequestedOffset(0),
	  mDesiredSize(TEXTURE_CACHE_ENTRY_SIZE),
	  mFileSize(0),
	  mCachedSize(0),
	  mLoaded(FALSE),
	  mSentRequest(UNSENT),
	  mDecodeHandle(0),
	  mDecoded(FALSE),
	  mWritten(FALSE),
	  mNeedsAux(FALSE),
	  mHaveAllData(FALSE),
	  mInLocalCache(FALSE),
	  mCanUseHTTP(true),
	  mRetryAttempt(0),
	  mActiveCount(0),
	  mWorkMutex(NULL),
	  mFirstPacket(0),
	  mLastPacket(-1),
	  mTotalPackets(0),
	  mImageCodec(IMG_CODEC_INVALID),
	  mMetricsStartTime(0),
	  mHttpHandle(LLCORE_HTTP_HANDLE_INVALID),
	  mHttpBufferArray(NULL),
	  mHttpPolicyClass(LLCore::HttpRequest::DEFAULT_POLICY_ID),
	  mHttpActive(false),
	  mHttpReplySize(0U),
	  mHttpReplyOffset(0U),
	  mHttpReleased(true)
{
	mCanUseNET = mUrl.empty() ;

	calcWorkPriority();
	mType = host.isOk() ? LLImageBase::TYPE_AVATAR_BAKE : LLImageBase::TYPE_NORMAL;
// 	llinfos << "Create: " << mID << " mHost:" << host << " Discard=" << discard << llendl;
	if (!mFetcher->mDebugPause)
	{
		U32 work_priority = mWorkPriority | LLWorkerThread::PRIORITY_HIGH;
		addWork(0, work_priority );
	}
	setDesiredDiscard(discard, size);
}

LLTextureFetchWorker::~LLTextureFetchWorker()
{
// 	llinfos << "Destroy: " << mID
// 			<< " Decoded=" << mDecodedDiscard
// 			<< " Requested=" << mRequestedDiscard
// 			<< " Desired=" << mDesiredDiscard << llendl;
	llassert_always(!haveWork());

	lockWorkMutex();													// +Mw (should be useless)
	if (mHttpActive)
	{
		// Issue a cancel on a live request...
		mFetcher->getHttpRequest().requestCancel(mHttpHandle, NULL);
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
	clearPackets();
	if (mHttpBufferArray)
	{
		mHttpBufferArray->release();
		mHttpBufferArray = NULL;
	}
	unlockWorkMutex();													// -Mw
	mFetcher->removeFromHTTPQueue(mID);
	mFetcher->removeHttpWaiter(mID);
}

// Locks:  Mw
void LLTextureFetchWorker::clearPackets()
{
	for_each(mPackets.begin(), mPackets.end(), DeletePointer());
	mPackets.clear();
	mTotalPackets = 0;
	mLastPacket = -1;
	mFirstPacket = 0;
}

// Locks:  Mw
void LLTextureFetchWorker::setupPacketData()
{
	S32 data_size = 0;
	if (mFormattedImage.notNull())
	{
		data_size = mFormattedImage->getDataSize();
	}
	if (data_size > 0)
	{
		// Only used for simulator requests
		mFirstPacket = (data_size - FIRST_PACKET_SIZE) / MAX_IMG_PACKET_SIZE + 1;
		if (FIRST_PACKET_SIZE + (mFirstPacket-1) * MAX_IMG_PACKET_SIZE != data_size)
		{
			llwarns << "Bad CACHED TEXTURE size: " << data_size << " removing." << llendl;
			removeFromCache();
			resetFormattedData();
			clearPackets();
		}
		else if (mFileSize > 0)
		{
			mLastPacket = mFirstPacket-1;
			mTotalPackets = (mFileSize - FIRST_PACKET_SIZE + MAX_IMG_PACKET_SIZE-1) / MAX_IMG_PACKET_SIZE + 1;
		}
		else
		{
			// This file was cached using HTTP so we have to refetch the first packet
			resetFormattedData();
			clearPackets();
		}
	}
}

// Locks:  Mw (ctor invokes without lock)
U32 LLTextureFetchWorker::calcWorkPriority()
{
 	//llassert_always(mImagePriority >= 0 && mImagePriority <= LLViewerFetchedTexture::maxDecodePriority());
	static const F32 PRIORITY_SCALE = (F32)LLWorkerThread::PRIORITY_LOWBITS / LLViewerFetchedTexture::maxDecodePriority();

	mWorkPriority = llmin((U32)LLWorkerThread::PRIORITY_LOWBITS, (U32)(mImagePriority * PRIORITY_SCALE));
	return mWorkPriority;
}

// Locks:  Mw (ctor invokes without lock)
void LLTextureFetchWorker::setDesiredDiscard(S32 discard, S32 size)
{
	bool prioritize = false;
	if (mDesiredDiscard != discard)
	{
		if (!haveWork())
		{
			calcWorkPriority();
			if (!mFetcher->mDebugPause)
			{
				U32 work_priority = mWorkPriority | LLWorkerThread::PRIORITY_HIGH;
				addWork(0, work_priority);
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
		mState = INIT;
		U32 work_priority = mWorkPriority | LLWorkerThread::PRIORITY_HIGH;
		setPriority(work_priority);
	}
}

// Locks:  Mw
void LLTextureFetchWorker::setImagePriority(F32 priority)
{
// 	llassert_always(priority >= 0 && priority <= LLViewerTexture::maxDecodePriority());
	F32 delta = fabs(priority - mImagePriority);
	if (delta > (mImagePriority * .05f) || mState == DONE)
	{
		mImagePriority = priority;
		calcWorkPriority();
		U32 work_priority = mWorkPriority | (getPriority() & LLWorkerThread::PRIORITY_HIGHBITS);
		setPriority(work_priority);
	}
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
	mHaveAllData = FALSE;
}

// Threads:  Tmain
void LLTextureFetchWorker::startWork(S32 param)
{
	llassert(mFormattedImage.isNull());
}

// Threads:  Ttf
bool LLTextureFetchWorker::doWork(S32 param)
{
	static const LLCore::HttpStatus http_not_found(HTTP_NOT_FOUND);
	static const LLCore::HttpStatus http_service_unavail(HTTP_SERVICE_UNAVAILABLE);
	
	// Release waiters while we aren't holding the Mw lock.
	mFetcher->releaseHttpWaiters();
	
	LLMutexLock lock(&mWorkMutex);										// +Mw

	if ((mFetcher->isQuitting() || getFlags(LLWorkerClass::WCF_DELETE_REQUESTED)))
	{
		if (mState < DECODE_IMAGE)
		{
			return true; // abort
		}
	}

	if(mImagePriority < F_ALMOST_ZERO)
	{
		if (mState == INIT || mState == LOAD_FROM_NETWORK || mState == LOAD_FROM_SIMULATOR)
		{
			return true; // abort
		}
	}
	if(mState > CACHE_POST && !mCanUseNET && !mCanUseHTTP)
	{
		//nowhere to get data, abort.
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
		mFetchTimer.reset();
	}

	if (mState == INIT)
	{		
		mRawImage = NULL ;
		mRequestedDiscard = -1;
		mLoadedDiscard = -1;
		mDecodedDiscard = -1;
		mRequestedSize = 0;
		mRequestedOffset = 0;
		mFileSize = 0;
		mCachedSize = 0;
		mLoaded = FALSE;
		mSentRequest = UNSENT;
		mDecoded  = FALSE;
		mWritten  = FALSE;
		if (mHttpBufferArray)
		{
			mHttpBufferArray->release();
			mHttpBufferArray = NULL;
		}
		mHaveAllData = FALSE;
		clearPackets(); // TODO: Shouldn't be necessary
		mCacheReadHandle = LLTextureCache::nullHandle();
		mCacheWriteHandle = LLTextureCache::nullHandle();
		mState = LOAD_FROM_TEXTURE_CACHE;
		mDesiredSize = llmax(mDesiredSize, TEXTURE_CACHE_ENTRY_SIZE); // min desired size is TEXTURE_CACHE_ENTRY_SIZE
		LL_DEBUGS("Texture") << mID << ": Priority: " << llformat("%8.0f",mImagePriority)
							 << " Desired Discard: " << mDesiredDiscard << " Desired Size: " << mDesiredSize << LL_ENDL;
		// fall through
	}

	if (mState == LOAD_FROM_TEXTURE_CACHE)
	{
		if (mCacheReadHandle == LLTextureCache::nullHandle())
		{
			U32 cache_priority = mWorkPriority;
			S32 offset = mFormattedImage.notNull() ? mFormattedImage->getDataSize() : 0;
			S32 size = mDesiredSize - offset;
			if (size <= 0)
			{
				mState = CACHE_POST;
				return false;
			}
			mFileSize = 0;
			mLoaded = FALSE;			
			
			if (mUrl.compare(0, 7, "file://") == 0)
			{
				setPriority(LLWorkerThread::PRIORITY_NORMAL | mWorkPriority); // Set priority first since Responder may change it

				// read file from local disk
				std::string filename = mUrl.substr(7, std::string::npos);
				CacheReadResponder* responder = new CacheReadResponder(mFetcher, mID, mFormattedImage);
				mCacheReadHandle = mFetcher->mTextureCache->readFromCache(filename, mID, cache_priority,
																		  offset, size, responder);
			}
			else if (mUrl.empty())
			{
				setPriority(LLWorkerThread::PRIORITY_NORMAL | mWorkPriority); // Set priority first since Responder may change it

				CacheReadResponder* responder = new CacheReadResponder(mFetcher, mID, mFormattedImage);
				mCacheReadHandle = mFetcher->mTextureCache->readFromCache(mID, cache_priority,
																		  offset, size, responder);
			}
			else if(mCanUseHTTP)
			{
				if (!(mUrl.compare(0, 7, "http://") == 0))
				{
					// *TODO:?remove this warning
					llwarns << "Unknown URL Type: " << mUrl << llendl;
				}
				setPriority(LLWorkerThread::PRIORITY_HIGH | mWorkPriority);
				mState = SEND_HTTP_REQ;
			}
			else
			{
				setPriority(LLWorkerThread::PRIORITY_HIGH | mWorkPriority);
				mState = LOAD_FROM_NETWORK;
			}
		}

		if (mLoaded)
		{
			// Make sure request is complete. *TODO: make this auto-complete
			if (mFetcher->mTextureCache->readComplete(mCacheReadHandle, false))
			{
				mCacheReadHandle = LLTextureCache::nullHandle();
				mState = CACHE_POST;
				// fall through
			}
			else
			{
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
		mCachedSize = mFormattedImage.notNull() ? mFormattedImage->getDataSize() : 0;
		// Successfully loaded
		if ((mCachedSize >= mDesiredSize) || mHaveAllData)
		{
			// we have enough data, decode it
			llassert_always(mFormattedImage->getDataSize() > 0);
			mLoadedDiscard = mDesiredDiscard;
			mState = DECODE_IMAGE;
			mWriteToCacheState = NOT_WRITE ;
			LL_DEBUGS("Texture") << mID << ": Cached. Bytes: " << mFormattedImage->getDataSize()
								 << " Size: " << llformat("%dx%d",mFormattedImage->getWidth(),mFormattedImage->getHeight())
								 << " Desired Discard: " << mDesiredDiscard << " Desired Size: " << mDesiredSize << LL_ENDL;
			// fall through
		}
		else
		{
			if (mUrl.compare(0, 7, "file://") == 0)
			{
				// failed to load local file, we're done.
				return true;
			}
			// need more data
			else
			{
				LL_DEBUGS("Texture") << mID << ": Not in Cache" << LL_ENDL;
				mState = LOAD_FROM_NETWORK;
			}
			// fall through
		}
	}

	if (mState == LOAD_FROM_NETWORK)
	{
		static LLCachedControl<bool> use_http(gSavedSettings,"ImagePipelineUseHTTP");

// 		if (mHost != LLHost::invalid) get_url = false;
		if ( use_http && mCanUseHTTP && mUrl.empty())//get http url.
		{
			LLViewerRegion* region = NULL;
			if (mHost == LLHost::invalid)
				region = gAgent.getRegion();
			else
				region = LLWorld::getInstance()->getRegion(mHost);

			if (region)
			{
				std::string http_url = region->getHttpUrl() ;
				if (!http_url.empty())
				{
					mUrl = http_url + "/?texture_id=" + mID.asString().c_str();
					mWriteToCacheState = CAN_WRITE ; //because this texture has a fixed texture id.
				}
				else
				{
					mCanUseHTTP = false ;
				}
			}
			else
			{
				// This will happen if not logged in or if a region deoes not have HTTP Texture enabled
				//llwarns << "Region not found for host: " << mHost << llendl;
				mCanUseHTTP = false;
			}
		}
		if (mCanUseHTTP && !mUrl.empty())
		{
			mState = SEND_HTTP_REQ;
			setPriority(LLWorkerThread::PRIORITY_HIGH | mWorkPriority);
			if(mWriteToCacheState != NOT_WRITE)
			{
				mWriteToCacheState = CAN_WRITE ;
			}
			// don't return, fall through to next state
		}
		else if (mSentRequest == UNSENT && mCanUseNET)
		{
			// Add this to the network queue and sit here.
			// LLTextureFetch::update() will send off a request which will change our state
			mWriteToCacheState = CAN_WRITE ;
			mRequestedSize = mDesiredSize;
			mRequestedDiscard = mDesiredDiscard;
			mSentRequest = QUEUED;
			mFetcher->addToNetworkQueue(this);
			recordTextureStart(false);
			setPriority(LLWorkerThread::PRIORITY_NORMAL | mWorkPriority);
			
			return false;
		}
		else
		{
			// Shouldn't need to do anything here
			//llassert_always(mFetcher->mNetworkQueue.find(mID) != mFetcher->mNetworkQueue.end());
			// Make certain this is in the network queue
			//mFetcher->addToNetworkQueue(this);
			//recordTextureStart(false);
			//setPriority(LLWorkerThread::PRIORITY_NORMAL | mWorkPriority);
			return false;
		}
	}
	
	if (mState == LOAD_FROM_SIMULATOR)
	{
		if (mFormattedImage.isNull())
		{
			mFormattedImage = new LLImageJ2C;
		}
		if (processSimulatorPackets())
		{
			LL_DEBUGS("Texture") << mID << ": Loaded from Sim. Bytes: " << mFormattedImage->getDataSize() << LL_ENDL;
			mFetcher->removeFromNetworkQueue(this, false);
			if (mFormattedImage.isNull() || !mFormattedImage->getDataSize())
			{
				// processSimulatorPackets() failed
// 				llwarns << "processSimulatorPackets() failed to load buffer" << llendl;
				return true; // failed
			}
			setPriority(LLWorkerThread::PRIORITY_HIGH | mWorkPriority);
			mState = DECODE_IMAGE;
			mWriteToCacheState = SHOULD_WRITE;
			recordTextureDone(false);
		}
		else
		{
			mFetcher->addToNetworkQueue(this); // failsafe
			setPriority(LLWorkerThread::PRIORITY_NORMAL | mWorkPriority);
			recordTextureStart(false);
		}
		return false;
	}
	
	if (mState == SEND_HTTP_REQ)
	{
		if (! mCanUseHTTP)
		{
			return true; // abort
		}

		// NOTE:
		// control the number of the http requests issued for:
		// 1, not openning too many file descriptors at the same time;
		// 2, control the traffic of http so udp gets bandwidth.
		//
		if (! mHttpReleased)
		{
			// If this request hasn't been released before and it looks like
			// we're busy, put this request into resource wait and allow something
			// else to come to the front.
			if (mFetcher->getNumHTTPRequests() >= HTTP_REQUESTS_IN_QUEUE_HIGH_WATER ||
				mFetcher->getHttpWaitersCount())
			{
				mState = WAIT_HTTP_RESOURCE;
				setPriority(LLWorkerThread::PRIORITY_LOW | mWorkPriority);
				mFetcher->addHttpWaiter(this->mID);
				return false;
			}
		}

		mFetcher->removeFromNetworkQueue(this, false);
			
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
					mState = DECODE_IMAGE;
					return false;
				}
				else
				{
					return true; // abort.
				}
			}
		}
		mRequestedSize = mDesiredSize;
		mRequestedDiscard = mDesiredDiscard;
		mRequestedSize -= cur_size;
		mRequestedOffset = cur_size;
			
		mHttpHandle = LLCORE_HTTP_HANDLE_INVALID;
		if (!mUrl.empty())
		{
			mLoaded = FALSE;
			mGetStatus = LLCore::HttpStatus();
			mGetReason.clear();
			LL_DEBUGS("Texture") << "HTTP GET: " << mID << " Offset: " << mRequestedOffset
								 << " Bytes: " << mRequestedSize
								 << " Bandwidth(kbps): " << mFetcher->getTextureBandwidth() << "/" << mFetcher->mMaxBandwidth
								 << LL_ENDL;
			
			// Will call callbackHttpGet when curl request completes
			mHttpHandle = mFetcher->mHttpRequest->requestGetByteRange(mHttpPolicyClass,
																	  mWorkPriority,
																	  mUrl,
																	  mRequestedOffset,
																	  mRequestedSize,
																	  mFetcher->mHttpOptions,
																	  mFetcher->mHttpHeaders,
																	  this);
		}
		if (LLCORE_HTTP_HANDLE_INVALID == mHttpHandle)
		{
			llwarns << "HTTP GET request failed for " << mID << llendl;
			resetFormattedData();
			return true; // failed
		}

		mHttpActive = true;
		mFetcher->addToHTTPQueue(mID);
		recordTextureStart(true);
		setPriority(LLWorkerThread::PRIORITY_NORMAL | mWorkPriority);
		mState = WAIT_HTTP_REQ;	
		
		// fall through
	}
	
	if (mState == WAIT_HTTP_RESOURCE)
	{
		// Nothing to do until releaseHttpWaiters() puts us back
		// into the flow...
		return false;
	}
	
	if (mState == WAIT_HTTP_REQ)
	{
		if (mLoaded)
		{
			S32 cur_size = mFormattedImage.notNull() ? mFormattedImage->getDataSize() : 0;
			if (mRequestedSize < 0)
			{
				if (http_not_found == mGetStatus)
				{
					llwarns << "Texture missing from server (404): " << mUrl << llendl;

					// roll back to try UDP
					if (mCanUseNET)
					{
						mState = INIT;
						mCanUseHTTP = false;
						setPriority(LLWorkerThread::PRIORITY_HIGH | mWorkPriority);
						return false;
					}
				}
				else if (http_service_unavail == mGetStatus)
				{
					LL_INFOS_ONCE("Texture") << "Texture server busy (503): " << mUrl << LL_ENDL;
				}
				else
				{
					llinfos << "HTTP GET failed for: " << mUrl
							<< " Status: " << mGetStatus.toHex()
							<< " Reason: '" << mGetReason << "'"
						// *FIXME:  Add retry info for reporting purposes...
						// << " Attempt:" << mHTTPFailCount+1 << "/" << max_attempts
							<< llendl;
				}

				if (cur_size > 0)
				{
					// Use available data
					mLoadedDiscard = mFormattedImage->getDiscardLevel();
					mState = DECODE_IMAGE;
					return false; 
				}

				// Fail harder
				resetFormattedData();
				mState = DONE;
				return true; // failed
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
				mState = DONE;
				return true;
			}

			const S32 append_size(mHttpBufferArray->size());
			const S32 total_size(cur_size + append_size);
			llassert_always(append_size == mRequestedSize);
			
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
						
			if (mHaveAllData /* && mRequestedDiscard == 0*/) //the image file is fully loaded.
			{
				mFileSize = total_size;
			}
			else //the file size is unknown.
			{
				mFileSize = total_size + 1 ; //flag the file is not fully loaded.
			}
			
			U8 * buffer = (U8 *) ALLOCATE_MEM(LLImageBase::getPrivatePool(), total_size);
			if (cur_size > 0)
			{
				memcpy(buffer, mFormattedImage->getData(), cur_size);
			}
			mHttpBufferArray->read(0, (char *) buffer + cur_size, append_size);

			// NOTE: setData releases current data and owns new data (buffer)
			mFormattedImage->setData(buffer, total_size);

			// Done with buffer array
			mHttpBufferArray->release();
			mHttpBufferArray = NULL;
			
			mLoadedDiscard = mRequestedDiscard;
			mState = DECODE_IMAGE;
			if (mWriteToCacheState != NOT_WRITE)
			{
				mWriteToCacheState = SHOULD_WRITE ;
			}
			setPriority(LLWorkerThread::PRIORITY_HIGH | mWorkPriority);
			return false;
		}
		else
		{
			setPriority(LLWorkerThread::PRIORITY_NORMAL | mWorkPriority);
			return false;
		}
	}
	
	if (mState == DECODE_IMAGE)
	{
		static LLCachedControl<bool> textures_decode_disabled(gSavedSettings,"TextureDecodeDisabled");

		setPriority(LLWorkerThread::PRIORITY_NORMAL | mWorkPriority); // Set priority first since Responder may change it
		if (textures_decode_disabled)
		{
			// for debug use, don't decode
			mState = DONE;
			return true;
		}

		if (mDesiredDiscard < 0)
		{
			// We aborted, don't decode
			mState = DONE;
			return true;
		}
		
		if (mFormattedImage->getDataSize() <= 0)
		{
			//llerrs << "Decode entered with invalid mFormattedImage. ID = " << mID << llendl;
			
			//abort, don't decode
			mState = DONE;
			return true;
		}
		if (mLoadedDiscard < 0)
		{
			//llerrs << "Decode entered with invalid mLoadedDiscard. ID = " << mID << llendl;

			//abort, don't decode
			mState = DONE;
			return true;
		}

		mRawImage = NULL;
		mAuxImage = NULL;
		llassert_always(mFormattedImage.notNull());
		S32 discard = mHaveAllData ? 0 : mLoadedDiscard;
		U32 image_priority = LLWorkerThread::PRIORITY_NORMAL | mWorkPriority;
		mDecoded  = FALSE;
		mState = DECODE_IMAGE_UPDATE;
		LL_DEBUGS("Texture") << mID << ": Decoding. Bytes: " << mFormattedImage->getDataSize() << " Discard: " << discard
				<< " All Data: " << mHaveAllData << LL_ENDL;
		mDecodeHandle = mFetcher->mImageDecodeThread->decodeImage(mFormattedImage, image_priority, discard, mNeedsAux,
																  new DecodeResponder(mFetcher, mID, this));
		// fall though
	}
	
	if (mState == DECODE_IMAGE_UPDATE)
	{
		if (mDecoded)
		{
			if (mDecodedDiscard < 0)
			{
				LL_DEBUGS("Texture") << mID << ": Failed to Decode." << LL_ENDL;
				if (mCachedSize > 0 && !mInLocalCache && mRetryAttempt == 0)
				{
					// Cache file should be deleted, try again
// 					llwarns << mID << ": Decode of cached file failed (removed), retrying" << llendl;
					llassert_always(mDecodeHandle == 0);
					mFormattedImage = NULL;
					++mRetryAttempt;
					setPriority(LLWorkerThread::PRIORITY_HIGH | mWorkPriority);
					mState = INIT;
					return false;
				}
				else
				{
// 					llwarns << "UNABLE TO LOAD TEXTURE: " << mID << " RETRIES: " << mRetryAttempt << llendl;
					mState = DONE; // failed
				}
			}
			else
			{
				llassert_always(mRawImage.notNull());
				LL_DEBUGS("Texture") << mID << ": Decoded. Discard: " << mDecodedDiscard
						<< " Raw Image: " << llformat("%dx%d",mRawImage->getWidth(),mRawImage->getHeight()) << LL_ENDL;
				setPriority(LLWorkerThread::PRIORITY_HIGH | mWorkPriority);
				mState = WRITE_TO_CACHE;
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
		if (mWriteToCacheState != SHOULD_WRITE || mFormattedImage.isNull())
		{
			// If we're in a local cache or we didn't actually receive any new data,
			// or we failed to load anything, skip
			mState = DONE;
			return false;
		}
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
		setPriority(LLWorkerThread::PRIORITY_NORMAL | mWorkPriority); // Set priority first since Responder may change it
		U32 cache_priority = mWorkPriority;
		mWritten = FALSE;
		mState = WAIT_ON_WRITE;
		CacheWriteResponder* responder = new CacheWriteResponder(mFetcher, mID);
		mCacheWriteHandle = mFetcher->mTextureCache->writeToCache(mID, cache_priority,
																  mFormattedImage->getData(), datasize,
																  mFileSize, responder);
		// fall through
	}
	
	if (mState == WAIT_ON_WRITE)
	{
		if (writeToCacheComplete())
		{
			mState = DONE;
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
		if (mDecodedDiscard > 0 && mDesiredDiscard < mDecodedDiscard)
		{
			// More data was requested, return to INIT
			mState = INIT;
			setPriority(LLWorkerThread::PRIORITY_HIGH | mWorkPriority);
			return false;
		}
		else
		{
			setPriority(LLWorkerThread::PRIORITY_NORMAL | mWorkPriority);
			return true;
		}
	}
	
	return false;
}																		// -Mw

// Threads:  Ttf
// virtual
void LLTextureFetchWorker::onCompleted(LLCore::HttpHandle handle, LLCore::HttpResponse * response)
{
	static LLCachedControl<bool> log_to_viewer_log(gSavedSettings, "LogTextureDownloadsToViewerLog");
	static LLCachedControl<bool> log_to_sim(gSavedSettings, "LogTextureDownloadsToSimulator");
	static LLCachedControl<bool> log_texture_traffic(gSavedSettings, "LogTextureNetworkTraffic") ;

	LLMutexLock lock(&mWorkMutex);										// +Mw

	mHttpActive = false;
	
	if (log_to_viewer_log || log_to_sim)
	{
		U64 timeNow = LLTimer::getTotalTime();
		mFetcher->mTextureInfo.setRequestStartTime(mID, mMetricsStartTime);
		mFetcher->mTextureInfo.setRequestType(mID, LLTextureInfoDetails::REQUEST_TYPE_HTTP);
		mFetcher->mTextureInfo.setRequestSize(mID, mRequestedSize);
		mFetcher->mTextureInfo.setRequestOffset(mID, mRequestedOffset);
		mFetcher->mTextureInfo.setRequestCompleteTimeAndLog(mID, timeNow);
	}

	bool success = true;
	bool partial = false;
	LLCore::HttpStatus status(response->getStatus());
	
	lldebugs << "HTTP COMPLETE: " << mID
			 << " status: " << status.toHex()
			 << " '" << status.toString() << "'"
			 << llendl;
//	unsigned int offset(0), length(0);
//	response->getRange(&offset, &length);
// 	llwarns << "HTTP COMPLETE: " << mID << " handle: " << handle
// 			<< " status: " << status.toULong() << " '" << status.toString() << "'"
// 			<< " req offset: " << mRequestedOffset << " req length: " << mRequestedSize
// 			<< " offset: " << offset << " length: " << length
// 			<< llendl;

	if (! status)
	{
		success = false;
		std::string reason(status.toString());
		setGetStatus(status, reason);
 		llwarns << "CURL GET FAILED, status: " << status.toHex()
				<< " reason: " << reason << llendl;
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
	
	S32 data_size = callbackHttpGet(response, partial, success);
			
	if (log_texture_traffic && data_size > 0)
	{
		LLViewerTexture* tex = LLViewerTextureManager::findTexture(mID);
		if (tex)
		{
			gTotalTextureBytesPerBoostLevel[tex->getBoostLevel()] += data_size ;
		}
	}

	mFetcher->removeFromHTTPQueue(mID);
	
	recordTextureDone(true);
}																		// -Mw


// Threads:  Tmain
void LLTextureFetchWorker::endWork(S32 param, bool aborted)
{
	if (mDecodeHandle != 0)
	{
		mFetcher->mImageDecodeThread->abortRequest(mDecodeHandle, false);
		mDecodeHandle = 0;
	}
	mFormattedImage = NULL;
}

//////////////////////////////////////////////////////////////////////////////

// Threads:  Ttf

// virtual
void LLTextureFetchWorker::finishWork(S32 param, bool completed)
{
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
	
	// Allow any pending reads or writes to complete
	if (mCacheReadHandle != LLTextureCache::nullHandle())
	{
		if (mFetcher->mTextureCache->readComplete(mCacheReadHandle, true))
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
		if (mFetcher->mTextureCache->writeComplete(mCacheWriteHandle))
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
bool LLTextureFetchWorker::processSimulatorPackets()
{
	if (mFormattedImage.isNull() || mRequestedSize < 0)
	{
		// not sure how we got here, but not a valid state, abort!
		llassert_always(mDecodeHandle == 0);
		mFormattedImage = NULL;
		return true;
	}
	
	if (mLastPacket >= mFirstPacket)
	{
		S32 buffer_size = mFormattedImage->getDataSize();
		for (S32 i = mFirstPacket; i<=mLastPacket; i++)
		{
			llassert_always(mPackets[i]);
			buffer_size += mPackets[i]->mSize;
		}
		bool have_all_data = mLastPacket >= mTotalPackets-1;
		if (mRequestedSize <= 0)
		{
			// We received a packed but haven't requested anything yet (edge case)
			// Return true (we're "done") since we didn't request anything
			return true;
		}
		if (buffer_size >= mRequestedSize || have_all_data)
		{
			/// We have enough (or all) data
			if (have_all_data)
			{
				mHaveAllData = TRUE;
			}
			S32 cur_size = mFormattedImage->getDataSize();
			if (buffer_size > cur_size)
			{
				/// We have new data
				U8* buffer = (U8*)ALLOCATE_MEM(LLImageBase::getPrivatePool(), buffer_size);
				S32 offset = 0;
				if (cur_size > 0 && mFirstPacket > 0)
				{
					memcpy(buffer, mFormattedImage->getData(), cur_size);
					offset = cur_size;
				}
				for (S32 i=mFirstPacket; i<=mLastPacket; i++)
				{
					memcpy(buffer + offset, mPackets[i]->mData, mPackets[i]->mSize);
					offset += mPackets[i]->mSize;
				}
				// NOTE: setData releases current data
				mFormattedImage->setData(buffer, buffer_size);
			}
			mLoadedDiscard = mRequestedDiscard;
			return true;
		}
	}
	return false;
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
		llwarns << "callbackHttpGet for unrequested fetch worker: " << mID
				<< " req=" << mSentRequest << " state= " << mState << llendl;
		return data_size;
	}
	if (mLoaded)
	{
		llwarns << "Duplicate callback for " << mID.asString() << llendl;
		return data_size ; // ignore duplicate callback
	}
	if (success)
	{
		// get length of stream:
		LLCore::BufferArray * body(response->getBody());
		data_size = body ? body->size() : 0;

		LL_DEBUGS("Texture") << "HTTP RECEIVED: " << mID.asString() << " Bytes: " << data_size << LL_ENDL;
		if (data_size > 0)
		{
			// *TODO: set the formatted image data here directly to avoid the copy
			// *FIXME:  deal with actual offset and actual datasize, don't assume
			// server gave exactly what was asked for.
			
			llassert_always(NULL == mHttpBufferArray);

			// Hold on to body for later copy
			body->addRef();
			mHttpBufferArray = body;

			if (! partial)
			{
				// Response indicates this is the entire asset regardless
				// of our asking for a byte range.  Mark it so and drop
				// any partial data we might have so that the current
				// response body becomes the entire dataset.
				if (data_size <= mRequestedOffset)
				{
					LL_WARNS("Texture") << "Fetched entire texture " << mID
										<< " when it was expected to be marked complete.  mImageSize:  "
										<< mFileSize << " datasize:  " << mFormattedImage->getDataSize()
										<< LL_ENDL;
				}
				mHaveAllData = TRUE;
				llassert_always(mDecodeHandle == 0);
				mFormattedImage = NULL; // discard any previous data we had
			}
			else if (data_size < mRequestedSize /*&& mRequestedDiscard == 0*/)
			{
				// *FIXME:  I think we can treat this as complete regardless
				// of requested discard level.  Revisit this...
				mHaveAllData = TRUE;
			}
			else if (data_size > mRequestedSize)
			{
				// *TODO: This shouldn't be happening any more  (REALLY don't expect this anymore)
				llwarns << "data_size = " << data_size << " > requested: " << mRequestedSize << llendl;
				mHaveAllData = TRUE;
				llassert_always(mDecodeHandle == 0);
				mFormattedImage = NULL; // discard any previous data we had
			}
		}
		else
		{
			// We requested data but received none (and no error),
			// so presumably we have all of it
			mHaveAllData = TRUE;
		}
		mRequestedSize = data_size;
	}
	else
	{
		mRequestedSize = -1; // error
	}
	mLoaded = TRUE;
	setPriority(LLWorkerThread::PRIORITY_HIGH | mWorkPriority);

	return data_size ;
}

//////////////////////////////////////////////////////////////////////////////

// Threads:  Ttc
void LLTextureFetchWorker::callbackCacheRead(bool success, LLImageFormatted* image,
											 S32 imagesize, BOOL islocal)
{
	LLMutexLock lock(&mWorkMutex);										// +Mw
	if (mState != LOAD_FROM_TEXTURE_CACHE)
	{
// 		llwarns << "Read callback for " << mID << " with state = " << mState << llendl;
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
			mHaveAllData = TRUE;
		}
	}
	mLoaded = TRUE;
	setPriority(LLWorkerThread::PRIORITY_HIGH | mWorkPriority);
}																		// -Mw

// Threads:  Ttc
void LLTextureFetchWorker::callbackCacheWrite(bool success)
{
	LLMutexLock lock(&mWorkMutex);										// +Mw
	if (mState != WAIT_ON_WRITE)
	{
// 		llwarns << "Write callback for " << mID << " with state = " << mState << llendl;
		return;
	}
	mWritten = TRUE;
	setPriority(LLWorkerThread::PRIORITY_HIGH | mWorkPriority);
}																		// -Mw

//////////////////////////////////////////////////////////////////////////////

// Threads:  Tid
void LLTextureFetchWorker::callbackDecoded(bool success, LLImageRaw* raw, LLImageRaw* aux)
{
	LLMutexLock lock(&mWorkMutex);										// +Mw
	if (mDecodeHandle == 0)
	{
		return; // aborted, ignore
	}
	if (mState != DECODE_IMAGE_UPDATE)
	{
// 		llwarns << "Decode callback for " << mID << " with state = " << mState << llendl;
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
 		LL_DEBUGS("Texture") << mID << ": Decode Finished. Discard: " << mDecodedDiscard
							 << " Raw Image: " << llformat("%dx%d",mRawImage->getWidth(),mRawImage->getHeight()) << LL_ENDL;
	}
	else
	{
		llwarns << "DECODE FAILED: " << mID << " Discard: " << (S32)mFormattedImage->getDiscardLevel() << llendl;
		removeFromCache();
		mDecodedDiscard = -1; // Redundant, here for clarity and paranoia
	}
	mDecoded = TRUE;
// 	llinfos << mID << " : DECODE COMPLETE " << llendl;
	setPriority(LLWorkerThread::PRIORITY_HIGH | mWorkPriority);
}																		// -Mw

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
	if (! mMetricsStartTime)
	{
		mMetricsStartTime = LLViewerAssetStatsFF::get_timestamp();
	}
	LLViewerAssetStatsFF::record_enqueue_thread1(LLViewerAssetType::AT_TEXTURE,
												 is_http,
												 LLImageBase::TYPE_AVATAR_BAKE == mType);
}


// Threads:  Ttf
void LLTextureFetchWorker::recordTextureDone(bool is_http)
{
	if (mMetricsStartTime)
	{
		LLViewerAssetStatsFF::record_response_thread1(LLViewerAssetType::AT_TEXTURE,
													  is_http,
													  LLImageBase::TYPE_AVATAR_BAKE == mType,
													  LLViewerAssetStatsFF::get_timestamp() - mMetricsStartTime);
		mMetricsStartTime = 0;
	}
	LLViewerAssetStatsFF::record_dequeue_thread1(LLViewerAssetType::AT_TEXTURE,
												 is_http,
												 LLImageBase::TYPE_AVATAR_BAKE == mType);
}


//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
// public

LLTextureFetch::LLTextureFetch(LLTextureCache* cache, LLImageDecodeThread* imagedecodethread, bool threaded, bool qa_mode)
	: LLWorkerThread("TextureFetch", threaded, true),
	  mDebugCount(0),
	  mDebugPause(FALSE),
	  mPacketCount(0),
	  mBadPacketCount(0),
	  mQueueMutex(getAPRPool()),
	  mNetworkQueueMutex(getAPRPool()),
	  mTextureCache(cache),
	  mImageDecodeThread(imagedecodethread),
	  mTextureBandwidth(0),
	  mHTTPTextureBits(0),
	  mTotalHTTPRequests(0),
	  mQAMode(qa_mode),
	  mHttpRequest(NULL),
	  mHttpOptions(NULL),
	  mHttpHeaders(NULL)
{
	mMaxBandwidth = gSavedSettings.getF32("ThrottleBandwidthKBPS");
	mTextureInfo.setUpLogging(gSavedSettings.getBOOL("LogTextureDownloadsToViewerLog"), gSavedSettings.getBOOL("LogTextureDownloadsToSimulator"), gSavedSettings.getU32("TextureLoggingThreshold"));
	
	mHttpRequest = new LLCore::HttpRequest;
	mHttpOptions = new LLCore::HttpOptions;
	mHttpHeaders = new LLCore::HttpHeaders;
	mHttpHeaders->mHeaders.push_back("Accept: image/x-j2c");
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

	if (mHttpOptions)
	{
		mHttpOptions->release();
		mHttpOptions = NULL;
	}

	if (mHttpHeaders)
	{
		mHttpHeaders->release();
		mHttpHeaders = NULL;
	}

	mHttpWaitResource.clear();
	
	delete mHttpRequest;
	mHttpRequest = NULL;

	// ~LLQueuedThread() called here
}

bool LLTextureFetch::createRequest(const std::string& url, const LLUUID& id, const LLHost& host, F32 priority,
								   S32 w, S32 h, S32 c, S32 desired_discard, bool needs_aux, bool can_use_http)
{
	if (mDebugPause)
	{
		return false;
	}
	
	LLTextureFetchWorker* worker = getWorker(id) ;
	if (worker)
	{
		if (worker->mHost != host)
		{
			llwarns << "LLTextureFetch::createRequest " << id << " called with multiple hosts: "
					<< host << " != " << worker->mHost << llendl;
			removeRequest(worker, true);
			worker = NULL;
			return false;
		}
	}

	S32 desired_size;
	std::string exten = gDirUtilp->getExtension(url);
	if (!url.empty() && (!exten.empty() && LLImageBase::getCodecFromExtension(exten) != IMG_CODEC_J2C))
	{
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
		desired_size = TEXTURE_CACHE_ENTRY_SIZE;
		desired_discard = MAX_DISCARD_LEVEL;
	}

	
	if (worker)
	{
		if (worker->wasAborted())
		{
			return false; // need to wait for previous aborted request to complete
		}
		worker->lockWorkMutex();										// +Mw
		worker->mActiveCount++;
		worker->mNeedsAux = needs_aux;
		worker->setImagePriority(priority);
		worker->setDesiredDiscard(desired_discard, desired_size);
		worker->setCanUseHTTP(can_use_http) ;
		if (!worker->haveWork())
		{
			worker->mState = LLTextureFetchWorker::INIT;
			worker->unlockWorkMutex();									// -Mw

			worker->addWork(0, LLWorkerThread::PRIORITY_HIGH | worker->mWorkPriority);
		}
		else
		{
			worker->unlockWorkMutex();									// -Mw
		}
	}
	else
	{
		worker = new LLTextureFetchWorker(this, url, id, host, priority, desired_discard, desired_size);
		lockQueue();													// +Mfq
		mRequestMap[id] = worker;
		unlockQueue();													// -Mfq

		worker->lockWorkMutex();										// +Mw
		worker->mActiveCount++;
		worker->mNeedsAux = needs_aux;
		worker->setCanUseHTTP(can_use_http) ;
		worker->unlockWorkMutex();										// -Mw
	}
	
// 	llinfos << "REQUESTED: " << id << " Discard: " << desired_discard << llendl;
	return true;
}


// Threads:  T* (but Ttf in practice)

// protected
void LLTextureFetch::addToNetworkQueue(LLTextureFetchWorker* worker)
{
	lockQueue();														// +Mfq
	bool in_request_map = (mRequestMap.find(worker->mID) != mRequestMap.end()) ;
	unlockQueue();														// -Mfq

	LLMutexLock lock(&mNetworkQueueMutex);								// +Mfnq		
	if (in_request_map)
	{
		// only add to the queue if in the request map
		// i.e. a delete has not been requested
		mNetworkQueue.insert(worker->mID);
	}
	for (cancel_queue_t::iterator iter1 = mCancelQueue.begin();
		 iter1 != mCancelQueue.end(); ++iter1)
	{
		iter1->second.erase(worker->mID);
	}
}																		// -Mfnq

// Threads:  T*
void LLTextureFetch::removeFromNetworkQueue(LLTextureFetchWorker* worker, bool cancel)
{
	LLMutexLock lock(&mNetworkQueueMutex);								// +Mfnq
	size_t erased = mNetworkQueue.erase(worker->mID);
	if (cancel && erased > 0)
	{
		mCancelQueue[worker->mHost].insert(worker->mID);
	}
}																		// -Mfnq

// Threads:  T*
//
// protected
void LLTextureFetch::addToHTTPQueue(const LLUUID& id)
{
	LLMutexLock lock(&mNetworkQueueMutex);								// +Mfnq
	mHTTPTextureQueue.insert(id);
	mTotalHTTPRequests++;
}																		// -Mfnq

// Threads:  T*
void LLTextureFetch::removeFromHTTPQueue(const LLUUID& id, S32 received_size)
{
	LLMutexLock lock(&mNetworkQueueMutex);								// +Mfnq
	mHTTPTextureQueue.erase(id);
	mHTTPTextureBits += received_size * 8; // Approximate - does not include header bits	
}																		// -Mfnq

// NB:  If you change deleteRequest() you should probably make
// parallel changes in removeRequest().  They're functionally
// identical with only argument variations.
//
// Threads:  T*
void LLTextureFetch::deleteRequest(const LLUUID& id, bool cancel)
{
	lockQueue();														// +Mfq
	LLTextureFetchWorker* worker = getWorkerAfterLock(id);
	if (worker)
	{		
		size_t erased_1 = mRequestMap.erase(worker->mID);
		unlockQueue();													// -Mfq

		llassert_always(erased_1 > 0) ;

		removeFromNetworkQueue(worker, cancel);
		llassert_always(!(worker->getFlags(LLWorkerClass::WCF_DELETE_REQUESTED))) ;

		worker->scheduleDelete();	
	}
	else
	{
		unlockQueue();													// -Mfq
	}
}

// NB:  If you change removeRequest() you should probably make
// parallel changes in deleteRequest().  They're functionally
// identical with only argument variations.
//
// Threads:  T*
void LLTextureFetch::removeRequest(LLTextureFetchWorker* worker, bool cancel)
{
	lockQueue();														// +Mfq
	size_t erased_1 = mRequestMap.erase(worker->mID);
	unlockQueue();														// -Mfq

	llassert_always(erased_1 > 0) ;
	removeFromNetworkQueue(worker, cancel);
	llassert_always(!(worker->getFlags(LLWorkerClass::WCF_DELETE_REQUESTED))) ;

	worker->scheduleDelete();	
}

// Threads:  T*
S32 LLTextureFetch::getNumRequests() 
{ 
	lockQueue();														// +Mfq
	S32 size = (S32)mRequestMap.size(); 
	unlockQueue();														// -Mfq

	return size;
}

// Threads:  T*
S32 LLTextureFetch::getNumHTTPRequests() 
{ 
	mNetworkQueueMutex.lock();											// +Mfq
	S32 size = (S32)mHTTPTextureQueue.size(); 
	mNetworkQueueMutex.unlock();										// -Mfq

	return size;
}

// Threads:  T*
U32 LLTextureFetch::getTotalNumHTTPRequests()
{
	mNetworkQueueMutex.lock();											// +Mfq
	U32 size = mTotalHTTPRequests;
	mNetworkQueueMutex.unlock();										// -Mfq

	return size;
}

// call lockQueue() first!
// Threads:  T*
// Locks:  Mfq
LLTextureFetchWorker* LLTextureFetch::getWorkerAfterLock(const LLUUID& id)
{
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
	LLMutexLock lock(&mQueueMutex);										// +Mfq

	return getWorkerAfterLock(id);
}																		// -Mfq


// Threads:  T*
bool LLTextureFetch::getRequestFinished(const LLUUID& id, S32& discard_level,
										LLPointer<LLImageRaw>& raw, LLPointer<LLImageRaw>& aux)
{
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
// 				llwarns << "Adding work for inactive worker: " << id << llendl;
				worker->addWork(0, LLWorkerThread::PRIORITY_HIGH | worker->mWorkPriority);
			}
		}
		else if (worker->checkWork())
		{
			worker->lockWorkMutex();									// +Mw
			discard_level = worker->mDecodedDiscard;
			raw = worker->mRawImage;
			aux = worker->mAuxImage;
			res = true;
			LL_DEBUGS("Texture") << id << ": Request Finished. State: " << worker->mState << " Discard: " << discard_level << LL_ENDL;
			worker->unlockWorkMutex();									// -Mw
		}
		else
		{
			worker->lockWorkMutex();									// +Mw
			if ((worker->mDecodedDiscard >= 0) &&
				(worker->mDecodedDiscard < discard_level || discard_level < 0) &&
				(worker->mState >= LLTextureFetchWorker::WAIT_ON_WRITE))
			{
				// Not finished, but data is ready
				discard_level = worker->mDecodedDiscard;
				raw = worker->mRawImage;
				aux = worker->mAuxImage;
			}
			worker->unlockWorkMutex();									// -Mw
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
	bool res = false;
	LLTextureFetchWorker* worker = getWorker(id);
	if (worker)
	{
		worker->lockWorkMutex();										// +Mw
		worker->setImagePriority(priority);
		worker->unlockWorkMutex();										// -Mw
		res = true;
	}
	return res;
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
S32 LLTextureFetch::getPending()
{
	S32 res;
	lockData();															// +Ct
    {
        LLMutexLock lock(&mQueueMutex);									// +Mfq
        
        res = mRequestQueue.size();
        res += mCommands.size();
    }																	// -Mfq
	unlockData();														// -Ct
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
		LLMutexLock lock(&mQueueMutex);									// +Mfq
		
		have_no_commands = mCommands.empty();
	}																	// -Mfq
	
	return ! (have_no_commands
			  && (mRequestQueue.empty() && mIdleThread));		// From base class
}

//////////////////////////////////////////////////////////////////////////////

// Threads:  Ttf
void LLTextureFetch::commonUpdate()
{
	// Run a cross-thread command, if any.
	cmdDoWork();
	
	// Update Curl on same thread as mCurlGetRequest was constructed
	LLCore::HttpStatus status = mHttpRequest->update(200);
	if (! status)
	{
		LL_INFOS_ONCE("Texture") << "Problem during HTTP servicing.  Reason:  "
								 << status.toString()
								 << LL_ENDL;
	}
		
#if 0
	// *FIXME:  maybe implement this another way...
	if (processed > 0)
	{
		lldebugs << "processed: " << processed << " messages." << llendl;
	}
#endif
}


// Threads:  Tmain

//virtual
S32 LLTextureFetch::update(F32 max_time_ms)
{
	static LLCachedControl<F32> band_width(gSavedSettings,"ThrottleBandwidthKBPS");

	{
		mNetworkQueueMutex.lock();										// +Mfnq
		mMaxBandwidth = band_width;

		gTextureList.sTextureBits += mHTTPTextureBits;
		mHTTPTextureBits = 0;

		mNetworkQueueMutex.unlock();									// -Mfnq
	}

	S32 res = LLWorkerThread::update(max_time_ms);
	
	if (!mDebugPause)
	{
		sendRequestListToSimulators();
	}

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
	
// called in the MAIN thread after the ImageDecodeThread shuts down.
//
// Threads:  Tmain
void LLTextureFetch::shutDownImageDecodeThread() 
{
	if(mImageDecodeThread)
	{
		llassert_always(mImageDecodeThread->isQuitting() || mImageDecodeThread->isStopped()) ;
		mImageDecodeThread = NULL ;
	}
}

// Threads:  Ttf
void LLTextureFetch::startThread()
{
}

// Threads:  Ttf
//
// This detaches the texture fetch thread from the LLCore
// HTTP library but doesn't stop the thread running in that
// library...
void LLTextureFetch::endThread()
{
}

// Threads:  Ttf
void LLTextureFetch::threadedUpdate()
{
	llassert_always(mHttpRequest);
	
	// Limit update frequency
	const F32 PROCESS_TIME = 0.05f; 
	static LLFrameTimer process_timer;
	if (process_timer.getElapsedTimeF32() < PROCESS_TIME)
	{
		return;
	}
	process_timer.reset();
	
	commonUpdate();

#if 0
	const F32 INFO_TIME = 1.0f; 
	static LLFrameTimer info_timer;
	if (info_timer.getElapsedTimeF32() >= INFO_TIME)
	{
		S32 q = mCurlGetRequest->getQueued();
		if (q > 0)
		{
			llinfos << "Queued gets: " << q << llendl;
			info_timer.reset();
		}
	}
#endif
	
}

//////////////////////////////////////////////////////////////////////////////

// Threads:  Tmain
void LLTextureFetch::sendRequestListToSimulators()
{
	// All requests
	const F32 REQUEST_DELTA_TIME = 0.10f; // 10 fps
	
	// Sim requests
	const S32 IMAGES_PER_REQUEST = 50;
	const F32 SIM_LAZY_FLUSH_TIMEOUT = 10.0f; // temp
	const F32 MIN_REQUEST_TIME = 1.0f;
	const F32 MIN_DELTA_PRIORITY = 1000.f;

	// Periodically, gather the list of textures that need data from the network
	// And send the requests out to the simulators
	static LLFrameTimer timer;
	if (timer.getElapsedTimeF32() < REQUEST_DELTA_TIME)
	{
		return;
	}
	timer.reset();
	
	// Send requests
	typedef std::set<LLTextureFetchWorker*,LLTextureFetchWorker::Compare> request_list_t;
	typedef std::map< LLHost, request_list_t > work_request_map_t;
	work_request_map_t requests;
	{
		LLMutexLock lock2(&mNetworkQueueMutex);							// +Mfnq
		for (queue_t::iterator iter = mNetworkQueue.begin(); iter != mNetworkQueue.end(); )
		{
			queue_t::iterator curiter = iter++;
			LLTextureFetchWorker* req = getWorker(*curiter);
			if (!req)
			{
				mNetworkQueue.erase(curiter);
				continue; // paranoia
			}
			if ((req->mState != LLTextureFetchWorker::LOAD_FROM_NETWORK) &&
				(req->mState != LLTextureFetchWorker::LOAD_FROM_SIMULATOR))
			{
				// We already received our URL, remove from the queue
				llwarns << "Worker: " << req->mID << " in mNetworkQueue but in wrong state: " << req->mState << llendl;
				mNetworkQueue.erase(curiter);
				continue;
			}
			if (req->mID == mDebugID)
			{
				mDebugCount++; // for setting breakpoints
			}
			if (req->mSentRequest == LLTextureFetchWorker::SENT_SIM &&
				req->mTotalPackets > 0 &&
				req->mLastPacket >= req->mTotalPackets-1)
			{
				// We have all the packets... make sure this is high priority
// 			req->setPriority(LLWorkerThread::PRIORITY_HIGH | req->mWorkPriority);
				continue;
			}
			F32 elapsed = req->mRequestedTimer.getElapsedTimeF32();
			{
				F32 delta_priority = llabs(req->mRequestedPriority - req->mImagePriority);
				if ((req->mSimRequestedDiscard != req->mDesiredDiscard) ||
					(delta_priority > MIN_DELTA_PRIORITY && elapsed >= MIN_REQUEST_TIME) ||
					(elapsed >= SIM_LAZY_FLUSH_TIMEOUT))
				{
					requests[req->mHost].insert(req);
				}
			}
		}
	}																	// -Mfnq

	for (work_request_map_t::iterator iter1 = requests.begin();
		 iter1 != requests.end(); ++iter1)
	{
		LLHost host = iter1->first;
		// invalid host = use agent host
		if (host == LLHost::invalid)
		{
			host = gAgent.getRegionHost();
		}

		S32 sim_request_count = 0;
		
		for (request_list_t::iterator iter2 = iter1->second.begin();
			 iter2 != iter1->second.end(); ++iter2)
		{
			LLTextureFetchWorker* req = *iter2;
			if (gMessageSystem)
			{
				if (req->mSentRequest != LLTextureFetchWorker::SENT_SIM)
				{
					// Initialize packet data based on data read from cache
					req->lockWorkMutex();								// +Mw
					req->setupPacketData();
					req->unlockWorkMutex();								// -Mw		
				}
				if (0 == sim_request_count)
				{
					gMessageSystem->newMessageFast(_PREHASH_RequestImage);
					gMessageSystem->nextBlockFast(_PREHASH_AgentData);
					gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
					gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
				}
				S32 packet = req->mLastPacket + 1;
				gMessageSystem->nextBlockFast(_PREHASH_RequestImage);
				gMessageSystem->addUUIDFast(_PREHASH_Image, req->mID);
				gMessageSystem->addS8Fast(_PREHASH_DiscardLevel, (S8)req->mDesiredDiscard);
				gMessageSystem->addF32Fast(_PREHASH_DownloadPriority, req->mImagePriority);
				gMessageSystem->addU32Fast(_PREHASH_Packet, packet);
				gMessageSystem->addU8Fast(_PREHASH_Type, req->mType);
// 				llinfos << "IMAGE REQUEST: " << req->mID << " Discard: " << req->mDesiredDiscard
// 						<< " Packet: " << packet << " Priority: " << req->mImagePriority << llendl;

				static LLCachedControl<bool> log_to_viewer_log(gSavedSettings,"LogTextureDownloadsToViewerLog");
				static LLCachedControl<bool> log_to_sim(gSavedSettings,"LogTextureDownloadsToSimulator");
				if (log_to_viewer_log || log_to_sim)
				{
					mTextureInfo.setRequestStartTime(req->mID, LLTimer::getTotalTime());
					mTextureInfo.setRequestOffset(req->mID, 0);
					mTextureInfo.setRequestSize(req->mID, 0);
					mTextureInfo.setRequestType(req->mID, LLTextureInfoDetails::REQUEST_TYPE_UDP);
				}

				req->lockWorkMutex();									// +Mw
				req->mSentRequest = LLTextureFetchWorker::SENT_SIM;
				req->mSimRequestedDiscard = req->mDesiredDiscard;
				req->mRequestedPriority = req->mImagePriority;
				req->mRequestedTimer.reset();
				req->unlockWorkMutex();									// -Mw
				sim_request_count++;
				if (sim_request_count >= IMAGES_PER_REQUEST)
				{
// 					llinfos << "REQUESTING " << sim_request_count << " IMAGES FROM HOST: " << host.getIPString() << llendl;

					gMessageSystem->sendSemiReliable(host, NULL, NULL);
					sim_request_count = 0;
				}
			}
		}
		if (gMessageSystem && sim_request_count > 0 && sim_request_count < IMAGES_PER_REQUEST)
		{
// 			llinfos << "REQUESTING " << sim_request_count << " IMAGES FROM HOST: " << host.getIPString() << llendl;
			gMessageSystem->sendSemiReliable(host, NULL, NULL);
			sim_request_count = 0;
		}
	}
	
	// Send cancelations
	{
		LLMutexLock lock2(&mNetworkQueueMutex);							// +Mfnq
		if (gMessageSystem && !mCancelQueue.empty())
		{
			for (cancel_queue_t::iterator iter1 = mCancelQueue.begin();
				 iter1 != mCancelQueue.end(); ++iter1)
			{
				LLHost host = iter1->first;
				if (host == LLHost::invalid)
				{
					host = gAgent.getRegionHost();
				}
				S32 request_count = 0;
				for (queue_t::iterator iter2 = iter1->second.begin();
					 iter2 != iter1->second.end(); ++iter2)
				{
					if (0 == request_count)
					{
						gMessageSystem->newMessageFast(_PREHASH_RequestImage);
						gMessageSystem->nextBlockFast(_PREHASH_AgentData);
						gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
						gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
					}
					gMessageSystem->nextBlockFast(_PREHASH_RequestImage);
					gMessageSystem->addUUIDFast(_PREHASH_Image, *iter2);
					gMessageSystem->addS8Fast(_PREHASH_DiscardLevel, -1);
					gMessageSystem->addF32Fast(_PREHASH_DownloadPriority, 0);
					gMessageSystem->addU32Fast(_PREHASH_Packet, 0);
					gMessageSystem->addU8Fast(_PREHASH_Type, 0);
// 				llinfos << "CANCELING IMAGE REQUEST: " << (*iter2) << llendl;

					request_count++;
					if (request_count >= IMAGES_PER_REQUEST)
					{
						gMessageSystem->sendSemiReliable(host, NULL, NULL);
						request_count = 0;
					}
				}
				if (request_count > 0 && request_count < IMAGES_PER_REQUEST)
				{
					gMessageSystem->sendSemiReliable(host, NULL, NULL);
				}
			}
			mCancelQueue.clear();
		}
	}																	// -Mfnq
}

//////////////////////////////////////////////////////////////////////////////

// Threads:  T*
// Locks:  Mw
bool LLTextureFetchWorker::insertPacket(S32 index, U8* data, S32 size)
{
	mRequestedTimer.reset();
	if (index >= mTotalPackets)
	{
// 		llwarns << "Received Image Packet " << index << " > max: " << mTotalPackets << " for image: " << mID << llendl;
		return false;
	}
	if (index > 0 && index < mTotalPackets-1 && size != MAX_IMG_PACKET_SIZE)
	{
// 		llwarns << "Received bad sized packet: " << index << ", " << size << " != " << MAX_IMG_PACKET_SIZE << " for image: " << mID << llendl;
		return false;
	}
	
	if (index >= (S32)mPackets.size())
	{
		mPackets.resize(index+1, (PacketData*)NULL); // initializes v to NULL pointers
	}
	else if (mPackets[index] != NULL)
	{
// 		llwarns << "Received duplicate packet: " << index << " for image: " << mID << llendl;
		return false;
	}

	mPackets[index] = new PacketData(data, size);
	while (mLastPacket+1 < (S32)mPackets.size() && mPackets[mLastPacket+1] != NULL)
	{
		++mLastPacket;
	}
	return true;
}

// Threads:  T*
bool LLTextureFetch::receiveImageHeader(const LLHost& host, const LLUUID& id, U8 codec, U16 packets, U32 totalbytes,
										U16 data_size, U8* data)
{
	LLTextureFetchWorker* worker = getWorker(id);
	bool res = true;

	++mPacketCount;
	
	if (!worker)
	{
// 		llwarns << "Received header for non active worker: " << id << llendl;
		res = false;
	}
	else if (worker->mState != LLTextureFetchWorker::LOAD_FROM_NETWORK ||
			 worker->mSentRequest != LLTextureFetchWorker::SENT_SIM)
	{
// 		llwarns << "receiveImageHeader for worker: " << id
// 				<< " in state: " << LLTextureFetchWorker::sStateDescs[worker->mState]
// 				<< " sent: " << worker->mSentRequest << llendl;
		res = false;
	}
	else if (worker->mLastPacket != -1)
	{
		// check to see if we've gotten this packet before
// 		llwarns << "Received duplicate header for: " << id << llendl;
		res = false;
	}
	else if (!data_size)
	{
// 		llwarns << "Img: " << id << ":" << " Empty Image Header" << llendl;
		res = false;
	}
	if (!res)
	{
		mNetworkQueueMutex.lock();										// +Mfnq
		++mBadPacketCount;
		mCancelQueue[host].insert(id);
		mNetworkQueueMutex.unlock();									// -Mfnq 
		return false;
	}

	worker->lockWorkMutex();											// +Mw

	//	Copy header data into image object
	worker->mImageCodec = codec;
	worker->mTotalPackets = packets;
	worker->mFileSize = (S32)totalbytes;	
	llassert_always(totalbytes > 0);
	llassert_always(data_size == FIRST_PACKET_SIZE || data_size == worker->mFileSize);
	res = worker->insertPacket(0, data, data_size);
	worker->setPriority(LLWorkerThread::PRIORITY_HIGH | worker->mWorkPriority);
	worker->mState = LLTextureFetchWorker::LOAD_FROM_SIMULATOR;
	worker->unlockWorkMutex();											// -Mw
	return res;
}


// Threads:  T*
bool LLTextureFetch::receiveImagePacket(const LLHost& host, const LLUUID& id, U16 packet_num, U16 data_size, U8* data)
{
	LLTextureFetchWorker* worker = getWorker(id);
	bool res = true;

	++mPacketCount;
	
	if (!worker)
	{
// 		llwarns << "Received packet " << packet_num << " for non active worker: " << id << llendl;
		res = false;
	}
	else if (worker->mLastPacket == -1)
	{
// 		llwarns << "Received packet " << packet_num << " before header for: " << id << llendl;
		res = false;
	}
	else if (!data_size)
	{
// 		llwarns << "Img: " << id << ":" << " Empty Image Header" << llendl;
		res = false;
	}
	if (!res)
	{
		mNetworkQueueMutex.lock();										// +Mfnq
		++mBadPacketCount;
		mCancelQueue[host].insert(id);
		mNetworkQueueMutex.unlock();									// -Mfnq
		return false;
	}

	worker->lockWorkMutex();											// +Mw
	
	res = worker->insertPacket(packet_num, data, data_size);
	
	if ((worker->mState == LLTextureFetchWorker::LOAD_FROM_SIMULATOR) ||
		(worker->mState == LLTextureFetchWorker::LOAD_FROM_NETWORK))
	{
		worker->setPriority(LLWorkerThread::PRIORITY_HIGH | worker->mWorkPriority);
		worker->mState = LLTextureFetchWorker::LOAD_FROM_SIMULATOR;
	}
	else
	{
// 		llwarns << "receiveImagePacket " << packet_num << "/" << worker->mLastPacket << " for worker: " << id
// 				<< " in state: " << LLTextureFetchWorker::sStateDescs[worker->mState] << llendl;
		removeFromNetworkQueue(worker, true); // failsafe
	}

	if (packet_num >= (worker->mTotalPackets - 1))
	{
		static LLCachedControl<bool> log_to_viewer_log(gSavedSettings,"LogTextureDownloadsToViewerLog");
		static LLCachedControl<bool> log_to_sim(gSavedSettings,"LogTextureDownloadsToSimulator");

		if (log_to_viewer_log || log_to_sim)
		{
			U64 timeNow = LLTimer::getTotalTime();
			mTextureInfo.setRequestSize(id, worker->mFileSize);
			mTextureInfo.setRequestCompleteTimeAndLog(id, timeNow);
		}
	}
	worker->unlockWorkMutex();											// -Mw

	return res;
}

//////////////////////////////////////////////////////////////////////////////

// Threads:  T*
BOOL LLTextureFetch::isFromLocalCache(const LLUUID& id)
{
	BOOL from_cache = FALSE ;

	LLTextureFetchWorker* worker = getWorker(id);
	if (worker)
	{
		worker->lockWorkMutex();										// +Mw
		from_cache = worker->mInLocalCache;
		worker->unlockWorkMutex();										// -Mw
	}

	return from_cache ;
}

// Threads:  T*
S32 LLTextureFetch::getFetchState(const LLUUID& id, F32& data_progress_p, F32& requested_priority_p,
								  U32& fetch_priority_p, F32& fetch_dtime_p, F32& request_dtime_p, bool& can_use_http)
{
	S32 state = LLTextureFetchWorker::INVALID;
	F32 data_progress = 0.0f;
	F32 requested_priority = 0.0f;
	F32 fetch_dtime = 999999.f;
	F32 request_dtime = 999999.f;
	U32 fetch_priority = 0;
	
	LLTextureFetchWorker* worker = getWorker(id);
	if (worker && worker->haveWork())
	{
		worker->lockWorkMutex();										// +Mw
		state = worker->mState;
		fetch_dtime = worker->mFetchTimer.getElapsedTimeF32();
		request_dtime = worker->mRequestedTimer.getElapsedTimeF32();
		if (worker->mFileSize > 0)
		{
			if (state == LLTextureFetchWorker::LOAD_FROM_SIMULATOR)
			{
				S32 data_size = FIRST_PACKET_SIZE + (worker->mLastPacket-1) * MAX_IMG_PACKET_SIZE;
				data_size = llmax(data_size, 0);
				data_progress = (F32)data_size / (F32)worker->mFileSize;
			}
			else if (worker->mFormattedImage.notNull())
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
		fetch_priority = worker->getPriority();
		can_use_http = worker->getCanUseHTTP() ;
		worker->unlockWorkMutex();										// -Mw
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
	llinfos << "LLTextureFetch REQUESTS:" << llendl;
	for (request_queue_t::iterator iter = mRequestQueue.begin();
		 iter != mRequestQueue.end(); ++iter)
	{
		LLQueuedThread::QueuedRequest* qreq = *iter;
		LLWorkerThread::WorkRequest* wreq = (LLWorkerThread::WorkRequest*)qreq;
		LLTextureFetchWorker* worker = (LLTextureFetchWorker*)wreq->getWorkerClass();
		llinfos << " ID: " << worker->mID
				<< " PRI: " << llformat("0x%08x",wreq->getPriority())
				<< " STATE: " << worker->sStateDescs[worker->mState]
				<< llendl;
	}

	llinfos << "LLTextureFetch ACTIVE_HTTP:" << llendl;
	for (queue_t::const_iterator iter(mHTTPTextureQueue.begin());
		 mHTTPTextureQueue.end() != iter;
		 ++iter)
	{
		llinfos << " ID: " << (*iter) << llendl;
	}

	llinfos << "LLTextureFetch WAIT_HTTP_RESOURCE:" << llendl;
	for (wait_http_res_queue_t::const_iterator iter(mHttpWaitResource.begin());
		 mHttpWaitResource.end() != iter;
		 ++iter)
	{
		llinfos << " ID: " << (*iter) << llendl;
	}
}

//////////////////////////////////////////////////////////////////////////////

// HTTP Resource Waiting Methods

// Threads:  Ttf
void LLTextureFetch::addHttpWaiter(const LLUUID & tid)
{
	mNetworkQueueMutex.lock();											// +Mfnq
	mHttpWaitResource.insert(tid);
	mNetworkQueueMutex.unlock();										// -Mfnq
}

// Threads:  Ttf
void LLTextureFetch::removeHttpWaiter(const LLUUID & tid)
{
	mNetworkQueueMutex.lock();											// +Mfnq
	wait_http_res_queue_t::iterator iter(mHttpWaitResource.find(tid));
	if (mHttpWaitResource.end() != iter)
	{
		mHttpWaitResource.erase(iter);
	}
	mNetworkQueueMutex.unlock();										// -Mfnq
}

// Threads:  Ttf
// Locks:  -Mw (must not hold any worker when called)
void LLTextureFetch::releaseHttpWaiters()
{
	if (HTTP_REQUESTS_IN_QUEUE_LOW_WATER < getNumHTTPRequests())
		return;

	// Quickly make a copy of all the LLUIDs.  Get off the
	// mutex as early as possible.

	typedef std::vector<LLUUID> uuid_vec_t;
	uuid_vec_t tids;

	{
		LLMutexLock lock(&mNetworkQueueMutex);							// +Mfnq

		if (mHttpWaitResource.empty())
			return;

		const size_t limit(mHttpWaitResource.size());
		tids.resize(limit);
		wait_http_res_queue_t::iterator iter(mHttpWaitResource.begin());
		for (int i(0);
			 i < limit && mHttpWaitResource.end() != iter;
			 ++i, ++iter)
		{
			tids[i] = *iter;
		}
	}																	// -Mfnq

	// Now lookup the UUUIDs to find valid requests and sort
	// them in priority order, highest to lowest.
	typedef std::set<LLTextureFetchWorker *, LLTextureFetchWorker::Compare> worker_set_t;
	worker_set_t tids2;

	for (uuid_vec_t::const_iterator iter(tids.begin());
		 tids.end() != iter;
		 ++iter)
	{
		LLTextureFetchWorker * worker(getWorker(* iter));
		if (worker)
		{
			tids2.insert(worker);
		}
	}

	// Release workers up to the high water mark.  Since we aren't
	// holding any locks at this point, we can be in competition
	// with other callers.  Do defensive things like getting
	// refreshed counts of requests and checking if someone else
	// has moved any worker state around....
	tids.clear();
	for (worker_set_t::iterator iter2(tids2.begin());
		 tids2.end() != iter2 && 0 < (HTTP_REQUESTS_IN_QUEUE_HIGH_WATER - getNumHTTPRequests());
		 ++iter2)
	{
		LLTextureFetchWorker * worker(* iter2);

		worker->lockWorkMutex();										// +Mw
		if (LLTextureFetchWorker::WAIT_HTTP_RESOURCE != worker->mState)
		{
			worker->unlockWorkMutex();									// -Mw
			continue;
		}
		worker->mHttpReleased = true;
		worker->mState = LLTextureFetchWorker::SEND_HTTP_REQ;
		worker->setPriority(LLWorkerThread::PRIORITY_HIGH | worker->mWorkPriority);
		worker->unlockWorkMutex();										// -Mw

		removeHttpWaiter(worker->mID);
	}
}

// Threads:  T*
void LLTextureFetch::cancelHttpWaiters()
{
	mNetworkQueueMutex.lock();											// +Mfnq
	mHttpWaitResource.clear();
	mNetworkQueueMutex.unlock();										// -Mfnq
}

// Threads:  T*
int LLTextureFetch::getHttpWaitersCount()
{
	mNetworkQueueMutex.lock();											// +Mfnq
	int ret(mHttpWaitResource.size());
	mNetworkQueueMutex.unlock();										// -Mfnq
	return ret;
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
										LLViewerAssetStats * main_stats)
{
	TFReqSendMetrics * req = new TFReqSendMetrics(caps_url, session_id, agent_id, main_stats);

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
	lockQueue();														// +Mfq
	mCommands.push_back(req);
	unlockQueue();														// -Mfq

	unpause();
}

// Threads:  T*
LLTextureFetch::TFRequest * LLTextureFetch::cmdDequeue()
{
	TFRequest * ret = 0;
	
	lockQueue();														// +Mfq
	if (! mCommands.empty())
	{
		ret = mCommands.front();
		mCommands.erase(mCommands.begin());
	}
	unlockQueue();														// -Mfq

	return ret;
}

// Threads:  Ttf
void LLTextureFetch::cmdDoWork()
{
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
			LL_WARNS("Texture") << "Successfully delivered asset metrics to grid."
								<< LL_ENDL;
		}
		else
		{
			LL_WARNS("Texture") << "Error delivering asset metrics to grid.  Reason:  "
								<< status.toString() << LL_ENDL;
		}
	}
}; // end class AssetReportHandler

AssetReportHandler stats_handler;


/**
 * Implements the 'Set Region' command.
 *
 * Thread:  Thread1 (TextureFetch)
 */
bool
TFReqSetRegion::doWork(LLTextureFetch *)
{
	LLViewerAssetStatsFF::set_region_thread1(mRegionHandle);

	return true;
}


TFReqSendMetrics::~TFReqSendMetrics()
{
	delete mMainStats;
	mMainStats = 0;
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
	static const U32 report_priority(1);
	static const int report_policy_class(LLCore::HttpRequest::DEFAULT_POLICY_ID);
	static LLCore::HttpHandler * const handler(fetcher->isQAMode() || true ? &stats_handler : NULL);
	
	if (! gViewerAssetStatsThread1)
		return true;

	static volatile bool reporting_started(false);
	static volatile S32 report_sequence(0);
    
	// We've taken over ownership of the stats copy at this
	// point.  Get a working reference to it for merging here
	// but leave it in 'this'.  Destructor will rid us of it.
	LLViewerAssetStats & main_stats = *mMainStats;

	// Merge existing stats into those from main, convert to LLSD
	main_stats.merge(*gViewerAssetStatsThread1);
	LLSD merged_llsd = main_stats.asLLSD(true);

	// Add some additional meta fields to the content
	merged_llsd["session_id"] = mSessionID;
	merged_llsd["agent_id"] = mAgentID;
	merged_llsd["message"] = "ViewerAssetMetrics";					// Identifies the type of metrics
	merged_llsd["sequence"] = report_sequence;						// Sequence number
	merged_llsd["initial"] = ! reporting_started;					// Initial data from viewer
	merged_llsd["break"] = LLTextureFetch::svMetricsDataBreak;		// Break in data prior to this report
		
	// Update sequence number
	if (S32_MAX == ++report_sequence)
		report_sequence = 0;
	reporting_started = true;
	
	// Limit the size of the stats report if necessary.
	merged_llsd["truncated"] = truncate_viewer_metrics(10, merged_llsd);

	if (! mCapsURL.empty())
	{
		// *FIXME:  This mess to get an llsd into a string though
		// it's actually no worse than what we currently do...
		std::stringstream body;
		LLSDSerialize::toXML(merged_llsd, body);
		std::string body_str(body.str());
		body.clear();
		
		LLCore::HttpHeaders * headers = new LLCore::HttpHeaders;
		headers->mHeaders.push_back("Content-Type: application/llsd+xml");

		LLCore::BufferArray * ba = new LLCore::BufferArray;
		ba->append(body_str.c_str(), body_str.length());
		body_str.clear();
		
		fetcher->getHttpRequest().requestPost(report_policy_class,
											  report_priority,
											  mCapsURL,
											  ba,
											  NULL,
											  headers,
											  handler);
		ba->release();
		headers->release();
		LLTextureFetch::svMetricsDataBreak = false;
	}
	else
	{
		LLTextureFetch::svMetricsDataBreak = true;
	}

	// In QA mode, Metrics submode, log the result for ease of testing
	if (fetcher->isQAMode() || true)
	{
		LL_INFOS("Textures") << merged_llsd << LL_ENDL;
	}

	gViewerAssetStatsThread1->reset();

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

