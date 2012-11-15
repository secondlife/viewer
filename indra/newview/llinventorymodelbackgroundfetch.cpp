/** 
 * @file llinventorymodel.cpp
 * @brief Implementation of the inventory model used to track agent inventory.
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

#include "llviewerprecompiledheaders.h"
#include "llinventorymodelbackgroundfetch.h"

#include "llagent.h"
#include "llappviewer.h"
#include "llcallbacklist.h"
#include "llinventorypanel.h"
#include "llinventorymodel.h"
#include "llviewercontrol.h"
#include "llviewerinventory.h"
#include "llviewermessage.h"
#include "llviewerregion.h"
#include "llviewerwindow.h"

const F32 MAX_TIME_FOR_SINGLE_FETCH = 10.f;
const S32 MAX_FETCH_RETRIES = 10;

LLInventoryModelBackgroundFetch::LLInventoryModelBackgroundFetch() :
	mBackgroundFetchActive(FALSE),
	mFolderFetchActive(false),
	mAllFoldersFetched(FALSE),
	mRecursiveInventoryFetchStarted(FALSE),
	mRecursiveLibraryFetchStarted(FALSE),
	mNumFetchRetries(0),
	mMinTimeBetweenFetches(0.3f),
	mMaxTimeBetweenFetches(10.f),
	mTimelyFetchPending(FALSE),
	mFetchCount(0)
{
}

LLInventoryModelBackgroundFetch::~LLInventoryModelBackgroundFetch()
{
}

bool LLInventoryModelBackgroundFetch::isBulkFetchProcessingComplete() const
{
	return mFetchQueue.empty() && mFetchCount<=0;
}

bool LLInventoryModelBackgroundFetch::libraryFetchStarted() const
{
	return mRecursiveLibraryFetchStarted;
}

bool LLInventoryModelBackgroundFetch::libraryFetchCompleted() const
{
	return libraryFetchStarted() && fetchQueueContainsNoDescendentsOf(gInventory.getLibraryRootFolderID());
}

bool LLInventoryModelBackgroundFetch::libraryFetchInProgress() const
{
	return libraryFetchStarted() && !libraryFetchCompleted();
}
	
bool LLInventoryModelBackgroundFetch::inventoryFetchStarted() const
{
	return mRecursiveInventoryFetchStarted;
}

bool LLInventoryModelBackgroundFetch::inventoryFetchCompleted() const
{
	return inventoryFetchStarted() && fetchQueueContainsNoDescendentsOf(gInventory.getRootFolderID());
}

bool LLInventoryModelBackgroundFetch::inventoryFetchInProgress() const
{
	return inventoryFetchStarted() && !inventoryFetchCompleted();
}

bool LLInventoryModelBackgroundFetch::isEverythingFetched() const
{
	return mAllFoldersFetched;
}

BOOL LLInventoryModelBackgroundFetch::folderFetchActive() const
{
	return mFolderFetchActive;
}

void LLInventoryModelBackgroundFetch::start(const LLUUID& id, BOOL recursive)
{
	LLViewerInventoryCategory* cat = gInventory.getCategory(id);
	if (cat || (id.isNull() && !isEverythingFetched()))
	{	// it's a folder, do a bulk fetch
		LL_DEBUGS("InventoryFetch") << "Start fetching category: " << id << ", recursive: " << recursive << LL_ENDL;

		mBackgroundFetchActive = TRUE;
		mFolderFetchActive = true;
		if (id.isNull())
		{
			if (!mRecursiveInventoryFetchStarted)
			{
				mRecursiveInventoryFetchStarted |= recursive;
				mFetchQueue.push_back(FetchQueueInfo(gInventory.getRootFolderID(), recursive));
				gIdleCallbacks.addFunction(&LLInventoryModelBackgroundFetch::backgroundFetchCB, NULL);
			}
			if (!mRecursiveLibraryFetchStarted)
			{
				mRecursiveLibraryFetchStarted |= recursive;
				mFetchQueue.push_back(FetchQueueInfo(gInventory.getLibraryRootFolderID(), recursive));
				gIdleCallbacks.addFunction(&LLInventoryModelBackgroundFetch::backgroundFetchCB, NULL);
			}
		}
		else
		{
			// Specific folder requests go to front of queue.
			if (mFetchQueue.empty() || mFetchQueue.front().mUUID != id)
			{
				mFetchQueue.push_front(FetchQueueInfo(id, recursive));
				gIdleCallbacks.addFunction(&LLInventoryModelBackgroundFetch::backgroundFetchCB, NULL);
			}
			if (id == gInventory.getLibraryRootFolderID())
			{
				mRecursiveLibraryFetchStarted |= recursive;
			}
			if (id == gInventory.getRootFolderID())
			{
				mRecursiveInventoryFetchStarted |= recursive;
			}
		}
	}
	else if (LLViewerInventoryItem* itemp = gInventory.getItem(id))
	{
		if (!itemp->mIsComplete && (mFetchQueue.empty() || mFetchQueue.front().mUUID != id))
		{
			mBackgroundFetchActive = TRUE;

			mFetchQueue.push_front(FetchQueueInfo(id, false, false));
			gIdleCallbacks.addFunction(&LLInventoryModelBackgroundFetch::backgroundFetchCB, NULL);
		}
	}
}

void LLInventoryModelBackgroundFetch::findLostItems()
{
	mBackgroundFetchActive = TRUE;
	mFolderFetchActive = true;
    mFetchQueue.push_back(FetchQueueInfo(LLUUID::null, TRUE));
    gIdleCallbacks.addFunction(&LLInventoryModelBackgroundFetch::backgroundFetchCB, NULL);
}

void LLInventoryModelBackgroundFetch::setAllFoldersFetched()
{
	if (mRecursiveInventoryFetchStarted &&
		mRecursiveLibraryFetchStarted)
	{
		mAllFoldersFetched = TRUE;
	}
	mFolderFetchActive = false;
}

void LLInventoryModelBackgroundFetch::backgroundFetchCB(void *)
{
	LLInventoryModelBackgroundFetch::instance().backgroundFetch();
}

void LLInventoryModelBackgroundFetch::backgroundFetch()
{
	if (mBackgroundFetchActive && gAgent.getRegion() && gAgent.getRegion()->capabilitiesReceived())
	{
		// If we'll be using the capability, we'll be sending batches and the background thing isn't as important.
		if (gSavedSettings.getBOOL("UseHTTPInventory")) 
		{
			bulkFetch();
			return;
		}
		
#if 1
		//--------------------------------------------------------------------------------
		// DEPRECATED OLD CODE
		//

		// No more categories to fetch, stop fetch process.
		if (mFetchQueue.empty())
		{
			llinfos << "Inventory fetch completed" << llendl;

			setAllFoldersFetched();
			mBackgroundFetchActive = false;
			mFolderFetchActive = false;

			return;
		}

		F32 fast_fetch_time = lerp(mMinTimeBetweenFetches, mMaxTimeBetweenFetches, 0.1f);
		F32 slow_fetch_time = lerp(mMinTimeBetweenFetches, mMaxTimeBetweenFetches, 0.5f);
		if (mTimelyFetchPending && mFetchTimer.getElapsedTimeF32() > slow_fetch_time)
		{
			// Double timeouts on failure.
			mMinTimeBetweenFetches = llmin(mMinTimeBetweenFetches * 2.f, 10.f);
			mMaxTimeBetweenFetches = llmin(mMaxTimeBetweenFetches * 2.f, 120.f);
			lldebugs << "Inventory fetch times grown to (" << mMinTimeBetweenFetches << ", " << mMaxTimeBetweenFetches << ")" << llendl;
			// fetch is no longer considered "timely" although we will wait for full time-out.
			mTimelyFetchPending = FALSE;
		}

		while(1)
		{
			if (mFetchQueue.empty())
			{
				break;
			}

			if(gDisconnected)
			{
				// Just bail if we are disconnected.
				break;
			}

			const FetchQueueInfo info = mFetchQueue.front();

			if (info.mIsCategory)
			{

				LLViewerInventoryCategory* cat = gInventory.getCategory(info.mUUID);

				// Category has been deleted, remove from queue.
				if (!cat)
				{
					mFetchQueue.pop_front();
					continue;
				}
			
				if (mFetchTimer.getElapsedTimeF32() > mMinTimeBetweenFetches && 
					LLViewerInventoryCategory::VERSION_UNKNOWN == cat->getVersion())
				{
					// Category exists but has no children yet, fetch the descendants
					// for now, just request every time and rely on retry timer to throttle.
					if (cat->fetch())
					{
						mFetchTimer.reset();
						mTimelyFetchPending = TRUE;
					}
					else
					{
						//  The catagory also tracks if it has expired and here it says it hasn't
						//  yet.  Get out of here because nothing is going to happen until we
						//  update the timers.
						break;
					}
				}
				// Do I have all my children?
				else if (gInventory.isCategoryComplete(info.mUUID))
				{
					// Finished with this category, remove from queue.
					mFetchQueue.pop_front();

					// Add all children to queue.
					LLInventoryModel::cat_array_t* categories;
					LLInventoryModel::item_array_t* items;
					gInventory.getDirectDescendentsOf(cat->getUUID(), categories, items);
					for (LLInventoryModel::cat_array_t::const_iterator it = categories->begin();
						 it != categories->end();
						 ++it)
					{
						mFetchQueue.push_back(FetchQueueInfo((*it)->getUUID(),info.mRecursive));
					}

					// We received a response in less than the fast time.
					if (mTimelyFetchPending && mFetchTimer.getElapsedTimeF32() < fast_fetch_time)
					{
						// Shrink timeouts based on success.
						mMinTimeBetweenFetches = llmax(mMinTimeBetweenFetches * 0.8f, 0.3f);
						mMaxTimeBetweenFetches = llmax(mMaxTimeBetweenFetches * 0.8f, 10.f);
						lldebugs << "Inventory fetch times shrunk to (" << mMinTimeBetweenFetches << ", " << mMaxTimeBetweenFetches << ")" << llendl;
					}

					mTimelyFetchPending = FALSE;
					continue;
				}
				else if (mFetchTimer.getElapsedTimeF32() > mMaxTimeBetweenFetches)
				{
					// Received first packet, but our num descendants does not match db's num descendants
					// so try again later.
					mFetchQueue.pop_front();

					if (mNumFetchRetries++ < MAX_FETCH_RETRIES)
					{
						// push on back of queue
						mFetchQueue.push_back(info);
					}
					mTimelyFetchPending = FALSE;
					mFetchTimer.reset();
					break;
				}

				// Not enough time has elapsed to do a new fetch
				break;
			}
			else
			{
				LLViewerInventoryItem* itemp = gInventory.getItem(info.mUUID);

				mFetchQueue.pop_front();
				if (!itemp) 
				{
					continue;
				}

				if (mFetchTimer.getElapsedTimeF32() > mMinTimeBetweenFetches)
				{
					itemp->fetchFromServer();
					mFetchTimer.reset();
					mTimelyFetchPending = TRUE;
				}
				else if (itemp->mIsComplete)
				{
					mTimelyFetchPending = FALSE;
				}
				else if (mFetchTimer.getElapsedTimeF32() > mMaxTimeBetweenFetches)
				{
					mFetchQueue.push_back(info);
					mFetchTimer.reset();
					mTimelyFetchPending = FALSE;
				}
				// Not enough time has elapsed to do a new fetch
				break;
			}
		}

		//
		// DEPRECATED OLD CODE
		//--------------------------------------------------------------------------------
#endif
	}
}

void LLInventoryModelBackgroundFetch::incrFetchCount(S16 fetching) 
{  
	mFetchCount += fetching; 
	if (mFetchCount < 0)
	{
		mFetchCount = 0; 
	}
}

class LLInventoryModelFetchItemResponder : public LLInventoryModel::fetchInventoryResponder
{
public:
	LLInventoryModelFetchItemResponder(const LLSD& request_sd) : LLInventoryModel::fetchInventoryResponder(request_sd) {};
	void result(const LLSD& content);			
	void error(U32 status, const std::string& reason);
};

void LLInventoryModelFetchItemResponder::result( const LLSD& content )
{
	LLInventoryModel::fetchInventoryResponder::result(content);
	LLInventoryModelBackgroundFetch::instance().incrFetchCount(-1);
}

void LLInventoryModelFetchItemResponder::error( U32 status, const std::string& reason )
{
	LLInventoryModel::fetchInventoryResponder::error(status, reason);
	LLInventoryModelBackgroundFetch::instance().incrFetchCount(-1);
}


class LLInventoryModelFetchDescendentsResponder: public LLHTTPClient::Responder
{
public:
	LLInventoryModelFetchDescendentsResponder(const LLSD& request_sd, uuid_vec_t recursive_cats) : 
		mRequestSD(request_sd),
		mRecursiveCatUUIDs(recursive_cats)
	{};
	//LLInventoryModelFetchDescendentsResponder() {};
	void result(const LLSD& content);
	void error(U32 status, const std::string& reason);
protected:
	BOOL getIsRecursive(const LLUUID& cat_id) const;
private:
	LLSD mRequestSD;
	uuid_vec_t mRecursiveCatUUIDs; // hack for storing away which cat fetches are recursive
};

// If we get back a normal response, handle it here.
void LLInventoryModelFetchDescendentsResponder::result(const LLSD& content)
{
	LLInventoryModelBackgroundFetch *fetcher = LLInventoryModelBackgroundFetch::getInstance();
	if (content.has("folders"))	
	{

		for(LLSD::array_const_iterator folder_it = content["folders"].beginArray();
			folder_it != content["folders"].endArray();
			++folder_it)
		{	
			LLSD folder_sd = *folder_it;
			

			//LLUUID agent_id = folder_sd["agent_id"];

			//if(agent_id != gAgent.getID())	//This should never happen.
			//{
			//	llwarns << "Got a UpdateInventoryItem for the wrong agent."
			//			<< llendl;
			//	break;
			//}

			LLUUID parent_id = folder_sd["folder_id"];
			LLUUID owner_id = folder_sd["owner_id"];
			S32    version  = (S32)folder_sd["version"].asInteger();
			S32    descendents = (S32)folder_sd["descendents"].asInteger();
			LLPointer<LLViewerInventoryCategory> tcategory = new LLViewerInventoryCategory(owner_id);

            if (parent_id.isNull())
            {
			    LLPointer<LLViewerInventoryItem> titem = new LLViewerInventoryItem;
			    for(LLSD::array_const_iterator item_it = folder_sd["items"].beginArray();
				    item_it != folder_sd["items"].endArray();
				    ++item_it)
			    {	
                    const LLUUID lost_uuid = gInventory.findCategoryUUIDForType(LLFolderType::FT_LOST_AND_FOUND);
                    if (lost_uuid.notNull())
                    {
				        LLSD item = *item_it;
				        titem->unpackMessage(item);
				
                        LLInventoryModel::update_list_t update;
                        LLInventoryModel::LLCategoryUpdate new_folder(lost_uuid, 1);
                        update.push_back(new_folder);
                        gInventory.accountForUpdate(update);

                        titem->setParent(lost_uuid);
                        titem->updateParentOnServer(FALSE);
                        gInventory.updateItem(titem);
                        gInventory.notifyObservers();
                        
                    }
                }
            }

	        LLViewerInventoryCategory* pcat = gInventory.getCategory(parent_id);
			if (!pcat)
			{
				continue;
			}

			for(LLSD::array_const_iterator category_it = folder_sd["categories"].beginArray();
				category_it != folder_sd["categories"].endArray();
				++category_it)
			{	
				LLSD category = *category_it;
				tcategory->fromLLSD(category); 
				
				const BOOL recursive = getIsRecursive(tcategory->getUUID());
				
				if (recursive)
				{
					fetcher->mFetchQueue.push_back(LLInventoryModelBackgroundFetch::FetchQueueInfo(tcategory->getUUID(), recursive));
				}
				else if ( !gInventory.isCategoryComplete(tcategory->getUUID()) )
				{
					gInventory.updateCategory(tcategory);
				}

			}
			LLPointer<LLViewerInventoryItem> titem = new LLViewerInventoryItem;
			for(LLSD::array_const_iterator item_it = folder_sd["items"].beginArray();
				item_it != folder_sd["items"].endArray();
				++item_it)
			{	
				LLSD item = *item_it;
				titem->unpackMessage(item);
				
				gInventory.updateItem(titem);
			}

			// Set version and descendentcount according to message.
			LLViewerInventoryCategory* cat = gInventory.getCategory(parent_id);
			if(cat)
			{
				cat->setVersion(version);
				cat->setDescendentCount(descendents);
				cat->determineFolderType();
			}

		}
	}
		
	if (content.has("bad_folders"))
	{
		for(LLSD::array_const_iterator folder_it = content["bad_folders"].beginArray();
			folder_it != content["bad_folders"].endArray();
			++folder_it)
		{	
			LLSD folder_sd = *folder_it;
			
			// These folders failed on the dataserver.  We probably don't want to retry them.
			llinfos << "Folder " << folder_sd["folder_id"].asString() 
					<< "Error: " << folder_sd["error"].asString() << llendl;
		}
	}

	fetcher->incrFetchCount(-1);
	
	if (fetcher->isBulkFetchProcessingComplete())
	{
		llinfos << "Inventory fetch completed" << llendl;
		fetcher->setAllFoldersFetched();
	}
	
	gInventory.notifyObservers();
}

// If we get back an error (not found, etc...), handle it here.
void LLInventoryModelFetchDescendentsResponder::error(U32 status, const std::string& reason)
{
	LLInventoryModelBackgroundFetch *fetcher = LLInventoryModelBackgroundFetch::getInstance();

	llinfos << "LLInventoryModelFetchDescendentsResponder::error "
		<< status << ": " << reason << llendl;
						
	fetcher->incrFetchCount(-1);

	if (status==499) // timed out
	{
		for(LLSD::array_const_iterator folder_it = mRequestSD["folders"].beginArray();
			folder_it != mRequestSD["folders"].endArray();
			++folder_it)
		{	
			LLSD folder_sd = *folder_it;
			LLUUID folder_id = folder_sd["folder_id"];
			const BOOL recursive = getIsRecursive(folder_id);
			fetcher->mFetchQueue.push_front(LLInventoryModelBackgroundFetch::FetchQueueInfo(folder_id, recursive));
		}
	}
	else
	{
		if (fetcher->isBulkFetchProcessingComplete())
		{
			fetcher->setAllFoldersFetched();
		}
	}
	gInventory.notifyObservers();
}

BOOL LLInventoryModelFetchDescendentsResponder::getIsRecursive(const LLUUID& cat_id) const
{
	return (std::find(mRecursiveCatUUIDs.begin(),mRecursiveCatUUIDs.end(), cat_id) != mRecursiveCatUUIDs.end());
}

// Bundle up a bunch of requests to send all at once.
// static   
void LLInventoryModelBackgroundFetch::bulkFetch()
{
	//Background fetch is called from gIdleCallbacks in a loop until background fetch is stopped.
	//If there are items in mFetchQueue, we want to check the time since the last bulkFetch was 
	//sent.  If it exceeds our retry time, go ahead and fire off another batch.  
	LLViewerRegion* region = gAgent.getRegion();
	if (!region) return;

	S16 max_concurrent_fetches=8;
	F32 new_min_time = 0.5f;			//HACK!  Clean this up when old code goes away entirely.
	if (mMinTimeBetweenFetches < new_min_time) 
	{
		mMinTimeBetweenFetches=new_min_time;  //HACK!  See above.
	}
	
	if (gDisconnected ||
		(mFetchCount > max_concurrent_fetches) ||
		(mFetchTimer.getElapsedTimeF32() < mMinTimeBetweenFetches))
	{
		return; // just bail if we are disconnected
	}	

	U32 item_count=0;
	U32 folder_count=0;
	U32 max_batch_size=5;

	U32 sort_order = gSavedSettings.getU32(LLInventoryPanel::DEFAULT_SORT_ORDER) & 0x1;

	uuid_vec_t recursive_cats;

	LLSD folder_request_body;
	LLSD folder_request_body_lib;
	LLSD item_request_body;
	LLSD item_request_body_lib;

	while (!mFetchQueue.empty() 
			&& (item_count + folder_count) < max_batch_size)
	{
		const FetchQueueInfo& fetch_info = mFetchQueue.front();
		if (fetch_info.mIsCategory)
		{
			const LLUUID &cat_id = fetch_info.mUUID;
			if (cat_id.isNull()) //DEV-17797
			{
				LLSD folder_sd;
				folder_sd["folder_id"]		= LLUUID::null.asString();
				folder_sd["owner_id"]		= gAgent.getID();
				folder_sd["sort_order"]		= (LLSD::Integer)sort_order;
				folder_sd["fetch_folders"]	= (LLSD::Boolean)FALSE;
				folder_sd["fetch_items"]	= (LLSD::Boolean)TRUE;
				folder_request_body["folders"].append(folder_sd);
				folder_count++;
			}
			else
			{
				const LLViewerInventoryCategory* cat = gInventory.getCategory(cat_id);
		
				if (cat)
				{
					if (LLViewerInventoryCategory::VERSION_UNKNOWN == cat->getVersion())
					{
						LLSD folder_sd;
						folder_sd["folder_id"]		= cat->getUUID();
						folder_sd["owner_id"]		= cat->getOwnerID();
						folder_sd["sort_order"]		= (LLSD::Integer)sort_order;
						folder_sd["fetch_folders"]	= TRUE; //(LLSD::Boolean)sFullFetchStarted;
						folder_sd["fetch_items"]	= (LLSD::Boolean)TRUE;
				    
						if (ALEXANDRIA_LINDEN_ID == cat->getOwnerID())
							folder_request_body_lib["folders"].append(folder_sd);
						else
							folder_request_body["folders"].append(folder_sd);
						folder_count++;
					}
					// May already have this folder, but append child folders to list.
					if (fetch_info.mRecursive)
					{	
						LLInventoryModel::cat_array_t* categories;
						LLInventoryModel::item_array_t* items;
						gInventory.getDirectDescendentsOf(cat->getUUID(), categories, items);
						for (LLInventoryModel::cat_array_t::const_iterator it = categories->begin();
							 it != categories->end();
							 ++it)
						{
							mFetchQueue.push_back(FetchQueueInfo((*it)->getUUID(), fetch_info.mRecursive));
						}
					}
				}
			}
			if (fetch_info.mRecursive)
				recursive_cats.push_back(cat_id);
		}
		else
		{
			LLViewerInventoryItem* itemp = gInventory.getItem(fetch_info.mUUID);
			if (itemp)
			{
				LLSD item_sd;
				item_sd["owner_id"] = itemp->getPermissions().getOwner();
				item_sd["item_id"] = itemp->getUUID();
				if (itemp->getPermissions().getOwner() == gAgent.getID())
				{
					item_request_body.append(item_sd);
				}
				else
				{
					item_request_body_lib.append(item_sd);
				}
				//itemp->fetchFromServer();
				item_count++;
			}
		}

		mFetchQueue.pop_front();
	}
		
	if (item_count + folder_count > 0)
	{
		if (folder_count)
		{
			std::string url = region->getCapability("FetchInventoryDescendents2");   
			mFetchCount++;
			if (folder_request_body["folders"].size())
			{
				LLInventoryModelFetchDescendentsResponder *fetcher = new LLInventoryModelFetchDescendentsResponder(folder_request_body, recursive_cats);
				LLHTTPClient::post(url, folder_request_body, fetcher, 300.0);
			}
			if (folder_request_body_lib["folders"].size())
			{
				std::string url_lib = gAgent.getRegion()->getCapability("FetchLibDescendents2");

				LLInventoryModelFetchDescendentsResponder *fetcher = new LLInventoryModelFetchDescendentsResponder(folder_request_body_lib, recursive_cats);
				LLHTTPClient::post(url_lib, folder_request_body_lib, fetcher, 300.0);
			}
		}
		if (item_count)
		{
			std::string url;

			if (item_request_body.size())
			{
				mFetchCount++;
				url = region->getCapability("FetchInventory2");
				if (!url.empty())
				{
					LLSD body;
					body["agent_id"]	= gAgent.getID();
					body["items"] = item_request_body;

					LLHTTPClient::post(url, body, new LLInventoryModelFetchItemResponder(body));
				}
				//else
				//{
				//	LLMessageSystem* msg = gMessageSystem;
				//	msg->newMessage("FetchInventory");
				//	msg->nextBlock("AgentData");
				//	msg->addUUID("AgentID", gAgent.getID());
				//	msg->addUUID("SessionID", gAgent.getSessionID());
				//	msg->nextBlock("InventoryData");
				//	msg->addUUID("OwnerID", mPermissions.getOwner());
				//	msg->addUUID("ItemID", mUUID);
				//	gAgent.sendReliableMessage();
				//}
			}

			if (item_request_body_lib.size())
			{
				mFetchCount++;

				url = region->getCapability("FetchLib2");
				if (!url.empty())
				{
					LLSD body;
					body["agent_id"]	= gAgent.getID();
					body["items"] = item_request_body_lib;

					LLHTTPClient::post(url, body, new LLInventoryModelFetchItemResponder(body));
				}
			}
		}
		mFetchTimer.reset();
	}

	else if (isBulkFetchProcessingComplete())
	{
		setAllFoldersFetched();
	}
}

bool LLInventoryModelBackgroundFetch::fetchQueueContainsNoDescendentsOf(const LLUUID& cat_id) const
{
	for (fetch_queue_t::const_iterator it = mFetchQueue.begin();
		 it != mFetchQueue.end(); ++it)
	{
		const LLUUID& fetch_id = (*it).mUUID;
		if (gInventory.isObjectDescendentOf(fetch_id, cat_id))
			return false;
	}
	return true;
}


