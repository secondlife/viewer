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
#include "llinventoryobserver.h"
#include "llviewercontrol.h"

///----------------------------------------------------------------------------
/// Classes for AISv3 support.
///----------------------------------------------------------------------------

// AISCommand - base class for retry-able HTTP requests using the AISv3 cap.
AISCommand::AISCommand(LLPointer<LLInventoryCallback> callback):
	mCommandFunc(NULL),
	mCallback(callback)
{
	mRetryPolicy = new LLAdaptiveRetryPolicy(1.0, 32.0, 2.0, 10);
}

bool AISCommand::run_command()
{
	if (NULL == mCommandFunc)
	{
		// This may happen if a command failed to initiate itself.
		LL_WARNS("Inventory") << "AIS command attempted with null command function" << LL_ENDL;
		return false;
	}
	else
	{
		mCommandFunc();
		return true;
	}
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
		LLUUID id; // will default to null if parse fails.
		getResponseUUID(content,id);
		mCallback->fire(id);
	}
}

/*virtual*/
void AISCommand::httpFailure()
{
	LL_WARNS("Inventory") << dumpResponse() << LL_ENDL;
	S32 status = getStatus();
	const LLSD& headers = getResponseHeaders();
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
		// *TODO: Notify user?  This seems bad.
		setCommandFunc(no_op);
	}
}

//static
bool AISCommand::isAPIAvailable()
{
	if (gAgent.getRegion())
	{
		return gAgent.getRegion()->isCapabilityAvailable("InventoryAPIv3");
	}
	return false;
}

//static
bool AISCommand::getInvCap(std::string& cap)
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

//static
bool AISCommand::getLibCap(std::string& cap)
{
	if (gAgent.getRegion())
	{
		cap = gAgent.getRegion()->getCapability("LibraryAPIv3");
	}
	if (!cap.empty())
	{
		return true;
	}
	return false;
}

//static
void AISCommand::getCapabilityNames(LLSD& capabilityNames)
{
	capabilityNames.append("InventoryAPIv3");
	capabilityNames.append("LibraryAPIv3");
}

RemoveItemCommand::RemoveItemCommand(const LLUUID& item_id,
									 LLPointer<LLInventoryCallback> callback):
	AISCommand(callback)
{
	std::string cap;
	if (!getInvCap(cap))
	{
		LL_WARNS() << "No cap found" << LL_ENDL;
		return;
	}
	std::string url = cap + std::string("/item/") + item_id.asString();
	LL_DEBUGS("Inventory") << "url: " << url << LL_ENDL;
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
	if (!getInvCap(cap))
	{
		LL_WARNS() << "No cap found" << LL_ENDL;
		return;
	}
	std::string url = cap + std::string("/category/") + item_id.asString();
	LL_DEBUGS("Inventory") << "url: " << url << LL_ENDL;
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
	if (!getInvCap(cap))
	{
		LL_WARNS() << "No cap found" << LL_ENDL;
		return;
	}
	std::string url = cap + std::string("/category/") + item_id.asString() + "/children";
	LL_DEBUGS("Inventory") << "url: " << url << LL_ENDL;
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
	if (!getInvCap(cap))
	{
		LL_WARNS() << "No cap found" << LL_ENDL;
		return;
	}
	std::string url = cap + std::string("/item/") + item_id.asString();
	LL_DEBUGS("Inventory") << "url: " << url << LL_ENDL;
	LL_DEBUGS("Inventory") << "request: " << ll_pretty_print_sd(mUpdates) << LL_ENDL;
	LLCurl::ResponderPtr responder = this;
	LLSD headers;
	headers["Content-Type"] = "application/llsd+xml";
	F32 timeout = HTTP_REQUEST_EXPIRY_SECS;
	command_func_type cmd = boost::bind(&LLHTTPClient::patch, url, mUpdates, responder, headers, timeout);
	setCommandFunc(cmd);
}

