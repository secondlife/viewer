/** 
 * @file llxfer_vfile.h
 * @brief definition of LLXfer_VFile class for a single xfer_vfile.
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLXFER_VFILE_H
#define LL_LLXFER_VFILE_H

#include <stdio.h>

#include "llxfer.h"
#include "llassetstorage.h"

class LLVFS;
class LLVFile;

class LLXfer_VFile : public LLXfer
{
 protected:
	LLUUID mLocalID;
	LLUUID mRemoteID;
	LLUUID mTempID;
	LLAssetType::EType mType;
	
	LLVFile *mVFile;

	LLVFS *mVFS;

	char mName[MAX_STRING];		/* Flawfinder : ignore */

 public:
	LLXfer_VFile ();
	LLXfer_VFile (LLVFS *vfs, const LLUUID &local_id, LLAssetType::EType type);
	virtual ~LLXfer_VFile();

	virtual void init(LLVFS *vfs, const LLUUID &local_id, LLAssetType::EType type);
	virtual void free();

	virtual S32 initializeRequest(U64 xfer_id,
			LLVFS *vfs,
			const LLUUID &local_id,
			const LLUUID &remote_id,
			const LLAssetType::EType type,
			const LLHost &remote_host,
			void (*callback)(void **,S32,LLExtStat),
			void **user_data);
	virtual S32 startDownload();

	virtual S32 processEOF();
	
	virtual S32 startSend (U64 xfer_id, const LLHost &remote_host);

	virtual S32 suck(S32 start_position);
	virtual S32 flush();

	virtual BOOL matchesLocalFile(const LLUUID &id, LLAssetType::EType type);
	virtual BOOL matchesRemoteFile(const LLUUID &id, LLAssetType::EType type);

	virtual void setXferSize(S32 xfer_size);
	virtual S32  getMaxBufferSize();

	virtual U32 getXferTypeTag();

	virtual const char *getName();
};

#endif





