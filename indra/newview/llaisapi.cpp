/** 
 * @file llaisapi.cpp
 * @brief classes and functions for interfacing with the v3+ ais inventory service. 
 *
 * $LicenseInfo:firstyear=2013&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2013, Linden Research, Inc.
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
 *
 */

#include "llviewerprecompiledheaders.h"
#include "llaisapi.h"

#include "llagent.h"
#include "llcallbacklist.h"
#include "llinventorymodel.h"
#include "llsdutil.h"
#include "llviewerregion.h"

///----------------------------------------------------------------------------
/// Classes for AISv3 support.
///----------------------------------------------------------------------------

// AISCommand - base class for retry-able HTTP requests using the AISv3 cap.
AISCommand::AISCommand(LLPointer<LLInventoryCallback> callback):
	mCallback(callback)
{
	mRetryPolicy = new LLAdaptiveRetryPolicy(1.0, 32.0, 2.0, 10);
}

void AISCommand::run_command()
{
	mCommandFunc();
}

void AISCommand::setCommandFunc(command_func_type command_func)
{
	mCommandFunc = command_func;
}
	
// virtual
bool AISCommand::getResponseUUID(const LLSD& content, LLUUID& id)
{
	return false;
}
	
/* virtual */
void AISCommand::httpSuccess()
{
	// Command func holds a reference to self, need to release it
	// after a success or final failure.
	setCommandFunc(no_op);
		
	const LLSD& content = getContent();
	if (!content.isMap())
	{
		failureResult(HTTP_INTERNAL_ERROR, "Malformed response contents", content);
		return;
	}
	mRetryPolicy->onSuccess();
		
	gInventory.onAISUpdateReceived("AISCommand", content);

	if (mCallback)
	{
		LLUUID item_id; // will default to null if parse fails.
		getResponseUUID(content,item_id);
		mCallback->fire(item_id);
	}
}

/*virtual*/
void AISCommand::httpFailure()
{
	const LLSD& content = getContent();
	S32 status = getStatus();
	const std::string& reason = getReason();
	const LLSD& headers = getResponseHeaders();
	if (!content.isMap())
	{
		LL_DEBUGS("Inventory") << "Malformed response contents " << content
							   << " status " << status << " reason " << reason << llendl;
	}
	else
	{
		LL_DEBUGS("Inventory") << "failed with content: " << ll_pretty_print_sd(content)
							   << " status " << status << " reason " << reason << llendl;
	}
	mRetryPolicy->onFailure(status, headers);
	F32 seconds_to_wait;
	if (mRetryPolicy->shouldRetry(seconds_to_wait))
	{
		doAfterInterval(boost::bind(&AISCommand::run_command,this),seconds_to_wait);
	}
	else
	{
		// Command func holds a reference to self, need to release it
		// after a success or final failure.
		setCommandFunc(no_op);
	}
}

//static
bool AISCommand::getCap(std::string& cap)
{
	if (gAgent.getRegion())
	{
		cap = gAgent.getRegion()->getCapability("InventoryAPIv3");
	}
	if (!cap.empty())
	{
		return true;
	}
	return false;
}

RemoveItemCommand::RemoveItemCommand(const LLUUID& item_id,
									 LLPointer<LLInventoryCallback> callback):
	AISCommand(callback)
{
	std::string cap;
	if (!getCap(cap))
	{
		llwarns << "No cap found" << llendl;
		return;
	}
	std::string url = cap + std::string("/item/") + item_id.asString();
	LL_DEBUGS("Inventory") << "url: " << url << llendl;
	LLHTTPClient::ResponderPtr responder = this;
	LLSD headers;
	F32 timeout = HTTP_REQUEST_EXPIRY_SECS;
	command_func_type cmd = boost::bind(&LLHTTPClient::del, url, responder, headers, timeout);
	setCommandFunc(cmd);
}

