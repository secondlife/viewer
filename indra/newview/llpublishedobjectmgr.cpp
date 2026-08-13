/**
 * @file llpublishedobjectmgr.cpp
 * @brief Published object state/logic manager extracted from llscripteditorws
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2026, Linden Research, Inc.
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
#include "llpublishedobjectmgr.h"

#include "llscripteditorws.h"

#include "llinventorydefines.h"
#include "llselectmgr.h"
#include "llviewerinventory.h"
#include "llviewerobject.h"
#include "llviewerobjectlist.h"
#include "llviewerregion.h"
#include "llvoinventorylistener.h"

namespace
{
    std::string nv_string(LLViewerObject* obj, const char* key)
    {
        if (!obj)
        {
            return std::string();
        }
        LLNameValue* nv = obj->getNVPair(key);
        if (!nv)
        {
            return std::string();
        }
        const char* s = nv->getString();
        if (!s || s[0] == '\0')
        {
            return std::string();
        }
        return std::string(s);
    }

    std::string get_prim_name(LLViewerObject* obj)
    {
        std::string name = nv_string(obj, "Name");
        if (!name.empty())
        {
            return name;
        }

        if (!obj)
        {
            return std::string();
        }

        LLSelectNode* node = LLSelectMgr::instance().getSelection()->findNode(obj);
        if (node && !node->mName.empty())
        {
            return node->mName;
        }

        // Never emit an empty prim/object name to downstream tooling.
        return obj->getID().asString();
    }
}

class LLPublishedPrimListener : public LLVOInventoryListener
{
public:
    LLPublishedPrimListener(LLScriptEditorWSServer* server, const LLUUID& object_id, const LLUUID& prim_id,
                            LLViewerObject* object)
        : mServer(server)
        , mObjectID(object_id)
        , mPrimID(prim_id)
    {
        registerVOInventoryListener(object, nullptr);
    }

    ~LLPublishedPrimListener() override = default;

    void inventoryChanged(LLViewerObject* object,
                         LLInventoryObject::object_list_t* inventory,
                         S32 serial_num, void* user_data) override
    {
        if (mServer)
        {
            if (mServer->isObjectPublished(mObjectID))
            {
                mServer->onPrimInventoryChanged(mObjectID, mPrimID);
            }
            else
            {
                mServer->onPrimInventoryReady(mObjectID, mPrimID);
            }
        }
    }

    const LLUUID& getObjectID() const { return mObjectID; }
    const LLUUID& getPrimID() const { return mPrimID; }

private:
    LLScriptEditorWSServer* mServer;
    LLUUID                  mObjectID;
    LLUUID                  mPrimID;
};

LLPublishedObjectMgr::LLPublishedObjectMgr(LLScriptEditorWSServer* server)
    : mServer(server)
{
}

LLPublishedObjectMgr::~LLPublishedObjectMgr() = default;

LLPublishedObjectMgr::PublishedObjectInfo::PublishedObjectInfo() = default;
LLPublishedObjectMgr::PublishedObjectInfo::~PublishedObjectInfo() = default;
LLPublishedObjectMgr::PublishedObjectInfo::PublishedObjectInfo(PublishedObjectInfo&&) noexcept = default;
LLPublishedObjectMgr::PublishedObjectInfo& LLPublishedObjectMgr::PublishedObjectInfo::operator=(PublishedObjectInfo&&) noexcept = default;

LLPublishedObjectMgr::PendingPublish::PendingPublish() = default;
LLPublishedObjectMgr::PendingPublish::~PendingPublish() = default;
LLPublishedObjectMgr::PendingPublish::PendingPublish(PendingPublish&&) noexcept = default;
LLPublishedObjectMgr::PendingPublish& LLPublishedObjectMgr::PendingPublish::operator=(PendingPublish&&) noexcept = default;

void LLPublishedObjectMgr::beginPendingPublish(const LLUUID& object_id, const std::vector<LLViewerObject*>& prims)
{
    PendingPublish pending;
    pending.mObjectID = object_id;
    for (LLViewerObject* prim : prims)
    {
        pending.mPendingPrims.insert(prim->getID());
        auto listener = std::make_unique<LLPublishedPrimListener>(
            mServer, object_id, prim->getID(), prim);
        pending.mListeners.push_back(std::move(listener));
    }
    mPendingPublishes[object_id] = std::move(pending);
}

bool LLPublishedObjectMgr::hasPendingPublish(const LLUUID& object_id) const
{
    return mPendingPublishes.find(object_id) != mPendingPublishes.end();
}

bool LLPublishedObjectMgr::markPendingPublishPrimReady(const LLUUID& object_id, const LLUUID& prim_id)
{
    auto it = mPendingPublishes.find(object_id);
    if (it == mPendingPublishes.end())
    {
        return false;
    }

    it->second.mPendingPrims.erase(prim_id);
    return it->second.mPendingPrims.empty();
}

void LLPublishedObjectMgr::recordPendingPropertyChange(
    const LLUUID& root_id,
    const LLUUID& prim_id,
    const std::string& name,
    const std::string& desc)
{
    auto it = mPendingPublishes.find(root_id);
    if (it == mPendingPublishes.end())
    {
        return;
    }

    PendingPublish& pending = it->second;
    if (prim_id == root_id)
    {
        pending.mHasRootProperties = true;
        pending.mObjectDescription = desc;
        if (!name.empty())
        {
            pending.mObjectName = name;
        }
        return;
    }

    if (!name.empty())
    {
        pending.mPrimNames[prim_id] = name;
    }
    pending.mPrimDescriptions[prim_id] = desc;
}

std::vector<std::unique_ptr<LLPublishedPrimListener>> LLPublishedObjectMgr::takePendingPublishListeners(const LLUUID& object_id)
{
    auto it = mPendingPublishes.find(object_id);
    if (it == mPendingPublishes.end())
    {
        return {};
    }

    auto listeners = std::move(it->second.mListeners);
    mPendingPublishes.erase(it);
    return listeners;
}

void LLPublishedObjectMgr::cancelPendingPublish(const LLUUID& object_id)
{
    mPendingPublishes.erase(object_id);
}

void LLPublishedObjectMgr::cancelPendingPublishWithCleanup(const LLUUID& object_id)
{
    auto it = mPendingPublishes.find(object_id);
    if (it == mPendingPublishes.end())
    {
        return;
    }

    it->second.mListeners.clear();
    mPendingPublishes.erase(it);
}

LLPublishedObjectMgr::PublishedObjectInfo* LLPublishedObjectMgr::getPublished(const LLUUID& object_id)
{
    auto it = mPublishedObjects.find(object_id);
    if (it == mPublishedObjects.end())
    {
        return nullptr;
    }

    return &it->second;
}

const LLPublishedObjectMgr::PublishedObjectInfo* LLPublishedObjectMgr::getPublished(const LLUUID& object_id) const
{
    auto it = mPublishedObjects.find(object_id);
    if (it == mPublishedObjects.end())
    {
        return nullptr;
    }

    return &it->second;
}

bool LLPublishedObjectMgr::reservePendingItemCreate(const LLUUID& prim_id, std::string&& pump_name)
{
    auto it = mPendingItemCreates.find(prim_id);
    if (it != mPendingItemCreates.end())
    {
        return false;
    }

    mPendingItemCreates[prim_id] = std::move(pump_name);
    return true;
}

bool LLPublishedObjectMgr::consumePendingItemCreate(const LLUUID& prim_id, std::string& pump_name)
{
    auto it = mPendingItemCreates.find(prim_id);
    if (it == mPendingItemCreates.end())
    {
        return false;
    }

    pump_name = it->second;
    mPendingItemCreates.erase(it);
    return true;
}

void LLPublishedObjectMgr::clearPendingItemCreate(const LLUUID& prim_id)
{
    mPendingItemCreates.erase(prim_id);
}

LLSD LLPublishedObjectMgr::buildPrimInventoryLLSD(LLViewerObject* object) const
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SCRIPTDEV;
    LLSD items = LLSD::emptyArray();
    if (!object)
    {
        return items;
    }

    LLInventoryObject::object_list_t contents;
    object->getInventoryContents(contents);

    for (const auto& obj : contents)
    {
        LLInventoryItem* item = dynamic_cast<LLInventoryItem*>(obj.get());
        if (!item)
        {
            continue;
        }

        LLAssetType::EType type = item->getType();
        if (type != LLAssetType::AT_LSL_TEXT && type != LLAssetType::AT_NOTECARD)
        {
            continue;
        }

        LLSD entry;
        entry["item_id"]     = item->getUUID();
        entry["name"]        = item->getName();
        entry["description"] = item->getDescription();
        entry["type"]        = (type == LLAssetType::AT_LSL_TEXT) ? "script" : "notecard";

        if (type == LLAssetType::AT_LSL_TEXT)
        {
            U8 subtype = item->getInventorySubType();
            entry["subtype"] = static_cast<S32>(subtype);

            const std::string& runtime = item->getRuntime();
            if (!runtime.empty())
            {
                entry["vm"] = runtime;
            }

            LLViewerInventoryItem* viewer_item = dynamic_cast<LLViewerInventoryItem*>(item);
            if (viewer_item)
            {
                entry["running"] = viewer_item->getIsRunning();
                entry["faulted"] = viewer_item->getIsFaulted();
            }
        }

        const LLPermissions& perms = item->getPermissions();
        LLSD perm_entry;
        perm_entry["owner"]      = static_cast<S32>(perms.getMaskOwner());
        perm_entry["next_owner"] = static_cast<S32>(perms.getMaskNextOwner());
        entry["permissions"]     = perm_entry;

        entry["creator_id"] = perms.getCreator();

        items.append(entry);
    }

    return items;
}

LLSD LLPublishedObjectMgr::buildPublishedObjectLLSD(LLViewerObject* root) const
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SCRIPTDEV;
    LLSD pub;
    pub["object_id"]          = root->getID();
    pub["object_name"]        = get_prim_name(root);
    pub["object_description"] = nv_string(root, "Desc");
    pub["owner_id"]           = root->mOwnerID;
    if (root->getRegion())
    {
        pub["region"] = root->getRegion()->getName();
    }
    pub["inventory"] = buildPrimInventoryLLSD(root);

    LLSD linked_objects = LLSD::emptyArray();
    S32 link_number = 2;
    for (LLViewerObject* child : root->getChildren())
    {
        LLSD link;
        link["link_id"]          = child->getID();
        link["link_number"]      = link_number++;
        link["link_name"]        = get_prim_name(child);
        link["link_description"] = nv_string(child, "Desc");
        link["inventory"]        = buildPrimInventoryLLSD(child);
        linked_objects.append(link);
    }
    if (linked_objects.size() > 0)
    {
        pub["linked_objects"] = linked_objects;
    }

    return pub;
}

LLSD LLPublishedObjectMgr::buildObjectListLLSD() const
{
    LLSD objects = LLSD::emptyArray();
    for (const auto& [object_id, info] : mPublishedObjects)
    {
        LLViewerObject* root = gObjectList.findObject(object_id);
        if (!root)
        {
            LL_DEBUGS("ScriptEditorWS") << "object.list: skipping " << object_id
                << " (no longer in scene)" << LL_ENDL;
            continue;
        }

        LLSD pub;
        pub["object_id"]          = info.mObjectID;
        pub["object_name"]        = info.mObjectName;
        pub["object_description"] = info.mObjectDescription;
        pub["owner_id"]           = info.mOwnerID;
        if (!info.mRegionName.empty())
        {
            pub["region"] = info.mRegionName;
        }
        pub["can_save_back"] = info.mCanSaveBackToContents;
        pub["inventory"] = buildPrimInventoryLLSD(root);

        LLSD linked_objects = LLSD::emptyArray();
        for (const auto& prim_info : info.mPrims)
        {
            if (prim_info.mLinkNumber == 1)
            {
                continue;
            }

            LLViewerObject* child = gObjectList.findObject(prim_info.mPrimID);
            if (!child)
            {
                continue;
            }

            LLSD link;
            link["link_id"]     = prim_info.mPrimID;
            link["link_number"] = prim_info.mLinkNumber;
            link["link_name"]   = prim_info.mPrimName;
            link["link_description"] = prim_info.mPrimDescription;
            link["inventory"]   = buildPrimInventoryLLSD(child);
            linked_objects.append(link);
        }
        if (linked_objects.size() > 0)
        {
            pub["linked_objects"] = linked_objects;
        }

        objects.append(pub);
    }

    return objects;
}

bool LLPublishedObjectMgr::buildLinksetUpdateLLSD(
    const LLUUID& root_id, LLSD& update) const
{
    const PublishedObjectInfo* info = getPublished(root_id);
    if (!info)
    {
        return false;
    }

    LLSD linked_objects = LLSD::emptyArray();
    for (const PublishedPrimInfo& prim_info : info->mPrims)
    {
        if (prim_info.mPrimID == root_id)
        {
            continue;
        }

        LLSD entry;
        entry["link_id"]     = prim_info.mPrimID;
        entry["link_number"] = prim_info.mLinkNumber;

        LLViewerObject* prim = gObjectList.findObject(prim_info.mPrimID);
        std::string link_name = prim ? get_prim_name(prim) : std::string();
        if (link_name.empty())
        {
            link_name = prim_info.mPrimName;
        }
        std::string link_desc = prim ? nv_string(prim, "Desc") : std::string();
        if (link_desc.empty())
        {
            link_desc = prim_info.mPrimDescription;
        }
        entry["link_name"] = link_name;
        entry["link_description"] = link_desc;
        entry["inventory"] = prim ? buildPrimInventoryLLSD(prim) : LLSD::emptyArray();

        linked_objects.append(entry);
    }

    update = LLSD();
    update["object_id"]      = root_id;
    update["linked_objects"] = linked_objects;
    return true;
}

bool LLPublishedObjectMgr::reconcileLinksetChildAdded(
    const LLUUID& root_id,
    LLViewerObject* child,
    F64 request_start_sec)
{
    PublishedObjectInfo* info = getPublished(root_id);
    if (!info || !child)
    {
        return false;
    }

    const LLUUID child_id = child->getID();

    info->mPrims.erase(
        std::remove_if(
            info->mPrims.begin(),
            info->mPrims.end(),
            [&](const PublishedPrimInfo& p) { return p.mPrimID == child_id; }),
        info->mPrims.end());

    PublishedPrimInfo prim_info;
    prim_info.mPrimID          = child_id;
    prim_info.mPrimName        = get_prim_name(child);
    prim_info.mPrimDescription = nv_string(child, "Desc");
    prim_info.mLinkNumber      = static_cast<S32>(info->mPrims.size()) + 1;
    prim_info.mInventorySerial = -1;
    info->mPrims.push_back(prim_info);

    auto listener = std::make_unique<LLPublishedPrimListener>(
        mServer, root_id, child_id, child);
    info->mListeners.push_back(std::move(listener));

    mInventoryRequestStartSec[child_id] = request_start_sec;
    mNewChildPrims[root_id].insert(child_id);
    return true;
}

bool LLPublishedObjectMgr::reconcileLinksetChildRemoved(
    const LLUUID& root_id, const LLUUID& child_id)
{
    PublishedObjectInfo* info = getPublished(root_id);
    if (!info)
    {
        return false;
    }

    info->mPrims.erase(
        std::remove_if(
            info->mPrims.begin(),
            info->mPrims.end(),
            [&](const PublishedPrimInfo& p) { return p.mPrimID == child_id; }),
        info->mPrims.end());

    info->mListeners.erase(
        std::remove_if(
            info->mListeners.begin(),
            info->mListeners.end(),
            [&](const std::unique_ptr<LLPublishedPrimListener>& l)
            {
                return l->getPrimID() == child_id;
            }),
        info->mListeners.end());

    bool root_empty_after_remove = false;
    consumePendingNewChild(root_id, child_id, root_empty_after_remove);

    mInventoryRequestStartSec.erase(child_id);

    S32 link_num = 2;
    for (auto& p : info->mPrims)
    {
        if (p.mPrimID != root_id)
        {
            p.mLinkNumber = link_num++;
        }
    }

    return true;
}

bool LLPublishedObjectMgr::handlePrimInventoryReadyEvent(
    const LLUUID& object_id, const LLUUID& prim_id)
{
    return markPendingPublishPrimReady(object_id, prim_id);
}

LLPublishedObjectMgr::PrimInventoryEventResult
LLPublishedObjectMgr::handlePrimInventoryChangedEvent(
    const LLUUID& object_id,
    const LLUUID& prim_id,
    LLViewerObject* prim,
    F64 now_sec)
{
    PrimInventoryEventResult result;
    if (!hasPublished(object_id) || !prim)
    {
        return result;
    }

    F64 request_start_sec = 0.0;
    if (consumeInventoryRequestStart(prim_id, request_start_sec))
    {
        result.mTimingConsumed = true;
        result.mTimingElapsedSec = llmax(0.0, now_sec - request_start_sec);
    }

    InventoryChangeResult inv_result = reconcileInventoryChanged(object_id, prim_id, prim);
    result.mKind = inv_result.mKind;
    result.mUpdate = inv_result.mUpdate;

    if (result.mKind == InventoryChangeKind::ROOT_INVENTORY_UPDATE ||
        result.mKind == InventoryChangeKind::CHILD_INVENTORY_UPDATE)
    {
        std::string pending_item_create_pump;
        if (consumePendingItemCreate(prim_id, pending_item_create_pump))
        {
            result.mHasPendingItemCreate = true;
            result.mPendingItemCreatePump = pending_item_create_pump;
        }
    }

    return result;
}

LLPublishedObjectMgr::InventoryChangeResult
LLPublishedObjectMgr::reconcileInventoryChanged(
    const LLUUID& object_id,
    const LLUUID& prim_id,
    LLViewerObject* prim)
{
    InventoryChangeResult result;
    PublishedObjectInfo* pub_info = getPublished(object_id);
    if (!pub_info || !prim)
    {
        return result;
    }

    bool root_empty_after_remove = false;
    if (consumePendingNewChild(object_id, prim_id, root_empty_after_remove))
    {
        for (auto& p : pub_info->mPrims)
        {
            if (p.mPrimID == prim_id)
            {
                p.mPrimName        = get_prim_name(prim);
                p.mPrimDescription = nv_string(prim, "Desc");
                p.mInventorySerial = 0;
                break;
            }
        }
        result.mKind = root_empty_after_remove
            ? InventoryChangeKind::CHILD_READY_FLUSH_NOW
            : InventoryChangeKind::CHILD_READY_WAIT;
        return result;
    }

    result.mUpdate = LLSD();
    result.mUpdate["object_id"] = object_id;
    LLSD inv = buildPrimInventoryLLSD(prim);
    if (prim_id == object_id)
    {
        result.mUpdate["inventory"] = inv;
        result.mKind = InventoryChangeKind::ROOT_INVENTORY_UPDATE;
    }
    else
    {
        LLSD modified_entry;
        modified_entry["link_id"]   = prim_id;
        modified_entry["inventory"] = inv;
        LLSD modified_arr = LLSD::emptyArray();
        modified_arr.append(modified_entry);
        result.mUpdate["changes"]["linked_objects"]["modified"] = modified_arr;
        result.mKind = InventoryChangeKind::CHILD_INVENTORY_UPDATE;
    }
    return result;
}

bool LLPublishedObjectMgr::applyPropertyChange(
    const LLUUID& root_id,
    const LLUUID& prim_id,
    const std::string& name,
    const std::string& desc,
    LLSD& update)
{
    PublishedObjectInfo* pub_info = getPublished(root_id);
    if (!pub_info)
    {
        return false;
    }

    update = LLSD();
    update["object_id"] = root_id;

    if (prim_id == root_id)
    {
        bool has_name = !name.empty();
        bool name_changed = has_name && (pub_info->mObjectName != name);
        bool desc_changed = (pub_info->mObjectDescription != desc);
        if (!name_changed && !desc_changed)
        {
            return false;
        }

        if (name_changed)
        {
            pub_info->mObjectName = name;
            update["object_name"] = name;
        }
        if (desc_changed)
        {
            pub_info->mObjectDescription = desc;
            update["object_description"] = desc;
        }
        return true;
    }

    auto prim_it = std::find_if(pub_info->mPrims.begin(), pub_info->mPrims.end(),
        [&](const PublishedPrimInfo& p) { return p.mPrimID == prim_id; });
    if (prim_it == pub_info->mPrims.end())
    {
        return false;
    }
    const bool name_changed = !name.empty() && prim_it->mPrimName != name;
    const bool desc_changed = prim_it->mPrimDescription != desc;
    if (!name_changed && !desc_changed)
    {
        return false;
    }

    LLSD modified_entry;
    modified_entry["link_id"] = prim_id;
    if (name_changed)
    {
        prim_it->mPrimName = name;
        modified_entry["link_name"] = name;
    }
    if (desc_changed)
    {
        prim_it->mPrimDescription = desc;
        modified_entry["link_description"] = desc;
    }
    LLSD modified_arr = LLSD::emptyArray();
    modified_arr.append(modified_entry);
    update["changes"]["linked_objects"]["modified"] = modified_arr;
    return true;
}

bool LLPublishedObjectMgr::hasActiveLinksetFlushTimer(const LLUUID& root_id) const
{
    auto it = mLinksetFlushTimers.find(root_id);
    if (it == mLinksetFlushTimers.end())
    {
        return false;
    }

    return !it->second.expired();
}

void LLPublishedObjectMgr::setLinksetFlushTimer(
    const LLUUID& root_id, const std::weak_ptr<LLEventTimer>& timer)
{
    mLinksetFlushTimers[root_id] = timer;
}

bool LLPublishedObjectMgr::cancelLinksetFlushTimer(const LLUUID& root_id)
{
    auto it = mLinksetFlushTimers.find(root_id);
    if (it == mLinksetFlushTimers.end())
    {
        return false;
    }

    if (auto locked = it->second.lock())
    {
        delete locked.get();
    }

    mLinksetFlushTimers.erase(it);
    return true;
}

void LLPublishedObjectMgr::clearLinksetFlushTimer(const LLUUID& root_id)
{
    mLinksetFlushTimers.erase(root_id);
}

bool LLPublishedObjectMgr::consumeInventoryRequestStart(
    const LLUUID& prim_id, F64& start_sec)
{
    auto it = mInventoryRequestStartSec.find(prim_id);
    if (it == mInventoryRequestStartSec.end())
    {
        return false;
    }

    start_sec = it->second;
    mInventoryRequestStartSec.erase(it);
    return true;
}

bool LLPublishedObjectMgr::markPrimInventorySerialAndDetectChange(
    const LLUUID& root_id, const LLUUID& prim_id, S16 inventory_serial)
{
    if (inventory_serial < 0)
    {
        return false;
    }

    PublishedObjectInfo* info = getPublished(root_id);
    if (!info)
    {
        return false;
    }

    auto it = std::find_if(
        info->mPrims.begin(),
        info->mPrims.end(),
        [&](const PublishedPrimInfo& p)
        {
            return p.mPrimID == prim_id;
        });
    if (it == info->mPrims.end())
    {
        return false;
    }

    if (it->mInventorySerial == inventory_serial)
    {
        return false;
    }

    it->mInventorySerial = inventory_serial;
    return true;
}

bool LLPublishedObjectMgr::consumePendingNewChild(
    const LLUUID& root_id, const LLUUID& child_id, bool& root_empty_after_remove)
{
    root_empty_after_remove = false;
    auto root_it = mNewChildPrims.find(root_id);
    if (root_it == mNewChildPrims.end())
    {
        return false;
    }

    auto child_it = root_it->second.find(child_id);
    if (child_it == root_it->second.end())
    {
        return false;
    }

    root_it->second.erase(child_it);
    if (root_it->second.empty())
    {
        root_empty_after_remove = true;
        mNewChildPrims.erase(root_it);
    }

    return true;
}

LLPublishedObjectMgr::PublishedObjectInfo& LLPublishedObjectMgr::finalizePendingPublish(
    const LLUUID& object_id, PublishedObjectInfo&& info)
{
    PublishedObjectInfo& published_info = mPublishedObjects[object_id];
    auto pending_it = mPendingPublishes.find(object_id);
    published_info = std::move(info);

    if (pending_it != mPendingPublishes.end())
    {
        PendingPublish& pending = pending_it->second;
        if (pending.mHasRootProperties)
        {
            if (!pending.mObjectName.empty())
            {
                published_info.mObjectName = pending.mObjectName;
            }
            published_info.mObjectDescription = pending.mObjectDescription;
        }

        for (PublishedPrimInfo& prim_info : published_info.mPrims)
        {
            auto name_it = pending.mPrimNames.find(prim_info.mPrimID);
            if (name_it != pending.mPrimNames.end() && !name_it->second.empty())
            {
                prim_info.mPrimName = name_it->second;
            }

            auto desc_it = pending.mPrimDescriptions.find(prim_info.mPrimID);
            if (desc_it != pending.mPrimDescriptions.end())
            {
                prim_info.mPrimDescription = desc_it->second;
            }
        }

        published_info.mListeners = std::move(pending.mListeners);
        mPendingPublishes.erase(pending_it);
    }
    else
    {
        published_info.mListeners = takePendingPublishListeners(object_id);
    }

    return published_info;
}

bool LLPublishedObjectMgr::cleanupObjectStateForUnpublish(const LLUUID& object_id)
{
    const bool was_published = hasPublished(object_id);

    cancelPendingPublishWithCleanup(object_id);
    clearPublishedListeners(object_id);
    erasePublished(object_id);

    cancelLinksetFlushTimer(object_id);
    clearPendingNewChildren(object_id);

    return was_published;
}

void LLPublishedObjectMgr::clearPublishedListeners(const LLUUID& object_id)
{
    auto pub_info = getPublished(object_id);
    if (!pub_info)
    {
        return;
    }

    pub_info->mListeners.clear();
}

void LLPublishedObjectMgr::clearAllStateWithListenerCleanup()
{
    for (auto& [id, pending] : mPendingPublishes)
    {
        pending.mListeners.clear();
    }
    mPendingPublishes.clear();

    for (auto& [id, info] : mPublishedObjects)
    {
        info.mListeners.clear();
    }
    mPublishedObjects.clear();
}
