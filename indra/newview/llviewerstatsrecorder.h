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

#include "llframetimer.h"
#include "llviewerobject.h"
#include "llviewerregion.h"

class LLMutex;
class LLViewerObject;

class LLViewerStatsRecorder : public LLSingleton<LLViewerStatsRecorder>
{
	LLSINGLETON(LLViewerStatsRecorder);
	LOG_CLASS(LLViewerStatsRecorder);
	~LLViewerStatsRecorder();

 public:
	// Enable/disable stats recording.  This is broken down into two
	// flags so we can record stats without writing them to the log
	// file.  This is useful to analyzing updates for scene loading.
   void enableObjectStatsRecording(bool enable, bool logging = false);

	bool isEnabled() const { return mEnableStatsRecording; }
	bool isLogging() const { return mEnableStatsLogging; }

	void objectUpdateFailure(S32 msg_size)
    {
        if (mEnableStatsRecording)
        {
            recordObjectUpdateFailure(msg_size);
        }
	}

	void cacheMissEvent(U8 cache_miss_type)
	{
        if (mEnableStatsRecording)
        {
            recordCacheMissEvent(cache_miss_type);
        }
    }

	void cacheHitEvent()
    {
        if (mEnableStatsRecording)
        {
            recordCacheHitEvent();
        }
    }

    void objectUpdateEvent(const EObjectUpdateType update_type)
    {
		if (mEnableStatsRecording)
		{
			recordObjectUpdateEvent(update_type);
		}
	}

	void cacheFullUpdate(LLViewerRegion::eCacheUpdateResult update_result)
    {
		if (mEnableStatsRecording)
		{
			recordCacheFullUpdate(update_result);
		}
	}

	void requestCacheMissesEvent(S32 count)
	{
        if (mEnableStatsRecording)
        {
            recordRequestCacheMissesEvent(count);
        }
	}

	void textureFetch(S32 msg_size)
	{
        if (mEnableStatsRecording)
        {
            recordTextureFetch(msg_size);
        }
	}

	void idle()
	{
        writeToLog(mInterval);
	}

	F32 getTimeSinceStart();

private:
	void recordObjectUpdateFailure(S32 msg_size);
	void recordCacheMissEvent(U8 cache_miss_type);
	void recordCacheHitEvent();
	void recordObjectUpdateEvent(const EObjectUpdateType update_type);
	void recordCacheFullUpdate(LLViewerRegion::eCacheUpdateResult update_result);
	void recordRequestCacheMissesEvent(S32 count);
	void recordTextureFetch(S32 msg_size);
	void writeToLog(F32 interval);
    void closeStatsFile();
	void makeStatsFileName();

	static LLViewerStatsRecorder* sInstance;

	LLFILE *	mStatsFile;			// File to write data into
    std::string mStatsFileName;

	LLFrameTimer	mTimer;
	F64			mFileOpenTime;
	F64			mLastSnapshotTime;
    F32         mInterval;					// Interval between data log writes
    F32         mMaxDuration;				// Time limit on file

	bool        mEnableStatsRecording;		// Set to true to enable recording stats data
    bool		mEnableStatsLogging;		// Set true to write stats to log file
	bool		mSkipSaveIfZeros;			// Set true to skip saving stats if all values are zero	

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
	S32			mObjectUpdateFailuresSize;
	S32			mTextureFetchSize;

	void	clearStats();
};

#endif // LLVIEWERSTATSRECORDER_H
