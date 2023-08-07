/**
 * @file llbvhloader.cpp
 * @brief Translates a BVH files to LindenLabAnimation format.
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
 * This library is distributed in the hope that it will be useful,7
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

#include "linden_common.h"
#include "llassimpinterface.h"

#include "assimp/DefaultLogger.hpp"
#include "assimp/scene.h"           // Output data structure
#include "assimp/postprocess.h"     // Post processing flags

using namespace std;

aiMatrix4x4 LLAssimpInterface::createIdentityMat4()
{
    aiMatrix4x4 x;
    x.a2 = x.a3 = x.a4 = 0;
    x.b1 = x.b3 = x.b4 = 0;
    x.c1 = x.c2 = x.c4 = 0;
    x.d1 = x.d2 = x.d3 = 0;
    x.a1 = x.b2 = x.c3 = x.d4 = 1;
    return x;
}

bool LLAssimpInterface::setMesh(U32 mesh_id)
{
    if (mesh_id > mScene->mNumMeshes)
    {
        return false;
    }
    mMesh = mScene->mMeshes[mesh_id];

    if (mMesh->mNumBones==0)
    {
        return false;
    }

    //Get the root transformation matrix, we'll need it later.

    mAIRootTransMat4  = mScene->mRootNode->mTransformation.Inverse();  // Counter rotation for arbitrary orientation into correct frame.

    updateBoneMap();

    return true;
}

bool LLAssimpInterface::setAnimation(U32 anim_id)
{
    if (anim_id > mScene->mNumAnimations)
    {
        return false;
    }
    mAnimation = mScene->mAnimations[anim_id];
    return true;
}

void LLAssimpInterface::generateGlobalTransforms(const aiNode *node, const aiMatrix4x4 &mat)
{
    string name = node->mName.C_Str();
    aiMatrix4x4 transmat = mat * node->mTransformation;

    if (name != "")
    {
        //The hierarchy may contain things that are not bones that we'll want for transforms
        //but do not belong in our bone map.  Bone names are allegedly 1:1 with node names.
        auto bone = mBoneMap.find(name);

        if (bone != mBoneMap.end())
        {
            bone->second.mWorldTransform = transmat;
        }
    }
    for (U32 i = 0; i < node->mNumChildren; ++i)
    {
        generateGlobalTransforms(node->mChildren[i], transmat);
    }
}

void LLAssimpInterface::updateBoneMap()
{
    mBoneMap.clear();

    for(int i=0;i<mMesh->mNumBones;++i)
    {
        string  name = mMesh->mBones[i]->mName.C_Str();

        if (name == "")
        {
            LL_WARNS("assimp") << "Bone " << i << " is unnamed." << LL_ENDL;
            continue;
        }

        LLAssimpBoneData data;
        data.mBone = mMesh->mBones[i];
        data.mWorldTransform = createIdentityMat4();

        mBoneMap[name] = data;
    }
    aiMatrix4x4 identity = createIdentityMat4();
    generateGlobalTransforms(mScene->mRootNode, identity);
}

void LLAssimpInterface::copyMat4(LLMatrix4 &lmat, aiMatrix4x4 &aiMat)
{
    // Copy the assimp mat4 to LL mat4
    // Nothing fancy, let's just brute-force copy.
    lmat.mMatrix[0][0] = aiMat.a1;
    lmat.mMatrix[0][1] = aiMat.a2;
    lmat.mMatrix[0][2] = aiMat.a3;
    lmat.mMatrix[0][3] = aiMat.a4;

    lmat.mMatrix[1][0] = aiMat.b1;
    lmat.mMatrix[1][1] = aiMat.b2;
    lmat.mMatrix[1][2] = aiMat.b3;
    lmat.mMatrix[1][3] = aiMat.b4;

    lmat.mMatrix[2][0] = aiMat.c1;
    lmat.mMatrix[2][1] = aiMat.c2;
    lmat.mMatrix[2][2] = aiMat.c3;
    lmat.mMatrix[2][3] = aiMat.c4;

    lmat.mMatrix[3][0] = aiMat.d1;
    lmat.mMatrix[3][1] = aiMat.d2;
    lmat.mMatrix[3][2] = aiMat.d3;
    lmat.mMatrix[3][3] = aiMat.d4;

    LL_INFOS("SPATTERS") << "SPATTERS LL: " << lmat.mMatrix[0][0] << ", " << lmat.mMatrix[0][1] << ", " << lmat.mMatrix[0][2] << ", "
                         << lmat.mMatrix[0][3] << LL_ENDL;
    LL_INFOS("SPATTERS") << "SPATTERS LL: " << lmat.mMatrix[1][0] << ", " << lmat.mMatrix[1][1] << ", " << lmat.mMatrix[1][2] << ", "
                         << lmat.mMatrix[1][3] << LL_ENDL;
    LL_INFOS("SPATTERS") << "SPATTERS LL: " << lmat.mMatrix[2][0] << ", " << lmat.mMatrix[2][1] << ", " << lmat.mMatrix[2][2] << ", "
                         << lmat.mMatrix[2][3] << LL_ENDL;
    LL_INFOS("SPATTERS") << "SPATTERS LL: " << lmat.mMatrix[3][0] << ", " << lmat.mMatrix[3][1] << ", " << lmat.mMatrix[3][2] << ", "
                         << lmat.mMatrix[3][3] << LL_ENDL;
}

LLMatrix4 LLAssimpInterface::getTransMat4( std::string name )
{
    //WARNING this name is a small lie.  Returning the inverse matrix.

    aiNode *cur_trans_node = mScene->mRootNode->FindNode(name.c_str());  // Find the node by unique name in the scene.
    aiMatrix4x4 cur_ai_transmat = cur_trans_node->mTransformation.Inverse();                // Counter rotation from resting pos to T.
    LLMatrix4   cur_transmat;
    copyMat4(cur_transmat, cur_ai_transmat);
    return  cur_transmat;
}

LLMatrix4 LLAssimpInterface::getOffsetMat4(std::string name)
{
    // WARNING this name is a small lie.  Returning the inverse matrix.

    auto cur_bone = mBoneMap.find(name);

    LLMatrix4 cur_transmat;

    if (cur_bone == mBoneMap.end())
    {
        LL_WARNS("assimp") << "Assimp did not have a bone with a name matching " << name << " returning 0 ofset matrix." << LL_ENDL;
    }
    aiMatrix4x4 cur_ai_transmat = cur_bone->second.mBone->mOffsetMatrix;//.Inverse();        // Counter rotation from resting pos to T.

    cur_ai_transmat = cur_ai_transmat * mAIRootTransMat4;
    copyMat4(cur_transmat, cur_ai_transmat);

    return cur_transmat;
}

LLMatrix4 LLAssimpInterface::getExperimental(std::string name)
{
    //Exploratory code, probably not correct.

    auto cur_bone = mBoneMap.find(name);

    LLMatrix4 cur_transmat;

    if (cur_bone == mBoneMap.end())
    {
        LL_WARNS("assimp") << "Assimp did not have a bone with a name matching " << name << " returning 0 ofset matrix." << LL_ENDL;
    }
    aiMatrix4x4 cur_ai_transmat = cur_bone->second.mBone->mOffsetMatrix;


    cur_ai_transmat = cur_ai_transmat * cur_bone->second.mWorldTransform;
    copyMat4(cur_transmat, cur_ai_transmat);

    return cur_transmat;
}