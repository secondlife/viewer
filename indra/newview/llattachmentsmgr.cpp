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

#include "llvoavatarself.h"
#include "llagent.h"
#include "llappearancemgr.h"
#include "llinventorymodel.h"
#include "lltooldraganddrop.h" // pack_permissions_slam
#include "llviewerinventory.h"
#include "llviewerregion.h"
#include "message.h"

const F32 COF_LINK_BATCH_TIME = 5.0F;
const F32 MAX_ATTACHMENT_REQUEST_LIFETIME = 30.0F;
const F32 MIN_RETRY_REQUEST_TIME = 5.0F;
const F32 MAX_BAD_COF_TIME = 30.0F;

LLAttachmentsMgr::LLAttachmentsMgr():
    mAttachmentRequests("attach",MIN_RETRY_REQUEST_TIME),
    mDetachRequests("detach",MIN_RETRY_REQUEST_TIME),
    mQuestionableCOFLinks("badcof",MAX_BAD_COF_TIME)
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

    if (mAttachmentRequests.wasRequestedRecently(item_id))
    {
        LL_DEBUGS("Avatar") << "ATT not adding attachment to mPendingAttachments, recent request is already pending: "
                            << (item ? item->getName() : "UNKNOWN") << " id " << item_id << LL_ENDL;
        return;
    }

	LL_DEBUGS("Avatar") << "ATT adding attachment to mPendingAttachments "
						<< (item ? item->getName() : "UNKNOWN") << " id " << item_id << LL_ENDL;

	AttachmentsInfo attachment;
	attachment.mItemID = item_id;
	attachment.mAttachmentPt = attachment_pt;
	attachment.mAdd = add;
	mPendingAttachments.push_back(attachment);

    mAttachmentRequests.addTime(item_id);
}

void LLAttachmentsMgr::onAttachmentRequested(const LLUUID& item_id)
{
	LLViewerInventoryItem *item = gInventory.getItem(item_id);
	LL_DEBUGS("Avatar") << "ATT attachment was requested "
						<< (item ? item->getName() : "UNKNOWN") << " id " << item_id << LL_ENDL;
    mAttachmentRequests.addTime(item_id);
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

    if (LLApp::isExiting())
    {
        return;
    }

	requestPendingAttachments();

    linkRecentlyArrivedAttachments();

    expireOldAttachmentRequests();

    expireOldDetachRequests();

    checkInvalidCOFLinks();
    
    spamStatusInfo();
}

void LLAttachmentsMgr::requestPendingAttachments()
{
	if (mPendingAttachments.size())
	{
		requestAttachments(mPendingAttachments);
	}
}

// Send request(s) for a group of attachments. As coded, this can
// request at most 40 attachments and the rest will be
// ignored. Currently the max attachments per avatar is 38, so the 40
// limit should not be hit in practice.
void LLAttachmentsMgr::requestAttachments(attachments_vec_t& attachment_requests)
{
	// Make sure we got a region before trying anything else
	if( !gAgent.getRegion() )
	{
		return;
	}

    // For unknown reasons, requesting many attachments at once causes
    // frequent server-side failures. Here we're limiting the number
    // of attachments requested per idle loop.
    const S32 max_objects_per_request = 5;
	S32 obj_count = llmin((S32)attachment_requests.size(),max_objects_per_request);
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
        LL_WARNS() << "ATT Too many attachments requested: " << obj_count
                   << " exceeds limit of " << MAX_OBJECTS_TO_SEND << LL_ENDL;

		obj_count = MAX_OBJECTS_TO_SEND;
	}

	LL_DEBUGS("Avatar") << "ATT [RezMultipleAttachmentsFromInv] attaching multiple from attachment_requests,"
		" total obj_count " << obj_count << LL_ENDL;

	LLUUID compound_msg_id;
	compound_msg_id.generate();
	LLMessageSystem* msg = gMessageSystem;

    // by construction above, obj_count <= attachment_requests.size(), so no
    // check against attachment_requests.empty() is needed.
    llassert(obj_count <= attachment_requests.size());

    for (S32 i=0; i<obj_count; i++)
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

		const AttachmentsInfo& attachment = attachment_requests.front();
		LLViewerInventoryItem* item = gInventory.getItem(attachment.mItemID);
		if (item)
        {
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
        }
        else
		{
			LL_WARNS("Avatar") << "ATT Attempted to add non-existent item ID:" << attachment.mItemID << LL_ENDL;
		}

		if( (i+1 == obj_count) || ((OBJECTS_PER_PACKET-1) == (i % OBJECTS_PER_PACKET)) )
		{
			// End of message chunk
			msg->sendReliable( gAgent.getRegion()->getHost() );
		}
        attachment_requests.pop_front();
	}
}

