/** 
 * @file lltexturefetch.h
 * @brief Object for managing texture fetches.
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
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

#ifndef LL_LLTEXTUREFETCH_H
#define LL_LLTEXTUREFETCH_H

#include "lldir.h"
#include "llimage.h"
#include "lluuid.h"
#include "llworkerthread.h"
#include "llcurl.h"
#include "lltextureinfo.h"
#include "llapr.h"
#include "llimageworker.h"
//#include "lltexturecache.h"

class LLViewerTexture;
class LLTextureFetchWorker;
class HTTPGetResponder;
class LLImageDecodeThread;
class LLHost;
class LLViewerAssetStats;
class LLTextureFetchDebugger;
class LLTextureCache;

// Interface class
class LLTextureFetch : public LLWorkerThread
{
	friend class LLTextureFetchWorker;
	friend class HTTPGetResponder;
	
public:
	LLTextureFetch(LLTextureCache* cache, LLImageDecodeThread* imagedecodethread, bool threaded, bool qa_mode);
	~LLTextureFetch();

	class TFRequest;
	
	/*virtual*/ S32 update(F32 max_time_ms);	
	void shutDownTextureCacheThread() ; //called in the main thread after the TextureCacheThread shuts down.
	void shutDownImageDecodeThread() ;  //called in the main thread after the ImageDecodeThread shuts down.

	bool createRequest(const std::string& url, const LLUUID& id, const LLHost& host, F32 priority,
					   S32 w, S32 h, S32 c, S32 discard, bool needs_aux, bool can_use_http);
	void deleteRequest(const LLUUID& id, bool cancel);
	void deleteAllRequests();
	bool getRequestFinished(const LLUUID& id, S32& discard_level,
							LLPointer<LLImageRaw>& raw, LLPointer<LLImageRaw>& aux);
	bool updateRequestPriority(const LLUUID& id, F32 priority);

	bool receiveImageHeader(const LLHost& host, const LLUUID& id, U8 codec, U16 packets, U32 totalbytes, U16 data_size, U8* data);
	bool receiveImagePacket(const LLHost& host, const LLUUID& id, U16 packet_num, U16 data_size, U8* data);

	void setTextureBandwidth(F32 bandwidth) { mTextureBandwidth = bandwidth; }
	F32 getTextureBandwidth() { return mTextureBandwidth; }
	
	// Debug
	BOOL isFromLocalCache(const LLUUID& id);
	S32 getFetchState(const LLUUID& id, F32& decode_progress_p, F32& requested_priority_p,
					  U32& fetch_priority_p, F32& fetch_dtime_p, F32& request_dtime_p, bool& can_use_http);
	void dump();
	S32 getNumRequests() ;
	
	// Public for access by callbacks
    S32 getPending();
	void lockQueue() { mQueueMutex.lock(); }
	void unlockQueue() { mQueueMutex.unlock(); }
	LLTextureFetchWorker* getWorker(const LLUUID& id);
	LLTextureFetchWorker* getWorkerAfterLock(const LLUUID& id);

	LLTextureInfo* getTextureInfo() { return &mTextureInfo; }

	// Commands available to other threads to control metrics gathering operations.
	void commandSetRegion(U64 region_handle);
	void commandSendMetrics(const std::string & caps_url,
							const LLUUID & session_id,
							const LLUUID & agent_id,
							LLViewerAssetStats * main_stats);
	void commandDataBreak();

	LLCurlTextureRequest & getCurlRequest()	{ return *mCurlGetRequest; }

	bool isQAMode() const				{ return mQAMode; }

	// Curl POST counter maintenance
	inline void incrCurlPOSTCount()		{ mCurlPOSTRequestCount++; }
	inline void decrCurlPOSTCount()		{ mCurlPOSTRequestCount--; }

protected:
	void addToNetworkQueue(LLTextureFetchWorker* worker);
	void removeFromNetworkQueue(LLTextureFetchWorker* worker, bool cancel);
	void addToHTTPQueue(const LLUUID& id);
	void removeRequest(LLTextureFetchWorker* worker, bool cancel);

	// Overrides from the LLThread tree
	bool runCondition();

