/** 
 * @file lltexturefetch.h
 * @brief Object for managing texture fetches.
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

#ifndef LL_LLTEXTUREFETCH_H
#define LL_LLTEXTUREFETCH_H

#include <vector>
#include <map>

#include "lldir.h"
#include "llimage.h"
#include "lluuid.h"
#include "llworkerthread.h"
#include "lltextureinfo.h"
#include "llapr.h"
#include "llimageworker.h"
#include "llcurl.h"
#include "httprequest.h"
#include "httpoptions.h"
#include "httpheaders.h"
#include "httphandler.h"

class LLViewerTexture;
class LLTextureFetchWorker;
class LLImageDecodeThread;
class LLHost;
class LLViewerAssetStats;
class LLTextureFetchDebugger;
class LLTextureCache;

// Interface class

class LLTextureFetch : public LLWorkerThread
{
	friend class LLTextureFetchWorker;
	
public:
	LLTextureFetch(LLTextureCache* cache, LLImageDecodeThread* imagedecodethread, bool threaded, bool qa_mode);
	~LLTextureFetch();

	class TFRequest;
	
    // Threads:  Tmain
	/*virtual*/ S32 update(F32 max_time_ms);
	
	// called in the main thread after the TextureCacheThread shuts down.
    // Threads:  Tmain
	void shutDownTextureCacheThread();

	//called in the main thread after the ImageDecodeThread shuts down.
    // Threads:  Tmain
	void shutDownImageDecodeThread();

	// Threads:  T* (but Tmain mostly)
	bool createRequest(const std::string& url, const LLUUID& id, const LLHost& host, F32 priority,
					   S32 w, S32 h, S32 c, S32 discard, bool needs_aux, bool can_use_http);

	// Requests that a fetch operation be deleted from the queue.
	// If @cancel is true, also stops any I/O operations pending.
	// Actual delete will be scheduled and performed later.
	//
	// Note:  This *looks* like an override/variant of the
	// base class's deleteRequest() but is functionally quite
	// different.
	//
	// Threads:  T*
	void deleteRequest(const LLUUID& id, bool cancel);

	void deleteAllRequests();

	// Threads:  T*
	bool getRequestFinished(const LLUUID& id, S32& discard_level,
							LLPointer<LLImageRaw>& raw, LLPointer<LLImageRaw>& aux);

	// Threads:  T*
	bool updateRequestPriority(const LLUUID& id, F32 priority);

    // Threads:  T*
	bool receiveImageHeader(const LLHost& host, const LLUUID& id, U8 codec, U16 packets, U32 totalbytes, U16 data_size, U8* data);

    // Threads:  T*
	bool receiveImagePacket(const LLHost& host, const LLUUID& id, U16 packet_num, U16 data_size, U8* data);

    // Threads:  T* (but not safe)
	void setTextureBandwidth(F32 bandwidth) { mTextureBandwidth = bandwidth; }
	
    // Threads:  T* (but not safe)
	F32 getTextureBandwidth() { return mTextureBandwidth; }
	
    // Threads:  T*
	BOOL isFromLocalCache(const LLUUID& id);

	// @return	Magic number giving the internal state of the
	//			request.  We should make these codes public if we're
	//			going to return them as a status value.
	//
    // Threads:  T*
	S32 getFetchState(const LLUUID& id, F32& decode_progress_p, F32& requested_priority_p,
					  U32& fetch_priority_p, F32& fetch_dtime_p, F32& request_dtime_p, bool& can_use_http);

	// Debug utility - generally not safe
	void dump();

	// Threads:  T*
	S32 getNumRequests();

	// Threads:  T*
	S32 getNumHTTPRequests();

	// Threads:  T*
	U32 getTotalNumHTTPRequests();
	
    // Threads:  T*
    S32 getPending();

    // Threads:  T*
	void lockQueue() { mQueueMutex.lock(); }

    // Threads:  T*
	void unlockQueue() { mQueueMutex.unlock(); }

	// Threads:  T*
	LLTextureFetchWorker* getWorker(const LLUUID& id);
	
	// Threads:  T*
	// Locks:  Mfq
	LLTextureFetchWorker* getWorkerAfterLock(const LLUUID& id);

	// Commands available to other threads to control metrics gathering operations.

	// Threads:  T*
	void commandSetRegion(U64 region_handle);

	// Threads:  T*
	void commandSendMetrics(const std::string & caps_url,
							const LLUUID & session_id,
							const LLUUID & agent_id,
							LLViewerAssetStats * main_stats);

	// Threads:  T*
	void commandDataBreak();

	// Threads:  T*
	LLCore::HttpRequest & getHttpRequest()	{ return *mHttpRequest; }

	// Threads:  T*
	LLCore::HttpRequest::policy_t getPolicyClass() const { return mHttpPolicyClass; }
	
	// Return a pointer to the shared metrics headers definition.
	// Does not increment the reference count, caller is required
	// to do that to hold a reference for any length of time.
	//
	// Threads:  T*
	LLCore::HttpHeaders * getMetricsHeaders() const	{ return mHttpMetricsHeaders; }

	bool isQAMode() const				{ return mQAMode; }

	// ----------------------------------
	// HTTP resource waiting methods

    // Threads:  T*
	void addHttpWaiter(const LLUUID & tid);

    // Threads:  T*
	void removeHttpWaiter(const LLUUID & tid);

    // Threads:  T*
	bool isHttpWaiter(const LLUUID & tid);

	// If there are slots, release one or more LLTextureFetchWorker
	// requests from resource wait state (WAIT_HTTP_RESOURCE) to
	// active (SEND_HTTP_REQ).
	//
	// Because this will modify state of many workers, you may not
	// hold any Mw lock while calling.  This makes it a little
	// inconvenient to use but that's the rule.
	//
    // Threads:  T*
	// Locks:  -Mw (must not hold any worker when called)
	void releaseHttpWaiters();

    // Threads:  T*
	void cancelHttpWaiters();

    // Threads:  T*
	int getHttpWaitersCount();
	// ----------------------------------
	// Stats management

	// Add given counts to the global totals for the states/requests
	// Threads:  T*
	void updateStateStats(U32 cache_read, U32 cache_write, U32 res_wait);

	// Return the global counts
	// Threads:  T*
	void getStateStats(U32 * cache_read, U32 * cache_write, U32 * res_wait);

	// ----------------------------------
	
