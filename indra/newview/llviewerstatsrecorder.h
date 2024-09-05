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

class LLViewerStatsRecorder : public LLSimpleton<LLViewerStatsRecorder>
{
public:
    LLViewerStatsRecorder();
    LOG_CLASS(LLViewerStatsRecorder);
    ~LLViewerStatsRecorder();
    // Enable/disable stats recording.  This is broken down into two
    // flags so we can record stats without writing them to the log
    // file.  This is useful to analyzing updates for scene loading.
   void enableObjectStatsRecording(bool enable, bool logging = false);

    bool isEnabled() const { return mEnableStatsRecording; }
    bool isLogging() const { return mEnableStatsLogging; }

    void objectUpdateFailure()
    {
        if (mEnableStatsRecording)
        {
            mObjectUpdateFailures++;
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
            mObjectCacheHitCount++;
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
            mObjectCacheMissRequests += count;
        }
    }

    void textureFetch()
    {
        if (mEnableStatsRecording)
        {
            mTextureFetchCount += 1;
        }
    }

    void meshLoaded()
    {
        if (mEnableStatsRecording)
        {
            mMeshLoadedCount += 1;
        }
    }

    void recordObjectKills(S32 num_objects)
    {
        if (mEnableStatsRecording)
        {
            mObjectKills += num_objects;
        }
    }

    void idle()
    {
        writeToLog(mInterval);
    }

    F32 getTimeSinceStart();

private:
    void recordCacheMissEvent(U8 cache_miss_type);
    void recordObjectUpdateEvent(const EObjectUpdateType update_type);
    void recordCacheFullUpdate(LLViewerRegion::eCacheUpdateResult update_result);
    void writeToLog(F32 interval);
    void closeStatsFile();
    void makeStatsFileName();

    LLFILE *    mStatsFile;         // File to write data into
    std::string mStatsFileName;

    LLFrameTimer    mTimer;
    F64         mFileOpenTime;
    F64         mLastSnapshotTime;
    F32         mInterval;                  // Interval between data log writes
    F32         mMaxDuration;               // Time limit on file

    bool        mEnableStatsRecording;      // Set to true to enable recording stats data
    bool        mEnableStatsLogging;        // Set true to write stats to log file
    bool        mSkipSaveIfZeros;           // Set true to skip saving stats if all values are zero

    S32         mObjectCacheHitCount;
    S32         mObjectCacheMissFullCount;
    S32         mObjectCacheMissCrcCount;
    S32         mObjectFullUpdates;
    S32         mObjectTerseUpdates;
    S32         mObjectCacheMissRequests;
    S32         mObjectCacheUpdateDupes;
    S32         mObjectCacheUpdateChanges;
    S32         mObjectCacheUpdateAdds;
    S32         mObjectCacheUpdateReplacements;
    S32         mObjectUpdateFailures;
    S32         mTextureFetchCount;
    S32         mMeshLoadedCount;
    S32         mObjectKills;

    void    clearStats();
};

#endif // LLVIEWERSTATSRECORDER_H
