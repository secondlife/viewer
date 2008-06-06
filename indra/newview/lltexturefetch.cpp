/** 
 * @file lltexturefetch.cpp
 * @brief Object which fetches textures from the cache and/or network
 *
 * $LicenseInfo:firstyear=2000&license=viewergpl$
 * 
 * Copyright (c) 2000-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"

#include "llstl.h"

#include "lltexturefetch.h"

#include "llcurl.h"
#include "llhttpclient.h"
#include "llimage.h"
#include "llimageworker.h"
#include "llworkerthread.h"

#include "llagent.h"
#include "lltexturecache.h"
#include "llviewerimagelist.h"
#include "llviewerimage.h"
#include "llviewerregion.h"

//////////////////////////////////////////////////////////////////////////////

//static
class LLTextureFetchWorker : public LLWorkerClass
{
friend class LLTextureFetch;

private:
#if 0
	class URLResponder : public LLHTTPClient::Responder
	{
	public:
		URLResponder(LLTextureFetch* fetcher, const LLUUID& id)
			: mFetcher(fetcher), mID(id)
		{
		}
		~URLResponder()
		{
		}
		virtual void error(U32 status, const std::string& reason)
		{
			mFetcher->lockQueue();
			LLTextureFetchWorker* worker = mFetcher->getWorker(mID);
			if (worker)
			{
				llinfos << "LLTextureFetchWorker::URLResponder::error " << status << ": " << reason << llendl;
 				worker->callbackURLReceived(LLSD(), false);
			}
			mFetcher->unlockQueue();
		}
		virtual void result(const LLSD& content)
		{
			mFetcher->lockQueue();
			LLTextureFetchWorker* worker = mFetcher->getWorker(mID);
			if (worker)
			{
 				worker->callbackURLReceived(content, true);
			}
			mFetcher->unlockQueue();
		}
	private:
		LLTextureFetch* mFetcher;
		LLUUID mID;
	};

	class HTTPGetResponder : public LLHTTPClient::Responder
	{
	public:
		HTTPGetResponder(LLTextureFetch* fetcher, const LLUUID& id)
			: mFetcher(fetcher), mID(id)
		{
		}
		~HTTPGetResponder()
		{
		}
		virtual void completed(U32 status, const std::stringstream& content)
		{
			mFetcher->lockQueue();
			LLTextureFetchWorker* worker = mFetcher->getWorker(mID);
			if (worker)
			{
				const std::string& cstr = content.str();
				if (200 <= status &&  status < 300)
				{
					if (203 == status) // partial information (i.e. last block)
					{
						worker->callbackHttpGet((U8*)cstr.c_str(), cstr.length(), true);
					}
					else
					{
						worker->callbackHttpGet((U8*)cstr.c_str(), cstr.length(), false);
					}
				}
				else
				{
					llinfos << "LLTextureFetchWorker::HTTPGetResponder::error " << status << ": " << cstr << llendl;
					worker->callbackHttpGet(NULL, -1, true);
				}
			}
			mFetcher->unlockQueue();
		}
	private:
		LLTextureFetch* mFetcher;
		LLUUID mID;
	};
#endif
	
	class CacheReadResponder : public LLTextureCache::ReadResponder
	{
	public:
		CacheReadResponder(LLTextureFetch* fetcher, const LLUUID& id, LLImageFormatted* image)
			: mFetcher(fetcher), mID(id)
		{
			setImage(image);
		}
		virtual void completed(bool success)
		{
			mFetcher->lockQueue();
			LLTextureFetchWorker* worker = mFetcher->getWorker(mID);
			if (worker)
			{
 				worker->callbackCacheRead(success, mFormattedImage, mImageSize, mImageLocal);
			}
			mFetcher->unlockQueue();
		}
	private:
		LLTextureFetch* mFetcher;
		LLUUID mID;
	};

	class CacheWriteResponder : public LLTextureCache::WriteResponder
	{
	public:
		CacheWriteResponder(LLTextureFetch* fetcher, const LLUUID& id)
			: mFetcher(fetcher), mID(id)
		{
		}
		virtual void completed(bool success)
		{
			mFetcher->lockQueue();
			LLTextureFetchWorker* worker = mFetcher->getWorker(mID);
			if (worker)
			{
				worker->callbackCacheWrite(success);
			}
			mFetcher->unlockQueue();
		}
	private:
		LLTextureFetch* mFetcher;
		LLUUID mID;
	};
	
	class DecodeResponder : public LLResponder
	{
	public:
		DecodeResponder(LLTextureFetch* fetcher, const LLUUID& id, LLTextureFetchWorker* worker)
			: mFetcher(fetcher), mID(id), mWorker(worker)
		{
		}
		virtual void completed(bool success)
		{
			mFetcher->lockQueue();
			LLTextureFetchWorker* worker = mFetcher->getWorker(mID);
			if (worker)
			{
 				worker->callbackDecoded(success);
			}
			mFetcher->unlockQueue();
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
	/*virtual*/ bool doWork(S32 param); // Called from LLWorkerThread::processRequest()
	/*virtual*/ void finishWork(S32 param, bool completed); // called from finishRequest() (WORK THREAD)
	/*virtual*/ bool deleteOK(); // called from update() (WORK THREAD)

	~LLTextureFetchWorker();
	void relese() { --mActiveCount; }

protected:
	LLTextureFetchWorker(LLTextureFetch* fetcher, const LLUUID& id, const LLHost& host,
						 F32 priority, S32 discard, S32 size);

