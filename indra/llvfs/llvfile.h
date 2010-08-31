/** 
 * @file llvfile.h
 * @brief Definition of virtual file
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
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
