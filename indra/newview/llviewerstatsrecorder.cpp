/**
 * @file llviewerstatsrecorder.cpp
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

#include "llviewerprecompiledheaders.h"
#include "llviewerstatsrecorder.h"

#include "llcontrol.h"
#include "llfile.h"
#include "llviewercontrol.h"
#include "llviewerregion.h"
#include "llviewerobject.h"
#include "llworld.h"

LLViewerStatsRecorder::LLViewerStatsRecorder() :
    mStatsFile(NULL),
    mTimer(),
    mFileOpenTime(0.0),
    mLastSnapshotTime(0.0),
    mEnableStatsRecording(false),
    mEnableStatsLogging(false),
    mInterval(0.2),
    mMaxDuration(300.f),
    mSkipSaveIfZeros(false)
{
    clearStats();
}

LLViewerStatsRecorder::~LLViewerStatsRecorder()
{
    if (mStatsFile)
    {
        writeToLog(0.f);  // Save last data
        closeStatsFile();
    }
}

void LLViewerStatsRecorder::clearStats()
{
    mObjectCacheHitCount = 0;
    mObjectCacheMissFullCount = 0;
    mObjectCacheMissCrcCount = 0;
    mObjectFullUpdates = 0;
    mObjectTerseUpdates = 0;
    mObjectCacheMissRequests = 0;
    mObjectCacheUpdateDupes = 0;
    mObjectCacheUpdateChanges = 0;
    mObjectCacheUpdateAdds = 0;
    mObjectCacheUpdateReplacements = 0;
    mObjectUpdateFailures = 0;
    mTextureFetchCount = 0;
    mMeshLoadedCount = 0;
    mObjectKills = 0;
}


void LLViewerStatsRecorder::enableObjectStatsRecording(bool enable, bool logging /* false */)
{
    mEnableStatsRecording = enable;

    // if logging is stopping, close the file
    if (mStatsFile && !logging)
    {
        writeToLog(0.f);  // Save last data
        closeStatsFile();
    }
    mEnableStatsLogging = logging;
}



void LLViewerStatsRecorder::recordCacheMissEvent(U8 cache_miss_type)
{
    if (LLViewerRegion::CACHE_MISS_TYPE_TOTAL == cache_miss_type)
    {
        mObjectCacheMissFullCount++;
    }
    else
    {
        mObjectCacheMissCrcCount++;
    }
}


void LLViewerStatsRecorder::recordObjectUpdateEvent(const EObjectUpdateType update_type)
{
    switch (update_type)
    {
    case OUT_FULL:
    case OUT_FULL_COMPRESSED:
            mObjectFullUpdates++;
        break;
    case OUT_TERSE_IMPROVED:
        mObjectTerseUpdates++;
        break;
    default:
        LL_WARNS() << "Unknown update_type" << LL_ENDL;
        break;
    };
}

void LLViewerStatsRecorder::recordCacheFullUpdate(LLViewerRegion::eCacheUpdateResult update_result)
{
    switch (update_result)
    {
        case LLViewerRegion::CACHE_UPDATE_DUPE:
            mObjectCacheUpdateDupes++;
            break;
        case LLViewerRegion::CACHE_UPDATE_CHANGED:
            mObjectCacheUpdateChanges++;
            break;
        case LLViewerRegion::CACHE_UPDATE_ADDED:
            mObjectCacheUpdateAdds++;
            break;
        case LLViewerRegion::CACHE_UPDATE_REPLACED:
            mObjectCacheUpdateReplacements++;
            break;
        default:
            LL_WARNS() << "Unknown update_result type " << (S32) update_result << LL_ENDL;
            break;
    };
}