private:
	/*virtual*/ void startWork(S32 param); // called from addWork() (MAIN THREAD)
	/*virtual*/ void endWork(S32 param, bool aborted); // called from doWork() (MAIN THREAD)

	virtual LLString getName() { return LLString::null; }
	void resetFormattedData();
	
	void setImagePriority(F32 priority);
	void setDesiredDiscard(S32 discard, S32 size);
	bool insertPacket(S32 index, U8* data, S32 size);
	void clearPackets();
	U32 calcWorkPriority();
	void removeFromCache();
	bool processSimulatorPackets();
	bool decodeImage();
	bool writeToCacheComplete();
	
	void lockWorkData() { mWorkMutex.lock(); }
	void unlockWorkData() { mWorkMutex.unlock(); }

	void callbackURLReceived(const LLSD& data, bool success);
	void callbackHttpGet(U8* data, S32 data_size, bool last_block);
	void callbackCacheRead(bool success, LLImageFormatted* image,
						   S32 imagesize, BOOL islocal);
	void callbackCacheWrite(bool success);
	void callbackDecoded(bool success);
	
private:
	enum e_state // mState
	{
		// NOTE: Affects LLTextureBar::draw in lltextureview.cpp (debug hack)
		INVALID = 0,
		INIT,
		LOAD_FROM_TEXTURE_CACHE,
		CACHE_POST,
		LOAD_FROM_NETWORK,
		LOAD_FROM_SIMULATOR,
		LOAD_FROM_HTTP_GET_URL,
		LOAD_FROM_HTTP_GET_DATA,
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
		SENT_SIM = 2,
		SENT_URL = 3,
		SENT_HTTP = 4
	};
	static const char* sStateDescs[];
	e_state mState;
	LLTextureFetch* mFetcher;
	LLImageWorker* mImageWorker;
	LLPointer<LLImageFormatted> mFormattedImage;
	LLPointer<LLImageRaw> mRawImage;
	LLPointer<LLImageRaw> mAuxImage;
	LLUUID mID;
	LLHost mHost;
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
	U8* mBuffer;
	S32 mBufferSize;
	S32 mRequestedSize;
	S32 mDesiredSize;
	S32 mFileSize;
	S32 mCachedSize;
	BOOL mLoaded;
	e_request_state mSentRequest;
	BOOL mDecoded;
	BOOL mWritten;
	BOOL mNeedsAux;
	BOOL mHaveAllData;
	BOOL mInLocalCache;
	S32 mRetryAttempt;
	std::string mURL;
	S32 mActiveCount;

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
};

class LLTextureFetchLocalFileWorker : public LLTextureFetchWorker
{
friend class LLTextureFetch;

protected:
	LLTextureFetchLocalFileWorker(LLTextureFetch* fetcher, const LLString& filename, const LLUUID& id, const LLHost& host,
						 F32 priority, S32 discard, S32 size)
		:	LLTextureFetchWorker(fetcher, id, host, priority, discard, size),
			mFileName(filename)
	{}

private:
	/*virtual*/ LLString getName() { return mFileName; }


private:
	LLString mFileName;
};


//static
const char* LLTextureFetchWorker::sStateDescs[] = {
	"INVALID",
	"INIT",
	"LOAD_FROM_TEXTURE_CACHE",
	"CACHE_POST",
	"LOAD_FROM_NETWORK",
	"LOAD_FROM_SIMULATOR",
	"LOAD_FROM_HTTP_URL",
	"LOAD_FROM_HTTP_DATA",
	"DECODE_IMAGE",
	"DECODE_IMAGE_UPDATE",
	"WRITE_TO_CACHE",
	"WAIT_ON_WRITE",
	"DONE",
};

// called from MAIN THREAD

LLTextureFetchWorker::LLTextureFetchWorker(LLTextureFetch* fetcher,
										   const LLUUID& id,	// Image UUID
										   const LLHost& host,	// Simulator host
										   F32 priority,		// Priority
										   S32 discard,			// Desired discard
										   S32 size)			// Desired size
	: LLWorkerClass(fetcher, "TextureFetch"),
	  mState(INIT),
	  mFetcher(fetcher),
	  mImageWorker(NULL),
	  mID(id),
	  mHost(host),
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
	  mBuffer(NULL),
	  mBufferSize(0),
	  mRequestedSize(0),
	  mDesiredSize(FIRST_PACKET_SIZE),
	  mFileSize(0),
	  mCachedSize(0),
	  mLoaded(FALSE),
	  mSentRequest(UNSENT),
	  mDecoded(FALSE),
	  mWritten(FALSE),
	  mNeedsAux(FALSE),
	  mHaveAllData(FALSE),
	  mInLocalCache(FALSE),
	  mRetryAttempt(0),
	  mActiveCount(0),
	  mWorkMutex(fetcher->getWorkerAPRPool()),
	  mFirstPacket(0),
	  mLastPacket(-1),
	  mTotalPackets(0),
	  mImageCodec(IMG_CODEC_INVALID)
{
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
	lockWorkData();
	if (mCacheReadHandle != LLTextureCache::nullHandle())
	{
		mFetcher->mTextureCache->readComplete(mCacheReadHandle, true);
	}
	if (mCacheWriteHandle != LLTextureCache::nullHandle())
	{
		mFetcher->mTextureCache->writeComplete(mCacheWriteHandle, true);
	}
	if (mImageWorker)
	{
		mImageWorker->scheduleDelete();
	}
	mFormattedImage = NULL;
	clearPackets();
	unlockWorkData();
}

