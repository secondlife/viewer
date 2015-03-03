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
#include "llappearancemgr.h"
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
	LLViewerInventoryItem *item = gInventory.getItem(item_id);
	LL_DEBUGS("Avatar") << "ATT adding attachment to mPendingAttachments "
						<< (item ? item->getName() : "UNKNOWN") << " id " << item_id << LL_ENDL;

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

	linkPendingAttachments();
}

class LLAttachAfterLinkCallback: public LLInventoryCallback
{
public:
	LLAttachAfterLinkCallback(const LLAttachmentsMgr::attachments_vec_t& to_link_and_attach):
		mToLinkAndAttach(to_link_and_attach)
	{
	}

	~LLAttachAfterLinkCallback()
	{
		LL_DEBUGS("Avatar") << "destructor" << LL_ENDL; 
		for (LLAttachmentsMgr::attachments_vec_t::const_iterator it = mToLinkAndAttach.begin();
			 it != mToLinkAndAttach.end(); ++it)
		{
			const LLAttachmentsMgr::AttachmentsInfo& att_info = *it;
			if (!LLAppearanceMgr::instance().isLinkedInCOF(att_info.mItemID))
			{
				LLViewerInventoryItem *item = gInventory.getItem(att_info.mItemID);
				LL_WARNS() << "ATT COF link creation failed for att item " << (item ? item->getName() : "UNKNOWN") << " id "
						   << att_info.mItemID << LL_ENDL;
			}
		}
		LLAppearanceMgr::instance().requestServerAppearanceUpdate();
		LLAttachmentsMgr::instance().requestAttachments(mToLinkAndAttach);
	}

	/* virtual */ void fire(const LLUUID& inv_item)
	{
		LL_DEBUGS("Avatar") << inv_item << LL_ENDL;
	}

private:
	LLAttachmentsMgr::attachments_vec_t mToLinkAndAttach;
};

//#define COF_LINK_FIRST

void LLAttachmentsMgr::linkPendingAttachments()
{
	if (mPendingAttachments.size())
	{
#ifdef COF_LINK_FIRST
		LLPointer<LLInventoryCallback> cb = new LLAttachAfterLinkCallback(mPendingAttachments);
		LLInventoryObject::const_object_list_t inv_items_to_link;
		LL_DEBUGS("Avatar") << "ATT requesting COF links for " << mPendingAttachments.size() << " object(s):" << LL_ENDL;
		for (attachments_vec_t::const_iterator it = mPendingAttachments.begin();
			 it != mPendingAttachments.end(); ++it)
		{
			const AttachmentsInfo& att_info = *it;
			LLViewerInventoryItem *item = gInventory.getItem(att_info.mItemID);
			if (item)
			{
				LL_DEBUGS("Avatar") << "ATT - requesting COF link for " << item->getName() << LL_ENDL;
				inv_items_to_link.push_back(item);
			}
			else
			{
				LL_WARNS() << "ATT unable to link requested attachment " << att_info.mItemID
						   << ", item not found in inventory" << LL_ENDL;
			}
		}
		link_inventory_array(LLAppearanceMgr::instance().getCOF(), inv_items_to_link, cb);
#else
		requestAttachments(mPendingAttachments);
#endif
		mPendingAttachments.clear();
	}

}

// FIXME this is basically the same code as LLAgentWearables::userAttachMultipleAttachments(),
// should consolidate.
void LLAttachmentsMgr::requestAttachments(const attachments_vec_t& attachment_requests)
{
	// Make sure we got a region before trying anything else
	if( !gAgent.getRegion() )
	{
		return;
	}

	S32 obj_count = attachment_requests.size();
	if (obj_count == 0)
	{
		return;
	}
	LL_DEBUGS("Avatar") << "ATT [RezMultipleAttachmentsFromInv] attaching multiple from attachment_requests,"
		" total obj_count " << obj_count << LL_ENDL;

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
	for (attachments_vec_t::const_iterator iter = attachment_requests.begin();
		 iter != attachment_requests.end();
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
			LL_INFOS() << "Attempted to add non-existent item ID:" << attachment.mItemID << LL_ENDL;
			continue;
		}
		LL_DEBUGS("Avatar") << "ATT requesting from attachment_requests " << item->getName()
							<< " " << item->getLinkedUUID() << LL_ENDL;
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
}