UpdateCategoryCommand::UpdateCategoryCommand(const LLUUID& cat_id,
											 const LLSD& updates,
											 LLPointer<LLInventoryCallback> callback):
	mUpdates(updates),
	AISCommand(callback)
{
	std::string cap;
	if (!getInvCap(cap))
	{
		LL_WARNS() << "No cap found" << LL_ENDL;
		return;
	}
	std::string url = cap + std::string("/category/") + cat_id.asString();
	LL_DEBUGS("Inventory") << "url: " << url << LL_ENDL;
	LLCurl::ResponderPtr responder = this;
	LLSD headers;
	headers["Content-Type"] = "application/llsd+xml";
	F32 timeout = HTTP_REQUEST_EXPIRY_SECS;
	command_func_type cmd = boost::bind(&LLHTTPClient::patch, url, mUpdates, responder, headers, timeout);
	setCommandFunc(cmd);
}

CreateInventoryCommand::CreateInventoryCommand(const LLUUID& parent_id,
							 				   const LLSD& new_inventory,
							 				   LLPointer<LLInventoryCallback> callback):
	mNewInventory(new_inventory),
	AISCommand(callback)
{
	std::string cap;
	if (!getInvCap(cap))
	{
		LL_WARNS() << "No cap found" << LL_ENDL;
		return;
	}
	LLUUID tid;
	tid.generate();
	std::string url = cap + std::string("/category/") + parent_id.asString() + "?tid=" + tid.asString();
	LL_DEBUGS("Inventory") << "url: " << url << LL_ENDL;
	LLCurl::ResponderPtr responder = this;
	LLSD headers;
	headers["Content-Type"] = "application/llsd+xml";
	F32 timeout = HTTP_REQUEST_EXPIRY_SECS;
	command_func_type cmd = boost::bind(&LLHTTPClient::post, url, mNewInventory, responder, headers, timeout);
	setCommandFunc(cmd);
}

SlamFolderCommand::SlamFolderCommand(const LLUUID& folder_id, const LLSD& contents, LLPointer<LLInventoryCallback> callback):
	mContents(contents),
	AISCommand(callback)
{
	std::string cap;
	if (!getInvCap(cap))
	{
		LL_WARNS() << "No cap found" << LL_ENDL;
		return;
	}
	LLUUID tid;
	tid.generate();
	std::string url = cap + std::string("/category/") + folder_id.asString() + "/links?tid=" + tid.asString();
	LL_INFOS() << url << LL_ENDL;
	LLCurl::ResponderPtr responder = this;
	LLSD headers;
	headers["Content-Type"] = "application/llsd+xml";
	F32 timeout = HTTP_REQUEST_EXPIRY_SECS;
	command_func_type cmd = boost::bind(&LLHTTPClient::put, url, mContents, responder, headers, timeout);
	setCommandFunc(cmd);
}

CopyLibraryCategoryCommand::CopyLibraryCategoryCommand(const LLUUID& source_id,
													   const LLUUID& dest_id,
													   LLPointer<LLInventoryCallback> callback,
													   bool copy_subfolders):
	AISCommand(callback)
{
	std::string cap;
	if (!getLibCap(cap))
	{
		LL_WARNS() << "No cap found" << LL_ENDL;
		return;
	}
	LL_DEBUGS("Inventory") << "Copying library category: " << source_id << " => " << dest_id << LL_ENDL;
	LLUUID tid;
	tid.generate();
	std::string url = cap + std::string("/category/") + source_id.asString() + "?tid=" + tid.asString();
	if (!copy_subfolders)
	{
		url += ",depth=0";
	}
	LL_INFOS() << url << LL_ENDL;
	LLCurl::ResponderPtr responder = this;
	LLSD headers;
	F32 timeout = HTTP_REQUEST_EXPIRY_SECS;
	command_func_type cmd = boost::bind(&LLHTTPClient::copy, url, dest_id.asString(), responder, headers, timeout);
	setCommandFunc(cmd);
}

bool CopyLibraryCategoryCommand::getResponseUUID(const LLSD& content, LLUUID& id)
{
	if (content.has("category_id"))
	{
		id = content["category_id"];
		return true;
	}
	return false;
}

AISUpdate::AISUpdate(const LLSD& update)
{
	parseUpdate(update);
}

void AISUpdate::clearParseResults()
{
	mCatDescendentDeltas.clear();
	mCatDescendentsKnown.clear();
	mCatVersionsUpdated.clear();
	mItemsCreated.clear();
	mItemsUpdated.clear();
	mCategoriesCreated.clear();
	mCategoriesUpdated.clear();
	mObjectsDeletedIds.clear();
	mItemIds.clear();
	mCategoryIds.clear();
}