void LLTextureFetchWorker::clearPackets()
{
	for_each(mPackets.begin(), mPackets.end(), DeletePointer());
	mPackets.clear();
	mTotalPackets = 0;
	mLastPacket = -1;
	mFirstPacket = 0;
}

U32 LLTextureFetchWorker::calcWorkPriority()
{
// 	llassert_always(mImagePriority >= 0 && mImagePriority <= LLViewerImage::maxDecodePriority());
	F32 priority_scale = (F32)LLWorkerThread::PRIORITY_LOWBITS / LLViewerImage::maxDecodePriority();
	mWorkPriority = (U32)(mImagePriority * priority_scale);
	return mWorkPriority;
}

// mWorkMutex is locked
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
	if ((prioritize && mState == INIT) || mState == DONE)
	{
		mState = INIT;
		U32 work_priority = mWorkPriority | LLWorkerThread::PRIORITY_HIGH;
		setPriority(work_priority);
	}
}

void LLTextureFetchWorker::setImagePriority(F32 priority)
{
// 	llassert_always(priority >= 0 && priority <= LLViewerImage::maxDecodePriority());
	F32 delta = fabs(priority - mImagePriority);
	if (delta > (mImagePriority * .05f) || mState == DONE)
	{
		mImagePriority = priority;
		calcWorkPriority();
		U32 work_priority = mWorkPriority | (getPriority() & LLWorkerThread::PRIORITY_HIGHBITS);
		setPriority(work_priority);
	}
}

void LLTextureFetchWorker::resetFormattedData()
{
	delete[] mBuffer;
	mBuffer = NULL;
	mBufferSize = 0;
	if (mFormattedImage.notNull())
	{
		mFormattedImage->deleteData();
	}
	mHaveAllData = FALSE;
}

// Called from MAIN thread
void LLTextureFetchWorker::startWork(S32 param)
{
	llassert(mImageWorker == NULL);
	llassert(mFormattedImage.isNull());
}

#include "llviewerimagelist.h" // debug

// Called from LLWorkerThread::processRequest()
bool LLTextureFetchWorker::doWork(S32 param)
{
	LLMutexLock lock(&mWorkMutex);

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
		mRequestedDiscard = -1;
		mLoadedDiscard = -1;
		mDecodedDiscard = -1;
		mRequestedSize = 0;
		mFileSize = 0;
		mCachedSize = 0;
		mLoaded = FALSE;
		mSentRequest = UNSENT;
		mDecoded  = FALSE;
		mWritten  = FALSE;
		delete[] mBuffer;
		mBuffer = NULL;
		mBufferSize = 0;
		mHaveAllData = FALSE;
		clearPackets(); // TODO: Shouldn't be necessary
		mCacheReadHandle = LLTextureCache::nullHandle();
		mCacheWriteHandle = LLTextureCache::nullHandle();
		mURL.clear();
		mState = LOAD_FROM_TEXTURE_CACHE;
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
			setPriority(LLWorkerThread::PRIORITY_LOW | mWorkPriority); // Set priority first since Responder may change it

			CacheReadResponder* responder = new CacheReadResponder(mFetcher, mID, mFormattedImage);
			if (getName().empty())
			{
				mCacheReadHandle = mFetcher->mTextureCache->readFromCache(mID, cache_priority,
																		  offset, size, responder);
			}
			else
			{
				// read file from local disk
				mCacheReadHandle = mFetcher->mTextureCache->readFromCache(getName(), mID, cache_priority,
																		  offset, size, responder);
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
		mDesiredSize = llmax(mDesiredSize, FIRST_PACKET_SIZE);
		mCachedSize = mFormattedImage.notNull() ? mFormattedImage->getDataSize() : 0;
		// Successfully loaded
		if ((mCachedSize >= mDesiredSize) || mHaveAllData)
		{
			// we have enough data, decode it
			llassert_always(mFormattedImage->getDataSize() > 0);
			mState = DECODE_IMAGE;
			// fall through
		}
		else
		{
			if (!getName().empty())
			{
				// failed to load local file, we're done.
				return true;
			}
			// need more data
			mState = LOAD_FROM_NETWORK;
			// fall through
		}
	}

	if (mState == LOAD_FROM_NETWORK)
	{
		if (mSentRequest == UNSENT)
		{
			if (mFormattedImage.isNull())
			{
				mFormattedImage = new LLImageJ2C;
			}
			// Add this to the network queue and sit here.
			// LLTextureFetch::update() will send off a request which will change our state
			S32 data_size = mFormattedImage->getDataSize();
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
				else
				{
					mLastPacket = mFirstPacket-1;
					mTotalPackets = (mFileSize - FIRST_PACKET_SIZE + MAX_IMG_PACKET_SIZE-1) / MAX_IMG_PACKET_SIZE + 1;
				}
			}
			mRequestedSize = mDesiredSize;
			mRequestedDiscard = mDesiredDiscard;
			mSentRequest = QUEUED;
			mFetcher->lockQueue();
			mFetcher->addToNetworkQueue(this);
			mFetcher->unlockQueue();
			setPriority(LLWorkerThread::PRIORITY_LOW | mWorkPriority);
		}
		return false;
	}
	
	if (mState == LOAD_FROM_SIMULATOR)
	{
		if (processSimulatorPackets())
		{
			mFetcher->lockQueue();
			mFetcher->removeFromNetworkQueue(this);
			mFetcher->unlockQueue();
			if (!mFormattedImage->getDataSize())
			{
				// processSimulatorPackets() failed
				llwarns << "processSimulatorPackets() failed to load buffer" << llendl;
				return true; // failed
			}
			setPriority(LLWorkerThread::PRIORITY_HIGH | mWorkPriority);
			mState = DECODE_IMAGE;
		}
		else
		{
			setPriority(LLWorkerThread::PRIORITY_LOW | mWorkPriority);
		}
		return false;
	}
	
