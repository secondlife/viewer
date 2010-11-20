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
	std::string stats_file_name("~/viewerstats.csv");
#endif

	if (mObjectCacheFile == NULL)
	{
		mObjectCacheFile = LLFile::fopen(stats_file_name, "wb");
		if (mObjectCacheFile)
		{	// Write column headers
			std::ostringstream data_msg;
			data_msg << "Time, "
				<< "Hits, "
				<< "Misses, "
				<< "Objects "
				<< "\n";

			fwrite(data_msg.str().c_str(), 1, data_msg.str().size(), mObjectCacheFile );
		}
	}
}


void LLViewerStatsRecorder::initCachedObjectUpdate(LLViewerRegion *regionp)
{
	mObjectCacheHitCount = 0;
	mObjectCacheMissCount = 0;
}


void LLViewerStatsRecorder::recordCachedObjectEvent(LLViewerRegion *regionp, U32 local_id, LLViewerObject * objectp)
{
	if (objectp)
	{
		mObjectCacheHitCount++;
	}
	else
	{	// no object, must be a miss
		mObjectCacheMissCount++;
	}
}

void LLViewerStatsRecorder::closeCachedObjectUpdate(LLViewerRegion *regionp)
{
	llinfos << "ILX: " << mObjectCacheHitCount 
		<< " hits " 
		<< mObjectCacheMissCount << " misses"
		<< llendl;

	S32 total_objects = mObjectCacheHitCount + mObjectCacheMissCount;
	if (mObjectCacheFile != NULL &&
		total_objects > 0)
	{
		std::ostringstream data_msg;
		F32 now32 = (F32) ((LLTimer::getTotalTime() - mStartTime) / 1000.0);

		data_msg << now32
			<< ", " << mObjectCacheHitCount
			<< ", " << mObjectCacheMissCount
			<< ", " << total_objects
			<< "\n";

		fwrite(data_msg.str().c_str(), 1, data_msg.str().size(), mObjectCacheFile );
	}

	mObjectCacheHitCount = 0;
	mObjectCacheMissCount = 0;
}



