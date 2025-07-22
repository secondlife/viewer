/**
 * @file llmodelloader.cpp
 * @brief LLModelLoader class implementation
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
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

#include "llmodelloader.h"

#include "llapp.h"
#include "llsdserialize.h"
#include "lljoint.h"
#include "llcallbacklist.h"

#include "llmatrix4a.h"
#include <boost/bind.hpp>
#include <boost/exception/diagnostic_information.hpp>

std::list<LLModelLoader*> LLModelLoader::sActiveLoaderList;

static void stretch_extents(const LLModel* model, const LLMatrix4a& mat, LLVector4a& min, LLVector4a& max, bool& first_transform)
{
    LLVector4a box[] =
    {
        LLVector4a(-1, 1,-1),
        LLVector4a(-1, 1, 1),
        LLVector4a(-1,-1,-1),
        LLVector4a(-1,-1, 1),
        LLVector4a( 1, 1,-1),
        LLVector4a( 1, 1, 1),
        LLVector4a( 1,-1,-1),
        LLVector4a( 1,-1, 1),
    };

    for (S32 j = 0; j < model->getNumVolumeFaces(); ++j)
    {
        const LLVolumeFace& face = model->getVolumeFace(j);

        LLVector4a center;
        center.setAdd(face.mExtents[0], face.mExtents[1]);
        center.mul(0.5f);
        LLVector4a size;
        size.setSub(face.mExtents[1],face.mExtents[0]);
        size.mul(0.5f);

        for (U32 i = 0; i < 8; i++)
        {
            LLVector4a t;
            t.setMul(size, box[i]);
            t.add(center);

            LLVector4a v;

            mat.affineTransform(t, v);

            if (first_transform)
            {
                first_transform = false;
                min = max = v;
            }
            else
            {
                update_min_max(min, max, v);
            }
        }
    }
}

void LLModelLoader::stretch_extents(const LLModel* model, const LLMatrix4& mat)
{
    LLVector4a mina, maxa;
    LLMatrix4a mata;

    mata.loadu(mat);
    mina.load3(mExtents[0].mV);
    maxa.load3(mExtents[1].mV);

    ::stretch_extents(model, mata, mina, maxa, mFirstTransform);

    mExtents[0].set(mina.getF32ptr());
    mExtents[1].set(maxa.getF32ptr());
}

//-----------------------------------------------------------------------------
// LLModelLoader
//-----------------------------------------------------------------------------
LLModelLoader::LLModelLoader(
    std::string         filename,
    S32                 lod,
    load_callback_t     load_cb,
    joint_lookup_func_t joint_lookup_func,
    texture_load_func_t texture_load_func,
    state_callback_t    state_cb,
    void*               opaque_userdata,
    JointTransformMap&  jointTransformMap,
    JointNameSet&       jointsFromNodes,
    JointMap&           legalJointNamesMap,
    U32                 maxJointsPerMesh,
    U32                 modelLimit,
    U32                 debugMode)
: mJointList( jointTransformMap )
, mJointsFromNode( jointsFromNodes )
, LLThread("Model Loader")
, mFilename(filename)
, mLod(lod)
, mTrySLM(false)
, mFirstTransform(true)
, mNumOfFetchingTextures(0)
, mLoadCallback(load_cb)
, mJointLookupFunc(joint_lookup_func)
, mTextureLoadFunc(texture_load_func)
, mStateCallback(state_cb)
, mOpaqueData(opaque_userdata)
, mRigValidJointUpload(true)
, mLegacyRigFlags(0)
, mNoNormalize(false)
, mNoOptimize(false)
, mCacheOnlyHitIfRigged(false)
, mMaxJointsPerMesh(maxJointsPerMesh)
, mGeneratedModelLimit(modelLimit)
, mDebugMode(debugMode)
, mJointMap(legalJointNamesMap)
{
    assert_main_thread();
    sActiveLoaderList.push_back(this) ;
    mWarningsArray = LLSD::emptyArray();
}

LLModelLoader::~LLModelLoader()
{
    assert_main_thread();
    sActiveLoaderList.remove(this);
}

void LLModelLoader::run()
{
    mWarningsArray.clear();
    try
    {
        doLoadModel();
    }
    // Model loader isn't mission critical, so we just log all exceptions
    catch (const LLException& e)
    {
        LL_WARNS("THREAD") << "LLException in model loader: " << e.what() << "" << LL_ENDL;
        LLSD args;
        args["Message"] = "UnknownException";
        args["FILENAME"] = mFilename;
        args["EXCEPTION"] = e.what();
        mWarningsArray.append(args);
        setLoadState(ERROR_PARSING);
    }
    catch (const std::exception& e)
    {
        LL_WARNS() << "Exception in LLModelLoader::run: " << e.what() << LL_ENDL;
        LLSD args;
        args["Message"] = "UnknownException";
        args["FILENAME"] = mFilename;
        args["EXCEPTION"] = e.what();
        mWarningsArray.append(args);
        setLoadState(ERROR_PARSING);
    }
    catch (...)
    {
        LOG_UNHANDLED_EXCEPTION("LLModelLoader");
        LLSD args;
        args["Message"] = "UnknownException";
        args["FILENAME"] = mFilename;
        args["EXCEPTION"] = boost::current_exception_diagnostic_information();
        mWarningsArray.append(args);
        setLoadState(ERROR_PARSING);
    }

    // todo: we are inside of a thread, push this into main thread worker,
    // not into doOnIdleOneTime that laks tread safety
    doOnIdleOneTime(boost::bind(&LLModelLoader::loadModelCallback,this));
}

// static
bool LLModelLoader::getSLMFilename(const std::string& model_filename, std::string& slm_filename)
{
    slm_filename = model_filename;

    std::string::size_type i = model_filename.rfind(".");
    if (i != std::string::npos)
    {
        slm_filename.resize(i, '\0');
        slm_filename.append(".slm");
        return true;
    }
    else
    {
        return false;
    }
}

bool LLModelLoader::doLoadModel()
{
    //first, look for a .slm file of the same name that was modified later
    //than the specified model file

    if (mTrySLM)
    {
        std::string slm_filename;
        if (getSLMFilename(mFilename, slm_filename))
        {
            llstat slm_status;
            if (LLFile::stat(slm_filename, &slm_status) == 0)
            { //slm file exists
                llstat model_file_status;
                if (LLFile::stat(mFilename, &model_file_status) != 0 ||
                    model_file_status.st_mtime < slm_status.st_mtime)
                {
                    if (loadFromSLM(slm_filename))
                    { //slm successfully loaded, if this fails, fall through and
                        //try loading from model file

                        mLod = -1; //successfully loading from an slm implicitly sets all
                                    //LoDs
                        return true;
                    }
                }
            }
        }
    }

    bool res = OpenFile(mFilename);
    dumpDebugData(); // conditional on mDebugMode
    return res;
}

void LLModelLoader::setLoadState(U32 state)
{
    mStateCallback(state, mOpaqueData);
}

bool LLModelLoader::loadFromSLM(const std::string& filename)
{
    //only need to populate mScene with data from slm
    llstat stat;

    if (LLFile::stat(filename, &stat))
    { //file does not exist
        return false;
    }

    S32 file_size = (S32) stat.st_size;

    llifstream ifstream(filename.c_str(), std::ifstream::in | std::ifstream::binary);
    LLSD data;
    LLSDSerialize::fromBinary(data, ifstream, file_size);
    ifstream.close();

    //build model list for each LoD
    model_list model[LLModel::NUM_LODS];

    if (data["version"].asInteger() != SLM_SUPPORTED_VERSION)
    {  //unsupported version
        return false;
    }

    LLSD& mesh = data["mesh"];

    LLVolumeParams volume_params;
    volume_params.setType(LL_PCODE_PROFILE_SQUARE, LL_PCODE_PATH_LINE);

    for (S32 lod = 0; lod < LLModel::NUM_LODS; ++lod)
    {
        for (U32 i = 0; i < mesh.size(); ++i)
        {
            std::stringstream str(mesh[i].asString());
            LLPointer<LLModel> loaded_model = new LLModel(volume_params, (F32) lod);
            if (loaded_model->loadModel(str))
            {
                loaded_model->mLocalID = i;
                model[lod].push_back(loaded_model);

                if (lod == LLModel::LOD_HIGH)
                {
                    if (!loaded_model->mSkinInfo.mJointNames.empty())
                    {
                        //check to see if rig is valid
                        critiqueRigForUploadApplicability( loaded_model->mSkinInfo.mJointNames );
                    }
                    else if (mCacheOnlyHitIfRigged)
                    {
                        return false;
                    }
                }
            }
        }
    }

    if (model[LLModel::LOD_HIGH].empty())
    { //failed to load high lod
        return false;
    }

    //load instance list
    model_instance_list instance_list;

    LLSD& instance = data["instance"];

    for (U32 i = 0; i < instance.size(); ++i)
    {
        //deserialize instance list
        instance_list.push_back(LLModelInstance(instance[i]));

        //match up model instance pointers
        S32 idx = instance_list[i].mLocalMeshID;
        std::string instance_label = instance_list[i].mLabel;

        for (U32 lod = 0; lod < LLModel::NUM_LODS; ++lod)
        {
            if (!model[lod].empty())
            {
                if (idx >= model[lod].size())
                {
                    instance_list[i].mLOD[lod] = model[lod].front();
                    continue;
                }

                if (model[lod][idx]
                    && model[lod][idx]->mLabel.empty()
                    && !instance_label.empty())
                {
                    // restore model names
                    std::string name = instance_label;
                    switch (lod)
                    {
                    case LLModel::LOD_IMPOSTOR: name += "_LOD0"; break;
                    case LLModel::LOD_LOW:      name += "_LOD1"; break;
                    case LLModel::LOD_MEDIUM:   name += "_LOD2"; break;
                    case LLModel::LOD_PHYSICS:  name += "_PHYS"; break;
                    case LLModel::LOD_HIGH:                      break;
                    }
                    model[lod][idx]->mLabel = name;
                }

                instance_list[i].mLOD[lod] = model[lod][idx];
            }
        }

        if (!instance_list[i].mModel)
            instance_list[i].mModel = model[LLModel::LOD_HIGH][idx];
    }

    // Set name for UI to use
    std::string name = data["name"];
    if (!name.empty())
    {
        model[LLModel::LOD_HIGH][0]->mRequestedLabel = name;
    }


    //convert instance_list to mScene
    mFirstTransform = true;
    for (U32 i = 0; i < instance_list.size(); ++i)
    {
        LLModelInstance& cur_instance = instance_list[i];
        mScene[cur_instance.mTransform].push_back(cur_instance);
        stretch_extents(cur_instance.mModel, cur_instance.mTransform);
    }

    setLoadState( DONE );

    return true;
}

//static
bool LLModelLoader::isAlive(LLModelLoader* loader)
{
    if(!loader)
    {
        return false ;
    }

    std::list<LLModelLoader*>::iterator iter = sActiveLoaderList.begin() ;
    for(; iter != sActiveLoaderList.end() && (*iter) != loader; ++iter) ;

    return *iter == loader ;
}

void LLModelLoader::loadModelCallback()
{
    if (!LLApp::isExiting())
    {
        mLoadCallback(mScene, mModelList, mLod, mOpaqueData);
    }

    while (!isStopped())
    { //wait until this thread is stopped before deleting self
        apr_sleep(100);
    }

    //double check if "this" is valid before deleting it, in case it is aborted during running.
    if(!isAlive(this))
    {
        return ;
    }

    delete this;
}

//-----------------------------------------------------------------------------
// critiqueRigForUploadApplicability()
//-----------------------------------------------------------------------------
void LLModelLoader::critiqueRigForUploadApplicability( const std::vector<std::string> &jointListFromAsset )
{
    //Determines the following use cases for a rig:
    //1. It is suitable for upload with skin weights & joint positions, or
    //2. It is suitable for upload as standard av with just skin weights

    bool isJointPositionUploadOK = isRigSuitableForJointPositionUpload( jointListFromAsset );
    U32 legacy_rig_flags         = determineRigLegacyFlags( jointListFromAsset );

    // It's OK that both could end up being true.

    // Both start out as true and are forced to false if any mesh in
    // the model file is not vald by that criterion. Note that a file
    // can contain multiple meshes.
    if ( !isJointPositionUploadOK )
    {
        // This starts out true, becomes false if false for any loaded
        // mesh.
        setRigValidForJointPositionUpload( false );
    }

    legacy_rig_flags |= getLegacyRigFlags();
    // This starts as 0, changes if any loaded mesh has issues
    setLegacyRigFlags(legacy_rig_flags);

}

//-----------------------------------------------------------------------------
// determineRigLegacyFlags()
//-----------------------------------------------------------------------------
U32 LLModelLoader::determineRigLegacyFlags( const std::vector<std::string> &jointListFromAsset )
{
    //No joints in asset
    if ( jointListFromAsset.size() == 0 )
    {
        return false;
    }

    // Too many joints in asset
    if (jointListFromAsset.size()>mMaxJointsPerMesh)
    {
        LL_WARNS() << "Rigged to " << jointListFromAsset.size() << " joints, max is " << mMaxJointsPerMesh << LL_ENDL;
        LL_WARNS() << "Skinning disabled due to too many joints" << LL_ENDL;
        LLSD args;
        args["Message"] = "TooManyJoint";
        args["[JOINTS]"] = LLSD::Integer(jointListFromAsset.size());
        args["[MAX]"] = LLSD::Integer(mMaxJointsPerMesh);
        mWarningsArray.append(args);
        return LEGACY_RIG_FLAG_TOO_MANY_JOINTS;
    }

    // Unknown joints in asset
    S32 unknown_joint_count = 0;
    for (std::vector<std::string>::const_iterator it = jointListFromAsset.begin();
         it != jointListFromAsset.end(); ++it)
    {
        if (mJointMap.find(*it)==mJointMap.end())
        {
            LL_WARNS() << "Rigged to unrecognized joint name " << *it << LL_ENDL;
            LLSD args;
            args["Message"] = "UnrecognizedJoint";
            args["[NAME]"] = *it;
            mWarningsArray.append(args);
            unknown_joint_count++;
        }
    }
    if (unknown_joint_count>0)
    {
        LL_WARNS() << "Skinning disabled due to unknown joints" << LL_ENDL;
        LLSD args;
        args["Message"] = "UnknownJoints";
        args["[COUNT]"] = LLSD::Integer(unknown_joint_count);
        mWarningsArray.append(args);
        return LEGACY_RIG_FLAG_UNKNOWN_JOINT;
    }

    return LEGACY_RIG_OK;
}
//-----------------------------------------------------------------------------
// isRigSuitableForJointPositionUpload()
//-----------------------------------------------------------------------------
bool LLModelLoader::isRigSuitableForJointPositionUpload( const std::vector<std::string> &jointListFromAsset )
{
    return true;
}

void LLModelLoader::dumpDebugData()
{
    if (mDebugMode == 0)
    {
        return;
    }

    std::string log_file = mFilename + "_importer.txt";
    LLStringUtil::toLower(log_file);
    llofstream file;
    file.open(log_file.c_str());
    if (!file)
    {
        LL_WARNS() << "dumpDebugData failed to open file " << log_file << LL_ENDL;
        return;
    }
    file << "Importing: " << mFilename << "\n";

    std::map<std::string, LLMatrix4a> inv_bind;
    std::map<std::string, LLMatrix4a> alt_bind;
    for (LLPointer<LLModel>& mdl : mModelList)
    {

        file << "Model name: " << mdl->mLabel << "\n";
        const LLMeshSkinInfo& skin_info = mdl->mSkinInfo;
        file << "Shape Bind matrix: " << skin_info.mBindShapeMatrix << "\n";
        file << "Skin Weights count: " << (S32)mdl->mSkinWeights.size() << "\n";

        // some objects might have individual bind matrices,
        // but for now it isn't accounted for
        size_t joint_count = skin_info.mJointNames.size();
        for (size_t i = 0; i< joint_count;i++)
        {
            const std::string& joint = skin_info.mJointNames[i];
            if (skin_info.mInvBindMatrix.size() > i)
            {
                inv_bind[joint] = skin_info.mInvBindMatrix[i];
            }
            if (skin_info.mAlternateBindMatrix.size() > i)
            {
                alt_bind[joint] = skin_info.mAlternateBindMatrix[i];
            }
        }
    }

    file << "\nInv Bind matrices.\n";
    for (auto& bind : inv_bind)
    {
        file << "Joint: " << bind.first << " Matrix: " << bind.second << "\n";
    }

    file << "\nAlt Bind matrices.\n";
    for (auto& bind : alt_bind)
    {
        file << "Joint: " << bind.first << " Matrix: " << bind.second << "\n";
    }

    if (mDebugMode == 2)
    {
        S32 model_count = 0;
        for (LLPointer<LLModel>& mdl : mModelList)
        {
            const LLVolume::face_list_t &face_list = mdl->getVolumeFaces();
            for (S32 face = 0; face < face_list.size(); face++)
            {
                const LLVolumeFace& vf = face_list[face];
                file << "\nModel: " << mdl->mLabel
                    << " face " << face
                    << " has " << vf.mNumVertices
                    << " vertices and " << vf.mNumIndices
                    << " indices " << "\n";

                file << "\nPositions for model: " << mdl->mLabel << " face " << face << "\n";

                for (S32 pos = 0; pos < vf.mNumVertices; ++pos)
                {
                    file << vf.mPositions[pos] << " ";
                }

                file << "\n\nIndices for model: " << mdl->mLabel << " face " << face << "\n";

                for (S32 ind = 0; ind < vf.mNumIndices; ++ind)
                {
                    file << vf.mIndices[ind] << " ";
                }
            }

            file << "\n\nWeights for model: " << mdl->mLabel;
            for (auto& weights : mdl->mSkinWeights)
            {
                file << "\nVertex: " << weights.first << " Weights: ";
                for (auto& weight : weights.second)
                {
                    file << weight.mJointIdx << ":" << weight.mWeight << " ";
                }
            }

            file << "\n";
            model_count++;
            if (model_count == 5)
            {
                file << "Too many models, stopping at 5.\n";
                break;
            }
        }
    }
    else if (mDebugMode > 2)
    {
        file << "\nModel LLSDs\n";
        S32 model_count = 0;
        // some files contain too many models, so stop at 5.
        for (LLPointer<LLModel>& mdl : mModelList)
        {
            const LLMeshSkinInfo& skin_info = mdl->mSkinInfo;
            size_t joint_count = skin_info.mJointNames.size();
            size_t alt_count = skin_info.mAlternateBindMatrix.size();

            LLModel::writeModel(
                file,
                nullptr,
                mdl,
                nullptr,
                nullptr,
                nullptr,
                mdl->mPhysics,
                joint_count > 0,
                alt_count > 0,
                false,
                LLModel::WRITE_HUMAN,
                false,
                mdl->mSubmodelID);

            file << "\n";
            model_count++;
            if (model_count == 5)
            {
                file << "Too many models, stopping at 5.\n";
                break;
            }
        }
    }
}

//called in the main thread
void LLModelLoader::loadTextures()
{
    bool is_paused = isPaused() ;
    pause() ; //pause the loader

    for(scene::iterator iter = mScene.begin(); iter != mScene.end(); ++iter)
    {
        for(U32 i = 0 ; i < iter->second.size(); i++)
        {
            for(std::map<std::string, LLImportMaterial>::iterator j = iter->second[i].mMaterial.begin();
                j != iter->second[i].mMaterial.end(); ++j)
            {
                LLImportMaterial& material = j->second;

                if(!material.mDiffuseMapFilename.empty())
                {
                    mNumOfFetchingTextures += mTextureLoadFunc(material, mOpaqueData);
                }
            }
        }
    }

    if(!is_paused)
    {
        unpause() ;
    }
}
