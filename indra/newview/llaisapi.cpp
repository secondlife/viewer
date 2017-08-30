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

//=========================================================================
const std::string AISAPI::INVENTORY_CAP_NAME("InventoryAPIv3");
const std::string AISAPI::LIBRARY_CAP_NAME("LibraryAPIv3");

//-------------------------------------------------------------------------
/*static*/
bool AISAPI::isAvailable()
{
    if (gAgent.getRegion())
    {
        return gAgent.getRegion()->isCapabilityAvailable(INVENTORY_CAP_NAME);
    }
    return false;
}

/*static*/
void AISAPI::getCapNames(LLSD& capNames)
{
    capNames.append(INVENTORY_CAP_NAME);
    capNames.append(LIBRARY_CAP_NAME);
}

/*static*/
std::string AISAPI::getInvCap()
{
    if (gAgent.getRegion())
    {
        return gAgent.getRegion()->getCapability(INVENTORY_CAP_NAME);
    }
    return std::string();
}

/*static*/
std::string AISAPI::getLibCap()
{
    if (gAgent.getRegion())
    {
        return gAgent.getRegion()->getCapability(LIBRARY_CAP_NAME);
    }
    return std::string();
}

/*static*/ 
void AISAPI::CreateInventory(const LLUUID& parentId, const LLSD& newInventory, completion_t callback)
{
    std::string cap = getInvCap();
    if (cap.empty())
    {
        LL_WARNS("Inventory") << "Inventory cap not found!" << LL_ENDL;
        return;
    }

    LLUUID tid;
    tid.generate();

    std::string url = cap + std::string("/category/") + parentId.asString() + "?tid=" + tid.asString();
    LL_DEBUGS("Inventory") << "url: " << url << LL_ENDL;

    // I may be suffering from golden hammer here, but the first part of this bind 
    // is actually a static cast for &HttpCoroutineAdapter::postAndSuspend so that 
    // the compiler can identify the correct signature to select.
    // 
    // Reads as follows:
    // LLSD     - method returning LLSD
    // (LLCoreHttpUtil::HttpCoroutineAdapter::*) - pointer to member function of HttpCoroutineAdapter
    // (LLCore::HttpRequest::ptr_t, const std::string &, const LLSD &, LLCore::HttpOptions::ptr_t, LLCore::HttpHeaders::ptr_t) - signature of method
    //  
    invokationFn_t postFn = boost::bind(
        // Humans ignore next line.  It is just a cast.
        static_cast<LLSD (LLCoreHttpUtil::HttpCoroutineAdapter::*)(LLCore::HttpRequest::ptr_t, const std::string &, const LLSD &, LLCore::HttpOptions::ptr_t, LLCore::HttpHeaders::ptr_t)>
        //----
        // _1 -> httpAdapter
        // _2 -> httpRequest
        // _3 -> url
        // _4 -> body 
        // _5 -> httpOptions
        // _6 -> httpHeaders
        (&LLCoreHttpUtil::HttpCoroutineAdapter::postAndSuspend), _1, _2, _3, _4, _5, _6);

    LLCoprocedureManager::CoProcedure_t proc(boost::bind(&AISAPI::InvokeAISCommandCoro,
        _1, postFn, url, parentId, newInventory, callback, COPYINVENTORY));
    EnqueueAISCommand("CreateInventory", proc);
}

