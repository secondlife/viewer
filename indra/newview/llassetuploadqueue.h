/** 
 * @file llassetuploadqueue.h
 * @brief Serializes asset upload request
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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