private:
	void sendRequestListToSimulators();
	/*virtual*/ void startThread(void);
	/*virtual*/ void endThread(void);
	/*virtual*/ void threadedUpdate(void);
	void commonUpdate();

	// Metrics command helpers
	/**
	 * Enqueues a command request at the end of the command queue
	 * and wakes up the thread as needed.
	 *
	 * Takes ownership of the TFRequest object.
	 *
	 * Method locks the command queue.
	 */
	void cmdEnqueue(TFRequest *);

	/**
	 * Returns the first TFRequest object in the command queue or
	 * NULL if none is present.
	 *
	 * Caller acquires ownership of the object and must dispose of it.
	 *
	 * Method locks the command queue.
	 */
	TFRequest * cmdDequeue();

	/**
	 * Processes the first command in the queue disposing of the
	 * request on completion.  Successive calls are needed to perform
	 * additional commands.
	 *
	 * Method locks the command queue.
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
	LLCurlTextureRequest* mCurlGetRequest;

	// Map of all requests by UUID
	typedef std::map<LLUUID,LLTextureFetchWorker*> map_t;
	map_t mRequestMap;

	// Set of requests that require network data
	typedef std::set<LLUUID> queue_t;
	queue_t mNetworkQueue;
	queue_t mHTTPTextureQueue;
	typedef std::map<LLHost,std::set<LLUUID> > cancel_queue_t;
	cancel_queue_t mCancelQueue;
	F32 mTextureBandwidth;
	F32 mMaxBandwidth;
	LLTextureInfo mTextureInfo;

	// Out-of-band cross-thread command queue.  This command queue
	// is logically tied to LLQueuedThread's list of
	// QueuedRequest instances and so must be covered by the
	// same locks.
	typedef std::vector<TFRequest *> command_queue_t;
	command_queue_t mCommands;

	// If true, modifies some behaviors that help with QA tasks.
	const bool mQAMode;

	// Count of POST requests outstanding.  We maintain the count
	// indirectly in the CURL request responder's ctor and dtor and
	// use it when determining whether or not to sleep the thread.  Can't
	// use the LLCurl module's request counter as it isn't thread compatible.
	// *NOTE:  Don't mix Atomic and static, apr_initialize must be called first.
	LLAtomic32<S32> mCurlPOSTRequestCount;
	
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
class LLTextureFetchDebugger
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

		FetchEntry() :
			mDecodedLevel(-1),
			mFetchedSize(0),
			mDecodedSize(0)
			{}
		FetchEntry(LLUUID& id, S32 r_size, /*S32 f_discard, S32 c,*/ S32 level, S32 f_size, S32 d_size) :
			mID(id),
			mRequestedSize(r_size),
			mDecodedLevel(level),
			mFetchedSize(f_size),
			mDecodedSize(d_size),
			mNeedsAux(false),
			mHTTPFailCount(0)
			{}
	};
	std::vector<FetchEntry> mFetchingHistory;
	
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
	LLCurlTextureRequest* mCurlGetRequest;
	
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
	
	void setCurlGetRequest(LLCurlTextureRequest* request) { mCurlGetRequest = request;}
	LLCurlTextureRequest* getCurlGetRequest() { return mCurlGetRequest;}

	void startWork(e_debug_state state);
	void setStopDebug() {mStopDebug = TRUE;}
	void tryToStopDebug(); //stop everything
	
	void callbackCacheRead(S32 id, bool success, LLImageFormatted* image,
						   S32 imagesize, BOOL islocal);
	void callbackCacheWrite(S32 id, bool success);
	void callbackDecoded(S32 id, bool success, LLImageRaw* raw, LLImageRaw* aux);
	void callbackHTTP(S32 id, const LLChannelDescriptors& channels,
					  const LLIOPipe::buffer_ptr_t& buffer, 
					  bool partial, bool success);
	

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

