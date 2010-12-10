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

LLViewerStatsRecorder::LLViewerStatsRecorder() :
	mObjectCacheFile(NULL),
	mTimer()
{
	mStartTime = LLTimer::getTotalTime();
}

LLViewerStatsRecorder::~LLViewerStatsRecorder()
{
	LLFile::close(mObjectCacheFile);
	mObjectCacheFile = NULL;
}


void LLViewerStatsRecorder::initStatsRecorder(LLViewerRegion *regionp)
{
	// To do - something using region name or global position
#if LL_WINDOWS
	std::string stats_file_name("C:\\ViewerObjectCacheStats.csv");
#else
	std::string stats_file_name("/tmp/viewerstats.csv");
#endif

	if (mObjectCacheFile == NULL)
	{
		mObjectCacheFile = LLFile::fopen(stats_file_name, "wb");
		if (mObjectCacheFile)
		{	// Write column headers
			std::ostringstream data_msg;
			data_msg << "EventTime, "
				<< "ProcessingTime, "
				<< "CacheHits, "
				<< "CacheMisses, "
				<< "FullUpdates, "
				<< "TerseUpdates, "
				<< "CacheMissResponses, "
				<< "TotalUpdates "
				<< "\n";

			fwrite(data_msg.str().c_str(), 1, data_msg.str().size(), mObjectCacheFile );
		}
	}
}


void LLViewerStatsRecorder::initObjectUpdateEvents(LLViewerRegion *regionp)
{
	initStatsRecorder(regionp);
	mObjectCacheHitCount = 0;
	mObjectCacheMissCount = 0;
	mObjectFullUpdates = 0;
	mObjectTerseUpdates = 0;
	mObjectCacheMissResponses = 0;
	mProcessingTime = LLTimer::getTotalTime();
}


void LLViewerStatsRecorder::recordObjectUpdateEvent(LLViewerRegion *regionp, U32 local_id, const EObjectUpdateType update_type, BOOL success, LLViewerObject * objectp)
{
	if (!objectp)
	{
		// no object, must be a miss
		mObjectCacheMissCount++;
	}
	else
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
		default:
			mObjectCacheHitCount++;
			break;
		};
	}
}

void LLViewerStatsRecorder::closeObjectUpdateEvents(LLViewerRegion *regionp)
{
	llinfos << "ILX: " 
		<< mObjectCacheHitCount << " hits, " 
		<< mObjectCacheMissCount << " misses, "
		<< mObjectFullUpdates << " full updates, "
		<< mObjectTerseUpdates << " terse updates, "
		<< mObjectCacheMissResponses << " cache miss responses"
		<< llendl;

	S32 total_objects = mObjectCacheHitCount + mObjectCacheMissCount + mObjectFullUpdates + mObjectTerseUpdates + mObjectCacheMissResponses;;
	if (mObjectCacheFile != NULL &&
		total_objects > 0)
	{
		std::ostringstream data_msg;
		F32 now32 = (F32) ((LLTimer::getTotalTime() - mStartTime) / 1000.0);
		F32 processing32 = (F32) ((LLTimer::getTotalTime() - mProcessingTime) / 1000.0);

		data_msg << now32
			<< ", " << processing32
			<< ", " << mObjectCacheHitCount
			<< ", " << mObjectCacheMissCount
			<< ", " << mObjectFullUpdates
			<< ", " << mObjectTerseUpdates
			<< ", " << mObjectCacheMissResponses
			<< ", " << total_objects
			<< "\n";

		fwrite(data_msg.str().c_str(), 1, data_msg.str().size(), mObjectCacheFile );
	}

	mObjectCacheHitCount = 0;
	mObjectCacheMissCount = 0;
}



