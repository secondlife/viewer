/**
 * @file lldrawpoolterrain.cpp
 * @brief LLDrawPoolTerrain class implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#include "llviewerprecompiledheaders.h"

#include "lldrawpoolterrain.h"

#include "llfasttimer.h"

#include "llagent.h"
#include "llviewercontrol.h"
#include "lldrawable.h"
#include "llface.h"
#include "llsky.h"
#include "llsurface.h"
#include "llsurfacepatch.h"
#include "llviewerregion.h"
#include "llvlcomposition.h"
#include "llviewerparcelmgr.h"      // for gRenderParcelOwnership
#include "llviewerparceloverlay.h"
#include "llvosurfacepatch.h"
#include "llviewercamera.h"
#include "llviewertexturelist.h" // To get alpha gradients
#include "llworld.h"
#include "pipeline.h"
#include "llviewershadermgr.h"
#include "llrender.h"
#include "llenvironment.h"
#include "llsettingsvo.h"

const F32 DETAIL_SCALE = 1.f/16.f;
int DebugDetailMap = 0;

S32 LLDrawPoolTerrain::sPBRDetailMode = 0;
F32 LLDrawPoolTerrain::sDetailScale = DETAIL_SCALE;
F32 LLDrawPoolTerrain::sPBRDetailScale = DETAIL_SCALE;
static LLGLSLShader* sShader = NULL;
static LLTrace::BlockTimerStatHandle FTM_SHADOW_TERRAIN("Terrain Shadow");


LLDrawPoolTerrain::LLDrawPoolTerrain(LLViewerTexture *texturep) :
    LLFacePool(POOL_TERRAIN),
    mTexturep(texturep)
{
    // Hack!
    sDetailScale = 1.f/gSavedSettings.getF32("RenderTerrainScale");
    sPBRDetailScale = 1.f/gSavedSettings.getF32("RenderTerrainPBRScale");
    sPBRDetailMode = gSavedSettings.getS32("RenderTerrainPBRDetail");
    mAlphaRampImagep = LLViewerTextureManager::getFetchedTexture(IMG_ALPHA_GRAD);

    //gGL.getTexUnit(0)->bind(mAlphaRampImagep.get());
    mAlphaRampImagep->setAddressMode(LLTexUnit::TAM_CLAMP);

    m2DAlphaRampImagep = LLViewerTextureManager::getFetchedTexture(IMG_ALPHA_GRAD_2D);

    //gGL.getTexUnit(0)->bind(m2DAlphaRampImagep.get());
    m2DAlphaRampImagep->setAddressMode(LLTexUnit::TAM_CLAMP);

    mTexturep->setBoostLevel(LLGLTexture::BOOST_TERRAIN);

    //gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
}

LLDrawPoolTerrain::~LLDrawPoolTerrain()
{
    llassert( gPipeline.findPool( getType(), getTexture() ) == NULL );
}

U32 LLDrawPoolTerrain::getVertexDataMask()
{
    if (LLPipeline::sShadowRender)
    {
        return LLVertexBuffer::MAP_VERTEX;
    }
    else if (LLGLSLShader::sCurBoundShaderPtr)
    {
        return VERTEX_DATA_MASK & ~(LLVertexBuffer::MAP_TEXCOORD2 | LLVertexBuffer::MAP_TEXCOORD3);
    }
    else
    {
        return VERTEX_DATA_MASK;
    }
}

void LLDrawPoolTerrain::prerender()
{
    static LLCachedControl<S32> render_terrain_pbr_detail(gSavedSettings, "RenderTerrainPBRDetail");
    sPBRDetailMode = render_terrain_pbr_detail;
}

void LLDrawPoolTerrain::boostTerrainDetailTextures()
{
    // Hack! Get the region that this draw pool is rendering from!
    LLViewerRegion *regionp = mDrawFace[0]->getDrawable()->getVObj()->getRegion();
    LLVLComposition *compp = regionp->getComposition();
    compp->boost();
}

void LLDrawPoolTerrain::beginDeferredPass(S32 pass)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL; //LL_RECORD_BLOCK_TIME(FTM_RENDER_TERRAIN);
    LLFacePool::beginRenderPass(pass);
}

void LLDrawPoolTerrain::endDeferredPass(S32 pass)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL; //LL_RECORD_BLOCK_TIME(FTM_RENDER_TERRAIN);
    LLFacePool::endRenderPass(pass);
    sShader->unbind();
}

void LLDrawPoolTerrain::renderDeferred(S32 pass)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL; //LL_RECORD_BLOCK_TIME(FTM_RENDER_TERRAIN);
    if (mDrawFace.empty())
    {
        return;
    }

    boostTerrainDetailTextures();

    renderFullShader();

    // Special-case for land ownership feedback
    if (gSavedSettings.getBOOL("ShowParcelOwners"))
    {
        hilightParcelOwners();
    }

}

void LLDrawPoolTerrain::beginShadowPass(S32 pass)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL; //LL_RECORD_BLOCK_TIME(FTM_SHADOW_TERRAIN);
    LLFacePool::beginRenderPass(pass);
    gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
    gDeferredShadowProgram.bind();

    LLEnvironment& environment = LLEnvironment::instance();
    gDeferredShadowProgram.uniform1i(LLShaderMgr::SUN_UP_FACTOR, environment.getIsSunUp() ? 1 : 0);
}

void LLDrawPoolTerrain::endShadowPass(S32 pass)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL; //LL_RECORD_BLOCK_TIME(FTM_SHADOW_TERRAIN);
    LLFacePool::endRenderPass(pass);
    gDeferredShadowProgram.unbind();
}

void LLDrawPoolTerrain::renderShadow(S32 pass)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL; //LL_RECORD_BLOCK_TIME(FTM_SHADOW_TERRAIN);
    if (mDrawFace.empty())
    {
        return;
    }
    //LLGLEnable offset(GL_POLYGON_OFFSET);
    //glCullFace(GL_FRONT);
    drawLoop();
    //glCullFace(GL_BACK);
}


void LLDrawPoolTerrain::drawLoop()
{
    if (!mDrawFace.empty())
    {
        for (std::vector<LLFace*>::iterator iter = mDrawFace.begin();
             iter != mDrawFace.end(); iter++)
        {
            LLFace *facep = *iter;

            llassert(gGL.getMatrixMode() == LLRender::MM_MODELVIEW);
            LLRenderPass::applyModelMatrix(&facep->getDrawable()->getRegion()->mRenderMatrix);

            facep->renderIndexed();
        }
    }
}

void LLDrawPoolTerrain::renderFullShader()
{
    const bool use_local_materials = gLocalTerrainMaterials.makeMaterialsReady(true, false);
    // Hack! Get the region that this draw pool is rendering from!
    LLViewerRegion *regionp = mDrawFace[0]->getDrawable()->getVObj()->getRegion();
    LLVLComposition *compp = regionp->getComposition();
    const bool use_textures = !use_local_materials && (compp->getMaterialType() == LLTerrainMaterials::Type::TEXTURE);

    if (use_textures)
    {
        // Use textures
        sShader = &gDeferredTerrainProgram;
        sShader->bind();
        renderFullShaderTextures();
    }
    else
    {
        // Use materials
        U32 paint_type = use_local_materials ? gLocalTerrainMaterials.getPaintType() : compp->getPaintType();
        paint_type = llclamp(paint_type, 0, TERRAIN_PAINT_TYPE_COUNT);
        sShader = &gDeferredPBRTerrainProgram[paint_type];
        sShader->bind();
        renderFullShaderPBR(use_local_materials);
    }
}

void LLDrawPoolTerrain::renderFullShaderTextures()
{
    // Hack! Get the region that this draw pool is rendering from!
    LLViewerRegion *regionp = mDrawFace[0]->getDrawable()->getVObj()->getRegion();
    LLVLComposition *compp = regionp->getComposition();

    LLViewerTexture *detail_texture0p = compp->mDetailTextures[0];
    LLViewerTexture *detail_texture1p = compp->mDetailTextures[1];
    LLViewerTexture *detail_texture2p = compp->mDetailTextures[2];
    LLViewerTexture *detail_texture3p = compp->mDetailTextures[3];

    LLVector3d region_origin_global = gAgent.getRegion()->getOriginGlobal();
    F32 offset_x = (F32)fmod(region_origin_global.mdV[VX], 1.0/(F64)sDetailScale)*sDetailScale;
    F32 offset_y = (F32)fmod(region_origin_global.mdV[VY], 1.0/(F64)sDetailScale)*sDetailScale;

    LLVector4 tp0, tp1;

    tp0.setVec(sDetailScale, 0.0f, 0.0f, offset_x);
    tp1.setVec(0.0f, sDetailScale, 0.0f, offset_y);

    //
    // detail texture 0
    //
    S32 detail0 = sShader->enableTexture(LLViewerShaderMgr::TERRAIN_DETAIL0);
    gGL.getTexUnit(detail0)->bind(detail_texture0p);
    gGL.getTexUnit(detail0)->setTextureAddressMode(LLTexUnit::TAM_WRAP);
    gGL.getTexUnit(detail0)->activate();

    LLGLSLShader* shader = LLGLSLShader::sCurBoundShaderPtr;
    llassert(shader);

    shader->uniform4fv(LLShaderMgr::OBJECT_PLANE_S, 1, tp0.mV);
    shader->uniform4fv(LLShaderMgr::OBJECT_PLANE_T, 1, tp1.mV);

    LLSettingsWater::ptr_t pwater = LLEnvironment::instance().getCurrentWater();

    //
    // detail texture 1
    //
    S32 detail1 = sShader->enableTexture(LLViewerShaderMgr::TERRAIN_DETAIL1);
    gGL.getTexUnit(detail1)->bind(detail_texture1p);
    gGL.getTexUnit(detail1)->setTextureAddressMode(LLTexUnit::TAM_WRAP);
    gGL.getTexUnit(detail1)->activate();

    // detail texture 2
    //
    S32 detail2 = sShader->enableTexture(LLViewerShaderMgr::TERRAIN_DETAIL2);
    gGL.getTexUnit(detail2)->bind(detail_texture2p);
    gGL.getTexUnit(detail2)->setTextureAddressMode(LLTexUnit::TAM_WRAP);
    gGL.getTexUnit(detail2)->activate();


    // detail texture 3
    //
    S32 detail3 = sShader->enableTexture(LLViewerShaderMgr::TERRAIN_DETAIL3);
    gGL.getTexUnit(detail3)->bind(detail_texture3p);
    gGL.getTexUnit(detail3)->setTextureAddressMode(LLTexUnit::TAM_WRAP);
    gGL.getTexUnit(detail3)->activate();

    //
    // Alpha Ramp
    //
    S32 alpha_ramp = sShader->enableTexture(LLViewerShaderMgr::TERRAIN_ALPHARAMP);
    gGL.getTexUnit(alpha_ramp)->bind(m2DAlphaRampImagep);
    gGL.getTexUnit(alpha_ramp)->setTextureAddressMode(LLTexUnit::TAM_CLAMP);

    // GL_BLEND disabled by default
    drawLoop();

    // Disable textures
    sShader->disableTexture(LLViewerShaderMgr::TERRAIN_ALPHARAMP);
    sShader->disableTexture(LLViewerShaderMgr::TERRAIN_DETAIL0);
    sShader->disableTexture(LLViewerShaderMgr::TERRAIN_DETAIL1);
    sShader->disableTexture(LLViewerShaderMgr::TERRAIN_DETAIL2);
    sShader->disableTexture(LLViewerShaderMgr::TERRAIN_DETAIL3);

    gGL.getTexUnit(alpha_ramp)->unbind(LLTexUnit::TT_TEXTURE);
    gGL.getTexUnit(alpha_ramp)->disable();
    gGL.getTexUnit(alpha_ramp)->activate();

    gGL.getTexUnit(detail3)->unbind(LLTexUnit::TT_TEXTURE);
    gGL.getTexUnit(detail3)->disable();
    gGL.getTexUnit(detail3)->activate();

    gGL.getTexUnit(detail2)->unbind(LLTexUnit::TT_TEXTURE);
    gGL.getTexUnit(detail2)->disable();
    gGL.getTexUnit(detail2)->activate();

    gGL.getTexUnit(detail1)->unbind(LLTexUnit::TT_TEXTURE);
    gGL.getTexUnit(detail1)->disable();
    gGL.getTexUnit(detail1)->activate();

    //----------------------------------------------------------------------------
    // Restore Texture Unit 0 defaults

    gGL.getTexUnit(detail0)->unbind(LLTexUnit::TT_TEXTURE);
    gGL.getTexUnit(detail0)->enable(LLTexUnit::TT_TEXTURE);
    gGL.getTexUnit(detail0)->activate();
}

// *TODO: Investigate use of bindFast for PBR terrain textures
void LLDrawPoolTerrain::renderFullShaderPBR(bool use_local_materials)
{
    // Hack! Get the region that this draw pool is rendering from!
    LLViewerRegion *regionp = mDrawFace[0]->getDrawable()->getVObj()->getRegion();
    LLVLComposition *compp = regionp->getComposition();
    LLPointer<LLFetchedGLTFMaterial> (*fetched_materials)[LLVLComposition::ASSET_COUNT] = &compp->mDetailRenderMaterials;

    constexpr U32 terrain_material_count = LLVLComposition::ASSET_COUNT;
#ifdef SHOW_ASSERT
    constexpr U32 shader_material_count = 1 + LLViewerShaderMgr::TERRAIN_DETAIL3_BASE_COLOR - LLViewerShaderMgr::TERRAIN_DETAIL0_BASE_COLOR;
    llassert(shader_material_count == terrain_material_count);
#endif

    if (use_local_materials)
    {
        // Override region terrain with the global local override terrain
        fetched_materials = &gLocalTerrainMaterials.mDetailRenderMaterials;
    }
    const LLGLTFMaterial* materials[terrain_material_count];
    for (U32 i = 0; i < terrain_material_count; ++i)
    {
        materials[i] = (*fetched_materials)[i].get();
        if (!materials[i]) { materials[i] = &LLGLTFMaterial::sDefault; }
    }

    U32 paint_type = use_local_materials ? gLocalTerrainMaterials.getPaintType() : compp->getPaintType();
    paint_type = llclamp(paint_type, 0, TERRAIN_PAINT_TYPE_COUNT);

    S32 detail_basecolor[terrain_material_count];
    S32 detail_normal[terrain_material_count];
    S32 detail_metalrough[terrain_material_count];
    S32 detail_emissive[terrain_material_count];

    for (U32 i = 0; i < terrain_material_count; ++i)
    {
        LLViewerTexture* detail_basecolor_texturep = nullptr;
        LLViewerTexture* detail_normal_texturep = nullptr;
        LLViewerTexture* detail_metalrough_texturep = nullptr;
        LLViewerTexture* detail_emissive_texturep = nullptr;

        const LLFetchedGLTFMaterial* fetched_material = (*fetched_materials)[i].get();
        if (fetched_material)
        {
            detail_basecolor_texturep = fetched_material->mBaseColorTexture;
            detail_normal_texturep = fetched_material->mNormalTexture;
            detail_metalrough_texturep = fetched_material->mMetallicRoughnessTexture;
            detail_emissive_texturep = fetched_material->mEmissiveTexture;
        }

        detail_basecolor[i] = sShader->enableTexture(LLViewerShaderMgr::TERRAIN_DETAIL0_BASE_COLOR + i);
        if (detail_basecolor_texturep)
        {
            gGL.getTexUnit(detail_basecolor[i])->bind(detail_basecolor_texturep);
        }
        else
        {
            gGL.getTexUnit(detail_basecolor[i])->bind(LLViewerFetchedTexture::sWhiteImagep);
        }
        gGL.getTexUnit(detail_basecolor[i])->setTextureAddressMode(LLTexUnit::TAM_WRAP);
        gGL.getTexUnit(detail_basecolor[i])->activate();

        if (sPBRDetailMode >= TERRAIN_PBR_DETAIL_NORMAL)
        {
            detail_normal[i] = sShader->enableTexture(LLViewerShaderMgr::TERRAIN_DETAIL0_NORMAL + i);
            if (detail_normal_texturep)
            {
                gGL.getTexUnit(detail_normal[i])->bind(detail_normal_texturep);
            }
            else
            {
                gGL.getTexUnit(detail_normal[i])->bind(LLViewerFetchedTexture::sFlatNormalImagep);
            }
            gGL.getTexUnit(detail_normal[i])->setTextureAddressMode(LLTexUnit::TAM_WRAP);
            gGL.getTexUnit(detail_normal[i])->activate();
        }

        if (sPBRDetailMode >= TERRAIN_PBR_DETAIL_METALLIC_ROUGHNESS)
        {
            detail_metalrough[i] = sShader->enableTexture(LLViewerShaderMgr::TERRAIN_DETAIL0_METALLIC_ROUGHNESS + i);
            if (detail_metalrough_texturep)
            {
                gGL.getTexUnit(detail_metalrough[i])->bind(detail_metalrough_texturep);
            }
            else
            {
                gGL.getTexUnit(detail_metalrough[i])->bind(LLViewerFetchedTexture::sWhiteImagep);
            }
            gGL.getTexUnit(detail_metalrough[i])->setTextureAddressMode(LLTexUnit::TAM_WRAP);
            gGL.getTexUnit(detail_metalrough[i])->activate();
        }

        if (sPBRDetailMode >= TERRAIN_PBR_DETAIL_EMISSIVE)
        {
            detail_emissive[i] = sShader->enableTexture(LLViewerShaderMgr::TERRAIN_DETAIL0_EMISSIVE + i);
            if (detail_emissive_texturep)
            {
                gGL.getTexUnit(detail_emissive[i])->bind(detail_emissive_texturep);
            }
            else
            {
                gGL.getTexUnit(detail_emissive[i])->bind(LLViewerFetchedTexture::sWhiteImagep);
            }
            gGL.getTexUnit(detail_emissive[i])->setTextureAddressMode(LLTexUnit::TAM_WRAP);
            gGL.getTexUnit(detail_emissive[i])->activate();
        }
    }

    LLGLSLShader* shader = LLGLSLShader::sCurBoundShaderPtr;
    llassert(shader);

    // Like for PBR materials, PBR terrain texture transforms are defined by
    // the KHR_texture_transform spec, but with the following notable
    // differences:
    //   1) The PBR UV origin is defined as the Southwest corner of the region,
    //      with positive U facing East and positive V facing South.
    //   2) There is an additional scaling factor RenderTerrainPBRScale. If
    //      we've done our math right, RenderTerrainPBRScale should not affect the
    //      overall behavior of KHR_texture_transform
    //   3) There is only one texture transform per material, whereas
    //      KHR_texture_transform supports one texture transform per texture info.
    //      i.e. this isn't fully compliant with KHR_texture_transform, but is
    //      compliant when all texture infos used by a material have the same
    //      texture transform.
    LLGLTFMaterial::TextureTransform::PackTight transforms_packed[terrain_material_count];
    for (U32 i = 0; i < terrain_material_count; ++i)
    {
        const LLFetchedGLTFMaterial* fetched_material = (*fetched_materials)[i].get();
        LLGLTFMaterial::TextureTransform transform;
        if (fetched_material)
        {
            transform = fetched_material->mTextureTransform[LLGLTFMaterial::GLTF_TEXTURE_INFO_BASE_COLOR];
#ifdef SHOW_ASSERT
            // Assert condition where the contents of the texture transforms
            // differ per texture info - we currently don't support this case.
            for (U32 ti = 1; ti < LLGLTFMaterial::GLTF_TEXTURE_INFO_COUNT; ++ti)
            {
                llassert(fetched_material->mTextureTransform[0] == fetched_material->mTextureTransform[ti]);
            }
#endif
        }
        // *NOTE: Notice here we are combining the scale from
        // RenderTerrainPBRScale into the KHR_texture_transform. This only
        // works if the scale is uniform and no other transforms are
        // applied to the terrain UVs.
        transform.mScale.mV[VX] *= sPBRDetailScale;
        transform.mScale.mV[VY] *= sPBRDetailScale;

        transform.getPackedTight(transforms_packed[i]);
    }
    const U32 transform_param_count = LLGLTFMaterial::TextureTransform::PACK_TIGHT_SIZE * terrain_material_count;
    constexpr U32 vec4_size = 4;
    const U32 transform_vec4_count = (transform_param_count + (vec4_size - 1)) / vec4_size;
    llassert(transform_vec4_count == 5); // If false, need to update shader
    shader->uniform4fv(LLShaderMgr::TERRAIN_TEXTURE_TRANSFORMS, transform_vec4_count, (F32*)transforms_packed);

    LLSettingsWater::ptr_t pwater = LLEnvironment::instance().getCurrentWater();

    //
    // Alpha Ramp or paint map
    //
    S32 alpha_ramp = -1;
    S32 paint_map = -1;
    if (paint_type == TERRAIN_PAINT_TYPE_HEIGHTMAP_WITH_NOISE)
    {
        alpha_ramp = sShader->enableTexture(LLViewerShaderMgr::TERRAIN_ALPHARAMP);
        gGL.getTexUnit(alpha_ramp)->bind(m2DAlphaRampImagep);
        gGL.getTexUnit(alpha_ramp)->setTextureAddressMode(LLTexUnit::TAM_CLAMP);
    }
    else if (paint_type == TERRAIN_PAINT_TYPE_PBR_PAINTMAP)
    {
        paint_map = sShader->enableTexture(LLViewerShaderMgr::TERRAIN_PAINTMAP);
        LLViewerTexture* tex_paint_map = use_local_materials ? gLocalTerrainMaterials.getPaintMap() : compp->getPaintMap();
        // If no paintmap is available, fall back to rendering just material slot 1 (by binding the appropriate image)
        if (!tex_paint_map) { tex_paint_map = LLViewerTexture::sBlackImagep.get(); }
        // This is a paint map for four materials, but we save a channel by
        // storing the paintmap as the "difference" between slot 1 and the
        // other 3 slots.
        llassert(tex_paint_map->getComponents() == 3);
        gGL.getTexUnit(paint_map)->bind(tex_paint_map);
        gGL.getTexUnit(paint_map)->setTextureAddressMode(LLTexUnit::TAM_CLAMP);

        shader->uniform1f(LLShaderMgr::REGION_SCALE, regionp->getWidth());
    }

    //
    // GLTF uniforms
    //

    LLColor4 base_color_factors[terrain_material_count];
    F32 metallic_factors[terrain_material_count];
    F32 roughness_factors[terrain_material_count];
    LLColor3 emissive_colors[terrain_material_count];
    F32 minimum_alphas[terrain_material_count];
    for (U32 i = 0; i < terrain_material_count; ++i)
    {
        const LLGLTFMaterial* material = materials[i];

        base_color_factors[i] = material->mBaseColor;
        metallic_factors[i] = material->mMetallicFactor;
        roughness_factors[i] = material->mRoughnessFactor;
        emissive_colors[i] = material->mEmissiveColor;
        // glTF 2.0 Specification 3.9.4. Alpha Coverage
        // mAlphaCutoff is only valid for LLGLTFMaterial::ALPHA_MODE_MASK
        // Use 0 here due to GLTF terrain blending (LLGLTFMaterial::bind uses
        // -1 for easier debugging)
        F32 min_alpha = -0.0f;
        if (material->mAlphaMode == LLGLTFMaterial::ALPHA_MODE_MASK)
        {
            // dividing the alpha cutoff by transparency here allows the shader to compare against
            // the alpha value of the texture without needing the transparency value
            min_alpha = material->mAlphaCutoff/material->mBaseColor.mV[3];
        }
        minimum_alphas[i] = min_alpha;
    }
    shader->uniform4fv(LLShaderMgr::TERRAIN_BASE_COLOR_FACTORS, terrain_material_count, (F32*)base_color_factors);
    if (sPBRDetailMode >= TERRAIN_PBR_DETAIL_METALLIC_ROUGHNESS)
    {
        shader->uniform4f(LLShaderMgr::TERRAIN_METALLIC_FACTORS, metallic_factors[0], metallic_factors[1], metallic_factors[2], metallic_factors[3]);
        shader->uniform4f(LLShaderMgr::TERRAIN_ROUGHNESS_FACTORS, roughness_factors[0], roughness_factors[1], roughness_factors[2], roughness_factors[3]);
    }
    if (sPBRDetailMode >= TERRAIN_PBR_DETAIL_EMISSIVE)
    {
        shader->uniform3fv(LLShaderMgr::TERRAIN_EMISSIVE_COLORS, terrain_material_count, (F32*)emissive_colors);
    }
    shader->uniform4f(LLShaderMgr::TERRAIN_MINIMUM_ALPHAS, minimum_alphas[0], minimum_alphas[1], minimum_alphas[2], minimum_alphas[3]);

    // GL_BLEND disabled by default
    drawLoop();

    // Disable textures

    if (paint_type == TERRAIN_PAINT_TYPE_HEIGHTMAP_WITH_NOISE)
    {
        sShader->disableTexture(LLViewerShaderMgr::TERRAIN_ALPHARAMP);

        gGL.getTexUnit(alpha_ramp)->unbind(LLTexUnit::TT_TEXTURE);
        gGL.getTexUnit(alpha_ramp)->disable();
        gGL.getTexUnit(alpha_ramp)->activate();
    }
    else if (paint_type == TERRAIN_PAINT_TYPE_PBR_PAINTMAP)
    {
        sShader->disableTexture(LLViewerShaderMgr::TERRAIN_PAINTMAP);

        gGL.getTexUnit(paint_map)->unbind(LLTexUnit::TT_TEXTURE);
        gGL.getTexUnit(paint_map)->disable();
        gGL.getTexUnit(paint_map)->activate();
    }

    for (U32 i = 0; i < terrain_material_count; ++i)
    {
        sShader->disableTexture(LLViewerShaderMgr::TERRAIN_DETAIL0_BASE_COLOR + i);
        if (sPBRDetailMode >= TERRAIN_PBR_DETAIL_NORMAL)
        {
            sShader->disableTexture(LLViewerShaderMgr::TERRAIN_DETAIL0_NORMAL + i);
        }
        if (sPBRDetailMode >= TERRAIN_PBR_DETAIL_METALLIC_ROUGHNESS)
        {
            sShader->disableTexture(LLViewerShaderMgr::TERRAIN_DETAIL0_METALLIC_ROUGHNESS + i);
        }
        if (sPBRDetailMode >= TERRAIN_PBR_DETAIL_EMISSIVE)
        {
            sShader->disableTexture(LLViewerShaderMgr::TERRAIN_DETAIL0_EMISSIVE + i);
        }

        gGL.getTexUnit(detail_basecolor[i])->unbind(LLTexUnit::TT_TEXTURE);
        gGL.getTexUnit(detail_basecolor[i])->disable();
        gGL.getTexUnit(detail_basecolor[i])->activate();

        if (sPBRDetailMode >= TERRAIN_PBR_DETAIL_NORMAL)
        {
            gGL.getTexUnit(detail_normal[i])->unbind(LLTexUnit::TT_TEXTURE);
            gGL.getTexUnit(detail_normal[i])->disable();
            gGL.getTexUnit(detail_normal[i])->activate();
        }

        if (sPBRDetailMode >= TERRAIN_PBR_DETAIL_METALLIC_ROUGHNESS)
        {
            gGL.getTexUnit(detail_metalrough[i])->unbind(LLTexUnit::TT_TEXTURE);
            gGL.getTexUnit(detail_metalrough[i])->disable();
            gGL.getTexUnit(detail_metalrough[i])->activate();
        }

        if (sPBRDetailMode >= TERRAIN_PBR_DETAIL_EMISSIVE)
        {
            gGL.getTexUnit(detail_emissive[i])->unbind(LLTexUnit::TT_TEXTURE);
            gGL.getTexUnit(detail_emissive[i])->disable();
            gGL.getTexUnit(detail_emissive[i])->activate();
        }
    }
}

void LLDrawPoolTerrain::hilightParcelOwners()
{
    { //use fullbright shader for highlighting
        LLGLSLShader* old_shader = sShader;
        sShader->unbind();
        sShader = &gDeferredHighlightProgram;
        sShader->bind();
        gGL.diffuseColor4f(1, 1, 1, 1);
        LLGLEnable polyOffset(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(-1.0f, -1.0f);
        renderOwnership();
        sShader = old_shader;
        sShader->bind();
    }

}

void LLDrawPoolTerrain::renderFull4TU()
{
    // Hack! Get the region that this draw pool is rendering from!
    LLViewerRegion *regionp = mDrawFace[0]->getDrawable()->getVObj()->getRegion();
    LLVLComposition *compp = regionp->getComposition();
    LLViewerTexture *detail_texture0p = compp->mDetailTextures[0];
    LLViewerTexture *detail_texture1p = compp->mDetailTextures[1];
    LLViewerTexture *detail_texture2p = compp->mDetailTextures[2];
    LLViewerTexture *detail_texture3p = compp->mDetailTextures[3];

    LLVector3d region_origin_global = gAgent.getRegion()->getOriginGlobal();
    F32 offset_x = (F32)fmod(region_origin_global.mdV[VX], 1.0/(F64)sDetailScale)*sDetailScale;
    F32 offset_y = (F32)fmod(region_origin_global.mdV[VY], 1.0/(F64)sDetailScale)*sDetailScale;

    LLVector4 tp0, tp1;

    tp0.setVec(sDetailScale, 0.0f, 0.0f, offset_x);
    tp1.setVec(0.0f, sDetailScale, 0.0f, offset_y);

    gGL.blendFunc(LLRender::BF_ONE_MINUS_SOURCE_ALPHA, LLRender::BF_SOURCE_ALPHA);

    //----------------------------------------------------------------------------
    // Pass 1/1

    //
    // Stage 0: detail texture 0
    //
    gGL.getTexUnit(0)->activate();
    gGL.getTexUnit(0)->bind(detail_texture0p);

    glEnable(GL_TEXTURE_GEN_S);
    glEnable(GL_TEXTURE_GEN_T);
    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);

    glTexGenfv(GL_S, GL_OBJECT_PLANE, tp0.mV);
    glTexGenfv(GL_T, GL_OBJECT_PLANE, tp1.mV);

    //
    // Stage 1: Generate alpha ramp for detail0/detail1 transition
    //

    gGL.getTexUnit(1)->bind(m2DAlphaRampImagep.get());
    gGL.getTexUnit(1)->enable(LLTexUnit::TT_TEXTURE);
    gGL.getTexUnit(1)->activate();

    //
    // Stage 2: Interpolate detail1 with existing based on ramp
    //
    gGL.getTexUnit(2)->bind(detail_texture1p);
    gGL.getTexUnit(2)->enable(LLTexUnit::TT_TEXTURE);
    gGL.getTexUnit(2)->activate();

    glEnable(GL_TEXTURE_GEN_S);
    glEnable(GL_TEXTURE_GEN_T);
    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
    glTexGenfv(GL_S, GL_OBJECT_PLANE, tp0.mV);
    glTexGenfv(GL_T, GL_OBJECT_PLANE, tp1.mV);

    //
    // Stage 3: Modulate with primary (vertex) color for lighting
    //
    gGL.getTexUnit(3)->bind(detail_texture1p);
    gGL.getTexUnit(3)->enable(LLTexUnit::TT_TEXTURE);
    gGL.getTexUnit(3)->activate();

    gGL.getTexUnit(0)->activate();

    // GL_BLEND disabled by default
    drawLoop();

    //----------------------------------------------------------------------------
    // Second pass

    // Stage 0: Write detail3 into base
    //
    gGL.getTexUnit(0)->activate();
    gGL.getTexUnit(0)->bind(detail_texture3p);

    glEnable(GL_TEXTURE_GEN_S);
    glEnable(GL_TEXTURE_GEN_T);
    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
    glTexGenfv(GL_S, GL_OBJECT_PLANE, tp0.mV);
    glTexGenfv(GL_T, GL_OBJECT_PLANE, tp1.mV);

    //
    // Stage 1: Generate alpha ramp for detail2/detail3 transition
    //
    gGL.getTexUnit(1)->bind(m2DAlphaRampImagep);
    gGL.getTexUnit(1)->enable(LLTexUnit::TT_TEXTURE);
    gGL.getTexUnit(1)->activate();

    // Set the texture matrix
    gGL.matrixMode(LLRender::MM_TEXTURE);
    gGL.loadIdentity();
    gGL.translatef(-2.f, 0.f, 0.f);

    //
    // Stage 2: Interpolate detail2 with existing based on ramp
    //
    gGL.getTexUnit(2)->bind(detail_texture2p);
    gGL.getTexUnit(2)->enable(LLTexUnit::TT_TEXTURE);
    gGL.getTexUnit(2)->activate();

    glEnable(GL_TEXTURE_GEN_S);
    glEnable(GL_TEXTURE_GEN_T);
    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
    glTexGenfv(GL_S, GL_OBJECT_PLANE, tp0.mV);
    glTexGenfv(GL_T, GL_OBJECT_PLANE, tp1.mV);

    //
    // Stage 3: Generate alpha ramp for detail1/detail2 transition
    //
    gGL.getTexUnit(3)->bind(m2DAlphaRampImagep);
    gGL.getTexUnit(3)->enable(LLTexUnit::TT_TEXTURE);
    gGL.getTexUnit(3)->activate();

    // Set the texture matrix
    gGL.matrixMode(LLRender::MM_TEXTURE);
    gGL.loadIdentity();
    gGL.translatef(-1.f, 0.f, 0.f);
    gGL.matrixMode(LLRender::MM_MODELVIEW);

    gGL.getTexUnit(0)->activate();
    {
        LLGLEnable blend(GL_BLEND);
        drawLoop();
    }

    LLVertexBuffer::unbind();
    // Disable textures
    gGL.getTexUnit(3)->unbind(LLTexUnit::TT_TEXTURE);
    gGL.getTexUnit(3)->disable();
    gGL.getTexUnit(3)->activate();

    gGL.matrixMode(LLRender::MM_TEXTURE);
    gGL.loadIdentity();
    gGL.matrixMode(LLRender::MM_MODELVIEW);

    gGL.getTexUnit(2)->unbind(LLTexUnit::TT_TEXTURE);
    gGL.getTexUnit(2)->disable();
    gGL.getTexUnit(2)->activate();

    glDisable(GL_TEXTURE_GEN_S);
    glDisable(GL_TEXTURE_GEN_T);
    gGL.matrixMode(LLRender::MM_TEXTURE);
    gGL.loadIdentity();
    gGL.matrixMode(LLRender::MM_MODELVIEW);

    gGL.getTexUnit(1)->unbind(LLTexUnit::TT_TEXTURE);
    gGL.getTexUnit(1)->disable();
    gGL.getTexUnit(1)->activate();

    gGL.matrixMode(LLRender::MM_TEXTURE);
    gGL.loadIdentity();
    gGL.matrixMode(LLRender::MM_MODELVIEW);

    // Restore blend state
    gGL.setSceneBlendType(LLRender::BT_ALPHA);

    //----------------------------------------------------------------------------
    // Restore Texture Unit 0 defaults

    gGL.getTexUnit(0)->activate();
    gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);


    glDisable(GL_TEXTURE_GEN_S);
    glDisable(GL_TEXTURE_GEN_T);
    gGL.matrixMode(LLRender::MM_TEXTURE);
    gGL.loadIdentity();
    gGL.matrixMode(LLRender::MM_MODELVIEW);
}

void LLDrawPoolTerrain::renderFull2TU()
{
    // Hack! Get the region that this draw pool is rendering from!
    LLViewerRegion *regionp = mDrawFace[0]->getDrawable()->getVObj()->getRegion();
    LLVLComposition *compp = regionp->getComposition();
    LLViewerTexture *detail_texture0p = compp->mDetailTextures[0];
    LLViewerTexture *detail_texture1p = compp->mDetailTextures[1];
    LLViewerTexture *detail_texture2p = compp->mDetailTextures[2];
    LLViewerTexture *detail_texture3p = compp->mDetailTextures[3];

    LLVector3d region_origin_global = gAgent.getRegion()->getOriginGlobal();
    F32 offset_x = (F32)fmod(region_origin_global.mdV[VX], 1.0/(F64)sDetailScale)*sDetailScale;
    F32 offset_y = (F32)fmod(region_origin_global.mdV[VY], 1.0/(F64)sDetailScale)*sDetailScale;

    LLVector4 tp0, tp1;

    tp0.setVec(sDetailScale, 0.0f, 0.0f, offset_x);
    tp1.setVec(0.0f, sDetailScale, 0.0f, offset_y);

    gGL.blendFunc(LLRender::BF_ONE_MINUS_SOURCE_ALPHA, LLRender::BF_SOURCE_ALPHA);

    //----------------------------------------------------------------------------
    // Pass 1/4

    //
    // Stage 0: Render detail 0 into base
    //
    gGL.getTexUnit(0)->bind(detail_texture0p);
    glEnable(GL_TEXTURE_GEN_S);
    glEnable(GL_TEXTURE_GEN_T);
    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);

    glTexGenfv(GL_S, GL_OBJECT_PLANE, tp0.mV);
    glTexGenfv(GL_T, GL_OBJECT_PLANE, tp1.mV);

    drawLoop();

    //----------------------------------------------------------------------------
    // Pass 2/4

    //
    // Stage 0: Generate alpha ramp for detail0/detail1 transition
    //
    gGL.getTexUnit(0)->bind(m2DAlphaRampImagep);

    glDisable(GL_TEXTURE_GEN_S);
    glDisable(GL_TEXTURE_GEN_T);

    //
    // Stage 1: Write detail1
    //
    gGL.getTexUnit(1)->bind(detail_texture1p);
    gGL.getTexUnit(1)->enable(LLTexUnit::TT_TEXTURE);
    gGL.getTexUnit(1)->activate();

    glEnable(GL_TEXTURE_GEN_S);
    glEnable(GL_TEXTURE_GEN_T);
    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
    glTexGenfv(GL_S, GL_OBJECT_PLANE, tp0.mV);
    glTexGenfv(GL_T, GL_OBJECT_PLANE, tp1.mV);

    gGL.getTexUnit(0)->activate();
    {
        LLGLEnable blend(GL_BLEND);
        drawLoop();
    }
    //----------------------------------------------------------------------------
    // Pass 3/4

    //
    // Stage 0: Generate alpha ramp for detail1/detail2 transition
    //
    gGL.getTexUnit(0)->bind(m2DAlphaRampImagep);

    // Set the texture matrix
    gGL.matrixMode(LLRender::MM_TEXTURE);
    gGL.loadIdentity();
    gGL.translatef(-1.f, 0.f, 0.f);
    gGL.matrixMode(LLRender::MM_MODELVIEW);

    //
    // Stage 1: Write detail2
    //
    gGL.getTexUnit(1)->bind(detail_texture2p);
    gGL.getTexUnit(1)->enable(LLTexUnit::TT_TEXTURE);
    gGL.getTexUnit(1)->activate();

    glEnable(GL_TEXTURE_GEN_S);
    glEnable(GL_TEXTURE_GEN_T);
    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
    glTexGenfv(GL_S, GL_OBJECT_PLANE, tp0.mV);
    glTexGenfv(GL_T, GL_OBJECT_PLANE, tp1.mV);

    {
        LLGLEnable blend(GL_BLEND);
        drawLoop();
    }

    //----------------------------------------------------------------------------
    // Pass 4/4

    //
    // Stage 0: Generate alpha ramp for detail2/detail3 transition
    //
    gGL.getTexUnit(0)->activate();
    gGL.getTexUnit(0)->bind(m2DAlphaRampImagep);
    // Set the texture matrix
    gGL.matrixMode(LLRender::MM_TEXTURE);
    gGL.loadIdentity();
    gGL.translatef(-2.f, 0.f, 0.f);
    gGL.matrixMode(LLRender::MM_MODELVIEW);

    // Stage 1: Write detail3
    gGL.getTexUnit(1)->bind(detail_texture3p);
    gGL.getTexUnit(1)->enable(LLTexUnit::TT_TEXTURE);
    gGL.getTexUnit(1)->activate();

    glEnable(GL_TEXTURE_GEN_S);
    glEnable(GL_TEXTURE_GEN_T);
    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
    glTexGenfv(GL_S, GL_OBJECT_PLANE, tp0.mV);
    glTexGenfv(GL_T, GL_OBJECT_PLANE, tp1.mV);

    gGL.getTexUnit(0)->activate();
    {
        LLGLEnable blend(GL_BLEND);
        drawLoop();
    }

    // Restore blend state
    gGL.setSceneBlendType(LLRender::BT_ALPHA);

    // Disable textures

    gGL.getTexUnit(1)->unbind(LLTexUnit::TT_TEXTURE);
    gGL.getTexUnit(1)->disable();
    gGL.getTexUnit(1)->activate();

    glDisable(GL_TEXTURE_GEN_S);
    glDisable(GL_TEXTURE_GEN_T);
    gGL.matrixMode(LLRender::MM_TEXTURE);
    gGL.loadIdentity();
    gGL.matrixMode(LLRender::MM_MODELVIEW);

    //----------------------------------------------------------------------------
    // Restore Texture Unit 0 defaults

    gGL.getTexUnit(0)->activate();
    gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);

    glDisable(GL_TEXTURE_GEN_S);
    glDisable(GL_TEXTURE_GEN_T);
    gGL.matrixMode(LLRender::MM_TEXTURE);
    gGL.loadIdentity();
    gGL.matrixMode(LLRender::MM_MODELVIEW);
}


void LLDrawPoolTerrain::renderSimple()
{
    LLVector4 tp0, tp1;

    //----------------------------------------------------------------------------
    // Pass 1/1

    // Stage 0: Base terrain texture pass
    mTexturep->addTextureStats(1024.f*1024.f);

    gGL.getTexUnit(0)->activate();
    gGL.getTexUnit(0)->enable(LLTexUnit::TT_TEXTURE);
    gGL.getTexUnit(0)->bind(mTexturep);

    LLVector3 origin_agent = mDrawFace[0]->getDrawable()->getVObj()->getRegion()->getOriginAgent();
    F32 tscale = 1.f/256.f;
    tp0.setVec(tscale, 0.f, 0.0f, -1.f*(origin_agent.mV[0]/256.f));
    tp1.setVec(0.f, tscale, 0.0f, -1.f*(origin_agent.mV[1]/256.f));

    sShader->uniform4fv(LLShaderMgr::OBJECT_PLANE_S, 1, tp0.mV);
    sShader->uniform4fv(LLShaderMgr::OBJECT_PLANE_T, 1, tp1.mV);

    drawLoop();

    //----------------------------------------------------------------------------
    // Restore Texture Unit 0 defaults

    gGL.getTexUnit(0)->activate();
    gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
    gGL.matrixMode(LLRender::MM_TEXTURE);
    gGL.loadIdentity();
    gGL.matrixMode(LLRender::MM_MODELVIEW);
}

//============================================================================

void LLDrawPoolTerrain::renderOwnership()
{
    LLGLSPipelineAlpha gls_pipeline_alpha;

    llassert(!mDrawFace.empty());

    // Each terrain pool is associated with a single region.
    // We need to peek back into the viewer's data to find out
    // which ownership overlay texture to use.
    LLFace                  *facep              = mDrawFace[0];
    LLDrawable              *drawablep          = facep->getDrawable();
    const LLViewerObject    *objectp                = drawablep->getVObj();
    const LLVOSurfacePatch  *vo_surface_patchp  = (LLVOSurfacePatch *)objectp;
    LLSurfacePatch          *surface_patchp     = vo_surface_patchp->getPatch();
    LLSurface               *surfacep           = surface_patchp->getSurface();
    LLViewerRegion          *regionp            = surfacep->getRegion();
    LLViewerParcelOverlay   *overlayp           = regionp->getParcelOverlay();
    LLViewerTexture         *texturep           = overlayp->getTexture();

    gGL.getTexUnit(0)->bind(texturep);

    // *NOTE: Because the region is 256 meters wide, but has 257 pixels, the
    // texture coordinates for pixel 256x256 is not 1,1. This makes the
    // ownership map not line up with the selection. We address this with
    // a texture matrix multiply.
    gGL.matrixMode(LLRender::MM_TEXTURE);
    gGL.pushMatrix();

    const F32 TEXTURE_FUDGE = 257.f / 256.f;
    gGL.scalef( TEXTURE_FUDGE, TEXTURE_FUDGE, 1.f );
    for (std::vector<LLFace*>::iterator iter = mDrawFace.begin();
         iter != mDrawFace.end(); iter++)
    {
        LLFace *facep = *iter;
        facep->renderIndexed();
    }

    gGL.matrixMode(LLRender::MM_TEXTURE);
    gGL.popMatrix();
    gGL.matrixMode(LLRender::MM_MODELVIEW);
}


void LLDrawPoolTerrain::dirtyTextures(const std::set<LLViewerFetchedTexture*>& textures)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL;
    LLViewerFetchedTexture* tex = LLViewerTextureManager::staticCastToFetchedTexture(mTexturep) ;
    if (tex && textures.find(tex) != textures.end())
    {
        for (std::vector<LLFace*>::iterator iter = mReferences.begin();
             iter != mReferences.end(); iter++)
        {
            LLFace *facep = *iter;
            gPipeline.markTextured(facep->getDrawable());
        }
    }
}

LLViewerTexture *LLDrawPoolTerrain::getTexture()
{
    return mTexturep;
}

LLViewerTexture *LLDrawPoolTerrain::getDebugTexture()
{
    return mTexturep;
}


LLColor3 LLDrawPoolTerrain::getDebugColor() const
{
    return LLColor3(0.f, 0.f, 1.f);
}