void LLViewerStatsRecorder::writeToLog( F32 interval )
{
    if (!mEnableStatsLogging || !mEnableStatsRecording)
    {
        return;
    }

    size_t data_size = 0;
    F64 delta_time = LLFrameTimer::getTotalSeconds() - mLastSnapshotTime;
    if (delta_time < interval)
        return;

    if (mSkipSaveIfZeros)
    {
        S32 total_events = mObjectCacheHitCount + mObjectCacheMissCrcCount + mObjectCacheMissFullCount + mObjectFullUpdates +
                            mObjectTerseUpdates + mObjectCacheMissRequests + mObjectCacheUpdateDupes +
                            mObjectCacheUpdateChanges + mObjectCacheUpdateAdds + mObjectCacheUpdateReplacements + mObjectUpdateFailures;
        if (total_events == 0)
        {
            LL_DEBUGS("ILXZeroData") << "ILX: not saving zero data" << LL_ENDL;
            return;
        }
    }

    mLastSnapshotTime = LLFrameTimer::getTotalSeconds();
    LL_DEBUGS("ILX") << "ILX: "
        << mObjectCacheHitCount << " hits, "
        << mObjectCacheMissFullCount << " full misses, "
        << mObjectCacheMissCrcCount << " crc misses, "
        << mObjectFullUpdates << " full updates, "
        << mObjectTerseUpdates << " terse updates, "
        << mObjectCacheMissRequests << " cache miss requests, "
        << mObjectCacheUpdateDupes << " cache update dupes, "
        << mObjectCacheUpdateChanges << " cache update changes, "
        << mObjectCacheUpdateAdds << " cache update adds, "
        << mObjectCacheUpdateReplacements << " cache update replacements,"
        << mObjectUpdateFailures << " update failures,"
        << mTextureFetchCount << " texture fetches, "
        << mMeshLoadedCount << " mesh loads, "
        << mObjectKills << " object kills"
        << LL_ENDL;

    if (mStatsFile == NULL)
    {
        // Refresh settings
        mInterval        = gSavedSettings.getF32("StatsReportFileInterval");
        mSkipSaveIfZeros = gSavedSettings.getBOOL("StatsReportSkipZeroDataSaves");
        mMaxDuration     = gSavedSettings.getF32("StatsReportMaxDuration");

        // Open the data file
        makeStatsFileName();
        mStatsFile = LLFile::fopen(mStatsFileName, "wb");

        if (mStatsFile)
        {
            LL_INFOS("ILX") << "ILX: Writing update information to " << mStatsFileName << LL_ENDL;

            mFileOpenTime = LLFrameTimer::getTotalSeconds();

            // Write column headers
            std::ostringstream col_headers;
            col_headers << "Time (sec),"
                    << "Regions,"
                    << "Active Cached Objects,"
                    << "Cache Hits,"
                    << "Cache Full Misses,"
                    << "Cache Crc Misses,"
                    << "Full Updates,"
                    << "Terse Updates,"
                    << "Cache Miss Requests,"   // Normally results in a Full Update from simulator
                    << "Cache Update Dupes,"
                    << "Cache Update Changes,"
                    << "Cache Update Adds,"
                    << "Cache Update Replacements,"
                    << "Update Failures,"
                    << "Texture Count,"
                    << "Mesh Load Count,"
                    << "Object Kills"
                    << "\n";

            data_size = col_headers.str().size();
            if (fwrite(col_headers.str().c_str(), 1, data_size, mStatsFile ) != data_size)
            {
                LL_WARNS() << "failed to write full headers to " << mStatsFileName << LL_ENDL;
                // Close the file and turn off stats logging
                closeStatsFile();
                return;
            }
        }
        else
        {   // Failed to open file
            LL_WARNS() << "Couldn't open " << mStatsFileName << " for logging, turning off stats recording." << LL_ENDL;
            mEnableStatsLogging = false;
            return;
        }
    }

    std::ostringstream stats_data;

    stats_data << getTimeSinceStart()
        << "," << LLWorld::getInstance()->getRegionList().size()
        << "," << LLWorld::getInstance()->getNumOfActiveCachedObjects()
        << "," << mObjectCacheHitCount
        << "," << mObjectCacheMissFullCount
        << "," << mObjectCacheMissCrcCount
        << "," << mObjectFullUpdates
        << "," << mObjectTerseUpdates
        << "," << mObjectCacheMissRequests
        << "," << mObjectCacheUpdateDupes
        << "," << mObjectCacheUpdateChanges
        << "," << mObjectCacheUpdateAdds
        << "," << mObjectCacheUpdateReplacements
        << "," << mObjectUpdateFailures
        << "," << mTextureFetchCount
        << "," << mMeshLoadedCount
        << "," << mObjectKills
        << "\n";

    data_size = stats_data.str().size();
    if ( data_size != fwrite(stats_data.str().c_str(), 1, data_size, mStatsFile ))
    {
        LL_WARNS() << "Unable to write complete column data to " << mStatsFileName << LL_ENDL;
        closeStatsFile();
    }

    clearStats();

    if (getTimeSinceStart() >= mMaxDuration)
    {  // If file recording has been running for too long, stop it.
        closeStatsFile();
    }
}

void LLViewerStatsRecorder::closeStatsFile()
{
    if (mStatsFile)
    {
        LL_INFOS("ILX") << "ILX: Stopped writing update information to " << mStatsFileName << " after " << getTimeSinceStart()
                        << " seconds." << LL_ENDL;
        LLFile::close(mStatsFile);
        mStatsFile = NULL;
    }
    mEnableStatsLogging = false;
}

void LLViewerStatsRecorder::makeStatsFileName()
{
    // Create filename - tbd: use pid?
#if LL_WINDOWS
    std::string stats_file_name("SLViewerStats-");
#else
    std::string stats_file_name("slviewerstats-");
#endif

    F64         now      = LLFrameTimer::getTotalSeconds();
    std::string date_str = LLDate(now).asString();
    std::replace(date_str.begin(), date_str.end(), ':', '-');  // Make it valid for a filename
    stats_file_name.append(date_str);
    stats_file_name.append(".csv");
    mStatsFileName = gDirUtilp->getExpandedFilename(LL_PATH_LOGS, stats_file_name);
}

F32 LLViewerStatsRecorder::getTimeSinceStart()
{
    return (F32) (LLFrameTimer::getTotalSeconds() - mFileOpenTime);
}