void LLAttachmentsMgr::linkRecentlyArrivedAttachments()
{
    if (mRecentlyArrivedAttachments.size())
    {
        // One or more attachments have arrived but have not yet been
        // processed for COF links
        if (mAttachmentRequests.empty())
        {
            // Not waiting for any more.
            LL_DEBUGS("Avatar") << "ATT all pending attachments have arrived after "
                                << mCOFLinkBatchTimer.getElapsedTimeF32() << " seconds" << LL_ENDL;
        }
        else if (mCOFLinkBatchTimer.getElapsedTimeF32() > COF_LINK_BATCH_TIME)
        {
            LL_DEBUGS("Avatar") << "ATT " << mAttachmentRequests.size()
                                << " pending attachments have not arrived, but wait time exceeded" << LL_ENDL;
        }
        else
        {
            return;
        }

        LL_DEBUGS("Avatar") << "ATT checking COF linkability for " << mRecentlyArrivedAttachments.size()
                            << " recently arrived items" << LL_ENDL;

        uuid_vec_t ids_to_link;
        for (std::set<LLUUID>::iterator it = mRecentlyArrivedAttachments.begin();
             it != mRecentlyArrivedAttachments.end(); ++it)
        {
            if (isAgentAvatarValid() &&
                gAgentAvatarp->isWearingAttachment(*it) &&
                !LLAppearanceMgr::instance().isLinkedInCOF(*it))
            {
                LLUUID item_id = *it;
                LLViewerInventoryItem *item = gInventory.getItem(item_id);
                LL_DEBUGS("Avatar") << "ATT adding COF link for attachment "
                                    << (item ? item->getName() : "UNKNOWN") << " " << item_id << LL_ENDL;
                ids_to_link.push_back(item_id);
            }
        }
        if (ids_to_link.size())
        {
            LLPointer<LLInventoryCallback> cb = new LLRequestServerAppearanceUpdateOnDestroy();
            for (uuid_vec_t::const_iterator uuid_it = ids_to_link.begin();
                 uuid_it != ids_to_link.end(); ++uuid_it)
            {
                LLAppearanceMgr::instance().addCOFItemLink(*uuid_it, cb);
            }
        }
        mRecentlyArrivedAttachments.clear();
    }
}

LLAttachmentsMgr::LLItemRequestTimes::LLItemRequestTimes(const std::string& op_name, F32 timeout):
    mOpName(op_name),
    mTimeout(timeout)
{
}

void LLAttachmentsMgr::LLItemRequestTimes::addTime(const LLUUID& inv_item_id)
{
    LLInventoryItem *item = gInventory.getItem(inv_item_id);
    LL_DEBUGS("Avatar") << "ATT " << mOpName << " adding request time " << (item ? item->getName() : "UNKNOWN") << " " << inv_item_id << LL_ENDL;
	LLTimer current_time;
	(*this)[inv_item_id] = current_time;
}

void LLAttachmentsMgr::LLItemRequestTimes::removeTime(const LLUUID& inv_item_id)
{
    LLInventoryItem *item = gInventory.getItem(inv_item_id);
	S32 remove_count = (*this).erase(inv_item_id);
    if (remove_count)
    {
        LL_DEBUGS("Avatar") << "ATT " << mOpName << " removing request time "
                            << (item ? item->getName() : "UNKNOWN") << " " << inv_item_id << LL_ENDL;
    }
}

BOOL LLAttachmentsMgr::LLItemRequestTimes::getTime(const LLUUID& inv_item_id, LLTimer& timer) const
{
	std::map<LLUUID,LLTimer>::const_iterator it = (*this).find(inv_item_id);
	if (it != (*this).end())
	{
        timer = it->second;
        return TRUE;
    }
    return FALSE;
}

BOOL LLAttachmentsMgr::LLItemRequestTimes::wasRequestedRecently(const LLUUID& inv_item_id) const
{
    LLTimer request_time;
    if (getTime(inv_item_id, request_time))
    {
		F32 request_time_elapsed = request_time.getElapsedTimeF32();
        return request_time_elapsed < mTimeout;
    }
    else
    {
        return FALSE;
    }
}

