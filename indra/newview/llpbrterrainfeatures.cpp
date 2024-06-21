/**
 * @file   llpbrterrainfeatures.cpp
 *
 * $LicenseInfo:firstyear=2024&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2024, Linden Research, Inc.
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

#include "llpbrterrainfeatures.h"

#include "llappviewer.h"
#include "llgltfmaterial.h"
#include "llviewerregion.h"
#include "llvlcomposition.h"

LLPBRTerrainFeatures gPBRTerrainFeatures;

// static
void LLPBRTerrainFeatures::queueQuery(LLViewerRegion& region, void(*done_callback)(LLUUID, bool, const LLModifyRegion&))
{
    llassert(on_main_thread());
    llassert(LLCoros::on_main_coro());

    LLUUID region_id = region.getRegionID();

    LLCoros::instance().launch("queryRegionCoro",
        std::bind(&LLPBRTerrainFeatures::queryRegionCoro,
            region.getCapability("ModifyRegion"),
            region_id,
            done_callback));
}

// static
void LLPBRTerrainFeatures::queueModify(LLViewerRegion& region, const LLModifyRegion& composition)
{
    llassert(on_main_thread());
    llassert(LLCoros::on_main_coro());

    LLSD updates = LLSD::emptyMap();

    LLSD override_updates = LLSD::emptyArray();
    for (S32 i = 0; i < LLTerrainMaterials::ASSET_COUNT; ++i)
    {
        const LLGLTFMaterial* material_override = composition.getMaterialOverride(i);
        LLSD override_update;
        if (material_override)
        {
            LLGLTFMaterial::sDefault.getOverrideLLSD(*material_override, override_update);
        }
        else
        {
            override_update = LLSD::emptyMap();
        }
        override_updates.append(override_update);
    }
    updates["overrides"] = override_updates;

    LLCoros::instance().launch("modifyRegionCoro",
        std::bind(&LLPBRTerrainFeatures::modifyRegionCoro,
            region.getCapability("ModifyRegion"),
            updates,
            nullptr));
}

// static
void LLPBRTerrainFeatures::queryRegionCoro(std::string cap_url, LLUUID region_id, void(*done_callback)(LLUUID, bool, const LLModifyRegion&) )
{
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("queryRegionCoro", httpPolicy));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);
    LLCore::HttpOptions::ptr_t httpOpts(new LLCore::HttpOptions);
    LLCore::HttpHeaders::ptr_t httpHeaders;

    httpOpts->setFollowRedirects(true);

    LL_DEBUGS("GLTF") << "Querying features via ModifyRegion endpoint" << LL_ENDL;

    LLSD result = httpAdapter->getAndSuspend(httpRequest, cap_url, httpOpts, httpHeaders);

    LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    bool success = true;
    if (!status || !result["success"].asBoolean())
    {
        if (result["message"].isUndefined())
        {
            LL_WARNS("PBRTerrain") << "Failed to query PBR terrain features." << LL_ENDL;
        }
        else
        {
            LL_WARNS("PBRTerrain") << "Failed to query PBR terrain features: " << result["message"] << LL_ENDL;
        }
        success = false;
    }

    LLTerrainMaterials* composition = new LLTerrainMaterials();

    if (success)
    {
        const LLSD& overrides = result["overrides"];
        if (!overrides.isArray() || overrides.size() < LLTerrainMaterials::ASSET_COUNT)
        {
            LL_WARNS("PBRTerrain") << "Invalid composition format: Missing/invalid overrides" << LL_ENDL;
            success = false;
        }
        else
        {
            for (S32 i = 0; i < LLTerrainMaterials::ASSET_COUNT; ++i)
            {
                const LLSD& override_llsd = overrides[i];
                LLPointer<LLGLTFMaterial> material_override = new LLGLTFMaterial();
                material_override->applyOverrideLLSD(override_llsd);
                if (*material_override == LLGLTFMaterial::sDefault)
                {
                    material_override = nullptr;
                }
                composition->setMaterialOverride(i, material_override.get());
            }
        }
    }

    if (done_callback)
    {
        LLAppViewer::instance()->postToMainCoro([=]()
        {
            done_callback(region_id, success, *composition);
            delete composition;
        });
    }
    else
    {
        delete composition;
    }
}

// static
void LLPBRTerrainFeatures::modifyRegionCoro(std::string cap_url, LLSD updates, void(*done_callback)(bool) )
{
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("modifyRegionCoro", httpPolicy));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);
    LLCore::HttpOptions::ptr_t httpOpts(new LLCore::HttpOptions);
    LLCore::HttpHeaders::ptr_t httpHeaders;

    httpOpts->setFollowRedirects(true);

    LL_DEBUGS("GLTF") << "Applying features via ModifyRegion endpoint: " << updates << LL_ENDL;

    LLSD result = httpAdapter->postAndSuspend(httpRequest, cap_url, updates, httpOpts, httpHeaders);

    LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    bool success = true;
    if (!status || !result["success"].asBoolean())
    {
        if (result["message"].isUndefined())
        {
            LL_WARNS("PBRTerrain") << "Failed to modify PBR terrain features." << LL_ENDL;
        }
        else
        {
            LL_WARNS("PBRTerrain") << "Failed to modify PBR terrain features: " << result["message"] << LL_ENDL;
        }
        success = false;
    }

    if (done_callback)
    {
        LLAppViewer::instance()->postToMainCoro([=]()
        {
            done_callback(success);
        });
    }
}

