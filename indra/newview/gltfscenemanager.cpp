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

    asset->allocateGLResources(filename, model);
    asset->updateTransforms();

    // hang the asset off the currently selected object, or off of the avatar if no object is selected
    LLViewerObject* obj = LLSelectMgr::instance().getSelection()->getFirstRootObject();

    if (obj)
    { // assign to self avatar
        obj->mGLTFAsset = asset;
        mObjects.push_back(obj);
    }
}

GLTFSceneManager::~GLTFSceneManager()
{
    mObjects.clear();
}

LLMatrix4a getModelViewMatrix(LLViewerObject* obj)
{
    LLMatrix4a modelview;
    modelview.loadu(gGLModelView);

    LLMatrix4a root;
    root.loadu((F32*)obj->getRenderMatrix().mMatrix);
    LLVector3 scale = obj->getScale();
    F32 scaleMat[16] = { scale.mV[0], 0.0f, 0.0f, 0.0f,
                            0.0f, scale.mV[1], 0.0f, 0.0f,
                            0.0f, 0.0f, scale.mV[2], 0.0f,
                            0.0f, 0.0f, 0.0f, 1.0f };
    LLMatrix4a scaleMat4;
    scaleMat4.loadu(scaleMat);
    matMul(scaleMat4, root, root);

    matMul(root, modelview, modelview);

    return modelview;
}

void GLTFSceneManager::renderOpaque()
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

        LLMatrix4a modelview = getModelViewMatrix(mObjects[i]);
        asset->updateRenderTransforms(modelview);
        asset->renderOpaque();

        gGL.popMatrix();
    }
}

bool GLTFSceneManager::lineSegmentIntersect(LLVOVolume* obj, Asset* asset, const LLVector4a& start, const LLVector4a& end, S32 face, BOOL pick_transparent, BOOL pick_rigged, BOOL pick_unselectable, S32* face_hitp,
    LLVector4a* intersection, LLVector2* tex_coord, LLVector4a* normal, LLVector4a* tangent)

{
    // line segment intersection test
    // start and end should be in agent space
    // volume space and asset space should be the same coordinate frame
    // results should be transformed back to agent space

    bool ret = false;

    LLVector4a local_start = start;
    LLVector4a local_end = end;

    LLVector3 v_start(start.getF32ptr());
    LLVector3 v_end(end.getF32ptr());

    v_start = obj->agentPositionToVolume(v_start);
    v_end = obj->agentPositionToVolume(v_end);

    local_start.load3(v_start.mV);
    local_end.load3(v_end.mV);

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

    S32 hit_node_index = asset->lineSegmentIntersect(local_start, local_end, &p, &tc, &n, &tn);

    if (hit_node_index >= 0)
    {
        local_end = p;
        if (face_hitp != NULL)
        {
            *face_hitp = -hit_node_index; // hack, return negative index to indicate its a node index and not a face index
        }

        if (intersection != NULL)
        {
            LLVector3 v_p(p.getF32ptr());

            intersection->load3(obj->volumePositionToAgent(v_p).mV);  // must map back to agent space
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
    S32* face_hit,                   // return the face hit
    LLVector4a* intersection,         // return the intersection point
    LLVector2* tex_coord,            // return the texture coordinates of the intersection point
    LLVector4a* normal,               // return the surface normal at the intersection point
    LLVector4a* tangent)			// return the surface tangent at the intersection point
{
    LLDrawable* drawable = nullptr;

    for (U32 i = 0; i < mObjects.size(); ++i)
    {
        if (mObjects[i]->isDead() || mObjects[i]->mGLTFAsset == nullptr || !mObjects[i]->getVolume())
        {
            mObjects.erase(mObjects.begin() + i);
            --i;
            continue;
        }

        // temporary debug -- always double check objects that have GLTF scenes hanging off of them even if the ray doesn't intersect the object bounds
        if (lineSegmentIntersect((LLVOVolume*) mObjects[i].get(), mObjects[i]->mGLTFAsset, start, end, -1, pick_transparent, pick_rigged, pick_unselectable, face_hit, intersection, tex_coord, normal, tangent))
        {
            drawable = mObjects[i]->mDrawable;
        }
    }

    return drawable;
}

void drawBoxOutline(const LLVector4a& pos, const LLVector4a& size);

void renderAssetDebug(Asset* asset)
{
    // render debug
    // assumes appropriate shader is already bound
    // assumes modelview matrix is already set

    gGL.pushMatrix();

    for (auto& node : asset->mNodes)
    {
        if (node.mMesh != INVALID_INDEX)
        {
            gGL.loadMatrix((F32*)node.mRenderMatrix.mMatrix);
            
            // draw bounding box of mesh primitives
            if (gPipeline.hasRenderDebugFeatureMask(LLPipeline::RENDER_DEBUG_BBOXES))
            {
                gGL.color3f(0.f, 1.f, 1.f);

                Mesh& mesh = asset->mMeshes[node.mMesh];
                for (auto& primitive : mesh.mPrimitives)
                {
                    auto* listener = (LLVolumeOctreeListener*) primitive.mOctree->getListener(0);

                    LLVector4a center = listener->mBounds[0];
                    LLVector4a size = listener->mBounds[1];

                    drawBoxOutline(center, size);
                }
            }
        }
    }

    gGL.popMatrix();
}

void GLTFSceneManager::renderDebug()
{
    if (!gPipeline.hasRenderDebugMask(
        LLPipeline::RENDER_DEBUG_BBOXES |
        LLPipeline::RENDER_DEBUG_RAYCAST))
    {
        return;
    }

    gDebugProgram.bind();

    LLGLDisable cullface(GL_CULL_FACE);
    LLGLEnable blend(GL_BLEND);
    gGL.setSceneBlendType(LLRender::BT_ALPHA);
    gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
    gPipeline.disableLights();

    for (auto& obj : mObjects)
    {
        if (obj->isDead() || obj->mGLTFAsset == nullptr)
        {
            continue;
        }

        Asset* asset = obj->mGLTFAsset;

        LLMatrix4a modelview = getModelViewMatrix(obj);

        asset->updateRenderTransforms(modelview);
        renderAssetDebug(asset);
    }

    gDebugProgram.unbind();
}

