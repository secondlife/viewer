/** 
 * @file llmodel.h
 * @brief Model handling class definitions
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

#ifndef LL_LLMODEL_H
#define LL_LLMODEL_H

#include "llpointer.h"
#include "llvolume.h"
#include "v4math.h"
#include "m4math.h"

class daeElement;
class domMesh;

#define MAX_MODEL_FACES 8


class LLMeshSkinInfo 
{
public:
	LLUUID mMeshID;
	std::vector<std::string> mJointNames;
	std::vector<LLMatrix4> mInvBindMatrix;
	std::vector<LLMatrix4> mAlternateBindMatrix;
	std::map<std::string, U32> mJointMap;

	LLMeshSkinInfo() { }
	LLMeshSkinInfo(LLSD& data);
	void fromLLSD(LLSD& data);
	LLSD asLLSD(bool include_joints) const;
	LLMatrix4 mBindShapeMatrix;
	float mPelvisOffset;
};

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
	
	//convex_hull_decomposition is a vector of convex hulls
	//each convex hull is a set of points
	typedef std::vector<std::vector<LLVector3> > convex_hull_decomposition;
	typedef std::vector<LLVector3> hull;
	
	class PhysicsMesh
	{
	public:
		std::vector<LLVector3> mPositions;
		std::vector<LLVector3> mNormals;

		void clear()
		{
			mPositions.clear();
			mNormals.clear();
		}

		bool empty() const
		{
			return mPositions.empty();
		}
	};

	class Decomposition
	{
	public:
		Decomposition() { }
		Decomposition(LLSD& data);
		void fromLLSD(LLSD& data);
		LLSD asLLSD() const;

		void merge(const Decomposition* rhs);

		LLUUID mMeshID;
		LLModel::convex_hull_decomposition mHull;
		LLModel::hull mBaseHull;

		std::vector<LLModel::PhysicsMesh> mMesh;
		LLModel::PhysicsMesh mBaseHullMesh;
		LLModel::PhysicsMesh mPhysicsShapeMesh;
	};

	LLModel(LLVolumeParams& params, F32 detail);
	~LLModel();

	bool loadModel(std::istream& is);
	bool loadSkinInfo(LLSD& header, std::istream& is);
	bool loadDecomposition(LLSD& header, std::istream& is);
	
	static LLSD writeModel(
		std::ostream& ostr,
		LLModel* physics,
		LLModel* high,
		LLModel* medium,
		LLModel* low,
		LLModel* imposotr,
		const LLModel::Decomposition& decomp,
		BOOL upload_skin,
		BOOL upload_joints,
		BOOL nowrite = FALSE);

	static LLSD writeModelToStream(
		std::ostream& ostr,
		LLSD& mdl,
		BOOL nowrite = FALSE);

	static LLModel* loadModelFromDomMesh(domMesh* mesh);
	static std::string getElementLabel(daeElement* element);
	std::string getName() const;

	void appendFaces(LLModel* model, LLMatrix4& transform, LLMatrix4& normal_transform);
	void appendFace(const LLVolumeFace& src_face, std::string src_material, LLMatrix4& mat, LLMatrix4& norm_mat);

	void setNumVolumeFaces(S32 count);
	void setVolumeFaceData(
		S32 f, 
		LLStrider<LLVector3> pos, 
		LLStrider<LLVector3> norm, 
		LLStrider<LLVector2> tc, 
		LLStrider<U16> ind, 
		U32 num_verts, 
		U32 num_indices);

	void generateNormals(F32 angle_cutoff);

	void addFace(const LLVolumeFace& face);

	void normalizeVolumeFaces();
	void optimizeVolumeFaces();
	void offsetMesh( const LLVector3& pivotPoint );
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

	LLMeshSkinInfo mSkinInfo;
	
	std::string mRequestedLabel; // name requested in UI, if any.
	std::string mLabel; // name computed from dae.

	LLVector3 mNormalizedScale;
	LLVector3 mNormalizedTranslation;

	float	mPelvisOffset;
	// convex hull decomposition
	S32 mDecompID;
	
	void setConvexHullDecomposition(
		const convex_hull_decomposition& decomp);
	void updateHullCenters();

	LLVector3 mCenterOfHullCenters;
	std::vector<LLVector3> mHullCenter;
	U32 mHullPoints;

	//ID for storing this model in a .slm file
	S32 mLocalID;

	Decomposition mPhysics;

protected:
	void addVolumeFacesFromDomMesh(domMesh* mesh);
	virtual BOOL createVolumeFacesFromDomMesh(domMesh *mesh);
};

#endif //LL_LLMODEL_H