void AISUpdate::parseUpdate(const LLSD& update)
{
	clearParseResults();
	parseMeta(update);
	parseContent(update);
}

void AISUpdate::parseMeta(const LLSD& update)
{
	// parse _categories_removed -> mObjectsDeletedIds
	uuid_list_t cat_ids;
	parseUUIDArray(update,"_categories_removed",cat_ids);
	for (uuid_list_t::const_iterator it = cat_ids.begin();
		 it != cat_ids.end(); ++it)
	{
		LLViewerInventoryCategory *cat = gInventory.getCategory(*it);
		if(cat)
		{
			mCatDescendentDeltas[cat->getParentUUID()]--;
			mObjectsDeletedIds.insert(*it);
		}
		else
		{
			LL_WARNS("Inventory") << "removed category not found " << *it << LL_ENDL;
		}
	}

	// parse _categories_items_removed -> mObjectsDeletedIds
	uuid_list_t item_ids;
	parseUUIDArray(update,"_category_items_removed",item_ids);
	parseUUIDArray(update,"_removed_items",item_ids);
	for (uuid_list_t::const_iterator it = item_ids.begin();
		 it != item_ids.end(); ++it)
	{
		LLViewerInventoryItem *item = gInventory.getItem(*it);
		if(item)
		{
			mCatDescendentDeltas[item->getParentUUID()]--;
			mObjectsDeletedIds.insert(*it);
		}
		else
		{
			LL_WARNS("Inventory") << "removed item not found " << *it << LL_ENDL;
		}
	}

	// parse _broken_links_removed -> mObjectsDeletedIds
	uuid_list_t broken_link_ids;
	parseUUIDArray(update,"_broken_links_removed",broken_link_ids);
	for (uuid_list_t::const_iterator it = broken_link_ids.begin();
		 it != broken_link_ids.end(); ++it)
	{
		LLViewerInventoryItem *item = gInventory.getItem(*it);
		if(item)
		{
			mCatDescendentDeltas[item->getParentUUID()]--;
			mObjectsDeletedIds.insert(*it);
		}
		else
		{
			LL_WARNS("Inventory") << "broken link not found " << *it << LL_ENDL;
		}
	}

	// parse _created_items
	parseUUIDArray(update,"_created_items",mItemIds);

	// parse _created_categories
	parseUUIDArray(update,"_created_categories",mCategoryIds);

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
			mCatVersionsUpdated[id] = version;
		}
	}
}

void AISUpdate::parseContent(const LLSD& update)
{
	if (update.has("linked_id"))
	{
		parseLink(update);
	}
	else if (update.has("item_id"))
	{
		parseItem(update);
	}

	if (update.has("category_id"))
	{
		parseCategory(update);
	}
	else
	{
		if (update.has("_embedded"))
		{
			parseEmbedded(update["_embedded"]);
		}
	}
}

void AISUpdate::parseItem(const LLSD& item_map)
{
	LLUUID item_id = item_map["item_id"].asUUID();
	LLPointer<LLViewerInventoryItem> new_item(new LLViewerInventoryItem);
	LLViewerInventoryItem *curr_item = gInventory.getItem(item_id);
	if (curr_item)
	{
		// Default to current values where not provided.
		new_item->copyViewerItem(curr_item);
	}
	BOOL rv = new_item->unpackMessage(item_map);
	if (rv)
	{
		if (curr_item)
		{
			mItemsUpdated[item_id] = new_item;
			// This statement is here to cause a new entry with 0
			// delta to be created if it does not already exist;
			// otherwise has no effect.
			mCatDescendentDeltas[new_item->getParentUUID()];
		}
		else
		{
			mItemsCreated[item_id] = new_item;
			mCatDescendentDeltas[new_item->getParentUUID()]++;
		}
	}
	else
	{
		// *TODO: Wow, harsh.  Should we just complain and get out?
		LL_ERRS() << "unpack failed" << LL_ENDL;
	}
}

