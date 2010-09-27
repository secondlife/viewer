/** 
 * @file llmodel.h
 * @brief Model handling class definitions
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#ifndef LL_LLMODEL_H
#define LL_LLMODEL_H

#include "llvolume.h"
#include "v4math.h"
#include "m4math.h"

class daeElement;
class domMesh;

#define MAX_MODEL_FACES 8

class LLModel : public LLVolume
{
public:

	enum
	{
		LOD_IMPOSTOR = 0,
		LOD_LOW,
		LOD_MEDIUM,
		LOD_HIGH,
		LOD_PHYSICS,
		NUM_LODS
	};
	
	//physics shape is a vector of convex hulls
	//each convex hull is a set of points
	typedef  std::vector<std::vector<LLVector3> > physics_shape;
	
	LLModel(LLVolumeParams& params, F32 detail);
	static LLSD writeModel(std::string filename, LLModel* physics, LLModel* high, LLModel* medium, LLModel* low, LLModel* imposotr, LLModel::physics_shape& physics_shape, BOOL nowrite = FALSE);
	static LLSD writeModel(std::ostream& ostr, LLModel* physics, LLModel* high, LLModel* medium, LLModel* low, LLModel* imposotr, LLModel::physics_shape& physics_shape, BOOL nowrite = FALSE);
	static LLSD writeModelToStream(std::ostream& ostr, LLSD& mdl, BOOL nowrite = FALSE);
	static LLModel* loadModelFromAsset(std::string filename, S32 lod);
	static LLModel* loadModelFromDae(std::string filename);
	static LLModel* loadModelFromDomMesh(domMesh* mesh);
	static std::string getElementLabel(daeElement* element);

	void appendFaces(LLModel* model, LLMatrix4& transform, LLMatrix4& normal_transform);
	void appendFace(const LLVolumeFace& src_face, std::string src_material, LLMatrix4& mat, LLMatrix4& norm_mat);

	void setNumVolumeFaces(S32 count);
	void setVolumeFaceData(S32 f, 
						LLStrider<LLVector3> pos, 
						LLStrider<LLVector3> norm, 
						LLStrider<LLVector2> tc, 
						LLStrider<U16> ind, 
						U32 num_verts, 
						U32 num_indices);

	void smoothNormals(F32 angle_cutoff);

	void addFace(const LLVolumeFace& face);

	void normalizeVolumeFaces();
	void optimizeVolumeFaces();


	U32 getResourceCost();
	void getNormalizedScaleTranslation(LLVector3& scale_out, LLVector3& translation_out);
	std::vector<std::string> mMaterialList;

	//data used for skin weights
	class JointWeight
	{
	public:
		S32 mJointIdx;
		F32 mWeight;
		
		JointWeight()
		{
			mJointIdx = 0;
			mWeight = 0.f;
		}

		JointWeight(S32 idx, F32 weight)
			: mJointIdx(idx), mWeight(weight)
		{
		}

		bool operator<(const JointWeight& rhs) const
		{
			if (mWeight == rhs.mWeight)
			{
				return mJointIdx < rhs.mJointIdx;
			}

			return mWeight < rhs.mWeight;
		}

	};

	struct CompareWeightGreater
	{
		bool operator()(const JointWeight& lhs, const JointWeight& rhs)
		{
			return rhs < lhs; // strongest = first
		}
	};

	//copy of position array for this model -- mPosition[idx].mV[X,Y,Z]
	std::vector<LLVector3> mPosition;

	//map of positions to skin weights --- mSkinWeights[pos].mV[0..4] == <joint_index>.<weight>
	//joint_index corresponds to mJointList
	typedef std::vector<JointWeight> weight_list;
	typedef std::map<LLVector3, weight_list > weight_map;
	weight_map mSkinWeights;

	//get list of weight influences closest to given position
	weight_list& getJointInfluences(const LLVector3& pos);

	//should always be true that mJointList[mJointMap["foo"]] == "foo"

	//map of joint names to joint index
	std::map<std::string, U32> mJointMap;

	//list of joint names
	std::vector<std::string> mJointList;

	LLMatrix4 mBindShapeMatrix;
	std::vector<LLMatrix4> mInvBindMatrix;
	std::vector<LLMatrix4> mAlternateBindMatrix;
	std::string mLabel;

	LLVector3 mNormalizedScale;
	LLVector3 mNormalizedTranslation;

	//physics shape
	physics_shape mPhysicsShape;

	LLVector3 mPhysicsCenter;
	std::vector<LLVector3> mHullCenter;
	U32 mPhysicsPoints;

protected:
	void addVolumeFacesFromDomMesh(domMesh* mesh);
	virtual BOOL createVolumeFacesFromFile(const std::string& file_name);
	virtual BOOL createVolumeFacesFromDomMesh(domMesh *mesh);
};

#endif //LL_LLMODEL_H
