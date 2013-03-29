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


#include "llfile.h"
#include "llviewerregion.h"
#include "llviewerobject.h"


// To do - something using region name or global position
#if LL_WINDOWS
	static const std::string STATS_FILE_NAME("C:\\ViewerObjectCacheStats.csv");
#else
	static const std::string STATS_FILE_NAME("/tmp/viewerstats.csv");
#endif

LLViewerStatsRecorder* LLViewerStatsRecorder::sInstance = NULL;
LLViewerStatsRecorder::LLViewerStatsRecorder() :
	mObjectCacheFile(NULL),
	mTimer(),
	mStartTime(0.0),
	mLastSnapshotTime(0.0)
{
	if (NULL != sInstance)
	{
		llerrs << "Attempted to create multiple instances of LLViewerStatsRecorder!" << llendl;
	}
	sInstance = this;
	clearStats();
}

LLViewerStatsRecorder::~LLViewerStatsRecorder()
{
	if (mObjectCacheFile != NULL)
	{
		// last chance snapshot
		writeToLog(0.f);
		LLFile::close(mObjectCacheFile);
		mObjectCacheFile = NULL;
	}
}

void LLViewerStatsRecorder::clearStats()
{
	mObjectCacheHitCount = 0;
	mObjectCacheHitSize = 0;
	mObjectCacheMissFullCount = 0;
	mObjectCacheMissFullSize = 0;
	mObjectCacheMissCrcCount = 0;
	mObjectCacheMissCrcSize = 0;
	mObjectFullUpdates = 0;
	mObjectFullUpdatesSize = 0;
	mObjectTerseUpdates = 0;
	mObjectTerseUpdatesSize = 0;
	mObjectCacheMissRequests = 0;
	mObjectCacheMissResponses = 0;
	mObjectCacheMissResponsesSize = 0;
	mObjectCacheUpdateDupes = 0;
	mObjectCacheUpdateChanges = 0;
	mObjectCacheUpdateAdds = 0;
	mObjectCacheUpdateReplacements = 0;
	mObjectUpdateFailures = 0;
	mObjectUpdateFailuresSize = 0;
	mTextureFetchSize = 0;
}


void LLViewerStatsRecorder::recordObjectUpdateFailure(U32 local_id, const EObjectUpdateType update_type, S32 msg_size)
{
	mObjectUpdateFailures++;
	mObjectUpdateFailuresSize += msg_size;
}

void LLViewerStatsRecorder::recordCacheMissEvent(U32 local_id, const EObjectUpdateType update_type, U8 cache_miss_type, S32 msg_size)
{
	if (LLViewerRegion::CACHE_MISS_TYPE_FULL == cache_miss_type)
	{
		mObjectCacheMissFullCount++;
		mObjectCacheMissFullSize += msg_size;
	}
	else
	{
		mObjectCacheMissCrcCount++;
		mObjectCacheMissCrcSize += msg_size;
	}
}

void LLViewerStatsRecorder::recordObjectUpdateEvent(U32 local_id, const EObjectUpdateType update_type, LLViewerObject * objectp, S32 msg_size)
{
	switch (update_type)
	{
	case OUT_FULL:
		mObjectFullUpdates++;
		mObjectFullUpdatesSize += msg_size;
		break;
	case OUT_TERSE_IMPROVED:
		mObjectTerseUpdates++;
		mObjectTerseUpdatesSize += msg_size;
		break;
	case OUT_FULL_COMPRESSED:
		mObjectCacheMissResponses++;
		mObjectCacheMissResponsesSize += msg_size;
		break;
	case OUT_FULL_CACHED:
		mObjectCacheHitCount++;
		mObjectCacheHitSize += msg_size;
		break;
	default:
		llwarns << "Unknown update_type" << llendl;
		break;
	};
}

void LLViewerStatsRecorder::recordCacheFullUpdate(U32 local_id, const EObjectUpdateType update_type, LLViewerRegion::eCacheUpdateResult update_result, LLViewerObject* objectp, S32 msg_size)
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
			llwarns << "Unknown update_result type" << llendl;
			break;
	};
}

void LLViewerStatsRecorder::recordRequestCacheMissesEvent(S32 count)
{
	mObjectCacheMissRequests += count;
}

