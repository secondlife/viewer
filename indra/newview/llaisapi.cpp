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
#include "llappviewer.h"
#include "llcallbacklist.h"
#include "llinventorymodel.h"
#include "llinventoryobserver.h"
#include "llnotificationsutil.h"
#include "llsdutil.h"
#include "llviewerregion.h"
#include "llvoavatar.h"
#include "llvoavatarself.h"
#include "llviewercontrol.h"

///----------------------------------------------------------------------------
/// Classes for AISv3 support.
///----------------------------------------------------------------------------

//=========================================================================
const std::string AISAPI::INVENTORY_CAP_NAME("InventoryAPIv3");
const std::string AISAPI::LIBRARY_CAP_NAME("LibraryAPIv3");
const S32 AISAPI::HTTP_TIMEOUT = 180;

std::list<AISAPI::ais_query_item_t> AISAPI::sPostponedQuery;

const S32 MAX_SIMULTANEOUS_COROUTINES = 2048;

// AIS3 allows '*' requests, but in reality those will be cut at some point
// Specify own depth to be able to anticipate it and mark folders as incomplete
const S32 MAX_FOLDER_DEPTH_REQUEST = 50;

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
        if (callback)
        {
            callback(LLUUID::null);
        }
        return;
    }

    LLUUID tid;
    tid.generate();

    std::string url = cap + std::string("/category/") + parentId.asString() + "?tid=" + tid.asString();
    LL_DEBUGS("Inventory") << "url: " << url << " parentID " << parentId << " newInventory " << newInventory << LL_ENDL;

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
        _1, postFn, url, parentId, newInventory, callback, CREATEINVENTORY));
    EnqueueAISCommand("CreateInventory", proc);
}

