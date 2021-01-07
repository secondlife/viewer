/**
 * @file llmodelpreview.cpp
 * @brief LLModelPreview class implementation
 *
 * $LicenseInfo:firstyear=2020&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2020, Linden Research, Inc.
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

#include "llmodelpreview.h"

#include "llmodelloader.h"
#include "lldaeloader.h"
#include "llfloatermodelpreview.h"

#include "llagent.h"
#include "llanimationstates.h"
#include "llcallbacklist.h"
#include "lldatapacker.h"
#include "lldrawable.h"
#include "llface.h"
#include "lliconctrl.h"
#include "llmatrix4a.h"
#include "llmeshrepository.h"
#include "llrender.h"
#include "llsdutil_math.h"
#include "llskinningutil.h"
#include "llstring.h"
#include "llsdserialize.h"
#include "lltoolmgr.h"
#include "llui.h"
#include "llvector4a.h"
#include "llviewercamera.h"
#include "llviewercontrol.h"
#include "llviewerobjectlist.h"
#include "llviewernetwork.h"
#include "llviewershadermgr.h"
#include "llviewertexteditor.h"
#include "llviewertexturelist.h"
#include "llvoavatar.h"
#include "pipeline.h"

// ui controls (from floater)
#include "llbutton.h"
#include "llcombobox.h"
#include "llspinctrl.h"
#include "lltabcontainer.h"
#include "lltextbox.h"

#include "glod/glod.h"
#include <boost/algorithm/string.hpp>

bool LLModelPreview::sIgnoreLoadedCallback = false;

// Extra configurability, to be exposed later in xml (LLModelPreview probably
// should become UI control at some point or get split into preview control)
static const LLColor4 PREVIEW_CANVAS_COL(0.169f, 0.169f, 0.169f, 1.f);
static const LLColor4 PREVIEW_EDGE_COL(0.4f, 0.4f, 0.4f, 1.0);
static const LLColor4 PREVIEW_BASE_COL(1.f, 1.f, 1.f, 1.f);
static const LLColor3 PREVIEW_BRIGHTNESS(0.9f, 0.9f, 0.9f);
static const F32 PREVIEW_EDGE_WIDTH(1.f);
static const LLColor4 PREVIEW_PSYH_EDGE_COL(0.f, 0.25f, 0.5f, 0.25f);
static const LLColor4 PREVIEW_PSYH_FILL_COL(0.f, 0.5f, 1.0f, 0.5f);
static const F32 PREVIEW_PSYH_EDGE_WIDTH(1.f);
static const LLColor4 PREVIEW_DEG_EDGE_COL(1.f, 0.f, 0.f, 1.f);
static const LLColor4 PREVIEW_DEG_FILL_COL(1.f, 0.f, 0.f, 0.5f);
static const F32 PREVIEW_DEG_EDGE_WIDTH(3.f);
static const F32 PREVIEW_DEG_POINT_SIZE(8.f);
static const F32 PREVIEW_ZOOM_LIMIT(10.f);

const F32 SKIN_WEIGHT_CAMERA_DISTANCE = 16.f;

BOOL stop_gloderror()
{
    GLuint error = glodGetError();

    if (error != GLOD_NO_ERROR)
    {
        LL_WARNS() << "GLOD error detected, cannot generate LOD: " << std::hex << error << LL_ENDL;
        return TRUE;
    }

    return FALSE;
}

LLViewerFetchedTexture* bindMaterialDiffuseTexture(const LLImportMaterial& material)
{
    LLViewerFetchedTexture *texture = LLViewerTextureManager::getFetchedTexture(material.getDiffuseMap(), FTT_DEFAULT, TRUE, LLGLTexture::BOOST_PREVIEW);

    if (texture)
    {
        if (texture->getDiscardLevel() > -1)
        {
            gGL.getTexUnit(0)->bind(texture, true);
            return texture;
        }
    }

    return NULL;
}

std::string stripSuffix(std::string name)
{
    if ((name.find("_LOD") != -1) || (name.find("_PHYS") != -1))
    {
        return name.substr(0, name.rfind('_'));
    }
    return name;
}

std::string getLodSuffix(S32 lod)
{
    std::string suffix;
    switch (lod)
    {
    case LLModel::LOD_IMPOSTOR: suffix = "_LOD0"; break;
    case LLModel::LOD_LOW:      suffix = "_LOD1"; break;
    case LLModel::LOD_MEDIUM:   suffix = "_LOD2"; break;
    case LLModel::LOD_PHYSICS:  suffix = "_PHYS"; break;
    case LLModel::LOD_HIGH:                       break;
    }
    return suffix;
}

void FindModel(LLModelLoader::scene& scene, const std::string& name_to_match, LLModel*& baseModelOut, LLMatrix4& matOut)
{
    LLModelLoader::scene::iterator base_iter = scene.begin();
    bool found = false;
    while (!found && (base_iter != scene.end()))
    {
        matOut = base_iter->first;

        LLModelLoader::model_instance_list::iterator base_instance_iter = base_iter->second.begin();
        while (!found && (base_instance_iter != base_iter->second.end()))
        {
            LLModelInstance& base_instance = *base_instance_iter++;
            LLModel* base_model = base_instance.mModel;

            if (base_model && (base_model->mLabel == name_to_match))
            {
                baseModelOut = base_model;
                return;
            }
        }
        base_iter++;
    }
}

//-----------------------------------------------------------------------------
// LLModelPreview
//-----------------------------------------------------------------------------

LLModelPreview::LLModelPreview(S32 width, S32 height, LLFloater* fmp)
    : LLViewerDynamicTexture(width, height, 3, ORDER_MIDDLE, FALSE), LLMutex()
    , mLodsQuery()
    , mLodsWithParsingError()
    , mPelvisZOffset(0.0f)
    , mLegacyRigFlags(U32_MAX)
    , mRigValidJointUpload(false)
    , mPhysicsSearchLOD(LLModel::LOD_PHYSICS)
    , mResetJoints(false)
    , mModelNoErrors(true)
    , mLastJointUpdate(false)
    , mFirstSkinUpdate(true)
    , mHasDegenerate(false)
    , mImporterDebug(LLCachedControl<bool>(gSavedSettings, "ImporterDebug", false))
{
    mNeedsUpdate = TRUE;
    mCameraDistance = 0.f;
    mCameraYaw = 0.f;
    mCameraPitch = 0.f;
    mCameraZoom = 1.f;
    mTextureName = 0;
    mPreviewLOD = 0;
    mModelLoader = NULL;
    mMaxTriangleLimit = 0;
    mDirty = false;
    mGenLOD = false;
    mLoading = false;
    mLookUpLodFiles = false;
    mLoadState = LLModelLoader::STARTING;
    mGroup = 0;
    mLODFrozen = false;
    mBuildShareTolerance = 0.f;
    mBuildQueueMode = GLOD_QUEUE_GREEDY;
    mBuildBorderMode = GLOD_BORDER_UNLOCK;
    mBuildOperator = GLOD_OPERATOR_EDGE_COLLAPSE;

    for (U32 i = 0; i < LLModel::NUM_LODS; ++i)
    {
        mRequestedTriangleCount[i] = 0;
        mRequestedCreaseAngle[i] = -1.f;
        mRequestedLoDMode[i] = 0;
        mRequestedErrorThreshold[i] = 0.f;
        mRequestedBuildOperator[i] = 0;
        mRequestedQueueMode[i] = 0;
        mRequestedBorderMode[i] = 0;
        mRequestedShareTolerance[i] = 0.f;
    }

    mViewOption["show_textures"] = false;

    mFMP = fmp;

    mHasPivot = false;
    mModelPivot = LLVector3(0.0f, 0.0f, 0.0f);

    glodInit();

    createPreviewAvatar();
}

LLModelPreview::~LLModelPreview()
{
    // glod apparently has internal mem alignment issues that are angering
    // the heap-check code in windows, these should be hunted down in that
    // TP code, if possible
    //
    // kernel32.dll!HeapFree()  + 0x14 bytes	
    // msvcr100.dll!free(void * pBlock)  Line 51	C
    // glod.dll!glodGetGroupParameteriv()  + 0x119 bytes	
    // glod.dll!glodShutdown()  + 0x77 bytes	
    //
    //glodShutdown();
    if (mModelLoader)
    {
        mModelLoader->shutdown();
    }

    if (mPreviewAvatar)
    {
        mPreviewAvatar->markDead();
        mPreviewAvatar = NULL;
    }
}

U32 LLModelPreview::calcResourceCost()
{
    assert_main_thread();

    rebuildUploadData();

    //Upload skin is selected BUT check to see if the joints coming in from the asset were malformed.
    if (mFMP && mFMP->childGetValue("upload_skin").asBoolean())
    {
        bool uploadingJointPositions = mFMP->childGetValue("upload_joints").asBoolean();
        if (uploadingJointPositions && !isRigValidForJointPositionUpload())
        {
            mFMP->childDisable("ok_btn");
        }
    }

    std::set<LLModel*> accounted;
    U32 num_points = 0;
    U32 num_hulls = 0;

    F32 debug_scale = mFMP ? mFMP->childGetValue("import_scale").asReal() : 1.f;
    mPelvisZOffset = mFMP ? mFMP->childGetValue("pelvis_offset").asReal() : 3.0f;

    if (mFMP && mFMP->childGetValue("upload_joints").asBoolean())
    {
        // FIXME if preview avatar ever gets reused, this fake mesh ID stuff will fail.
        // see also call to addAttachmentPosOverride.
        LLUUID fake_mesh_id;
        fake_mesh_id.generate();
        getPreviewAvatar()->addPelvisFixup(mPelvisZOffset, fake_mesh_id);
    }

    F32 streaming_cost = 0.f;
    F32 physics_cost = 0.f;
    for (U32 i = 0; i < mUploadData.size(); ++i)
    {
        LLModelInstance& instance = mUploadData[i];

        if (accounted.find(instance.mModel) == accounted.end())
        {
            accounted.insert(instance.mModel);

            LLModel::Decomposition& decomp =
                instance.mLOD[LLModel::LOD_PHYSICS] ?
                instance.mLOD[LLModel::LOD_PHYSICS]->mPhysics :
                instance.mModel->mPhysics;

            //update instance skin info for each lods pelvisZoffset 
            for (int j = 0; j<LLModel::NUM_LODS; ++j)
            {
                if (instance.mLOD[j])
                {
                    instance.mLOD[j]->mSkinInfo.mPelvisOffset = mPelvisZOffset;
                }
            }

            std::stringstream ostr;
            LLSD ret = LLModel::writeModel(ostr,
                instance.mLOD[4],
                instance.mLOD[3],
                instance.mLOD[2],
                instance.mLOD[1],
                instance.mLOD[0],
                decomp,
                mFMP->childGetValue("upload_skin").asBoolean(),
                mFMP->childGetValue("upload_joints").asBoolean(),
                mFMP->childGetValue("lock_scale_if_joint_position").asBoolean(),
                TRUE,
                FALSE,
                instance.mModel->mSubmodelID);

            num_hulls += decomp.mHull.size();
            for (U32 i = 0; i < decomp.mHull.size(); ++i)
            {
                num_points += decomp.mHull[i].size();
            }

            //calculate streaming cost
            LLMatrix4 transformation = instance.mTransform;

            LLVector3 position = LLVector3(0, 0, 0) * transformation;

            LLVector3 x_transformed = LLVector3(1, 0, 0) * transformation - position;
            LLVector3 y_transformed = LLVector3(0, 1, 0) * transformation - position;
            LLVector3 z_transformed = LLVector3(0, 0, 1) * transformation - position;
            F32 x_length = x_transformed.normalize();
            F32 y_length = y_transformed.normalize();
            F32 z_length = z_transformed.normalize();
            LLVector3 scale = LLVector3(x_length, y_length, z_length);

            F32 radius = scale.length()*0.5f*debug_scale;

            LLMeshCostData costs;
            if (gMeshRepo.getCostData(ret, costs))
            {
                streaming_cost += costs.getRadiusBasedStreamingCost(radius);
            }
        }
    }

    F32 scale = mFMP ? mFMP->childGetValue("import_scale").asReal()*2.f : 2.f;

    mDetailsSignal(mPreviewScale[0] * scale, mPreviewScale[1] * scale, mPreviewScale[2] * scale, streaming_cost, physics_cost);

    updateStatusMessages();

    return (U32)streaming_cost;
}

void LLModelPreview::rebuildUploadData()
{
    assert_main_thread();

    mUploadData.clear();
    mTextureSet.clear();

    //fill uploaddata instance vectors from scene data

    std::string requested_name = mFMP->getChild<LLUICtrl>("description_form")->getValue().asString();

    LLSpinCtrl* scale_spinner = mFMP->getChild<LLSpinCtrl>("import_scale");

    F32 scale = scale_spinner->getValue().asReal();

    LLMatrix4 scale_mat;
    scale_mat.initScale(LLVector3(scale, scale, scale));

    F32 max_scale = 0.f;

    BOOL legacyMatching = gSavedSettings.getBOOL("ImporterLegacyMatching");
    U32 load_state = 0;

    for (LLModelLoader::scene::iterator iter = mBaseScene.begin(); iter != mBaseScene.end(); ++iter)
    { //for each transform in scene
        LLMatrix4 mat = iter->first;

        // compute position
        LLVector3 position = LLVector3(0, 0, 0) * mat;

        // compute scale
        LLVector3 x_transformed = LLVector3(1, 0, 0) * mat - position;
        LLVector3 y_transformed = LLVector3(0, 1, 0) * mat - position;
        LLVector3 z_transformed = LLVector3(0, 0, 1) * mat - position;
        F32 x_length = x_transformed.normalize();
        F32 y_length = y_transformed.normalize();
        F32 z_length = z_transformed.normalize();

        max_scale = llmax(llmax(llmax(max_scale, x_length), y_length), z_length);

        mat *= scale_mat;

        for (LLModelLoader::model_instance_list::iterator model_iter = iter->second.begin(); model_iter != iter->second.end();)
        { //for each instance with said transform applied 
            LLModelInstance instance = *model_iter++;

            LLModel* base_model = instance.mModel;

            if (base_model && !requested_name.empty())
            {
                base_model->mRequestedLabel = requested_name;
            }

            for (int i = LLModel::NUM_LODS - 1; i >= LLModel::LOD_IMPOSTOR; i--)
            {
                LLModel* lod_model = NULL;
                if (!legacyMatching)
                {
                    // Fill LOD slots by finding matching meshes by label with name extensions
                    // in the appropriate scene for each LOD. This fixes all kinds of issues
                    // where the indexed method below fails in spectacular fashion.
                    // If you don't take the time to name your LOD and PHYS meshes
                    // with the name of their corresponding mesh in the HIGH LOD,
                    // then the indexed method will be attempted below.

                    LLMatrix4 transform;

                    std::string name_to_match = instance.mLabel;
                    llassert(!name_to_match.empty());

                    int extensionLOD;
                    if (i != LLModel::LOD_PHYSICS || mModel[LLModel::LOD_PHYSICS].empty())
                    {
                        extensionLOD = i;
                    }
                    else
                    {
                        //Physics can be inherited from other LODs or loaded, so we need to adjust what extension we are searching for
                        extensionLOD = mPhysicsSearchLOD;
                    }

                    std::string toAdd = getLodSuffix(extensionLOD);

                    if (name_to_match.find(toAdd) == -1)
                    {
                        name_to_match += toAdd;
                    }

                    FindModel(mScene[i], name_to_match, lod_model, transform);

                    if (!lod_model && i != LLModel::LOD_PHYSICS)
                    {
                        if (mImporterDebug)
                        {
                            std::ostringstream out;
                            out << "Search of" << name_to_match;
                            out << " in LOD" << i;
                            out << " list failed. Searching for alternative among LOD lists.";
                            LL_INFOS() << out.str() << LL_ENDL;
                            LLFloaterModelPreview::addStringToLog(out, false);
                        }

                        int searchLOD = (i > LLModel::LOD_HIGH) ? LLModel::LOD_HIGH : i;
                        while ((searchLOD <= LLModel::LOD_HIGH) && !lod_model)
                        {
                            std::string name_to_match = instance.mLabel;
                            llassert(!name_to_match.empty());

                            std::string toAdd = getLodSuffix(searchLOD);

                            if (name_to_match.find(toAdd) == -1)
                            {
                                name_to_match += toAdd;
                            }

                            // See if we can find an appropriately named model in LOD 'searchLOD'
                            //
                            FindModel(mScene[searchLOD], name_to_match, lod_model, transform);
                            searchLOD++;
                        }
                    }
                }
                else
                {
                    // Use old method of index-based association
                    U32 idx = 0;
                    for (idx = 0; idx < mBaseModel.size(); ++idx)
                    {
                        // find reference instance for this model
                        if (mBaseModel[idx] == base_model)
                        {
                            if (mImporterDebug)
                            {
                                std::ostringstream out;
                                out << "Attempting to use model index " << idx;
                                out << " for LOD" << i;
                                out << " of " << instance.mLabel;
                                LL_INFOS() << out.str() << LL_ENDL;
                                LLFloaterModelPreview::addStringToLog(out, false);
                            }
                            break;
                        }
                    }

                    // If the model list for the current LOD includes that index...
                    //
                    if (mModel[i].size() > idx)
                    {
                        // Assign that index from the model list for our LOD as the LOD model for this instance
                        //
                        lod_model = mModel[i][idx];
                        if (mImporterDebug)
                        {
                            std::ostringstream out;
                            out << "Indexed match of model index " << idx << " at LOD " << i << " to model named " << lod_model->mLabel;
                            LL_INFOS() << out.str() << LL_ENDL;
                            LLFloaterModelPreview::addStringToLog(out, false);
                        }
                    }
                    else if (mImporterDebug)
                    {
                        std::ostringstream out;
                        out << "List of models does not include index " << idx;
                        LL_INFOS() << out.str() << LL_ENDL;
                        LLFloaterModelPreview::addStringToLog(out, false);
                    }
                }

                if (lod_model)
                {
                    if (mImporterDebug)
                    {
                        std::ostringstream out;
                        if (i == LLModel::LOD_PHYSICS)
                        {
                            out << "Assigning collision for " << instance.mLabel << " to match " << lod_model->mLabel;
                        }
                        else
                        {
                            out << "Assigning LOD" << i << " for " << instance.mLabel << " to found match " << lod_model->mLabel;
                        }
                        LL_INFOS() << out.str() << LL_ENDL;
                        LLFloaterModelPreview::addStringToLog(out, false);
                    }
                    instance.mLOD[i] = lod_model;
                }
                else
                {
                    if (i < LLModel::LOD_HIGH && !lodsReady())
                    {
                        // assign a placeholder from previous LOD until lod generation is complete.
                        // Note: we might need to assign it regardless of conditions like named search does, to prevent crashes.
                        instance.mLOD[i] = instance.mLOD[i + 1];
                    }
                    if (mImporterDebug)
                    {
                        std::ostringstream out;
                        out << "List of models does not include " << instance.mLabel;
                        LL_INFOS() << out.str() << LL_ENDL;
                        LLFloaterModelPreview::addStringToLog(out, false);
                    }
                }
            }

            LLModel* high_lod_model = instance.mLOD[LLModel::LOD_HIGH];
            if (!high_lod_model)
            {
                LLFloaterModelPreview::addStringToLog("Model " + instance.mLabel + " has no High Lod (LOD3).", true);
                load_state = LLModelLoader::ERROR_MATERIALS;
                mFMP->childDisable("calculate_btn");
            }
            else
            {
                for (U32 i = 0; i < LLModel::NUM_LODS - 1; i++)
                {
                    int refFaceCnt = 0;
                    int modelFaceCnt = 0;
                    llassert(instance.mLOD[i]);
                    if (instance.mLOD[i] && !instance.mLOD[i]->matchMaterialOrder(high_lod_model, refFaceCnt, modelFaceCnt))
                    {
                        LLFloaterModelPreview::addStringToLog("Model " + instance.mLabel + " has mismatching materials between lods." , true);
                        load_state = LLModelLoader::ERROR_MATERIALS;
                        mFMP->childDisable("calculate_btn");
                    }
                }
                LLFloaterModelPreview* fmp = (LLFloaterModelPreview*)mFMP;
                bool upload_skinweights = fmp && fmp->childGetValue("upload_skin").asBoolean();
                if (upload_skinweights && high_lod_model->mSkinInfo.mJointNames.size() > 0)
                {
                    LLQuaternion bind_rot = LLSkinningUtil::getUnscaledQuaternion(high_lod_model->mSkinInfo.mBindShapeMatrix);
                    LLQuaternion identity;
                    if (!bind_rot.isEqualEps(identity, 0.01))
                    {
                        // Bind shape matrix is not in standard X-forward orientation.
                        // Might be good idea to only show this once. It can be spammy.
                        std::ostringstream out;
                        out << "non-identity bind shape rot. mat is ";
                        out << high_lod_model->mSkinInfo.mBindShapeMatrix;
                        out << " bind_rot ";
                        out << bind_rot;
                        LL_WARNS() << out.str() << LL_ENDL;

                        LLFloaterModelPreview::addStringToLog(out, getLoadState() != LLModelLoader::WARNING_BIND_SHAPE_ORIENTATION);
                        load_state = LLModelLoader::WARNING_BIND_SHAPE_ORIENTATION;
                    }
                }
            }
            instance.mTransform = mat;
            mUploadData.push_back(instance);
        }
    }

    for (U32 lod = 0; lod < LLModel::NUM_LODS - 1; lod++)
    {
        // Search for models that are not included into upload data
        // If we found any, that means something we loaded is not a sub-model.
        for (U32 model_ind = 0; model_ind < mModel[lod].size(); ++model_ind)
        {
            bool found_model = false;
            for (LLMeshUploadThread::instance_list::iterator iter = mUploadData.begin(); iter != mUploadData.end(); ++iter)
            {
                LLModelInstance& instance = *iter;
                if (instance.mLOD[lod] == mModel[lod][model_ind])
                {
                    found_model = true;
                    break;
                }
            }
            if (!found_model && mModel[lod][model_ind] && !mModel[lod][model_ind]->mSubmodelID)
            {
                if (mImporterDebug)
                {
                    std::ostringstream out;
                    out << "Model " << mModel[lod][model_ind]->mLabel << " was not used - mismatching lod models.";
                    LL_INFOS() << out.str() << LL_ENDL;
                    LLFloaterModelPreview::addStringToLog(out, true);
                }
                load_state = LLModelLoader::ERROR_MATERIALS;
                mFMP->childDisable("calculate_btn");
            }
        }
    }

    // Update state for notifications
    if (load_state > 0)
    {
        // encountered issues
        setLoadState(load_state);
    }
    else if (getLoadState() == LLModelLoader::ERROR_MATERIALS
             || getLoadState() == LLModelLoader::WARNING_BIND_SHAPE_ORIENTATION)
    {
        // This is only valid for these two error types because they are 
        // only used inside rebuildUploadData() and updateStatusMessages()
        // updateStatusMessages() is called after rebuildUploadData()
        setLoadState(LLModelLoader::DONE);
    }

    F32 max_import_scale = (DEFAULT_MAX_PRIM_SCALE - 0.1f) / max_scale;

    F32 max_axis = llmax(mPreviewScale.mV[0], mPreviewScale.mV[1]);
    max_axis = llmax(max_axis, mPreviewScale.mV[2]);
    max_axis *= 2.f;

    //clamp scale so that total imported model bounding box is smaller than 240m on a side
    max_import_scale = llmin(max_import_scale, 240.f / max_axis);

    scale_spinner->setMaxValue(max_import_scale);

    if (max_import_scale < scale)
    {
        scale_spinner->setValue(max_import_scale);
    }

}

void LLModelPreview::saveUploadData(bool save_skinweights, bool save_joint_positions, bool lock_scale_if_joint_position)
{
    if (!mLODFile[LLModel::LOD_HIGH].empty())
    {
        std::string filename = mLODFile[LLModel::LOD_HIGH];
        std::string slm_filename;

        if (LLModelLoader::getSLMFilename(filename, slm_filename))
        {
            saveUploadData(slm_filename, save_skinweights, save_joint_positions, lock_scale_if_joint_position);
        }
    }
}

void LLModelPreview::saveUploadData(const std::string& filename,
    bool save_skinweights, bool save_joint_positions, bool lock_scale_if_joint_position)
{

    std::set<LLPointer<LLModel> > meshes;
    std::map<LLModel*, std::string> mesh_binary;

    LLModel::hull empty_hull;

    LLSD data;

    data["version"] = SLM_SUPPORTED_VERSION;
    if (!mBaseModel.empty())
    {
        data["name"] = mBaseModel[0]->getName();
    }

    S32 mesh_id = 0;

    //build list of unique models and initialize local id
    for (U32 i = 0; i < mUploadData.size(); ++i)
    {
        LLModelInstance& instance = mUploadData[i];

        if (meshes.find(instance.mModel) == meshes.end())
        {
            instance.mModel->mLocalID = mesh_id++;
            meshes.insert(instance.mModel);

            std::stringstream str;
            LLModel::Decomposition& decomp =
                instance.mLOD[LLModel::LOD_PHYSICS].notNull() ?
                instance.mLOD[LLModel::LOD_PHYSICS]->mPhysics :
                instance.mModel->mPhysics;

            LLModel::writeModel(str,
                instance.mLOD[LLModel::LOD_PHYSICS],
                instance.mLOD[LLModel::LOD_HIGH],
                instance.mLOD[LLModel::LOD_MEDIUM],
                instance.mLOD[LLModel::LOD_LOW],
                instance.mLOD[LLModel::LOD_IMPOSTOR],
                decomp,
                save_skinweights,
                save_joint_positions,
                lock_scale_if_joint_position,
                FALSE, TRUE, instance.mModel->mSubmodelID);

            data["mesh"][instance.mModel->mLocalID] = str.str();
        }

        data["instance"][i] = instance.asLLSD();
    }

    llofstream out(filename.c_str(), std::ios_base::out | std::ios_base::binary);
    LLSDSerialize::toBinary(data, out);
    out.flush();
    out.close();
}

void LLModelPreview::clearModel(S32 lod)
{
    if (lod < 0 || lod > LLModel::LOD_PHYSICS)
    {
        return;
    }

    mVertexBuffer[lod].clear();
    mModel[lod].clear();
    mScene[lod].clear();
}

void LLModelPreview::getJointAliases(JointMap& joint_map)
{
    // Get all standard skeleton joints from the preview avatar.
    LLVOAvatar *av = getPreviewAvatar();

    //Joint names and aliases come from avatar_skeleton.xml

    joint_map = av->getJointAliases();

    std::vector<std::string> cv_names, attach_names;
    av->getSortedJointNames(1, cv_names);
    av->getSortedJointNames(2, attach_names);
    for (std::vector<std::string>::iterator it = cv_names.begin(); it != cv_names.end(); ++it)
    {
        joint_map[*it] = *it;
    }
    for (std::vector<std::string>::iterator it = attach_names.begin(); it != attach_names.end(); ++it)
    {
        joint_map[*it] = *it;
    }
}

void LLModelPreview::loadModel(std::string filename, S32 lod, bool force_disable_slm)
{
    assert_main_thread();

    LLMutexLock lock(this);

    if (lod < LLModel::LOD_IMPOSTOR || lod > LLModel::NUM_LODS - 1)
    {
        std::ostringstream out;
        out << "Invalid level of detail: ";
        out << lod;
        LL_WARNS() << out.str() << LL_ENDL;
        LLFloaterModelPreview::addStringToLog(out, true);
        assert(lod >= LLModel::LOD_IMPOSTOR && lod < LLModel::NUM_LODS);
        return;
    }

    // This triggers if you bring up the file picker and then hit CANCEL.
    // Just use the previous model (if any) and ignore that you brought up
    // the file picker.

    if (filename.empty())
    {
        if (mBaseModel.empty())
        {
            // this is the initial file picking. Close the whole floater
            // if we don't have a base model to show for high LOD.
            mFMP->closeFloater(false);
        }
        mLoading = false;
        return;
    }

    if (mModelLoader)
    {
        LL_WARNS() << "Incompleted model load operation pending." << LL_ENDL;
        return;
    }

    mLODFile[lod] = filename;

    if (lod == LLModel::LOD_HIGH)
    {
        clearGLODGroup();
    }

    std::map<std::string, std::string> joint_alias_map;
    getJointAliases(joint_alias_map);

    mModelLoader = new LLDAELoader(
        filename,
        lod,
        &LLModelPreview::loadedCallback,
        &LLModelPreview::lookupJointByName,
        &LLModelPreview::loadTextures,
        &LLModelPreview::stateChangedCallback,
        this,
        mJointTransformMap,
        mJointsFromNode,
        joint_alias_map,
        LLSkinningUtil::getMaxJointCount(),
        gSavedSettings.getU32("ImporterModelLimit"),
        gSavedSettings.getBOOL("ImporterPreprocessDAE"));

    if (force_disable_slm)
    {
        mModelLoader->mTrySLM = false;
    }
    else
    {
        // For MAINT-6647, we have set force_disable_slm to true,
        // which means this code path will never be taken. Trying to
        // re-use SLM files has never worked properly; in particular,
        // it tends to force the UI into strange checkbox options
        // which cannot be altered.

        //only try to load from slm if viewer is configured to do so and this is the 
        //initial model load (not an LoD or physics shape)
        mModelLoader->mTrySLM = gSavedSettings.getBOOL("MeshImportUseSLM") && mUploadData.empty();
    }
    mModelLoader->start();

    mFMP->childSetTextArg("status", "[STATUS]", mFMP->getString("status_reading_file"));

    setPreviewLOD(lod);

    if (getLoadState() >= LLModelLoader::ERROR_PARSING)
    {
        mFMP->childDisable("ok_btn");
        mFMP->childDisable("calculate_btn");
    }

    if (lod == mPreviewLOD)
    {
        mFMP->childSetValue("lod_file_" + lod_name[lod], mLODFile[lod]);
    }
    else if (lod == LLModel::LOD_PHYSICS)
    {
        mFMP->childSetValue("physics_file", mLODFile[lod]);
    }

    mFMP->openFloater();
}

void LLModelPreview::setPhysicsFromLOD(S32 lod)
{
    assert_main_thread();

    if (lod >= 0 && lod <= 3)
    {
        mPhysicsSearchLOD = lod;
        mModel[LLModel::LOD_PHYSICS] = mModel[lod];
        mScene[LLModel::LOD_PHYSICS] = mScene[lod];
        mLODFile[LLModel::LOD_PHYSICS].clear();
        mFMP->childSetValue("physics_file", mLODFile[LLModel::LOD_PHYSICS]);
        mVertexBuffer[LLModel::LOD_PHYSICS].clear();
        rebuildUploadData();
        refresh();
        updateStatusMessages();
    }
}

void LLModelPreview::clearIncompatible(S32 lod)
{
    //Don't discard models if specified model is the physic rep
    if (lod == LLModel::LOD_PHYSICS)
    {
        return;
    }

    // at this point we don't care about sub-models,
    // different amount of sub-models means face count mismatch, not incompatibility
    U32 lod_size = countRootModels(mModel[lod]);
    for (U32 i = 0; i <= LLModel::LOD_HIGH; i++)
    { //clear out any entries that aren't compatible with this model
        if (i != lod)
        {
            if (countRootModels(mModel[i]) != lod_size)
            {
                mModel[i].clear();
                mScene[i].clear();
                mVertexBuffer[i].clear();

                if (i == LLModel::LOD_HIGH)
                {
                    mBaseModel = mModel[lod];
                    clearGLODGroup();
                    mBaseScene = mScene[lod];
                    mVertexBuffer[5].clear();
                }
            }
        }
    }
}

void LLModelPreview::clearGLODGroup()
{
    if (mGroup)
    {
        for (std::map<LLPointer<LLModel>, U32>::iterator iter = mObject.begin(); iter != mObject.end(); ++iter)
        {
            glodDeleteObject(iter->second);
            stop_gloderror();
        }
        mObject.clear();

        glodDeleteGroup(mGroup);
        stop_gloderror();
        mGroup = 0;
    }
}

void LLModelPreview::loadModelCallback(S32 loaded_lod)
{
    assert_main_thread();

    LLMutexLock lock(this);
    if (!mModelLoader)
    {
        mLoading = false;
        return;
    }
    if (getLoadState() >= LLModelLoader::ERROR_PARSING)
    {
        mLoading = false;
        mModelLoader = NULL;
        mLodsWithParsingError.push_back(loaded_lod);
        return;
    }

    mLodsWithParsingError.erase(std::remove(mLodsWithParsingError.begin(), mLodsWithParsingError.end(), loaded_lod), mLodsWithParsingError.end());
    if (mLodsWithParsingError.empty())
    {
        mFMP->childEnable("calculate_btn");
    }

    // Copy determinations about rig so UI will reflect them
    //
    setRigValidForJointPositionUpload(mModelLoader->isRigValidForJointPositionUpload());
    setLegacyRigFlags(mModelLoader->getLegacyRigFlags());

    mModelLoader->loadTextures();

    if (loaded_lod == -1)
    { //populate all LoDs from model loader scene
        mBaseModel.clear();
        mBaseScene.clear();

        bool skin_weights = false;
        bool joint_overrides = false;
        bool lock_scale_if_joint_position = false;

        for (S32 lod = 0; lod < LLModel::NUM_LODS; ++lod)
        { //for each LoD

            //clear scene and model info
            mScene[lod].clear();
            mModel[lod].clear();
            mVertexBuffer[lod].clear();

            if (mModelLoader->mScene.begin()->second[0].mLOD[lod].notNull())
            { //if this LoD exists in the loaded scene

                //copy scene to current LoD
                mScene[lod] = mModelLoader->mScene;

                //touch up copied scene to look like current LoD
                for (LLModelLoader::scene::iterator iter = mScene[lod].begin(); iter != mScene[lod].end(); ++iter)
                {
                    LLModelLoader::model_instance_list& list = iter->second;

                    for (LLModelLoader::model_instance_list::iterator list_iter = list.begin(); list_iter != list.end(); ++list_iter)
                    {
                        //override displayed model with current LoD
                        list_iter->mModel = list_iter->mLOD[lod];

                        if (!list_iter->mModel)
                        {
                            continue;
                        }

                        //add current model to current LoD's model list (LLModel::mLocalID makes a good vector index)
                        S32 idx = list_iter->mModel->mLocalID;

                        if (mModel[lod].size() <= idx)
                        { //stretch model list to fit model at given index
                            mModel[lod].resize(idx + 1);
                        }

                        mModel[lod][idx] = list_iter->mModel;
                        if (!list_iter->mModel->mSkinWeights.empty())
                        {
                            skin_weights = true;

                            if (!list_iter->mModel->mSkinInfo.mAlternateBindMatrix.empty())
                            {
                                joint_overrides = true;
                            }
                            if (list_iter->mModel->mSkinInfo.mLockScaleIfJointPosition)
                            {
                                lock_scale_if_joint_position = true;
                            }
                        }
                    }
                }
            }
        }

        if (mFMP)
        {
            LLFloaterModelPreview* fmp = (LLFloaterModelPreview*)mFMP;

            if (skin_weights)
            { //enable uploading/previewing of skin weights if present in .slm file
                fmp->enableViewOption("show_skin_weight");
                mViewOption["show_skin_weight"] = true;
                fmp->childSetValue("upload_skin", true);
            }

            if (joint_overrides)
            {
                fmp->enableViewOption("show_joint_overrides");
                mViewOption["show_joint_overrides"] = true;
                fmp->enableViewOption("show_joint_positions");
                mViewOption["show_joint_positions"] = true;
                fmp->childSetValue("upload_joints", true);
            }
            else
            {
                fmp->clearAvatarTab();
            }

            if (lock_scale_if_joint_position)
            {
                fmp->enableViewOption("lock_scale_if_joint_position");
                mViewOption["lock_scale_if_joint_position"] = true;
                fmp->childSetValue("lock_scale_if_joint_position", true);
            }
        }

        //copy high lod to base scene for LoD generation
        mBaseScene = mScene[LLModel::LOD_HIGH];
        mBaseModel = mModel[LLModel::LOD_HIGH];

        mDirty = true;
        resetPreviewTarget();
    }
    else
    { //only replace given LoD
        mModel[loaded_lod] = mModelLoader->mModelList;
        mScene[loaded_lod] = mModelLoader->mScene;
        mVertexBuffer[loaded_lod].clear();

        setPreviewLOD(loaded_lod);

        if (loaded_lod == LLModel::LOD_HIGH)
        { //save a copy of the highest LOD for automatic LOD manipulation
            if (mBaseModel.empty())
            { //first time we've loaded a model, auto-gen LoD
                mGenLOD = true;
            }

            mBaseModel = mModel[loaded_lod];
            clearGLODGroup();

            mBaseScene = mScene[loaded_lod];
            mVertexBuffer[5].clear();
        }
        else
        {
            BOOL legacyMatching = gSavedSettings.getBOOL("ImporterLegacyMatching");
            if (!legacyMatching)
            {
                if (!mBaseModel.empty())
                {
                    BOOL name_based = FALSE;
                    BOOL has_submodels = FALSE;
                    for (U32 idx = 0; idx < mBaseModel.size(); ++idx)
                    {
                        if (mBaseModel[idx]->mSubmodelID)
                        { // don't do index-based renaming when the base model has submodels
                            has_submodels = TRUE;
                            if (mImporterDebug)
                            {
                                std::ostringstream out;
                                out << "High LOD has submodels";
                                LL_INFOS() << out.str() << LL_ENDL;
                                LLFloaterModelPreview::addStringToLog(out, false);
                            }
                            break;
                        }
                    }

                    for (U32 idx = 0; idx < mModel[loaded_lod].size(); ++idx)
                    {
                        std::string loaded_name = stripSuffix(mModel[loaded_lod][idx]->mLabel);

                        LLModel* found_model = NULL;
                        LLMatrix4 transform;
                        FindModel(mBaseScene, loaded_name, found_model, transform);
                        if (found_model)
                        { // don't rename correctly named models (even if they are placed in a wrong order)
                            name_based = TRUE;
                        }

                        if (mModel[loaded_lod][idx]->mSubmodelID)
                        { // don't rename the models when loaded LOD model has submodels
                            has_submodels = TRUE;
                        }
                    }

                    if (mImporterDebug)
                    {
                        std::ostringstream out;
                        out << "Loaded LOD " << loaded_lod << ": correct names" << (name_based ? "" : "NOT ") << "found; submodels " << (has_submodels ? "" : "NOT ") << "found";
                        LL_INFOS() << out.str() << LL_ENDL;
                        LLFloaterModelPreview::addStringToLog(out, false);
                    }

                    if (!name_based && !has_submodels)
                    { // replace the name of the model loaded for any non-HIGH LOD to match the others (MAINT-5601)
                        // this actually works like "ImporterLegacyMatching" for this particular LOD
                        for (U32 idx = 0; idx < mModel[loaded_lod].size() && idx < mBaseModel.size(); ++idx)
                        {
                            std::string name = mBaseModel[idx]->mLabel;
                            std::string loaded_name = stripSuffix(mModel[loaded_lod][idx]->mLabel);

                            if (loaded_name != name)
                            {
                                name += getLodSuffix(loaded_lod);

                                if (mImporterDebug)
                                {
                                    std::ostringstream out;
                                    out << "Loded model name " << mModel[loaded_lod][idx]->mLabel;
                                    out << " for LOD " << loaded_lod;
                                    out << " doesn't match the base model. Renaming to " << name;
                                    LL_WARNS() << out.str() << LL_ENDL;
                                    LLFloaterModelPreview::addStringToLog(out, false);
                                }

                                mModel[loaded_lod][idx]->mLabel = name;
                            }
                        }
                    }
                }
            }
        }

        clearIncompatible(loaded_lod);

        mDirty = true;

        if (loaded_lod == LLModel::LOD_HIGH)
        {
            resetPreviewTarget();
        }
    }

    mLoading = false;
    if (mFMP)
    {
        if (!mBaseModel.empty())
        {
            const std::string& model_name = mBaseModel[0]->getName();
            LLLineEditor* description_form = mFMP->getChild<LLLineEditor>("description_form");
            if (description_form->getText().empty())
            {
                description_form->setText(model_name);
            }
            // Add info to log that loading is complete (purpose: separator between loading and other logs)
            LLSD args;
            args["MODEL_NAME"] = model_name; // Teoretically shouldn't be empty, but might be better idea to add filename here
            LLFloaterModelPreview::addStringToLog("ModelLoaded", args, false, loaded_lod);
        }
    }
    refresh();

    mModelLoadedSignal();

    mModelLoader = NULL;
}

void LLModelPreview::resetPreviewTarget()
{
    if (mModelLoader)
    {
        mPreviewTarget = (mModelLoader->mExtents[0] + mModelLoader->mExtents[1]) * 0.5f;
        mPreviewScale = (mModelLoader->mExtents[1] - mModelLoader->mExtents[0]) * 0.5f;
    }

    setPreviewTarget(mPreviewScale.magVec()*10.f);
}

void LLModelPreview::generateNormals()
{
    assert_main_thread();

    S32 which_lod = mPreviewLOD;

    if (which_lod > 4 || which_lod < 0 ||
        mModel[which_lod].empty())
    {
        return;
    }

    F32 angle_cutoff = mFMP->childGetValue("crease_angle").asReal();

    mRequestedCreaseAngle[which_lod] = angle_cutoff;

    angle_cutoff *= DEG_TO_RAD;

    if (which_lod == 3 && !mBaseModel.empty())
    {
        if (mBaseModelFacesCopy.empty())
        {
            mBaseModelFacesCopy.reserve(mBaseModel.size());
            for (LLModelLoader::model_list::iterator it = mBaseModel.begin(), itE = mBaseModel.end(); it != itE; ++it)
            {
                v_LLVolumeFace_t faces;
                (*it)->copyFacesTo(faces);
                mBaseModelFacesCopy.push_back(faces);
            }
        }

        for (LLModelLoader::model_list::iterator it = mBaseModel.begin(), itE = mBaseModel.end(); it != itE; ++it)
        {
            (*it)->generateNormals(angle_cutoff);
        }

        mVertexBuffer[5].clear();
    }

    bool perform_copy = mModelFacesCopy[which_lod].empty();
    if (perform_copy) {
        mModelFacesCopy[which_lod].reserve(mModel[which_lod].size());
    }

    for (LLModelLoader::model_list::iterator it = mModel[which_lod].begin(), itE = mModel[which_lod].end(); it != itE; ++it)
    {
        if (perform_copy)
        {
            v_LLVolumeFace_t faces;
            (*it)->copyFacesTo(faces);
            mModelFacesCopy[which_lod].push_back(faces);
        }

        (*it)->generateNormals(angle_cutoff);
    }

    mVertexBuffer[which_lod].clear();
    refresh();
    updateStatusMessages();
}

void LLModelPreview::restoreNormals()
{
    S32 which_lod = mPreviewLOD;

    if (which_lod > 4 || which_lod < 0 ||
        mModel[which_lod].empty())
    {
        return;
    }

    if (!mBaseModelFacesCopy.empty())
    {
        llassert(mBaseModelFacesCopy.size() == mBaseModel.size());

        vv_LLVolumeFace_t::const_iterator itF = mBaseModelFacesCopy.begin();
        for (LLModelLoader::model_list::iterator it = mBaseModel.begin(), itE = mBaseModel.end(); it != itE; ++it, ++itF)
        {
            (*it)->copyFacesFrom((*itF));
        }

        mBaseModelFacesCopy.clear();
    }

    if (!mModelFacesCopy[which_lod].empty())
    {
        vv_LLVolumeFace_t::const_iterator itF = mModelFacesCopy[which_lod].begin();
        for (LLModelLoader::model_list::iterator it = mModel[which_lod].begin(), itE = mModel[which_lod].end(); it != itE; ++it, ++itF)
        {
            (*it)->copyFacesFrom((*itF));
        }

        mModelFacesCopy[which_lod].clear();
    }

    mVertexBuffer[which_lod].clear();
    refresh();
    updateStatusMessages();
}

void LLModelPreview::genLODs(S32 which_lod, U32 decimation, bool enforce_tri_limit)
{
    // Allow LoD from -1 to LLModel::LOD_PHYSICS
    if (which_lod < -1 || which_lod > LLModel::NUM_LODS - 1)
    {
        std::ostringstream out;
        out << "Invalid level of detail: " << which_lod;
        LL_WARNS() << out.str() << LL_ENDL;
        LLFloaterModelPreview::addStringToLog(out, false);
        assert(which_lod >= -1 && which_lod < LLModel::NUM_LODS);
        return;
    }

    if (mBaseModel.empty())
    {
        return;
    }

    LLVertexBuffer::unbind();

    bool no_ff = LLGLSLShader::sNoFixedFunction;
    LLGLSLShader* shader = LLGLSLShader::sCurBoundShaderPtr;
    LLGLSLShader::sNoFixedFunction = false;

    if (shader)
    {
        shader->unbind();
    }

    stop_gloderror();
    static U32 cur_name = 1;

    S32 limit = -1;

    U32 triangle_count = 0;

    U32 instanced_triangle_count = 0;

    //get the triangle count for the whole scene
    for (LLModelLoader::scene::iterator iter = mBaseScene.begin(), endIter = mBaseScene.end(); iter != endIter; ++iter)
    {
        for (LLModelLoader::model_instance_list::iterator instance = iter->second.begin(), end_instance = iter->second.end(); instance != end_instance; ++instance)
        {
            LLModel* mdl = instance->mModel;
            if (mdl)
            {
                instanced_triangle_count += mdl->getNumTriangles();
            }
        }
    }

    //get the triangle count for the non-instanced set of models
    for (U32 i = 0; i < mBaseModel.size(); ++i)
    {
        triangle_count += mBaseModel[i]->getNumTriangles();
    }

    //get ratio of uninstanced triangles to instanced triangles
    F32 triangle_ratio = (F32)triangle_count / (F32)instanced_triangle_count;

    U32 base_triangle_count = triangle_count;

    U32 type_mask = LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_NORMAL | LLVertexBuffer::MAP_TEXCOORD0;

    U32 lod_mode = 0;

    F32 lod_error_threshold = 0;

    // The LoD should be in range from Lowest to High
    if (which_lod > -1 && which_lod < NUM_LOD)
    {
        LLCtrlSelectionInterface* iface = mFMP->childGetSelectionInterface("lod_mode_" + lod_name[which_lod]);
        if (iface)
        {
            lod_mode = iface->getFirstSelectedIndex();
        }

        lod_error_threshold = mFMP->childGetValue("lod_error_threshold_" + lod_name[which_lod]).asReal();
    }

    if (which_lod != -1)
    {
        mRequestedLoDMode[which_lod] = lod_mode;
    }

    if (lod_mode == 0)
    {
        lod_mode = GLOD_TRIANGLE_BUDGET;

        // The LoD should be in range from Lowest to High
        if (which_lod > -1 && which_lod < NUM_LOD)
        {
            limit = mFMP->childGetValue("lod_triangle_limit_" + lod_name[which_lod]).asInteger();
            //convert from "scene wide" to "non-instanced" triangle limit
            limit = (S32)((F32)limit*triangle_ratio);
        }
    }
    else
    {
        lod_mode = GLOD_ERROR_THRESHOLD;
    }

    bool object_dirty = false;

    if (mGroup == 0)
    {
        object_dirty = true;
        mGroup = cur_name++;
        glodNewGroup(mGroup);
    }

    if (object_dirty)
    {
        for (LLModelLoader::model_list::iterator iter = mBaseModel.begin(); iter != mBaseModel.end(); ++iter)
        { //build GLOD objects for each model in base model list
            LLModel* mdl = *iter;

            if (mObject[mdl] != 0)
            {
                glodDeleteObject(mObject[mdl]);
            }

            mObject[mdl] = cur_name++;

            glodNewObject(mObject[mdl], mGroup, GLOD_DISCRETE);
            stop_gloderror();

            if (iter == mBaseModel.begin() && !mdl->mSkinWeights.empty())
            { //regenerate vertex buffer for skinned models to prevent animation feedback during LOD generation
                mVertexBuffer[5].clear();
            }

            if (mVertexBuffer[5].empty())
            {
                genBuffers(5, false);
            }

            U32 tri_count = 0;
            for (U32 i = 0; i < mVertexBuffer[5][mdl].size(); ++i)
            {
                LLVertexBuffer* buff = mVertexBuffer[5][mdl][i];
                buff->setBuffer(type_mask & buff->getTypeMask());

                U32 num_indices = mVertexBuffer[5][mdl][i]->getNumIndices();
                if (num_indices > 2)
                {
                    glodInsertElements(mObject[mdl], i, GL_TRIANGLES, num_indices, GL_UNSIGNED_SHORT, (U8*)mVertexBuffer[5][mdl][i]->getIndicesPointer(), 0, 0.f);
                }
                tri_count += num_indices / 3;
                stop_gloderror();
            }

            glodBuildObject(mObject[mdl]);
            stop_gloderror();
        }
    }


    S32 start = LLModel::LOD_HIGH;
    S32 end = 0;

    if (which_lod != -1)
    {
        start = end = which_lod;
    }

    mMaxTriangleLimit = base_triangle_count;

    for (S32 lod = start; lod >= end; --lod)
    {
        if (which_lod == -1)
        {
            if (lod < start)
            {
                triangle_count /= decimation;
            }
        }
        else
        {
            if (enforce_tri_limit)
            {
                triangle_count = limit;
            }
            else
            {
                for (S32 j = LLModel::LOD_HIGH; j>which_lod; --j)
                {
                    triangle_count /= decimation;
                }
            }
        }

        mModel[lod].clear();
        mModel[lod].resize(mBaseModel.size());
        mVertexBuffer[lod].clear();

        U32 actual_tris = 0;
        U32 actual_verts = 0;
        U32 submeshes = 0;

        mRequestedTriangleCount[lod] = (S32)((F32)triangle_count / triangle_ratio);
        mRequestedErrorThreshold[lod] = lod_error_threshold;

        glodGroupParameteri(mGroup, GLOD_ADAPT_MODE, lod_mode);
        stop_gloderror();

        glodGroupParameteri(mGroup, GLOD_ERROR_MODE, GLOD_OBJECT_SPACE_ERROR);
        stop_gloderror();

        glodGroupParameterf(mGroup, GLOD_OBJECT_SPACE_ERROR_THRESHOLD, lod_error_threshold);
        stop_gloderror();

        if (lod_mode != GLOD_TRIANGLE_BUDGET)
        {
            glodGroupParameteri(mGroup, GLOD_MAX_TRIANGLES, 0);
        }
        else
        {
            //SH-632: always add 1 to desired amount to avoid decimating below desired amount
            glodGroupParameteri(mGroup, GLOD_MAX_TRIANGLES, triangle_count + 1);
        }

        stop_gloderror();
        glodAdaptGroup(mGroup);
        stop_gloderror();

        for (U32 mdl_idx = 0; mdl_idx < mBaseModel.size(); ++mdl_idx)
        {
            LLModel* base = mBaseModel[mdl_idx];

            GLint patch_count = 0;
            glodGetObjectParameteriv(mObject[base], GLOD_NUM_PATCHES, &patch_count);
            stop_gloderror();

            LLVolumeParams volume_params;
            volume_params.setType(LL_PCODE_PROFILE_SQUARE, LL_PCODE_PATH_LINE);
            mModel[lod][mdl_idx] = new LLModel(volume_params, 0.f);

            std::string name = base->mLabel + getLodSuffix(lod);

            mModel[lod][mdl_idx]->mLabel = name;
            mModel[lod][mdl_idx]->mSubmodelID = base->mSubmodelID;

            GLint* sizes = new GLint[patch_count * 2];
            glodGetObjectParameteriv(mObject[base], GLOD_PATCH_SIZES, sizes);
            stop_gloderror();

            GLint* names = new GLint[patch_count];
            glodGetObjectParameteriv(mObject[base], GLOD_PATCH_NAMES, names);
            stop_gloderror();

            mModel[lod][mdl_idx]->setNumVolumeFaces(patch_count);

            LLModel* target_model = mModel[lod][mdl_idx];

            for (GLint i = 0; i < patch_count; ++i)
            {
                type_mask = mVertexBuffer[5][base][i]->getTypeMask();

                LLPointer<LLVertexBuffer> buff = new LLVertexBuffer(type_mask, 0);

                if (sizes[i * 2 + 1] > 0 && sizes[i * 2] > 0)
                {
                    if (!buff->allocateBuffer(sizes[i * 2 + 1], sizes[i * 2], true))
                    {
                        // Todo: find a way to stop preview in this case instead of crashing
                        LL_ERRS() << "Failed buffer allocation during preview LOD generation."
                            << " Vertices: " << sizes[i * 2 + 1]
                            << " Indices: " << sizes[i * 2] << LL_ENDL;
                    }
                    buff->setBuffer(type_mask);
                    glodFillElements(mObject[base], names[i], GL_UNSIGNED_SHORT, (U8*)buff->getIndicesPointer());
                    stop_gloderror();
                }
                else
                {
                    // This face was eliminated or we failed to allocate buffer,
                    // attempt to create a dummy triangle (one vertex, 3 indices, all 0)
                    buff->allocateBuffer(1, 3, true);
                    memset((U8*)buff->getMappedData(), 0, buff->getSize());
                    memset((U8*)buff->getIndicesPointer(), 0, buff->getIndicesSize());
                }

                buff->validateRange(0, buff->getNumVerts() - 1, buff->getNumIndices(), 0);

                LLStrider<LLVector3> pos;
                LLStrider<LLVector3> norm;
                LLStrider<LLVector2> tc;
                LLStrider<U16> index;

                buff->getVertexStrider(pos);
                if (type_mask & LLVertexBuffer::MAP_NORMAL)
                {
                    buff->getNormalStrider(norm);
                }
                if (type_mask & LLVertexBuffer::MAP_TEXCOORD0)
                {
                    buff->getTexCoord0Strider(tc);
                }

                buff->getIndexStrider(index);

                target_model->setVolumeFaceData(names[i], pos, norm, tc, index, buff->getNumVerts(), buff->getNumIndices());
                actual_tris += buff->getNumIndices() / 3;
                actual_verts += buff->getNumVerts();
                ++submeshes;

                if (!validate_face(target_model->getVolumeFace(names[i])))
                {
                    LL_ERRS() << "Invalid face generated during LOD generation." << LL_ENDL;
                }
            }

            //blind copy skin weights and just take closest skin weight to point on
            //decimated mesh for now (auto-generating LODs with skin weights is still a bit
            //of an open problem).
            target_model->mPosition = base->mPosition;
            target_model->mSkinWeights = base->mSkinWeights;
            target_model->mSkinInfo = base->mSkinInfo;
            //copy material list
            target_model->mMaterialList = base->mMaterialList;

            if (!validate_model(target_model))
            {
                LL_ERRS() << "Invalid model generated when creating LODs" << LL_ENDL;
            }

            delete[] sizes;
            delete[] names;
        }

        //rebuild scene based on mBaseScene
        mScene[lod].clear();
        mScene[lod] = mBaseScene;

        for (U32 i = 0; i < mBaseModel.size(); ++i)
        {
            LLModel* mdl = mBaseModel[i];
            LLModel* target = mModel[lod][i];
            if (target)
            {
                for (LLModelLoader::scene::iterator iter = mScene[lod].begin(); iter != mScene[lod].end(); ++iter)
                {
                    for (U32 j = 0; j < iter->second.size(); ++j)
                    {
                        if (iter->second[j].mModel == mdl)
                        {
                            iter->second[j].mModel = target;
                        }
                    }
                }
            }
        }
    }

    mResourceCost = calcResourceCost();

    LLVertexBuffer::unbind();
    LLGLSLShader::sNoFixedFunction = no_ff;
    if (shader)
    {
        shader->bind();
    }
}

void LLModelPreview::updateStatusMessages()
{
    // bit mask values for physics errors. used to prevent overwrite of single line status
    // TODO: use this to provied multiline status
    enum PhysicsError
    {
        NONE = 0,
        NOHAVOK = 1,
        DEGENERATE = 2,
        TOOMANYHULLS = 4,
        TOOMANYVERTSINHULL = 8
    };

    assert_main_thread();

    U32 has_physics_error{ PhysicsError::NONE }; // physics error bitmap
    //triangle/vertex/submesh count for each mesh asset for each lod
    std::vector<S32> tris[LLModel::NUM_LODS];
    std::vector<S32> verts[LLModel::NUM_LODS];
    std::vector<S32> submeshes[LLModel::NUM_LODS];

    //total triangle/vertex/submesh count for each lod
    S32 total_tris[LLModel::NUM_LODS];
    S32 total_verts[LLModel::NUM_LODS];
    S32 total_submeshes[LLModel::NUM_LODS];

    for (U32 i = 0; i < LLModel::NUM_LODS - 1; i++)
    {
        total_tris[i] = 0;
        total_verts[i] = 0;
        total_submeshes[i] = 0;
    }

    for (LLMeshUploadThread::instance_list::iterator iter = mUploadData.begin(); iter != mUploadData.end(); ++iter)
    {
        LLModelInstance& instance = *iter;

        LLModel* model_high_lod = instance.mLOD[LLModel::LOD_HIGH];
        if (!model_high_lod)
        {
            setLoadState(LLModelLoader::ERROR_MATERIALS);
            mFMP->childDisable("calculate_btn");
            continue;
        }

        for (U32 i = 0; i < LLModel::NUM_LODS - 1; i++)
        {
            LLModel* lod_model = instance.mLOD[i];
            if (!lod_model)
            {
                setLoadState(LLModelLoader::ERROR_MATERIALS);
                mFMP->childDisable("calculate_btn");
            }
            else
            {
                //for each model in the lod
                S32 cur_tris = 0;
                S32 cur_verts = 0;
                S32 cur_submeshes = lod_model->getNumVolumeFaces();

                for (S32 j = 0; j < cur_submeshes; ++j)
                { //for each submesh (face), add triangles and vertices to current total
                    const LLVolumeFace& face = lod_model->getVolumeFace(j);
                    cur_tris += face.mNumIndices / 3;
                    cur_verts += face.mNumVertices;
                }

                std::string instance_name = instance.mLabel;

                if (mImporterDebug)
                {
                    // Useful for debugging generalized complaints below about total submeshes which don't have enough
                    // context to address exactly what needs to be fixed to move towards compliance with the rules.
                    //
                    std::ostringstream out;
                    out << "Instance " << lod_model->mLabel << " LOD " << i << " Verts: " << cur_verts;
                    LL_INFOS() << out.str() << LL_ENDL;
                    LLFloaterModelPreview::addStringToLog(out, false);

                    out.str("");
                    out << "Instance " << lod_model->mLabel << " LOD " << i << " Tris:  " << cur_tris;
                    LL_INFOS() << out.str() << LL_ENDL;
                    LLFloaterModelPreview::addStringToLog(out, false);

                    out.str("");
                    out << "Instance " << lod_model->mLabel << " LOD " << i << " Faces: " << cur_submeshes;
                    LL_INFOS() << out.str() << LL_ENDL;
                    LLFloaterModelPreview::addStringToLog(out, false);

                    out.str("");
                    LLModel::material_list::iterator mat_iter = lod_model->mMaterialList.begin();
                    while (mat_iter != lod_model->mMaterialList.end())
                    {
                        out << "Instance " << lod_model->mLabel << " LOD " << i << " Material " << *(mat_iter);
                        LL_INFOS() << out.str() << LL_ENDL;
                        LLFloaterModelPreview::addStringToLog(out, false);
                        out.str("");
                        mat_iter++;
                    }
                }

                //add this model to the lod total
                total_tris[i] += cur_tris;
                total_verts[i] += cur_verts;
                total_submeshes[i] += cur_submeshes;

                //store this model's counts to asset data
                tris[i].push_back(cur_tris);
                verts[i].push_back(cur_verts);
                submeshes[i].push_back(cur_submeshes);
            }
        }
    }

    if (mMaxTriangleLimit == 0)
    {
        mMaxTriangleLimit = total_tris[LLModel::LOD_HIGH];
    }

    mHasDegenerate = false;
    {//check for degenerate triangles in physics mesh
        U32 lod = LLModel::LOD_PHYSICS;
        const LLVector4a scale(0.5f);
        for (U32 i = 0; i < mModel[lod].size() && !mHasDegenerate; ++i)
        { //for each model in the lod
            if (mModel[lod][i] && mModel[lod][i]->mPhysics.mHull.empty())
            { //no decomp exists
                S32 cur_submeshes = mModel[lod][i]->getNumVolumeFaces();
                for (S32 j = 0; j < cur_submeshes && !mHasDegenerate; ++j)
                { //for each submesh (face), add triangles and vertices to current total
                    LLVolumeFace& face = mModel[lod][i]->getVolumeFace(j);
                    for (S32 k = 0; (k < face.mNumIndices) && !mHasDegenerate;)
                    {
                        U16 index_a = face.mIndices[k + 0];
                        U16 index_b = face.mIndices[k + 1];
                        U16 index_c = face.mIndices[k + 2];

                        if (index_c == 0 && index_b == 0 && index_a == 0) // test in reverse as 3rd index is less likely to be 0 in a normal case
                        {
                            LL_DEBUGS("MeshValidation") << "Empty placeholder triangle (3 identical index 0 verts) ignored" << LL_ENDL;
                        }
                        else
                        {
                            LLVector4a v1; v1.setMul(face.mPositions[index_a], scale);
                            LLVector4a v2; v2.setMul(face.mPositions[index_b], scale);
                            LLVector4a v3; v3.setMul(face.mPositions[index_c], scale);
                            if (ll_is_degenerate(v1, v2, v3))
                            {
                                mHasDegenerate = true;
                            }
                        }
                        k += 3;
                    }
                }
            }
        }
    }

    // flag degenerates here rather than deferring to a MAV error later
    mFMP->childSetVisible("physics_status_message_text", mHasDegenerate); //display or clear
    auto degenerateIcon = mFMP->getChild<LLIconCtrl>("physics_status_message_icon");
    degenerateIcon->setVisible(mHasDegenerate);
    if (mHasDegenerate)
    {
        has_physics_error |= PhysicsError::DEGENERATE;
        mFMP->childSetValue("physics_status_message_text", mFMP->getString("phys_status_degenerate_triangles"));
        LLUIImagePtr img = LLUI::getUIImage("ModelImport_Status_Error");
        degenerateIcon->setImage(img);
    }

    mFMP->childSetTextArg("submeshes_info", "[SUBMESHES]", llformat("%d", total_submeshes[LLModel::LOD_HIGH]));

    std::string mesh_status_na = mFMP->getString("mesh_status_na");

    S32 upload_status[LLModel::LOD_HIGH + 1];

    mModelNoErrors = true;

    const U32 lod_high = LLModel::LOD_HIGH;
    U32 high_submodel_count = mModel[lod_high].size() - countRootModels(mModel[lod_high]);

    for (S32 lod = 0; lod <= lod_high; ++lod)
    {
        upload_status[lod] = 0;

        std::string message = "mesh_status_good";

        if (total_tris[lod] > 0)
        {
            mFMP->childSetValue(lod_triangles_name[lod], llformat("%d", total_tris[lod]));
            mFMP->childSetValue(lod_vertices_name[lod], llformat("%d", total_verts[lod]));
        }
        else
        {
            if (lod == lod_high)
            {
                upload_status[lod] = 2;
                message = "mesh_status_missing_lod";
            }
            else
            {
                for (S32 i = lod - 1; i >= 0; --i)
                {
                    if (total_tris[i] > 0)
                    {
                        upload_status[lod] = 2;
                        message = "mesh_status_missing_lod";
                    }
                }
            }

            mFMP->childSetValue(lod_triangles_name[lod], mesh_status_na);
            mFMP->childSetValue(lod_vertices_name[lod], mesh_status_na);
        }

        if (lod != lod_high)
        {
            if (total_submeshes[lod] && total_submeshes[lod] != total_submeshes[lod_high])
            { //number of submeshes is different
                message = "mesh_status_submesh_mismatch";
                upload_status[lod] = 2;
            }
            else if (mModel[lod].size() - countRootModels(mModel[lod]) != high_submodel_count)
            {//number of submodels is different, not all faces are matched correctly.
                message = "mesh_status_submesh_mismatch";
                upload_status[lod] = 2;
                // Note: Submodels in instance were loaded from higher LOD and as result face count
                // returns same value and total_submeshes[lod] is identical to high_lod one.
            }
            else if (!tris[lod].empty() && tris[lod].size() != tris[lod_high].size())
            { //number of meshes is different
                message = "mesh_status_mesh_mismatch";
                upload_status[lod] = 2;
            }
            else if (!verts[lod].empty())
            {
                S32 sum_verts_higher_lod = 0;
                S32 sum_verts_this_lod = 0;
                for (U32 i = 0; i < verts[lod].size(); ++i)
                {
                    sum_verts_higher_lod += ((i < verts[lod + 1].size()) ? verts[lod + 1][i] : 0);
                    sum_verts_this_lod += verts[lod][i];
                }

                if ((sum_verts_higher_lod > 0) &&
                    (sum_verts_this_lod > sum_verts_higher_lod))
                {
                    //too many vertices in this lod
                    message = "mesh_status_too_many_vertices";
                    upload_status[lod] = 1;
                }
            }
        }

        LLIconCtrl* icon = mFMP->getChild<LLIconCtrl>(lod_icon_name[lod]);
        LLUIImagePtr img = LLUI::getUIImage(lod_status_image[upload_status[lod]]);
        icon->setVisible(true);
        icon->setImage(img);

        if (upload_status[lod] >= 2)
        {
            mModelNoErrors = false;
        }

        if (lod == mPreviewLOD)
        {
            mFMP->childSetValue("lod_status_message_text", mFMP->getString(message));
            icon = mFMP->getChild<LLIconCtrl>("lod_status_message_icon");
            icon->setImage(img);
        }

        updateLodControls(lod);
    }


    //warn if hulls have more than 256 points in them
    BOOL physExceededVertexLimit = FALSE;
    for (U32 i = 0; mModelNoErrors && i < mModel[LLModel::LOD_PHYSICS].size(); ++i)
    {
        LLModel* mdl = mModel[LLModel::LOD_PHYSICS][i];

        if (mdl)
        {
            for (U32 j = 0; j < mdl->mPhysics.mHull.size(); ++j)
            {
                if (mdl->mPhysics.mHull[j].size() > 256)
                {
                    physExceededVertexLimit = TRUE;
                    LL_INFOS() << "Physical model " << mdl->mLabel << " exceeds vertex per hull limitations." << LL_ENDL;
                    break;
                }
            }
        }
    }

    if (physExceededVertexLimit)
    {
        has_physics_error |= PhysicsError::TOOMANYVERTSINHULL;
    }

    if (!(has_physics_error & PhysicsError::DEGENERATE)){ // only update this field (incluides clearing it) if it is not already in use.
        mFMP->childSetVisible("physics_status_message_text", physExceededVertexLimit);
        LLIconCtrl* physStatusIcon = mFMP->getChild<LLIconCtrl>("physics_status_message_icon");
        physStatusIcon->setVisible(physExceededVertexLimit);
        if (physExceededVertexLimit)
        {
            mFMP->childSetValue("physics_status_message_text", mFMP->getString("phys_status_vertex_limit_exceeded"));
            LLUIImagePtr img = LLUI::getUIImage("ModelImport_Status_Warning");
            physStatusIcon->setImage(img);
        }
    }

    if (getLoadState() >= LLModelLoader::ERROR_PARSING)
    {
        mModelNoErrors = false;
        LL_INFOS() << "Loader returned errors, model can't be uploaded" << LL_ENDL;
    }

    bool uploadingSkin = mFMP->childGetValue("upload_skin").asBoolean();
    bool uploadingJointPositions = mFMP->childGetValue("upload_joints").asBoolean();

    if (uploadingSkin)
    {
        if (uploadingJointPositions && !isRigValidForJointPositionUpload())
        {
            mModelNoErrors = false;
            LL_INFOS() << "Invalid rig, there might be issues with uploading Joint positions" << LL_ENDL;
        }
    }

    if (mModelNoErrors && mModelLoader)
    {
        if (!mModelLoader->areTexturesReady() && mFMP->childGetValue("upload_textures").asBoolean())
        {
            // Some textures are still loading, prevent upload until they are done
            mModelNoErrors = false;
        }
    }

    if (!mModelNoErrors || mHasDegenerate)
    {
        mFMP->childDisable("ok_btn");
        mFMP->childDisable("calculate_btn");
    }
    else
    {
        mFMP->childEnable("ok_btn");
        mFMP->childEnable("calculate_btn");
    }

    if (mModelNoErrors && mLodsWithParsingError.empty())
    {
        mFMP->childEnable("calculate_btn");
    }
    else
    {
        mFMP->childDisable("calculate_btn");
    }

    //add up physics triangles etc
    S32 phys_tris = 0;
    S32 phys_hulls = 0;
    S32 phys_points = 0;

    //get the triangle count for the whole scene
    for (LLModelLoader::scene::iterator iter = mScene[LLModel::LOD_PHYSICS].begin(), endIter = mScene[LLModel::LOD_PHYSICS].end(); iter != endIter; ++iter)
    {
        for (LLModelLoader::model_instance_list::iterator instance = iter->second.begin(), end_instance = iter->second.end(); instance != end_instance; ++instance)
        {
            LLModel* model = instance->mModel;
            if (model)
            {
                S32 cur_submeshes = model->getNumVolumeFaces();

                LLModel::convex_hull_decomposition& decomp = model->mPhysics.mHull;

                if (!decomp.empty())
                {
                    phys_hulls += decomp.size();
                    for (U32 i = 0; i < decomp.size(); ++i)
                    {
                        phys_points += decomp[i].size();
                    }
                }
                else
                { //choose physics shape OR decomposition, can't use both
                    for (S32 j = 0; j < cur_submeshes; ++j)
                    { //for each submesh (face), add triangles and vertices to current total
                        const LLVolumeFace& face = model->getVolumeFace(j);
                        phys_tris += face.mNumIndices / 3;
                    }
                }
            }
        }
    }

    if (phys_tris > 0)
    {
        mFMP->childSetTextArg("physics_triangles", "[TRIANGLES]", llformat("%d", phys_tris));
    }
    else
    {
        mFMP->childSetTextArg("physics_triangles", "[TRIANGLES]", mesh_status_na);
    }

    if (phys_hulls > 0)
    {
        mFMP->childSetTextArg("physics_hulls", "[HULLS]", llformat("%d", phys_hulls));
        mFMP->childSetTextArg("physics_points", "[POINTS]", llformat("%d", phys_points));
    }
    else
    {
        mFMP->childSetTextArg("physics_hulls", "[HULLS]", mesh_status_na);
        mFMP->childSetTextArg("physics_points", "[POINTS]", mesh_status_na);
    }

    LLFloaterModelPreview* fmp = LLFloaterModelPreview::sInstance;
    if (fmp)
    {
        if (phys_tris > 0 || phys_hulls > 0)
        {
            if (!fmp->isViewOptionEnabled("show_physics"))
            {
                fmp->enableViewOption("show_physics");
                mViewOption["show_physics"] = true;
                fmp->childSetValue("show_physics", true);
            }
        }
        else
        {
            fmp->disableViewOption("show_physics");
            mViewOption["show_physics"] = false;
            fmp->childSetValue("show_physics", false);

        }

        //bool use_hull = fmp->childGetValue("physics_use_hull").asBoolean();

        //fmp->childSetEnabled("physics_optimize", !use_hull);

        bool enable = (phys_tris > 0 || phys_hulls > 0) && fmp->mCurRequest.empty();
        //enable = enable && !use_hull && fmp->childGetValue("physics_optimize").asBoolean();

        //enable/disable "analysis" UI
        LLPanel* panel = fmp->getChild<LLPanel>("physics analysis");
        LLView* child = panel->getFirstChild();
        while (child)
        {
            child->setEnabled(enable);
            child = panel->findNextSibling(child);
        }

        enable = phys_hulls > 0 && fmp->mCurRequest.empty();
        //enable/disable "simplification" UI
        panel = fmp->getChild<LLPanel>("physics simplification");
        child = panel->getFirstChild();
        while (child)
        {
            child->setEnabled(enable);
            child = panel->findNextSibling(child);
        }

        if (fmp->mCurRequest.empty())
        {
            fmp->childSetVisible("Simplify", true);
            fmp->childSetVisible("simplify_cancel", false);
            fmp->childSetVisible("Decompose", true);
            fmp->childSetVisible("decompose_cancel", false);

            if (phys_hulls > 0)
            {
                fmp->childEnable("Simplify");
            }

            if (phys_tris || phys_hulls > 0)
            {
                fmp->childEnable("Decompose");
            }
        }
        else
        {
            fmp->childEnable("simplify_cancel");
            fmp->childEnable("decompose_cancel");
        }
    }


    LLCtrlSelectionInterface* iface = fmp->childGetSelectionInterface("physics_lod_combo");
    S32 which_mode = 0;
    S32 file_mode = 1;
    if (iface)
    {
        which_mode = iface->getFirstSelectedIndex();
        file_mode = iface->getItemCount() - 1;
    }

    if (which_mode == file_mode)
    {
        mFMP->childEnable("physics_file");
        mFMP->childEnable("physics_browse");
    }
    else
    {
        mFMP->childDisable("physics_file");
        mFMP->childDisable("physics_browse");
    }

    LLSpinCtrl* crease = mFMP->getChild<LLSpinCtrl>("crease_angle");

    if (mRequestedCreaseAngle[mPreviewLOD] == -1.f)
    {
        mFMP->childSetColor("crease_label", LLColor4::grey);
        crease->forceSetValue(75.f);
    }
    else
    {
        mFMP->childSetColor("crease_label", LLColor4::white);
        crease->forceSetValue(mRequestedCreaseAngle[mPreviewLOD]);
    }

    mModelUpdatedSignal(true);

}

void LLModelPreview::updateLodControls(S32 lod)
{
    if (lod < LLModel::LOD_IMPOSTOR || lod > LLModel::LOD_HIGH)
    {
        std::ostringstream out;
        out << "Invalid level of detail: " << lod;
        LL_WARNS() << out.str() << LL_ENDL;
        LLFloaterModelPreview::addStringToLog(out, false);
        assert(lod >= LLModel::LOD_IMPOSTOR && lod <= LLModel::LOD_HIGH);
        return;
    }

    const char* lod_controls[] =
    {
        "lod_mode_",
        "lod_triangle_limit_",
        "lod_error_threshold_"
    };
    const U32 num_lod_controls = sizeof(lod_controls) / sizeof(char*);

    const char* file_controls[] =
    {
        "lod_browse_",
        "lod_file_",
    };
    const U32 num_file_controls = sizeof(file_controls) / sizeof(char*);

    LLFloaterModelPreview* fmp = LLFloaterModelPreview::sInstance;
    if (!fmp) return;

    LLComboBox* lod_combo = mFMP->findChild<LLComboBox>("lod_source_" + lod_name[lod]);
    if (!lod_combo) return;

    S32 lod_mode = lod_combo->getCurrentIndex();
    if (lod_mode == LOD_FROM_FILE) // LoD from file
    {
        fmp->mLODMode[lod] = 0;
        for (U32 i = 0; i < num_file_controls; ++i)
        {
            mFMP->childSetVisible(file_controls[i] + lod_name[lod], true);
        }

        for (U32 i = 0; i < num_lod_controls; ++i)
        {
            mFMP->childSetVisible(lod_controls[i] + lod_name[lod], false);
        }
    }
    else if (lod_mode == USE_LOD_ABOVE) // use LoD above
    {
        fmp->mLODMode[lod] = 2;
        for (U32 i = 0; i < num_file_controls; ++i)
        {
            mFMP->childSetVisible(file_controls[i] + lod_name[lod], false);
        }

        for (U32 i = 0; i < num_lod_controls; ++i)
        {
            mFMP->childSetVisible(lod_controls[i] + lod_name[lod], false);
        }

        if (lod < LLModel::LOD_HIGH)
        {
            mModel[lod] = mModel[lod + 1];
            mScene[lod] = mScene[lod + 1];
            mVertexBuffer[lod].clear();

            // Also update lower LoD
            if (lod > LLModel::LOD_IMPOSTOR)
            {
                updateLodControls(lod - 1);
            }
        }
    }
    else // auto generate, the default case for all LoDs except High
    {
        fmp->mLODMode[lod] = 1;

        //don't actually regenerate lod when refreshing UI
        mLODFrozen = true;

        for (U32 i = 0; i < num_file_controls; ++i)
        {
            mFMP->getChildView(file_controls[i] + lod_name[lod])->setVisible(false);
        }

        for (U32 i = 0; i < num_lod_controls; ++i)
        {
            mFMP->getChildView(lod_controls[i] + lod_name[lod])->setVisible(true);
        }


        LLSpinCtrl* threshold = mFMP->getChild<LLSpinCtrl>("lod_error_threshold_" + lod_name[lod]);
        LLSpinCtrl* limit = mFMP->getChild<LLSpinCtrl>("lod_triangle_limit_" + lod_name[lod]);

        limit->setMaxValue(mMaxTriangleLimit);
        limit->forceSetValue(mRequestedTriangleCount[lod]);

        threshold->forceSetValue(mRequestedErrorThreshold[lod]);

        mFMP->getChild<LLComboBox>("lod_mode_" + lod_name[lod])->selectNthItem(mRequestedLoDMode[lod]);

        if (mRequestedLoDMode[lod] == 0)
        {
            limit->setVisible(true);
            threshold->setVisible(false);

            limit->setMaxValue(mMaxTriangleLimit);
            limit->setIncrement(mMaxTriangleLimit / 32);
        }
        else
        {
            limit->setVisible(false);
            threshold->setVisible(true);
        }

        mLODFrozen = false;
    }
}

void LLModelPreview::setPreviewTarget(F32 distance)
{
    mCameraDistance = distance;
    mCameraZoom = 1.f;
    mCameraPitch = 0.f;
    mCameraYaw = 0.f;
    mCameraOffset.clearVec();
}

void LLModelPreview::clearBuffers()
{
    for (U32 i = 0; i < 6; i++)
    {
        mVertexBuffer[i].clear();
    }
}

void LLModelPreview::genBuffers(S32 lod, bool include_skin_weights)
{
    U32 tri_count = 0;
    U32 vertex_count = 0;
    U32 mesh_count = 0;


    LLModelLoader::model_list* model = NULL;

    if (lod < 0 || lod > 4)
    {
        model = &mBaseModel;
        lod = 5;
    }
    else
    {
        model = &(mModel[lod]);
    }

    if (!mVertexBuffer[lod].empty())
    {
        mVertexBuffer[lod].clear();
    }

    mVertexBuffer[lod].clear();

    LLModelLoader::model_list::iterator base_iter = mBaseModel.begin();

    for (LLModelLoader::model_list::iterator iter = model->begin(); iter != model->end(); ++iter)
    {
        LLModel* mdl = *iter;
        if (!mdl)
        {
            continue;
        }

        LLModel* base_mdl = *base_iter;
        base_iter++;

        S32 num_faces = mdl->getNumVolumeFaces();
        for (S32 i = 0; i < num_faces; ++i)
        {
            const LLVolumeFace &vf = mdl->getVolumeFace(i);
            U32 num_vertices = vf.mNumVertices;
            U32 num_indices = vf.mNumIndices;

            if (!num_vertices || !num_indices)
            {
                continue;
            }

            LLVertexBuffer* vb = NULL;

            bool skinned = include_skin_weights && !mdl->mSkinWeights.empty();

            U32 mask = LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_NORMAL | LLVertexBuffer::MAP_TEXCOORD0;

            if (skinned)
            {
                mask |= LLVertexBuffer::MAP_WEIGHT4;
            }

            vb = new LLVertexBuffer(mask, 0);

            if (!vb->allocateBuffer(num_vertices, num_indices, TRUE))
            {
                // We are likely to crash due this failure, if this happens, find a way to gracefully stop preview
                std::ostringstream out;
                out << "Failed to allocate Vertex Buffer for model preview ";
                out << num_vertices << " vertices and ";
                out << num_indices << " indices";
                LL_WARNS() << out.str() << LL_ENDL;
                LLFloaterModelPreview::addStringToLog(out, true);
            }

            LLStrider<LLVector3> vertex_strider;
            LLStrider<LLVector3> normal_strider;
            LLStrider<LLVector2> tc_strider;
            LLStrider<U16> index_strider;
            LLStrider<LLVector4> weights_strider;

            vb->getVertexStrider(vertex_strider);
            vb->getIndexStrider(index_strider);

            if (skinned)
            {
                vb->getWeight4Strider(weights_strider);
            }

            LLVector4a::memcpyNonAliased16((F32*)vertex_strider.get(), (F32*)vf.mPositions, num_vertices * 4 * sizeof(F32));

            if (vf.mTexCoords)
            {
                vb->getTexCoord0Strider(tc_strider);
                S32 tex_size = (num_vertices * 2 * sizeof(F32) + 0xF) & ~0xF;
                LLVector4a::memcpyNonAliased16((F32*)tc_strider.get(), (F32*)vf.mTexCoords, tex_size);
            }

            if (vf.mNormals)
            {
                vb->getNormalStrider(normal_strider);
                LLVector4a::memcpyNonAliased16((F32*)normal_strider.get(), (F32*)vf.mNormals, num_vertices * 4 * sizeof(F32));
            }

            if (skinned)
            {
                for (U32 i = 0; i < num_vertices; i++)
                {
                    //find closest weight to vf.mVertices[i].mPosition
                    LLVector3 pos(vf.mPositions[i].getF32ptr());

                    const LLModel::weight_list& weight_list = base_mdl->getJointInfluences(pos);
                    llassert(weight_list.size()>0 && weight_list.size() <= 4); // LLModel::loadModel() should guarantee this

                    LLVector4 w(0, 0, 0, 0);

                    for (U32 i = 0; i < weight_list.size(); ++i)
                    {
                        F32 wght = llclamp(weight_list[i].mWeight, 0.001f, 0.999f);
                        F32 joint = (F32)weight_list[i].mJointIdx;
                        w.mV[i] = joint + wght;
                        llassert(w.mV[i] - (S32)w.mV[i]>0.0f); // because weights are non-zero, and range of wt values
                        //should not cause floating point precision issues.
                    }

                    *(weights_strider++) = w;
                }
            }

            // build indices
            for (U32 i = 0; i < num_indices; i++)
            {
                *(index_strider++) = vf.mIndices[i];
            }

            mVertexBuffer[lod][mdl].push_back(vb);

            vertex_count += num_vertices;
            tri_count += num_indices / 3;
            ++mesh_count;

        }
    }
}

void LLModelPreview::update()
{
    if (mGenLOD)
    {
        bool subscribe_for_generation = mLodsQuery.empty();
        mGenLOD = false;
        mDirty = true;
        mLodsQuery.clear();

        for (S32 lod = LLModel::LOD_HIGH; lod >= 0; --lod)
        {
            // adding all lods into query for generation
            mLodsQuery.push_back(lod);
        }

        if (subscribe_for_generation)
        {
            doOnIdleRepeating(lodQueryCallback);
        }
    }

    if (mDirty && mLodsQuery.empty())
    {
        mDirty = false;
        mResourceCost = calcResourceCost();
        refresh();
        updateStatusMessages();
    }
}

//-----------------------------------------------------------------------------
// createPreviewAvatar
//-----------------------------------------------------------------------------
void LLModelPreview::createPreviewAvatar(void)
{
    mPreviewAvatar = (LLVOAvatar*)gObjectList.createObjectViewer(LL_PCODE_LEGACY_AVATAR, gAgent.getRegion(), LLViewerObject::CO_FLAG_UI_AVATAR);
    if (mPreviewAvatar)
    {
        mPreviewAvatar->createDrawable(&gPipeline);
        mPreviewAvatar->mSpecialRenderMode = 1;
        mPreviewAvatar->startMotion(ANIM_AGENT_STAND);
        mPreviewAvatar->hideSkirt();
    }
    else
    {
        LL_INFOS() << "Failed to create preview avatar for upload model window" << LL_ENDL;
    }
}

//static
U32 LLModelPreview::countRootModels(LLModelLoader::model_list models)
{
    U32 root_models = 0;
    model_list::iterator model_iter = models.begin();
    while (model_iter != models.end())
    {
        LLModel* mdl = *model_iter;
        if (mdl && mdl->mSubmodelID == 0)
        {
            root_models++;
        }
        model_iter++;
    }
    return root_models;
}

void LLModelPreview::loadedCallback(
    LLModelLoader::scene& scene,
    LLModelLoader::model_list& model_list,
    S32 lod,
    void* opaque)
{
    LLModelPreview* pPreview = static_cast< LLModelPreview* >(opaque);
    if (pPreview && !LLModelPreview::sIgnoreLoadedCallback)
    {
        // Load loader's warnings into floater's log tab
        const LLSD out = pPreview->mModelLoader->logOut();
        LLSD::array_const_iterator iter_out = out.beginArray();
        LLSD::array_const_iterator end_out = out.endArray();
        for (; iter_out != end_out; ++iter_out)
        {
            if (iter_out->has("Message"))
            {
                LLFloaterModelPreview::addStringToLog(iter_out->get("Message"), *iter_out, true, pPreview->mModelLoader->mLod);
            }
        }
        pPreview->mModelLoader->clearLog();
        pPreview->loadModelCallback(lod); // removes mModelLoader in some cases
        if (pPreview->mLookUpLodFiles && (lod != LLModel::LOD_HIGH))
        {
            pPreview->lookupLODModelFiles(lod);
        }
    }

}

void LLModelPreview::lookupLODModelFiles(S32 lod)
{
    if (lod == LLModel::LOD_PHYSICS)
    {
        mLookUpLodFiles = false;
        return;
    }
    S32 next_lod = (lod - 1 >= LLModel::LOD_IMPOSTOR) ? lod - 1 : LLModel::LOD_PHYSICS;

    std::string lod_filename = mLODFile[LLModel::LOD_HIGH];
    std::string ext = ".dae";
    std::string::size_type i = lod_filename.rfind(ext);
    if (i != std::string::npos)
    {
        lod_filename.replace(i, lod_filename.size() - ext.size(), getLodSuffix(next_lod) + ext);
    }
    if (gDirUtilp->fileExists(lod_filename))
    {
        LLFloaterModelPreview* fmp = LLFloaterModelPreview::sInstance;
        if (fmp)
        {
            fmp->setCtrlLoadFromFile(next_lod);
        }
        loadModel(lod_filename, next_lod);
    }
    else
    {
        lookupLODModelFiles(next_lod);
    }
}

void LLModelPreview::stateChangedCallback(U32 state, void* opaque)
{
    LLModelPreview* pPreview = static_cast< LLModelPreview* >(opaque);
    if (pPreview)
    {
        pPreview->setLoadState(state);
    }
}

LLJoint* LLModelPreview::lookupJointByName(const std::string& str, void* opaque)
{
    LLModelPreview* pPreview = static_cast< LLModelPreview* >(opaque);
    if (pPreview)
    {
        return pPreview->getPreviewAvatar()->getJoint(str);
    }
    return NULL;
}

U32 LLModelPreview::loadTextures(LLImportMaterial& material, void* opaque)
{
    (void)opaque;

    if (material.mDiffuseMapFilename.size())
    {
        material.mOpaqueData = new LLPointer< LLViewerFetchedTexture >;
        LLPointer< LLViewerFetchedTexture >& tex = (*reinterpret_cast< LLPointer< LLViewerFetchedTexture > * >(material.mOpaqueData));

        tex = LLViewerTextureManager::getFetchedTextureFromUrl("file://" + LLURI::unescape(material.mDiffuseMapFilename), FTT_LOCAL_FILE, TRUE, LLGLTexture::BOOST_PREVIEW);
        tex->setLoadedCallback(LLModelPreview::textureLoadedCallback, 0, TRUE, FALSE, opaque, NULL, FALSE);
        tex->forceToSaveRawImage(0, F32_MAX);
        material.setDiffuseMap(tex->getID()); // record tex ID
        return 1;
    }

    material.mOpaqueData = NULL;
    return 0;
}

void LLModelPreview::addEmptyFace(LLModel* pTarget)
{
    U32 type_mask = LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_NORMAL | LLVertexBuffer::MAP_TEXCOORD0;

    LLPointer<LLVertexBuffer> buff = new LLVertexBuffer(type_mask, 0);

    buff->allocateBuffer(1, 3, true);
    memset((U8*)buff->getMappedData(), 0, buff->getSize());
    memset((U8*)buff->getIndicesPointer(), 0, buff->getIndicesSize());

    buff->validateRange(0, buff->getNumVerts() - 1, buff->getNumIndices(), 0);

    LLStrider<LLVector3> pos;
    LLStrider<LLVector3> norm;
    LLStrider<LLVector2> tc;
    LLStrider<U16> index;

    buff->getVertexStrider(pos);

    if (type_mask & LLVertexBuffer::MAP_NORMAL)
    {
        buff->getNormalStrider(norm);
    }
    if (type_mask & LLVertexBuffer::MAP_TEXCOORD0)
    {
        buff->getTexCoord0Strider(tc);
    }

    buff->getIndexStrider(index);

    //resize face array
    int faceCnt = pTarget->getNumVolumeFaces();
    pTarget->setNumVolumeFaces(faceCnt + 1);
    pTarget->setVolumeFaceData(faceCnt + 1, pos, norm, tc, index, buff->getNumVerts(), buff->getNumIndices());

}

//-----------------------------------------------------------------------------
// render()
//-----------------------------------------------------------------------------
// Todo: we shouldn't be setting all those UI elements on render.
// Note: Render happens each frame with skinned avatars
BOOL LLModelPreview::render()
{
    assert_main_thread();

    LLMutexLock lock(this);
    mNeedsUpdate = FALSE;

    bool use_shaders = LLGLSLShader::sNoFixedFunction;

    bool edges = mViewOption["show_edges"];
    bool joint_overrides = mViewOption["show_joint_overrides"];
    bool joint_positions = mViewOption["show_joint_positions"];
    bool skin_weight = mViewOption["show_skin_weight"];
    bool textures = mViewOption["show_textures"];
    bool physics = mViewOption["show_physics"];

    S32 width = getWidth();
    S32 height = getHeight();

    LLGLSUIDefault def; // GL_BLEND, GL_ALPHA_TEST, GL_CULL_FACE, depth test
    LLGLDisable no_blend(GL_BLEND);
    LLGLEnable cull(GL_CULL_FACE);
    LLGLDepthTest depth(GL_FALSE); // SL-12781 disable z-buffer to render background color
    LLGLDisable fog(GL_FOG);

    {
        if (use_shaders)
        {
            gUIProgram.bind();
        }
        //clear background to grey
        gGL.matrixMode(LLRender::MM_PROJECTION);
        gGL.pushMatrix();
        gGL.loadIdentity();
        gGL.ortho(0.0f, width, 0.0f, height, -1.0f, 1.0f);

        gGL.matrixMode(LLRender::MM_MODELVIEW);
        gGL.pushMatrix();
        gGL.loadIdentity();

        gGL.color4fv(PREVIEW_CANVAS_COL.mV);
        gl_rect_2d_simple(width, height);

        gGL.matrixMode(LLRender::MM_PROJECTION);
        gGL.popMatrix();

        gGL.matrixMode(LLRender::MM_MODELVIEW);
        gGL.popMatrix();
        if (use_shaders)
        {
            gUIProgram.unbind();
        }
    }

    LLFloaterModelPreview* fmp = LLFloaterModelPreview::sInstance;

    bool has_skin_weights = false;
    bool upload_skin = mFMP->childGetValue("upload_skin").asBoolean();
    bool upload_joints = mFMP->childGetValue("upload_joints").asBoolean();

    if (upload_joints != mLastJointUpdate)
    {
        mLastJointUpdate = upload_joints;
        if (fmp)
        {
            fmp->clearAvatarTab();
        }
    }

    for (LLModelLoader::scene::iterator iter = mScene[mPreviewLOD].begin(); iter != mScene[mPreviewLOD].end(); ++iter)
    {
        for (LLModelLoader::model_instance_list::iterator model_iter = iter->second.begin(); model_iter != iter->second.end(); ++model_iter)
        {
            LLModelInstance& instance = *model_iter;
            LLModel* model = instance.mModel;
            model->mPelvisOffset = mPelvisZOffset;
            if (!model->mSkinWeights.empty())
            {
                has_skin_weights = true;
            }
        }
    }

    if (has_skin_weights && lodsReady())
    { //model has skin weights, enable view options for skin weights and joint positions
        U32 flags = getLegacyRigFlags();
        if (fmp)
        {
            if (flags == LEGACY_RIG_OK)
            {
                if (mFirstSkinUpdate)
                {
                    // auto enable weight upload if weights are present
                    // (note: all these UI updates need to be somewhere that is not render)
                    mViewOption["show_skin_weight"] = true;
                    skin_weight = true;
                    fmp->childSetValue("upload_skin", true);
                    mFirstSkinUpdate = false;
                }

                fmp->enableViewOption("show_skin_weight");
                fmp->setViewOptionEnabled("show_joint_overrides", skin_weight);
                fmp->setViewOptionEnabled("show_joint_positions", skin_weight);
                mFMP->childEnable("upload_skin");
                mFMP->childSetValue("show_skin_weight", skin_weight);

            }
            else if ((flags & LEGACY_RIG_FLAG_TOO_MANY_JOINTS) > 0)
            {
                mFMP->childSetVisible("skin_too_many_joints", true);
            }
            else if ((flags & LEGACY_RIG_FLAG_UNKNOWN_JOINT) > 0)
            {
                mFMP->childSetVisible("skin_unknown_joint", true);
            }
        }
    }
    else
    {
        mFMP->childDisable("upload_skin");
        if (fmp)
        {
            mViewOption["show_skin_weight"] = false;
            fmp->disableViewOption("show_skin_weight");
            fmp->disableViewOption("show_joint_overrides");
            fmp->disableViewOption("show_joint_positions");

            skin_weight = false;
            mFMP->childSetValue("show_skin_weight", false);
            fmp->setViewOptionEnabled("show_skin_weight", skin_weight);
        }
    }

    if (upload_skin && !has_skin_weights)
    { //can't upload skin weights if model has no skin weights
        mFMP->childSetValue("upload_skin", false);
        upload_skin = false;
    }

    if (!upload_skin && upload_joints)
    { //can't upload joints if not uploading skin weights
        mFMP->childSetValue("upload_joints", false);
        upload_joints = false;
    }

    if (fmp)
    {
        if (upload_skin)
        {
            // will populate list of joints
            fmp->updateAvatarTab(upload_joints);
        }
        else
        {
            fmp->clearAvatarTab();
        }
    }

    if (upload_skin && upload_joints)
    {
        mFMP->childEnable("lock_scale_if_joint_position");
    }
    else
    {
        mFMP->childDisable("lock_scale_if_joint_position");
        mFMP->childSetValue("lock_scale_if_joint_position", false);
    }

    //Only enable joint offsets if it passed the earlier critiquing
    if (isRigValidForJointPositionUpload())
    {
        mFMP->childSetEnabled("upload_joints", upload_skin);
    }

    F32 explode = mFMP->childGetValue("physics_explode").asReal();

    LLGLDepthTest gls_depth(GL_TRUE); // SL-12781 re-enable z-buffer for 3D model preview

    LLRect preview_rect;

    preview_rect = mFMP->getChildView("preview_panel")->getRect();

    F32 aspect = (F32)preview_rect.getWidth() / preview_rect.getHeight();

    LLViewerCamera::getInstance()->setAspect(aspect);

    LLViewerCamera::getInstance()->setView(LLViewerCamera::getInstance()->getDefaultFOV() / mCameraZoom);

    LLVector3 offset = mCameraOffset;
    LLVector3 target_pos = mPreviewTarget + offset;

    F32 z_near = 0.001f;
    F32 z_far = mCameraDistance*10.0f + mPreviewScale.magVec() + mCameraOffset.magVec();

    if (skin_weight)
    {
        target_pos = getPreviewAvatar()->getPositionAgent() + offset;
        z_near = 0.01f;
        z_far = 1024.f;

        //render avatar previews every frame
        refresh();
    }

    if (use_shaders)
    {
        gObjectPreviewProgram.bind();
    }

    gGL.loadIdentity();
    gPipeline.enableLightsPreview();

    LLQuaternion camera_rot = LLQuaternion(mCameraPitch, LLVector3::y_axis) *
        LLQuaternion(mCameraYaw, LLVector3::z_axis);

    LLQuaternion av_rot = camera_rot;
    F32 camera_distance = skin_weight ? SKIN_WEIGHT_CAMERA_DISTANCE : mCameraDistance;
    LLViewerCamera::getInstance()->setOriginAndLookAt(
        target_pos + ((LLVector3(camera_distance, 0.f, 0.f) + offset) * av_rot),		// camera
        LLVector3::z_axis,																	// up
        target_pos);											// point of interest


    z_near = llclamp(z_far * 0.001f, 0.001f, 0.1f);

    LLViewerCamera::getInstance()->setPerspective(FALSE, mOrigin.mX, mOrigin.mY, width, height, FALSE, z_near, z_far);

    stop_glerror();

    gGL.pushMatrix();
    gGL.color4fv(PREVIEW_EDGE_COL.mV);

    const U32 type_mask = LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_NORMAL | LLVertexBuffer::MAP_TEXCOORD0;

    LLGLEnable normalize(GL_NORMALIZE);

    if (!mBaseModel.empty() && mVertexBuffer[5].empty())
    {
        genBuffers(-1, skin_weight);
        //genBuffers(3);
        //genLODs();
    }

    if (!mModel[mPreviewLOD].empty())
    {
        mFMP->childEnable("reset_btn");

        bool regen = mVertexBuffer[mPreviewLOD].empty();
        if (!regen)
        {
            const std::vector<LLPointer<LLVertexBuffer> >& vb_vec = mVertexBuffer[mPreviewLOD].begin()->second;
            if (!vb_vec.empty())
            {
                const LLVertexBuffer* buff = vb_vec[0];
                regen = buff->hasDataType(LLVertexBuffer::TYPE_WEIGHT4) != skin_weight;
            }
            else
            {
                LL_INFOS() << "Vertex Buffer[" << mPreviewLOD << "]" << " is EMPTY!!!" << LL_ENDL;
                regen = TRUE;
            }
        }

        if (regen)
        {
            genBuffers(mPreviewLOD, skin_weight);
        }

        if (!skin_weight)
        {
            for (LLMeshUploadThread::instance_list::iterator iter = mUploadData.begin(); iter != mUploadData.end(); ++iter)
            {
                LLModelInstance& instance = *iter;

                LLModel* model = instance.mLOD[mPreviewLOD];

                if (!model)
                {
                    continue;
                }

                gGL.pushMatrix();
                LLMatrix4 mat = instance.mTransform;

                gGL.multMatrix((GLfloat*)mat.mMatrix);


                U32 num_models = mVertexBuffer[mPreviewLOD][model].size();
                for (U32 i = 0; i < num_models; ++i)
                {
                    LLVertexBuffer* buffer = mVertexBuffer[mPreviewLOD][model][i];

                    buffer->setBuffer(type_mask & buffer->getTypeMask());

                    if (textures)
                    {
                        int materialCnt = instance.mModel->mMaterialList.size();
                        if (i < materialCnt)
                        {
                            const std::string& binding = instance.mModel->mMaterialList[i];
                            const LLImportMaterial& material = instance.mMaterial[binding];

                            gGL.diffuseColor4fv(material.mDiffuseColor.mV);

                            // Find the tex for this material, bind it, and add it to our set
                            //
                            LLViewerFetchedTexture* tex = bindMaterialDiffuseTexture(material);
                            if (tex)
                            {
                                mTextureSet.insert(tex);
                            }
                        }
                    }
                    else
                    {
                        gGL.diffuseColor4fv(PREVIEW_BASE_COL.mV);
                    }

                    buffer->drawRange(LLRender::TRIANGLES, 0, buffer->getNumVerts() - 1, buffer->getNumIndices(), 0);
                    gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
                    gGL.diffuseColor4fv(PREVIEW_EDGE_COL.mV);
                    if (edges)
                    {
                        glLineWidth(PREVIEW_EDGE_WIDTH);
                        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                        buffer->drawRange(LLRender::TRIANGLES, 0, buffer->getNumVerts() - 1, buffer->getNumIndices(), 0);
                        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                        glLineWidth(1.f);
                    }
                }
                gGL.popMatrix();
            }

            if (physics)
            {
                glClear(GL_DEPTH_BUFFER_BIT);

                for (U32 pass = 0; pass < 2; pass++)
                {
                    if (pass == 0)
                    { //depth only pass
                        gGL.setColorMask(false, false);
                    }
                    else
                    {
                        gGL.setColorMask(true, true);
                    }

                    //enable alpha blending on second pass but not first pass
                    LLGLState blend(GL_BLEND, pass);

                    gGL.blendFunc(LLRender::BF_SOURCE_ALPHA, LLRender::BF_ONE_MINUS_SOURCE_ALPHA);

                    for (LLMeshUploadThread::instance_list::iterator iter = mUploadData.begin(); iter != mUploadData.end(); ++iter)
                    {
                        LLModelInstance& instance = *iter;

                        LLModel* model = instance.mLOD[LLModel::LOD_PHYSICS];

                        if (!model)
                        {
                            continue;
                        }

                        gGL.pushMatrix();
                        LLMatrix4 mat = instance.mTransform;

                        gGL.multMatrix((GLfloat*)mat.mMatrix);


                        bool render_mesh = true;
                        LLPhysicsDecomp* decomp = gMeshRepo.mDecompThread;
                        if (decomp)
                        {
                            LLMutexLock(decomp->mMutex);

                            LLModel::Decomposition& physics = model->mPhysics;

                            if (!physics.mHull.empty())
                            {
                                render_mesh = false;

                                if (physics.mMesh.empty())
                                { //build vertex buffer for physics mesh
                                    gMeshRepo.buildPhysicsMesh(physics);
                                }

                                if (!physics.mMesh.empty())
                                { //render hull instead of mesh
                                    for (U32 i = 0; i < physics.mMesh.size(); ++i)
                                    {
                                        if (explode > 0.f)
                                        {
                                            gGL.pushMatrix();

                                            LLVector3 offset = model->mHullCenter[i] - model->mCenterOfHullCenters;
                                            offset *= explode;

                                            gGL.translatef(offset.mV[0], offset.mV[1], offset.mV[2]);
                                        }

                                        static std::vector<LLColor4U> hull_colors;

                                        if (i + 1 >= hull_colors.size())
                                        {
                                            hull_colors.push_back(LLColor4U(rand() % 128 + 127, rand() % 128 + 127, rand() % 128 + 127, 128));
                                        }

                                        gGL.diffuseColor4ubv(hull_colors[i].mV);
                                        LLVertexBuffer::drawArrays(LLRender::TRIANGLES, physics.mMesh[i].mPositions, physics.mMesh[i].mNormals);

                                        if (explode > 0.f)
                                        {
                                            gGL.popMatrix();
                                        }
                                    }
                                }
                            }
                        }

                        if (render_mesh)
                        {
                            if (mVertexBuffer[LLModel::LOD_PHYSICS].empty())
                            {
                                genBuffers(LLModel::LOD_PHYSICS, false);
                            }

                            U32 num_models = mVertexBuffer[LLModel::LOD_PHYSICS][model].size();
                            if (pass > 0){
                                for (U32 i = 0; i < num_models; ++i)
                                {
                                    LLVertexBuffer* buffer = mVertexBuffer[LLModel::LOD_PHYSICS][model][i];

                                    gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
                                    gGL.diffuseColor4fv(PREVIEW_PSYH_FILL_COL.mV);

                                    buffer->setBuffer(type_mask & buffer->getTypeMask());
                                    buffer->drawRange(LLRender::TRIANGLES, 0, buffer->getNumVerts() - 1, buffer->getNumIndices(), 0);

                                    gGL.diffuseColor4fv(PREVIEW_PSYH_EDGE_COL.mV);
                                    glLineWidth(PREVIEW_PSYH_EDGE_WIDTH);
                                    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                                    buffer->drawRange(LLRender::TRIANGLES, 0, buffer->getNumVerts() - 1, buffer->getNumIndices(), 0);

                                    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                                    glLineWidth(1.f);
                                }
                            }
                        }
                        gGL.popMatrix();
                    }

                    // only do this if mDegenerate was set in the preceding mesh checks [Check this if the ordering ever breaks]
                    if (mHasDegenerate)
                    {
                        glLineWidth(PREVIEW_DEG_EDGE_WIDTH);
                        glPointSize(PREVIEW_DEG_POINT_SIZE);
                        gPipeline.enableLightsFullbright();
                        //show degenerate triangles
                        LLGLDepthTest depth(GL_TRUE, GL_TRUE, GL_ALWAYS);
                        LLGLDisable cull(GL_CULL_FACE);
                        gGL.diffuseColor4f(1.f, 0.f, 0.f, 1.f);
                        const LLVector4a scale(0.5f);

                        for (LLMeshUploadThread::instance_list::iterator iter = mUploadData.begin(); iter != mUploadData.end(); ++iter)
                        {
                            LLModelInstance& instance = *iter;

                            LLModel* model = instance.mLOD[LLModel::LOD_PHYSICS];

                            if (!model)
                            {
                                continue;
                            }

                            gGL.pushMatrix();
                            LLMatrix4 mat = instance.mTransform;

                            gGL.multMatrix((GLfloat*)mat.mMatrix);


                            LLPhysicsDecomp* decomp = gMeshRepo.mDecompThread;
                            if (decomp)
                            {
                                LLMutexLock(decomp->mMutex);

                                LLModel::Decomposition& physics = model->mPhysics;

                                if (physics.mHull.empty())
                                {
                                    if (mVertexBuffer[LLModel::LOD_PHYSICS].empty())
                                    {
                                        genBuffers(LLModel::LOD_PHYSICS, false);
                                    }

                                    U32 num_models = mVertexBuffer[LLModel::LOD_PHYSICS][model].size();
                                    for (U32 v = 0; v < num_models; ++v)
                                    {
                                        LLVertexBuffer* buffer = mVertexBuffer[LLModel::LOD_PHYSICS][model][v];

                                        buffer->setBuffer(type_mask & buffer->getTypeMask());

                                        LLStrider<LLVector3> pos_strider;
                                        buffer->getVertexStrider(pos_strider, 0);
                                        LLVector4a* pos = (LLVector4a*)pos_strider.get();

                                        LLStrider<U16> idx;
                                        buffer->getIndexStrider(idx, 0);

                                        for (U32 i = 0; i < buffer->getNumIndices(); i += 3)
                                        {
                                            LLVector4a v1; v1.setMul(pos[*idx++], scale);
                                            LLVector4a v2; v2.setMul(pos[*idx++], scale);
                                            LLVector4a v3; v3.setMul(pos[*idx++], scale);

                                            if (ll_is_degenerate(v1, v2, v3))
                                            {
                                                buffer->draw(LLRender::LINE_LOOP, 3, i);
                                                buffer->draw(LLRender::POINTS, 3, i);
                                            }
                                        }
                                    }
                                }
                            }

                            gGL.popMatrix();
                        }
                        glLineWidth(1.f);
                        glPointSize(1.f);
                        gPipeline.enableLightsPreview();
                        gGL.setSceneBlendType(LLRender::BT_ALPHA);
                    }
                }
            }
        }
        else
        {
            target_pos = getPreviewAvatar()->getPositionAgent();
            getPreviewAvatar()->clearAttachmentOverrides(); // removes pelvis fixup
            LLUUID fake_mesh_id;
            fake_mesh_id.generate();
            getPreviewAvatar()->addPelvisFixup(mPelvisZOffset, fake_mesh_id);
            bool pelvis_recalc = false;

            LLViewerCamera::getInstance()->setOriginAndLookAt(
                target_pos + ((LLVector3(camera_distance, 0.f, 0.f) + offset) * av_rot),		// camera
                LLVector3::z_axis,																	// up
                target_pos);											// point of interest

            for (LLModelLoader::scene::iterator iter = mScene[mPreviewLOD].begin(); iter != mScene[mPreviewLOD].end(); ++iter)
            {
                for (LLModelLoader::model_instance_list::iterator model_iter = iter->second.begin(); model_iter != iter->second.end(); ++model_iter)
                {
                    LLModelInstance& instance = *model_iter;
                    LLModel* model = instance.mModel;

                    if (!model->mSkinWeights.empty())
                    {
                        const LLMeshSkinInfo *skin = &model->mSkinInfo;
                        LLSkinningUtil::initJointNums(&model->mSkinInfo, getPreviewAvatar());// inits skin->mJointNums if nessesary
                        U32 joint_count = LLSkinningUtil::getMeshJointCount(skin);
                        U32 bind_count = skin->mAlternateBindMatrix.size();

                        if (joint_overrides
                            && bind_count > 0
                            && joint_count == bind_count)
                        {
                            // mesh_id is used to determine which mesh gets to
                            // set the joint offset, in the event of a conflict. Since
                            // we don't know the mesh id yet, we can't guarantee that
                            // joint offsets will be applied with the same priority as
                            // in the uploaded model. If the file contains multiple
                            // meshes with conflicting joint offsets, preview may be
                            // incorrect.
                            LLUUID fake_mesh_id;
                            fake_mesh_id.generate();
                            for (U32 j = 0; j < joint_count; ++j)
                            {
                                LLJoint *joint = getPreviewAvatar()->getJoint(skin->mJointNums[j]);
                                if (joint)
                                {
                                    const LLVector3& jointPos = skin->mAlternateBindMatrix[j].getTranslation();
                                    if (joint->aboveJointPosThreshold(jointPos))
                                    {
                                        bool override_changed;
                                        joint->addAttachmentPosOverride(jointPos, fake_mesh_id, "model", override_changed);

                                        if (override_changed)
                                        {
                                            //If joint is a pelvis then handle old/new pelvis to foot values
                                            if (joint->getName() == "mPelvis")// or skin->mJointNames[j]
                                            {
                                                pelvis_recalc = true;
                                            }
                                        }
                                        if (skin->mLockScaleIfJointPosition)
                                        {
                                            // Note that unlike positions, there's no threshold check here,
                                            // just a lock at the default value.
                                            joint->addAttachmentScaleOverride(joint->getDefaultScale(), fake_mesh_id, "model");
                                        }
                                    }
                                }
                            }
                        }

                        for (U32 i = 0, e = mVertexBuffer[mPreviewLOD][model].size(); i < e; ++i)
                        {
                            LLVertexBuffer* buffer = mVertexBuffer[mPreviewLOD][model][i];

                            const LLVolumeFace& face = model->getVolumeFace(i);

                            LLStrider<LLVector3> position;
                            buffer->getVertexStrider(position);

                            LLStrider<LLVector4> weight;
                            buffer->getWeight4Strider(weight);

                            //quick 'n dirty software vertex skinning

                            //build matrix palette

                            LLMatrix4a mat[LL_MAX_JOINTS_PER_MESH_OBJECT];
                            LLSkinningUtil::initSkinningMatrixPalette((LLMatrix4*)mat, joint_count,
                                skin, getPreviewAvatar());

                            LLMatrix4a bind_shape_matrix;
                            bind_shape_matrix.loadu(skin->mBindShapeMatrix);
                            U32 max_joints = LLSkinningUtil::getMaxJointCount();
                            for (U32 j = 0; j < buffer->getNumVerts(); ++j)
                            {
                                LLMatrix4a final_mat;
                                F32 *wptr = weight[j].mV;
                                LLSkinningUtil::getPerVertexSkinMatrix(wptr, mat, true, final_mat, max_joints);

                                //VECTORIZE THIS
                                LLVector4a& v = face.mPositions[j];

                                LLVector4a t;
                                LLVector4a dst;
                                bind_shape_matrix.affineTransform(v, t);
                                final_mat.affineTransform(t, dst);

                                position[j][0] = dst[0];
                                position[j][1] = dst[1];
                                position[j][2] = dst[2];
                            }

                            llassert(model->mMaterialList.size() > i);
                            const std::string& binding = instance.mModel->mMaterialList[i];
                            const LLImportMaterial& material = instance.mMaterial[binding];

                            buffer->setBuffer(type_mask & buffer->getTypeMask());
                            gGL.diffuseColor4fv(material.mDiffuseColor.mV);
                            gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);

                            // Find the tex for this material, bind it, and add it to our set
                            //
                            LLViewerFetchedTexture* tex = bindMaterialDiffuseTexture(material);
                            if (tex)
                            {
                                mTextureSet.insert(tex);
                            }

                            buffer->draw(LLRender::TRIANGLES, buffer->getNumIndices(), 0);

                            if (edges)
                            {
                                gGL.diffuseColor4fv(PREVIEW_EDGE_COL.mV);
                                glLineWidth(PREVIEW_EDGE_WIDTH);
                                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                                buffer->draw(LLRender::TRIANGLES, buffer->getNumIndices(), 0);
                                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                                glLineWidth(1.f);
                            }
                        }
                    }
                }
            }

            if (joint_positions)
            {
                LLGLSLShader* shader = LLGLSLShader::sCurBoundShaderPtr;
                if (shader)
                {
                    gDebugProgram.bind();
                }
                getPreviewAvatar()->renderCollisionVolumes();
                if (fmp->mTabContainer->getCurrentPanelIndex() == fmp->mAvatarTabIndex)
                {
                    getPreviewAvatar()->renderBones(fmp->mSelectedJointName);
                }
                else
                {
                    getPreviewAvatar()->renderBones();
                }
                if (shader)
                {
                    shader->bind();
                }
            }

            if (pelvis_recalc)
            {
                // size/scale recalculation
                getPreviewAvatar()->postPelvisSetRecalc();
            }
        }
    }

    if (use_shaders)
    {
        gObjectPreviewProgram.unbind();
    }

    gGL.popMatrix();

    return TRUE;
}

//-----------------------------------------------------------------------------
// refresh()
//-----------------------------------------------------------------------------
void LLModelPreview::refresh()
{
    mNeedsUpdate = TRUE;
}

//-----------------------------------------------------------------------------
// rotate()
//-----------------------------------------------------------------------------
void LLModelPreview::rotate(F32 yaw_radians, F32 pitch_radians)
{
    mCameraYaw = mCameraYaw + yaw_radians;

    mCameraPitch = llclamp(mCameraPitch + pitch_radians, F_PI_BY_TWO * -0.8f, F_PI_BY_TWO * 0.8f);
}

//-----------------------------------------------------------------------------
// zoom()
//-----------------------------------------------------------------------------
void LLModelPreview::zoom(F32 zoom_amt)
{
    F32 new_zoom = mCameraZoom + zoom_amt;
    // TODO: stop clamping in render
    mCameraZoom = llclamp(new_zoom, 1.f, PREVIEW_ZOOM_LIMIT);
}

void LLModelPreview::pan(F32 right, F32 up)
{
    bool skin_weight = mViewOption["show_skin_weight"];
    F32 camera_distance = skin_weight ? SKIN_WEIGHT_CAMERA_DISTANCE : mCameraDistance;
    mCameraOffset.mV[VY] = llclamp(mCameraOffset.mV[VY] + right * camera_distance / mCameraZoom, -1.f, 1.f);
    mCameraOffset.mV[VZ] = llclamp(mCameraOffset.mV[VZ] + up * camera_distance / mCameraZoom, -1.f, 1.f);
}

void LLModelPreview::setPreviewLOD(S32 lod)
{
    lod = llclamp(lod, 0, (S32)LLModel::LOD_HIGH);

    if (lod != mPreviewLOD)
    {
        mPreviewLOD = lod;

        LLComboBox* combo_box = mFMP->getChild<LLComboBox>("preview_lod_combo");
        combo_box->setCurrentByIndex((NUM_LOD - 1) - mPreviewLOD); // combo box list of lods is in reverse order
        mFMP->childSetValue("lod_file_" + lod_name[mPreviewLOD], mLODFile[mPreviewLOD]);

        LLColor4 highlight_color = LLUIColorTable::instance().getColor("MeshImportTableHighlightColor");
        LLColor4 normal_color = LLUIColorTable::instance().getColor("MeshImportTableNormalColor");

        for (S32 i = 0; i <= LLModel::LOD_HIGH; ++i)
        {
            const LLColor4& color = (i == lod) ? highlight_color : normal_color;

            mFMP->childSetColor(lod_status_name[i], color);
            mFMP->childSetColor(lod_label_name[i], color);
            mFMP->childSetColor(lod_triangles_name[i], color);
            mFMP->childSetColor(lod_vertices_name[i], color);
        }

        LLFloaterModelPreview* fmp = (LLFloaterModelPreview*)mFMP;
        if (fmp)
        {
            // make preview repopulate tab
            fmp->clearAvatarTab();
        }
    }
    refresh();
    updateStatusMessages();
}

//static
void LLModelPreview::textureLoadedCallback(
    BOOL success,
    LLViewerFetchedTexture *src_vi,
    LLImageRaw* src,
    LLImageRaw* src_aux,
    S32 discard_level,
    BOOL final,
    void* userdata)
{
    LLModelPreview* preview = (LLModelPreview*)userdata;
    preview->refresh();

    if (final && preview->mModelLoader)
    {
        if (preview->mModelLoader->mNumOfFetchingTextures > 0)
        {
            preview->mModelLoader->mNumOfFetchingTextures--;
        }
    }
}

// static
bool LLModelPreview::lodQueryCallback()
{
    // not the best solution, but model preview belongs to floater
    // so it is an easy way to check that preview still exists.
    LLFloaterModelPreview* fmp = LLFloaterModelPreview::sInstance;
    if (fmp && fmp->mModelPreview)
    {
        LLModelPreview* preview = fmp->mModelPreview;
        if (preview->mLodsQuery.size() > 0)
        {
            S32 lod = preview->mLodsQuery.back();
            preview->mLodsQuery.pop_back();
            preview->genLODs(lod);

            if (preview->mLookUpLodFiles && (lod == LLModel::LOD_HIGH))
            {
                preview->lookupLODModelFiles(LLModel::LOD_HIGH);
            }

            // return false to continue cycle
            return false;
        }
    }
    // nothing to process
    return true;
}

void LLModelPreview::onLODParamCommit(S32 lod, bool enforce_tri_limit)
{
    if (!mLODFrozen)
    {
        genLODs(lod, 3, enforce_tri_limit);
        refresh();
    }
}

