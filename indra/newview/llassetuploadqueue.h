/** 
 * @file llassetuploadqueue.h
 * @brief Serializes asset upload request
 * Copyright (c) 2007, Linden Research, Inc.
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * $/LicenseInfo$
 */

#ifndef LL_LLASSETUPLOADQUEUE_H
#define LL_LLASSETUPLOADQUEUE_H

#include "lluuid.h"

#include <string>
#include <deque>

class LLAssetUploadQueueSupplier;

class LLAssetUploadQueue
{
public:

	// Takes ownership of supplier.
	LLAssetUploadQueue(LLAssetUploadQueueSupplier* supplier);
	virtual ~LLAssetUploadQueue();

	void queue(const std::string& filename,
			   const LLUUID& task_id,
			   const LLUUID& item_id,
			   BOOL is_running, 
			   BOOL is_target_mono, 
			   const LLUUID& queue_id,
			   U8* data,
			   U32 data_size,
			   std::string script_name);

	bool isEmpty() const {return mQueue.empty();}

private:

	friend class LLAssetUploadChainResponder;

	struct UploadData
	{
		std::string mFilename;
		LLUUID mTaskId;
		LLUUID mItemId;
		BOOL mIsRunning;
		BOOL mIsTargetMono;
		LLUUID mQueueId;
		U8* mData;
		U32 mDataSize;
		std::string mScriptName;
	};

	// Ownership of mSupplier passed to currently waiting responder
	// and returned to queue when no requests in progress.
	LLAssetUploadQueueSupplier* mSupplier;
	std::deque<UploadData> mQueue;

	// Passes on ownership of mSupplier if request is made.
	void request(LLAssetUploadQueueSupplier** supplier);
};

class LLAssetUploadQueueSupplier
{
public:
	virtual ~LLAssetUploadQueueSupplier();
	virtual LLAssetUploadQueue* get() const = 0;
	virtual void log(std::string message) const = 0;
};

#endif // LL_LLASSETUPLOADQUEUE_H
