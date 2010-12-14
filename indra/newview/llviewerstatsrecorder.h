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
#define LL_RECORD_VIEWER_STATS	1


#if LL_RECORD_VIEWER_STATS
#include "llframetimer.h"
#include "llviewerobject.h"
#include "llviewerregion.h"

class LLMutex;
class LLViewerRegion;
class LLViewerObject;

class LLViewerStatsRecorder
{
 public:
	LLViewerStatsRecorder();
	~LLViewerStatsRecorder();

	static void initClass();
	static void cleanupClass();
	static LLViewerStatsRecorder* instance() {return sInstance; }

	void initStatsRecorder(LLViewerRegion *regionp);

	void beginObjectUpdateEvents(LLViewerRegion *regionp);
	void recordObjectUpdateFailure(U32 local_id, const EObjectUpdateType update_type);
	void recordCacheMissEvent(U32 local_id, const EObjectUpdateType update_type, U8 cache_miss_type);
	void recordObjectUpdateEvent(U32 local_id, const EObjectUpdateType update_type, LLViewerObject * objectp);
	void recordCacheFullUpdate(U32 local_id, const EObjectUpdateType update_type, LLViewerRegion::eCacheUpdateResult update_result, LLViewerObject* objectp);
	void recordRequestCacheMissesEvent(S32 count);
	void endObjectUpdateEvents();

	F32 getTimeSinceStart();

private:
	static LLViewerStatsRecorder* sInstance;

	LLFILE *	mObjectCacheFile;		// File to write data into
	LLFrameTimer	mTimer;
	LLViewerRegion*	mRegionp;
	F64			mStartTime;
	F64			mProcessingTime;

	S32			mObjectCacheHitCount;
	S32			mObjectCacheMissFullCount;
	S32			mObjectCacheMissCrcCount;
	S32			mObjectFullUpdates;
	S32			mObjectTerseUpdates;
	S32			mObjectCacheMissRequests;
	S32			mObjectCacheMissResponses;
	S32			mObjectCacheUpdateDupes;
	S32			mObjectCacheUpdateChanges;
	S32			mObjectCacheUpdateAdds;
	S32			mObjectCacheUpdateReplacements;
	S32			mObjectUpdateFailures;


	void	clearStats();
};
#endif	// LL_RECORD_VIEWER_STATS

#endif // LLVIEWERSTATSRECORDER_H