void AISUpdate::parseLink(const LLSD& link_map)
{
	LLUUID item_id = link_map["item_id"].asUUID();
	LLPointer<LLViewerInventoryItem> new_link(new LLViewerInventoryItem);
	LLViewerInventoryItem *curr_link = gInventory.getItem(item_id);
	if (curr_link)
	{
		// Default to current values where not provided.
		new_link->copyViewerItem(curr_link);
	}
	BOOL rv = new_link->unpackMessage(link_map);
	if (rv)
	{
		const LLUUID& parent_id = new_link->getParentUUID();
		if (curr_link)
		{
			mItemsUpdated[item_id] = new_link;
			// This statement is here to cause a new entry with 0
			// delta to be created if it does not already exist;
			// otherwise has no effect.
			mCatDescendentDeltas[parent_id];
		}
		else
		{
			LLPermissions default_perms;
			default_perms.init(gAgent.getID(),gAgent.getID(),LLUUID::null,LLUUID::null);
			default_perms.initMasks(PERM_NONE,PERM_NONE,PERM_NONE,PERM_NONE,PERM_NONE);
			new_link->setPermissions(default_perms);
			LLSaleInfo default_sale_info;
			new_link->setSaleInfo(default_sale_info);
			//LL_DEBUGS("Inventory") << "creating link from llsd: " << ll_pretty_print_sd(link_map) << LL_ENDL;
			mItemsCreated[item_id] = new_link;
			mCatDescendentDeltas[parent_id]++;
		}
	}
	else
	{
		// *TODO: Wow, harsh.  Should we just complain and get out?
		LL_ERRS() << "unpack failed" << LL_ENDL;
	}
}


void AISUpdate::parseCategory(const LLSD& category_map)
{
	LLUUID category_id = category_map["category_id"].asUUID();

	// Check descendent count first, as it may be needed
	// to populate newly created categories
	if (category_map.has("_embedded"))
	{
		parseDescendentCount(category_id, category_map["_embedded"]);
	}

	LLPointer<LLViewerInventoryCategory> new_cat(new LLViewerInventoryCategory(category_id));
	LLViewerInventoryCategory *curr_cat = gInventory.getCategory(category_id);
	if (curr_cat)
	{
		// Default to current values where not provided.
		new_cat->copyViewerCategory(curr_cat);
	}
	BOOL rv = new_cat->unpackMessage(category_map);
	// *NOTE: unpackMessage does not unpack version or descendent count.
	//if (category_map.has("version"))
	//{
	//	mCatVersionsUpdated[category_id] = category_map["version"].asInteger();
	//}
	if (rv)
	{
		if (curr_cat)
		{
			mCategoriesUpdated[category_id] = new_cat;
			// This statement is here to cause a new entry with 0
			// delta to be created if it does not already exist;
			// otherwise has no effect.
			mCatDescendentDeltas[new_cat->getParentUUID()];
			// Capture update for the category itself as well.
			mCatDescendentDeltas[category_id];
		}
		else
		{
			// Set version/descendents for newly created categories.
			if (category_map.has("version"))
			{
				S32 version = category_map["version"].asInteger();
				LL_DEBUGS("Inventory") << "Setting version to " << version
									   << " for new category " << category_id << LL_ENDL;
				new_cat->setVersion(version);
			}
			uuid_int_map_t::const_iterator lookup_it = mCatDescendentsKnown.find(category_id);
			if (mCatDescendentsKnown.end() != lookup_it)
			{
				S32 descendent_count = lookup_it->second;
				LL_DEBUGS("Inventory") << "Setting descendents count to " << descendent_count 
									   << " for new category " << category_id << LL_ENDL;
				new_cat->setDescendentCount(descendent_count);
			}
			mCategoriesCreated[category_id] = new_cat;
			mCatDescendentDeltas[new_cat->getParentUUID()]++;
		}
	}
	else
	{
		// *TODO: Wow, harsh.  Should we just complain and get out?
		LL_ERRS() << "unpack failed" << LL_ENDL;
	}

	// Check for more embedded content.
	if (category_map.has("_embedded"))
	{
		parseEmbedded(category_map["_embedded"]);
	}
}

void AISUpdate::parseDescendentCount(const LLUUID& category_id, const LLSD& embedded)
{
	// We can only determine true descendent count if this contains all descendent types.
	if (embedded.has("categories") &&
		embedded.has("links") &&
		embedded.has("items"))
	{
		mCatDescendentsKnown[category_id]  = embedded["categories"].size();
		mCatDescendentsKnown[category_id] += embedded["links"].size();
		mCatDescendentsKnown[category_id] += embedded["items"].size();
	}
}

