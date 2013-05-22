/** 
 * @file llvfile.h
 * @brief Definition of virtual file
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#ifndef LL_LLVFILE_H
#define LL_LLVFILE_H

#include "lluuid.h"
#include "llassettype.h"
#include "llvfs.h"
#include "llvfsthread.h"

class LLVFile
{
public:
	LLVFile(LLVFS *vfs, const LLUUID &file_id, const LLAssetType::EType file_type, S32 mode = LLVFile::READ);
	~LLVFile();

	BOOL read(U8 *buffer, S32 bytes, BOOL async = FALSE, F32 priority = 128.f);	/* Flawfinder: ignore */ 
	static U8* readFile(LLVFS *vfs, const LLUUID &uuid, LLAssetType::EType type, S32* bytes_read = 0);
	void setReadPriority(const F32 priority);
	BOOL isReadComplete();
	S32  getLastBytesRead();
	BOOL eof();

	BOOL write(const U8 *buffer, S32 bytes);
	static BOOL writeFile(const U8 *buffer, S32 bytes, LLVFS *vfs, const LLUUID &uuid, LLAssetType::EType type);
	BOOL seek(S32 offset, S32 origin = -1);
	S32  tell() const;

	S32 getSize();
	S32 getMaxSize();
	BOOL setMaxSize(S32 size);
	BOOL rename(const LLUUID &new_id, const LLAssetType::EType new_type);
	BOOL remove();

	bool isLocked(EVFSLock lock);
	void waitForLock(EVFSLock lock);
	
	static void initClass(LLVFSThread* vfsthread = NULL);
	static void cleanupClass();
	static LLVFSThread* getVFSThread() { return sVFSThread; }

protected:
	static LLVFSThread* sVFSThread;
	static BOOL sAllocdVFSThread;
	U32 threadPri() { return LLVFSThread::PRIORITY_NORMAL + llmin((U32)mPriority,(U32)0xfff); }
	
public:
	static const S32 READ;
	static const S32 WRITE;
	static const S32 READ_WRITE;
	static const S32 APPEND;
	
protected:
	LLAssetType::EType mFileType;

	LLUUID	mFileID;
	S32		mPosition;
	S32		mMode;
	LLVFS	*mVFS;
	F32		mPriority;

	S32		mBytesRead;
	LLVFSThread::handle_t mHandle;
};

#endif
