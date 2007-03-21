/** 
 * @file lltexturefetch.h
 * @brief Object for managing texture fetches.
 *
 * Copyright (c) 2000-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLTEXTUREFETCH_H
#define LL_LLTEXTUREFETCH_H

#include "lldir.h"
#include "llimage.h"
#include "lluuid.h"
#include "llworkerthread.h"

class LLViewerImage;
class LLTextureFetchWorker;
class LLTextureCache;
class LLHost;

// Interface class
class LLTextureFetch : public LLWorkerThread
{
	friend class LLTextureFetchWorker;
	
public:
	LLTextureFetch(LLTextureCache* cache, bool threaded);
	~LLTextureFetch();

	/*virtual*/ S32 update(U32 max_time_ms);	

	bool createRequest(const LLUUID& id, const LLHost& host, F32 priority,
					   S32 w, S32 h, S32 c, S32 discard, bool needs_aux);
	void deleteRequest(const LLUUID& id, bool cancel);
	bool getRequestFinished(const LLUUID& id, S32& discard_level,
							LLPointer<LLImageRaw>& raw, LLPointer<LLImageRaw>& aux);
	bool updateRequestPriority(const LLUUID& id, F32 priority);

	bool receiveImageHeader(const LLHost& host, const LLUUID& id, U8 codec, U16 packets, U32 totalbytes, U16 data_size, U8* data);
	bool receiveImagePacket(const LLHost& host, const LLUUID& id, U16 packet_num, U16 data_size, U8* data);

	// Debug
	S32 getFetchState(const LLUUID& id, F32& decode_progress_p, F32& requested_priority_p,
					  U32& fetch_priority_p, F32& fetch_dtime_p, F32& request_dtime_p);
	void dump();
	S32 getNumRequests() { return mRequestMap.size(); }
	
	// Public for access by callbacks
	void lockQueue() { mQueueMutex.lock(); }
	void unlockQueue() { mQueueMutex.unlock(); }
	LLTextureFetchWorker* getWorker(const LLUUID& id);
	
protected:
	void addToNetworkQueue(LLTextureFetchWorker* worker);
	void removeFromNetworkQueue(LLTextureFetchWorker* worker);
	void removeRequest(LLTextureFetchWorker* worker, bool cancel);

private:
	void sendRequestListToSimulators();

public:
	LLUUID mDebugID;
	S32 mDebugCount;
	BOOL mDebugPause;
	S32 mPacketCount;
	S32 mBadPacketCount;
	
private:
	LLMutex mQueueMutex;

	LLTextureCache* mTextureCache;
	
	// Map of all requests by UUID
	typedef std::map<LLUUID,LLTextureFetchWorker*> map_t;
	map_t mRequestMap;

	// Set of requests that require network data
	typedef std::set<LLUUID> queue_t;
	queue_t mNetworkQueue;
	typedef std::map<LLHost,std::set<LLUUID> > cancel_queue_t;
	cancel_queue_t mCancelQueue;

	LLFrameTimer mNetworkTimer;
};

#endif // LL_LLTEXTUREFETCH_H
