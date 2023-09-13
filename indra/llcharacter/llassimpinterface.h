/**
 * @file llassimpinterface.h
 * @brief An interface between our importer and the assimp library.
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

#ifndef LL_ASSIMPINTERFACE_H
#define LL_ASSIMPINTERFACE_H

#include "v3math.h"
#include "m3math.h"
#include "m4math.h"
#include "llmath.h"
#include <map> 

#include "assimp/Importer.hpp"      // C++ importer interface


struct aiScene;
struct aiNode;
struct aiMesh;
struct aiAnimation;
struct aiBone;

struct LLAssimpBoneData
{
    aiBone *mBone;
    aiMatrix4x4 mWorldTransform;
};
   
typedef std::map<std::string, LLAssimpBoneData> LLaiBoneMap;

class LLAssimpInterface
{
    public:
        LLAssimpInterface():mScene(nullptr),mMesh(nullptr),mAnimation(nullptr){};
        void setScene(aiScene* scene) { mScene = scene; }
        bool setMesh(U32 mesh_id);
        bool setAnimation(U32 anim_id);
        void updateBoneMap();
        void copyMat4(LLMatrix4 &lmat, aiMatrix4x4 &aiMat);
        void generateGlobalTransforms(const aiNode *node, const aiMatrix4x4 &mat);

        aiNode* getSceneRootNode();
        LLMatrix4 getTransMat4(std::string name);
        LLMatrix4 getOffsetMat4(std::string name);
        aiMatrix4x4 createIdentityMat4();

    public:
        const aiScene* mScene;
        aiMesh* mMesh;
        aiAnimation* mAnimation;
        aiMatrix4x4 mAIRootTransMat4;
        LLaiBoneMap mBoneMap;
        LLSD mResting;
};

#endif // LL_ASSIMPINTERFACE_H
