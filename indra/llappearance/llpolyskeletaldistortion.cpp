/** 
 * @file llpolyskeletaldistortion.cpp
 * @brief Implementation of LLPolySkeletalDistortion classes
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

//-----------------------------------------------------------------------------
// Header Files
//-----------------------------------------------------------------------------
#include "llpreprocessor.h"
#include "llerrorlegacy.h"
//#include "llcommon.h"
//#include "llmemory.h"
#include "llavatarappearance.h"
#include "llavatarjoint.h"
#include "llpolymorph.h"
//#include "llviewercontrol.h"
//#include "llxmltree.h"
//#include "llvoavatar.h"
#include "llwearable.h"
//#include "lldir.h"
//#include "llvolume.h"
//#include "llendianswizzle.h"

#include "llpolyskeletaldistortion.h"

//-----------------------------------------------------------------------------
// LLPolySkeletalDistortionInfo()
//-----------------------------------------------------------------------------
LLPolySkeletalDistortionInfo::LLPolySkeletalDistortionInfo()
{
}

BOOL LLPolySkeletalDistortionInfo::parseXml(LLXmlTreeNode* node)
{
        llassert( node->hasName( "param" ) && node->getChildByName( "param_skeleton" ) );
        
        if (!LLViewerVisualParamInfo::parseXml(node))
                return FALSE;

        LLXmlTreeNode* skeletalParam = node->getChildByName("param_skeleton");

        if (NULL == skeletalParam)
        {
                llwarns << "Failed to getChildByName(\"param_skeleton\")"
                        << llendl;
                return FALSE;
        }

        for( LLXmlTreeNode* bone = skeletalParam->getFirstChild(); bone; bone = skeletalParam->getNextChild() )
        {
                if (bone->hasName("bone"))
                {
                        std::string name;
                        LLVector3 scale;
                        LLVector3 pos;
                        BOOL haspos = FALSE;
                        
                        static LLStdStringHandle name_string = LLXmlTree::addAttributeString("name");
                        if (!bone->getFastAttributeString(name_string, name))
                        {
                                llwarns << "No bone name specified for skeletal param." << llendl;
                                continue;
                        }

                        static LLStdStringHandle scale_string = LLXmlTree::addAttributeString("scale");
                        if (!bone->getFastAttributeVector3(scale_string, scale))
                        {
                                llwarns << "No scale specified for bone " << name << "." << llendl;
                                continue;
                        }

                        // optional offset deformation (translation)
                        static LLStdStringHandle offset_string = LLXmlTree::addAttributeString("offset");
                        if (bone->getFastAttributeVector3(offset_string, pos))
                        {
                                haspos = TRUE;
                        }
                        mBoneInfoList.push_back(LLPolySkeletalBoneInfo(name, scale, pos, haspos));
                }
                else
                {
                        llwarns << "Unrecognized element " << bone->getName() << " in skeletal distortion" << llendl;
                        continue;
                }
        }
        return TRUE;
}

//-----------------------------------------------------------------------------
// LLPolySkeletalDistortion()
//-----------------------------------------------------------------------------
LLPolySkeletalDistortion::LLPolySkeletalDistortion(LLAvatarAppearance *avatarp)
{
        mAvatar = avatarp;
        mDefaultVec.splat(0.001f);
}

//-----------------------------------------------------------------------------
// ~LLPolySkeletalDistortion()
//-----------------------------------------------------------------------------
LLPolySkeletalDistortion::~LLPolySkeletalDistortion()
{
}

BOOL LLPolySkeletalDistortion::setInfo(LLPolySkeletalDistortionInfo *info)
{
        llassert(mInfo == NULL);
        if (info->mID < 0)
                return FALSE;
        mInfo = info;
        mID = info->mID;
        setWeight(getDefaultWeight(), FALSE );

        LLPolySkeletalDistortionInfo::bone_info_list_t::iterator iter;
        for (iter = getInfo()->mBoneInfoList.begin(); iter != getInfo()->mBoneInfoList.end(); iter++)
        {
                LLPolySkeletalBoneInfo *bone_info = &(*iter);
                LLJoint* joint = mAvatar->getJoint(bone_info->mBoneName);
                if (!joint)
                {
                        llwarns << "Joint " << bone_info->mBoneName << " not found." << llendl;
                        continue;
                }

                if (mJointScales.find(joint) != mJointScales.end())
                {
                        llwarns << "Scale deformation already supplied for joint " << joint->getName() << "." << llendl;
                }

                // store it
                mJointScales[joint] = bone_info->mScaleDeformation;

                // apply to children that need to inherit it
                for (LLJoint::child_list_t::iterator iter = joint->mChildren.begin();
                     iter != joint->mChildren.end(); ++iter)
                {
                        LLAvatarJoint* child_joint = (LLAvatarJoint*)(*iter);
                        if (child_joint->inheritScale())
                        {
                                LLVector3 childDeformation = LLVector3(child_joint->getScale());
                                childDeformation.scaleVec(bone_info->mScaleDeformation);
                                mJointScales[child_joint] = childDeformation;
                        }
                }

                if (bone_info->mHasPositionDeformation)
                {
                        if (mJointOffsets.find(joint) != mJointOffsets.end())
                        {
                                llwarns << "Offset deformation already supplied for joint " << joint->getName() << "." << llendl;
                        }
                        mJointOffsets[joint] = bone_info->mPositionDeformation;
                }
        }
        return TRUE;
}

/*virtual*/ LLViewerVisualParam* LLPolySkeletalDistortion::cloneParam(LLWearable* wearable) const
{
        LLPolySkeletalDistortion *new_param = new LLPolySkeletalDistortion(mAvatar);
        *new_param = *this;
        return new_param;
}