#if 0
	if (mState == LOAD_FROM_HTTP_GET_URL)
	{
		if (!mSentRequest)
		{
			mSentRequest = TRUE;
			mLoaded = FALSE;
			std::string url;
			LLViewerRegion* region = gAgent.getRegion();
			if (region)
			{
				url = region->getCapability("RequestTextureDownload");
			}
			if (!url.empty())
			{
				LLSD sd;
				sd = mID.asString();
				setPriority(LLWorkerThread::PRIORITY_LOW | mWorkPriority);
				LLHTTPClient::post(url, sd, new URLResponder(mFetcher, mID));
				return false;
			}
			else
			{
				llwarns << mID << ": HTTP get url failed, requesting from simulator" << llendl;
				mSentRequest = FALSE;
				mState = LOAD_FROM_SIMULATOR;
				return false;
			}
		}
		else
		{
			if (mLoaded)
			{
				if (!mURL.empty())
				{
					mState = LOAD_FROM_HTTP_GET_DATA;
					mSentRequest = FALSE; // reset
					mLoaded = FALSE; // reset
				}
				else
				{
					llwarns << mID << ": HTTP get url is empty, requesting from simulator" << llendl;
					mSentRequest = FALSE;
					mState = LOAD_FROM_SIMULATOR;
					return false;
				}
			}
		}
		// fall through
	}
	
	if (mState == LOAD_FROM_HTTP_GET_DATA)
	{
		if (!mSentRequest)
		{
			mSentRequest = TRUE;
			S32 cur_size = mFormattedImage->getDataSize(); // amount of data we already have
			mRequestedSize = mDesiredSize;
			mRequestedDiscard = mDesiredDiscard;
#if 1 // *TODO: LLCurl::getByteRange is broken (ignores range)
			cur_size = 0;
			mFormattedImage->deleteData();
#endif
			mRequestedSize -= cur_size;
			// 			  F32 priority = mImagePriority / (F32)LLViewerImage::maxDecodePriority(); // 0-1
			S32 offset = cur_size;
			mBufferSize = cur_size; // This will get modified by callbackHttpGet()
			std::string url;
			if (mURL.empty())
			{
				//url = "http://asset.agni/0000002f-38ae-0e17-8e72-712e58964e9c.texture";
				std::stringstream urlstr;
				urlstr << "http://asset.agni/" << mID.asString() << ".texture";
				url = urlstr.str();
			}
			else
			{
				url = mURL;
			}
			mLoaded = FALSE;
// 			llinfos << "HTTP GET: " << mID << " Offset: " << offset << " Bytes: " << mRequestedSize << llendl;
			setPriority(LLWorkerThread::PRIORITY_LOW | mWorkPriority);
			LLCurl::getByteRange(url, offset, mRequestedSize,
								 new HTTPGetResponder(mFetcher, mID)); // *TODO: use mWorkPriority
			return false; // not done
		}

		if (mLoaded)
		{
			S32 cur_size = mFormattedImage->getDataSize();
			if (mRequestedSize < 0)
			{
				llwarns << "http get failed for: " << mID << llendl;
				if (cur_size == 0)
				{
					resetFormattedData();
					return true; // failed
				}
				else
				{
					mState = DECODE_IMAGE;
					return false; // use what we have
				}
			}
			llassert_always(mBufferSize == cur_size + mRequestedSize);
			if (mHaveAllData)
			{
				mFileSize = mBufferSize;
			}
			U8* buffer = new U8[mBufferSize];
			if (cur_size > 0)
			{
				memcpy(buffer, mFormattedImage->getData(), cur_size);
			}
			memcpy(buffer + cur_size, mBuffer, mRequestedSize); // append
			// NOTE: setData releases current data and owns new data (buffer)
			mFormattedImage->setData(buffer, mBufferSize);
			// delete temp data
			delete[] mBuffer; // Note: not 'buffer' (assigned in setData())
			mBuffer = NULL;
			mBufferSize = 0;
			mLoadedDiscard = mRequestedDiscard;
			setPriority(LLWorkerThread::PRIORITY_HIGH | mWorkPriority);
			mState = DECODE_IMAGE;
			return false;
		}

		// NOTE: Priority gets updated when the http get completes (in callbackHTTPGet())
		setPriority(LLWorkerThread::PRIORITY_LOW | mWorkPriority);
		return false;
	}
