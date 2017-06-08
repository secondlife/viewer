/** 
 * @file llinventorymodelbackgroundfetch.cpp
 * @brief Implementation of background fetching of inventory.
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2014, Linden Research, Inc.
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
#include "llhttpconstants.h"
#include "bufferarray.h"
#include "bufferstream.h"
#include "llcorehttputil.h"

// History (may be apocryphal)
//
// Around V2, an HTTP inventory download mechanism was added
// along with inventory LINK items referencing other inventory
// items.  As part of this, at login, the entire inventory
// structure is downloaded 'in the background' using the
// backgroundFetch()/bulkFetch() methods.  The UDP path can
// still be used and is found in the 'DEPRECATED OLD CODE'
// section.
//
// The old UDP path implemented a throttle that adapted
// itself during running.  The mechanism survived info HTTP
// somewhat but was pinned to poll the HTTP plumbing at
// 0.5S intervals.  The reasons for this particular value
// have been lost.  It's possible to switch between UDP
// and HTTP while this is happening but there may be
// surprises in what happens in that case.
//
// Conversion to llcorehttp reduced the number of connections
// used but batches more data and queues more requests (but
// doesn't due pipelining due to libcurl restrictions).  The
// poll interval above was re-examined and reduced to get
// inventory into the viewer more quickly.
//
// Possible future work:
//
// * Don't download the entire heirarchy in one go (which
//   might have been how V1 worked).  Implications for
//   links (which may not have a valid target) and search
//   which would then be missing data.
//
// * Review the download rate throttling.  Slow then fast?
//   Detect bandwidth usage and speed up when it drops?
//
// * An error on a fetch could be due to one item in the batch.
//   If the batch were broken up, perhaps more of the inventory
//   would download.  (Handwave here, not certain this is an
//   issue in practice.)
//
// * Conversion to AISv3.
//


namespace
{

///----------------------------------------------------------------------------
/// Class <anonymous>::BGItemHttpHandler
///----------------------------------------------------------------------------

//
// Http request handler class for single inventory item requests.
//
// We'll use a handler-per-request pattern here rather than
// a shared handler.  Mainly convenient as this was converted
// from a Responder class model.
//
// Derives from and is identical to the normal FetchItemHttpHandler
// except that:  1) it uses the background request object which is
// updated more slowly than the foreground and 2) keeps a count of
// active requests on the LLInventoryModelBackgroundFetch object
// to indicate outstanding operations are in-flight.
//
class BGItemHttpHandler : public LLInventoryModel::FetchItemHttpHandler
{
	LOG_CLASS(BGItemHttpHandler);
	
public:
	BGItemHttpHandler(const LLSD & request_sd)
		: LLInventoryModel::FetchItemHttpHandler(request_sd)
		{
			LLInventoryModelBackgroundFetch::instance().incrFetchCount(1);
		}

	virtual ~BGItemHttpHandler()
		{
			LLInventoryModelBackgroundFetch::instance().incrFetchCount(-1);
		}

protected:
	BGItemHttpHandler(const BGItemHttpHandler &);				// Not defined
	void operator=(const BGItemHttpHandler &);					// Not defined
};


///----------------------------------------------------------------------------
/// Class <anonymous>::BGFolderHttpHandler
///----------------------------------------------------------------------------

// Http request handler class for folders.
//
// Handler for FetchInventoryDescendents2 and FetchLibDescendents2
// caps requests for folders.
//
class BGFolderHttpHandler : public LLCore::HttpHandler
{
	LOG_CLASS(BGFolderHttpHandler);
	
public:
	BGFolderHttpHandler(const LLSD & request_sd, const uuid_vec_t & recursive_cats)
		: LLCore::HttpHandler(),
		  mRequestSD(request_sd),
		  mRecursiveCatUUIDs(recursive_cats)
		{
			LLInventoryModelBackgroundFetch::instance().incrFetchCount(1);
		}

	virtual ~BGFolderHttpHandler()
		{
			LLInventoryModelBackgroundFetch::instance().incrFetchCount(-1);
		}
	
protected:
	BGFolderHttpHandler(const BGFolderHttpHandler &);			// Not defined
	void operator=(const BGFolderHttpHandler &);				// Not defined

public:
	virtual void onCompleted(LLCore::HttpHandle handle, LLCore::HttpResponse * response);

	bool getIsRecursive(const LLUUID & cat_id) const;

private:
	void processData(LLSD & body, LLCore::HttpResponse * response);
	void processFailure(LLCore::HttpStatus status, LLCore::HttpResponse * response);
	void processFailure(const char * const reason, LLCore::HttpResponse * response);

private:
	LLSD mRequestSD;
	const uuid_vec_t mRecursiveCatUUIDs; // hack for storing away which cat fetches are recursive
};


const char * const LOG_INV("Inventory");

} // end of namespace anonymous


///----------------------------------------------------------------------------
/// Class LLInventoryModelBackgroundFetch
///----------------------------------------------------------------------------

LLInventoryModelBackgroundFetch::LLInventoryModelBackgroundFetch():
	mBackgroundFetchActive(FALSE),
	mFolderFetchActive(false),
	mFetchCount(0),
	mAllFoldersFetched(FALSE),
	mRecursiveInventoryFetchStarted(FALSE),
	mRecursiveLibraryFetchStarted(FALSE),
	mMinTimeBetweenFetches(0.3f)
{}

LLInventoryModelBackgroundFetch::~LLInventoryModelBackgroundFetch()
{}

bool LLInventoryModelBackgroundFetch::isBulkFetchProcessingComplete() const
{
	return mFetchQueue.empty() && mFetchCount <= 0;
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
	return inventoryFetchStarted() && ! inventoryFetchCompleted();
}

bool LLInventoryModelBackgroundFetch::isEverythingFetched() const
{
	return mAllFoldersFetched;
}

BOOL LLInventoryModelBackgroundFetch::folderFetchActive() const
{
	return mFolderFetchActive;
}

void LLInventoryModelBackgroundFetch::addRequestAtFront(const LLUUID & id, BOOL recursive, bool is_category)
{
	mFetchQueue.push_front(FetchQueueInfo(id, recursive, is_category));
}

void LLInventoryModelBackgroundFetch::addRequestAtBack(const LLUUID & id, BOOL recursive, bool is_category)
{
	mFetchQueue.push_back(FetchQueueInfo(id, recursive, is_category));
}

void LLInventoryModelBackgroundFetch::start(const LLUUID& id, BOOL recursive)
{
	LLViewerInventoryCategory * cat(gInventory.getCategory(id));

	if (cat || (id.isNull() && ! isEverythingFetched()))
	{
		// it's a folder, do a bulk fetch
		LL_DEBUGS(LOG_INV) << "Start fetching category: " << id << ", recursive: " << recursive << LL_ENDL;

		mBackgroundFetchActive = TRUE;
		mFolderFetchActive = true;
		if (id.isNull())
		{
			if (! mRecursiveInventoryFetchStarted)
			{
				mRecursiveInventoryFetchStarted |= recursive;
				mFetchQueue.push_back(FetchQueueInfo(gInventory.getRootFolderID(), recursive));
				gIdleCallbacks.addFunction(&LLInventoryModelBackgroundFetch::backgroundFetchCB, NULL);
			}
			if (! mRecursiveLibraryFetchStarted)
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
	else if (LLViewerInventoryItem * itemp = gInventory.getItem(id))
	{
		if (! itemp->mIsComplete && (mFetchQueue.empty() || mFetchQueue.front().mUUID != id))
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
		//LL_INFOS(LOG_INV) << "All folders fetched, validating" << LL_ENDL;
		//gInventory.validate();
	}
	mFolderFetchActive = false;
	mBackgroundFetchActive = false;
	LL_INFOS(LOG_INV) << "Inventory background fetch completed" << LL_ENDL;
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
		bulkFetch();
	}
}

void LLInventoryModelBackgroundFetch::incrFetchCount(S32 fetching) 
{  
	mFetchCount += fetching; 
	if (mFetchCount < 0)
	{
		LL_WARNS_ONCE(LOG_INV) << "Inventory fetch count fell below zero (0)." << LL_ENDL;
		mFetchCount = 0; 
	}
}

static LLTrace::BlockTimerStatHandle FTM_BULK_FETCH("Bulk Fetch");

// Bundle up a bunch of requests to send all at once.
void LLInventoryModelBackgroundFetch::bulkFetch()
{
	LL_RECORD_BLOCK_TIME(FTM_BULK_FETCH);
	//Background fetch is called from gIdleCallbacks in a loop until background fetch is stopped.
	//If there are items in mFetchQueue, we want to check the time since the last bulkFetch was 
	//sent.  If it exceeds our retry time, go ahead and fire off another batch.  
	LLViewerRegion * region(gAgent.getRegion());
	if (! region || gDisconnected)
	{
		return;
	}

	// *TODO:  These values could be tweaked at runtime to effect
	// a fast/slow fetch throttle.  Once login is complete and the scene
	// is mostly loaded, we could turn up the throttle and fill missing
	// inventory more quickly.
	static const U32 max_batch_size(10);
	static const S32 max_concurrent_fetches(12);		// Outstanding requests, not connections
	static const F32 new_min_time(0.05f);		// *HACK:  Clean this up when old code goes away entirely.
	
	mMinTimeBetweenFetches = new_min_time;
	if (mMinTimeBetweenFetches < new_min_time) 
	{
		mMinTimeBetweenFetches = new_min_time;  // *HACK:  See above.
	}

	if (mFetchCount)
	{
		// Process completed background HTTP requests
		gInventory.handleResponses(false);
		// Just processed a bunch of items.
		// Note: do we really need notifyObservers() here?
		// OnIdle it will be called anyway due to Add flag for processed item.
		// It seems like in some cases we are updaiting on fail (no flag),
		// but is there anything to update?
		gInventory.notifyObservers();
	}
	
	if ((mFetchCount > max_concurrent_fetches) ||
		(mFetchTimer.getElapsedTimeF32() < mMinTimeBetweenFetches))
	{
		return;
	}

	U32 item_count(0);
	U32 folder_count(0);

	const U32 sort_order(gSavedSettings.getU32(LLInventoryPanel::DEFAULT_SORT_ORDER) & 0x1);

	// *TODO:  Think I'd like to get a shared pointer to this and share it
	// among all the folder requests.
	uuid_vec_t recursive_cats;

	LLSD folder_request_body;
	LLSD folder_request_body_lib;
	LLSD item_request_body;
	LLSD item_request_body_lib;

	while (! mFetchQueue.empty() 
			&& (item_count + folder_count) < max_batch_size)
	{
		const FetchQueueInfo & fetch_info(mFetchQueue.front());
		if (fetch_info.mIsCategory)
		{
			const LLUUID & cat_id(fetch_info.mUUID);
			if (cat_id.isNull()) //DEV-17797
			{
				LLSD folder_sd;
				folder_sd["folder_id"]		= LLUUID::null.asString();
				folder_sd["owner_id"]		= gAgent.getID();
				folder_sd["sort_order"]		= LLSD::Integer(sort_order);
				folder_sd["fetch_folders"]	= LLSD::Boolean(FALSE);
				folder_sd["fetch_items"]	= LLSD::Boolean(TRUE);
				folder_request_body["folders"].append(folder_sd);
				folder_count++;
			}
			else
			{
				const LLViewerInventoryCategory * cat(gInventory.getCategory(cat_id));
		
				if (cat)
				{
					if (LLViewerInventoryCategory::VERSION_UNKNOWN == cat->getVersion())
					{
						LLSD folder_sd;
						folder_sd["folder_id"]		= cat->getUUID();
						folder_sd["owner_id"]		= cat->getOwnerID();
						folder_sd["sort_order"]		= LLSD::Integer(sort_order);
						folder_sd["fetch_folders"]	= LLSD::Boolean(TRUE); //(LLSD::Boolean)sFullFetchStarted;
						folder_sd["fetch_items"]	= LLSD::Boolean(TRUE);
				    
						if (ALEXANDRIA_LINDEN_ID == cat->getOwnerID())
						{
							folder_request_body_lib["folders"].append(folder_sd);
						}
						else
						{
							folder_request_body["folders"].append(folder_sd);
						}
						folder_count++;
					}

					// May already have this folder, but append child folders to list.
					if (fetch_info.mRecursive)
					{	
						LLInventoryModel::cat_array_t * categories(NULL);
						LLInventoryModel::item_array_t * items(NULL);
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
			{
				recursive_cats.push_back(cat_id);
			}
		}
		else
		{
			LLViewerInventoryItem * itemp(gInventory.getItem(fetch_info.mUUID));

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

	// Issue HTTP POST requests to fetch folders and items
	
	if (item_count + folder_count > 0)
	{
		if (folder_count)
		{
			if (folder_request_body["folders"].size())
			{
				const std::string url(region->getCapability("FetchInventoryDescendents2"));

				if (! url.empty())
				{
                    LLCore::HttpHandler::ptr_t  handler(new BGFolderHttpHandler(folder_request_body, recursive_cats));
					gInventory.requestPost(false, url, folder_request_body, handler, "Inventory Folder");
				}
			}
			
			if (folder_request_body_lib["folders"].size())
			{
				const std::string url(region->getCapability("FetchLibDescendents2"));

				if (! url.empty())
				{
                    LLCore::HttpHandler::ptr_t  handler(new BGFolderHttpHandler(folder_request_body_lib, recursive_cats));
					gInventory.requestPost(false, url, folder_request_body_lib, handler, "Library Folder");
				}
			}
		} // if (folder_count)

		if (item_count)
		{
			if (item_request_body.size())
			{
				const std::string url(region->getCapability("FetchInventory2"));

				if (! url.empty())
				{
					LLSD body;
					body["items"] = item_request_body;
                    LLCore::HttpHandler::ptr_t  handler(new BGItemHttpHandler(body));
					gInventory.requestPost(false, url, body, handler, "Inventory Item");
				}
			}

			if (item_request_body_lib.size())
			{
				const std::string url(region->getCapability("FetchLib2"));

				if (! url.empty())
				{
					LLSD body;
					body["items"] = item_request_body_lib;
                    LLCore::HttpHandler::ptr_t handler(new BGItemHttpHandler(body));
					gInventory.requestPost(false, url, body, handler, "Library Item");
				}
			}
		} // if (item_count)
		
		mFetchTimer.reset();
	}
	else if (isBulkFetchProcessingComplete())
	{
		setAllFoldersFetched();
	}
}

bool LLInventoryModelBackgroundFetch::fetchQueueContainsNoDescendentsOf(const LLUUID & cat_id) const
{
	for (fetch_queue_t::const_iterator it = mFetchQueue.begin();
		 it != mFetchQueue.end();
		 ++it)
	{
		const LLUUID & fetch_id = (*it).mUUID;
		if (gInventory.isObjectDescendentOf(fetch_id, cat_id))
			return false;
	}
	return true;
}


namespace
{

///----------------------------------------------------------------------------
/// Class <anonymous>::BGFolderHttpHandler
///----------------------------------------------------------------------------

void BGFolderHttpHandler::onCompleted(LLCore::HttpHandle handle, LLCore::HttpResponse * response)
{
	do  	// Single-pass do-while used for common exit handling
	{
		LLCore::HttpStatus status(response->getStatus());
		// status = LLCore::HttpStatus(404);				// Dev tool to force error handling
		if (! status)
		{
			processFailure(status, response);
			break;			// Goto common exit
		}

		// Response body should be present.
		LLCore::BufferArray * body(response->getBody());
		// body = NULL;									// Dev tool to force error handling
		if (! body || ! body->size())
		{
			LL_WARNS(LOG_INV) << "Missing data in inventory folder query." << LL_ENDL;
			processFailure("HTTP response missing expected body", response);
			break;			// Goto common exit
		}

		// Could test 'Content-Type' header but probably unreliable.

		// Convert response to LLSD
		// body->write(0, "Garbage Response", 16);		// Dev tool to force error handling
		LLSD body_llsd;
		if (! LLCoreHttpUtil::responseToLLSD(response, true, body_llsd))
		{
			// INFOS-level logging will occur on the parsed failure
			processFailure("HTTP response contained malformed LLSD", response);
			break;			// goto common exit
		}

		// Expect top-level structure to be a map
		// body_llsd = LLSD::emptyArray();				// Dev tool to force error handling
		if (! body_llsd.isMap())
		{
			processFailure("LLSD response not a map", response);
			break;			// goto common exit
		}

		// Check for 200-with-error failures
		//
		// See comments in llinventorymodel.cpp about this mode of error.
		//
		// body_llsd["error"] = LLSD::emptyMap();		// Dev tool to force error handling
		// body_llsd["error"]["identifier"] = "Development";
		// body_llsd["error"]["message"] = "You left development code in the viewer";
		if (body_llsd.has("error"))
		{
			processFailure("Inventory application error (200-with-error)", response);
			break;			// goto common exit
		}

		// Okay, process data if possible
		processData(body_llsd, response);
	}
	while (false);
}


void BGFolderHttpHandler::processData(LLSD & content, LLCore::HttpResponse * response)
{
	LLInventoryModelBackgroundFetch * fetcher(LLInventoryModelBackgroundFetch::getInstance());

	// API V2 and earlier should probably be testing for "error" map
	// in response as an application-level error.

	// Instead, we assume success and attempt to extract information.
	if (content.has("folders"))	
	{
		LLSD folders(content["folders"]);
		
		for (LLSD::array_const_iterator folder_it = folders.beginArray();
			folder_it != folders.endArray();
			++folder_it)
		{	
			LLSD folder_sd(*folder_it);

			//LLUUID agent_id = folder_sd["agent_id"];

			//if(agent_id != gAgent.getID())	//This should never happen.
			//{
			//	LL_WARNS(LOG_INV) << "Got a UpdateInventoryItem for the wrong agent."
			//			<< LL_ENDL;
			//	break;
			//}

			LLUUID parent_id(folder_sd["folder_id"].asUUID());
			LLUUID owner_id(folder_sd["owner_id"].asUUID());
			S32    version(folder_sd["version"].asInteger());
			S32    descendents(folder_sd["descendents"].asInteger());
			LLPointer<LLViewerInventoryCategory> tcategory = new LLViewerInventoryCategory(owner_id);

            if (parent_id.isNull())
            {
				LLSD items(folder_sd["items"]);
			    LLPointer<LLViewerInventoryItem> titem = new LLViewerInventoryItem;
				
			    for (LLSD::array_const_iterator item_it = items.beginArray();
				    item_it != items.endArray();
				    ++item_it)
			    {	
                    const LLUUID lost_uuid(gInventory.findCategoryUUIDForType(LLFolderType::FT_LOST_AND_FOUND));

                    if (lost_uuid.notNull())
                    {
				        LLSD item(*item_it);

				        titem->unpackMessage(item);
				
                        LLInventoryModel::update_list_t update;
                        LLInventoryModel::LLCategoryUpdate new_folder(lost_uuid, 1);
                        update.push_back(new_folder);
                        gInventory.accountForUpdate(update);

                        titem->setParent(lost_uuid);
                        titem->updateParentOnServer(FALSE);
                        gInventory.updateItem(titem);
                    }
                }
            }

	        LLViewerInventoryCategory * pcat(gInventory.getCategory(parent_id));
			if (! pcat)
			{
				continue;
			}

			LLSD categories(folder_sd["categories"]);
			for (LLSD::array_const_iterator category_it = categories.beginArray();
				category_it != categories.endArray();
				++category_it)
			{	
				LLSD category(*category_it);
				tcategory->fromLLSD(category); 
				
				const bool recursive(getIsRecursive(tcategory->getUUID()));
				if (recursive)
				{
					fetcher->addRequestAtBack(tcategory->getUUID(), recursive, true);
				}
				else if (! gInventory.isCategoryComplete(tcategory->getUUID()))
				{
					gInventory.updateCategory(tcategory);
				}
			}

			LLSD items(folder_sd["items"]);
			LLPointer<LLViewerInventoryItem> titem = new LLViewerInventoryItem;
			for (LLSD::array_const_iterator item_it = items.beginArray();
				 item_it != items.endArray();
				 ++item_it)
			{	
				LLSD item(*item_it);
				titem->unpackMessage(item);
				
				gInventory.updateItem(titem);
			}

			// Set version and descendentcount according to message.
			LLViewerInventoryCategory * cat(gInventory.getCategory(parent_id));
			if (cat)
			{
				cat->setVersion(version);
				cat->setDescendentCount(descendents);
				cat->determineFolderType();
			}
		}
	}
		
	if (content.has("bad_folders"))
	{
		LLSD bad_folders(content["bad_folders"]);
		for (LLSD::array_const_iterator folder_it = bad_folders.beginArray();
			 folder_it != bad_folders.endArray();
			 ++folder_it)
		{
			// *TODO: Stop copying data [ed:  this isn't copying data]
			LLSD folder_sd(*folder_it);
			
			// These folders failed on the dataserver.  We probably don't want to retry them.
			LL_WARNS(LOG_INV) << "Folder " << folder_sd["folder_id"].asString() 
							  << "Error: " << folder_sd["error"].asString() << LL_ENDL;
		}
	}
	
	if (fetcher->isBulkFetchProcessingComplete())
	{
		fetcher->setAllFoldersFetched();
	}
}


void BGFolderHttpHandler::processFailure(LLCore::HttpStatus status, LLCore::HttpResponse * response)
{
	const std::string & ct(response->getContentType());
	LL_WARNS(LOG_INV) << "Inventory folder fetch failure\n"
					  << "[Status: " << status.toTerseString() << "]\n"
					  << "[Reason: " << status.toString() << "]\n"
					  << "[Content-type: " << ct << "]\n"
					  << "[Content (abridged): "
					  << LLCoreHttpUtil::responseToString(response) << "]" << LL_ENDL;

	// Could use a 404 test here to try to detect revoked caps...
	
	// This was originally the request retry logic for the inventory
	// request which tested on HTTP_INTERNAL_ERROR status.  This
	// retry logic was unbounded and lacked discrimination as to the
	// cause of the retry.  The new http library should be doing
	// adquately on retries but I want to keep the structure of a
	// retry for reference.
	LLInventoryModelBackgroundFetch *fetcher = LLInventoryModelBackgroundFetch::getInstance();
	if (false)
	{
		// timed out or curl failure
		for (LLSD::array_const_iterator folder_it = mRequestSD["folders"].beginArray();
			 folder_it != mRequestSD["folders"].endArray();
			 ++folder_it)
		{
			LLSD folder_sd(*folder_it);
			LLUUID folder_id(folder_sd["folder_id"].asUUID());
			const BOOL recursive = getIsRecursive(folder_id);
			fetcher->addRequestAtFront(folder_id, recursive, true);
		}
	}
	else
	{
		if (fetcher->isBulkFetchProcessingComplete())
		{
			fetcher->setAllFoldersFetched();
		}
	}
}


void BGFolderHttpHandler::processFailure(const char * const reason, LLCore::HttpResponse * response)
{
	LL_WARNS(LOG_INV) << "Inventory folder fetch failure\n"
					  << "[Status: internal error]\n"
					  << "[Reason: " << reason << "]\n"
					  << "[Content (abridged): "
					  << LLCoreHttpUtil::responseToString(response) << "]" << LL_ENDL;

	// Reverse of previous processFailure() method, this is invoked
	// when response structure is found to be invalid.  Original
	// always re-issued the request (without limit).  This does
	// the same but be aware that this may be a source of problems.
	// Philosophy is that inventory folders are so essential to
	// operation that this is a reasonable action.
	LLInventoryModelBackgroundFetch *fetcher = LLInventoryModelBackgroundFetch::getInstance();
	if (true)
	{
		for (LLSD::array_const_iterator folder_it = mRequestSD["folders"].beginArray();
			 folder_it != mRequestSD["folders"].endArray();
			 ++folder_it)
		{
			LLSD folder_sd(*folder_it);
			LLUUID folder_id(folder_sd["folder_id"].asUUID());
			const BOOL recursive = getIsRecursive(folder_id);
			fetcher->addRequestAtFront(folder_id, recursive, true);
		}
	}
	else
	{
		if (fetcher->isBulkFetchProcessingComplete())
		{
			fetcher->setAllFoldersFetched();
		}
	}
}


bool BGFolderHttpHandler::getIsRecursive(const LLUUID & cat_id) const
{
	return std::find(mRecursiveCatUUIDs.begin(), mRecursiveCatUUIDs.end(), cat_id) != mRecursiveCatUUIDs.end();
}

///----------------------------------------------------------------------------
/// Class <anonymous>::BGItemHttpHandler
///----------------------------------------------------------------------------

// Nothing to implement here.  All ctor/dtor changes.

} // end namespace anonymous
