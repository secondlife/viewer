/**
 * @file llpublishedobjectmgr.h
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

#pragma once

#include "llsd.h"
#include "lltimer.h"
#include "lluuid.h"
#include "lleventtimer.h"
#include "stdtypes.h"

#include <map>
#include <functional>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

class LLScriptEditorWSServer;
class LLChat;
class LLPublishedPrimListener;
class LLViewerObject;

class LLPublishedObjectMgr
{
public:
    struct PublishedPrimInfo
    {
        LLUUID      mPrimID;
        std::string mPrimName;
        S32         mLinkNumber;
        std::string mPrimDescription;
        S16         mInventorySerial;
    };

    struct PublishedObjectInfo
    {
        PublishedObjectInfo();
        ~PublishedObjectInfo();
        PublishedObjectInfo(PublishedObjectInfo&&) noexcept;
        PublishedObjectInfo& operator=(PublishedObjectInfo&&) noexcept;
        PublishedObjectInfo(const PublishedObjectInfo&) = delete;
        PublishedObjectInfo& operator=(const PublishedObjectInfo&) = delete;

        LLUUID      mObjectID;
        LLUUID      mOwnerID;
        std::string mObjectName;
        std::string mObjectDescription;
        std::string mRegionName;
        bool        mCanSaveBackToContents{ false };
        LLUUID      mSourceTaskID;
        std::vector<PublishedPrimInfo> mPrims;
        std::vector<std::unique_ptr<LLPublishedPrimListener>> mListeners;
    };

    struct PendingPublish
    {
        PendingPublish();
        ~PendingPublish();
        PendingPublish(PendingPublish&&) noexcept;
        PendingPublish& operator=(PendingPublish&&) noexcept;
        PendingPublish(const PendingPublish&) = delete;
        PendingPublish& operator=(const PendingPublish&) = delete;

        LLUUID      mObjectID;
        std::set<LLUUID> mPendingPrims;
        std::vector<std::unique_ptr<LLPublishedPrimListener>> mListeners;
        bool        mHasRootProperties{ false };
        std::string mObjectName;
        std::string mObjectDescription;
        std::map<LLUUID, std::string> mPrimNames;
        std::map<LLUUID, std::string> mPrimDescriptions;
    };

    enum class InventoryChangeKind
    {
        NOT_PUBLISHED,
        CHILD_READY_WAIT,
        CHILD_READY_FLUSH_NOW,
        ROOT_INVENTORY_UPDATE,
        CHILD_INVENTORY_UPDATE
    };

    struct InventoryChangeResult
    {
        InventoryChangeKind mKind{ InventoryChangeKind::NOT_PUBLISHED };
        LLSD mUpdate;
    };

    struct PrimInventoryEventResult
    {
        InventoryChangeKind mKind{ InventoryChangeKind::NOT_PUBLISHED };
        LLSD mUpdate;
        bool mTimingConsumed{ false };
        F64 mTimingElapsedSec{ 0.0 };
        bool mHasPendingItemCreate{ false };
        std::string mPendingItemCreatePump;
    };

    class RuntimeEventAggregator
    {
    public:
        enum class Channel
        {
            DEBUG,
            OWNER_SAY
        };

        enum class VM
        {
            LSL2,
            LUAU
        };

        struct StackFrame
        {
            S32         mLine{ 0 };
            std::string mFunction;
            std::string mSource;
        };

        struct ParsedError
        {
            std::string             mSource;
            std::string             mError;
            S32                     mLine{ 0 };
            S32                     mColumn{ 0 };
            std::vector<StackFrame> mStack;
        };

        struct Fragment
        {
            LLUUID      mFromID;
            std::string mFromName;
            std::string mText;
        };

        using fragments_t = std::vector<Fragment>;

        struct RuntimeEvent
        {
            VM          mVM;
            Channel     mChannel;
            fragments_t mFragments;
            ParsedError mError;
        };

        using FlushCallback = std::function<void(const RuntimeEvent&)>;

        explicit RuntimeEventAggregator(FlushCallback flush_callback);

        void ingest(const LLUUID& from_id,
                    const std::string& from_name,
                    const std::string& text,
                    VM vm,
                    Channel channel);
        void flushExpired();
        void flush();
        ParsedError parseError(VM vm,
                               const fragments_t& fragments) const;
        bool hasPending() const;

    private:
        struct PendingBurst
        {
            LLUUID      mFromID;
            std::string mFromName;
            VM          mVM{ VM::LSL2 };
            Channel     mChannel{ Channel::DEBUG };
            fragments_t mFragments;
            LLTimer     mTimer;
        };

        static constexpr F32 FRAGMENT_TIMEOUT = 1.0f;

        void flushPending();
        bool isNewBurst(const LLUUID& from_id,
                const std::string& from_name,
                VM vm,
                Channel channel) const;

        FlushCallback mFlushCallback;
        std::unique_ptr<PendingBurst> mPending;
    };

    struct RuntimeChatEvent
    {
        RuntimeEventAggregator::Channel mChannel;
        RuntimeEventAggregator::VM      mVM;
        LLUUID                          mRootID;
        LLUUID                          mPrimID;
        LLUUID                          mItemID;
        std::string                     mObjectName;
        std::string                     mScriptName;
        std::string                     mMessage;
        std::string                     mError;
        S32                             mLine{ 0 };
        S32                             mColumn{ 0 };
        std::vector<std::string>        mStack;
        bool                            mIsError{ false };
    };

    using RuntimeEventCallback =
        std::function<void(const RuntimeChatEvent&)>;

    explicit LLPublishedObjectMgr(
        LLScriptEditorWSServer* server = nullptr,
        RuntimeEventCallback runtime_event_callback = {});
    ~LLPublishedObjectMgr();

    void flushExpiredRuntimeFragments();
    void ingestRuntimeChat(
        const LLChat& chat_msg,
        RuntimeEventAggregator::Channel channel);
    std::optional<RuntimeChatEvent> buildRuntimeChatEvent(
        const RuntimeEventAggregator::RuntimeEvent& event) const;

    bool hasPublished(const LLUUID& object_id) const { return mPublishedObjects.find(object_id) != mPublishedObjects.end(); }
    void erasePublished(const LLUUID& object_id) { mPublishedObjects.erase(object_id); }
    PublishedObjectInfo* getPublished(const LLUUID& object_id);
    const PublishedObjectInfo* getPublished(const LLUUID& object_id) const;

    template <typename Fn>
    void forEachPublished(Fn&& fn) const
    {
        for (const auto& [id, info] : mPublishedObjects)
        {
            fn(id, info);
        }
    }

    LLSD buildPrimInventoryLLSD(LLViewerObject* object) const;
    LLSD buildPublishedObjectLLSD(LLViewerObject* root) const;
    LLSD buildObjectListLLSD() const;
    bool buildLinksetUpdateLLSD(const LLUUID& root_id, LLSD& update) const;
    bool reconcileLinksetChildAdded(const LLUUID& root_id, LLViewerObject* child, F64 request_start_sec);
    bool reconcileLinksetChildRemoved(const LLUUID& root_id, const LLUUID& child_id);
    bool handlePrimInventoryReadyEvent(const LLUUID& object_id, const LLUUID& prim_id);
    PrimInventoryEventResult handlePrimInventoryChangedEvent(
        const LLUUID& object_id,
        const LLUUID& prim_id,
        LLViewerObject* prim,
        F64 now_sec);
    InventoryChangeResult reconcileInventoryChanged(
        const LLUUID& object_id,
        const LLUUID& prim_id,
        LLViewerObject* prim);
    bool applyPropertyChange(
        const LLUUID& root_id,
        const LLUUID& prim_id,
        const std::string& name,
        const std::string& desc,
        LLSD& update);

    void beginPendingPublish(const LLUUID& object_id, const std::vector<LLViewerObject*>& prims);
    bool hasPendingPublish(const LLUUID& object_id) const;
    bool markPendingPublishPrimReady(const LLUUID& object_id, const LLUUID& prim_id);
    std::vector<std::unique_ptr<LLPublishedPrimListener>> takePendingPublishListeners(const LLUUID& object_id);
    void recordPendingPropertyChange(
        const LLUUID& root_id,
        const LLUUID& prim_id,
        const std::string& name,
        const std::string& desc);
    void cancelPendingPublish(const LLUUID& object_id);
    PublishedObjectInfo& finalizePendingPublish(const LLUUID& object_id, PublishedObjectInfo&& info);
    bool cleanupObjectStateForUnpublish(const LLUUID& object_id);
    void clearAllStateWithListenerCleanup();

    bool reservePendingItemCreate(const LLUUID& prim_id, std::string&& pump_name);
    bool consumePendingItemCreate(const LLUUID& prim_id, std::string& pump_name);
    void clearPendingItemCreate(const LLUUID& prim_id);

    bool consumePendingNewChild(const LLUUID& root_id, const LLUUID& child_id, bool& root_empty_after_remove);
    void clearPendingNewChildren(const LLUUID& root_id) { mNewChildPrims.erase(root_id); }

    bool hasActiveLinksetFlushTimer(const LLUUID& root_id) const;
    void setLinksetFlushTimer(const LLUUID& root_id, const std::weak_ptr<LLEventTimer>& timer);
    bool cancelLinksetFlushTimer(const LLUUID& root_id);
    void clearLinksetFlushTimer(const LLUUID& root_id);

    void setInventoryRequestStart(const LLUUID& prim_id, F64 start_sec) { mInventoryRequestStartSec[prim_id] = start_sec; }
    bool hasInventoryRequestStart(const LLUUID& prim_id) const { return mInventoryRequestStartSec.find(prim_id) != mInventoryRequestStartSec.end(); }
    bool consumeInventoryRequestStart(const LLUUID& prim_id, F64& start_sec);
    bool markPrimInventorySerialAndDetectChange(const LLUUID& root_id, const LLUUID& prim_id, S16 inventory_serial);

private:
    LLScriptEditorWSServer* mServer{ nullptr };
    std::unique_ptr<RuntimeEventAggregator> mRuntimeEventAggregator;
    RuntimeEventCallback mRuntimeEventCallback;
    std::unique_ptr<LLEventTimer> mRuntimeFlushTimer;

    static constexpr F32 RUNTIME_FLUSH_INTERVAL = 0.25f;

    void cancelPendingPublishWithCleanup(const LLUUID& object_id);
    void clearPublishedListeners(const LLUUID& object_id);

    using published_map_t = std::map<LLUUID, PublishedObjectInfo>;
    using pending_publish_map_t = std::map<LLUUID, PendingPublish>;
    using pending_item_create_map_t = std::map<LLUUID, std::string>;
    using new_child_prims_map_t = std::map<LLUUID, std::set<LLUUID>>;
    using linkset_flush_timer_map_t = std::map<LLUUID, std::weak_ptr<LLEventTimer>>;
    using inventory_request_start_map_t = std::map<LLUUID, F64>;

    published_map_t mPublishedObjects;
    pending_publish_map_t mPendingPublishes;
    pending_item_create_map_t mPendingItemCreates;
    new_child_prims_map_t mNewChildPrims;
    linkset_flush_timer_map_t mLinksetFlushTimers;
    inventory_request_start_map_t mInventoryRequestStartSec;
};
