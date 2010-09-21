/** 
 * @file lltextureinfo.cpp
 * @brief Object which handles local texture info
 *
 * $LicenseInfo:firstyear=2000&license=viewergpl$
 * 
 * Copyright (c) 2000-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"

#include "lltextureinfo.h"
#include "lltexturestats.h"
#include "llviewercontrol.h"

LLTextureInfo::LLTextureInfo() : 
	mLogTextureDownloadsToViewerLog(false),
	mLogTextureDownloadsToSimulator(false),
	mTotalBytes(0),
	mTotalMilliseconds(0),
	mTextureDownloadsStarted(0),
	mTextureDownloadsCompleted(0),
	mTextureDownloadProtocol("NONE"),
	mTextureLogThreshold(100 * 1024),
	mCurrentStatsBundleStartTime(0)
{
	mTextures.clear();
}

void LLTextureInfo::setUpLogging(bool writeToViewerLog, bool sendToSim, U32 textureLogThreshold)
{
	mLogTextureDownloadsToViewerLog = writeToViewerLog;
	mLogTextureDownloadsToSimulator = sendToSim;
	mTextureLogThreshold = textureLogThreshold;
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
	mTextureDownloadsStarted++;
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

void LLTextureInfo::setRequestCompleteTimeAndLog(const LLUUID& id, U64 completeTime)
{
	if (!has(id))
	{
		addRequest(id);
	}
	mTextures[id]->mCompleteTime = completeTime;

	std::string protocol = "NONE";
	switch(mTextures[id]->mType)
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
		llinfos << "texture=" << id 
			<< " start=" << mTextures[id]->mStartTime 
			<< " end=" << mTextures[id]->mCompleteTime
			<< " size=" << mTextures[id]->mSize
			<< " offset=" << mTextures[id]->mOffset
			<< " length_in_ms=" << (mTextures[id]->mCompleteTime - mTextures[id]->mStartTime) / 1000
			<< " protocol=" << protocol
			<< llendl;
	}

	if(mLogTextureDownloadsToSimulator)
	{
		S32 texture_stats_upload_threshold = mTextureLogThreshold;
		mTotalBytes += mTextures[id]->mSize;
		mTotalMilliseconds += mTextures[id]->mCompleteTime - mTextures[id]->mStartTime;
		mTextureDownloadsCompleted++;
		mTextureDownloadProtocol = protocol;
		if (mTotalBytes >= texture_stats_upload_threshold)
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
	if(mTotalMilliseconds == 0)
	{
		averageDownloadRate = 0;
	}
	else
	{
		averageDownloadRate = (mTotalBytes * 8) / mTotalMilliseconds;
	}

	averagedTextureData["bits_per_second"] = averageDownloadRate;
	averagedTextureData["bytes_downloaded"] = mTotalBytes;
	averagedTextureData["texture_downloads_started"] = mTextureDownloadsStarted;
	averagedTextureData["texture_downloads_completed"] = mTextureDownloadsCompleted;
	averagedTextureData["transport"] = mTextureDownloadProtocol;

	return averagedTextureData;
}

void LLTextureInfo::resetTextureStatistics()
{
	mTotalMilliseconds = 0;
	mTotalBytes = 0;
	mTextureDownloadsStarted = 0;
	mTextureDownloadsCompleted = 0;
	mTextureDownloadProtocol = "NONE";
	mCurrentStatsBundleStartTime = LLTimer::getTotalTime();
}

U32 LLTextureInfo::getRequestStartTime(const LLUUID& id)
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

U32 LLTextureInfo::getRequestSize(const LLUUID& id)
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

U32 LLTextureInfo::getRequestCompleteTime(const LLUUID& id)
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

