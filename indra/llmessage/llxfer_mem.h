/** 
 * @file llxfer_mem.h
 * @brief definition of LLXfer_Mem class for a single xfer
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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
	std::string mRemoteFilename;
	ELLPath mRemotePath;
	BOOL mDeleteRemoteOnCompletion;

 public:

 private:
 protected:
 public:
	LLXfer_Mem ();
	virtual ~LLXfer_Mem();

	virtual void init();
	virtual void cleanup();

	virtual S32 startSend (U64 xfer_id, const LLHost &remote_host);
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





