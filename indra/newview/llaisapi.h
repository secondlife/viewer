/** 
 * @file llaisapi.h
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
 */

#ifndef LL_LLAISAPI_H
#define LL_LLAISAPI_H

#include "lluuid.h"
#include <map>
#include <set>
#include <string>
#include "llviewerinventory.h"
#include "llcorehttputil.h"
#include "llcoproceduremanager.h"

class AISAPI
{
public:
    typedef boost::function<void(const LLUUID &invItem)>    completion_t;

    static bool isAvailable();
    static void getCapNames(LLSD& capNames);

    static void CreateInventory(const LLUUID& parentId, const LLSD& newInventory, completion_t callback = completion_t());
    static void SlamFolder(const LLUUID& folderId, const LLSD& newInventory, completion_t callback = completion_t());
    static void RemoveCategory(const LLUUID &categoryId, completion_t callback = completion_t());
    static void RemoveItem(const LLUUID &itemId, completion_t callback = completion_t());
    static void PurgeDescendents(const LLUUID &categoryId, completion_t callback = completion_t());
    static void UpdateCategory(const LLUUID &categoryId, const LLSD &updates, completion_t callback = completion_t());
    static void UpdateItem(const LLUUID &itemId, const LLSD &updates, completion_t callback = completion_t());
    static void CopyLibraryCategory(const LLUUID& sourceId, const LLUUID& destId, bool copySubfolders, completion_t callback = completion_t());

private:
    typedef enum {
        COPYINVENTORY,
        SLAMFOLDER,
        REMOVECATEGORY,
        REMOVEITEM,
        PURGEDESCENDENTS,
        UPDATECATEGORY,
        UPDATEITEM,
        COPYLIBRARYCATEGORY
    } COMMAND_TYPE;

    static const std::string INVENTORY_CAP_NAME;
    static const std::string LIBRARY_CAP_NAME;

    typedef boost::function < LLSD (LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t, LLCore::HttpRequest::ptr_t,
        const std::string, LLSD, LLCore::HttpOptions::ptr_t, LLCore::HttpHeaders::ptr_t) > invokationFn_t;

    static void EnqueueAISCommand(const std::string &procName, LLCoprocedureManager::CoProcedure_t proc);
    static void onIdle(void *userdata); // launches postponed AIS commands

    static std::string getInvCap();
    static std::string getLibCap();

    static void InvokeAISCommandCoro(LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t httpAdapter, 
        invokationFn_t invoke, std::string url, LLUUID targetId, LLSD body, 
        completion_t callback, COMMAND_TYPE type);

    typedef std::pair<std::string, LLCoprocedureManager::CoProcedure_t> ais_query_item_t;
    static std::list<ais_query_item_t> sPostponedQuery;
};

class AISUpdate
{
public:
	AISUpdate(const LLSD& update);
	void parseUpdate(const LLSD& update);
	void parseMeta(const LLSD& update);
	void parseContent(const LLSD& update);
	void parseUUIDArray(const LLSD& content, const std::string& name, uuid_list_t& ids);
	void parseLink(const LLSD& link_map);
	void parseItem(const LLSD& link_map);
	void parseCategory(const LLSD& link_map);
	void parseDescendentCount(const LLUUID& category_id, const LLSD& embedded);
	void parseEmbedded(const LLSD& embedded);
	void parseEmbeddedLinks(const LLSD& links);
	void parseEmbeddedItems(const LLSD& items);
	void parseEmbeddedCategories(const LLSD& categories);
	void parseEmbeddedItem(const LLSD& item);
	void parseEmbeddedCategory(const LLSD& category);
	void doUpdate();
private:
	void clearParseResults();

	typedef std::map<LLUUID,S32> uuid_int_map_t;
	uuid_int_map_t mCatDescendentDeltas;
	uuid_int_map_t mCatDescendentsKnown;
	uuid_int_map_t mCatVersionsUpdated;

	typedef std::map<LLUUID,LLPointer<LLViewerInventoryItem> > deferred_item_map_t;
	deferred_item_map_t mItemsCreated;
	deferred_item_map_t mItemsUpdated;
	typedef std::map<LLUUID,LLPointer<LLViewerInventoryCategory> > deferred_category_map_t;
	deferred_category_map_t mCategoriesCreated;
	deferred_category_map_t mCategoriesUpdated;

	// These keep track of uuid's mentioned in meta values.
	// Useful for filtering out which content we are interested in.
	uuid_list_t mObjectsDeletedIds;
	uuid_list_t mItemIds;
	uuid_list_t mCategoryIds;
};

#endif
