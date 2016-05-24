/**
 * @file llviewerstatsrecorder.h
 * @brief record info about viewer events to a metrics log file
 *
 * $LicenseInfo:firstyear=2010&license=viewerlgpl$
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

#ifndef LLVIEWERSTATSRECORDER_H
#define LLVIEWERSTATSRECORDER_H


// This is a diagnostic class used to record information from the viewer
// for analysis.

// This is normally 0.  Set to 1 to enable viewer stats recording
#define LL_RECORD_VIEWER_STATS	0


#include "llframetimer.h"
#include "llviewerobject.h"
#include "llviewerregion.h"

class LLMutex;
class LLViewerObject;

class LLViewerStatsRecorder : public LLSingleton<LLViewerStatsRecorder>
{
 public:
	LOG_CLASS(LLViewerStatsRecorder);	 
	LLViewerStatsRecorder();
	~LLViewerStatsRecorder();

	void objectUpdateFailure(U32 local_id, const EObjectUpdateType update_type, S32 msg_size)
	{
#if LL_RECORD_VIEWER_STATS
		recordObjectUpdateFailure(local_id, update_type, msg_size);
#endif
	}

	void cacheMissEvent(U32 local_id, const EObjectUpdateType update_type, U8 cache_miss_type, S32 msg_size)
	{
#if LL_RECORD_VIEWER_STATS
		recordCacheMissEvent(local_id, update_type, cache_miss_type, msg_size);
#endif
	}

	void objectUpdateEvent(U32 local_id, const EObjectUpdateType update_type, LLViewerObject * objectp, S32 msg_size)
	{
#if LL_RECORD_VIEWER_STATS
		recordObjectUpdateEvent(local_id, update_type, objectp, msg_size);
#endif
	}

	void cacheFullUpdate(U32 local_id, const EObjectUpdateType update_type, LLViewerRegion::eCacheUpdateResult update_result, LLViewerObject* objectp, S32 msg_size)
	{
#if LL_RECORD_VIEWER_STATS
		recordCacheFullUpdate(local_id, update_type, update_result, objectp, msg_size);
#endif
	}

	void requestCacheMissesEvent(S32 count)
	{
#if LL_RECORD_VIEWER_STATS
		recordRequestCacheMissesEvent(count);
#endif
	}

	void textureFetch(S32 msg_size)
	{
#if LL_RECORD_VIEWER_STATS
		recordTextureFetch(msg_size);
#endif
	}

	void log(F32 interval)
	{
#if LL_RECORD_VIEWER_STATS
		writeToLog(interval);
#endif
	}

	F32 getTimeSinceStart();

private:
	void recordObjectUpdateFailure(U32 local_id, const EObjectUpdateType update_type, S32 msg_size);
	void recordCacheMissEvent(U32 local_id, const EObjectUpdateType update_type, U8 cache_miss_type, S32 msg_size);
	void recordObjectUpdateEvent(U32 local_id, const EObjectUpdateType update_type, LLViewerObject * objectp, S32 msg_size);
	void recordCacheFullUpdate(U32 local_id, const EObjectUpdateType update_type, LLViewerRegion::eCacheUpdateResult update_result, LLViewerObject* objectp, S32 msg_size);
	void recordRequestCacheMissesEvent(S32 count);
	void recordTextureFetch(S32 msg_size);
	void writeToLog(F32 interval);

	static LLViewerStatsRecorder* sInstance;

	LLFILE *	mObjectCacheFile;		// File to write data into
	LLFrameTimer	mTimer;
	F64			mStartTime;
	F64			mLastSnapshotTime;

	S32			mObjectCacheHitCount;
	S32			mObjectCacheHitSize;
	S32			mObjectCacheMissFullCount;
	S32			mObjectCacheMissFullSize;
	S32			mObjectCacheMissCrcCount;
	S32			mObjectCacheMissCrcSize;
	S32			mObjectFullUpdates;
	S32			mObjectFullUpdatesSize;
	S32			mObjectTerseUpdates;
	S32			mObjectTerseUpdatesSize;
	S32			mObjectCacheMissRequests;
	S32			mObjectCacheMissResponses;
	S32			mObjectCacheMissResponsesSize;
	S32			mObjectCacheUpdateDupes;
	S32			mObjectCacheUpdateChanges;
	S32			mObjectCacheUpdateAdds;
	S32			mObjectCacheUpdateReplacements;
	S32			mObjectUpdateFailures;
	S32			mObjectUpdateFailuresSize;
	S32			mTextureFetchSize;


	void	clearStats();
};

#endif // LLVIEWERSTATSRECORDER_H

