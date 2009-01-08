/** 
 * @file llassetuploadqueue.h
 * @brief Serializes asset upload request
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
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
