/** 
 * @file lltransfertargetfile.h
 * @brief Transfer system for receiving a file.
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2009, Linden Research, Inc.
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

#ifndef LL_LLTRANSFERTARGETFILE_H
#define LL_LLTRANSFERTARGETFILE_H

#include "lltransfermanager.h"

typedef void (*LLTTFCompleteCallback)(const LLTSCode status, void *user_data);

class LLTransferTargetParamsFile : public LLTransferTargetParams
{
public:
	LLTransferTargetParamsFile()
		: LLTransferTargetParams(LLTTT_FILE),

		mCompleteCallback(NULL),
		mUserData(NULL)
	{}
	void setFilename(const std::string& filename)	{ mFilename = filename; }
	void setCallback(LLTTFCompleteCallback cb, void *user_data)		{ mCompleteCallback = cb; mUserData = user_data; }

	friend class LLTransferTargetFile;
protected:
	std::string				mFilename;
	LLTTFCompleteCallback	mCompleteCallback;
	void *					mUserData;
};


class LLTransferTargetFile : public LLTransferTarget
{
public:
	LLTransferTargetFile(const LLUUID& uuid, LLTransferSourceType src_type);
	virtual ~LLTransferTargetFile();

	static void requestTransfer(LLTransferTargetChannel *channelp,
								const char *local_filename,
								const LLTransferSourceParams &source_params,
								LLTTFCompleteCallback callback);
protected:
	virtual bool unpackParams(LLDataPacker& dp);
	/*virtual*/ void applyParams(const LLTransferTargetParams &params);
	/*virtual*/ LLTSCode dataCallback(const S32 packet_id, U8 *in_datap, const S32 in_size);
	/*virtual*/ void completionCallback(const LLTSCode status);

	LLTransferTargetParamsFile mParams;

	LLFILE *mFP;
};

#endif // LL_LLTRANSFERTARGETFILE_H