protected:
	// Threads:  T* (but Ttf in practice)
	void addToNetworkQueue(LLTextureFetchWorker* worker);

	// Threads:  T*
	void removeFromNetworkQueue(LLTextureFetchWorker* worker, bool cancel);

    // Threads:  T*
	void addToHTTPQueue(const LLUUID& id);

	// XXX possible delete
    // Threads:  T*
	void removeFromHTTPQueue(const LLUUID& id, S32 received_size);

	// Identical to @deleteRequest but with different arguments
	// (caller already has the worker pointer).
	//
	// Threads:  T*
	void removeRequest(LLTextureFetchWorker* worker, bool cancel);
	
	// Overrides from the LLThread tree
	// Locks:  Ct
	bool runCondition();

private:
    // Threads:  Tmain
	void sendRequestListToSimulators();
	
	// Threads:  Ttf
	/*virtual*/ void startThread(void);
	
	// Threads:  Ttf
	/*virtual*/ void endThread(void);
	
	// Threads:  Ttf
	/*virtual*/ void threadedUpdate(void);

	// Threads:  Ttf
	void commonUpdate();

	// Metrics command helpers
	/**
	 * Enqueues a command request at the end of the command queue
	 * and wakes up the thread as needed.
	 *
	 * Takes ownership of the TFRequest object.
	 *
	 * Method locks the command queue.
	 *
	 * Threads:  T*
	 */
	void cmdEnqueue(TFRequest *);

	/**
	 * Returns the first TFRequest object in the command queue or
	 * NULL if none is present.
	 *
	 * Caller acquires ownership of the object and must dispose of it.
	 *
	 * Method locks the command queue.
	 *
	 * Threads:  T*
	 */
	TFRequest * cmdDequeue();

	/**
	 * Processes the first command in the queue disposing of the
	 * request on completion.  Successive calls are needed to perform
	 * additional commands.
	 *
	 * Method locks the command queue.
	 *
	 * Threads:  Ttf
	 */
	void cmdDoWork();
	