void AISUpdate::parseEmbedded(const LLSD& embedded)
{
	if (embedded.has("links")) // _embedded in a category
	{
		parseEmbeddedLinks(embedded["links"]);
	}
	if (embedded.has("items")) // _embedded in a category
	{
		parseEmbeddedItems(embedded["items"]);
	}
	if (embedded.has("item")) // _embedded in a link
	{
		parseEmbeddedItem(embedded["item"]);
	}
	if (embedded.has("categories")) // _embedded in a category
	{
		parseEmbeddedCategories(embedded["categories"]);
	}
	if (embedded.has("category")) // _embedded in a link
	{
		parseEmbeddedCategory(embedded["category"]);
	}
}

void AISUpdate::parseUUIDArray(const LLSD& content, const std::string& name, uuid_list_t& ids)
{
	if (content.has(name))
	{
		for(LLSD::array_const_iterator it = content[name].beginArray(),
				end = content[name].endArray();
				it != end; ++it)
		{
			ids.insert((*it).asUUID());
		}
	}
}

void AISUpdate::parseEmbeddedLinks(const LLSD& links)
{
	for(LLSD::map_const_iterator linkit = links.beginMap(),
			linkend = links.endMap();
		linkit != linkend; ++linkit)
	{
		const LLUUID link_id((*linkit).first);
		const LLSD& link_map = (*linkit).second;
		if (mItemIds.end() == mItemIds.find(link_id))
		{
			LL_DEBUGS("Inventory") << "Ignoring link not in items list " << link_id << LL_ENDL;
		}
		else
		{
			parseLink(link_map);
		}
	}
}

void AISUpdate::parseEmbeddedItem(const LLSD& item)
{
	// a single item (_embedded in a link)
	if (item.has("item_id"))
	{
		if (mItemIds.end() != mItemIds.find(item["item_id"].asUUID()))
		{
			parseItem(item);
		}
	}
}

void AISUpdate::parseEmbeddedItems(const LLSD& items)
{
	// a map of items (_embedded in a category)
	for(LLSD::map_const_iterator itemit = items.beginMap(),
			itemend = items.endMap();
		itemit != itemend; ++itemit)
	{
		const LLUUID item_id((*itemit).first);
		const LLSD& item_map = (*itemit).second;
		if (mItemIds.end() == mItemIds.find(item_id))
		{
			LL_DEBUGS("Inventory") << "Ignoring item not in items list " << item_id << LL_ENDL;
		}
		else
		{
			parseItem(item_map);
		}
	}
}

void AISUpdate::parseEmbeddedCategory(const LLSD& category)
{
	// a single category (_embedded in a link)
	if (category.has("category_id"))
	{
		if (mCategoryIds.end() != mCategoryIds.find(category["category_id"].asUUID()))
		{
			parseCategory(category);
		}
	}
}

void AISUpdate::parseEmbeddedCategories(const LLSD& categories)
{
	// a map of categories (_embedded in a category)
	for(LLSD::map_const_iterator categoryit = categories.beginMap(),
			categoryend = categories.endMap();
		categoryit != categoryend; ++categoryit)
	{
		const LLUUID category_id((*categoryit).first);
		const LLSD& category_map = (*categoryit).second;
		if (mCategoryIds.end() == mCategoryIds.find(category_id))
		{
			LL_DEBUGS("Inventory") << "Ignoring category not in categories list " << category_id << LL_ENDL;
		}
		else
		{
			parseCategory(category_map);
		}
	}
}