// If we've been waiting for an attachment a long time, we want to
// forget the request, because if the request is invalid (say the
// object does not exist), the existence of a request that never goes
// away will gum up the COF batch logic, causing it to always wait for
// the timeout. Expiring a request means if the item does show up
// late, the COF link request may not get properly batched up, but
// behavior will be no worse than before we had the batching mechanism
// in place; the COF link will still be created, but extra
// requestServerAppearanceUpdate() calls may occur.
void LLAttachmentsMgr::expireOldAttachmentRequests()
{
	for (std::map<LLUUID,LLTimer>::iterator it = mAttachmentRequests.begin();
         it != mAttachmentRequests.end(); )
    {
        std::map<LLUUID,LLTimer>::iterator curr_it = it;
        ++it;
        if (curr_it->second.getElapsedTimeF32() > MAX_ATTACHMENT_REQUEST_LIFETIME)
        {
            LLInventoryItem *item = gInventory.getItem(curr_it->first);
            LL_WARNS("Avatar") << "ATT expiring request for attachment "
                                << (item ? item->getName() : "UNKNOWN") << " item_id " << curr_it->first
                                << " after " << MAX_ATTACHMENT_REQUEST_LIFETIME << " seconds" << LL_ENDL;
            mAttachmentRequests.erase(curr_it);
        }
    }
}

void LLAttachmentsMgr::expireOldDetachRequests()
{
	for (std::map<LLUUID,LLTimer>::iterator it = mDetachRequests.begin();
         it != mDetachRequests.end(); )
    {
        std::map<LLUUID,LLTimer>::iterator curr_it = it;
        ++it;
        if (curr_it->second.getElapsedTimeF32() > MAX_ATTACHMENT_REQUEST_LIFETIME)
        {
            LLInventoryItem *item = gInventory.getItem(curr_it->first);
            LL_WARNS("Avatar") << "ATT expiring request for detach "
                                << (item ? item->getName() : "UNKNOWN") << " item_id " << curr_it->first
                                << " after " << MAX_ATTACHMENT_REQUEST_LIFETIME << " seconds" << LL_ENDL;
            mDetachRequests.erase(curr_it);
        }
    }
}

// When an attachment arrives, we want to stop waiting for it, and add
// it to the set of recently arrived items.
void LLAttachmentsMgr::onAttachmentArrived(const LLUUID& inv_item_id)
{
    LLTimer timer;
    bool expected = mAttachmentRequests.getTime(inv_item_id, timer);
    if (!expected)
    {
        LLInventoryItem *item = gInventory.getItem(inv_item_id);
        LL_WARNS() << "ATT Attachment was unexpected or arrived after " << MAX_ATTACHMENT_REQUEST_LIFETIME << " seconds: "
                   << (item ? item->getName() : "UNKNOWN") << " id " << inv_item_id << LL_ENDL;
    }
    mAttachmentRequests.removeTime(inv_item_id);
    if (expected && mAttachmentRequests.empty())
    {
        // mAttachmentRequests just emptied out
        LL_DEBUGS("Avatar") << "ATT all active attachment requests have completed" << LL_ENDL;
    }
    if (mRecentlyArrivedAttachments.empty())
    {
        // Start the timer for sending off a COF link batch.
        mCOFLinkBatchTimer.reset();
    }
    mRecentlyArrivedAttachments.insert(inv_item_id);
}

void LLAttachmentsMgr::onDetachRequested(const LLUUID& inv_item_id)
{
    mDetachRequests.addTime(inv_item_id);
}

void LLAttachmentsMgr::onDetachCompleted(const LLUUID& inv_item_id)
{
    LLTimer timer;
    LLInventoryItem *item = gInventory.getItem(inv_item_id);
    if (mDetachRequests.getTime(inv_item_id, timer))
    {
        LL_DEBUGS("Avatar") << "ATT detach completed after " << timer.getElapsedTimeF32()
                            << " seconds for " << (item ? item->getName() : "UNKNOWN") << " " << inv_item_id << LL_ENDL;
        mDetachRequests.removeTime(inv_item_id);
        if (mDetachRequests.empty())
        {
            LL_DEBUGS("Avatar") << "ATT all detach requests have completed" << LL_ENDL;
        }
    }
    else
    {
        LL_WARNS() << "ATT unexpected detach for "
                   << (item ? item->getName() : "UNKNOWN") << " id " << inv_item_id << LL_ENDL;
    }

    LL_DEBUGS("Avatar") << "ATT detached item flagging as questionable for COF link checking "
                        << (item ? item->getName() : "UNKNOWN") << " id " << inv_item_id << LL_ENDL;
    mQuestionableCOFLinks.addTime(inv_item_id);
}

