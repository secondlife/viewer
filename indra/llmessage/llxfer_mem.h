/** 
 * @file llxfer_mem.h
 * @brief definition of LLXfer_Mem class for a single xfer
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2007, Linden Research, Inc.
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

#ifndef LL_LLXFER_MEM_H
#define LL_LLXFER_MEM_H

#include "message.h"
#include "lltimer.h"
#include "llxfer.h"
#include "lldir.h"

class LLXfer_Mem : public LLXfer
{
 private:
 protected:
	void (*mCallback)(void *, S32, void **, S32, LLExtStat);	
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
								  void (*callback)(void*,S32,void**,S32,LLExtStat),
								  void** user_data);
	virtual S32 startDownload();

	virtual S32 processEOF();

	virtual U32 getXferTypeTag();
};

#endif