/*static*/
void AISAPI::SlamFolder(const LLUUID& folderId, const LLSD& newInventory, completion_t callback)
{
    std::string cap = getInvCap();
    if (cap.empty())
    {
        LL_WARNS("Inventory") << "Inventory cap not found!" << LL_ENDL;
        if (callback)
        {
            callback(LLUUID::null);
        }
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
        if (callback)
        {
            callback(LLUUID::null);
        }
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
        if (callback)
        {
            callback(LLUUID::null);
        }
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
        if (callback)
        {
            callback(LLUUID::null);
        }
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
        if (callback)
        {
            callback(LLUUID::null);
        }
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
        if (callback)
        {
            callback(LLUUID::null);
        }
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
        if (callback)
        {
            callback(LLUUID::null);
        }
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
void AISAPI::FetchItem(const LLUUID &itemId, ITEM_TYPE type, completion_t callback)
{
    std::string cap;

    cap = (type == INVENTORY) ? getInvCap() : getLibCap();
    if (cap.empty())
    {
        LL_WARNS("Inventory") << "Inventory cap not found!" << LL_ENDL;
        if (callback)
        {
            callback(LLUUID::null);
        }
        return;
    }
    std::string url = cap + std::string("/item/") + itemId.asString();

    invokationFn_t getFn = boost::bind(
        // Humans ignore next line.  It is just a cast to specify which LLCoreHttpUtil::HttpCoroutineAdapter routine overload.
        static_cast<LLSD(LLCoreHttpUtil::HttpCoroutineAdapter::*)(LLCore::HttpRequest::ptr_t, const std::string &, LLCore::HttpOptions::ptr_t, LLCore::HttpHeaders::ptr_t)>
        //----
        // _1 -> httpAdapter
        // _2 -> httpRequest
        // _3 -> url
        // _4 -> body
        // _5 -> httpOptions
        // _6 -> httpHeaders
        (&LLCoreHttpUtil::HttpCoroutineAdapter::getAndSuspend), _1, _2, _3, _5, _6);

    LLCoprocedureManager::CoProcedure_t proc(boost::bind(&AISAPI::InvokeAISCommandCoro,
        _1, getFn, url, itemId, LLSD(), callback, FETCHITEM));

    EnqueueAISCommand("FetchItem", proc);
}

/*static*/
void AISAPI::FetchCategoryChildren(const LLUUID &catId, ITEM_TYPE type, bool recursive, completion_t callback, S32 depth)
{
    std::string cap;

    cap = (type == INVENTORY) ? getInvCap() : getLibCap();
    if (cap.empty())
    {
        LL_WARNS("Inventory") << "Inventory cap not found!" << LL_ENDL;
        if (callback)
        {
            callback(LLUUID::null);
        }
        return;
    }
    std::string url = cap + std::string("/category/") + catId.asString() + "/children";

    if (recursive)
    {
        // can specify depth=*, but server side is going to cap requests
        // and reject everything 'over the top',.
        depth = MAX_FOLDER_DEPTH_REQUEST;
    }
    else
    {
        depth = llmin(depth, MAX_FOLDER_DEPTH_REQUEST);
    }

    url += "?depth=" + std::to_string(depth);

    invokationFn_t getFn = boost::bind(
        // Humans ignore next line.  It is just a cast to specify which LLCoreHttpUtil::HttpCoroutineAdapter routine overload.
        static_cast<LLSD(LLCoreHttpUtil::HttpCoroutineAdapter::*)(LLCore::HttpRequest::ptr_t, const std::string &, LLCore::HttpOptions::ptr_t, LLCore::HttpHeaders::ptr_t)>
        //----
        // _1 -> httpAdapter
        // _2 -> httpRequest
        // _3 -> url
        // _4 -> body
        // _5 -> httpOptions
        // _6 -> httpHeaders
        (&LLCoreHttpUtil::HttpCoroutineAdapter::getAndSuspend), _1, _2, _3, _5, _6);

    // get doesn't use body, can pass additional data
    LLSD body;
    body["depth"] = depth;
    LLCoprocedureManager::CoProcedure_t proc(boost::bind(&AISAPI::InvokeAISCommandCoro,
        _1, getFn, url, catId, body, callback, FETCHCATEGORYCHILDREN));

    EnqueueAISCommand("FetchCategoryChildren", proc);
}

// some folders can be requested by name, like
// animatn | bodypart | clothing | current | favorite | gesture | inbox | landmark | lsltext
// lstndfnd | my_otfts | notecard | object | outbox | root | snapshot | sound | texture | trash
void AISAPI::FetchCategoryChildren(const std::string &identifier, bool recursive, completion_t callback, S32 depth)
{
    std::string cap;

    cap = getInvCap();
    if (cap.empty())
    {
        LL_WARNS("Inventory") << "Inventory cap not found!" << LL_ENDL;
        if (callback)
        {
            callback(LLUUID::null);
        }
        return;
    }
    std::string url = cap + std::string("/category/") + identifier + "/children";

    if (recursive)
    {
        // can specify depth=*, but server side is going to cap requests
        // and reject everything 'over the top',.
        depth = MAX_FOLDER_DEPTH_REQUEST;
    }
    else
    {
        depth = llmin(depth, MAX_FOLDER_DEPTH_REQUEST);
    }

    url += "?depth=" + std::to_string(depth);

    invokationFn_t getFn = boost::bind(
        // Humans ignore next line.  It is just a cast to specify which LLCoreHttpUtil::HttpCoroutineAdapter routine overload.
        static_cast<LLSD(LLCoreHttpUtil::HttpCoroutineAdapter::*)(LLCore::HttpRequest::ptr_t, const std::string &, LLCore::HttpOptions::ptr_t, LLCore::HttpHeaders::ptr_t)>
        //----
        // _1 -> httpAdapter
        // _2 -> httpRequest
        // _3 -> url
        // _4 -> body
        // _5 -> httpOptions
        // _6 -> httpHeaders
        (&LLCoreHttpUtil::HttpCoroutineAdapter::getAndSuspend), _1, _2, _3, _5, _6);

    // get doesn't use body, can pass additional data
    LLSD body;
    body["depth"] = depth;
    LLCoprocedureManager::CoProcedure_t proc(boost::bind(&AISAPI::InvokeAISCommandCoro,
        _1, getFn, url, LLUUID::null, body, callback, FETCHCATEGORYCHILDREN));

    EnqueueAISCommand("FetchCategoryChildren", proc);
}

/*static*/
void AISAPI::FetchCategoryCategories(const LLUUID &catId, ITEM_TYPE type, bool recursive, completion_t callback, S32 depth)
{
    std::string cap;

    cap = (type == INVENTORY) ? getInvCap() : getLibCap();
    if (cap.empty())
    {
        LL_WARNS("Inventory") << "Inventory cap not found!" << LL_ENDL;
        if (callback)
        {
            callback(LLUUID::null);
        }
        return;
    }
    std::string url = cap + std::string("/category/") + catId.asString() + "/categories";

    if (recursive)
    {
        // can specify depth=*, but server side is going to cap requests
        // and reject everything 'over the top',.
        depth = MAX_FOLDER_DEPTH_REQUEST;
    }
    else
    {
        depth = llmin(depth, MAX_FOLDER_DEPTH_REQUEST);
    }

    url += "?depth=" + std::to_string(depth);

    invokationFn_t getFn = boost::bind(
        // Humans ignore next line.  It is just a cast to specify which LLCoreHttpUtil::HttpCoroutineAdapter routine overload.
        static_cast<LLSD(LLCoreHttpUtil::HttpCoroutineAdapter::*)(LLCore::HttpRequest::ptr_t, const std::string &, LLCore::HttpOptions::ptr_t, LLCore::HttpHeaders::ptr_t)>
        //----
        // _1 -> httpAdapter
        // _2 -> httpRequest
        // _3 -> url
        // _4 -> body
        // _5 -> httpOptions
        // _6 -> httpHeaders
        (&LLCoreHttpUtil::HttpCoroutineAdapter::getAndSuspend), _1, _2, _3, _5, _6);

    // get doesn't use body, can pass additional data
    LLSD body;
    body["depth"] = depth;
    LLCoprocedureManager::CoProcedure_t proc(boost::bind(&AISAPI::InvokeAISCommandCoro,
        _1, getFn, url, catId, body, callback, FETCHCATEGORYCATEGORIES));

    EnqueueAISCommand("FetchCategoryCategories", proc);
}

void AISAPI::FetchCategorySubset(const LLUUID& catId,
                                   const uuid_vec_t specificChildren,
                                   ITEM_TYPE type,
                                   bool recursive,
                                   completion_t callback,
                                   S32 depth)
{
    std::string cap = (type == INVENTORY) ? getInvCap() : getLibCap();
    if (cap.empty())
    {
        LL_WARNS("Inventory") << "Inventory cap not found!" << LL_ENDL;
        if (callback)
        {
            callback(LLUUID::null);
        }
        return;
    }
    if (specificChildren.empty())
    {
        LL_WARNS("Inventory") << "Empty request!" << LL_ENDL;
        if (callback)
        {
            callback(LLUUID::null);
        }
        return;
    }
    // category/any_folder_id/children?depth=*&children=child_id1,child_id2,child_id3
    std::string url = cap + std::string("/category/") + catId.asString() + "/children";

    if (recursive)
    {
        depth = MAX_FOLDER_DEPTH_REQUEST;
    }
    else
    {
        depth = llmin(depth, MAX_FOLDER_DEPTH_REQUEST);
    }

    uuid_vec_t::const_iterator iter = specificChildren.begin();
    uuid_vec_t::const_iterator end = specificChildren.end();

    url += "?depth=" + std::to_string(depth) + "&children=" + iter->asString();
    iter++;

    while (iter != end)
    {
        url += "," + iter->asString();
        iter++;
    }

    const S32 MAX_URL_LENGH = 2000; // RFC documentation specifies a maximum length of 2048
    if (url.length() > MAX_URL_LENGH)
    {
        LL_WARNS("Inventory") << "Request url is too long, url: " << url << LL_ENDL;
    }

    invokationFn_t getFn = boost::bind(
        // Humans ignore next line.  It is just a cast to specify which LLCoreHttpUtil::HttpCoroutineAdapter routine overload.
        static_cast<LLSD(LLCoreHttpUtil::HttpCoroutineAdapter::*)(LLCore::HttpRequest::ptr_t, const std::string&, LLCore::HttpOptions::ptr_t, LLCore::HttpHeaders::ptr_t)>
        //----
        // _1 -> httpAdapter
        // _2 -> httpRequest
        // _3 -> url
        // _4 -> body
        // _5 -> httpOptions
        // _6 -> httpHeaders
        (&LLCoreHttpUtil::HttpCoroutineAdapter::getAndSuspend), _1, _2, _3, _5, _6);

    // get doesn't use body, can pass additional data
    LLSD body;
    body["depth"] = depth;
    LLCoprocedureManager::CoProcedure_t proc(boost::bind(&AISAPI::InvokeAISCommandCoro,
                                                         _1, getFn, url, catId, body, callback, FETCHCATEGORYSUBSET));

    EnqueueAISCommand("FetchCategorySubset", proc);
}

/*static*/
// Will get COF folder, links in it and items those links point to
void AISAPI::FetchCOF(completion_t callback)
{
    std::string cap = getInvCap();
    if (cap.empty())
    {
        LL_WARNS("Inventory") << "Inventory cap not found!" << LL_ENDL;
        if (callback)
        {
            callback(LLUUID::null);
        }
        return;
    }
    std::string url = cap + std::string("/category/current/links");

    invokationFn_t getFn = boost::bind(
        // Humans ignore next line.  It is just a cast to specify which LLCoreHttpUtil::HttpCoroutineAdapter routine overload.
        static_cast<LLSD(LLCoreHttpUtil::HttpCoroutineAdapter::*)(LLCore::HttpRequest::ptr_t, const std::string&, LLCore::HttpOptions::ptr_t, LLCore::HttpHeaders::ptr_t)>
        //----
        // _1 -> httpAdapter
        // _2 -> httpRequest
        // _3 -> url
        // _4 -> body
        // _5 -> httpOptions
        // _6 -> httpHeaders
        (&LLCoreHttpUtil::HttpCoroutineAdapter::getAndSuspend), _1, _2, _3, _5, _6);

    LLSD body;
    // Only cof folder will be full, but cof can contain an outfit
    // link with embedded outfit folder for request to parse
    body["depth"] = 0;
    LLCoprocedureManager::CoProcedure_t proc(boost::bind(&AISAPI::InvokeAISCommandCoro,
                                                         _1, getFn, url, LLUUID::null, body, callback, FETCHCOF));

    EnqueueAISCommand("FetchCOF", proc);
}

void AISAPI::FetchCategoryLinks(const LLUUID &catId, completion_t callback)
{
    std::string cap = getInvCap();
    if (cap.empty())
    {
        LL_WARNS("Inventory") << "Inventory cap not found!" << LL_ENDL;
        if (callback)
        {
            callback(LLUUID::null);
        }
        return;
    }
    std::string url = cap + std::string("/category/") + catId.asString() + "/links";

    invokationFn_t getFn = boost::bind(
        // Humans ignore next line.  It is just a cast to specify which LLCoreHttpUtil::HttpCoroutineAdapter routine overload.
        static_cast<LLSD (LLCoreHttpUtil::HttpCoroutineAdapter::*)(LLCore::HttpRequest::ptr_t, const std::string &,
                                                                   LLCore::HttpOptions::ptr_t, LLCore::HttpHeaders::ptr_t)>
        //----
        // _1 -> httpAdapter
        // _2 -> httpRequest
        // _3 -> url
        // _4 -> body
        // _5 -> httpOptions
        // _6 -> httpHeaders
        (&LLCoreHttpUtil::HttpCoroutineAdapter::getAndSuspend),
        _1, _2, _3, _5, _6);

    LLSD body;
    body["depth"] = 0;
    LLCoprocedureManager::CoProcedure_t proc(
        boost::bind(&AISAPI::InvokeAISCommandCoro, _1, getFn, url, LLUUID::null, body, callback, FETCHCATEGORYLINKS));

    EnqueueAISCommand("FetchCategoryLinks", proc);
}

/*static*/
void AISAPI::FetchOrphans(completion_t callback)
{
    std::string cap = getInvCap();
    if (cap.empty())
    {
        LL_WARNS("Inventory") << "Inventory cap not found!" << LL_ENDL;
        if (callback)
        {
            callback(LLUUID::null);
        }
        return;
    }
    std::string url = cap + std::string("/orphans");

    invokationFn_t getFn = boost::bind(
        // Humans ignore next line.  It is just a cast to specify which LLCoreHttpUtil::HttpCoroutineAdapter routine overload.
        static_cast<LLSD(LLCoreHttpUtil::HttpCoroutineAdapter::*)(LLCore::HttpRequest::ptr_t , const std::string& , LLCore::HttpOptions::ptr_t , LLCore::HttpHeaders::ptr_t)>
        //----
        // _1 -> httpAdapter
        // _2 -> httpRequest
        // _3 -> url
        // _4 -> body
        // _5 -> httpOptions
        // _6 -> httpHeaders
        (&LLCoreHttpUtil::HttpCoroutineAdapter::getAndSuspend) , _1 , _2 , _3 , _5 , _6);

    LLCoprocedureManager::CoProcedure_t proc(boost::bind(&AISAPI::InvokeAISCommandCoro ,
                                                         _1 , getFn , url , LLUUID::null , LLSD() , callback , FETCHORPHANS));

    EnqueueAISCommand("FetchOrphans" , proc);
}

/*static*/
void AISAPI::EnqueueAISCommand(const std::string &procName, LLCoprocedureManager::CoProcedure_t proc)
{
    LLCoprocedureManager &inst = LLCoprocedureManager::instance();
    auto pending_in_pool = inst.countPending("AIS");
    std::string procFullName = "AIS(" + procName + ")";
    if (pending_in_pool < MAX_SIMULTANEOUS_COROUTINES)
    {
        inst.enqueueCoprocedure("AIS", procFullName, proc);
    }
    else
    {
        // As I understand it, coroutines have built-in 'pending' pool
        // but unfortunately it has limited size which inventory often goes over
        // so this is a workaround to not overfill it.
        if (sPostponedQuery.empty())
        {
            sPostponedQuery.push_back(ais_query_item_t(procFullName, proc));
            gIdleCallbacks.addFunction(onIdle, NULL);
        }
        else
        {
            sPostponedQuery.push_back(ais_query_item_t(procFullName, proc));
        }
    }
}

/*static*/
void AISAPI::onIdle(void *userdata)
{
    if (!sPostponedQuery.empty())
    {
        LLCoprocedureManager &inst = LLCoprocedureManager::instance();
        auto pending_in_pool = inst.countPending("AIS");
        while (pending_in_pool < MAX_SIMULTANEOUS_COROUTINES && !sPostponedQuery.empty())
        {
            ais_query_item_t &item = sPostponedQuery.front();
            inst.enqueueCoprocedure("AIS", item.first, item.second);
            sPostponedQuery.pop_front();
            pending_in_pool++;
        }
    }

    if (sPostponedQuery.empty())
    {
        // Nothing to do anymore
        gIdleCallbacks.deleteFunction(onIdle, NULL);
    }
}

/*static*/
void AISAPI::onUpdateReceived(const LLSD& update, COMMAND_TYPE type, const LLSD& request_body)
{
    LLTimer timer;
    if ( (type == UPDATECATEGORY || type == UPDATEITEM)
        && gSavedSettings.getBOOL("DebugAvatarAppearanceMessage"))
    {
        dump_sequential_xml(gAgentAvatarp->getFullname() + "_ais_update", update);
    }

    AISUpdate ais_update(update, type, request_body);
    ais_update.doUpdate(); // execute the updates in the appropriate order.
    LL_DEBUGS("Inventory", "AIS3") << "Elapsed processing: " << timer.getElapsedTimeF32() << LL_ENDL;
}

/*static*/
void AISAPI::InvokeAISCommandCoro(LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t httpAdapter,
        invokationFn_t invoke, std::string url,
        LLUUID targetId, LLSD body, completion_t callback, COMMAND_TYPE type)
{
    if (gDisconnected)
    {
        if (callback)
        {
            callback(LLUUID::null);
        }
        return;
    }

    LLCore::HttpOptions::ptr_t httpOptions(new LLCore::HttpOptions);
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest());
    LLCore::HttpHeaders::ptr_t httpHeaders;

    httpOptions->setTimeout(HTTP_TIMEOUT);

    LL_DEBUGS("Inventory") << "Request url: " << url << LL_ENDL;

    LLSD result;
    LLSD httpResults;
    LLCore::HttpStatus status;

    result = invoke(httpAdapter , httpRequest , url , body , httpOptions , httpHeaders);
    httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

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
        else if (status == LLCore::HttpStatus(HTTP_FORBIDDEN) /*403*/)
        {
            if (type == FETCHCATEGORYCHILDREN)
            {
                if (body.has("depth") && body["depth"].asInteger() == 0)
                {
                    // Can't fetch a single folder with depth 0, folder is too big.
                    static bool first_call = true;
                    if (first_call)
                    {
                        first_call = false;
                        LLNotificationsUtil::add("InventoryLimitReachedAISAlert");
                    }
                    else
                    {
                        LLNotificationsUtil::add("InventoryLimitReachedAIS");
                    }
                    LL_WARNS("Inventory") << "Fetch failed, content is over limit, url: " << url << LL_ENDL;
                }
                else
                {
                    // Result was too big, but situation is recoverable by requesting with lower depth
                    LL_DEBUGS("Inventory") << "Fetch failed, content is over limit, url: " << url << LL_ENDL;
                }
            }
        }
        LL_WARNS("Inventory") << "Inventory error: " << status.toString() << LL_ENDL;
        LL_WARNS("Inventory") << ll_pretty_print_sd(result) << LL_ENDL;
    }

    LL_DEBUGS("Inventory", "AIS3") << "Result: " << result << LL_ENDL;
    onUpdateReceived(result, type, body);

    if (callback && !callback.empty())
    {
        bool needs_callback = true;
        LLUUID id(LLUUID::null);

        switch (type)
        {
        case COPYLIBRARYCATEGORY:
        case FETCHCATEGORYCATEGORIES:
        case FETCHCATEGORYCHILDREN:
        case FETCHCATEGORYSUBSET:
        case FETCHCATEGORYLINKS:
        case FETCHCOF:
            if (result.has("category_id"))
            {
                id = result["category_id"];
            }
            break;
        case FETCHITEM:
            if (result.has("item_id"))
            {
                // Error message might contain an item_id!!!
                id = result["item_id"];
            }
            if (result.has("linked_id"))
            {
                id = result["linked_id"];
            }
            break;
        case CREATEINVENTORY:
            // CREATEINVENTORY can have multiple callbacks
            if (result.has("_created_categories"))
            {
                LLSD& cats = result["_created_categories"];
                LLSD::array_const_iterator cat_iter;
                for (cat_iter = cats.beginArray(); cat_iter != cats.endArray(); ++cat_iter)
                {
                    LLUUID cat_id = *cat_iter;
                    callback(cat_id);
                    needs_callback = false;
                }
            }
            if (result.has("_created_items"))
            {
                LLSD& items = result["_created_items"];
                LLSD::array_const_iterator item_iter;
                for (item_iter = items.beginArray(); item_iter != items.endArray(); ++item_iter)
                {
                    LLUUID item_id = *item_iter;
                    callback(item_id);
                    needs_callback = false;
                }
            }
            break;
        default:
            break;
        }

        if (needs_callback)
        {
            // Call callback at least once regardless of failure.
            // UPDATEITEM doesn't expect an id
            callback(id);
        }
    }

}

