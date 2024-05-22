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
#include "llviewertexturelist.h"
#include "llimagej2c.h"
#include "llfloaterperms.h"
#include "llagentbenefits.h"
#include "llfilesystem.h"
#include "boost/json.hpp"

#define GLTF_SIM_SUPPORT 1

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
            false);
    }
    else
    {
        LLNotificationsUtil::add("GLTFOpenSelection");
    }
}

void GLTFSceneManager::saveAs()
{
    LLViewerObject* obj = LLSelectMgr::instance().getSelection()->getFirstRootObject();
    if (obj && obj->mGLTFAsset)
    {
        LLFilePickerReplyThread::startPicker(
            [](const std::vector<std::string>& filenames, LLFilePicker::ELoadFilter load_filter, LLFilePicker::ESaveFilter save_filter)
            {
                if (LLAppViewer::instance()->quitRequested())
                {
                    return;
                }
                if (filenames.size() > 0)
                {
                    GLTFSceneManager::instance().save(filenames[0]);
                }
            },
            LLFilePicker::FFSAVE_GLTF,
            "scene.gltf");
    }
    else
    {
        LLNotificationsUtil::add("GLTFSaveSelection");
    }
}

void GLTFSceneManager::decomposeSelection()
{
    LLViewerObject* obj = LLSelectMgr::instance().getSelection()->getFirstRootObject();
    if (obj && obj->mGLTFAsset)
    {
        LLFilePickerReplyThread::startPicker(
            [](const std::vector<std::string>& filenames, LLFilePicker::ELoadFilter load_filter, LLFilePicker::ESaveFilter save_filter)
            {
                if (LLAppViewer::instance()->quitRequested())
                {
                    return;
                }
                if (filenames.size() > 0)
                {
                    GLTFSceneManager::instance().decomposeSelection(filenames[0]);
                }
            },
            LLFilePicker::FFSAVE_GLTF,
            "scene.gltf");
    }
    else
    {
        LLNotificationsUtil::add("GLTFSaveSelection");
    }
}

