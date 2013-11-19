/** 
 * @file llattachmentsmgr.cpp
 * @brief Manager for initiating attachments changes on the viewer
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

#include "llviewerprecompiledheaders.h"
#include "llattachmentsmgr.h"

#include "llagent.h"
#include "llinventorymodel.h"
#include "lltooldraganddrop.h" // pack_permissions_slam
#include "llviewerinventory.h"
#include "llviewerregion.h"
#include "message.h"


LLAttachmentsMgr::LLAttachmentsMgr()
{
}

LLAttachmentsMgr::~LLAttachmentsMgr()
{
}

void LLAttachmentsMgr::addAttachment(const LLUUID& item_id,
									 const U8 attachment_pt,
									 const BOOL add)
{
	AttachmentsInfo attachment;
	attachment.mItemID = item_id;
	attachment.mAttachmentPt = attachment_pt;
	attachment.mAdd = add;
	mPendingAttachments.push_back(attachment);
}

// static
void LLAttachmentsMgr::onIdle(void *)
{
	LLAttachmentsMgr::instance().onIdle();
}

void LLAttachmentsMgr::onIdle()
{
	// Make sure we got a region before trying anything else
	if( !gAgent.getRegion() )
	{
		return;
	}

	S32 obj_count = mPendingAttachments.size();
	if (obj_count == 0)
	{
		return;
	}
	
	// Limit number of packets to send
	const S32 MAX_PACKETS_TO_SEND = 10;
	const S32 OBJECTS_PER_PACKET = 4;
	const S32 MAX_OBJECTS_TO_SEND = MAX_PACKETS_TO_SEND * OBJECTS_PER_PACKET;
	if( obj_count > MAX_OBJECTS_TO_SEND )
	{
		obj_count = MAX_OBJECTS_TO_SEND;
	}

	LLUUID compound_msg_id;
	compound_msg_id.generate();
	LLMessageSystem* msg = gMessageSystem;

	
	S32 i = 0;
	for (attachments_vec_t::const_iterator iter = mPendingAttachments.begin();
		 iter != mPendingAttachments.end();
		 ++iter)
	{
		if( 0 == (i % OBJECTS_PER_PACKET) )
		{
			// Start a new message chunk
			msg->newMessageFast(_PREHASH_RezMultipleAttachmentsFromInv);
			msg->nextBlockFast(_PREHASH_AgentData);
			msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
			msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
			msg->nextBlockFast(_PREHASH_HeaderData);
			msg->addUUIDFast(_PREHASH_CompoundMsgID, compound_msg_id );
			msg->addU8Fast(_PREHASH_TotalObjects, obj_count );
			msg->addBOOLFast(_PREHASH_FirstDetachAll, false );
		}

		const AttachmentsInfo &attachment = (*iter);
		LLViewerInventoryItem* item = gInventory.getItem(attachment.mItemID);
		if (!item)
		{
			llinfos << "Attempted to add non-existant item ID:" << attachment.mItemID << llendl;
			continue;
		}
		S32 attachment_pt = attachment.mAttachmentPt;
		if (attachment.mAdd) 
			attachment_pt |= ATTACHMENT_ADD;

		msg->nextBlockFast(_PREHASH_ObjectData );
		msg->addUUIDFast(_PREHASH_ItemID, item->getLinkedUUID());
		msg->addUUIDFast(_PREHASH_OwnerID, item->getPermissions().getOwner());
		msg->addU8Fast(_PREHASH_AttachmentPt, attachment_pt);
		pack_permissions_slam(msg, item->getFlags(), item->getPermissions());
		msg->addStringFast(_PREHASH_Name, item->getName());
		msg->addStringFast(_PREHASH_Description, item->getDescription());

		if( (i+1 == obj_count) || ((OBJECTS_PER_PACKET-1) == (i % OBJECTS_PER_PACKET)) )
		{
			// End of message chunk
			msg->sendReliable( gAgent.getRegion()->getHost() );
		}
		i++;
	}

	mPendingAttachments.clear();
}