//-------------------------------------------------------------------------
AISUpdate::AISUpdate(const LLSD& update, AISAPI::COMMAND_TYPE type, const LLSD& request_body)
: mType(type)
{
    mFetch = (type == AISAPI::FETCHITEM)
        || (type == AISAPI::FETCHCATEGORYCHILDREN)
        || (type == AISAPI::FETCHCATEGORYCATEGORIES)
        || (type == AISAPI::FETCHCATEGORYSUBSET)
        || (type == AISAPI::FETCHCOF)
        || (type == AISAPI::FETCHCATEGORYLINKS)
        || (type == AISAPI::FETCHORPHANS);
    // parse update llsd into stuff to do or parse received items.
    mFetchDepth = MAX_FOLDER_DEPTH_REQUEST;
    if (mFetch && request_body.has("depth"))
    {
        mFetchDepth = request_body["depth"].asInteger();
    }

    mTimer.setTimerExpirySec(AIS_EXPIRY_SECONDS);
    mTimer.start();
    parseUpdate(update);
}

void AISUpdate::clearParseResults()
{
    mCatDescendentDeltas.clear();
    mCatDescendentsKnown.clear();
    mCatVersionsUpdated.clear();
    mItemsCreated.clear();
    mItemsLost.clear();
    mItemsUpdated.clear();
    mCategoriesCreated.clear();
    mCategoriesUpdated.clear();
    mObjectsDeletedIds.clear();
    mItemIds.clear();
    mCategoryIds.clear();
}

