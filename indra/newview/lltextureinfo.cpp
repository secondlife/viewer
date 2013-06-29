/** 
 * @file lltextureinfo.cpp
 * @brief Object which handles local texture info
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
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

#include "lltextureinfo.h"
#include "lltexturestats.h"
#include "llviewercontrol.h"
#include "lltrace.h"

static LLTrace::CountStatHandle<S32> sTextureDownloadsStarted("texture_downloads_started", "number of texture downloads initiated");
static LLTrace::CountStatHandle<S32> sTextureDownloadsCompleted("texture_downloads_completed", "number of texture downloads completed");
static LLTrace::CountStatHandle<LLUnit<S32, LLUnits::Bytes> > sTextureDataDownloaded("texture_data_downloaded", "amount of texture data downloaded");
static LLTrace::CountStatHandle<LLUnit<U32, LLUnits::Milliseconds> > sTexureDownloadTime("texture_download_time", "amount of time spent fetching textures");

LLTextureInfo::LLTextureInfo() : 
	mLogTextureDownloadsToViewerLog(false),
	mLogTextureDownloadsToSimulator(false),
	mTextureDownloadProtocol("NONE"),
	mTextureLogThreshold(LLUnits::Kibibytes::fromValue(100))
{
	mTextures.clear();
	mRecording.start();
}

void LLTextureInfo::setUpLogging(bool writeToViewerLog, bool sendToSim, LLUnit<U32, LLUnits::Bytes> textureLogThreshold)
{
	mLogTextureDownloadsToViewerLog = writeToViewerLog;
	mLogTextureDownloadsToSimulator = sendToSim;
	mTextureLogThreshold = LLUnit<U32, LLUnits::Bytes>(textureLogThreshold);
}

LLTextureInfo::~LLTextureInfo()
{
	std::map<LLUUID, LLTextureInfoDetails *>::iterator iterator;
	for (iterator = mTextures.begin(); iterator != mTextures.end(); iterator++)
	{
		LLTextureInfoDetails *info = (*iterator).second;
		delete info;
	}

	mTextures.clear();
}

void LLTextureInfo::addRequest(const LLUUID& id)
{
	LLTextureInfoDetails *info = new LLTextureInfoDetails();
	mTextures[id] = info;
}

U32 LLTextureInfo::getTextureInfoMapSize()
{
	return mTextures.size();
}

bool LLTextureInfo::has(const LLUUID& id)
{
	std::map<LLUUID, LLTextureInfoDetails *>::iterator iterator = mTextures.find(id);
	if (iterator == mTextures.end())
	{
		return false;
	}
	else
	{
		return true;
	}
}

void LLTextureInfo::setRequestStartTime(const LLUUID& id, U64 startTime)
{
	if (!has(id))
	{
		addRequest(id);
	}
	mTextures[id]->mStartTime = startTime;
	add(sTextureDownloadsStarted, 1);
}

void LLTextureInfo::setRequestSize(const LLUUID& id, U32 size)
{
	if (!has(id))
	{
		addRequest(id);
	}
	mTextures[id]->mSize = size;
}

void LLTextureInfo::setRequestOffset(const LLUUID& id, U32 offset)
{
	if (!has(id))
	{
		addRequest(id);
	}
	mTextures[id]->mOffset = offset;
}

void LLTextureInfo::setRequestType(const LLUUID& id, LLTextureInfoDetails::LLRequestType type)
{
	if (!has(id))
	{
		addRequest(id);
	}
	mTextures[id]->mType = type;
}

void LLTextureInfo::setRequestCompleteTimeAndLog(const LLUUID& id, LLUnit<U64, LLUnits::Microseconds> completeTime)
{
	if (!has(id))
	{
		addRequest(id);
	}
	
	LLTextureInfoDetails& details = *mTextures[id];

	details.mCompleteTime = completeTime;

	std::string protocol = "NONE";
	switch(details.mType)
	{
	case LLTextureInfoDetails::REQUEST_TYPE_HTTP:
		protocol = "HTTP";
		break;

	case LLTextureInfoDetails::REQUEST_TYPE_UDP:
		protocol = "UDP";
		break;

	case LLTextureInfoDetails::REQUEST_TYPE_NONE:
	default:
		break;
	}

	if (mLogTextureDownloadsToViewerLog)
	{
		llinfos << "texture="   << id 
			    << " start="    << details.mStartTime 
			    << " end="      << details.mCompleteTime
			    << " size="     << details.mSize
			    << " offset="   << details.mOffset
			    << " length="   << LLUnit<U32, LLUnits::Milliseconds>(details.mCompleteTime - details.mStartTime)
			    << " protocol=" << protocol
			    << llendl;
	}

	if(mLogTextureDownloadsToSimulator)
	{
		add(sTextureDataDownloaded, details.mSize);
		add(sTexureDownloadTime, details.mCompleteTime - details.mStartTime);
		add(sTextureDownloadsCompleted, 1);
		mTextureDownloadProtocol = protocol;
		if (mRecording.getSum(sTextureDataDownloaded) >= mTextureLogThreshold)
		{
			LLSD texture_data;
			std::stringstream startTime;
			startTime << mCurrentStatsBundleStartTime;
			texture_data["start_time"] = startTime.str();
			std::stringstream endTime;
			endTime << completeTime;
			texture_data["end_time"] = endTime.str();
			texture_data["averages"] = getAverages();
			send_texture_stats_to_sim(texture_data);
			resetTextureStatistics();
		}
	}

	mTextures.erase(id);
}

LLSD LLTextureInfo::getAverages()
{
	LLSD averagedTextureData;
	S32 averageDownloadRate;
	LLUnit<U32, LLUnits::Milliseconds> download_time = mRecording.getSum(sTexureDownloadTime);
	if(download_time == 0)
	{
		averageDownloadRate = 0;
	}
	else
	{
		averageDownloadRate = mRecording.getSum(sTextureDataDownloaded).valueAs<LLUnits::Bits>() / download_time.valueAs<LLUnits::Seconds>();
	}

	averagedTextureData["bits_per_second"]             = averageDownloadRate;
	averagedTextureData["bytes_downloaded"]            = mRecording.getSum(sTextureDataDownloaded).valueAs<LLUnits::Bytes>();
	averagedTextureData["texture_downloads_started"]   = mRecording.getSum(sTextureDownloadsStarted);
	averagedTextureData["texture_downloads_completed"] = mRecording.getSum(sTextureDownloadsCompleted);
	averagedTextureData["transport"]                   = mTextureDownloadProtocol;

	return averagedTextureData;
}

void LLTextureInfo::resetTextureStatistics()
{
	mRecording.restart();
	mTextureDownloadProtocol = "NONE";
	mCurrentStatsBundleStartTime = LLTimer::getTotalTime();
}

LLUnit<U32, LLUnits::Microseconds> LLTextureInfo::getRequestStartTime(const LLUUID& id)
{
	if (!has(id))
	{
		return 0;
	}
	else
	{
		std::map<LLUUID, LLTextureInfoDetails *>::iterator iterator = mTextures.find(id);
		return (*iterator).second->mStartTime;
	}
}

LLUnit<U32, LLUnits::Bytes> LLTextureInfo::getRequestSize(const LLUUID& id)
{
	if (!has(id))
	{
		return 0;
	}
	else
	{
		std::map<LLUUID, LLTextureInfoDetails *>::iterator iterator = mTextures.find(id);
		return (*iterator).second->mSize;
	}
}

U32 LLTextureInfo::getRequestOffset(const LLUUID& id)
{
	if (!has(id))
	{
		return 0;
	}
	else
	{
		std::map<LLUUID, LLTextureInfoDetails *>::iterator iterator = mTextures.find(id);
		return (*iterator).second->mOffset;
	}
}

LLTextureInfoDetails::LLRequestType LLTextureInfo::getRequestType(const LLUUID& id)
{
	if (!has(id))
	{
		return LLTextureInfoDetails::REQUEST_TYPE_NONE;
	}
	else
	{
		std::map<LLUUID, LLTextureInfoDetails *>::iterator iterator = mTextures.find(id);
		return (*iterator).second->mType;
	}
}

LLUnit<U32, LLUnits::Microseconds> LLTextureInfo::getRequestCompleteTime(const LLUUID& id)
{
	if (!has(id))
	{
		return 0;
	}
	else
	{
		std::map<LLUUID, LLTextureInfoDetails *>::iterator iterator = mTextures.find(id);
		return (*iterator).second->mCompleteTime;
	}
}