/*static*/ 
void AISAPI::SlamFolder(const LLUUID& folderId, const LLSD& newInventory, completion_t callback)
{
    std::string cap = getInvCap();
    if (cap.empty())
    {
        LL_WARNS("Inventory") << "Inventory cap not found!" << LL_ENDL;
        return;
    }

    LLUUID tid;
    tid.generate();

    std::string url = cap + std::string("/category/") + folderId.asString() + "/links?tid=" + tid.asString();

    // see comment above in CreateInventoryCommand
    invokationFn_t putFn = boost::bind(
        // Humans ignore next line.  It is just a cast to specify which LLCoreHttpUtil::HttpCoroutineAdapter routine overload.
        static_cast<LLSD(LLCoreHttpUtil::HttpCoroutineAdapter::*)(LLCore::HttpRequest::ptr_t, const std::string &, const LLSD &, LLCore::HttpOptions::ptr_t, LLCore::HttpHeaders::ptr_t)>
        //----
        // _1 -> httpAdapter
        // _2 -> httpRequest
        // _3 -> url
        // _4 -> body 
        // _5 -> httpOptions
        // _6 -> httpHeaders
        (&LLCoreHttpUtil::HttpCoroutineAdapter::putAndSuspend), _1, _2, _3, _4, _5, _6);

    LLCoprocedureManager::CoProcedure_t proc(boost::bind(&AISAPI::InvokeAISCommandCoro,
        _1, putFn, url, folderId, newInventory, callback, SLAMFOLDER));

    EnqueueAISCommand("SlamFolder", proc);
}

void AISAPI::RemoveCategory(const LLUUID &categoryId, completion_t callback)
{
    std::string cap;

    cap = getInvCap();
    if (cap.empty())
    {
        LL_WARNS("Inventory") << "Inventory cap not found!" << LL_ENDL;
        return;
    }

    std::string url = cap + std::string("/category/") + categoryId.asString();
    LL_DEBUGS("Inventory") << "url: " << url << LL_ENDL;

    invokationFn_t delFn = boost::bind(
        // Humans ignore next line.  It is just a cast to specify which LLCoreHttpUtil::HttpCoroutineAdapter routine overload.
        static_cast<LLSD(LLCoreHttpUtil::HttpCoroutineAdapter::*)(LLCore::HttpRequest::ptr_t, const std::string &, LLCore::HttpOptions::ptr_t, LLCore::HttpHeaders::ptr_t)>
        //----
        // _1 -> httpAdapter
        // _2 -> httpRequest
        // _3 -> url
        // _4 -> body 
        // _5 -> httpOptions
        // _6 -> httpHeaders
        (&LLCoreHttpUtil::HttpCoroutineAdapter::deleteAndSuspend), _1, _2, _3, _5, _6);

    LLCoprocedureManager::CoProcedure_t proc(boost::bind(&AISAPI::InvokeAISCommandCoro,
        _1, delFn, url, categoryId, LLSD(), callback, REMOVECATEGORY));

    EnqueueAISCommand("RemoveCategory", proc);
}

/*static*/ 
void AISAPI::RemoveItem(const LLUUID &itemId, completion_t callback)
{
    std::string cap;

    cap = getInvCap();
    if (cap.empty())
    {
        LL_WARNS("Inventory") << "Inventory cap not found!" << LL_ENDL;
        return;
    }

    std::string url = cap + std::string("/item/") + itemId.asString();
    LL_DEBUGS("Inventory") << "url: " << url << LL_ENDL;

    invokationFn_t delFn = boost::bind(
        // Humans ignore next line.  It is just a cast to specify which LLCoreHttpUtil::HttpCoroutineAdapter routine overload.
        static_cast<LLSD(LLCoreHttpUtil::HttpCoroutineAdapter::*)(LLCore::HttpRequest::ptr_t, const std::string &, LLCore::HttpOptions::ptr_t, LLCore::HttpHeaders::ptr_t)>
        //----
        // _1 -> httpAdapter
        // _2 -> httpRequest
        // _3 -> url
        // _4 -> body 
        // _5 -> httpOptions
        // _6 -> httpHeaders
        (&LLCoreHttpUtil::HttpCoroutineAdapter::deleteAndSuspend), _1, _2, _3, _5, _6);

    LLCoprocedureManager::CoProcedure_t proc(boost::bind(&AISAPI::InvokeAISCommandCoro,
        _1, delFn, url, itemId, LLSD(), callback, REMOVEITEM));

    EnqueueAISCommand("RemoveItem", proc);
}

