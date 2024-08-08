/**
 * @file llterrainpaintmap.cpp
 * @brief Utilities for managing terrain paint maps
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#include "llterrainpaintmap.h"

#include "llviewerprecompiledheaders.h"

// library includes
#include "llglslshader.h"
#include "llrendertarget.h"
#include "llvertexbuffer.h"

// newview includes
#include "llrender.h"
#include "llsurface.h"
#include "llsurfacepatch.h"
#include "llviewercamera.h"
#include "llviewercontrol.h"
#include "llviewerregion.h"
#include "llviewershadermgr.h"
#include "llviewertexture.h"

namespace
{
#ifdef SHOW_ASSERT
void check_tex(const LLViewerTexture& tex)
{
    llassert(tex.getComponents() == 3);
    llassert(tex.getWidth() > 0 && tex.getHeight() > 0);
    llassert(tex.getWidth() == tex.getHeight());
    llassert(tex.getPrimaryFormat() == GL_RGB);
    llassert(tex.getGLTexture());
}
#endif
} // namespace

// static
bool LLTerrainPaintMap::bakeHeightNoiseIntoPBRPaintMapRGB(const LLViewerRegion& region, LLViewerTexture& tex)
{
#ifdef SHOW_ASSERT
    check_tex(tex);
#endif

    const LLSurface& surface = region.getLand();
    const U32 patch_count = surface.getPatchesPerEdge();

    // *TODO: mHeightsGenerated isn't guaranteed to be true. Assume terrain is
    // loaded for now. Would be nice to fix the loading issue or find a better
    // heuristic to determine that the terrain is sufficiently loaded.
#if 0
    // Don't proceed if the region heightmap isn't loaded
    for (U32 rj = 0; rj < patch_count; ++rj)
    {
        for (U32 ri = 0; ri < patch_count; ++ri)
        {
            const LLSurfacePatch* patch = surface.getPatch(ri, rj);
            if (!patch->isHeightsGenerated())
            {
                LL_WARNS() << "Region heightmap not fully loaded" << LL_ENDL;
                return false;
            }
        }
    }
#endif

    // Bind the debug shader and render terrain to tex
    // Use a scratch render target because its dimensions may exceed the standard bake target, and this is a one-off bake
    LLRenderTarget scratch_target;
    const S32 dim = llmin(tex.getWidth(), tex.getHeight());
    scratch_target.allocate(dim, dim, GL_RGB, false, LLTexUnit::eTextureType::TT_TEXTURE,
                                   LLTexUnit::eTextureMipGeneration::TMG_NONE);
    if (!scratch_target.isComplete())
    {
        llassert(false);
        LL_WARNS() << "Failed to allocate render target" << LL_ENDL;
        return false;
    }
    gGL.getTexUnit(0)->disable();
    stop_glerror();

    scratch_target.bindTarget();
    glClearColor(0, 0, 0, 0);
    scratch_target.clear();

    // Render terrain heightmap to paint map via shader

    // Set up viewport, camera, and orthographic projection matrix. Position
    // the camera such that the camera points straight down, and the region
    // completely covers the "screen". Since orthographic projection does not
    // distort, we arbitrarily choose the near plane and far plane to cover the
    // full span of region heights, plus a small amount of padding to account
    // for rounding errors.
    const F32 region_width = region.getWidth();
    const F32 region_half_width = region_width / 2.0f;
    const F32 region_camera_height = surface.getMaxZ() + DEFAULT_NEAR_PLANE;
    LLViewerCamera camera;
    const LLVector3 region_center = LLVector3(region_half_width, region_half_width, 0.0) + region.getOriginAgent();
    const LLVector3 camera_origin = LLVector3(0.0f, 0.0f, region_camera_height) + region_center;
    camera.lookAt(camera_origin, region_center, LLVector3::y_axis);
    camera.setAspect(F32(scratch_target.getHeight()) / F32(scratch_target.getWidth()));
    const LLRect texture_rect(0, scratch_target.getHeight(), scratch_target.getWidth(), 0);
    glViewport(texture_rect.mLeft, texture_rect.mBottom, texture_rect.getWidth(), texture_rect.getHeight());
    // Manually get modelview matrix from camera orientation.
    glm::mat4 modelview(glm::make_mat4((GLfloat *) OGL_TO_CFR_ROTATION));
    GLfloat ogl_matrix[16];
    camera.getOpenGLTransform(ogl_matrix);
    modelview *= glm::make_mat4(ogl_matrix);
    gGL.matrixMode(LLRender::MM_MODELVIEW);
    gGL.loadMatrix(glm::value_ptr(modelview));
    // Override the projection matrix from the camera
    gGL.matrixMode(LLRender::MM_PROJECTION);
    gGL.pushMatrix();
    gGL.loadIdentity();
    llassert(camera_origin.mV[VZ] >= surface.getMaxZ());
    const F32 region_high_near = camera_origin.mV[VZ] - surface.getMaxZ();
    constexpr F32 far_plane_delta = 0.25f;
    const F32 region_low_far = camera_origin.mV[VZ] - surface.getMinZ() + far_plane_delta;
    gGL.ortho(-region_half_width, region_half_width, -region_half_width, region_half_width, region_high_near, region_low_far);
    // No need to call camera.setPerspective because we don't need the clip planes. It would be inaccurate due to the perspective rendering anyway.

    // Need to get the full resolution vertices in order to get an accurate
    // paintmap. It's not sufficient to iterate over the surface patches, as
    // they may be at lower LODs.
    // The functionality here is a subset of
    // LLVOSurfacePatch::getTerrainGeometry. Unlike said function, we don't
    // care about stride length since we're always rendering at full
    // resolution. We also don't care about normals/tangents because those
    // don't contribute to the paintmap.
    // *NOTE: The actual getTerrainGeometry fits the terrain vertices snugly
    // under the 16-bit indices limit. For the sake of simplicity, that has not
    // been replicated here.
    std::vector<LLPointer<LLDrawInfo>> infos;
    // Vertex and index counts adapted from LLVOSurfacePatch::getGeomSizesMain,
    // with additional vertices added as we are including the north and east
    // edges here.
    const U32 patch_size = (U32)surface.getGridsPerPatchEdge();
    constexpr U32 stride = 1;
    const U32 vert_size = (patch_size / stride) + 1;
    const U32 n = vert_size * vert_size;
    const U32 ni = 6 * (vert_size - 1) * (vert_size - 1);
    const U32 region_vertices = n * patch_count * patch_count;
    const U32 region_indices = ni * patch_count * patch_count;
    if (LLGLSLShader::sCurBoundShaderPtr == nullptr)
    { // make sure a shader is bound to satisfy mVertexBuffer->setBuffer
        gDebugProgram.bind();
    }
    LLPointer<LLVertexBuffer> buf = new LLVertexBuffer(LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_TEXCOORD1);
    {
        buf->allocateBuffer(region_vertices, region_indices*2); // hack double index count... TODO: find a better way to indicate 32-bit indices will be used
        buf->setBuffer();
        U32 vertex_total = 0;
        std::vector<U32> index_array(region_indices);
        std::vector<LLVector4a> positions(region_vertices);
        std::vector<LLVector2> texcoords1(region_vertices);
        auto idx = index_array.begin();
        auto pos = positions.begin();
        auto tex1 = texcoords1.begin();
        for (U32 rj = 0; rj < patch_count; ++rj)
        {
            for (U32 ri = 0; ri < patch_count; ++ri)
            {
                const U32 index_offset = vertex_total;
                for (U32 j = 0; j < (vert_size - 1); ++j)
                {
                    for (U32 i = 0; i < (vert_size - 1); ++i)
                    {
                        // y
                        //    2....3
                        // ^  .    .
                        // |  0....1
                        // |
                        // ------->  x
                        //
                        // triangle 1: 0,1,2
                        // triangle 2: 1,3,2
                        // 0: vert0
                        // 1: vert0 + 1
                        // 2: vert0 + vert_size
                        // 3: vert0 + vert_size + 1
                        const U32 vert0 = index_offset + i + (j*vert_size);
                        *idx++ = vert0;
                        *idx++ = vert0 + 1;
                        *idx++ = vert0 + vert_size;
                        *idx++ = vert0 + 1;
                        *idx++ = vert0 + vert_size + 1;
                        *idx++ = vert0 + vert_size;
                    }
                }

                const LLSurfacePatch* patch = surface.getPatch(ri, rj);
                for (U32 j = 0; j < vert_size; ++j)
                {
                    for (U32 i = 0; i < vert_size; ++i)
                    {
                        LLVector3 scratch3;
                        LLVector3 pos3;
                        LLVector2 tex1_temp;
                        patch->eval(i, j, stride, &pos3, &scratch3, &tex1_temp);
                        (*pos++).set(pos3.mV[VX], pos3.mV[VY], pos3.mV[VZ]);
                        *tex1++ = tex1_temp;
                        vertex_total++;
                    }
                }
            }
        }
        buf->setIndexData(index_array.data(), 0, (U32)index_array.size());
        buf->setPositionData(positions.data(), 0, (U32)positions.size());
        buf->setTexCoord1Data(texcoords1.data(), 0, (U32)texcoords1.size());
        buf->unmapBuffer();
        buf->unbind();
    }

    // Draw the region in agent space at full resolution
    {

        LLGLSLShader::unbind();
        // *NOTE: A theoretical non-PBR terrain bake program would be
        // *slightly* different, due the texture terrain shader not having an
        // alpha ramp threshold (TERRAIN_RAMP_MIX_THRESHOLD)
        LLGLSLShader& shader = gPBRTerrainBakeProgram;
        shader.bind();

        LLGLDisable stencil(GL_STENCIL_TEST);
        LLGLDisable scissor(GL_SCISSOR_TEST);
        LLGLEnable cull_face(GL_CULL_FACE);
        LLGLDepthTest depth_test(GL_FALSE, GL_FALSE, GL_ALWAYS);

        S32 alpha_ramp = shader.enableTexture(LLViewerShaderMgr::TERRAIN_ALPHARAMP);
        LLPointer<LLViewerTexture> alpha_ramp_texture = LLViewerTextureManager::getFetchedTexture(IMG_ALPHA_GRAD_2D);
        gGL.getTexUnit(alpha_ramp)->bind(alpha_ramp_texture);
        gGL.getTexUnit(alpha_ramp)->setTextureAddressMode(LLTexUnit::TAM_CLAMP);

        buf->setBuffer();
        for (U32 rj = 0; rj < patch_count; ++rj)
        {
            for (U32 ri = 0; ri < patch_count; ++ri)
            {
                const U32 patch_index = ri + (rj * patch_count);
                const U32 index_offset = ni * patch_index;
                const U32 vertex_offset = n * patch_index;
                llassert(index_offset + ni <= region_indices);
                llassert(vertex_offset + n <= region_vertices);
                buf->drawRange(LLRender::TRIANGLES, vertex_offset, vertex_offset + n - 1, ni, index_offset);
            }
        }

        shader.disableTexture(LLViewerShaderMgr::TERRAIN_ALPHARAMP);

        gGL.getTexUnit(alpha_ramp)->unbind(LLTexUnit::TT_TEXTURE);
        gGL.getTexUnit(alpha_ramp)->disable();
        gGL.getTexUnit(alpha_ramp)->activate();

        shader.unbind();
    }

    gGL.matrixMode(LLRender::MM_PROJECTION);
    gGL.popMatrix();

    gGL.flush();
    LLVertexBuffer::unbind();
    // Final step: Copy the output to the terrain paintmap
    const bool success = tex.getGLTexture()->setSubImageFromFrameBuffer(0, 0, 0, 0, dim, dim);
    if (!success)
    {
        LL_WARNS() << "Failed to copy framebuffer to paintmap" << LL_ENDL;
    }
    glGenerateMipmap(GL_TEXTURE_2D);
    stop_glerror();

    scratch_target.flush();

    LLGLSLShader::unbind();

    return success;
}

// TODO: Decide when to apply the paint queue - ideally once per frame per region
// Applies paints and then clears the paint queue
// *NOTE The paint queue is also cleared when setting the paintmap texture
void LLTerrainPaintMap::applyPaintQueue(LLViewerTexture& tex, LLTerrainPaintQueue& queue)
{
    if (queue.empty()) { return; }

#ifdef SHOW_ASSERT
    check_tex(tex);
#endif

    gGL.getTexUnit(0)->bind(tex.getGLTexture(), false, true);

    const std::vector<LLTerrainPaint::ptr_t>& queue_list = queue.get();
    for (size_t i = 0; i < queue_list.size(); ++i)
    {
        // It is currently the responsibility of the paint queue to convert
        // incoming bits to the right bit depth for the paintmap (this could
        // change in the future).
        queue.convertBitDepths(i, 8);
        const LLTerrainPaint::ptr_t& paint = queue_list[i];

        if (paint->mData.empty()) { continue; }
        constexpr GLint level = 0;
        if ((paint->mStartX >= tex.getWidth() - 1) || (paint->mStartY >= tex.getHeight() - 1)) { continue; }
        constexpr GLint miplevel = 0;
        const S32 x_offset = paint->mStartX;
        const S32 y_offset = paint->mStartY;
        const S32 width = llmin(paint->mWidthX, tex.getWidth() - x_offset);
        const S32 height = llmin(paint->mWidthY, tex.getHeight() - y_offset);
        const U8* pixels = paint->mData.data();
        constexpr GLenum pixformat = GL_RGB;
        constexpr GLenum pixtype = GL_UNSIGNED_BYTE;
        glTexSubImage2D(GL_TEXTURE_2D, miplevel, x_offset, y_offset, width, height, pixformat, pixtype, pixels);
        stop_glerror();
    }

    // Generating mipmaps at the end...
    glGenerateMipmap(GL_TEXTURE_2D);
    stop_glerror();

    gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);

    queue.clear();
}

bool LLTerrainPaintQueue::enqueue(LLTerrainPaint::ptr_t& paint)
{
    llassert(paint);
    if (!paint) { return false; }

    // The paint struct should be pre-validated before this code is reached.
    llassert(!paint->mData.empty());
    // The internal paint map image is currently 8 bits, so that's the maximum
    // allowed bit depth.
    llassert(paint->mBitDepth > 0 && paint->mBitDepth <= 8);
    llassert(paint->mData.size() == (LLTerrainPaint::COMPONENTS * paint->mWidthX * paint->mWidthY));
    llassert(paint->mWidthX > 0);
    llassert(paint->mWidthY > 0);
#ifdef SHOW_ASSERT
    static LLCachedControl<U32> max_texture_width(gSavedSettings, "RenderMaxTextureResolution", 2048);
#endif
    llassert(paint->mWidthX <= max_texture_width);
    llassert(paint->mWidthY <= max_texture_width);
    llassert(paint->mStartX < max_texture_width);
    llassert(paint->mStartY < max_texture_width);

    mList.push_back(paint);
    return true;
}

bool LLTerrainPaintQueue::empty() const
{
    return mList.empty();
}

void LLTerrainPaintQueue::clear()
{
    mList.clear();
}

void LLTerrainPaintQueue::convertBitDepths(size_t index, U8 target_bit_depth)
{
    llassert(target_bit_depth > 0 && target_bit_depth <= 8);
    llassert(index < mList.size());

    LLTerrainPaint::ptr_t& paint = mList[index];
    if (paint->mBitDepth == target_bit_depth) { return; }

    const F32 old_bit_max = F32((1 << paint->mBitDepth) - 1);
    const F32 new_bit_max = F32((1 << target_bit_depth) - 1);
    const F32 bit_conversion_factor = new_bit_max / old_bit_max;

    for (U8& color : paint->mData)
    {
        color = (U8)llround(F32(color) * bit_conversion_factor);
    }

    paint->mBitDepth = target_bit_depth;
}