public:
	LLUUID mDebugID;
	S32 mDebugCount;
	BOOL mDebugPause;
	S32 mPacketCount;
	S32 mBadPacketCount;
	
private:
	LLMutex mQueueMutex;        //to protect mRequestMap and mCommands only
	LLMutex mNetworkQueueMutex; //to protect mNetworkQueue, mHTTPTextureQueue and mCancelQueue.

	static LLStat sCacheHitRate;
	static LLStat sCacheReadLatency;

	LLTextureCache* mTextureCache;
	LLImageDecodeThread* mImageDecodeThread;
	
	// Map of all requests by UUID
	typedef std::map<LLUUID,LLTextureFetchWorker*> map_t;
	map_t mRequestMap;													// Mfq

	// Set of requests that require network data
	typedef std::set<LLUUID> queue_t;
	queue_t mNetworkQueue;												// Mfnq
	queue_t mHTTPTextureQueue;											// Mfnq
	typedef std::map<LLHost,std::set<LLUUID> > cancel_queue_t;
	cancel_queue_t mCancelQueue;										// Mfnq
	F32 mTextureBandwidth;												// <none>
	F32 mMaxBandwidth;													// Mfnq
	LLTextureInfo mTextureInfo;

	// XXX possible delete
	U32 mHTTPTextureBits;												// Mfnq

	// XXX possible delete
	//debug use
	U32 mTotalHTTPRequests;

	// Out-of-band cross-thread command queue.  This command queue
	// is logically tied to LLQueuedThread's list of
	// QueuedRequest instances and so must be covered by the
	// same locks.
	typedef std::vector<TFRequest *> command_queue_t;
	command_queue_t mCommands;											// Mfq

	// If true, modifies some behaviors that help with QA tasks.
	const bool mQAMode;

	// Interfaces and objects into the core http library used
	// to make our HTTP requests.  These replace the various
	// LLCurl interfaces used in the past.
	LLCore::HttpRequest *				mHttpRequest;					// Ttf
	LLCore::HttpOptions *				mHttpOptions;					// Ttf
	LLCore::HttpHeaders *				mHttpHeaders;					// Ttf
	LLCore::HttpHeaders *				mHttpMetricsHeaders;			// Ttf
	LLCore::HttpRequest::policy_t		mHttpPolicyClass;				// T*

	// We use a resource semaphore to keep HTTP requests in
	// WAIT_HTTP_RESOURCE2 if there aren't sufficient slots in the
	// transport.  This keeps them near where they can be cheaply
	// reprioritized rather than dumping them all across a thread
	// where it's more expensive to get at them.  Requests in either
	// SEND_HTTP_REQ or WAIT_HTTP_REQ charge against the semaphore
	// and tracking state transitions is critical to liveness.
	LLAtomicS32							mHttpSemaphore;					// Ttf + Tmain
	
	typedef std::set<LLUUID> wait_http_res_queue_t;
	wait_http_res_queue_t				mHttpWaitResource;				// Mfnq

	// Cumulative stats on the states/requests issued by
	// textures running through here.
	U32 mTotalCacheReadCount;											// Mfq
	U32 mTotalCacheWriteCount;											// Mfq
	U32 mTotalResourceWaitCount;										// Mfq
	
public:
	// A probabilistically-correct indicator that the current
	// attempt to log metrics follows a break in the metrics stream
	// reporting due to either startup or a problem POSTing data.
	static volatile bool svMetricsDataBreak;

public:
	//debug use
	enum e_tex_source
	{
		FROM_ALL = 0,
		FROM_HTTP_ONLY,
		INVALID_SOURCE
	};
private:
	//debug use
	LLTextureFetchDebugger* mFetchDebugger;
	bool mFetcherLocked;
	
	e_tex_source mFetchSource;
	e_tex_source mOriginFetchSource;

public:
	//debug use
	LLTextureFetchDebugger* getFetchDebugger() { return mFetchDebugger;}
	void lockFetcher(bool lock) { mFetcherLocked = lock;}

	void setLoadSource(e_tex_source source) {mFetchSource = source;}
	void resetLoadSource() {mFetchSource = mOriginFetchSource;}
	bool canLoadFromCache() { return mFetchSource != FROM_HTTP_ONLY;}
};

