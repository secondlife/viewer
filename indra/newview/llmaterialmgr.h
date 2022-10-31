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

#ifndef LL_LLMATERIALMGR_H
#define LL_LLMATERIALMGR_H

#include "llmaterial.h"
#include "llmaterialid.h"
#include "llsingleton.h"
#include "httprequest.h"
#include "httpheaders.h"
#include "httpoptions.h"

class LLViewerRegion;

class LLMaterialMgr : public LLSingleton<LLMaterialMgr>
{
    LLSINGLETON(LLMaterialMgr);
    virtual ~LLMaterialMgr();

public:
    typedef std::map<LLMaterialID, LLMaterialPtr> material_map_t;

    typedef boost::signals2::signal<void (const LLMaterialID&, const LLMaterialPtr)> get_callback_t;
    const LLMaterialPtr         get(const LLUUID& region_id, const LLMaterialID& material_id);
    boost::signals2::connection get(const LLUUID& region_id, const LLMaterialID& material_id, get_callback_t::slot_type cb);

    typedef boost::signals2::signal<void (const LLMaterialID&, const LLMaterialPtr, U32 te)> get_callback_te_t;
    boost::signals2::connection getTE(const LLUUID& region_id, const LLMaterialID& material_id, U32 te, get_callback_te_t::slot_type cb);

    typedef boost::signals2::signal<void (const LLUUID&, const material_map_t&)> getall_callback_t;
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
    // struct for TE-specific material ID query
    class TEMaterialPair
    {
    public:

        U32 te;
        LLMaterialID materialID;

        bool operator==(const TEMaterialPair& b) const { return (materialID == b.materialID) && (te == b.te); }
    };
    
    friend inline bool operator<(
        const LLMaterialMgr::TEMaterialPair& lhs,
        const LLMaterialMgr::TEMaterialPair& rhs)
    {
        return (lhs.te  < rhs.te) ? TRUE :
            (lhs.materialID < rhs.materialID);
    }

    struct TEMaterialPairHasher
    {
        enum { bucket_size = 8 };
        size_t operator()(const TEMaterialPair& key_value) const { return *((size_t*)key_value.materialID.get());  } // cheesy, but effective
        bool   operator()(const TEMaterialPair& left, const TEMaterialPair& right) const { return left < right; }
    };

    typedef std::set<LLMaterialID> material_queue_t;
    typedef std::map<LLUUID, material_queue_t> get_queue_t;
    typedef std::pair<const LLUUID, LLMaterialID> pending_material_t;
    typedef std::map<const pending_material_t, F64> get_pending_map_t;
    typedef std::map<LLMaterialID, get_callback_t*> get_callback_map_t;


    typedef boost::unordered_map<TEMaterialPair, get_callback_te_t*, TEMaterialPairHasher> get_callback_te_map_t;
    typedef std::set<LLUUID> getall_queue_t;
    typedef std::map<LLUUID, F64> getall_pending_map_t;
    typedef std::map<LLUUID, getall_callback_t*> getall_callback_map_t;
    typedef std::map<U8, LLMaterial> facematerial_map_t;
    typedef std::map<LLUUID, facematerial_map_t> put_queue_t;


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
    LLCore::HttpRequest::priority_t mHttpPriority;

    U32 getMaxEntries(const LLViewerRegion* regionp);
};

#endif // LL_LLMATERIALMGR_H

