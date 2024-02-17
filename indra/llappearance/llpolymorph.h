/** 
 * @file llpolymorph.h
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

#ifndef LL_LLPOLYMORPH_H
#define LL_LLPOLYMORPH_H

#include <string>
#include <vector>

#include "llviewervisualparam.h"

class LLAvatarJointCollisionVolume;
class LLPolyMeshSharedData;
class LLVector2;
class LLAvatarJointCollisionVolume;
class LLWearable;

//-----------------------------------------------------------------------------
// LLPolyMorphData()
//-----------------------------------------------------------------------------
class alignas(16) LLPolyMorphData
{
    LL_ALIGN_NEW
public:
	LLPolyMorphData(const std::string& morph_name);
	~LLPolyMorphData();
	LLPolyMorphData(const LLPolyMorphData &rhs);

	bool			loadBinary(LLFILE* fp, LLPolyMeshSharedData *mesh);
	const std::string& getName() { return mName; }

public:
	std::string			mName;

	// morphology
	U32					mNumIndices;
	U32*				mVertexIndices;
	U32					mCurrentIndex;
	LLVector4a*			mCoords;
	LLVector4a*			mNormals;
	LLVector4a*			mBinormals;
	LLVector2*			mTexCoords;

	F32					mTotalDistortion;	// vertex distortion summed over entire morph
	F32					mMaxDistortion;		// maximum single vertex distortion in a given morph
	LLVector4a			mAvgDistortion;		// average vertex distortion, to infer directionality of the morph
	LLPolyMeshSharedData*	mMesh;

private:
	void freeData();
} LL_ALIGN_POSTFIX(16);


//-----------------------------------------------------------------------------
// LLPolyVertexMask()
//-----------------------------------------------------------------------------
class LLPolyVertexMask
{
public:
	LLPolyVertexMask(LLPolyMorphData* morph_data);
	LLPolyVertexMask(const LLPolyVertexMask& pOther);
	~LLPolyVertexMask();

	void generateMask(const U8 *maskData, S32 width, S32 height, S32 num_components, bool invert, LLVector4a *clothing_weights);
	F32* getMorphMaskWeights();


protected:
	F32*		mWeights;
	LLPolyMorphData *mMorphData;
	bool			mWeightsGenerated;

};

//-----------------------------------------------------------------------------
// LLPolyMorphTarget Data structs
//-----------------------------------------------------------------------------
struct LLPolyVolumeMorphInfo
{
	LLPolyVolumeMorphInfo(std::string &name, LLVector3 &scale, LLVector3 &pos)
		: mName(name), mScale(scale), mPos(pos) {};

	std::string						mName;
	LLVector3						mScale;
	LLVector3						mPos;
};

struct LLPolyVolumeMorph
{
	LLPolyVolumeMorph(LLAvatarJointCollisionVolume* volume, LLVector3 scale, LLVector3 pos)
		: mVolume(volume), mScale(scale), mPos(pos) {};

	LLAvatarJointCollisionVolume*	mVolume;
	LLVector3						mScale;
	LLVector3						mPos;
};

//-----------------------------------------------------------------------------
// LLPolyMorphTargetInfo
// Shared information for LLPolyMorphTargets
//-----------------------------------------------------------------------------
class LLPolyMorphTargetInfo : public LLViewerVisualParamInfo
{
	friend class LLPolyMorphTarget;
public:
	LLPolyMorphTargetInfo();
	/*virtual*/ ~LLPolyMorphTargetInfo() {};
	
	/*virtual*/ bool parseXml(LLXmlTreeNode* node);

protected:
	std::string		mMorphName;
	bool			mIsClothingMorph;
	typedef std::vector<LLPolyVolumeMorphInfo> volume_info_list_t;
	volume_info_list_t mVolumeInfoList;	
};

//-----------------------------------------------------------------------------
// LLPolyMorphTarget
// A set of vertex data associated with morph target.
// These morph targets must be topologically consistent with a given Polymesh
// (share face sets)
//-----------------------------------------------------------------------------
class alignas(16) LLPolyMorphTarget : public LLViewerVisualParam
{
    LL_ALIGN_NEW
public:
	LLPolyMorphTarget(LLPolyMesh *poly_mesh);
	~LLPolyMorphTarget();

	// Special: These functions are overridden by child classes
	LLPolyMorphTargetInfo*	getInfo() const { return (LLPolyMorphTargetInfo*)mInfo; }
	//   This sets mInfo and calls initialization functions
	bool					setInfo(LLPolyMorphTargetInfo *info);

	/*virtual*/ LLViewerVisualParam* cloneParam(LLWearable* wearable) const;

	// LLVisualParam Virtual functions
	///*virtual*/ bool				parseData(LLXmlTreeNode* node);
	/*virtual*/ void				apply( ESex sex );
	
	// LLViewerVisualParam Virtual functions
	/*virtual*/ F32					getTotalDistortion();
	/*virtual*/ const LLVector4a&	getAvgDistortion();
	/*virtual*/ F32					getMaxDistortion();
	/*virtual*/ LLVector4a			getVertexDistortion(S32 index, LLPolyMesh *poly_mesh);
	/*virtual*/ const LLVector4a*	getFirstDistortion(U32 *index, LLPolyMesh **poly_mesh);
	/*virtual*/ const LLVector4a*	getNextDistortion(U32 *index, LLPolyMesh **poly_mesh);

	void	applyMask(const U8 *maskData, S32 width, S32 height, S32 num_components, bool invert);
	void	addPendingMorphMask() { mNumMorphMasksPending++; }

    void    applyVolumeChanges(F32 delta_weight); // SL-315 - for resetSkeleton()

protected:
	LLPolyMorphTarget(const LLPolyMorphTarget& pOther);

	LLPolyMorphData*				mMorphData;
	LLPolyMesh*						mMesh;
	LLPolyVertexMask *				mVertMask;
	ESex							mLastSex;
	// number of morph masks that haven't been generated, must be 0 before this morph is applied
	S32								mNumMorphMasksPending;

	typedef std::vector<LLPolyVolumeMorph> volume_list_t;
	volume_list_t 					mVolumeMorphs;

};

#endif // LL_LLPOLYMORPH_H
