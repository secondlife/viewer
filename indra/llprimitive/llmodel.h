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
	
	//convex_hull_decomposition is a vector of convex hulls
	//each convex hull is a set of points
	typedef  std::vector<std::vector<LLVector3> > convex_hull_decomposition;
	typedef std::vector<LLVector3> hull;
	
	LLModel(LLVolumeParams& params, F32 detail);
	~LLModel();

	static LLSD writeModel(
		std::string filename,
		LLModel* physics,
		LLModel* high,
		LLModel* medium,
		LLModel* low,
		LLModel* imposotr,
		const LLModel::convex_hull_decomposition& convex_hull_decomposition,
		const LLModel::hull& base_hull,
		BOOL upload_skin,
		BOOL upload_joints,
		BOOL nowrite = FALSE);
	static LLSD writeModel(
		std::string filename,
		LLModel* physics,
		LLModel* high,
		LLModel* medium,
		LLModel* low,
		LLModel* imposotr,
		const LLModel::convex_hull_decomposition& convex_hull_decomposition,
		BOOL upload_skin,
		BOOL upload_joints,
		BOOL nowrite = FALSE);
	static LLSD writeModel(
		std::ostream& ostr,
		LLModel* physics,
		LLModel* high,
		LLModel* medium,
		LLModel* low,
		LLModel* imposotr,
		const LLModel::convex_hull_decomposition& convex_hull_decomposition,
		const LLModel::hull& base_hull,
		BOOL upload_skin,
		BOOL upload_joints,
		BOOL nowrite = FALSE);
	static LLSD writeModelToStream(
		std::ostream& ostr,
		LLSD& mdl,
		BOOL nowrite = FALSE);

	static LLModel* loadModelFromAsset(std::string filename, S32 lod);
	static LLModel* loadModelFromDae(std::string filename);
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

	//should always be true that mJointList[mJointMap["foo"]] == "foo"

	//map of joint names to joint index
	std::map<std::string, U32> mJointMap;

	//list of joint names
	std::vector<std::string> mJointList;

	LLMatrix4 mBindShapeMatrix;
	std::vector<LLMatrix4> mInvBindMatrix;
	std::vector<LLMatrix4> mAlternateBindMatrix;
	std::string mRequestedLabel; // name requested in UI, if any.
	std::string mLabel; // name computed from dae.

	LLVector3 mNormalizedScale;
	LLVector3 mNormalizedTranslation;

	float	mPelvisOffset;
	// convex hull decomposition
	S32 mDecompID;
	convex_hull_decomposition mConvexHullDecomp;
	void setConvexHullDecomposition(
		const convex_hull_decomposition& decomp);

	LLVector3 mCenterOfHullCenters;
	std::vector<LLVector3> mHullCenter;
	U32 mHullPoints;

protected:
	void addVolumeFacesFromDomMesh(domMesh* mesh);
	virtual BOOL createVolumeFacesFromFile(const std::string& file_name);
	virtual BOOL createVolumeFacesFromDomMesh(domMesh *mesh);
};

#endif //LL_LLMODEL_H