void LLViewerStatsRecorder::writeToLog( F32 interval )
{
	F64 delta_time = LLTimer::getTotalSeconds() - mLastSnapshotTime;
	S32 total_objects = mObjectCacheHitCount + mObjectCacheMissCrcCount + mObjectCacheMissFullCount + mObjectFullUpdates + mObjectTerseUpdates + mObjectCacheMissRequests + mObjectCacheMissResponses + mObjectCacheUpdateDupes + mObjectCacheUpdateChanges + mObjectCacheUpdateAdds + mObjectCacheUpdateReplacements + mObjectUpdateFailures;

	if ( delta_time < interval || total_objects == 0) return;

	mLastSnapshotTime = LLTimer::getTotalSeconds();
	lldebugs << "ILX: " 
		<< mObjectCacheHitCount << " hits, " 
		<< mObjectCacheMissFullCount << " full misses, "
		<< mObjectCacheMissCrcCount << " crc misses, "
		<< mObjectFullUpdates << " full updates, "
		<< mObjectTerseUpdates << " terse updates, "
		<< mObjectCacheMissRequests << " cache miss requests, "
		<< mObjectCacheMissResponses << " cache miss responses, "
		<< mObjectCacheUpdateDupes << " cache update dupes, "
		<< mObjectCacheUpdateChanges << " cache update changes, "
		<< mObjectCacheUpdateAdds << " cache update adds, "
		<< mObjectCacheUpdateReplacements << " cache update replacements, "
		<< mObjectUpdateFailures << " update failures"
		<< llendl;

	if (mObjectCacheFile == NULL)
	{
		mStartTime = LLTimer::getTotalSeconds();
		mObjectCacheFile = LLFile::fopen(STATS_FILE_NAME, "wb");
		if (mObjectCacheFile)
		{	// Write column headers
			std::ostringstream data_msg;
			data_msg << "EventTime(ms)\t"
				<< "Cache Hits\t"
				<< "Cache Full Misses\t"
				<< "Cache Crc Misses\t"
				<< "Full Updates\t"
				<< "Terse Updates\t"
				<< "Cache Miss Requests\t"
				<< "Cache Miss Responses\t"
				<< "Cache Update Dupes\t"
				<< "Cache Update Changes\t"
				<< "Cache Update Adds\t"
				<< "Cache Update Replacements\t"
				<< "Update Failures\t"
				<< "Cache Hits bps\t"
				<< "Cache Full Misses bps\t"
				<< "Cache Crc Misses bps\t"
				<< "Full Updates bps\t"
				<< "Terse Updates bps\t"
				<< "Cache Miss Responses bps\t"
				<< "Texture Fetch bps\t"
				<< "\n";

			fwrite(data_msg.str().c_str(), 1, data_msg.str().size(), mObjectCacheFile );
		}
		else
		{
			llwarns << "Couldn't open " << STATS_FILE_NAME << " for logging." << llendl;
			return;
		}
	}

	std::ostringstream data_msg;

	data_msg << getTimeSinceStart()
		<< "\t " << mObjectCacheHitCount
		<< "\t" << mObjectCacheMissFullCount
		<< "\t" << mObjectCacheMissCrcCount
		<< "\t" << mObjectFullUpdates
		<< "\t" << mObjectTerseUpdates
		<< "\t" << mObjectCacheMissRequests
		<< "\t" << mObjectCacheMissResponses
		<< "\t" << mObjectCacheUpdateDupes
		<< "\t" << mObjectCacheUpdateChanges
		<< "\t" << mObjectCacheUpdateAdds
		<< "\t" << mObjectCacheUpdateReplacements
		<< "\t" << mObjectUpdateFailures
		<< "\t" << (mObjectCacheHitSize * 8 / delta_time)
		<< "\t" << (mObjectCacheMissFullSize * 8 / delta_time)
		<< "\t" << (mObjectCacheMissCrcSize * 8 / delta_time)
		<< "\t" << (mObjectFullUpdatesSize * 8 / delta_time)
		<< "\t" << (mObjectTerseUpdatesSize * 8 / delta_time)
		<< "\t" << (mObjectCacheMissResponsesSize * 8 / delta_time)
		<< "\t" << (mTextureFetchSize * 8 / delta_time)
		<< "\n";

	fwrite(data_msg.str().c_str(), 1, data_msg.str().size(), mObjectCacheFile );
	clearStats();
}

F32 LLViewerStatsRecorder::getTimeSinceStart()
{
	return (F32) (LLTimer::getTotalSeconds() - mStartTime);
}

void LLViewerStatsRecorder::recordTextureFetch( S32 msg_size )
{
	mTextureFetchSize += msg_size;
}



