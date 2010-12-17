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
	mRegionp(NULL),
	mStartTime(0.f),
	mProcessingTime(0.f)
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
		LLFile::close(mObjectCacheFile);
		mObjectCacheFile = NULL;
	}
}

// static
void LLViewerStatsRecorder::initClass()
{
	sInstance = new LLViewerStatsRecorder();
}

// static
void LLViewerStatsRecorder::cleanupClass()
{
	delete sInstance;
	sInstance = NULL;
}


void LLViewerStatsRecorder::initStatsRecorder(LLViewerRegion *regionp)
{
	if (mObjectCacheFile == NULL)
	{
		mStartTime = LLTimer::getTotalTime();
		mObjectCacheFile = LLFile::fopen(STATS_FILE_NAME, "wb");
		if (mObjectCacheFile)
		{	// Write column headers
			std::ostringstream data_msg;
			data_msg << "EventTime, "
				<< "ProcessingTime, "
				<< "CacheHits, "
				<< "CacheFullMisses, "
				<< "CacheCrcMisses, "
				<< "FullUpdates, "
				<< "TerseUpdates, "
				<< "CacheMissRequests, "
				<< "CacheMissResponses, "
				<< "CacheUpdateDupes, "
				<< "CacheUpdateChanges, "
				<< "CacheUpdateAdds, "
				<< "CacheUpdateReplacements, "
				<< "UpdateFailures"
				<< "\n";

			fwrite(data_msg.str().c_str(), 1, data_msg.str().size(), mObjectCacheFile );
		}
	}
}

void LLViewerStatsRecorder::beginObjectUpdateEvents(LLViewerRegion *regionp)
{
	initStatsRecorder(regionp);
	mRegionp = regionp;
	mProcessingTime = LLTimer::getTotalTime();
	clearStats();
}

void LLViewerStatsRecorder::clearStats()
{
	mObjectCacheHitCount = 0;
	mObjectCacheMissFullCount = 0;
	mObjectCacheMissCrcCount = 0;
	mObjectFullUpdates = 0;
	mObjectTerseUpdates = 0;
	mObjectCacheMissRequests = 0;
	mObjectCacheMissResponses = 0;
	mObjectCacheUpdateDupes = 0;
	mObjectCacheUpdateChanges = 0;
	mObjectCacheUpdateAdds = 0;
	mObjectCacheUpdateReplacements = 0;
	mObjectUpdateFailures = 0;
}


void LLViewerStatsRecorder::recordObjectUpdateFailure(U32 local_id, const EObjectUpdateType update_type)
{
	mObjectUpdateFailures++;
}

void LLViewerStatsRecorder::recordCacheMissEvent(U32 local_id, const EObjectUpdateType update_type, U8 cache_miss_type)
{
	if (LLViewerRegion::CACHE_MISS_TYPE_FULL == cache_miss_type)
	{
		mObjectCacheMissFullCount++;
	}
	else
	{
		mObjectCacheMissCrcCount++;
	}
}

void LLViewerStatsRecorder::recordObjectUpdateEvent(U32 local_id, const EObjectUpdateType update_type, LLViewerObject * objectp)
{
	switch (update_type)
	{
	case OUT_FULL:
		mObjectFullUpdates++;
		break;
	case OUT_TERSE_IMPROVED:
		mObjectTerseUpdates++;
		break;
	case OUT_FULL_COMPRESSED:
		mObjectCacheMissResponses++;
		break;
	case OUT_FULL_CACHED:
		mObjectCacheHitCount++;
		break;
	default:
		llwarns << "Unknown update_type" << llendl;
		break;
	};
}

void LLViewerStatsRecorder::recordCacheFullUpdate(U32 local_id, const EObjectUpdateType update_type, LLViewerRegion::eCacheUpdateResult update_result, LLViewerObject* objectp)
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

void LLViewerStatsRecorder::endObjectUpdateEvents()
{
	llinfos << "ILX: " 
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

	S32 total_objects = mObjectCacheHitCount + mObjectCacheMissCrcCount + mObjectCacheMissFullCount + mObjectFullUpdates + mObjectTerseUpdates + mObjectCacheMissRequests + mObjectCacheMissResponses + mObjectCacheUpdateDupes + mObjectCacheUpdateChanges + mObjectCacheUpdateAdds + mObjectCacheUpdateReplacements + mObjectUpdateFailures;
	if (mObjectCacheFile != NULL &&
		total_objects > 0)
	{
		std::ostringstream data_msg;
		F32 processing32 = (F32) ((LLTimer::getTotalTime() - mProcessingTime) / 1000.0);

		data_msg << getTimeSinceStart()
			<< ", " << processing32
			<< ", " << mObjectCacheHitCount
			<< ", " << mObjectCacheMissFullCount
			<< ", " << mObjectCacheMissCrcCount
			<< ", " << mObjectFullUpdates
			<< ", " << mObjectTerseUpdates
			<< ", " << mObjectCacheMissRequests
			<< ", " << mObjectCacheMissResponses
			<< ", " << mObjectCacheUpdateDupes
			<< ", " << mObjectCacheUpdateChanges
			<< ", " << mObjectCacheUpdateAdds
			<< ", " << mObjectCacheUpdateReplacements
			<< ", " << mObjectUpdateFailures
			<< "\n";

		fwrite(data_msg.str().c_str(), 1, data_msg.str().size(), mObjectCacheFile );
	}

	clearStats();
}

F32 LLViewerStatsRecorder::getTimeSinceStart()
{
	return (F32) ((LLTimer::getTotalTime() - mStartTime) / 1000.0);
}


