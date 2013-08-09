/** 
 * @file llagentwearablesfetch.cpp
 * @brief LLAgentWearblesFetch class implementation
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

#include "llviewerprecompiledheaders.h"
#include "llagentwearablesfetch.h"

#include "llagent.h"
#include "llagentwearables.h"
#include "llappearancemgr.h"
#include "llinventoryfunctions.h"
#include "llstartup.h"
#include "llvoavatarself.h"


LLInitialWearablesFetch::LLInitialWearablesFetch(const LLUUID& cof_id) :
	LLInventoryFetchDescendentsObserver(cof_id)
{
	if (isAgentAvatarValid())
	{
		gAgentAvatarp->startPhase("initial_wearables_fetch");
		gAgentAvatarp->outputRezTiming("Initial wearables fetch started");
	}
}

LLInitialWearablesFetch::~LLInitialWearablesFetch()
{
}

// virtual
void LLInitialWearablesFetch::done()
{
	// Delay processing the actual results of this so it's not handled within
	// gInventory.notifyObservers.  The results will be handled in the next
	// idle tick instead.
	gInventory.removeObserver(this);
	doOnIdleOneTime(boost::bind(&LLInitialWearablesFetch::processContents,this));
	if (isAgentAvatarValid())
	{
		gAgentAvatarp->stopPhase("initial_wearables_fetch");
		gAgentAvatarp->outputRezTiming("Initial wearables fetch done");
	}
}

void LLInitialWearablesFetch::add(InitialWearableData &data)

{
	mAgentInitialWearables.push_back(data);
}

void LLInitialWearablesFetch::processContents()
{
	if(!gAgentAvatarp) //no need to process wearables if the agent avatar is deleted.
	{
		delete this;
		return ;
	}

	// Fetch the wearable items from the Current Outfit Folder
	LLInventoryModel::cat_array_t cat_array;
	LLInventoryModel::item_array_t wearable_array;
	LLFindWearables is_wearable;
	llassert_always(mComplete.size() != 0);
	gInventory.collectDescendentsIf(mComplete.front(), cat_array, wearable_array, 
									LLInventoryModel::EXCLUDE_TRASH, is_wearable);

	LLAppearanceMgr::instance().setAttachmentInvLinkEnable(true);
	if (wearable_array.count() > 0)
	{
		gAgentWearables.notifyLoadingStarted();
		LLAppearanceMgr::instance().updateAppearanceFromCOF();
	}
	else
	{
		// if we're constructing the COF from the wearables message, we don't have a proper outfit link
		LLAppearanceMgr::instance().setOutfitDirty(true);
		processWearablesMessage();
	}
	delete this;
}

class LLFetchAndLinkObserver: public LLInventoryFetchItemsObserver
{
public:
	LLFetchAndLinkObserver(uuid_vec_t& ids):
		LLInventoryFetchItemsObserver(ids)
	{
	}
	~LLFetchAndLinkObserver()
	{
	}
	virtual void done()
	{
		gInventory.removeObserver(this);

		// Link to all fetched items in COF.
		LLPointer<LLInventoryCallback> link_waiter = new LLUpdateAppearanceOnDestroy;
		LLInventoryObject::const_object_list_t item_array;
		for (uuid_vec_t::iterator it = mIDs.begin();
			 it != mIDs.end();
			 ++it)
		{
			LLUUID id = *it;
			LLConstPointer<LLInventoryObject> item = gInventory.getItem(*it);
			if (!item)
			{
				llwarns << "fetch failed for item " << (*it) << "!" << llendl;
				continue;
			}
			item_array.push_back(item);
		}
		link_inventory_array(LLAppearanceMgr::instance().getCOF(), item_array, link_waiter);
	}
};

void LLInitialWearablesFetch::processWearablesMessage()
{
	if (!mAgentInitialWearables.empty()) // We have an empty current outfit folder, use the message data instead.
	{
		const LLUUID current_outfit_id = LLAppearanceMgr::instance().getCOF();
		uuid_vec_t ids;
		for (U8 i = 0; i < mAgentInitialWearables.size(); ++i)
		{
			// Populate the current outfit folder with links to the wearables passed in the message
			InitialWearableData *wearable_data = new InitialWearableData(mAgentInitialWearables[i]); // This will be deleted in the callback.
			
			if (wearable_data->mAssetID.notNull())
			{
				ids.push_back(wearable_data->mItemID);
			}
			else
			{
				llinfos << "Invalid wearable, type " << wearable_data->mType << " itemID "
				<< wearable_data->mItemID << " assetID " << wearable_data->mAssetID << llendl;
				delete wearable_data;
			}
		}

		// Add all current attachments to the requested items as well.
		if (isAgentAvatarValid())
		{
			for (LLVOAvatar::attachment_map_t::const_iterator iter = gAgentAvatarp->mAttachmentPoints.begin(); 
				 iter != gAgentAvatarp->mAttachmentPoints.end(); ++iter)
			{
				LLViewerJointAttachment* attachment = iter->second;
				if (!attachment) continue;
				for (LLViewerJointAttachment::attachedobjs_vec_t::iterator attachment_iter = attachment->mAttachedObjects.begin();
					 attachment_iter != attachment->mAttachedObjects.end();
					 ++attachment_iter)
				{
					LLViewerObject* attached_object = (*attachment_iter);
					if (!attached_object) continue;
					const LLUUID& item_id = attached_object->getAttachmentItemID();
					if (item_id.isNull()) continue;
					ids.push_back(item_id);
				}
			}
		}

		// Need to fetch the inventory items for ids, then create links to them after they arrive.
		LLFetchAndLinkObserver *fetcher = new LLFetchAndLinkObserver(ids);
		fetcher->startFetch();
		// If no items to be fetched, done will never be triggered.
		// TODO: Change LLInventoryFetchItemsObserver::fetchItems to trigger done() on this condition.
		if (fetcher->isFinished())
		{
			fetcher->done();
		}
		else
		{
			gInventory.addObserver(fetcher);
		}
	}
	else
	{
		LL_WARNS("Wearables") << "No current outfit folder items found and no initial wearables fallback message received." << LL_ENDL;
	}
}

