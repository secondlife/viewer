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
#include "llrender2dutils.h"
#include "llshadermgr.h"
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
#else
#define check_tex(tex)
#endif
} // namespace

// static
bool LLTerrainPaintMap::bakeHeightNoiseIntoPBRPaintMapRGB(const LLViewerRegion& region, LLViewerTexture& tex)
{
    check_tex(tex);

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
    const S32 max_dim = llmax(tex.getWidth(), tex.getHeight());
    scratch_target.allocate(max_dim, max_dim, GL_RGB, false, LLTexUnit::eTextureType::TT_TEXTURE,
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
    // TODO: Huh... I just realized this view vector is not completely vertical
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
        // TODO: Consider using LLGLSLShader::bindTexture
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
                // TODO: Try a single big drawRange and see if that still works
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
    const bool success = tex.getGLTexture()->setSubImageFromFrameBuffer(0, 0, 0, 0, tex.getWidth(), tex.getHeight());
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

// *TODO: Decide when to apply the paint queue - ideally once per frame per region
// Applies paints and then clears the paint queue
// *NOTE The paint queue is also cleared when setting the paintmap texture
void LLTerrainPaintMap::applyPaintQueueRGB(LLViewerTexture& tex, LLTerrainPaintQueue& queue)
{
    if (queue.empty()) { return; }

    check_tex(tex);

    gGL.getTexUnit(0)->bind(tex.getGLTexture(), false, true);

    // glTexSubImage2D replaces all pixels in the rectangular region. That
    // makes it unsuitable for alpha.
    llassert(queue.getComponents() == LLTerrainPaint::RGB);
    constexpr GLenum pixformat = GL_RGB;

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
        constexpr GLenum pixtype = GL_UNSIGNED_BYTE;
        // *TODO: Performance suggestion: Use the sub-image utility function
        // that LLImageGL::setSubImage uses to split texture updates into
        // lines, if that's faster.
        glTexSubImage2D(GL_TEXTURE_2D, miplevel, x_offset, y_offset, width, height, pixformat, pixtype, pixels);
        stop_glerror();
    }

    // Generating mipmaps at the end...
    glGenerateMipmap(GL_TEXTURE_2D);
    stop_glerror();

    gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);

    queue.clear();
}

namespace
{

// A general-purpose vertex buffer of a quad for stamping textures on the z=0
// plane.
// *NOTE: Because we know the vertex XY coordinates go from 0 to 1
// pre-transform, UVs can be calculated from the vertices
LLVertexBuffer* get_paint_triangle_buffer()
{
    static LLPointer<LLVertexBuffer> buf = new LLVertexBuffer(LLVertexBuffer::MAP_VERTEX);
    static bool initialized = false;
    if (!initialized)
    {
        // Two triangles forming a square from (0,0) to (1,1)
        buf->allocateBuffer(/*vertices =*/ 4, /*indices =*/ 6);
        LLStrider<U16> indices;
        LLStrider<LLVector3> vertices;
        buf->getVertexStrider(vertices);
        buf->getIndexStrider(indices);
        // y
        //    2....3
        // ^  .    .
        // |  0....1
        // |
        // ------->  x
        //
        // triangle 1: 0,1,2
        // triangle 2: 1,3,2
        (*(vertices++)).set(0.0f, 0.0f, 0.0f);
        (*(vertices++)).set(1.0f, 0.0f, 0.0f);
        (*(vertices++)).set(0.0f, 1.0f, 0.0f);
        (*(vertices++)).set(1.0f, 1.0f, 0.0f);
        *(indices++) = 0;
        *(indices++) = 1;
        *(indices++) = 2;
        *(indices++) = 1;
        *(indices++) = 3;
        *(indices++) = 2;
        buf->unmapBuffer();
    }
    return buf.get();
}

};

