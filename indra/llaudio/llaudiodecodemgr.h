/** 
 * @file llaudiodecodemgr.h
 *
 * Copyright (c) 2003-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLAUDIODECODEMGR_H
#define LL_LLAUDIODECODEMG_H

#include "stdtypes.h"

#include "lllinkedqueue.h"
#include "lluuid.h"

#include "llassettype.h"
#include "llframetimer.h"

class LLVFS;
class LLVorbisDecodeState;


class LLAudioDecodeMgr
{
public:
	LLAudioDecodeMgr();
	~LLAudioDecodeMgr();

	void processQueue(const F32 num_secs = 0.005);

	LLLinkedQueue<LLUUID> mDecodeQueue;

	LLVorbisDecodeState *mCurrentDecodep;

	BOOL addDecodeRequest(const LLUUID &uuid);
	void addAudioRequest(const LLUUID &uuid);

	S32 getRequestCount();
	
protected:
	LLLinkedQueue<LLUUID> mDownloadQueue;
	LLFrameTimer mCurrentTransferAge;
};

extern LLAudioDecodeMgr *gAudioDecodeMgrp;

#endif