//-----------------------------------------------------------------------------
// apply()
//-----------------------------------------------------------------------------
static LLFastTimer::DeclareTimer FTM_POLYSKELETAL_DISTORTION_APPLY("Skeletal Distortion");

void LLPolySkeletalDistortion::apply( ESex avatar_sex )
{
	LLFastTimer t(FTM_POLYSKELETAL_DISTORTION_APPLY);

        F32 effective_weight = ( getSex() & avatar_sex ) ? mCurWeight : getDefaultWeight();

        LLJoint* joint;
        joint_vec_map_t::iterator iter;

        for (iter = mJointScales.begin();
             iter != mJointScales.end();
             iter++)
        {
                joint = iter->first;
                LLVector3 newScale = joint->getScale();
                LLVector3 scaleDelta = iter->second;
                newScale = newScale + (effective_weight * scaleDelta) - (mLastWeight * scaleDelta);
                joint->setScale(newScale);
        }

        for (iter = mJointOffsets.begin();
             iter != mJointOffsets.end();
             iter++)
        {
                joint = iter->first;
                LLVector3 newPosition = joint->getPosition();
                LLVector3 positionDelta = iter->second;
                newPosition = newPosition + (effective_weight * positionDelta) - (mLastWeight * positionDelta);
                joint->setPosition(newPosition);
        }

        if (mLastWeight != mCurWeight && !mIsAnimating)
        {
                mAvatar->setSkeletonSerialNum(mAvatar->getSkeletonSerialNum() + 1);
        }
        mLastWeight = mCurWeight;
}


LLPolyMorphData *clone_morph_param_duplicate(const LLPolyMorphData *src_data,
					     const std::string &name)
{
        LLPolyMorphData* cloned_morph_data = new LLPolyMorphData(*src_data);
        cloned_morph_data->mName = name;
        for (U32 v=0; v < cloned_morph_data->mNumIndices; v++)
        {
                cloned_morph_data->mCoords[v] = src_data->mCoords[v];
                cloned_morph_data->mNormals[v] = src_data->mNormals[v];
                cloned_morph_data->mBinormals[v] = src_data->mBinormals[v];
        }
        return cloned_morph_data;
}

LLPolyMorphData *clone_morph_param_direction(const LLPolyMorphData *src_data,
					     const LLVector3 &direction,
					     const std::string &name)
{
        LLPolyMorphData* cloned_morph_data = new LLPolyMorphData(*src_data);
        cloned_morph_data->mName = name;
		LLVector4a dir;
		dir.load3(direction.mV);

        for (U32 v=0; v < cloned_morph_data->mNumIndices; v++)
        {
                cloned_morph_data->mCoords[v] = dir;
                cloned_morph_data->mNormals[v].clear();
                cloned_morph_data->mBinormals[v].clear();
        }
        return cloned_morph_data;
}

LLPolyMorphData *clone_morph_param_cleavage(const LLPolyMorphData *src_data,
                                            F32 scale,
                                            const std::string &name)
{
        LLPolyMorphData* cloned_morph_data = new LLPolyMorphData(*src_data);
        cloned_morph_data->mName = name;

		LLVector4a sc;
		sc.splat(scale);

		LLVector4a nsc;
		nsc.set(scale, -scale, scale, scale);

        for (U32 v=0; v < cloned_morph_data->mNumIndices; v++)
        {
            if (cloned_morph_data->mCoords[v][1] < 0)
            {
                cloned_morph_data->mCoords[v].setMul(src_data->mCoords[v],nsc);
				cloned_morph_data->mNormals[v].setMul(src_data->mNormals[v],nsc);
				cloned_morph_data->mBinormals[v].setMul(src_data->mBinormals[v],nsc);
			}
			else
			{
				cloned_morph_data->mCoords[v].setMul(src_data->mCoords[v],sc);
				cloned_morph_data->mNormals[v].setMul(src_data->mNormals[v], sc);
				cloned_morph_data->mBinormals[v].setMul(src_data->mBinormals[v],sc);
			}
        }
        return cloned_morph_data;
}

// End