// static
LLTerrainPaintQueue LLTerrainPaintMap::convertPaintQueueRGBAToRGB(LLViewerTexture& tex, LLTerrainPaintQueue& queue_in)
{
    check_tex(tex);
    llassert(queue_in.getComponents() == LLTerrainPaint::RGBA);

    // TODO: Avoid allocating a scratch render buffer and use mAuxillaryRT instead
    // TODO: even if it means performing extra render operations to apply the paints, in rare cases where the paints can't all fit within an area that can be represented by the buffer
    LLRenderTarget scratch_target;
    const S32 max_dim = llmax(tex.getWidth(), tex.getHeight());
    scratch_target.allocate(max_dim, max_dim, GL_RGB, false, LLTexUnit::eTextureType::TT_TEXTURE,
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
    const F32 target_half_width = (F32)scratch_target.getWidth() / 2.0f;
    const F32 target_half_height = (F32)scratch_target.getHeight() / 2.0f;

    LLVertexBuffer* buf = get_paint_triangle_buffer();

    // Update projection matrix and viewport
    // *NOTE: gl_state_for_2d also sets the modelview matrix. This will be overridden later.
    {
        stop_glerror();
        gGL.matrixMode(LLRender::MM_PROJECTION);
        gGL.pushMatrix();
        gGL.loadIdentity();
        gGL.ortho(-target_half_width, target_half_width, -target_half_height, target_half_height, 0.25f, 1.0f);
        stop_glerror();
        const LLRect texture_rect(0, scratch_target.getHeight(), scratch_target.getWidth(), 0);
        glViewport(texture_rect.mLeft, texture_rect.mBottom, texture_rect.getWidth(), texture_rect.getHeight());
    }

    // View matrix
    // Coordinates should be in pixels. 1.0f = 1 pixel on the framebuffer.
    // Camera is centered in the middle of the framebuffer.
    glm::mat4 view(glm::make_mat4((GLfloat*) OGL_TO_CFR_ROTATION));
    {
        LLViewerCamera camera;
        const LLVector3 camera_origin(target_half_width, target_half_height, 0.5f);
        const LLVector3 camera_look_down(target_half_width, target_half_height, 0.0f);
        camera.lookAt(camera_origin, camera_look_down, LLVector3::y_axis);
        camera.setAspect(F32(scratch_target.getHeight()) / F32(scratch_target.getWidth()));
        GLfloat ogl_matrix[16];
        camera.getOpenGLTransform(ogl_matrix);
        view *= glm::make_mat4(ogl_matrix);
    }

    LLGLDisable stencil(GL_STENCIL_TEST);
    LLGLDisable scissor(GL_SCISSOR_TEST);
    LLGLEnable cull_face(GL_CULL_FACE);
    LLGLDepthTest depth_test(GL_FALSE, GL_FALSE, GL_ALWAYS);
    LLGLEnable blend(GL_BLEND);
    gGL.setSceneBlendType(LLRender::BT_ALPHA);

    LLGLSLShader& shader = gTerrainStampProgram;
    shader.bind();

    // First, apply the paint map as the background
    {
        glm::mat4 model;
        {
            model = glm::scale(model, glm::vec3((F32)tex.getWidth(), (F32)tex.getHeight(), 1.0f));
            model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
        }
        glm::mat4 modelview = view * model;
        gGL.matrixMode(LLRender::MM_MODELVIEW);
        gGL.loadMatrix(glm::value_ptr(modelview));

        shader.bindTexture(LLShaderMgr::DIFFUSE_MAP, &tex);
        // We care about the whole paintmap, which is already a power of two.
        // Hence, TERRAIN_STAMP_SCALE = (1.0,1.0)
        shader.uniform2f(LLShaderMgr::TERRAIN_STAMP_SCALE, 1.0f, 1.0f);
        buf->setBuffer();
        buf->draw(LLRender::TRIANGLES, buf->getIndicesSize(), 0);
    }

    LLTerrainPaintQueue queue_out(LLTerrainPaint::RGB);

    // Incrementally apply each RGBA paint to the render target, then extract
    // the result back into memory as an RGB paint.
    // Put each result in queue_out.
    const std::vector<LLTerrainPaint::ptr_t>& queue_in_list = queue_in.get();
    for (size_t i = 0; i < queue_in_list.size(); ++i)
    {
        // It is currently the responsibility of the paint queue to convert
        // incoming bits to the right bit depth for paint operations (this
        // could change in the future).
        queue_in.convertBitDepths(i, 8);
        const LLTerrainPaint::ptr_t& paint_in = queue_in_list[i];

        // Modelview matrix for the current paint
        // View matrix is already computed. Just need the model matrix.
        // Orthographic projection matrix is already updated
        glm::mat4 model;
        {
            model = glm::scale(model, glm::vec3(paint_in->mWidthX, paint_in->mWidthY, 1.0f));
            model = glm::translate(model, glm::vec3(paint_in->mStartX, paint_in->mStartY, 0.0f));
        }
        glm::mat4 modelview = view * model;
        gGL.matrixMode(LLRender::MM_MODELVIEW);
        gGL.loadMatrix(glm::value_ptr(modelview));

        // Generate temporary stamp texture from paint contents.
        // Our stamp image needs to be a power of two.
        // Because the paint data may not cover a whole power-of-two region,
        // allocate a bigger 2x2 image if needed, but set the image data later
        // for a subset of the image.
        // Pixel data outside this subset is left undefined. We will use
        // TERRAIN_STAMP_SCALE in the stamp shader to define the subset of the
        // image we care about.
        const U32 width_rounded =  1 << U32(ceil(log2(F32(paint_in->mWidthX))));
        const U32 height_rounded = 1 << U32(ceil(log2(F32(paint_in->mWidthY))));
        LLPointer<LLImageGL> stamp_image;
        {
            // Create image object (dimensions not yet initialized in GL)
            U32 stamp_tex_name;
            LLImageGL::generateTextures(1, &stamp_tex_name);
            const U32 components = paint_in->mComponents;
            constexpr LLGLenum target = GL_TEXTURE_2D;
            const LLGLenum internal_format = paint_in->mComponents == 4 ? GL_RGBA8 : GL_RGB8;
            const LLGLenum format = paint_in->mComponents == 4 ? GL_RGBA : GL_RGB;
            constexpr LLGLenum type = GL_UNSIGNED_BYTE;
            stamp_image = new LLImageGL(stamp_tex_name, components, target, internal_format, format, type, LLTexUnit::TAM_WRAP);
            // Nearest-neighbor filtering to reduce surprises
            stamp_image->setFilteringOption(LLTexUnit::TFO_POINT);

            // Initialize the image dimensions in GL
            constexpr U8* undefined_data_for_now = nullptr;
            gGL.getTexUnit(0)->bind(stamp_image, false, true);
            glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width_rounded, height_rounded, 0, format, type, undefined_data_for_now);
            gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
            stamp_image->setSize(width_rounded, height_rounded, components);
            stamp_image->setDiscardLevel(0);

            // Manually set a subset of the image in GL
            const U8* data = paint_in->mData.data();
            const S32 data_width = paint_in->mWidthX;
            const S32 data_height = paint_in->mWidthY;
            constexpr S32 origin = 0;
            // width = data_width; height = data_height. i.e.: Copy the full
            // contents of data into the image.
            stamp_image->setSubImage(data, data_width, data_height, origin, origin, /*width=*/data_width, /*height=*/data_height);
        }

        // Apply ("stamp") the paint to the render target
        {
            shader.bindTextureImageGL(LLShaderMgr::DIFFUSE_MAP, stamp_image);
            const F32 width_fraction = F32(paint_in->mWidthX) / F32(width_rounded);
            const F32 height_fraction = F32(paint_in->mWidthY) / F32(height_rounded);
            shader.uniform2f(LLShaderMgr::TERRAIN_STAMP_SCALE, width_fraction, height_fraction);
            buf->setBuffer();
            buf->draw(LLRender::TRIANGLES, buf->getIndicesSize(), 0);
        }

        // Extract the result back into memory as an RGB paint
        LLTerrainPaint::ptr_t paint_out = std::make_shared<LLTerrainPaint>();
        {
            paint_out->mStartX = paint_in->mStartX;
            paint_out->mStartY = paint_in->mStartY;
            paint_out->mWidthX = paint_in->mWidthX;
            paint_out->mWidthY = paint_in->mWidthY;
            paint_out->mBitDepth = 8; // Will be reduced to TerrainPaintBitDepth bits later
            paint_out->mComponents = LLTerrainPaint::RGB;
#ifdef SHOW_ASSERT
            paint_out->assert_confined_to(tex);
#endif
            paint_out->confine_to(tex);
            paint_out->mData.resize(paint_out->mComponents * paint_out->mWidthX * paint_out->mWidthY);
            constexpr GLint miplevel = 0;
            const S32 x_offset = paint_out->mStartX;
            const S32 y_offset = paint_out->mStartY;
            const S32 width = paint_out->mWidthX;
            const S32 height = paint_out->mWidthY;
            constexpr GLenum pixformat = GL_RGB;
            constexpr GLenum pixtype = GL_UNSIGNED_BYTE;
            llassert(paint_out->mData.size() <= std::numeric_limits<GLsizei>::max());
            const GLsizei buf_size = (GLsizei)paint_out->mData.size();
            U8* pixels = paint_out->mData.data();
            glReadPixels(x_offset, y_offset, width, height, pixformat, pixtype, pixels);
        }

        // Enqueue the result to the new paint queue, with bit depths per color
        // channel reduced from 8 to 5, and reduced from RGBA (paintmap
        // sub-rectangle update with alpha mask) to RGB (paintmap sub-rectangle
        // update without alpha mask). This format is suitable for sending
        // over the network.
        // *TODO: At some point, queue_out will pass through a network
        // round-trip which will reduce the bit depth, making the
        // pre-conversion step not necessary.
        queue_out.enqueue(paint_out);
        LLCachedControl<U32> bit_depth(gSavedSettings, "TerrainPaintBitDepth");
        queue_out.convertBitDepths(queue_out.size()-1, bit_depth);
    }

    queue_in.clear();

    scratch_target.flush();

    LLGLSLShader::unbind();

    gGL.matrixMode(LLRender::MM_PROJECTION);
    gGL.popMatrix();

    return queue_out;
}