void AISAPI::CopyLibraryCategory(const LLUUID& sourceId, const LLUUID& destId, bool copySubfolders, completion_t callback)
{
    std::string cap;

    cap = getLibCap();
    if (cap.empty())
    {
        LL_WARNS("Inventory") << "Library cap not found!" << LL_ENDL;
        return;
    }

    LL_DEBUGS("Inventory") << "Copying library category: " << sourceId << " => " << destId << LL_ENDL;

    LLUUID tid;
    tid.generate();

    std::string url = cap + std::string("/category/") + sourceId.asString() + "?tid=" + tid.asString();
    if (!copySubfolders)
    {
        url += ",depth=0";
    }
    LL_INFOS() << url << LL_ENDL;

    std::string destination = destId.asString();

    invokationFn_t copyFn = boost::bind(
        // Humans ignore next line.  It is just a cast to specify which LLCoreHttpUtil::HttpCoroutineAdapter routine overload.
        static_cast<LLSD(LLCoreHttpUtil::HttpCoroutineAdapter::*)(LLCore::HttpRequest::ptr_t, const std::string &, const std::string, LLCore::HttpOptions::ptr_t, LLCore::HttpHeaders::ptr_t)>
        //----
        // _1 -> httpAdapter
        // _2 -> httpRequest
        // _3 -> url
        // _4 -> body 
        // _5 -> httpOptions
        // _6 -> httpHeaders
        (&LLCoreHttpUtil::HttpCoroutineAdapter::copyAndSuspend), _1, _2, _3, destination, _5, _6);
         
    LLCoprocedureManager::CoProcedure_t proc(boost::bind(&AISAPI::InvokeAISCommandCoro,
        _1, copyFn, url, destId, LLSD(), callback, COPYLIBRARYCATEGORY));

    EnqueueAISCommand("CopyLibraryCategory", proc);
}

/*static*/ 
void AISAPI::PurgeDescendents(const LLUUID &categoryId, completion_t callback)
{
    std::string cap;

    cap = getInvCap();
    if (cap.empty())
    {
        LL_WARNS("Inventory") << "Inventory cap not found!" << LL_ENDL;
        return;
    }

    std::string url = cap + std::string("/category/") + categoryId.asString() + "/children";
    LL_DEBUGS("Inventory") << "url: " << url << LL_ENDL;

    invokationFn_t delFn = boost::bind(
        // Humans ignore next line.  It is just a cast to specify which LLCoreHttpUtil::HttpCoroutineAdapter routine overload.
        static_cast<LLSD(LLCoreHttpUtil::HttpCoroutineAdapter::*)(LLCore::HttpRequest::ptr_t, const std::string &, LLCore::HttpOptions::ptr_t, LLCore::HttpHeaders::ptr_t)>
        //----
        // _1 -> httpAdapter
        // _2 -> httpRequest
        // _3 -> url
        // _4 -> body 
        // _5 -> httpOptions
        // _6 -> httpHeaders
        (&LLCoreHttpUtil::HttpCoroutineAdapter::deleteAndSuspend), _1, _2, _3, _5, _6);

    LLCoprocedureManager::CoProcedure_t proc(boost::bind(&AISAPI::InvokeAISCommandCoro,
        _1, delFn, url, categoryId, LLSD(), callback, PURGEDESCENDENTS));

    EnqueueAISCommand("PurgeDescendents", proc);
}


