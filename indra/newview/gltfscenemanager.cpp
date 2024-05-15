/**
 * @file gltfscenemanager.cpp
 * @brief Builds menus out of items.
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

#include "gltfscenemanager.h"
#include "llviewermenufile.h"
#include "llappviewer.h"
#include "lltinygltfhelper.h"
#include "llvertexbuffer.h"
#include "llselectmgr.h"
#include "llagent.h"
#include "llnotificationsutil.h"
#include "llvoavatarself.h"
#include "llvolumeoctree.h"
#include "gltf/asset.h"
#include "pipeline.h"
#include "llviewershadermgr.h"


using namespace LL;

// temporary location of LL GLTF Implementation
using namespace LL::GLTF;

void GLTFSceneManager::load()
{
    LLViewerObject* obj = LLSelectMgr::instance().getSelection()->getFirstRootObject();

    if (obj)
    {
        // Load a scene from disk
        LLFilePickerReplyThread::startPicker(
            [](const std::vector<std::string>& filenames, LLFilePicker::ELoadFilter load_filter, LLFilePicker::ESaveFilter save_filter)
            {
                if (LLAppViewer::instance()->quitRequested())
                {
                    return;
                }
                if (filenames.size() > 0)
                {
                    GLTFSceneManager::instance().load(filenames[0]);
                }
            },
            LLFilePicker::FFLOAD_GLTF,
            true);
    }
    else
    {
        LLNotificationsUtil::add("GLTFPreviewSelection");
    }
}

void GLTFSceneManager::load(const std::string& filename)
{
    tinygltf::Model model;
    LLTinyGLTFHelper::loadModel(filename, model);

    LLPointer<Asset> asset = new Asset();
    *asset = model;

    gDebugProgram.bind(); // bind a shader to satisfy LLVertexBuffer assertions
    asset->allocateGLResources(filename, model);
    asset->updateTransforms();

    // hang the asset off the currently selected object, or off of the avatar if no object is selected
    LLViewerObject* obj = LLSelectMgr::instance().getSelection()->getFirstRootObject();

    if (obj)
    { // assign to self avatar
        obj->mGLTFAsset = asset;

        if (std::find(mObjects.begin(), mObjects.end(), obj) == mObjects.end())
        {
            mObjects.push_back(obj);
        }
    }
}

GLTFSceneManager::~GLTFSceneManager()
{
    mObjects.clear();
}

void GLTFSceneManager::renderOpaque()
{
    render(true);
}

void GLTFSceneManager::renderAlpha()
{
    render(false);
}

void GLTFSceneManager::update()
{
    for (U32 i = 0; i < mObjects.size(); ++i)
    {
        if (mObjects[i]->isDead() || mObjects[i]->mGLTFAsset == nullptr)
        {
            mObjects.erase(mObjects.begin() + i);
            --i;
            continue;
        }

        Asset* asset = mObjects[i]->mGLTFAsset;

        asset->update();

    }
}

void GLTFSceneManager::render(bool opaque, bool rigged)
{
    // for debugging, just render the whole scene as opaque
    // by traversing the whole scenegraph
    // Assumes camera transform is already set and
    // appropriate shader is already bound

    gGL.matrixMode(LLRender::MM_MODELVIEW);

    for (U32 i = 0; i < mObjects.size(); ++i)
    {
        if (mObjects[i]->isDead() || mObjects[i]->mGLTFAsset == nullptr)
        {
            mObjects.erase(mObjects.begin() + i);
            --i;
            continue;
        }

        Asset* asset = mObjects[i]->mGLTFAsset;

        gGL.pushMatrix();

        LLMatrix4a mat = mObjects[i]->getGLTFAssetToAgentTransform();

        LLMatrix4a modelview;
        modelview.loadu(gGLModelView);

        matMul(mat, modelview, modelview);

        asset->updateRenderTransforms(modelview);
        asset->render(opaque, rigged);

        gGL.popMatrix();
    }
}

LLMatrix4a inverse(const LLMatrix4a& mat)
{
    glh::matrix4f m((F32*)mat.mMatrix);
    m = m.inverse();
    LLMatrix4a ret;
    ret.loadu(m.m);
    return ret;
}

bool GLTFSceneManager::lineSegmentIntersect(LLVOVolume* obj, Asset* asset, const LLVector4a& start, const LLVector4a& end, S32 face, BOOL pick_transparent, BOOL pick_rigged, BOOL pick_unselectable, S32* node_hit, S32* primitive_hit,
    LLVector4a* intersection, LLVector2* tex_coord, LLVector4a* normal, LLVector4a* tangent)

{
    // line segment intersection test
    // start and end should be in agent space
    // volume space and asset space should be the same coordinate frame
    // results should be transformed back to agent space

    bool ret = false;

    LLVector4a local_start;
    LLVector4a local_end;

    LLMatrix4a asset_to_agent = obj->getGLTFAssetToAgentTransform();
    LLMatrix4a agent_to_asset = inverse(asset_to_agent);

    agent_to_asset.affineTransform(start, local_start);
    agent_to_asset.affineTransform(end, local_end);

    LLVector4a p;
    LLVector4a n;
    LLVector2 tc;
    LLVector4a tn;

    if (intersection != NULL)
    {
        p = *intersection;
    }

    if (tex_coord != NULL)
    {
        tc = *tex_coord;
    }

    if (normal != NULL)
    {
        n = *normal;
    }

    if (tangent != NULL)
    {
        tn = *tangent;
    }

    S32 hit_node_index = asset->lineSegmentIntersect(local_start, local_end, &p, &tc, &n, &tn, primitive_hit);

    if (hit_node_index >= 0)
    {
        local_end = p;
        if (node_hit != NULL)
        {
            *node_hit = hit_node_index;
        }

        if (intersection != NULL)
        {
            asset_to_agent.affineTransform(p, *intersection);
        }

        if (normal != NULL)
        {
            LLVector3 v_n(n.getF32ptr());
            normal->load3(obj->volumeDirectionToAgent(v_n).mV);
            (*normal).normalize3fast();
        }

        if (tangent != NULL)
        {
            LLVector3 v_tn(tn.getF32ptr());

            LLVector4a trans_tangent;
            trans_tangent.load3(obj->volumeDirectionToAgent(v_tn).mV);

            LLVector4Logical mask;
            mask.clear();
            mask.setElement<3>();

            tangent->setSelectWithMask(mask, tn, trans_tangent);
            (*tangent).normalize3fast();
        }

        if (tex_coord != NULL)
        {
            *tex_coord = tc;
        }

        ret = true;
    }

    return ret;
}

LLDrawable* GLTFSceneManager::lineSegmentIntersect(const LLVector4a& start, const LLVector4a& end,
    BOOL pick_transparent,
    BOOL pick_rigged,
    BOOL pick_unselectable,
    BOOL pick_reflection_probe,
    S32* node_hit,                   // return the index of the node that was hit
    S32* primitive_hit,               // return the index of the primitive that was hit
    LLVector4a* intersection,         // return the intersection point
    LLVector2* tex_coord,            // return the texture coordinates of the intersection point
    LLVector4a* normal,               // return the surface normal at the intersection point
    LLVector4a* tangent)            // return the surface tangent at the intersection point
{
    LLDrawable* drawable = nullptr;

    LLVector4a local_end = end;
    LLVector4a position;

    for (U32 i = 0; i < mObjects.size(); ++i)
    {
        if (mObjects[i]->isDead() || mObjects[i]->mGLTFAsset == nullptr || !mObjects[i]->getVolume())
        {
            mObjects.erase(mObjects.begin() + i);
            --i;
            continue;
        }

        // temporary debug -- always double check objects that have GLTF scenes hanging off of them even if the ray doesn't intersect the object bounds
        if (lineSegmentIntersect((LLVOVolume*) mObjects[i].get(), mObjects[i]->mGLTFAsset, start, local_end, -1, pick_transparent, pick_rigged, pick_unselectable, node_hit, primitive_hit, &position, tex_coord, normal, tangent))
        {
            local_end = position;
            if (intersection)
            {
                *intersection = position;
            }
            drawable = mObjects[i]->mDrawable;
        }
    }

    return drawable;
}

void drawBoxOutline(const LLVector4a& pos, const LLVector4a& size);

extern LLVector4a       gDebugRaycastStart;
extern LLVector4a       gDebugRaycastEnd;

void renderOctreeRaycast(const LLVector4a& start, const LLVector4a& end, const LLVolumeOctree* octree);

void renderAssetDebug(LLViewerObject* obj, Asset* asset)
{
    // render debug
    // assumes appropriate shader is already bound
    // assumes modelview matrix is already set

    gGL.pushMatrix();

    // get raycast in asset space
    LLMatrix4a agent_to_asset = obj->getAgentToGLTFAssetTransform();

    LLVector4a start;
    LLVector4a end;

    agent_to_asset.affineTransform(gDebugRaycastStart, start);
    agent_to_asset.affineTransform(gDebugRaycastEnd, end);


    for (auto& node : asset->mNodes)
    {
        Mesh& mesh = asset->mMeshes[node.mMesh];

        if (node.mMesh != INVALID_INDEX)
        {
            gGL.loadMatrix((F32*)node.mRenderMatrix.mMatrix);

            // draw bounding box of mesh primitives
            if (gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_BBOXES))
            {
                gGL.color3f(0.f, 1.f, 1.f);

                for (auto& primitive : mesh.mPrimitives)
                {
                    auto* listener = (LLVolumeOctreeListener*) primitive.mOctree->getListener(0);

                    LLVector4a center = listener->mBounds[0];
                    LLVector4a size = listener->mBounds[1];

                    drawBoxOutline(center, size);
                }
            }

#if 0
            if (gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_RAYCAST))
            {
                gGL.flush();
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

                // convert raycast to node local space
                LLVector4a local_start;
                LLVector4a local_end;

                node.mAssetMatrixInv.affineTransform(start, local_start);
                node.mAssetMatrixInv.affineTransform(end, local_end);

                for (auto& primitive : mesh.mPrimitives)
                {
                    if (primitive.mOctree.notNull())
                    {
                        renderOctreeRaycast(local_start, local_end, primitive.mOctree);
                    }
                }

                gGL.flush();
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            }
#endif
        }
    }

    gGL.popMatrix();
}

void GLTFSceneManager::renderDebug()
{
    if (!gPipeline.hasRenderDebugMask(
        LLPipeline::RENDER_DEBUG_BBOXES |
        LLPipeline::RENDER_DEBUG_RAYCAST |
        LLPipeline::RENDER_DEBUG_NODES))
    {
        return;
    }

    gDebugProgram.bind();

    LLGLDisable cullface(GL_CULL_FACE);
    LLGLEnable blend(GL_BLEND);
    gGL.setSceneBlendType(LLRender::BT_ALPHA);
    gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
    gPipeline.disableLights();

    // force update all mRenderMatrix, not just nodes with meshes
    for (auto& obj : mObjects)
    {
        if (obj->isDead() || obj->mGLTFAsset == nullptr)
        {
            continue;
        }

        LLMatrix4a mat = obj->getGLTFAssetToAgentTransform();

        LLMatrix4a modelview;
        modelview.loadu(gGLModelView);

        matMul(mat, modelview, modelview);

        Asset* asset = obj->mGLTFAsset;

        for (auto& node : asset->mNodes)
        {
            matMul(node.mAssetMatrix, modelview, node.mRenderMatrix);
        }
    }

    for (auto& obj : mObjects)
    {
        if (obj->isDead() || obj->mGLTFAsset == nullptr)
        {
            continue;
        }

        Asset* asset = obj->mGLTFAsset;

        LLMatrix4a mat = obj->getGLTFAssetToAgentTransform();

        LLMatrix4a modelview;
        modelview.loadu(gGLModelView);

        matMul(mat, modelview, modelview);

        renderAssetDebug(obj, asset);
    }

    if (gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_NODES))
    { //render node hierarchy

        for (U32 i = 0; i < 2; ++i)
        {
            LLGLDepthTest depth(GL_TRUE, i == 0 ? GL_FALSE : GL_TRUE, i == 0 ? GL_GREATER : GL_LEQUAL);
            LLGLState blend(GL_BLEND, i == 0 ? TRUE : FALSE);


            gGL.pushMatrix();

            for (auto& obj : mObjects)
            {
                if (obj->isDead() || obj->mGLTFAsset == nullptr)
                {
                    continue;
                }

                LLMatrix4a mat = obj->getGLTFAssetToAgentTransform();

                LLMatrix4a modelview;
                modelview.loadu(gGLModelView);

                matMul(mat, modelview, modelview);

                Asset* asset = obj->mGLTFAsset;

                for (auto& node : asset->mNodes)
                {
                    // force update all mRenderMatrix, not just nodes with meshes
                    matMul(node.mAssetMatrix, modelview, node.mRenderMatrix);

                    gGL.loadMatrix(node.mRenderMatrix.getF32ptr());
                    // render x-axis red, y-axis green, z-axis blue
                    gGL.color4f(1.f, 0.f, 0.f, 0.5f);
                    gGL.begin(LLRender::LINES);
                    gGL.vertex3f(0.f, 0.f, 0.f);
                    gGL.vertex3f(1.f, 0.f, 0.f);
                    gGL.end();
                    gGL.flush();

                    gGL.color4f(0.f, 1.f, 0.f, 0.5f);
                    gGL.begin(LLRender::LINES);
                    gGL.vertex3f(0.f, 0.f, 0.f);
                    gGL.vertex3f(0.f, 1.f, 0.f);
                    gGL.end();
                    gGL.flush();

                    gGL.begin(LLRender::LINES);
                    gGL.color4f(0.f, 0.f, 1.f, 0.5f);
                    gGL.vertex3f(0.f, 0.f, 0.f);
                    gGL.vertex3f(0.f, 0.f, 1.f);
                    gGL.end();
                    gGL.flush();

                    // render path to child nodes cyan
                    gGL.color4f(0.f, 1.f, 1.f, 0.5f);
                    gGL.begin(LLRender::LINES);
                    for (auto& child_idx : node.mChildren)
                    {
                        Node& child = asset->mNodes[child_idx];
                        gGL.vertex3f(0.f, 0.f, 0.f);
                        gGL.vertex3fv(child.mMatrix.getTranslation().getF32ptr());
                    }
                    gGL.end();
                    gGL.flush();
                }
            }

            gGL.popMatrix();
        }

    }


    if (gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_RAYCAST))
    {
        S32 node_hit = -1;
        S32 primitive_hit = -1;
        LLVector4a intersection;

        LLDrawable* drawable = lineSegmentIntersect(gDebugRaycastStart, gDebugRaycastEnd, TRUE, TRUE, TRUE, TRUE, &node_hit, &primitive_hit, &intersection, nullptr, nullptr, nullptr);

        if (drawable)
        {
            gGL.pushMatrix();
            Asset* asset = drawable->getVObj()->mGLTFAsset;
            Node* node = &asset->mNodes[node_hit];
            Primitive* primitive = &asset->mMeshes[node->mMesh].mPrimitives[primitive_hit];

            gGL.flush();
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            gGL.color3f(1, 0, 1);
            drawBoxOutline(intersection, LLVector4a(0.1f, 0.1f, 0.1f, 0.f));

            gGL.loadMatrix((F32*) node->mRenderMatrix.mMatrix);



            auto* listener = (LLVolumeOctreeListener*) primitive->mOctree->getListener(0);
            drawBoxOutline(listener->mBounds[0], listener->mBounds[1]);

            gGL.flush();
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            gGL.popMatrix();
        }
    }
    gDebugProgram.unbind();
}