void AISUpdate::checkTimeout()
{
    if (mTimer.hasExpired())
    {
        llcoro::suspend();
        LLCoros::checkStop();
        mTimer.setTimerExpirySec(AIS_EXPIRY_SECONDS);
    }
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
    // Errors from a fetch request might contain id without
    // full item or folder.
    // Todo: Depending on error we might want to do something,
    // like removing a 404 item or refetching parent folder
    if (update.has("linked_id") && update.has("parent_id"))
    {
        parseLink(update, mFetchDepth);
    }
    else if (update.has("item_id") && update.has("parent_id"))
    {
        parseItem(update);
    }

    if (mType == AISAPI::FETCHCATEGORYSUBSET)
    {
        // initial category is incomplete, don't process it,
        // go for content instead
        if (update.has("_embedded"))
        {
            parseEmbedded(update["_embedded"], mFetchDepth - 1);
        }
    }
    else if (update.has("category_id") && update.has("parent_id"))
    {
        parseCategory(update, mFetchDepth);
    }
    else
    {
        if (update.has("_embedded"))
        {
            parseEmbedded(update["_embedded"], mFetchDepth);
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
    bool rv = new_item->unpackMessage(item_map);
    if (rv)
    {
        if (mFetch)
        {
            mItemsCreated[item_id] = new_item;
            new_item->setComplete(true);

            if (new_item->getParentUUID().isNull())
            {
                mItemsLost[item_id] = new_item;
            }
        }
        else if (curr_item)
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
            new_item->setComplete(true);
        }
    }
    else
    {
        // *TODO: Wow, harsh.  Should we just complain and get out?
        LL_ERRS() << "unpack failed" << LL_ENDL;
    }
}

void AISUpdate::parseLink(const LLSD& link_map, S32 depth)
{
    LLUUID item_id = link_map["item_id"].asUUID();
    LLPointer<LLViewerInventoryItem> new_link(new LLViewerInventoryItem);
    LLViewerInventoryItem *curr_link = gInventory.getItem(item_id);
    if (curr_link)
    {
        // Default to current values where not provided.
        new_link->copyViewerItem(curr_link);
    }
    bool rv = new_link->unpackMessage(link_map);
    if (rv)
    {
        const LLUUID& parent_id = new_link->getParentUUID();
        if (mFetch)
        {
            LLPermissions default_perms;
            default_perms.init(gAgent.getID(), gAgent.getID(), LLUUID::null, LLUUID::null);
            default_perms.initMasks(PERM_NONE, PERM_NONE, PERM_NONE, PERM_NONE, PERM_NONE);
            new_link->setPermissions(default_perms);
            LLSaleInfo default_sale_info;
            new_link->setSaleInfo(default_sale_info);
            //LL_DEBUGS("Inventory") << "creating link from llsd: " << ll_pretty_print_sd(link_map) << LL_ENDL;
            mItemsCreated[item_id] = new_link;
            new_link->setComplete(true);

            if (new_link->getParentUUID().isNull())
            {
                mItemsLost[item_id] = new_link;
            }
        }
        else if (curr_link)
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
            new_link->setComplete(true);
        }

        if (link_map.has("_embedded"))
        {
            parseEmbedded(link_map["_embedded"], depth);
        }
    }
    else
    {
        // *TODO: Wow, harsh.  Should we just complain and get out?
        LL_ERRS() << "unpack failed" << LL_ENDL;
    }
}