/*static*/
void AISAPI::UpdateCategory(const LLUUID &categoryId, const LLSD &updates, completion_t callback)
{
    std::string cap;

    cap = getInvCap();
    if (cap.empty())
    {
        LL_WARNS("Inventory") << "Inventory cap not found!" << LL_ENDL;
        return;
    }
    std::string url = cap + std::string("/category/") + categoryId.asString();

    invokationFn_t patchFn = boost::bind(
        // Humans ignore next line.  It is just a cast to specify which LLCoreHttpUtil::HttpCoroutineAdapter routine overload.
        static_cast<LLSD(LLCoreHttpUtil::HttpCoroutineAdapter::*)(LLCore::HttpRequest::ptr_t, const std::string &, const LLSD &, LLCore::HttpOptions::ptr_t, LLCore::HttpHeaders::ptr_t)>
        //----
        // _1 -> httpAdapter
        // _2 -> httpRequest
        // _3 -> url
        // _4 -> body 
        // _5 -> httpOptions
        // _6 -> httpHeaders
        (&LLCoreHttpUtil::HttpCoroutineAdapter::patchAndSuspend), _1, _2, _3, _4, _5, _6);

    LLCoprocedureManager::CoProcedure_t proc(boost::bind(&AISAPI::InvokeAISCommandCoro,
        _1, patchFn, url, categoryId, updates, callback, UPDATECATEGORY));

    EnqueueAISCommand("UpdateCategory", proc);
}

/*static*/
void AISAPI::UpdateItem(const LLUUID &itemId, const LLSD &updates, completion_t callback)
{

    std::string cap;

    cap = getInvCap();
    if (cap.empty())
    {
        LL_WARNS("Inventory") << "Inventory cap not found!" << LL_ENDL;
        return;
    }
    std::string url = cap + std::string("/item/") + itemId.asString();

    invokationFn_t patchFn = boost::bind(
        // Humans ignore next line.  It is just a cast to specify which LLCoreHttpUtil::HttpCoroutineAdapter routine overload.
        static_cast<LLSD(LLCoreHttpUtil::HttpCoroutineAdapter::*)(LLCore::HttpRequest::ptr_t, const std::string &, const LLSD &, LLCore::HttpOptions::ptr_t, LLCore::HttpHeaders::ptr_t)>
        //----
        // _1 -> httpAdapter
        // _2 -> httpRequest
        // _3 -> url
        // _4 -> body 
        // _5 -> httpOptions
        // _6 -> httpHeaders
        (&LLCoreHttpUtil::HttpCoroutineAdapter::patchAndSuspend), _1, _2, _3, _4, _5, _6);

    LLCoprocedureManager::CoProcedure_t proc(boost::bind(&AISAPI::InvokeAISCommandCoro,
        _1, patchFn, url, itemId, updates, callback, UPDATEITEM));

    EnqueueAISCommand("UpdateItem", proc);
}

/*static*/
void AISAPI::EnqueueAISCommand(const std::string &procName, LLCoprocedureManager::CoProcedure_t proc)
{
    std::string procFullName = "AIS(" + procName + ")";
    LLCoprocedureManager::instance().enqueueCoprocedure("AIS", procFullName, proc);

}

