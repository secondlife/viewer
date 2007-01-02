/** 
 * @file llviewerimage.cpp
 * @brief Object which handles a received image (and associated texture(s))
 *
 * Copyright (c) 2000-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "llstl.h"

#include "lltexturefetch.h"

#include "llworkerthread.h"
#include "llimage.h"
#include "llvfs.h"

#include "llviewerimage.h"
#include "viewer.h"

//////////////////////////////////////////////////////////////////////////////

class LLTextureFetchWorker : public LLWorkerClass
{
	friend class LLTextureFetchImpl;

public:
	// From LLWorkerClass
	static void initClass(bool threaded, bool runalways);
	static void updateClass();
	static void cleanupClass();

	// New methods
	static LLTextureFetchWorker* getWorker(const LLUUID& id, const LLHost& host,
										   F32 mPriority, S32 discard,
										   BOOL needs_aux = FALSE);
	static LLTextureFetchWorker* getActiveWorker(const LLUUID& id);
	
private:
	static void sendRequestListToSimulators();
	
public:
	virtual bool doWork(S32 param); // Called from LLWorkerThread::processRequest()

	~LLTextureFetchWorker();
	void relese() { --mActiveCount; }


protected:
	LLTextureFetchWorker(const LLUUID& id, const LLHost& host, F32 mPriority, S32 discard);

private:
	virtual void startWork(S32 param); // called from addWork() (MAIN THREAD)
	virtual void endWork(S32 param, bool aborted); // called from doWork() (MAIN THREAD)

	void setImagePriority(F32 priority);
	void setDesiredDiscard(S32 discard);
	void insertPacket(S32 index, U8* data, S32 size);
	void clearPackets();
	U32 calcWorkPriority();
	bool startVFSLoad(LLVFS* vfs, LLAssetType::EType asset_type);
	bool loadFromVFS();
	bool processSimulatorPackets();
	void startDecode();
	bool decodeImage();

	void lockWorkData() { mWorkMutex.lock(); }
	void unlockWorkData() { mWorkMutex.unlock(); }
	
	static void lockQueue() { sDataMutex->lock(); }
	static void unlockQueue() { sDataMutex->unlock(); }
	
private:
	enum e_state
	{
		INIT = 1,
		LOAD_FROM_CACHE,
		LOAD_FROM_SIMULATOR,
		WRITE_TO_VFS,
		DECODE_IMAGE,
		DECODE_IMAGE_UPDATE,
		DONE
	};
	e_state mState;
	LLPointer<LLImageFormatted> mFormattedImage;
	LLPointer<LLImageRaw> mRawImage;
	LLPointer<LLImageRaw> mAuxImage;
	LLUUID mID;
	LLHost mHost;
	F32 mPriority;
	S32 mDesiredDiscard;
	S32 mRequestedDiscard;
	S32 mDecodedDiscard;
	LLFrameTimer mRequestedTimer;
	LLFrameTimer mIdleTimer;
	LLVFSThread::handle_t mFileHandle;
	U8* mBuffer;
	S32 mBufferSize;
	S32 mRequestedSize;
	BOOL mLoaded;
	BOOL mRequested;
	BOOL mDecoded;
	BOOL mNeedsAux;
	S32 mActiveCount;

	// Work Data
	LLMutex mWorkMutex;
	struct PacketData
	{
		PacketData(U8* data, S32 size) { mData = data; mSize = size; }
		~PacketData() { clearData(); }
		void clearData() { delete mData; mData = NULL; }
		U8* mData;
		U32 mSize;
	};
	std::vector<PacketData*> mPackets;
	S32 mLastPacket;
	S32 mTotalPackets;
	S32 mTotalBytes;
	
	// Class variables (statics)
	
	static LLWorkerThread* sWorkerThread;
	static LLMutex* sDataMutex;

	// Map of all requests by UUID
	typedef std::map<LLUUID,LLTextureFetchWorker*> map_t;
	static map_t sRequests;

	// Set of requests that require network data
	typedef std::set<LLTextureFetchWorker*> queue_t ;
	static queue_t sNetworkQueue;

	static LLFrameTimer sNetworkTimer;
};

//statics
LLTextureFetchWorker::map_t LLTextureFetchWorker::sRequests;
LLTextureFetchWorker::queue_t LLTextureFetchWorker::sNetworkQueue;
LLFrameTimer LLTextureFetchWorker::sNetworkTimer;
LLWorkerThread* LLTextureFetchWorker::sWorkerThread = NULL;
LLMutex* LLTextureFetchWorker::sDataMutex = NULL;

// called from MAIN THREAD
//static
void LLTextureFetchWorker::initClass(bool threaded, bool runalways)
{
	sWorkerThread = new LLWorkerThread(threaded, runalways);
	sDataMutex = new LLMutex(sWorkerThread->getAPRPool());
}

// called from MAIN THREAD
//static
void LLTextureFetchWorker::updateClass()
{
	const F32 REQUEST_TIME = 1.f;
	const F32 MIN_IDLE_TIME = 1.f * 60.f; // 1 minute
	const F32 MAX_IDLE_TIME = 5.f * 60.f; // 5 minutes
	const S32 MIN_IDLE_COUNT = 16; // always keep last 16 idle requests
	const F32 MAX_IDLE_COUNT = 1024; // max number of idle requests

	// Periodically, gather the list of textures that need data from the network
	// And send the requests out to the simulators
	if (sNetworkTimer.getElapsedTimeF32() >= REQUEST_TIME)
	{
		sNetworkTimer.reset();
		sendRequestListToSimulators();
	}
	// Remove any old requests (releasing their raw data)
	typedef std::pair<F32, LLTextureFetchWorker*> idle_pair;
	typedef std::set<idle_pair, compare_pair_greater<F32,LLTextureFetchWorker*> > idle_set;
	idle_set remove_set;
	for (map_t::iterator iter = sRequests.begin(); iter != sRequests.end(); ++iter)
	{
		LLTextureFetchWorker* worker = iter->second;
		if (worker->mActiveCount > 0)
			continue;
		if (worker->haveWork())
			continue;
		F32 idletime = worker->mIdleTimer.getElapsedTimeF32();
		if (idletime < MIN_IDLE_TIME)
			continue;
		remove_set.insert(std::make_pair(idletime, worker));
	}
	S32 num_left = remove_set.size();
	for (idle_set::iterator iter = remove_set.begin(); iter != remove_set.end(); ++iter)
	{
		if (num_left <= MIN_IDLE_COUNT)
			break;
		if (iter->first < MAX_IDLE_TIME &&
			num_left < MAX_IDLE_COUNT)
			break;
		num_left--;
	}
}

// called from MAIN THREAD
//static
void LLTextureFetchWorker::sendRequestListToSimulators()
{
	const F32 LAZY_FLUSH_TIMEOUT = 60.f;
	const S32 IMAGES_PER_REQUEST = 50;
	// Send requests
	typedef std::map< LLHost, std::vector<LLTextureFetchWorker*> > work_request_map_t;
	work_request_map_t requests;
	for (queue_t::iterator iter = sNetworkQueue.begin(); iter != sNetworkQueue.end(); ++iter)
	{
		LLTextureFetchWorker* req = *iter;
		if (req->haveWork())
		{
			continue; // busy
		}
		if ((req->mRequestedDiscard == req->mDesiredDiscard) &&
			(req->mRequestedTimer.getElapsedTimeF32() < LAZY_FLUSH_TIMEOUT))
		{
			continue;
		}
		req->mRequestedDiscard = req->mDesiredDiscard;
		req->mRequestedTimer.reset();
		requests[req->mHost].push_back(req);
	}
	for (work_request_map_t::iterator iter1 = requests.begin();
		 iter1 != requests.end(); ++iter1)
	{
		LLHost host = iter1->first;
		// invalid host = load from static VFS
		if (host != LLHost::invalid)
		{
			S32 request_count = 0;
			for (std::vector<LLTextureFetchWorker*>::iterator iter2 = iter1->second.begin();
				 iter2 != iter1->second.end(); ++iter2)
			{
				LLTextureFetchWorker* req = *iter2;
				if (0 == request_count)
				{
					gMessageSystem->newMessageFast(_PREHASH_RequestImage);
				}
				S32 packet = req->mLastPacket + 1;
				gMessageSystem->nextBlockFast(_PREHASH_RequestImage);
				gMessageSystem->addUUIDFast(_PREHASH_Image, req->mID);
				gMessageSystem->addS32Fast(_PREHASH_DiscardLevel, req->mDesiredDiscard);
				gMessageSystem->addF32Fast(_PREHASH_DownloadPriority, req->mPriority);
				gMessageSystem->addU32Fast(_PREHASH_Packet, packet);

				request_count++;
				if (request_count >= IMAGES_PER_REQUEST)
				{
					gMessageSystem->sendSemiReliable(host, NULL, NULL);
					request_count = 0;
				}
			}
			if (request_count >= IMAGES_PER_REQUEST)
			{
				gMessageSystem->sendSemiReliable(host, NULL, NULL);
			}
		}
	}
}

//static
LLTextureFetchWorker* LLTextureFetchWorker::getWorker(const LLUUID& id,
													  const LLHost& host,
													  F32 priority,
													  S32 discard,
													  BOOL needs_aux)
{
	LLTextureFetchWorker* res;
	lockQueue();
	map_t::iterator iter = sRequests.find(id);
	if (iter != sRequests.end())
	{
		res = iter->second;
		if (res->mHost != host)
		{
			llerrs << "LLTextureFetchWorker::getWorker called with multiple hosts" << llendl;
		}
		res->setImagePriority(priority);
		res->setDesiredDiscard(discard);
		
	}
	else
	{
		res = new LLTextureFetchWorker(id, host, priority, discard);
	}
	unlockQueue();
	res->mActiveCount++;
	res->mNeedsAux = needs_aux;
	return res;
}

LLTextureFetchWorker* LLTextureFetchWorker::getActiveWorker(const LLUUID& id)
{
	LLTextureFetchWorker* res = NULL;
	lockQueue();
	map_t::iterator iter = sRequests.find(id);
	if (iter != sRequests.end())
	{
		res = iter->second;
	}
	unlockQueue();
	return res;
}

LLTextureFetchWorker::LLTextureFetchWorker(const LLUUID& id,	// Image UUID
										   const LLHost& host,	// Simulator host
										   F32 priority,		// Priority
										   S32 discard)			// Desired discard level
	: LLWorkerClass(sWorkerThread, "TextureFetch"),
	  mState(INIT),
	  mID(id),
	  mHost(host),
	  mPriority(priority),
	  mDesiredDiscard(discard),
	  mRequestedDiscard(-1),
	  mDecodedDiscard(-1),
	  mFileHandle(LLVFSThread::nullHandle()),
	  mBuffer(NULL),
	  mBufferSize(0),
	  mRequestedSize(0),
	  mLoaded(FALSE),
	  mRequested(FALSE),
	  mDecoded(FALSE),
	  mActiveCount(0),
	  mWorkMutex(sWorkerThread->getAPRPool()),
	  mLastPacket(-1),
	  mTotalPackets(0),
	  mTotalBytes(0)
{
	lockQueue();
	sRequests[mID] = this;
	unlockQueue();	
	addWork(0, calcWorkPriority());
}

LLTextureFetchWorker::~LLTextureFetchWorker()
{
	lockQueue();
	mFormattedImage = NULL;
	map_t::iterator iter = sRequests.find(mID);
	if (iter != sRequests.end() && iter->second == this)
	{
		sRequests.erase(iter);
	}
	sNetworkQueue.erase(this);
	unlockQueue();
	clearPackets();
}

void LLTextureFetchWorker::clearPackets()
{
	lockWorkData();
	for_each(mPackets.begin(), mPackets.end(), DeletePointer());
	mPackets.clear();
	unlockWorkData();
}

U32 LLTextureFetchWorker::calcWorkPriority()
{
	F32 priority_scale = (F32)LLWorkerThread::PRIORITY_LOWBITS / LLViewerImage::maxDecodePriority();
	U32 priority = (U32)(mPriority * priority_scale);
	return LLWorkerThread::PRIORITY_NORMAL | priority;
}

void LLTextureFetchWorker::setDesiredDiscard(S32 discard)
{
	if (mDesiredDiscard != discard)
	{
		mDesiredDiscard = discard;
		if (!haveWork())
		{
			addWork(0, calcWorkPriority());
		}
	}
}

void LLTextureFetchWorker::setImagePriority(F32 priority)
{
	if (priority != mPriority)
	{
		mPriority = priority;
		setPriority(calcWorkPriority());
	}
}

void LLTextureFetchWorker::insertPacket(S32 index, U8* data, S32 size)
{
	PacketData* packet = new PacketData(data, size);
	
	lockWorkData();
	if (index >= (S32)mPackets.size())
	{
		mPackets.resize(index+1, (PacketData*)NULL); // initializes v to NULL pointers
	}
	if (mPackets[index] != NULL)
	{
		llwarns << "LLTextureFetchWorker::insertPacket called for duplicate packet: " << index << llendl;
	}
	mPackets[index] = packet;
	while (mLastPacket+1 < (S32)mPackets.size() && mPackets[mLastPacket+1] != NULL)
	{
		++mLastPacket;
	}
	unlockWorkData();
}

// Called from LLWorkerThread::processRequest()
bool LLTextureFetchWorker::doWork(S32 param)
{
	switch(mState)
	{
	  case INIT:
	  {
		  // fall through
	  }
	  case LOAD_FROM_CACHE:
	  {
		  // Load any existing data from the cache
		  if (mFileHandle == LLVFSThread::nullHandle())
		  {
			  bool res = startVFSLoad(gVFS, LLAssetType::AT_TEXTURE);
			  if (!res) res = startVFSLoad(gStaticVFS, LLAssetType::AT_TEXTURE_TGA);
			  if (!res) res = startVFSLoad(gStaticVFS, LLAssetType::AT_TEXTURE);
			  if (!res)
			  {
				  // Didn't load from VFS
				  mFormattedImage = new LLImageJ2C;
				  mState = LOAD_FROM_SIMULATOR;
			  }
		  }
		  if (mFileHandle != LLVFSThread::nullHandle())
		  {
			  if (!loadFromVFS())
			  {
				  return false; // not done
			  }
			  if (!mLoaded)
			  {
				  llwarns << "Load from VFS failed on: " << mID << llendl;
				  return true;
			  }
			  bool res = mFormattedImage->setData(mBuffer, mBufferSize);
			  if (!res)
			  {
				  llwarns << "loadLocalImage() - setData() failed" << llendl;
				  mFormattedImage->deleteData();
				  return true;
			  }
			  // Successfully loaded
			  if (mFormattedImage->getDiscardLevel() <= mRequestedDiscard)
			  {
				  // we have enough data, decode it
				  mState = DECODE_IMAGE;
				  mRequestedSize = mBufferSize;
			  }
			  else
			  {
				  // need more data
				  mState = LOAD_FROM_SIMULATOR;
				  mRequestedSize = mFormattedImage->calcDataSize(mRequestedDiscard);
			  }
		  }
		  return false;
	  }
	  case LOAD_FROM_SIMULATOR:
	  {
		  if (!mRequested)
		  {
			  lockQueue();
			  sNetworkQueue.insert(this);
			  unlockQueue();
			  mRequested = TRUE;
		  }
		  if (processSimulatorPackets())
		  {
			  mState = WRITE_TO_VFS;
		  }
		  return false;
	  }
	  case WRITE_TO_VFS:
	  {
		  mState = DECODE_IMAGE;
		  // fall through
	  }
	  case DECODE_IMAGE:
	  {
		  startDecode();
		  mState = DECODE_IMAGE_UPDATE;
		  // fall through
	  }
	  case DECODE_IMAGE_UPDATE:
	  {
		  if (decodeImage())
		  {
			  mState = DONE;
		  }
		  return false;
	  }
	  case DONE:
	  {
		  // Do any cleanup here
		  // Destroy the formatted image, we don't need it any more (raw image is still valid)
		  mFormattedImage = NULL;
		  mIdleTimer.reset();
		  return true;
	  }
	  default:
	  {
		  llerrs << "LLTextureFetchWorker::doWork() has illegal state" << llendl;
		  return true;
	  }
	}
}

// Called from MAIN thread
void LLTextureFetchWorker::startWork(S32 param)
{
}

void LLTextureFetchWorker::endWork(S32 param, bool aborted)
{
}

//////////////////////////////////////////////////////////////////////////////

bool LLTextureFetchWorker::startVFSLoad(LLVFS* vfs, LLAssetType::EType asset_type)
{
	// Start load from VFS if it's there
	if (vfs->getExists(mID, asset_type))
	{
		mBufferSize = vfs->getSize(mID, asset_type);
		mBuffer = new U8[mBufferSize];
		mFileHandle = LLVFSThread::sLocal->read(vfs, mID, asset_type, mBuffer, 0, mBufferSize);
		if (mFileHandle == LLVFSThread::nullHandle())
		{
			llwarns << "loadLocalImage() - vfs read failed in static VFS: " << mID << llendl;
			delete mBuffer;
			mBuffer = NULL;
			return false;
		}
		if (asset_type == LLAssetType::AT_TEXTURE_TGA)
		{
			mFormattedImage = new LLImageTGA;
		}
		else if (asset_type == LLAssetType::AT_TEXTURE)
		{
			mFormattedImage = new LLImageJ2C;
		}
		else
		{
			llerrs << "LLTextureFetchWorker::startVFSLoad called with bad asset type: " << asset_type << llendl;
		}
		return true;
	}
	return false;
}

bool LLTextureFetchWorker::loadFromVFS()
{
	LLMemType mt1(LLMemType::MTYPE_APPFMTIMAGE);

	llassert(mLoaded == FALSE);
	
	// Check loading status
	LLVFSThread::status_t status = LLVFSThread::sLocal->getRequestStatus(mFileHandle);
	if (status == LLVFSThread::STATUS_QUEUED || status == LLVFSThread::STATUS_INPROGRESS)
	{
		return false;
	}
	else if (status == LLVFSThread::STATUS_COMPLETE)
	{
		mLoaded = TRUE;
		return true;
	}
	else
	{
		llwarns << "loadLocalImage() - vfs read failed" << llendl;
		LLVFSThread::Request* req = (LLVFSThread::Request*)LLVFSThread::sLocal->getRequest(mFileHandle);
		if (req && mFormattedImage.notNull())
		{
			LLVFS* vfs = req->getVFS();
			LLAssetType::EType asset_type = mFormattedImage->getCodec() == IMG_CODEC_TGA ? LLAssetType::AT_TEXTURE_TGA : LLAssetType::AT_TEXTURE;
			vfs->removeFile(mID, asset_type);
		}
		return true;
	}
}


//////////////////////////////////////////////////////////////////////////////

bool LLTextureFetchWorker::processSimulatorPackets()
{
	bool res = false;
	lockWorkData();
	if (mLastPacket >= 0)
	{
		S32 data_size = 0;
		for (S32 i = 0; i<=mLastPacket; i++)
		{
			data_size += mPackets[i]->mSize;
		}
		if (data_size >= mRequestedSize || mLastPacket == mTotalPackets)
		{
			/// We have enough (or all) data, copy it into mBuffer
			if (mBufferSize < data_size)
			{
				delete mBuffer;
				mBuffer = new U8[data_size];
				mBufferSize = data_size;
			}
			S32 offset = 0;
			for (S32 i = 0; i<=mLastPacket; i++)
			{
				memcpy(mBuffer + offset, mPackets[i]->mData, mPackets[i]->mSize);
				offset += mPackets[i]->mSize;
			}
			res = true;
		}
	}
	unlockWorkData();
	return res;
}

//////////////////////////////////////////////////////////////////////////////

void LLTextureFetchWorker::startDecode()
{
	mRawImage = NULL;
	mAuxImage = NULL;
}

bool LLTextureFetchWorker::decodeImage()
{
	const F32 MAX_DECODE_TIME = .001f; // 1 ms
	if (mRawImage->getDataSize() == 0)
	{
		if (!mFormattedImage->requestDecodedData(mRawImage, -1, MAX_DECODE_TIME))
		{
			return false;
		}
		mFormattedImage->releaseDecodedData(); // so that we have the only ref to the raw image
	}
	if (mNeedsAux && mAuxImage->getDataSize() == 0)
	{
		if (!mFormattedImage->requestDecodedAuxData(mAuxImage, 4, -1, MAX_DECODE_TIME ))
		{
			return false;
		}
		mFormattedImage->releaseDecodedData(); // so that we have the only ref to the raw image
	}
	mDecodedDiscard = mFormattedImage->getDiscardLevel();
	return true;
}

//////////////////////////////////////////////////////////////////////////////

#if 0
// static
void LLTextureFetchWorker::receiveImageHeader(LLMessageSystem *msg, void **user_data)
{
	LLFastTimer t(LLFastTimer::FTM_PROCESS_IMAGES);
	
	// Receive image header, copy into image object and decompresses 
	// if this is a one-packet image. 

	gImageList.sTextureBits += msg->getReceiveBytes();
	gImageList.sTexturePackets++;

	LLUUID id;
	msg->getUUIDFast(_PREHASH_ImageID, _PREHASH_ID, id);
// 	LLString ip_string(u32_to_ip_string(msg->getSenderIP()));
	
	LLTextureFetchWorker* worker = getActiveWorker(id);
	if (!worker)
	{
		llwarns << "receiveImageHeader for non active worker: " << id << llendl;
		return;
	}
	worker->mRequestedTimer.reset();
		
	// check to see if we've gotten this packet before
	if (worker->mLastPacket != -1)
	{
		llwarns << "Img: " << id << ":" << " Duplicate Image Header" << llendl;
		return;
	}

	//	Copy header data into image object
	worker->lockWorkData();
	msg->getU8Fast(_PREHASH_ImageID, _PREHASH_Codec, image->mDataCodec);
	msg->getU16Fast(_PREHASH_ImageID, _PREHASH_Packets, image->mTotalPackets);
	msg->getU32Fast(_PREHASH_ImageID, _PREHASH_Size, image->mTotalBytes);
	if (0 == image->mTotalPackets)
	{
		llwarns << "Img: " << id << ":" << " Number of packets is 0" << llendl;
	}
	worker->unlockWorkData();

	U16 data_size = msg->getSizeFast(_PREHASH_ImageData, _PREHASH_Data); 
	if (data_size)
	{
		U8 *data = new U8[data_size];
		msg->getBinaryDataFast(_PREHASH_ImageData, _PREHASH_Data, data, data_size);
		worker->insertPacket(0, data, data_size)
	}
}

///////////////////////////////////////////////////////////////////////////////
// static
void LLTextureFetchWorker::receiveImagePacket(LLMessageSystem *msg, void **user_data)
{
	LLMemType mt1(LLMemType::MTYPE_APPFMTIMAGE);
	LLFastTimer t(LLFastTimer::FTM_PROCESS_IMAGES);
	
	gImageList.sTextureBits += msg->getReceiveBytes();
	gImageList.sTexturePackets++;

	LLUUID id;
	msg->getUUIDFast(_PREHASH_ImageID, _PREHASH_ID, id);
// 	LLString ip_string(u32_to_ip_string(msg->getSenderIP()));

	U16 packet_num;
	msg->getU16Fast(_PREHASH_ImageID, _PREHASH_Packet, packet_num);

	LLTextureFetchWorker* worker = getActiveWorker(id);
	if (!worker)
	{
		llwarns << "receiveImageHeader for non active worker: " << id << llendl;
		return;
	}
	worker->mRequestedTimer.reset();

	U16 data_size = msg->getSizeFast(_PREHASH_ImageData, _PREHASH_Data); 
	if (data_size)
	{
		U8 *data = new U8[data_size];
		msg->getBinaryDataFast(_PREHASH_ImageData, _PREHASH_Data, data, data_size);
		worker->insertPacket(0, data, data_size)
	}
}
#endif

 //////////////////////////////////////////////////////////////////////////////