//debug use
class LLViewerFetchedTexture;
class LLTextureFetchDebugger : public LLCore::HttpHandler
{
	friend class LLTextureFetch;
public:
	LLTextureFetchDebugger(LLTextureFetch* fetcher, LLTextureCache* cache, LLImageDecodeThread* imagedecodethread) ;
	~LLTextureFetchDebugger();

public:
	enum e_debug_state
	{
		IDLE = 0,
		START_DEBUG,
		READ_CACHE,
		WRITE_CACHE,
		DECODING,
		HTTP_FETCHING,
		GL_TEX,
		REFETCH_VIS_CACHE,
		REFETCH_VIS_HTTP,
		REFETCH_ALL_CACHE,
		REFETCH_ALL_HTTP,
		INVALID
	};

private:	
	struct FetchEntry
	{
		enum e_curl_state
		{
			CURL_NOT_DONE = 0,
			CURL_IN_PROGRESS,
			CURL_DONE
		};
		LLUUID mID;
		S32 mRequestedSize;
		S32 mDecodedLevel;
		S32 mFetchedSize;
		S32 mDecodedSize;
		BOOL mNeedsAux;
		U32 mCacheHandle;
		LLPointer<LLImageFormatted> mFormattedImage;
		LLPointer<LLImageRaw> mRawImage;
		e_curl_state mCurlState;
		S32 mCurlReceivedSize;
		S32 mHTTPFailCount;
		LLCore::HttpHandle mHttpHandle;

		FetchEntry() :
			mDecodedLevel(-1),
			mFetchedSize(0),
			mDecodedSize(0),
			mHttpHandle(LLCORE_HTTP_HANDLE_INVALID)
			{}
		FetchEntry(LLUUID& id, S32 r_size, /*S32 f_discard, S32 c,*/ S32 level, S32 f_size, S32 d_size) :
			mID(id),
			mRequestedSize(r_size),
			mDecodedLevel(level),
			mFetchedSize(f_size),
			mDecodedSize(d_size),
			mNeedsAux(false),
			mHTTPFailCount(0),
			mHttpHandle(LLCORE_HTTP_HANDLE_INVALID)
			{}
	};
	typedef std::vector<FetchEntry> fetch_list_t;
	fetch_list_t mFetchingHistory;

	typedef std::map<LLCore::HttpHandle, S32> handle_fetch_map_t;
	handle_fetch_map_t mHandleToFetchIndex;
	
	e_debug_state mState;
	
	F32 mCacheReadTime;
	F32 mCacheWriteTime;
	F32 mDecodingTime;
	F32 mHTTPTime;
	F32 mGLCreationTime;

	F32 mTotalFetchingTime;
	F32 mRefetchVisCacheTime;
	F32 mRefetchVisHTTPTime;
	F32 mRefetchAllCacheTime;
	F32 mRefetchAllHTTPTime;

	LLTimer mTimer;
	
	LLTextureFetch* mFetcher;
	LLTextureCache* mTextureCache;
	LLImageDecodeThread* mImageDecodeThread;
	LLCore::HttpHeaders* mHttpHeaders;
	LLCore::HttpRequest::policy_t mHttpPolicyClass;
	
	S32 mNumFetchedTextures;
	S32 mNumCacheHits;
	S32 mNumVisibleFetchedTextures;
	S32 mNumVisibleFetchingRequests;
	U32 mFetchedData;
	U32 mDecodedData;
	U32 mVisibleFetchedData;
	U32 mVisibleDecodedData;
	U32 mRenderedData;
	U32 mRenderedDecodedData;
	U32 mFetchedPixels;
	U32 mRenderedPixels;
	U32 mRefetchedVisData;
	U32 mRefetchedVisPixels;
	U32 mRefetchedAllData;
	U32 mRefetchedAllPixels;

	BOOL mFreezeHistory;
	BOOL mStopDebug;
	BOOL mClearHistory;
	BOOL mRefetchNonVis;

	std::string mHTTPUrl;
	S32 mNbCurlRequests;
	S32 mNbCurlCompleted;