#endif
	
	if (mState == DECODE_IMAGE)
	{
		llassert_always(mFormattedImage->getDataSize() > 0);
		setPriority(LLWorkerThread::PRIORITY_LOW | mWorkPriority); // Set priority first since Responder may change it
		mRawImage = NULL;
		mAuxImage = NULL;
		llassert_always(mImageWorker == NULL);
		llassert_always(mFormattedImage.notNull());
		S32 discard = mHaveAllData ? 0 : mLoadedDiscard;
		U32 image_priority = LLWorkerThread::PRIORITY_NORMAL | mWorkPriority;
		mDecoded  = FALSE;
		mState = DECODE_IMAGE_UPDATE;
		mImageWorker = new LLImageWorker(mFormattedImage, image_priority, discard, new DecodeResponder(mFetcher, mID, this));
		// fall though (need to call requestDecodedData() to start work)
	}
	
	if (mState == DECODE_IMAGE_UPDATE)
	{
		if (decodeImage())
		{
			if (mDecodedDiscard < 0)
			{
				if (mCachedSize > 0 && !mInLocalCache && mRetryAttempt == 0)
				{
					// Cache file should be deleted, try again
					llwarns << mID << ": Decode of cached file failed (removed), retrying" << llendl;
					mFormattedImage = NULL;
					++mRetryAttempt;
					setPriority(LLWorkerThread::PRIORITY_HIGH | mWorkPriority);
					mState = INIT;
					return false;
				}
				else
				{
					llwarns << "UNABLE TO LOAD TEXTURE: " << mID << " RETRIES: " << mRetryAttempt << llendl;
					mState = DONE; // failed
				}
			}
			else
			{
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
		if (mInLocalCache || !mFileSize || mSentRequest == UNSENT)
		{
			// If we're in a local cache or we didn't actually receive any new data, skip
			mState = DONE;
			return false;
		}
		S32 datasize = mFormattedImage->getDataSize();
		llassert_always(datasize);
		setPriority(LLWorkerThread::PRIORITY_LOW | mWorkPriority); // Set priority first since Responder may change it
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
		if (mDecodedDiscard >= 0 && mDesiredDiscard < mDecodedDiscard)
		{
			// More data was requested, return to INIT
			mState = INIT;
			setPriority(LLWorkerThread::PRIORITY_HIGH | mWorkPriority);
			return false;
		}
		else
		{
			setPriority(LLWorkerThread::PRIORITY_LOW | mWorkPriority);
			return true;
		}
	}
	
	return false;
}

// Called from MAIN thread
void LLTextureFetchWorker::endWork(S32 param, bool aborted)
{
	if (mImageWorker)
	{
		mImageWorker->scheduleDelete();
		mImageWorker = NULL;
	}
	mFormattedImage = NULL;
}

//////////////////////////////////////////////////////////////////////////////

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

// virtual
bool LLTextureFetchWorker::deleteOK()
{
	bool delete_ok = true;
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
		 ((mState >= LOAD_FROM_HTTP_GET_URL && mState <= LOAD_FROM_HTTP_GET_DATA) ||
		  (mState >= WRITE_TO_CACHE && mState <= WAIT_ON_WRITE))))
	{
		delete_ok = false;
	}
	
	return delete_ok;
}


void LLTextureFetchWorker::removeFromCache()
{
	if (!mInLocalCache)
	{
		mFetcher->mTextureCache->removeFromCache(mID);
	}
}


//////////////////////////////////////////////////////////////////////////////