RemoveCategoryCommand::RemoveCategoryCommand(const LLUUID& item_id,
											 LLPointer<LLInventoryCallback> callback):
	AISCommand(callback)
{
	std::string cap;
	if (!getCap(cap))
	{
		llwarns << "No cap found" << llendl;
		return;
	}
	std::string url = cap + std::string("/category/") + item_id.asString();
	LL_DEBUGS("Inventory") << "url: " << url << llendl;
	LLHTTPClient::ResponderPtr responder = this;
	LLSD headers;
	F32 timeout = HTTP_REQUEST_EXPIRY_SECS;
	command_func_type cmd = boost::bind(&LLHTTPClient::del, url, responder, headers, timeout);
	setCommandFunc(cmd);
}

PurgeDescendentsCommand::PurgeDescendentsCommand(const LLUUID& item_id,
												 LLPointer<LLInventoryCallback> callback):
	AISCommand(callback)
{
	std::string cap;
	if (!getCap(cap))
	{
		llwarns << "No cap found" << llendl;
		return;
	}
	std::string url = cap + std::string("/category/") + item_id.asString() + "/children";
	LL_DEBUGS("Inventory") << "url: " << url << llendl;
	LLCurl::ResponderPtr responder = this;
	LLSD headers;
	F32 timeout = HTTP_REQUEST_EXPIRY_SECS;
	command_func_type cmd = boost::bind(&LLHTTPClient::del, url, responder, headers, timeout);
	setCommandFunc(cmd);
}

UpdateItemCommand::UpdateItemCommand(const LLUUID& item_id,
									 const LLSD& updates,
									 LLPointer<LLInventoryCallback> callback):
	mUpdates(updates),
	AISCommand(callback)
{
	std::string cap;
	if (!getCap(cap))
	{
		llwarns << "No cap found" << llendl;
		return;
	}
	std::string url = cap + std::string("/item/") + item_id.asString();
	LL_DEBUGS("Inventory") << "url: " << url << llendl;
	LL_DEBUGS("Inventory") << "request: " << ll_pretty_print_sd(mUpdates) << llendl;
	LLCurl::ResponderPtr responder = this;
	LLSD headers;
	headers["Content-Type"] = "application/llsd+xml";
	F32 timeout = HTTP_REQUEST_EXPIRY_SECS;
	command_func_type cmd = boost::bind(&LLHTTPClient::patch, url, mUpdates, responder, headers, timeout);
	setCommandFunc(cmd);
}

UpdateCategoryCommand::UpdateCategoryCommand(const LLUUID& item_id,
											 const LLSD& updates,
											 LLPointer<LLInventoryCallback> callback):
	mUpdates(updates),
	AISCommand(callback)
{
	std::string cap;
	if (!getCap(cap))
	{
		llwarns << "No cap found" << llendl;
		return;
	}
	std::string url = cap + std::string("/category/") + item_id.asString();
	LL_DEBUGS("Inventory") << "url: " << url << llendl;
	LLCurl::ResponderPtr responder = this;
	LLSD headers;
	headers["Content-Type"] = "application/llsd+xml";
	F32 timeout = HTTP_REQUEST_EXPIRY_SECS;
	command_func_type cmd = boost::bind(&LLHTTPClient::patch, url, mUpdates, responder, headers, timeout);
	setCommandFunc(cmd);
}

SlamFolderCommand::SlamFolderCommand(const LLUUID& folder_id, const LLSD& contents, LLPointer<LLInventoryCallback> callback):
	mContents(contents),
	AISCommand(callback)
{
	std::string cap;
	if (!getCap(cap))
	{
		llwarns << "No cap found" << llendl;
		return;
	}
	LLUUID tid;
	tid.generate();
	std::string url = cap + std::string("/category/") + folder_id.asString() + "/links?tid=" + tid.asString();
	llinfos << url << llendl;
	LLCurl::ResponderPtr responder = this;
	LLSD headers;
	headers["Content-Type"] = "application/llsd+xml";
	F32 timeout = HTTP_REQUEST_EXPIRY_SECS;
	command_func_type cmd = boost::bind(&LLHTTPClient::put, url, mContents, responder, headers, timeout);
	setCommandFunc(cmd);
}

AISUpdate::AISUpdate(const LLSD& update)
{
	parseUpdate(update);
}

