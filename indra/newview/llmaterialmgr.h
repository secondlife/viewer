/**
 * @file llmaterialmgr.h
 * @brief Material manager
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
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

#pragma once

#include "llmaterial.h"
#include "llmaterialid.h"
#include "llsingleton.h"
#include "httprequest.h"
#include "httpheaders.h"
#include "httpoptions.h"
#include <boost/container_hash/hash.hpp>

class LLViewerRegion;

// struct for TE-specific material ID query
class TEMaterialPair
{
public:
    U32          te;
    LLMaterialID materialID;

    bool operator==(const TEMaterialPair& b) const { return (materialID == b.materialID) && (te == b.te); }
};

inline bool operator<(const TEMaterialPair& lhs, const TEMaterialPair& rhs)
{
    return (lhs.te < rhs.te) ? true : (lhs.materialID < rhs.materialID);
}

// std::hash implementation for TEMaterialPair
namespace std
{
    template<>
    struct hash<TEMaterialPair>
    {
        inline size_t operator()(const TEMaterialPair& p) const noexcept
        {
            // Utilize boost::hash_combine to generate a good hash
            size_t seed = 0;
            boost::hash_combine(seed, p.te + 1);
            boost::hash_combine(seed, p.materialID);
            return seed;
        }
    };
} // namespace std

class LLMaterialMgr : public LLSingleton<LLMaterialMgr>
{
    LLSINGLETON(LLMaterialMgr);
    virtual ~LLMaterialMgr();

public:
    using material_map_t = std::map<LLMaterialID, LLMaterialPtr>;

    using get_callback_t = boost::signals2::signal<void (const LLMaterialID&, const LLMaterialPtr)>;
    const LLMaterialPtr         get(const LLUUID& region_id, const LLMaterialID& material_id);
    boost::signals2::connection get(const LLUUID& region_id, const LLMaterialID& material_id, get_callback_t::slot_type cb);

    using get_callback_te_t = boost::signals2::signal<void (const LLMaterialID&, const LLMaterialPtr, U32 te)>;
    boost::signals2::connection getTE(const LLUUID& region_id, const LLMaterialID& material_id, U32 te, get_callback_te_t::slot_type cb);

    using getall_callback_t = boost::signals2::signal<void (const LLUUID&, const material_map_t&)>;
    void                        getAll(const LLUUID& region_id);
    boost::signals2::connection getAll(const LLUUID& region_id, getall_callback_t::slot_type cb);
    void put(const LLUUID& object_id, const U8 te, const LLMaterial& material);
    void remove(const LLUUID& object_id, const U8 te);

    //explicitly add new material to material manager
    void setLocalMaterial(const LLUUID& region_id, LLMaterialPtr material_ptr);

private:
    void clearGetQueues(const LLUUID& region_id);
    bool isGetPending(const LLUUID& region_id, const LLMaterialID& material_id) const;
    bool isGetAllPending(const LLUUID& region_id) const;
    void markGetPending(const LLUUID& region_id, const LLMaterialID& material_id);
    const LLMaterialPtr setMaterial(const LLUUID& region_id, const LLMaterialID& material_id, const LLSD& material_data);
    void setMaterialCallbacks(const LLMaterialID& material_id, const LLMaterialPtr material_ptr);

    static void onIdle(void*);

    static void CapsRecvForRegion(const LLUUID& regionId, LLUUID regionTest, std::string pumpname);

    void processGetQueue();
    void processGetQueueCoro();
    void onGetResponse(bool success, const LLSD& content, const LLUUID& region_id);
    void processGetAllQueue();
    void processGetAllQueueCoro(LLUUID regionId);
    void onGetAllResponse(bool success, const LLSD& content, const LLUUID& region_id);
    void processPutQueue();
    void onPutResponse(bool success, const LLSD& content);
    void onRegionRemoved(LLViewerRegion* regionp);

private:
    using material_queue_t = std::set<LLMaterialID>;
    using get_queue_t = std::map<LLUUID, material_queue_t>;
    using pending_material_t = std::pair<const LLUUID, LLMaterialID>;
    using get_pending_map_t = std::map<const pending_material_t, F64>;
    using get_callback_map_t = std::map<LLMaterialID, get_callback_t*>;


    using get_callback_te_map_t = std::unordered_map<TEMaterialPair, get_callback_te_t*>;
    using getall_queue_t = std::set<LLUUID>;
    using getall_pending_map_t = std::map<LLUUID, F64>;
    using getall_callback_map_t = std::map<LLUUID, getall_callback_t*>;
    using facematerial_map_t = std::map<U8, LLMaterial>;
    using put_queue_t = std::map<LLUUID, facematerial_map_t>;


    get_queue_t             mGetQueue;
    uuid_set_t              mRegionGets;
    get_pending_map_t       mGetPending;
    get_callback_map_t      mGetCallbacks;

    get_callback_te_map_t   mGetTECallbacks;
    getall_queue_t          mGetAllQueue;
    getall_queue_t          mGetAllRequested;
    getall_pending_map_t    mGetAllPending;
    getall_callback_map_t   mGetAllCallbacks;
    put_queue_t             mPutQueue;
    material_map_t          mMaterials;

    LLCore::HttpRequest::ptr_t      mHttpRequest;
    LLCore::HttpHeaders::ptr_t      mHttpHeaders;
    LLCore::HttpOptions::ptr_t      mHttpOptions;
    LLCore::HttpRequest::policy_t   mHttpPolicy;

    U32 getMaxEntries(const LLViewerRegion* regionp);
};


