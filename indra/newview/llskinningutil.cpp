/**
* @file llskinningutil.cpp
* @brief  Functions for mesh object skinning
* @author vir@lindenlab.com
*
* $LicenseInfo:firstyear=2015&license=viewerlgpl$
* Second Life Viewer Source Code
* Copyright (C) 2015, Linden Research, Inc.
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

#include "llskinningutil.h"
#include "llvoavatar.h"
#include "llviewercontrol.h"
#include "llmeshrepository.h"
#include "llvolume.h"
#include "llrigginginfo.h"

#define DEBUG_SKINNING  LL_DEBUG

void dump_avatar_and_skin_state(const std::string& reason, LLVOAvatar *avatar, const LLMeshSkinInfo *skin)
{
#if DEBUG_SKINNING
    static S32 dump_count = 0;
    const S32 max_dump = 10;

    if (dump_count < max_dump)
    {
        LL_WARNS("Avatar") << avatar->getFullname() << " dumping, reason " << reason
                           << " avatar build state: isBuilt() " << avatar->isBuilt()
                           << " mInitFlags " << avatar->mInitFlags << LL_ENDL;
        LL_WARNS("Avatar") << "Skin num joints " << skin->mJointNames.size() << " " << skin->mJointNums.size() << LL_ENDL;
        LL_WARNS("Avatar") << "Skin scrubbed " << skin->mInvalidJointsScrubbed
                           << " nums init " << skin->mJointNumsInitialized << LL_ENDL;
        for (S32 j=0; j<skin->mJointNames.size(); j++)
        {
            LL_WARNS("Avatar") << "skin joint idx " << j << " name [" << skin->mJointNames[j]
                               << "] num " << skin->mJointNums[j] << LL_ENDL;
            const std::string& name = skin->mJointNames[j];
            S32 joint_num = skin->mJointNums[j];

            LLJoint *name_joint = avatar->getJoint(name);
            LLJoint *num_joint = avatar->getJoint(joint_num);
            if (!name_joint)
            {
                LL_WARNS("Avatar") << "failed to find joint by name" << LL_ENDL;
            }
            if (!num_joint)
            {
                LL_WARNS("Avatar") << "failed to find joint by num" << LL_ENDL;
            }
            if (num_joint != name_joint)
            {
                LL_WARNS("Avatar") << "joint pointers don't match" << LL_ENDL;
            }
            if (num_joint && num_joint->getJointNum() != joint_num)
            {
                LL_WARNS("Avatar") << "joint found by num has wrong num " << joint_num << "!=" << num_joint->getJointNum() << LL_ENDL;
            }
            if (name_joint && name_joint->getJointNum() != joint_num)
            {
                LL_WARNS("Avatar") << "joint found by name has wrong num " << joint_num << "!=" << name_joint->getJointNum() << LL_ENDL;
            }
        }
        LL_WARNS("Avatar") << LL_ENDL;

        dump_count++;
    }
#endif
}

S32 LLSkinningUtil::getMaxJointCount()
{
    return (S32)LL_MAX_JOINTS_PER_MESH_OBJECT;
}

U32 LLSkinningUtil::getMeshJointCount(const LLMeshSkinInfo *skin)
{
    return llmin((U32)getMaxJointCount(), (U32)skin->mJointNames.size());
}

S32 LLSkinningUtil::getMaxGLTFJointCount()
{
    // this is the maximum number of 3x4 matrices than can fit in a UBO
    return gGLManager.mMaxUniformBlockSize / 48;
}

void LLSkinningUtil::scrubInvalidJoints(LLVOAvatar *avatar, LLMeshSkinInfo* skin)
{
    if (skin->mInvalidJointsScrubbed)
    {
        return;
    }
    for (U32 j = 0; j < skin->mJointNames.size(); ++j)
    {
        // Fix invalid names to "mPelvis". Currently meshes with
        // invalid names will be blocked on upload, so this is just
        // needed for handling of any legacy bad data.
        if (!avatar->getJoint(skin->mJointNames[j]))
        {
            LL_DEBUGS("Avatar") << avatar->getFullname() << " mesh rigged to invalid joint " << skin->mJointNames[j] << LL_ENDL;
            LL_WARNS_ONCE("Avatar") << avatar->getFullname() << " mesh rigged to invalid joint" << skin->mJointNames[j] << LL_ENDL;
            skin->mJointNames[j] = "mPelvis";
            skin->mJointNumsInitialized = false; // force update after names change.
        }
    }
    skin->mInvalidJointsScrubbed = true;
}

void LLSkinningUtil::initSkinningMatrixPalette(
    LLMatrix4a* mat,
    S32 count,
    const LLMeshSkinInfo* skin,
    LLVOAvatar *avatar)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_AVATAR;

    initJointNums(const_cast<LLMeshSkinInfo*>(skin), avatar);

    LLMatrix4a world[LL_CHARACTER_MAX_ANIMATED_JOINTS];

    for (S32 j = 0; j < count; ++j)
    {
        S32 joint_num = skin->mJointNums[j];
        LLJoint *joint = avatar->getJoint(joint_num);

        if (joint)
        {
            world[j] = joint->getWorldMatrix4a();
        }
        else
        {
            mat[j] = skin->mInvBindMatrix[j];
#if DEBUG_SKINNING
            // This  shouldn't  happen   -  in  mesh  upload,  skinned
            // rendering  should  be disabled  unless  all joints  are
            // valid.  In other  cases of  skinned  rendering, invalid
            // joints should already have  been removed during scrubInvalidJoints().
            LL_WARNS_ONCE("Avatar") << avatar->getFullname()
                << " rigged to invalid joint name " << skin->mJointNames[j]
                << " num " << skin->mJointNums[j] << LL_ENDL;
            LL_WARNS_ONCE("Avatar") << avatar->getFullname()
                << " avatar build state: isBuilt() " << avatar->isBuilt()
                << " mInitFlags " << avatar->mInitFlags << LL_ENDL;
#endif
            dump_avatar_and_skin_state("initSkinningMatrixPalette joint not found", avatar, skin);
        }
    }

    //NOTE: pointer striders used here as a micro-optimization over vector/array lookups
    const LLMatrix4a* invBind = &(skin->mInvBindMatrix[0]);
    const LLMatrix4a* w = world;
    LLMatrix4a* m = mat;
    LLMatrix4a* end = m + count;

    while (m < end)
    {
        matMulUnsafe(*(invBind++), *(w++), *(m++));
    }
}

void LLSkinningUtil::checkSkinWeights(LLVector4a* weights, U32 num_vertices, const LLMeshSkinInfo* skin)
{
#if DEBUG_SKINNING
    const S32 max_joints = skin->mJointNames.size();
    for (U32 j=0; j<num_vertices; j++)
    {
        F32 *w = weights[j].getF32ptr();

        F32 wsum = 0.0;
        for (U32 k=0; k<4; ++k)
        {
            S32 i = llfloor(w[k]);
            llassert(i>=0);
            llassert(i<max_joints);
            wsum += w[k]-i;
        }
        llassert(wsum > 0.0f);
    }
#endif
}

void LLSkinningUtil::scrubSkinWeights(LLVector4a* weights, U32 num_vertices, const LLMeshSkinInfo* skin)
{
    const S32 max_joints = static_cast<S32>(skin->mJointNames.size());
    for (U32 j=0; j<num_vertices; j++)
    {
        F32 *w = weights[j].getF32ptr();

        for (U32 k=0; k<4; ++k)
        {
            S32 i = llfloor(w[k]);
            F32 f = w[k]-i;
            i = llclamp(i,0,max_joints-1);
            w[k] = i + f;
        }
    }
    checkSkinWeights(weights, num_vertices, skin);
}

void LLSkinningUtil::getPerVertexSkinMatrix(
    F32* weights,
    const LLMatrix4a* mat,
    bool handle_bad_scale,
    LLMatrix4a& final_mat,
    U32 max_joints)
{
    bool valid_weights = true;
    final_mat.clear();

    S32 idx[4];

    LLVector4 wght;

    F32 scale = 0.f;
    for (U32 k = 0; k < 4; k++)
    {
        F32 w = weights[k];

        // BENTO potential optimizations
        // - Do clamping in unpackVolumeFaces() (once instead of every time)
        // - int vs floor: if we know w is
        // >= 0.0, we can use int instead of floorf; the latter
        // allegedly has a lot of overhead due to ieeefp error
        // checking which we should not need.
        idx[k] = llclamp((S32) floorf(w), (S32)0, (S32)max_joints-1);

        wght[k] = w - floorf(w);
        scale += wght[k];
    }
    if (handle_bad_scale && scale <= 0.f)
    {
        wght = LLVector4(1.0f, 0.0f, 0.0f, 0.0f);
        valid_weights = false;
    }
    else
    {
        // This is enforced  in unpackVolumeFaces()
        llassert(scale>0.f);
        wght *= 1.f/scale;
    }

    for (U32 k = 0; k < 4; k++)
    {
        F32 w = wght[k];

        LLMatrix4a src;
        src.setMul(mat[idx[k]], w);

        final_mat.add(src);
    }
    // SL-366 - with weight validation/cleanup code, it should no longer be
    // possible to hit the bad scale case.
    llassert(valid_weights);
    // When building for Release, the above llassert() goes away. Ward off
    // variable-set-but-unused error.
    (void)valid_weights;
}

void LLSkinningUtil::initJointNums(LLMeshSkinInfo* skin, LLVOAvatar *avatar)
{
    if (!skin->mJointNumsInitialized)
    {
        LL_PROFILE_ZONE_SCOPED_CATEGORY_AVATAR;
        for (U32 j = 0; j < skin->mJointNames.size(); ++j)
        {
    #if DEBUG_SKINNING
            LLJoint *joint = NULL;
            if (skin->mJointNums[j] == -1)
            {
                joint = avatar->getJoint(skin->mJointNames[j]);
                if (joint)
                {
                    skin->mJointNums[j] = joint->getJointNum();
                    if (skin->mJointNums[j] < 0)
                    {
                        LL_WARNS_ONCE("Avatar") << avatar->getFullname() << " joint has unusual number " << skin->mJointNames[j] << ": " << skin->mJointNums[j] << LL_ENDL;
                        LL_WARNS_ONCE("Avatar") << avatar->getFullname() << " avatar build state: isBuilt() " << avatar->isBuilt() << " mInitFlags " << avatar->mInitFlags << LL_ENDL;
                    }
                }
                else
                {
                    LL_WARNS_ONCE("Avatar") << avatar->getFullname() << " unable to find joint " << skin->mJointNames[j] << LL_ENDL;
                    LL_WARNS_ONCE("Avatar") << avatar->getFullname() << " avatar build state: isBuilt() " << avatar->isBuilt() << " mInitFlags " << avatar->mInitFlags << LL_ENDL;
                    dump_avatar_and_skin_state("initJointNums joint not found", avatar, skin);
                    skin->mJointNums[j] = 0;
                }
            }
    #else
            LLJoint *joint = (skin->mJointNums[j] == -1) ? avatar->getJoint(skin->mJointNames[j]) : avatar->getJoint(skin->mJointNums[j]);
            skin->mJointNums[j] = joint ? joint->getJointNum() : 0;
    #endif
            // insure we have *a* valid joint to reference
            llassert(skin->mJointNums[j] >= 0);
        }
        skin->mJointNumsInitialized = true;
    }
}

void LLSkinningUtil::updateRiggingInfo(const LLMeshSkinInfo* skin, LLVOAvatar *avatar, LLVolumeFace& vol_face)
{
    if (vol_face.mJointRiggingInfoTab.needsUpdate())
    {
        S32 num_verts = vol_face.mNumVertices;
        S32 num_joints = static_cast<S32>(skin->mJointNames.size());
        if (num_verts > 0 && vol_face.mWeights && num_joints > 0)
        {
            LL_PROFILE_ZONE_SCOPED_CATEGORY_AVATAR;
            initJointNums(const_cast<LLMeshSkinInfo*>(skin), avatar);
            if (vol_face.mJointRiggingInfoTab.size()==0)
            {
                vol_face.mJointRiggingInfoTab.resize(LL_CHARACTER_MAX_ANIMATED_JOINTS);
                LLJointRiggingInfoTab &rig_info_tab = vol_face.mJointRiggingInfoTab;
                for (S32 i=0; i<vol_face.mNumVertices; i++)
                {
                    LLVector4a& pos = vol_face.mPositions[i];
                    F32 *weights = vol_face.mWeights[i].getF32ptr();
                    LLVector4 wght;
                    S32 idx[4];
                    F32 scale = 0.0f;
                    // FIXME unpacking of weights should be pulled into a common function and optimized if possible.
                    for (U32 k = 0; k < 4; k++)
                    {
                        F32 w = weights[k];
                        idx[k] = llclamp((S32) floorf(w), (S32)0, (S32)LL_CHARACTER_MAX_ANIMATED_JOINTS-1);
                        wght[k] = w - idx[k];
                    }

                    for (U32 k=0; k<4; ++k)
                    {
                        S32 joint_index = idx[k];
                        if (wght[k] > 0.2f && num_joints > joint_index)
                        {
                            S32 joint_num = skin->mJointNums[joint_index];
                            if (joint_num >= 0 && joint_num < LL_CHARACTER_MAX_ANIMATED_JOINTS)
                            {
                                rig_info_tab[joint_num].setIsRiggedTo(true);

                                const LLMatrix4a& mat = skin->mBindPoseMatrix[joint_index];
                                LLVector4a pos_joint_space;

                                mat.affineTransform(pos, pos_joint_space);

                                LLVector4a *extents = rig_info_tab[joint_num].getRiggedExtents();
                                update_min_max(extents[0], extents[1], pos_joint_space);
                            }
                        }
                    }
                }
                vol_face.mJointRiggingInfoTab.setNeedsUpdate(false);
            }
        }
    }
}

// This is used for extracting rotation from a bind shape matrix that
// already has scales baked in
LLQuaternion LLSkinningUtil::getUnscaledQuaternion(const LLMatrix4& mat4)
{
    LLMatrix3 bind_mat = mat4.getMat3();
    for (auto i = 0; i < 3; i++)
    {
        F32 len = 0.0f;
        for (auto j = 0; j < 3; j++)
        {
            len += bind_mat.mMatrix[i][j] * bind_mat.mMatrix[i][j];
        }
        if (len > 0.0f)
        {
            len = sqrt(len);
            for (auto j = 0; j < 3; j++)
            {
                bind_mat.mMatrix[i][j] /= len;
            }
        }
    }
    bind_mat.invert();
    LLQuaternion bind_rot = bind_mat.quaternion();
    bind_rot.normalize();
    return bind_rot;
}

