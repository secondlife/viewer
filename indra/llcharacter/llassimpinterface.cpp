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

bool LLAssimpInterface::setMesh(U32 mesh_id)
{
    if (mesh_id > mScene->mNumMeshes)
    {
        return false;
    }
    mMesh = mScene->mMeshes[mesh_id];
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

void LLAssimpInterface::createBoneMap()
{
    aiBone *cur_bone = mMesh->mBones[0];
    for(int i=0;i<mMesh->mNumBones;++i)
    {
        mBoneMap[cur_bone->mName.C_Str()] = cur_bone;
    }
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