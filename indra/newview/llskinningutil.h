/** 
* @file   llskinningutil.h
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
#ifndef LLSKINNINGUTIL_H
#define LLSKINNINGUTIL_H

#include "v2math.h"
#include "v4math.h"
#include "llvector4a.h"
#include "llmatrix4a.h"

class LLVOAvatar;
class LLMeshSkinInfo;
class LLVolumeFace;
class LLJointRiggingInfoTab;

namespace LLSkinningUtil
{
    S32 getMaxJointCount();
    U32 getMeshJointCount(const LLMeshSkinInfo *skin);
    void scrubInvalidJoints(LLVOAvatar *avatar, LLMeshSkinInfo* skin);
    void initSkinningMatrixPalette(LLMatrix4* mat, S32 count, const LLMeshSkinInfo* skin, LLVOAvatar *avatar);
    void checkSkinWeights(LLVector4a* weights, U32 num_vertices, const LLMeshSkinInfo* skin);
    void scrubSkinWeights(LLVector4a* weights, U32 num_vertices, const LLMeshSkinInfo* skin);
    void getPerVertexSkinMatrix(F32* weights, LLMatrix4a* mat, bool handle_bad_scale, LLMatrix4a& final_mat, U32 max_joints);

    LL_FORCE_INLINE void getPerVertexSkinMatrixWithIndices(
        F32*        weights,
        U8*         idx,
        LLMatrix4a* mat,
        LLMatrix4a& final_mat,
        LLMatrix4a* src)
    {    
        final_mat.clear();
        src[0].setMul(mat[idx[0]], weights[0]);
        src[1].setMul(mat[idx[1]], weights[1]);
        final_mat.add(src[0]);
        final_mat.add(src[1]);
        src[2].setMul(mat[idx[2]], weights[2]);        
        src[3].setMul(mat[idx[3]], weights[3]);
        final_mat.add(src[2]);
        final_mat.add(src[3]);
    }

    void initJointNums(LLMeshSkinInfo* skin, LLVOAvatar *avatar);
    void updateRiggingInfo(const LLMeshSkinInfo* skin, LLVOAvatar *avatar, LLVolumeFace& vol_face);
    void updateRiggingInfo_(LLMeshSkinInfo* skin, LLVOAvatar *avatar, S32 num_verts, LLVector4a* weights, LLVector4a* positions, U8* joint_indices, LLJointRiggingInfoTab &rig_info_tab);
	LLQuaternion getUnscaledQuaternion(const LLMatrix4& mat4);
};

#endif
