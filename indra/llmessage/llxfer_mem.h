/** 
 * @file llxfer_mem.h
 * @brief definition of LLXfer_Mem class for a single xfer
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLXFER_MEM_H
#define LL_LLXFER_MEM_H

#include <stdio.h>

#include "message.h"
#include "lltimer.h"
#include "llxfer.h"
#include "lldir.h"

class LLXfer_Mem : public LLXfer
{
 private:
 protected:
	void (*mCallback)(void *, S32, void **,S32);	
	char mRemoteFilename[LL_MAX_PATH];		/* Flawfinder : ignore */
	ELLPath mRemotePath;
	BOOL mDeleteRemoteOnCompletion;

 public:

 private:
 protected:
 public:
	LLXfer_Mem ();
	virtual ~LLXfer_Mem();

	virtual void init();
	virtual void free();

	virtual S32 startSend (U64 xfer_id, const LLHost &remote_host);
	virtual U64 registerXfer(U64 xfer_id, const void *datap, const S32 length);
	virtual void setXferSize (S32 data_size);

	virtual S32 initializeRequest(U64 xfer_id,
								  const std::string& remote_filename,
								  ELLPath remote_path,
								  const LLHost& remote_host,
								  BOOL delete_remote_on_completion,
								  void (*callback)(void*,S32,void**,S32),
								  void** user_data);
	virtual S32 startDownload();

	virtual S32 processEOF();

	virtual U32 getXferTypeTag();
};

#endif




