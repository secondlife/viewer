/** 
 * @file lltransfertargetfile.h
 * @brief Transfer system for receiving a file.
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLTRANSFERTARGETFILE_H
#define LL_LLTRANSFERTARGETFILE_H

#include "lltransfermanager.h"

#include <stdio.h>

typedef void (*LLTTFCompleteCallback)(const LLTSCode status, void *user_data);

class LLTransferTargetParamsFile : public LLTransferTargetParams
{
public:
	LLTransferTargetParamsFile() : LLTransferTargetParams(LLTTT_FILE) {}
	void setFilename(const char *filename)			{ mFilename = filename; }
	void setCallback(LLTTFCompleteCallback cb, void *user_data)		{ mCompleteCallback = cb; mUserData = user_data; }

	friend class LLTransferTargetFile;
protected:
	LLString				mFilename;
	LLTTFCompleteCallback	mCompleteCallback;
	void *					mUserData;
};


class LLTransferTargetFile : public LLTransferTarget
{
public:
	LLTransferTargetFile(const LLUUID &uuid);
	virtual ~LLTransferTargetFile();

	static void requestTransfer(LLTransferTargetChannel *channelp,
								const char *local_filename,
								const LLTransferSourceParams &source_params,
								LLTTFCompleteCallback callback);
protected:
	/*virtual*/ void applyParams(const LLTransferTargetParams &params);
	/*virtual*/ LLTSCode dataCallback(const S32 packet_id, U8 *in_datap, const S32 in_size);
	/*virtual*/ void completionCallback(const LLTSCode status);

	LLTransferTargetParamsFile mParams;

	FILE *mFP;
};

#endif // LL_LLTRANSFERTARGETFILE_H
