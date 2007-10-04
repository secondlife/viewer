/** 
 * @file llxfer_file.h
 * @brief definition of LLXfer_File class for a single xfer_file.
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLXFER_FILE_H
#define LL_LLXFER_FILE_H

#include "llxfer.h"
#include "lldir.h"

class LLXfer_File : public LLXfer
{
 protected:
 	FILE *mFp;
	char mLocalFilename[LL_MAX_PATH];		/* Flawfinder : ignore */
	char mRemoteFilename[LL_MAX_PATH];		/* Flawfinder : ignore */
	ELLPath mRemotePath;
	char mTempFilename[LL_MAX_PATH];		/* Flawfinder : ignore */

	BOOL mDeleteLocalOnCompletion;
	BOOL mDeleteRemoteOnCompletion;

 public:
	LLXfer_File (S32 chunk_size);
	LLXfer_File (const LLString& local_filename, BOOL delete_local_on_completion, S32 chunk_size);
	virtual ~LLXfer_File();

	virtual void init(const LLString& local_filename, BOOL delete_local_on_completion, S32 chunk_size);
	virtual void free();

	virtual S32 initializeRequest(U64 xfer_id,
								  const LLString& local_filename,
								  const LLString& remote_filename,
								  ELLPath remote_path,
								  const LLHost& remote_host,
								  BOOL delete_remote_on_completion,
								  void (*callback)(void**,S32,LLExtStat),
								  void** user_data);
	virtual S32 startDownload();

	virtual S32 processEOF();
	
	virtual S32 startSend (U64 xfer_id, const LLHost &remote_host);

	virtual S32 suck(S32 start_position);
	virtual S32 flush();

	virtual BOOL matchesLocalFilename(const LLString& filename);
	virtual BOOL matchesRemoteFilename(const LLString& filename, ELLPath remote_path);

	virtual S32  getMaxBufferSize();

	virtual U32 getXferTypeTag();

	virtual const char *getName();
};

#endif





