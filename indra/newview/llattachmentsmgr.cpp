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

void LLAttachmentsMgr::addAttachmentRequest(const LLUUID& item_id,
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

    addAttachmentRequestTime(item_id);
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

	requestPendingAttachments();

    linkRecentlyArrivedAttachments();
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

void LLAttachmentsMgr::requestPendingAttachments()
{
	if (mPendingAttachments.size())
	{
		requestAttachments(mPendingAttachments);
		mPendingAttachments.clear();
	}
}

void LLAttachmentsMgr::linkRecentlyArrivedAttachments()
{
    const F32 COF_LINK_BATCH_TIME = 5.0F;
    
    // One or more attachments have arrived but not been processed for COF links
    if (mRecentlyArrivedAttachments.size())
    {
        if (mAttachmentRequests.empty())
        {
            // Not waiting for more
            LL_DEBUGS("Avatar") << "ATT all pending attachments have arrived" << LL_ENDL;
        }
        else if (mCOFLinkBatchTimer.getElapsedTimeF32() > COF_LINK_BATCH_TIME)
        {
            LL_DEBUGS("Avatar") << mAttachmentRequests.size()
                                << "ATT pending attachments have not arrived but wait time exceeded" << LL_ENDL;
        }
        else
        {
            return;
        }

        LL_DEBUGS("Avatar") << "ATT requesting COF links for " << mRecentlyArrivedAttachments.size() << LL_ENDL;
		LLInventoryObject::const_object_list_t inv_items_to_link;
        for (std::set<LLUUID>::iterator it = mRecentlyArrivedAttachments.begin();
             it != mRecentlyArrivedAttachments.end(); ++it)
        {
            if (!LLAppearanceMgr::instance().isLinkedInCOF(*it))
            {
                LLUUID item_id = *it;
                LLViewerInventoryItem *item = gInventory.getItem(item_id);
                LL_DEBUGS("Avatar") << "ATT adding COF link for attachment "
                                    << (item ? item->getName() : "UNKNOWN") << " " << item_id << LL_ENDL;
                inv_items_to_link.push_back(item);
            }
        }
        if (inv_items_to_link.size())
        {
            LLPointer<LLInventoryCallback> cb = new LLRequestServerAppearanceUpdateOnDestroy();
            link_inventory_array(LLAppearanceMgr::instance().getCOF(), inv_items_to_link, cb);
        }
        mRecentlyArrivedAttachments.clear();
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

void LLAttachmentsMgr::addAttachmentRequestTime(const LLUUID& inv_item_id)
{
    LLInventoryItem *item = gInventory.getItem(inv_item_id);
    LL_DEBUGS("Avatar") << "ATT add request time " << (item ? item->getName() : "UNKNOWN") << " " << inv_item_id << LL_ENDL;
	LLTimer current_time;
	mAttachmentRequests[inv_item_id] = current_time;
}

void LLAttachmentsMgr::removeAttachmentRequestTime(const LLUUID& inv_item_id)
{
    LLInventoryItem *item = gInventory.getItem(inv_item_id);
    LL_DEBUGS("Avatar") << "ATT remove request time " << (item ? item->getName() : "UNKNOWN") << " " << inv_item_id << LL_ENDL;
	mAttachmentRequests.erase(inv_item_id);
}

BOOL LLAttachmentsMgr::attachmentWasRequestedRecently(const LLUUID& inv_item_id, F32 seconds) const
{
	std::map<LLUUID,LLTimer>::const_iterator it = mAttachmentRequests.find(inv_item_id);
	if (it != mAttachmentRequests.end())
	{
		const LLTimer& request_time = it->second;
		F32 request_time_elapsed = request_time.getElapsedTimeF32();
		if (request_time_elapsed > seconds)
		{
            LLInventoryItem *item = gInventory.getItem(inv_item_id);
            LL_DEBUGS("Avatar") << "ATT time ignored, exceeded " << seconds
                                << " " << (item ? item->getName() : "UNKNOWN") << " " << inv_item_id << LL_ENDL;
			return FALSE;
		}
		else
		{
			return TRUE;
		}
	}
	else
	{
		return FALSE;
	}
}

void LLAttachmentsMgr::onAttachmentArrived(const LLUUID& inv_item_id)
{
    removeAttachmentRequestTime(inv_item_id);
    if (mRecentlyArrivedAttachments.empty())
    {
        mCOFLinkBatchTimer.reset();
    }
    mRecentlyArrivedAttachments.insert(inv_item_id);
}
