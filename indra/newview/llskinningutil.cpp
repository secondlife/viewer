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

bool LLSkinningUtil::sIncludeEnhancedSkeleton = true;
U32 LLSkinningUtil::sMaxJointsPerMeshObject = LL_MAX_JOINTS_PER_MESH_OBJECT;

namespace {

bool get_name_index(const std::string& name, std::vector<std::string>& names, U32& result)
{
    std::vector<std::string>::const_iterator find_it =
        std::find(names.begin(), names.end(), name);
    if (find_it != names.end())
    {
        result = find_it - names.begin();
        return true;
    }
    else
    {
        return false;
    }
}

// Find a name table index that is also a valid joint on the
// avatar. Order of preference is: requested name, mPelvis, first
// valid match in names table.
U32 get_valid_joint_index(const std::string& name, LLVOAvatar *avatar, std::vector<std::string>& joint_names)
{
    U32 result;
    if (avatar->getJoint(name) && get_name_index(name,joint_names,result))
    {
        return result;
    }
    if (get_name_index("mPelvis",joint_names,result))
    {
        return result;
    }
    for (U32 j=0; j<joint_names.size(); j++)
    {
        if (avatar->getJoint(joint_names[j]))
        {
            return j;
        }
    }
    // BENTO how to handle?
    LL_ERRS() << "no valid joints in joint_names" << LL_ENDL;
    return 0;
}

// Which joint will stand in for this joint? 
U32 get_proxy_joint_index(U32 joint_index, LLVOAvatar *avatar, std::vector<std::string>& joint_names)
{
	bool include_enhanced = LLSkinningUtil::sIncludeEnhancedSkeleton;
    U32 j_proxy = get_valid_joint_index(joint_names[joint_index], avatar, joint_names);
    LLJoint *joint = avatar->getJoint(joint_names[j_proxy]);
    llassert(joint);
    // Find the first ancestor that's not flagged as extended, or the
    // last ancestor that's rigged in this mesh, whichever
    // comes first.
    while (1)
    {
        if (include_enhanced || 
            joint->getSupport()==LLJoint::SUPPORT_BASE)
            break;
        LLJoint *parent = joint->getParent();
        if (!parent)
            break;
        if (!get_name_index(parent->getName(), joint_names, j_proxy))
        {
            break;
        }
        joint = parent;
    }
    return j_proxy;
}

}

// static
void LLSkinningUtil::initClass()
{
    sIncludeEnhancedSkeleton = gSavedSettings.getBOOL("IncludeEnhancedSkeleton");
	// BENTO - remove MaxJointsPerMeshObject before release.
    sMaxJointsPerMeshObject = gSavedSettings.getU32("MaxJointsPerMeshObject");
}

// static
U32 LLSkinningUtil::getMaxJointCount()
{
    U32 result = llmin(LL_MAX_JOINTS_PER_MESH_OBJECT, sMaxJointsPerMeshObject);
    if (!sIncludeEnhancedSkeleton)
    {
        result = llmin(result,(U32)52); // BENTO replace with LLAvatarAppearance::getBaseJointCount()) or equivalent 
    }
	return result;
}

// static
U32 LLSkinningUtil::getMeshJointCount(const LLMeshSkinInfo *skin)
{
	return llmin((U32)getMaxJointCount(), (U32)skin->mJointNames.size());
}

// static