void AISUpdate::parseUpdate(const LLSD& update)
{
	// parse _categories_removed -> mObjectsDeleted
	uuid_vec_t cat_ids;
	parseUUIDArray(update,"_categories_removed",cat_ids);
	for (uuid_vec_t::const_iterator it = cat_ids.begin();
		 it != cat_ids.end(); ++it)
	{
		LLViewerInventoryCategory *cat = gInventory.getCategory(*it);
		mCatDeltas[cat->getParentUUID()]--;
		mObjectsDeleted.insert(*it);
	}

	// parse _categories_items_removed -> mObjectsDeleted
	uuid_vec_t item_ids;
	parseUUIDArray(update,"_category_items_removed",item_ids);
	for (uuid_vec_t::const_iterator it = item_ids.begin();
		 it != item_ids.end(); ++it)
	{
		LLViewerInventoryItem *item = gInventory.getItem(*it);
		mCatDeltas[item->getParentUUID()]--;
		mObjectsDeleted.insert(*it);
	}

	// parse _broken_links_removed -> mObjectsDeleted
	uuid_vec_t broken_link_ids;
	parseUUIDArray(update,"_broken_links_removed",broken_link_ids);
	for (uuid_vec_t::const_iterator it = broken_link_ids.begin();
		 it != broken_link_ids.end(); ++it)
	{
		LLViewerInventoryItem *item = gInventory.getItem(*it);
		mCatDeltas[item->getParentUUID()]--;
		mObjectsDeleted.insert(*it);
	}

	// parse _created_items
	parseUUIDArray(update,"_created_items",mItemsCreatedIds);

	if (update.has("_embedded"))
	{
		const LLSD& embedded = update["_embedded"];
		for(LLSD::map_const_iterator it = embedded.beginMap(),
				end = embedded.endMap();
				it != end; ++it)
		{
			const std::string& field = (*it).first;
			
			// parse created links
			if (field == "link")
			{
				const LLSD& links = embedded["link"];
				parseCreatedLinks(links);
			}
		}
	}

	// Parse item update at the top level.
	if (update.has("item_id"))
	{
		LLUUID item_id = update["item_id"].asUUID();
		LLPointer<LLViewerInventoryItem> new_item(new LLViewerInventoryItem);
		LLViewerInventoryItem *curr_item = gInventory.getItem(item_id);
		if (curr_item)
		{
			// Default to current values where not provided.
			new_item->copyViewerItem(curr_item);
		}
		BOOL rv = new_item->unpackMessage(update);
		if (rv)
		{
			mItemsUpdated[item_id] = new_item;
			// This statement is here to cause a new entry with 0
			// delta to be created if it does not already exist;
			// otherwise has no effect.
			mCatDeltas[new_item->getParentUUID()];
		}
		else
		{
			llerrs << "unpack failed" << llendl;
		}
	}

	// Parse updated category versions.
	const std::string& ucv = "_updated_category_versions";
	if (update.has(ucv))
	{
		for(LLSD::map_const_iterator it = update[ucv].beginMap(),
				end = update[ucv].endMap();
			it != end; ++it)
		{
			const LLUUID id((*it).first);
			S32 version = (*it).second.asInteger();
			mCatVersions[id] = version;
		}
	}
}

void AISUpdate::parseUUIDArray(const LLSD& content, const std::string& name, uuid_vec_t& ids)
{
	ids.clear();
	if (content.has(name))
	{
		for(LLSD::array_const_iterator it = content[name].beginArray(),
				end = content[name].endArray();
				it != end; ++it)
		{
			ids.push_back((*it).asUUID());
		}
	}
}