void GLTFSceneManager::uploadSelection()
{
    if (mUploadingAsset)
    { // upload already in progress
        LLNotificationsUtil::add("GLTFUploadInProgress");
        return;
    }

    LLViewerObject* obj = LLSelectMgr::instance().getSelection()->getFirstRootObject();
    if (obj && obj->mGLTFAsset)
    {
        // make a copy of the asset prior to uploading
        mUploadingAsset = std::make_shared<Asset>();
        mUploadingObject = obj;
        *mUploadingAsset = *obj->mGLTFAsset;

        GLTF::Asset& asset = *mUploadingAsset;

        for (auto& image : asset.mImages)
        {
            if (!image.mData.empty())
            {
                mPendingImageUploads++;

                LLPointer<LLImageRaw> raw = new LLImageRaw(image.mWidth, image.mHeight, image.mComponent);
                U8* data = raw->allocateData();
                llassert_always(image.mData.size() == raw->getDataSize());
                memcpy(data, image.mData.data(), image.mData.size());

                // for GLTF native content, store image in GLTF orientation
                raw->verticalFlip();

                LLPointer<LLImageJ2C> j2c = LLViewerTextureList::convertToUploadFile(raw);

                std::string buffer;
                buffer.assign((const char*)j2c->getData(), j2c->getDataSize());

                LLUUID asset_id = LLUUID::generateNewID();

                std::string name;
                S32 idx = (S32)(&image - &asset.mImages[0]);

                if (image.mName.empty())
                {

                    name = llformat("Image_%d", idx);
                }
                else
                {
                    name = image.mName;
                }

                LLNewBufferedResourceUploadInfo::uploadFailure_f failure = [this](LLUUID assetId, LLSD response, std::string reason)
                    {
                        // TODO: handle failure
                        mPendingImageUploads--;
                        return false;
                    };

                
                LLNewBufferedResourceUploadInfo::uploadFinish_f finish = [this, idx, raw, j2c](LLUUID assetId, LLSD response)
                    {
                        if (mUploadingAsset && mUploadingAsset->mImages.size() > idx)
                        {
                            mUploadingAsset->mImages[idx].mUri = assetId.asString();
                            mPendingImageUploads--;
                        }
                    };

                S32 expected_upload_cost = LLAgentBenefitsMgr::current().getTextureUploadCost(j2c);

                LLResourceUploadInfo::ptr_t uploadInfo(std::make_shared<LLNewBufferedResourceUploadInfo>(
                    buffer,
                    asset_id,
                    name,
                    name,
                    0,
                    LLFolderType::FT_TEXTURE,
                    LLInventoryType::IT_TEXTURE,
                    LLAssetType::AT_TEXTURE,
                    LLFloaterPerms::getNextOwnerPerms("Uploads"),
                    LLFloaterPerms::getGroupPerms("Uploads"),
                    LLFloaterPerms::getEveryonePerms("Uploads"),
                    expected_upload_cost,
                    false,
                    finish,
                    failure));

                upload_new_resource(uploadInfo);

                image.clearData(asset);
            }
        }

        // upload .bin
        for (auto& bin : asset.mBuffers)
        {
            mPendingBinaryUploads++;

            S32 idx = (S32)(&bin - &asset.mBuffers[0]);

            std::string buffer;
            buffer.assign((const char*)bin.mData.data(), bin.mData.size());

            LLUUID asset_id = LLUUID::generateNewID();

            LLNewBufferedResourceUploadInfo::uploadFailure_f failure = [this](LLUUID assetId, LLSD response, std::string reason)
                {
                    // TODO: handle failure
                    mPendingBinaryUploads--;
                    mUploadingAsset = nullptr;
                    mUploadingObject = nullptr;
                    LL_WARNS("GLTF") << "Failed to upload GLTF binary: " << reason << LL_ENDL;
                    LL_WARNS("GLTF") << response << LL_ENDL;
                    return false;
                };

            LLNewBufferedResourceUploadInfo::uploadFinish_f finish = [this, idx](LLUUID assetId, LLSD response)
                {
                    if (mUploadingAsset && mUploadingAsset->mBuffers.size() > idx)
                    {
                        mUploadingAsset->mBuffers[idx].mUri = assetId.asString();
                        mPendingBinaryUploads--;

                        // HACK: save buffer to cache to emulate a successful download
                        LLFileSystem cache(assetId, LLAssetType::AT_GLTF_BIN, LLFileSystem::WRITE);
                        auto& data = mUploadingAsset->mBuffers[idx].mData;

                        cache.write((const U8*)data.data(), data.size());
                    }
                };
#if GLTF_SIM_SUPPORT
            S32 expected_upload_cost = 1;

            LLResourceUploadInfo::ptr_t uploadInfo(std::make_shared<LLNewBufferedResourceUploadInfo>(
                buffer,
                asset_id,
                "",
                "",
                0,
                LLFolderType::FT_NONE,
                LLInventoryType::IT_GLTF_BIN,
                LLAssetType::AT_GLTF_BIN, 
                LLFloaterPerms::getNextOwnerPerms("Uploads"),
                LLFloaterPerms::getGroupPerms("Uploads"),
                LLFloaterPerms::getEveryonePerms("Uploads"),
                expected_upload_cost,
                false,
                finish,
                failure));

            upload_new_resource(uploadInfo);
#else
            // dummy finish
            finish(LLUUID::generateNewID(), LLSD());
#endif
        }
    }
    else
    {
        LLNotificationsUtil::add("GLTFUploadSelection");
    }
}

void GLTFSceneManager::decomposeSelection(const std::string& filename)
{
    LLViewerObject* obj = LLSelectMgr::instance().getSelection()->getFirstRootObject();
    if (obj && obj->mGLTFAsset)
    {
        // copy asset out for decomposition
        Asset asset = *obj->mGLTFAsset;

        // decompose the asset into component parts
        asset.decompose(filename);

        // copy decomposed asset into tinygltf for serialization
        tinygltf::Model model;
        asset.save(model);

        LLTinyGLTFHelper::saveModel(filename, model);
    }
}

void GLTFSceneManager::save(const std::string& filename)
{
    LLViewerObject* obj = LLSelectMgr::instance().getSelection()->getFirstRootObject();
    if (obj && obj->mGLTFAsset)
    {
        Asset* asset = obj->mGLTFAsset.get();
        tinygltf::Model model;
        asset->save(model);

        LLTinyGLTFHelper::saveModel(filename, model);
    }
}