	std::map< LLPointer<LLViewerFetchedTexture>, std::vector<S32> > mRefetchList;
	std::vector< LLPointer<LLViewerFetchedTexture> > mTempTexList;
	S32 mTempIndex;
	S32 mHistoryListIndex;

public:
	bool update(F32 max_time); //called in the main thread once per frame

	//fetching history
	void clearHistory();
	void addHistoryEntry(LLTextureFetchWorker* worker);
	
	// Inherited from LLCore::HttpHandler
	// Threads:  Ttf
	virtual void onCompleted(LLCore::HttpHandle handle, LLCore::HttpResponse * response);

	void startWork(e_debug_state state);
	void setStopDebug() {mStopDebug = TRUE;}
	void tryToStopDebug(); //stop everything
	void callbackCacheRead(S32 id, bool success, LLImageFormatted* image,
						   S32 imagesize, BOOL islocal);
	void callbackCacheWrite(S32 id, bool success);
	void callbackDecoded(S32 id, bool success, LLImageRaw* raw, LLImageRaw* aux);
	void callbackHTTP(FetchEntry & fetch, LLCore::HttpResponse * response);

	e_debug_state getState()             {return mState;}
	S32  getNumFetchedTextures()         {return mNumFetchedTextures;}
	S32  getNumFetchingRequests()        {return mFetchingHistory.size();}
	S32  getNumCacheHits()               {return mNumCacheHits;}
	S32  getNumVisibleFetchedTextures()  {return mNumVisibleFetchedTextures;}
	S32  getNumVisibleFetchingRequests() {return mNumVisibleFetchingRequests;}
	U32  getFetchedData()                {return mFetchedData;}
	U32  getDecodedData()                {return mDecodedData;}
	U32  getVisibleFetchedData()         {return mVisibleFetchedData;}
	U32  getVisibleDecodedData()         {return mVisibleDecodedData;}
	U32  getRenderedData()               {return mRenderedData;}
	U32  getRenderedDecodedData()        {return mRenderedDecodedData;}
	U32  getFetchedPixels()              {return mFetchedPixels;}
	U32  getRenderedPixels()             {return mRenderedPixels;}
	U32  getRefetchedVisData()              {return mRefetchedVisData;}
	U32  getRefetchedVisPixels()            {return mRefetchedVisPixels;}
	U32  getRefetchedAllData()              {return mRefetchedAllData;}
	U32  getRefetchedAllPixels()            {return mRefetchedAllPixels;}

	F32  getCacheReadTime()     {return mCacheReadTime;}
	F32  getCacheWriteTime()    {return mCacheWriteTime;}
	F32  getDecodeTime()        {return mDecodingTime;}
	F32  getGLCreationTime()    {return mGLCreationTime;}
	F32  getHTTPTime()          {return mHTTPTime;}
	F32  getTotalFetchingTime() {return mTotalFetchingTime;}
	F32  getRefetchVisCacheTime() {return mRefetchVisCacheTime;}
	F32  getRefetchVisHTTPTime()  {return mRefetchVisHTTPTime;}
	F32  getRefetchAllCacheTime() {return mRefetchAllCacheTime;}
	F32  getRefetchAllHTTPTime()  {return mRefetchAllHTTPTime;}

private:
	void init();
	void clearTextures();//clear fetching results of all textures.
	void clearCache();
	void makeRefetchList();
	void scanRefetchList();

	void lockFetcher();
	void unlockFetcher();

	void lockCache();
	void unlockCache();

	void lockDecoder();
	void unlockDecoder();
	
	S32 fillCurlQueue();

	void startDebug();
	void debugCacheRead();
	void debugCacheWrite();	
	void debugHTTP();
	void debugDecoder();
	void debugGLTextureCreation();
	void debugRefetchVisibleFromCache();
	void debugRefetchVisibleFromHTTP();
	void debugRefetchAllFromCache();
	void debugRefetchAllFromHTTP();

	bool processStartDebug(F32 max_time);
	bool processGLCreation(F32 max_time);

private:
	static bool sDebuggerEnabled;
public:
	static bool isEnabled() {return sDebuggerEnabled;}
};
#endif // LL_LLTEXTUREFETCH_H