void AISUpdate::parseCategory(const LLSD& category_map, S32 depth)
{
    LLUUID category_id = category_map["category_id"].asUUID();
    S32 version = LLViewerInventoryCategory::VERSION_UNKNOWN;

    if (category_map.has("version"))
    {
        version = category_map["version"].asInteger();
    }

    LLViewerInventoryCategory *curr_cat = gInventory.getCategory(category_id);

    if (curr_cat
        && curr_cat->getVersion() > LLViewerInventoryCategory::VERSION_UNKNOWN
        && curr_cat->getDescendentCount() != LLViewerInventoryCategory::DESCENDENT_COUNT_UNKNOWN
        && version > LLViewerInventoryCategory::VERSION_UNKNOWN
        && version < curr_cat->getVersion())
    {
        LL_WARNS() << "Got stale folder, known: " << curr_cat->getVersion()
            << ", received: " << version << LL_ENDL;
        return;
    }

    LLPointer<LLViewerInventoryCategory> new_cat;
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
    bool rv = new_cat->unpackMessage(category_map);
    // *NOTE: unpackMessage does not unpack version or descendent count.
    if (rv)
    {
        // Check descendent count first, as it may be needed
        // to populate newly created categories
        if (category_map.has("_embedded"))
        {
            parseDescendentCount(category_id, new_cat->getPreferredType(), category_map["_embedded"]);
        }

        if (mFetch)
        {
            uuid_int_map_t::const_iterator lookup_it = mCatDescendentsKnown.find(category_id);
            if (mCatDescendentsKnown.end() != lookup_it)
            {
                S32 descendent_count = static_cast<S32>(lookup_it->second);
                LL_DEBUGS("Inventory") << "Setting descendents count to " << descendent_count
                    << " for category " << category_id << LL_ENDL;
                new_cat->setDescendentCount(descendent_count);

                // set version only if we are sure this update has full data and embeded items
                // since viewer uses version to decide if folder and content still need fetching
                if (version > LLViewerInventoryCategory::VERSION_UNKNOWN
                    && depth >= 0)
                {
                    if (curr_cat && curr_cat->getVersion() > version)
                    {
                        LL_WARNS("Inventory") << "Version was " << curr_cat->getVersion()
                            << ", but fetch returned version " << version
                            << " for category " << category_id << LL_ENDL;
                    }
                    else
                    {
                        LL_DEBUGS("Inventory") << "Setting version to " << version
                            << " for category " << category_id << LL_ENDL;
                    }

                    new_cat->setVersion(version);
                }
            }
            else if (curr_cat
                     && curr_cat->getVersion() > LLViewerInventoryCategory::VERSION_UNKNOWN
                     && version > curr_cat->getVersion())
            {
                LL_DEBUGS("Inventory") << "Category " << category_id
                    << " is stale. Known version: " << curr_cat->getVersion()
                    << " server version: " << version << LL_ENDL;
            }
            mCategoriesCreated[category_id] = new_cat;
        }
        else if (curr_cat)
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
            uuid_int_map_t::const_iterator lookup_it = mCatDescendentsKnown.find(category_id);
            if (mCatDescendentsKnown.end() != lookup_it)
            {
                S32 descendent_count = static_cast<S32>(lookup_it->second);
                LL_DEBUGS("Inventory") << "Setting descendents count to " << descendent_count
                    << " for new category " << category_id << LL_ENDL;
                new_cat->setDescendentCount(descendent_count);

                // Don't set version unles correct children count is present
                if (category_map.has("version"))
                {
                    S32 version = category_map["version"].asInteger();
                    LL_DEBUGS("Inventory") << "Setting version to " << version
                        << " for new category " << category_id << LL_ENDL;
                    new_cat->setVersion(version);
                }
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
        parseEmbedded(category_map["_embedded"], depth - 1);
    }
}

void AISUpdate::parseDescendentCount(const LLUUID& category_id, LLFolderType::EType type, const LLSD& embedded)
{
    // We can only determine true descendent count if this contains all descendent types.
    if (embedded.has("categories") &&
        embedded.has("links") &&
        embedded.has("items"))
    {
        mCatDescendentsKnown[category_id] = embedded["categories"].size();
        mCatDescendentsKnown[category_id] += embedded["links"].size();
        mCatDescendentsKnown[category_id] += embedded["items"].size();
    }
    else if (mFetch && embedded.has("links") && (type == LLFolderType::FT_CURRENT_OUTFIT || type == LLFolderType::FT_OUTFIT))
    {
        // COF and outfits contain links only
        mCatDescendentsKnown[category_id] = embedded["links"].size();
    }
}

void AISUpdate::parseEmbedded(const LLSD& embedded, S32 depth)
{
    checkTimeout();

    if (embedded.has("links")) // _embedded in a category
    {
        parseEmbeddedLinks(embedded["links"], depth);
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
        parseEmbeddedCategories(embedded["categories"], depth);
    }
    if (embedded.has("category")) // _embedded in a link
    {
        parseEmbeddedCategory(embedded["category"], depth);
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

void AISUpdate::parseEmbeddedLinks(const LLSD& links, S32 depth)
{
    for(LLSD::map_const_iterator linkit = links.beginMap(),
            linkend = links.endMap();
        linkit != linkend; ++linkit)
    {
        const LLUUID link_id((*linkit).first);
        const LLSD& link_map = (*linkit).second;
        if (!mFetch && mItemIds.end() == mItemIds.find(link_id))
        {
            LL_DEBUGS("Inventory") << "Ignoring link not in items list " << link_id << LL_ENDL;
        }
        else
        {
            parseLink(link_map, depth);
        }
    }
}

void AISUpdate::parseEmbeddedItem(const LLSD& item)
{
    // a single item (_embedded in a link)
    if (item.has("item_id"))
    {
        if (mFetch || mItemIds.end() != mItemIds.find(item["item_id"].asUUID()))
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
        if (!mFetch && mItemIds.end() == mItemIds.find(item_id))
        {
            LL_DEBUGS("Inventory") << "Ignoring item not in items list " << item_id << LL_ENDL;
        }
        else
        {
            parseItem(item_map);
        }
    }
}

void AISUpdate::parseEmbeddedCategory(const LLSD& category, S32 depth)
{
    // a single category (_embedded in a link)
    if (category.has("category_id"))
    {
        if (mFetch || mCategoryIds.end() != mCategoryIds.find(category["category_id"].asUUID()))
        {
            parseCategory(category, depth);
        }
    }
}

void AISUpdate::parseEmbeddedCategories(const LLSD& categories, S32 depth)
{
    // a map of categories (_embedded in a category)
    for(LLSD::map_const_iterator categoryit = categories.beginMap(),
            categoryend = categories.endMap();
        categoryit != categoryend; ++categoryit)
    {
        const LLUUID category_id((*categoryit).first);
        const LLSD& category_map = (*categoryit).second;
        if (!mFetch && mCategoryIds.end() == mCategoryIds.find(category_id))
        {
            LL_DEBUGS("Inventory") << "Ignoring category not in categories list " << category_id << LL_ENDL;
        }
        else
        {
            parseCategory(category_map, depth);
        }
    }
}

void AISUpdate::doUpdate()
{
    checkTimeout();

    // Do version/descendant accounting.
    for (std::map<LLUUID,size_t>::const_iterator catit = mCatDescendentDeltas.begin();
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
            S32 descendent_delta = static_cast<S32>(catit->second);
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
    const S32 MAX_UPDATE_BACKLOG = 50; // stall prevention
    for (deferred_category_map_t::const_iterator create_it = mCategoriesCreated.begin();
         create_it != mCategoriesCreated.end(); ++create_it)
    {
        LLUUID category_id(create_it->first);
        LLPointer<LLViewerInventoryCategory> new_category = create_it->second;

        gInventory.updateCategory(new_category, LLInventoryObserver::CREATE);
        LL_DEBUGS("Inventory") << "created category " << category_id << LL_ENDL;

        // fetching can receive massive amount of items and folders
        if (gInventory.getChangedIDs().size() > MAX_UPDATE_BACKLOG)
        {
            gInventory.notifyObservers();
            checkTimeout();
        }
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

    // LOST ITEMS
    if (!mItemsLost.empty())
    {
        LL_INFOS("Inventory") << "Received " << (S32)mItemsLost.size() << " items without a parent" << LL_ENDL;
        const LLUUID lost_uuid(gInventory.findCategoryUUIDForType(LLFolderType::FT_LOST_AND_FOUND));
        if (lost_uuid.notNull())
        {
            for (deferred_item_map_t::const_iterator lost_it = mItemsLost.begin();
                 lost_it != mItemsLost.end(); ++lost_it)
            {
                LLPointer<LLViewerInventoryItem> new_item = lost_it->second;

                new_item->setParent(lost_uuid);
                new_item->updateParentOnServer(false);
            }
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

        // fetching can receive massive amount of items and folders
        if (gInventory.getChangedIDs().size() > MAX_UPDATE_BACKLOG)
        {
            gInventory.notifyObservers();
            checkTimeout();
        }
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
        S32 version = static_cast<S32>(ucv_it->second);
        LLViewerInventoryCategory *cat = gInventory.getCategory(id);
        LL_DEBUGS("Inventory") << "cat version update " << cat->getName() << " to version " << cat->getVersion() << LL_ENDL;
        if (cat->getVersion() != version)
        {
            // the AIS version should be considered the true version. Adjust
            // our local category model to reflect this version number.  Otherwise
            // it becomes possible to get stuck with the viewer being out of
            // sync with the inventory system.  Under normal circumstances
            // inventory COF is maintained on the viewer through calls to
            // LLInventoryModel::accountForUpdate when a changing operation
            // is performed.  This occasionally gets out of sync however.
            if (version != LLViewerInventoryCategory::VERSION_UNKNOWN)
            {
                LL_WARNS() << "Possible version mismatch for category " << cat->getName()
                    << ", viewer version " << cat->getVersion()
                    << " AIS version " << version << " !!!Adjusting local version!!!" << LL_ENDL;
                cat->setVersion(version);
            }
            else
            {
                // We do not account for update if version is UNKNOWN, so we shouldn't rise version
                // either or viewer will get stuck on descendants count -1, try to refetch folder instead
                //
                // Todo: proper backoff?

                LL_WARNS() << "Possible version mismatch for category " << cat->getName()
                    << ", viewer version " << cat->getVersion()
                    << " AIS version " << version << " !!!Rerequesting category!!!" << LL_ENDL;
                const S32 LONG_EXPIRY = 360;
                cat->fetch(LONG_EXPIRY);
            }
        }
    }

    checkTimeout();

    gInventory.notifyObservers();
}