// Destructively remap the joints in skin info based on what joints
// are known in the avatar, and which are currently supported.  This
// will also populate mJointRemap[] in the skin, which can be used to
// make the corresponding changes to the integer part of vertex
// weights.
//
// This will throw away joint info for any joints that are not known
// in the avatar, or not currently flagged to support based on the
// debug setting for IncludeEnhancedSkeleton.
//
// static
void LLSkinningUtil::remapSkinInfoJoints(LLVOAvatar *avatar, LLMeshSkinInfo* skin)
{
	// skip if already done.
    if (!skin->mJointRemap.empty())
    {
        return; 
    }

    U32 max_joints = getMeshJointCount(skin);

    // Compute the remap
    std::vector<U32> j_proxy(skin->mJointNames.size());
    for (U32 j = 0; j < skin->mJointNames.size(); ++j)
    {
        U32 j_rep = get_proxy_joint_index(j, avatar, skin->mJointNames);
        j_proxy[j] = j_rep;
    }
    S32 top = 0;
    std::vector<U32> j_remap(skin->mJointNames.size());
    // Fill in j_remap for all joints that will be kept.
    for (U32 j = 0; j < skin->mJointNames.size(); ++j)
    {
        if (j_proxy[j] == j)
        {
            // Joint will be included
            j_remap[j] = top;
            if (top < max_joints-1)
            {
                top++;
            }

             
        }
    }
    // Then use j_proxy to fill in j_remap for the joints that will be discarded
    for (U32 j = 0; j < skin->mJointNames.size(); ++j)
    {
        if (j_proxy[j] != j)
        {
            j_remap[j] = j_remap[j_proxy[j]];
        }
    }
    
    
    // Apply the remap to mJointNames, mInvBindMatrix, and mAlternateBindMatrix
    std::vector<std::string> new_joint_names;
    std::vector<LLMatrix4> new_inv_bind_matrix;
    std::vector<LLMatrix4> new_alternate_bind_matrix;

    for (U32 j = 0; j < skin->mJointNames.size(); ++j)
    {
        if (j_proxy[j] == j && new_joint_names.size() < max_joints)
        {
            new_joint_names.push_back(skin->mJointNames[j]);
            new_inv_bind_matrix.push_back(skin->mInvBindMatrix[j]);
            if (!skin->mAlternateBindMatrix.empty())
            {
                new_alternate_bind_matrix.push_back(skin->mAlternateBindMatrix[j]);
            }
        }
    }
    llassert(new_joint_names.size() <= max_joints);

    for (U32 j = 0; j < skin->mJointNames.size(); ++j)
    {
        LL_DEBUGS("Avatar") << "Starting joint[" << j << "] = " << skin->mJointNames[j] << " j_remap " << j_remap[j] << " ==> " << new_joint_names[j_remap[j]] << LL_ENDL;
    }

    skin->mJointNames = new_joint_names;
    skin->mInvBindMatrix = new_inv_bind_matrix;
    skin->mAlternateBindMatrix = new_alternate_bind_matrix;
    skin->mJointRemap = j_remap;
}

// static
void LLSkinningUtil::initSkinningMatrixPalette(
    LLMatrix4* mat,
    S32 count, 
    const LLMeshSkinInfo* skin,
    LLVOAvatar *avatar)
{
    // BENTO - switching to use Matrix4a and SSE might speed this up.
    // Note that we are mostly passing Matrix4a's to this routine anyway, just dubiously casted.
    for (U32 j = 0; j < count; ++j)
    {
        LLJoint* joint = avatar->getJoint(skin->mJointNames[j]);
        mat[j] = skin->mInvBindMatrix[j];
        if (joint)
        {
            mat[j] *= joint->getWorldMatrix();
        }
        else
        {
            // This  shouldn't  happen   -  in  mesh  upload,  skinned
            // rendering  should  be disabled  unless  all joints  are
            // valid.  In other  cases of  skinned  rendering, invalid
            // joints should already have  been removed during remap.
            LL_WARNS_ONCE("Avatar") << "Rigged to invalid joint name " << skin->mJointNames[j] << LL_ENDL;
        }
    }
}

// Transform the weights based on the remap info stored in skin. Note
// that this is destructive and non-idempotent, so we need to keep
// track of whether we've done it already. If the desired remapping
// changes, the viewer must be restarted.
//
// static
void LLSkinningUtil::remapSkinWeights(LLVector4a* weights, U32 num_vertices, const LLMeshSkinInfo* skin)
{
    llassert(skin->mJointRemap.size()>0); // Must call remapSkinInfoJoints() first, which this checks for.
    const U32* remap = &skin->mJointRemap[0];
    const S32 max_joints = skin->mJointNames.size();
    for (U32 j=0; j<num_vertices; j++)
    {
        F32 *w = weights[j].getF32ptr();

        for (U32 k=0; k<4; ++k)
        {
            S32 i = llfloor(w[k]);
            F32 f = w[k]-i;
            i = llclamp(i,0,max_joints-1);
            w[k] = remap[i] + f;
        }
    }
}

// static
void LLSkinningUtil::checkSkinWeights(LLVector4a* weights, U32 num_vertices, const LLMeshSkinInfo* skin)
{
#ifndef LL_RELEASE_FOR_DOWNLOAD
	const S32 max_joints = skin->mJointNames.size();
    if (skin->mJointRemap.size()>0)
    {
        // Check the weights are consistent with the current remap.
        for (U32 j=0; j<num_vertices; j++)
        {
            F32 *w = weights[j].getF32ptr();
            
            for (U32 k=0; k<4; ++k)
            {
                S32 i = llfloor(w[k]);
                llassert(i>=0);
                llassert(i<max_joints);
            }
        }
    }
#endif
}

// static
void LLSkinningUtil::getPerVertexSkinMatrix(
    F32* weights,
    LLMatrix4a* mat,
    bool handle_bad_scale,
    LLMatrix4a& final_mat,
    U32 max_joints)
{

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
}

