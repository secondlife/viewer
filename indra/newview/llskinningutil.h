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

class LLVOAvatar;
class LLMeshSkinInfo;
class LLMatrix4a;

class LLSkinningUtil
{
public:
    static void initClass();
    static U32 getMaxJointCount();
    static U32 getMeshJointCount(const LLMeshSkinInfo *skin);
    static void scrubInvalidJoints(LLVOAvatar *avatar, LLMeshSkinInfo* skin);
    static void initSkinningMatrixPalette(LLMatrix4* mat, S32 count, const LLMeshSkinInfo* skin, LLVOAvatar *avatar);
    static void checkSkinWeights(LLVector4a* weights, U32 num_vertices, const LLMeshSkinInfo* skin);
    static void getPerVertexSkinMatrix(F32* weights, LLMatrix4a* mat, bool handle_bad_scale, LLMatrix4a& final_mat, U32 max_joints);
};

#endif