void AISUpdate::doUpdate()
{
	// Do version/descendent accounting.
	for (std::map<LLUUID,S32>::const_iterator catit = mCatDescendentDeltas.begin();
		 catit != mCatDescendentDeltas.end(); ++catit)
	{
		LL_DEBUGS("Inventory") << "descendent accounting for " << catit->first << LL_ENDL;

		const LLUUID cat_id(catit->first);
		// Don't account for update if we just created this category.
		if (mCategoriesCreated.find(cat_id) != mCategoriesCreated.end())
		{
			LL_DEBUGS("Inventory") << "Skipping version increment for new category " << cat_id << LL_ENDL;
			continue;
		}

		// Don't account for update unless AIS told us it updated that category.
		if (mCatVersionsUpdated.find(cat_id) == mCatVersionsUpdated.end())
		{
			LL_DEBUGS("Inventory") << "Skipping version increment for non-updated category " << cat_id << LL_ENDL;
			continue;
		}

		// If we have a known descendent count, set that now.
		LLViewerInventoryCategory* cat = gInventory.getCategory(cat_id);
		if (cat)
		{
			S32 descendent_delta = catit->second;
			S32 old_count = cat->getDescendentCount();
			LL_DEBUGS("Inventory") << "Updating descendent count for "
								   << cat->getName() << " " << cat_id
								   << " with delta " << descendent_delta << " from "
								   << old_count << " to " << (old_count+descendent_delta) << LL_ENDL;
			LLInventoryModel::LLCategoryUpdate up(cat_id, descendent_delta);
			gInventory.accountForUpdate(up);
		}
		else
		{
			LL_DEBUGS("Inventory") << "Skipping version accounting for unknown category " << cat_id << LL_ENDL;
		}
	}

	// CREATE CATEGORIES
	for (deferred_category_map_t::const_iterator create_it = mCategoriesCreated.begin();
		 create_it != mCategoriesCreated.end(); ++create_it)
	{
		LLUUID category_id(create_it->first);
		LLPointer<LLViewerInventoryCategory> new_category = create_it->second;

		gInventory.updateCategory(new_category, LLInventoryObserver::CREATE);
		LL_DEBUGS("Inventory") << "created category " << category_id << LL_ENDL;
	}

	// UPDATE CATEGORIES
	for (deferred_category_map_t::const_iterator update_it = mCategoriesUpdated.begin();
		 update_it != mCategoriesUpdated.end(); ++update_it)
	{
		LLUUID category_id(update_it->first);
		LLPointer<LLViewerInventoryCategory> new_category = update_it->second;
		// Since this is a copy of the category *before* the accounting update, above,
		// we need to transfer back the updated version/descendent count.
		LLViewerInventoryCategory* curr_cat = gInventory.getCategory(new_category->getUUID());
		if (!curr_cat)
		{
			LL_WARNS("Inventory") << "Failed to update unknown category " << new_category->getUUID() << LL_ENDL;
		}
		else
		{
			new_category->setVersion(curr_cat->getVersion());
			new_category->setDescendentCount(curr_cat->getDescendentCount());
			gInventory.updateCategory(new_category);
			LL_DEBUGS("Inventory") << "updated category " << new_category->getName() << " " << category_id << LL_ENDL;
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
		LL_DEBUGS("Inventory") << "created item " << item_id << LL_ENDL;
		gInventory.updateItem(new_item, LLInventoryObserver::CREATE);
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
		LL_DEBUGS("Inventory") << "updated item " << item_id << LL_ENDL;
		//LL_DEBUGS("Inventory") << ll_pretty_print_sd(new_item->asLLSD()) << LL_ENDL;
		gInventory.updateItem(new_item);
	}

	// DELETE OBJECTS
	for (uuid_list_t::const_iterator del_it = mObjectsDeletedIds.begin();
		 del_it != mObjectsDeletedIds.end(); ++del_it)
	{
		LL_DEBUGS("Inventory") << "deleted item " << *del_it << LL_ENDL;
		gInventory.onObjectDeletedFromServer(*del_it, false, false, false);
	}

	// TODO - how can we use this version info? Need to be sure all
	// changes are going through AIS first, or at least through
	// something with a reliable responder.
	for (uuid_int_map_t::iterator ucv_it = mCatVersionsUpdated.begin();
		 ucv_it != mCatVersionsUpdated.end(); ++ucv_it)
	{
		const LLUUID id = ucv_it->first;
		S32 version = ucv_it->second;
		LLViewerInventoryCategory *cat = gInventory.getCategory(id);
		LL_DEBUGS("Inventory") << "cat version update " << cat->getName() << " to version " << cat->getVersion() << LL_ENDL;
		if (cat->getVersion() != version)
		{
			LL_WARNS() << "Possible version mismatch for category " << cat->getName()
					<< ", viewer version " << cat->getVersion()
					<< " server version " << version << LL_ENDL;
		}
	}

	gInventory.notifyObservers();
}

