/** 
 * @file llpolyskeletaldistortion.h
 * @brief Implementation of LLPolyMesh class
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

#ifndef LL_LLPOLYSKELETALDISTORTION_H
#define LL_LLPOLYSKELETALDISTORTION_H

#include "llcommon.h"

#include <string>
#include <map>
#include "llstl.h"

#include "v3math.h"
#include "v2math.h"
#include "llquaternion.h"
//#include "llpolymorph.h"
#include "lljoint.h"
#include "llviewervisualparam.h"
//#include "lldarray.h"

//class LLSkinJoint;
class LLAvatarAppearance;

//#define USE_STRIPS	// Use tri-strips for rendering.

//-----------------------------------------------------------------------------
// LLPolySkeletalDeformationInfo
// Shared information for LLPolySkeletalDeformations
//-----------------------------------------------------------------------------
struct LLPolySkeletalBoneInfo
{
	LLPolySkeletalBoneInfo(std::string &name, LLVector3 &scale, LLVector3 &pos, BOOL haspos)
		: mBoneName(name),
		  mScaleDeformation(scale),
		  mPositionDeformation(pos),
		  mHasPositionDeformation(haspos) {}
	std::string mBoneName;
	LLVector3 mScaleDeformation;
	LLVector3 mPositionDeformation;
	BOOL mHasPositionDeformation;
};

class LLPolySkeletalDistortionInfo : public LLViewerVisualParamInfo
{
	friend class LLPolySkeletalDistortion;
public:
	
	LLPolySkeletalDistortionInfo();
	/*virtual*/ ~LLPolySkeletalDistortionInfo() {};
	
	/*virtual*/ BOOL parseXml(LLXmlTreeNode* node);

protected:
	typedef std::vector<LLPolySkeletalBoneInfo> bone_info_list_t;
	bone_info_list_t mBoneInfoList;
};

//-----------------------------------------------------------------------------
// LLPolySkeletalDeformation
// A set of joint scale data for deforming the avatar mesh
//-----------------------------------------------------------------------------
class LLPolySkeletalDistortion : public LLViewerVisualParam
{
public:
	void* operator new(size_t size)
	{
		return ll_aligned_malloc_16(size);
	}

	void operator delete(void* ptr)
	{
		ll_aligned_free_16(ptr);
	}

	LLPolySkeletalDistortion(LLAvatarAppearance *avatarp);
	~LLPolySkeletalDistortion();

	// Special: These functions are overridden by child classes
	LLPolySkeletalDistortionInfo*	getInfo() const { return (LLPolySkeletalDistortionInfo*)mInfo; }
	//   This sets mInfo and calls initialization functions
	BOOL							setInfo(LLPolySkeletalDistortionInfo *info);

	/*virtual*/ LLViewerVisualParam* cloneParam(LLWearable* wearable) const;

	// LLVisualParam Virtual functions
	///*virtual*/ BOOL				parseData(LLXmlTreeNode* node);
	/*virtual*/ void				apply( ESex sex );
	
	// LLViewerVisualParam Virtual functions
	/*virtual*/ F32					getTotalDistortion() { return 0.1f; }
	/*virtual*/ const LLVector4a&	getAvgDistortion()	{ return mDefaultVec; }
	/*virtual*/ F32					getMaxDistortion() { return 0.1f; }
	/*virtual*/ LLVector4a			getVertexDistortion(S32 index, LLPolyMesh *poly_mesh){return LLVector4a(0.001f, 0.001f, 0.001f);}
	/*virtual*/ const LLVector4a*	getFirstDistortion(U32 *index, LLPolyMesh **poly_mesh){index = 0; poly_mesh = NULL; return &mDefaultVec;};
	/*virtual*/ const LLVector4a*	getNextDistortion(U32 *index, LLPolyMesh **poly_mesh){index = 0; poly_mesh = NULL; return NULL;};

protected:
	typedef std::map<LLJoint*, LLVector3> joint_vec_map_t;
	joint_vec_map_t mJointScales;
	joint_vec_map_t mJointOffsets;
	LLVector4a	mDefaultVec;
	// Backlink only; don't make this an LLPointer.
	LLAvatarAppearance *mAvatar;
};

#endif // LL_LLPOLYSKELETALDISTORTION_H

