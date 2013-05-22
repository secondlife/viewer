/** 
 * @file llattachmentsmgr.h
 * @brief Batches up attachment requests and sends them all
 * in one message.
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
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

#ifndef LL_LLATTACHMENTSMGR_H
#define LL_LLATTACHMENTSMGR_H

#include "llsingleton.h"

class LLViewerInventoryItem;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// LLAttachmentsMgr
// 
// The sole purpose of this class is to take attachment
// requests, queue them up, and send them all at once.
// This handles situations where the viewer may request
// a bunch of attachments at once in a short period of
// time, where each of the requests would normally be
// sent as a separate message versus being batched into
// one single message.
// 
// The intent of this batching is to reduce viewer->server
// traffic.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class LLAttachmentsMgr: public LLSingleton<LLAttachmentsMgr>
{
public:
	LLAttachmentsMgr();
	virtual ~LLAttachmentsMgr();

	void addAttachment(const LLUUID& item_id,
					   const U8 attachment_pt,
					   const BOOL add);
	static void onIdle(void *);
protected:
	void onIdle();
private:
	struct AttachmentsInfo
	{
		LLUUID mItemID;
		U8 mAttachmentPt;
		BOOL mAdd;
	};

	typedef std::vector<AttachmentsInfo> attachments_vec_t;
	attachments_vec_t mPendingAttachments;
};

#endif
