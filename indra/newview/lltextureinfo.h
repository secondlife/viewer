/** 
 * @file lltextureinfo.h
 * @brief Object for managing texture information.
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

#ifndef LL_LLTEXTUREINFO_H
#define LL_LLTEXTUREINFO_H

#include "lluuid.h"
#include "lltextureinfodetails.h"
#include <map>

class LLTextureInfo
{
public:
	LLTextureInfo();
	~LLTextureInfo();

	void setUpLogging(bool writeToViewerLog, bool sendToSim, U32 textureLogThreshold);
	bool has(const LLUUID& id);
	void setRequestStartTime(const LLUUID& id, U64 startTime);
	void setRequestSize(const LLUUID& id, U32 size);
	void setRequestOffset(const LLUUID& id, U32 offset);
	void setRequestType(const LLUUID& id, LLTextureInfoDetails::LLRequestType type);
	void setRequestCompleteTimeAndLog(const LLUUID& id, U64 completeTime);
	U32 getRequestStartTime(const LLUUID& id);
	U32 getRequestSize(const LLUUID& id);
	U32 getRequestOffset(const LLUUID& id);
	LLTextureInfoDetails::LLRequestType getRequestType(const LLUUID& id);
	U32 getRequestCompleteTime(const LLUUID& id);
	void resetTextureStatistics();
	U32 getTextureInfoMapSize();
	LLSD getAverages();

private:
	void addRequest(const LLUUID& id);

	std::map<LLUUID, LLTextureInfoDetails *> mTextures;

	LLSD mAverages;

	bool mLogTextureDownloadsToViewerLog;
	bool mLogTextureDownloadsToSimulator;
	S32 mTotalBytes;
	S32 mTotalMilliseconds;
	S32 mTextureDownloadsStarted;
	S32 mTextureDownloadsCompleted;
	std::string mTextureDownloadProtocol;
	U32 mTextureLogThreshold; // in bytes
	U64 mCurrentStatsBundleStartTime;
};

#endif // LL_LLTEXTUREINFO_H