bool LLTextureFetchWorker::processSimulatorPackets()
{
	if (mLastPacket >= mFirstPacket)
	{
		S32 buffer_size = mFormattedImage->getDataSize();
		for (S32 i = mFirstPacket; i<=mLastPacket; i++)
		{
			buffer_size += mPackets[i]->mSize;
		}
		bool have_all_data = mLastPacket >= mTotalPackets-1;
		llassert_always(mRequestedSize > 0);
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
				U8* buffer = new U8[buffer_size];
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

void LLTextureFetchWorker::callbackURLReceived(const LLSD& data, bool success)
{
#if 0
	LLMutexLock lock(&mWorkMutex);
	if (!mSentRequest || mState != LOAD_FROM_HTTP_GET_URL)
	{
		llwarns << "callbackURLReceived for unrequested fetch worker, req="
				<< mSentRequest << " state= " << mState << llendl;
		return;
	}
	if (success)
	{
		mURL = data.asString();
	}
	mLoaded = TRUE;
	setPriority(LLWorkerThread::PRIORITY_HIGH | mWorkPriority);
#endif
}

//////////////////////////////////////////////////////////////////////////////

void LLTextureFetchWorker::callbackHttpGet(U8* data, S32 data_size, bool last_block)
{
#if 0
	LLMutexLock lock(&mWorkMutex);
	if (!mSentRequest || mState != LOAD_FROM_HTTP_GET_DATA)
	{
		llwarns << "callbackHttpGet for unrequested fetch worker, req="
				<< mSentRequest << " state= " << mState << llendl;
		return;
	}
// 	llinfos << "HTTP RECEIVED: " << mID.asString() << " Bytes: " << data_size << llendl;
	if (mLoaded)
	{
		llwarns << "Duplicate callback for " << mID.asString() << llendl;
		return; // ignore duplicate callback
	}
	if (data_size >= 0)
	{
		if (data_size > 0)
		{
			mBuffer = new U8[data_size];
			// *TODO: set the formatted image data here
			memcpy(mBuffer, data, data_size);
			mBufferSize += data_size;
			if (data_size < mRequestedSize || last_block == true)
			{
				mHaveAllData = TRUE;
			}
			else if (data_size > mRequestedSize)
			{
				// *TODO: This will happen until we fix LLCurl::getByteRange()
				llinfos << "HUH?" << llendl;
				mHaveAllData = TRUE;
				mFormattedImage->deleteData();
				mBufferSize = data_size;
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
#endif
}

//////////////////////////////////////////////////////////////////////////////

void LLTextureFetchWorker::callbackCacheRead(bool success, LLImageFormatted* image,
											 S32 imagesize, BOOL islocal)
{
	LLMutexLock lock(&mWorkMutex);
	if (mState != LOAD_FROM_TEXTURE_CACHE)
	{
		llwarns << "Read callback for " << mID << " with state = " << mState << llendl;
		return;
	}
	if (success)
	{
		llassert_always(imagesize > 0);
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
}

void LLTextureFetchWorker::callbackCacheWrite(bool success)
{
	LLMutexLock lock(&mWorkMutex);
	if (mState != WAIT_ON_WRITE)
	{
		llwarns << "Write callback for " << mID << " with state = " << mState << llendl;
		return;
	}
	mWritten = TRUE;
	setPriority(LLWorkerThread::PRIORITY_HIGH | mWorkPriority);
}

//////////////////////////////////////////////////////////////////////////////

void LLTextureFetchWorker::callbackDecoded(bool success)
{
	if (mState != DECODE_IMAGE_UPDATE)
	{
		llwarns << "Decode callback for " << mID << " with state = " << mState << llendl;
		return;
	}
// 	llinfos << mID << " : DECODE COMPLETE " << llendl;
	setPriority(LLWorkerThread::PRIORITY_HIGH | mWorkPriority);
}

//////////////////////////////////////////////////////////////////////////////

bool LLTextureFetchWorker::decodeImage()
{
	llassert_always(mImageWorker);
	bool res = true;
	if (mRawImage.isNull())
	{
		res = false;
		if (mImageWorker->requestDecodedData(mRawImage, -1))
		{
			res = true;
// 			llinfos << mID << " : BASE DECODE FINISHED" << llendl;
		}
	}
	if (res &&
		(mRawImage.notNull() && mRawImage->getDataSize() > 0) &&
		(mNeedsAux && mAuxImage.isNull()))
	{
		res = false;
		if (mImageWorker->requestDecodedAuxData(mAuxImage, 4, -1))
		{
			res = true;
// 			llinfos << mID << " : AUX DECODE FINISHED" << llendl;
		}
	}
	if (res)
	{
		if ((mRawImage.notNull() && mRawImage->getDataSize() > 0) &&
			(!mNeedsAux || (mAuxImage.notNull() && mAuxImage->getDataSize() > 0)))
		{
			mDecodedDiscard = mFormattedImage->getDiscardLevel();
// 			llinfos << mID << " : DECODE FINISHED. DISCARD: " << mDecodedDiscard << llendl;
		}
		else
		{
			llwarns << "DECODE FAILED: " << mID << " Discard: " << (S32)mFormattedImage->getDiscardLevel() << llendl;
			removeFromCache();
		}
		mImageWorker->scheduleDelete();
		mImageWorker = NULL;
	}
	return res;
}

//////////////////////////////////////////////////////////////////////////////

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


//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
// public

LLTextureFetch::LLTextureFetch(LLTextureCache* cache, bool threaded)
	: LLWorkerThread("TextureFetch", threaded),
	  mDebugCount(0),
	  mDebugPause(FALSE),
	  mPacketCount(0),
	  mBadPacketCount(0),
	  mQueueMutex(getAPRPool()),
	  mTextureCache(cache)
{
}

LLTextureFetch::~LLTextureFetch()
{
	// ~LLQueuedThread() called here
}

bool LLTextureFetch::createRequest(const LLUUID& id, const LLHost& host, F32 priority,
									S32 w, S32 h, S32 c, S32 desired_discard, bool needs_aux)
{
	return createRequest(LLString::null, id, host, priority, w, h, c, desired_discard, needs_aux);
}

bool LLTextureFetch::createRequest(const LLString& filename, const LLUUID& id, const LLHost& host, F32 priority,
								   S32 w, S32 h, S32 c, S32 desired_discard, bool needs_aux)
{
	if (mDebugPause)
	{
		return false;
	}
	
	LLTextureFetchWorker* worker = NULL;
	LLMutexLock lock(&mQueueMutex);
	map_t::iterator iter = mRequestMap.find(id);
	if (iter != mRequestMap.end())
	{
		worker = iter->second;
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
	if (desired_discard == 0)
	{
		// if we want the entire image, and we know its size, then get it all
		// (calcDataSizeJ2C() below makes assumptions about how the image
		// was compressed - this code ensures that when we request the entire image,
		// we really do get it.)
		desired_size = MAX_IMAGE_DATA_SIZE;
	}
	else if (w*h*c > 0)
	{
		// If the requester knows the dimentions of the image,
		// this will calculate how much data we need without having to parse the header

		desired_size = LLImageJ2C::calcDataSizeJ2C(w, h, c, desired_discard);
	}
	else
	{
		desired_size = FIRST_PACKET_SIZE;
		desired_discard = MAX_DISCARD_LEVEL;
	}

	
	if (worker)
	{
		if (worker->wasAborted())
		{
			return false; // need to wait for previous aborted request to complete
		}
		worker->lockWorkData();
		worker->setImagePriority(priority);
		worker->setDesiredDiscard(desired_discard, desired_size);
		worker->unlockWorkData();
		if (!worker->haveWork())
		{
			worker->mState = LLTextureFetchWorker::INIT;
			worker->addWork(0, LLWorkerThread::PRIORITY_HIGH | worker->mWorkPriority);
		}
	}
	else
	{
		if (filename.empty())
		{
			// do remote fetch
			worker = new LLTextureFetchWorker(this, id, host, priority, desired_discard, desired_size);
		}
		else
		{
			// do local file fetch
			worker = new LLTextureFetchLocalFileWorker(this, filename, id, host, priority, desired_discard, desired_size);
		}
		mRequestMap[id] = worker;
	}
	worker->mActiveCount++;
	worker->mNeedsAux = needs_aux;
// 	llinfos << "REQUESTED: " << id << " Discard: " << desired_discard << llendl;
	return true;
}

void LLTextureFetch::deleteRequest(const LLUUID& id, bool cancel)
{
	LLMutexLock lock(&mQueueMutex);
	LLTextureFetchWorker* worker = getWorker(id);
	if (worker)
	{		
		removeRequest(worker, cancel);
	}
}

// protected

// call lockQueue() first!
void LLTextureFetch::addToNetworkQueue(LLTextureFetchWorker* worker)
{
	if (mRequestMap.find(worker->mID) != mRequestMap.end())
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
}

// call lockQueue() first!
void LLTextureFetch::removeFromNetworkQueue(LLTextureFetchWorker* worker)
{
	mNetworkQueue.erase(worker->mID);
}

// call lockQueue() first!
void LLTextureFetch::removeRequest(LLTextureFetchWorker* worker, bool cancel)
{
	mRequestMap.erase(worker->mID);
	size_t erased = mNetworkQueue.erase(worker->mID);
	if (cancel && erased > 0)
	{
		mCancelQueue[worker->mHost].insert(worker->mID);
	}
	worker->scheduleDelete();
}

// call lockQueue() first!
LLTextureFetchWorker* LLTextureFetch::getWorker(const LLUUID& id)
{
	LLTextureFetchWorker* res = NULL;
	map_t::iterator iter = mRequestMap.find(id);
	if (iter != mRequestMap.end())
	{
		res = iter->second;
	}
	return res;
}


bool LLTextureFetch::getRequestFinished(const LLUUID& id, S32& discard_level,
										LLPointer<LLImageRaw>& raw, LLPointer<LLImageRaw>& aux)
{
	bool res = false;
	LLMutexLock lock(&mQueueMutex);
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
			discard_level = worker->mDecodedDiscard;
			raw = worker->mRawImage; worker->mRawImage = NULL;
			aux = worker->mAuxImage; worker->mAuxImage = NULL;
			res = true;
		}
		else
		{
			worker->lockWorkData();
			if ((worker->mDecodedDiscard >= 0) &&
				(worker->mDecodedDiscard < discard_level || discard_level < 0) &&
				(worker->mState >= LLTextureFetchWorker::WAIT_ON_WRITE))
			{
				// Not finished, but data is ready
				discard_level = worker->mDecodedDiscard;
				if (worker->mRawImage) raw = worker->mRawImage;
				if (worker->mAuxImage) aux = worker->mAuxImage;
			}
			worker->unlockWorkData();
		}
	}
	else
	{
		res = true;
	}
	return res;
}

bool LLTextureFetch::updateRequestPriority(const LLUUID& id, F32 priority)
{
	bool res = false;
	LLMutexLock lock(&mQueueMutex);
	LLTextureFetchWorker* worker = getWorker(id);
	if (worker)
	{
		worker->lockWorkData();
		worker->setImagePriority(priority);
		worker->unlockWorkData();
		res = true;
	}
	return res;
}

//////////////////////////////////////////////////////////////////////////////

//virtual
S32 LLTextureFetch::update(U32 max_time_ms)
{
	S32 res;
	res = LLWorkerThread::update(max_time_ms);
	
	const F32 REQUEST_TIME = 1.f;

	// Periodically, gather the list of textures that need data from the network
	// And send the requests out to the simulators
	if (mNetworkTimer.getElapsedTimeF32() >= REQUEST_TIME)
	{
		mNetworkTimer.reset();
		sendRequestListToSimulators();
	}
	
	return res;
}

//////////////////////////////////////////////////////////////////////////////

void LLTextureFetch::sendRequestListToSimulators()
{
	const S32 IMAGES_PER_REQUEST = 50;
	const F32 LAZY_FLUSH_TIMEOUT = 15.f; // 10.0f // temp
	const F32 MIN_REQUEST_TIME = 1.0f;
	const F32 MIN_DELTA_PRIORITY = 1000.f;

	LLMutexLock lock(&mQueueMutex);
	
	// Send requests
	typedef std::set<LLTextureFetchWorker*,LLTextureFetchWorker::Compare> request_list_t;
	typedef std::map< LLHost, request_list_t > work_request_map_t;
	work_request_map_t requests;
	for (queue_t::iterator iter = mNetworkQueue.begin(); iter != mNetworkQueue.end(); )
	{
		queue_t::iterator curiter = iter++;
		LLTextureFetchWorker* req = getWorker(*curiter);
		if (!req)
		{
			mNetworkQueue.erase(curiter);
			continue; // paranoia
		}
		if (req->mID == mDebugID)
		{
			mDebugCount++; // for setting breakpoints
		}
		if (req->mTotalPackets > 0 && req->mLastPacket >= req->mTotalPackets-1)
		{
			// We have all the packets... make sure this is high priority
// 			req->setPriority(LLWorkerThread::PRIORITY_HIGH | req->mWorkPriority);
			continue;
		}
		F32 elapsed = req->mRequestedTimer.getElapsedTimeF32();
		F32 delta_priority = llabs(req->mRequestedPriority - req->mImagePriority);
		if ((req->mSimRequestedDiscard != req->mDesiredDiscard) ||
			(delta_priority > MIN_DELTA_PRIORITY && elapsed >= MIN_REQUEST_TIME) ||
			(elapsed >= LAZY_FLUSH_TIMEOUT))
		{
			requests[req->mHost].insert(req);
		}
	}

	std::string http_url;
#if 0
	if (gSavedSettings.getBOOL("ImagePipelineUseHTTP"))
	{
		LLViewerRegion* region = gAgent.getRegion();
		if (region)
		{
			http_url = region->getCapability("RequestTextureDownload");
		}
	}
#endif
	
	for (work_request_map_t::iterator iter1 = requests.begin();
		 iter1 != requests.end(); ++iter1)
	{
		bool use_http = http_url.empty() ? false : true;
		LLHost host = iter1->first;
		// invalid host = use agent host
		if (host == LLHost::invalid)
		{
			host = gAgent.getRegionHost();
		}
		else
		{
			use_http = false;
		}

		if (use_http)
		{
		}
		else
		{
			S32 request_count = 0;
			for (request_list_t::iterator iter2 = iter1->second.begin();
				 iter2 != iter1->second.end(); ++iter2)
			{
				LLTextureFetchWorker* req = *iter2;
				req->mSentRequest = LLTextureFetchWorker::SENT_SIM;
				if (0 == request_count)
				{
					gMessageSystem->newMessageFast(_PREHASH_RequestImage);
					gMessageSystem->nextBlockFast(_PREHASH_AgentData);
					gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
					gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
				}
				S32 packet = req->mLastPacket + 1;
				gMessageSystem->nextBlockFast(_PREHASH_RequestImage);
				gMessageSystem->addUUIDFast(_PREHASH_Image, req->mID);
				gMessageSystem->addS8Fast(_PREHASH_DiscardLevel, (S8)req->mSimRequestedDiscard);
				gMessageSystem->addF32Fast(_PREHASH_DownloadPriority, req->mImagePriority);
				gMessageSystem->addU32Fast(_PREHASH_Packet, packet);
				gMessageSystem->addU8Fast(_PREHASH_Type, req->mType);
// 				llinfos << "IMAGE REQUEST: " << req->mID << " Discard: " << req->mDesiredDiscard
// 						<< " Packet: " << packet << " Priority: " << req->mImagePriority << llendl;

				req->lockWorkData();
				req->mSimRequestedDiscard = req->mDesiredDiscard;
				req->mRequestedPriority = req->mImagePriority;
				req->mRequestedTimer.reset();
				req->unlockWorkData();
				request_count++;
				if (request_count >= IMAGES_PER_REQUEST)
				{
// 					llinfos << "REQUESTING " << request_count << " IMAGES FROM HOST: " << host.getIPString() << llendl;
					gMessageSystem->sendSemiReliable(host, NULL, NULL);
					request_count = 0;
				}
			}
			if (request_count > 0 && request_count < IMAGES_PER_REQUEST)
			{
// 				llinfos << "REQUESTING " << request_count << " IMAGES FROM HOST: " << host.getIPString() << llendl;
				gMessageSystem->sendSemiReliable(host, NULL, NULL);
				request_count = 0;
			}
		}
	}
	
	// Send cancelations
	if (!mCancelQueue.empty())
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
}

//////////////////////////////////////////////////////////////////////////////

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

bool LLTextureFetch::receiveImageHeader(const LLHost& host, const LLUUID& id, U8 codec, U16 packets, U32 totalbytes,
										U16 data_size, U8* data)
{
	LLMutexLock lock(&mQueueMutex);
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
		llwarns << "Img: " << id << ":" << " Empty Image Header" << llendl;
		res = false;
	}
	if (!res)
	{
		++mBadPacketCount;
		mCancelQueue[host].insert(id);
		return false;
	}

	worker->lockWorkData();

	//	Copy header data into image object
	worker->mImageCodec = codec;
	worker->mTotalPackets = packets;
	worker->mFileSize = (S32)totalbytes;	
	llassert_always(totalbytes > 0);
	llassert_always(data_size == FIRST_PACKET_SIZE || data_size == worker->mFileSize);
	res = worker->insertPacket(0, data, data_size);
	worker->setPriority(LLWorkerThread::PRIORITY_HIGH | worker->mWorkPriority);
	worker->mState = LLTextureFetchWorker::LOAD_FROM_SIMULATOR;
	worker->unlockWorkData();
	return res;
}

bool LLTextureFetch::receiveImagePacket(const LLHost& host, const LLUUID& id, U16 packet_num, U16 data_size, U8* data)
{
	LLMutexLock lock(&mQueueMutex);
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
		llwarns << "Img: " << id << ":" << " Empty Image Header" << llendl;
		res = false;
	}
	if (!res)
	{
		++mBadPacketCount;
		mCancelQueue[host].insert(id);
		return false;
	}

	worker->lockWorkData();
	
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
// 			<< " in state: " << LLTextureFetchWorker::sStateDescs[worker->mState] << llendl;
		removeFromNetworkQueue(worker); // failsafe
		mCancelQueue[host].insert(id);
	}
	
	worker->unlockWorkData();

	return res;
}

//////////////////////////////////////////////////////////////////////////////

S32 LLTextureFetch::getFetchState(const LLUUID& id, F32& data_progress_p, F32& requested_priority_p,
								  U32& fetch_priority_p, F32& fetch_dtime_p, F32& request_dtime_p)
{
	S32 state = LLTextureFetchWorker::INVALID;
	F32 data_progress = 0.0f;
	F32 requested_priority = 0.0f;
	F32 fetch_dtime = 999999.f;
	F32 request_dtime = 999999.f;
	U32 fetch_priority = 0;
	
	LLMutexLock lock(&mQueueMutex);
	LLTextureFetchWorker* worker = getWorker(id);
	if (worker && worker->haveWork())
	{
		worker->lockWorkData();
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
		if (state >= LLTextureFetchWorker::LOAD_FROM_NETWORK && state <= LLTextureFetchWorker::LOAD_FROM_HTTP_GET_DATA)
		{
			requested_priority = worker->mRequestedPriority;
		}
		else
		{
			requested_priority = worker->mImagePriority;
		}
		fetch_priority = worker->getPriority();
		worker->unlockWorkData();
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
}


//////////////////////////////////////////////////////////////////////////////
