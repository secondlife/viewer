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
#include "llvoavatarself.h"
#include "llvolumeoctree.h"
#include "gltf/asset.h"

using namespace LL;

// temporary location of LL GLTF Implementation
using namespace LL::GLTF;

Asset* gGLTFAsset = nullptr;
LLPointer<LLViewerObject> gGLTFObject;

void GLTFSceneManager::load()
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

void GLTFSceneManager::load(const std::string& filename)
{
    tinygltf::Model model;
    LLTinyGLTFHelper::loadModel(filename, model);

    delete gGLTFAsset;
    gGLTFAsset = new Asset();
    *gGLTFAsset = model;

    gGLTFAsset->allocateGLResources(filename, model);

    // hang the asset off the currently selected object, or off of the avatar if no object is selected
    gGLTFObject = LLSelectMgr::instance().getSelection()->getFirstRootObject();

    if (gGLTFObject.isNull())
    { // assign to self avatar
        gGLTFObject = gAgentAvatarp;
    }
}


void Node::updateRenderTransforms(Asset& asset, const LLMatrix4a& modelview)
{
    matMul(mMatrix, modelview, mRenderMatrix);

    for (auto& childIndex : mChildren)
    {
        Node& child = asset.mNodes[childIndex];
        child.updateRenderTransforms(asset, mRenderMatrix);
    }
}


GLTFSceneManager::~GLTFSceneManager()
{
    delete gGLTFAsset;
    gGLTFObject = nullptr;
}

void GLTFSceneManager::renderOpaque()
{
    // for debugging, just render the whole scene as opaque
    // by traversing the whole scenegraph
    // Assumes camera transform is already set and 
    // appropriate shader is already bound
    if (gGLTFObject.isNull())
    {
        return;
    }

    if (gGLTFObject->isDead())
    {
        gGLTFObject = nullptr;
        return;
    }

    if (gGLTFAsset == nullptr)
    {
        return;
    }

    gGL.matrixMode(LLRender::MM_MODELVIEW);
    gGL.pushMatrix();

    LLMatrix4a modelview;
    modelview.loadu(gGLModelView);

    LLMatrix4a root;
    root.loadu((F32*) gGLTFObject->getRenderMatrix().mMatrix);
    LLVector3 scale = gGLTFObject->getScale();
    F32 scaleMat[16] = { scale.mV[0], 0.0f, 0.0f, 0.0f,
                            0.0f, scale.mV[1], 0.0f, 0.0f,
                            0.0f, 0.0f, scale.mV[2], 0.0f,
                            0.0f, 0.0f, 0.0f, 1.0f };
    LLMatrix4a scaleMat4;
    scaleMat4.loadu(scaleMat);
    matMul(scaleMat4, root, root);

    matMul(root, modelview, modelview);

    gGLTFAsset->updateRenderTransforms(modelview);

    gGLTFAsset->renderOpaque();

    gGL.popMatrix();
}


