/** 
 * @file llassetupload.cpp
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

#include "llviewerprecompiledheaders.h"

#include "llassetuploadqueue.h"
#include "llviewerregion.h"
#include "llviewerobjectlist.h"

#include "llassetuploadresponders.h"
#include "llsd.h"
#include <iostream>

class LLAssetUploadChainResponder : public LLUpdateTaskInventoryResponder
{
public:
	
	LLAssetUploadChainResponder(const LLSD& post_data,
								const std::string& file_name,
								const LLUUID& queue_id,
								U8* data, 
								U32 data_size,
								std::string script_name,
								LLAssetUploadQueueSupplier *supplier) :
		LLUpdateTaskInventoryResponder(post_data, file_name, queue_id, LLAssetType::AT_LSL_TEXT),
		mSupplier(supplier),
		mData(data),
		mDataSize(data_size),
		mScriptName(script_name)
	{
 	}

	virtual ~LLAssetUploadChainResponder() 
   	{
   		if(mSupplier)
   		{
   			LLAssetUploadQueue *queue = mSupplier->get();
   			if (queue)
   			{
   				// Give ownership of supplier back to queue.
   				queue->mSupplier = mSupplier;
   				mSupplier = NULL;
   			}
   		}
   		delete mSupplier;
		delete mData;
   	}
	
	virtual void error(U32 statusNum, const std::string& reason)
   	{
		llwarns << "Error: " << reason << llendl;
		LLUpdateTaskInventoryResponder::error(statusNum, reason);
   		LLAssetUploadQueue *queue = mSupplier->get();
   		if (queue)
		{
   			queue->request(&mSupplier);
   		}
   	}

	virtual void result(const LLSD& content)
   	{
		LLUpdateTaskInventoryResponder::result(content);
   		LLAssetUploadQueue *queue = mSupplier->get();
   		if (queue)
   		{
   			// Responder is reused across 2 phase upload,
   			// so only start next upload after 2nd phase complete.
   			std::string state = content["state"];
   			if(state == "complete")
   			{
   				queue->request(&mSupplier);
   			}
   		}	
   	}
	
	virtual void uploadUpload(const LLSD& content)
	{
		std::string uploader = content["uploader"];

		mSupplier->log(std::string("Compiling " + mScriptName).c_str());
		llinfos << "Compiling " << llendl;

		// postRaw takes ownership of mData and will delete it.
		LLHTTPClient::postRaw(uploader, mData, mDataSize, this);
		mData = NULL;
		mDataSize = 0;
	}

	virtual void uploadComplete(const LLSD& content)
	{
		// Bytecode save completed
		if (content["compiled"])
		{
			mSupplier->log("Compilation succeeded");
			llinfos << "Compiled!" << llendl;
		}
		else
		{
			LLSD compile_errors = content["errors"];
			for(LLSD::array_const_iterator line	= compile_errors.beginArray();
				line < compile_errors.endArray(); line++)
			{
				mSupplier->log(line->asString());
				llinfos << content["errors"] << llendl;
			}
		}
		LLUpdateTaskInventoryResponder::uploadComplete(content);
	}

	LLAssetUploadQueueSupplier *mSupplier;
	U8* mData;
	U32 mDataSize;
	std::string mScriptName;
};


LLAssetUploadQueue::LLAssetUploadQueue(LLAssetUploadQueueSupplier *supplier) :
	mSupplier(supplier)
{
}

//virtual 
LLAssetUploadQueue::~LLAssetUploadQueue()
{
	delete mSupplier;
}

// Takes ownership of supplier.
void LLAssetUploadQueue::request(LLAssetUploadQueueSupplier** supplier)
{
	if (mQueue.empty())
		return;

	UploadData data = mQueue.front();
	mQueue.pop_front();
	
	LLSD body;
	body["task_id"] = data.mTaskId;
	body["item_id"] = data.mItemId;
	body["is_script_running"] = data.mIsRunning;
	body["target"] = data.mIsTargetMono? "mono" : "lsl2";

	std::string url = "";
	LLViewerObject* object = gObjectList.findObject(data.mTaskId);
	if (object)
	{
		url = object->getRegion()->getCapability("UpdateScriptTask");
		LLHTTPClient::post(url, body,
							new LLAssetUploadChainResponder(
								body, data.mFilename, data.mQueueId, 
								data.mData, data.mDataSize, data.mScriptName, *supplier));
	}

	*supplier = NULL;
}

void LLAssetUploadQueue::queue(const std::string& filename,
							   const LLUUID& task_id,
							   const LLUUID& item_id,
							   BOOL is_running, 
							   BOOL is_target_mono, 
							   const LLUUID& queue_id,
							   U8* script_data,
							   U32 data_size,
							   std::string script_name)
{
	UploadData data;
	data.mTaskId = task_id;
	data.mItemId = item_id;
	data.mIsRunning = is_running;
	data.mIsTargetMono = is_target_mono;
	data.mQueueId = queue_id;
	data.mFilename = filename;
	data.mData = script_data;
	data.mDataSize = data_size;
	data.mScriptName = script_name;
			
	mQueue.push_back(data);

	if(mSupplier)
	{
		request(&mSupplier);
	}	
}

LLAssetUploadQueueSupplier::~LLAssetUploadQueueSupplier()
{
}