/*static*/
void AISAPI::InvokeAISCommandCoro(LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t httpAdapter, 
        invokationFn_t invoke, std::string url, 
        LLUUID targetId, LLSD body, completion_t callback, COMMAND_TYPE type)
{
    LLCore::HttpOptions::ptr_t httpOptions(new LLCore::HttpOptions);
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest());
    LLCore::HttpHeaders::ptr_t httpHeaders;

    httpOptions->setTimeout(LLCoreHttpUtil::HTTP_REQUEST_EXPIRY_SECS);

    LL_DEBUGS("Inventory") << "url: " << url << LL_ENDL;

    LLSD result = invoke(httpAdapter, httpRequest, url, body, httpOptions, httpHeaders);
    LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    if (!status || !result.isMap())
    {
        if (!result.isMap())
        {
            status = LLCore::HttpStatus(HTTP_INTERNAL_ERROR, "Malformed response contents");
        }
        else if (status.getType() == 410) //GONE
        {
            // Item does not exist or was already deleted from server.
            // parent folder is out of sync
            if (type == REMOVECATEGORY)
            {
                LLViewerInventoryCategory *cat = gInventory.getCategory(targetId);
                if (cat)
                {
                    LL_WARNS("Inventory") << "Purge failed for '" << cat->getName()
                        << "' local version:" << cat->getVersion()
                        << " since folder no longer exists at server. Descendent count: server == " << cat->getDescendentCount()
                        << ", viewer == " << cat->getViewerDescendentCount()
                        << LL_ENDL;
                    gInventory.fetchDescendentsOf(cat->getParentUUID());
                    // Note: don't delete folder here - contained items will be deparented (or deleted)
                    // and since we are clearly out of sync we can't be sure we won't get rid of something we need.
                    // For example folder could have been moved or renamed with items intact, let it fetch first.
                }
            }
            else if (type == REMOVEITEM)
            {
                LLViewerInventoryItem *item = gInventory.getItem(targetId);
                if (item)
                {
                    LL_WARNS("Inventory") << "Purge failed for '" << item->getName()
                        << "' since item no longer exists at server." << LL_ENDL;
                    gInventory.fetchDescendentsOf(item->getParentUUID());
                    // since item not on the server and exists at viewer, so it needs an update at the least,
                    // so delete it, in worst case item will be refetched with new params.
                    gInventory.onObjectDeletedFromServer(targetId);
                }
            }
        }
        LL_WARNS("Inventory") << "Inventory error: " << status.toString() << LL_ENDL;
        LL_WARNS("Inventory") << ll_pretty_print_sd(result) << LL_ENDL;
    }

    gInventory.onAISUpdateReceived("AISCommand", result);

    if (callback && !callback.empty())
    {   
        LLUUID id(LLUUID::null);

        if (result.has("category_id") && (type == COPYLIBRARYCATEGORY))
	    {
		    id = result["category_id"];
	    }

        callback(id);
    }

}

//-------------------------------------------------------------------------
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

	LLPointer<LLViewerInventoryCategory> new_cat;
	LLViewerInventoryCategory *curr_cat = gInventory.getCategory(category_id);
	if (curr_cat)
	{
		// Default to current values where not provided.
        new_cat = new LLViewerInventoryCategory(curr_cat);
    }
    else
    {
        if (category_map.has("agent_id"))
        {
            new_cat = new LLViewerInventoryCategory(category_map["agent_id"].asUUID());
        }
        else
        {
            LL_DEBUGS() << "No owner provided, folder might be assigned wrong owner" << LL_ENDL;
            new_cat = new LLViewerInventoryCategory(LLUUID::null);
        }
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
	// Do version/descendant accounting.
	for (std::map<LLUUID,S32>::const_iterator catit = mCatDescendentDeltas.begin();
		 catit != mCatDescendentDeltas.end(); ++catit)
	{
		LL_DEBUGS("Inventory") << "descendant accounting for " << catit->first << LL_ENDL;

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

		// If we have a known descendant count, set that now.
		LLViewerInventoryCategory* cat = gInventory.getCategory(cat_id);
		if (cat)
		{
			S32 descendent_delta = catit->second;
			S32 old_count = cat->getDescendentCount();
			LL_DEBUGS("Inventory") << "Updating descendant count for "
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
		// we need to transfer back the updated version/descendant count.
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
					<< " AIS version " << version << " !!!Adjusting local version!!!" <<  LL_ENDL;

            // the AIS version should be considered the true version. Adjust 
            // our local category model to reflect this version number.  Otherwise 
            // it becomes possible to get stuck with the viewer being out of 
            // sync with the inventory system.  Under normal circumstances 
            // inventory COF is maintained on the viewer through calls to 
            // LLInventoryModel::accountForUpdate when a changing operation 
            // is performed.  This occasionally gets out of sync however.
            if (version != LLViewerInventoryCategory::VERSION_UNKNOWN)
            {
                cat->setVersion(version);
            }
            else
            {
                // We do not account for update if version is UNKNOWN, so we shouldn't rise version
                // either or viewer will get stuck on descendants count -1, try to refetch folder instead
                cat->fetch();
            }
		}
	}

	gInventory.notifyObservers();
}