// static
LLTerrainPaintQueue LLTerrainPaintMap::convertBrushQueueToPaintRGB(const LLViewerRegion& region, LLViewerTexture& tex, LLTerrainBrushQueue& queue_in)
{
    check_tex(tex);

    // TODO: Avoid allocating a scratch render buffer and use mAuxillaryRT instead
    // TODO: even if it means performing extra render operations to apply the brushes, in rare cases where the paints can't all fit within an area that can be represented by the buffer
    LLRenderTarget scratch_target;
    const S32 max_dim = llmax(tex.getWidth(), tex.getHeight());
    scratch_target.allocate(max_dim, max_dim, GL_RGB, false, LLTexUnit::eTextureType::TT_TEXTURE,
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
    const F32 target_half_width = (F32)scratch_target.getWidth() / 2.0f;
    const F32 target_half_height = (F32)scratch_target.getHeight() / 2.0f;

    LLVertexBuffer* buf = get_paint_triangle_buffer();

    // Update projection matrix and viewport
    // *NOTE: gl_state_for_2d also sets the modelview matrix. This will be overridden later.
    {
        stop_glerror();
        gGL.matrixMode(LLRender::MM_PROJECTION);
        gGL.pushMatrix();
        gGL.loadIdentity();
        gGL.ortho(-target_half_width, target_half_width, -target_half_height, target_half_height, 0.25f, 1.0f);
        stop_glerror();
        const LLRect texture_rect(0, scratch_target.getHeight(), scratch_target.getWidth(), 0);
        glViewport(texture_rect.mLeft, texture_rect.mBottom, texture_rect.getWidth(), texture_rect.getHeight());
    }

    // View matrix
    // Coordinates should be in pixels. 1.0f = 1 pixel on the framebuffer.
    // Camera is centered in the middle of the framebuffer.
    glm::mat4 view(glm::make_mat4((GLfloat*)OGL_TO_CFR_ROTATION));
    {
        LLViewerCamera camera;
        const LLVector3 camera_origin(target_half_width, target_half_height, 0.5f);
        const LLVector3 camera_look_down(target_half_width, target_half_height, 0.0f);
        camera.lookAt(camera_origin, camera_look_down, LLVector3::y_axis);
        camera.setAspect(F32(scratch_target.getHeight()) / F32(scratch_target.getWidth()));
        GLfloat ogl_matrix[16];
        camera.getOpenGLTransform(ogl_matrix);
        view *= glm::make_mat4(ogl_matrix);
    }

    LLGLDisable stencil(GL_STENCIL_TEST);
    LLGLDisable scissor(GL_SCISSOR_TEST);
    LLGLEnable cull_face(GL_CULL_FACE);
    LLGLDepthTest depth_test(GL_FALSE, GL_FALSE, GL_ALWAYS);
    LLGLEnable blend(GL_BLEND);
    gGL.setSceneBlendType(LLRender::BT_ALPHA);

    LLGLSLShader& shader = gTerrainStampProgram;
    shader.bind();

    // First, apply the paint map as the background
    {
        glm::mat4 model;
        {
            model = glm::scale(model, glm::vec3((F32)tex.getWidth(), (F32)tex.getHeight(), 1.0f));
            model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
        }
        glm::mat4 modelview = view * model;
        gGL.matrixMode(LLRender::MM_MODELVIEW);
        gGL.loadMatrix(glm::value_ptr(modelview));

        shader.bindTexture(LLShaderMgr::DIFFUSE_MAP, &tex);
        // We care about the whole paintmap, which is already a power of two.
        // Hence, TERRAIN_STAMP_SCALE = (1.0,1.0)
        shader.uniform2f(LLShaderMgr::TERRAIN_STAMP_SCALE, 1.0f, 1.0f);
        buf->setBuffer();
        buf->draw(LLRender::TRIANGLES, buf->getIndicesSize(), 0);
    }

    LLTerrainPaintQueue queue_out(LLTerrainPaint::RGB);

    // Incrementally apply each brush stroke to the render target, then extract
    // the result back into memory as an RGB paint.
    // Put each result in queue_out.
    const std::vector<LLTerrainBrush::ptr_t>& brush_list = queue_in.get();
    for (size_t i = 0; i < brush_list.size(); ++i)
    {
        const LLTerrainBrush::ptr_t& brush_in = brush_list[i];

        // Modelview matrix for the current brush
        // View matrix is already computed. Just need the model matrix.
        // Orthographic projection matrix is already updated
        // *NOTE: Brush path information is in region space. It will need to be
        // converted to paintmap pixel space before it makes sense.
        F32 brush_width_x;
        F32 brush_width_y;
        F32 brush_start_x;
        F32 brush_start_y;
        {
            F32 min_x = brush_in->mPath[0].mV[VX];
            F32 max_x = min_x;
            F32 min_y = brush_in->mPath[0].mV[VY];
            F32 max_y = min_y;
            for (size_t i = 1; i < brush_in->mPath.size(); ++i)
            {
                const F32 x = brush_in->mPath[i].mV[VX];
                const F32 y = brush_in->mPath[i].mV[VY];
                min_x = llmin(min_x, x);
                max_x = llmax(max_x, x);
                min_y = llmin(min_y, y);
                max_y = llmax(max_y, y);
            }
            brush_width_x = brush_in->mBrushSize + (max_x - min_x);
            brush_width_y = brush_in->mBrushSize + (max_y - min_y);
            brush_start_x = min_x - (brush_in->mBrushSize / 2.0f);
            brush_start_y = min_y - (brush_in->mBrushSize / 2.0f);
            // Convert brush path information to paintmap pixel space from region
            // space.
            brush_width_x *= tex.getWidth()  / region.getWidth();
            brush_width_y *= tex.getHeight() / region.getWidth();
            brush_start_x *= tex.getWidth()  / region.getWidth();
            brush_start_y *= tex.getHeight() / region.getWidth();
        }
        glm::mat4 model;
        {
            model = glm::scale(model, glm::vec3(brush_width_x, brush_width_y, 1.0f));
            model = glm::translate(model, glm::vec3(brush_start_x, brush_start_y, 0.0f));
        }
        glm::mat4 modelview = view * model;
        gGL.matrixMode(LLRender::MM_MODELVIEW);
        gGL.loadMatrix(glm::value_ptr(modelview));

        // Apply the "brush" to the render target
        {
            // TODO: Use different shader for this - currently this is using the stamp shader. The white image is just a placeholder for now
            shader.bindTexture(LLShaderMgr::DIFFUSE_MAP, LLViewerFetchedTexture::sWhiteImagep);
            shader.uniform2f(LLShaderMgr::TERRAIN_STAMP_SCALE, 1.0f, 1.0f);
            buf->setBuffer();
            buf->draw(LLRender::TRIANGLES, buf->getIndicesSize(), 0);
        }

        // Extract the result back into memory as an RGB paint
        LLTerrainPaint::ptr_t paint_out = std::make_shared<LLTerrainPaint>();
        {
            paint_out->mStartX = U16(floor(brush_start_x));
            paint_out->mStartY = U16(floor(brush_start_y));
            const F32 dX = brush_start_x - F32(paint_out->mStartX);
            const F32 dY = brush_start_y - F32(paint_out->mStartY);
            paint_out->mWidthX = U16(ceil(brush_width_x + dX));
            paint_out->mWidthY = U16(ceil(brush_width_y + dY));
            paint_out->mBitDepth = 8; // Will be reduced to TerrainPaintBitDepth bits later
            paint_out->mComponents = LLTerrainPaint::RGB;
            // The brush strokes are expected to sometimes partially venture
            // outside of the paintmap bounds.
            paint_out->confine_to(tex);
            paint_out->mData.resize(paint_out->mComponents * paint_out->mWidthX * paint_out->mWidthY);
            constexpr GLint miplevel = 0;
            const S32 x_offset = paint_out->mStartX;
            const S32 y_offset = paint_out->mStartY;
            const S32 width = paint_out->mWidthX;
            const S32 height = paint_out->mWidthY;
            constexpr GLenum pixformat = GL_RGB;
            constexpr GLenum pixtype = GL_UNSIGNED_BYTE;
            llassert(paint_out->mData.size() <= std::numeric_limits<GLsizei>::max());
            const GLsizei buf_size = (GLsizei)paint_out->mData.size();
            U8* pixels = paint_out->mData.data();
            glReadPixels(x_offset, y_offset, width, height, pixformat, pixtype, pixels);
        }

        // Enqueue the result to the new paint queue, with bit depths per color
        // channel reduced from 8 to 5, and reduced from RGBA (paintmap
        // sub-rectangle update with alpha mask) to RGB (paintmap sub-rectangle
        // update without alpha mask). This format is suitable for sending
        // over the network.
        // *TODO: At some point, queue_out will pass through a network
        // round-trip which will reduce the bit depth, making the
        // pre-conversion step not necessary.
        queue_out.enqueue(paint_out);
        LLCachedControl<U32> bit_depth(gSavedSettings, "TerrainPaintBitDepth");
        queue_out.convertBitDepths(queue_out.size()-1, bit_depth);
    }

    queue_in.clear();

    scratch_target.flush();

    LLGLSLShader::unbind();

    gGL.matrixMode(LLRender::MM_PROJECTION);
    gGL.popMatrix();

    return queue_out;
}

template<typename T>
LLTerrainQueue<T>::LLTerrainQueue(LLTerrainQueue<T>& other)
{
    *this = other;
}

template<typename T>
LLTerrainQueue<T>& LLTerrainQueue<T>::operator=(LLTerrainQueue<T>& other)
{
    mList = other.mList;
    return *this;
}

template<typename T>
bool LLTerrainQueue<T>::enqueue(std::shared_ptr<T>& t, bool dry_run)
{
    if (!dry_run) { mList.push_back(t); }
    return true;
}

template<typename T>
bool LLTerrainQueue<T>::enqueue(std::vector<std::shared_ptr<T>>& list)
{
    constexpr bool dry_run = true;
    for (auto& t : list)
    {
        if (!enqueue(t, dry_run)) { return false; }
    }
    for (auto& t : list)
    {
        enqueue(t);
    }
    return true;
}

template<typename T>
size_t LLTerrainQueue<T>::size() const
{
    return mList.size();
}

template<typename T>
bool LLTerrainQueue<T>::empty() const
{
    return mList.empty();
}

template<typename T>
void LLTerrainQueue<T>::clear()
{
    mList.clear();
}

void LLTerrainPaint::assert_confined_to(const LLTexture& tex) const
{
    llassert(mStartX >= 0 && mStartX < tex.getWidth());
    llassert(mStartY >= 0 && mStartY < tex.getHeight());
    llassert(mWidthX <= tex.getWidth() - mStartX);
    llassert(mWidthY <= tex.getHeight() - mStartY);
}

void LLTerrainPaint::confine_to(const LLTexture& tex)
{
    mStartX = llmax(mStartX, 0);
    mStartY = llmax(mStartY, 0);
    mWidthX = llmin(mWidthX, tex.getWidth() - mStartX);
    mWidthY = llmin(mWidthY, tex.getHeight() - mStartY);
    assert_confined_to(tex);
}

LLTerrainPaintQueue::LLTerrainPaintQueue(U8 components)
: mComponents(components)
{
    llassert(mComponents == LLTerrainPaint::RGB || mComponents == LLTerrainPaint::RGBA);
}

LLTerrainPaintQueue::LLTerrainPaintQueue(LLTerrainPaintQueue& other)
: LLTerrainQueue<LLTerrainPaint>(other)
, mComponents(other.mComponents)
{
    llassert(mComponents == LLTerrainPaint::RGB || mComponents == LLTerrainPaint::RGBA);
}

LLTerrainPaintQueue& LLTerrainPaintQueue::operator=(LLTerrainPaintQueue& other)
{
    LLTerrainQueue<LLTerrainPaint>::operator=(other);
    mComponents = other.mComponents;
    return *this;
}

bool LLTerrainPaintQueue::enqueue(LLTerrainPaint::ptr_t& paint, bool dry_run)
{
    llassert(paint);
    if (!paint) { return false; }

    // The paint struct should be pre-validated before this code is reached.
    llassert(!paint->mData.empty());
    // The internal paint map image is currently 8 bits, so that's the maximum
    // allowed bit depth.
    llassert(paint->mBitDepth > 0 && paint->mBitDepth <= 8);
    llassert(paint->mData.size() == (mComponents * paint->mWidthX * paint->mWidthY));
    llassert(paint->mWidthX > 0);
    llassert(paint->mWidthY > 0);
#ifdef SHOW_ASSERT
    static LLCachedControl<U32> max_texture_width(gSavedSettings, "RenderMaxTextureResolution", 2048);
#endif
    llassert(paint->mWidthX <= max_texture_width);
    llassert(paint->mWidthY <= max_texture_width);
    llassert(paint->mStartX < max_texture_width);
    llassert(paint->mStartY < max_texture_width);

    return LLTerrainQueue<LLTerrainPaint>::enqueue(paint, dry_run);
}

bool LLTerrainPaintQueue::enqueue(LLTerrainPaintQueue& queue)
{
    return LLTerrainQueue<LLTerrainPaint>::enqueue(queue.mList);
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

LLTerrainBrushQueue::LLTerrainBrushQueue()
: LLTerrainQueue<LLTerrainBrush>()
{
}

LLTerrainBrushQueue::LLTerrainBrushQueue(LLTerrainBrushQueue& other)
: LLTerrainQueue<LLTerrainBrush>(other)
{
}

LLTerrainBrushQueue& LLTerrainBrushQueue::operator=(LLTerrainBrushQueue& other)
{
    LLTerrainQueue<LLTerrainBrush>::operator=(other);
    return *this;
}

bool LLTerrainBrushQueue::enqueue(LLTerrainBrush::ptr_t& brush, bool dry_run)
{
    llassert(brush->mBrushSize > 0);
    llassert(!brush->mPath.empty());
    llassert(brush->mPathOffset < brush->mPath.size());
    llassert(brush->mPathOffset < 2); // Harmless, but doesn't do anything useful, so might be a sign of implementation error
    return LLTerrainQueue<LLTerrainBrush>::enqueue(brush, dry_run);
}

bool LLTerrainBrushQueue::enqueue(LLTerrainBrushQueue& queue)
{
    return LLTerrainQueue<LLTerrainBrush>::enqueue(queue.mList);
}

template class LLTerrainQueue<LLTerrainPaint>;
template class LLTerrainQueue<LLTerrainBrush>;