void GLTFSceneManager::load(const std::string& filename)
{
    tinygltf::Model model;
    LLTinyGLTFHelper::loadModel(filename, model);

    std::shared_ptr<Asset> asset = std::make_shared<Asset>();
    *asset = model;

    gDebugProgram.bind(); // bind a shader to satisfy LLVertexBuffer assertions
    asset->allocateGLResources(filename, model);
    asset->updateTransforms();

    // hang the asset off the currently selected object, or off of the avatar if no object is selected
    LLViewerObject* obj = LLSelectMgr::instance().getSelection()->getFirstRootObject();

    if (obj)
    { // assign to self avatar
        obj->mGLTFAsset = asset;
        obj->markForUpdate();
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

void GLTFSceneManager::addGLTFObject(LLViewerObject* obj, LLUUID gltf_id)
{
    llassert(obj->getVolume()->getParams().getSculptID() == gltf_id);
    llassert(obj->getVolume()->getParams().getSculptType() == LL_SCULPT_TYPE_GLTF);

    obj->ref();
    gAssetStorage->getAssetData(gltf_id, LLAssetType::AT_GLTF, onGLTFLoadComplete, obj);
}

//static
void GLTFSceneManager::onGLTFBinLoadComplete(const LLUUID& id, LLAssetType::EType asset_type, void* user_data, S32 status, LLExtStat ext_status)
{
    LLViewerObject* obj = (LLViewerObject*)user_data;
    llassert(asset_type == LLAssetType::AT_GLTF_BIN);

    if (status == LL_ERR_NOERR)
    {
        if (obj)
        {
            // find the Buffer with the given id in the asset
            if (obj->mGLTFAsset)
            {
                for (auto& buffer : obj->mGLTFAsset->mBuffers)
                {
                    LLUUID buffer_id;
                    if (LLUUID::parseUUID(buffer.mUri, &buffer_id) && buffer_id == id)
                    {
                        LLFileSystem file(id, asset_type, LLFileSystem::READ);

                        buffer.mData.resize(file.getSize());
                        file.read((U8*)buffer.mData.data(), buffer.mData.size());

                        obj->mGLTFAsset->mPendingBuffers--;

                        if (obj->mGLTFAsset->mPendingBuffers == 0)
                        {
                            obj->mGLTFAsset->allocateGLResources();
                            GLTFSceneManager& mgr = GLTFSceneManager::instance();
                            if (std::find(mgr.mObjects.begin(), mgr.mObjects.end(), obj) == mgr.mObjects.end())
                            {
                                GLTFSceneManager::instance().mObjects.push_back(obj);
                            }
                        }
                    }
                }   
            }
            
            
        }
    }
    else
    {
        LL_WARNS("GLTF") << "Failed to load GLTF asset: " << id << LL_ENDL;
        obj->unref();
    }
}

//static
void GLTFSceneManager::onGLTFLoadComplete(const LLUUID& id, LLAssetType::EType asset_type, void* user_data, S32 status, LLExtStat ext_status)
{
    LLViewerObject* obj = (LLViewerObject*)user_data;
    llassert(asset_type == LLAssetType::AT_GLTF);

    if (status == LL_ERR_NOERR)
    {
        if (obj)
        {
            LLFileSystem file(id, asset_type, LLFileSystem::READ);
            std::string data;
            data.resize(file.getSize());
            file.read((U8*)data.data(), data.size());

            boost::json::value json = boost::json::parse(data);

            std::shared_ptr<Asset> asset = std::make_shared<Asset>(json);
            obj->mGLTFAsset = asset;

            for (auto& buffer : asset->mBuffers)
            {
                // for now just assume the buffer is already in the asset cache
                LLUUID buffer_id;
                if (LLUUID::parseUUID(buffer.mUri, &buffer_id))
                {
                    asset->mPendingBuffers++;

                    gAssetStorage->getAssetData(buffer_id, LLAssetType::AT_GLTF_BIN, onGLTFBinLoadComplete, obj);
                }
                else
                {
                    LL_WARNS("GLTF") << "Buffer URI is not a valid UUID: " << buffer.mUri << LL_ENDL;
                    obj->unref();
                    return;
                }
            }
        }
    }
    else
    {
        LL_WARNS("GLTF") << "Failed to load GLTF asset: " << id << LL_ENDL;
        obj->unref();
    }
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

        mObjects[i]->mGLTFAsset->update();
    }

    // process pending uploads
    if (mUploadingAsset && !mGLTFUploadPending)
    {
        if (mPendingImageUploads == 0 && mPendingBinaryUploads == 0)
        {
            std::string filename(gDirUtilp->getTempDir() + "/upload.gltf");
#if 0
            tinygltf::Model model;
            mUploadingAsset->save(model);

            tinygltf::TinyGLTF writer;
            
            writer.WriteGltfSceneToFile(&model, filename, false, false, true, false);
#else
            boost::json::object obj;
            mUploadingAsset->serialize(obj);
            std::string json = boost::json::serialize(obj, {});

            {
                std::ofstream o(filename);
                o << json;
            }
#endif

            std::ifstream t(filename);
            std::stringstream str;
            str << t.rdbuf();

            std::string buffer = str.str();

            LLNewBufferedResourceUploadInfo::uploadFailure_f failure = [this](LLUUID assetId, LLSD response, std::string reason)
                {
                    // TODO: handle failure
                    LL_WARNS("GLTF") << "Failed to upload GLTF json: " << reason << LL_ENDL;
                    LL_WARNS("GLTF") << response << LL_ENDL;

                    mUploadingAsset = nullptr;
                    mUploadingObject = nullptr;
                    mGLTFUploadPending = false;
                    return false;
                };

            LLNewBufferedResourceUploadInfo::uploadFinish_f finish = [this, buffer](LLUUID assetId, LLSD response)
            {
                LLAppViewer::instance()->postToMainCoro(
                    [=]()
                    {
                        if (mUploadingAsset)
                        {
                            // HACK: save buffer to cache to emulate a successful upload
                            LLFileSystem cache(assetId, LLAssetType::AT_GLTF, LLFileSystem::WRITE);

                            LL_INFOS("GLTF") << "Uploaded GLTF json: " << assetId << LL_ENDL;
                            cache.write((const U8 *) buffer.c_str(), buffer.size());

                            mUploadingAsset = nullptr;
                        }

                        if (mUploadingObject)
                        {
                            mUploadingObject->mGLTFAsset = nullptr;
                            mUploadingObject->setGLTFAsset(assetId);
                            mUploadingObject->markForUpdate();
                            mUploadingObject = nullptr;
                        }

                        mGLTFUploadPending = false;
                    });
            };

#if GLTF_SIM_SUPPORT
            S32 expected_upload_cost = 1;
            LLUUID asset_id = LLUUID::generateNewID();

            mGLTFUploadPending = true;

            LLResourceUploadInfo::ptr_t uploadInfo(std::make_shared<LLNewBufferedResourceUploadInfo>(
                buffer,
                asset_id,
                "",
                "",
                0,
                LLFolderType::FT_NONE,
                LLInventoryType::IT_GLTF,
                LLAssetType::AT_GLTF,
                LLFloaterPerms::getNextOwnerPerms("Uploads"),
                LLFloaterPerms::getGroupPerms("Uploads"),
                LLFloaterPerms::getEveryonePerms("Uploads"),
                expected_upload_cost,
                false,
                finish,
                failure));

            upload_new_resource(uploadInfo);
#else
            // dummy finish
            finish(LLUUID::generateNewID(), LLSD());
#endif
        }
    }
}

void GLTFSceneManager::render(bool opaque, bool rigged)
{
    // for debugging, just render the whole scene as opaque
    // by traversing the whole scenegraph
    // Assumes camera transform is already set and 
    // appropriate shader is already boundd
    
    gGL.matrixMode(LLRender::MM_MODELVIEW);

    for (U32 i = 0; i < mObjects.size(); ++i)
    {
        if (mObjects[i]->isDead() || mObjects[i]->mGLTFAsset == nullptr)
        {
            mObjects.erase(mObjects.begin() + i);
            --i;
            continue;
        }

        Asset* asset = mObjects[i]->mGLTFAsset.get();

        gGL.pushMatrix();

        LLMatrix4a mat = mObjects[i]->getGLTFAssetToAgentTransform();

        LLMatrix4a modelview;
        modelview.loadu(gGLModelView);

        matMul(mat, modelview, modelview);

        mat4 mdv = glm::make_mat4(modelview.getF32ptr());
        asset->updateRenderTransforms(mdv);
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

bool GLTFSceneManager::lineSegmentIntersect(LLVOVolume* obj, Asset* asset, const LLVector4a& start, const LLVector4a& end, S32 face, bool pick_transparent, bool pick_rigged, bool pick_unselectable, S32* node_hit, S32* primitive_hit,
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
    bool pick_transparent,
    bool pick_rigged,
    bool pick_unselectable,
    bool pick_reflection_probe,
    S32* node_hit,                   // return the index of the node that was hit
    S32* primitive_hit,               // return the index of the primitive that was hit
    LLVector4a* intersection,         // return the intersection point
    LLVector2* tex_coord,            // return the texture coordinates of the intersection point
    LLVector4a* normal,               // return the surface normal at the intersection point
    LLVector4a* tangent)			// return the surface tangent at the intersection point
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
        if (lineSegmentIntersect((LLVOVolume*) mObjects[i].get(), mObjects[i]->mGLTFAsset.get(), start, local_end, -1, pick_transparent, pick_rigged, pick_unselectable, node_hit, primitive_hit, &position, tex_coord, normal, tangent))
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

extern LLVector4a		gDebugRaycastStart;
extern LLVector4a		gDebugRaycastEnd;

void renderOctreeRaycast(const LLVector4a& start, const LLVector4a& end, const LLVolumeOctree* octree);

void renderAssetDebug(LLViewerObject* obj, Asset* asset)
{
    // render debug
    // assumes appropriate shader is already bound
    // assumes modelview matrix is already set

    gGL.pushMatrix();

    // get raycast in asset space
    LLMatrix4a agent_to_asset = obj->getAgentToGLTFAssetTransform();

    vec4 start;
    vec4 end;

    LLVector4a t;
    agent_to_asset.affineTransform(gDebugRaycastStart, t);
    start = glm::make_vec4(t.getF32ptr());
    agent_to_asset.affineTransform(gDebugRaycastEnd, t);
    end = glm::make_vec4(t.getF32ptr());

    start.w = end.w = 1.0;

    for (auto& node : asset->mNodes)
    {
        Mesh& mesh = asset->mMeshes[node.mMesh];

        if (node.mMesh != INVALID_INDEX)
        {
            gGL.loadMatrix((F32*)glm::value_ptr(node.mRenderMatrix));
            
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

#if 1
            if (gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_RAYCAST))
            {
                gGL.flush();
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

                // convert raycast to node local space
                vec4 local_start = node.mAssetMatrixInv * start;
                vec4 local_end = node.mAssetMatrixInv * end;

                for (auto& primitive : mesh.mPrimitives)
                {
                    if (primitive.mOctree.notNull())
                    {
                        LLVector4a s, e;
                        s.load3(glm::value_ptr(local_start));
                        e.load3(glm::value_ptr(local_end));
                        renderOctreeRaycast(s, e, primitive.mOctree);
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

        mat4 mat = glm::make_mat4(obj->getGLTFAssetToAgentTransform().getF32ptr());

        mat4 modelview = glm::make_mat4(gGLModelView);
        

        modelview = modelview * mat;
        
        Asset* asset = obj->mGLTFAsset.get();

        for (auto& node : asset->mNodes)
        {
            node.mRenderMatrix = modelview * node.mAssetMatrix;
        }
    }

    for (auto& obj : mObjects)
    {
        if (obj->isDead() || obj->mGLTFAsset == nullptr)
        {
            continue;
        }

        Asset* asset = obj->mGLTFAsset.get();

        renderAssetDebug(obj, asset);
    }

    if (gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_NODES))
    { //render node hierarchy

        for (U32 i = 0; i < 2; ++i)
        {
            LLGLDepthTest depth(GL_TRUE, i == 0 ? GL_FALSE : GL_TRUE, i == 0 ? GL_GREATER : GL_LEQUAL);
            LLGLState blend(GL_BLEND, i == 0 ? GL_TRUE : GL_FALSE);


            gGL.pushMatrix();

            for (auto& obj : mObjects)
            {
                if (obj->isDead() || obj->mGLTFAsset == nullptr)
                {
                    continue;
                }

                mat4 mat = glm::make_mat4(obj->getGLTFAssetToAgentTransform().getF32ptr());

                mat4 modelview = glm::make_mat4(gGLModelView);

                modelview = modelview * mat;

                Asset* asset = obj->mGLTFAsset.get();

                for (auto& node : asset->mNodes)
                {
                    // force update all mRenderMatrix, not just nodes with meshes
                    node.mRenderMatrix = modelview * node.mAssetMatrix;

                    gGL.loadMatrix(glm::value_ptr(node.mRenderMatrix));
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

                        
                        gGL.vertex3fv(glm::value_ptr(child.mMatrix[3]));
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

        LLDrawable* drawable = lineSegmentIntersect(gDebugRaycastStart, gDebugRaycastEnd, true, true, true, true, &node_hit, &primitive_hit, &intersection, nullptr, nullptr, nullptr);

        if (drawable)
        {
            gGL.pushMatrix();
            Asset* asset = drawable->getVObj()->mGLTFAsset.get();
            Node* node = &asset->mNodes[node_hit];
            Primitive* primitive = &asset->mMeshes[node->mMesh].mPrimitives[primitive_hit];

            gGL.flush();
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            gGL.color3f(1, 0, 1);
            drawBoxOutline(intersection, LLVector4a(0.1f, 0.1f, 0.1f, 0.f));

            gGL.loadMatrix(glm::value_ptr(node->mRenderMatrix));


            auto* listener = (LLVolumeOctreeListener*) primitive->mOctree->getListener(0);
            drawBoxOutline(listener->mBounds[0], listener->mBounds[1]);

            gGL.flush();
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            gGL.popMatrix();
        }
    }
    gDebugProgram.unbind();
}



