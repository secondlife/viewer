/** 
 * @file llxfer_vfile.h
 * @brief definition of LLXfer_VFile class for a single xfer_vfile.
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

#ifndef LL_LLXFER_VFILE_H
#define LL_LLXFER_VFILE_H

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