// Check for attachments that are (a) linked in COF and (b) not
// attached to the avatar.  This is a rotten function to have to
// include, because it runs the risk of either repeatedly spamming out
// COF link removals if they're failing for some reason, or getting
// into a tug of war with some other sequence of events that's in the
// process of adding the attachment in question. However, it's needed
// because we have no definitive source of authority for what things
// are actually supposed to be attached. Scripts, run on the server
// side, can remove an attachment without our expecting it. If this
// happens to an attachment that's just been added, then the COF link
// creation may still be in flight, and we will have to delete the
// link after it shows up.
//
// Note that we only flag items for possible link removal if they have
// been previously detached. This means that an attachment failure
// will leave the link in the COF, where it will hopefully resolve
// correctly on relog.
//
// See related: MAINT-5070, MAINT-4409
//
void LLAttachmentsMgr::checkInvalidCOFLinks()
{
        LLInventoryModel::cat_array_t cat_array;
        LLInventoryModel::item_array_t item_array;
        gInventory.collectDescendents(LLAppearanceMgr::instance().getCOF(),
                                      cat_array,item_array,LLInventoryModel::EXCLUDE_TRASH);
        for (S32 i=0; i<item_array.size(); i++)
        {
            const LLViewerInventoryItem* inv_item = item_array.at(i).get();
            const LLUUID& item_id = inv_item->getLinkedUUID();
            if (inv_item->getType() == LLAssetType::AT_OBJECT)
            {
                LLTimer timer;
                bool is_flagged_questionable = mQuestionableCOFLinks.getTime(item_id,timer);
                bool is_wearing_attachment = isAgentAvatarValid() && gAgentAvatarp->isWearingAttachment(item_id);
                if (is_wearing_attachment && is_flagged_questionable)
                {
                    LL_DEBUGS("Avatar") << "ATT was flagged questionable but is now " 
                                        << (is_wearing_attachment ? "attached " : "") 
                                        <<"removing flag after "
                                        << timer.getElapsedTimeF32() << " item "
                                        << inv_item->getName() << " id " << item_id << LL_ENDL;
                    mQuestionableCOFLinks.removeTime(item_id);
                }
            }
        }

        for(LLItemRequestTimes::iterator it = mQuestionableCOFLinks.begin();
            it != mQuestionableCOFLinks.end(); )
        {
            LLItemRequestTimes::iterator curr_it = it;
            ++it;
            const LLUUID& item_id = curr_it->first;
            LLViewerInventoryItem *inv_item = gInventory.getItem(item_id);
            if (curr_it->second.getElapsedTimeF32() > MAX_BAD_COF_TIME)
            {
                if (LLAppearanceMgr::instance().isLinkedInCOF(item_id))
                {
                    LL_DEBUGS("Avatar") << "ATT Linked in COF but not attached or requested, deleting link after "
                                        << curr_it->second.getElapsedTimeF32() << " seconds for " 
                                        << (inv_item ? inv_item->getName() : "UNKNOWN") << " id " << item_id << LL_ENDL;
                    LLAppearanceMgr::instance().removeCOFItemLinks(item_id);
                }
				mQuestionableCOFLinks.erase(curr_it);
                continue;
            }
        }
}

void LLAttachmentsMgr::spamStatusInfo()
{
#if 0
    static LLTimer spam_timer;
    const F32 spam_frequency = 100.0F;

    if (spam_timer.getElapsedTimeF32() > spam_frequency)
    {
        spam_timer.reset();
        
        LLInventoryModel::cat_array_t cat_array;
        LLInventoryModel::item_array_t item_array;
        gInventory.collectDescendents(LLAppearanceMgr::instance().getCOF(),
                                      cat_array,item_array,LLInventoryModel::EXCLUDE_TRASH);
        for (S32 i=0; i<item_array.size(); i++)
        {
            const LLViewerInventoryItem* inv_item = item_array.at(i).get();
            if (inv_item->getType() == LLAssetType::AT_OBJECT)
            {
                LL_DEBUGS("Avatar") << "item_id: " << inv_item->getUUID()
                                    << " linked_item_id: " << inv_item->getLinkedUUID()
                                    << " name: " << inv_item->getName()
                                    << " parent: " << inv_item->getParentUUID()
                                    << LL_ENDL;
            }
        }
    }
#endif
}