void AISUpdate::parseLink(const LLUUID& link_id, const LLSD& link_map)
{
	LLPointer<LLViewerInventoryItem> new_link(new LLViewerInventoryItem);
	BOOL rv = new_link->unpackMessage(link_map);
	if (rv)
	{
		LLPermissions default_perms;
		default_perms.init(gAgent.getID(),gAgent.getID(),LLUUID::null,LLUUID::null);
		default_perms.initMasks(PERM_NONE,PERM_NONE,PERM_NONE,PERM_NONE,PERM_NONE);
		new_link->setPermissions(default_perms);
		LLSaleInfo default_sale_info;
		new_link->setSaleInfo(default_sale_info);
		//LL_DEBUGS("Inventory") << "creating link from llsd: " << ll_pretty_print_sd(link_map) << llendl;
		mItemsCreated[link_id] = new_link;
		const LLUUID& parent_id = new_link->getParentUUID();
		mCatDeltas[parent_id]++;
	}
	else
	{
		llwarns << "failed to parse" << llendl;
	}
}

void AISUpdate::parseCreatedLinks(const LLSD& links)
{
	for(LLSD::map_const_iterator linkit = links.beginMap(),
			linkend = links.endMap();
		linkit != linkend; ++linkit)
	{
		const LLUUID link_id((*linkit).first);
		const LLSD& link_map = (*linkit).second;
		uuid_vec_t::const_iterator pos =
			std::find(mItemsCreatedIds.begin(),
					  mItemsCreatedIds.end(),link_id);
		if (pos != mItemsCreatedIds.end())
		{
			parseLink(link_id,link_map);
		}
		else
		{
			LL_DEBUGS("Inventory") << "Ignoring link not in created items list " << link_id << llendl;
		}
	}
}

void AISUpdate::doUpdate()
{
	// Do descendent/version accounting.
	// Can remove this if/when we use the version info directly.
	for (std::map<LLUUID,S32>::const_iterator catit = mCatDeltas.begin();
		 catit != mCatDeltas.end(); ++catit)
	{
		const LLUUID cat_id(catit->first);
		S32 delta = catit->second;
		LLInventoryModel::LLCategoryUpdate up(cat_id, delta);
		gInventory.accountForUpdate(up);
	}
	
	// TODO - how can we use this version info? Need to be sure all
	// changes are going through AIS first, or at least through
	// something with a reliable responder.
	for (uuid_int_map_t::iterator ucv_it = mCatVersions.begin();
		 ucv_it != mCatVersions.end(); ++ucv_it)
	{
		const LLUUID id = ucv_it->first;
		S32 version = ucv_it->second;
		LLViewerInventoryCategory *cat = gInventory.getCategory(id);
		if (cat->getVersion() != version)
		{
			llwarns << "Possible version mismatch for category " << cat->getName()
					<< ", viewer version " << cat->getVersion()
					<< " server version " << version << llendl;
		}
	}

	// CREATE ITEMS
	for (deferred_item_map_t::const_iterator create_it = mItemsCreated.begin();
		 create_it != mItemsCreated.end(); ++create_it)
	{
		LLUUID item_id(create_it->first);
		LLPointer<LLViewerInventoryItem> new_item = create_it->second;

		// FIXME risky function since it calls updateServer() in some
		// cases.  Maybe break out the update/create cases, in which
		// case this is create.
		LL_DEBUGS("Inventory") << "created item " << item_id << llendl;
		gInventory.updateItem(new_item);
	}
	
	// UPDATE ITEMS
	for (deferred_item_map_t::const_iterator update_it = mItemsUpdated.begin();
		 update_it != mItemsUpdated.end(); ++update_it)
	{
		LLUUID item_id(update_it->first);
		LLPointer<LLViewerInventoryItem> new_item = update_it->second;
		// FIXME risky function since it calls updateServer() in some
		// cases.  Maybe break out the update/create cases, in which
		// case this is update.
		LL_DEBUGS("Inventory") << "updated item " << item_id << llendl;
		//LL_DEBUGS("Inventory") << ll_pretty_print_sd(new_item->asLLSD()) << llendl;
		gInventory.updateItem(new_item);
	}

	// DELETE OBJECTS
	for (std::set<LLUUID>::const_iterator del_it = mObjectsDeleted.begin();
		 del_it != mObjectsDeleted.end(); ++del_it)
	{
		LL_DEBUGS("Inventory") << "deleted item " << *del_it << llendl;
		gInventory.onObjectDeletedFromServer(*del_it, false, false, false);
	}

	gInventory.notifyObservers();
}

